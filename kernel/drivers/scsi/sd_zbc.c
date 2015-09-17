/*
 * sd_zbc.c - SCSI Zoned Block commands
 *
 * Copyright (C) 2014 SUSE Linux Products GmbH
 * Written by: Hannes Reinecke <hare@suse.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
 */

#include <linux/blkdev.h>
#include <linux/rbtree.h>

#include <asm/unaligned.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_driver.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_eh.h>

#include "sd.h"
#include "scsi_priv.h"

enum zbc_zone_cond {
	ZBC_ZONE_COND_NO_WP,
	ZBC_ZONE_COND_EMPTY,
	ZBC_ZONE_COND_IMPLICIT_OPEN,
	ZBC_ZONE_COND_EXPLICIT_OPEN,
	ZBC_ZONE_COND_CLOSED,
	ZBC_ZONE_COND_READONLY = 0xd,
	ZBC_ZONE_COND_FULL,
	ZBC_ZONE_COND_OFFLINE,
};

enum zbc_zone_same {
	ZBC_ZONE_SAME_NONE                  = 0,
	ZBC_ZONE_SAME_ALL                   = 1,
	ZBC_ZONE_SAME_LAST_ZONE_DIFFERS     = 2,
	ZBC_ZONE_SAME_ALL_LEN_TYPES_DIFFER  = 3,
};

#define SD_ZBC_BUF_SIZE 65536
#define SD_ZBC_QUEUE_DELAY 5

static int sd_zbc_chunk_limits(struct request_queue *, sector_t);

static int
zbc_report_zone(struct scsi_disk *sdkp, sector_t start_lba,
		unsigned char *buffer, int bufflen )
{
	struct scsi_device *sdp = sdkp->device;
	const int timeout = sdp->request_queue->rq_timeout
		* SD_FLUSH_TIMEOUT_MULTIPLIER;
	struct scsi_sense_hdr sshdr;
	unsigned char cmd[16];
	int result;

	if (!scsi_device_online(sdp)) {
		sd_printk(KERN_INFO, sdkp, "device not online\n");
		return -ENODEV;
	}

	sd_printk(KERN_INFO, sdkp, "REPORT ZONES lba %zu len %d\n",
		  start_lba, bufflen);

	memset(cmd, 0, 16);
	cmd[0] = ZBC_IN;
	cmd[1] = ZI_REPORT_ZONES;
	put_unaligned_be64(start_lba, &cmd[2]);
	put_unaligned_be32(bufflen, &cmd[10]);
	memset(buffer, 0, bufflen);

	result = scsi_execute_req(sdp, cmd, DMA_FROM_DEVICE,
				  buffer, bufflen, &sshdr,
				  timeout, SD_MAX_RETRIES, NULL);

	if (result) {
		sd_printk(KERN_NOTICE, sdkp,
			  "REPORT ZONES lba %zu failed with %d/%d\n",
			  start_lba, host_byte(result), driver_byte(result));
		return -EINVAL;
	}
	return 0;
}

struct blk_zone *zbc_desc_to_zone(struct scsi_disk *sdkp, unsigned char *rec,
				  int bflags)
{
	struct blk_zone *zone;
	enum zbc_zone_cond zone_cond;
	u64 wp = (u64)-1;

	zone = kzalloc(sizeof(struct blk_zone), GFP_KERNEL);
	if (!zone)
		return NULL;

	spin_lock_init(&zone->lock);
	zone->type = rec[0] & 0xf;
	zone_cond = (rec[1] >> 4) & 0xf;
	zone->len = get_unaligned_be64(&rec[8]);
	zone->start = get_unaligned_be64(&rec[16]);

	if (blk_zone_is_smr(zone)) {
		wp = get_unaligned_be64(&rec[24]);
		if (zone_cond == ZBC_ZONE_COND_READONLY) {
			zone->state = BLK_ZONE_READONLY;
		} else if (zone_cond == ZBC_ZONE_COND_OFFLINE) {
			zone->state = BLK_ZONE_OFFLINE;
		} else {
			zone->state = BLK_ZONE_OPEN;
		}
	} else
		zone->state = BLK_ZONE_NO_WP;

	zone->wp = zone->shadow_wp = wp;
	/*
	 * Fixup block zone state
	 */
	if (zone_cond == ZBC_ZONE_COND_EMPTY &&
	    zone->wp != zone->start) {
		sd_printk(KERN_INFO, sdkp,
			  "zone %llu state EMPTY wp %llu: adjust wp\n",
			  zone->start, zone->wp);
		zone->wp = zone->start;
	}
	if (zone_cond == ZBC_ZONE_COND_FULL &&
	    zone->wp != zone->start + zone->len) {
		sd_printk(KERN_INFO, sdkp,
			  "zone %llu state FULL wp %llu: adjust wp\n",
			  zone->start, zone->wp);
		zone->wp = zone->start + zone->len;
	}

	return zone;
}

sector_t zbc_parse_zones(struct scsi_disk *sdkp, unsigned char *buf,
			 unsigned int *buf_len, sector_t start_lba, int bflags)
{
	struct request_queue *q = sdkp->disk->queue;
	unsigned char *rec = buf;
	int rec_no = 0, start_rec = 0;
	unsigned int list_length, zone_len = q->zone_len;
	sector_t last_lba = 0;
	u8 same;

	/* Parse REPORT ZONES header */
	list_length = get_unaligned_be32(&buf[0]);
	same = buf[4] & 0xf;
	if (same > 0 && zone_len)
		rec_no = start_rec = start_lba / zone_len;
	rec = buf + 64;
	list_length += 64;

	if (list_length < *buf_len)
		*buf_len = list_length;

	while (rec < buf + *buf_len) {
		struct blk_zone *this, *old;
		unsigned long flags;

		this = zbc_desc_to_zone(sdkp, rec, bflags);
		if (!this)
			break;

		if (last_lba <= this->start)
			last_lba = this->start + this->len;
		old = blk_insert_zone(q, this);
		if (old) {
			spin_lock_irqsave(&old->lock, flags);
			if (old->state == BLK_ZONE_BUSY ||
			    old->state == BLK_ZONE_UNKNOWN) {
				old->wp = this->wp;
				old->shadow_wp = this->shadow_wp;
				old->state = this->state;
			}
			spin_unlock_irqrestore(&old->lock, flags);
			kfree(this);
		} else if (blk_zone_is_smr(this)) {
			if (!zone_len || this->len > zone_len)
				zone_len = this->len;
		}
		rec += 64;
		rec_no++;
	}
	if (!q->zone_len && zone_len)
		q->zone_len = zone_len;

	if (start_lba == 0) {
		sd_printk(KERN_INFO, sdkp,
			  "Setting queue limits same %d len %zu\n",
			  same, q->zone_len);
		if (   (ZBC_ZONE_SAME_ALL == same) 
		    || (ZBC_ZONE_SAME_ALL_LEN_TYPES_DIFFER == same) ) {
			/* Zone sizes are identical */
			sdkp->unmap_granularity = q->zone_len;
			sdkp->max_ws_blocks = q->zone_len;
			blk_queue_chunk_sectors(sdkp->disk->queue, q->zone_len);
		} else {
			/* One or more zones differ byte size */
			blk_queue_chunk_limits(sdkp->disk->queue,
					       sd_zbc_chunk_limits);
		}
		sd_config_discard(sdkp, SD_ZBC_RESET_WP);
	}
	sd_printk(KERN_INFO, sdkp,
		  "Inserted %d zones (%d - %d), next lba %zu len %d\n",
		  rec_no, start_rec, rec_no, last_lba, list_length);
	if (*buf_len < list_length) {
		*buf_len = list_length;
		return last_lba;
	}
	return 0;
}

static void sd_zbc_refresh_wp(struct scsi_disk *sdkp, sector_t wp)
{
	struct request_queue *q = sdkp->disk->queue;
	struct blk_zone *zone;
	unsigned long flags;

	if (sdkp->zone_buf) {
		/* zone update in progress */
		sd_printk(KERN_INFO, sdkp,
			  "zone update in progress\n");
		return;
	}
	zone = blk_lookup_zone(q, wp);
	if (!zone)
		return;

	spin_lock_irqsave(&zone->lock, flags);
	if (blk_zone_is_cmr(zone))
		goto out;
	if (zone->state == BLK_ZONE_BUSY) {
		sd_printk(KERN_INFO, sdkp,
			  "zone busy, not updating wp");
		goto out;
	}

	zone->wp = zone->shadow_wp = wp;
	zone->state = BLK_ZONE_OPEN;
out:
	spin_unlock_irqrestore(&zone->lock, flags);
}

void sd_zbc_refresh_zone_work(struct work_struct *work)
{
	struct scsi_disk *sdkp =
		container_of(work, struct scsi_disk, zone_work);
	struct request_queue *q = sdkp->disk->queue;
	unsigned long flags;
	unsigned int zone_buflen;
	void *zone_buf;
	int bflags, ret;
	sector_t last_lba, zone_lba, zone_wp;

	spin_lock_irqsave(&sdkp->zone_lock, flags);
	if (sdkp->zone_wp != -1) {
		zone_wp = sdkp->zone_wp;
		sdkp->zone_wp = (sector_t)-1;
		sd_zbc_refresh_wp(sdkp, zone_wp);
	}
	if (!sdkp->zone_buf) {
		sd_printk(KERN_INFO, sdkp,
			  "zone update sector %zu cancelled\n",
			  sdkp->zone_lba);
		spin_unlock_irqrestore(&sdkp->zone_lock, flags);
		goto done_start_queue;
	}
	zone_lba = sdkp->zone_lba;
	zone_buf = sdkp->zone_buf;
	zone_buflen = sdkp->zone_buflen;
	spin_unlock_irqrestore(&sdkp->zone_lock, flags);

	bflags = scsi_get_device_flags_keyed(sdkp->device,
					     &sdkp->device->inquiry[8],
					     &sdkp->device->inquiry[16],
					     SCSI_DEVINFO_GLOBAL);

	ret = zbc_report_zone(sdkp, zone_lba,
			      zone_buf, zone_buflen);
	if (ret)
		goto done_free;

	last_lba = zbc_parse_zones(sdkp, sdkp->zone_buf, &zone_buflen,
				   zone_lba, bflags);
	if (last_lba && !sdkp->zone_update) {
		spin_lock_irqsave(&sdkp->zone_lock, flags);
		sdkp->zone_lba = last_lba;
		spin_unlock_irqrestore(&sdkp->zone_lock, flags);
		queue_work(sdkp->zone_work_q, &sdkp->zone_work);
		/* Kick request queue to be on the safe side */
		goto done_start_queue;
	}
done_free:
	spin_lock_irqsave(&sdkp->zone_lock, flags);
	kfree(sdkp->zone_buf);
	sdkp->zone_buf = NULL;
	spin_unlock_irqrestore(&sdkp->zone_lock, flags);
done_start_queue:
	spin_lock_irqsave(q->queue_lock, flags);
	blk_start_queue(q);
	spin_unlock_irqrestore(q->queue_lock, flags);
}

void sd_zbc_update_zones(struct scsi_disk *sdkp, sector_t lba, int bufsize,
			 bool update)
{
	struct request_queue *q = sdkp->disk->queue;
	void *zone_buf;
	struct blk_zone *zone;
	unsigned long flags;

retry:
	zone_buf = kzalloc(bufsize, GFP_KERNEL);
	if (!zone_buf) {
		if (bufsize > 512) {
			sd_printk(KERN_INFO, sdkp,
				  "retry with buffer size %d\n", bufsize);
			bufsize = bufsize >> 1;
			goto retry;
		}
		sd_printk(KERN_INFO, sdkp,
			  "failed to allocate %d bytes\n", bufsize);
		return;
	}
	spin_lock_irqsave(&sdkp->zone_lock, flags);
	if (sdkp->zone_buf) {
		spin_unlock_irqrestore(&sdkp->zone_lock, flags);
		kfree(zone_buf);
		/* zone update in progress */
		sd_printk(KERN_INFO, sdkp,
			  "zone update in progress\n");
		return;
	}
	sdkp->zone_buf = zone_buf;
	sdkp->zone_lba = lba;
	sdkp->zone_buflen = bufsize;
	sdkp->zone_update = update;
	spin_unlock_irqrestore(&sdkp->zone_lock, flags);

	if (!update) {
		struct blk_zone *next;

		rbtree_postorder_for_each_entry_safe(zone, next,
						     &q->zones, node) {
			unsigned long flags;

			if (zone->start + zone->len <= lba)
				continue;

			spin_lock_irqsave(&zone->lock, flags);
			if (blk_zone_is_smr(zone))
				zone->state = BLK_ZONE_BUSY;
			spin_unlock_irqrestore(&zone->lock, flags);
		}
	}

	if (!queue_work(sdkp->zone_work_q, &sdkp->zone_work)) {
		sd_printk(KERN_INFO, sdkp,
			  "zone update already queued?\n");
	}
}

void sd_zbc_update_wp(struct scsi_disk *sdkp, sector_t lba, sector_t wp)
{
	unsigned long flags;

	spin_lock_irqsave(&sdkp->zone_lock, flags);
	if (sdkp->zone_wp != -1) {
		sd_printk(KERN_INFO, sdkp,
			  "WP update in progress, wp %zu / %zu\n",
			  wp, sdkp->zone_wp);
		spin_unlock_irqrestore(&sdkp->zone_lock, flags);
		return;
	}
	sdkp->zone_wp = wp;
	spin_unlock_irqrestore(&sdkp->zone_lock, flags);
	if (!queue_work(sdkp->zone_work_q, &sdkp->zone_work))
		sd_printk(KERN_INFO, sdkp,
			  "zone update already queued for wp update\n");
}

int sd_zbc_lookup_zone(struct scsi_disk *sdkp, struct request *rq,
		       sector_t sector, unsigned int num_sectors)
{
	struct request_queue *q = sdkp->disk->queue;
	struct blk_zone *zone = NULL;
	int ret = BLKPREP_OK;
	unsigned long flags;

	/* Simple last-hit cache to save rbtree lookups */
	if (q->zone_cache && q->zone_cache->start < sector &&
	    q->zone_cache->start + q->zone_cache->len > sector) {
		zone = q->zone_cache;
#if 0
		if (blk_zone_is_smr(zone))
			sd_printk(KERN_INFO, sdkp,
				  "SMR zone %x %llu/%llu %lld (cached)\n",
				  zone->state, zone->start,
				  zone->len, zone->wp);
#endif
	} else {
		zone = blk_lookup_zone(q, sector);
#if 0
		if (zone && blk_zone_is_smr(zone))
			sd_printk(KERN_INFO, sdkp,
				  "SMR zone %x %llu/%llu %lld\n",
				  zone->state, zone->start,
				  zone->len, zone->wp);
#endif
		q->zone_cache = zone;
	}
	/* Might happen during zone initialization */
	if (!zone) {
		if (printk_ratelimit())
			sd_printk(KERN_INFO, sdkp,
				  "zone for sector %zu not found, %s\n",
				  sector, sdkp->device->type == TYPE_ZBC ?
				  "deferring" : "skipping");
		if (sdkp->device->type != TYPE_ZBC)
			return BLKPREP_OK;
		blk_delay_queue(q, 5);
		return BLKPREP_DEFER;
	}
	spin_lock_irqsave(&zone->lock, flags);
	if (zone->state == BLK_ZONE_UNKNOWN ||
	    zone->state == BLK_ZONE_BUSY) {
		if (printk_ratelimit())
			sd_printk(KERN_INFO, sdkp,
				  "zone %llu state %x, deferring\n",
				  zone->start, zone->state);
		q->zone_cache = NULL;
		blk_delay_queue(q, 5);
		ret = BLKPREP_DEFER;
	} else if (zone->type == BLK_ZONE_TYPE_SEQWRITE_REQ) {
		if (rq_data_dir(rq) == WRITE) {
			if (rq->cmd_flags & REQ_WPUPDATE) {
				if (zone->wp < sector &&
				    zone->shadow_wp > zone->wp) {
					sd_printk(KERN_ERR, sdkp,
						  "Non-sequential write "
						  "%llu/%zu/%llu, requeue\n",
						  zone->wp, sector,
						  zone->shadow_wp);
					rq->cmd_flags &= ~REQ_WPUPDATE;
					ret = BLKPREP_REQUEUE;
					goto out;
				}
			}
			
			if (blk_zone_is_full(zone)) {
				sd_printk(KERN_ERR, sdkp,
					  "Write to full zone %zu/%llu\n",
					  sector, zone->wp);
				ret = BLKPREP_KILL;
				goto out;
			}
			if (zone->wp != sector) {
				sd_printk(KERN_ERR, sdkp,
					  "Misaligned write %zu/%llu\n",
					  sector, zone->wp);
				ret = BLKPREP_KILL;
				goto out;
			}
			zone->wp += num_sectors;
		} else if (zone->wp <= sector) {
			sd_printk(KERN_INFO, sdkp,
				    "Read beyond wp %zu/%llu\n",
				    sector, zone->wp);
			ret = BLKPREP_DONE;
		}
	}
out:
	spin_unlock_irqrestore(&zone->lock, flags);

	return ret;
}

static int sd_zbc_chunk_limits(struct request_queue *q, sector_t sector)
{
	struct blk_zone *zone = NULL;

	/* Simple last-hit cache to save rbtree lookups */
	if (q->zone_cache && q->zone_cache->start < sector &&
	    q->zone_cache->start + q->zone_cache->len > sector)
		zone = q->zone_cache;
	else {
		zone = blk_lookup_zone(q, sector);
		q->zone_cache = zone;
	}

	if (!zone)
		return q->limits.max_sectors;

	return zone->start + zone->len - sector;
}

void sd_zbc_reset_zones(struct scsi_disk *sdkp)
{
	struct request_queue *q = sdkp->disk->queue;
	struct rb_root *root = &q->zones;
	struct blk_zone *zone, *next;
	unsigned long flags;

	rbtree_postorder_for_each_entry_safe(zone, next, root, node) {
		spin_lock_irqsave(&zone->lock, flags);
		zone->wp = zone->shadow_wp = -1;
		zone->state = BLK_ZONE_UNKNOWN;
		spin_unlock_irqrestore(&zone->lock, flags);
	}
}

int sd_zbc_init_zones(struct scsi_disk *sdkp, unsigned char *buf, int buf_len)
{
	int ret;
	int bflags;
	unsigned long flags;

	cancel_work_sync(&sdkp->zone_work);
	spin_lock_irqsave(&sdkp->zone_lock, flags);
	if (sdkp->zone_buf) {
		kfree(sdkp->zone_buf);
		sdkp->zone_buf = NULL;
	}
	sdkp->zone_wp = -1;
	spin_unlock_irqrestore(&sdkp->zone_lock, flags);
	sd_zbc_reset_zones(sdkp);

	bflags = scsi_get_device_flags_keyed(sdkp->device,
					     &sdkp->device->inquiry[8],
					     &sdkp->device->inquiry[16],
					     SCSI_DEVINFO_GLOBAL);

	ret = zbc_report_zone(sdkp, 0, buf, buf_len);
	if (!ret) {
		sector_t last_lba;
		unsigned int zbc_buf_len = buf_len;

		last_lba = zbc_parse_zones(sdkp, buf, &zbc_buf_len, 0, bflags);
		if (last_lba) {
			/* ZAC can only handle 512-byte transfers */
			if (zbc_buf_len & 0x1ff)
				zbc_buf_len = (zbc_buf_len / 2) & ~0x1ff;
			sd_zbc_update_zones(sdkp, last_lba,
					    zbc_buf_len, false);
		}
	}

	blk_queue_io_min(sdkp->disk->queue, 4);
	return 0;
}

void sd_zbc_remove_zones(struct scsi_disk *sdkp)
{
	struct request_queue *q = sdkp->disk->queue;
	unsigned long flags;

	cancel_work_sync(&sdkp->zone_work);
	spin_lock_irqsave(&sdkp->zone_lock, flags);
	if (sdkp->zone_buf) {
		kfree(sdkp->zone_buf);
		sdkp->zone_buf = NULL;
	}
	sdkp->zone_wp = -1;
	spin_unlock_irqrestore(&sdkp->zone_lock, flags);
	q->zone_cache = NULL;
	sd_printk(KERN_INFO, sdkp,
		  "Drop zone information\n");
}

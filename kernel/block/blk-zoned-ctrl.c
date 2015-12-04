/*
 * Functions for zone based SMR devices.
 *
 * Copyright (C) 2015 Seagate Technology PLC
 *
 * Written by:
 * Shaun Tancheff <shaun.tancheff@seagate.com>
 * XiaoDong Han <xiaodong.h.han@seagate.com>
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/sched/sysctl.h>

#include "blk.h"
#include <linux/blk-zoned-ctrl.h>
#include <linux/ata.h>

/*
 * for max sense size
 */
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_dbg.h>
#include <linux/ata.h>

#define ZBC_TIMEOUT		   (30 * HZ)
#define ZBC_MAX_RETRIES		     5
#define CMD_LEN			    16
#define INQUIRY_CMDLEN		     6

/**
 * ATA pass through 16.
 */
#define ZAC_PASS_THROUGH16_OPCODE  ATA_16
#define ZAC_PASS_THROUGH16_CDB_LEN 16

/**
 * ATA commands.
 */
#define ZAC_ATA_OPCODE_IDENTIFY    ATA_CMD_ID_ATA

/**
 * zac/zbc common sub command/action codes
 */
#define ZONE_CLOSE      ZO_CLOSE_ZONE
#define ZONE_FINISH     ZO_FINISH_ZONE
#define ZONE_OPEN       ZO_OPEN_ZONE
#define ZONE_RESET_WP   ZO_RESET_WRITE_POINTER

/**
 * ZBC zone command
 */
#define ZBC_ZONE_ACTION  ZBC_OUT /* 0x94 */

/**
 * Zoned block device support (for SMR drives)
 */
static inline void _len_to_cmd_zbc(u8 *cmd, u32 _len)
{
	u32 len = cpu_to_be32(_len);

	memcpy(cmd, &len, sizeof(len));
}

static inline void _lba_to_cmd_zbc(u8 *cmd, u64 _lba)
{
	u64 lba = cpu_to_be64(_lba);

	memcpy(cmd, &lba, sizeof(lba));
}

static inline u16 zc_get_word(u8 *buf)
{
	u16 w = buf[1];

	w <<= 8;
	w |= buf[0];
	return w;
}

static void pr_debug_scsi_cmd(const char *func_name, u8 *cmd)
{
#define FMT_LBA "lba:%02x%02x%02x%02x%02x%02x%02x%02x "
#define FMT_LEN "len:%02x%02x%02x%02x"

	pr_debug("%s: %02x:%02x " FMT_LBA FMT_LEN " %02x %02x\n",
		func_name, cmd[0],  cmd[1],
		cmd[2],  cmd[3],  cmd[4],  cmd[5],
		cmd[6],  cmd[7],  cmd[8],  cmd[9],
		cmd[10], cmd[11], cmd[12], cmd[13],
		cmd[14], cmd[15]);

}

/* NOTE: this is basically scsi_execute */
int blk_cmd_execute(struct request_queue *queue,
			   const unsigned char *cmd,
			   int data_direction,
			   void *buffer,
			   unsigned bufflen,
			   unsigned char *sense,
			   int timeout,
			   int retries,
			   u64 flags,
			   int *resid)
{
	struct request *req;
	int write = (data_direction == DMA_TO_DEVICE);
	int ret = DRIVER_ERROR << 24;

	req = blk_get_request(queue, write, __GFP_WAIT);
	if (IS_ERR(req))
		return ret;
	blk_rq_set_block_pc(req);

	if (bufflen &&  blk_rq_map_kern(queue, req,
					buffer, bufflen, __GFP_WAIT))
		goto out;

	req->cmd_len = COMMAND_SIZE(cmd[0]);
	memcpy(req->cmd, cmd, req->cmd_len);
	req->sense = sense;
	req->sense_len = 0;
	req->retries = retries;
	req->timeout = timeout;
	req->cmd_flags |= flags | REQ_QUIET | REQ_PREEMPT;

	/*
	 * head injection *required* here otherwise quiesce won't work
	 */
	blk_execute_rq(req->q, NULL, req, 1);

	/*
	 * Some devices (USB mass-storage in particular) may transfer
	 * garbage data together with a residue indicating that the data
	 * is invalid.  Prevent the garbage from being misinterpreted
	 * and prevent security leaks by zeroing out the excess data.
	 */
	if (unlikely(req->resid_len > 0 && req->resid_len <= bufflen))
		memset(buffer + (bufflen - req->resid_len), 0, req->resid_len);

	if (resid)
		*resid = req->resid_len;
	ret = req->errors;
 out:
	blk_put_request(req);

	return ret;
}
EXPORT_SYMBOL(blk_cmd_execute);

int blk_cmd_with_sense(struct gendisk *disk,
	u8 *cmd, int data_direction,
	void *buf, u32 buf_len, u8 *sense_buffer)
{
	struct request_queue *queue = disk->queue;
	int rc;
	struct scsi_sense_hdr sshdr = { 0 };

	if (!sense_buffer) {
		pr_err("scsi cmd exec: sense buffer is NULL\n");
		return -1;
	}

	rc = blk_cmd_execute(queue, cmd, data_direction, buf, buf_len,
		sense_buffer, ZBC_TIMEOUT, ZBC_MAX_RETRIES, 0, NULL);

	pr_debug("%s: %s -> 0x%08x [h:%02x d:%02x m:%02x s:%02x]\n", __func__,
		 disk->disk_name, rc, host_byte(rc), driver_byte(rc),
		 msg_byte(rc), status_byte(rc));

	scsi_normalize_sense(sense_buffer, SCSI_SENSE_BUFFERSIZE, &sshdr);
	if (host_byte(rc)
	    || (driver_byte(rc) && (driver_byte(rc) != DRIVER_SENSE))
	    || (status_byte(rc)	&& (status_byte(rc) != CHECK_CONDITION))) {
		pr_err("exec scsi cmd failed,opcode:%d\n", cmd[0]);
		if (driver_byte(rc) & DRIVER_SENSE)
			pr_err("%s: %s", __func__, disk->disk_name);

		return -1;
	} else if ((driver_byte(rc) == DRIVER_SENSE)
		    && ((cmd[0] == ATA_16) || (cmd[0] == ATA_12))) {
		if (sense_buffer[21] != 0x50) {
			pr_err("%s: ATA pass through command failed\n",
				__func__);
			return -1;
		}
	} else if (rc) {
		if ((driver_byte(rc) == DRIVER_SENSE)
		    && (status_byte(rc) == CHECK_CONDITION)
		    && (0 != sense_buffer[0])) {
			pr_err("%s: Something else failed\n", __func__);
			return -1;
		}
	}

	return 0;
}
EXPORT_SYMBOL(blk_cmd_with_sense);

int blk_zoned_report(struct gendisk *disk,
			u64 start_lba,
			u8 opt,
			struct bdev_zone_report *buf,
			size_t bufsz)
{
	int ret = 0;
	u8 cmd[CMD_LEN] = {0};
	u8 sense_buf[SCSI_SENSE_BUFFERSIZE] = {0};

	cmd[0] = ZBC_IN;
	cmd[1] = ZI_REPORT_ZONES;

	_lba_to_cmd_zbc(&cmd[2],  start_lba);
	_len_to_cmd_zbc(&cmd[10], (u32)bufsz);

	cmd[14] = opt;

	pr_debug_scsi_cmd(__func__, cmd);
	ret = blk_cmd_with_sense(disk, cmd, DMA_FROM_DEVICE, buf, bufsz,
					&sense_buf[0]);
	return ret;
}
EXPORT_SYMBOL(blk_zoned_report);

int blk_zoned_inquiry(struct gendisk *disk, u8 evpd, u8 pg_op,
	u16 mx_resp_len, u8 *buf)
{
	int ret = 0;
	u8 cmd[INQUIRY_CMDLEN] = {0};
	u8 sense_buf[SCSI_SENSE_BUFFERSIZE] = {0};

	__be16 slen = cpu_to_be16(mx_resp_len);

	if (0xb1 != pg_op) {
		pr_err("Unsupported Page Code %02x. Expected 0xb1\n", pg_op);
		return -1;
	}

	cmd[0] = INQUIRY;
	if (evpd)
		cmd[1] |= 1;
	cmd[2] = pg_op;
	cmd[3] = slen & 0xff;
	cmd[4] = (slen >> 8) & 0xff;

	pr_debug("%s: cmd: %02x:%02x:%02x:%02x:%02x:%02x\n",
		__func__, cmd[0],  cmd[1], cmd[2],  cmd[3],  cmd[4],  cmd[5]);

	ret = blk_cmd_with_sense(disk, cmd, DMA_FROM_DEVICE,
					buf, mx_resp_len, &sense_buf[0]);
	if (ret != 0) {
		pr_err("%s: inquiry failed\n", disk->disk_name);
		goto out;
	}

out:
	return ret;
}
EXPORT_SYMBOL(blk_zoned_inquiry);

static int blk_zoned_cmd(struct gendisk *disk, u64 start_lba, u8 command)
{
	int ret = 0;
	u8 cmd[CMD_LEN] = {0};
	u8 sense_buf[SCSI_SENSE_BUFFERSIZE] = {0};
	u8 all_bit = 0;

	pr_debug("zoned cmd (%x): on %s, start_lba %lld\n",
		command, disk->disk_name, start_lba);

	if (start_lba == ~0ul) {
		all_bit = 1;
		start_lba = 0;
	}

	cmd[0] = ZBC_ZONE_ACTION;
	cmd[1] = command;

	_lba_to_cmd_zbc(&cmd[2], start_lba);

	cmd[14] = all_bit;
	pr_debug_scsi_cmd(__func__, cmd);

	ret = blk_cmd_with_sense(disk, cmd, DMA_FROM_DEVICE, NULL, 0,
		&sense_buf[0]);
	if (ret != 0) {
		pr_err("%s: zone command %d failed\n",
			disk->disk_name, command);
		return -1;
	}
	return ret;
}

int blk_zoned_close(struct gendisk *disk, u64 start_lba)
{
	return blk_zoned_cmd(disk, start_lba, ZONE_CLOSE);
}
EXPORT_SYMBOL(blk_zoned_close);

int blk_zoned_finish(struct gendisk *disk, u64 start_lba)
{
	return blk_zoned_cmd(disk, start_lba, ZONE_FINISH);
}
EXPORT_SYMBOL(blk_zoned_finish);

int blk_zoned_open(struct gendisk *disk, u64 start_lba)
{
	return blk_zoned_cmd(disk, start_lba, ZONE_OPEN);
}
EXPORT_SYMBOL(blk_zoned_open);

int blk_zoned_reset_wp(struct gendisk *disk, u64 start_lba)
{
	return blk_zoned_cmd(disk, start_lba, ZONE_RESET_WP);
}
EXPORT_SYMBOL(blk_zoned_reset_wp);

static inline void _lba_to_cmd_ata(u8 *cmd, u64 _lba)
{
	cmd[1] =  _lba	      & 0xff;
	cmd[3] = (_lba >>  8) & 0xff;
	cmd[5] = (_lba >> 16) & 0xff;
	cmd[0] = (_lba >> 24) & 0xff;
	cmd[2] = (_lba >> 32) & 0xff;
	cmd[4] = (_lba >> 40) & 0xff;
}

/*
 * ata-16 passthrough byte 1:
 *   multiple [bits 7:5]
 *   protocol [bits 4:1]
 *   ext      [bit    0]
 */
static inline u8 ata16byte1(u8 multiple, u8 protocol, u8 ext)
{
	return ((multiple & 0x7) << 5) | ((protocol & 0xF) << 1) | (ext & 0x01);
}

int blk_zoned_report_ata(struct gendisk *disk, u64 start_lba, u8 opt,
			 struct bdev_zone_report *buf, size_t bufsz)
{
	int ret = 0;
	u8 cmd[ZAC_PASS_THROUGH16_CDB_LEN] = { 0 };
	u8 sense_buf[SCSI_SENSE_BUFFERSIZE] = { 0 };

	cmd[0] = ZAC_PASS_THROUGH16_OPCODE;
	cmd[1] = ata16byte1(0, ATAPI_PROT_PIO, 1);
	cmd[2] = 0x0e;

	cmd[4] = ATA_SUBCMD_ZAC_MGMT_IN_REPORT_ZONES;
	cmd[3] = opt;

	cmd[5] = (bufsz / 512) >> 8;
	cmd[6] = (bufsz / 512) & 0xff;

	_lba_to_cmd_ata(&cmd[7], start_lba);

	cmd[13] = 1 << 6;
	cmd[14] = ATA_CMD_ZAC_MGMT_IN;

	pr_debug_scsi_cmd(__func__, cmd);

	ret = blk_cmd_with_sense(disk, cmd, DMA_FROM_DEVICE, buf, bufsz,
					&sense_buf[0]);
	return ret;
}
EXPORT_SYMBOL(blk_zoned_report_ata);

int blk_zoned_identify_ata(struct gendisk *disk, struct zoned_identity *ident)
{
	int ret = 0;
	u8 identify_cmd[ZAC_PASS_THROUGH16_CDB_LEN] = { 0 };
	u8 sense_buf[SCSI_SENSE_BUFFERSIZE] = { 0 };
	u8 buffer[512] = { 0 };
	int flag = 0;

	ident->type_id = NOT_ZONED;

	if (NULL == disk)
		return -1;

	identify_cmd[0] = ZAC_PASS_THROUGH16_OPCODE;
	identify_cmd[1] = ata16byte1(0, ATA_PROT_NCQ, 1);
	identify_cmd[2] = 0xe;
	identify_cmd[6] = 0x1;
	identify_cmd[8] = 0x1;
	identify_cmd[14] = ZAC_ATA_OPCODE_IDENTIFY;

	ret = blk_cmd_with_sense(disk, identify_cmd, DMA_FROM_DEVICE,
				 buffer, sizeof(buffer), &sense_buf[0]);
	if (ret != 0) {
		pr_err("%s: identify failed.\n", disk->disk_name);
		goto out;
	}

	flag = zc_get_word(&buffer[138]);
	if ((flag & 0x3) == 0x1) {
		ident->type_id = HOST_AWARE;
		pr_debug("%s: is SMR-HostAware\n", disk->disk_name);
	} else {
		ret = -1;
		pr_debug("%s: not HostAware\n", disk->disk_name);
	}

out:
	return ret;
}
EXPORT_SYMBOL(blk_zoned_identify_ata);

static int _blk_zoned_command_ata(struct gendisk *disk, u64 start_lba,
				  u8 sub_command)
{
	int ret = 0;
	u8 cmd[ZAC_PASS_THROUGH16_CDB_LEN] = { 0 };
	u8 sense_buf[SCSI_SENSE_BUFFERSIZE] = { 0 };

	pr_debug("zoned command (%x): %s, start_lba %llx\n",
		 sub_command, disk->disk_name, start_lba);

	cmd[0] = ZAC_PASS_THROUGH16_OPCODE;
	cmd[1] = ata16byte1(0, ATA_PROT_DMA, 1);
	cmd[4] = sub_command; /* [feature] */

	if (start_lba == ~0ul || start_lba == ~1ul)
		cmd[3] = 0x1; /* [hob_feature] apply command to all zones */
	else
		_lba_to_cmd_ata(&cmd[7], start_lba);

	cmd[13] = 1 << 6;
	cmd[14] = ATA_CMD_ZAC_MGMT_OUT;

	pr_debug_scsi_cmd(__func__, cmd);

	ret = blk_cmd_with_sense(disk, cmd, DMA_NONE, NULL, 0, &sense_buf[0]);
	if (ret != 0) {
		pr_err("%s: command %d failed\n", disk->disk_name, sub_command);
		return -1;
	}
	return ret;
}

int blk_zoned_close_ata(struct gendisk *disk, u64 start_lba)
{
	return _blk_zoned_command_ata(disk, start_lba, ZONE_CLOSE);
}
EXPORT_SYMBOL(blk_zoned_close_ata);

int blk_zoned_finish_ata(struct gendisk *disk, u64 start_lba)
{
	return _blk_zoned_command_ata(disk, start_lba, ZONE_FINISH);
}
EXPORT_SYMBOL(blk_zoned_finish_ata);

int blk_zoned_open_ata(struct gendisk *disk, u64 start_lba)
{
	return _blk_zoned_command_ata(disk, start_lba, ZONE_OPEN);
}
EXPORT_SYMBOL(blk_zoned_open_ata);

int blk_zoned_reset_wp_ata(struct gendisk *disk, u64 start_lba)
{
	return _blk_zoned_command_ata(disk, start_lba, ZONE_RESET_WP);
}
EXPORT_SYMBOL(blk_zoned_reset_wp_ata);

int _inquiry_ioctl(struct gendisk *disk, void __user *parg)
{
	int error = 0;
	size_t result_size = 0;
	size_t alloc_size = PAGE_SIZE;
	struct zoned_inquiry *inq = kmalloc(alloc_size, GFP_KERNEL);
	u8 extended;

	if (!inq) {
		error = -ENOMEM;
		goto out;
	}
	if (copy_from_user(inq, parg, sizeof(*inq))) {
		error = -EFAULT;
		goto out;
	}
	result_size = inq->mx_resp_len + offsetof(struct zoned_inquiry, result);
	if (result_size > alloc_size) {
		void *tmp;

		alloc_size = result_size;
		tmp = krealloc(inq, alloc_size, GFP_KERNEL);
		if (!tmp) {
			error = -ENOMEM;
			goto out;
		}
		inq = tmp;
	}

	extended = inq->evpd & 0x7f;
	if (inq->evpd & ZOPT_USE_ATA_PASS) {
		struct zoned_identity ident;

		pr_debug("%s: using ata passthrough.\n", __func__);
		error = blk_zoned_identify_ata(disk, &ident);
		inq->result[8] = ident.type_id << 4;
	} else {
		error = blk_zoned_inquiry(disk, extended,   inq->pg_op,
					  inq->mx_resp_len, inq->result);
	}
	if (error) {
		error = -EFAULT;
		goto out;
	}
	if (copy_to_user(parg, inq, result_size)) {
		error = -EFAULT;
		goto out;
	}

out:
	kfree(inq);

	return error;
}
EXPORT_SYMBOL(_inquiry_ioctl);

int _zone_close_ioctl(struct gendisk *disk, unsigned long arg)
{
	int error = -EFAULT;

	if (arg & 1) {
		if (arg != ~0ul)
			arg &= ~1ul; /* ~1 :: 0xFF...FE */
		error = blk_zoned_close_ata(disk, arg);
	} else {
		if (arg == ~1ul)
			arg = ~0ul;
		error = blk_zoned_close(disk, arg);
	}

	return error;
}
EXPORT_SYMBOL(_zone_close_ioctl);

int _zone_finish_ioctl(struct gendisk *disk, unsigned long arg)
{
	int error = -EFAULT;

	if (arg & 1) {
		if (arg != ~0ul)
			arg &= ~1ul; /* ~1 :: 0xFF...FE */
		error = blk_zoned_finish_ata(disk, arg);
	} else {
		if (arg == ~1ul)
			arg = ~0ul;
		error = blk_zoned_finish(disk, arg);
	}

	return error;
}
EXPORT_SYMBOL(_zone_finish_ioctl);

int _zone_open_ioctl(struct gendisk *disk, unsigned long arg)
{
	int error = -EFAULT;

	if (arg & 1) {
		if (arg != ~0ul)
			arg &= ~1ul; /* ~1 :: 0xFF...FE */
		error = blk_zoned_open_ata(disk, arg);
	} else {
		if (arg == ~1ul)
			arg = ~0ul;
		error = blk_zoned_open(disk, arg);
	}

	return error;
}
EXPORT_SYMBOL(_zone_open_ioctl);

int _reset_wp_ioctl(struct gendisk *disk, unsigned long arg)
{
	int error = -EFAULT;

	if (arg & 1) {
		if (arg != ~0ul)
			arg &= ~1ul; /* ~1 :: 0xFF...FE */
		error = blk_zoned_reset_wp_ata(disk, arg);
	} else {
		if (arg == ~1ul)
			arg = ~0ul;
		error = blk_zoned_reset_wp(disk, arg);
	}

	return error;
}
EXPORT_SYMBOL(_reset_wp_ioctl);

int _report_zones_ioctl(struct gendisk *disk, void __user *parg)
{
	int error = -EFAULT;
	struct bdev_zone_report_io *zone_iodata = NULL;
	u32 alloc_size = max(PAGE_SIZE, sizeof(*zone_iodata));
	u8 opt = 0;

	zone_iodata = kmalloc(alloc_size, GFP_KERNEL);
	if (!zone_iodata) {
		error = -ENOMEM;
		goto report_zones_out;
	}
	if (copy_from_user(zone_iodata, parg, sizeof(*zone_iodata))) {
		error = -EFAULT;
		goto report_zones_out;
	}
	if (zone_iodata->data.in.return_page_count > alloc_size) {
		void *tmp;

		alloc_size = zone_iodata->data.in.return_page_count;
		if (alloc_size < KMALLOC_MAX_SIZE) {
			tmp = krealloc(zone_iodata, alloc_size, GFP_KERNEL);
			if (!tmp) {
				error = -ENOMEM;
				goto report_zones_out;
			}
			zone_iodata = tmp;
		} else {
			/* Result requires DMA capable memory */
			pr_err("Not enough memory available for request.\n");
			error = -ENOMEM;
			goto report_zones_out;
		}
	}
	opt = zone_iodata->data.in.report_option & 0x7F;
	if (zone_iodata->data.in.report_option & ZOPT_USE_ATA_PASS)
		error = blk_zoned_report_ata(disk,
				zone_iodata->data.in.zone_locator_lba,
				opt, &zone_iodata->data.out, alloc_size);
	else
		error = blk_zoned_report(disk,
				zone_iodata->data.in.zone_locator_lba,
				opt, &zone_iodata->data.out, alloc_size);

	if (error)
		goto report_zones_out;

	if (copy_to_user(parg, zone_iodata, alloc_size))
		error = -EFAULT;

report_zones_out:
	kfree(zone_iodata);
	return error;
}
EXPORT_SYMBOL(_report_zones_ioctl);

bool blk_is_zoned(struct gendisk *disk, bool *is_host_aware)
{
	int ata = 1;
	int error = 0;
	u8 extended = 1;
	u8 *inq = NULL;
	u8 page_op = 0xb1;	/* block device characteristics page */
	u16 size = 64;

	if (ata) {
		struct zoned_identity ident;

		error = blk_zoned_identify_ata(disk, &ident);
		if (error)
			return false;
		if (is_host_aware && ident.type_id == HOST_AWARE)
			*is_host_aware = 1;
		return true;
	}

	inq = kmalloc(PAGE_SIZE, GFP_KERNEL);	/* zoned inq */
	if (!inq)
		return false;

	error = blk_zoned_inquiry(disk, extended, page_op, size, inq);

	if (!error && is_host_aware) {
		int type = inq[8] >> 4 & 0x03;
		*is_host_aware = type == IS_HA;
	}

	kfree(inq);

	return true;
}
EXPORT_SYMBOL(blk_is_zoned);

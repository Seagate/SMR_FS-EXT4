/*
 * Functions for zone based SMR devices.
 *
 * Copyright (C) 2015 Seagate Technology PLC
 *
 * Written by:
 * Shaun Tancheff <shaun.tancheff@seagate.com>
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef BLK_ZONED_CTRL_H
#define BLK_ZONED_CTRL_H

#include <uapi/linux/blk-zoned-ctrl.h>

/* this is basically scsi_execute */
int blk_cmd_execute(struct request_queue *, const unsigned char *, int, void *,
		    unsigned, unsigned char *, int, int, u64, int *);
int blk_cmd_with_sense(struct gendisk *, u8 *, int, void *, u32 len, u8 *);

/* scsi zbc commands */
int blk_zoned_inquiry(struct gendisk *, u8, u8, u16, u8 *);
int blk_zoned_close(struct gendisk *disk, u64 start_lba);
int blk_zoned_finish(struct gendisk *disk, u64 start_lba);
int blk_zoned_open(struct gendisk *disk, u64 start_lba);
int blk_zoned_reset_wp(struct gendisk *disk, u64 start_lba);
int blk_zoned_report(struct gendisk *, u64, u8,
		     struct bdev_zone_report *, size_t);

/* ata zac variants */
int blk_zoned_identify_ata(struct gendisk *disk, struct zoned_identity *);
int blk_zoned_close_ata(struct gendisk *disk, u64 start_lba);
int blk_zoned_finish_ata(struct gendisk *disk, u64 start_lba);
int blk_zoned_open_ata(struct gendisk *disk, u64 start_lba);
int blk_zoned_reset_wp_ata(struct gendisk *disk, u64 start_lba);
int blk_zoned_report_ata(struct gendisk *disk, u64, u8,
			 struct bdev_zone_report *, size_t);

/* device type agnostic commands */
bool blk_is_zoned(struct gendisk *disk, bool *is_host_aware);

/* for testing from userspace via ioctl */
int _inquiry_ioctl(struct gendisk *disk, void __user *parg);
int _zone_close_ioctl(struct gendisk *disk, unsigned long arg);
int _zone_finish_ioctl(struct gendisk *disk, unsigned long arg);
int _zone_open_ioctl(struct gendisk *disk, unsigned long arg);
int _reset_wp_ioctl(struct gendisk *disk, unsigned long arg);
int _report_zones_ioctl(struct gendisk *disk, void __user *parg);

#endif /* BLK_ZONED_CTRL_H */

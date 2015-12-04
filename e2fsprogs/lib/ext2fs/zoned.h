/*
 * zoned.h --- header file for querying zoned device information
 *
 * Copyright (C) 2015 Seagate Technology PLC
 *
 * Written by:
 * Paul Paulson <paul.paulson@seagate.com>
 *
 * %Begin-Header%
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 * %End-Header%
 */

#ifndef _ZONED_H
#define _ZONED_H

#include "ext2fs/ext2fs.h"

typedef enum
{
	INQ_STANDARD = 0,			// standard inquiry
	INQ_DEVICE_ID = 0x83,			// device identification inquiry
	INQ_BLKDEV_CHARACTERISTICS = 0xb1,	// block device characteristics
} InquiryPageCode;

int is_zoned(const char* device_name);
blk64_t get_offset_to_zone(const char* device_name,
	blk64_t zone_size, blk64_t block_size, blk64_t lba);
blk64_t get_partition_start(const char *device_name);
int get_zone_size(const char* device_name, __u64 *size);

#endif	/* _ZONED_H */

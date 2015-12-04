/*
 * fs/ext4/zoned.h
 *
 * Header for support functions for zone based SMR devices.
 *
 * Copyright (C) 2015 Seagate Technology PLC
 *
 * Written by:
 * Paul Paulson <paul.paulson@seagate.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _EXT4_ZONED_H
#define _EXT4_ZONED_H

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/blk-zoned-ctrl.h>

extern int get_zone_info(struct super_block *sb, ext4_fsblk_t block, sector_t *lba_sector, sector_t *zone_sector, struct bdev_zone_descriptor *info);

void get_zone_sector_info(struct super_block *sb, u64 lba, sector_t *lba_sector, sector_t *zone_sector);

void pr_group_info(const char* msg, struct super_block *sb, ext4_fsblk_t block);

extern int report_zone_info(const char* msg, struct super_block *sb, ext4_fsblk_t block);

int verify_wp(const char* msg, struct super_block *sb, ext4_fsblk_t block, ext4_fsblk_t count);

extern int verify_wp_2(const char *msg, struct super_block *sb, ext4_group_t group, ext4_grpblk_t group_block, ext4_fsblk_t count);

#endif	/* _EXT4_ZONED_H */

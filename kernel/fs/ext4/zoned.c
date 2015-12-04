/*
 * Support functions for zone based SMR devices.
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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public Licens
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-
 */

#ifdef CONFIG_EXT4_SMR_HA

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include "ext4.h"
#include "zoned.h"

#define SECTORS_PER_BLOCK	(EXT4_BLOCK_SIZE(sb) / 512)
#define SECTORS_PER_ZONE	524288ULL

/**
 * get_zone_sector_info() - gets zone sector info from lba
 * @sb:			the super block of the filesystem
 * @block:		filesystem-relative logical block address
 * @start_sector	device relative starting sector of zone
 * @zone_sector		device relative starting sector of lba
 *
 * Return:	0 on success or negative error code
 */
void get_zone_sector_info(struct super_block *sb, ext4_fsblk_t block,
			  sector_t *lba_sector, sector_t *zone_sector)
{
	const sector_t partition_offset = get_start_sect(sb->s_bdev);

	*lba_sector = block * SECTORS_PER_BLOCK + partition_offset;
	*zone_sector = (*lba_sector / SECTORS_PER_ZONE) * SECTORS_PER_ZONE;
}

/**
 * report_zone_info() - report zone info for filesystem block
 * @msg:	a message to print on verification failure
 * @sb:		the super block of the filesystem
 * @block:	filesystem-relative logical block address
 *
 * Return:	0 on success or negative error code
 */
int report_zone_info(const char* msg,
		     struct super_block *sb, ext4_fsblk_t block)
{
	struct bdev_zone_descriptor info;
	sector_t lba_sector;
	sector_t zone_sector;
	int error = 0;

	error = get_zone_info(sb, block, &lba_sector, &zone_sector, &info);

	if (error)
		pr_info("%s: %s: error %d\n", __func__, msg, error);
	else
		pr_info("%s: %s: lba %lld start %lld wp %lld\n",
			__func__, msg, block, info.lba_start, info.lba_wptr);

	return error;
}

/**
 * get_zone_info() - gets zone descriptor for a filesystem-relative block number
 * @sb:		the super block of the filesystem
 * @lba:	logical block address anywhere within zone
 * @info:	report zone info
 *
 * Return:	0 on success or negative error code
 */
int get_zone_info(struct super_block *sb, ext4_fsblk_t block,
		  sector_t *lba_sector, sector_t *zone_sector,
		  struct bdev_zone_descriptor *info)
{
	const size_t bufsize = 32 * 1024;
	struct gendisk *disk = sb->s_bdev->bd_disk;
	u8* buf;
	int error = 0;
	u8  opt = 0;	/* list all zone types */

	get_zone_sector_info(sb, block, lba_sector, zone_sector);

	buf = kmalloc(bufsize, GFP_KERNEL);

	if (buf == NULL)
		return -ENOMEM;

	error = blk_zoned_report_ata(disk,
		(u64)(*zone_sector), opt,
		(struct bdev_zone_report *)buf, bufsize);

	if (error)
		ext4_smr_debug("error %d\n", error);
	else
		*info = ((struct bdev_zone_report *)buf)->descriptors[0];

	kfree(buf);

	return error;
}

/**
 * pr_group_info() - prints group info
 * @msg:	a message to print
 * @sb:		the super block of the filesystem
 * @block:	filesystem-relative block number
 * @count:	block count
 *
 * Return:	0 on success or negative error code
 */
void pr_group_info(const char* msg, struct super_block *sb, ext4_fsblk_t block)
{
	ext4_group_t group;
	ext4_grpblk_t offset;

	ext4_get_group_no_and_offset(sb, block, &group, &offset);

	ext4_smr_debug("%s: group %u offset %u fs block %lld\n",
		msg, group, offset, block);
}

/**
 * verify_wp() - test block number against write pointer
 * @msg:	a message to print on verification failure
 * @sb:		the super block of the filesystem
 * @block:	filesystem-relative block number
 * @count:	block count
 *
 * Return:	0 on success or negative error code
 */
int verify_wp(const char* msg,
	      struct super_block *sb, ext4_fsblk_t block, ext4_fsblk_t count)
{
	ext4_group_t group;
	ext4_grpblk_t group_block;

	ext4_get_group_no_and_offset(sb, block, &group, &group_block);

	return verify_wp_2(msg, sb, group, group_block, count);
}

/**
 * verify_wp() - test block number against write pointer
 * @msg:	a message to print on verification failure
 * @sb:		the super block of the filesystem
 * @group:	block group number
 * @group_block:group relative block number
 * @count:	block count
 *
 * Return:	0 on success or negative error code
 */
int verify_wp_2(const char* msg, struct super_block *sb,
	      ext4_group_t group, ext4_grpblk_t group_block, ext4_fsblk_t count)
{
	ext4_fsblk_t first_block = ext4_group_first_block_no(sb, group);
	ext4_fsblk_t block = first_block + group_block;
	struct bdev_zone_descriptor info;
	sector_t lba_sector;
	sector_t zone_sector;
	int error = 0;

	error = get_zone_info(sb, block, &lba_sector, &zone_sector, &info);

	if (error)
		ext4_smr_debug("%s: error %d\n", msg, error);
#if 0
	else
		ext4_smr_debug("%s: group %u group block %d fs block %lld "
			"lba sector %lld zone sector %lld\n",
			msg, group, group_block, block,
			(u64)lba_sector, (u64)zone_sector);
#endif
	ext4_smr_debug("group %u offset %d block %lld count %lld "
		"block sector %lld sectors %lld wp %lld %s (%s)\n",
		group, group_block, block, count,
		(u64)lba_sector, count * SECTORS_PER_BLOCK, info.lba_wptr,
		lba_sector == info.lba_wptr ? "PASS" : "FAIL",
		msg);

	return error;
}

#endif	/* CONFIG_EXT4_SMR_HA */

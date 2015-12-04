/*
 * zoned.cpp --- Support for querying zoned device information
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

#include "config.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <scsi/scsi.h>
#include <scsi/sg.h>

#include <linux/types.h>
#include <linux/blk-zoned-ctrl.h>

#include "zoned.h"

#define SCSI_IOCTL_REPORT_ZONES         0x10005

/* Inquiry command allocation lengths */
static const int INQ_BUF_SIZE = 64;
static const int INQ_DEVICE_ID_BUF_SIZE = 240;

static int inquiry(int fd, InquiryPageCode pagecode,
	uint8_t* buf, int buf_size, bool vpd);
#ifdef DEBUG
static void print_buf(const char* msg, uint8_t* buf, size_t size);
#endif

#define min(x, y) (((x) > (y)) ? (y) : (x))

/**
 * Get block number of next zone boundary.
 *
 * @param device name
 * @param zone_size zone size in bytes
 * @param block_size block size in bytes
 * @param lba target LBA
 * @return block number of next zone boundary
 */
blk64_t get_offset_to_zone(const char* device_name,
	blk64_t zone_size, blk64_t block_size, blk64_t lba)
{
	blk64_t sector_size = 512;
	blk64_t align = zone_size / block_size;
	blk64_t part_offset =
		(get_partition_start(device_name) * sector_size) / block_size;
	blk64_t phys_lba = part_offset + lba;
	blk64_t new_phys_lba = ((phys_lba + align - 1) / align) * align;
	blk64_t delta = new_phys_lba - phys_lba;

	return delta;
}

/**
 * Sense whether a device is zoned.
 *
 * @param device name
 * @return whether the device supports zoned command set
 */
int is_zoned(const char* device_name)
{
	int zoned = 0;
	uint8_t buf[INQ_BUF_SIZE];
	int fd = ext2fs_open_file(device_name, O_RDWR, 0666);
	int size = 0;

	if (fd == -1) {
		fprintf(stderr, "%s: %s\n", device_name, strerror(errno));
	}

	if ((size = inquiry(fd,
		INQ_BLKDEV_CHARACTERISTICS, buf, sizeof(buf), true)) > 0) {
		zoned = (buf[8] >> 4) & 1;	/* per sbc4r04 */
#ifdef DEBUG
		printf("%s: %s\n", __func__, zoned ? "zoned" : "not zoned");
		print_buf("inquiry data", buf, size);
#endif
	}

	if (fd >= 0) {
		close(fd);
	}

	return zoned;
}

/**
 * Gets starting sector of a device.
 *
 * @param device_name name of device
 * @return starting sector number of device
 */
blk64_t get_partition_start(const char *device_name)
{
	char sysfs[256];
	const char *p = "";
	char letter;
	int number;
	FILE *fp;
	int i;
	int len = strlen(device_name);
	blk64_t start = 0;

	/* extract the drive letter and number */

	for (i = len; --i >= 0; ) {
		if (device_name[i] == '/') {
			p = &device_name[i + 1];
			break;
		}
	}

	if ((sscanf(p, "sd%c%d", &letter, &number)) == 2)
	{
		snprintf(sysfs, sizeof(sysfs),
			"/sys/block/sd%c/sd%c%d/start", letter, letter, number);

		if ((fp = fopen(sysfs, "r")) != 0) {
			fscanf(fp, "%llu", &start);
			fclose(fp);
		}
	}

	return start;
}

/**
 * Gets a single zone entry.
 *
 * @param device name
 * @param lba first LBA in zone
 * @param info where to store the zone entry info
 * @return 0 on success, errno on failure
 */
static int get_zone_entry(const char *device_name, blk64_t lba,
		   struct bdev_zone_descriptor *info)
{
	int do_ata = 1;		/* whether to use ATA command */
	int error = 0;
	uint8_t buf[32 * 1024];
	struct bdev_zone_report_io *zone_info =
		(struct bdev_zone_report_io *)buf;
	int fd = ext2fs_open_file(device_name, O_RDWR, 0666);

	if (fd == -1) {
		error = errno;
		fprintf(stderr, "%s: %s\n", device_name, strerror(errno));
		return error;
	}

	memset(buf, 0, sizeof(buf));
	zone_info->data.in.report_option = ZOPT_NON_SEQ_AND_RESET;
	zone_info->data.in.return_page_count = sizeof(buf);
	zone_info->data.in.zone_locator_lba = lba;

	if (do_ata) {
		zone_info->data.in.report_option |= 0x80;
	}

	error = ioctl(fd, SCSI_IOCTL_REPORT_ZONES, zone_info);

	if (error == -1) {
		error = errno;
		fprintf(stderr, "%s: %s\n", device_name, strerror(error));
	}
	else
		*info = zone_info->data.out.descriptors[0];

	close(fd);

	return error;
}

/**
 * Gets the zone size in bytes.
 *
 * @param device name
 * @param where to store zone size
 * @return 0 on success, errno on failure
 */
int get_zone_size(const char* device_name, __u64 *size)
{
	struct bdev_zone_descriptor info;
	int error = get_zone_entry(device_name, 0, &info);

	if (error == -1)
		fprintf(stderr, "%s: %s: %s\n",
			__func__, device_name, strerror(error));
	else {
		*size = info.length * 512;	/* convert sectors to bytes */
	}

	return error;
}

/**
 * Issues the Inquiry command.
 *
 * @param pagecode Inquiry page code
 * @param buffer where to put data returned by the command
 * @param buf_size the allocation length of the data returned by the command
 * @param vpd whether the request is for Vital Product Data
 * @return number of bytes returned
 */
static int inquiry(int fd, InquiryPageCode pagecode,
	uint8_t* buf, int buf_size, bool vpd)
{
	sg_io_hdr_t hdr;
	uint8_t inqbuf[buf_size];
	uint8_t sensebuf[32];
	uint8_t cdb[] =
	{
		INQUIRY,
		vpd,
		pagecode,
		0,
		sizeof(inqbuf),
		0
	};
	int size = 0;

	memset(&hdr, 0, sizeof(hdr));

	hdr.interface_id = 'S';
	hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	hdr.cmd_len = sizeof(cdb);
	hdr.mx_sb_len = sizeof(sensebuf);
	hdr.dxfer_len = sizeof(inqbuf);
	hdr.dxferp = inqbuf;
	hdr.cmdp = cdb;
	hdr.sbp = sensebuf;
	hdr.timeout = 20000;	/* milliseconds */

#ifdef DEBUG
	print_buf("inquiry cdb", cdb, sizeof(cdb));
#endif

	if (ioctl(fd, SG_IO, &hdr) < 0)
	{
		fprintf(stderr, "ioctl(SG_IO) %s", strerror(errno));
		return 0;
	}

	size = min(buf_size, sizeof(inqbuf));

	memcpy(buf, inqbuf, size);

	return size;
}

/**
 * Prints a byte sized buffer.
 *
 * @param msg a message to print
 * @param buf a command descriptor block
 * @param size size of command descriptor block
 */
#ifdef DEBUG
static void print_buf(const char* msg, uint8_t* buf, size_t size)
{
	const int MAX_LINE_LENGTH = 16;
	int i;

	printf("%s:\n   ", msg);

	for (i = 1; i <= size; ++i) {
		printf(" %02x", buf[i - 1]);
		if (i && (i % MAX_LINE_LENGTH) == 0)
			printf("\n   ");
	}

	printf("\n");
}
#endif

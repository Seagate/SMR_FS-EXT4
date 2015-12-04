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

#ifndef _UAPI_BLK_ZONED_CTRL_H
#define _UAPI_BLK_ZONED_CTRL_H

enum zone_report_option {
	ZOPT_NON_SEQ_AND_RESET   = 0x00,
	ZOPT_ZC1_EMPTY,
	ZOPT_ZC2_OPEN_IMPLICIT,
	ZOPT_ZC3_OPEN_EXPLICIT,
	ZOPT_ZC4_CLOSED,
	ZOPT_ZC5_FULL,
	ZOPT_ZC6_READ_ONLY,
	ZOPT_ZC7_OFFLINE,
	ZOPT_RESET               = 0x10,
	ZOPT_NON_SEQ             = 0x11,
	ZOPT_NON_WP_ZONES        = 0x3f,
	ZOPT_USE_ATA_PASS        = 0x80,
};

/* Report, close, finish, open, reset wp: */
enum zone_zm_action {
	REPORT_ZONES_EXT   = 0x00,
	CLOSE_ZONE_EXT,
	FINISH_ZONE_EXT,
	OPEN_ZONE_EXT,
	RESET_WP_EXT,
};

enum bdev_zone_type {
	ZTYP_RESERVED            = 0,
	ZTYP_CONVENTIONAL        = 1,
	ZTYP_SEQ_WRITE_REQUIRED  = 2,
	ZTYP_SEQ_WRITE_PREFERRED = 3,
};

enum bdev_zone_condition {
	ZCOND_CONVENTIONAL       = 0, /* no write pointer */
	ZCOND_ZC1_EMPTY          = 1,
	ZCOND_ZC2_OPEN_IMPLICIT  = 2,
	ZCOND_ZC3_OPEN_EXPLICIT  = 3,
	ZCOND_ZC4_CLOSED         = 4,
	/* 5 - 0xC - reserved */
	ZCOND_ZC6_READ_ONLY      = 0xd,
	ZCOND_ZC5_FULL           = 0xe,
	ZCOND_ZC7_OFFLINE        = 0xf,
};


enum bdev_zone_same {
	ZS_ALL_DIFFERENT        = 0,
	ZS_ALL_SAME             = 1,
	ZS_LAST_DIFFERS         = 2,
	ZS_SAME_LEN_DIFF_TYPES  = 3,
};

enum zc_report_options {
	ZC_RO_RESET = 0x00,
	ZC_RO_OFFLINE = 0x01,
	ZC_RO_RDONLY = 0x02,
	ZC_RO_FULL = 0x03,
	ZC_RO_OP_NOT_READY = 0x4,
	ZC_RO_ALL = 0xF,
};

struct bdev_zone_get_report {
	__u64 zone_locator_lba;	  /* starting lba for first [reported] zone */
	__u32 return_page_count;  /* number of bytes allocated for result */
	__u8  report_option;	  /* see: zone_report_option enum */
} __packed;

/* NOTE: all LBA's are u64 only use the lower 48 bits */

struct bdev_zone_descriptor {
	__u8  type;         /* see zone_type enum */
	__u8  flags;        /* 0:reset, 1:non-seq, 2-3: resv,
			     * bits 4-7: see zone_condition enum */
	__u8  reserved1[6];
	__u64 length;       /* length of zone: in sectors */
	__u64 lba_start;    /* lba of zone start */
	__u64 lba_wptr;     /* lba of write pointer - ready to be written
			     * next */
	__u8 reserved[32];
} __packed;


struct bdev_zone_report {
	__u32 descriptor_count;   /* number of zone_descriptor entries that
				   * follow */
	__u8  same_field;         /* bits 0-3: enum zone_same (MASK: 0x0F) */
	__u8  reserved1[3];
	__u64 maximum_lba;        /* The MAXIMUM LBA field indicates the
				   * LBA of the last logical sector on the
				   * device, including all logical sectors
				   * in all zones. */
	__u8  reserved2[48];
	struct bdev_zone_descriptor descriptors[0];
} __packed;

struct bdev_zone_report_io {
	union {
		struct bdev_zone_get_report in;
		struct bdev_zone_report out;
	} data;
} __packed;

struct zoned_inquiry {
	__u8  evpd;
	__u8  pg_op;
	__u16 mx_resp_len;
	__u8  result[0];
} __packed;

/**
 * Flags to determine if the connected disk is ZONED:
 *   - Host Aware of Host Managed (or not)
 */
enum zoned_identity_type_id {
	NOT_ZONED    = 0x00,
	HOST_AWARE   = 0x01,
	HOST_MANAGE  = 0x02,
};

/* ata passthrough variant */
struct zoned_identity {
	__u8 type_id;
	__u8 reserved[3];
} __packed;

#endif /* _UAPI_BLK_ZONED_CTRL_H */

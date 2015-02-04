#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

/* Used for Zone based SMR devices */
#define SCSI_IOCTL_IDENTIFY		0x10000
#define SCSI_IOCTL_RESET_WP		0x10001
#define SCSI_IOCTL_REPORT_ZONES		0x10002
#define SCSI_IOCTL_OPEN_ZONE		0x10003
#define SCSI_IOCTL_CLOSE_ZONE		0x10004

/* seagate: report zones: T13 -> 0x4A: REPORT_ZONES */
// #define EXT4_IOC_REPORT_ZONES       	_IOWR('f', 19, struct bdev_zone_report_ioctl_t)

#ifndef __packed
#define __packed __attribute__((packed))
#endif

typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;


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
};

enum zone_zm_action {
 // Report, close, finish, open, reset wp:
	REPORT_ZONES_EXT   = 0x00,
	CLOSE_ZONE_EXT,
	FINISH_ZONE_EXT,
	OPEN_ZONE_EXT,
	RESET_WP_EXT,
};

struct bdev_zone_report_request_t {
	u64 zone_locator_lba;	  /* starting lba for first zone to be reported. */
	u32 return_page_count;  /* number of bytes allocated for result */
	u8  report_option;	  /* see: zone_report_option enum */
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

/* NOTE: all LBA's are u64 only use the lower 48 bits */

struct bdev_zone_descriptor_entry_t {
	u8  type;         /* see zone_type enum */
	u8  flags;        /* 0:reset, 1:non-seq, 2-3: resv,
                           * bits 4-7: see zone_condition enum */
	u8  reserved1[6];
	u64 length;       /* length of zone: in sectors */
	u64 lba_start;    /* lba of zone start */
	u64 lba_wptr;     /* lba of write pointer - ready to be written next */
        u8 reserved[32];
} __packed;

enum bdev_zone_same {
	ZS_ALL_DIFFERENT        = 0,
	ZS_ALL_SAME             = 1,
	ZS_LAST_DIFFERS         = 2,
	ZS_SAME_LEN_DIFF_TYPES  = 3,
};

const char * same_text[] = {
	"all zones are different",
	"all zones are same size",
	"last zone differs by size",
	"all zones same size - different types",
};

struct bdev_zone_report_result_t {
	u32 descriptor_count;   /* number of zone_descriptor entries that follow */
	u8  same_field;         /* bits 0-3: enum zone_same (MASK: 0x0F) */
	u8  reserved1[3];
	u64 maximum_lba;        /* The MAXIMUM LBA field indicates the LBA of the
	                         * last logical sector on the device, including
	                         * all logical sectors in all zones. */
	u8  reserved2[48];
	struct bdev_zone_descriptor_entry_t descriptors[0];
} __packed;

struct bdev_zone_report_ioctl_t {
	union {
		struct bdev_zone_report_request_t in;
		struct bdev_zone_report_result_t out;
	} data;
} __packed;

const char * type_text[] = {
	"RESERVED",
	"CONVENTIONAL",
	"SEQ_WRITE_REQUIRED",
	"SEQ_WRITE_PREFERRED",
};

unsigned char r_opts[] = {
	ZOPT_NON_SEQ_AND_RESET,
	ZOPT_ZC1_EMPTY,
	ZOPT_ZC2_OPEN_IMPLICIT,
	ZOPT_ZC3_OPEN_EXPLICIT,
	ZOPT_ZC4_CLOSED,
	ZOPT_ZC5_FULL,
	ZOPT_ZC6_READ_ONLY,
	ZOPT_ZC7_OFFLINE,
	ZOPT_RESET,
	ZOPT_NON_SEQ,
	ZOPT_NON_WP_ZONES,
};

static char * r_opt_text[] = {
        "NON_SEQ_AND_RESET",
        "ZC1_EMPTY",
	"ZC2_OPEN_IMPLICIT",
	"ZC3_OPEN_EXPLICIT",
	"ZC4_CLOSED",
	"ZC5_FULL",
	"ZC6_READ_ONLY",
	"ZC7_OFFLINE",
	"RESET",
	"NON_SEQ",
        "NON_WP_ZONES",
};

#define ARRAY_COUNT(x) (sizeof((x))/sizeof((*x)))

void print_zones(struct bdev_zone_report_result_t * info, uint32_t size)
{
	u32 count = info->descriptor_count;
	u32 max_count;
	int iter;
	int same_code = info->same_field & 0x0f;
	fprintf(stdout, "  count: %u, same %u (%s), max_lba %lu\n", 
		count,
		same_code, same_text[same_code],
		info->maximum_lba & (~0ul >> 16) );

	max_count = (size - sizeof(struct bdev_zone_report_result_t)) 
                        / sizeof(struct bdev_zone_descriptor_entry_t);
	if (count > max_count) {
		fprintf(stderr, "Truncating report to %d of %d zones.\n",
			max_count, count );
		count = max_count;
	}

	for (iter = 0; iter < count; iter++ ) {
		struct bdev_zone_descriptor_entry_t * entry = 
			&info->descriptors[iter];
		unsigned int type  = entry->type & 0xF;
		unsigned int flags = entry->flags;

		fprintf(stdout, 
			"  start: %lu, len %lu, wptr %lu\n"
			"   type: %u(%s) reset:%u non-seq:%u, zcond:%u\n",
		entry->lba_start,
		entry->length, 
		entry->lba_wptr,
		type, type_text[type],
		flags & 0x01, (flags & 0x02) >> 1, (flags & 0xF0) >> 4);  
	}
}

int do_report_zones_ioctl(const char * pathname, uint64_t lba)
{
	int rc = -4;
        int fd = open(pathname, O_RDWR);
        if (fd != -1) {
		struct bdev_zone_report_ioctl_t * zone_info;
                uint64_t size;

		/* NOTE: 128 seems to be about the RELIABLE limit ...     */
                /*       150 worked 180 was iffy (some or all ROs failed) */
                /*       256 all ROs failed..                             */
                size = 128 * 4096;
                zone_info = malloc(size);
                if (zone_info) {
			int opt = 0;
			uint32_t cmd = SCSI_IOCTL_REPORT_ZONES;

			for (opt = 0; opt < ARRAY_COUNT(r_opts); opt++) {
				memset(zone_info, 0, size);
				zone_info->data.in.report_option = r_opts[opt];
				zone_info->data.in.return_page_count = size;
				zone_info->data.in.zone_locator_lba  = lba;

				rc = ioctl(fd, cmd, zone_info);
				if (rc != -1) {
					fprintf(stdout, "%s(%d): found %d zones\n", 
						r_opt_text[opt], 
						r_opts[opt],
						zone_info->data.out.descriptor_count);
					print_zones(&zone_info->data.out, size);
				} else {
					fprintf(stderr, "ERR: %d -> %s\n\n", errno, strerror(errno));
					break;
				}
			}
		}
                close(fd);
        } else {
                fprintf(stderr, "%s\n\n", strerror(errno));
        }

	return rc;
}


/*
 *
 */
int main(int argc, char *argv[])
{
        const char * pathname = "/dev/sdm";
        uint64_t lba = 0;
        char * fname = NULL;
	int rc;
	int ii;

	for (ii = 1; ii < argc; ii++) {
		char * endptr ;
		uint64_t tmp = strtoull(argv[ii], &endptr, 0);
		if (0 == *endptr) {
			lba = tmp;
		} else if (!fname) {
			fname = argv[ii];
		}
	}
	rc = do_report_zones_ioctl(fname ? fname : pathname, lba);
	return rc;
}

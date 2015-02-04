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

#ifndef __packed
#define __packed __attribute__((packed))
#endif

enum zone_report_option {
	NON_SEQ_AND_RESET   = 0x00,
	ZC1_EMPTY,
	ZC2_OPEN_IMPLICIT,
	ZC3_OPEN_EXPLICIT,
	ZC4_CLOSED,
	ZC5_FULL,
	ZC6_READ_ONLY,
	ZC7_OFFLINE,
	RESET               = 0x10,
	NON_SEQ             = 0x11,
	NON_WP_ZONES        = 0x3f,
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
	__u64 zone_locator_lba;	  /* starting lba for first zone to be reported. */
	__u16 return_page_count;  /* The device shall return the number of 512-byte
	                             pages specified in the RETURN PAGE COUNT field. */
	__u8  report_option;	  /* see: zone_report_option enum */
};

enum bdev_zone_type {
	RESERVED            = 0,
	CONVENTIONAL        = 1,
	SEQ_WRITE_REQUIRED  = 2,
	SEQ_WRITE_PREFERRED = 3,
};

enum bdev_zone_condition {
	COND_CONVENTIONAL       = 0, /* no write pointer */
	COND_ZC1_EMPTY          = 1,
	COND_ZC2_OPEN_IMPLICIT  = 2,
	COND_ZC3_OPEN_EXPLICIT  = 3,
	COND_ZC4_CLOSED         = 4,
	/* 5 - 0xC - reserved */
	COND_ZC6_READ_ONLY      = 0xd,
	COND_ZC5_FULL           = 0xe,
	COND_ZC7_OFFLINE        = 0xf,
};

/* NOTE: all LBA's are u64 only use the lower 48 bits */

struct bdev_zone_descriptor_entry_t {
	__u8  type;         /* see zone_type enum */
	__u8  flags;        /* 0:reset, 1:non-seq, 2-3: resv,
	                     * bits 4-7: see zone_condition enum */
	__u8  reserved1[6];
	__u64 length;       /* length of zone: in sectors */
	__u64 lba_start;    /* lba of zone start */
	__u64 lba_wptr;     /* lba of write pointer - ready to be written next */
        __u8 reserved[32];
} __packed;

enum bdev_zone_same {
	ALL_DIFFERENT        = 0,
	ALL_SAME             = 1,
	LAST_DIFFERS         = 2,
	SAME_LEN_DIFF_TYPES  = 3,
};

const char * same_text[] = {
	"all zones are different",
	"all zones are same size",
	"last zone differs by size",
	"all zones same size - different types",
};

struct bdev_zone_report_result_t {
	__u32 descriptor_count;   /* number of zone_descriptor entries that follow */
	__u8  same_field;         /* bits 0-3: enum zone_same (MASK: 0x0F) */
	__u8  reserved1[3];
	__u64 maximum_lba;        /* The MAXIMUM LBA field indicates the LBA of the
	                           * last logical sector on the device, including
	                           * all logical sectors in all zones. */
	__u8  reserved2[48];
	struct bdev_zone_descriptor_entry_t descriptors[0];
} __packed;

struct bdev_zone_report_ioctl_t {
        size_t size;
	union {
		struct bdev_zone_report_request_t in;
		struct bdev_zone_report_result_t out;
	} data;
} __packed;


/* seagate: report zones: T13 -> 0x4A: REPORT_ZONES */
#define EXT4_IOC_REPORT_ZONES       	_IOWR('f', 19, struct bdev_zone_report_ioctl_t)

const char * type_text[] = {
	"RESERVED",
	"CONVENTIONAL",
	"SEQ_WRITE_REQUIRED",
	"SEQ_WRITE_PREFERRED",
};

void print_zones(struct bdev_zone_report_result_t * info, uint32_t size)
{
	uint32_t count = info->descriptor_count;
	uint32_t max_count;
	int iter;
	int same_code = info->same_field & 0x0f;
	fprintf(stderr, "  count: %u, same %u (%s), max_lba %llu\n", 
		count,
		same_code, same_text[same_code],
		info->maximum_lba & (~0ul >> 16) );

	max_count = (size - sizeof(struct bdev_zone_report_result_t)) / sizeof(struct bdev_zone_descriptor_entry_t);
	if (count > max_count) 
		count = max_count;

	for (iter = 0; iter < count; iter++ ) {
		struct bdev_zone_descriptor_entry_t * entry = 
			&info->descriptors[iter];
		unsigned int type  = entry->type & 0xF;
		unsigned int flags = entry->flags;

		fprintf(stderr, 
			"  start: %llu, len %llu, wptr %llu\n"
			"   type: %u(%s) reset:%u non-seq:%u, zcond:%u\n",
		entry->lba_start,
		entry->length, 
		entry->lba_wptr,
		type, type_text[type],
		flags & 0x01, (flags & 0x02) >> 1, (flags & 0xF0) >> 4);  
	}
}

unsigned char r_opts[] = {
        NON_SEQ_AND_RESET,
        ZC1_EMPTY,
	ZC2_OPEN_IMPLICIT,
	ZC3_OPEN_EXPLICIT,
	ZC4_CLOSED,
	ZC5_FULL,
	ZC6_READ_ONLY,
	ZC7_OFFLINE,
	RESET,
	NON_SEQ,
        NON_WP_ZONES,
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

int do_report_zones_ioctl(const char * pathname, uint64_t lba)
{
        int fd = open(pathname, O_RDWR);
        if (fd != -1) {
		struct bdev_zone_report_ioctl_t * zone_info;
                uint64_t size;

#ifdef USE_BLOCK
                if (ioctl(fd, BLKGETSIZE64, &size) != -1) {
                        fprintf(stderr, "Size: %" PRIu64 "\n", size);
                } else {
                        fprintf(stderr, "%s\n\n", strerror(errno));
                }
#endif

		fprintf(stderr, "ioctl: %s\n", pathname );

                size = 8 * 4096;
                zone_info = malloc(size);
                if (zone_info) {
			int opt = 0;
			uint32_t cmd = EXT4_IOC_REPORT_ZONES;

			for (opt = 0; opt < ARRAY_COUNT(r_opts); opt++) {
				int rc;
				memset(zone_info, 0, size);
				zone_info->size = size;
				zone_info->data.in.report_option = r_opts[opt];
				zone_info->data.in.return_page_count = size / 512;
				zone_info->data.in.zone_locator_lba  = lba;

				rc = ioctl(fd, cmd, zone_info);
				if (rc != -1) {
					fprintf(stderr, "%s(%d): found %d zones\n", 
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

	return 0;
}


/*
 *
 */
int main(int argc, char *argv[])
{
        const char * pathname = "/mnt/testb/x/blah.txt";
        uint64_t lba = 0;
        char * fname = NULL;
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
	do_report_zones_ioctl(fname ? fname : pathname, lba);
	return 0;
}

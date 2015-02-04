#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#ifndef __packed
#define __packed __attribute__((packed))
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/**
 * Flags to determine if the connected disk is ZONED:
 *   - Host Aware of Host Managed (or not)
 */
typedef enum zc_type {
	NOT_ZONED    = 0x00,
	HOST_AWARE   = 0x01,
	HOST_MANAGE  = 0x02,
} zc_type_t;

typedef enum zc_vendor_type {
	ZONE_DEV_ATA_SEAGATE = 0x00,
	ZONE_DEV_BLK         = 0x01,
} zc_vendor_type_t;

struct zoned_inquiry {
	u8  evpd;
	u8  pg_op;
	u16 mx_resp_len;
	u8  result[0];
} __packed;
typedef struct zoned_inquiry zoned_inquiry_t;

/* Used for Zone based SMR devices */
#define SCSI_IOCTL_IDENTIFY		0x10000
#define SCSI_IOCTL_RESET_WP		0x10001
#define SCSI_IOCTL_REPORT_ZONES		0x10002
#define SCSI_IOCTL_OPEN_ZONE		0x10003
#define SCSI_IOCTL_CLOSE_ZONE		0x10004

#define Z_VPD_INFO_BYTE 8

#define DATA_OFFSET (offsetof(zoned_inquiry_t, result))

int do_identify_ioctl(const char * sddev)
{
	int rc = -4;

#ifdef DEBUG
	fprintf(stderr, "offsetof: %lu\n",  DATA_OFFSET );
#endif
	int fd = open(sddev, O_RDWR);
	if (fd != -1) {
		zoned_inquiry_t * inquire;
		int sz = 64;
		int bytes = sz + DATA_OFFSET;
		inquire = malloc(bytes);
		if (inquire) {
			inquire->evpd	= 1;
			inquire->pg_op       = 0xb1;
			inquire->mx_resp_len = sz;

			fprintf(stderr, "ioctl: %s\n", sddev );
			rc = ioctl(fd, SCSI_IOCTL_IDENTIFY, inquire);
			if (rc != -1) {
				int is_smr = 0;
				int is_ha  = 0;

#ifdef DEBUG
				fprintf(stderr, "rc -> %d, len %d\n", rc, inquire->mx_resp_len );
#endif // DEBUG

				if (inquire->mx_resp_len > Z_VPD_INFO_BYTE) {
					u8 flags = inquire->result[Z_VPD_INFO_BYTE] >> 4 & 0x03;

#ifdef DEBUG
					int x;
					for (x = 0; x < 10; x++) {
						fprintf(stdout, " %d: %02x\n", x , inquire->result[x] );
					}
#endif // DEBUG
					switch (flags) {
						case 1:
							is_ha = 1;
							is_smr = 1;
							break;
						case 2:
							is_smr = 1;
							break;
						default: 
							break;
					}
				}


				fprintf(stderr, "Identify: %s %s\n", 
					is_smr ? "SMR" : "PMR",
					is_smr ? (is_ha  ? "Host AWARE"  : "Host or Drive Managed") : "" );
			} else {
				fprintf(stderr, "ERR: %d -> %s\n\n", errno, strerror(errno));
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
	const char * sddev = "/dev/sdm";
	char * fname = NULL;
	int ii;

	for (ii = 1; ii < argc; ii++) {
		if (!fname) {
			fname = argv[ii];
		}
	}
	return do_identify_ioctl(fname ? fname : sddev);
}

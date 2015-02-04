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

int do_reset_wp_ioctl(const char * pathname, uint64_t lba)
{
	int rc = -4;
        int fd = open(pathname, O_RDWR);
        if (fd != -1) {
		fprintf(stderr, "ioctl: %s\n", pathname );
		rc = ioctl(fd, SCSI_IOCTL_RESET_WP, lba);
		if (rc != -1) {
			fprintf(stderr, "reset wp on %" PRIx64 "\n", lba );
		} else {
			fprintf(stderr, "ERR: %d -> %s\n\n", errno, strerror(errno));
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
	
	return do_reset_wp_ioctl(fname ? fname : pathname, lba);
}

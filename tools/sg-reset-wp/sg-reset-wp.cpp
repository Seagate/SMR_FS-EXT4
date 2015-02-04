/**
 * Issues the reset write pointer command to an SMR drive.
 *
 * This command will reset a single zone write pointer or all write pointers on
 * the drive depending on the command line options. Use the -z option to reset
 * the write pointer for a single zone or --all to reset the write pointers for
 * all zones on the drive. The dev-name argument is a drive or partition name.
 * The --verbose option makes the output verbose.
 *
 *      usage: sg-reset-wp -d dev-name [-z zone] [--all] [--verbose]
 */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

#include <byteswap.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <vector>

using namespace std;

static const uint64_t MIB_PER_ZONE = 256ULL * 1024ULL * 1024ULL;
static const uint64_t SECTOR_SIZE = 512;
static const uint64_t SECTORS_PER_ZONE = MIB_PER_ZONE / SECTOR_SIZE;

static int parseInt(string token);
static void printUsage(const char* progName);
static void resetWritePointer(int fd, uint64_t zoneLba, bool isAll);

/**
 * Main entry point.
 *
 * @param argc the argument count
 * @param argv the argument vector
 * @return 0 for success, errno for failure
 */
int
main(int argc, char *argv[])
{
    int error = 0;
    int fd = -1;
    int result = 0;
    const char *fileName = "undefined";
    bool isAll = false;
    uint64_t zone = -1;
    bool isVerbose = false;

    try
    {
        if (argc < 4 || argc > 6)
        {
            throw EINVAL;
        }

        // Crack the argument list

        for (int i = 1; i < argc; ++i)
        {
            string option = argv[i];

            if (option == "-d" && i < argc - 1)
            {
                fileName = argv[++i];
            }
            else if (option == "-z" && i < argc - 1)
            {
                zone = parseInt(argv[++i]);
            }
            else if (option == "--all")
            {
                isAll = true;
            }
            else if (option == "--verbose")
            {
                isVerbose = true;
            }
            else
            {
                throw EINVAL;
            }
        }

        if (zone == uint64_t(-1) && !isAll)
        {
            throw EINVAL;
        }

        if (isVerbose)
        {
            if (isAll)
            {
                printf("resetting all zone pointers on %s\n", fileName);
            }
            else
            {
                printf("resetting write pointer for zone %lu "
                    "(sector %lu) on %s\n",
                    zone, zone * SECTORS_PER_ZONE, fileName);
            }
        }

        // Open the device and issue the command

        if ((fd = open(fileName, O_WRONLY)) == -1)
        {
            error = errno;
            fprintf(stderr, "%s: %s\n", fileName, strerror(error));
            throw error;
        }

        resetWritePointer(fd, zone, isAll);
    }
    catch (int e)
    {
        result = e;

        if (e == EINVAL)
        {
            printUsage(argv[0]);
        }
    }

    if (fd != -1)
    {
        close(fd);
    }

    return result;
}

/**
 * Parses an integer from either decimal or hexadecimal string representation.
 *
 * @param token decimal or hexadecimal encoded integer
 * @return the integer value
 */
static int
parseInt(string token)
{
    int count = 0;
    int value = 0;

    if (token.find("0x") == 0 ||
        token.find("0X") == 0)
    {
        count = sscanf(token.c_str(), "%x", &value);
    }
    else
    {
        count = sscanf(token.c_str(), "%d", &value);
    }

    if (count == 0)
    {
        throw EINVAL;
    }

    return value;
}

/**
 */
static void
printUsage(const char* progName)
{
    fprintf(stderr, "usage: %s -d devname [-z zone] [--all] [--verbose]\n",
        progName);
}

/**
 * Issues SMR reset write pointer command.
 *
 * @param fd an open file descriptor to the device or partition
 * @param zone a zero relative zone number
 * @param isAll reset write pointers in all zones
 */
static void
resetWritePointer(int fd, uint64_t zone, bool isAll)
{
    int error = 0;
    sg_io_hdr_t hdr;
    uint8_t cdb[16];
    uint8_t senseBuf[32];
    vector<uint16_t> data(256, 0); // single sector
    uint64_t sector = zone * SECTORS_PER_ZONE;
    uint64_t beSector = htobe64(sector);    // convert to big endian

    // Construct the IO header

    memset(&hdr, 0, sizeof(hdr));

    hdr.interface_id = 'S';
    hdr.dxfer_direction = SG_DXFER_TO_DEV;
    hdr.cmd_len = sizeof(cdb);
    hdr.mx_sb_len = sizeof(senseBuf);
    hdr.dxfer_len = data.size() * sizeof(data[0]);
    hdr.dxferp = static_cast<void*>(&data[0]);
    hdr.cmdp = cdb;
    hdr.sbp = senseBuf;
    hdr.timeout = 20000;    // milliseconds

    // Construct the command buffer

    memset(&cdb, 0, sizeof(cdb));

    // Construct 16 byte version of ATA pass through command

    uint8_t block = 1;      // 0 for bytes, 1 for 512 byte blocks
    uint8_t checkCond= 1;   // set to 1 to read back registers
    uint8_t direction = 0;  // 0 to device, 1 from device
    uint8_t extend = 1;     // sector count and LBA fields are reserved
    uint8_t length = 0;     // 0 for no data, 2 for sector count
    uint8_t protocol = 3;   // non-data

    cdb[0] = 0x85;          // ATA pass through (16) command
    cdb[1] = (protocol << 1) | extend;
    cdb[2] = (checkCond << 5) | (direction << 3) | (block << 2) | length;
    cdb[3] = isAll;         // feature MSB (RESET ALL bit)
    cdb[4] = 4;             // feature LSB (RESET WRITE POINTERS EXT)

    cdb[7]  = beSector >> 0;
    cdb[8]  = beSector >> 8;
    cdb[9]  = beSector >> 16;
    cdb[10] = beSector >> 24;
    cdb[11] = beSector >> 32;
    cdb[12] = beSector >> 40;

    cdb[13] = 1 << 6;       // device bit 6 must be set to 1
    cdb[14] = 0x9f;         // ZAC RESET WRITE POINTERS EXT command

    // Execute the command

    if (ioctl(fd, SG_IO, &hdr) < 0)
    {
        error = errno;
        fprintf(stderr, "ioctl(SG_IO) %s\n", strerror(error));
        throw error;
    }
}

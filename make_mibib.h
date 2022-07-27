#include <endian.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <argp.h>

#ifndef __MAKE_MIBIB_H__
#define __MAKE_MIBIB_H__

#define RELEASE_VER "0.0.3"

/* MBN Header Magic */
/*  ac 9f 56 fe 7a 12 7f cd  04 00 00 00 01 00 00 00 */
#define MBN_HEADER_MAGIC1 0xFE569FAC
#define MBN_HEADER_MAGIC2 0xCD7F127A

/* Footer has two magic numbers and a CRC too */
#define MBN_FOOTER_MG1 0x9D41BEA1
#define MBN_FOOTER_MG2 0xF1DED2EA
#define MBN_FOOTER_VERSION 0x00000001

/* Starting at 0x800 comes the GPT */
/* GPT Header Magic */
#define HEADER_MAGIC1 0x55EE73AA
#define HEADER_MAGIC2 0xE35EBDDB
#define HEADER_VERSION 4

/* Block sizes */
#define PAGE_SIZE 2
#define SECTOR_SIZE 2048  // 2K
#define BLOCK_SIZE 131072 // 128KB

#define MBN_OUTPUT_FILE "mibib.mbn"
#define OUTPUT_FILE_SZ 1310720 // (640 sectors x 2048 byte sector size)

/* Make MIBIB */

/* First header to appear on the file */
struct mbn_header {
  uint32_t header1;
  uint32_t header2;
  uint32_t version;
  uint32_t age; // 1
} __attribute__((packed));

/* Aligned to next page */
struct partition_table_header {
  uint32_t header1;
  uint32_t header2;
  uint32_t version;
  uint32_t total_partitions;
} __attribute__((packed));

/* Each partition shall have this structure */
struct partition_def { // 28 byte / partition
  char name[16];       // 0:ALL, 0:boot 0:SBL...
  uint32_t offset;     //(offset / blocksize)
  uint32_t size;       // Size
  uint8_t attr1;       // Always 0xff
  uint8_t attr2;       // Always 0x01?
  uint8_t attr3;       // 0xFF in EFS and MIBIB, otherwise 0x00
  uint8_t which_flash; // 0x00
} __attribute__((packed));

/* Apparently only for NAND devices, page aligned too */
struct mbn_crc_footer {
  uint32_t magic1;
  uint32_t magic2;
  uint32_t version;
  uint32_t crcval;
} __attribute__((packed));

#endif
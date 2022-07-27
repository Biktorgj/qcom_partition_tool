#include <endian.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <argp.h>

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
/*
static const struct {
  uint8_t id;
  uint32_t size;
  uint8_t attr1;
  uint8_t attr2;
  uint8_t attr3;
  uint8_t which_flash;
  const char *name;
} part_list_quec[] = {
    {0, 640, 0xff, 0x01, 0x00, 0x00, "0:SBL"},   //
    {0, 640, 0xff, 0x01, 0xff, 0x00, "0:MIBIB"}, //
    {0, 8704, 0xff, 0x01, 0xff, 0x00, "0:EFS2"}, //
    {0, 2560, 0xff, 0x01, 0x00, 0x00, "0:sys_rev"},
    {0, 640, 0xff, 0x01, 0x00, 0x00, "0:RAWDATA"},
    {0, 768, 0xff, 0x01, 0x00, 0x00, "0:TZ"},  //
    {0, 576, 0xff, 0x01, 0x00, 0x00, "0:RPM"}, //
    {0, 1024, 0xff, 0x01, 0x00, 0x00, "0:cust_info"},
    {0, 768, 0xff, 0x01, 0x00, 0x00, "0:aboot"},     //
    {0, 3968, 0xff, 0x01, 0x00, 0x00, "0:boot"},     //
    {0, 3968, 0xff, 0x01, 0x00, 0x00, "0:recovery"}, //
    {0, 27392, 0xff, 0x01, 0x00, 0x00, "0:modem"},   //
    {0, 320, 0xff, 0x01, 0x00, 0x00, "0:misc"},
    {0, 10368, 0xff, 0x01, 0x00, 0x00, "0:recoveryfs"}, //
    {0, 30720, 0xff, 0x01, 0x00, 0x00, "0:usr_data"},   //
    {0, 256, 0xff, 0x01, 0x00, 0x00, "0:sec"},
    {0, 37760, 0xff, 0x01, 0x00, 0x00, "0:system"},
};
*/
static const struct {
  uint8_t id;
  uint32_t size;
  uint8_t attr1;
  uint8_t attr2;
  uint8_t attr3;
  uint8_t which_flash;
  const char *name;
} part_list[] = {
    {0, 640, 0xff, 0x01, 0x00, 0x00, "0:SBL"},
    {0, 640, 0xff, 0x01, 0xff, 0x00, "0:MIBIB"},
    {0, 6144, 0xff, 0x01, 0xff, 0x00, "0:EFS2"},
    {0, 1152, 0xff, 0x01, 0x00, 0x00, "0:TZ"},
    {0, 192, 0xff, 0x01, 0x00, 0x00, "0:DEVCFG"},
    {0, 192, 0xff, 0x01, 0x00, 0x00, "0:APDP"},
    {0, 256, 0xff, 0x01, 0x00, 0x00, "0:MSADP"},
    {0, 128, 0xff, 0x01, 0x00, 0x00, "0:INFO"},
    {0, 256, 0xff, 0x01, 0x00, 0x00, "0:MBA"},
    {0, 256, 0xff, 0x01, 0x00, 0x00, "0:ACDB"},
    {0, 192, 0xff, 0x01, 0x00, 0x00, "0:RPM"},
    {0, 24192, 0xff, 0x01, 0x00, 0x00, "0:QDSP"},
    {0, 2624, 0xff, 0x01, 0x00, 0x00, "0:APPS"},
    {0, 256, 0xff, 0x01, 0x00, 0x00, "0:Cache_QDSP"},
    {0, 256, 0xff, 0x01, 0x00, 0x00, "0:Cache_APPS"},
    {0, 256, 0xff, 0x01, 0x00, 0x00, "0:Cache_ACDB"},
    {0, 256, 0xff, 0x01, 0x00, 0x00, "0:misc"},
    {0, 1536, 0xff, 0x01, 0x00, 0x00, "0:EFS2BAK"},
    {0, 256, 0xff, 0x01, 0x00, 0x00, "0:BACKUP"},
    {0, 128, 0xff, 0x01, 0x00, 0x00, "0:sec"},
    {0, 3968, 0xff, 0x01, 0x00, 0x00, "0:boot"},
    {0, 21760, 0xff, 0x01, 0x00, 0x00, "0:system"},
};
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

uint32_t crc32_table[256];
void make_crc_table(uint32_t poly) {
  unsigned int i, j;
  unsigned int c;
  memset(crc32_table, 0, 256 * sizeof(uint32_t));
  for (i = 0; i < 256; i++) {
    for (c = i << 24, j = 8; j > 0; --j)
      c = c & 0x80000000 ? (c << 1) ^ poly : (c << 1);
    crc32_table[i] = c;
  }

  return;
}

uint32_t xcrc32 (uint8_t *buf, int len, unsigned int init)
{
  unsigned int crc = init;
  make_crc_table(0x04c11db7);
  while (len--)
    {
      crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
      buf++;
    }
  return crc;
}

int main(void) {
  uint32_t total_partitions = (sizeof(part_list) / sizeof(part_list[0]));
  uint8_t *output_buffer;
  output_buffer = calloc(1310720, sizeof(uint8_t));
  int bufwrsz = 0;
  int offset = 0;

  printf("Tiny MIBIB Builder version %s\n", RELEASE_VER);
  printf("--------------------------------- \n");
  printf("Number of partitions: %i\n\n", total_partitions);

  struct mbn_header *mbnheader;
  mbnheader = calloc(1, sizeof(struct mbn_header));

  struct partition_table_header *table_header;
  table_header = calloc(1, sizeof(struct partition_table_header));

  struct mbn_crc_footer *footer;
  footer = calloc(1, sizeof(struct mbn_crc_footer));

  printf("1. Build MIBIB Header\n");
  /* MIBIB Header: 2 Magic numbers, version and "age", aligned to page size */
  mbnheader->header1 = MBN_HEADER_MAGIC1;
  mbnheader->header2 = MBN_HEADER_MAGIC2;
  mbnheader->version = HEADER_VERSION;
  mbnheader->age = 1;

  memcpy(output_buffer + bufwrsz, mbnheader, sizeof(struct mbn_header));
  bufwrsz += sizeof(struct mbn_header);
  printf(" --> Header size is %i, filling up to minimum of page size of %i \n",
         bufwrsz, PAGE_SIZE * 1024);
  memset(output_buffer + bufwrsz, 0xff,
         (((PAGE_SIZE * 1024) - bufwrsz) * sizeof(uint8_t)));
  bufwrsz += ((PAGE_SIZE * 1024) - bufwrsz) * sizeof(uint8_t);

  printf("2. Build Partition table header\n");
  /* Partition Table header: 2 magix numbers, version and partition count */
  table_header->header1 = HEADER_MAGIC1;
  table_header->header2 = HEADER_MAGIC2;
  table_header->version = HEADER_VERSION;
  table_header->total_partitions = total_partitions;

  printf(" --> Total partitions: %i\n", table_header->total_partitions);
  memcpy(output_buffer + bufwrsz, table_header,
         sizeof(struct partition_table_header));
  bufwrsz += sizeof(struct partition_table_header);

  int partition_layout_sz = 0; // Include the header here
  struct partition_def *part;
  part = calloc(1, sizeof(struct partition_def));

  printf("ID \t Sectors \t Bytes \t\t Bytes hex \t Sectors hex \t Offset \t "
         "Attr1 \t Attr2 \t Attr3 \t Other \t Name\n");
         
  for (int i = 0; i < table_header->total_partitions; i++) {
    memset(part, 0, sizeof(struct partition_def));
    part->size = (part_list[i].size * SECTOR_SIZE / BLOCK_SIZE);
    part->attr1 = part_list[i].attr1;
    part->attr2 = part_list[i].attr2;
    part->attr3 = part_list[i].attr3;
    part->which_flash = part_list[i].which_flash;

    printf("%i \t %.6i \t %.6i \t 0x%.8x \t 0x%.8x\t 0x%.8x\t  %x\t   %x\t   "
           "%x\t   %x\t %s\n",
           i, part_list[i].size, part_list[i].size * SECTOR_SIZE,
           part_list[i].size * SECTOR_SIZE,
           (part_list[i].size * SECTOR_SIZE / BLOCK_SIZE),
           (offset * SECTOR_SIZE / BLOCK_SIZE), part_list[i].attr1,
           part_list[i].attr2, part_list[i].attr3, part_list[i].which_flash,
           part_list[i].name);

    memcpy(part->name, part_list[i].name, strlen(part_list[i].name));
    if (offset > 0)
      part->offset = (offset * SECTOR_SIZE / BLOCK_SIZE);

    memcpy(output_buffer + bufwrsz, part, sizeof(struct partition_def));

    // Total output buffer size
    bufwrsz += sizeof(struct partition_def); 
    // Partition list size
    partition_layout_sz += sizeof(struct partition_def); 
    // Increase the offset on this partition
    offset += part_list[i].size;
  }

  int pending_size =  (PAGE_SIZE * 1024)-partition_layout_sz - sizeof(struct partition_table_header);
  memset(output_buffer + bufwrsz, 0xff, pending_size * sizeof(uint8_t));
  bufwrsz += pending_size;

  footer->magic1 = MBN_FOOTER_MG1;
  footer->magic2 = MBN_FOOTER_MG2;
  footer->version = MBN_FOOTER_VERSION;

  footer->crcval = xcrc32(output_buffer, bufwrsz, 0);

  memset(output_buffer + bufwrsz, 0xff, (PAGE_SIZE * 1024 * sizeof(uint8_t)));
  bufwrsz+=  (PAGE_SIZE * 1024 * sizeof(uint8_t));

  printf("3. Writing footer with CRC %.8x...\n", footer->crcval);
  memcpy(output_buffer + bufwrsz, footer, sizeof(struct mbn_crc_footer));
  bufwrsz += sizeof(struct mbn_crc_footer);
  free(footer);

  printf(" --> Partition list size is %i bytes. Fill up to page size of %i \n",
         partition_layout_sz, PAGE_SIZE * 1024);

  memset(output_buffer + bufwrsz, 0xff,
         ((PAGE_SIZE * 1024) - sizeof(struct mbn_crc_footer)) *
             sizeof(uint8_t));
  bufwrsz += ((PAGE_SIZE * 1024) - sizeof(struct mbn_crc_footer));

  int fdmbn = open(MBN_OUTPUT_FILE, O_CREAT | O_RDWR);
  if (fdmbn < 0) {
    fprintf(stderr, "Error opening %s\n", MBN_OUTPUT_FILE);
    return 0;
  }

  write(fdmbn, output_buffer, bufwrsz);

  close(fdmbn);
  return 0;
}

#include <endian.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define RELEASE_VER "0.0.2"

static const unsigned int crc32_table[] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
    0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
    0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
    0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
    0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
    0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
    0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
    0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
    0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
    0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
    0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
    0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
    0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
    0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
    0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
    0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
    0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};

uint32_t xcrc32(uint8_t *buf, int len, unsigned int init) {
  unsigned int crc = init;
  while (len--) {
    crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
    buf++;
  }
  printf("%.8x \n", crc);
  return crc;
}

/* MBN Header Magic */
/*  ac 9f 56 fe 7a 12 7f cd  04 00 00 00 01 00 00 00 */
#define MBN_HEADER_MAGIC1 0xFE569FAC
#define MBN_HEADER_MAGIC2 0xCD7F127A

/* Footer has two magic numbers and a CRC too */
#define MBN_FOOTER_MG1 0x9D41BEA1
#define MBN_FOOTER_MG2 0xF1DED2EA
#define MBN_FOOTER_VERSION 0x00000001
#define MBN_FOOTER_MG4 0x4FC78CDB
uint32_t crc_result;

/* Starting at 800 comes the GPT */
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
static const struct {
  uint8_t id;
  uint32_t size;
  uint8_t attr1;
  uint8_t attr2;
  uint8_t attr3;
  uint8_t which_flash;
  const char *name;
} part_list[] = {
    {0, 640, 0xff, 0x01, 0x00, 0x00, "0:SBL"}, //
    {0, 640, 0xff, 0x01, 0xff, 0x00, "0:MIBIB"}, //
    {0, 8704, 0xff, 0x01, 0xff, 0x00, "0:EFS2"}, //
    {0, 2560, 0xff, 0x01, 0x00, 0x00, "0:sys_rev"},
    {0, 640, 0xff, 0x01, 0x00, 0x00, "0:RAWDATA"},
    {0, 768, 0xff, 0x01, 0x00, 0x00, "0:TZ"}, //
    {0, 576, 0xff, 0x01, 0x00, 0x00, "0:RPM"}, //
    {0, 1024, 0xff, 0x01, 0x00, 0x00, "0:cust_info"},
    {0, 768, 0xff, 0x01, 0x00, 0x00, "0:aboot"}, //
    {0, 3968, 0xff, 0x01, 0x00, 0x00, "0:boot"}, //
    {0, 3968, 0xff, 0x01, 0x00, 0x00, "0:recovery"}, //
    {0, 27392, 0xff, 0x01, 0x00, 0x00, "0:modem"}, //
    {0, 320, 0xff, 0x01, 0x00, 0x00, "0:misc"},
    {0, 10368, 0xff, 0x01, 0x00, 0x00, "0:recoveryfs"}, //
    {0, 30720, 0xff, 0x01, 0x00, 0x00, "0:usr_data"}, //
    {0, 256, 0xff, 0x01, 0x00, 0x00, "0:sec"},
    {0, 37760, 0xff, 0x01, 0x00, 0x00, "0:system"}, //
};

static const struct {
  uint8_t id;
  uint32_t size;
  uint8_t attr1;
  uint8_t attr2;
  uint8_t attr3;
  uint8_t which_flash;
  const char *name;
} part_list_broadmobi[] = {
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
    {0, 25728, 0xff, 0x01, 0x00, 0x00, "0:EFS2APPS"},
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

  int partition_layout_sz = 0;

  printf("ID \t Sectors \t Bytes \t\t Bytes hex \t Sectors hex \t Offset \t "
         "Attr1 \t Attr2 \t Attr3 \t Other \t Name\n");
  for (int i = 0; i < (sizeof(part_list) / sizeof(part_list[0])); i++) {
    struct partition_def *part;
    part = calloc(1, sizeof(struct partition_def));
    part->size = (part_list[i].size * SECTOR_SIZE / BLOCK_SIZE);
    part->attr1 = part_list[i].attr1;
    part->attr2 = part_list[i].attr2;
    part->attr3 = part_list[i].attr3;

    part->offset = 0;
    printf("%i \t %.6i \t %.6i \t 0x%.8x \t 0x%.8x\t 0x%.8x\t  %x\t   %x\t   "
           "%x\t   %x\t %s\n",
           i, part_list[i].size, part_list[i].size * SECTOR_SIZE,
           part_list[i].size * SECTOR_SIZE,
           (part_list[i].size * SECTOR_SIZE / BLOCK_SIZE),
           (offset * SECTOR_SIZE / BLOCK_SIZE), part_list[i].attr1,
           part_list[i].attr2, part_list[i].attr3, part_list[i].which_flash,
           part_list[i].name);

    /* STRUCT */

    memcpy(part->name, part_list[i].name, strlen(part_list[i].name));
    if (offset > 0)
      part->offset = (offset * SECTOR_SIZE / BLOCK_SIZE);

    part->which_flash = part_list[i].which_flash;
    memcpy(output_buffer + bufwrsz, part, sizeof(struct partition_def));
    bufwrsz += sizeof(struct partition_def); // Total output buffer size
    partition_layout_sz +=
        sizeof(struct partition_def); // Partition list block size

    offset += part_list[i].size;

    free(part);
    part = NULL;
  }

  printf(" --> Partition list size is %i bytes. Fill up to page size of %i \n",
         partition_layout_sz, PAGE_SIZE * 1024);
  memset(output_buffer + bufwrsz, 0xff,
         ((2 * PAGE_SIZE * 1024) - partition_layout_sz) * sizeof(uint8_t));
  bufwrsz += ((2 * PAGE_SIZE * 1024) - partition_layout_sz - 16);

  footer->magic1 = MBN_FOOTER_MG1;
  footer->magic2 = MBN_FOOTER_MG2;
  footer->version = MBN_FOOTER_VERSION;
  footer->crcval = (xcrc32(output_buffer, bufwrsz, 0xFFFFFFFF)& 0xFFFFFFFF);

  // CRC Should be 0xdb 8c c7 4f for the broadmobi list
  // CRC Should be 0x02 61 5d de for the Quectel list
  printf("3. Writing footer with CRC %.8x...\n", footer->crcval) ;
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

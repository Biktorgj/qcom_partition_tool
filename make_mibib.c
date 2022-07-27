#include <endian.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define RELEASE_VER "0.0.1"

/* MBN Header Magic */
/*  ac 9f 56 fe 7a 12 7f cd  04 00 00 00 01 00 00 00 */
#define MBN_HEADER_MAGIC1 0xFE569FAC
#define MBN_HEADER_MAGIC2 0xCD7F127A

#define MBN_FOOTER_MG1 0x9D41BEA1
#define MBN_FOOTER_MG2 0xF1DED2EA
#define MBN_FOOTER_MG3 0x00000001
#define MBN_FOOTER_MG4 0x4FC78CDB
/* Starting at 800 comes the GPT*/
/* GPT Header Magic */
#define HEADER_MAGIC1 0x55EE73AA
#define HEADER_MAGIC2 0xE35EBDDB
#define HEADER_VERSION 4
#define MBN_OUTPUT_FILE "mibib.mbn"
#define GPT_OUTPUT_FILE "gpt0_main.bin"

#define SECTOR_SIZE 2048 // 2K
#define BLOCK_SIZE 131072 // 128KB
#define PAGE_SIZE 2

static const struct {
  uint8_t id;
  uint32_t size;
  uint8_t attr1;
  uint8_t attr2;
  uint8_t attr3;
  uint8_t which_flash;
  const char *name;
} partition_names[] = {
    {0, 640, 0xff, 0x01, 0x00, 0x00, "0:SBL"},   // 1310720 (640 sectors x 2048 byte sector size)
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

struct partition_def {      // 28 byte / partition
  char name[16];        // 0:ALL, 0:boot 0:SBL...
  uint32_t offset;      //(offset / blocksize)
  uint32_t length;
  uint8_t attr1;
  uint8_t attr2;
  uint8_t attr3;       // 0xFF in EFS, otherwise 0x00
  uint8_t which_flash; // 0x00
} __attribute__((packed));

struct partition_header {
    uint32_t header1;
    uint32_t header2;
    uint8_t version;
    uint32_t total_partitions;
    uint8_t padding[3];
} __attribute__((packed));


struct mbn_header {
    uint32_t header1;
    uint32_t header2;
    uint32_t version;
    uint32_t age; //1
} __attribute__((packed));

struct mbn_footer {
  uint32_t footer1;
  uint32_t footer2;
  uint32_t footer3;
  uint32_t footer4;
} __attribute__((packed));

int main(void) {
  uint32_t total_partitions =
      (sizeof(partition_names) / sizeof(partition_names[0]));
  uint32_t size, offset, attr1, attr2, attr3, which_flash;

  struct mbn_header *mbnheader;
  mbnheader = calloc(1, sizeof(struct mbn_header));
  mbnheader->header1 = MBN_HEADER_MAGIC1;
  mbnheader->header2 = MBN_HEADER_MAGIC2;
  mbnheader->version = HEADER_VERSION;
  mbnheader->age = 1;

  uint8_t padding = 0xff;
  uint8_t *dummy;
  dummy = &padding;
  int wr_bytes_gpt = 0;
  int wr_bytes_mbn = 0;

  struct partition_header *header;
  header = calloc(1, sizeof(struct partition_header));
  header->header1 = HEADER_MAGIC1;
  header->header2 = HEADER_MAGIC2;
  header->version = HEADER_VERSION;
  header->total_partitions = htobe32(total_partitions);

  offset = 0;

  int fdmbn = open(MBN_OUTPUT_FILE, O_CREAT | O_RDWR);
  if (fdmbn < 0) {
    fprintf(stderr, "Error opening %s\n", MBN_OUTPUT_FILE);
    return 0;
  }

  int fdgpt = open(GPT_OUTPUT_FILE, O_CREAT | O_RDWR);
  if (fdgpt < 0) {
    fprintf(stderr, "Error opening %s\n", GPT_OUTPUT_FILE);
    close(fdmbn);
    return 0;
  }

  printf("Tiny MIBIB Builder version %s\n", RELEASE_VER);
  printf("--------------------------------- \n");
  printf("Number of partitions: %i\n\n", total_partitions);
  printf("1. Build header\n");

  wr_bytes_mbn = write(fdmbn, mbnheader ,sizeof(struct mbn_header));
  
  printf("Bytes written: %i, pending %i\n", wr_bytes_gpt, (2048 - wr_bytes_gpt));
  for (uint32_t j = wr_bytes_mbn; j < 2048; j++) {
    wr_bytes_mbn+= write(fdmbn, dummy, sizeof(uint8_t));
  }


  wr_bytes_mbn+= write(fdmbn, header ,sizeof(struct partition_header));
  wr_bytes_gpt = write(fdgpt, header ,sizeof(struct partition_header));
  free(header);
  header = NULL;

  printf("2. Loop through partitions\n");
  printf(
      "ID \t Sectors \t Bytes \t\t Bytes hex \t Sectors hex \t Offset \t Attr1 \t Attr2 \t Attr3 \t Other \t Name\n");
  for (int i = 0; i < (sizeof(partition_names) / sizeof(partition_names[0]));
       i++) {
    struct partition_def *part;
    part = calloc(1, sizeof(struct partition_def));
    size = partition_names[i].size;
    attr1 = partition_names[i].attr1;
    attr2 = partition_names[i].attr2;
    attr3 = partition_names[i].attr3;
    which_flash = partition_names[i].which_flash;
    part->offset = 0;
    printf("%i \t %.6i \t %.6i \t 0x%.8x \t 0x%.8x\t 0x%.8x\t  %x\t   %x\t   %x\t   %x\t %s\n", i, size, size*SECTOR_SIZE, size*SECTOR_SIZE, (size * SECTOR_SIZE / BLOCK_SIZE),
           (offset * SECTOR_SIZE / BLOCK_SIZE), attr1, attr2, attr3, which_flash, partition_names[i].name);

    /* STRUCT */

    memcpy(part->name, partition_names[i].name, strlen(partition_names[i].name));
    if (offset > 0)
        part->offset = (offset * SECTOR_SIZE / BLOCK_SIZE);
 
    part->length = (size * SECTOR_SIZE / BLOCK_SIZE);
    part->attr1 = attr1;
    part->attr2 = attr2;
    part->attr3 = attr3;
    part->which_flash = which_flash;
    wr_bytes_mbn+= write(fdmbn, part ,sizeof(struct partition_def));
    wr_bytes_gpt+= write(fdgpt, part ,sizeof(struct partition_def));
    offset += partition_names[i].size;

    free(part);
    part = NULL;
  }

  struct mbn_footer *footer;
  footer = calloc(1, sizeof(struct mbn_footer));
  footer->footer1 = MBN_FOOTER_MG1;
  footer->footer2 = MBN_FOOTER_MG2;
  footer->footer3 = MBN_FOOTER_MG3;
  footer->footer4 = MBN_FOOTER_MG4;

  printf("%i bytes written. Fill up to 2K...\n", wr_bytes_mbn);
  for (uint32_t j = wr_bytes_mbn; j < 6144; j++) {
    wr_bytes_mbn+= write(fdmbn, dummy, sizeof(uint8_t));
  }
  printf("Writing footer...\n");
  wr_bytes_mbn+= write(fdmbn, footer, sizeof(struct mbn_footer));

  free(footer);
  printf("Bytes written: %i, pending %i\n", wr_bytes_gpt, (1310720 - wr_bytes_gpt));
  for (uint32_t j = wr_bytes_gpt; j < 1310720; j++) {
   wr_bytes_gpt+= write(fdgpt, dummy, sizeof(uint8_t));
  }

  for (uint32_t j = wr_bytes_mbn; j < 1310720; j++) {
    wr_bytes_mbn+= write(fdmbn, dummy, sizeof(uint8_t));
  }

  dummy = NULL;
  close(fdmbn);
  close(fdgpt);
  return 0;
}

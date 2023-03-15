#include "make_mibib.h"
#include <endian.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Make MIBIB */

struct partition_list *part_list;
uint8_t total_partitions = 0;
uint32_t total_blocks = 0;
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

uint32_t xcrc32(uint8_t *buf, int len, unsigned int init) {
  unsigned int crc = init;
  make_crc_table(0x04c11db7);
  while (len--) {
    crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
    buf++;
  }
  return crc;
}

void showHelp() {
  fprintf(stdout, " Utility to create flashable partition table layout files "
                  "for Qualcomm devices \n");
  fprintf(stdout, "Usage:\n");
  fprintf(stdout, "  make mibib -i INPUT_FILE -o OUTPUT_FILE -p page_size -b "
                  "block_size -s sector_size\n");
  fprintf(stdout, "Arguments: \n"
                  "\t-i: Input file with the partition table schema\n"
                  "\t-o: Output file (.mbn) that will be flashed later\n"
                  "\t-x: Output file (.xml) used to help you program the new partition structure (MAKE SURE YOU EDIT THIS FILE LATER)\n"
                  "\t-p: Page size: Used to properly align the file to what "
                  "the SBL expects (in K)\n"
                  "\t-s: Sector Size: Used to calculate the correct size from "
                  "the number of sectors (in bytes)\n"
                  "\t-b: Block size: Used to calculate the size from the "
                  "number of sectors (in bytes)\n");
  fprintf(stdout, "Example:\n"
                  "\t ./make_mibib -i examples/mdm9607-example-quectel-eg25-linux.conf -o "
                  "flashable_file.mbn -p 2 -s 2048 -b 131072\n");
  fprintf(stdout, "\n"
                  "************************************************************"
                  "*****************\n"
                  "                               WARNING\n"
                  "************************************************************"
                  "*****************\n"
                  " - You can *easily* kill your device with this tool\n"
                  " - Have a proper EFS backup\n"
                  " - Have a full EDL dump that *you know* you can restore if "
                  "something breaks\n"
                  " I'm in no way responsible for any damage done to any "
                  "device as a result of using this tool.\n");
}

int read_config_file(char *filename) {
  char line[128];
  FILE *fd = fopen(filename, "r");
  int line_no = 0;
  total_partitions = 0;
  if (fd == NULL) {
    fprintf(stderr, "Error opening %s!\n", filename);
    return 1;
  }
 
  part_list = malloc(MAX_PARTITION_NUM * sizeof(struct partition_list));
  fprintf(stdout, "* Scanning partition list inside the config file...\n");
  while (fgets(line, sizeof line, fd)) {
    char *token;
    char *rest = line;
    int count = 0;
    struct partition_list this_partition;
    if (line[0] != '#') {
      while ((token = strtok_r(rest, " ", &rest))) {
        switch (count) {
        case 0:
          this_partition.size = (uint32_t)atoi(token);
          total_blocks+= this_partition.size;
          break;
        case 1:
          this_partition.attr1 = (uint8_t)atoi(token);
          break;
        case 2:
          this_partition.attr2 = (uint8_t)atoi(token);
          break;
        case 3:
          this_partition.attr3 = (uint8_t)atoi(token);
          break;
        case 4:
          this_partition.which_flash = (uint8_t)atoi(token);
          break;
        case 5:
          strncpy(this_partition.name, token, MAX_PARTITION_NAME_SZ);
          this_partition.name[strlen(this_partition.name)-1] = '\0';
          break;
        default:
          fprintf(stderr, "Error: Malformed data in config file at line %i\n",
                  line_no);
          return 1;
        }
        count++;
      }
    }
    if (count == 6) {
      if (this_partition.size > 0 && strlen(this_partition.name) > 2) {
        fprintf(stdout, " -> Adding partition %i: %s\n", total_partitions, this_partition.name);
        part_list[total_partitions] = this_partition;
        total_partitions++;
      } else {
        fprintf(stderr, " ** Can't add partition %i, invalid param\n", total_partitions);
      }
    }
    line_no++;
  }
  fclose(fd);
  return 0;
}


int dump_xml(char *filename, uint32_t page_size, uint32_t block_size, uint32_t sector_size) {
  fprintf(stdout,"Dump XML Start\n");
  FILE *fd = fopen(filename, "w");
  uint32_t offset = 0;
  if (fd == NULL) {
    fprintf(stderr, "Error opening %s!\n", filename);
    return 1;
  }
  fprintf(fd, "<?xml version=\"1.0\" ?>\n"
              "<data>\n"
  "  <!--NOTE: This is an ** Autogenerated file **-->\n"
  "  <!--BlockSize = %i KB-->\n"
  "  <!--PageSize  = %i KB-->\n"
  "  <!--NUM_PARTITION_SECTORS  = %i-->\n",
    block_size/1024, page_size, block_size);

  for (uint8_t i = 0; i < total_partitions; i++) {
    fprintf(stdout, "XML: Processing partition %u: %s\n", i, part_list[i].name);
    fprintf(fd, "  <erase PAGES_PER_BLOCK=\"64\" SECTOR_SIZE_IN_BYTES=\"%i\" num_partition_sectors=\"%i\" physical_partition_number=\"0\" start_sector=\"%i\"/>\n",
        sector_size, part_list[i].size, offset);
    fprintf(fd, "  <program PAGES_PER_BLOCK=\"64\" SECTOR_SIZE_IN_BYTES=\"%i\" filename=\"REPLACE_NAME_%s\" num_partition_sectors=\"%i\" physical_partition_number=\"0\" start_sector=\"%i\"/>\n",
        sector_size, part_list[i].name, part_list[i].size, offset);
    offset+= part_list[i].size;
  }
    fprintf(fd, "</data>\n");

  fclose(fd);
  return 0;
}

int main(int argc, char **argv) {
  uint8_t *output_buffer;
  int bufwrsz = 0;
  int offset = 0;
  char *input_config_file;
  char *output_data_file;
  char *output_xml_file;
  uint32_t page_size = 0;
  uint32_t block_size = 0;
  uint32_t sector_size = 0;
  int c;
  fprintf(stdout, "Partitioning Tool for Qualcomm devices v%s\n", RELEASE_VER);
  if (argc < 10) {
    showHelp();
    return 0;
  }

  while ((c = getopt(argc, argv, "i:o:x:p:b:s:h")) != -1)
    switch (c) {
    case 'i':
      if (optarg == NULL) {
        fprintf(stderr,
                "You need to give me a list of partitions to work with\n");
      }
      input_config_file = optarg;
      break;
    case 'o':
      if (optarg == NULL) {
        fprintf(stderr, "You need to give me an output file\n");
      }
      output_data_file = optarg;
      break;
    case 'x':
      if (optarg == NULL) {
        fprintf(stderr, "You need to give me an output file name for the XML too!\n");
      }
      output_xml_file = optarg;
      break;
    case 'p':
      page_size = atoi(optarg);
      break;
    case 'b':
      block_size = atoi(optarg);
      break;
    case 's':
      sector_size = atoi(optarg);
      break;
    case 'h':
    default:
      showHelp();
      return 0;
    }

  fprintf(stdout, "Selected options:\n"
                  "\tInput file: %s\n"
                  "\tOutput file: %s\n"
                  "\tPage size: %i Kb\n"
                  "\tBlock size: %i bytes\n"
                  "\tSector size: %i bytes\n",
                  input_config_file,
                  output_data_file,
                  page_size,
                  block_size,
                  sector_size
                  );

  if (read_config_file(input_config_file) != 0)
    return 1;

  fprintf(stdout, "* Total number of partitions: %i\n\n", total_partitions);
  fprintf(stdout, "* Total blocks in flash: %u\n", total_blocks);

  /* Create a buffer large enough */
  /* Partition 1 is always the SBL, and Partition 2 should always
     be the MIBIB
  */
  if (total_partitions < 2) {
    fprintf(stderr, "ERROR: You're probably missing some partitions!\n");
    return 1;
  }
  output_buffer = calloc(part_list[1].size * sector_size, sizeof(uint8_t));

  struct mbn_header *mbnheader;
  mbnheader = calloc(1, sizeof(struct mbn_header));

  struct partition_table_header *table_header;
  table_header = calloc(1, sizeof(struct partition_table_header));

  struct mbn_crc_footer *footer;
  footer = calloc(1, sizeof(struct mbn_crc_footer));

  fprintf(stdout, "* Build MIBIB Header\n");
  /* MIBIB Header: 2 Magic numbers, version and "age", aligned to page size */
  mbnheader->header1 = MBN_HEADER_MAGIC1;
  mbnheader->header2 = MBN_HEADER_MAGIC2;
  mbnheader->version = HEADER_VERSION;
  mbnheader->age = 1;

  memcpy(output_buffer + bufwrsz, mbnheader, sizeof(struct mbn_header));
  bufwrsz += sizeof(struct mbn_header);
  fprintf(stdout, " -> Header size is %i bytes, filling to page size (%i bytes) \n",
         bufwrsz, page_size * 1024);
  memset(output_buffer + bufwrsz, 0xff,
         (((page_size * 1024) - bufwrsz) * sizeof(uint8_t)));
  bufwrsz += ((page_size * 1024) - bufwrsz) * sizeof(uint8_t);

  fprintf(stdout, "* Build Partition table header\n");
  /* Partition Table header: 2 magix numbers, version and partition count */
  table_header->header1 = HEADER_MAGIC1;
  table_header->header2 = HEADER_MAGIC2;
  table_header->version = HEADER_VERSION;
  table_header->total_partitions = total_partitions;

  memcpy(output_buffer + bufwrsz, table_header,
         sizeof(struct partition_table_header));
  bufwrsz += sizeof(struct partition_table_header);

  int partition_layout_sz = 0; // Include the header here
  struct partition_def *part;
  part = calloc(1, sizeof(struct partition_def));
  fprintf(stdout, " -> Partition table layout: \n");
  fprintf(stdout, "ID \t Sectors \t Bytes \t\t Bytes hex \t Sectors hex \t Offset \t "
         "Attr1 \t Attr2 \t Attr3 \t Other \t Name\n");

  for (int i = 0; i < table_header->total_partitions; i++) {
    memset(part, 0, sizeof(struct partition_def));
    part->size = (part_list[i].size * sector_size / block_size);
    part->attr1 = part_list[i].attr1;
    part->attr2 = part_list[i].attr2;
    part->attr3 = part_list[i].attr3;
    part->which_flash = part_list[i].which_flash;

    fprintf(stdout, "%i \t %.6i \t %.6i \t 0x%.8x \t 0x%.8x\t 0x%.8x\t  %x\t   %x\t   "
           "%x\t   %x\t %s\n",
           i, part_list[i].size, part_list[i].size * sector_size,
           part_list[i].size * sector_size,
           (part_list[i].size * sector_size / block_size),
           (offset * sector_size / block_size), part_list[i].attr1,
           part_list[i].attr2, part_list[i].attr3, part_list[i].which_flash,
           part_list[i].name);

    memcpy(part->name, part_list[i].name, strlen(part_list[i].name));
    if (offset > 0)
      part->offset = (offset * sector_size / block_size);

    memcpy(output_buffer + bufwrsz, part, sizeof(struct partition_def));

    // Total output buffer size
    bufwrsz += sizeof(struct partition_def);
    // Partition list size
    partition_layout_sz += sizeof(struct partition_def);
    // Increase the offset on this partition
    offset += part_list[i].size;
  }


  int pending_size = (page_size * 1024) - partition_layout_sz -
                     sizeof(struct partition_table_header);
   fprintf(stdout, " -> Table size is %i bytes, filling to page size (%i bytes) \n",
         partition_layout_sz, page_size * 1024);
  memset(output_buffer + bufwrsz, 0xff, pending_size * sizeof(uint8_t));
  bufwrsz += pending_size;

  fprintf(stdout, "* Calculate CRC and add the footer\n");
  /* First we calculate the CRC, then we add an empty page
     where the user partition table would go, and *then* 
     we add the footer to the file */
  footer->magic1 = MBN_FOOTER_MG1;
  footer->magic2 = MBN_FOOTER_MG2;
  footer->version = MBN_FOOTER_VERSION;
  footer->crcval = xcrc32(output_buffer, bufwrsz, 0);

  memset(output_buffer + bufwrsz, 0xff, (page_size * 1024 * sizeof(uint8_t)));
  bufwrsz += (page_size * 1024 * sizeof(uint8_t));

  fprintf(stdout, " -> Writing footer with CRC %.8x...\n", footer->crcval);
  memcpy(output_buffer + bufwrsz, footer, sizeof(struct mbn_crc_footer));
  bufwrsz += sizeof(struct mbn_crc_footer);
  free(footer);

   fprintf(stdout, " -> Footer size is %ld bytes, filling to page size (%i bytes) \n",
         sizeof(struct mbn_crc_footer), page_size * 1024);

  memset(output_buffer + bufwrsz, 0xff,
         ((page_size * 1024) - sizeof(struct mbn_crc_footer)) *
             sizeof(uint8_t));
  bufwrsz += ((page_size * 1024) - sizeof(struct mbn_crc_footer));

  fprintf(stdout, "* Saving file to disk\n");
  int fdmbn =
      open(output_data_file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
  if (fdmbn < 0) {
    fprintf(stderr, "Error opening %s\n", output_data_file);
    return 0;
  }

  write(fdmbn, output_buffer, bufwrsz);

  close(fdmbn);

  dump_xml(output_xml_file, page_size, block_size, sector_size);
  return 0;
}

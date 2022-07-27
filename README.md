# make_mibib
### Small utility to re-create the partition table for Qualcomm devices

This is still pretty much a work in progress, but functional enough to work in mdm9607 platforms.

I made this utility to be able to rename and add custom partitions to mdm9607 modems, but should work mostly as-is in any oldish Qualcomm device using a NAND and not requiring a signed partition table (make sure you have EDL access to the device just in case).

Build it with `make`

Run it with `./make_mibib`

It will generate an equivalent to `partition_complete_p2k_p128k.mbn` called `mibib.mbn` as included with your typical firmware update file for a qualcomm Modem.

The example partition layout is in the C file (will add support to read it from file soon)

```
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
```

This partition table mimics the Broadmobi BM818 modem, but replaces the last partition (EFS2APPS) with a boot and system partition to be able to hold a small linux install
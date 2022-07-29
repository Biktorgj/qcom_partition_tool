# Partitioning tool for Qualcomm based devices
### Small utility to re-create the partition table for Qualcomm devices

This is a small utility I created to be able to repartition mdm9x07 based modems, but should work with (any?most?) qualcomm device which has a `mibib` partition.

*****************************************************************************
    ## WARNING ##
                               
    You can *easily* kill your device with this tool
    Have a proper EFS backup
    Have a full EDL dump that *you know* you can restore if something breaks
#### I'm in no way responsible for any damage done to any device as a result of using this tool.
*****************************************************************************

#### Usage:

`make mibib -i INPUT_FILE -o OUTPUT_FILE -p page_size -b block_size -s sector_size`


#### Arguments: 

  -i: Input file with the partition table schema

  -o: Output file (.mbn) that will be flashed later

  -p: Page size: Used to properly align the file to what the SBL expects (in K)

  -s: Sector Size: Used to calculate the correct size from the number of sectors (in bytes)

  -b: Block size: Used to calculate the size from the number of sectors (in bytes)

#### Example:

For a NAND flash with a page size of 2048 (2K), sector size of 2048 bytes, and 128K blocks:
  
	 `./make_mibib -i examples/mdm9607-example-broadmobi-linux.conf -o flashable_file.mbn -p 2 -s 2048 -b 131072`

### Building it
1. Run `make`


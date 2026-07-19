__section("header") const struct cart_header {
 char title[21];
 char mode;
 char chipset;
 char ROM_size;
 char RAM_size;
 char country;
 char dev_id;
 char version;
 int checksum_compl;
 int checksum;
} __header_hirom_ntsc = {"ZATHRONTRIA TESTGAME", 0x21, 0x02, 9, 5, 0, 0, 0, 0xffff, 0x0000};


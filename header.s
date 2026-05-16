; For Calypsi

    .section snesheader,root
    .byte "ZATHRONTRIA TESTGAME " ;rom name 21 chars
    .byte 0x21  ; HiROM FastROM
    .byte 0x02  ; extra chips in cartridge, 00: no extra RAM; 02: RAM with battery
    .byte 0x09  ; ROM size (2^# in kB)
    .byte 0x05  ; backup RAM size
    .byte 0x01  ; US
    .byte 0x33  ; publisher id
    .byte 0x00  ; ROM revision number
    .word 0x0000  ; checksum of all bytes
    .word 0x0000  ; $FFFF minus checksum 
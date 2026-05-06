@echo off
REM Compile using VBCC 
set cmd_asm1=interrupt.c
set cmd_code=main.c system.c interrupt.asm interrupt_sub.c sram_management.c dma.c asm.c math_int.c level.c loop.c map.c obj.c routines.c hittest.c ani.c ani_bg.c ani_fixedspr.c ani_pal.c ani_pal_hdma.c spr.c spr_metaspr.c ui.c snd.c lz4.c 
set cmd_data=vars_memory.c data_strings.c data_palette.asm data_sprite.asm data_bg.asm data_ui.asm data_snd.asm data_samples.asm
set sfc_name=main

REM the below was a workaround for broken interrupt assembly code generation
REM uncomment if the source code is changed, then comment it back after editing the assembly code to fix a crash bug
REM vc +snes-hi -O4 -size -msfp4 -lms4 -S %cmd_asm1% 
vc +snes-hi -O4 -size -msfp4 -lms4 %cmd_code% %cmd_data% -o %sfc_name%.sfc

REM Also compile using calypsi for testing purposes
REM this will invoke make.
REM The makefile needs to be edited so that it doesn't error out if the ROM image was already made
REM TODO: there are still show-stopping bugs with the created ROM image. Although it runs, it has bugs that make the game unplayable.
REM make

pause
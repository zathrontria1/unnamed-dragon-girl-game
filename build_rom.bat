@echo off
REM Compile using VBCC 
set cmd_asm1=interrupt.c
set cmd_code=main.c system.c interrupt.asm interrupt_sub.c sram_management.c dma.c hdma.c asm.c math_int.c level.c loop.c loop_subscreen.c loop_cutscene.c map.c obj.c movement.c routines.c routines_player.c routines_enemy.c routines_enemy_ai.c hittest.c ani.c ani_bg.c ani_fixedspr.c ani_pal.c spr.c spr_metaspr.c ui.c ui_messagebox.c ui_vwf.c snd.c lz4.c gfx.c errorhandling.c vbcc-header-hi-ntsc.c
set cmd_data=vars_memory.c data_strings.c data_palette.asm data_sprite.asm data_bg.asm data_ui.asm data_snd.asm data_samples.asm data_csdata.asm
REM set cmd_data=vars_memory.c data_strings.c data_binary.asm
set sfc_name=main

REM build the startup code
vasm6502_oldstyle -816 -quiet -nowarn=62 -opt-branch -ldots -Fvobj -o .\startup-fast.o .\startup-fast.s

REM the below was a workaround for broken interrupt assembly code generation
REM uncomment if the source code is changed, then comment it back after editing the assembly code to fix a crash bug
REM vc +vlink-config -O4 -speed -msfp4 -lms4 --DFASTROM=1 -S %cmd_asm1% 

REM the below uses the custom startup code, which should let the game start faster
vc +vlink-config -O4 -size -msfp4 -lms4 --Mmapfile --DFASTROM=1 %cmd_code% %cmd_data% -o %sfc_name%_temp.sfc

REM the below uses the stock startup code provided by VBCC
REM vc +vlink-config-stockstartup -O4 -size -msfp4 -lms4 -no-inline-peephole %cmd_code% %cmd_data% -o %sfc_name%_temp.sfc

REM Also compile using calypsi for testing purposes
REM this will invoke make.
REM The makefile needs to be edited so that it doesn't error out if the ROM image was already made
REM TODO: there are still show-stopping bugs with the created ROM image. Although it runs, it has bugs that make the game unplayable.
REM make

REM compute and patch the checksum
python .\checksum.py --hirom %sfc_name%_temp.sfc -o %sfc_name%.sfc

pause
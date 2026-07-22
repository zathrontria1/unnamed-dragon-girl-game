@echo off
set "skip_pause="
if /i "%~1"=="--no-pause" set "skip_pause=1"

REM Compile using VBCC 
set cmd_asm1=src/core/interrupt.c
set cmd_code=src/main.c src/core/system.c src/core/crash_handler.c src/core/crash_handler.asm src/core/interrupt.asm src/core/interrupt_sub.c src/gameplay/sram_management.c src/core/dma.c src/core/hdma.c src/core/asm.c src/core/math_int.c src/gameplay/level.c src/gameplay/loop.c src/gameplay/loop_subscreen.c src/gameplay/loop_cutscene.c src/gameplay/loop_title.c src/gameplay/loop_gameover.c src/gameplay/map.c src/gameplay/obj.c src/gameplay/movement.c src/gameplay/routines.c src/gameplay/routines_player.c src/gameplay/routines_enemy.c src/gameplay/routines_enemy_ai.c src/gameplay/routines_boss.c src/gameplay/hittest.c src/graphics/ani.c src/graphics/ani_bg.c src/graphics/ani_fixedspr.c src/graphics/ani_pal.c src/graphics/spr.c src/graphics/spr_metaspr.c src/ui/ui.c src/ui/ui_messagebox.c src/ui/ui_vwf.c src/audio/snd.c src/graphics/lz4.c src/graphics/gfx.c src/core/errorhandling.c src/core/vbcc-header-hi-ntsc.c
set cmd_data=src/core/vars_memory.c src/core/vars_memory_aligned.asm src/data/data_strings.c src/data/data_palette.asm src/data/data_sprite.asm src/data/data_bg.asm src/data/data_ui.asm src/data/data_snd.asm src/data/data_samples.asm src/data/data_csdata.asm
REM set cmd_data=vars_memory.c data_strings.c data_binary.asm
REM build the startup code
vasm6502_oldstyle -816 -quiet -nowarn=62 -opt-branch -ldots -Fvobj -o .\src\core\startup-fast.o .\src\core\startup-fast.s

REM the below was a workaround for broken interrupt assembly code generation
REM uncomment if the source code is changed, then comment it back after editing the assembly code to fix a crash bug
REM vc +vlink-config -I. -Isrc -Isrc/core -Isrc/gameplay -Isrc/graphics -Isrc/ui -Isrc/audio -Isrc/data -O4 -speed -msfp4 -lms4 --DFASTROM=1 -S %cmd_asm1% 

REM The custom startup code should let the game start faster.
set sfc_name=main
set profile_define=
vc.exe +vlink-config -I. -Isrc -Isrc/core -Isrc/gameplay -Isrc/graphics -Isrc/ui -Isrc/audio -Isrc/data -O4 -size -msfp4 -lms4 --Mmapfile --DFASTROM=1 %profile_define% "--symfmt %%06x:%%s" "--symfile %sfc_name%.sym" %cmd_code% %cmd_data% -o %sfc_name%_temp.sfc

REM the below uses the stock startup code provided by VBCC
REM vc +vlink-config-stockstartup -I. -Isrc -Isrc/core -Isrc/gameplay -Isrc/graphics -Isrc/ui -Isrc/audio -Isrc/data -O4 -size -msfp4 -lms4 -no-inline-peephole %cmd_code% %cmd_data% -o %sfc_name%_temp.sfc

REM Also compile using calypsi for testing purposes
REM this will invoke make.
REM The makefile needs to be edited so that it doesn't error out if the ROM image was already made
REM TODO: there are still show-stopping bugs with the created ROM image. Although it runs, it has bugs that make the game unplayable.
REM make

REM Compute and patch the checksum.
call python .\tools\checksum.py --hirom --pad %sfc_name%_temp.sfc -o %sfc_name%.sfc
DEL %sfc_name%_temp.sfc

REM Display how much ROM is used.
call python .\tools\check_rom_size.py %sfc_name%.sfc

REM Fix symbols.
call python .\tools\reprocess_symbols.py %sfc_name%.sym
call python .\tools\record_build.py %sfc_name%.sfc release

set sfc_name=main_debug
set debug_define=-DDEBUG_ALL=1
vc.exe +vlink-config -I. -Isrc -Isrc/core -Isrc/gameplay -Isrc/graphics -Isrc/ui -Isrc/audio -Isrc/data -O4 -size -msfp4 -lms4 --Mmapfile --DFASTROM=1 %debug_define% "--symfmt %%06x:%%s" "--symfile %sfc_name%.sym" %cmd_code% %cmd_data% -o %sfc_name%_temp.sfc
call python .\tools\checksum.py --hirom --pad %sfc_name%_temp.sfc -o %sfc_name%.sfc
DEL %sfc_name%_temp.sfc
call python .\tools\check_rom_size.py %sfc_name%.sfc
call python .\tools\reprocess_symbols.py %sfc_name%.sym
call python .\tools\record_build.py %sfc_name%.sfc debug

if not defined skip_pause pause

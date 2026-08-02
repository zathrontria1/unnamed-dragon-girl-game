@echo off
set "skip_pause="
set "target=debug"
set "clean_mode="

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="--no-pause" set "skip_pause=1"
if /i "%~1"=="debug" set "target=debug"
if /i "%~1"=="release" set "target=release"
if /i "%~1"=="all" set "target=all"
if /i "%~1"=="clean" set "clean_mode=1"
shift
goto parse_args

:args_done

if defined clean_mode (
    call clean.bat --no-pause
    if not defined skip_pause pause
    exit /b 0
)

set cmd_code=src/main.c src/core/system.c src/core/crash_handler.c src/core/crash_handler.asm src/core/interrupt.asm src/core/interrupt_sub.c src/gameplay/sram_management.c src/core/dma.c src/core/hdma.c src/core/asm.c src/core/math_int.c src/gameplay/level.c src/gameplay/loop.c src/gameplay/loop_subscreen.c src/gameplay/loop_cutscene.c src/gameplay/loop_title.c src/gameplay/loop_gameover.c src/gameplay/map.c src/gameplay/obj.c src/gameplay/movement.c src/gameplay/routines.c src/gameplay/routines_player.c src/gameplay/routines_enemy.c src/gameplay/routines_enemy_ai.c src/gameplay/routines_boss.c src/gameplay/hittest.c src/graphics/ani.c src/graphics/ani_bg.c src/graphics/ani_fixedspr.c src/graphics/ani_pal.c src/graphics/spr.c src/graphics/spr_metaspr.c src/ui/ui.c src/ui/ui_messagebox.c src/ui/ui_vwf.c src/audio/snd.c src/graphics/lz4.c src/graphics/gfx.c src/core/errorhandling.c src/core/vbcc-header-hi-ntsc.c
set cmd_data=src/core/vars_memory.c src/core/vars_memory_aligned.asm src/data/data_strings.c src/data/data_palette.asm src/data/data_sprite.asm src/data/data_bg.asm src/data/data_ui.asm src/data/data_snd.asm src/data/data_samples.asm src/data/data_csdata.asm

REM Build the startup code
vasm6502_oldstyle -816 -quiet -nowarn=62 -opt-branch -ldots -Fvobj -o .\src\core\startup-fast.o .\src\core\startup-fast.s
if errorlevel 1 (
    echo Startup assembly failed!
    if not defined skip_pause pause
    exit /b 1
)

if "%target%"=="release" goto build_release
if "%target%"=="all" goto build_all

REM debug builds are done with -O4 optimisation level as the speed of compilation isn't very much different from -O2, and the only "thing" that -O2 gives is worse performance.
:build_debug
echo Compiling ROM [main_debug.sfc]...
set sfc_name=main_debug
set debug_define=-DDEBUG_ALL=1
vc.exe +vlink-config -I. -Isrc -Isrc/core -Isrc/gameplay -Isrc/graphics -Isrc/ui -Isrc/audio -Isrc/data -O4 -size -msfp4 -lms4 --Mmapfile_debug --DFASTROM=1 --DLZ4_DIRECT_CAST=1 %debug_define% "--symfmt %%06x:%%s" "--symfile %sfc_name%.sym" %cmd_code% %cmd_data% -o %sfc_name%_temp.sfc
if errorlevel 1 goto build_failed

python .\tools\checksum.py --hirom --pad %sfc_name%_temp.sfc -o %sfc_name%.sfc
if exist %sfc_name%_temp.sfc del %sfc_name%_temp.sfc
python .\tools\check_rom_size.py %sfc_name%.sfc
python .\tools\reprocess_symbols.py %sfc_name%.sym

if "%target%"=="debug" goto build_success
if "%target%"=="all" goto build_release

:build_release
echo Compiling ROM [main.sfc]...
set sfc_name=main
vc.exe +vlink-config -I. -Isrc -Isrc/core -Isrc/gameplay -Isrc/graphics -Isrc/ui -Isrc/audio -Isrc/data -O4 -size -msfp4 -lms4 --Mmapfile --DFASTROM=1 --DLZ4_DIRECT_CAST=1 "--symfmt %%06x:%%s" "--symfile %sfc_name%.sym" %cmd_code% %cmd_data% -o %sfc_name%_temp.sfc
if errorlevel 1 goto build_failed

python .\tools\checksum.py --hirom --pad %sfc_name%_temp.sfc -o %sfc_name%.sfc
if exist %sfc_name%_temp.sfc del %sfc_name%_temp.sfc
python .\tools\check_rom_size.py %sfc_name%.sfc
python .\tools\reprocess_symbols.py %sfc_name%.sym

goto build_success

:build_all
goto build_debug

:build_success
echo ROM build completed successfully.
if not defined skip_pause pause
exit /b 0

:build_failed
echo ROM build failed!
if not defined skip_pause pause
exit /b 1

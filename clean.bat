@echo off
set "skip_pause="
if /i "%~1"=="--no-pause" set "skip_pause=1"
if /i "%~2"=="--no-pause" set "skip_pause=1"

echo Cleaning build artifacts and generated assets...

if exist ".build" rd /s /q ".build"

del /f /q main.sfc main.sym main_debug.sfc main_debug.sym main_temp.sfc main_debug_temp.sfc mapfile mapfile_debug 2>nul

del /f /q palette\*.bin 2>nul
del /f /q bg\*.bin bg\*.lz4 2>nul
del /f /q sprites\*.bin sprites\*.lz4 sprites\*.dd 2>nul
del /f /q sprites\boss\*.bin sprites\boss\*.lz4 sprites\boss\*.dd 2>nul
del /f /q ui\*.bin ui\*.lz4 2>nul
del /f /q splash\*.bin splash\*.lz4 2>nul
del /f /q error\*.bin error\*.lz4 2>nul
del /f /q cutscene\intro\*.bin cutscene\intro\*.lz4 2>nul
del /f /q maps\*.h 2>nul

echo Clean completed.

if not defined skip_pause pause

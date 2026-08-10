@ECHO OFF
ca65 --debug-info -o sndeng.o sndeng.asm
ld65 -o sndeng.bin -C spc.cfg -m sndeng.map -Ln sndeng.lbl --dbgfile sndeng.dbg sndeng.o
pause
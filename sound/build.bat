@ECHO OFF
ca65 sndeng.asm
ld65 -o sndeng.bin -C spc.cfg sndeng.o
pause
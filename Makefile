C_SRCS = system.c interrupt.c interrupt_sub.c sram_management.c dma.c hdma.c asm.c \
math_int.c level.c loop.c loop_subscreen.c map.c obj.c movement.c routines.c routines_player.c routines_enemy.c \
routines_enemy_ai.c hittest.c ani.c ani_bg.c ani_fixedspr.c ani_pal.c spr.c spr_metaspr.c ui.c \
snd.c lz4.c gfx.c main.c vars_memory.c data_strings.c

ASM_SRCS = header.s data_palette.s data_sprite.s data_bg.s data_ui.s data_snd.s data_samples.s
	
OBJS = $(ASM_SRCS:%.s=obj-calypsi/%.o) $(C_SRCS:%.c=obj-calypsi/%.o)

obj-calypsi/%.o: %.s
	as65816 --target=snes --list-file=$(@:%.o=%.lst) -o $@ $<

obj-calypsi/%.o: %.c
	cc65816 --target=snes --speed -I include -O2 --list-file=$(@:%.o=%.lst) --data-model=large --code-model=large -o $@ $<

main_calypsi.elf: $(OBJS)
	del /q .\main_calypsi.elf
	del /q .\main_calypsi.raw
	del /q .\main_calypsi.sfc
	ln65816 HiROM.scm --output-format=raw --target=snes --debug -o $@ $^ --list-file=main_calypsi-debug.lst --semi-hosted --verbose --stack-size=512
	ren main_calypsi.raw main_calypsi.sfc
clean:
	del /q .\main_calypsi.elf
	del /q .\main_calypsi.raw
	del /q .\obj-calypsi\*.*

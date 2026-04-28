C_SRCS = main.c system.c interrupt.c sram_management.c dma.c \
	asm.c math_int.c level.c loop.c map.c obj.c routines.c hittest.c \
	ani.c ani_bg.c ani_fixedspr.c ani_pal.c ani_pal_hdma.c spr.c \
	spr_metaspr.c ui.c snd.c lz4.c vars_memory.c data_strings.c

ASM_SRCS = header.s data_palette.s data_sprite.s data_bg.s data_ui.s data_snd.s data_samples.s
	
OBJS = $(ASM_SRCS:%.s=obj-calypsi/%.o) $(C_SRCS:%.c=obj-calypsi/%.o)

obj-calypsi/%.o: %.s
	as65816 --target=snes --list-file=$(@:%.o=%.lst) -o $@ $<

obj-calypsi/%.o: %.c
	cc65816 --target=snes --speed -I include -O2 --list-file=$(@:%.o=%.lst) --data-model=large --code-model=large -o $@ $<

hello.elf: $(OBJS)
	ln65816 HiROM.scm --output-format=raw --target=snes --debug -o $@ $^ --list-file=hello-debug.lst --semi-hosted --verbose --stack-size=512
	ren hello.raw hello.sfc
clean:
	-rm $(OBJS)

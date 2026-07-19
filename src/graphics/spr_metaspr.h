#if VBCC_ASM == 1
NO_INLINE void SpriteEngine_AddMetaSprite(__reg("a/x") struct game_object * o, __reg("r0/r1") const struct spr_metaspr_definition * m);
#else
void SpriteEngine_AddMetaSprite(struct game_object * o, const struct spr_metaspr_definition * m);
#endif

#if VBCC_ASM == 1
NO_INLINE void SpriteEngine_AddMetaSprite_Back(__reg("a/x") struct game_object * o, __reg("r0/r1") const struct spr_metaspr_definition * m);
#else
void SpriteEngine_AddMetaSprite_Back(struct game_object * o, const struct spr_metaspr_definition * m);
#endif

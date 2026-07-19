uint16_t ObjectSystem_Move(struct game_object * o);

#if VBCC_ASM == 1
    NO_INLINE void ObjectSystem_MoveWithoutCollision(__reg("a/x") struct game_object * o);
#else
    void ObjectSystem_MoveWithoutCollision(struct game_object * o);
#endif

#if VBCC_ASM == 1
    NO_INLINE void ObjectSystem_MoveWithoutCollision_Fast(__reg("a/x") struct game_object * o);
#else
    void ObjectSystem_MoveWithoutCollision_Fast(struct game_object * o);
#endif

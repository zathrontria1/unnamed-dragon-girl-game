#if VBCC_ASM == 1
    NO_INLINE void Nmi_Primary();
    NO_INLINE void Nmi_Alternate();
#else
    void Nmi_Primary();
    void Nmi_Alternate();
#endif

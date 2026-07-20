#include "consts.h"

#if VBCC_ASM == 1
    NO_INLINE void Nmi_Primary();
    NO_INLINE void Nmi_Alternate();
    NO_INLINE void Nmi_Cutscene();
#else
    void Nmi_Primary();
    void Nmi_Alternate();
    void Nmi_Cutscene();
#endif

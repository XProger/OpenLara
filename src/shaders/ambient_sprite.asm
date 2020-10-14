#include "common.asm"

#ifdef VERTEX
    m4x4 oPos, aCoord, c[uViewProj]
    mov oD0, aLight
    mov oT0, aTexCoord
#else
    tex t0
    mov r0, t0
#endif
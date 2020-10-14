#include "common.asm"

#ifdef VERTEX
    mulMat(vPosition, aCoord, uViewProj)

    mul vColor, aLight, c[uMaterial]

; vTecCoord = (aTexCoord.xyz, 1)
    mov vTexCoord, aTexCoord
    mov vTexCoord.w, ONE
#else
    tex t0
    mul r0, t0, vColor
#endif
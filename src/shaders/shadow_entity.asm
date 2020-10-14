#include "common.asm"

#ifdef VERTEX
; variables
    #define pos    r7

; joint = int(aCoord.w)
    mov a0.x, aCoord.w

; pos = mulQuatPos(uBasis[joint], aCoord)
    mulQuatPos(pos, aCoord, a0.x)

; vPosition = uViewProj * pos
    mulMat(vPosition, pos, uViewProj)

; vTecCoord = (aTexCoord.xyz, 1) TODO check AKILL
    mov vTexCoord, aTexCoord
    mov vTexCoord.w, ONE
#else
    def c0, 0.0f, 0.0f, 0.0f, 1.0f
    mov r0, c0
#endif
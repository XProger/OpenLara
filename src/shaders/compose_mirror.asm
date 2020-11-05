#include "common.asm"

#ifdef VERTEX
; variables
    #define lv0     r0
    #define lv1     r1
    #define lv2     r2
    #define lv3     r3
    #define normal  r4
    #define tmp2    r5
    #define viewVec r6
    #define pos     r7

; joint = int(aCoord.w)
    mov a0.x, aCoord.w

; pos = mulQuatPos(uBasis[joint], aCoord)
    mulQuatPos(pos, aCoord, a0.x)

; vFog = length((uViewPos - pos) * uFogParams.w)
    applyFog(vFog, pos)

; vPosition = uViewProj * pos
    mulMat(vPosition, pos, uViewProj)

; viewVec = normalize(pos - uViewPos)
    sub viewVec, pos, c[uViewPos]
    normalize(viewVec)

; normal = mulQuat(uBasis, aNormal)
    mulQuat(normal, aNormal, a0.x)

; vTexCoord = reflect(viewVec, normal)
    reflect(vTexCoord, viewVec, normal)

; vColor = uMaterial
    mov tmp, c[uMaterial]
    mul tmp, tmp, HALF
    encodeColor(tmp)
#else
    tex t0
    applyColor(r0, t0)
#endif
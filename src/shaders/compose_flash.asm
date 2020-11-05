#include "common.asm"

#ifdef VERTEX
; variables
    #define lv0    r0
    #define lv1    r1
    #define lv2    r2
    #define lv3    r3
    #define normal r4
    #define att    r5
    #define light  r6
    #define pos    r7
    #define MAT_DIFFUSE  c[uMaterial].x
    #define MAT_EMISSIVE c[uMaterial].w

; pos = mulQuatPos(uBasis[0], aCoord)
    mulQuatPos(pos, aCoord, 0)

; vFog = length((uViewPos - pos) * uFogParams.w)
    applyFog(vFog, pos)

; vPosition = uViewProj * pos
    mulMat(vPosition, pos, uViewProj)

; vColor = (material.diffuse * aColor.xyz * 2.0 + material.emissive, 1.0)
    mul tmp, MAT_DIFFUSE, aColor
    add tmp, tmp, tmp
    add tmp, tmp, MAT_EMISSIVE
    mov tmp.w, MAT_EMISSIVE
    mul tmp, tmp, HALF
    encodeColor(tmp)

; vTecCoord = (aTexCoord.xyz, 1)
    mov vTexCoord, aTexCoord
    mov vTexCoord.w, ONE
#else
    tex t0
    applyColor(r0, t0)
#endif
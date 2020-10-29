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

; joint = int(aCoord.w)
    mov a0.x, aCoord.w

; pos = mulQuatPos(uBasis[joint], aCoord)
    mulQuatPos(pos, aCoord, a0.x)

; vFog = length((uViewPos - pos) * uFogParams.w)
    applyFog(vFog, pos)

; vPosition = uViewProj * pos
    mulMat(vPosition, pos, uViewProj)

; vColor = uMaterial
    mov vColor, ONE ; TODO

; normal = mulQuat(uBasis, aNormal)
    mov normal, aNormal
    mul pos.xyz, c[uBasis + 0 + a0.x], normal.zxyw
    mad pos.xyz, normal, c[uBasis + 0 + a0.x].zxyw, -pos
    mad pos.xyz, normal.yzxw, c[uBasis + 0 + a0.x].w, pos
    mul tmp.xyz, c[uBasis + 0 + a0.x].zxyw, pos
    mad pos.xyz, pos.yzxw, c[uBasis + 0 + a0.x].yzxw, -tmp
    mad vTexCoord, pos, TWO, normal
#else
    tex t0
    mul r0, t0, vColor
#endif
#include "common.asm"

#ifdef VERTEX
; variables
    #define lv1    r1
    #define lv2    r2
    #define lv3    r3
    #define normal r4
    #define att    r5
    #define light  r6
    #define pos    r7

; pos = mulQuatPos(uBasis[0], aCoord)
    mulQuatPos(pos, aCoord, 0)

; vFog = length((uViewPos - pos) * uFogParams.w)
    applyFog(vFog, pos)

; vPosition = uViewProj * pos
    mulMat(vPosition, pos, uViewProj)

; lighting
    ; lv[1..3] = (uLightPos[1..3].xyz - pos) * uLightColor[1..3].w;
    add lv1.xyz, c[uLightPos + 1], -pos
    add lv2.xyz, c[uLightPos + 2], -pos
    add lv3.xyz, c[uLightPos + 3], -pos
    mul lv1.xyz, c[uLightColor + 1].w, lv1
    mul lv2.xyz, c[uLightColor + 2].w, lv2
    mul lv3.xyz, c[uLightColor + 3].w, lv3

    ; att[1..3] = dot(lv[1..3], lv[1..3])
    mov att.x, ONE
    dp3 att.y, lv1, lv1
    dp3 att.z, lv2, lv2
    dp3 att.w, lv3, lv3

    ; att = max(0, 1 - att) / sqrt(att)
    rsq tmp.y, att.y
    rsq tmp.z, att.z
    rsq tmp.w, att.w

    add att, ONE, -att
    max att, ZERO, att
    mul att, tmp,  att

    ; light = max(0, dot(aNormal, lv[1..3]))
    mov light.x, ZERO
    dp3 light.y, lv1, aNormal
    dp3 light.z, lv2, aNormal
    dp3 light.w, lv3, aNormal

    ; light = max(0, light) * att
    max light, ZERO, light
    mul light, light, att

; vColor = (aLight + light[1..3] * uLightColor[1..3]) * aColor * 2
    mov att, aColor
    mov tmp, aLight
    mad tmp.xyz, light.y, c[uLightColor + 1], tmp
    mad tmp.xyz, light.z, c[uLightColor + 2], tmp
    mad tmp.xyz, light.w, c[uLightColor + 3], tmp
    mul att, att, tmp
    applyUnderwater(tmp, pos)
    add vColor, tmp, tmp

; vTecCoord = (aTexCoord.xyz, 1)
    mov vTexCoord, aTexCoord
    mov vTexCoord.w, ONE
#else
    tex t0
    mul r0, t0, vColor
#endif
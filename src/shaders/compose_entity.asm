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

    #define MAT_AMBIENT c[uMaterial].y
    #define MAT_ALPHA   c[uMaterial].w

; joint = int(aCoord.w)
    mov a0.x, aCoord.w

; pos = mulQuatPos(uBasis[joint], aCoord)
    mulQuatPos(pos, aCoord, a0.x)

; vFog = length((uViewPos - pos) * uFogParams.w)
    applyFog(vFog, pos)

; vPosition = uViewProj * pos
    mulMat(vPosition, pos, uViewProj)

; lighting
    ; lv[0..3] = (uLightPos[0..3].xyz - pos) * uLightColor[0..3].w;
    add lv0.xyz, c[uLightPos + 0], -pos
    add lv1.xyz, c[uLightPos + 1], -pos
    add lv2.xyz, c[uLightPos + 2], -pos
    add lv3.xyz, c[uLightPos + 3], -pos
    mul lv0.xyz, c[uLightColor + 0].w, lv0
    mul lv1.xyz, c[uLightColor + 1].w, lv1
    mul lv2.xyz, c[uLightColor + 2].w, lv2
    mul lv3.xyz, c[uLightColor + 3].w, lv3

    ; att[0..3] = dot(lv[0..3], lv[0..3])
    dp3 att.x, lv0, lv0
    dp3 att.y, lv1, lv1
    dp3 att.z, lv2, lv2
    dp3 att.w, lv3, lv3

    ; att = max(0, 1 - att) / sqrt(att)
    rsq tmp.x, att.x
    rsq tmp.y, att.y
    rsq tmp.z, att.z
    rsq tmp.w, att.w

    add att, ONE, -att
    max att, ZERO, att
    mul att, tmp,  att

    ; normal = mulQuat(uBasis[joint], aNormal)
    mulQuat(normal, aNormal, a0.x)

    ; light = max(0, dot(normal, lv[0..3]))
    dp3 light.x, lv0, normal
    dp3 light.y, lv1, normal
    dp3 light.z, lv2, normal
    dp3 light.w, lv3, normal

    ; light = max(0, light) * att
    max light, ZERO, light
    mul light, light, att

; vColor = aColor * material.alpha * (material.ambient + light[0..3] * uLightColor[0..3]) * 2
    mov att, aColor
    mul att, MAT_ALPHA, att
    mov tmp.xyz, MAT_AMBIENT
    mov tmp.w, ONE
    mad tmp.xyz, light.x, c[uLightColor + 0], tmp
    mad tmp.xyz, light.y, c[uLightColor + 1], tmp
    mad tmp.xyz, light.z, c[uLightColor + 2], tmp
    mad tmp.xyz, light.w, c[uLightColor + 3], tmp
    mul att, att, tmp
    applyUnderwater(att, pos)
    encodeColor(att)

; vTecCoord = (aTexCoord.xyz, 1)
    mov vTexCoord, aTexCoord
    mov vTexCoord.w, ONE
#else
    tex t0
    applyColor(r0, t0)
#endif

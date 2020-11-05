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
    #define size   r4
    #define vv     r6

    #define MAT_AMBIENT c[uMaterial].y
    #define MAT_ALPHA   c[uMaterial].w

; pos = (aTexCoord.zw * 32767, 0, 0)
    mov pos, ZERO
    mul pos.xy, aTexCoord.zw, MAX_SHORT

; size = mulQuatPos(uBasis[0], pos)
    mulQuatPos(size, pos, 0)

; pos = size + aCoord
    add pos, size, aCoord
    mov pos.w, ONE

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

    ; viewVec = uViewPos - pos
    add vv, c[uViewPos], -pos

    ; normal = normalize(viewVec)
    dp3 tmp.x, vv, vv
    rsq tmp.x, tmp.x
    mul normal, vv, tmp.x

    ; light = max(0, dot(normal, lv[1..3]))
    mov light.x, MAT_AMBIENT
    dp3 light.y, lv1, normal
    dp3 light.z, lv2, normal
    dp3 light.w, lv3, normal

    ; light = max(0, light) * att
    max light, ZERO, light
    mul light, light, att

; vColor = vec4(aLight.rgb * aLight.a, aLight.a) * 2
    mov att, aLight
    mad att.xyz, light.x, c[uLightColor + 0], att
    mad att.xyz, light.y, c[uLightColor + 1], att
    mad att.xyz, light.z, c[uLightColor + 2], att
    mad att.xyz, light.w, c[uLightColor + 3], att
    mul att.xyz, att, att.w
    applyUnderwater(att, pos)
    encodeColor(att)

; vTecCoord = (aTexCoord.xyz, 1)
    mov vTexCoord, aTexCoord
    mov vTexCoord.w, ONE
#else
    tex t0
    applyColor(r0, t0)
#endif
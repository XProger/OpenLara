#include "common.asm"

#ifdef VERTEX
    mulMat(vPosition, aCoord, uViewProj)


    #ifdef GRAYSCALE
        mov vColor.xyz, c[uParam].xyz
        mov vColor.w, ONE
    #else
        mov vColor, aLight
    #endif

; vTecCoord = (aTexCoord.xyz, 1)
    #ifdef UPSCALE
        mov vTexCoord, aTexCoord ; TODO + 0.5 * (1 / uParam.xy)
    #endif

    #ifdef DOWNSAMPLE
        mov vTexCoord, aTexCoord
    #endif

    #ifdef GRAYSCALE
        mov tmp, c[uParam].wwww
        mad vTexCoord, tmp, HALF, aTexCoord
    #endif

    #ifdef BLUR
        mov tmp, c[uParam].wwww
        mad vTexCoord, tmp, HALF, aTexCoord
    #endif

    #ifdef ANAGLYPH
        mov vTexCoord, aTexCoord
    #endif

    mov vTexCoord.w, ONE
#else
    def c0, 0.299f, 0.587f, 0.114f, 0.0f

    tex t0

    #ifdef UPSCALE
        mul r0, t0, vColor
    #endif

    #ifdef DOWNSAMPLE
        mul r0, t0, vColor
    #endif

    #ifdef GRAYSCALE
        dp3 r1.rgb, t0, c0
        mov r0, vColor
        mul r0.rgb, r0.rgb, r1.rgb
    #endif

    #ifdef BLUR ; TODO four tap blur
        mul r0, t0, vColor
    #endif

    #ifdef ANAGLYPH
        mul r0, t0, vColor
    #endif

#endif
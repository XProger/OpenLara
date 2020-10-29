#ifdef VERTEX
    xvs.1.1

    #define vPosition   oPos
    #define vColor      oD0
    #define vTexCoord   oT0
    #define vFog        oFog
#else
    ps.1.0

    #define vColor      v0
#endif

; attributes
#define aCoord          v0
#define aNormal         v1
#define aTexCoord       v2
#define aColor          v3
#define aLight          v4

; constants
#define uParam          0
#define uTexParam       1
#define uViewProj       2
#define uBasis          6
#define uLightProj      70
#define uMaterial       74
#define uAmbient        75
#define uFogParams      81
#define uViewPos        82
#define uLightPos       83
#define uLightColor     87
#define uRoomSize       91
#define uPosScale       92

#define ZERO      c94.xxxx
#define HALF      c94.yyyy
#define ONE       c94.zzzz
#define TWO       c94.wwww
#define UW_COLOR  c95.xyz
#define MAX_SHORT c95.wwww

#define tmp       r8

#define mulMat(dst, src, matrix)        \
    mul tmp, c[matrix + 0], src.x       \
    mad tmp, src.y, c[matrix + 1], tmp  \
    mad tmp, src.z, c[matrix + 2], tmp  \
    mad dst, src.w, c[matrix + 3], tmp

#define mulQuatPos(dst, src, joint)                     \
    mul dst.xyz, c[uBasis + joint], src.zxyw            \
    mad dst.xyz, src, c[uBasis + joint].zxyw, -dst      \
    mad dst.xyz, src.yzxw, c[uBasis + joint].w, dst     \
    mul tmp.xyz, c[uBasis + joint].zxyw, dst            \
    mad dst.xyz, dst.yzxw, c[uBasis + joint].yzxw, -tmp \
    mad dst.xyz, dst, TWO, src                          \
    add dst.xyz, c[uBasis + joint + 1], dst             \
    mov dst.w, c[uBasis + joint + 1].w

#define applyFog(dst, src)                  \
    add tmp.xyz, c[uViewPos].xyz, -src.xyz  \
    mul tmp.xyz, tmp.xyz, c[uFogParams].www \
    dp3 tmp.x, tmp.xyz, tmp.xyz             \
    rsq tmp.x, tmp.x                        \
    rcp dst.x, tmp.x

#ifdef UNDERWATER
    #define applyUnderwater(dst)    \
        mul dst.xyz, dst.xyz, UW_COLOR
#else
    #define applyUnderwater(dst)
#endif

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
#define uCosCoeff       92
#define uAngles         93

#define ZERO      c94.xxxx
#define HALF      c94.yyyy
#define ONE       c94.zzzz
#define TWO       c94.wwww
#define UW_COLOR  c95.xyz
#define MAX_SHORT c95.wwww
#define WAVE_SIZE c[uAngles].wwww   ; 1 / 1024
#define WAVE_LUM  c[uAngles].zzzz   ; 0.75

#define tmp       r8

#define mulMat(dst, src, matrix)        \
    mul tmp, c[matrix + 0], src.x       \
    mad tmp, src.y, c[matrix + 1], tmp  \
    mad tmp, src.z, c[matrix + 2], tmp  \
    mad dst, src.w, c[matrix + 3], tmp

#define mulQuat(dst, src, joint)                        \
    mul dst.xyz, c[uBasis + joint], src.zxyw            \
    mad dst.xyz, src, c[uBasis + joint].zxyw, -dst      \
    mad dst.xyz, src.yzxw, c[uBasis + joint].w, dst     \
    mul tmp.xyz, c[uBasis + joint].zxyw, dst            \
    mad dst.xyz, dst.yzxw, c[uBasis + joint].yzxw, -tmp \
    mad dst.xyz, dst, TWO, src                          \
    mov dst.w, ONE

#define mulQuatPos(dst, src, joint)                     \
    mul dst.xyz, c[uBasis + joint], src.zxyw            \
    mad dst.xyz, src, c[uBasis + joint].zxyw, -dst      \
    mad dst.xyz, src.yzxw, c[uBasis + joint].w, dst     \
    mul tmp.xyz, c[uBasis + joint].zxyw, dst            \
    mad dst.xyz, dst.yzxw, c[uBasis + joint].yzxw, -tmp \
    mad dst.xyz, dst, TWO, src                          \
    add dst.xyz, c[uBasis + joint + 1], dst             \
    mov dst.w, c[uBasis + joint + 1].w

#define reflect(dst, a, b)      \
    dp3 tmp.x, a, b             \
    add tmp.x, tmp.x, tmp.x     \
    mad dst, -tmp.xxxx, b, a

#define normalize(v)    \
    dp3 tmp.x, v, v     \
    rsq tmp.x, tmp.x    \
    mul v, v, tmp.xxxx

#define applyFog(dst, src)                  \
    add tmp.xyz, c[uViewPos].xyz, -src.xyz  \
    mul tmp.xyz, tmp.xyz, c[uFogParams].www \
    dp3 tmp.x, tmp.xyz, tmp.xyz             \
    rsq tmp.x, tmp.x                        \
    rcp dst.x, tmp.x

#define encodeColor(v)      \
    mul vColor, v, HALF

#define applyColor(dst, src)    \
    mul dst, src, vColor        \
    add dst, dst, dst           \
    add dst, dst, dst

#define cos(dst, ang)                           \
    mad dst.x, ang, c[uAngles].y, c[uAngles].x  \
    expp dst.yw, dst.xxxx                       \
    add dst.x, dst.y, -c[uAngles].x             \
    mul dst, dst.wwxx, dst.wxxx                 \
    mul dst.w, dst.w, dst.y                     \
    mul dst, dst, dst                           \
    dp4 dst.x, dst, c[uCosCoeff]

#ifdef UNDERWATER
    #define applyUnderwater(dst, pos)   \
        dp3 tmp.x, pos, WAVE_SIZE       \
        add tmp.x, tmp.x, c[uParam].x   \
        cos(tmp, tmp.x)                 \
        max tmp.x, tmp.x, -tmp.x        \
        mul tmp.x, tmp.x, WAVE_LUM      \
        add tmp.x, tmp.x, HALF          \
        mul dst.xyz, dst.xyz, tmp.xxx   \
        mul dst.xyz, dst.xyz, UW_COLOR

#else
    #define applyUnderwater(dst, pos)
#endif

    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    IMPORT matrixPtr
    IMPORT viewportRel
    IMPORT divTable
    IMPORT gVertices
    EXPORT projectVertices_asm

vCount  RN r0
mx      RN vCount
my      RN r1
mz      RN r2
x       RN r3
y       RN r4
z       RN r5
dz      RN r6
minXY   RN r7
maxXY   RN r8
minZ    RN r9
maxZ    RN r12
m       RN dz
vp      RN dz
last    RN r10
vertex  RN r11
divLUT  RN lr

FIXED_SHIFT     EQU 14
PROJ_SHIFT      EQU 4
CLIP_SHIFT      EQU 8
CLIP_LEFT       EQU (1 << 0)
CLIP_RIGHT      EQU (1 << 1)
CLIP_TOP        EQU (1 << 2)
CLIP_BOTTOM     EQU (1 << 3)
CLIP_FAR        EQU (1 << 4)
CLIP_NEAR       EQU (1 << 5)

DIV_TABLE_END   EQU (1025 - 1)
VIEW_DIST       EQU (1024 * 10)             ; max = DIV_TABLE_END << PROJ_SHIFT
VIEW_MIN        EQU (256 << CLIP_SHIFT)
VIEW_MAX        EQU (VIEW_DIST << CLIP_SHIFT)

MulManyVec3Mat33_F16 EQU (0x50000 + 2)

projectVertices_asm
        stmfd sp!, {r4-r11, lr}

        ldr m, =matrixPtr
        ldr m, [m]
        ldr vertex, =gVertices
        mov r3, vCount
        add vCount, vCount, vCount, lsl #1
        add last, vertex, vCount, lsl #2    ; last = gVertices + vCount * 12

        mov r2, m
        mov r1, vertex
        mov r0, vertex
        swi MulManyVec3Mat33_F16

        add m, m, #36           ; skip 3x3 matrix part
        ldmia m, {mx, my, mz}   ; get view space offset from matrix
        ldr divLUT, =divTable
        ldr vp, =viewportRel
        ldmia vp, {minXY, maxXY}

        mov minZ, #VIEW_MIN
        mov maxZ, #VIEW_MAX

        mov my, my, asr #FIXED_SHIFT
        mov my, my, lsl #PROJ_SHIFT

        mov mz, mz, asr #FIXED_SHIFT
        mov mz, mz, lsl #CLIP_SHIFT

loop    ldmia vertex, {x, y, z}     ; read unpacked vertex

        add x, x, mx, asr #FIXED_SHIFT
        add y, my, y, lsl #PROJ_SHIFT ; extra shift for min/max cmp with hi half-word
        add z, mz, z, lsl #CLIP_SHIFT ; add some bits for the clipping flags

    ; check z clipping
        cmp z, minZ
        orrlt z, minZ, #CLIP_NEAR
        cmp z, maxZ
        orrgt z, maxZ, #CLIP_FAR

    ; projection
        mov dz, z, lsr #(PROJ_SHIFT + CLIP_SHIFT)   ; z is positive
        ldr dz, [divLUT, dz, lsl #2]
        mul x, dz, x
        mul y, dz, y
        mov x, x, asr #(16 - PROJ_SHIFT)
        ; keep y shifted by 16 for min/max cmp

    ; check xy clipping
        cmp x, minXY, asr #16
        orrlt z, z, #CLIP_LEFT
        cmp y, minXY, lsl #16
        orrlt z, z, #CLIP_TOP
        cmp x, maxXY, asr #16
        orrgt z, z, #CLIP_RIGHT
        cmp y, maxXY, lsl #16
        orrgt z, z, #CLIP_BOTTOM

        mov y, y, asr #16

        stmia vertex!, {x, y, z}    ; store projected vertex
        cmp vertex, last
        blt loop

done    ldmfd sp!, {r4-r11, pc}
    END

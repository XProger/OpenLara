    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

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

projectVertices_asm
        stmfd sp!, {r4-r11, lr}

        ldr m, =gMatrixPtr
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

loop    ldmia vertex, {x, y, z}     ; read transformed vertex

        add x, x, mx, asr #FIXED_SHIFT
        add y, my, y, lsl #PROJ_SHIFT ; extra shift for min/max cmp with hi half-word
        add z, mz, z, lsl #CLIP_SHIFT ; add some bits for the clipping flags

    ; check z clipping
        cmp z, minZ
        orrle z, minZ, #CLIP_NEAR
        cmp z, maxZ
        orrge z, maxZ, #CLIP_FAR

    ; projection
        mov dz, z, lsr #(PROJ_SHIFT + CLIP_SHIFT)   ; z is positive
        ldr dz, [divLUT, dz, lsl #2]
        mul x, dz, x
        mul y, dz, y
        mov x, x, asr #(16 - PROJ_SHIFT)
        ; keep y shifted by 16 for min/max cmp

    ; check xy clipping
        cmp x, minXY, asr #16
        orrle z, z, #CLIP_LEFT
        cmp y, minXY, lsl #16
        orrle z, z, #CLIP_TOP
        cmp x, maxXY, asr #16
        orrge z, z, #CLIP_RIGHT
        cmp y, maxXY, lsl #16
        orrge z, z, #CLIP_BOTTOM

        mov y, y, asr #16

        stmia vertex!, {x, y, z}    ; store projected vertex
        cmp vertex, last
        blt loop

        ldmfd sp!, {r4-r11, pc}
    END

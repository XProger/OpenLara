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
minX    RN r7
minY    RN r8
maxX    RN r9
maxY    RN r12
m       RN minX
vp      RN minX
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
        ldmia vp, {minX, minY, maxX, maxY}

loop    ldmia vertex, {x, y, z}     ; read unpacked vertex

        add x, x, mx, asr #FIXED_SHIFT
        add y, y, my, asr #FIXED_SHIFT
        add z, z, mz, asr #FIXED_SHIFT

        mov z, z, lsl #CLIP_SHIFT   ; add some bits for the clipping flags

    ; check z clipping
        cmp z, #VIEW_MIN
        movlt z, #VIEW_MIN
        orrlt z, z, #CLIP_NEAR
        cmp z, #VIEW_MAX
        movgt z, #VIEW_MAX
        orrgt z, z, #CLIP_FAR

    ; projection
        mov dz, z, lsr #(PROJ_SHIFT + CLIP_SHIFT)   ; z is positive
        ldr dz, [divLUT, dz, lsl #2]
        mul x, dz, x
        mul y, dz, y
        mov x, x, asr #(16 - PROJ_SHIFT)
        mov y, y, asr #(16 - PROJ_SHIFT)

    ; check xy clipping
        cmp x, minX
        orrlt z, z, #CLIP_LEFT
        cmp y, minY
        orrlt z, z, #CLIP_TOP
        cmp x, maxX
        orrgt z, z, #CLIP_RIGHT
        cmp y, maxY
        orrgt z, z, #CLIP_BOTTOM

        stmia vertex!, {x, y, z}    ; store projected vertex
        cmp vertex, last
        blt loop

done    ldmfd sp!, {r4-r11, pc}
    END

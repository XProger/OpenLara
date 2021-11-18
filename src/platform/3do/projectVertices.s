    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    IMPORT matrixPtr
    IMPORT viewportRel
    IMPORT divTable
    IMPORT gVertices
    EXPORT projectVertices_asm

vCount  RN r0
mx      RN r0
my      RN r1
mz      RN r2
vx      RN r3
vy      RN r4
vz      RN r5
x       RN r6
y       RN r7
z       RN r8

minX    RN mx
minY    RN my
maxX    RN mz
maxY    RN vx
dz      RN vy

vp      RN r9
m       RN r10
vertex  RN r11
divLUT  RN r12
last    RN lr

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
VIEW_DIST       EQU (1024 * 10)
VIEW_MIN        EQU (256 << CLIP_SHIFT)
VIEW_MAX        EQU (VIEW_DIST << CLIP_SHIFT)

projectVertices_asm
        stmfd sp!, {r4-r11, lr}

        ldr divLUT, =divTable
        ldr vertex, =gVertices
        ldr vp, =viewportRel
        ldr m, =matrixPtr
        ldr m, [m]
        add vCount, vCount, vCount, lsl #1
        add last, vertex, vCount, lsl #2    ; last = gVertices + vCount * 12

loop    ldmia vertex, {vx, vy, vz}  ; read unpacked vertex

    ; transform x
        ldmia m!, {mx, my, mz, x}
        mla x, mx, vx, x
        mla x, my, vy, x
        mla x, mz, vz, x
    ; transform y
        ldmia m!, {mx, my, mz, y}
        mla y, mx, vx, y
        mla y, my, vy, y
        mla y, mz, vz, y
    ; transform z
        ldmia m!, {mx, my, mz, z}
        mla z, mx, vx, z
        mla z, my, vy, z
        mla z, mz, vz, z

        sub m, m, #48               ; restore matrixPtr

        mov x, x, asr #FIXED_SHIFT
        mov y, y, asr #FIXED_SHIFT
        mov z, z, asr #FIXED_SHIFT
        mov z, z, lsl #CLIP_SHIFT   ; add some bits for the clipping flags

    ; check z clipping
        cmp z, #VIEW_MIN
        movlt z, #VIEW_MIN
        orrlt z, z, #CLIP_NEAR
        cmp z, #VIEW_MAX
        movge z, #VIEW_MAX
        orrge z, z, #CLIP_FAR

    ; projection
        mov dz, z, lsr #(PROJ_SHIFT + CLIP_SHIFT)
        cmp dz, #DIV_TABLE_END
        movge dz, #DIV_TABLE_END
        ldr dz, [divLUT, dz, lsl #2]
        mul x, dz, x
        mul y, dz, y
        mov x, x, asr #12
        mov y, y, asr #12

    ; check xy clipping
        ldmia vp, {minX, minY, maxX, maxY}
        cmp x, minX
        orrlt z, z, #CLIP_LEFT
        cmp y, minY
        orrlt z, z, #CLIP_TOP
        cmp x, maxX
        orrge z, z, #CLIP_RIGHT
        cmp y, maxY
        orrge z, z, #CLIP_BOTTOM

        stmia vertex!, {x, y, z}    ; store projected vertex
        cmp vertex, last
        blt loop

done    ldmfd sp!, {r4-r11, pc}
    END

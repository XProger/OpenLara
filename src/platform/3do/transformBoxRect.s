    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    IMPORT projectVertices_asm

    EXPORT transformBoxRect_asm

boxArg  RN r0
rectArg RN r1
divLUT  RN r2
m       RN r3
vx      RN r4
vy      RN r5
vz      RN r6
x       RN r7
y       RN r8
z       RN r9
rMinX   RN r10
rMinY   RN r11
rMaxX   RN r12
rMaxY   RN lr

bz      RN divLUT
offset  RN m
xx      RN rMinX
yy      RN rMinY
zz      RN rMaxX
min     RN rMaxY
max     RN rMaxY
w       RN x
h       RN y
mx      RN boxArg
my      RN rectArg
mz      RN divLUT

minX    RN x
minY    RN y
minZ    RN z
maxX    RN mx
maxY    RN my
maxZ    RN mz

MAX_X   EQU (0 * 3 * 4)
MIN_X   EQU (1 * 3 * 4)
MAX_Y   EQU (2 * 3 * 4)
MIN_Y   EQU (3 * 3 * 4)
MAX_Z   EQU (4 * 3 * 4)
MIN_Z   EQU (5 * 3 * 4)
SIZE    EQU (6 * 3 * 4)

    MACRO
$index  project $dx, $dy, $dz
        add offset, sp, $dz
        ldmia offset, {x, y, z}

        add offset, sp, $dy
        ldmia offset, {vx, vy, vz}
        add x, x, vx
        add y, y, vy
        add z, z, vz

        add offset, sp, $dx
        ldmia offset, {vx, vy, vz}
        add z, z, vz

        ; check z clipping
        sub offset, z, #VIEW_MIN_F
        cmp offset, #(VIEW_MAX_F - VIEW_MIN_F)
        bhi $index.skip

        add x, x, vx
        add y, y, vy

        mov z, z, lsr #(FIXED_SHIFT + PROJ_SHIFT)   ; z is positive
        ldr z, [divLUT, z, lsl #2]
        mul x, z, x
        mul y, z, y

        cmp x, rMinX
        movlt rMinX, x
        cmp y, rMinY
        movlt rMinY, y
        cmp x, rMaxX
        movgt rMaxX, x
        cmp y, rMaxY
        movgt rMaxY, y
$index.skip
    MEND

transformBoxRect_asm
        ldr m, =matrixPtr
        ldr m, [m]
        ldr bz, [m, #(11 * 4)]
        sub bz, bz, #VIEW_MIN_F
        cmp bz, #(VIEW_MAX_F - VIEW_MIN_F)
        movhi r0, #0
        movhi pc, lr

        stmfd sp!, {rectArg, r4-r11, lr}

        ldmia boxArg, {xx, yy, zz}

        add m, m, #(12 * 4)

        ; pre-transform min/max Z
        ldmdb m!, {mx, my, mz, vx, vy, vz}
        mov min, zz, asr #16
        mla minX, min, mx, vx
        mla minY, min, my, vy
        mla minZ, min, mz, vz
        mov minX, minX, asr #FIXED_SHIFT
        mov minY, minY, asr #FIXED_SHIFT

        mov max, zz, lsl #16
        mov max, max, asr #16
        mla maxX, max, mx, vx
        mla maxY, max, my, vy
        mla maxZ, max, mz, vz
        mov maxX, maxX, asr #FIXED_SHIFT
        mov maxY, maxY, asr #FIXED_SHIFT
        stmdb sp!, {maxX, maxY, maxZ, minX, minY, minZ}

        ; pre-transform min/max Y
        ldmdb m!, {mx, my, mz}

        mov min, yy, asr #16
        mul minX, mx, min
        mul minY, my, min
        mul minZ, mz, min
        mov minX, minX, asr #FIXED_SHIFT
        mov minY, minY, asr #FIXED_SHIFT

        mov max, yy, lsl #16
        mov max, max, asr #16
        mul maxX, max, mx
        mul maxY, max, my
        mul maxZ, max, mz
        mov maxX, maxX, asr #FIXED_SHIFT
        mov maxY, maxY, asr #FIXED_SHIFT
        stmdb sp!, {maxX, maxY, maxZ, minX, minY, minZ}

        ; pre-transform min/max X
        ldmdb m!, {mx, my, mz}

        mov min, xx, asr #16
        mul minX, mx, min
        mul minY, my, min
        mul minZ, mz, min
        mov minX, minX, asr #FIXED_SHIFT
        mov minY, minY, asr #FIXED_SHIFT

        mov max, xx, lsl #16
        mov max, max, asr #16
        mul maxX, max, mx
        mul maxY, max, my
        mul maxZ, max, mz
        mov maxX, maxX, asr #FIXED_SHIFT
        mov maxY, maxY, asr #FIXED_SHIFT
        stmdb sp!, {maxX, maxY, maxZ, minX, minY, minZ}

        ldr divLUT, =divTable
        mov rMinX, #MAX_INT32
        mov rMinY, #MAX_INT32
        mov rMaxX, #MIN_INT32
        mov rMaxY, #MIN_INT32

_0      project #MIN_X, #MIN_Y, #MIN_Z
_1      project #MAX_X, #MIN_Y, #MIN_Z
_2      project #MIN_X, #MAX_Y, #MIN_Z
_3      project #MAX_X, #MAX_Y, #MIN_Z
_4      project #MIN_X, #MIN_Y, #MAX_Z
_5      project #MAX_X, #MIN_Y, #MAX_Z
_6      project #MIN_X, #MAX_Y, #MAX_Z
_7      project #MAX_X, #MAX_Y, #MAX_Z

_done   mov w, #(FRAME_WIDTH >> 1)
        mov h, #(FRAME_HEIGHT >> 1)
        add rMinX, w, rMinX, asr #(16 - PROJ_SHIFT)
        add rMinY, h, rMinY, asr #(16 - PROJ_SHIFT)
        add rMaxX, w, rMaxX, asr #(16 - PROJ_SHIFT)
        add rMaxY, h, rMaxY, asr #(16 - PROJ_SHIFT)

        add sp, sp, #SIZE
        ldmfd sp!, {rectArg}

        stmia rectArg, {rMinX, rMinY, rMaxX, rMaxY}

        mov r0, #1
        ldmfd sp!, {r4-r11, pc}
    END

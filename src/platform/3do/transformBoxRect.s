    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    IMPORT projectVertices_asm

    EXPORT transformBoxRect_asm

boxArg  RN r0
rectArg RN r1
m       RN r2
vert    RN r3
vptr    RN r4
rect    RN r5   ; must be in r4-r6
minX    RN boxArg
maxX    RN rectArg
minY    RN m
maxY    RN r6
minZ    RN r12
maxZ    RN lr

xx      RN maxX
yy      RN maxY
zz      RN maxZ
rMinX   RN boxArg
rMinY   RN rectArg
rMaxX   RN m
rMaxY   RN maxY
vx      RN vptr
vy      RN minZ
vz      RN maxZ

INT_MIN EQU 0x80000000
INT_MAX EQU 0x7FFFFFFF

    MACRO
$index  check
        ldmia vert!, {vx, vy, vz}
        tst vz, #(CLIP_NEAR | CLIP_FAR)
        bne $index.skip
        cmp vx, rMinX
        movlt rMinX, vx
        cmp vy, rMinY
        movlt rMinY, vy
        cmp vx, rMaxX
        movgt rMaxX, vx
        cmp vy, rMaxY
        movgt rMaxY, vy
$index.skip
    MEND

transformBoxRect_asm
        ldr m, =matrixPtr
        ldr m, [m]
        ldr m, [m, #(11 * 4)]
        cmp m, #VIEW_MIN_F
        movlt r0, #0
        movlt pc, lr
        cmp m, #VIEW_MAX_F
        movge r0, #0
        movge pc, lr

        stmfd sp!, {r4-r6, lr}

        mov rect, rectArg   ; to use after projectVertices_asm call
        ldmia boxArg, {xx, yy, zz}

        mov minX, xx, asr #16
        mov maxX, xx, lsl #16
        mov maxX, maxX, asr #(16 - F16_SHIFT)
        mov minX, minX, lsl #2

        mov minY, yy, asr #16
        mov maxY, yy, lsl #16
        mov maxY, maxY, asr #(16 - F16_SHIFT)
        mov minY, minY, lsl #2

        mov minZ, zz, asr #16
        mov maxZ, zz, lsl #16
        mov maxZ, maxZ, asr #(16 - F16_SHIFT)
        mov minZ, minZ, lsl #2

        ldr vptr, =gVertices

        mov vert, vptr
        stmia vert!, {minX, minY, minZ}
        stmia vert!, {maxX, minY, minZ}
        stmia vert!, {minX, maxY, minZ}
        stmia vert!, {maxX, maxY, minZ}
        stmia vert!, {minX, minY, maxZ}
        stmia vert!, {maxX, minY, maxZ}
        stmia vert!, {minX, maxY, maxZ}
        stmia vert!, {maxX, maxY, maxZ}

        mov r0, #8
        bl projectVertices_asm  ; TODO compare with non-SWI version

        mov rMinX, #INT_MAX
        mov rMinY, #INT_MAX
        mov rMaxX, #INT_MIN
        mov rMaxY, #INT_MIN

        mov vert, vptr
_0      check
_1      check
_2      check
_3      check
_4      check
_5      check
_6      check
_7      check

_done   add rMinX, rMinX, #(FRAME_WIDTH >> 1)
        add rMinY, rMinY, #(FRAME_HEIGHT >> 1)
        add rMaxX, rMaxX, #(FRAME_WIDTH >> 1)
        add rMaxY, rMaxY, #(FRAME_HEIGHT >> 1)

        stmia rect, {rMinX, rMinY, rMaxX, rMaxY}

        mov r0, #1
        ldmfd sp!, {r4-r6, pc}
    END

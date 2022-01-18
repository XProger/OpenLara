    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT sphereIsVisible_asm

x       RN r0
y       RN r1
z       RN r2
r       RN r3
vx      RN r4
vy      RN r5
vz      RN r6
mx      RN r7
my      RN r8
mz      RN r12

m       RN lr
divLUT  RN m
vp      RN m
vMinXY  RN z
vMaxXY  RN r

rMinX   RN vx
rMaxX   RN x
rMinY   RN vy
rMaxY   RN y

sphereIsVisible_asm
        stmfd sp!, {r4-r8, lr}

        ldr m, =gMatrixPtr
        ldr m, [m]

        ldmia m!, {mx, my, mz}
        mul vx, mx, x
        mul vy, my, x
        mul vz, mz, x
        ldmia m!, {mx, my, mz}
        mla vx, mx, y, vx
        mla vy, my, y, vy
        mla vz, mz, y, vz
        ldmia m!, {mx, my, mz}
        mla vx, mx, z, vx
        mla vy, my, z, vy
        mla vz, mz, z, vz

        cmp vz, #VIEW_MAX_F
        bhi _fail

        mov x, vx, asr #FIXED_SHIFT
        mov y, vy, asr #FIXED_SHIFT
        mov z, vz, lsr #(FIXED_SHIFT + PROJ_SHIFT)

        ldr divLUT, =divTable
        ldr z, [divLUT, z, lsl #2]
        mul x, z, x
        mul y, z, y
        mul r, z, r

        mov x, x, asr #(16 - PROJ_SHIFT)
        mov y, y, lsl #(PROJ_SHIFT)

        sub rMinX, x, r, lsr #(16 - PROJ_SHIFT)
        add rMaxX, x, r, lsr #(16 - PROJ_SHIFT)
        sub rMinY, y, r, lsl #PROJ_SHIFT
        add rMaxY, y, r, lsl #PROJ_SHIFT

        ldr vp, =viewportRel
        ldmia vp, {vMinXY, vMaxXY}

        cmp rMaxX, vMinXY, asr #16
        blt _fail
        cmp rMaxY, vMinXY, lsl #16
        blt _fail
        cmp rMinX, vMaxXY, asr #16
        bgt _fail
        cmp rMinY, vMaxXY, lsl #16
        bgt _fail

        mov r0, #1
        ldmfd sp!, {r4-r8, pc}

_fail   mov r0, #0
        ldmfd sp!, {r4-r8, pc}
    END

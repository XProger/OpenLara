    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    IMPORT phd_sin_asm

    EXPORT matrixRotateX_asm
    EXPORT matrixRotateY_asm
    EXPORT matrixRotateZ_asm
    EXPORT matrixRotateYQ_asm
    EXPORT matrixRotateYXZ_asm

    MACRO
        sincos $angle, $sin, $cos
        ldr $sin, =gSinCosTable
        ldr $sin, [$sin, $angle, lsl #2]
        mov $cos, $sin, lsl #16
        mov $cos, $cos, asr #16
        mov $sin, $sin, asr #16
    MEND

    MACRO
        rotxy $x, $y, $sin, $cos, $t
        mul $t, $y, $cos
        mla $t, $x, $sin, $t
        mul $x, $cos, $x
        rsb $y, $y, #0
        mla $x, $y, $sin, $x
        mov $y, $t, asr #FIXED_SHIFT
        mov $x, $x, asr #FIXED_SHIFT
    MEND

angle   RN r0
ex0     RN r1
ex1     RN r2
ex2     RN r3
ey0     RN r4
ey1     RN r5
ey2     RN r6
s       RN r7
c       RN r12
v       RN lr
m       RN angle

matrixRotateX_asm
        stmfd sp!, {r4-r7, lr}

        mov angle, angle, lsl #16
        mov angle, angle, lsr #20

        sincos angle, s, c

        ldr m, =gMatrixPtr
        ldr m, [m]
        add m, m, #(3 * 4)

        ldmia m, {ex0, ex1, ex2, ey0, ey1, ey2}

        rotxy ey0, ex0, s, c, v
        rotxy ey1, ex1, s, c, v
        rotxy ey2, ex2, s, c, v

        stmia m, {ex0, ex1, ex2, ey0, ey1, ey2}

        ldmfd sp!, {r4-r7, pc}


matrixRotateY_asm
        stmfd sp!, {r4-r7, lr}

        mov angle, angle, lsl #16
        mov angle, angle, lsr #20

        sincos angle, s, c

        ldr m, =gMatrixPtr
        ldr m, [m]

        ldmia m!, {ex0, ex1, ex2}
        add m, m, #(3 * 4)
        ldmia m!, {ey0, ey1, ey2}

        rotxy ex0, ey0, s, c, v
        rotxy ex1, ey1, s, c, v
        rotxy ex2, ey2, s, c, v

        stmdb m!, {ey0, ey1, ey2}
        sub m, m, #(3 * 4)
        stmdb m!, {ex0, ex1, ex2}

        ldmfd sp!, {r4-r7, pc}


matrixRotateZ_asm
        stmfd sp!, {r4-r7, lr}

        mov angle, angle, lsl #16
        mov angle, angle, lsr #20

        sincos angle, s, c

        ldr m, =gMatrixPtr
        ldr m, [m]

        ldmia m, {ex0, ex1, ex2, ey0, ey1, ey2}

        rotxy ey0, ex0, s, c, v
        rotxy ey1, ex1, s, c, v
        rotxy ey2, ex2, s, c, v

        stmia m, {ex0, ex1, ex2, ey0, ey1, ey2}

        ldmfd sp!, {r4-r7, pc}


angleX  RN r0
angleY  RN r1
angleZ  RN r2
e00     RN r3
e10     RN r4
e20     RN r5
e01     RN r6
e11     RN r7
e21     RN r8
e02     RN r9
e12     RN r10
e22     RN r11
tmp     RN r12
sinX    RN lr
sinY    RN sinX
sinZ    RN sinX
cosX    RN angleX
cosY    RN angleY
cosZ    RN angleZ
mask    RN tmp
mm      RN tmp

matrixRotateYXZ_asm
        mov mask, #0xFF
        orr mask, mask, #0xF00  ; mask = 0xFFF

        and angleX, mask, angleX, lsr #4
        and angleY, mask, angleY, lsr #4
        and angleZ, mask, angleZ, lsr #4

        orr mask, angleX, angleY
        orrs mask, mask, angleZ
        moveq pc, lr

        stmfd sp!, {r4-r11, lr}

        ldr mm, =gMatrixPtr
        ldr mm, [mm]
        ldmia mm, {e00, e10, e20, e01, e11, e21, e02, e12, e22}

_rotY   cmp angleY, #0
        beq _rotX

        sincos angleY, sinY, cosY

        rotxy e00, e02, sinY, cosY, tmp
        rotxy e10, e12, sinY, cosY, tmp
        rotxy e20, e22, sinY, cosY, tmp

_rotX   cmp angleX, #0
        beq _rotZ

        sincos angleX, sinX, cosX

        rotxy e02, e01, sinX, cosX, tmp
        rotxy e12, e11, sinX, cosX, tmp
        rotxy e22, e21, sinX, cosX, tmp

_rotZ   cmp angleZ, #0
        beq _done

        sincos angleZ, sinZ, cosZ

        rotxy e01, e00, sinZ, cosZ, tmp
        rotxy e11, e10, sinZ, cosZ, tmp
        rotxy e21, e20, sinZ, cosZ, tmp

_done   ldr mm, =gMatrixPtr
        ldr mm, [mm]
        stmia mm, {e00, e10, e20, e01, e11, e21, e02, e12, e22}
        ldmfd sp!, {r4-r11, pc}

q   RN r0
mx  RN r1
my  RN r2

mx0 RN r3
mx1 RN r4
mx2 RN r5

my0 RN q
my1 RN r12
my2 RN lr

matrixRotateYQ_asm
        cmp q, #2
        moveq pc, lr

        stmfd sp!, {r4-r5, lr}

        ldr mx, =gMatrixPtr
        ldr mx, [mx]
        add my, mx, #(6 * 4)

        ldmia mx, {mx0, mx1, mx2}

        cmp q, #1
        beq q_1
        cmp q, #3
        beq q_3

q_0     ldmia my, {my0, my1, my2}
        rsb mx0, mx0, #0
        rsb mx1, mx1, #0
        rsb mx2, mx2, #0
        rsb my0, my0, #0
        rsb my1, my1, #0
        rsb my2, my2, #0
        stmia mx, {mx0, mx1, mx2}
        stmia my, {my0, my1, my2}
        ldmfd sp!, {r4-r5, pc}

q_1     ldmia my, {my0, my1, my2}
        stmia mx, {my0, my1, my2}
        rsb mx0, mx0, #0
        rsb mx1, mx1, #0
        rsb mx2, mx2, #0
        stmia my, {mx0, mx1, mx2}
        ldmfd sp!, {r4-r5, pc}

q_3     ldmia my, {my0, my1, my2}
        stmia my, {mx0, mx1, mx2}
        rsb my0, my0, #0
        rsb my1, my1, #0
        rsb my2, my2, #0
        stmia mx, {my0, my1, my2}
        ldmfd sp!, {r4-r5, pc}

    END

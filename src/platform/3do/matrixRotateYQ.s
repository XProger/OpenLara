    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT matrixRotateYQ_asm

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

        ldr mx, =matrixPtr
        ldr mx, [mx]
        add my, mx, #(6 * 4)

        ldmia mx, {mx0, mx1, mx2}

        cmp q, #1
        beq q_1
        cmp q, #3
        beq q_3

        ; column-major

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

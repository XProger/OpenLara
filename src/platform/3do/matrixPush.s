    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT matrixPush_asm

e0  RN r0
e1  RN r1
e2  RN r2
e3  RN r3
m   RN e0
src RN r12
dst RN lr

matrixPush_asm
        stmfd sp!, {lr}
        ldr m, =gMatrixPtr
        ldr src, [m]
        add dst, src, #(12 * 4)
        str dst, [m]

        ldmia src!, {e0, e1, e2, e3}
        stmia dst!, {e0, e1, e2, e3}

        ldmia src!, {e0, e1, e2, e3}
        stmia dst!, {e0, e1, e2, e3}

        ldmia src!, {e0, e1, e2, e3}
        stmia dst!, {e0, e1, e2, e3}

        ldmfd sp!, {pc}
    END

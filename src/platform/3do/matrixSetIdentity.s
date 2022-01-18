    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT matrixSetIdentity_asm

e0  RN r0
e1  RN r1
e2  RN r2
e3  RN r3
m   RN r12

matrixSetIdentity_asm
        ldr m, =gMatrixPtr
        ldr m, [m]
        mov e0, #0x4000
        mov e1, #0
        mov e2, #0
        mov e3, #0

        ; column-major
        ; e0 e1 e2
        ; e3 e0 e1
        ; e2 e3 e0
        ; e1 e2 e3

        stmia m!, {e0, e1, e2, e3}
        stmia m!, {e0, e1, e2, e3}
        stmia m!, {e0, e1, e2, e3}

        mov pc, lr
    END

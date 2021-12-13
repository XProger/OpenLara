    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT matrixSetBasis_asm

dst RN r0
src RN r1

e0  RN r2
e1  RN r3
e2  RN r12

matrixSetBasis_asm
        ; column-major
        ; e0 e1 e2
        ; e0 e1 e2
        ; e0 e1 e2
        ; x  y  z

        ldmia src!, {e0, e1, e2}
        stmia dst!, {e0, e1, e2}

        ldmia src!, {e0, e1, e2}
        stmia dst!, {e0, e1, e2}

        ldmia src!, {e0, e1, e2}
        stmia dst!, {e0, e1, e2}

        mov pc, lr
    END

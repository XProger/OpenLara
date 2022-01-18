    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT matrixTranslateRel_asm
    EXPORT matrixTranslateAbs_asm
    EXPORT matrixTranslateSet_asm

x   RN r0
y   RN r1
z   RN r2
m   RN r3
e0  RN r4
e1  RN r5
e2  RN r6
dx  RN r7
dy  RN r12
dz  RN lr

matrixTranslateRel_asm
        stmfd sp!, {r4-r7, lr}

        ldr m, =gMatrixPtr
        ldr m, [m]
        add m, m, #(12 * 4)

        ldmdb m!, {dx, dy, dz}

        ldmdb m!, {e0, e1, e2}
        mla dx, e0, z, dx
        mla dy, e1, z, dy
        mla dz, e2, z, dz

        ldmdb m!, {e0, e1, e2}
        mla dx, e0, y, dx
        mla dy, e1, y, dy
        mla dz, e2, y, dz

        ldmdb m!, {e0, e1, e2}
        mla dx, e0, x, dx
        mla dy, e1, x, dy
        mla dz, e2, x, dz

        add m, m, #(9 * 4)
        stmia m!, {dx, dy, dz}

        ldmfd sp!, {r4-r7, pc}


matrixTranslateAbs_asm
        stmfd sp!, {r4-r7, lr}

        ldr m, =gCameraViewPos
        ldmia m, {e0, e1, e2}
        sub x, x, e0
        sub y, y, e1
        sub z, z, e2

        ldr m, =gMatrixPtr
        ldr m, [m]

        ldmia m!, {e0, e1, e2}
        mul dx, e0, x
        mul dy, e1, x
        mul dz, e2, x

        ldmia m!, {e0, e1, e2}
        mla dx, e0, y, dx
        mla dy, e1, y, dy
        mla dz, e2, y, dz

        ldmia m!, {e0, e1, e2}
        mla dx, e0, z, dx
        mla dy, e1, z, dy
        mla dz, e2, z, dz

        stmia m!, {dx, dy, dz}

        ldmfd sp!, {r4-r7, pc}


matrixTranslateSet_asm
        stmfd sp!, {r4-r7, lr}

        ldr m, =gMatrixPtr
        ldr m, [m]

        ldmia m!, {e0, e1, e2}
        mul dx, e0, x
        mul dy, e1, x
        mul dz, e2, x

        ldmia m!, {e0, e1, e2}
        mla dx, e0, y, dx
        mla dy, e1, y, dy
        mla dz, e2, y, dz

        ldmia m!, {e0, e1, e2}
        mla dx, e0, z, dx
        mla dy, e1, z, dy
        mla dz, e2, z, dz

        stmia m, {dx, dy, dz}

        ldmfd sp!, {r4-r7, pc}

    END

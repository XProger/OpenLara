    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT matrixLerp_asm

n       RN r0
pmul    RN r1
pdiv    RN r2
m0      RN r3
m1      RN r4
m2      RN r5
n0      RN r6
n1      RN r7
n2      RN r12
m       RN lr
tmp     RN m0
divLUT  RN m0

    MACRO
    load
        ldmia m, {m0, m1, m2}
        ldmia n!, {n0, n1, n2}
    MEND

    MACRO
    store
        stmia m!, {m0, m1, m2}
    MEND

    MACRO ; a = (a + b) / 2
    _1_2
        load
        add m0, m0, n0
        add m1, m1, n1
        add m2, m2, n2
        mov m0, m0, asr #1
        mov m1, m1, asr #1
        mov m2, m2, asr #1
        store
    MEND

    MACRO ; a = a + (b - a) / 4
    _1_4
        load
        sub n0, n0, m0
        sub n1, n1, m1
        sub n2, n2, m2
        add m0, m0, n0, asr #2
        add m1, m1, n1, asr #2
        add m2, m2, n2, asr #2
        store
    MEND

    MACRO ; a = b - (b - a) / 4
    _3_4
        load
        sub m0, n0, m0
        sub m1, n1, m1
        sub m2, n2, m2
        sub m0, n0, m0, asr #2
        sub m1, n1, m1, asr #2
        sub m2, n2, m2, asr #2
        store
    MEND

    MACRO ; a = a + (b - a) * mul / div
    _X_Y
        load
        sub n0, n0, m0
        sub n1, n1, m1
        sub n2, n2, m2
        mul n0, pmul, n0
        mul n1, pmul, n1
        mul n2, pmul, n2
        add m0, m0, n0, asr #8
        add m1, m1, n1, asr #8
        add m2, m2, n2, asr #8
        store
    MEND

    MACRO
    lerp $func
        $func    ; e00, e10, e20
        $func    ; e01, e11, e21
        $func    ; e02, e12, e22
    MEND

matrixLerp_asm
        stmfd sp!, {r4-r7, lr}
        ldr m, =gMatrixPtr
        ldr m, [m]
check_2
        cmp pdiv, #2
        beq m1_d2
check_4
        cmp pdiv, #4
        bne mX_dY
        cmp pmul, #1
        beq m1_d4
        cmp pmul, #2
        beq m1_d2 ; 2/4 = 1/2
m3_d4
        lerp _3_4
        b done
m1_d4
        lerp _1_4
        b done
m1_d2
        lerp _1_2
        b done
mX_dY
        ldr divLUT, =divTable
        ldr tmp, [divLUT, pdiv, lsl #2]
        mul tmp, pmul, tmp
        mov pmul, tmp, asr #8
        lerp _X_Y
done    ldmfd sp!, {r4-r7, pc}
    END

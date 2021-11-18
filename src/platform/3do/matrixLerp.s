    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    IMPORT matrixPtr
    IMPORT divTable
    EXPORT matrixLerp_asm

n       RN r0
pmul    RN r1
pdiv    RN r2
m0      RN r3
m1      RN r4
m2      RN r5
m3      RN r6
n0      RN r7
n1      RN r8
n2      RN r9
n3      RN r12
m       RN lr
tmp     RN m0

    MACRO
    next_row
        add m, m, #16
        add n, n, #16
    MEND

    MACRO
    load
        ldmia m, {m0, m1, m2, m3}
        ldmia n, {n0, n1, n2, n3}
    MEND

    MACRO
    store
        stmia m, {m0, m1, m2, m3}
    MEND

    MACRO ; a = (a + b) / 2
    _1_2
        load
        add m0, m0, n0
        add m1, m1, n1
        add m2, m2, n2
        add m3, m3, n3
        mov m0, m0, asr #1
        mov m1, m1, asr #1
        mov m2, m2, asr #1
        mov m3, m3, asr #1
        store
    MEND

    MACRO ; a = a + (b - a) / 4
    _1_4
        load
        sub n0, n0, m0
        sub n1, n1, m1
        sub n2, n2, m2
        sub n3, n3, m3
        add m0, m0, n0, asr #2
        add m1, m1, n1, asr #2
        add m2, m2, n2, asr #2
        add m3, m3, n3, asr #2
        store
    MEND

    MACRO ; a = b - (b - a) / 4
    _3_4
        load
        sub m0, n0, m0
        sub m1, n1, m1
        sub m2, n2, m2
        sub m3, n3, m3
        sub m0, n0, m0, asr #2
        sub m1, n1, m1, asr #2
        sub m2, n2, m2, asr #2
        sub m3, n3, m3, asr #2
        store
    MEND

    MACRO ; a = a + (b - a) * mul / div
    _X_Y
        load
        sub n0, n0, m0
        sub n1, n1, m1
        sub n2, n2, m2
        sub n3, n3, m3
        mul n0, pmul, n0
        mul n1, pmul, n1
        mul n2, pmul, n2
        mul n3, pmul, n3
        add m0, m0, n0, asr #8
        add m1, m1, n1, asr #8
        add m2, m2, n2, asr #8
        add m3, m3, n3, asr #8
        store
    MEND

    MACRO
    lerp $func
        $func    ; e00, e01, e02, e03
        next_row
        $func    ; e10, e11, e12, e13
        next_row
        $func    ; e20, e21, e22, e23
        b done
    MEND

matrixLerp_asm
        stmfd sp!, {r4-r9, lr}
        ldr m, =matrixPtr
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
        b m3_d4

mX_dY
        ldr tmp, =divTable
        ldr tmp, [tmp, pdiv, lsl #2]
        mul tmp, pmul, tmp
        mov pmul, tmp, asr #8
        lerp _X_Y
m1_d2
        lerp _1_2
m1_d4
        lerp _1_4
m3_d4
        lerp _3_4

done    ldmfd sp!, {r4-r9, pc}
    END

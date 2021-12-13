    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT phd_sin_asm

x   RN r0
y   RN r1
lut RN r2

phd_sin_asm
        tst     x, #0x4000
        mov     y, x, lsl #16
        mov     y, y, lsr #16
        bic     x, y, #0x8000
        rsbne   x, x, #0x8000
        tst     x, #16
        mov     x, x, lsr #5
        ldr     lut, =sinTable
        ldr     x, [lut, x, lsl #2]
        movne   x, x, lsl #16
        mov     x, x, lsr #16
        cmp     y, #0x8000
        rsbgt   x, x, #0
        mov     pc, lr
    END

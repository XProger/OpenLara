#include "common_asm.inc"

state   .req r0
buffer  .req r1
data    .req r2
size    .req r3
smp     .req r4
idx     .req r5
stepLUT .req r6
step    .req r7
n       .req r8
index   .req r9
mask    .req r10
out     .req r12
tmp     .req out
diff    .req step

IMA_STEP_SIZE = 88

.macro ima_decode
    ldr step, [stepLUT, idx, lsl #2]

    mul tmp, step, index
    add diff, tmp, lsl #1

    subne smp, diff, lsr #3
    addeq smp, diff, lsr #3

    subs index, #3
    suble idx, #1
    addgt idx, index, lsl #1

    // clamp 0..88
    bic idx, idx, asr #31
    cmp idx, #IMA_STEP_SIZE
    movgt idx, #IMA_STEP_SIZE

    mov out, smp, asr #(2 + SND_VOL_SHIFT)
    strb out, [buffer], #1
.endm

.global sndIMA_fill_asm
sndIMA_fill_asm:
    stmfd sp!, {r4-r9}

    ldmia state, {smp, idx}
    ldr stepLUT, =IMA_STEP

    mov mask, #7
.loop:
    ldrb n, [data], #1

    and index, mask, n
    tst n, #8
    ima_decode

    and index, mask, n, lsr #4
    tst n, #(8 << 4)
    ima_decode

    subs size, #1
    bne .loop

    stmia state, {smp, idx}

    ldmfd sp!, {r4-r9}
    bx lr

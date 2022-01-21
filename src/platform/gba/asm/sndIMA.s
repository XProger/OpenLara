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
outA    .req r12
outB    .req lr
tmp     .req outB

IMA_STEP_SIZE = 88

.macro decode4 n, out
    ldr step, [stepLUT, idx, lsl #2]

    and index, \n, #7
    mov tmp, step, lsl #1
    mla step, index, tmp, step
    tst \n, #8
    subne smp, smp, step, lsr #3
    addeq smp, smp, step, lsr #3

    subs index, #3
    suble idx, idx, #1
    bicle idx, idx, idx, asr #31
    addgt idx, idx, index, lsl #1
    cmpgt idx, #IMA_STEP_SIZE
    movgt idx, #IMA_STEP_SIZE

    mov \out, smp, asr #2
.endm

.global sndIMA_asm
sndIMA_asm:
    stmfd sp!, {r4-r9, lr}

    ldmia state, {smp, idx}

    ldr stepLUT, =IMA_STEP

.loop:
    ldrb n, [data], #1

    decode4 n, outA
    
    mov n, n, lsr #4

    decode4 n, outB

    stmia buffer!, {outA, outB}

    subs size, #1
    bne .loop

    stmia state, {smp, idx}

    ldmfd sp!, {r4-r9, lr}
    bx lr

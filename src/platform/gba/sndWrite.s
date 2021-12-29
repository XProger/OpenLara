#include "common_asm.inc"

buffer  .req r0
count   .req r1
data    .req r2
vA      .req r3
vB      .req r4
vC      .req r5
vD      .req r12

SND_VOL_SHIFT = 6

.macro encode amp
    mov \amp, \amp, asr #SND_VOL_SHIFT
    cmp \amp, #-128
    movlt \amp, #-128
    cmp \amp, #127
    movgt \amp, #127
.endm

.global sndWrite_asm
sndWrite_asm:
    stmfd sp!, {r4-r5}
.loop:
    ldmia data!, {vA, vB, vC, vD}

    encode vA
    encode vB
    encode vC
    encode vD

    and vA, vA, #0xFF
    and vB, vB, #0xFF
    and vC, vC, #0xFF
    orr vA, vA, vB, lsl #8
    orr vA, vA, vC, lsl #16
    orr vA, vA, vD, lsl #24
    str vA, [buffer], #4

    subs count, #4
    bne .loop

    ldmfd sp!, {r4-r5}
    bx lr

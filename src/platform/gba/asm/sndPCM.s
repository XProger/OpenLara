#include "common_asm.inc"

pos     .req r0
inc     .req r1
size    .req r2
volume  .req r3

data    .req r4
buffer  .req r5
count   .req r6
ampA    .req r7
ampB    .req r8
outA    .req r9
outB    .req r12
last    .req count
tmp     .req outB

.global sndPCM_asm
sndPCM_asm:
    mov tmp, sp
    stmfd sp!, {r4-r9}

    ldmia tmp, {data, buffer, count}

    mla last, inc, count, pos
    cmp last, size
    movgt last, size

.loop:
    ldrb ampA, [data, pos, lsr #8]
    add pos, pos, inc
    ldrb ampB, [data, pos, lsr #8]
    add pos, pos, inc
    cmp pos, last

    sub ampA, ampA, #128
    sub ampB, ampB, #128

    ldmia buffer, {outA, outB}
    mla outA, volume, ampA, outA
    mla outB, volume, ampB, outB
    stmia buffer!, {outA, outB}

    blt .loop

.done:
    ldmfd sp!, {r4-r9}
    bx lr

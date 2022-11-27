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
tmpSP   .req outB
tmp     .req ampA

.macro clamp amp
    // Vanadium's clamp trick (-128..127)
    mov tmp, \amp, asr #31  // tmp <- 0xffffffff
    cmp tmp, \amp, asr #7  // not equal
    eorne \amp, tmp, #0x7F  // amp <- 0xffffff80
.endm

.global sndPCM_asm
sndPCM_asm:
    mov tmpSP, sp
    stmfd sp!, {r4-r9}

    ldmia tmpSP, {data, buffer, count}

    mla last, inc, count, pos
    cmp last, size
    movgt last, size

.loop:
    ldrb ampA, [data, pos, lsr #SND_FIXED_SHIFT]
    add pos, pos, inc
    ldrb ampB, [data, pos, lsr #SND_FIXED_SHIFT]
    add pos, pos, inc

    // can't use signed PCM because of LDRSB restrictions
    sub ampA, ampA, #128
    sub ampB, ampB, #128

    mul ampA, volume
    mul ampB, volume

    ldrsb outA, [buffer, #0]
    ldrsb outB, [buffer, #1]

    add outA, ampA, asr #SND_VOL_SHIFT
    add outB, ampB, asr #SND_VOL_SHIFT

    clamp outA
    clamp outB

    strb outA, [buffer], #1
    strb outB, [buffer], #1

    cmp pos, last
    blt .loop

.done:
    ldmfd sp!, {r4-r9}
    bx lr

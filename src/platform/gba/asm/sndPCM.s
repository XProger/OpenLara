#include "common_asm.inc"

pos     .req r0
inc     .req r1
size    .req r2
volume  .req r3

data    .req r4
buffer  .req r5
tmp     .req r6
mask    .req r7
last    .req r12
tmpSP   .req last
out     .req size

.macro clamp
    // Aikku93's quick-and-dirty clamp (-128..+127)
    // This only works for inputs of -256..+255
    teq out, out, lsl #32-8       // If the sign of 8bit value does not match...
    eormi out, mask, out, asr #31 // ... then clip using the real sign
.endm

.macro calc_last
    // last = pos + inc * SND_SAMPLES (176)
    add last, inc, inc, lsl #2      // last = inc * 5
    add last, inc, last, lsl #1     // last = inc * 11
    add last, pos, last, lsl #4     // last = pos + (inc * 11) * 16
.endm

.macro pcm_sample_fetch
    ldrb out, [data, pos, lsr #SND_FIXED_SHIFT]
    add pos, inc
    subs out, #128
    mulne out, volume
.endm

.macro pcm_sample_fill
    pcm_sample_fetch
    asr out, #SND_VOL_SHIFT
    strb out, [buffer], #1
.endm

.macro pcm_sample_mix
    pcm_sample_fetch
    ldrsb tmp, [buffer]
    add out, tmp, out, asr #SND_VOL_SHIFT
    clamp
    strb out, [buffer], #1
.endm

.global sndPCM_fill_asm
sndPCM_fill_asm:
    mov tmpSP, sp
    stmfd sp!, {r4-r5}

    ldmia tmpSP, {data, buffer}

    calc_last

    cmp last, size
    movgt last, size

.loop_fill:
    pcm_sample_fill
    pcm_sample_fill

    cmp pos, last
    blt .loop_fill

    ldmfd sp!, {r4-r5}
    bx lr


.global sndPCM_mix_asm
sndPCM_mix_asm:
    mov tmpSP, sp
    stmfd sp!, {r4-r7} // tmp reg required
    mov mask, #127

    ldmia tmpSP, {data, buffer}

    calc_last

    cmp last, size
    movgt last, size

.loop_mix:
    pcm_sample_mix
    pcm_sample_mix

    cmp pos, last
    blt .loop_mix

    ldmfd sp!, {r4-r7}
    bx lr

.global sndClear_asm
sndClear_asm:
    // 4 words
    mov r1, #0
    mov r2, #0
    mov r3, #0
    mov r12, #0

    // fill 11 * 4 * 4 = 176 bytes
    stmia r0!, {r1-r3, r12}
    stmia r0!, {r1-r3, r12}
    stmia r0!, {r1-r3, r12}
    stmia r0!, {r1-r3, r12}
    stmia r0!, {r1-r3, r12}
    stmia r0!, {r1-r3, r12}
    stmia r0!, {r1-r3, r12}
    stmia r0!, {r1-r3, r12}
    stmia r0!, {r1-r3, r12}
    stmia r0!, {r1-r3, r12}
    stmia r0!, {r1-r3, r12}

    bx lr
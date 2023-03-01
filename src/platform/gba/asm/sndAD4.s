#include "common_asm.inc"

// Clamping is only required if the encoder gives overflow warnings.
// To improve on speed, the music volume has been reduced to avoid this.
#define CLAMP_OUTPUT 0

// Unrolling saves 3.75 cycles per sample, but uses a lot more RAM.
#define UNROLL 0

state   .req r0
buffer  .req r1
data    .req r2
size    .req r3
zM1     .req r4
zM2     .req r5
tap     .req r6
quant   .req r7
n       .req r8
mask    .req r9
adapt   .req r10
stepLUT .req r11
temp    .req r12

.macro adpcm4_decode zM1, zM2
    sub   tap, tap, tap, asr #3
    add   tap, tap, \zM2
    mov   temp, tap, asr #8
#if CLAMP_OUTPUT
    teq   temp, temp, lsl #32-8
    eormi temp, mask, temp, asr #31
#endif
    strb  temp, [buffer], #1
    mov   n, n, ror #4
    mov   temp, n, asr #32-4
    sub   \zM2, \zM1, \zM2
    mla   \zM2, quant, temp, \zM2
    // zM1 and zM2 now swapped
    ldrb  temp, [stepLUT, temp]
    mla   temp, quant, temp, mask
    mov   quant, temp, lsr #7
.endm

.global sndADPCM4_fill_asm
sndADPCM4_fill_asm:
    stmfd sp!, {r4-r11}

    ldmia state, {zM1,zM2,tap,quant}
    mov mask, #127
    ldr stepLUT, =ADPCM4_ADAPT+8

.loop:
    ldr   n, [data], #4
#if UNROLL
.rept 8/2
#endif
1:  adpcm4_decode zM1, zM2 // zM1 and zM2 get swapped...
    adpcm4_decode zM2, zM1 // ... and swapped back
#if UNROLL
.endr
#else
    adds  size, size, #1<<(32-3+1) // Count up the 8 samples
    bcc   1b
#endif
    subs  size, #8/2 // size is provided as number of bytes
    bne   .loop

    stmia state, {zM1,zM2,tap,quant}

    ldmfd sp!, {r4-r11}
    bx lr

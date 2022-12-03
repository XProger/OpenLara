#include "common_asm.inc"

pos     .req r0     // arg
angles  .req r1     // arg
x       .req pos
y       .req angles
z       .req r2
packed  .req r12

.extern matrixTranslateRel_asm, matrixRotateYXZ_asm

.global matrixFrame_asm
matrixFrame_asm:
    stmfd sp!, {lr}

    ldr packed, [angles]
    ldrsh z, [pos, #4]
    ldrsh y, [pos, #2]
    ldrsh x, [pos, #0]

    bl matrixTranslateRel_asm   // doesn't affect user mode r12

    lsl z, packed, #22
    asr z, #16

    lsr packed, #10

    lsl y, packed, #22
    asr y, #16

    lsr packed, #10

    lsl x, packed, #22
    asr x, #16

    ldmfd sp!, {lr}
    b matrixRotateYXZ_asm

// TODO matrixFrameLerp
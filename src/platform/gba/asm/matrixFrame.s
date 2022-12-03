#include "common_asm.inc"

pos     .req r0     // arg
angles  .req r1     // arg
x       .req pos
y       .req angles
z       .req r2
mask    .req r3
packed  .req r12

.extern matrixTranslateRel_asm, matrixRotateYXZ_fast_asm

.global matrixFrame_asm
matrixFrame_asm:
    stmfd sp!, {lr}

    ldr packed, [angles]
    ldrsh z, [pos, #4]
    ldrsh y, [pos, #2]
    ldrsh x, [pos, #0]

    bl matrixTranslateRel_asm   // doesn't affect user mode r12

    mov mask, #4096
    sub mask, #4

    and z, mask, packed, lsl #2
    and y, mask, packed, lsr #(10 - 2)
    and x, mask, packed, lsr #(20 - 2)

    ldmfd sp!, {lr}
    b matrixRotateYXZ_fast_asm

// TODO matrixFrameLerp
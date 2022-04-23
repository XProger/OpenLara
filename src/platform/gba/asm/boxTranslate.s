#include "common_asm.inc"

aabb    .req r0     // arg
x       .req r1     // arg
y       .req r2     // arg
z       .req r3     // arg
// FIQ regs
minX    .req r8
maxX    .req r9
minY    .req r10
maxY    .req r11
minZ    .req r12
maxZ    .req r13

.global boxTranslate_asm
boxTranslate_asm:
    fiq_on

    ldmia aabb, {minX, maxX, minY, maxY, minZ, maxZ}
    add minX, minX, x
    add maxX, maxX, x
    add minY, minY, y
    add maxY, maxY, y
    add minZ, minZ, z
    add maxZ, maxZ, z
    stmia aabb, {minX, maxX, minY, maxY, minZ, maxZ}

    fiq_off
    bx lr

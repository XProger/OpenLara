#include "common_asm.inc"

aabb    .req r0
x       .req r1
y       .req r2
z       .req r3
minX    .req r4
maxX    .req r5
minY    .req r6
maxY    .req r7
minZ    .req r12
maxZ    .req lr

.global boxTranslate_asm
boxTranslate_asm:
    stmfd sp!, {r4-r7, lr}

    ldmia aabb, {minX, maxX, minY, maxY, minZ, maxZ}
    add minX, minX, x
    add maxX, maxX, x
    add minY, minY, y
    add maxY, maxY, y
    add minZ, minZ, z
    add maxZ, maxZ, z
    stmia aabb, {minX, maxX, minY, maxY, minZ, maxZ}

    ldmfd sp!, {r4-r7, lr}
    bx lr

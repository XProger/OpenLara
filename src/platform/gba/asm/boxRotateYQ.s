#include "common_asm.inc"

v       .req r0     // arg
q       .req r1     // arg

min     .req q
max     .req r2

minX    .req r3
maxX    .req r12
minZ    .req minX
maxZ    .req maxX

.global boxRotateYQ_asm
boxRotateYQ_asm:
    cmp q, #2
    bxeq lr

    cmp q, #1
    beq .q_1
    cmp q, #3
    beq .q_3

.q_0:
    ldmia v, {minX, maxX}
    rsb min, maxX, #0
    rsb max, minX, #0
    stmia v, {min, max}
    add v, #16
    ldmia v, {minZ, maxZ}
    rsb min, maxZ, #0
    rsb max, minZ, #0
    stmia v, {min, max}
    bx lr

.q_1:
    ldmia v, {min, max}
    add v, #16
    ldmia v, {minZ, maxZ}
    stmia v, {min, max}
    rsb min, maxZ, #0
    rsb max, minZ, #0
    sub v, #16
    stmia v, {min, max}
    bx lr

.q_3:
    add v, #16
    ldmia v, {min, max}
    sub v, #16
    ldmia v, {minX, maxX}
    stmia v, {min, max}
    rsb min, maxX, #0
    rsb max, minX, #0
    add v, #16
    stmia v, {min, max}
    bx lr

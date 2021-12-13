    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT boxRotateYQ_asm

vx  RN r0
q   RN r1
vz  RN r2

min RN q
max RN r3

minX RN r12
maxX RN lr
minZ RN minX
maxZ RN maxX

boxRotateYQ_asm
        cmp q, #2
        moveq pc, lr

        stmfd sp!, {lr}

        add vz, vx, #(4 * 4)

        cmp q, #1
        beq q_1
        cmp q, #3
        beq q_3

q_0     ldmia vx, {minX, maxX}
        rsb min, maxX, #0
        rsb max, minX, #0
        stmia vx, {min, max}
        ldmia vz, {minZ, maxZ}
        rsb min, maxZ, #0
        rsb max, minZ, #0
        stmia vz, {min, max}
        ldmfd sp!, {pc}

q_1     ldmia vz, {minZ, maxZ}
        ldmia vx, {min, max}
        stmia vz, {min, max}
        rsb min, maxZ, #0
        rsb max, minZ, #0
        stmia vx, {min, max}
        ldmfd sp!, {pc}

q_3     ldmia vx, {minX, maxX}
        ldmia vz, {min, max}
        stmia vx, {min, max}
        rsb min, maxX, #0
        rsb max, minX, #0
        stmia vz, {min, max}
        ldmfd sp!, {pc}
    END

    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT boxTranslate_asm

aabb    RN r0
x       RN r1
y       RN r2
z       RN r3
minX    RN r4
maxX    RN r5
minY    RN r6
maxY    RN r7
minZ    RN r12
maxZ    RN lr

boxTranslate_asm
        stmfd sp!, {r4-r7, lr}

        ldmia aabb, {minX, maxX, minY, maxY, minZ, maxZ}
        add minX, minX, x
        add maxX, maxX, x
        add minY, minY, y
        add maxY, maxY, y
        add minZ, minZ, z
        add maxZ, maxZ, z
        stmia aabb, {minX, maxX, minY, maxY, minZ, maxZ}

        ldmfd sp!, {r4-r7, pc}
    END

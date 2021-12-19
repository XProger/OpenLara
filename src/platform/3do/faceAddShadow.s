    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    IMPORT projectVertices_asm
    IMPORT faceAddMeshQuadsFlat_asm

    EXPORT faceAddShadow_asm

x       RN r0
z       RN r1
sx      RN r2
sz      RN r3
xns1    RN x
xps1    RN z
xns2    RN sx
xps2    RN sz
zero    RN r4
zns1    RN r5
zps1    RN r6
zns2    RN r7
zps2    RN r12
vertex  RN lr

SHADE_SHADOW_1 EQU 0x0FC0
SHADE_SHADOW_2 EQU 0x8000

faceAddShadow_asm
        stmfd sp!, {r4-r7, lr}

        mov x, x, lsl #F16_SHIFT
        mov z, z, lsl #F16_SHIFT

        add zps1, z, sz, lsl #F16_SHIFT
        sub zns1, z, sz, lsl #F16_SHIFT
        add zps2, zps1, sz, lsl #F16_SHIFT
        sub zns2, zns1, sz, lsl #F16_SHIFT

        add xps1, x, sx, lsl #F16_SHIFT
        sub xns1, x, sx, lsl #F16_SHIFT
        add xps2, xps1, sx, lsl #F16_SHIFT
        sub xns2, xns1, sx, lsl #F16_SHIFT

        mov zero, #0

        ldr vertex, =gVertices
        stmia vertex!, {xns1, zero, zps2}
        stmia vertex!, {xps1, zero, zps2}
        stmia vertex!, {xps2, zero, zps1}
        stmia vertex!, {xps2, zero, zns1}
        stmia vertex!, {xps1, zero, zns2}
        stmia vertex!, {xns1, zero, zns2}
        stmia vertex!, {xns2, zero, zns1}
        stmia vertex!, {xns2, zero, zps1}

        mov r0, #8
        bl projectVertices_asm

        ldmfd sp!, {r4-r7, lr}

        ldr r0, =gShadowQuads
        mov r1, #3
        mov r2, #SHADE_SHADOW_1
        orr r2, r2, #SHADE_SHADOW_2
        b faceAddMeshQuadsFlat_asm

    END

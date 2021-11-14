    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    IMPORT matrixPtr
    IMPORT gVertices
    EXPORT unpackMesh_asm__FPC10MeshVertexl

unpackMesh_asm__FPC10MeshVertexl

vertices RN r0
vptr     RN r5
vCount   RN r1
cx       RN r2
cy       RN r3
cz       RN r4
last     RN lr
n0       RN r6
n1       RN r8
n2       RN r10
res      RN r1
DOT3_F16 EQU 0x5000C

        stmfd sp!, {r4-r10, lr}
        add last, vertices, vCount, lsl #2
        add last, last, vCount, lsl #1
        mov vptr, vertices  ; save vertices ptr
        ldr r2, =matrixPtr
        ldr r8, [r2]        ; &m.e00
        add r2, r8, #36     ; &m.e03

        mov r0, r2
        add r1, r8, #24     ; &m.e02
        swi DOT3_F16
        mov cz, r0, asr #10

        mov r0, r2
        add r1, r8, #12     ; &m.e01
        swi DOT3_F16
        mov cy, r0, asr #10

        mov r0, r2
        mov r1, r8          ; &m.e00
        swi DOT3_F16
        mov cx, r0, asr #10

        mov vertices, vptr  ; restore vertices ptr
        ldr res, =gVertices

loop    ldmia vertices!, {n0, n1, n2} ; load two encoded vertices
        cmp vertices, last

        add r5, cx, n0, asr #16
        mov n0, n0, lsl #16
        add r6, cy, n0, asr #16

        add r7, cz, n1, asr #16
        mov n1, n1, lsl #16
        add r8, cx, n1, asr #16

        add r9, cy, n2, asr #16
        mov n2, n2, lsl #16
        add r10, cz, n2, asr #16

        stmia res!, {r5, r6, r7, r8, r9, r10}
        blt loop

        ldmfd sp!, {r4-r10, pc}
    END

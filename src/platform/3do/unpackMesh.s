    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    IMPORT matrixPtr
    IMPORT gVertices
    EXPORT unpackMesh_asm__FPC10MeshVertexl

unpackMesh_asm__FPC10MeshVertexl

vertices RN r0
vptr     RN r5
vCount   RN r1
cx       RN r8
cy       RN r9
cz       RN r10
last     RN lr
n0       RN r3
n1       RN r5
n2       RN r7
res      RN r1
DOT3_F16 EQU 0x5000C

        stmfd sp!, {r4-r10, lr}
        add last, vertices, vCount, lsl #2
        add last, last, vCount, lsl #1
        mov vptr, vertices  ; save vertices ptr
        ldr r4, =matrixPtr
        ldr r7, [r4]        ; &m.e00
        add r4, r7, #36     ; &m.e03

        mov r0, r4
        add r1, r7, #24     ; &m.e02
        swi DOT3_F16
        mov cz, r0, asr #10

        mov r0, r4
        add r1, r7, #12     ; &m.e01
        swi DOT3_F16
        mov cy, r0, asr #10

        mov r0, r4
        mov r1, r7          ; &m.e00
        swi DOT3_F16
        mov cx, r0, asr #10

        mov vertices, vptr  ; restore vertices ptr
        ldr res, =gVertices

loop    ldmia vertices!, {n0, n1, n2} ; load two encoded vertices
        cmp vertices, last

        add r2, cx, n0, asr #16 ; x
        mov n0, n0, lsl #16
        add r3, cy, n0, asr #16 ; y

        add r4, cz, n1, asr #16 ; z
        mov n1, n1, lsl #16
        add r5, cx, n1, asr #16 ; x

        add r6, cy, n2, asr #16 ; y
        mov n2, n2, lsl #16
        add r7, cz, n2, asr #16 ; z

        stmia res!, {r2, r3, r4, r5, r6, r7}
        blt loop

        ldmfd sp!, {r4-r10, pc}
    END

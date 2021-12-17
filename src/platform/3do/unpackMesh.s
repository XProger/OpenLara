    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT unpackMesh_asm

unpackMesh_asm

data     RN r0
vCount   RN r1
vx0      RN vCount
vy0      RN r2
vz0      RN r3
vx1      RN r4
vy1      RN r5
vz1      RN r6
n0       RN vy0
n1       RN vx1
n2       RN vz1
vertex   RN r12
last     RN lr

        stmfd sp!, {r4-r6, lr}
        ldr vertex, =gVertices
        add vCount, vCount, vCount, lsl #1
        add last, data, vCount, lsl #1      ; last = data + vCount * 6

loop    ldmia data!, {n0, n1, n2} ; load two encoded vertices
        cmp data, last

        mov vx0, n0, asr #16 ; x
        mov n0, n0, lsl #16
        mov vy0, n0, asr #16 ; y

        mov vz0, n1, asr #16 ; z
        mov n1, n1, lsl #16
        mov vx1, n1, asr #16 ; x

        mov vy1, n2, asr #16 ; y
        mov n2, n2, lsl #16
        mov vz1, n2, asr #16 ; z

        stmia vertex!, {vx0, vy0, vz0, vx1, vy1, vz1}
        blt loop

        ldmfd sp!, {r4-r6, pc}
    END

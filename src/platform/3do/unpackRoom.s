    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT unpackRoom_asm

unpackRoom_asm

data     RN r0
vCount   RN r1
vx0      RN vCount
vy0      RN r2
vz0      RN r3
vx1      RN r4
vy1      RN r5
vz1      RN r6
vx2      RN vx0
vy2      RN vy0
vz2      RN vz0
vx3      RN vx1
vy3      RN vy1
vz3      RN vz1
n0       RN vz1
n1       RN r7
maskH    RN r8
maskV    RN r9
vertex   RN r12
last     RN lr

        stmfd sp!, {r4-r9, lr}
        ldr vertex, =gVertices
        add last, data, vCount, lsl #1 ; last = data + vCount * 2
        mov maskH, #0x1F000
        mov maskV, #0x0FC00

loop    ldmia data!, {n0, n1} ; load four encoded vertices
        cmp data, last

        ; n0 = z1:5, y1:6, x1:5, z0:5, y0:6, x0:5
        ; n1 = z3:5, y3:6, x3:5, z2:5, y2:6, x2:5

    ; 1st vertex
        and vx0, maskH, n0, lsl #12     ; decode x0
        and vy0, maskV, n0, lsl #5      ; decode y0
        and vz0, maskH, n0, lsl #1      ; decode z0

    ; 2nd vertex
        and vx1, maskH, n0, lsr #4      ; decode x1
        and vy1, maskV, n0, lsr #11     ; decode y1
        and vz1, maskH, n0, lsr #15     ; decode z1

    ; store
        stmia vertex!, {vx0, vy0, vz0, vx1, vy1, vz1}

    ; 3rd vertex
        and vx2, maskH, n1, lsl #12     ; decode x2
        and vy2, maskV, n1, lsl #5      ; decode y2
        and vz2, maskH, n1, lsl #1      ; decode z2

    ; 4th vertex
        and vx3, maskH, n1, lsr #4      ; decode x3
        and vy3, maskV, n1, lsr #11     ; decode y3
        and vz3, maskH, n1, lsr #15     ; decode z3

    ; store
        stmia vertex!, {vx2, vy2, vz2, vx3, vy3, vz3}

        blt loop

        ldmfd sp!, {r4-r9, pc}
    END

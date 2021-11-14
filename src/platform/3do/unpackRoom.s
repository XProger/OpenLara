    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    IMPORT cameraViewOffset
    IMPORT gVertices
    EXPORT unpackRoom_asm__FPC10RoomVertexl

unpackRoom_asm__FPC10RoomVertexl

vertices RN r0
vCount   RN r1
cx       RN r2
cy       RN r3
cz       RN r4
res      RN r5
last     RN lr

        stmfd sp!, {r4-r11, lr}
        ldr r2, =cameraViewOffset
        ldr res, =gVertices
        add last, vertices, vCount, lsl #2
        ldmfd r2, {cx, cy, cz}
        mov cx, cx, lsl #2
        mov cy, cy, lsl #2
        mov cz, cz, lsl #2

loop    ldmia vertices!, {r9, r11} ; load two encoded vertices
        cmp vertices, last

    ; 1st vertex
        and r6, r9, #0xFF00    ; decode x
        mov r7, r9, asr #16    ; decode y (signed)
        and r8, r9, #0xFF      ; decode z
        add r6, cx, r6, lsl #4
        add r7, cy, r7, asl #2
        add r8, cz, r8, lsl #12

    ; 2nd vertex
        and r9, r11, #0xFF00   ; decode x
        mov r10, r11, asr #16  ; decode y (signed)
        and r11, r11, #0xFF    ; decode z
        add r9,  cx, r9,  lsl #4
        add r10, cy, r10, asl #2
        add r11, cz, r11, lsl #12

    ; store
        stmia res!, {r6, r7, r8, r9, r10, r11}
        blt loop

        ldmfd sp!, {r4-r11, pc}
    END

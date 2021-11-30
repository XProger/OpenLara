    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    EXPORT ccbMap3_asm

ccbMap3_asm

face     RN r0
vp0      RN r1
vp1      RN r2
vp2      RN r3

vx0      RN vp0
vy0      RN vp1
xpos     RN vx0
ypos     RN vy0

vx1      RN vp2
vy1      RN r4
hdx0     RN vx1
hdy0     RN vy1

vx2      RN r5
vy2      RN r6
vdx0     RN vx2
vdy0     RN vy2

ws       RN r12
hs       RN lr
shift    RN hs

hddx     RN ws
hddy     RN hs

FRAME_WIDTH  EQU 320
FRAME_HEIGHT EQU 240

        stmfd sp!, {r4-r6, lr}

        add face, face, #16     ; offset to ccb_XPos
        ldr shift, [sp, #16]    ; skip 4 regs stored on the stack
        ldmia vp2, {vx2, vy2}
        ldmia vp1, {vx1, vy1}
        ldmia vp0, {vx0, vy0}

        and ws, shift, #0xFF
        mov hs, shift, lsr #8

        sub hdx0, vx1, vx0
        sub hdy0, vy1, vy0
        mov hdx0, hdx0, lsl ws
        mov hdy0, hdy0, lsl ws

        sub vdx0, vx2, vx0
        sub vdy0, vy2, vy0
        mov vdx0, vdx0, lsl hs
        mov vdy0, vdy0, lsl hs

        rsb hs, hs, #16
        mov hddx, hdx0, asr hs
        mov hddy, hdy0, asr hs
        rsb hddx, hddx, #0
        rsb hddy, hddy, #0

        mov xpos, vx0, lsl #16
        mov ypos, vy0, lsl #16
        add xpos, xpos, #(FRAME_WIDTH << 15)
        add ypos, ypos, #(FRAME_HEIGHT << 15)

        stmia face, {xpos, ypos, hdx0, hdy0, vdx0, vdy0, hddx, hddy}

        ldmfd sp!, {r4-r6, pc}
    END

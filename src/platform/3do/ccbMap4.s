    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT ccbMap4_asm

ccbMap4_asm

face     RN r0
vp0      RN r1
vp1      RN r2
vp2      RN r3
vp3      RN r4

vx0      RN vp0
vy0      RN vp1
xpos     RN vx0
ypos     RN vy0

vx1      RN vp2
vy1      RN vp3
hdx0     RN vx1
hdy0     RN vy1

vx3      RN r5
vy3      RN r6
vdx0     RN vx3
vdy0     RN vy3

vx2      RN r7
vy2      RN r8
hdx1     RN vx2
hdy1     RN vy2
hddx     RN hdx1
hddy     RN hdy1

ws       RN r12
hs       RN lr
shift    RN hs

        stmfd sp!, {r4-r8, lr}

        add face, face, #16     ; offset to ccb_XPos
        add shift, sp, #24      ; skip 6 regs stored on the stack
        ldmia shift, {vp3, shift}
        ldmia vp3, {vx3, vy3}
        ldmia vp2, {vx2, vy2}
        ldmia vp1, {vx1, vy1}
        ldmia vp0, {vx0, vy0}

        and ws, shift, #0xFF
        mov hs, shift, lsr #8
        and hs, hs, #0xFF

        sub hdx1, vx2, vx3
        sub hdy1, vy2, vy3
        mov hdx1, hdx1, lsl ws
        mov hdy1, hdy1, lsl ws

        sub hdx0, vx1, vx0
        sub hdy0, vy1, vy0
        mov hdx0, hdx0, lsl ws
        mov hdy0, hdy0, lsl ws

        sub vdx0, vx3, vx0
        sub vdy0, vy3, vy0
        mov vdx0, vdx0, lsl hs
        mov vdy0, vdy0, lsl hs

        rsb hs, hs, #16
        sub hddx, hdx1, hdx0
        sub hddy, hdy1, hdy0
        mov hddx, hddx, asr hs
        mov hddy, hddy, asr hs

        mov xpos, vx0, lsl #16
        mov ypos, vy0, lsl #16
        add xpos, xpos, #(FRAME_WIDTH << 15)
        add ypos, ypos, #(FRAME_HEIGHT << 15)

        stmia face, {xpos, ypos, hdx0, hdy0, vdx0, vdy0, hddx, hddy}

        ldmfd sp!, {r4-r8, pc}
    END

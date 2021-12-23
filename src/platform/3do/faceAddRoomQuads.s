    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT faceAddRoomQuads_asm

polysArg    RN r0
countArg    RN r1

flags       RN polysArg

vx0         RN r2
vy0         RN r3

vx1         RN r4
vy1         RN r5

vx3         RN r6
vy3         RN r7

vx2         RN r8
vy2         RN r9

pixc        RN r10
tex         RN r11

mask        RN r12
depth       RN lr

fPolys      RN countArg
fLast       RN pixc
fVertices   RN tex

spPolys     RN vx0
spLast      RN vx1
spVertices  RN vy3
spOT        RN vx2
spShadeLUT  RN vy2
spTextures  RN pixc
spFaceBase  RN tex
spPalette   RN mask

face        RN mask
faceBase    RN mask
cross       RN mask

i0          RN vy0
i1          RN vy1

vz0         RN vy0
vz1         RN vy1
vz2         RN vy2
vz3         RN vy3

vp0         RN vx0
vp1         RN vx1
vp2         RN vx2
vp3         RN vx3

xpos        RN vx0
ypos        RN vy0
hdx0        RN vx1
hdy0        RN vy1
hdx1        RN vx2
hdy1        RN vy2
vdx0        RN vx3
vdy0        RN vy3
hddx        RN hdx1
hddy        RN hdy1

nextPtr     RN vy2
dataPtr     RN polysArg
plutPtr     RN countArg

tmp         RN countArg
ot          RN countArg
otTail      RN depth

shadeLUT    RN pixc
fog         RN pixc

intensity   RN vy2
plutOffset  RN vy2
texIndex    RN vy2

ws          RN tex
hs          RN depth
shift       RN depth

SP_POLYS    EQU 0
SP_LAST     EQU 4
SP_VERTICES EQU 8
SP_OT       EQU 12
SP_SHADELUT EQU 16
SP_TEXTURES EQU 20
SP_FACEBASE EQU 24
SP_PALETTE  EQU 28
SP_SIZE     EQU 32

faceAddRoomQuads_asm
        stmfd sp!, {r4-r11, lr}
        sub sp, sp, #SP_SIZE

        add countArg, countArg, countArg, lsl #1
        add spLast, polysArg, countArg, lsl #2
        ldr spVertices, =gVertices
        ldr spOT, =gOT
        ldr spShadeLUT, =shadeTable
        ldr spTextures, =level
        ldr spTextures, [spTextures, #LVL_TEX_OFFSET]
        ldr spFaceBase, =gFacesBase
        ldr spPalette, =gPalette
        ldr spPalette, [spPalette]

        stmia sp, {polysArg, spLast, spVertices, spOT, spShadeLUT, spTextures, spFaceBase, spPalette}

loop    ldmia sp, {fPolys, fLast, fVertices}
skip    cmp fPolys, fLast
        bge done

        ldmia fPolys!, {flags, i0, i1}

        ; get vertex pointers (indices are pre-multiplied by 12)
        add vp0, fVertices, i0, lsr #16
        mov i0, i0, lsl #16
        add vp1, fVertices, i0, lsr #16

        add vp2, fVertices, i1, lsr #16
        mov i1, i1, lsl #16
        add vp3, fVertices, i1, lsr #16

        ; read z value with clip mask
        ldr vz0, [vp0, #8]
        ldr vz1, [vp1, #8]
        ldr vz2, [vp2, #8]
        ldr vz3, [vp3, #8]

        ; check clipping
        and mask, vz1, vz0
        and mask, vz2, mask
        and mask, vz3, mask
        tst mask, #CLIP_MASK
        bne skip

        MAX_Z4 depth, vz0, vz1, vz2, vz3

        ; (vx1 - vx0) * (vy3 - vy0) <= (vy1 - vy0) * (vx3 - vx0)
        ldmia vp0, {vx0, vy0}
        ldmia vp1, {vx1, vy1}
        ldmia vp3, {vx3, vy3}
        sub hdx0, vx1, vx0
        sub hdy0, vy1, vy0
        sub vdx0, vx3, vx0
        sub vdy0, vy3, vy0

        CCW cross, hdx0, hdy0, vdx0, vdy0, skip

        ; poly is visible, store fPolys on the stack to reuse the reg
        str fPolys, [sp, #SP_POLYS]

        ; fog = max(0, (depth - (FOG_MIN >> OT_SHIFT)) >> 1)
        sub fog, depth, #(FOG_MIN >> OT_SHIFT)
        movs fog, fog, asr #1
        movmi fog, #0

        ; intensity = min(255, fog + ((flags >> (FACE_MIP_SHIFT + FACE_MIP_SHIFT)) & 0xFF)) >> 3
        mov intensity, flags, lsl #(32 - 8 - FACE_MIP_SHIFT - FACE_MIP_SHIFT)
        add intensity, fog, intensity, lsr #(32 - 8)
        cmp intensity, #255
        movcs intensity, #255
        mov intensity, intensity, lsr #3

        add tmp, sp, #SP_OT
        ldmia tmp, {ot, shadeLUT, tex, faceBase}

        ; pixc = shadeTable[intensity]
        ldr pixc, [shadeLUT, intensity, lsl #2]

        ; get texture ptr (base or mip)
        mov texIndex, flags
        cmp depth, #(MIP_DIST >> OT_SHIFT)
        movgt texIndex, texIndex, lsr #FACE_MIP_SHIFT
        mov texIndex, texIndex, lsl #(32 - FACE_MIP_SHIFT)
        add tex, tex, texIndex, lsr #(32 - FACE_MIP_SHIFT - 3)  ; sizeof(Texture) = 2^3

        ; faceAdd
        add ot, ot, depth, lsl #3   ; mul by size of OT element

        mov depth, faceBase     ; use depth reg due face vs faceBase reg collision

        ldr face, [depth]
        add nextPtr, face, #SIZE_OF_CCB
        str nextPtr, [depth]

        ldmia ot, {nextPtr, otTail}
        cmp nextPtr, #0
        moveq otTail, face
        stmia ot, {face, otTail}

        ; ccb flags
        ands flags, flags, #(1 << 30)
        movne flags, #(CCB_BGND)
        orr flags, flags, #(CCB_NOBLK)
        orr flags, flags, #(CCB_ACE + CCB_ACCW + CCB_ACW + CCB_ALSC + CCB_ACSC + CCB_YOXY)
        orr flags, flags, #(CCB_LDPLUT + CCB_LDPPMP + CCB_LDPRS + CCB_LDSIZE + CCB_PPABS + CCB_SPABS + CCB_NPABS)

        ; ccbMap4
        stmia face!, {flags, nextPtr}
        ldmia tex, {dataPtr, shift}

        ; plutPtr = plutOffset + (tex->shift >> 16)
        ldr plutOffset, [sp, #SP_PALETTE]
        add plutPtr, plutOffset, shift, lsr #16

        ldmia vp2, {vx2, vy2}
        sub vx2, vx2, vx0
        sub vy2, vy2, vy0
        sub hdx1, vx2, vx3
        sub hdy1, vy2, vy3

        and ws, shift, #0xFF
        mov hs, shift, lsr #8
        and hs, hs, #0xFF

        mov hdx0, hdx0, lsl ws
        mov hdy0, hdy0, lsl ws

        mov vdx0, vdx0, lsl hs
        mov vdy0, vdy0, lsl hs

        rsb hs, hs, #16
        rsb hddx, hdx0, hdx1, lsl ws
        rsb hddy, hdy0, hdy1, lsl ws
        mov hddx, hddx, asr hs
        mov hddy, hddy, asr hs

        mov xpos, vx0, lsl #16
        mov ypos, vy0, lsl #16
        add xpos, xpos, #(FRAME_WIDTH << 15)
        add ypos, ypos, #(FRAME_HEIGHT << 15)

        stmia face, {dataPtr, plutPtr, xpos, ypos, hdx0, hdy0, vdx0, vdy0, hddx, hddy, pixc}

        b loop

done    add sp, sp, #SP_SIZE
        ldmfd sp!, {r4-r11, pc}
    END

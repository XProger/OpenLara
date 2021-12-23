    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT faceAddRoomTriangles_asm

polysArg    RN r0
countArg    RN r1

flags       RN polysArg

vx0         RN r2
vy0         RN r3

vx1         RN r4
vy1         RN r5

vx2         RN r6
vy2         RN r7

vx3         RN r8
vy3         RN r9

pixc        RN r10
tex         RN r11

mask        RN r12
depth       RN lr

fPolys      RN countArg
fLast       RN pixc
fVertices   RN tex

spPolys     RN vx0
spLast      RN vx1
spVertices  RN vy2
spOT        RN vx3
spPalette   RN vy3
spShadeLUT  RN pixc
spTextures  RN tex
spFaceBase  RN mask

face        RN mask
faceBase    RN mask
cross       RN mask

i0          RN vy0
i1          RN vy1

vz0         RN vy0
vz1         RN vy1
vz2         RN vy2

vp0         RN vx0
vp1         RN vx1
vp2         RN vx2

xpos        RN vx0
ypos        RN vy0
hdx0        RN vx1
hdy0        RN vy1
vdx0        RN vx2
vdy0        RN vy2
hddx        RN vx3
hddy        RN vy3

nextPtr     RN vy3
dataPtr     RN polysArg
plutPtr     RN countArg

tmp         RN countArg
ot          RN countArg
otTail      RN depth

shadeLUT    RN pixc
fog         RN pixc

intensity   RN vy3
plutOffset  RN vx3
texIndex    RN vy3

ws          RN tex
hs          RN depth
shift       RN depth

SP_POLYS    EQU 0
SP_LAST     EQU 4
SP_VERTICES EQU 8
SP_OT       EQU 12
SP_PALETTE  EQU 16
SP_SHADELUT EQU 20
SP_TEXTURES EQU 24
SP_FACEBASE EQU 28
SP_SIZE     EQU 32

faceAddRoomTriangles_asm
        stmfd sp!, {r4-r11, lr}
        sub sp, sp, #SP_SIZE

        add countArg, countArg, countArg, lsl #1
        add spLast, polysArg, countArg, lsl #2
        ldr spVertices, =gVertices
        ldr spOT, =gOT
        ldr spPalette, =gPalette
        ldr spPalette, [spPalette]
        ldr spShadeLUT, =shadeTable
        ldr spTextures, =level
        ldr spTextures, [spTextures, #LVL_TEX_OFFSET]
        ldr spFaceBase, =gFacesBase

        stmia sp, {polysArg, spLast, spVertices, spOT, spPalette, spShadeLUT, spTextures, spFaceBase}

loop    ldmia sp, {fPolys, fLast, fVertices}
skip    cmp fPolys, fLast
        bge done

        ldmia fPolys!, {flags, i0, i1}

        ; get vertex pointers (indices are pre-multiplied by 12)
        add vp0, fVertices, i0, lsr #16
        mov i0, i0, lsl #16
        add vp1, fVertices, i0, lsr #16

        add vp2, fVertices, i1, lsr #16

        ; read z value with clip mask
        ldr vz0, [vp0, #8]
        ldr vz1, [vp1, #8]
        ldr vz2, [vp2, #8]

        ; check clipping
        and mask, vz1, vz0
        and mask, vz2, mask
        tst mask, #CLIP_MASK
        bne skip

        MAX_Z3 depth, vz0, vz1, vz2

        ; (vx1 - vx0) * (vy2 - vy0) <= (vy1 - vy0) * (vx2 - vx0)
        ldmia vp0, {vx0, vy0}
        ldmia vp1, {vx1, vy1}
        ldmia vp2, {vx2, vy2}
        sub hdx0, vx1, vx0
        sub hdy0, vy1, vy0
        sub vdx0, vx2, vx0
        sub vdy0, vy2, vy0

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
        ldmia tmp, {ot, plutOffset, shadeLUT, tex, faceBase}

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
        add plutPtr, plutOffset, shift, lsr #16

        and ws, shift, #0xFF
        mov hs, shift, lsr #8
        and hs, hs, #0xFF

        mov hdx0, hdx0, lsl ws
        mov hdy0, hdy0, lsl ws

        mov vdx0, vdx0, lsl hs
        mov vdy0, vdy0, lsl hs

        rsb hs, hs, #16
        rsb hddx, hdx0, #0
        rsb hddy, hdy0, #0
        mov hddx, hddx, asr hs
        mov hddy, hddy, asr hs

        add xpos, vx0, #(FRAME_WIDTH >> 1)
        add ypos, vy0, #(FRAME_HEIGHT >> 1)
        mov xpos, vx0, lsl #16
        mov ypos, vy0, lsl #16

        stmia face, {dataPtr, plutPtr, xpos, ypos, hdx0, hdy0, vdx0, vdy0, hddx, hddy, pixc}

        b loop

done    add sp, sp, #SP_SIZE
        ldmfd sp!, {r4-r11, pc}
    END

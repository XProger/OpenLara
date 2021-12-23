    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    INCLUDE common_asm.inc

    EXPORT faceAddMeshTrianglesFlat_asm

polysArg    RN r0
countArg    RN r1
shadeArg    RN r2

flags       RN polysArg

vx0         RN shadeArg
vy0         RN r3

vx1         RN r4
vy1         RN r5

vx2         RN r6
vy2         RN r7

vx3         RN r8
vy3         RN r9

pixc        RN r10
color       RN r11

face        RN r12
depth       RN lr

mask        RN depth

fPolys      RN countArg
fLast       RN color
fVertices   RN face

spPolys     RN vx0
spLast      RN vx1
spVertices  RN vy2
spFlags     RN vx3
spOT        RN vy3
spFaceBase  RN color
spPalette   RN face

faceBase    RN vy3
cross       RN vy3

indices     RN vy0

vz0         RN vy0
vz1         RN vy1
vz2         RN vy3

vp0         RN vx0
vp1         RN vx1
vp2         RN vx3

xpos        RN vx0
ypos        RN vy0
hdx0        RN vx1
hdy0        RN vy1
vdx0        RN vx2
vdy0        RN vy2
hddx        RN vx3
hddy        RN vy3

nextPtr     RN vy3
dataPtr     RN color
plutPtr     RN countArg

tmp         RN countArg
ot          RN countArg
otTail      RN depth
nextFace    RN depth

plutOffset  RN color
colorIndex  RN face

SP_POLYS    EQU 0
SP_LAST     EQU 4
SP_VERTICES EQU 8
SP_FLAGS    EQU 12
SP_OT       EQU 16
SP_FACEBASE EQU 20
SP_PALETTE  EQU 24
SP_SIZE     EQU 28

faceAddMeshTrianglesFlat_asm
        stmfd sp!, {r4-r11, lr}
        sub sp, sp, #SP_SIZE

        mov pixc, shadeArg

        add spLast, polysArg, countArg, lsl #3
        ldr spVertices, =gVertices
        mov spFlags, #(CCB_NOBLK + CCB_BGND)
        orr spFlags, spFlags, #(CCB_ACE + CCB_ACCW + CCB_ACW + CCB_ALSC + CCB_ACSC + CCB_YOXY)
        orr spFlags, spFlags, #(CCB_CCBPRE + CCB_LDPPMP + CCB_LDPRS + CCB_LDSIZE + CCB_PPABS + CCB_SPABS + CCB_NPABS)
        ldr spOT, =gOT
        ldr spFaceBase, =gFacesBase
        ldr spPalette, =gPalette
        ldr spPalette, [spPalette]

        stmia sp, {polysArg, spLast, spVertices, spFlags, spOT, spFaceBase, spPalette}

loop    ldmia sp, {fPolys, fLast, fVertices}
skip    cmp fPolys, fLast
        bge done

        ldmia fPolys!, {flags, indices}

        ; get vertex pointers
        mov mask, #0xFF
        and vp0, mask, indices
        and vp1, mask, indices, lsr #8
        and vp2, mask, indices, lsr #16

        add vp0, vp0, vp0, lsl #1
        add vp1, vp1, vp1, lsl #1
        add vp2, vp2, vp2, lsl #1

        add vp0, fVertices, vp0, lsl #2
        add vp1, fVertices, vp1, lsl #2
        add vp2, fVertices, vp2, lsl #2

        ; read z value with clip mask
        ldr vz0, [vp0, #8]
        ldr vz1, [vp1, #8]
        ldr vz2, [vp2, #8]

        ; check clipping
        and mask, vz1, vz0
        and mask, vz2, mask
        tst mask, #CLIP_MASK
        bne skip

        AVG_Z3 depth, vz0, vz1, vz2

        ; (vx1 - vx0) * (vy2 - vy0) - (vy1 - vy0) * (vx2 - vx0) <= 0
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

        ; get color index from flags
        and colorIndex, flags, #0xFF

        add tmp, sp, #SP_FLAGS
        ldmia tmp, {flags, ot, faceBase, plutOffset}

        ; get color ptr
        add dataPtr, plutOffset, colorIndex, lsl #1

        ; faceAdd
        add ot, ot, depth, lsl #3   ; mul by size of OT element

        ldr face, [faceBase]
        add nextFace, face, #SIZE_OF_CCB
        str nextFace, [faceBase]

        ; add face to Ordering Table
        ldmia ot, {nextPtr, otTail}
        cmp nextPtr, #0
        moveq otTail, face
        stmia ot, {face, otTail}

        ; ccbMap3 (colored)
        stmia face, {flags, nextPtr, dataPtr}

        mov hdx0, hdx0, lsl #20
        mov hdy0, hdy0, lsl #20

        mov vdx0, vdx0, lsl #16
        mov vdy0, vdy0, lsl #16

        rsb hddx, hdx0, #0
        rsb hddy, hdy0, #0

        add xpos, vx0, #(FRAME_WIDTH >> 1)
        add ypos, vy0, #(FRAME_HEIGHT >> 1)
        mov xpos, vx0, lsl #16
        mov ypos, vy0, lsl #16

        add face, face, #16    ; skip flags, nextPtr, dataPtr, plutPtr

        stmia face, {xpos, ypos, hdx0, hdy0, vdx0, vdy0, hddx, hddy, pixc}

        b loop

done    add sp, sp, #SP_SIZE
        ldmfd sp!, {r4-r11, pc}
    END

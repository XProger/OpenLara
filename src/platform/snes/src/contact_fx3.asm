; Current-frame collision readers and arithmetic, in concurrent FX3 FastROM.
; contact_fx3.c supplies executable C_MOTION references. A/X/Y16, D0, DB7E.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFDEF FX3
.IFNDEF C_MOTION
.INCLUDE "texbank.inc"
.BASE $80
.BANK 4 SLOT 0
.SECTION "contact_fx3_native" SEMIFREE
.accu 16
.index 16
.16bit

; Far item 4/6,s -> far six-word box in r0/r0h. Invalid local frame uses
; the retained union. Offset arithmetic is bank-local; no ROM is staged.
ItemObj_frameBox:
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    lda.b olAnimBoxData
    sta.b tcc__r2
    lda.b olAnimBoxData+2
    sta.b tcc__r2h
    ldy #12
    lda [tcc__r0],y
    asl a
    sta.b tcc__r1
    lda.l olHostDirectory+20    ; animation count: the level's directory (olHostBind)
    asl a
    asl a
    sta.b tcc__r3
    asl a
    clc
    adc.b tcc__r3
    clc
    adc.b tcc__r1
    tay
    lda [tcc__r2],y
    beq FB_Union
    sta.b tcc__r3
    tay
    lda [tcc__r2],y
    sta.b tcc__r4
    ldy #14
    lda [tcc__r0],y
    and #$FF
    cmp.b tcc__r4
    bcs FB_Union
    asl a
    clc
    adc.b tcc__r3
    inc a
    inc a
    tay
    lda [tcc__r2],y
    bra FB_Result
FB_Union:
    lda.b tcc__r1              ; 2 * anim -> 12 * anim
    asl a
    sta.b tcc__r3
    asl a
    clc
    adc.b tcc__r3
FB_Result:
    clc
    adc.b tcc__r2
    sta.b tcc__r0
    lda.b tcc__r2h
    sta.b tcc__r0h
    rtl

; Source getAdjRooms is this room plus its direct renderer portals.
; The GSU-relative bank in LEVEL.BIN becomes a full-bank CPU FastROM bank.
Room_isAdjacent:
    lda 4,s
    cmp.w olRoomCount
    bcs RA_No
    lda 6,s
    cmp.w olRoomCount
    bcs RA_No
    sta.b tcc__r2
    cmp 4,s
    beq RA_Yes
    lda 4,s
    asl a
    asl a
    asl a
    asl a
    asl a
    ; The room's record in LEVEL.BIN, which starts the level's first bank
    ; (olLevel.levelBank, main.asm SelectLevel: the cartridge may carry
    ; several levels): its portal list's address, bank and count.
    clc
    adc #6
    tay
    stz.b tcc__r0
    lda.w olLevel+2
    sta.b tcc__r0h
    lda [tcc__r0],y
    pha                         ; the list's address
    iny
    iny
    lda [tcc__r0],y
    tax
    xba
    and #$FF
    bne +
    pla
    bra RA_No
+   sta.b tcc__r1
    txa
    and #$FF
    clc
    adc.w olLevel+2
    sta.b tcc__r0h
    pla
    sta.b tcc__r0
RA_Next:
    lda [tcc__r0]
    cmp.b tcc__r2
    beq RA_Yes
    lda.b tcc__r0
    clc
    adc #32
    sta.b tcc__r0
    dec.b tcc__r1
    bne RA_Next
RA_No:
    lda #0
    sta.b tcc__r0
    rtl
RA_Yes:
    lda #1
    sta.b tcc__r0
    rtl

; signed (x*c-z*s)>>14, args x/z/s/c at 4/6/8/10,s; full result r0/r1.
Frame_rotate:
    lda 10,s
    tay
    lda 4,s
    jsr.l Contact_multiply
    sty.b tcc__r0
    stx.b tcc__r0h
    lda 8,s
    tay
    lda 6,s
    jsr.l Contact_multiply
    sty.b tcc__r2
    stx.b tcc__r2h
    lda.b tcc__r0
    sec
    sbc.b tcc__r2
    sta.b tcc__r0
    lda.b tcc__r0h
    sbc.b tcc__r2h
    sta.b tcc__r0h
    lda #0
    ldx.b tcc__r0h
    bpl +
    dec a
+   sta.b tcc__r1
    asl.b tcc__r0
    rol.b tcc__r0h
    rol.b tcc__r1
    asl.b tcc__r0
    rol.b tcc__r0h
    rol.b tcc__r1
    lda.b tcc__r0h
    sta.b tcc__r0
    rtl

; Signed 16-bit A * Y -> X:Y, preserving r0-r5. Magnitudes make the FX3
; tcc helper use CPU byte products with its required eight-cycle latency.
Contact_multiply:
    sta.b tcc__r9
    sty.b tcc__r10
    eor.b tcc__r10
    php
    lda.b tcc__r9
    bpl +
    eor #$FFFF
    inc a
    sta.b tcc__r9
+   lda.b tcc__r10
    bpl +
    eor #$FFFF
    inc a
    sta.b tcc__r10
+   stz.b tcc__r9h
    stz.b tcc__r10h
    jsr.l tcc__mull
    plp
    bpl +
    tya
    eor #$FFFF
    clc
    adc #1
    tay
    txa
    eor #$FFFF
    adc #0
    tax
+   rtl

; Full signed p (4/6,s) within [low-radius, high+radius] (8/10/12,s).
; Widen BEFORE growing the bounds, including radius=-32768. R3 is p,
; r2 the radius, r4 a signed 32-bit edge; Compare returns edge cmp p.
Frame_inBounds:
    lda 4,s
    sta.b tcc__r3
    lda 6,s
    sta.b tcc__r3h
    lda 12,s
    sta.b tcc__r2
    ldy #0
    cmp #$8000
    bcc +
    dey
+   sty.b tcc__r2h
    lda 8,s
    ldy #0
    cmp #$8000
    bcc +
    dey
+   sec
    sbc.b tcc__r2
    sta.b tcc__r4
    tya
    sbc.b tcc__r2h
    sta.b tcc__r4h
    jsr FC_Compare
    beq +
    bcs FC_No
+   lda 10,s
    ldy #0
    cmp #$8000
    bcc +
    dey
+   clc
    adc.b tcc__r2
    sta.b tcc__r4
    tya
    adc.b tcc__r2h
    sta.b tcc__r4h
    jsr FC_Compare
    bcc FC_No
    lda #1
    sta.b tcc__r0
    rtl
FC_No:
    lda #0
    sta.b tcc__r0
    rtl
FC_Compare:
    lda.b tcc__r3h
    eor #$8000
    sta.b tcc__r5
    lda.b tcc__r4h
    eor #$8000
    cmp.b tcc__r5
    bne +
    lda.b tcc__r4
    cmp.b tcc__r3
+   rts

; Fused current-frame bounds and unexpanded push. Both tests use the same
; pre-push local coordinates. No persistent scratch/cached ROM copy: 26
; bytes on the CPU stack, not shared with FX or NMI. All products and grown
; bounds stay 32-bit; only a point INSIDE the raw int16 box is narrowed.
; item 4/6,s, Lara 8/10,s, radius 12,s, push 14,s -> bit 0 hit, bit 1 pushed.
.DEFINE CF_LOCALS 26
.DEFINE CF_BOX 1
.DEFINE CF_SIN 5
.DEFINE CF_COS 7
.DEFINE CF_DX 9
.DEFINE CF_DZ 11
.DEFINE CF_PX 13
.DEFINE CF_PZ 17
.DEFINE CF_RESULT 21
.DEFINE CF_OUTX 25
ItemObj_frameContact:
    tsc
    sec
    sbc #CF_LOCALS
    tcs
    lda #0
    sta CF_RESULT,s
    lda CF_LOCALS+6,s
    pha
    lda CF_LOCALS+4+2,s
    pha
    jsr.l ItemObj_frameBox
    pla
    pla
    lda.b tcc__r0
    sta CF_BOX,s
    lda.b tcc__r0h
    sta CF_BOX+2,s
    lda CF_LOCALS+10,s
    pha
    lda CF_LOCALS+8+2,s
    pha
    jsr.l ItemObj_frameBox    ; r0 = Lara box
    pla
    pla
    lda CF_LOCALS+4,s
    sta.b tcc__r1
    lda CF_LOCALS+6,s
    sta.b tcc__r1h
    lda CF_LOCALS+8,s
    sta.b tcc__r2
    lda CF_LOCALS+10,s
    sta.b tcc__r2h
    ldy #6
    lda [tcc__r2],y
    sec
    sbc [tcc__r1],y
    eor #$8000
    sta.b tcc__r4            ; biased wrapped dy
    lda CF_BOX,s
    sta.b tcc__r1
    lda CF_BOX+2,s
    sta.b tcc__r1h
    ldy #4
    lda [tcc__r0],y
    sta.b tcc__r3
    ldy #6
    lda [tcc__r1],y
    sec
    sbc.b tcc__r3            ; a.maxY - b.minY, including signed overflow
    bvc CF_UpperWord
    bmi CF_UpperOK           ; positive 17-bit difference > every word dy
    jmp CF_Done
CF_UpperWord:
    eor #$8000
    cmp.b tcc__r4
    beq CF_NoVertical
    bcc CF_NoVertical
CF_UpperOK:
    ldy #6
    lda [tcc__r0],y
    sta.b tcc__r3
    ldy #4
    lda [tcc__r1],y
    sec
    sbc.b tcc__r3            ; a.minY - b.maxY must be strictly below dy
    bvc CF_LowerWord
    bpl CF_VerticalOK        ; negative 17-bit difference < every word dy
CF_NoVertical:
    jmp CF_Done
CF_LowerWord:
    eor #$8000
    cmp.b tcc__r4
    bcs CF_NoVertical
CF_VerticalOK:
    lda CF_LOCALS+4,s
    sta.b tcc__r0
    lda CF_LOCALS+6,s
    sta.b tcc__r0h
    ldy #4
    lda [tcc__r2],y
    sec
    sbc [tcc__r0],y
    sta CF_DX,s
    ldy #8
    lda [tcc__r2],y
    sec
    sbc [tcc__r0],y
    sta CF_DZ,s
    ldy #10
    lda [tcc__r0],y
    clc
    adc #32
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    and #$07FE
    tax
    lda.l olSinTable,x
    sta CF_SIN,s
    txa
    clc
    adc #512
    and #$07FE
    tax
    lda.l olSinTable,x
    sta CF_COS,s
    sta.b tcc__r4h
    lda CF_SIN,s
    sta.b tcc__r4
    lda CF_DX,s
    sta.b tcc__r3
    lda CF_DZ,s
    sta.b tcc__r3h
    jsr CF_Rotate
    lda.b tcc__r5
    sta CF_PX,s
    lda.b tcc__r5h
    sta CF_PX+2,s
    lda CF_BOX,s
    sta.b tcc__r2
    lda CF_BOX+2,s
    sta.b tcc__r2h
    lda CF_LOCALS+12,s
    ldy #0
    jsr CF_InBox
    bcs +
    jmp CF_Done
+   lda CF_DZ,s
    sta.b tcc__r3
    lda CF_DX,s
    sta.b tcc__r3h
    lda CF_SIN,s
    eor #$FFFF
    inc a
    sta.b tcc__r4
    lda CF_COS,s
    sta.b tcc__r4h
    jsr CF_Rotate
    lda.b tcc__r5
    sta CF_PZ,s
    lda.b tcc__r5h
    sta CF_PZ+2,s
    lda CF_LOCALS+12,s
    ldy #8
    jsr CF_InBox
    bcs +
    jmp CF_Done
+   lda #1
    sta CF_RESULT,s
    lda CF_LOCALS+14,s
    bne +
    jmp CF_Done
+   lda #0
    ldy #8                   ; pz is still in r5/r5h
    jsr CF_InBox
    bcs +
    jmp CF_Done
+   lda CF_PX,s
    sta.b tcc__r5
    lda CF_PX+2,s
    sta.b tcc__r5h
    lda #0
    ldy #0
    jsr CF_InBox
    bcs +
    jmp CF_Done
+   lda CF_PX,s              ; now inside raw box: uint16 edge distances
    sec
    sbc [tcc__r2]
    sta.b tcc__r3
    stz.b tcc__r4            ; stable source tie order: left/right/back/front
    ldy #2
    lda [tcc__r2],y
    sec
    sbc CF_PX,s
    cmp.b tcc__r3
    bcs +
    sta.b tcc__r3
    sty.b tcc__r4
+   ldy #8
    lda CF_PZ,s
    sec
    sbc [tcc__r2],y
    cmp.b tcc__r3
    bcs +
    sta.b tcc__r3
    sty.b tcc__r4
+   ldy #10
    lda [tcc__r2],y
    sec
    sbc CF_PZ,s
    cmp.b tcc__r3
    bcs +
    sty.b tcc__r4
+   ldy.b tcc__r4
    lda [tcc__r2],y
    cpy #8
    bcs CF_PushZ
    sta CF_PX,s
    bra CF_Push
CF_PushZ:
    sta CF_PZ,s
CF_Push:
    lda CF_PX,s
    sta.b tcc__r3
    lda CF_PZ,s
    sta.b tcc__r3h
    lda CF_SIN,s
    eor #$FFFF
    inc a
    sta.b tcc__r4
    lda CF_COS,s
    sta.b tcc__r4h
    jsr CF_Rotate
    lda.b tcc__r5
    sta CF_OUTX,s
    lda CF_PZ,s
    sta.b tcc__r3
    lda CF_PX,s
    sta.b tcc__r3h
    lda CF_SIN,s
    sta.b tcc__r4
    jsr CF_Rotate
    lda CF_LOCALS+4,s
    sta.b tcc__r0
    lda CF_LOCALS+6,s
    sta.b tcc__r0h
    lda CF_LOCALS+8,s
    sta.b tcc__r1
    lda CF_LOCALS+10,s
    sta.b tcc__r1h
    ldy #4
    lda CF_OUTX,s
    clc
    adc [tcc__r0],y
    sta [tcc__r1],y
    ldy #8
    lda.b tcc__r5
    clc
    adc [tcc__r0],y
    sta [tcc__r1],y
    lda #3
    sta CF_RESULT,s
CF_Done:                     ; a push (3) leaves the creature's box in r2/r2h
    lda CF_RESULT,s
    sta.b tcc__r0
    tsc
    clc
    adc #CF_LOCALS
    tcs
    rtl

; r3/r3h = x/z, r4/r4h = sin/cos -> full signed result r5/r5h.
; Preserve r0..r4 so a fused phase retains its box and trigonometry.
CF_Rotate:
    lda.b tcc__r3
    ldy.b tcc__r4h
    jsr.l Contact_multiply
    sty.b tcc__r5
    stx.b tcc__r5h
    lda.b tcc__r3h
    ldy.b tcc__r4
    jsr.l Contact_multiply
    sty.b tcc__r9
    stx.b tcc__r9h
    lda.b tcc__r5
    sec
    sbc.b tcc__r9
    sta.b tcc__r5
    lda.b tcc__r5h
    sbc.b tcc__r9h
    sta.b tcc__r5h
    lda #0
    ldx.b tcc__r5h
    bpl +
    dec a
+   sta.b tcc__r9
    asl.b tcc__r5
    rol.b tcc__r5h
    rol.b tcc__r9
    asl.b tcc__r5
    rol.b tcc__r5h
    rol.b tcc__r9
    lda.b tcc__r5h
    sta.b tcc__r5
    lda.b tcc__r9
    sta.b tcc__r5h
    rts

; Full r5/r5h inside box r2/r2h at Y (X=0,Z=8), grown by signed A.
; Return carry=inside. Preserve box and point; clobber r0/r3/r4 and Y.
CF_InBox:
    sta.b tcc__r3
    ldx #0
    cmp #$8000
    bcc +
    dex
+   stx.b tcc__r3h
    lda.b tcc__r5h
    eor #$8000
    sta.b tcc__r0
    lda [tcc__r2],y
    ldx #0
    cmp #$8000
    bcc +
    dex
+   sec
    sbc.b tcc__r3
    sta.b tcc__r4
    txa
    sbc.b tcc__r3h
    sta.b tcc__r4h
    jsr CF_Compare
    beq +
    bcs CF_Outside
+   iny
    iny
    lda [tcc__r2],y
    ldx #0
    cmp #$8000
    bcc +
    dex
+   clc
    adc.b tcc__r3
    sta.b tcc__r4
    txa
    adc.b tcc__r3h
    sta.b tcc__r4h
    jsr CF_Compare
    rts                     ; high >= point -> carry set
CF_Outside:
    clc
    rts
CF_Compare:
    lda.b tcc__r4h
    eor #$8000
    cmp.b tcc__r0
    bne +
    lda.b tcc__r4
    cmp.b tcc__r5
+   rts

; collidePush(lara, cinfo, true)'s animHit, recorded: item far 4/6,s; her
; x/z before the push 8/10,s. Lara_hitResolve works the quadrant out when her
; pose is published. contact_fx3.c keeps the C_MOTION references.
Lara_enemyHit:
    lda 6,s
    pha
    lda 4+2,s
    pha
    jsr.l ItemObj_frameBox
    pla
    pla
    lda.b tcc__r0
    sta.b tcc__r2
    lda.b tcc__r0h
    sta.b tcc__r2h
    lda 4,s
    sta.b tcc__r1
    lda 6,s
    sta.b tcc__r1h
    lda 8,s
    sta.b tcc__r3
    lda 10,s
    sta.b tcc__r3h
    jsr LE_Record
    rtl

; r1/r1h the creature, r2/r2h its current-frame box, r3/r3h her x/z before
; the push. Enemy_collideLara calls this with the box frameContact left.
LE_Record:
    lda.w cinfo+56             ; cinfo.enemyHit
    and #$00FF
    bne +
    rts
+   ldy #6
    lda [tcc__r2],y
    ldy #4
    sec
    sbc [tcc__r2],y            ; maxY - minY > 256, as 17 bits
    bvc LE_HeightWord
    bmi LE_Tall
    rts
LE_HeightWord:
    bmi LE_Short
    cmp #257
    bcs LE_Tall
LE_Short:
    rts
LE_Tall:
    lda [tcc__r2]              ; cx = (minX + maxX) >> 1
    ldy #2
    clc
    adc [tcc__r2],y
    jsr LE_Half
    sta.w olLaraHitIn+4
    ldy #8                     ; cz = (minZ + maxZ) >> 1
    lda [tcc__r2],y
    ldy #10
    clc
    adc [tcc__r2],y
    jsr LE_Half
    sta.w olLaraHitIn+6
    ldy #10
    lda [tcc__r1],y
    sta.w olLaraHitIn+8        ; its yaw
    lda.w items+10
    sta.w olLaraHitIn+10       ; hers
    lda.b tcc__r3
    ldy #4
    sec
    sbc [tcc__r1],y
    sta.w olLaraHitIn          ; wrapped word offsets
    lda.b tcc__r3h
    ldy #8
    sec
    sbc [tcc__r1],y
    sta.w olLaraHitIn+2
    sep #$20
    lda #$FF                   ; OL_HIT_PENDING
    sta.w olLaraHitQuadrant
    lda #5
    sta.w olLaraHitTimer
    rep #$20
    rts

; The tail of Lara::updateCollision, on a tick with the timer running (lara.c
; tests it): SND_HIT on the first frame, hitFrame to 34, both cleared when the
; timer runs out; dead clears the quadrant (updateObjectsCollision).
Lara_hitTick:
    lda.w items+16             ; her health
    beq +
    bpl LT_Alive
+   sep #$20
    stz.w olLaraHitQuadrant
    rep #$20
LT_Alive:
    lda.w olLaraHitTimer
    and #$00FF
    bne +
    rtl
+   lda.w olLaraHitFrame
    and #$00FF
    bne LT_Count
    pea $007E
    pea (items & $FFFF)
    pea 27                     ; SND_HIT
    jsr.l Sound_playItem
    pla
    pla
    pla
LT_Count:
    sep #$20
    lda.w olLaraHitFrame
    cmp #34
    bcs +
    inc a
    sta.w olLaraHitFrame
+   dec.w olLaraHitTimer
    bne +
    stz.w olLaraHitQuadrant
    stz.w olLaraHitFrame
+   rep #$20
    rtl

; ItemObj::animHit's quadrant from the recorded push: the offset less the box
; centre turned back into the world, full 32-bit as upstream, then phd_atan.
.DEFINE LR_LOCALS 10
.DEFINE LR_SIN 1
.DEFINE LR_DX 3
.DEFINE LR_DZ 7
Lara_hitResolve:
    lda.w olLaraHitQuadrant
    and #$00FF
    cmp #$00FF
    beq +
    rtl
+   tsc
    sec
    sbc #LR_LOCALS
    tcs
    lda.w olLaraHitIn+8
    clc
    adc #32
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    and #$07FE
    tax
    lda.l olSinTable,x
    sta LR_SIN,s
    txa
    clc
    adc #512
    and #$07FE
    tax
    lda.l olSinTable,x
    sta.b tcc__r4h             ; cos
    lda.w olLaraHitIn+4
    sta.b tcc__r3
    lda.w olLaraHitIn+6
    sta.b tcc__r3h
    lda LR_SIN,s               ; X_ROTX(cx, cz, -s, c)
    eor #$FFFF
    inc a
    sta.b tcc__r4
    jsr CF_Rotate
    lda.w olLaraHitIn
    jsr LR_Offset
    sta LR_DX,s
    txa
    sta LR_DX+2,s
    lda.b tcc__r3              ; X_ROTY(cx, cz, -s, c) = rotate(cz, cx, s, c)
    ldx.b tcc__r3h
    sta.b tcc__r3h
    stx.b tcc__r3
    lda LR_SIN,s
    sta.b tcc__r4
    jsr CF_Rotate
    lda.w olLaraHitIn+2
    jsr LR_Offset
    sta LR_DZ,s
    txa
    sta LR_DZ+2,s
    lda LR_DX+2,s              ; phd_atan(dz, dx)
    pha
    lda LR_DX+2,s
    pha
    lda LR_DZ+6,s
    pha
    lda LR_DZ+6,s
    pha
    jsr.l phd_atan
    pla
    pla
    pla
    pla
    lda.w olLaraHitIn+10       ; her yaw - atan + ANGLE_180 + ANGLE_45
    sec
    sbc.b tcc__r0
    clc
    adc #$A000
    asl a
    rol a
    rol a
    and #3
    inc a                      ; stored plus one: 0 is upstream's -1
    sep #$20
    sta.w olLaraHitQuadrant
    rep #$20
    tsc
    clc
    adc #LR_LOCALS
    tcs
    rtl

; The 17-bit signed sum just added, halved: its sign is N xor V.
LE_Half:
    bvs +
    cmp #$8000
    ror a
    rts
+   cmp #$8000
    ror a
    eor #$8000
    rts

; A = a recorded word offset: sign-extended, less the rotated centre in
; r5/r5h. Result low A, high X.
LR_Offset:
    ldx #0
    cmp #$8000
    bcc +
    dex
+   sec
    sbc.b tcc__r5
    pha
    txa
    sbc.b tcc__r5h
    tax
    pla
    rts

; Skip the phase when no migrated wolf/bear/bat/raptor/Lion is enabled. Keep the empty-species
; gate allocation-free. The complete phase below uses fourteen stack bytes.
Enemy_collideLara:
    ldx #0
EC_Probe:
    lda.l olNav+8214,x
    cmp.w itemsCount
    bcs EC_Next
    phx
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda.w items,x
    and #$FF
    plx
    cmp #19
    beq +
    cmp #7
    beq +
    cmp #8
    beq +
    cmp #9
    beq +
    cmp #12
    bcc EC_Next
    cmp #15
    bcs EC_Next
+   jmp EC_Begin
EC_Next:
    txa
    clc
    adc #8240
    tax
    cpx #(8240*3)
    bcc EC_Probe
    rtl

; Same Nav-owner order, status/proximity/portal filters and room guard as
; the C_MOTION body. Item storage is the fixed $7E array; Lara remains a
; full caller-supplied far pointer. No NMI/FX reader consumes the latch
; between the fused pre-push calculation and its publication here.
.DEFINE EC_LOCALS 14
.DEFINE EC_CURSOR 1
.DEFINE EC_OWNER 3
.DEFINE EC_ITEM 5
.DEFINE EC_ANGLE 7
.DEFINE EC_RADIUS 9
.DEFINE EC_LX 11            ; her x/z before the push, for animHit
.DEFINE EC_LZ 13
EC_Begin:
    tsc
    sec
    sbc #EC_LOCALS
    tcs
    lda #0
    sta EC_CURSOR,s
EC_Loop:
    lda EC_CURSOR,s
    tax
    lda.l olNav+8214,x
    cmp.w itemsCount
    bcc +
    jmp EC_Advance
+   sta EC_OWNER,s
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #(items & $FFFF)
    sta EC_ITEM,s
    sta.b tcc__r0
    tax
    lda.w 0,x
    and #$FF
    cmp #19
    beq EC_Species
    cmp #7
    beq EC_Species
    cmp #8
    beq EC_Species
    cmp #9
    beq EC_Species
    cmp #12
    bcc EC_NotSpecies
    cmp #15
    bcc EC_Species
EC_NotSpecies:
    jmp EC_Advance
EC_Species:
    lda EC_OWNER,s
    tax
    sep #$20
    lda #0
    sta.w olEnemyTouch,x
    rep #$20
    lda #$7E
    sta.b tcc__r0h
    lda EC_LOCALS+4,s
    sta.b tcc__r1
    lda EC_LOCALS+6,s
    sta.b tcc__r1h
    ldy #16
    lda [tcc__r1],y
    beq EC_FilterNo
    bmi EC_FilterNo
    ldy #24
    lda [tcc__r0],y
    and #3
    cmp #2
    bcs EC_FilterNo
    ldy #4
EC_Proximity:
    lda [tcc__r1],y
    sec
    sbc [tcc__r0],y
    clc
    adc #4095               ; wrapped delta strictly within (-4096,4096)
    cmp #8191
    bcs EC_FilterNo
    iny
    iny
    cpy #10
    bcc EC_Proximity
    ldy #1
    lda [tcc__r0],y
    and #$FF
    pha
    lda [tcc__r1],y
    and #$FF
    pha
    jsr.l Room_isAdjacent
    pla
    pla
    lda.b tcc__r0
    bne EC_Contact
EC_FilterNo:
    jmp EC_Advance
EC_Contact:
    lda EC_ITEM,s
    sta.b tcc__r0
    lda #$7E
    sta.b tcc__r0h
    lda EC_LOCALS+4,s
    sta.b tcc__r1
    lda EC_LOCALS+6,s
    sta.b tcc__r1h
    ldy #4
    lda [tcc__r1],y
    sta EC_LX,s
    ldy #8
    lda [tcc__r1],y
    sta EC_LZ,s
    lda.w cinfo+55
    and #$FF
    pha
    lda.w cinfo+26
    pha
    pei (tcc__r1h)
    pei (tcc__r1)
    pei (tcc__r0h)
    pei (tcc__r0)
    jsr.l ItemObj_frameContact
    tsc
    clc
    adc #12
    tcs
    lda.b tcc__r0
    beq EC_FilterNo
    lda EC_OWNER,s
    tax
    sep #$20
    lda #1
    sta.w olEnemyTouch,x
    rep #$20
    lda.b tcc__r0
    and #2
    beq EC_FilterNo
    lda EC_ITEM,s           ; collidePush(lara, cinfo, true)'s animHit; a
    sta.b tcc__r1           ; push (3) leaves the creature's box in r2/r2h
    lda #$7E
    sta.b tcc__r1h
    lda EC_LX,s
    sta.b tcc__r3
    lda EC_LZ,s
    sta.b tcc__r3h
    jsr LE_Record
    lda.w cinfo+26
    sta EC_RADIUS,s
    stz.w cinfo+26           ; Enemy::collide: zero through the room guard
    lda.w cinfo+46
    sta EC_ANGLE,s
    lda #32512              ; -WALL, where WALL = -127*256
    sta.w cinfo+28
    lda #$FE80              ; -LARA_STEP_HEIGHT = -384
    sta.w cinfo+30
    stz.w cinfo+32
    lda EC_LOCALS+4,s
    sta.b tcc__r0
    lda EC_LOCALS+6,s
    sta.b tcc__r0h
    ldy #4
    lda [tcc__r0],y
    sec
    sbc.w cinfo+40
    sta.b tcc__r1
    ldx #0
    cmp #$8000
    bcc +
    dex
+   phx
    pei (tcc__r1)
    ldy #8
    lda [tcc__r0],y
    sec
    sbc.w cinfo+44
    sta.b tcc__r1
    ldx #0
    cmp #$8000
    bcc +
    dex
+   phx
    pei (tcc__r1)
    jsr.l phd_atan
    pla
    pla
    pla
    pla
    lda.b tcc__r0
    jsr EC_SetAngle
    lda EC_ITEM,s
    sta.b tcc__r0
    pea 0
    pea 762                 ; LARA_HEIGHT, not the creature's height
    pea $7E
    pea (cinfo & $FFFF)
    pea $7E
    pei (tcc__r0)
    jsr.l ItemObj_collideRoom
    tsc
    clc
    adc #12
    tcs
    lda EC_ANGLE,s
    jsr EC_SetAngle
    lda EC_LOCALS+4,s
    sta.b tcc__r0
    lda EC_LOCALS+6,s
    sta.b tcc__r0h
    lda.w cinfo+50
    and #$FF
    beq EC_Accept
    ldy #4
    lda.w cinfo+40
    sta [tcc__r0],y
    ldy #8
    lda.w cinfo+44
    sta [tcc__r0],y
    bra EC_RestoreRadius
EC_Accept:
    ldy #4
    lda [tcc__r0],y
    sta.w cinfo+40
    ldy #6
    lda [tcc__r0],y
    sta.w cinfo+42
    ldy #8
    lda [tcc__r0],y
    sta.w cinfo+44
    pea $FFF6               ; Lara::updateRoom(-10)
    pei (tcc__r0h)
    pei (tcc__r0)
    jsr.l ItemObj_updateRoom
    pla
    pla
    pla
EC_RestoreRadius:
    lda EC_RADIUS,s
    sta.w cinfo+26
EC_Advance:
    lda EC_CURSOR,s
    clc
    adc #8240
    sta EC_CURSOR,s
    cmp #(8240*3)
    bcs +
    jmp EC_Loop
+   tsc
    clc
    adc #EC_LOCALS
    tcs
    rtl
EC_SetAngle:
    sta.w cinfo+46
    clc
    adc #$2000
    asl a
    rol a
    rol a
    and #3
    sta.w cinfo+48
    rts
.ENDS
.ENDIF
.ENDIF

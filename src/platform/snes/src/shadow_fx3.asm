; OpenLara - SNES / Super FX target
; Shadow_publish and shadowOf, exactly as the C_MOTION inventory_text.c
; oracle: Lara plus the stable nearest seven eligible creature records.
.INCLUDE "snes.inc"
.INCLUDE "hdr.asm"
.IFDEF FX3
.IFNDEF C_MOTION
.BASE $80
.BANK 9 SLOT 0
.SECTION "shadow_publish_native" SEMIFREE
.accu 16
.index 16
.16bit

.DEFINE SHP_N       olShadowWork
.DEFINE SHP_M       olShadowWork+2
.DEFINE SHP_K       olShadowWork+4
.DEFINE SHP_I       olShadowWork+6
.DEFINE SHP_D       olShadowWork+8
.DEFINE SHP_OUT     olShadowWork+10
.DEFINE SHP_FLOOR   olShadowWork+12
.DEFINE SHP_PICK    olShadowWork+14       ; seven word-sized slot indices
.DEFINE SHP_DIST    olShadowWork+28       ; seven unsigned wrapped distances
.DEFINE SHP_MIN     olShadowWork+42
.DEFINE SHP_MAX     olShadowWork+44

Shadow_publish:
    stz.w olShadow
    lda.w itemsCount
    beq SHP_Return
    lda.w olInvState
    bne SHP_Return
    lda.w olLevel+24                    ; fontSize
    beq SHP_Return
    lda.w ol_animFreeze
    bne SHP_Return

    ; fontAt adds the field offset in sixteen bits, keeping the same bank.
    lda.w olLevel+20
    clc
    adc #58                            ; FONT_BANK
    sta.b tcc__r1
    lda.w olLevel+22
    sta.b tcc__r1h
    ldy #0
    lda [tcc__r1],y
    and #$00FF
    sta.w olShadow+2
    lda.b tcc__r1
    clc
    adc #4                             ; FONT_DARK
    sta.b tcc__r1
    lda [tcc__r1],y
    sta.w olShadow+4
    stz.w SHP_N
    stz.w SHP_M
    stz.w SHP_K
    lda #6
    sta.w SHP_OUT
    stz.w SHP_I
    lda.w ol_floorEff
    cmp #$7FFF                         ; OL_NO_FLOOR
    beq SHP_Creatures
    sta.w SHP_FLOOR
    jsr SHP_Emit
    bra SHP_Creatures
SHP_Return:
    rtl

SHP_Creatures:
    lda.w SHP_K
    cmp.w olCreatureCount
    bcc +
    jmp.w SHP_Selected
+
    tax
    lda.w olCreatures,x
    and #$00FF
    sta.w SHP_I
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda.w items+24,x
    and #3
    cmp #3                             ; INVISIBLE, not inactive or dead
    beq SHP_Next
    lda.w items+26,x
    cmp #$7FFF
    beq SHP_Next
    lda.w items+4,x
    sec
    sbc.w items+4
    bpl +
    eor #$FFFF
    inc a
+   sta.w SHP_D
    lda.w items+8,x
    sec
    sbc.w items+8
    bpl +
    eor #$FFFF
    inc a
+   clc
    adc.w SHP_D                         ; unsigned sum wraps as in C
    sta.w SHP_D

    lda.w SHP_M
    cmp #7
    beq SHP_Full
    inc.w SHP_M
    asl a                              ; old m is the new last index
    tax
    bra SHP_Insert
SHP_Full:
    lda.w SHP_D
    cmp.w SHP_DIST+12
    bcs SHP_Next                        ; later equal-distance items lose
    ldx #12
SHP_Insert:
    cpx #0
    beq SHP_Store
    lda.w SHP_DIST-2,x
    cmp.w SHP_D
    beq SHP_Store
    bcc SHP_Store
    sta.w SHP_DIST,x
    lda.w SHP_PICK-2,x
    sta.w SHP_PICK,x
    dex
    dex
    bra SHP_Insert
SHP_Store:
    lda.w SHP_D
    sta.w SHP_DIST,x
    lda.w SHP_I
    sta.w SHP_PICK,x
SHP_Next:
    inc.w SHP_K
    jmp.w SHP_Creatures

SHP_Selected:
    stz.w SHP_K
SHP_SelectedNext:
    lda.w SHP_K
    cmp.w SHP_M
    bcs SHP_Done
    asl a
    tax
    lda.w SHP_PICK,x
    sta.w SHP_I
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda.w items+26,x
    sta.w SHP_FLOOR
    jsr SHP_Emit
    inc.w SHP_K
    bra SHP_SelectedNext
SHP_Done:
    lda.w SHP_N
    sta.w olShadow
    rtl

; Six output words: renderer slot, floor depth, box centres, scaled extents.
; Frame lookup uses the existing far-pointer ABI and its current-frame boxes.
SHP_Emit:
    ldy.w SHP_OUT
    lda.w SHP_I
    asl a
    asl a
    sta.w SHP_MIN
    asl a
    clc
    adc.w SHP_MIN
    clc
    adc #$F800
    sta.w olShadow,y
    lda.w SHP_I
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda.w SHP_FLOOR
    sec
    sbc.w items+6,x
    sta.w olShadow+2,y
    txa
    clc
    adc #(items & $FFFF)
    pea $007E
    pha
    jsr.l ItemObj_frameBox
    pla
    pla

    ldx.w SHP_OUT
    ldy #0
    lda [tcc__r0],y
    sta.w SHP_MIN
    ldy #2
    lda [tcc__r0],y
    sta.w SHP_MAX
    clc
    adc.w SHP_MIN
    cmp #$8000                         ; signed sum wraps before >> 1
    ror a
    sta.w olShadow+4,x
    lda.w SHP_MAX
    sec
    sbc.w SHP_MIN
    sta.w SHP_MIN
    asl a
    asl a
    clc
    adc.w SHP_MIN                       ; width * 5 wraps before >> 5
    jsr SHP_Shift5
    sta.w olShadow+8,x

    ldy #8
    lda [tcc__r0],y
    sta.w SHP_MIN
    ldy #10
    lda [tcc__r0],y
    sta.w SHP_MAX
    clc
    adc.w SHP_MIN
    cmp #$8000
    ror a
    sta.w olShadow+6,x
    lda.w SHP_MAX
    sec
    sbc.w SHP_MIN
    sta.w SHP_MIN
    asl a
    asl a
    clc
    adc.w SHP_MIN
    jsr SHP_Shift5
    sta.w olShadow+10,x
    lda.w SHP_OUT
    clc
    adc #12
    sta.w SHP_OUT
    inc.w SHP_N
    rts

SHP_Shift5:
    bpl +
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    ora #$F800
    rts
+   lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    rts
.ENDS
.ENDIF
.ENDIF

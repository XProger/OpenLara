; FX3 creature dispatch. enemy.c retains the complete C_MOTION oracle.
; Dead actors retain GBA's corpse countdown; completed death poses release
; Nav and remain on the list until expiry. Preserve admission/visibility and
; dirty publication order. Species receive the original item/Lara arguments
; through a tail call; no extra persistent RAM or local stack frame.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFDEF FX3
.IFNDEF C_MOTION
.BASE $80
.BANK 8 SLOT 0                  ; $88 (was $BE)
.SECTION "enemy_update_native" SEMIFREE
.accu 16
.index 16
.16bit

Enemy_update:
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    ldy #16                    ; health: live creatures keep the short dispatch
    lda [tcc__r0],y
    beq +
    bpl EU_Status
+   jmp EU_Dead
EU_Status:
    ldy #24
    lda [tcc__r0],y
    and #3
    cmp #2                     ; ITEM_STATUS_INACTIVE (item.h)
    bne EU_Admission
    lda 6,s
    pha
    lda 6,s
    pha
    jsr.l Enemy_navRelease
    pla
    pla
    jml ItemObj_deactivate
EU_Admission:
    cmp #0
    beq +
    lda #0
    bra ++
+   lda #1
++  pha                        ; forced only for ITEM_STATUS_NONE
    lda 8,s
    pha
    lda 8,s
    pha
    jsr.l Enemy_navAcquire
    pla
    pla
    pla
    lda.b tcc__r0
    ora.b tcc__r0h
    bne EU_Acquired
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    ldy #24
    lda [tcc__r0],y
    and #$FFFC
    ora #3                     ; ITEM_STATUS_INVISIBLE; retain flags/nextActive
    sta [tcc__r0],y
    jsr EU_Touch
    rtl
EU_Acquired:
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    ldy #24
    lda [tcc__r0],y
    and #3
    cmp #3                     ; ITEM_STATUS_INVISIBLE
    bne EU_Dispatch
    lda [tcc__r0],y
    and #$FFFC
    ora #1
    sta [tcc__r0],y
    jsr EU_Touch
EU_Dispatch:
    lda [tcc__r0]
    and #$00FF
    cmp #7
    bne +
    jml Wolf_update
+   cmp #9
    bne +
    jml Bat_update
+   cmp #19
    bne +
    jml Raptor_update
+   cmp #12
    bcc +
    cmp #15
    bcs +
    jml Lion_update           ; Lion male/female/puma inherit Enemy::updateState
+   jml Bear_update
EU_Dead:
    lda 6,s
    pha
    lda 6,s
    pha
    jsr.l Enemy_corpseTick
    pla
    pla
    lda.b tcc__r0
    beq +
    rtl
+   lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    jmp EU_Status
EU_Touch:
    lda.b tcc__r0               ; fixed item array's slot, as ItemObj_touch
    sec
    sbc #(items & $FFFF)
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    tax
    sep #$20
    lda #1
    sta.w olItemDirty,x
    rep #$20
    rts
.ENDS
.ENDIF
.ENDIF

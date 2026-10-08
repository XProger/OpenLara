; OpenLara - SNES / Super FX target
; Exact input snapshot for Inventory_text. Its output records stand until
; an input changes or Inventory_fpsText replaces them. No shadows are cached.
; The C_MOTION cartridge keeps the original uncached composer as an oracle.

.INCLUDE "snes.inc"
.INCLUDE "hdr.asm"

.IFDEF FX3
.IFNDEF C_MOTION
.BASE $80
.BANK 9 SLOT 0
.SECTION "inventory_text_cache_native" SEMIFREE

.accu 16
.index 16
.16bit

; Snapshot one word in A, preserving X. Y advances through the exact key.
; r0 stays true only if this was a valid cache and every word still matches.
ITC_Word:
    cmp.w olInvTextCache,y
    beq +
    sta.w olInvTextCache,y
    stz.b tcc__r0
+   iny
    iny
    rts

; Copy/compare an independently declared array or complete level descriptor.
; Do not rely on adjacency between separate C globals.
.MACRO ITC_Range
    ldx #0
-   lda.w \1,x
    jsr ITC_Word
    inx
    inx
    cpx #\2
    bne -
.ENDM

Inventory_textCached:
    lda.w olInvTextCacheValid
    sta.b tcc__r0
    ldy #0
    lda.w olInvState
    jsr ITC_Word
    lda.w olInvPage
    jsr ITC_Word

    ; Only the selected slot and its count affect drawSlot and itemName.
    ldx.w olInvItemIndex
    lda.w olInvItemsList,x
    and #$00FF
    jsr ITC_Word
    asl a
    tax
    lda.w olInvCounts,x
    jsr ITC_Word
    lda.w olInvNumKeys
    jsr ITC_Word

    ; The composer reads equality, not either animation frame separately.
    lda.w olInvFrameIndex
    cmp.w olInvFrameTarget
    beq +
    lda #0
    bra ++
+   lda #1
++  jsr ITC_Word
    lda.w olInvPassportPage
    jsr ITC_Word
    lda.w olInvOptionIndex
    jsr ITC_Word
    lda.w olInvOptionsCount
    jsr ITC_Word
    lda.w olInvOptionsHeight
    jsr ITC_Word

    lda.w olSettings+5         ; Show FPS, a byte
    and #$00FF
    jsr ITC_Word
    cmp #0
    beq +
    lda.b $00                 ; the frame counter's direct-page word
+   jsr ITC_Word

    ; The descriptor contains font pointer/size and the loaded LevelID.
    ; A boot or level change cannot reuse another level's font/remaps.
    ITC_Range olLevel, 64
    ITC_Range olLevelIds, 8
    lda.w olLevelIndex
    jsr ITC_Word
    lda.w olLevelCount
    jsr ITC_Word
    ITC_Range olInvOptType, 8
    ITC_Range olInvOptStr, 8
    ITC_Range olInvOptValue, 16

    lda #1
    sta.w olInvTextCacheValid
    rtl
.ENDS
.ENDIF
.ENDIF

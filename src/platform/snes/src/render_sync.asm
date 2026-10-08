; FX3's CPU-side render snapshot. game.c retains the complete C reference.
; This only packs WRAM: the host still publishes to SRAM between render jobs,
; after STOP/upload. FX3's concurrent ROM/SRAM access rules are unchanged.
; A/X/Y 16, D=0, DB=$7E, JSL; items and the output are allocated in $7E.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFDEF FX3
.IFNDEF C_MOTION
.BASE $80
.BANK 9 SLOT 0                  ; $89 (was $BF)
.DEFINE PPU_M7A     $00211B     ; the PPU's 16x8 signed multiplier (trap_fx3.asm)
.DEFINE PPU_M7B     $00211C
.DEFINE PPU_MPYM    $002135
.SECTION "render_sync_native" SEMIFREE
.accu 16
.index 16
.16bit

; r0 index, r1 item, r2 animation, r3 output, r4 local animation,
; r5 divisor then pending-quotient flag. No persistent scratch allocation.
gameRenderSync:
    lda.w itemsCount
    sta.w olSyncEnd
    bne +
    rtl
    ; The effects pool is the list's tail (fx.c). While none is in flight
    ; and none has gone since the last frame, its records stand as last
    ; published: stop short of it here and in ItemsExchange's copy.
+   lda.w olFxLive
    ora.w olFxPending
    bne +
    lda.w olFxCount
    beq +
    lda.w olFxFirst
    cmp.w itemsCount
    bcs +                       ; not the list's tail: nothing to stop short of
    sta.w olSyncEnd
+   stz.w olFxPending
    stz.b tcc__r0
    ldx #(items & $FFFF)
RS_Loop:
    lda.b tcc__r0
    beq RS_Selected
    lda.w ol_animFreeze
    bne RS_Selected
    ldy.b tcc__r0
    lda.w olItemDirty,y
    and #$00FF
    bne RS_Selected
    lda.w 15,x
    and #$0040
    bne RS_Selected
    jmp RS_Next
RS_Selected:
    stx.b tcc__r1
    ldy.b tcc__r0
    sep #$20
    lda #0
    sta.w olItemDirty,y          ; do not clear the adjacent dirty byte
    rep #$20
    lda.b tcc__r0
    asl a
    asl a
    sta.b tcc__r3
    asl a
    clc
    adc.b tcc__r3
    clc
    adc #(ol_itemState_gsu & $FFFF)
    sta.b tcc__r3
    stz.b tcc__r5
    lda.w 24,x
    and #3
    cmp #3
    beq RS_Hidden
    lda.w ol_animFreeze
    beq RS_Animation
    dec a
    xba
    and #$FF00                 ; freeze selects anim 0 and a byte frame
    bra RS_Ready
RS_Hidden:
    lda #$00FF                 ; invisible always wins over animation freeze
RS_Ready:
    ldy.b tcc__r3
    sta.w 0,y
    bra RS_Copy
RS_Animation:
    lda.w 12,x
    sta.b tcc__r2
    lda.w 22,x
    and #$00FF
    asl a
    asl a
    asl a
    asl a
    clc
    HOST_ADD_BASE olModelBase
    tax
    HOST_LDA olModelData, 0
    sta.b tcc__r4
    lda.b tcc__r2
    sec
    sbc.b tcc__r4
    and #$00FF
    sta.b tcc__r4
    lda.b tcc__r2
    asl a
    sta.b tcc__r2
    asl a
    asl a
    asl a
    sec
    sbc.b tcc__r2               ; animation * 14
    clc
    HOST_ADD_BASE olAnimBase
    tax
    HOST_LDA olAnimData, 1
    and #$00FF
    sta.b tcc__r5
    ldx.b tcc__r1
    lda.w 14,x
    and #$00FF
    sta.l $004204              ; unsigned byte frame / unsigned byte rate
    sep #$20
    lda.b tcc__r5
    sta.l $004206
    rep #$20
    lda #1
    sta.b tcc__r5
    ldy.b tcc__r3
RS_Copy:
    ; Four word load/store pairs alone exceed the divider's 16 CPU cycles.
    ; NMI/IRQ preserve registers and never touch CPU multiply/divide.
    lda.w 4,x
    sta.w 2,y
    lda.w 6,x
    sta.w 4,y
    lda.w 8,x
    sta.w 6,y
    lda.w 10,x
    sta.w 8,y
    ; Byte 10 is the room it is in now, which DrawRoomItems draws it in;
    ; byte 11 (her pitch in slot 0) is ItemsExchange's, in main.asm.
    sep #$20
    lda.w 1,x
    sta.w 10,y
    rep #$20
    lda.b tcc__r5
    beq RS_Next
    lda.l $004214
    and #$00FF
    xba
    ora.b tcc__r4
    sta.w 0,y
RS_Next:
    txa
    clc
    adc #32
    tax
    inc.b tcc__r0
    lda.b tcc__r0
    cmp.w olSyncEnd
    bcs RS_Return
    jmp RS_Loop
RS_Return:
    rtl

; olUpdateItems (game.c), FX3: upstream's updateItems over the active list,
; `next` read before the update (which may remove the entity it is given).
; The effects pool is the list's tail (fx.c), so an index from olFxFirst up
; goes to Fx_update directly, and the rest to ItemObj_update(item, items).
; The 816-tcc loop spent ~5.6 K master clocks a tick of the gates' budget.
olUpdateItems:
    lda.w ol_animFreeze
    beq +
    rtl
+   lda.w olFirstActive
UI_Loop:
    cmp #$00FF                  ; OL_NO_ACTIVE
    bne +
    rtl
+   sta.b tcc__r0
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #(items & $FFFF)
    tax
    lda.w 25,x                  ; nextActive
    and #$00FF
    pha
    lda.b tcc__r0
    cmp.w olFxFirst
    bcs UI_Fx
    pea :items                  ; lara = items
    pea items & $FFFF
    pea :items
    phx
    jsl ItemObj_update
    pla
    pla
    pla
    pla
    pla                         ; next
    bra UI_Loop
UI_Fx:
    lda.w 0,x
    and #$00FF
    cmp #155                    ; a Bubble: fx.c's
    beq +
    jsr FX_Sprite
    pla                         ; next
    jmp UI_Loop
+   pea :items
    phx
    jsl Fx_update
    pla
    pla
    pla                         ; next
    jmp UI_Loop

; SpriteEffect::update for the pool's record at X (fx.c's C is the
; reference): its tick (in `state`) up to the timer, then a frame on if
; ANIMATED, gone past the last or if not; then hSpeed along its angle on the
; PPU's 16x8 multiplier (FX3 has no Mode 7), sin * hSpeed >> 14 exactly. A
; speed past a signed byte, which no effect here is given, takes fx.c's.
FX_Sprite:
    sep #$20
    inc.w 2,x                   ; tick
    rep #$20
    lda.w 2,x
    and #$00FF
    cmp.w 18,x                  ; timer, 0..4
    bcc FS_Move
    sep #$20
    stz.w 2,x
    rep #$20
    lda.w 24,x
    and #$0020                  ; ITEM_FLAG_ANIMATED
    beq FS_Remove
    sep #$20
    inc.w 14,x                  ; frameIndex
    rep #$20
    phx
    lda.w 12,x                  ; olAnimFrames(animIndex): 14 bytes a record
    asl a
    sta.b tcc__r2
    asl a
    asl a
    asl a
    sec
    sbc.b tcc__r2
    tax
    HOST_LDA olAnimData, 0
    and #$00FF
    sta.b tcc__r2
    plx
    lda.w 14,x
    and #$00FF
    cmp.b tcc__r2
    bcc FS_Move
FS_Remove:
    pea :items
    phx
    jsl Fx_kill
    pla
    pla
    rts
FS_Move:
    lda.w 30,x                  ; hSpeed
    bne +
    rts
+   clc
    adc #$0080
    cmp #$0100
    bcc +
    pea :items                  ; past a signed byte: the C
    phx
    jsl Fx_spriteMove
    pla
    pla
    rts
+   phx
    lda.w 10,x                  ; angleY: sin, then cos (a quarter on)
    clc
    adc #32
    jsr FS_Product
    plx
    phx
    clc
    adc.w 4,x
    sta.w 4,x                   ; x += sin * hSpeed >> 14
    lda.w 10,x
    clc
    adc #$4020
    jsr FS_Product
    plx
    clc
    adc.w 8,x
    sta.w 8,x                   ; z += cos * hSpeed >> 14
    rts
; A = angle + rounding; the record at 1,s+2 (X pushed by the caller). Out:
; A = olSinTable's entry * hSpeed >> 14, from the 24-bit product's top 16
FS_Product:
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    and #$07FE
    tax
    lda.l olSinTable,x
    sep #$20
    sta.l PPU_M7A
    xba
    sta.l PPU_M7A
    rep #$20
    lda 3,s                     ; the record, under the return address
    tax
    sep #$20
    lda.w 30,x                  ; hSpeed, a signed byte
    sta.l PPU_M7B
    rep #$20
    lda.l PPU_MPYM              ; product >> 8, signed 16
    cmp #$8000
    ror a
    cmp #$8000
    ror a
    cmp #$8000
    ror a
    cmp #$8000
    ror a
    cmp #$8000
    ror a
    cmp #$8000
    ror a                       ; >> 6 more: >> 14
    rts
.ENDS
.ENDIF
.ENDIF

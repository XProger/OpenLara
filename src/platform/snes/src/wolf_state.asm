; Compact implementation of Wolf_state in wolf.c, kept there as an executable
; C reference. No animation selection here: return a goal and retain nextState
; until ItemObj_animProcess grants it. Flags: front=1, behind=2, canBite=4,
; inZone=8, touch=16. Distance is saturated fastLength, 0..32767.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF C_MOTION
.IFDEF FX3
.BASE $80
.BANK 5 SLOT 0
.ELSE
.BASE $7A
.BANK 5 SLOT 6
.ENDIF
.SECTION "wolf_state_native" SEMIFREE
.accu 16
.index 16
.16bit

Wolf_state:
    lda 4,s
    sta.b tcc__r0
    sec
    sbc #(items & $FFFF)
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    tax                         ; X = item slot, including slot 127
    lda 6,s
    sta.b tcc__r0h
    lda 8,s
    sta.b tcc__r1                ; target-info flags
    lda 10,s
    sta.b tcc__r1h               ; nonnegative distance
    ldy #16
    lda [tcc__r0],y
    beq WS_Hold
    bmi WS_Hold
    lda.l olEnemyNext,x
    and #$00FF
    sta.b tcc__r2
    ldy #23
    lda [tcc__r0],y
    and #$00FF
    sta.b tcc__r2h               ; mood
    ldy #2
    lda [tcc__r0],y
    and #$00FF
    cmp #13
    bcs WS_Hold
    asl a
    phx
    tax
    lda.l WS_Table,x             ; DB remains $7E, code/table may be $7F/$85
    plx
    pha
    rts                         ; table stores target-1, preserves X
WS_Table:
    .dw WS_Hold-1, WS_Stop-1, WS_Walk-1, WS_Run-1, WS_Hold-1
    .dw WS_Stalk-1, WS_Attack-1, WS_Hold-1, WS_Sleep-1, WS_Growl-1
    .dw WS_Hold-1, WS_Hold-1, WS_Bite-1

WS_Hold:
    ldy #3
    lda [tcc__r0],y
    and #$00FF
WS_Return:
    sta.b tcc__r0
    rtl
WS_Stop:
    lda.b tcc__r2
    bne WS_Return
    lda #2
    bra WS_Return
WS_Walk:
    lda #2
    jsr WS_SetTurn
    lda.b tcc__r2h
    beq +
    lda #0
    jsr WS_SetNext
    lda #5
    bra WS_Return
+   jsr WS_Random
    cmp #$20
    bcs WS_Hold
    lda #8
    jsr WS_SetNext
    lda #1
    bra WS_Return
WS_Run:
    lda #5
    jsr WS_SetTurn
    lda.b tcc__r1
    and #1
    beq WS_RunMood
    lda.b tcc__r1h
    cmp #1536
    bcs WS_RunMood
    cmp #769
    bcs WS_NextStalk
    lda.b tcc__r1
    and #2
    bne WS_NextStalk
    lda #0
    jsr WS_SetNext
    lda #6
    bra WS_Return
WS_RunMood:
    lda.b tcc__r2h
    cmp #1
    bne +
    lda.b tcc__r1h
    cmp #3069
    bcc WS_NextStalk
+   lda.b tcc__r2h
    beq +
    jmp WS_Hold
+
    lda #9
    jmp WS_Return
WS_NextStalk:
    lda #5
    jsr WS_SetNext
    lda #9
    jmp WS_Return

WS_Stalk:
    lda #2
    jsr WS_SetTurn
    lda.b tcc__r2h
    cmp #3
    beq WS_ReturnRun
    jsr WS_CanBite
    bne WS_ReturnBite
    lda.b tcc__r1h
    cmp #3070
    bcs WS_ReturnRun
    lda.b tcc__r2h
    cmp #2
    bne WS_StalkMood
    lda.b tcc__r1
    and #3
    cmp #1                      ; front, not behind
    bne WS_ReturnRun
    lda.b tcc__r1h
    cmp #1537
    bcs WS_ReturnRun
    jmp WS_Hold
WS_StalkMood:
    jsr WS_Random
    cmp #$180
    bcs +
    lda #7
    jsr WS_SetNext
    bra WS_ReturnGrowl
+   ldy #23                     ; PRNG clobbers all tcc scratch registers
    lda [tcc__r0],y
    and #$00FF
    beq WS_ReturnGrowl
    jmp WS_Hold
WS_Growl:
    lda.b tcc__r2
    beq +
    jmp WS_Return
+   lda.b tcc__r2h
    cmp #3
    beq WS_ReturnRun
    jsr WS_CanBite
    bne WS_ReturnBite
    lda.b tcc__r2h
    cmp #1
    bne +
    lda #5
    jmp WS_Return
+   cmp #0
    bne WS_ReturnRun
    lda #1
    jmp WS_Return
WS_ReturnBite:
    lda #12
    jmp WS_Return
WS_ReturnRun:
    lda #3
    jmp WS_Return
WS_ReturnGrowl:
    lda #9
    jmp WS_Return
WS_Sleep:
    lda.b tcc__r2h
    cmp #3
    beq WS_Wake
    lda.b tcc__r1
    and #8
    bne WS_Wake
    jsr WS_Random
    cmp #$20
    bcc +
    jmp WS_Hold
+   lda #2
    bra WS_WakeNext
WS_Wake:
    lda #9
WS_WakeNext:
    jsr WS_SetNext
    lda #1
    jmp WS_Return

WS_Attack:
    ldy #50
    lda #3
    bra WS_Hit
WS_Bite:
    ldy #100
    lda #9
WS_Hit:
    pha                         ; pending state after contact
    lda.b tcc__r2
    bne WS_NoHit
    lda.b tcc__r1
    and #16
    beq WS_NoHit
    sty.b tcc__r2
.IFDEF FX3
    ; Enemy::bite: the blood at joint 6 + (0, -14, 174), then the damage
    phx
    pei (tcc__r2)
    pei (tcc__r0h)
    pei (tcc__r0)
    pea 0                       ; FX_SITE_WOLF
    pei (tcc__r0h)
    pei (tcc__r0)
    jsl Fx_bite
    pla
    pla
    pla
    pla
    sta.b tcc__r0
    pla
    sta.b tcc__r0h
    pla
    sta.b tcc__r2
    plx
.ENDIF
    lda.l items+16
    beq +
    bmi +
    sec
    sbc.b tcc__r2
    bpl ++
    lda #0
++  sta.l items+16
+   pla
    jsr WS_SetNext
    jmp WS_ReturnRun
WS_NoHit:
    pla
    jmp WS_ReturnRun

WS_CanBite:
    lda.b tcc__r1h
    cmp #345
    bcs +
    lda.b tcc__r1
    and #4
    beq +
    lda.l items+16
    beq +
    bmi +
    lda #1
    rts
+   lda #0
    rts
WS_SetNext:
    sep #$20
    sta.l olEnemyNext,x
    rep #$20
    rts
WS_SetTurn:
    sep #$20
    sta.l olEnemyTurn,x
    rep #$20
    rts
WS_Random:
    phx
    jsr.l rand_logic
    lda.b tcc__r0
    plx
    pha
    ; This JSR and PHA add four bytes to the original argument offsets.
    lda 8,s
    sta.b tcc__r0
    lda 10,s
    sta.b tcc__r0h
    pla
    rts
.ENDS
.ENDIF

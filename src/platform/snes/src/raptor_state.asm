; Original Raptor::updateState. raptor.c and the extracted original C++
; graph remain independent executable oracles. front=1, canAttack=4, touch=16.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFDEF FX3
.IFNDEF C_MOTION
.BASE $80
.BANK 8 SLOT 0                  ; $88 (was $BE)
.SECTION "raptor_state_native" SEMIFREE
.accu 16
.index 16
.16bit

Raptor_state:
    lda 4,s
    sta.b tcc__r0
    sec
    sbc #(items & $FFFF)
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    tax
    lda 6,s
    sta.b tcc__r0h
    lda 8,s
    sta.b tcc__r1
    lda 10,s
    sta.b tcc__r1h
    ldy #16
    lda [tcc__r0],y
    beq RR_Hold
    bmi RR_Hold
    lda.l olEnemyNext,x
    and #$FF
    sta.b tcc__r2
    ldy #23
    lda [tcc__r0],y
    and #$FF
    sta.b tcc__r2h
    ldy #2
    lda [tcc__r0],y
    and #$FF
    cmp #9
    bcs RR_Hold
    asl a
    phx
    tax
    lda.l RR_Table,x
    plx
    pha
    rts
RR_Table:
    .dw RR_Hold-1, RR_Standing-1, RR_Walk-1, RR_Run-1, RR_Attack-1
    .dw RR_Hold-1, RR_Hold-1, RR_Attack-1, RR_Attack-1
RR_Hold:
    ldy #3
    lda [tcc__r0],y
    and #$FF
RR_Return:
    sta.b tcc__r0
    rtl
RR_Stop:
    lda #1
    bra RR_Return
RR_Bite:
    lda #8
    bra RR_Return

RR_Standing:
    lda.b tcc__r2
    bne RR_Return
    lda.b tcc__r1
    bit #16
    bne RR_Bite
    and #4
    beq RR_StandingMood
    lda.b tcc__r1h
    bmi RR_Bite
    cmp #680
    bcc RR_Bite
    cmp #1536
    bcs RR_StandingMood
    lda #4
    bra RR_Return
RR_StandingMood:
    lda.b tcc__r2h
    beq +
    lda #3
    bra RR_Return
+   lda #2
    bra RR_Return

RR_Walk:
    lda #1
    jsr RR_SetTurn
    lda.b tcc__r2h
    bne RR_Stop
    jsr RR_Random
    cmp #$100
    bcs RR_Hold
RR_Roar:
    lda #6
    jsr RR_SetNext
    bra RR_Stop

RR_Run:
    lda #4
    jsr RR_SetTurn
    lda.b tcc__r1
    bit #16
    beq +
    jmp RR_Stop
+
    and #4
    beq RR_RunMood
    lda.b tcc__r1h
    bmi RR_RunAttack
    cmp #1536
    bcs RR_RunMood
RR_RunAttack:
    ldy #3
    lda [tcc__r0],y
    and #$FF
    cmp #3
    bne RR_RunHold
    jsr RR_Random
    cmp #$2000
    bcs +
    jmp RR_Stop
+
    lda #7
    jmp RR_Return
RR_RunMood:
    lda.b tcc__r1
    and #1
    beq RR_RunSleep
    lda.b tcc__r2h
    cmp #3
    beq RR_RunSleep
    jsr RR_Random
    cmp #$100
    bcc RR_Roar
RR_RunSleep:
    lda.b tcc__r2h
    bne RR_RunHold
    jmp RR_Stop
RR_RunHold:
    jmp RR_Hold

RR_Attack:
    lda.b tcc__r2
    bne RR_RunHold
    lda.b tcc__r1
    and #17
    cmp #17
    bne RR_RunHold
    ; Enemy::bite: the blood at joint 22 + (0, 66, 318), then the damage
    phx
    pei (tcc__r0h)
    pei (tcc__r0)
    pea 3                       ; FX_SITE_RAPTOR
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
    plx
    lda.l items+16
    beq RR_AttackNext
    bmi RR_AttackNext
    sec
    sbc #100
    bpl +
    lda #0
+   sta.l items+16
RR_AttackNext:
    ldy #2
    lda [tcc__r0],y
    and #$FF
    cmp #7
    bne +
    lda #3
    bra ++
+   lda #1
++  jsr RR_SetNext
    jmp RR_Hold

RR_SetNext:
    sep #$20
    sta.l olEnemyNext,x
    rep #$20
    rts
RR_SetTurn:
    sep #$20
    sta.l olEnemyTurn,x
    rep #$20
    rts
RR_Random:
    phx
    lda.b tcc__r0
    pha
    lda.b tcc__r0h
    pha
    lda.b tcc__r1
    pha
    lda.b tcc__r1h
    pha
    lda.b tcc__r2
    pha
    lda.b tcc__r2h
    pha
    jsr.l rand_logic
    lda.b tcc__r0
    sta.b tcc__r9
    pla
    sta.b tcc__r2h
    pla
    sta.b tcc__r2
    pla
    sta.b tcc__r1h
    pla
    sta.b tcc__r1
    pla
    sta.b tcc__r0h
    pla
    sta.b tcc__r0
    plx
    lda.b tcc__r9
    rts
.ENDS
.ENDIF
.ENDIF

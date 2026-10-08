; Original Bear::updateState, with bear.c retained as the executable C oracle.
; Info: front=1, canAttack=4, contact=16. Single-player target is items[0].
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF C_MOTION
.IFDEF FX3
.BASE $80
.BANK 8 SLOT 0                  ; $88 (was $BE)
.ELSE
.BASE $40
.BANK 62 SLOT 7
.ENDIF
.SECTION "bear_state_native" SEMIFREE
.accu 16
.index 16
.16bit

Bear_state:
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
    beq BR_Dead
    bmi BR_Dead
    jmp BR_Live
BR_Dead:
    ldy #2
    lda [tcc__r0],y
    and #$FF
    beq BR_Stop
    cmp #3
    beq BR_Stop
    cmp #2
    beq BR_Howl
    cmp #1
    beq BR_DeathFlat
    cmp #4
    beq BR_DeathUpright
    cmp #9
    bne BR_Hold
    ldy #24
    lda [tcc__r0],y
    and #$10
    beq BR_Death
    lda.b tcc__r1
    and #16
    beq BR_Death
    lda #200
    jsr BR_Damage
BR_DeathFlat:
    ldy #24
    lda [tcc__r0],y
    and #$FFEF
    sta [tcc__r0],y
BR_Death:
    lda #9
    bra BR_Return
BR_DeathUpright:
    ldy #24
    lda [tcc__r0],y
    ora #$10
    sta [tcc__r0],y
    bra BR_Death
BR_Stop:
    lda #1
    bra BR_Return
BR_Howl:
    lda #4
    bra BR_Return
BR_Hold:
    ldy #3
    lda [tcc__r0],y
    and #$FF
BR_Return:
    sta.b tcc__r0
    rtl

BR_Live:
    ldy #24
    lda [tcc__r0],y
    bit #8
    bne BR_LatchInjury
    bit #16
    beq BR_ReadState
    ora #8
    bra BR_StoreInjury
BR_LatchInjury:
    ora #16
BR_StoreInjury:
    sta [tcc__r0],y
BR_ReadState:
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
    cmp #10
    bcs BR_Hold
    asl a
    phx
    tax
    lda.l BR_Table,x
    plx
    pha
    rts
BR_Table:
    .dw BR_Walk-1, BR_Standing-1, BR_Hind-1, BR_Run-1, BR_Howling-1
    .dw BR_Hold-1, BR_Bite-1, BR_Attack-1, BR_Hold-1, BR_Hold-1

BR_Walk:
    lda #2
    jsr BR_SetTurn
    lda.l items+16
    beq +
    bpl BR_WalkMood
+   lda.b tcc__r1
    and #17
    cmp #17
    bne BR_WalkMood
    lda #1
    jsr BR_SetNext
    jmp BR_Stop
BR_WalkMood:
    lda.b tcc__r2h
    beq BR_WalkRoll
    cmp #3
    bne +
    lda #0
    jsr BR_SetNext
+   jmp BR_Stop
BR_WalkRoll:
    jsr BR_Random
    cmp #$50
    bcc +
    jmp BR_Hold
+   lda #5
    jsr BR_SetNext
    jmp BR_Stop

BR_Standing:
    lda.l items+16
    beq BR_Eat
    bmi BR_Eat
    lda.b tcc__r2
    beq +
    jmp BR_Return
+   lda.b tcc__r2h
    beq BR_ReturnWalk
    lda #3
    jmp BR_Return
BR_Eat:
    lda.b tcc__r1
    and #4
    beq BR_ReturnWalk
    lda.b tcc__r1h
    cmp #768
    bcs BR_ReturnWalk
    lda #8
    jmp BR_Return
BR_ReturnWalk:
    lda #0
    jmp BR_Return

BR_Hind:
    ldy #24
    lda [tcc__r0],y
    and #8
    bne BR_HindClear
    lda.b tcc__r1
    and #17
    cmp #17
    bne +
    jmp BR_Howl
+   lda.b tcc__r2h
    cmp #3
    beq BR_HindClear
    cmp #0
    beq BR_HindGrowl
    jsr BR_Random
    cmp #$50
    bcc BR_HindGrowl
    lda.b tcc__r1h
    cmp #2049
    bcs BR_HindStop
    jsr BR_Random
    cmp #$600
    bcc BR_HindStop
    jmp BR_Howl
BR_HindClear:
    lda #0
    bra BR_HindNext
BR_HindGrowl:
    lda #5
    bra BR_HindNext
BR_HindStop:
    lda #1
BR_HindNext:
    jsr BR_SetNext
    jmp BR_Howl

BR_Run:
    lda #5
    jsr BR_SetTurn
    lda.b tcc__r1
    and #16
    beq +
    lda #3
    jsr BR_Damage
    lda #0
    jsr BR_SetNext
    sta.b tcc__r2
+   lda.l items+16
    beq BR_RunStop
    bmi BR_RunStop
    lda.b tcc__r2h
    beq BR_RunStop
    lda.b tcc__r2
    bne BR_RunHold
    lda.b tcc__r1
    and #1
    beq BR_RunHold
    lda.b tcc__r1h
    cmp #2048
    bcs BR_RunHold
    ldy #24
    lda [tcc__r0],y
    and #8
    bne BR_RunBite
    jsr BR_Random
    cmp #$300
    bcs BR_RunBite
    lda #4
    jsr BR_SetNext
BR_RunStop:
    jmp BR_Stop
BR_RunBite:
    lda.b tcc__r1h
    cmp #1024
    bcs BR_RunHold
    lda #6
    jmp BR_Return
BR_RunHold:
    jmp BR_Hold

BR_Howling:
    ldy #24
    lda [tcc__r0],y
    and #8
    beq +
    lda #0
    jsr BR_SetNext
    jmp BR_Stop
+   lda.b tcc__r2
    beq +
    jmp BR_Return
+   lda.b tcc__r2h
    beq BR_RunStop
    cmp #3
    beq BR_RunStop
    lda.b tcc__r1
    and #4
    beq BR_ReturnHind
    lda.b tcc__r1h
    cmp #600
    bcs BR_ReturnHind
    lda #7
    jmp BR_Return
BR_ReturnHind:
    lda #2
    jmp BR_Return
BR_Bite:
    ldy #1
    lda #200
    bra BR_AttackHit
BR_Attack:
    ldy #4
    lda #400
BR_AttackHit:
    pha
    lda.b tcc__r2
    bne BR_NoAttack
    lda.b tcc__r1
    and #16
    beq BR_NoAttack
.IFDEF FX3
    cpy #1                      ; BITE is Enemy::bite, with blood at joint
    bne +                       ; 14 + (0, 96, 335); ATTACK is a plain hit
    phx
    phy
    pei (tcc__r0h)
    pei (tcc__r0)
    pea 1                       ; FX_SITE_BEAR
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
    ply
    plx
+
.ENDIF
    pla
    jsr BR_Damage
    tya
    jsr BR_SetNext
    jmp BR_Hold
BR_NoAttack:
    pla
    jmp BR_Hold

BR_Damage:
    sta.b tcc__r3
    lda.l items+16
    beq +
    bmi +
    sec
    sbc.b tcc__r3
    bpl ++
    lda #0
++  sta.l items+16
+   rts
BR_SetNext:
    sep #$20
    sta.l olEnemyNext,x
    rep #$20
    rts
BR_SetTurn:
    sep #$20
    sta.l olEnemyTurn,x
    rep #$20
    rts
BR_Random:
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

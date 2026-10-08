; Bear's target-info/navigation/state/animation/location order. bear.c is the
; complete C oracle. All items are in $7E; only immutable blob reads differ.
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
.SECTION "bear_update_native" SEMIFREE
.accu 16
.index 16
.16bit

; Locals: dx 1,s, dz 3,s, target distance 5,s, turn 7,s, limit 9,s,
; info 11,s, slot 13,s, temporary 15,s. Item/Lara arguments at 20/24,s.
Bear_update:
    tsc
    sec
    sbc #16
    tcs
    lda #0
    sta 7,s
    sta 11,s
    lda #16384
    sta 5,s
    lda 20,s
    tax
    sec
    sbc #(items & $FFFF)
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sta 13,s
    lda 24,s
    tay
    lda.w 16,x
    beq BU_Touch
    bmi BU_Touch
    lda.w 16,y
    beq BU_Info
    bmi BU_Info
BU_Touch:
.IFDEF FX3
    lda 13,s                  ; hitMask captured before the enemy push
    tax
    lda.w olEnemyTouch,x
    and #$00FF
.ELSE
    lda.w 12,y
    asl a
    asl a
    sta.b tcc__r0
    asl a
    clc
    adc.b tcc__r0
    HOST_ADD_BASE olAnimBoxBase
    tax
    HOST_LDA olAnimBoxData, 4
    eor #$FFFF
    inc a
    pha
    lda #100
    pha
    lda 30,s
    pha
    lda 30,s
    pha
    lda 30,s
    pha
    lda 30,s
    pha
    jsr.l ItemObj_collideBounds
    tsc
    clc
    adc #12
    tcs
    lda.b tcc__r0
.ENDIF
    beq BU_Info
    lda #16
    sta 11,s
BU_Info:
    lda 20,s
    tax
    lda.w 16,x
    beq +
    bpl ++
+   jmp BU_State
++  lda 24,s
    tay
    lda.w 16,y
    beq +
    bpl ++
+   jmp BU_Navigate
++  lda.w 10,x
    jsr BU_Head
    sta 15,s
    lda 20,s
    tax
    lda 24,s
    tay
    lda.w 4,y
    sec
    sbc.w 4,x
    sec
    sbc 15,s
    sta 1,s
    lda.w 10,x
    clc
    adc #$4000
    jsr BU_Head
    sta 15,s
    lda 20,s
    tax
    lda 24,s
    tay
    lda.w 8,y
    sec
    sbc.w 8,x
    sec
    sbc 15,s
    sta 3,s
    pha
    lda 3,s
    pha
    jsr.l Enemy_distance
    pla
    pla
    lda.b tcc__r0
    sta 5,s
    jsr BU_Atan
    lda 20,s
    tax
    lda.b tcc__r0
    sec
    sbc.w 10,x
    clc
    adc #$3FFF
    cmp #$7FFF
    bcs BU_Navigate
    lda 11,s
    ora #1
    sta 11,s
    lda 24,s
    tay
    lda.w 6,y
    sec
    sbc.w 6,x
    clc
    adc #255
    cmp #511
    bcs BU_Navigate
    lda 11,s
    ora #4
    sta 11,s
BU_Navigate:
    lda #$7E
    pha
    tsc
    clc
    adc #5
    pha
    lda #$7E
    pha
    tsc
    clc
    adc #7
    pha
    lda 34,s
    pha
    lda 34,s
    pha
    lda 34,s
    pha
    lda 34,s
    pha
    jsr.l Enemy_target
    tsc
    clc
    adc #16
    tcs
    lda.b tcc__r0
    bne +
    jmp BU_State
+   lda 20,s
    tax
    lda.w 30,x
    bne +
    jmp BU_State
+   lda 13,s
    tax
    lda.w olEnemyTurn,x
    and #$FF
    bne +
    jmp BU_State
+   sta.b tcc__r9
    lda #182
    sta.b tcc__r10
    jsr.l tcc__mul
    sta 9,s
    jsr BU_Atan
    lda 20,s
    tax
    lda.b tcc__r0
    sec
    sbc.w 10,x
    sta 7,s
    clc
    adc #$3FFF
    cmp #$7FFF
    bcc BU_Clamp
    lda 3,s
    pha
    lda 3,s
    pha
    jsr.l Enemy_distance
    pla
    pla
    lda.b tcc__r0
    sta 15,s
    lda 20,s
    tax
    lda 9,s
    pha
    lda.w 30,x
    pha
    lda 19,s
    pha
    jsr.l Bear_closeTurn
    pla
    pla
    pla
    lda.b tcc__r0
    beq BU_Clamp
    lda 9,s
    lsr a
    sta 9,s
BU_Clamp:
    lda 7,s
    bmi BU_Negative
    cmp 9,s
    bcc BU_State
    lda 9,s
    sta 7,s
    bra BU_State
BU_Negative:
    eor #$FFFF
    inc a
    cmp 9,s
    bcc BU_State
    lda 9,s
    eor #$FFFF
    inc a
    sta 7,s
BU_State:
    lda 5,s
    pha
    lda 13,s
    pha
    lda 26,s
    pha
    lda 26,s
    pha
    jsr.l Bear_state
    pla
    pla
    pla
    pla
    lda 20,s
    tax
    sep #$20
    lda.b tcc__r0
    sta.w 3,x
    rep #$20
    lda 7,s
    pha
    lda 24,s
    pha
    lda 24,s
    pha
    jsr.l Enemy_moveGround
    tsc
    clc
    adc #22
    tcs
    rtl

BU_Atan:
    lda 3,s
    ldx #0
    cmp #0
    bpl +
    dex
+   phx
    pha
    lda 9,s
    ldx #0
    cmp #0
    bpl +
    dex
+   phx
    pha
    jsr.l phd_atan
    tsc
    clc
    adc #8
    tcs
    rts

; A=angle, result=(sin(angle)*500)>>14. Multiplication uses the existing
; runtime: FX3 can use its CPU byte-product path; GSU-2 keeps software math.
BU_Head:
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
    php
    bpl +
    eor #$FFFF
    inc a
+   sta.b tcc__r9
    stz.b tcc__r9h
    lda #500
    sta.b tcc__r10
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
+   stx.b tcc__r1
    tya
    asl a
    rol.b tcc__r1
    asl a
    rol.b tcc__r1
    lda.b tcc__r1
    rts
.ENDS
.ENDIF

; Exact legacy GSU-2 shared mood/path target; enemy.c retains the C oracle.
; Active wolf/bear/bat ItemObj pointers reside in $7E; output pointers are far.
; 16-bit wrapped deltas, PRNG decisions, injury consumption and flood state
; follow the C order. Room origins and paired box bounds are read directly.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF FX3
.IFNDEF C_MOTION
.BASE $40
.BANK 62 SLOT 7                ; resident $7E:5800 code image
.SECTION "enemy_target_gsu2_native" SEMIFREE
.accu 16
.index 16
.16bit

Enemy_target:
    tsc
    sec
    sbc #20
    tcs
    ; Locals dx,dz,csx,csz,mine,hers,flying,inZone,dist,temp at 1..19,s.
    ; Original args: item24/26, Lara28/30, tx32/34, tz36/38.
    stz.w olEnemyInZone
    lda 24,s
    tax
    lda.w 4,x
    sta 5,s
    lda.w 8,x
    sta 7,s
    lda.w 0,x
    and #$00FF
    cmp #9
    beq +
    lda #0
    bra ++
+   lda #1
++  sta 13,s
    lda 28,s
    tax
    lda.w 4,x
    sec
    sbc 5,s
    sta 1,s
    bpl +
    eor #$FFFF
    inc a
+   cmp #5121
    bcc +
    jmp ET_SleepEarly
+   sta 17,s
    lda.w 8,x
    sec
    sbc 7,s
    sta 3,s
    bpl +
    eor #$FFFF
    inc a
+   cmp #5121
    bcc +
    jmp ET_SleepEarly
+   cmp 17,s
    bcc +
    sta 17,s
+   lda 24,s
    tax
    lda.w 1,x
    and #$00FF
    asl a
    sta.b tcc__r3
    asl a
    asl a
    clc
    adc.b tcc__r3
    clc
    adc.w olRoomBase
    tax
    lda.l $710000,x           ; paired unsigned world-sector origins
    sta 19,s
    lda.l $710002,x
    sta.b tcc__r3
    lda 5,s
    sec
    sbc.b tcc__r3
    jsr ET_Asr10
    sta.b tcc__r3
    lda 19,s
    and #$00FF
    clc
    adc.b tcc__r3
    sta 5,s
    lda.l $710004,x
    sta.b tcc__r3
    lda 7,s
    sec
    sbc.b tcc__r3
    jsr ET_Asr10
    sta.b tcc__r3
    lda 19,s
    xba
    and #$00FF
    clc
    adc.b tcc__r3
    sta 7,s

    lda 24,s
    tax
    lda.w 8,x
    pha
    lda.w 4,x
    pha
    lda.w 1,x
    and #$00FF
    pha
    jsr.l navBoxAt
    pla
    pla
    pla
    lda.b tcc__r0
    sta 9,s
    lda 28,s
    tax
    lda.w 8,x
    pha
    lda.w 4,x
    pha
    lda.w 1,x
    and #$00FF
    pha
    jsr.l navBoxAt
    pla
    pla
    pla
    lda.b tcc__r0
    sta 11,s
    cmp #$FFFF
    bne +
    jmp ET_SleepEarly
+
    lda 9,s
    cmp #$FFFF
    bne +
    jmp ET_SleepEarly
+
    lda 13,s
    beq +
    lda.w olBoxCount
    asl a
+   clc
    adc.w olZoneBase
    sta 19,s
    lda 9,s
    asl a
    clc
    adc 19,s
    tax
    lda.l $710000,x
    sta.b tcc__r2
    lda 11,s
    asl a
    clc
    adc 19,s
    tax
    lda.l $710000,x
    cmp.b tcc__r2
    beq +
    lda #0
    bra ++
+   lda #1
++  sta 15,s
    sta.w olEnemyInZone
    lda 28,s
    tax
    lda.w 16,x
    bne +
    jmp ET_Sleep
+   bpl +
    jmp ET_Sleep
+
    lda 24,s
    tax
    lda.w 23,x
    and #$00FF
    sta 19,s
    cmp #2
    bcc ET_MoodWatch
    beq ET_MoodAttack
    lda 15,s
    beq ET_Escape
    jsr.l rand_logic
    lda.b tcc__r0
    cmp #$100
    bcs ET_Escape
    lda #1
    bra ET_MoodReady

ET_MoodWatch:
    lda.w 24,x
    and #8
    beq ET_WatchUnhurt
    lda 15,s
    beq ET_Escape
    jsr.l rand_logic
    lda.b tcc__r0
    cmp #$800
    bcc ET_Escape
ET_WatchUnhurt:
    lda 15,s
    beq ET_WatchOutside
    lda 17,s
    cmp #3072
    bcc ET_Attack
    lda #1
    bra ET_MoodReady
ET_WatchOutside:
    lda 19,s
    bra ET_MoodReady

ET_MoodAttack:
    lda.w 24,x
    and #8
    beq ET_AttackUnhurt
    jsr.l rand_logic
    lda.b tcc__r0
    cmp #$800
    bcc ET_Escape
ET_AttackUnhurt:
    lda 15,s
    beq ET_Sleep
ET_Attack:
    lda #2
    bra ET_MoodReady
ET_Escape:
    lda #3
    bra ET_MoodReady
ET_Sleep:
    lda #0
ET_MoodReady:
    sta 19,s
    lda 24,s
    tax
    lda.w 23,x
    and #$F700                 ; replace mood; consume only flags2.INJURED
    ora 19,s
    sta.w 23,x
    lda 19,s
    bne +
    jmp ET_False
+
    cmp #3
    bne +
    lda 1,s
    eor #$FFFF
    inc a
    sta 1,s
    lda 3,s
    eor #$FFFF
    inc a
    sta 3,s
    jmp ET_Output
+   cmp #1
    bne ET_Path
    lda 13,s
    bne ET_Path
    lda 17,s
    cmp #2048
    bcs +
    jmp ET_False
+
ET_Path:
    lda 9,s
    cmp 11,s
    bne +
    jmp ET_Output
+   lda 13,s
    asl a
    tax
    lda.w navFrom,x
    cmp 11,s
    beq +
    lda 11,s
    pha
    lda 15,s                   ; flying at original13,s after push
    pha
    jsr.l navFlood
    pla
    pla
+   lda 9,s
    cmp #1024
    bcs ET_False
    asl a
    sta.b tcc__r2
    lda 13,s
    beq +
    lda #2048
+   clc
    adc.b tcc__r2
    tax
    lda.w navNext,x
    cmp #$FFFF
    beq ET_False
    asl a
    asl a
    asl a
    clc
    adc.w olBoxBase
    tax
    lda.l $710002,x
    jsr ET_BoxCentre
    sec
    sbc 5,s
    jsr ET_Direction
    sta 1,s
    lda.l $710000,x
    jsr ET_BoxCentre
    sec
    sbc 7,s
    jsr ET_Direction
    sta 3,s
ET_Output:
    lda 32,s
    sta.b tcc__r1
    lda 34,s
    sta.b tcc__r1h
    ldy #0
    lda 1,s
    sta [tcc__r1],y
    lda 36,s
    sta.b tcc__r1
    lda 38,s
    sta.b tcc__r1h
    lda 3,s
    sta [tcc__r1],y
    lda #1
    bra ET_Return
ET_SleepEarly:
    lda 24,s
    tax
    lda.w 23,x
    and #$FF00                 ; early failures do not consume injury
    sta.w 23,x
ET_False:
    lda #0
ET_Return:
    sta.b tcc__r0
    tsc
    clc
    adc #20
    tcs
    rtl

; Floor division of a wrapped signed16 delta by1024; preserve X.
ET_Asr10:
    sta.b tcc__r2
    xba
    and #$00FF
    lsr a
    lsr a
    bit.b tcc__r2
    bpl +
    ora #$FFC0
+   rts

; Paired unsigned byte bounds -> (min+max)>>1; preserve X.
ET_BoxCentre:
    sta.b tcc__r2
    xba
    and #$00FF
    sta.b tcc__r3
    lda.b tcc__r2
    and #$00FF
    clc
    adc.b tcc__r3
    lsr a
    rts

; Signed sector delta, saturated to[-31,31], then wrapped <<10.
ET_Direction:
    eor #$8000
    cmp #$8020
    bcc +
    lda #$801F
+   cmp #$7FE1
    bcs +
    lda #$7FE1
+   eor #$8000
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    rts
.ENDS
.ENDIF
.ENDIF

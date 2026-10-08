; Native equivalent of wolf.c's update shell. The original animation graph,
; mood rules, previous-tick turn limit, head offset and damage latch remain
; unchanged. C_MOTION retains the complete C update as an executable oracle.
; Items are allocated in $7E. FX3 uses FastROM; GSU-2 copies this complete
; code image and its referenced math tables into resident WRAM.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF C_MOTION
.IFDEF FX3
.BASE $80
.BANK 5 SLOT 0
.ELSE
.BASE $7B
.BANK 4 SLOT 4                 ; resident $7F:0000, replacing C wolf.c
.ENDIF
.SECTION "wolf_update_native" SEMIFREE
.accu 16
.index 16
.16bit

Enemy_distance:
    lda 4,s
    bpl +
    eor #$FFFF
    inc a
+   sta.b tcc__r0
    lda 6,s
    bpl +
    eor #$FFFF
    inc a
+   cmp.b tcc__r0
    bcc +
    lsr.b tcc__r0
    clc
    adc.b tcc__r0
    bra ED_Saturate
+   lsr a
    clc
    adc.b tcc__r0
ED_Saturate:
    bpl +
    lda #$7FFF
+   sta.b tcc__r0
    rtl

; Sixteen local bytes: dx 1,s, dz 3,s, distance 5,s, turn 7,s, limit 9,s,
; info 11,s, slot 13,s, temporary 15,s. Item/Lara arguments at 20/24,s.
Wolf_update:
    tsc
    sec
    sbc #16
    tcs
    lda #0
    sta 7,s
    sta 11,s
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
    lda.w 16,x
    beq WU_Dead
    bmi WU_Dead
    jmp WU_Live
WU_Dead:
    lda.w 2,x
    and #$00FF
    cmp #11
    bne +
    jmp WU_Move
+   lda.w 22,x
    and #$00FF
    asl a
    asl a
    asl a
    asl a
    clc
    HOST_ADD_BASE olModelBase
    tax
    HOST_LDA olModelData, 0
    clc
    adc #20
    sta 15,s
    jsr.l rand_logic
    lda.b tcc__r0
    asl a
    clc
    adc.b tcc__r0
    rol a
    rol a
    and #3                      ; (rand_logic() * 3) >> 15, one PRNG roll
    clc
    adc 15,s
    pha
    lda 24,s
    pha
    lda 24,s
    pha
    jsr.l ItemObj_animSet
    pla
    pla
    pla
    lda 13,s
    tax
    sep #$20
    stz.w olEnemyNext,x
    rep #$20
    jmp WU_Move
WU_Live:
    lda #$7E                    ; &dz / &dx in this stack frame
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
    jmp WU_Info
+   lda 20,s
    tax
    lda.w 30,x
    bne +
    jmp WU_Info
+   lda 13,s
    tax
    lda.w olEnemyTurn,x
    and #$00FF
    bne +
    jmp WU_Info
+   sta.b tcc__r9
    lda #182
    sta.b tcc__r10
    jsr.l tcc__mul
    sta 9,s
    jsr WU_Atan
    lda 20,s
    tax
    lda.b tcc__r0
    sec
    sbc.w 10,x
    sta 7,s
    clc
    adc #$3FFF
    cmp #$7FFF                  ; strictly within (-90,+90) needs no radius check
    bcc WU_Clamp
    lda 3,s
    pha
    lda 3,s
    pha
    jsr.l Enemy_distance
    pla
    pla
    lda.b tcc__r0
    sta 5,s
    lda 20,s
    tax
    lda 9,s
    pha
    lda.w 30,x
    pha
    lda 9,s
    pha
    jsr.l Wolf_closeTurn
    pla
    pla
    pla
    lda.b tcc__r0
    beq WU_Clamp
    lda 9,s
    cmp #$8000
    ror a
    sta 9,s
WU_Clamp:
    ; Legal per-creature limits are 0, 2 or 5 degrees. Compare magnitudes
    ; unsigned so the wrapped -180-degree turn also clamps correctly.
    lda 7,s
    bmi WU_ClampNegative
    cmp 9,s
    bcc WU_Info
    lda 9,s
    sta 7,s
    bra WU_Info
WU_ClampNegative:
    eor #$FFFF
    inc a
    cmp 9,s
    bcc WU_Info
    lda 9,s
    eor #$FFFF
    inc a
    sta 7,s
WU_Info:
    lda.w olEnemyInZone
    beq +
    lda #8
    sta 11,s
+   lda 20,s
    tax
    lda.w 10,x
    jsr WU_Head
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
    jsr WU_Head
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
    jsr WU_Atan
    lda.b tcc__r0
    sta 15,s
    lda 20,s
    tax
    lda.b tcc__r0
    sec
    sbc.w 10,x
    clc
    adc #$3FFF
    cmp #$7FFF
    bcs WU_Behind
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
    cmp #511                    ; strictly within (-256,+256)
    bcs WU_Behind
    lda 11,s
    ora #4
    sta 11,s
WU_Behind:
    lda 24,s
    tay
    lda 15,s
    sec
    sbc.w 10,y
    sec
    sbc #$8000
    clc
    adc #$3FFF
    cmp #$7FFF
    bcs WU_Touch
    lda 11,s
    ora #2
    sta 11,s
WU_Touch:
    lda.w 16,y
    beq WU_State
    bmi WU_State
    lda.w 2,x
    and #$00FF
    cmp #6
    beq +
    cmp #12
    bne WU_State
+
.IFDEF FX3
    lda 13,s                  ; contact captured before Lara's enemy push
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
    beq WU_State
    lda 11,s
    ora #16
    sta 11,s
WU_State:
    lda 5,s
    pha
    lda 13,s
    pha
    lda 26,s
    pha
    lda 26,s
    pha
    jsr.l Wolf_state
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
WU_Move:
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

; Local JSR adds two bytes. Build both signed 32-bit atan arguments from
; dx/dz at caller 1/3,s; the original 16-bit wrapped coordinate contract.
WU_Atan:
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

; A=angle. Return the exact signed (sin(angle)*375)>>14 low word in A.
; Only the final two low-product bits enter the high word; avoid fourteen
; 32-bit shifts while preserving floor rounding for negative products.
WU_Head:
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
    lda #375
    sta.b tcc__r10
    stz.b tcc__r10h
    jsr.l tcc__mull
    ; Magnitudes can use the CPU word-product path. Restore the FULL
    ; product's sign before shifting, including negative floor rounding.
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
+
    stx.b tcc__r1
    tya
    asl a
    rol.b tcc__r1
    asl a
    rol.b tcc__r1
    lda.b tcc__r1
    rts
.ENDS
.ENDIF

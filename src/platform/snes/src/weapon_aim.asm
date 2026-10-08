; Lara::weaponGetAimPoint, with weapon.c's C expression as the reference.
; 816-tcc ABI: 16-bit A/X/Y, D=0, DB=$7E, far pointers at 4,s and 8,s.
; All tcc__r registers and A/X/Y are caller scratch. No new persistent state.
;
; Local box coordinates are signed words. Their midpoint may need 17 bits,
; and the full (ordered) Y span is unsigned 16-bit. Rotation products and
; their sums retain 32 bits until the final 1.14 shift.

.INCLUDE "hdr.asm"
.INCLUDE "config.inc"

.IFDEF FX3
.BASE $80
.BANK 4 SLOT 0
.ELSE
.BASE $7B
.BANK 4 SLOT 4
.DEFINE AIM_BLOB_BANK $71
.ENDIF
.SECTION "weapon_aim_native" SEMIFREE

.accu 16
.index 16
.16bit

Weapon_getAimPoint:
    lda 4,s
    sta.b tcc__r0               ; target pointer
    lda 6,s
    sta.b tcc__r0h
    lda 8,s
    sta.b tcc__r1               ; output Location pointer
    lda 10,s
    sta.b tcc__r1h

.IFDEF FX3
    lda.b olAnimAimData+2
    sta.b tcc__r2h
    ora.b olAnimAimData
    beq Aim_UnionBox
    lda.b olAnimAimData
.ELSE
    lda #AIM_BLOB_BANK
    sta.b tcc__r2h
    lda.w olAnimAimBase
    beq Aim_UnionBox            ; synthetic fixtures can disable the aim stream
.ENDIF
    sta.b tcc__r2
    ldy #12
    lda [tcc__r0],y
    cmp [tcc__r2]               ; animation count
    bcc +
    ; A corrupt animation must not index either table out of bounds.
    stz.b tcc__r3
    stz.b tcc__r3h
    lda #0
    brl Aim_AddY
+   asl a
    inc a
    inc a
    tay
    lda [tcc__r2],y             ; relative tick-map offset (0 = union box)
    beq Aim_UnionBox
    clc
    adc.b tcc__r2
    sta.b tcc__r2
    ldy #14
    lda [tcc__r0],y
    and #$00FF                 ; ItemObj.frameIndex is a game-tick byte
    cmp [tcc__r2]
    bcc +
    lda #0                     ; a corrupt tick uses this animation's start
+   asl a
    inc a
    inc a
    tay
    lda [tcc__r2],y             ; shared XYZ point, relative to aim table
    clc
    adc.w olAnimAimBase
    sta.b tcc__r2
    lda [tcc__r2]
    sta.b tcc__r3               ; local X
    ldy #4
    lda [tcc__r2],y
    sta.b tcc__r3h              ; local Z
    ldy #2
    lda [tcc__r2],y             ; local Y
    brl Aim_AddY

Aim_UnionBox:
    ldy #12                    ; ItemObj.animIndex
    lda [tcc__r0],y
    asl a
    asl a
    sta.b tcc__r2
    asl a
    clc
    adc.b tcc__r2               ; anim * 12
    clc
    adc.w olAnimBoxBase
    sta.b tcc__r2               ; box pointer
.IFDEF FX3
    lda.b olAnimBoxData+2
    sta.b tcc__r2h
.ENDIF

    ; Bias signed endpoints to unsigned, then ROR the 17-bit sum. Undoing
    ; the bias gives floor((min + max) / 2), including negative odd sums.
    lda [tcc__r2]
    eor #$8000
    sta.b tcc__r3
    ldy #2
    lda [tcc__r2],y
    eor #$8000
    clc
    adc.b tcc__r3
    ror a
    eor #$8000
    sta.b tcc__r3               ; centre X
    ldy #8
    lda [tcc__r2],y
    eor #$8000
    sta.b tcc__r3h
    ldy #10
    lda [tcc__r2],y
    eor #$8000
    clc
    adc.b tcc__r3h
    ror a
    eor #$8000
    sta.b tcc__r3h              ; centre Z

    ldy #4
    lda [tcc__r2],y             ; minY
    pha
    ldy #6
    lda [tcc__r2],y             ; maxY
    sec
    sbc 1,s                    ; unsigned span, up to 65535
    tax
    lda #3
    jsr.l tcc__udiv            ; quotient in r9; preserves r0-r4
    lda 1,s
    clc
    adc.b tcc__r9
    ply
Aim_AddY:
    ldy #6
    clc
    adc [tcc__r0],y             ; target Y
    ldy #2
    sta [tcc__r1],y             ; point Y

    ldy #10
    lda [tcc__r0],y             ; same rounded lookup as math.asm
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
    sta.b tcc__r4
    txa
    clc
    adc #512                   ; a quarter turn in byte indices
    and #$07FE
    tax
    lda.l olSinTable,x
    sta.b tcc__r4h

    lda.b tcc__r3
    ldy.b tcc__r4h
    jsr Aim_Mul                ; X * cos
    phx
    phy
    lda.b tcc__r3h
    ldy.b tcc__r4
    jsr Aim_Mul                ; Z * sin
    tya
    clc
    adc 1,s
    sta 1,s
    txa
    adc 3,s
    jsr Aim_Shift              ; (X * cos + Z * sin) >> 14
    ply
    plx
    ldy #4
    clc
    adc [tcc__r0],y
    sta [tcc__r1]              ; point X

    lda.b tcc__r3h
    ldy.b tcc__r4h
    jsr Aim_Mul                ; Z * cos
    phx
    phy
    lda.b tcc__r3
    ldy.b tcc__r4
    jsr Aim_Mul                ; X * sin
    sty.b tcc__r5
    lda 1,s
    sec
    sbc.b tcc__r5
    sta 1,s
    stx.b tcc__r5
    lda 3,s
    sbc.b tcc__r5
    jsr Aim_Shift              ; (Z * cos - X * sin) >> 14
    ply
    plx
    ldy #8
    clc
    adc [tcc__r0],y
    ldy #4
    sta [tcc__r1],y             ; point Z

    lda [tcc__r0]              ; type / room bytes
    xba
    and #$00FF
    ldy #6
    sta [tcc__r1],y
    rtl

; High word in A, low word at 3,s (past this JSR's return). The output
; Location is a word, so bits 14..29 are exactly the required shifted result.
Aim_Shift:
    sta.b tcc__r5
    lda 3,s
    asl a
    rol.b tcc__r5
    asl a
    rol.b tcc__r5
    lda.b tcc__r5
    rts

; Signed A * Y -> X:Y. Multiply magnitudes so tcc__mull needs at most
; 16 iterations, then restore the product sign. Only r5/r9/r10 are changed.
Aim_Mul:
    sta.b tcc__r9
    sty.b tcc__r10
    tya
    eor.b tcc__r9
    sta.b tcc__r5
    lda.b tcc__r9
    bpl +
    eor #$FFFF
    inc a
    sta.b tcc__r9
+   lda.b tcc__r10
    bpl +
    eor #$FFFF
    inc a
    sta.b tcc__r10
+   stz.b tcc__r9h
    stz.b tcc__r10h
    jsr.l tcc__mull
    lda.b tcc__r5
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
+   rts

.ENDS

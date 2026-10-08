; OpenLara - SNES / Super FX target
; The ring's matrix steps (inventory_draw.c), natively (FX3).
;
; Inventory_publish builds a model-view matrix per ring item every render
; sync: upstream's matrixRotateY/X and matrixTranslateRel on an int32 1.14
; matrix. 816-tcc spends that in its int32 helpers -- sign extension, wide
; shifts -- and the ring's render sync came to ~87K instructions, five
; fields in which no tick ran: the bunched ticks after it lost the release
; between two short presses (the settings scene, once frames went up twice
; as often). The same arithmetic here, on the same olInvM[12]; the C in
; inventory_draw.c stays the C_MOTION oracle's.
;
; void Inv_rot(unsigned int a, unsigned int b, int s, int c)
;   every row r: x = (int)M[r + a], y = (int)M[r + b];
;   M[r + a] = (x * c - y * s) >> 14, M[r + b] = (y * c + x * s) >> 14
; void Inv_trans(int x, int y, int z)
;   every row r: M[r + 3] += M[r] * x + M[r + 1] * y + M[r + 2] * z
;   (the rotation entries' low words, as the C's (int) casts take them)
;
; 816-tcc ABI: A/X/Y 16, D 0; arguments at 4,s after the JSL; tcc__r* are
; scratch for a C callee. The PPU's 16x8 signed multiplier, as olMulS16.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFDEF FX3
.IFNDEF C_MOTION
.BASE $80
.BANK 6 SLOT 0                  ; $86
.SECTION "inventory_fx3_native" SEMIFREE
.accu 16
.index 16
.16bit

.DEFINE PPU_M7A     $00211B
.DEFINE PPU_M7B     $00211C
.DEFINE PPU_MPYL    $002134
.DEFINE PPU_MPYM    $002135

; (tcc__r0, tcc__r1) = (int16)tcc__r2 * (int16)tcc__r3, olMulS16's way:
; b = hi' * 256 + (int8)lo, hi' its high byte rounded.
InvMul:
    lda.b tcc__r3
    clc
    adc #$0080
    xba
    and #$00FF
    sta.b tcc__r3h              ; hi'
    sep #$20
    lda.b tcc__r2
    sta.l PPU_M7A
    lda.b tcc__r2 + 1
    sta.l PPU_M7A
    lda.b tcc__r3
    sta.l PPU_M7B               ; a * (int8)lo
    rep #$20
    lda.l PPU_MPYL
    sta.b tcc__r0
    lda.l PPU_MPYM              ; bits 8-23: the high word is MPYH's sign
    xba
    and #$00FF
    bit #$0080
    beq +
    ora #$FF00
+   sta.b tcc__r1
    sep #$20
    lda.b tcc__r3h
    sta.l PPU_M7B               ; a * hi', a byte up
    rep #$20
    lda.l PPU_MPYL
    xba
    and #$FF00
    clc
    adc.b tcc__r0
    sta.b tcc__r0
    lda.l PPU_MPYM
    adc.b tcc__r1
    sta.b tcc__r1
    rts

; (tcc__r9, tcc__r9h) >> 14, arithmetic, into olInvM at X (low, high word).
InvShr14:
    lda.b tcc__r9               ; the low word's top two bits
    stz.b tcc__r10
    asl a
    rol.b tcc__r10
    asl a
    rol.b tcc__r10
    lda.b tcc__r9h
    asl a
    asl a
    ora.b tcc__r10
    sta.l olInvM,x
    lda.b tcc__r9h              ; the high word: bits 30-31, sign-extended
    asl a
    bcs @neg
    asl a
    lda #$0000
    rol a
    bra @hi
@neg:
    asl a
    lda #$7FFF
    rol a
@hi:
    sta.l olInvM + 2,x
    rts

Inv_rot:
    lda 4,s
    asl a
    asl a
    sta.b tcc__r4               ; column a's byte offset in a row
    lda 6,s
    asl a
    asl a
    sta.b tcc__r4h              ; column b's
    lda 8,s
    sta.b tcc__r1h               ; s
    lda 10,s
    sta.b tcc__r2h              ; c
    ldy #0
@row:
    tya
    clc
    adc.b tcc__r4
    tax
    lda.l olInvM,x
    sta.b tcc__r5               ; x
    tya
    clc
    adc.b tcc__r4h
    tax
    lda.l olInvM,x
    sta.b tcc__r5h              ; y
    lda.b tcc__r5               ; x * c
    sta.b tcc__r2
    lda.b tcc__r2h
    sta.b tcc__r3
    jsr InvMul
    lda.b tcc__r0
    sta.b tcc__r9
    lda.b tcc__r1
    sta.b tcc__r9h
    lda.b tcc__r5h              ; - y * s
    sta.b tcc__r2
    lda.b tcc__r1h
    sta.b tcc__r3
    jsr InvMul
    lda.b tcc__r9
    sec
    sbc.b tcc__r0
    sta.b tcc__r9
    lda.b tcc__r9h
    sbc.b tcc__r1
    sta.b tcc__r9h
    lda.b tcc__r5h              ; y * c
    sta.b tcc__r2
    lda.b tcc__r2h
    sta.b tcc__r3
    jsr InvMul
    lda.b tcc__r0
    sta.b tcc__r0h
    lda.b tcc__r1
    sta.b tcc__r10h
    lda.b tcc__r5               ; + x * s
    sta.b tcc__r2
    lda.b tcc__r1h
    sta.b tcc__r3
    jsr InvMul
    tya
    clc
    adc.b tcc__r4
    tax
    jsr InvShr14                ; M[r + a]
    lda.b tcc__r0h
    clc
    adc.b tcc__r0
    sta.b tcc__r9
    lda.b tcc__r10h
    adc.b tcc__r1
    sta.b tcc__r9h
    tya
    clc
    adc.b tcc__r4h
    tax
    jsr InvShr14                ; M[r + b]
    tya
    clc
    adc #16
    tay
    cpy #48
    bcs +
    jmp @row
+   rtl

Inv_trans:
    ldy #0
@row:
    tya
    tax
    lda.l olInvM,x              ; M[r] * x
    sta.b tcc__r2
    lda 4,s
    sta.b tcc__r3
    jsr InvMul
    lda.b tcc__r0
    sta.b tcc__r9
    lda.b tcc__r1
    sta.b tcc__r9h
    tya
    tax
    lda.l olInvM + 4,x          ; + M[r + 1] * y
    sta.b tcc__r2
    lda 6,s
    sta.b tcc__r3
    jsr InvMul
    lda.b tcc__r9
    clc
    adc.b tcc__r0
    sta.b tcc__r9
    lda.b tcc__r9h
    adc.b tcc__r1
    sta.b tcc__r9h
    tya
    tax
    lda.l olInvM + 8,x          ; + M[r + 2] * z
    sta.b tcc__r2
    lda 8,s
    sta.b tcc__r3
    jsr InvMul
    tya
    tax
    lda.b tcc__r9
    clc
    adc.b tcc__r0
    sta.b tcc__r9
    lda.b tcc__r9h
    adc.b tcc__r1
    sta.b tcc__r9h
    lda.l olInvM + 12,x         ; M[r + 3] += the sum
    clc
    adc.b tcc__r9
    sta.l olInvM + 12,x
    lda.l olInvM + 14,x
    adc.b tcc__r9h
    sta.l olInvM + 14,x
    tya
    clc
    adc #16
    tay
    cpy #48
    bcc @row
    rtl

; matrixRotateY(ANGLE_90): swap whole signed-32-bit columns, a' = -b,
; b' = a. Retain the carry between low/high words, including INT32_MIN.
Inv_rotY90:
    ldx #0
@row:
    lda.l olInvM,x
    sta.b tcc__r9
    lda.l olInvM + 2,x
    sta.b tcc__r9h
    lda.l olInvM + 8,x
    eor #$FFFF
    clc
    adc #1
    sta.l olInvM,x
    lda.l olInvM + 10,x
    eor #$FFFF
    adc #0
    sta.l olInvM + 2,x
    lda.b tcc__r9
    sta.l olInvM + 8,x
    lda.b tcc__r9h
    sta.l olInvM + 10,x
    txa
    clc
    adc #16
    tax
    cpx #48
    bcc @row
    rtl

; void Inv_pack(uint16* out): olInvM[12] -> the record's first twelve words.
; Rotation: double in signed-32-bit width, then clamp to [-32767,32767].
; Translation: arithmetic >>14 followed by the C cast to signed int16.
; First eight words: signed v*23 >>5, rounding negative values down.
; The PPU's signed 16x8 multiplier returns all 24 product bits immediately
; (ppu-registers.md: $211B M7A and $2134-$2136 multiplication result).
Inv_pack:
    lda 4,s
    sta.b tcc__r4
    lda 6,s
    sta.b tcc__r4h
    ldx #0
    ldy #0
@word:
    txa
    and #$000C
    cmp #$000C
    beq @translation
    lda.l olInvM,x
    asl a
    sta.b tcc__r9
    lda.l olInvM + 2,x
    rol a                      ; preserve the C's wrapped 32-bit doubling
    beq @positiveWord
    cmp #$FFFF
    beq @negativeWord
    cmp #$8000
    bcs @clampNegative
@clampPositive:
    lda #$7FFF
    bra @value
@positiveWord:
    lda.b tcc__r9
    bmi @clampPositive
    bra @value
@negativeWord:
    lda.b tcc__r9
    cmp #$8001
    bcs @value
@clampNegative:
    lda #$8001
    bra @value
@translation:
    stz.b tcc__r9
    lda.l olInvM,x
    asl a
    rol.b tcc__r9
    asl a
    rol.b tcc__r9
    lda.l olInvM + 2,x
    asl a
    asl a
    ora.b tcc__r9               ; low word of signed M[r] >>14
@value:
    cpx #32
    bcs @store
    sta.b tcc__r2
    sep #$20
    lda.b tcc__r2
    sta.l PPU_M7A
    lda.b tcc__r2 + 1
    sta.l PPU_M7A
    lda #23
    sta.l PPU_M7B
    rep #$20
    lda.l PPU_MPYM              ; product bits 8..23 -> output bits 3..18
    asl a
    asl a
    asl a
    sta.b tcc__r9
    lda.l PPU_MPYL
    and #$00E0                 ; product bits 5..7 -> output bits 0..2
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    ora.b tcc__r9
@store:
    sta [tcc__r4],y
    iny
    iny
    txa
    clc
    adc #4
    tax
    cpx #48
    bcs +
    jmp @word
+   rtl

.ENDS
.ENDIF
.ENDIF

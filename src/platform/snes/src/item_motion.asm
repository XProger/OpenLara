; ItemObj_move and ItemObj_getBoundingBox, with item.c as the executable
; reference. 816-tcc ABI: 16-bit A/X/Y, D=0, DB=$7E; far pointer at 4,s.
; All tcc__r registers are caller scratch. No persistent or extra WRAM state.
;
; Animation speed/acceleration are signed 16.16. Keep the complete 32-bit
; ramp until extracting the high word, including the previous-frame ramp
; used by gravity. Movement wraps at the same 16-bit world-coordinate seam.

.INCLUDE "hdr.asm"
.INCLUDE "config.inc"

.IFNDEF C_MOTION
.IFDEF FX3
.BASE $80
.BANK 4 SLOT 0
.ELSE
.BASE $7B
.BANK 4 SLOT 4
.DEFINE MOTION_BLOB_BANK $71
.ENDIF
.SECTION "item_motion_native" SEMIFREE

.accu 16
.index 16
.16bit

ItemObj_move:
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h              ; item pointer, preserved throughout
.IFDEF FX3
    lda.b olAnimSpeedData+2
.ELSE
    lda #MOTION_BLOB_BANK
.ENDIF
    sta.b tcc__r1h
    ldy #12
    lda [tcc__r0],y
    asl a
    asl a
    asl a
    clc
    adc.w olAnimSpeedBase
    sta.b tcc__r1              ; animation speed/acceleration pointer
    ldy #4
    lda [tcc__r1],y
    sta.b tcc__r3
    sta.b tcc__r10
    ldy #6
    lda [tcc__r1],y
    sta.b tcc__r3h
    sta.b tcc__r10h            ; full signed acceleration
    ldy #14
    lda [tcc__r0],y
    and #$00FF
    sta.b tcc__r9
    stz.b tcc__r9h             ; frame is unsigned, at most eight mul steps
    jsr.l tcc__mull            ; accel * frame, modulo 2^32 -> X:Y
    tya
    clc
    adc [tcc__r1]
    sta.b tcc__r2
    txa
    ldy #2
    adc [tcc__r1],y
    sta.b tcc__r2h             ; speed + accel * frame

    ldy #24
    lda [tcc__r0],y
    bit #4
    beq Motion_NoGravity

    lda.b tcc__r2
    sec
    sbc.b tcc__r3
    lda.b tcc__r2h
    sbc.b tcc__r3h             ; high word of speed + accel * (frame - 1)
    sta.b tcc__r4
    lda.b tcc__r2h
    sec
    sbc.b tcc__r4
    ldy #30
    clc
    adc [tcc__r0],y
    sta [tcc__r0],y            ; preserve animation-command impulse, add ramp step

    ldy #28
    lda [tcc__r0],y
    bmi Motion_Gravity6
    cmp #128
    bcc Motion_Gravity6
    lda #1
    bra Motion_Fall
Motion_Gravity6:
    lda #6
Motion_Fall:
    clc
    adc [tcc__r0],y
    sta [tcc__r0],y
    ldy #6
    clc
    adc [tcc__r0],y
    sta [tcc__r0],y
    bra Motion_Horizontal

Motion_NoGravity:
    lda.b tcc__r2h
    ldy #30
    sta [tcc__r0],y
Motion_Horizontal:
    ldy #30
    lda [tcc__r0],y
    bne +
    rtl
+   sta.b tcc__r2              ; signed horizontal speed
    lda [tcc__r0]
    and #$00FF                 ; type byte, not room
    bne +
    lda.w olLaraMoveAngle      ; Lara may move sideways/backwards from her facing
    bra Motion_Angle
+   ldy #10
    lda [tcc__r0],y
Motion_Angle:
    clc
    adc #32                    ; identical rounded trig lookup to math.asm
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    and #$07FE
    tax
    lda.l olSinTable,x
    sta.b tcc__r3
    txa
    clc
    adc #512
    and #$07FE
    tax
    lda.l olSinTable,x
    sta.b tcc__r3h

    lda.b tcc__r3
    jsr Motion_Scale
    ldy #4
    clc
    adc [tcc__r0],y
    sta [tcc__r0],y
    lda.b tcc__r3h
    jsr Motion_Scale
    ldy #8
    clc
    adc [tcc__r0],y
    sta [tcc__r0],y
    rtl

; Signed A * r2 -> bits 14..29 (the int16 result of the arithmetic shift).
; Use magnitudes so a negative factor doesn't make the helper loop 32 times.
; Only r4/r9/r10 and A/X/Y are changed; the item and sin/cos remain intact.
Motion_Scale:
    sta.b tcc__r9
    eor.b tcc__r2
    sta.b tcc__r4
    lda.b tcc__r9
    bpl +
    eor #$FFFF
    inc a
    sta.b tcc__r9
+   lda.b tcc__r2
    bpl +
    eor #$FFFF
    inc a
+   sta.b tcc__r10
    stz.b tcc__r9h
    stz.b tcc__r10h
    jsr.l tcc__mull
    lda.b tcc__r4
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
+   stx.b tcc__r4
    tya
    asl a
    rol.b tcc__r4
    asl a
    rol.b tcc__r4
    lda.b tcc__r4
    rts

ItemObj_getBoundingBox:
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    lda 8,s
    sta.b tcc__r1
    lda 10,s
    sta.b tcc__r1h
    ldy #12
    lda [tcc__r0],y
    asl a
    asl a
    sta.b tcc__r2
    asl a
    clc
    adc.b tcc__r2              ; animation * 12
    clc
    adc.w olAnimBoxBase
    sta.b tcc__r2
.IFDEF FX3
    lda.b olAnimBoxData+2
.ELSE
    lda #MOTION_BLOB_BANK
.ENDIF
    sta.b tcc__r2h
    ldy #10
-   lda [tcc__r2],y
    sta [tcc__r1],y
    dey
    dey
    bpl -
    rtl

.ENDS
.ENDIF

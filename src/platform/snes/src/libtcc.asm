; OpenLara - SNES / Super FX target
; 816-tcc runtime helpers.
;
; Taken from PVSnesLib (pvsneslib/source/libtcc.asm, MIT licence, Alekmaul and
; contributors). GSU-2 executes the helpers in WRAM; FX3 uses FastROM.
; Both native builds use exact multiplication shortcuts for zero/sign-
; extended factors and the CPU's independent 8x8 multiplier. GSU-2's
; C_MOTION oracle retains the original software helpers. Operand registers
; remain scratch; r0..r5 are
; preserved, as required by compiler-generated calls and native callers.
;
; .BASE below must match tools/tccfix.py: bank 4 addressed as $7F on GSU-2,
; or $84 on FX3.

.INCLUDE "hdr.asm"
.INCLUDE "config.inc"

.IFDEF FX3
.DEFINE TCC_FAST_PRODUCTS 1
.IFNDEF C_MOTION
.DEFINE TCC_FAST_MIDDLE 1
.DEFINE TCC_WORD_PRODUCTS 1
.ENDIF
.ELSE
.IFNDEF C_MOTION
.DEFINE TCC_FAST_PRODUCTS 1
.ENDIF
.ENDIF

.IFDEF FX3
.BASE $80
.BANK 4 SLOT 0
.ELSE
.BASE $7B
.BANK 4 SLOT 4
.ENDIF
.SECTION "libtcc" SEMIFREE

.accu 16
.index 16
.16bit

; multiplication implementation lifted from WDC's "Programming the 65816"
tcc__mul:
.IFDEF TCC_FAST_PRODUCTS
    ; 816-tcc implements a long product with two extra word products for
    ; its high halves. Those factors are very often zero or sign extension.
    lda.b tcc__r10
    beq TM_Zero
    cmp #$FFFF
    bne TM_General
    lda.b tcc__r9
    eor #$FFFF
    inc a
    rtl
TM_Zero:
    lda #0
    rtl
TM_General:
    ; Modulo 2^16, negating both factors preserves the product. Signed
    ; coordinates such as -1 otherwise make the shift/add loop visit all
    ; sixteen bits instead of the short magnitude. Preserve r0..r5.
    lda.b tcc__r9
    bpl TM_Positive
    eor #$FFFF
    inc a
    sta.b tcc__r9
    lda.b tcc__r10
    eor #$FFFF
    inc a
    sta.b tcc__r10
TM_Positive:
    ; Loop over the smaller unsigned factor. Nav's 8240-byte context stride
    ; otherwise takes fourteen rounds even when the slot index is only 1/2.
    lda.b tcc__r9
    cmp.b tcc__r10
    bcc TM_Ready
    beq TM_Ready
    tax
    lda.b tcc__r10
    sta.b tcc__r9
    stx.b tcc__r10
TM_Ready:
.ENDIF
	lda #0
	.repeat 4
	.repeat 4
	ldx.b tcc__r9
	beq ++
	lsr.b tcc__r9
	bcc +
	clc
	adc.b tcc__r10
+   asl.b tcc__r10
	.endr
++
	.endr
  rtl

; adapted from 6502 16x16 mult (same manual)
; this is a 32x32 => 32 multiplication routine
; we actually only need 16x16 => 32, maybe there's something to optimize
; here...
; alternatively, one could fix tcc to use a 32x32 multiply.
tcc__mull:
.IFDEF TCC_FAST_PRODUCTS
      ; Compiler long products first multiply two zero-extended low words.
      ; Word factors use up to four CPU 8x8 products; native FX3 returns
      ; zero/unit cases directly. Other builds retain the tiny-factor loop.
      ; NMI/IRQ never use $4202/3 or
      ; $4204..6, so an interrupt cannot replace an in-flight result. Each
      ; write to $4203 is followed by at least eight CPU cycles before read.
      lda.b tcc__r9h
      ora.b tcc__r10h
      bne TML_Signed
.IFDEF TCC_WORD_PRODUCTS
      ; Measured small-word cases favor the hardware path on FX3. Zero/unit
      ; factors can return directly, without entering either product loop.
      ; r0..r5, stack and the caller's register widths remain untouched.
      lda.b tcc__r9
      beq TML_Zero
      cmp #1
      beq TML_CopyB
      lda.b tcc__r10
      beq TML_Zero
      cmp #1
      beq TML_CopyA
      jmp TML_Word
TML_Zero:
      ldy #0
      ldx #0
      rtl
TML_CopyB:
      ldy.b tcc__r10
      ldx #0
      rtl
TML_CopyA:
      ldy.b tcc__r9
      ldx #0
      rtl
.ELSE
      lda.b tcc__r9
      cmp #32
      bcc TML_Signed
      lda.b tcc__r10
      cmp #32
      bcc TML_Signed
      jmp TML_Word
.ENDIF
TML_Signed:
      ; The same identity modulo 2^32 avoids 32 rounds for a sign-extended
      ; 16-bit coordinate. Inputs are scratch already; X:Y remains the result.
      lda.b tcc__r9h
      bpl TML_Positive
      lda.b tcc__r9
      eor #$FFFF
      clc
      adc #1
      sta.b tcc__r9
      lda.b tcc__r9h
      eor #$FFFF
      adc #0
      sta.b tcc__r9h
      lda.b tcc__r10
      eor #$FFFF
      clc
      adc #1
      sta.b tcc__r10
      lda.b tcc__r10h
      eor #$FFFF
      adc #0
      sta.b tcc__r10h
TML_Positive:
      ; Shift the smaller unsigned factor. Compiler-generated calls usually
      ; pass two zero-extended low words, even for a signed long product.
      lda.b tcc__r9h
      cmp.b tcc__r10h
      bcc TML_Ready
      bne TML_Swap
      lda.b tcc__r9
      cmp.b tcc__r10
      bcc TML_Ready
      beq TML_Ready
TML_Swap:
      ldx.b tcc__r9
      lda.b tcc__r10
      sta.b tcc__r9
      stx.b tcc__r10
      ldx.b tcc__r9h
      lda.b tcc__r10h
      sta.b tcc__r9h
      stx.b tcc__r10h
TML_Ready:
.ENDIF
      ldx #0
      ldy #0
-     lda.b tcc__r9
      ora.b tcc__r9h
      beq ++
      lsr.b tcc__r9h
      ror.b tcc__r9
      bcc +
      clc
      tya
      adc.b tcc__r10
      tay
      txa
      adc.b tcc__r10h
      tax
+     asl.b tcc__r10
      rol.b tcc__r10h
      bra -
++    rtl

.IFDEF TCC_FAST_PRODUCTS
; Add a 16-bit cross product at bit 8 of the split 32-bit accumulator.
; Byte additions propagate carries directly, avoiding a temporary stack
; word and the masking/swapping of two separate 16-bit additions. A is
; scratch; X/Y still carry the two high input bytes. Interrupts preserve M.
.MACRO TMW_MIDDLE
.IFDEF TCC_FAST_MIDDLE
      sep #$20
      clc
      adc.b tcc__r9h+1
      sta.b tcc__r9h+1
      xba
      adc.b tcc__r10h
      sta.b tcc__r10h
      bcc +
      inc.b tcc__r10h+1
+     rep #$20
.ELSE
      jsr TMW_AddMiddle
.ENDIF
.ENDM
; Unsigned 16x16 -> X:Y. Inputs r9/r10 stay available as bytes; their zero
; high halves become the low/high accumulator. Preserve ALL r0..r5 halves.
TML_Word:
.IFNDEF FX3
      ; Dense products use only DP, the WRAM stack and CPU multiply I/O.
      ; Let the GSU render throughout the arithmetic, preserving X:Y and
      ; every compiler register when reclaiming the caller's RAM ownership.
      jsl olGsuYieldRam
      pha
.ENDIF
      sep #$20
      lda.b tcc__r9
      sta.l $004202
      lda.b tcc__r10
      sta.l $004203
      rep #$20
      lda.b tcc__r9             ; useful work covers the multiply latency
      xba
      and #$00FF
      tax                       ; a.high
      lda.b tcc__r10
      xba
      and #$00FF
      tay                       ; b.high
      lda.l $004216
      sta.b tcc__r9h             ; a.low * b.low
      cpx #0
      beq TMW_HighB
      sep #$20
      txa
      sta.l $004202
      lda.b tcc__r10
      sta.l $004203
      rep #$20                  ; 3 + 3*2 = 9 CPU cycles before each read
      nop
      nop
      nop
      lda.l $004216
      TMW_MIDDLE
TMW_HighB:
      cpy #0
      beq TMW_Done
      sep #$20
      lda.b tcc__r9
      sta.l $004202
      tya
      sta.l $004203
      rep #$20
      nop
      nop
      nop
      lda.l $004216
      TMW_MIDDLE
      cpx #0
      beq TMW_Done
      sep #$20
      txa
      sta.l $004202
      tya
      sta.l $004203
      rep #$20
      nop
      nop
      nop
      lda.l $004216
      clc
      adc.b tcc__r10h
      sta.b tcc__r10h
TMW_Done:
      ldx.b tcc__r10h
      ldy.b tcc__r9h
.IFNDEF FX3
      pla
      jml olGsuReclaimRam
.ELSE
      rtl
.ENDIF
.IFNDEF TCC_FAST_MIDDLE
TMW_AddMiddle:
      pha
      and #$00FF
      xba
      clc
      adc.b tcc__r9h
      sta.b tcc__r9h
      pla
      xba
      and #$00FF
      adc.b tcc__r10h           ; includes carry from the low accumulator
      sta.b tcc__r10h
      rts
.ENDIF
.ENDIF


; division implementation lifted from WDC's "Programming the 65816"
; optimized by mic_
tcc__udiv:
	stz.b tcc__r9
	ldy #1
	.repeat 16
	asl a
	bcs tcc__udiv1
	iny
	.endr
tcc__udiv1:
	ror a
- 	sta.b tcc__r5
	cpx.b tcc__r5
	bcc +
	txa
	sbc.b tcc__r5
	tax
+ 	rol.b tcc__r9
	lda.b tcc__r5
	lsr a
	dey
	bne -
	rtl

; looks like the damn 6502 was designed before negative numbers...
tcc__div:
      pha
      ldy #0	; number of negative operands
      stz.b tcc__r10	; dividend negative?
      txa
      bpl +
      eor #$ffff	; negate dividend
      ina
      iny
      inc.b tcc__r10
      tax
+     pla
      bpl +
      eor #$ffff	; negate divisor
      ina
      iny
+     phy
      jsr.l tcc__udiv
      ; do not overwrite x here (remainder!)
      lda.b tcc__r9	; get quotient
      ply	; get number of negative operands
      cpy #1
      bne +
      eor #$ffff ; one neg. operand -> negate quotient
      ina
      sta.b tcc__r9
      ; these extra insns graciously donated by C99
+     lda.b tcc__r10	 ; dividend negative?
      beq +
      txa	 ; give remainder same sign as dividend
      eor #$ffff
      ina
      tax
+     rtl

tcc__divl:
      ldx.w #0	; bit 0 -> dividend neg?, bit 1 -> divisor neg?
      bit.b tcc__r9h
      bpl +	; dividend positive

      ; negate dividend
      inx	; set bit 0
      lda.b tcc__r9h
      eor.w #$ffff
      sta.b tcc__r9h
      lda.b tcc__r9
      eor.w #$ffff
      ina
      sta.b tcc__r9
      bne +
      inc.b tcc__r9h

+     bit.b tcc__r10h
      bpl +	; divisor positive

      ; negate divisor
      inx	; set bit 1
      inx
      lda.b tcc__r10h
      eor.w #$ffff
      sta.b tcc__r10h
      lda.b tcc__r10
      eor.w #$ffff
      ina
      sta.b tcc__r10
      bne +
      inc.b tcc__r10h

+     phx
      jsr tcc__udivl
      tay	; quot low word -> y (high word in x)
      pla	; get sign flags
      beq +++	; no sign -> all done
      cmp.w #3
      beq ++	; two signs -> only correct remainder

      ; negate quotient
      pha
      txa
      eor.w #$ffff
      tax
      tya
      eor.w #$ffff
      ina
      bne +
      inx
+     tay
      pla

++    bit.w #1	; dividend negative?
      beq +++	; no -> done

      ; negate remainder
      lda.b tcc__r9h
      eor.w #$ffff
      sta.b tcc__r9h
      lda.b tcc__r9
      eor.w #$ffff
      ina
      sta.b tcc__r9
      bne +++
      inc.b tcc__r9h

+++   rts

tcc__udivl:
      lda.w #0
.IFNDEF FX3
.IFNDEF C_MOTION
      pha                        ; private ownership token, below quotient
.ENDIF
.ENDIF
      tax
      pha
      ldy.w #1
.IFNDEF C_MOTION
      ; Leading zero quotient bits do not need subtraction rounds. Align
      ; only until the divisor reaches the dividend, or its top bit is set.
      ; Unsigned comparisons retain all 32 bits, including signed minima
      ; after tcc__divl takes their magnitudes. Keep zero's legacy path.
      lda.b tcc__r10
      ora.b tcc__r10h
      beq TDU_LegacyAlign
TDU_Align:
      lda.b tcc__r10h
      bmi TDU_Ready
      cmp.b tcc__r9h
      bcc TDU_Shift
      bne TDU_Ready
      lda.b tcc__r10
      cmp.b tcc__r9
      bcs TDU_Ready
TDU_Shift:
      asl.b tcc__r10
      rol.b tcc__r10h
      iny
      bra TDU_Align
TDU_LegacyAlign:
.ENDIF
      lda.b tcc__r10h
      bmi TDU_Ready
-     iny
      asl.b tcc__r10
      rol.b tcc__r10h
      bmi TDU_Ready
      cpy.w #33
      bne -
TDU_Ready:
.IFNDEF FX3
.IFNDEF C_MOTION
      ; Skip the handoff for short quotients. Longer restoring divisions
      ; need only DP/stack and can run alongside the cartridge renderer.
      cpy #4
      bcc _div2
      jsl olGsuYieldRam
      sta 3,s
.ENDIF
.ENDIF
_div2:sec
      lda.b tcc__r9
      sbc.b tcc__r10
      pha
      lda.b tcc__r9h
      sbc.b tcc__r10h
      bcc +
      sta.b tcc__r9h
      pla
      sta.b tcc__r9
      pha
+     pla
      pla
      rol a
      pha
      txa
      rol a
      tax
      lsr.b tcc__r10h
      ror.b tcc__r10
      dey
      bne _div2

      pla
.IFNDEF FX3
.IFNDEF C_MOTION
      tay                        ; preserve quotient A across the handoff
      pla
      beq +
      jsl olGsuReclaimRam
+     tya
.ENDIF
.ENDIF
      rts

tcc__divdi3:
      lda.b 4,s
      sta.b tcc__r9
      lda.b 6,s
      sta.b tcc__r9h
      lda.b 8,s
      sta.b tcc__r10
      lda.b 10,s
      sta.b tcc__r10h
      jsr tcc__divl
      stx.b tcc__r1	; LRET
      sty.b tcc__r0	; IRET
      rtl

tcc__moddi3:
      lda.b 4,s
      sta.b tcc__r9
      lda.b 6,s
      sta.b tcc__r9h
      lda.b 8,s
      sta.b tcc__r10
      lda.b 10,s
      sta.b tcc__r10h
      jsr tcc__divl
      lda.b tcc__r9h
      sta.b tcc__r1	; LRET
      lda.b tcc__r9
      sta.b tcc__r0	; IRET
      rtl

tcc__udivdi3:
      lda.b 4,s
      sta.b tcc__r9
      lda.b 6,s
      sta.b tcc__r9h
      lda.b 8,s
      sta.b tcc__r10
      lda.b 10,s
      sta.b tcc__r10h
      jsr tcc__udivl
      stx.b tcc__r1	; LRET
      sta.b tcc__r0	; IRET
      rtl

tcc__umoddi3:
      lda.b 4,s
      sta.b tcc__r9
      lda.b 6,s
      sta.b tcc__r9h
      lda.b 8,s
      sta.b tcc__r10
      lda.b 10,s
      sta.b tcc__r10h
      jsr tcc__udivl
      lda.b tcc__r9h
      sta.b tcc__r1	; LRET
      lda.b tcc__r9
      sta.b tcc__r0	; IRET
      rtl

;optimized version by mic_
;however, it does not seem to be ever called in a fairly large project
tcc__shldi3:
	lda.b 6,s ; hi word
	sta.b tcc__r1
	lda.b 8,s ; shift count
	beq +
	tax
	lda.b 4,s ; low word
-	asl a
	rol.b tcc__r1
	dex
	bne -
	sta.b tcc__r0
	rtl
+	lda.b 4,s ; low word
	sta.b tcc__r0
	rtl

tcc__sardi3:
      lda.b 4,s ; low word
      sta.b tcc__r0
      lda.b 6,s
      sta.b tcc__r1
      lda.b 8,s ; shift count
      beq +
      tax
      lda.b tcc__r1
      bpl _shr
-     sec
      ror.b tcc__r1
      ror.b tcc__r0
      dex
      bne -
      rtl
_shr: lsr.b tcc__r1
      ror.b tcc__r0
      dex
      bne _shr
+     rtl

tcc__shrdi3:
      lda.b 4,s
      sta.b tcc__r0
      lda.b 6,s
      sta.b tcc__r1
      lda.b 8,s
      beq +
      tax
      bra _shr
+     rtl

; long call to the subroutine pointed to in r10
tcc__jsl_r10:
      sep #$20
      lda.b tcc__r10 + 2
      pha
      rep #$20
      lda.b tcc__r10
      dec a
      pha
      rtl

; long call to the subroutine pointed to by the pointer r9 points to...
tcc__jsl_ind_r9:
      lda.b [tcc__r9]
      sta.b tcc__r10
      ldy.w #2
      lda.b [tcc__r9],y
      sta.b tcc__r10h
      ;jmp.w tcc__jsl_r10
      sep #$20
      lda.b tcc__r10 + 2
      pha
      rep #$20
      lda.b tcc__r10
      dec a
      pha
      rtl
.ENDS

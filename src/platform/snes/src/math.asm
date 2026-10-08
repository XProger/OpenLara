; OpenLara - SNES / Super FX target
; Compact equivalents of math.c's trig and integer square root.
;
; 816-tcc ABI: native mode, 16-bit A/X/Y, D=0, DB=$7E; arguments at
; 4,s (low word), 6,s (high word), then the next argument. Return a
; 32-bit integer in tcc__r0 / tcc__r1 (NOT r0h, which is a pointer bank).
; Only caller-scratch registers used.
; Code and tables share $7E:0600 on GSU-2, or $BD:8000 FastROM on FX3.

.INCLUDE "hdr.asm"
.INCLUDE "config.inc"

.IFDEF FX3
.BASE $80
.BANK 7 SLOT 0                  ; $87 (was $BD)
.ELSE
.BASE $41
.BANK 61 SLOT 8
.ENDIF
.SECTION "math_native" SEMIFREE

.accu 16
.index 16
.16bit

.IFDEF FX3
.IFNDEF C_MOTION
; Exact uint32 LCG: seed = seed * $41C64E6D + $3039 (mod 2^32).
; Expand seed as four bytes. The 256 uint32 constant products occupy only
; 1 KB of concurrent CPU FastROM, not WRAM or FX-visible shared storage.
; Unaligned word reads select each product's middle bytes for the << 8
; term. Carries between the low/high sums are retained; terms above bit
; 31 are discarded. No multiply register, stack frame or extra RAM needed.
; NMI preserves the registers/status and does not enter the logic PRNG.
rand_logic:
    lda.w olRandSeed
    and #$00FF
    asl a
    asl a
    tax
    lda.l olRandProducts,x
    clc
    adc #$3039
    sta.b tcc__r2
    lda.l olRandProducts+2,x
    adc #0
    sta.b tcc__r2h
    lda.w olRandSeed
    xba
    and #$00FF
    asl a
    asl a
    tax
    lda.l olRandProducts,x
    xba
    and #$FF00
    clc
    adc.b tcc__r2
    sta.b tcc__r2
    lda.l olRandProducts+1,x
    adc.b tcc__r2h
    sta.b tcc__r2h
    lda.w olRandSeed+2
    and #$00FF
    asl a
    asl a
    tax
    lda.l olRandProducts,x
    clc
    adc.b tcc__r2h
    sta.b tcc__r2h
    lda.w olRandSeed+2
    xba
    and #$00FF
    asl a
    asl a
    tax
    lda.l olRandProducts,x
    xba
    and #$FF00
    clc
    adc.b tcc__r2h
    sta.w olRandSeed+2
    and #$01FF
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    sta.b tcc__r0
    lda.b tcc__r2
    sta.w olRandSeed
    xba
    and #$00FC
    lsr a
    lsr a
    ora.b tcc__r0
    sta.b tcc__r0
    stz.b tcc__r1               ; int32 return, NOT pointer-high r0h
    rtl
.ENDIF
.ENDIF

phd_cos:
    lda 4,s
    clc
    adc #$4020                  ; quarter turn plus half a table step
    bra Math_SinLookup
phd_sin:
    lda 4,s
    clc
    adc #32
Math_SinLookup:
    ; (uint16(angle + round) >> 6) * sizeof(int16)
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    and #$07FE
    tax
    lda.l olSinTable,x
    sta.b tcc__r0
    ldx #0
    cmp #0
    bpl +
    dex
+   stx.b tcc__r1              ; sign-extend the 1.14 result
    rtl

phd_atan:
    stz.b tcc__r3              ; negative X / negative Y / swapped
    lda 4,s
    bpl +
    eor #$FFFF
    inc a
    inc.b tcc__r3
+   sta.b tcc__r1             ; unsigned magnitude, including -32768
    lda 8,s
    bpl +
    eor #$FFFF
    inc a
    ldx #2
    stx.b tcc__r3h
    bra ++
+   stz.b tcc__r3h
++  sta.b tcc__r2
    lda.b tcc__r3
    ora.b tcc__r3h
    sta.b tcc__r3

.IFNDEF FX3
.IFNDEF C_MOTION
    ; Arguments are now in DP; the remaining angle arithmetic and lookup
    ; touch only WRAM. Keep a private ownership token below the return.
    jsl olGsuYieldRam
    pha
.ENDIF
.ENDIF
    lda.b tcc__r2
    cmp.b tcc__r1
    bcc Math_AtanRatio
    beq Math_AtanRatio
    tay
    lda.b tcc__r1
    sta.b tcc__r2
    sty.b tcc__r1
    lda.b tcc__r3
    ora #4
    sta.b tcc__r3
Math_AtanRatio:
    stz.b tcc__r0
    lda.b tcc__r1
    bne +
.IFNDEF FX3
.IFNDEF C_MOTION
    jmp Math_AtanReturn
.ELSE
    rtl                         ; (0,0)
.ENDIF
.ELSE
    rtl
.ENDIF
+   lda.b tcc__r2
    cmp.b tcc__r1
    bne Math_AtanDivide
    lda #128                    ; exactly 45 degrees
    bra Math_AtanLookup
Math_AtanDivide:
    ; Seven fractional quotient bits give floor(ay * 128 / ax) exactly.
    ; remainder < ax <= 32768, so doubling never overflows 16 bits.
    ldx #7
-   asl a
    cmp.b tcc__r1
    bcc +
    sbc.b tcc__r1
+   rol.b tcc__r0
    dex
    bne -
    lda.b tcc__r0
Math_AtanLookup:
    asl a
    tax
    lda.l olAtanTable,x
    sta.b tcc__r0
    lda.b tcc__r3
    bit #4
    beq +
    lda #$4000
    sec
    sbc.b tcc__r0
    sta.b tcc__r0
+   lda.b tcc__r3
    bit #2
    beq +
    lda #0
    sec
    sbc.b tcc__r0
    sta.b tcc__r0
+   lda.b tcc__r3
    bit #1
    beq +
    lda #$8000
    sec
    sbc.b tcc__r0
    sta.b tcc__r0
+   stz.b tcc__r1              ; unsigned 16-bit angle, integer high word zero
Math_AtanReturn:
.IFNDEF FX3
.IFNDEF C_MOTION
    pla
    jml olGsuReclaimRam
.ELSE
    rtl
.ENDIF
.ELSE
    rtl
.ENDIF

phd_sqrt:
    lda 4,s
    sta.b tcc__r1
    lda 6,s
    sta.b tcc__r1h
    stz.b tcc__r0              ; root
    stz.b tcc__r2              ; remainder, full width
    stz.b tcc__r2h
    ldx #16
Math_SqrtPair:
    asl.b tcc__r0
    .REPEAT 2
        asl.b tcc__r1
        rol.b tcc__r1h
        rol.b tcc__r2
        rol.b tcc__r2h
    .ENDR
    lda.b tcc__r0
    asl a
    sta.b tcc__r3
    lda #0
    rol a
    sta.b tcc__r3h
    inc.b tcc__r3              ; trial = 2 * root + 1 (up to 17 bits)
    lda.b tcc__r2
    sec
    sbc.b tcc__r3
    tay
    lda.b tcc__r2h
    sbc.b tcc__r3h
    bcc +
    sta.b tcc__r2h
    sty.b tcc__r2
    inc.b tcc__r0
+   dex
    bne Math_SqrtPair
    stz.b tcc__r1              ; integer return's high word, not r0h
    rtl

.ENDS

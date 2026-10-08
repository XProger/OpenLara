; Exact sector -> navigation box lookup. FX3's full pointer / GSU-2's footer:
;   byte 'N', literalBits (1..10)
;   two-bit tags[sectorCount]: none, previous Z, previous X, literal
;   word ranks[ceil(sectorCount/32)+1]: literals before each 32-sector group
;   little-endian packed literal IDs
; Predictors never cross room boundaries. Following them is strictly
; backwards; a literal needs at most 31 tags counted after its rank entry.
;
; 816-tcc ABI: native 16-bit A/X/Y, D=0, DB=$7E. Sector at 4,s, depth at 6,s.
; All tcc registers are caller-clobbered. No persistent cache or work RAM.

.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFDEF FX3
.BASE $80
.BANK 9 SLOT 0                  ; $89 (was $BF)
.ELSE
.DEFINE NAV_BLOB $710000
.BASE $3F
.BANK 63 SLOT 6
.ENDIF
.SECTION "nav_sector_native" SEMIFREE
.accu 16
.index 16
.16bit

navSectorBox:
    lda #$FFFF
    sta.b tcc__r0
    lda 4,s
    cmp.w olSectorCount
    bcc +
    rtl
+   sta.b tcc__r4             ; sector being resolved
    lda 6,s
    bne +
    rtl
+   sta.b tcc__r5             ; room's Z stride
.IFDEF FX3
    ldx.b olNavMapData
    lda [olNavMapData]
.ELSE
    lda.l NAV_BLOB
    tax
    lda.l NAV_BLOB-2,x        ; footer: nav table byte offset
    tax
    lda.l NAV_BLOB,x
.ENDIF
    sta.b tcc__r4h
    and #$00FF
    cmp #$004E
    beq +
    rtl
+   lda.b tcc__r4h
    xba
    and #$00FF
    beq Nav_Invalid
    cmp #11
    bcs Nav_Invalid
    sta.b tcc__r4h            ; bits per literal
    inx
    inx
    stx.b tcc__r1             ; tags
.IFDEF FX3
    lda.b olNavMapData+2
.ELSE
    lda #(NAV_BLOB >> 16)
.ENDIF
    sta.b tcc__r1h
    sta.b tcc__r2h
    sta.b tcc__r3h
    lda.w olSectorCount
    clc
    adc #3
    lsr a
    lsr a
    clc
    adc.b tcc__r1
    sta.b tcc__r2             ; ranks
    lda.w olSectorCount
    clc
    adc #31
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    inc a
    asl a
    clc
    adc.b tcc__r2
    sta.b tcc__r3             ; literals

Nav_Predict:
    lda.b tcc__r4
    lsr a
    lsr a
    tay
    lda.b tcc__r4
    and #3
    asl a
    tax
    lda [tcc__r1],y
    cpx #0
    beq Nav_Tag
-   lsr a
    dex
    bne -
Nav_Tag:
    and #3
    beq Nav_Invalid
    cmp #3
    beq Nav_Literal
    cmp #2
    beq Nav_PreviousX
    lda.b tcc__r4
    beq Nav_Invalid
    dec.b tcc__r4
    bra Nav_Predict
Nav_PreviousX:
    lda.b tcc__r4
    sec
    sbc.b tcc__r5
    bcc Nav_Invalid
    sta.b tcc__r4
    bra Nav_Predict
Nav_Invalid:
    rtl

Nav_Literal:
    lda.b tcc__r4
    lsr a
    lsr a
    lsr a
    lsr a
    and #$FFFE
    tay
    lda [tcc__r2],y
    sta.b tcc__r5h            ; literal rank at this group's start
    lda.b tcc__r4
    and #31
    beq Nav_Ranked
    sta.b tcc__r9h            ; preceding tags still to count
    lda.b tcc__r4
    lsr a
    lsr a
    and #$FFF8
    tay
Nav_CountWord:
    ldx.b tcc__r9h
    cpx #8
    bcc Nav_CountPrefix
    lda [tcc__r1],y
    jsr Nav_CountLiterals
    clc
    adc.b tcc__r5h
    sta.b tcc__r5h
    lda.b tcc__r9h
    sec
    sbc #8
    sta.b tcc__r9h
    beq Nav_Ranked
    iny
    iny
    bra Nav_CountWord
Nav_CountPrefix:
    txa
    asl a
    tax
    lda [tcc__r1],y
    and.l Nav_PrefixMasks,x   ; exactly the remaining 1..7 preceding tags
    jsr Nav_CountLiterals
    clc
    adc.b tcc__r5h
    sta.b tcc__r5h
Nav_Ranked:
    ; rank * literalBits, retaining carry above bit 15 before dividing
    ; by eight: the bit offset can exceed a word even though bytes cannot.
    stz.b tcc__r10h
    ldx.b tcc__r4h
    lda #0
-   clc
    adc.b tcc__r5h
    bcc +
    inc.b tcc__r10h
+   dex
    bne -
    sta.b tcc__r10
    and #7
    tax
    lda.b tcc__r10
    lsr.b tcc__r10h
    ror a
    lsr.b tcc__r10h
    ror a
    lsr.b tcc__r10h
    ror a
    tay
    lda [tcc__r3],y
    sta.b tcc__r9
    iny
    iny
    lda [tcc__r3],y
    sta.b tcc__r9h
    cpx #0
    beq Nav_Mask
-   lsr.b tcc__r9h
    ror.b tcc__r9
    dex
    bne -
Nav_Mask:
    lda #1
    ldx.b tcc__r4h
-   asl a
    dex
    bne -
    dec a
    and.b tcc__r9
    cmp.w olBoxCount
    bcs +
    sta.b tcc__r0
+   rtl

; A is eight packed tags. Return their literal (11) count in A, retaining
; Y's source cursor and every codec pointer. The fold never carries between
; retained groups: 1-bit flags -> 2-tag nibbles -> 4-tag bytes -> count <=8.
Nav_CountLiterals:
    sta.b tcc__r9
    lsr a
    and.b tcc__r9
    and #$5555
    sta.b tcc__r9
    lsr a
    lsr a
    ; The sparse input has no bit 1, so the second LSR supplied carry=0.
    adc.b tcc__r9
    and #$3333
    sta.b tcc__r9
    xba
    ; The preceding sum is at most $6AAA: carry is still zero here.
    adc.b tcc__r9
    sta.b tcc__r9
    lsr a
    lsr a
    lsr a
    lsr a
    ; Each nibble is <=4, so bit 3 was zero and the fourth LSR cleared C.
    adc.b tcc__r9
    and #$000F
    rts
; Long-indexed reads resolve to FastROM on FX3 and relocated WRAM on GSU-2.
Nav_PrefixMasks:
    .dw $0000, $0003, $000F, $003F, $00FF, $03FF, $0FFF, $3FFF
.ENDS

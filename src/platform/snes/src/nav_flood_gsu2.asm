; OpenLara - SNES / Super FX target
; Exact GSU-2 target-outward BFS. nav.c retains the C_MOTION reference.
; Reuses navQueue/navWrote and caller-clobbered tcc registers only.
; 816-tcc ABI: native A/X/Y16, D=0, DB=$7E; flying/start at 4/6,s.
; Executed from the bank-$3E image copied to $7E:5800 before GSU startup.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF FX3
.IFNDEF C_MOTION
.BASE $40
.BANK 62 SLOT 7
.SECTION "nav_flood_gsu2_native" SEMIFREE
.accu 16
.index 16
.16bit

; Byte queue positions are formed only when indexing, so head/tail retain
; the C's box counts. All table-address additions wrap in sixteen bits.
.DEFINE NF_HEAD      tcc__r0
.DEFINE NF_TAIL      tcc__r0h
.DEFINE NF_NEXT      tcc__r1
.DEFINE NF_WROTE     tcc__r1h
.DEFINE NF_ZONEBASE  tcc__r2
.DEFINE NF_ZONE      tcc__r2h
.DEFINE NF_CUR       tcc__r3
.DEFINE NF_P         tcc__r3h
.DEFINE NF_WORD      tcc__r4
.DEFINE NF_BOX       tcc__r4h
.DEFINE NF_CLEAR     tcc__r5
.DEFINE NF_MASK      tcc__r5h

navFlood:
    lda 4,s
    asl a
    tax
    lda 6,s
    sta.w navFrom,x                 ; written even for an invalid start

    lda 4,s
    .REPEAT 7
    asl a
    .ENDR
    clc
    adc #(navWrote & $FFFF)
    sta.b NF_WROTE
    lda 4,s
    .REPEAT 11
    asl a
    .ENDR
    clc
    adc #(navNext & $FFFF)
    sta.b NF_NEXT

    ; Clear exactly the cells marked by this kind's previous flood. Every
    ; wrote word is zeroed, while the other kind and stale queue stay intact.
    ldy #0
NF_ClearGroup:
    tya
    clc
    adc.b NF_WROTE
    tax
    lda.w 0,x
    sta.b NF_CLEAR
    stz.w 0,x
    beq NF_ClearAdvance
    tya
    asl a
    asl a
    asl a
    asl a
    clc
    adc.b NF_NEXT
    tax
NF_ClearBits:
    lsr.b NF_CLEAR
    bcc +
    lda #$FFFF
    sta.w 0,x
+   inx
    inx
    lda.b NF_CLEAR
    bne NF_ClearBits
NF_ClearAdvance:
    iny
    iny
    cpy #128
    bne NF_ClearGroup

    lda 6,s
    cmp.w olBoxCount
    bcs NF_Invalid
    cmp #1024
    bcs NF_Invalid
    sta.b NF_BOX
    lda 4,s
    beq +
    lda.w olBoxCount
    asl a
+   clc
    adc.w olZoneBase
    sta.b NF_ZONEBASE
    lda.b NF_BOX
    asl a
    clc
    adc.b NF_ZONEBASE
    tax
    lda.l $710000,x
    sta.b NF_ZONE
    lda.b NF_BOX
    asl a
    clc
    adc.b NF_NEXT
    tax
    lda.b NF_BOX
    sta.w 0,x                       ; target terminates every path
    jsr NF_Mark
    lda.b NF_BOX
    sta.w navQueue
    stz.b NF_HEAD
    lda #1
    sta.b NF_TAIL
    bra NF_Head
NF_Invalid:
    rtl                             ; invalid starts preserve navReached

NF_Head:
    lda.b NF_HEAD
    cmp.b NF_TAIL
    bcc +
    lda.b NF_TAIL
    sta.w navReached
    rtl
+   asl a
    tax
    lda.w navQueue,x
    sta.b NF_CUR
    inc.b NF_HEAD
    asl a
    asl a
    asl a
    clc
    adc.w olBoxBase
    clc
    adc #6
    tax
    lda.l $710000,x
    bit #$4000                      ; blocked, not just blockable ($8000)
    bne NF_Head
    and #$3FFF
    sta.b NF_P

NF_Overlap:
    lda.b NF_P
    asl a
    clc
    adc.w olOverlapBase
    tax
    lda.l $710000,x
    sta.b NF_WORD
    and #$7FFF
    sta.b NF_BOX
    cmp.w olBoxCount
    bcs NF_NeighborEnd
    cmp #1024
    bcs NF_NeighborEnd
    asl a
    clc
    adc.b NF_NEXT
    tax
    lda.w 0,x
    cmp #$FFFF
    bne NF_NeighborEnd
    lda.b NF_BOX
    asl a
    clc
    adc.b NF_ZONEBASE
    tax
    lda.l $710000,x
    cmp.b NF_ZONE
    bne NF_NeighborEnd
    lda.b NF_BOX
    asl a
    clc
    adc.b NF_NEXT
    tax
    lda.b NF_CUR
    sta.w 0,x                       ; first discovery fixes BFS tie order
    jsr NF_Mark
    lda.b NF_TAIL
    cmp #1024
    bcs NF_NeighborEnd               ; mark visited even if queue is full
    asl a
    tax
    lda.b NF_BOX
    sta.w navQueue,x
    inc.b NF_TAIL
NF_NeighborEnd:
    lda.b NF_WORD
    bit #$8000
    beq +
    jmp.w NF_Head
+   inc.b NF_P
    jmp.w NF_Overlap

; Set this box's wrote bit. Only A/X and NF_MASK are clobbered; persistent
; flood state and the current encoded overlap word remain untouched.
NF_Mark:
    lda.b NF_BOX
    and #15
    asl a
    tax
    lda.w NF_Bits,x
    sta.b NF_MASK
    lda.b NF_BOX
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    clc
    adc.b NF_WROTE
    tax
    lda.w 0,x
    ora.b NF_MASK
    sta.w 0,x
    rts

; Code-image constants are copied with the routine, never fetched from ROM
; while the GSU owns that bus. DB=$7E also addresses this table.
NF_Bits:
    .DW $0001,$0002,$0004,$0008,$0010,$0020,$0040,$0080
    .DW $0100,$0200,$0400,$0800,$1000,$2000,$4000,$8000
.ENDS
.ENDIF
.ENDIF

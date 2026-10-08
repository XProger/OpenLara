; Exact GSU-2 gameplay point-sector cache. The packed navigation map is
; immutable between level loads; Room_reset and navResetCache invalidate
; entries when tables/maps change. Raw navSectorBox remains the codec oracle.
; Key every record by the original sector, predictor stride and sector count.
; A NO_BOX result is valid cached data; only the impossible sector tag $FFFF
; denotes an empty record. NMI/IRQ never enter this lookup or change its data.
;
; 816-tcc ABI: A/X/Y16, D=0, DB=$7E; sector at4,s and stride at6,s.
; Scratch registers, A and X are caller-clobbered; stack and widths preserved.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF FX3
.IFNDEF C_MOTION
.RAMSECTION "globram_nav_cache_gsu2" BANK $7E SLOT 2
olNavSectorCacheGsu2 dsb 256     ;32 direct-mapped four-word records
.ENDS

.BASE $3F
.BANK 63 SLOT 6                 ; resident image at $7E:8000
.SECTION "nav_cache_gsu2_native" SEMIFREE
.ACCU 16
.INDEX 16
.16bit

navInvalidateSectorsGsu2:
    ldx #248
    lda #$FFFF
-   sta.w olNavSectorCacheGsu2,x
    dex
    dex
    dex
    dex
    dex
    dex
    dex
    dex
    bpl -
    rtl

navCachedSectorBoxGsu2:
    lda 4,s
    cmp.w olSectorCount
    bcs NCG_Invalid
    lda 6,s
    beq NCG_Invalid
    lda 4,s
    and #31
    asl a
    asl a
    asl a
    tax
    lda 4,s
    cmp.w olNavSectorCacheGsu2,x
    bne NCG_Miss
    lda 6,s
    cmp.w olNavSectorCacheGsu2+2,x
    bne NCG_Miss
    lda.w olSectorCount
    cmp.w olNavSectorCacheGsu2+6,x
    bne NCG_Miss
    lda.w olNavSectorCacheGsu2+4,x
    sta.b tcc__r0
    rtl
NCG_Invalid:
    lda #$FFFF
    sta.b tcc__r0
    rtl

NCG_Miss:
    phx
    lda 8,s                    ; saved X precedes the original stride
    pha
    lda 8,s                    ; pushed stride precedes the original sector
    pha
    jsr.l navSectorBox
    pla
    pla
    plx
    lda 4,s
    sta.w olNavSectorCacheGsu2,x
    lda 6,s
    sta.w olNavSectorCacheGsu2+2,x
    lda.b tcc__r0
    sta.w olNavSectorCacheGsu2+4,x
    lda.w olSectorCount
    sta.w olNavSectorCacheGsu2+6,x
    rtl
.ENDS
.ENDIF
.ENDIF

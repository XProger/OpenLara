; FX3-only cold table binding. ROM reads are legal during FX execution;
; only SECTOR is copied, because movable blocks modify its floor bytes.
; D=0, A/X/Y=16 on entry. Uses caller-scratch r0-r2; no stack locals.
.IFDEF FX3
.IFNDEF HOST_DIRECTORY_ADDR
.INCLUDE "hostbank.inc"
.ENDIF

.RAMSECTION "host_ptrs_low" BANK 0 SLOT 1 ORGA $28 FORCE PRIORITY 1000
olRoomData dsb 0
olRoomBase dsb 4
olSectorData dsb 0
olSectorBase dsb 4
.ENDS
.RAMSECTION "host_ptrs_middle" BANK 0 SLOT 1 ORGA $58 FORCE PRIORITY 1000
olLinkData dsb 0
olLinkBase dsb 4
olModelData dsb 0
olModelBase dsb 4
.ENDS
.RAMSECTION "host_ptrs_high" BANK 0 SLOT 1 ORGA $C0 FORCE PRIORITY 1000
olAnimBoxData dsb 0
olAnimBoxBase dsb 4
olAnimSpeedData dsb 0
olAnimSpeedBase dsb 4
olAnimData dsb 0
olAnimBase dsb 4
olTransData dsb 0
olTransBase dsb 4
olCmdData dsb 0
olCmdBase dsb 4
olTriggerData dsb 0
olTriggerBase dsb 4
olItemData dsb 0
olItemBase dsb 4
olBoxData dsb 0
olBoxBase dsb 4
olOverlapData dsb 0
olOverlapBase dsb 4
olZoneData dsb 0
olZoneBase dsb 4
olStaticData dsb 0
olStaticBase dsb 4
olStaticFirstData dsb 0
olStaticFirstBase dsb 4
olAnimAimData dsb 0
olAnimAimBase dsb 4
olNavMapData dsb 0
olNavMapBase dsb 4
olHostTablesReady dsb 2
.ENDS

.BASE $80
.BANK 6 SLOT 0                  ; $86 (was $BC; FX3 multi-level layout)
.ORG 0
.SECTION "host_tables_native" SEMIFREE
.accu 16
.index 16
.16bit
.MACRO HOST_EQUAL
    beq +
    jmp Host_Invalid
+
.ENDM
.MACRO HOST_NOT_BELOW
    bcs +
    jmp Host_Invalid
+
.ENDM

olHostBind:
    phb
    pea $0000
    plb
    plb
    stz.b olHostTablesReady
    ; The level's directory, from the start of its host bank (olLevel's
    ; first word: main.asm SelectLevel) into the copy everything below and
    ; olHeader16 read -- the cartridge may carry several levels' tables.
    stz.b tcc__r0
    lda.l olLevel
    sta.b tcc__r0h
    ldx #254
-   txy
    lda [tcc__r0],y
    sta.l olHostDirectory,x
    dex
    dex
    bpl -
    lda.l olHostDirectory
    cmp #$4C4F                 ; OL
    HOST_EQUAL
    lda.l olHostDirectory+2
    cmp #$3248                 ; H2
    HOST_EQUAL
    lda.l olHostDirectory+4
    cmp #5                     ; current-frame boxes, six zones and byte slopes
    HOST_EQUAL
    lda.l olHostDirectory+6
    cmp #18
    HOST_EQUAL
    lda.l olHostDirectory+8
    sta.b tcc__r0
    lda.l olHostDirectory+10
    sta.b tcc__r0h
    cmp #$20
    bcc Host_SizeFits
    HOST_EQUAL
    lda.b tcc__r0
    HOST_EQUAL                 ; exactly 2 MiB also fits
Host_SizeFits:
    lda #256
    sta.b tcc__r1
    stz.b tcc__r1h             ; previous table end, initially the directory
    lda.b tcc__r0
    sec
    sbc.b tcc__r1
    lda.b tcc__r0h
    sbc #0
    HOST_NOT_BELOW
    lda.l olHostDirectory+12
    cmp #256
    HOST_EQUAL
    lda.l olHostDirectory+14
    HOST_EQUAL
    ldx #0
Host_BindNext:
    ; Ordered, even, non-overlapping ranges wholly inside one CPU bank.
    ; Build-time validation additionally checks table shapes vs counts.
    lda.l olHostDirectory+50,x
    HOST_EQUAL                 ; reserved
    lda #0
    cpx #12
    bne +
    inc a                      ; SECTOR alone has the writable flag
+   cmp.l olHostDirectory+48,x
    HOST_EQUAL
    lda.l olHostDirectory+40,x
    bit #1
    HOST_EQUAL
    sec
    sbc.b tcc__r1
    lda.l olHostDirectory+42,x
    sbc.b tcc__r1h
    HOST_NOT_BELOW
    lda.l olHostDirectory+42,x
    cmp #$20
    bcc +
    jmp Host_Invalid           ; start must be an addressable ROM byte
+   lda.l olHostDirectory+46,x
    beq Host_ShortTable
    cmp #1
    HOST_EQUAL
    lda.l olHostDirectory+44,x
    HOST_EQUAL                 ; full bank is exactly $00010000 bytes
    lda.l olHostDirectory+40,x
    HOST_EQUAL                 ; and must start on a bank boundary
Host_ShortTable:
    lda.l olHostDirectory+40,x
    clc
    adc.l olHostDirectory+44,x
    sta.b tcc__r2
    bcc +
    HOST_EQUAL                 ; a bank crossing can only land exactly at 0
+   lda.l olHostDirectory+42,x
    adc.l olHostDirectory+46,x
    sta.b tcc__r2h
    lda.b tcc__r0
    sec
    sbc.b tcc__r2
    lda.b tcc__r0h
    sbc.b tcc__r2h
    HOST_NOT_BELOW
    lda.b tcc__r2
    sta.b tcc__r1
    lda.b tcc__r2h
    sta.b tcc__r1h
    lda.l Host_PointerSlots,x
    tay
    lda.l olHostDirectory+40,x
    sta.w $0000,y
    lda.l olHostDirectory+42,x
    clc
    adc.l olLevel               ; the level's host bank
    sta.w $0002,y
    txa
    clc
    adc #12
    tax
    cpx #18*12
    beq +
    jmp Host_BindNext
+   lda.b tcc__r1
    cmp.b tcc__r0
    HOST_EQUAL
    lda.b tcc__r1h
    cmp.b tcc__r0h
    HOST_EQUAL
    ; A table never crosses a source bank. DMA's count 0 is the entire
    ; 65536-byte bank, not an empty table. An empty SECTOR skips DMA.
    lda.l olHostDirectory+56
    ora.l olHostDirectory+58
    beq Host_SectorsReady
    lda.l olHostDirectory+56
    tax
    stx.w DAS0L
    sep #$20
    stz.w WMADDL
    stz.w WMADDM
    lda #1
    sta.w WMADDH
    stz.w DMAP0
    lda #$80
    sta.w BBAD0
    ldx.b olSectorData
    stx.w A1T0L
    lda.b olSectorData+2
    sta.w A1B0
    lda #1
    sta.w MDMAEN
    rep #$20
Host_SectorsReady:
    stz.b olSectorData
    lda #$007F
    sta.b olSectorData+2
    lda #1
    sta.b olHostTablesReady
Host_Invalid:
    plb
    rtl

; Matching descriptor stride allows one X to address both tables. These
; are addresses, not values: replacing the directory rebinds all tables.
Host_PointerSlots:
    .DW olRoomData,0,0,0,0,0, olSectorData,0,0,0,0,0
    .DW olLinkData,0,0,0,0,0, olModelData,0,0,0,0,0
    .DW olAnimBoxData,0,0,0,0,0, olAnimSpeedData,0,0,0,0,0
    .DW olAnimData,0,0,0,0,0, olTransData,0,0,0,0,0
    .DW olCmdData,0,0,0,0,0, olTriggerData,0,0,0,0,0
    .DW olItemData,0,0,0,0,0, olBoxData,0,0,0,0,0
    .DW olOverlapData,0,0,0,0,0, olZoneData,0,0,0,0,0
    .DW olStaticData,0,0,0,0,0, olStaticFirstData,0,0,0,0,0
    .DW olAnimAimData,0,0,0,0,0, olNavMapData,0,0,0,0,0
.ENDS
.BASE $00
.ENDIF

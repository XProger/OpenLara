; OpenLara - SNES / Super FX target
; 65816 host side.
;
; The GSU renders the scene; the 65816 runs gameplay, uploads the frame buffer
; into VRAM and reads the pad. Their shared state is published between jobs.
;
; On GSU-2, while the GSU owns the ROM bus the 65816 cannot fetch from game
; pak ROM at all -- it reads dummy data and gets dummy interrupt vectors
; pointing at WRAM $0100/$0104/$0108/$010C (Book II 2-5). The host's runtime
; code therefore lives in WRAM on that target.
;
; FX3 is the exception: concurrent ROM/RAM access, $7000 registers, no IRQ,
; and firmware C2P into bank $71. See the FX3 blocks and README: host level
; sectors alone use $7F, immutable tables and code use FastROM, completion is
; polled before the seven-field packed Mode 3 upload.

.INCLUDE "snes.inc"

.INCLUDE "hdr.asm"
.INCLUDE "texbank.inc"          ; the sound image's size, from tools/mkbanks.py
.IFDEF FX3
.INCLUDE "hostbank.inc"         ; the aux tables' banks (the font's palette)
.INCLUDE "levels.inc"           ; the level table's shape (tools/mkbanks.py)
; A restart's request for the next boot, in the battery RAM (it survives the
; reset): "OLBR", the level, the kind of boot (save.c, the level's end). The
; save is below it, $70:1000-$30FF.
.DEFINE BOOT_REQUEST        $703100
.DEFINE BOOT_START          0       ; save.h's kinds of boot: a level from its start,
.DEFINE BOOT_LOAD           1       ; and the save over it,
.DEFINE BOOT_TITLE          2       ; or the title page, over the first level
.ENDIF

; ---------------------------------------------------------------------------
; 816-tcc pseudo registers.
;
; The compiler works through a bank of direct-page scratch words rather than
; the 65816's single accumulator, so they have to exist at fixed direct-page
; addresses. Placed above this port's own variables, ending at $26 on GSU-2
; and $27 on FX3.
; ---------------------------------------------------------------------------

.RAMSECTION ".tccregs" BANK 0 SLOT 1 ORGA $30 FORCE PRIORITY 1000
tcc__registers  dsb 0
tcc__r0         dsb 2
tcc__r0h        dsb 2
tcc__r1         dsb 2
tcc__r1h        dsb 2
tcc__r2         dsb 2
tcc__r2h        dsb 2
tcc__r3         dsb 2
tcc__r3h        dsb 2
tcc__r4         dsb 2
tcc__r4h        dsb 2
tcc__r5         dsb 2
tcc__r5h        dsb 2
tcc__r9         dsb 2
tcc__r9h        dsb 2
tcc__r10        dsb 2
tcc__r10h       dsb 2
tcc__f2         dsb 2
tcc__f2h        dsb 2
tcc__f3         dsb 2
tcc__f3h        dsb 2
.ENDS

; ---------------------------------------------------------------------------
; Base sections for the APPENDTO groups 816-tcc emits for globals.
; ---------------------------------------------------------------------------

.RAMSECTION "globram.data" BANK $7E SLOT 2
    olRestartArm    db          ; dead, and Select released since
    olIrqUpload     db          ; FX3: IrqFx3 is uploading; the NMI keeps off
    olTickKeys      dsw 8       ; FX3: each queued tick's keys (TickKeysPush)
    olTickHead      dw          ;      the next to run's slot, * 2
    olTickTail      dw          ;      the next to queue's
.IFDEF FX3_STAGE
    olFrameLoadHold dw          ; 1..127: queued work blocks staging; >=128: burst
    olFrameStaged   db          ; immutable source mode of the queued upload
.IFNDEF C_MOTION
    olFrameEntry    dw          ; prepared GSU entry, starts after upload
.ENDIF
.ENDIF
.ENDS

.SECTION "glob.data" BANK 3 SLOT 4
.ENDS

.IFDEF FX3_STAGE
; Only used between STOP and QueueUpload. Once queued, owned by the upload
; until dmaBusy clears. Mutable level sectors must fit below $7F:8000;
; tools/budget.py validates the two allocations together.
.RAMSECTION "fx3_frame_stage" BANK $7F SLOT 3 ORGA $8000 FORCE
olFrameStage dsb (FRAME_COLS * FX3_COL_BYTES)
.ENDS
.ENDIF


; ---------------------------------------------------------------------------
; Direct page variables ($00-$FF, WRAM)
; ---------------------------------------------------------------------------

.INCLUDE "src/host_dp.inc"

; ---------------------------------------------------------------------------
; Bank 0: boot code, cartridge header, vectors
; ---------------------------------------------------------------------------

.BANK 0
.ORG 0
.SECTION "boot" FORCE

Reset:
    sei
    clc
    xce                         ; native mode
    rep #$38                    ; 16-bit A/X/Y, decimal off
    lda #$0000
    tcd                         ; direct page = $0000
    SETA8
    lda #$00
    pha
    plb                         ; data bank = $00

    ; Zero both WRAM banks. Inline, and before the stack pointer is set up:
    ; the stack lives in WRAM, so a JSR here would have its own return address
    ; erased by its own DMA. That bug boots to a BRK loop at $0100.
    stz WMADDL
    stz WMADDM
    stz WMADDH
    lda #$08                    ; A->B, fixed A-bus address, one register
    sta DMAP0
    lda #$80                    ; WMDATA
    sta BBAD0
    SETXY16
    ldx #ZeroByte
    stx A1T0L
    lda #:ZeroByte
    sta A1B0
    ldx #$0000                  ; 0 means 65536 bytes
    stx DAS0L
    lda #$01
    sta MDMAEN                  ; bank $7E
    ldx #$0000
    stx DAS0L
    lda #$01
    sta MDMAEN                  ; bank $7F

    ldx #WRAM_STACK
    txs

    jsr InitHardware
    jsr ClearVRAM
.IFDEF FX3
    jsr SelectLevel             ; before anything reads the level's data
.ENDIF
    jsr CopyHostCode
    jsr CopyGameCode
    jsr UploadSound
    jsr InitGame
    jsr UploadPalette
    jsr UploadTilemap
.IFDEF MODE7
    jsr InitLetterbox
.ENDIF
    ; Take both cartridge buses explicitly. The shade-map seed and InitGSU's
    ; GS_LEVEL_READY clear both write game pak RAM before StartGSU makes the
    ; first SCMR write. The manual gives SCMR a reset default of 00H (Book II
    ; 2-3: RON = RAN = 0, buses to the CPU), so this states the assumption
    ; rather than repairs it -- a soft reset with the GSU mid-frame is the
    ; case worth five bytes.
    SETA8
    lda #(SCMR_HT_160 | SCMR_MD_16)     ; RON = 0, RAN = 0
    sta GSU_SCMR
.IFDEF SHADE_MAP
    jsr InitShadeMaps
.ENDIF
.IFDEF FX3
.IF OL_TITLE_SIZE > 0
    jsr CopyTitle               ; the buses are the CPU's, the GSU not started
.ENDIF
.ENDIF
    jsr InitPPU
.IFNDEF MODE7
    jsr InitHud
.ENDIF
    ; The pad state, before anything can read it. WRAM comes up undefined and
    ; there is no crt0 here to clear it, so an uninitialised accumulator is a
    ; button held down at power-on -- which read as the game firing two shots
    ; before the player touched anything.
    SETA16
    stz keys
    stz keysPrev
    stz keysAccum
    stz tickCounter
    stz tickFieldAcc
    SETA8

    jsr InitGSU

    ; From here on the GSU owns the ROM bus, so never come back. Same reason
    ; as the vector pads below: stay in bank $00 and jump into the mirror.
    jmp.w HostMain

; ---------------------------------------------------------------------------
; FX3: which of the cartridge's levels this boot plays, and where its data
; is. A restart may ask for one in the battery RAM, which the reset leaves
; alone (BOOT_REQUEST: save.c's Current Position, the level's end, the
; title); a power-on finds none, or one already spent, and opens the title
; page (upstream's startLevel(LVL_TR1_TITLE)) over the first level -- or,
; on a cartridge without a title screen (no TITLEH.PCX at the pack), plays
; it. The level's entry of LevelTable (tools/mkbanks.py) becomes olLevel
; before anything reads the level: the host tables' bind, the uploads, the
; GSU's entry. DB is 0 and olLevel is past the low-RAM mirror: long stores.
; ---------------------------------------------------------------------------

.IFDEF FX3
SelectLevel:
    SETAXY16
    lda #$0000
    sta.l olLevelIndex
.IF OL_TITLE_SIZE > 0
    lda #BOOT_TITLE
.ENDIF
    sta.l olBootKind
    lda.l BOOT_REQUEST
    cmp #$4C4F                  ; "OL"
    bne @chosen
    lda.l BOOT_REQUEST+2
    cmp #$5242                  ; "BR"
    bne @chosen
    lda #$0000
    sta.l BOOT_REQUEST          ; spent, whatever it asked
    lda.l BOOT_REQUEST+4
    cmp #LEVEL_COUNT
    bcs @chosen
    sta.l olLevelIndex
    lda.l BOOT_REQUEST+6
    cmp #BOOT_TITLE
    bne +
.IF OL_TITLE_SIZE > 0
    lda #$0000                  ; the title's colours are the first level's
    sta.l olLevelIndex
    lda #BOOT_TITLE
.ELSE
    lda #BOOT_START
.ENDIF
+   sta.l olBootKind
@chosen:
    lda #LEVEL_COUNT
    sta.l olLevelCount
    ; Every level's host bank and LevelID, for the passport and a save.
    ldx #$0000
    txy
-   lda.l LevelTable,x
    phx
    tyx
    sta.l olLevelHostBanks,x
    plx
    lda.l LevelTable+6,x
    phx
    tyx
    sta.l olLevelIds,x
    plx
    txa
    clc
    adc #LEVEL_ENTRY
    tax
    iny
    iny
    cpy #(LEVEL_COUNT * 2)
    bne -
.IF LEVEL_ENTRY != 64
    .FAIL "SelectLevel multiplies the level index by 64"
.ENDIF
    lda.l olLevelIndex          ; * LEVEL_ENTRY, which is 64
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    ldy #$0000
-   lda.l LevelTable,x
    phx
    tyx
    sta.l olLevel,x
    plx
    inx
    inx
    iny
    iny
    cpy #LEVEL_ENTRY
    bne -
    SETA8
    rts
.ENDIF

; ---------------------------------------------------------------------------

InitHardware:
    SETA8
    SETXY16
    lda #$00
    sta NMITIMEN
    lda #$FF
    sta WRIO
    lda #$00
    ldx #$0002
-   sta $4200,x                 ; $4202-$420D
    inx
    cpx #$000E
    bne -

.IFDEF FX3
    lda #$01
    sta MEMSEL                  ; FastROM code in banks $84/$85
.ENDIF
    lda #$8F
    sta INIDISP                 ; forced blank, full brightness

    ldx #$0000
-   lda.w PPUInitTable,x
    cmp #$FF
    beq +
    tay
    inx
    lda.w PPUInitTable,x
    inx
    sta $2100,y
    bra -
+   rts

; register index, value; $FF terminates
PPUInitTable:
    .DB $01, $00        ; OBJSEL
    .DB $05, $00        ; BGMODE
    .DB $06, $00        ; MOSAIC
    .DB $07, $00        ; BG1SC
    .DB $08, $00        ; BG2SC
    .DB $09, $00        ; BG3SC
    .DB $0A, $00        ; BG4SC
    .DB $0B, $00        ; BG12NBA
    .DB $0C, $00        ; BG34NBA
    .DB $23, $00        ; W12SEL
    .DB $24, $00        ; W34SEL
    .DB $25, $00        ; WOBJSEL
    .DB $2C, $00        ; TM
    .DB $2D, $00        ; TS
    .DB $2E, $00        ; TMW
    .DB $2F, $00        ; TSW
.IFDEF FX3
    .DB $30, $00        ; CGWSEL: colour math always, the fixed colour
    .DB $31, $81        ; CGADSUB: subtract it from BG1 -- the fade (NMI)
.ELSE
    .DB $30, $30        ; CGWSEL
    .DB $31, $00        ; CGADSUB
.ENDIF
    .DB $32, $E0        ; COLDATA: fixed colour black
    .DB $33, $00        ; SETINI
    .DB $FF

ZeroByte:
    .DB $00

; ---------------------------------------------------------------------------
; Zero all of VRAM. This also leaves the blank character used by the bottom
; rows of the tile map at zero in both frame buffer bases.
; ---------------------------------------------------------------------------

ClearVRAM:
    SETA8
    SETXY16
    lda #$80
    sta VMAIN
    ldx #$0000
    stx VMADDL
    lda #$09                    ; A->B, fixed, two registers (L,H)
    sta DMAP0
    lda #$18                    ; VMDATAL
    sta BBAD0
    ldx #ZeroWord
    stx A1T0L
    lda #:ZeroWord
    sta A1B0
    ldx #$0000                  ; 65536 bytes = all 32 K words
    stx DAS0L
    lda #$01
    sta MDMAEN
    rts

ZeroWord:
    .DW $0000

; ---------------------------------------------------------------------------
; Copy the resident host code from ROM bank 1 into WRAM $7E:0100.
; ---------------------------------------------------------------------------

CopyHostCode:
    SETA8
    SETXY16
    lda #<WRAM_VECTORS
    sta WMADDL
    lda #>WRAM_VECTORS
    sta WMADDM
    stz WMADDH                  ; bank $7E
    lda #$00                    ; A->B, increment, one register
    sta DMAP0
    lda #$80                    ; WMDATA
    sta BBAD0
    ldx #$8000                  ; ROM bank 1 sits at $01:8000
    stx A1T0L
    lda #$01
    sta A1B0
    ldx #(HostCodeEnd - HostCodeStart)
    stx DAS0L
    lda #$01
    sta MDMAEN
    rts

; ---------------------------------------------------------------------------
; GSU-2: copy the compiled C image into WRAM. FX3: bind CPU ROM tables.
;
; 816-tcc would happily leave its code in ROM, which is fine on an ordinary
; SNES and FX3, but not GSU-2: its CPU cannot fetch ROM while the GSU holds
; the bus. On GSU-2 tools/tccfix.py retargets every emitted section into a
; nominated ROM bank with a .BASE that makes its labels read as $7F:xxxx.
;
; It takes two banks and two DMAs, because WRAM bank $7F is 64 KB and a LoROM
; bank is 32. The WRAM port auto-increments across both, so the second DMA
; would continue where the first left off -- it is pointed at $7F:8000
; explicitly anyway, because a copy that depends on where the last one
; happened to stop is a copy nobody can move.
; ---------------------------------------------------------------------------

; 816-opt shortens the compiler's long addressing to plain absolute, which is
; only correct under PVSnesLib's convention that the data bank register points
; at the bank holding .bss. Their crt0 does `pea $7e7e / plb / plb`; there is
; no crt0 here, so every entry into C does it instead.
.MACRO ENTER_C
    phb
    pea $7E7E
    plb
    plb
.ENDM

.MACRO LEAVE_C
    plb
.ENDM

InitGame:
    SETAXY16
    ENTER_C
    jsl gameInit
    LEAVE_C
    SETA8
    rts

CopyGameCode:
.IFDEF FX3
    SETAXY16
    jsl olHostBind
    SETA8
.ELSE
    SETA8
    SETXY16
    stz WMADDL
    stz WMADDM
    lda #$01
    sta WMADDH                  ; bank $7F
    lda #$00                    ; A->B, increment, one register
    sta DMAP0
    lda #$80                    ; WMDATA
    sta BBAD0
    ldx #$8000                  ; ROM bank 4 sits at $04:8000
    stx A1T0L
    lda #$04
    sta A1B0
    ldx #$8000                  ; the whole bank
    stx DAS0L
    lda #$01
    sta MDMAEN

    stz WMADDL                  ; and the upper half, from ROM bank 5
    lda #$80
    sta WMADDM
    lda #$01
    sta WMADDH
    ldx #$8000
    stx A1T0L
    lda #$05
    sta A1B0
    ldx #$8000
    stx DAS0L
    lda #$01
    sta MDMAEN

    ; And the third image: the upper half of bank $7E, from ROM bank $3F --
    ; the top of the cartridge, where neither the level nor the texture pages
    ; can grow into it. Its .bss keeps to $7E:2000-$57FF; hdr.asm's SLOT 2 is
    ; what enforces that.
    stz WMADDL
    lda #$80
    sta WMADDM
    stz WMADDH                  ; bank $7E
    ldx #$8000
    stx A1T0L
    lda #$3F
    sta A1B0
    ldx #$8000
    stx DAS0L
    lda #$01
    sta MDMAEN

    ; The fourth: 10 KB over the .bss, $7E:5800-$7FFF, from ROM bank $3E.
    ; (6 KB from $6800 until 2026-09-04; the nav flood's second queue went
    ; and the .bss cap came down a page.)
    ldx #$5800
    stx WMADDL                  ; L and M
    stz WMADDH
    ldx #$8000
    stx A1T0L
    lda #$3E
    sta A1B0
    ldx #$2800
    stx DAS0L
    lda #$01
    sta MDMAEN

    ; The fifth: $1600 bytes under the stack, $7E:0600-$1BFF, from bank $3D.
    ; Resident host assembly must end before $0600 (checked after linking).
    ; stack starts at $1FFF and was measured 421 bytes deep over a long scene;
    ; WRAM_STACK_FLOOR is where this image ends.
    ldx #$0600
    stx WMADDL
    stz WMADDH
    ldx #$8000
    stx A1T0L
    lda #$3D
    sta A1B0
    ldx #$1600
    stx DAS0L
    lda #$01
    sta MDMAEN
.ENDIF                         ; FX3 executes all five images from FastROM
    rts

; ---------------------------------------------------------------------------
; CG-RAM. Eight background palettes of sixteen colours: the same sixteen, each
; row darker than the last. That is where the shade term lives on this target,
; because the frame buffer is four bits deep and all four are texel.
;
; BG palettes 0-7 are entries $00-$7F, which is the whole of what a 4bpp BG can
; select; anything a HUD wants has to come out of the sprite half of CG-RAM.
; ---------------------------------------------------------------------------

UploadPalette:
    SETA8
    SETXY16
    stz CGADD
.IFDEF FX3
    ; The level's own (olLevel, SelectLevel): CPU-only ROM, any bank.
    SETA16
    lda.l olLevel+OLD_PALETTE
    sta.b tcc__r0
    lda.l olLevel+OLD_PALETTE+2
    sta.b tcc__r0h
    SETA8
    ldy #$0000
-   lda [tcc__r0],y
    sta CGDATA
    iny
    cpy #PALETTE_BYTES
    bne -
.ELSE
    ldx #$0000
-   lda.w PaletteData,x
    sta CGDATA
    inx
    cpx #PALETTE_BYTES
    bne -
.ENDIF
.IFDEF PIXEL_FRAME
    ; CG-RAM 128-255: black. The letterbox sprites draw in colour 1 of
    ; sprite palette 0, entry 129, and a Mode 7 pixel's high nibble could
    ; select any of these rows. Opaque texture faces also write the source-
    ; zero marker $80 (OR shade) unchanged in 8bpp; these entries keep it black.
    ldx #$0000
-   stz CGDATA
    inx
    cpx #$0100
    bne -
.ENDIF
.IFDEF FX3
.IF OL_TITLE_SIZE > 0
    ; A title boot's own 64 colours (packer writeTitle) at CG-RAM 192-255,
    ; which nothing else uses: the picture's, beside the level's.
    SETA16
    lda.l olBootKind
    cmp #BOOT_TITLE
    SETA8
    bne +
    lda #$C0
    sta CGADD
    ldx #$0000
-   lda.l OL_TITLE_ADDR,x
    sta CGDATA
    inx
    cpx #128
    bne -
+
.ENDIF
.ENDIF
    rts

.IFDEF FX3
.IF OL_TITLE_SIZE > 0
; A title boot's picture (packer writeTitle) into BG_BUF, where the ring's
; background is plotted from (mesh.gsu IR_Grey); ItemsExchange holds
; BG_STATE at 1 for the boot, so BgCapture never takes a game frame over
; it. Before InitGSU, with both buses the CPU's: one MVN, 31,104 bytes,
; about four fields of the boot.
CopyTitle:
    SETAXY16
    lda.l olBootKind
    cmp #BOOT_TITLE
    bne +
    lda #(216 * 144 - 1)
    ldx #(OL_TITLE_OFFSET + 128)
    ldy #(BG_BUF_LONG & $FFFF)
    phb
    mvn OL_TITLE_BANK, $70
    plb
+   SETA8
    rts
.ENDIF
.ENDIF

.IFNDEF FX3
PaletteData:
    .INCBIN "PALETTE.BIN"
.ENDIF

; ---------------------------------------------------------------------------
; The HUD: the bar palettes into sprite palettes 0 and 1 (CG-RAM 128-159),
; the 23 bar characters to VRAM_HUD, every sprite parked below the screen,
; and sprites on the main screen. src/game.c builds the 26 sprites of the two
; bars and the NMI copies them to OAM. Not in the Mode 7 build, whose sprite
; characters are the letterbox's.
; ---------------------------------------------------------------------------

.IFNDEF MODE7
InitHud:
    SETA8
    SETXY16
.IFDEF FX3
    ; The level's HUD.BIN (olLevel, SelectLevel), through [tcc__r0].
    SETA16
    lda.l olLevel+OLD_HUD
    sta.b tcc__r0
    lda.l olLevel+OLD_HUD+2
    sta.b tcc__r0h
    SETA8
.ENDIF
    lda #$80
    sta CGADD
    ldx #$0000
.IFDEF FX3
-   txy
    lda [tcc__r0],y
.ELSE
-   lda.l HudData,x
.ENDIF
    sta CGDATA
    inx
    cpx #64
    bne -

    lda #$80
    sta VMAIN                   ; +1 word after the $2119 write
    ldx #VRAM_HUD
    stx VMADDL
.IFDEF FX3
    ldy #64                     ; the characters follow the palettes
-   lda [tcc__r0],y
    sta VMDATAL
    iny
    lda [tcc__r0],y
    sta VMDATAH
    iny
    cpy #(64 + 23 * 32)
    bne -
.ELSE
    ldx #64                     ; the characters follow the palettes
-   lda.l HudData,x
    sta VMDATAL
    lda.l HudData+1,x
    sta VMDATAH
    inx
    inx
    cpx #(64 + 23 * 32)
    bne -
.ENDIF

    stz OAMADDL
    stz OAMADDH
    ldx #128
-   stz OAMDATA                 ; X 0
    lda #$F0
    sta OAMDATA                 ; Y 240: below the 224 lines
    stz OAMDATA
    stz OAMDATA
    dex
    bne -
    ldx #32
-   stz OAMDATA                 ; high table: X bit 8 clear, small size
    dex
    bne -

.IFDEF FX3
    ; The font's colours (packer FONT.BIN) as sprite palette 2, CG-RAM
    ; 160-175, the inventory background's eight greys after them, 176-183,
    ; and the options' three border colours, 184-186: the 8bpp frame
    ; reaches them all. The GSU's text pixels are $A1-$AF (mesh.gsu
    ; TextRender), its grey background $B0-$B7, the borders $B8-$BA.
    SETA16
    lda.l olLevel+OLD_FONT_SIZE
    beq @noFont
    lda.l olLevel+OLD_FONT
    sta.b tcc__r0
    lda.l olLevel+OLD_FONT+2
    sta.b tcc__r0h
    SETA8
    lda #$A0
    sta CGADD
    ldy #$0000
-   lda [tcc__r0],y
    sta CGDATA
    iny
    cpy #56
    bne -
    ; and the options' bar (TR 41-44, after the glyph table: 64 + 6 x the
    ; count at byte 57) at 188-191, beside the borders: settings.c's pages
    SETA16
    ldy #57
    lda [tcc__r0],y
    and #$00FF
    sta.b tcc__r1
    asl a
    clc
    adc.b tcc__r1
    asl a
    clc
    adc #64
    tay
    SETA8
    ldx #8
-   lda [tcc__r0],y
    sta CGDATA
    iny
    dex
    bne -
@noFont:
    SETA8
.ENDIF

    lda #OBJSEL_HUD
    sta OBJSEL
    rts

.IFNDEF FX3
HudData:
    .INCBIN "HUD.BIN"
.ENDIF
.ENDIF

; ---------------------------------------------------------------------------
; The sound CPU. src/spc.asm's driver and the packer's SOUND.BIN go up
; through the IPL ROM's own protocol (Book I 3-3), at boot, while this CPU
; still reads the cartridge: three blocks, the driver at $0200 and the image
; in one or two halves from $0400, then the start address. The host's own
; tables (SNDTAB.BIN) are copied into WRAM first, the way the C image is.
;
; An emulator without a sound CPU never answers $BBAA; that is a bounded
; wait, and ol_sndReady stays clear so the game plays silently rather than
; hanging. Everything after the first handshake trusts the IPL to echo, as
; the manual's own loader does. The counter the IPL wants at each block
; header is the last index sent plus two, never zero (3-3-2).
;
; tcc__r0/r0h is the far source pointer, tcc__r1 the counter: C has not run
; yet and the registers are free. NMI is still off, so nothing can delay the
; final acknowledgement past the window Caution 9 warns about.
; ---------------------------------------------------------------------------

UploadSound:
    SETA8
    SETXY16
    ; SNDTAB.BIN -> olSndTable
    ldx #olSndTable
    stx WMADDL
    stz WMADDH                  ; bank $7E
    lda #$00                    ; A->B, increment, one register
    sta DMAP0
    lda #$80                    ; WMDATA
    sta BBAD0
.IFDEF FX3
    SETA16                      ; the level's (olLevel, SelectLevel); DB is 0,
    lda.l olLevel+OLD_SNDTAB    ; and olLevel is past the low-RAM mirror
    sta A1T0L
    SETA8
    lda.l olLevel+OLD_SNDTAB+2
    sta A1B0
    ldx #SNDTAB_SIZE
.ELSE
    ldx #SndTable
    stx A1T0L
    lda #:SndTable
    sta A1B0
    ldx #(SndTableEnd - SndTable)
.ENDIF
    stx DAS0L
    lda #$01
    sta MDMAEN

    SETA16
    lda #$0000
    sta.l ol_sndReady
    ldx #$0000
-   lda $2140                   ; $2140 and $2141: the IPL says $BBAA
    cmp #$BBAA
    beq +
    dex
    bne -
    SETA8
    rts                         ; no sound CPU answered
+   SETA8
    lda #$CC
    sta.b tcc__r1

    ldx #SpcDriver
    stx.b tcc__r0
    lda #:SpcDriver
    sta.b tcc__r0h
    ldx #(SpcDriverEnd - SpcDriver)
    ldy #$0200
    jsr ApuBlock

.IFDEF FX3
    ; The level's image, whole: bank-aligned in CPU-only ROM, so one
    ; block of up to $FB00 bytes (olLevel, SelectLevel).
    SETA16
    lda.l olLevel+OLD_SOUND
    sta.b tcc__r0
    lda.l olLevel+OLD_SOUND+2
    sta.b tcc__r0h
    lda.l olLevel+OLD_SOUND_SIZE
    tax
    SETA8
    ldy #$0400
    jsr ApuBlock
.ELSE
    ldx #SoundImage0
    stx.b tcc__r0
    lda #:SoundImage0
    sta.b tcc__r0h
    ldx #SOUND_IMAGE_SIZE_0
    ldy #$0400
    jsr ApuBlock
.ENDIF
.IFNDEF FX3
.IF SOUND_IMAGE_SIZE_1 > 0
    ldx #SoundImage1
    stx.b tcc__r0
    lda #:SoundImage1
    sta.b tcc__r0h
    ldx #SOUND_IMAGE_SIZE_1
    ldy #($0400 + SOUND_IMAGE_SIZE_0)
    jsr ApuBlock
.ENDIF
.ENDIF

    ; The end code: no more data, start the driver at $0200.
    ldy #$0200
    sty $2142
    stz $2141
    lda.b tcc__r1
    sta $2140
-   cmp $2140
    bne -
    ; Now the driver is running its own loop: clear the port, and wait for
    ; it to answer zero -- from then on zero means idle on both sides.
    stz $2140
    ldx #$0000
-   lda $2140
    beq +
    dex
    bne -
    rts                         ; uploaded, but the driver never spoke
+   SETA16
    lda #$0001
    sta.l ol_sndReady
    SETA8
    rts

; One block: X bytes from [tcc__r0] to sound CPU address Y; tcc__r1 is the
; counter to announce it with, and leaves holding the next one.
ApuBlock:
    sty $2142                   ; $2142 and $2143: the destination
    lda #$01
    sta $2141                   ; non-zero: a data block follows
    lda.b tcc__r1
    sta $2140
-   cmp $2140
    bne -
    ldy #$0000
@byte:
    lda [tcc__r0],y
    sta $2141
    tya
    sta $2140
-   cmp $2140
    bne -
    iny
    dex
    bne @byte
    clc
    adc #$02
    bne +
    inc a
+   sta.b tcc__r1
    rts

.IFDEF FX3
; void olSndGun(unsigned int gun) -- C, gun at 4,s: the gun's sounds into
; the region the level's APU image keeps for whichever gun she holds
; (packer sound_SNES.h, GUNSND.BIN). The driver goes back to the IPL on a
; STOP for source $FF, as RestartLevel sends it; three blocks follow -- the
; region's two directory entries, their pitch words, the samples -- and the
; driver starts again at $0200, idle, as the boot leaves it. About 90 ms at
; the IPL's pace for the shotgun's 3 KB: a change of gun, once.
.ACCU 16
.INDEX 16
olSndGun:
    php
    phb
    phk
    plb                         ; I/O as the boot's upload addresses it
    SETAXY16
    lda.l ol_sndReady
    beq +
    lda.l olLevel+OLD_GUN_SIZE
    bne ++
+   jmp @out
++  lda 6,s                     ; the gun, past P and B
    asl a
    asl a
    asl a
    asl a
    clc
    adc #8
    clc
    adc.l olLevel+OLD_GUN
    sta.b tcc__r2               ; its record: directory, pitches, size, offset
    lda.l olLevel+OLD_GUN+2
    sta.b tcc__r2h
    SETA8
    ldx #$0000
-   lda $2140                   ; the last command acknowledged
    cmp.l olSndSeq
    beq +
    dex
    bne -
+   lda #$FF
    sta $2141
    lda.l olSndSeq
    clc
    adc #$04
    and #$FC
    bne +
    lda #$04
+   ora #$03                    ; STOP
    sta $2140
    ldx #$0000
-   SETA16
    lda $2140
    cmp #$BBAA
    SETA8
    beq +
    dex
    bne -
    jmp @lost                   ; the IPL never answered
+   lda #$CC
    sta.b tcc__r1
    SETA16
    lda.l olLevel+OLD_GUN       ; the header: shot source, region address
    sta.b tcc__r0
    lda.l olLevel+OLD_GUN+2
    sta.b tcc__r0h
    lda [tcc__r0]
    asl a
    asl a
    clc
    adc #$0400
    pha                         ; the directory entries' address
    lda [tcc__r0]
    asl a
    clc
    adc #$0600
    pha                         ; the pitch words'
    ldy #4
    lda [tcc__r0],y
    pha                         ; the region's
    lda.b tcc__r2               ; block 1: 8 bytes of directory
    sta.b tcc__r0
    lda.b tcc__r2h
    sta.b tcc__r0h
    SETA8
    ldx #8
    SETA16
    lda 5,s
    tay
    SETA8
    jsr ApuBlock
    SETA16                      ; block 2: the two pitch words
    lda.b tcc__r2
    clc
    adc #8
    sta.b tcc__r0
    lda 3,s
    tay
    SETA8
    ldx #4
    jsr ApuBlock
    SETA16                      ; block 3: the samples, from the file's offset
    ldy #14
    lda [tcc__r2],y
    clc
    adc.l olLevel+OLD_GUN
    sta.b tcc__r0
    ldy #12
    lda [tcc__r2],y
    tax
    pla
    tay
    pla
    pla
    SETA8
    cpx #0
    beq +
    jsr ApuBlock
+   ldy #$0200                  ; the end code: start the driver
    sty $2142
    stz $2141
    lda.b tcc__r1
    sta $2140
-   cmp $2140
    bne -
    stz $2140
    ldx #$0000
-   lda $2140
    beq +
    dex
    bne -
@lost:
    SETA16
    lda #$0000
    sta.l ol_sndReady           ; no driver: no sound from here
    bra @out
+   SETA16
    lda #$0000
    sta.l olSndSeq              ; idle, as after the boot's upload
@out:
    plb
    plp
    rtl
.ENDIF

SpcDriver:
    .INCBIN "spc.bin"
SpcDriverEnd:
.IFNDEF FX3
SndTable:
    .INCBIN "SNDTAB.BIN"
SndTableEnd:
.ENDIF

; ---------------------------------------------------------------------------
; Seed both shade maps with the character number bits the GSU must not compute.
;
; A shade map entry is a tile map high byte. Bits 2-4 are the palette, which is
; the renderer's to write; bits 0-1 are character number bits 8-9, and with 640
; characters on screen they are not optional. Rather than have the GSU
; reconstruct CN = $14 * Xc + Yc per character -- in a loop that runs per span
; per scanline -- it read-modify-writes, and this puts the invariant half
; there once, before the GSU has ever run.
;
; Runs at boot while the CPU still owns the game pak RAM bus.
; ---------------------------------------------------------------------------

.IFDEF SHADE_MAP
InitShadeMaps:
    SETAXY16
    ldx #$0000
@entry:
    txa
    and #$001F                  ; Xc
    sta tmapTmp
    asl a
    asl a                       ; Xc * 4
    sta dmaTmp
    lda tmapTmp
    asl a
    asl a
    asl a
    asl a                       ; Xc * 16
    clc
    adc dmaTmp                  ; $14 * Xc
    sta dmaTmp
    txa
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a                       ; Yc
    clc
    adc dmaTmp                  ; CN
    xba
    and #$00FF                  ; CN >> 8
    sep #$20
    sta.l SHADE0_LONG,x
    sta.l SHADE1_LONG,x
    rep #$20
    inx
    cpx #SHADE_SIZE
    bne @entry
    SETA8
    rts
.ENDIF

; ---------------------------------------------------------------------------
; Tile map. The GSU numbers characters column major in 160-dot mode,
; CN = $14 * Xc + Yc, so the map is just that formula. Rows 20-31 are off the
; bitmap and get the blank character.
;
; The Mode 7 map is the low bytes of VRAM words 0-$3FFF, 128 entries to a
; row: rows 0-7 name buffer 0's 128 characters and rows 8-15 buffer 1's,
; numbered down the columns as the GSU writes them, character (r, c) =
; c * 8 + r. The rest of the map is zero from ClearVRAM and lies under the
; letterbox sprites. Written a byte at a time through $2118 with VMAIN
; incrementing on the low byte, so the character data in the high bytes is
; untouched.
; ---------------------------------------------------------------------------

.IFDEF MODE7
UploadTilemap:
    SETA8
    stz VMAIN                   ; +1 word after the $2118 write
    SETAXY16
    ldx #$0000                  ; the character
@t:
    ; its map entry: row (t & 7), plus 8 for buffer 1's half; column
    ; (t >> 3) & 15 -- at word row * 128 + column
    txa
    and #$0007
    xba
    lsr a                       ; (t & 7) << 7
    sta tmapTmp
    txa
    and #$0080
    asl a
    asl a
    asl a                       ; (t & $80) << 3: eight rows down for buffer 1
    clc
    adc tmapTmp
    sta tmapTmp
    txa
    lsr a
    lsr a
    lsr a
    and #$000F
    clc
    adc tmapTmp
    sta VMADDL
    SETA8
    txa
    sta VMDATAL
    SETA16
    inx
    cpx #$0100
    bne @t
    SETA8
    rts

; ---------------------------------------------------------------------------
; The letterbox: eight 64x64 sprites of one colour -- CG-RAM 129, black --
; over display lines 0-47 and 176-223, where the Mode 7 layer would show
; whatever the map wraps to there. Sprites lie over BG1 in Mode 7 at every
; priority. A 64x64 sprite is tiles n..n+7 of eight consecutive rows of
; sixteen, so all 128 tiles of the sprite character area at VRAM_OBJ are
; the same solid 4bpp pattern; the other 120 sprites are parked off the
; bottom of the screen.
; ---------------------------------------------------------------------------

InitLetterbox:
    SETA8
    lda #$80
    sta VMAIN                   ; +1 word after the $2119 write
    SETXY16
    ldx #VRAM_OBJ
    stx VMADDL
    ldx #128                    ; tiles
@tile:
    ldy #8
-   lda #$FF
    sta VMDATAL                 ; plane 0: every pixel colour 1
    stz VMDATAH                 ; plane 1
    dey
    bne -
    ldy #8
-   stz VMDATAL                 ; planes 2 and 3
    stz VMDATAH
    dey
    bne -
    dex
    bne @tile

    stz OAMADDL
    stz OAMADDH
    ldx #$0000
-   lda.w LetterboxOAM,x
    sta OAMDATA
    inx
    cpx #32
    bne -
    ldx #120
-   stz OAMDATA                 ; x
    lda #$E0
    sta OAMDATA                 ; y = 224: a small sprite there is off screen
    stz OAMDATA                 ; tile
    stz OAMDATA                 ; attributes
    dex
    bne -
    ; the high table, two bits a sprite: X bit 8 and the size; the first
    ; eight are the large size
    lda #$AA
    sta OAMDATA
    sta OAMDATA
    ldx #30
-   stz OAMDATA
    dex
    bne -
    rts

; x, y, tile, attributes (priority 3, palette 0)
LetterboxOAM:
    .DB $00, OBJ_LETTERBOX_Y0, $00, $30
    .DB $40, OBJ_LETTERBOX_Y0, $00, $30
    .DB $80, OBJ_LETTERBOX_Y0, $00, $30
    .DB $C0, OBJ_LETTERBOX_Y0, $00, $30
    .DB $00, OBJ_LETTERBOX_Y1, $00, $30
    .DB $40, OBJ_LETTERBOX_Y1, $00, $30
    .DB $80, OBJ_LETTERBOX_Y1, $00, $30
    .DB $C0, OBJ_LETTERBOX_Y1, $00, $30

.ELSE
.IFDEF FX3
UploadTilemap:
    SETA8
    stz dmaBuffer
    jmp.w UploadFX3Map
.ELSE
UploadTilemap:
    SETA8
    lda #$80
    sta VMAIN                   ; +1 word after the $2119 write
    SETXY16
    ldx #VRAM_TILEMAP
    stx VMADDL

    SETA16
    ldy #$0000                  ; Yc, screen row
@row:
    cpy #FRAME_ROWS
    bcs @blankRow

    ldx #$0000                  ; Xc, screen column
@col:
    txa
    asl a
    asl a                       ; Xc * 4
    sta tmapTmp
    txa
    asl a
    asl a
    asl a
    asl a                       ; Xc * 16
    clc
    adc tmapTmp                 ; Xc * 20 = $14 * Xc
    sta tmapTmp
    tya
    clc
    adc tmapTmp                 ; CN = $14 * Xc + Yc
    sta VMDATAL
    inx
    cpx #FRAME_COLS
    bne @col
    bra @next

@blankRow:
    ldx #$0000
-   lda #BLANK_CHAR
    sta VMDATAL
    inx
    cpx #FRAME_COLS
    bne -

@next:
    iny
    cpy #32
    bne @row
    SETA8
    rts
.ENDIF
.ENDIF
.ENDS

; ---------------------------------------------------------------------------
; Bank 0 continued: PPU mode and GSU bring-up
; ---------------------------------------------------------------------------

.BANK 0
.SECTION "ppuinit" FREE

InitPPU:
    SETA8
.IFDEF FX3
    lda #$03                    ; BG1: 8bpp, 8x8 tiles
    sta BGMODE
    lda #BG1SC_VALUE
    sta BG1SC
    stz BG12NBA
    lda #$EC                    ; -20: centre 216 pixels in 256
    sta BG1HOFS
    lda #$03
    sta BG1HOFS
    ; -41: centre the 144-line frame in 224. The PPU shows BG line
    ; VOFS + 1 on the first displayed line (TypeSNES models it since
    ; 2026-09-26), so -40 put the frame at lines 39-182.
    lda #$D7
    sta BG1VOFS
    lda #$03
    sta BG1VOFS
    lda #$11
    sta TM                      ; BG1 and the HUD's sprites
    stz TS
    rts
.ELSE
.IFDEF MODE7
    lda #$07
    sta BGMODE                  ; Mode 7
    stz M7SEL                   ; the map wraps; the sprites cover the wrap

    ; The matrix: A = D = 0.5 and no rotation, so the 128x64 frame fills
    ; 256x128 of the display; the centre at the origin.
    lda #$80
    sta M7A
    stz M7A
    stz M7B
    stz M7B
    stz M7C
    stz M7C
    lda #$80
    sta M7D
    stz M7D
    stz M7X
    stz M7X
    stz M7Y
    stz M7Y

    ; M7HOFS / M7VOFS are BG1's scroll registers: map row 0 on display
    ; line 48, which is buffer 0; the flip in UploadChunk scrolls to
    ; buffer 1's rows.
    stz BG1HOFS
    stz BG1HOFS
    lda #(M7VOFS_FB0 & $FF)
    sta BG1VOFS
    lda #(M7VOFS_FB0 >> 8)
    sta BG1VOFS

    lda #OBJSEL_VALUE
    sta OBJSEL                  ; 8x8 and 64x64 sprites, characters at VRAM_OBJ
    lda #$11
    sta TM                      ; BG1 and the sprites on the main screen
    stz TS
    rts
.ELSE
    lda #$01
    sta BGMODE                  ; mode 1, 8x8 characters
    lda #BG1SC_VALUE
    sta BG1SC                   ; tile map at word $6000, 32x32
    lda #BG12NBA_FB0
    sta BG12NBA                 ; BG1 characters at word $0000

    stz BG1HOFS                 ; scroll registers take two writes
    stz BG1HOFS
    lda #$FF                    ; -1: the PPU shows BG line VOFS + 1 on the
    sta BG1VOFS                 ; first displayed line; this puts the frame's
    lda #$03                    ; row 0 on it, and BG row n level with OAM Y n
    sta BG1VOFS

    lda #$11
    sta TM                      ; BG1 and the HUD's sprites on the main screen
    stz TS
    rts
.ENDIF
.ENDIF
.ENDS

.BANK 0
.SECTION "gsuinit" FREE

InitGSU:
    SETA8
    SETXY16

    ; Which die is under us. VCR is readable whether or not the GSU is
    ; running, and it is the only thing on the cartridge that answers the
    ; question: $01 is the original MARIO chip, $04 everything after it.
    ;
    ; This port cannot run on $01. It reads its program, its tables and its
    ; texture pages through the linear 64 KB-bank ROM image in banks $40-$5F,
    ; and MARIO does not decode that image at all. The value is kept for
    ; diagnostics rather than acted on -- there is nothing useful to do about
    ; it on a soldered board -- but `make check` reports it, so a wrong
    ; assumption shows up as a number instead of as a blank screen.
    SETA16
    lda GSU_VCR
    and #$00FF
    sta.l ol_gsuVersion         ; a C global, so the test can find it by name
    SETA8

    ; Force the GSU idle and reset its cache state before the first run.
    stz GSU_SFR
    stz GSU_SFRH

    lda #$01
    sta GSU_CLSR                ; 21.4 MHz
    ; CFGR: IRQ to the CPU enabled, standard multiplier.
    ;
    ; Bit 5 (MS0) is the high-speed multiplier, and the manual is explicit
    ; that it must be 0 at 21 MHz (superfx.md: "At 21 MHz (CLSR=1), MS0 must
    ; be 0"). GSU-2 silicon has been measured running it fast at both clocks,
    ; so it is tempting -- and it is not worth it: the frame issues 14,695
    ; FMULT/LMULT, and 7 cycles down to 3 saves 58,780 of roughly 19 million.
    ; Three tenths of one percent, against a documented hardware constraint.
    ; The frame is bound by instruction fetch, not by the multiplier.
.IFDEF FX3
    lda #$80                    ; real FX3 has no IRQ; mask it in emulators too
.ELSE
    lda #$00
.ENDIF
    sta GSU_CFGR
    stz GSU_BRAMR

.IFDEF FX3
    lda #(SCMR_RON | SCMR_RAN | SCMR_HT_160 | SCMR_MD_256)
.ELSE
    lda #(SCMR_RON | SCMR_RAN | SCMR_HT_160 | SCMR_MD_16)
.ENDIF
    sta scmrShadow

    stz fbDraw
    stz fbShow
    stz gsuDone
    stz dmaBusy
    stz cpuOwnsRam
    stz logicPhase
    stz logicPending
.IFDEF FX3
    stz renderHold
.ENDIF
.IFDEF FX3
    lda #$01                    ; CPU tables are bound; skip legacy GSU staging
.ELSE
    lda #$00                    ; the GSU publishes this only after the whole
.ENDIF
    sta.l GS_LEVEL_READY_LONG   ; host level blob has reached game pak RAM
    lda #$00                    ; the GSU builds its entity slot table once
    sta.l ENT_READY_LONG
    sta.l ENT_READY_LONG+1
.IFDEF FX3
    sta.l PROJ_MODE_LONG        ; FrameBody: fill the projection table
    sta.l PROJ_MODE_LONG+1
    ; The reset leaves game pak RAM as the last boot left it, and the first
    ; frame starts before any render sync publishes: it must find gameInit's
    ; hidden entity records, no ring, no shadows, no text. A restart into
    ; another level would otherwise draw the old level's ring and entities
    ; through the new level's banks -- the cartridge's levels test found its
    ; GS_LEVEL_READY trampled that way.
    SETAXY16
    lda #(GS_ITEM_RECORDS * 12 - 1)
    ldx #ol_itemState_gsu
    ldy #(GS_ITEM_LONG & $FFFF)
    phb
    mvn $7E, $70
    plb
    lda #$0000
    sta.l GS_INV_LONG
    sta.l GS_SHADOW_LONG
    sta.l GS_TEXT_LONG
    sta.l GS_RECT_LONG
    ; her meshes as gameInit's Weapon_init left them: the pistols on her
    ; thighs, the shotgun on her back
    lda #23
    ldx #olWeaponKitDraw
    ldy #(GS_KIT_LONG & $FFFF)
    phb
    mvn $7E, $70
    plb
    SETA8
.ENDIF
    lda #$FF
    sta dmaBuffer
    rts
.ENDS

; ---------------------------------------------------------------------------
; Bank 1: the image of the resident host code. Addresses come from slot 1
; ($0100) while the bytes are stored here, which is how the copy above gets a
; position-correct image.
; ---------------------------------------------------------------------------

.BANK 1 SLOT 5
.ORG 0
.SECTION "hostcode" FORCE

HostCodeStart:

; --- $0100-$010F: the landing pads the GSU's dummy vectors point at ---------
;
; JMP rather than JML on purpose. An interrupt in native mode pushes the
; program bank and clears it, so we arrive in bank $00 with the WRAM mirror
; under us; a long jump here would resolve its bank byte to ROM bank 1, where
; this code is stored but never runs. Each pad is padded to 4 bytes because
; the GSU's dummy vectors are 4 apart.

Vec_Brk:                        ; $0100 - BRK / ABORT
    jmp.w BrkHandler
    nop
Vec_Cop:                        ; $0104 - COP
    jmp.w CopHandler
    nop
Vec_Nmi:                        ; $0108 - NMI
    jmp.w NmiHandler
    nop
Vec_Irq:                        ; $010C - IRQ, this is the GSU STOP interrupt
.IFDEF FX3
    jml IrqFx3                  ; FX3: the V-timer's, at the bottom letterbox
.ELSE
    jmp.w IrqHandler
    nop
.ENDIF

; ---------------------------------------------------------------------------

HostMain:
    SETA8
    SETXY16
    lda #$00
    pha
    plb

    jsr StartGSU

    lda #$0F
    sta INIDISP                 ; release forced blank
.IFDEF FX3
    lda #FX3_IRQ_LINE
    sta VTIMEL
    stz VTIMEH
    lda #$A1                    ; NMI, the V-timer IRQ, auto joypad read
.ELSE
    lda #$81
.ENDIF
    sta NMITIMEN                ; NMI + auto joypad read
    cli

MainLoop:
    ; Game time is 30 Hz, independent of how long the GSU takes to draw. NMI
    ; queues one step every two NTSC fields. On GSU-2, taking RAN away parks
    ; a running GSU at its next RAM access but leaves it its ROM bus. FX3
    ; permits concurrent ROM/SRAM access; Take/ReleaseRamBus are no-ops there.
    lda logicPending
    beq @frame
    dec logicPending
    jsr TakeRamBus
    jsr UpdateDemo
.IFNDEF FX3
    jsr CheckRestart            ; FX3: Select opens the death page (game.c)
.ENDIF
    jsr ReleaseRamBus

@frame:
    ; Service completed renders between logic ticks. Waiting for an empty
    ; queue froze GSU-2 presentation under sustained creature load. FX3's
    ; bounded priority below defers publication; both paths keep the upload
    ; source owned until DMA completes and publish only after GSU STOP.
.IFNDEF FX3_STAGE
    lda dmaBusy
    bne @idle
.ENDIF

.IFDEF FX3
    ; The native pipeline stages a completed frame when logic has headroom.
    ; Otherwise it prepares the next job while the planar buffer uploads,
    ; and starts it only after that upload's last planar read.
    ; Claim the completed-buffer handoff atomically against NMI/IRQ.
    ; A set flag also holds ownership throughout CPU publication/startup.
.IFNDEF C_MOTION
    lda #$01
    tsb gsuDone
.ELSE
    lda gsuDone
.ENDIF
    bne @uploaded
.IFDEF FX3_STAGE
    ; The GSU may render while WRAM uploads, but the single staging image
    ; cannot be replaced until the preceding upload has completely finished.
    lda dmaBusy
.IFNDEF C_MOTION
    bne @unclaim
.ELSE
    bne @idle
.ENDIF
.ENDIF
    lda GSU_R15 + 1             ; atomic: executing PCs are all >= $0100
.IFNDEF C_MOTION
    bne @unclaim
.ELSE
    bne @idle
.ENDIF
.IFDEF FX3_STAGE
    jsr StageFX3Frame           ; packed copy, keeping each NMI/input sample
.ENDIF
    jsr QueueUpload
.IFDEF C_MOTION
    lda #$01
    sta gsuDone
.ENDIF
.IFNDEF FX3_STAGE
    bra @idle
.ENDIF
@uploaded:
.IFDEF FX3_STAGE
.IFNDEF C_MOTION
    lda gsuDone
    cmp #$01
    bne @idle                   ; prepared job or an interrupt already started it
.ELSE
    ; Loaded ticks use the original zero-copy, serial pipeline. A staged
    ; frame can release the planar buffer while its WRAM copy uploads.
    lda.l olFrameStaged
    bne +
    lda dmaBusy
    bne @idle
+
.ENDIF
.ENDIF
    ; Native staging admits three queued ticks normally, one during the
    ; burst guard after a 3+-field tick. Without staging, admit two. After
    ; any dropped tick, require an empty queue until level initialization.
    ; Native staging's deadline is eight fields ($88 down to $80): publish
    ; at the next tick boundary. Other pipelines retain sixteen fields.
    ; Both keep upload ownership
    ; and every queued tick/input sample. MainLoop runs
    ; a tick first. The C oracle retains its original empty-queue policy.
.IFNDEF C_MOTION
.IFDEF FX3_STAGE
    jsr FX3PublishPriority
    bra @priority
.ELSE
    lda.l ol_ticksDropped
    ora.l ol_ticksDropped + 1
    bne @overloaded
    lda logicPending
    cmp #$03
    bra @priority
@overloaded:
.ENDIF
.ENDIF
    lda logicPending
    cmp #$01
@priority:
    bcc @publish
    lda renderHold
    beq @holdRender
    cmp #$80
    bne @idle
    bra @publish
@holdRender:
.IFDEF FX3_STAGE
.IFNDEF C_MOTION
    lda #$88                   ; bound stale display time during logic bursts
.ELSE
    lda #$90
.ENDIF
.ELSE
    lda #$90
.ENDIF
    sta renderHold
    bra @idle
@publish:
    stz renderHold
.IFDEF C_MOTION
    stz gsuDone
.ENDIF
.ELSE
    lda gsuDone
    beq @idle
    stz gsuDone

    ; The GSU just finished fbDraw. Queue it for upload, flip, render on.
    jsr QueueUpload
.ENDIF
    jsr TakeRamBus
    jsr UpdateRendererState
    jsr ItemsExchange
    lda fbDraw
    eor #$01
    sta fbDraw
    jsr StartGSU
.IFDEF FX3
.IFNDEF C_MOTION
.IFDEF FX3_STAGE
    bra @idle                   ; StartGSU retains ownership for a deferred job
.ENDIF
@unclaim:
    stz gsuDone                 ; release only after R15 starts the new job
.ENDIF
.ENDIF

@idle:
.IFDEF FX3
    jsr IdleFx3                 ; until a tick is queued or the GSU stops
.ELSE
    lda logicPending            ; catch up without waiting for another field
    bne MainLoop
    wai
.ENDIF
    bra MainLoop

; The per-frame game update lives in src/game.c, in WRAM on GSU-2 and
; FastROM on FX3.
; This is the seam the rest of the engine port will grow through: the host
; assembly does bus and I/O, the C does game state, the GSU does pixels.
;
; 816-tcc code assumes 16-bit A and index registers throughout, and reaches
; its globals with long addressing, so the data bank does not matter.
; ---------------------------------------------------------------------------
; Upstream's death: Select, pressed after she died, opens the inventory on the
; passport, whose Load Game lists the levels -- this one among them, from its
; start. This cartridge carries one level and no menu yet, so that press
; restarts it, through the boot: power-on's own path rebuilds every piece of
; level state, host tables, GSU and sound alike. The press has to start after
; death (isKeyHit): the latch arms on a tick that finds Select up.
; ---------------------------------------------------------------------------
CheckRestart:
    SETA16
    lda.l itemsCount
    beq @alive
    lda.l items+16              ; her health
    beq @dead
    bpl @alive
@dead:
    lda.l ol_keys
    and #IK_SELECT
    SETA8
    bne @pressed
    lda #$01
    sta.l olRestartArm
    rts
@pressed:
    lda.l olRestartArm
    bne RestartLevel
    rts
@alive:
    SETA8
    lda #$00
    sta.l olRestartArm
    rts

; The C's way in (FX3: the death page's Load Game, inventory.c): C runs
; with DBR = $7E (ENTER_C), and everything below addresses the hardware
; through DBR = 0.
.IFDEF FX3
olRestartLevel:
    sei
    SETA8
    lda #$00
    pha
    plb
.ENDIF
RestartLevel:
    sei
    SETA8
    SETXY16
    stz NMITIMEN                ; no NMI: it would rewrite SCMR from its shadow
    stz HDMAEN
    lda #$80
    sta INIDISP                 ; forced blank, as the boot runs
    stz GSU_SFR                 ; GO = 0: the GSU stops where it is
    lda #(SCMR_HT_160 | SCMR_MD_16)
    sta GSU_SCMR                ; both buses the CPU's, as at power-on
    ; The sound CPU runs the driver, not its IPL ROM: a STOP for source $FF
    ; sends it back (spc.asm), and the boot's upload finds $BBAA as ever.
    lda.l ol_sndReady
    beq @boot
    ldx #$0000
-   lda.l APU_PORT0_LONG        ; the last command acknowledged
    cmp.l olSndSeq
    beq +
    dex
    bne -
+   lda #$FF
    sta.l APU_PORT0_LONG+1
    lda.l olSndSeq
    clc
    adc #$04
    and #$FC
    bne +
    lda #$04
+   ora #$03                    ; STOP
    sta.l APU_PORT0_LONG
    ldx #$0000
-   SETA16
    lda.l APU_PORT0_LONG
    cmp #$BBAA
    SETA8
    beq @boot
    dex
    bne -
@boot:
    jml Reset

UpdateDemo:
    SETAXY16
    lda frameIndex
    sta tickStart
    ; What the tick sees is every bit tapped since the last tick, OR the pad
    ; as it stands. The accumulator alone was wrong once ticks got quick: a
    ; tick that runs before another field has been read finds it empty and
    ; reports every button released, and the next tick then sees a fresh
    ; press -- a held fire button fired three times.
.IFDEF FX3
    jsr TickKeysPop             ; its own two fields' keys (TickKeysPush)
.ELSE
    lda keysAccum
    pha
    ora keys
    sta.l ol_keys
    pla
    ; TRB, not STZ: an NMI between the load and the clear ORs in fresh bits,
    ; and STZ would erase them -- a tap shorter than a field, gone. A still
    ; holds the snapshot, and TRB clears exactly the bits that were sampled.
    ; (SEI would not help; NMI is not maskable.)
    trb keysAccum
.ENDIF

.IFNDEF FX3
    ; The load tick: olLevelInit lives in ROM bank $3C (cold, 3.5 KB), so
    ; take the ROM bus too while it runs. RON = 0 parks the GSU at its next
    ; ROM access (Book II 2-9) and it resumes when RON returns. The bit
    ; is cleared in scmrShadow itself, first, so an NMI between the two
    ; stores -- which rewrites SCMR from the shadow -- cannot hand ROM back
    ; in the middle of the call. The real ROM vectors point at the same
    ; landing pads as the GSU's dummy ones, so interrupts are unaffected.
    SETA8
    stz romTaken                ; a byte: logicPhase follows it
    SETA16
    lda.l itemsCount
    bne @noLoad
    lda.l GS_LEVEL_READY_LONG
    and #$00FF
    beq @noLoad
    SETA8
    inc romTaken
    lda scmrShadow
    and #$EF                    ; RON = 0
    sta scmrShadow
    and #$F7                    ; RAN = 0: TakeRamBus already has it
    sta GSU_SCMR
    SETA16
@noLoad:
.ENDIF
    ENTER_C
.IFDEF FX3
.IFNDEF C_MOTION
    jsl RQ_CacheSelect          ; interleaved probes only with admitted actors
.ENDIF
.ENDIF
    jsl gameUpdate
    LEAVE_C
.IFNDEF FX3
    SETA8
    lda romTaken
    beq +
    lda scmrShadow
    ora #$10                    ; RON = 1, in the shadow first
    sta scmrShadow
    and #$F7
    sta GSU_SCMR
+   SETA16
.ENDIF

    ; How many field boundaries the tick straddled. frameIndex counts 0..59,
    ; so a wrap reads negative and is put back; a tick is far shorter than
    ; sixty fields.
    lda frameIndex
    sec
    sbc tickStart
    bpl +
    clc
    adc #60
+   clc
.IFDEF FX3_STAGE
    jsl StageFX3TickCost        ; preserves A; measures staging/publication headroom
.ENDIF
    clc
    adc tickFieldAcc
    sta tickFieldAcc
    inc tickCounter

    SETA8
    rts

; Serialize the current entity state only when a renderer is ready to consume
; it. Doing this for all 60 LEVEL1 items on every 30 Hz logic tick spent most
; of the 65816's budget producing bytes no in-flight frame could observe.
UpdateRendererState:
    SETAXY16
    ENTER_C
    jsl gameRenderSync
.IFDEF FX3
    lda.l olWeaponKitDirty      ; her guns' meshes: only when they changed
    beq +
    jsl Weapon_publish
+
    lda.l olLaraHitQuadrant     ; Lara::draw's hit animation, FX3 only
    and #$00FF
    beq +
    jsl olLaraHitPublish
+
    ; The inventory's ring: its matrices only when a frame will use them
    ; (inventory_draw.c); a list left over from a closed ring is emptied.
    lda.l olInvState
    ora.l olInvGsu
    beq +
    jsl Inventory_publish
+   jsl Settings_text           ; drawFPS's counter, when it changes
    jsl Shadow_publish          ; her shadow, from the same tick
.ENDIF
    LEAVE_C
    SETA8
    rts

; ---------------------------------------------------------------------------
; The two things that cross this bus outside of V-Blank.
;
; Down: the level's entity list, which the GSU staged and which the host has
; no other way to learn -- game pak ROM is unreadable from this side. Read
; where it lies, in game pak RAM bank 1, by the C; nothing is copied.
;
; Up: a byte per entity, the only instruction this side gives the renderer.
;
; Both happen here rather than in V-Blank because the GSU is stopped at this
; point, so taking the RAM bus back costs nothing -- there is nothing to
; stall. The V-Blank window is where the host is under time pressure, and
; neither of these belongs in it.
;
; RAN is left clear; StartGSU writes SCMR from scmrShadow a few instructions
; later, which is what hands the bus back.
; ---------------------------------------------------------------------------

; Take the RAM bus back. The GSU is stopped between frames, so this costs
; nothing -- and it is what lets the C read the staged level where it lies
; instead of out of a copy. The blob is 65,225 of the 65,534 bytes its bank
; allows, against 56 KB of WRAM in all.
TakeRamBus:
    SETA8
.IFNDEF FX3
    ; Record the desired owner first. If NMI lands between these stores, its
    ; own prologue takes the bus and its epilogue sees this byte and leaves it
    ; here. Recording it second lets NMI hand RAM back to the GSU in the
    ; middle of a C update, after which level-blob reads return dummy data.
    lda #$01
    sta cpuOwnsRam
    lda scmrShadow
    and #$F7                    ; RAN = 0
    sta GSU_SCMR
.ENDIF
    rts

; Give game pak RAM back without restarting the GSU. Used after a fixed logic
; step while the renderer is still in flight. Publish the desired owner first
; so an NMI between the two stores restores the same state.
ReleaseRamBus:
    SETA8
.IFNDEF FX3
    stz cpuOwnsRam
    lda scmrShadow
    sta GSU_SCMR
.ENDIF
    rts

.IFDEF FX3
; A (16-bit) = an arm's flash end tick (weapon.c). Out: A, the end to keep --
; itself while the flash runs, else ol_ticks, retired so the comparison never
; comes round again -- and X, upstream's timer: 1..3, or 0 for none.
.ACCU 16                        ; called with A and X/Y 16-bit; wla otherwise
.INDEX 16                       ; carries the widths of the code above
FlashTimer:
    pha
    sec
    sbc.l ol_ticks
    dec a                       ; 0..2 while it runs
    cmp #3
    bcs +
    inc a
    tax
    pla
    rts
+   pla
    ldx #0
    lda.l ol_ticks
    rts
.ENDIF

ItemsExchange:
    ; Where to probe: Lara's eye, which is no longer where the view is.
    SETAXY16
    lda.l ol_probe
    sta.l GS_PROBE_LONG
    lda.l ol_probe + 2
    sta.l GS_PROBE_LONG + 2
    lda.l ol_probe + 4
    sta.l GS_PROBE_LONG + 4

    ; The two arms are independent of Lara's body animation. Publish the
    ; live state only at this stopped-GSU handoff, with the item positions.
    lda.l olLaraWeaponState
    sta.l GS_WEAPON_LONG
    lda #23
    ldx #olWeaponArms
    ldy #((GS_WEAPON_LONG + 2) & $FFFF)
    phb
    mvn $7E, $70
    plb

    ; Local torso/head rotations belong to the same completed-frame snapshot.
    lda #7
    ldx #olLaraLook
    ldy #(GS_LOOK_LONG & $FFFF)
    phb
    mvn $7E, $70
    plb
.IFDEF FX3
    ; and which of the guns' meshes she wears, with the shotgun's torso
    ; (weapon.c Weapon_publish, at the render sync before this)
    lda #23
    ldx #olWeaponKitDraw
    ldy #(GS_KIT_LONG & $FFFF)
    phb
    mvn $7E, $70
    plb
.ENDIF

    ; Twelve bytes an entity, as many entities as the level has (at least
    ; her), by DMA: B to A, from the WRAM port to game pak RAM, 8 master
    ; clocks a byte against MVN's 56 -- 47 K clocks back a frame, which is
    ; where the three-creature gate's headroom comes from. DMA holds NMI
    ; off until it ends, and the V-Blank upload has about six lines to
    ; spare, so it goes in 128-byte pieces: under a line each. Address and
    ; WRAM pointer carry on from one piece to the next; only the count is
    ; rewritten. NMI uses channels 0 and 1 and never the WRAM port. The C
    ; .bss is at $7E:2000 (tools/budget.py prints it at every link).
    SETA16
.IFDEF FX3
    lda.l olSyncEnd             ; gameRenderSync's: short of an idle effects
    bne +                       ; pool, whose records stand as published
    lda.l itemsCount
+   sta.b tcc__r0
    asl a
    clc
    adc.b tcc__r0               ; * 3
.ELSE
    lda.l itemsCount
    asl a
    clc
    adc.l itemsCount            ; * 3
.ENDIF
    asl a
    asl a                       ; * 12
    tay
    beq @pieces                 ; none yet: DAS 0 would be 64 KB over bank $70
    ldx #ol_itemState_gsu
    stx WMADDL                  ; L and M
    ldx #(GS_ITEM_LONG & $FFFF)
    stx A1T2L
    SETA8
    stz WMADDH                  ; bank $7E
    lda #$80                    ; B -> A, increment, one register
    sta DMAP2
    sta BBAD2                   ; WMDATA
    lda #$70
    sta A1B2
    SETA16
@piece:
    tya
    cmp #128
    bcc +
    lda #128
+   sta DAS2L
    SETA8
    lda #$04
    sta MDMAEN
    SETA16
    tya
    sec
    sbc #128
    tay
    beq @pieces
    bpl @piece
@pieces:
    SETA8
    ; Her pitch, byte 11 of her record: the renderer turns X after Y, as
    ; upstream's matrixRotateYXZ(angle.x, ...). Nobody else has one.
    lda.l olLaraAngleX + 1
    sta.l GS_ITEM_LONG + 11
.IFDEF FX3
    ; and her lean, the top byte of angle.z, turned after the pitch
    lda.l olLaraAngleZ + 1
    sta.l GS_ROLL_LONG
    ; and the arms' muzzle flashes (weapon.c): upstream's timer, from the
    ; shot's end tick -- 1..3 ticks ahead, else none, and a spent one is
    ; retired to now so it never comes round again -- and the roll
    SETA16
    lda.l olWeaponFlash
    jsr FlashTimer
    sta.l olWeaponFlash
    SETA8
    txa
    sta.l GS_FLASH_LONG
    lda.l olWeaponFlash + 2
    sta.l GS_FLASH_LONG + 1
    SETA16
    lda.l olWeaponFlash + 4
    jsr FlashTimer
    sta.l olWeaponFlash + 4
    SETA8
    txa
    sta.l GS_FLASH_LONG + 2
    lda.l olWeaponFlash + 6
    sta.l GS_FLASH_LONG + 3
    ; and the shadows (inventory_text.c Shadow_publish): 6 + 12n bytes
    SETA16
    lda.l olShadow
    asl a
    clc
    adc.l olShadow
    asl a
    asl a                       ; 12n
    clc
    adc #5
    ldx #olShadow
    ldy #(GS_SHADOW_LONG & $FFFF)
    phb
    mvn $7E, $70
    plb
    SETA8
    ; The inventory's ring (inventory_draw.c): a count, then 32 bytes an
    ; item, to GS_INV. A zero count is an ordinary frame.
    SETA16
    lda.l olInvGsu
    asl a
    asl a
    asl a
    asl a
    asl a
    inc a                       ; 2 + 32n bytes, less one for MVN
    ldx #olInvGsu
    ldy #(GS_INV_LONG & $FFFF)
    phb
    mvn $7E, $70
    plb
    ; and while there is a ring, its text (inventory_text.c): a count, the
    ; font's bank, the two remaps, 8 bytes a glyph, to GS_TEXT; and its
    ; rectangles, a count and 6 bytes each, to GS_RECT
    lda.l olInvGsu
    beq @gameText
    lda.l olInvText
    asl a
    asl a
    asl a
    clc
    adc #7                      ; 8 + 8n bytes, less one for MVN
    ldx #olInvText
    ldy #(GS_TEXT_LONG & $FFFF)
    phb
    mvn $7E, $70
    plb
.IF OL_TITLE_SIZE > 0
    jsl olTitlePin              ; the title's picture stays BG_BUF's
.ENDIF
    lda.l olInvRects
    and #$00FF
    sta.l GS_RECT_LONG          ; the count, a word for the GSU
    asl a
    adc.l olInvRects
    and #$00FF
    asl a                       ; 6n bytes (n <= 12)
    beq +
    dec a
    ldx #(olInvRects + 2)
    ldy #((GS_RECT_LONG + 2) & $FFFF)
    phb
    mvn $7E, $70
    plb
    bra +
@gameText:
    ; no ring: the game's own text, drawFPS's (settings.c; FrameBody draws
    ; GS_TEXT over a game frame too), when it has changed
    jsl olGameText
+   SETA8
.ENDIF
    rts

; ---------------------------------------------------------------------------
; Hand the buses to the GSU and set it running at the frame entry point.
; Book II 2-4: CLSR, CFGR, SCBR, PBR, SCMR, then R15 last -- the write to R15
; is what starts it.
; ---------------------------------------------------------------------------

StartGSU:
    SETA8
    ; No CPU game pak RAM access occurs below this point. Publish the desired
    ; owner before touching the GSU registers so an intervening NMI restores
    ; RAN to the side that is about to run.
    stz cpuOwnsRam
    lda fbDraw
    bne +
    lda #FB0_SCBR
    bra ++
+   lda #FB1_SCBR
++  sta GSU_SCBR

    ; R3 carries the frame buffer offset into the GSU program; SCBR alone is
    ; write only, so the GSU cannot read back where it is drawing.
    SETA16
    lda fbDraw
    and #$00FF
    beq +
    lda #FB1_ADDR
    bra ++
+   lda #FB0_ADDR
++  sta GSU_R3

    ; The camera and the room come from the C side. GSU registers are writable
    ; while it is idle, which is exactly now.
    lda.l ol_camRoom
    sta GSU_R6
    lda.l ol_camX
    sta GSU_R7
    ; The GSU's sine table is 256 entries, so it takes the top byte of the
    ; host's full TR angle. TURN_SPEED is a multiple of 256 to keep them in
    ; step; see src/game.c.
    lda.l ol_angle
    xba
    and #$00FF
    sta GSU_R8
    lda.l ol_camZ
    sta GSU_R9
    lda.l ol_camY
    sta GSU_R10
    lda.l ol_camTgtY            ; what the view looks at: the GSU's shear
    sta GSU_R11

    SETA8
    lda #GSU_PROGRAM_BANK
    sta GSU_PBR
    lda scmrShadow
    sta GSU_SCMR

    SETA16
.IFDEF FX3
    lda.l olLevel+OLD_GSU_ENTRY ; the level's program (several share PBR $41)
.ELSE
    lda #GSU_FRAME_ENTRY
.ENDIF
.IFDEF FX3_STAGE
.IFNDEF C_MOTION
    jmp PrepareFX3Start
.ENDIF
.ENDIF
    sta GSU_R15                 ; low byte first, and away it goes
    SETA8
    rts

; ---------------------------------------------------------------------------
; Set up the chunked VRAM upload of the buffer the GSU has just finished.
; ---------------------------------------------------------------------------

QueueUpload:
.IFDEF FX3_STAGE
    jmp QueueUploadFX3Staged
.ELSE
    SETA8
    lda fbDraw
    sta dmaBuffer
    bne +
    SETA16
    lda #FB0_ADDR
    sta dmaSrc
    lda #VRAM_FB0
    sta dmaDst
    bra ++
+   SETA16
    lda #FB1_ADDR
    sta dmaSrc
    lda #VRAM_FB1
    sta dmaDst
++  lda #FRAME_SIZE
.IFDEF FX3
    lda #FRAME_COLS             ; upload skips the two unused rows per column
.ENDIF
    sta dmaLeft
    SETA8
    lda #$01
    sta dmaBusy
    rts
.ENDIF

; ---------------------------------------------------------------------------
; V-Blank. Push one chunk of the frame buffer into VRAM, then read the pad.
;
; GSU-2 is left running throughout; taking RAN away parks it at its next
; game pak RAM access and giving RAN back releases it (Book II 2-5). FX3
; permits concurrent CPU/FX ROM and SRAM access and does not change RAN.
; ---------------------------------------------------------------------------

NmiHandler:
    rep #$30
    pha
    phx
    phy
    phb
    phd
    SETA8
    lda #$00
    pha
    plb
    rep #$20
    lda #$0000
    tcd

    SETA8
    lda RDNMI                   ; clear the NMI flag

    SETA16
    inc frameIndex              ; 16 bit: INC honours the M flag, and doing
    lda frameIndex              ; this with A 8 bit only ever bumps the low
    cmp #60                     ; byte
    bcc +
    stz frameIndex
    lda fpsCounter
    sta fps
    stz fpsCounter
    lda tickCounter             ; the clock's gauges, beside fps
    sta.l ol_tps
    stz tickCounter
    lda tickFieldAcc
    sta.l ol_tickFields
    stz tickFieldAcc
+   SETA8

.IFDEF FX3
    ; Upstream's fade-in (gBrightness through palBright): every channel of
    ; the picture less 28, 24 .. 4, 0 over the ticks after the load, two
    ; fields a tick of real time, as upstream's `frames` are -- the fixed
    ; colour colour math subtracts from BG1. game.c starts olFadeFields at
    ; 15; m fields left is 4 * ceil(m / 2).
    lda.l olFadeFields
    beq +
    dec a
    sta.l olFadeFields
    inc a
    and #$FE
    asl a
+   ora #$E0
    sta COLDATA
.ENDIF

    ; GSU-2 only: clearing RAN parks the GSU at its next game pak RAM access;
    ; setting it releases it again. FX3 does not need this ownership handoff.
.IFNDEF FX3
    lda scmrShadow
    and #$F7                    ; RAN = 0
    sta GSU_SCMR
.ENDIF
.IFNDEF MODE7
.IFDEF FX3
    ; The settings' gamma (settings.c): 64 bytes of CG-RAM a field -- the
    ; frame's chunk below wants most of V-Blank -- and none in a field the
    ; HUD goes up in (save_fx3.asm olCgramPiece)
    jsl olCgramPiece
.ENDIF
    ; The HUD's 26 sprites, first: OAM is only writable in V-Blank, and the
    ; frame's chunk below is sized to fill most of it. 104 bytes, 13 us.
    lda.l olHudDirty
    beq @nohud
    lda #$00
    sta.l olHudDirty
    stz OAMADDL
    stz OAMADDH
    stz DMAP1                   ; A->B, increment, one register
    lda #$04                    ; OAMDATA
    sta BBAD1
    ldx #olHudOam
    stx A1T1L
    lda #$7E
    sta A1B1
    ldx #(26 * 4)
    stx DAS1L
    lda #$02
    sta MDMAEN
@nohud:
.ENDIF
.IFDEF FX3
.IFNDEF C_MOTION
    jsr TryQueueFX3Frame
.ENDIF
.ENDIF
    lda dmaBusy
    beq @nodma
.IFDEF FX3
    lda.l olIrqUpload           ; preempted IrqFx3 owns the upload
    bne @nodma
.ENDIF
    jsr UploadChunk
@nodma:
    ; An NMI can also interrupt UpdateDemo/ItemsExchange while the host owns
    ; game pak RAM and the GSU is stopped. Do not blindly give the bus away on
    ; return: the interrupted C code may still be reading the staged level.
.IFNDEF FX3
    lda cpuOwnsRam
    bne @keepRam
    lda scmrShadow
    sta GSU_SCMR                ; RAN = 1, the running GSU resumes
@keepRam:
.ENDIF
    jsr ReadPad

.IFDEF FX3
    ; Saturate at the forced-publication marker, not zero: zero means no
    ; deferral has started. This keeps the deadline across frameIndex wraps
    ; and even a logic call longer than one complete sixty-field epoch.
    lda renderHold
    cmp #$81
    bcc @renderTimerDone
    dec renderHold
@renderTimerDone:
.IFDEF FX3_STAGE
    jsl StageFX3Field
.ENDIF
.ENDIF

    ; The cartridge is NTSC (header country 1): two fields are one 30 Hz game
    ; tick. Cap the backlog so a debugger pause cannot turn into seconds of
    ; catch-up with the renderer starved of RAM.
    lda logicPhase
    eor #$01
    sta logicPhase
    bne @logicDone
    lda logicPending
    cmp #$04
    bcs @dropped
.IFDEF FX3
    jsr TickKeysPush            ; and its keys
.ENDIF
    inc logicPending
    bra @logicDone
@dropped:
    ; The cap held: game time just dilated by a tick. Count it, so that a
    ; scene expensive enough to saturate the backlog says so.
.IFDEF FX3
    jsr TickKeysMerge           ; its keys go to the last one queued
.ENDIF
    SETA16
    lda.l ol_ticksDropped
    inc a
    sta.l ol_ticksDropped
    SETA8
@logicDone:

    rep #$30
    pld
    plb
    ply
    plx
    pla
    rti

; ---------------------------------------------------------------------------

UploadChunk:
.IFDEF FX3
    jmp.w UploadFX3Chunk
.ELSE
    SETA8
    lda #$80
    sta VMAIN                   ; +1 word after the $2119 write
.IFDEF MODE7
    lda #$00                    ; A->B, increment, one register
    sta DMAP0
    lda #$19                    ; VMDATAH alone: a byte of frame a word of
    sta BBAD0                   ; VRAM, and the map in the low bytes stays
.ELSE
    lda #$01                    ; A->B, increment, two registers L,H
    sta DMAP0
    lda #$18                    ; VMDATAL
    sta BBAD0
.ENDIF
    lda #FB_BANK
    sta A1B0

    SETA16
    lda dmaDst
    sta VMADDL
    lda dmaSrc
    sta A1T0L

    lda dmaLeft
    cmp #DMA_CHUNK
    bcc +                       ; last, short chunk
    lda #DMA_CHUNK
+   sta DAS0L
    tax                         ; X = bytes about to move

    SETA8
    lda #$01
    sta MDMAEN

    SETA16
    txa
    clc
    adc dmaSrc
    sta dmaSrc

    txa
.IFNDEF MODE7
    lsr a                       ; VRAM addresses count words
.ENDIF
    clc
    adc dmaDst
    sta dmaDst

    lda dmaLeft
    stx dmaTmp
    sec
    sbc dmaTmp
    sta dmaLeft
    bne @more

.IFDEF SHADE_MAP
    ; Frame complete. The shade map goes up in the same field as the flip:
    ; the tile map is shared by both frame buffers, so uploading it earlier
    ; would re-shade the frame still on screen.
    jsr UploadShade
.ENDIF

    ; Show what was just uploaded.
    SETA8
    lda dmaBuffer
    sta fbShow
.IFDEF MODE7
    ; The Mode 7 map's rows 0-7 name one buffer's characters and 8-15 the
    ; other's: the flip is a vertical scroll of 64 map pixels.
    bne +
    lda #(M7VOFS_FB0 & $FF)
    sta BG1VOFS
    lda #(M7VOFS_FB0 >> 8)
    sta BG1VOFS
    bra ++
+   lda #(M7VOFS_FB1 & $FF)
    sta BG1VOFS
    lda #(M7VOFS_FB1 >> 8)
    sta BG1VOFS
++
.ELSE
    bne +
    lda #BG12NBA_FB0
    bra ++
+   lda #BG12NBA_FB1
++  sta BG12NBA
.ENDIF
    stz dmaBusy
    SETA16
    inc fpsCounter
    SETA8
    rts

@more:
    SETA8
    rts
.ENDIF

; ---------------------------------------------------------------------------
; Push the shade map over the tile map's high bytes.
;
; Write pattern 0 to $2119 alone, with VMAIN incrementing on the high byte, so
; each transferred byte lands in one entry's attribute half and the character
; numbers written at boot stay where they are. 640 bytes.
; ---------------------------------------------------------------------------

.IFDEF SHADE_MAP
UploadShade:
    SETA8
    lda #$80
    sta VMAIN                   ; +1 word after the $2119 write
    lda #$00                    ; A->B, increment, one register
    sta DMAP0
    lda #$19                    ; VMDATAH
    sta BBAD0
    lda #FB_BANK
    sta A1B0

    SETA16
    lda #VRAM_TILEMAP
    sta VMADDL
    lda dmaBuffer
    and #$00FF
    beq +
    lda #SHADE1_ADDR
    bra ++
+   lda #SHADE0_ADDR
++  sta A1T0L
    lda #SHADE_SIZE
    sta DAS0L

    SETA8
    lda #$01
    sta MDMAEN
    rts
.ENDIF

; ---------------------------------------------------------------------------
; Auto joypad read finishes a few scanlines into V-Blank, so wait it out.
; ---------------------------------------------------------------------------

ReadPad:
    SETA8
-   lda HVBJOY
    and #$01
    bne -

    SETA16
    lda keys
    sta keysPrev
    lda JOY1L                   ; $4218/$4219 as one 16-bit read
    sta padRaw
    jsr RemapPad

    ; The pad is read every field and the game updates every other one. Keep
    ; every bit seen between ticks so a one-field tap survives.
    SETA16
    lda keys
    ora keysAccum
    sta keysAccum
    SETA8
    rts

; SNES pad bit order is  high byte: B Y Select Start Up Down Left Right,
; low byte: A X L R - - - -. Fold it into OpenLara's InputKey bits.
RemapPad:
    SETA16
    stz keys
    ldx #$0000
-   lda.w PadMap,x
    beq +
    and padRaw
    beq @skip
    inx
    inx
    lda.w PadMap,x
    ora keys
    sta keys
    dex
    dex
@skip:
    inx
    inx
    inx
    inx
    bra -
+   rts

PadMap:
    .DW $0800, IK_UP
    .DW $0400, IK_DOWN
    .DW $0200, IK_LEFT
    .DW $0100, IK_RIGHT
    .DW $8000, IK_B            ; SNES B  -> action
    .DW $0080, IK_A            ; SNES A  -> jump
    .DW $0020, IK_L
    .DW $0010, IK_R
    .DW $4000, IK_Y            ; SNES Y  -> look
    .DW $0040, IK_UP | IK_DOWN ; SNES X  -> roll: upstream's IN_UP | IN_DOWN,
                               ; which a d-pad cannot press (NDS: a button)
    .DW $1000, IK_START
    .DW $2000, IK_SELECT
    .DW $0000, $0000

; ---------------------------------------------------------------------------
; The GSU raises IRQ when it executes STOP.
; ---------------------------------------------------------------------------

.IFNDEF FX3
IrqHandler:
    rep #$30
    pha
    phb
    SETA8
    lda #$00
    pha
    plb

    lda GSU_SFRH                ; reading the status byte clears the IRQ flag
    lda #$01
    sta gsuDone

    plb
    rep #$30
    pla
    rti
.ENDIF

BrkHandler:
CopHandler:
    rti

HostCodeEnd:

; Budget check for WRAM $0100-$05FF lives in the Makefile's link rule: under
; -d -s -x wla defers section-relative arithmetic to link time, so neither
; .IF nor .PRINTV can see this size here. The link reads HostCodeEnd out of
; the symbol file and fails the build past $0600.
.ENDS

; ---------------------------------------------------------------------------
; Bank 2 onwards: the GSU program image. The GSU sees ROM linearly in banks
; $40-$5F, so ROM offset $010000 is GSU address $41:0000.
; ---------------------------------------------------------------------------

.DEFINE GSU_PROGRAM_BANK    $41
.IFDEF FX3
.DEFINE GSU_FRAME_ENTRY     $0100
.ELSE
.DEFINE GSU_FRAME_ENTRY     $0000
.ENDIF

.BANK 2
.ORG 0
.SECTION "gsuprogram" FORCE
    .INCBIN "gsu.bin"
.ENDS
.IFDEF FX3
.IF LEVEL_COUNT > 1
; The second level's GSU program, assembled to lie here, at $41:8000.
.BANK 3
.ORG 0
.SECTION "gsuprogram1" FORCE
    .INCBIN "gsu_1/gsu.bin"
.ENDS
.ENDIF
.ENDIF

; ---------------------------------------------------------------------------
; The level and the texture pages, placed on GSU bank boundaries by
; tools/mkbanks.py. Both are addressed through the GSU's linear $40-$5F image,
; so the unit of placement is a 64 KB GSU bank -- two wla banks in LoROM.
; ---------------------------------------------------------------------------

.INCLUDE "textures.inc"

.IFDEF FX3
; This code stays in ROM: FX3's host ROM bus remains available at all times.
.BANK 0 SLOT 0
.SECTION "fx3upload" FREE
; The view is 144 of 224 lines, so lines 0-39 show only the blank tile: under
; forced blank they are V-Blank as far as VRAM is concerned (the view's first
; line is 41: BG1VOFS -41). A field's chunk runs from the NMI, past V-Blank's
; 37 lines, into them: seven columns, 8 KB, about 50 lines of DMA at 165
; bytes a line (less DRAM refresh) against the 77 there are to line 41; the
; last field's six and the map, 8.8 KB. Ten columns ended at line 39 in
; TypeSNES, which does not charge the refresh: too close. The display comes
; back when the DMA has stopped, not on a timer, so a late chunk would black
; out the top of the view for a field rather than lose VRAM writes; no HDMA
; (S-CPU v1's GP-DMA / H-DMA collision, cpu-system.md Problem 1). Was four
; columns in V-Blank alone: seven fields a frame, the frame rate's floor.
UploadFX3Chunk:
    SETA8
    ldy #FX3_DMA_COLS
UploadFX3Cols:                  ; Y columns at most (IrqFx3: FX3_IRQ_COLS)
    lda #$80
    sta INIDISP                 ; forced blank, full brightness
    sta VMAIN
    lda #$01
    sta DMAP0
    lda #$18
    sta BBAD0
.IFDEF FX3_STAGE
    lda.l olFrameStaged
    beq +
    lda #:olFrameStage
    ldx #FX3_COL_BYTES
    bra ++
+   lda #FB_BANK
    ldx #FX3_COL_STRIDE
++
.ELSE
    lda #FB_BANK
.ENDIF
    sta A1B0
.IFDEF FX3_STAGE
    stx dmaTmp                 ; invariant stride, once for the whole chunk
.ENDIF
@column:
    SETA16
    lda dmaDst
    sta VMADDL
    lda dmaSrc
    sta A1T0L
    lda #FX3_COL_BYTES
    sta DAS0L
    SETA8
    lda #$01
    sta MDMAEN
    SETA16
    lda dmaSrc
    clc
.IFDEF FX3_STAGE
    adc dmaTmp
.ELSE
    adc #FX3_COL_STRIDE
.ENDIF
    sta dmaSrc
    lda dmaDst
    clc
    adc #(FX3_COL_BYTES / 2)
    sta dmaDst
    dec dmaLeft
    beq @complete
    dey
    bne @column
    SETA8
    lda #$0F
    sta INIDISP                 ; display on: the view starts at line 40
    rts
@complete:
.IFDEF FX3_STAGE
.IFNDEF C_MOTION
    SETA8
    jsr StartQueuedFX3Frame     ; planar reads are done; tilemap DMA can overlap
.ENDIF
.ENDIF
    ; Last chunk: its columns and the map's view rows.
    jsr UploadFX3MapRows
    lda #$0F
    sta INIDISP
    lda dmaBuffer
    sta fbShow
    stz dmaBusy
    SETA16
    inc fpsCounter
    SETA8
    rts

.IFDEF FX3_STAGE
.IFNDEF C_MOTION
; All render state and GSU registers are prepared at a tick boundary. The
; last DMA chunk can then launch this exact snapshot while CPU logic runs.
; gsuDone: 0 running, 1 completed/claimed, 3 prepared and awaiting upload.
PrepareFX3Start:
    .ACCU 16
    sta.l olFrameEntry
    SETA8
    lda #$03
    sta gsuDone
    ; Check after publishing ready: an NMI may have finished the upload
    ; between the last register write and this store.
StartQueuedFX3Frame:
    .ACCU 8
    lda.l olFrameStaged
    bne @ready
    lda dmaBusy
    beq @ready
    lda dmaLeft
    ora dmaLeft + 1
    bne @out
@ready:
    lda #$02
    trb gsuDone                 ; nested NMI/IRQ can claim the ready bit once
    beq @out
    SETA16
    lda.l olFrameEntry
@launch:
    sta GSU_R15
    SETA8
    stz gsuDone
@out:
    rts
.ENDIF

; A-bus cartridge SRAM -> B-bus WMDATA, 27 small DMAs. The destination
; auto-increments; source columns include two unused tile rows. NMI/IRQ use
; channels 0/1, never channel 2 or WMADD. Do not mask NMI over this copy.
; cpu-system.md "General purpose DMA (17.1)"; ppu-registers.md "$2180".
StageFX3Frame:
    SETA8
    lda #$00
    sta.l olFrameStaged
.IFNDEF C_MOTION
    jsr FX3WantStaging
    beq @direct
.ELSE
    lda.l olFrameLoadHold
    bne @direct
.ENDIF
    lda #$01
    sta.l olFrameStaged
    stz WMADDL
    lda #$80
    sta WMADDM
    lda #$01
    sta WMADDH
    stz DMAP2
    lda #$80
    sta BBAD2
    lda #FB_BANK
    sta A1B2
    SETA16
    ldx #FB0_ADDR
    ldy #FRAME_COLS
@column:
    stx A1T2L
    lda #FX3_COL_BYTES
    sta DAS2L
    SETA8
    lda #$04
    sta MDMAEN
    SETA16
    txa
    clc
    adc #FX3_COL_STRIDE
    tax
    dey
    bne @column
    SETA8
@direct:
    rts

; A queued frame records its source mode; later load measurements cannot
; change that source until its last DMA column and tilemap have landed.
QueueUploadFX3Staged:
    SETA8
    lda fbDraw
    sta dmaBuffer
    SETA16
    and #$00FF
    beq +
    lda #VRAM_FB1
    bra ++
+   lda #VRAM_FB0
++  sta dmaDst
    lda #FB0_ADDR
    sta dmaSrc
    SETA8
    lda.l olFrameStaged
    beq +
    SETA16
    lda #olFrameStage
    sta dmaSrc
+   SETA16
    lda #FRAME_COLS
    sta dmaLeft
    SETA8
    lda #$01
    sta dmaBusy
    rts

; Preserve the original 30 Hz tick gauge. Two-field ticks renew a 120-field
; hold: native staging then requires an empty logic queue and no drops.
; A 3+-field tick starts a four-field burst guard (131..128), which also
; reserves publication headroom. Moderate ticks cannot shorten that guard.
; The C oracle retains its original two-second zero-copy hold.
StageFX3TickCost:
    .ACCU 16                    ; caller's A16, independent of preceding RTS
    cmp #2
    bcc @out
    pha
.IFNDEF C_MOTION
    cmp #3
    bcc @moderate
    lda #131                    ; four fields of burst headroom (131..128)
    bra @save
@moderate:
    lda.l olFrameLoadHold
    cmp #128
    bcs @restore
.ENDIF
    lda #120
@save:
    sta.l olFrameLoadHold
@restore:
    pla
@out:
    rtl

.IFNDEF C_MOTION
FX3WantStaging:
    .ACCU 8
    lda.l olFrameLoadHold
    bmi @no
    beq @yes
    lda logicPending
    bne @no
    lda.l ol_ticksDropped
    ora.l ol_ticksDropped + 1
    bne @no
@yes:
    lda #$01
    rts
@no:
    lda #$00
    rts

FX3PublishPriority:
    .ACCU 8
    lda.l ol_ticksDropped
    ora.l ol_ticksDropped + 1
    bne @overloaded
    lda.l olFrameLoadHold
    bmi @burst
    lda logicPending
    cmp #$04
    rts
@burst:
    lda logicPending
    cmp #$02
    rts
@overloaded:
    lda logicPending
    cmp #$01
    rts
.ENDIF

StageFX3Field:
    .ACCU 8                     ; NMI calls with A8
    lda.l olFrameLoadHold
    beq @out
    dec a
    sta.l olFrameLoadHold
@out:
    rtl
.ENDIF

; The keys a tick sees, FX3: its own. Every second field the NMI queues a
; tick and, with it, every key seen in that tick's two fields (TickKeysPush);
; the tick takes its own slot (TickKeysPop). Upstream's ticks each see their
; period's input. Taking whatever had piled up instead lost presses when
; ticks ran late and bunched -- a render sync of the ring's is ~5 fields --
; because the bunched ticks saw one accumulated state and never the release
; between two taps. Eight slots for at most four queued: the slot the NMI
; writes is never the one being read. A dropped tick's keys join the last
; queued one's. The NMI runs these with D = 0 and DBR = 0, A 8 bits.
TickKeysPush:
    SETA16
    lda.l olTickTail
    and #$000E
    tax
    lda keysAccum               ; ReadPad has ORed this field's in
    ora keys
    sta.l olTickKeys,x
    txa
    inc a
    inc a
    and #$000E
    sta.l olTickTail
    stz keysAccum
    SETA8
    rts

TickKeysMerge:
    SETA16
    lda.l olTickTail
    dec a
    dec a
    and #$000E
    tax
    lda keysAccum
    ora keys
    ora.l olTickKeys,x
    sta.l olTickKeys,x
    stz keysAccum
    SETA8
    rts

; UpdateDemo's (A 16 bits): the next queued tick's keys to ol_keys.
.ACCU 16                        ; wla tracks SEP/REP textually: the above ends A8
TickKeysPop:
    lda.l olTickHead
    and #$000E
    tax
    lda.l olTickKeys,x
    sta.l ol_keys
    txa
    inc a
    inc a
    and #$000E
    sta.l olTickHead
    rts

.IFNDEF C_MOTION
; A finished direct frame can begin uploading at either display interrupt,
; even during a long logic tick. CPU publication still waits for that tick.
; TSB is the only claim operation on both sides. Claim before checking R15;
; retain the flag until the next StartGSU has completed. No C scratch used.
TryQueueFX3Frame:
    .ACCU 8                     ; NMI and IRQ enter with A8
    lda dmaBusy
    bne @out
.IFDEF FX3_STAGE
    jsr FX3WantStaging
    bne @out                    ; stage only when the logic queue has headroom
.ENDIF
    lda #$01
    tsb gsuDone
    bne @out
    lda GSU_R15 + 1
    beq @stopped
    stz gsuDone
@out:
    rts
@stopped:
.IFDEF FX3_STAGE
    lda #$00
    sta.l olFrameStaged
.ENDIF
    jmp QueueUpload             ; RTS back to the claiming interrupt
.ENDIF

; The main loop's wait, FX3: back as soon as the NMI queues a tick or the
; GSU STOPs. WAI saw the STOP only at the next NMI, up to a field late, and
; with the render overlapping the upload that render is the frame rate's
; floor. R15 is read a few hundred times a field, not in a tight loop: the
; register window is on the cartridge bus the GSU is fetching from, and a
; spinning read starved it (TypeSNES: the first frame's TextRender never
; finished).
IdleFx3:
    lda logicPending
    bne @out
    lda dmaBusy                 ; an upload: nothing to start before the NMI
    bne @wait
    lda gsuDone                 ; a publication held for logic: the same
    bne @wait
    lda GSU_R15 + 1             ; atomic: executing PCs are all >= $0100
    beq @out
    ldx #256                    ; about 1,300 cycles between reads
-   dex
    bne -
    bra IdleFx3
@wait:
    wai
@out:
    rts

; The bottom letterbox, lines 185-224, is as blank as the top: the V-timer
; IRQ at line 185 blanks and uploads up to fourteen columns (16 KB) there,
; on through V-Blank into the top one -- about 100 lines of the 119 to line
; 41 -- so a frame goes up in two fields (14, 13 + the map) where the NMI
; alone took four. The NMI arriving in the middle of it leaves the upload
; to it (olIrqUpload) and does the rest of its work; a frame queued after
; line 185 starts with the NMI's own chunk. On the IRQ vector's four bytes
; (the GSU-2's STOP interrupt; FX3 masks the GSU's).
IrqFx3:
    rep #$30
    pha
    phx
    phy
    phb
    phd
    pea $0000
    pld
    SETA8
    lda #$00
    pha
    plb
    lda TIMEUP                  ; acknowledge
.IFNDEF C_MOTION
    jsr TryQueueFX3Frame
.ENDIF
    lda dmaBusy
    beq @out
    lda #$01
    sta.l olIrqUpload
    ldy #FX3_IRQ_COLS
    jsr UploadFX3Cols
    lda #$00
    sta.l olIrqUpload
@out:
    rep #$30
    pld
    plb
    ply
    plx
    pla
    rti

; The boot's whole map; a frame's flip sends only the view's 18 rows, the
; only ones in which the two buffers' maps differ (the rest are the blank
; tile in both): 1,152 bytes rather than 2,048, off the heaviest chunk.
UploadFX3Map:
    ldx #$0800
    bra +
UploadFX3MapRows:
    ldx #(FRAME_ROWS * 64)
+   SETA8
    lda #$80
    sta VMAIN
    lda #$01
    sta DMAP0
    lda #$18
    sta BBAD0
    lda #:FX3Tilemaps
    sta A1B0
    SETA16
    lda #VRAM_TILEMAP
    sta VMADDL
    lda dmaBuffer
    and #$00FF
    beq +
    lda #(FX3Tilemaps + $800)
    bra ++
+   lda #FX3Tilemaps
++  sta A1T0L
    stx DAS0L
    SETA8
    lda #$01
    sta MDMAEN
    rts
FX3Tilemaps:
    .INCBIN "fx3maps.bin"
.ENDS
.ENDIF

; ---------------------------------------------------------------------------
; Cartridge header. Hand rolled rather than via .SNESHEADER because a Super FX
; pak has to declare its expansion RAM at $FFBD, which the directive does not
; cover. tools/snesfix.py fills in the checksum pair afterwards.
;
; Nothing in the header names the Super FX die, so the pair below is what
; selects it: 128 KB of expansion RAM is more than the 100-pin package can
; address, so the board can only be carrying a GSU-2. That matters -- see
; "Which Super FX" in the README -- and shrinking $FFBD would quietly move the
; cartridge onto a part whose ROM and RAM windows this port is already
; outgrowing. `make check` asserts the revision for that reason.
; ---------------------------------------------------------------------------

.BANK 0
.ORGA $FFB0
.SECTION "header" FORCE
    .DB "00"                    ; $FFB0 maker code
    .DB "OLRA"                  ; $FFB2 game code
    .DB $00, $00, $00, $00, $00, $00, $00   ; $FFB6 fixed zero
    .DB $07                     ; $FFBD expansion RAM: 1 Mbit (128 KB), GSU-2
    .DB $00                     ; $FFBE special version
    .DB $00                     ; $FFBF cartridge sub-number
.IFDEF FX3
    .DB "OPENLARA FX3        "  ; $FFC0 title, 21 bytes including next byte
.ELSE
    .DB "OPENLARA            "  ; $FFC0 title, 21 bytes
.ENDIF
    .DB " "
.IFDEF FX3
    .DB $30                     ; $FFD5 FastROM
    .DB $18                     ; $FFD6 FX3 with battery: the save (src/save.c)
    .DB $0C                     ; $FFD7 FX3: 32 Mbit CPU ROM, 24 Mbit FX window
.ELSE
    .DB $20                     ; $FFD5 map mode 20 (LoROM), 2.68 MHz
    .DB $15                     ; $FFD6 Super FX + RAM + battery
    .DB $0B                     ; $FFD7 GSU-2: 16 Mbit
.ENDIF
    .DB $00                     ; $FFD8 CPU-bus SRAM: none, it is all GSU RAM
    .DB $01                     ; $FFD9 destination: North America
    .DB $33                     ; $FFDA fixed
    .DB $00                     ; $FFDB mask ROM version
    .DW $FFFF                   ; $FFDC complement, patched by snesfix.py
    .DW $0000                   ; $FFDE checksum, patched by snesfix.py
.ENDS

.BANK 0
.ORGA $FFE4
.SECTION "vectors_native" FORCE
    .DW Vec_Cop                 ; $FFE4 COP
    .DW Vec_Brk                 ; $FFE6 BRK
    .DW Vec_Brk                 ; $FFE8 ABORT
    .DW Vec_Nmi                 ; $FFEA NMI
    .DW $0000                   ; $FFEC unused
    .DW Vec_Irq                 ; $FFEE IRQ
.ENDS

.BANK 0
.ORGA $FFF4
.SECTION "vectors_emu" FORCE
    .DW Vec_Cop                 ; $FFF4 COP
    .DW $0000                   ; $FFF6 unused
    .DW Vec_Brk                 ; $FFF8 ABORT
    .DW Vec_Nmi                 ; $FFFA NMI
    .DW Reset                   ; $FFFC RESET
    .DW Vec_Irq                 ; $FFFE IRQ / BRK
.ENDS

.IFDEF FX3
; The cartridge's levels: where each one's data is (tools/mkbanks.py).
.BASE $80
.BANK 6 SLOT 0
.SECTION "level_table" FREE
LevelTable:
.INCLUDE "leveltable.inc"
.ENDS
.BASE $00
.ENDIF

; Keep this last: its native routine selects .16bit address operands. That
; assembler setting must not leak into the host loop's direct-page opcodes.
.INCLUDE "src/host_tables.asm"

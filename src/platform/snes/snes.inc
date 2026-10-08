; OpenLara - SNES / Super FX target
; Hardware definitions for the 65816 host side.
;
; Register documentation follows the SNES Development Manual Book I/II.

; Build configuration, written by the Makefile. Defines SHADING when the
; per-character shade pass is wanted; see the Shading section of the README.
.INCLUDE "config.inc"
.IFDEF MODE7
.DEFINE PIXEL_FRAME 1
.ENDIF
.IFDEF FX3
.DEFINE PIXEL_FRAME 1
.ENDIF

.IFDEF FX3
.DEFINE GSU_REG_OFFSET $4000
.ELSE
.DEFINE GSU_REG_OFFSET $0000
.ENDIF

.DEFINE INIDISP     $2100   ; screen display: D7 forced blank, D3-D0 brightness
.DEFINE OBJSEL      $2101
.DEFINE OAMADDL     $2102
.DEFINE OAMADDH     $2103
.DEFINE OAMDATA     $2104
.DEFINE BGMODE      $2105   ; D2-D0 mode, D7-D4 char size per BG
.DEFINE MOSAIC      $2106
.DEFINE BG1SC       $2107   ; D7-D2 tilemap base (1K-word units), D1-D0 size
.DEFINE BG2SC       $2108
.DEFINE BG3SC       $2109
.DEFINE BG4SC       $210A
.DEFINE BG12NBA     $210B   ; D3-D0 BG1 char base, D7-D4 BG2 (4K-word units)
.DEFINE BG34NBA     $210C
.DEFINE BG1HOFS     $210D   ; write twice (low, high)
.DEFINE BG1VOFS     $210E
.DEFINE BG2HOFS     $210F
.DEFINE BG2VOFS     $2110
.DEFINE VMAIN       $2115   ; D7 inc-on-high, D1-D0 inc step
.DEFINE VMADDL      $2116
.DEFINE VMADDH      $2117
.DEFINE VMDATAL     $2118
.DEFINE VMDATAH     $2119
.DEFINE M7SEL       $211A
.DEFINE M7A         $211B   ; the Mode 7 matrix, 8.8, each written twice (low, high)
.DEFINE M7B         $211C
.DEFINE M7C         $211D
.DEFINE M7D         $211E
.DEFINE M7X         $211F   ; its centre, 13 bits, written twice
.DEFINE M7Y         $2120
.DEFINE CGADD       $2121
.DEFINE CGDATA      $2122
.DEFINE W12SEL      $2123
.DEFINE W34SEL      $2124
.DEFINE WOBJSEL     $2125
.DEFINE TM          $212C   ; main screen layer enable
.DEFINE TS          $212D   ; sub screen layer enable
.DEFINE TMW         $212E
.DEFINE TSW         $212F
.DEFINE CGWSEL      $2130
.DEFINE CGADSUB     $2131
.DEFINE COLDATA     $2132
.DEFINE SETINI      $2133
.DEFINE RDNMI       $4210
.DEFINE TIMEUP      $4211
.DEFINE HVBJOY      $4212
.DEFINE WMDATA      $2180
.DEFINE WMADDL      $2181
.DEFINE WMADDM      $2182
.DEFINE WMADDH      $2183

.DEFINE NMITIMEN    $4200   ; D7 NMI enable, D5 V-IRQ, D4 H-IRQ, D0 auto-joy
.DEFINE WRIO        $4201
.DEFINE WRMPYA      $4202
.DEFINE WRMPYB      $4203
.DEFINE WRDIVL      $4204
.DEFINE WRDIVH      $4205
.DEFINE WRDIVB      $4206
.DEFINE HTIMEL      $4207
.DEFINE HTIMEH      $4208
.DEFINE VTIMEL      $4209
.DEFINE VTIMEH      $420A
.DEFINE MDMAEN      $420B   ; general purpose DMA channel enable = start
.DEFINE HDMAEN      $420C
.DEFINE MEMSEL      $420D   ; D0: 1 = 3.58 MHz access for banks $80-$FF
.DEFINE RDDIVL      $4214
.DEFINE RDDIVH      $4215
.DEFINE RDMPYL      $4216
.DEFINE RDMPYH      $4217
.DEFINE JOY1L       $4218
.DEFINE JOY1H       $4219
.DEFINE JOY2L       $421A
.DEFINE JOY2H       $421B

; DMA channel 0 parameter block ($43x0 for channel x)
.DEFINE DMAP0       $4300   ; D7 direction, D4 dec, D3 fixed, D2-D0 write pattern
.DEFINE BBAD0       $4301   ; B-bus address, low byte of $21xx
.DEFINE A1T0L       $4302
.DEFINE A1T0H       $4303
.DEFINE A1B0        $4304
.DEFINE DAS0L       $4305
.DEFINE DMAP1       $4310   ; channel 1: the HUD's OAM copy, nothing else
.DEFINE BBAD1       $4311
.DEFINE A1T1L       $4312
.DEFINE A1B1        $4314
.DEFINE DAS1L       $4315
.DEFINE DMAP2       $4320   ; channel 2: ItemsExchange's WRAM -> game pak RAM
.DEFINE BBAD2       $4321
.DEFINE A1T2L       $4322
.DEFINE A1B2        $4324
.DEFINE DAS2L       $4325
.DEFINE DAS0H       $4306
.DEFINE DASB0       $4307
.DEFINE A2A0L       $4308
.DEFINE A2A0H       $4309
.DEFINE NTRL0       $430A

; ---------------------------------------------------------------------------
; Super FX (GSU) interface, visible to the 65816 at $3000-$32FF in banks
; $00-$3F and $80-$BF. Book II Ch.2.
; ---------------------------------------------------------------------------

.DEFINE GSU_R0      ($3000 + GSU_REG_OFFSET)   ; default Sreg/Dreg
.DEFINE GSU_R1      ($3002 + GSU_REG_OFFSET)   ; PLOT x
.DEFINE GSU_R2      ($3004 + GSU_REG_OFFSET)   ; PLOT y
.DEFINE GSU_R3      ($3006 + GSU_REG_OFFSET)
.DEFINE GSU_R4      ($3008 + GSU_REG_OFFSET)
.DEFINE GSU_R5      ($300A + GSU_REG_OFFSET)
.DEFINE GSU_R6      ($300C + GSU_REG_OFFSET)
.DEFINE GSU_R7      ($300E + GSU_REG_OFFSET)
.DEFINE GSU_R8      ($3010 + GSU_REG_OFFSET)
.DEFINE GSU_R9      ($3012 + GSU_REG_OFFSET)
.DEFINE GSU_R10     ($3014 + GSU_REG_OFFSET)
.DEFINE GSU_R11     ($3016 + GSU_REG_OFFSET)
.DEFINE GSU_R12     ($3018 + GSU_REG_OFFSET)
.DEFINE GSU_R13     ($301A + GSU_REG_OFFSET)
.DEFINE GSU_R14     ($301C + GSU_REG_OFFSET)   ; ROM address pointer, write starts a ROM buffer load
.DEFINE GSU_R15     ($301E + GSU_REG_OFFSET)   ; PC. A CPU write here while idle STARTS the GSU.

.DEFINE GSU_SFR     ($3030 + GSU_REG_OFFSET)   ; flags (low) / status (high)
.DEFINE GSU_SFRH    ($3031 + GSU_REG_OFFSET)
.DEFINE GSU_BRAMR   ($3033 + GSU_REG_OFFSET)
.DEFINE GSU_PBR     ($3034 + GSU_REG_OFFSET)   ; program bank
.DEFINE GSU_ROMBR   ($3036 + GSU_REG_OFFSET)   ; ROM data bank (read only)
.DEFINE GSU_CFGR    ($3037 + GSU_REG_OFFSET)   ; D7 mask IRQ, D5 fast multiply
.DEFINE GSU_SCBR    ($3038 + GSU_REG_OFFSET)   ; char data base = $70:0000 + SCBR * $400
.DEFINE GSU_CLSR    ($3039 + GSU_REG_OFFSET)   ; D0: 0 = 10.7 MHz, 1 = 21.4 MHz
.DEFINE GSU_SCMR    ($303A + GSU_REG_OFFSET)   ; D5 HT1, D4 RON, D3 RAN, D2 HT0, D1 MD1, D0 MD0
.DEFINE GSU_VCR     ($303B + GSU_REG_OFFSET)
.DEFINE GSU_RAMBR   ($303C + GSU_REG_OFFSET)
.DEFINE GSU_CBR     ($303E + GSU_REG_OFFSET)
.DEFINE GSU_CACHE   ($3100 + GSU_REG_OFFSET)   ; 512 bytes of instruction cache RAM, $3100-$32FF

; SFR bits
.DEFINE SFR_Z       $02
.DEFINE SFR_CY      $04
.DEFINE SFR_S       $08
.DEFINE SFR_OV      $10
.DEFINE SFR_G       $20     ; go flag: 1 while the GSU is running
.DEFINE SFR_R       $40
.DEFINE SFRH_IRQ    $80     ; high byte bit 7, reading $3031 clears it

; SCMR field values
.DEFINE SCMR_HT_128 $00     ; HT1:HT0 = 00
.DEFINE SCMR_HT_160 $04     ; HT1:HT0 = 01
.DEFINE SCMR_HT_192 $20     ; HT1:HT0 = 10
.DEFINE SCMR_HT_OBJ $24     ; HT1:HT0 = 11
.DEFINE SCMR_MD_4   $00
.DEFINE SCMR_MD_16  $01
.DEFINE SCMR_MD_256 $03
.DEFINE SCMR_RON    $10
.DEFINE SCMR_RAN    $08

; ---------------------------------------------------------------------------
; Frame buffer configuration
;
; 256x160 in 16 colours, which is the GSU 160-dot BG bitmap mode. Character
; numbering in that mode is column major: CN = $14 * Xc + Yc (Book II 2-8).
;
;   chars      = 32 columns * 20 rows          = 640
;   char size  = 32 bytes (4bpp)
;   frame size = 640 * 32                      = $5000 bytes
;
; Game pak RAM map:
;
;   $70:0000-$70:01FF   GSU short-address window (LMS/SMS reach 0-510 only),
;                       which is where the renderer keeps its locals -- the
;                       GSU has 16 registers and a span rasterizer needs more
;   $70:0200-$70:0FFF   the renderer's face batch (gsu/mesh.gsu)
;   $70:1000-$70:5FFF   frame buffer 0
;   $70:6000-$70:AFFF   frame buffer 1
;   $70:B000-$71:FFFF   engine data (84 KB)
; ---------------------------------------------------------------------------

.IFDEF MODE7
; The Mode 7 frame: 128x64, a byte a pixel, in Mode 7's character format --
; 8x8 tiles of 64 chunky bytes, 128 tiles a buffer, numbered down the
; columns: tile (x >> 3) * 8 + (y >> 3). A Mode 7 map entry is one byte, so
; 256 tiles is the whole character set: buffer 0 is tiles 0-127, buffer 1
; tiles 128-255, and the flip is a vertical scroll of the map, whose rows
; 0-7 name one buffer and rows 8-15 the other. The matrix halves screen
; coordinates, so the 128x64 shows as 256x128, and eight 64x64 black
; sprites cover the 48 lines above and below it. See gsu/gsu.inc for the
; layout the GSU writes.
.DEFINE FRAME_WIDTH     128
.DEFINE FRAME_HEIGHT    64
.DEFINE FRAME_COLS      16
.DEFINE FRAME_ROWS      8
.DEFINE FRAME_SIZE      $2000
.ELSE
.DEFINE FRAME_WIDTH     256
.DEFINE FRAME_HEIGHT    160
.DEFINE FRAME_COLS      32
.DEFINE FRAME_ROWS      20
.DEFINE FRAME_SIZE      $5000
.ENDIF

.IFDEF FX3
.REDEFINE FRAME_WIDTH   216
.REDEFINE FRAME_HEIGHT  144
.REDEFINE FRAME_COLS    27
.REDEFINE FRAME_ROWS    18
.REDEFINE FRAME_SIZE    $7980           ; 486 packed 8bpp tiles
.DEFINE FX3_COL_BYTES   (18 * 64)
.DEFINE FX3_COL_STRIDE  (20 * 64)
.DEFINE FX3_DMA_COLS    7               ; the NMI's chunk: V-Blank and the top letterbox
.DEFINE FX3_IRQ_COLS    14              ; IrqFx3's: the bottom letterbox on
.DEFINE FX3_IRQ_LINE    185             ; the view is lines 41-184
.ENDIF

.DEFINE FB0_SCBR        $04             ; $70:1000 / $400
.DEFINE FB1_SCBR        $18             ; $70:6000 / $400
.DEFINE FB0_ADDR        $1000
.DEFINE FB1_ADDR        $6000
.DEFINE FB_BANK         $70
.IFDEF FX3
.REDEFINE FB0_SCBR      $40
.REDEFINE FB1_SCBR      $40
.REDEFINE FB0_ADDR      $0000
.REDEFINE FB1_ADDR      $0000
.REDEFINE FB_BANK       $71
.ENDIF

.IFDEF MODE7
; VRAM layout, in words. Mode 7 owns words $0000-$3FFF: the map in their
; low bytes, the character data in their high bytes -- character n at words
; n * 64 onward. The upload writes high bytes alone ($2119 with VMAIN's
; increment on the high byte), so a byte of frame is a word of VRAM.
.DEFINE VRAM_FB0        $0000           ; characters 0-127
.DEFINE VRAM_FB1        $2000           ; characters 128-255
.DEFINE VRAM_OBJ        $4000           ; the letterbox sprites' character
.DEFINE M7VOFS_FB0      $1FD0           ; -48, 13 bits: map row 0 on display line 48
.DEFINE M7VOFS_FB1      $0050           ; 80: map pixel row 64 on display line 48
.DEFINE OBJSEL_VALUE    $42             ; 8x8 and 64x64 sprites, characters at $4000
.DEFINE OBJ_LETTERBOX_Y0 $F0            ; 240: a 64-line sprite showing lines 0-47
.DEFINE OBJ_LETTERBOX_Y1 $B0            ; 176: lines 176-223
.ELSE
; VRAM layout, in words
.DEFINE VRAM_FB0        $0000           ; $0000-$27FF
.DEFINE VRAM_FB1        $3000           ; $3000-$57FF
.ENDIF
.DEFINE VRAM_BLANK0     $2800           ; blank char for buffer 0 (char $280)
.DEFINE VRAM_BLANK1     $5800           ; blank char for buffer 1 (char $280)
.DEFINE VRAM_TILEMAP    $6000           ; $6000-$63FF
.DEFINE BLANK_CHAR      $0280           ; char index that is zero in both bases

; The HUD's sprite characters (packer HUD.BIN, src/game.c olHudUpdate): OBJ
; base $6000 words, whose first 64 characters the BG1 tile map covers.
.DEFINE VRAM_HUD        $6400
.DEFINE OBJSEL_HUD      $03             ; 8x8/16x16, base $6000, gap 0

.DEFINE BG12NBA_FB0     $00             ; BG1 char base $0000 words
.DEFINE BG12NBA_FB1     $03             ; BG1 char base $3000 words
.DEFINE BG1SC_VALUE     $60             ; tilemap at $6000 words, 32x32
.IFDEF FX3
; Two tightly packed 486-tile images. The base cannot flip in $210B:
; buffer 1 begins at tile 486, so the final upload swaps the tile map.
.REDEFINE VRAM_FB1      $3CC0
.REDEFINE VRAM_TILEMAP  $7C00
.REDEFINE BLANK_CHAR    972
.REDEFINE BG1SC_VALUE   $7C
; After buffer 1 and its blank character: second name table index 154.
.REDEFINE VRAM_HUD      $79A0
.ENDIF

; Bytes pushed to VRAM per V-Blank. NTSC V-Blank is roughly 2.4 ms and DMA
; moves a byte every 8 master cycles (~2.68 MB/s), so ~6.4 KB is the ceiling.
; $1400 keeps margin for the NMI prologue and gives 4 chunks per frame.
.DEFINE DMA_CHUNK       $1400

; ---------------------------------------------------------------------------
; Shading.
;
; A 4bpp frame buffer has no bits left for a lightmap, so the shade term rides
; on the tile map instead: bits 10-12 of an entry choose one of eight
; background palettes, and CG-RAM holds eight progressively darker copies of
; the sixteen colours. Shade is therefore a per-character quantity.
;
; The GSU writes a byte per character into game pak RAM -- one map per frame
; buffer -- and the final V-Blank chunk DMAs it over the tile map's high
; bytes, in the same field that flips BG12NBA, so shade and pixels arrive
; together. 640 bytes on top of the last $1400 chunk, comfortably inside the
; V-Blank budget above.
;
; Bits 0-1 of each high byte are character number bits 8-9, which the frame
; needs (640 characters) and the GSU preserves; InitShadeMaps seeds them.
; ---------------------------------------------------------------------------

.DEFINE SNES_SHADES     8
.DEFINE PALETTE_BYTES   (SNES_SHADES * 16 * 2)

; The shade map is the 4bpp frame's: in the Mode 7 frame the shade is in the
; pixel and the host has nothing to upload for it. Matches gsu/gsu.inc.
.IFDEF SHADING
.IFNDEF MODE7
.IFNDEF FX3
.DEFINE SHADE_MAP       1
.ENDIF
.ENDIF
.ENDIF

.DEFINE SHADE_SIZE      (FRAME_COLS * FRAME_ROWS)
.DEFINE SHADE0_ADDR     $EA00           ; game pak RAM, must match gsu/gsu.inc
.DEFINE SHADE1_ADDR     $ED00
.DEFINE SHADE0_LONG     $70EA00         ; and the same, as the CPU sees it
.DEFINE SHADE1_LONG     $70ED00

; ---------------------------------------------------------------------------
; What the GSU stages for the host, in game pak RAM. Must match gsu/gsu.inc.
;
; The host cannot read game pak ROM while the GSU runs, so everything it will
; ever need to know about the level has to come through here -- written by the
; co-processor, collected in V-Blank while RAN is clear and the RAM bus is
; back on this side. The room the camera is in is the first of it.
; ---------------------------------------------------------------------------

.DEFINE GS_ROOM_LONG    $70F300
.DEFINE GS_FLOOR_LONG   $70F302
.DEFINE GS_CEIL_LONG    $70F304
.DEFINE GS_SX_LONG      $70F306
.DEFINE GS_SZ_LONG      $70F308
.DEFINE GS_FXNEG_LONG   $70F30A
.DEFINE GS_FXPOS_LONG   $70F30C
.DEFINE GS_FZNEG_LONG   $70F30E
.DEFINE GS_FZPOS_LONG   $70F310
.DEFINE GS_TRIG_LONG    $70F312
.DEFINE GS_LEVEL_READY_LONG $70F314
.DEFINE APU_PORT0_LONG  $002140   ; ports 0-3 as long addresses
.DEFINE ENT_READY_LONG  $70F62E   ; mesh.gsu's entity slot table is built
.DEFINE PROJ_MODE_LONG  $70F638   ; FX3: which reciprocals the projection table holds

; The channel that runs the other way: one byte per entity, host to GSU. See
; the comment on GS_ITEM in gsu/gsu.inc for why it exists.
.DEFINE GS_PROBE_LONG   $70F700
.DEFINE GS_WEAPON_LONG  $70F710  ; state word, then two 12-byte WeaponArm records
.DEFINE GS_LOOK_LONG    $70F72A  ; LaraLook: torso pitch/yaw, head pitch/yaw
.DEFINE GS_ITEM_LONG    $70F800
.DEFINE GS_ITEM_RECORDS 128         ; gsu.inc GS_ITEM_MAX, src/fixed.h OL_ITEM_MAX
.DEFINE GS_INV_LONG     $70DE40     ; FX3: the inventory ring (gsu.inc GS_INV)
.DEFINE GS_TEXT_LONG    $70EA00     ; FX3: its glyphs (gsu.inc GS_TEXT)
.DEFINE GS_RECT_LONG    $70EC10     ; FX3: its rectangles (gsu.inc GS_RECT)
.DEFINE GS_ROLL_LONG    $70F732     ; FX3: her lean (gsu.inc GS_ROLL)
.DEFINE GS_SHADOW_LONG  $70F734     ; FX3: the shadows (gsu.inc GS_SHADOW)
.DEFINE GS_FLASH_LONG   $70F640     ; FX3: the muzzle flashes (gsu.inc GS_FLASH)
.DEFINE GS_KIT_LONG     $70F648     ; FX3: her meshes, the shotgun's torso (gsu.inc GS_KIT)
.DEFINE BG_BUF_LONG     $703400     ; FX3: the ring's background (gsu.inc BG_BUF)
.DEFINE BG_STATE_LONG   $70F63A     ; FX3: 1 = BG_BUF is this ring's (gsu.inc BG_STATE)
.DEFINE GS_ITEM_BYTES   1536    ; 12 per entity, 128 of them

; The level's entity list, staged by the GSU and collected once. See the
; comment on GS_ITEMS in gsu/gsu.inc.
; Game pak RAM bank 1. The blob outgrew what was left of bank 0 the moment it
; carried an animation table, and the cartridge has 128 KB of which the
; renderer uses one bank.
.DEFINE GS_ITEMS_LONG   $710000
.DEFINE GS_ITEMS_MAX    65534

; ---------------------------------------------------------------------------
; WRAM layout. Banks $7E/$7F; $7E:0000-$1FFF is mirrored into $00-$3F, which
; is what lets the host code keep running while the GSU owns the ROM bus.
; ---------------------------------------------------------------------------

.DEFINE WRAM_VARS       $0000           ; $0000-$00FF direct page
.DEFINE WRAM_VECTORS    $0100           ; $0100-$010F GSU dummy vector landing
.DEFINE WRAM_CODE       $0110           ; $0110-$05FF resident host code
.DEFINE WRAM_CODE_MAX   $0F00
.DEFINE WRAM_STACK      $1FFF           ; grows down towards WRAM_STACK_FLOOR
.DEFINE WRAM_STACK_FLOOR $1C00          ; the fifth C image is below; the stack was
                                        ; measured 421 bytes deep over a long scene
                                        ; (2026-09-04), and the runner watches the floor

; Input bits, matching InputKey in src/fixed/common.h
.DEFINE IK_UP       $0001
.DEFINE IK_RIGHT    $0002
.DEFINE IK_DOWN     $0004
.DEFINE IK_LEFT     $0008
.DEFINE IK_A        $0010
.DEFINE IK_B        $0020
.DEFINE IK_L        $0040
.DEFINE IK_R        $0080
.DEFINE IK_START    $0100
.DEFINE IK_SELECT   $0200
.DEFINE IK_Y        $0400

; ---------------------------------------------------------------------------
; Assembler helpers
; ---------------------------------------------------------------------------

.MACRO SETA8
    sep #$20
.ENDM

.MACRO SETA16
    rep #$20
.ENDM

.MACRO SETXY8
    sep #$10
.ENDM

.MACRO SETXY16
    rep #$10
.ENDM

.MACRO SETAXY8
    sep #$30
.ENDM

.MACRO SETAXY16
    rep #$30
.ENDM

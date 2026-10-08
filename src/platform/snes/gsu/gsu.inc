; OpenLara - SNES / Super FX target
; Shared definitions for the GSU side.
;
; The GSU program is assembled to a flat 32 KB blob and INCBIN'd into ROM
; bank 2 by main.asm, so ROM offset $010000. The GSU sees game pak ROM
; linearly in banks $40-$5F, which puts this code at $41:0000 -- hence
; PBR = $41 and entry at $0000.
;
; LEVEL_BANK_BASE and TEXTURE_BANK_BASE come from build/texbank.inc, which
; tools/mkbanks.py writes when it places those blobs; neither side hard-codes
; the other's layout.

; Build configuration, written by the Makefile. Defines SHADING when the
; per-character shade pass is wanted; see the Shading section of the README.
.INCLUDE "config.inc"

; A cartridge with several levels assembles this program once per level,
; each with its own layout (texbank.inc); the second lies in the upper half
; of the program's bank, at GSU_ORIGIN $8000 (config.inc), PBR $41 for both.
.MEMORYMAP
    DEFAULTSLOT 0
    SLOTSIZE $8000
.IFDEF GSU_ORIGIN
    SLOT 0 GSU_ORIGIN
.ELSE
    SLOT 0 $0000
.ENDIF
.ENDME

.ROMBANKSIZE $8000
.ROMBANKS 1

; ---------------------------------------------------------------------------
; Frame buffer geometry. Must match snes.inc.
; ---------------------------------------------------------------------------

.DEFINE GSU_BANK        $41     ; PBR/ROMBR for this blob: ROM offset $010000

; 65536 / i for i in 0..255, one word each: DivTableData copied out of ROM
; once a frame (frame.gsu) so that the span setup and the edge pass read a
; reciprocal with one LDW instead of a ROM-buffer fetch behind a ROMB
; switch. $FE00-$FFFF, the page past the entity states; the host never reads it.
.DEFINE RECIP_TAB       $FE00

.IFDEF MODE7
; The Mode 7 frame: 128x64, a byte a pixel, laid out as the PPU's Mode 7
; character data wants it -- 8x8 tiles of 64 chunky bytes -- with the tiles
; running down the columns: tile (x >> 3) * 8 + (y >> 3), so pixel (x, y) is
; at
;     fb + (x >> 3) * 512 + y * 8 + (x & 7)
; and a line is eight bytes on from the one above it wherever it is. The
; host's map names the tiles in the same order. Written with STB rather
; than PLOT; the PPU shows it at twice the size. 8 KB, so the two buffers
; leave $3000-$5FFF and $8000-$AFFF to the tables below.
.DEFINE FRAME_WIDTH     128
.DEFINE FRAME_HEIGHT    64
.DEFINE FRAME_SIZE      $2000
; The per-frame tables the span setup adds: each pixel column's address on
; line 0 with the buffer folded in, one word a column; and 32768 / i, the
; projection's reciprocal, which halves every screen coordinate for free
; where the vertex loop would otherwise shift each one.
.DEFINE M7_COLTAB       $3000   ; 128 words
.DEFINE M7_PROJTAB      $3200   ; 256 words
.ELSE
.DEFINE FRAME_WIDTH     256
.DEFINE FRAME_HEIGHT    160
.DEFINE FRAME_SIZE      $5000
.DEFINE M7_PROJTAB      RECIP_TAB   ; the projection reads 65536 / dz
.ENDIF
.IFDEF FX3
.REDEFINE FRAME_WIDTH   216
.REDEFINE FRAME_HEIGHT  144
.REDEFINE FRAME_SIZE    $7980
; FX3 draws into bank $71, which leaves the GSU-2 frame buffers' $70:1000-
; $AFFF alone: the projection gets a table of its own there (FrameBody,
; mesh.gsu), 65536 / dz as RECIP_TAB for the game, twice that for the
; inventory. The camera's shear keeps reading the game's.
.REDEFINE M7_PROJTAB    $3200
.DEFINE SHEAR_TAB       RECIP_TAB
.ELSE
.DEFINE SHEAR_TAB       M7_PROJTAB
.ENDIF
.DEFINE FRAME_WORDS     (FRAME_SIZE / 2)

; Where the shade goes when SHADING is on: into the tile map's high bytes, a
; row per 8x8 character, for the 4bpp frame (SHADE_MAP); into the pixel's
; high nibble, per span, for the Mode 7 frame (SHADE_PIXEL). The edge
; accumulators are the same either way; what differs is the write.
.IFDEF SHADING
.IFDEF MODE7
.DEFINE SHADE_PIXEL     1
.ELSE
.IFDEF FX3
.DEFINE SHADE_PIXEL     1
.ELSE
.DEFINE SHADE_MAP       1
.ENDIF
.ENDIF
.ENDIF

.DEFINE FB0_ADDR        $1000
.DEFINE FB1_ADDR        $6000

; MERGE ($70) is a firmware command on FX3, not byte concatenation.
; Product callers already zeroed R7's low byte. These macros preserve R7/R8;
; none of the callers consumes MERGE's unusual Z/S/OV/CY flags.
.MACRO MERGE_PRODUCT
.IFDEF FX3
    from r8
    hib
    .DB ($10 + \1)             ; TO Rn (wla cannot substitute register tokens)
    or r7
.ELSE
    .DB ($10 + \1)
    merge
.ENDIF
.ENDM

; Texture address: R11 is scratch inside the pixel loop. FX3 span entries
; keep $FF00 in R6: masking V's fractional byte replaces HIB plus SWAP.
; Write R14 only after the complete address is ready, starting one fetch.
.MACRO MERGE_TEXEL
.IFDEF FX3
    from r7
    to r11
    and r6
    from r8
    hib
    to r14
    or r11
.ELSE
    to r14
    merge
.ENDIF
.ENDM

.MACRO FX3_COMMAND
    ibt r0, #\1
    merge
.ENDM


; ---------------------------------------------------------------------------
; Shade map -- the tile map's high bytes, one per 8x8 character.
;
; Four bits of frame buffer leaves nowhere to put a lightmap, so the shade
; term goes where the SNES keeps colour selection anyway: bits 10-12 of a tile
; map entry pick one of eight background palettes, and CG-RAM holds eight
; darkened copies of the sixteen colours. Texel per pixel, shade per character.
;
; The renderer writes only bits 2-4 of each high byte. Bits 0-1 are character
; number bits 8-9 -- the frame is 640 characters, so they are needed -- and the
; host seeds both maps with them at boot; the per-character write is a
; read-modify-write that preserves them. One map per frame buffer, because the
; GSU is already drawing the next frame while the finished one uploads.
; ---------------------------------------------------------------------------

.DEFINE SHADE0_ADDR     $EA00
.DEFINE SHADE1_ADDR     $ED00
.DEFINE SHADE_COLS      32
.DEFINE SHADE_ROWS      20
.DEFINE SHADE_SIZE      (SHADE_COLS * SHADE_ROWS)
.DEFINE SHADE_CN_MASK   $0003   ; character number bits 8-9, not ours to touch
.DEFINE SHADE_PAL_MASK  $001C   ; palette select, bits 10-12 of the entry

; ---------------------------------------------------------------------------
; POR / CMODE plot option bits (Book II table 2-8-1)
; ---------------------------------------------------------------------------

.DEFINE POR_TRANSPARENT $01     ; 1 = plot colour 0 as well
.DEFINE POR_DITHER      $02
.DEFINE POR_HIGH_NIBBLE $04
.DEFINE POR_FREEZE_HIGH $08
.DEFINE POR_OBJ         $10

; ---------------------------------------------------------------------------
; Register contract with the 65816 host.
;
; The host writes these while the GSU is idle, immediately before the write to
; R15 that starts it. They are the only channel the CPU has -- GSU registers
; other than SFR and VCR are unreadable and unwritable while it runs.
;
;   R3  frame buffer byte offset in game pak RAM ($1000 or $6000)
;   R6  room index to draw
;   R7  camera X      R8 yaw      R9 camera Z      R10 camera Y
;   R11 link register, callee return address
;
; Everything else is scratch.
; ---------------------------------------------------------------------------

; (R3 and R11 are referenced literally below; wla does not accept a .DEFINE
; where it expects a register token.)

; ---------------------------------------------------------------------------
; Game pak RAM scratch map, $70:0000-$70:01FF.
;
; This is the window LMS/SMS can reach with a one-byte operand, which is what
; makes it worth rationing. Each file owns a range and declares its own names
; inside it; the ranges are here so that growing one is a visible decision.
; Extending the rasterizer into $40 silently ate frame.gsu's saved link and
; sent a return into the middle of another routine's call sequence.
;
;   $00-$5F   raster.gsu   the two edge blocks, span state; $3A-$5F free
;   $60-$7F   mesh.gsu     near-plane clipper; $7E is BatchFlush's link
;   $80-$DF   mesh.gsu     matrix, transform, face, ordering and batch state
;   $E0-$EF   frame.gsu    frame entry scratch
;   $F0-$10F  raster.gsu   the span's shade; $F0-$FB free
;   $110-$13F mesh.gsu     shade, the current room's table entry, portals
;   $140-$1FF mesh.gsu     room list and clip, room find, sector and probe,
;                          instances, matrix stack, entities and models
;
; Free: $5A. ($EE and $10E are FR_CY and FR_TGTY since 2026-09-23.) Growing anything else means aliasing, which
; SI_* already does over R_CAM*/P_LINK because StageItems runs while those
; are idle -- do it on purpose, in the open, or not at all.
; ---------------------------------------------------------------------------

.DEFINE FR_LINK     $E0     ; MeshTest's return address
.DEFINE FR_CAMX     $E2     ; camera, handed over by the C running on the host
.DEFINE FR_ANGLE    $E4
.DEFINE FR_CAMZ     $E6
.DEFINE FR_CAMY     $E8
.DEFINE FR_SHADE    $EA     ; shade map matching the frame buffer in R3
.DEFINE FR_ROOM     $EC     ; which room to draw, also from the host
.DEFINE FR_CY       $EE     ; this frame's projection centre line: the pitch
.DEFINE FR_TGTY     $10E    ; the height the view looks at, from the host's R11

; The projection's centre. SCREEN_CY is the level view's; FrameStart shears
; it to FR_CY so the camera's target lands on it -- upstream's pitch, which
; this yaw-only view matrix does not have. See README "The pitch".
.IFDEF MODE7
.DEFINE SCREEN_CX   64
.DEFINE SCREEN_CY   32
.ELSE
.DEFINE SCREEN_CX   128
.DEFINE SCREEN_CY   80
.ENDIF
.IFDEF FX3
.REDEFINE SCREEN_CX 108
.REDEFINE SCREEN_CY 72
.ENDIF

; ---------------------------------------------------------------------------
; The clip rectangle, shared by the portal walk and the rasterizer.
;
; It was the whole frame, hard-coded, until rooms started arriving through
; portals: a room seen through a doorway must not paint outside it, and
; narrowing the rectangle at each portal is also what stops the traversal
; fanning out through the level -- a portal whose screen box misses the
; rectangle its own room was seen through leads nowhere visible.
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; What the GSU tells the host.
;
; The other direction of the register handover, and the answer to a question
; the port will keep asking: the level is in ROM, the host cannot read ROM
; while the GSU runs, so anything host-side game logic needs about the level
; has to be staged into game pak RAM -- which the host does get back, once a
; field, when it clears RAN to run its DMA. This is the first tenant.
; ---------------------------------------------------------------------------

.DEFINE GS_ROOM     $F300   ; the room the camera turned out to be in
.DEFINE GS_FLOOR    $F302   ; and the floor and ceiling under it, world units
.DEFINE GS_CEIL     $F304
.DEFINE GS_SX       $F306   ; the world corner of the sector it is standing in
.DEFINE GS_SZ       $F308
.DEFINE GS_FXNEG    $F30A   ; and the floors of the four sectors around it,
.DEFINE GS_FXPOS    $F30C   ; which is what tells the host where the walls are
.DEFINE GS_FZNEG    $F30E
.DEFINE GS_FZPOS    $F310
.DEFINE GS_TRIG     $F312   ; the trigger the camera's sector fires, or $FFFF
.DEFINE GS_LEVEL_READY $F314 ; host table is complete in game pak RAM bank 1

; ---------------------------------------------------------------------------
; And the one channel that runs the other way: a byte per entity, written by
; the host and read here. Everything else in game pak RAM is the GSU
; answering a question; this is the host giving an instruction.
;
; It carries an animation phase today. The point of it is not the phase --
; that could have stayed a global clock -- but that the host now owns a slot
; per entity, which is where per-entity state has to live: the host is the
; only side that can hold anything across frames and the only side that can
; decide anything, and it cannot read the level to find out what entities
; exist. The packer numbers them so both ends agree without either having to
; ask.
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; Game pak RAM above the frame buffers, in order. Kept here as one list
; because it was not, and $E000 looked free: it is inside OT_NEXT, and the
; renderer wrote the ordering table over the entity channel every frame.
;
;   $B000  TV_BASE     768 transformed vertices, 14 bytes each, ends $D9FF
;   $DA00  OT_NEAR     576 per-filed-face near flags, ends $DC3F
;   $DC40  ENT_TABLE   entity slot -> model address and bank, ends $DE3F
;   $E080  OT_NEXT     576 faces, 2 bytes            $E500  OT_OFF, the same
;   $EA00  SHADE0      640 bytes                     $ED00  SHADE1
;   $F000  RL_LIST     $F100 RL_VISITED   $F200 RL_RECT
;   $F300  GS_*        what the GSU stages for the host, ends $F315
;   $F400  MX_STACK    8 levels of 24 bytes, ends $F4BF
;   $F500  CLIP_A      clipper scratch               $F580  CLIP_B
;   $F600  model header scratch, ends $F60D
;   $F62A  R_INDEX, ENT_COUNT, ENT_READY, EB_LINK (mesh.gsu), ends $F631
;   $F610  weapon renderer scratch, ends $F629
;   $F700  GS_PROBE    where the host wants the floor looked at, 6 bytes
;   $F710  GS_WEAPON   state and two arm records, ends $F729
;   $F72A  GS_LOOK     torso/head pitch and yaw, ends $F731
;   $DE40  GS_INV      FX3: inventory ring, count + 13 x 32 bytes, ends $DFE1
;   $EA00  GS_TEXT     FX3 (no shade map): count, bank, 2 remaps + 64 x 8 bytes, ends $EC07
;   $EC10  GS_RECT     FX3: count + 24 x 6 bytes, ends $ECA1
;   $3400  BG_BUF      FX3 (bank $70, the GSU-2 frames'): the ring's grey background, ends $AD7F
;   $F63A  BG_STATE    FX3: BG_BUF holds this opening's capture
;   $F63C  TILT_LINK   FX3: ItemTilt's return
;   $F63E  MD_VIS      FX3: upstream's visibleMask, a bit a node (DrawModel)
;   $F732  GS_ROLL     FX3: her lean, the top byte of angle.z
;   $F734  GS_SHADOW   FX3: count, remap bank/address + 8 x 12 bytes, ends $F799
;   $F7A0  SH_CORN     FX3: a shadow's corners, then on screen, then scratch to $F7F3
;   $ED00  SH_XL/XR    FX3 (no shade map): each line's span, 2 x 144 words, ends $EF3F
;   $F632  INV_N, INV_P, INV_LINK (mesh.gsu FrameBody/InvRender)
;   $F640  GS_FLASH    FX3: per arm, the muzzle flash's timer and roll, ends $F643
;   $F644  FL_LINK, FL_ANG (mesh.gsu DrawMeshFlash), ends $F647
;   $F648  GS_KIT      FX3: a mesh byte per node (15), pad, the shotgun's
;                      torso X/Y/Z and the flash's distance down the hand, ends $F65F
;   $F660  NK_MASK, NK_LINK, SHC_LINK  FX3: the near plane (mesh.gsu), ends $F665
;   $F800  GS_ITEM     12 bytes per entity, ends $FDFF
;
; Free: $E980-$E9FF, $EF80-$EFFF, $F666-$F6FF,
; $F706-$F70F and $F7F4-$F7FF. Below the frame buffers, $0200-$0FFF is the face batch
; (BATCH_BASE in mesh.gsu).
; ---------------------------------------------------------------------------

; Twelve bytes an entity. It carried an animation and a frame; it carries
; where the entity *is* as well, because the alternative is that the renderer
; reads a position out of ROM and nothing in the world can move. The host owns
; entity state now and the renderer draws what it is told.
;
;   uint8  anim        which of the model's animations, or $FF to skip it
;   uint8  frame       which frame of that animation
;   int16  x, y, z     world, so the renderer subtracts the room's origin
;   uint16 angle       full TR angle; the renderer takes the top byte
;   uint8  room        the room it is in now; DrawRoomItems draws it there
;   uint8  pitch       top byte of angle.x, turned after the yaw (hers only)
; Where to probe. The view and the point the game stands on stopped being the
; same thing the moment the camera followed Lara rather than being her: the
; floor she walks on is under *her*, and the room the renderer starts from is
; the one the camera is in. Both are needed, so both are handed over.
.DEFINE GS_PROBE    $F700
.DEFINE GS_WEAPON   $F710      ; weapon state, then right/left WeaponArm
.DEFINE GS_LOOK     $F72A      ; LaraLook: torso X/Y, head X/Y

.DEFINE GS_INV      $DE40      ; FX3: the inventory ring (inventory_draw.c)
.DEFINE GS_TEXT     $EA00      ; FX3: the glyphs (inventory_text.c, TextRender; drawFPS in game frames)
.DEFINE GS_RECT     $EC10      ; FX3: the options' rectangles (RectRender)
.DEFINE BG_BUF      $3400      ; FX3: the inventory's grey background, 216 x 144
.DEFINE BG_STATE    $F63A      ; FX3: 1 once BgCapture has filled it this opening
.DEFINE TILT_LINK   $F63C      ; FX3: ItemTilt's return
.DEFINE MD_VIS      $F63E      ; FX3: a ring item's visibleMask, shifted a node at a time
.DEFINE GS_ROLL     $F732      ; FX3: her lean (angle.z's top byte), after the pitch
.DEFINE GS_SHADOW   $F734      ; FX3: the shadows (Shadow_publish, ShadowRender)
.DEFINE GS_FLASH    $F640      ; FX3: the muzzle flashes (weapon.c, DrawMeshFlash)
.DEFINE GS_KIT      $F648      ; FX3: her meshes and the shotgun's torso (Weapon_publish)
.DEFINE NK_MASK     $F660      ; FX3: TM_Near's side bits for this room (RoomClipFx3)
.DEFINE NK_LINK     $F662      ; FX3: RoomClipFx3's return
.DEFINE SHC_LINK    $F664      ; FX3: ShadowCentre's return
.DEFINE KIT_MESHES  23         ; GS_KIT's indices: 0 (none) and KitMeshes' 22
.DEFINE FL_LINK     $F644
.DEFINE FL_ANG      $F646
.DEFINE SH_CORN     $F7A0      ; 8 x (x, z), in the entity's frame
.DEFINE SH_PTS      $F7C0      ; 8 x (sx, sy)
.DEFINE SH_LINK     $F7E0
.DEFINE SH_SRC      $F7E2
.DEFINE SH_DST      $F7E4
.DEFINE SH_N        $F7E6
.DEFINE SH_YMIN     $F7E8
.DEFINE SH_YMAX     $F7EA
.DEFINE SH_Y0       $F7EC
.DEFINE SH_OFFX     $F7EE      ; the renderer's projection of the centre, less ours
.DEFINE SH_OFFY     $F7F0
.DEFINE SH_REC      $F7F2      ; the shadow being drawn, in GS_SHADOW
.DEFINE SH_XL       $ED00      ; per line, the span's left
.DEFINE SH_XR       $EE20      ; and right
.DEFINE INV_N       $F632      ; InvRender's items left
.DEFINE INV_P       $F634      ; and the next one
.DEFINE INV_LINK    $F636      ; FrameBody's return
.DEFINE PROJ_MODE   $F638      ; which reciprocals M7_PROJTAB holds (FX3)
.DEFINE PROJ_GAME   $A501
.DEFINE PROJ_INV    $A502
.DEFINE GS_ITEM     $F800
.DEFINE GS_ITEM_SIZE 12
.DEFINE GS_ITEM_MAX 128

; And the level's entity list, staged here for the host: a flat list of what
; exists, in the shape game logic asks about rather than the shape the
; renderer draws from. The packer builds it; the GSU only copies it, which is
; the whole point -- the host cannot read game pak ROM at all, so a straight
; copy through here is the only way anything about the level reaches it.
;
; Staged once. The 65816 explicitly clears GS_LEVEL_READY at boot and the GSU
; sets it only after the complete copy, so fixed-rate host logic can never
; parse a partial table while this first render is still in flight.
.DEFINE GS_ITEMS    $0000   ; in game pak RAM bank 1, which nothing else uses
.DEFINE GS_ITEMS_BANK 1
.DEFINE GS_SIZE     18

; A sector with no floor -- a wall, or a probe outside every room grid.
.DEFINE SECTOR_NONE $7FFF

; How far a probe reaches: one sector, so each one lands in the middle of a
; neighbour rather than near its edge.
.DEFINE PROBE_STEP  1024

.DEFINE MX_SP       $1A4    ; matrix stack depth; frame.gsu zeroes it

.DEFINE V_CLIPX0    $106
.DEFINE V_CLIPX1    $108
.DEFINE V_CLIPY0    $10A
.DEFINE V_CLIPY1    $10C

; ---------------------------------------------------------------------------
; Toolchain trap: wla-superfx assembles `blt` to $07 and `bge` to $06, the
; reverse of the SNES Development Manual, where $06 is BLT ("S xor OV = 1").
; Every other branch mnemonic checks out. Nothing in this port uses either
; one; signed comparisons subtract and test the sign with BPL/BMI.
; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; Call / return.
;
; The GSU has no stack and no call instruction. The convention in the manual
; is a return address in R11 and "MOVE R15,R11" to come back. Both macros
; leave a NOP in the delay slot: every branch on this chip executes the next
; instruction regardless, and a multi-byte instruction there gets its first
; byte pipelined and the rest of it eaten from the branch target.
; ---------------------------------------------------------------------------

.MACRO GCALL
    iwt r11, #_gcall_ret\@
    bra \1
    nop
_gcall_ret\@:
.ENDM

; ---------------------------------------------------------------------------
; Signed branches. The manual's instruction pages (9.17 BGE, 9.20 BLT)
; give BLT = $06 and BGE = $07; the hardware has them the other way round:
; $06 is BGE (S xor OV = 0) and $07 is BLT (S xor OV = 1). Star Fox's own
; outcode routine only works that way, and bsnes, snes9x, Mesen, fullsnes
; and wla-superfx all agree. Until 2026-09-26 this port "corrected" the
; assembler to the manual -- and TypeSNES and tools/gsusim.py decoded the
; manual's way too, so all three agreed on code that would have branched
; the wrong way on a cartridge. The afternoon once lost to MatrixPush was
; that swap, masked by swapping the port as well.
;
; The macros stay, so the four sites read as what they mean.
; ---------------------------------------------------------------------------
.MACRO GBLT
    blt \1
.ENDM

.MACRO GBGE
    bge \1
.ENDM

.MACRO GRET
    move r15, r11
    nop
.ENDM

; ---------------------------------------------------------------------------
; Far call / far jump. GSU branch displacements are a signed byte, so anything
; past 127 bytes has to go through JMP Rn. R13 is the trampoline register:
; every callee here either saves the link register into it or reloads it as a
; loop address, so nothing expects it to survive a call anyway.
; ---------------------------------------------------------------------------

.MACRO GFCALL
    iwt r11, #_gfcall_ret\@
    iwt r13, #\1
    jmp r13
    nop
_gfcall_ret\@:
.ENDM

.MACRO GFJMP
    iwt r13, #\1
    jmp r13
    nop
.ENDM

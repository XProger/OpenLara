# OpenLara - SNES / Super FX target
#
# Three assemblers, because a 65816 object, a Super FX object and an SPC700
# object cannot be linked together (wlalink rejects mixed memory
# architectures). The GSU side and the sound CPU's driver are each built into
# a flat binary and INCBIN'd by the host side.
#
#   brew install wla-dx      # wla-65816, wla-superfx, wla-spc700, wlalink
#   python3                  # header checksum
#
# Targets:
#   make            build build/openlara.sfc
#   make fx3        build build_fx3/openlara.sfc (new 216x144 FX3 pipeline)
#   make run        build and launch it in an emulator
#   make clean

TARGET   := openlara
BUILD    := build

# Shading is optional. At 256x160 the 4bpp path can select a palette only per
# 8x8 character, which is too coarse to be the default look. SHADING=0 leaves
# every pixel on CG-RAM row 0; SHADING=1 enables the tile-map shade there, or
# the per-span high-nibble shade in the optional Mode 7 frame.
#
#   make BUILD=build_shaded SHADING=1 sim
#
# The setting is baked into build/config.inc, which every object depends on, so
# changing it rebuilds what it needs to and nothing else.
SHADING  ?= 0

# The frame: 256x160 in 4bpp GSU bitmap format, written with PLOT. The
# lower-resolution 128x64 chunky frame remains available through MODE7=1;
# it is shown at twice the size by the PPU's Mode 7 matrix. Build variants in
# separate directories because they share nothing in build/:
#
#   make BUILD=build_m7 MODE7=1 sim
#
# With SHADING=1 the Mode 7 frame carries the shade in each pixel's high
# nibble, per span (SHADE_PIXEL in gsu/gsu.inc); the 4bpp frame carries it
# in the tile map, per character (SHADE_MAP).
MODE7    ?= 0
FX3      ?= 0
# CPU-only C motion/animation/bounds, collision, logic PRNG, wolf/bear and
# navigation reference (not playable).
C_MOTION ?= 0
# Native FX3 overlaps packed WRAM uploads when logic has headroom, and
# prepares direct-buffer jobs for launch after the last planar DMA read.
# FX3_STAGE=0 supports levels whose mutable sectors exceed 32 KiB.
# The C oracle retains its original pipeline.
FX3_STAGE ?= $(if $(and $(filter-out 0,$(FX3)),$(filter 0,$(C_MOTION))),1,0)
ifeq ($(FX3),0)
ifneq ($(FX3_STAGE),0)
$(error FX3_STAGE=1 requires FX3=1)
endif
endif
ifneq ($(FX3),0)
ifneq ($(MODE7),0)
$(error FX3=1 uses its own 216x144 Mode 3 framebuffer; MODE7 must be 0)
endif
endif
GSUSIM_FLAGS := $(if $(filter-out 0,$(FX3)),--fx3,$(if $(filter-out 0,$(MODE7)),--mode7,))

PVSNESLIB ?= $(HOME)/dev/pvsneslib

# The GSU side uses whatever wla-dx is on PATH; the 65816 side uses the one
# PVSnesLib ships, because 816-tcc emits {WLA_FILENAME} substitutions that
# only its patched build (v10.7a) understands. The two never link against each
# other -- the GSU program becomes a raw blob that the 65816 side INCBINs.
WLASFX     := wla-superfx
WLASPC     := wla-spc700
WLALINK_FX := wlalink
WLA65816   := $(PVSNESLIB)/devkitsnes/bin/wla-65816
WLALINK    := $(PVSNESLIB)/devkitsnes/bin/wlalink
PYTHON     := python3

# -d -s -x is what PVSnesLib passes; among other things it enables the
# {WLA_FILENAME} substitution 816-tcc emits in its section names.
ASFLAGS    := -d -s -x

EMU      ?= /Applications/bsnes+.app/Contents/MacOS/bsnes+

# TypeSNES is used as a headless oracle: it boots the ROM for N fields and
# compares the uploaded VRAM with the standalone GSU interpreter. Point at
# your checkout; `make check` fails clearly if it is unavailable.
TYPESNES ?= $(HOME)/dev/typesnes
# Enough fields for at least one complete GSU frame plus its four-field
# upload, with margin: a portal-traversed frame is well under one per field
# and the check compares an uploaded frame, not a partial one.
FRAMES   ?= 600
# Fields the walking test spends holding Up. Zero skips it.
WALK     ?= 900
# Run only the tests whose name matches: `make check TESTS=walking`.
TESTS    ?=

DATA     := $(BUILD)/data
TRDATA   ?= $(CURDIR)/../../../tomb_raider_dos/TOMBENG/data
LEVEL    ?= LEVEL1

# Several levels in one FX3 cartridge, as the GBA's alpha carries LEVEL1 and
# LEVEL2: `make BUILD=build_fx3_multi FX3=1 LEVELS="LEVEL1 LEVEL2"`. A
# power-on plays the first; the passport and a level's end boot the others
# (main.asm SelectLevel). Each level is packed into $(BUILD)/levels/<name>/
# and gets its own GSU program (tools/mkbanks.py --fx3-multi); the CPU
# program is one. BOOT_LEVEL picks the level `check` and `sim` exercise.
LEVELS     ?=
BOOT_LEVEL ?= 0
ifneq ($(strip $(LEVELS)),)
ifeq ($(FX3),0)
$(error LEVELS needs FX3=1: a GSU-2 cartridge's 2 MB holds one level)
endif
LEVEL := $(word $(shell expr $(BOOT_LEVEL) + 1),$(LEVELS))
ifeq ($(LEVEL),)
$(error BOOT_LEVEL=$(BOOT_LEVEL) is not one of LEVELS="$(LEVELS)")
endif
DATA  := $(BUILD)/levels/$(LEVEL)/data
MULTI_GSU := $(foreach i,$(wordlist 2,$(words $(LEVELS)),$(shell seq 1 $(words $(LEVELS)))),$(BUILD)/gsu_$(shell expr $(i) - 1)/gsu.bin)
endif

GSU_SRC  := gsu/frame.gsu
GSU_INC  := gsu/gsu.inc gsu/raster.gsu gsu/mesh.gsu gsu/weapon.gsu
CPU_SRC  := main.asm
CPU_INC  := snes.inc hdr.asm

# --- C side -----------------------------------------------------------------
# 816-tcc from PVSnesLib, plus its peephole pass. Set PVSNESLIB to your
# checkout. The compiled image is retargeted into WRAM by tools/tccfix.py --
# see that file for why it cannot simply live in ROM.
TCC       := $(PVSNESLIB)/compiler/tcc/816-tcc
TCCOPT    := $(PVSNESLIB)/devkitsnes/tools/816-opt

# The compiled engine is resident in WRAM bank $7F, which is 64 KB, and it
# gets there from ROM -- where a LoROM bank is 32 KB. So it takes two banks
# and two DMAs, one into each half, and a translation unit is assigned to a
# half here. That is why the assignment is per file rather than global: wla
# packs sections within a bank and will not span one, so a unit has to fit
# the half it is given. Splitting a unit that outgrows 32 KB means splitting
# the file.
#
# The ceiling is now the WRAM bank, not the ROM bank: 64 KB of resident code
# and constants, with .bss separately in $7E.
# src/room.asm is the other half of room.c, assembled rather than compiled;
# it is listed with the runtime below.
C_SRCS    := src/game.c src/item.c src/level.c src/room.c src/nav.c \
             src/enemy.c src/object.c src/common.c src/camera.c src/math.c \
             src/lara.c src/collision.c src/use.c src/block.c src/weapon.c \
             src/mathcheck.c src/weapon_anim.c src/bat.c src/room_point.c src/level_coords.c \
             src/wolf.c src/enemy_move.c src/bear.c src/sound.c src/game_init.c src/camera_cold.c
ifneq ($(FX3),0)
C_SRCS += src/nav_fx3.c src/enemy_nav.c src/enemy_corpse.c src/raptor.c src/contact_fx3.c src/trap.c src/camera_fix.c \
          src/inventory.c src/inventory_draw.c src/inventory_text.c src/save.c src/fx.c src/weapon_kit.c \
          src/settings.c src/door.c src/lion.c src/platform_floor.c
C_BANK_lion := 60
C_BANK_platform_floor := 60
C_BANK_contact_fx3 := 4
# a shut door's sectors (door.c, 2026-10-06): $86 in both ROMs
C_BANK_door := 60
C_BANK_settings := 62
C_BANK_fx := 60
C_BANK_trap := 62
C_BANK_camera_fix := 62
C_BANK_inventory := 60
C_BANK_inventory_draw := 63
C_BANK_inventory_text := 60
C_BANK_nav_fx3 := 61
C_BANK_enemy_nav := 62
C_BANK_enemy_corpse := 61
C_BANK_raptor := 62
C_BANK_save := 61
C_BANK_weapon_kit := 62
C_BSS_nav_fx3 := 9
endif
C_OBJS    := $(patsubst src/%.c,$(BUILD)/c_%.o,$(C_SRCS))
# Retain the compiler intermediates. GNU make 3.81 can leave an implicit C
# prerequisite permanently "being made" when a shared generated include is
# recovered during a parallel incremental build and these files are absent.
.SECONDARY: $(C_OBJS:.o=.raw) $(C_OBJS:.o=.asm)
C_WRAMBANK := 0x7F

# Which half of which WRAM bank each unit lands in:
#
#   ROM bank 4    -> $7F:0000    ROM bank 5    -> $7F:8000
#   ROM bank $3F  -> $7E:8000    (C_WRAM_x := 0x7E; .bss is confined to
#                                 $7E:2000-$7FFF by SLOT 2 in hdr.asm)
#
# lara.c moved from $3F to 5 on 2026-09-02, when hang and climb left the
# bank it shared with collision.c 4 KB from full; the camera clipper then
# filled bank 5, and nav.c and enemy.c went to $3F beside collision.c. The
# water slice (2026-09-04) doubled lara.c and filled bank 5 again, and
# object.c and common.c -- 4.5 KB between them -- went to bank 4 to make
# room for it; math.c's 4 KB would not have fitted there beside them. The
# same evening two more regions were found in $7E: 6 KB above the .bss
# ($6800-$7FFF, ROM bank $3E, slot 7; 10 KB from $5800 since the nav
# flood's second queue went) and 3 KB under the stack ($1000-
# $1BFF, ROM bank $3D, slot 8; the stack was measured 421 bytes deep and
# keeps $1C00-$1FFF). enemy.c went to the first -- it is the unit that will
# grow most, and the region a per-level creature page would be paged into
# -- and common.c to the second.
#
#   ROM bank $3E  -> $7E:5800    ROM bank $3D  -> $7E:1000
#
# The interaction slice (2026-09-04, later): object.c to $3F, use.c beside
# enemy.c in the enlarged $3E region, block.c to bank 5.
# The aiming slice swaps level.c (3613 bytes) and block.c (3100 bytes),
# recovering 513 bytes in bank 4 for the rotated-box acquisition code.
#
# A unit has to fit the half it is given, because wla packs sections within a
# bank and will not span one; the link fails if it does not. `make` prints
# what is used and free in each after every link -- see tools/budget.py.
C_BANK_game  := 4
C_SLOT_game  := 4
C_BANK_weapon := 4
C_SLOT_weapon := 4
C_BANK_weapon_anim := 4
C_SLOT_weapon_anim := 4
C_BANK_item  := 4
C_SLOT_item  := 4
C_BANK_level := 4
C_SLOT_level := 4
C_BANK_game_init := 4
C_SLOT_game_init := 4
C_BANK_camera_cold := 4
C_SLOT_camera_cold := 4
C_BANK_level_coords := 4
C_SLOT_level_coords := 4
C_BANK_bat   := 5
C_SLOT_bat   := 6
C_BANK_wolf  := 4
C_SLOT_wolf  := 4
C_BANK_enemy_move := 63
C_SLOT_enemy_move := 6
C_WRAM_enemy_move := 0x7E
C_BANK_room_point := 62
C_SLOT_room_point := 7
C_WRAM_room_point := 0x7E
C_BANK_room  := 4
C_SLOT_room  := 4
C_BANK_nav   := 63
C_SLOT_nav   := 6
C_WRAM_nav   := 0x7E
C_BANK_enemy := 62
C_SLOT_enemy := 7
C_WRAM_enemy := 0x7E
C_BANK_bear := 62
C_SLOT_bear := 7
C_WRAM_bear := 0x7E
C_BANK_lara  := 5
C_SLOT_lara  := 6
C_BANK_collision := 63
C_SLOT_collision := 6
C_WRAM_collision := 0x7E
C_BANK_object := 63
C_SLOT_object := 6
C_WRAM_object := 0x7E
C_BANK_camera := 5
C_SLOT_camera := 6
C_BANK_common := 61
C_SLOT_common := 8
C_BANK_use   := 62
C_SLOT_use   := 7
C_WRAM_use   := 0x7E
C_BANK_block := 4
C_SLOT_block := 4
C_WRAM_common := 0x7E
# The sound effects (2026-09-07): upstream's soundPlay over the tables the
# boot copied in, and the four-port protocol to src/spc.asm.
C_BANK_sound := 63
C_SLOT_sound := 6
C_WRAM_sound := 0x7E
# The host assembly ends below $0600 in every renderer variant. Extending
# slot 8 down from $1000 recovers 2560 bytes without reducing the stack.
# The trig tables and PRNG use that space, freeing bank 5 for Lara states.
# Native trig shares this image with its tables; Room_pointOpen occupies the
# remaining small region above .bss, leaving a contiguous bank-5 slot for bats.
C_BANK_math  := 61
C_SLOT_math  := 8
C_WRAM_math := 0x7E
# Boot only, before StartGSU hands ROM to the coprocessor. Keep this cold
# diagnostic in ROM instead of consuming 1443 bytes of resident WRAM.
C_BANK_mathcheck := 60
C_SLOT_mathcheck := 0
C_WRAM_mathcheck := 0xBC

# FX3 specification §§1,6: the CPU can fetch FastROM while the FX uses ROM
# and SRAM. All engine code/rodata can stay in ROM, not just banks 4/5.
# Keep globals in $7E and only mutable sectors in $7F. Per-creature Nav
# uses former code space via BSS slot 9; other data remains in slot 2.
ifeq ($(FX3),0)
# Load tick only: UpdateDemo takes the ROM bus (RON = 0, the GSU parks at
# its next ROM fetch) around the tick that calls olLevelInit, so its 3.5 KB
# stay in ROM beside olMathCheck instead of in resident WRAM.
C_BANK_level := 60
C_SLOT_level := 0
C_WRAM_level := 0xBC
# Boot only, before StartGSU: the power-on state.
C_BANK_game_init := 60
C_SLOT_game_init := 0
C_WRAM_game_init := 0xBC
# Camera_init (boot) and Camera_snap (the load tick).
C_BANK_camera_cold := 60
C_SLOT_camera_cold := 0
C_WRAM_camera_cold := 0xBC
endif

ifneq ($(FX3),0)
# Native sign extension trades a few ROM bytes for much shorter CPU work.
# These C units use FX3's ample ROM bank $BD, not GSU-2's resident slots.
C_BANK_item := 61
C_BANK_bat := 61
# all four guns (weapon_kit.c in $88): weapon.c's FX3 growth takes block.c's
# place in $84, and camera_fix.c leaves $88 for $84
C_BANK_block := 5
C_BANK_camera_fix := 4
# Cold coordinate lookup frees space for the trigger additions in the
# two-level cartridge's crowded $84 bank. It needs no resident WRAM on FX3.
C_BANK_level_coords := 63
FX3_ROM_UNITS := $(patsubst src/%.c,%,$(C_SRCS))
$(foreach unit,$(FX3_ROM_UNITS),$(eval C_SLOT_$(unit) := 0))
$(foreach unit,$(FX3_ROM_UNITS),$(eval C_WRAM_$(unit) := $(shell expr 128 + $(C_BANK_$(unit)))))
CFLAGS_VARIANT := -DOL_FX3
endif
ifneq ($(C_MOTION),0)
CFLAGS_VARIANT += -DOL_C_MOTION
# Oracle only: the C implementations must not consume the space recovered by
# their native replacements. Bank $3C is outside the packed level/texture
# range. The CPU fixture calls this image with the GSU stopped; this reference
# ROM is not a playable GSU-2 build (its item code reads cartridge ROM).
C_BANK_item := 60
C_SLOT_item := 0
C_WRAM_item := 0xBC
C_BANK_wolf := 60
C_SLOT_wolf := 0
C_WRAM_wolf := 0xBC
C_BANK_bear := 60
C_SLOT_bear := 0
C_WRAM_bear := 0xBC
C_BANK_sound := 60
C_SLOT_sound := 0
C_WRAM_sound := 0xBC
# The C units fill $3C here, so the load/boot-only units move down a bank.
# $3B is past every packed level's data (LEVEL1 ends at $2F, LEVEL2 at $33).
ifeq ($(FX3),0)
C_BANK_level := 59
C_WRAM_level := 0xBB
C_BANK_game_init := 59
C_WRAM_game_init := 0xBB
C_BANK_camera_cold := 59
C_WRAM_camera_cold := 0xBB
endif
# FX3's inventory lives in $BC beside the load code; here the C references
# have $BC, so it goes down to $BB, past the packed data as above.
ifneq ($(FX3),0)
C_BANK_inventory := 59
C_WRAM_inventory := 0xBB
C_BANK_inventory_draw := 59
C_WRAM_inventory_draw := 0xBB
C_BANK_inventory_text := 59
C_WRAM_inventory_text := 0xBB
C_BANK_fx := 59
C_WRAM_fx := 0xBB
# settings.c ($88 in play) has the room the C references left in $87
C_BANK_settings := 61
# The four guns' weapon_kit.c goes in $87, which the native units this
# oracle leaves out have freed; block.c and camera_fix.c keep the banks they
# had before FX3's weapon_kit.c moved them.
C_BANK_weapon_kit := 61
# Corpse dispatch adds a C call; keep this small reference unit beside the
# corpse helper instead of filling the already tight species bank $88.
C_BANK_enemy := 61
C_BANK_block := 4
C_BANK_camera_fix := 62
# C species/navigation references now fill $88; its small point-open
# helper fits beside the arithmetic/animation references in $86.
C_BANK_room_point := 60
endif
endif

# FX3 code banks. The unit assignments above number GSU-2's banks; FX3 has no
# WRAM images, and its ROM wants the GSU's whole window for level data -- a
# second level beside the first needs 42 of the 43 banks the code leaves --
# so its $BC-$BF code moves down beside $84/$85: wla 60-63 become 6-9
# ($86-$89, GSU $43-$44) and the data runs from GSU $45 to $6F unbroken.
# C_MOTION's $BB (59) stays: its oracle ROM holds one level.
ifneq ($(FX3),0)
$(foreach unit,$(FX3_ROM_UNITS),$(eval C_BANK_$(unit) := $(if $(filter 60,$(C_BANK_$(unit))),6,$(if $(filter 61,$(C_BANK_$(unit))),7,$(if $(filter 62,$(C_BANK_$(unit))),8,$(if $(filter 63,$(C_BANK_$(unit))),9,$(C_BANK_$(unit))))))))
$(foreach unit,$(FX3_ROM_UNITS),$(eval C_WRAM_$(unit) := $(shell expr 128 + $(C_BANK_$(unit)))))
endif

ROM      := $(BUILD)/$(TARGET).sfc
GSU_BIN  := $(BUILD)/gsu.bin

.PHONY: all fx3 run check paritycheck motioncheck keycheck baseline sim profile fixedcheck matcheck matrixcheck animcheck weaponcheck nearcheck clean FORCE
.DELETE_ON_ERROR:

all: $(ROM)

fx3:
	$(MAKE) BUILD=build_fx3 FX3=1 MODE7=0 all

$(BUILD):
	@mkdir -p $(BUILD)

# --- build configuration ----------------------------------------------------
# Emitted rather than passed with -D so that the assemblers' own dependency
# tracking sees it: every object includes this file, so flipping SHADING
# rebuilds them. Rewritten only when the contents actually change, so an
# unchanged setting does not force a rebuild.
#
# A changed setting costs a second, deliberately. GNU make 3.81, which is what
# macOS ships, compares mtimes to the whole second, and a build of this size
# fits inside one -- so a config file written in the same second as the
# objects that depend on it reads as up to date, and `make SHADING=0` after
# `make SHADING=1` left the shading ROM in place claiming to be the other one.
# Sleeping before the touch puts config.inc strictly ahead of everything
# already built, which is the only thing make will believe.
#
# Deleting the stale objects instead does not work: make stats its targets
# once, at the start, so a file removed by a recipe partway through is still
# remembered as present and up to date.

$(BUILD)/config.inc: FORCE | $(BUILD)
	@{ echo "; generated by the Makefile -- do not edit"; \
	   echo ".IFNDEF OL_CONFIG"; echo ".DEFINE OL_CONFIG 1"; \
	   if [ "$(SHADING)" != "0" ]; then echo ".DEFINE SHADING 1"; fi; \
	   if [ "$(MODE7)" != "0" ]; then echo ".DEFINE MODE7 1"; fi; \
	   if [ "$(FX3)" != "0" ]; then echo ".DEFINE FX3 1"; fi; \
	   if [ "$(FX3_STAGE)" != "0" ]; then echo ".DEFINE FX3_STAGE 1"; fi; \
	   if [ "$(C_MOTION)" != "0" ]; then echo ".DEFINE C_MOTION 1"; fi; \
	   $(foreach unit,$(patsubst src/%.c,%,$(C_SRCS)),echo "; C_LAYOUT $(unit) $(C_BANK_$(unit)) $(or $(C_WRAM_$(unit)),$(C_WRAMBANK)) $(C_SLOT_$(unit))";) \
	   echo ".ENDIF"; \
	 } > $@.tmp
	@cmp -s $@.tmp $@ 2>/dev/null || { mv $@.tmp $@; sleep 1; touch $@; \
	   echo "config: SHADING=$(SHADING) MODE7=$(MODE7) FX3=$(FX3) FX3_STAGE=$(FX3_STAGE) C_MOTION=$(C_MOTION)"; }
	@rm -f $@.tmp

# --- GSU side ---------------------------------------------------------------

$(BUILD)/divtable.bin: tools/mkdivtable.py | $(BUILD)
	$(PYTHON) tools/mkdivtable.py $@

# --- level data -------------------------------------------------------------
# The packer reads the original PC release straight out of TRDATA. Nothing in
# the repository ships game data.

$(BUILD)/packer: packer/main.cpp packer/out_SNES.h packer/sound_SNES.h | $(BUILD)
	$(CXX) -O1 -std=c++11 -w -I../gba/packer -I../gba/packer/libimagequant \
	  packer/main.cpp -o $@

# A stamp serializes each multi-output generator on GNU make 3.81 as well as
# newer make. Grouped targets (&:) require 4.3 and became a literal '&' target
# with the macOS system make, allowing two packers to write the same files.
# A missing output forces regeneration of the whole group before any consumer
# can read it. The stamp remains a real file so dependents see its completed
# generation time, including after a failed generator is retried.
PACKED_META := $(addprefix $(DATA)/,LEVEL.BIN AIM.json NAV.json WEAPON.json TEXTURES.json PALETTE.BIN HUD.BIN HOST.RAW HOST.json SOUND.BIN SNDTAB.BIN SOUND.json)
ifneq ($(FX3),0)
PACKED_META += $(DATA)/FRAMES.json
PACKED_META += $(DATA)/SPHERE.BIN
PACKED_META += $(DATA)/CAMERA.BIN
PACKED_META += $(DATA)/INV.BIN
PACKED_META += $(DATA)/FONT.BIN
PACKED_META += $(DATA)/FX.BIN
PACKED_META += $(DATA)/TITLE.BIN
endif
# The title screen (FX3, packer writeTitle): the DOS release's own picture,
# when it is there -- without it TITLE.BIN is empty and the cartridge boots
# straight into its first level.
TITLE_PCX := $(if $(filter-out 0,$(FX3)),$(firstword $(wildcard $(TRDATA)/TITLEH.PCX $(TRDATA)/titleh.pcx)))
ifeq ($(strip $(LEVELS)),)
ifneq ($(words $(wildcard $(PACKED_META))),$(words $(PACKED_META)))
$(DATA)/packed.stamp: FORCE
endif

$(BUILD)/level.cfg: FORCE | $(BUILD)
	@printf '%s\n' '$(TRDATA)/$(LEVEL).PHD' 'FX3=$(FX3)' > $@.tmp
	@cmp -s $@.tmp $@ 2>/dev/null || { mv $@.tmp $@; sleep 1; touch $@; }
	@rm -f $@.tmp

$(DATA)/packed.stamp: $(BUILD)/packer $(BUILD)/level.cfg $(TRDATA)/$(LEVEL).PHD $(TITLE_PCX)
	@mkdir -p $(DATA)
	@rm -f $@
	$(BUILD)/packer $(TRDATA) $(DATA) $(LEVEL) $(if $(filter-out 0,$(FX3)),--fx3,)
	@touch $@ $(PACKED_META)

$(PACKED_META): $(DATA)/packed.stamp
	@test -f $@

$(DATA)/HOST.BIN: $(DATA)/HOST.RAW $(DATA)/HOST.json tools/hosttables.py
	$(PYTHON) tools/hosttables.py $(DATA)/HOST.RAW $(DATA)/HOST.json $@
else
# One packer run per level, each into its own directory; the same stamp
# discipline as the single-level rules above.
define LEVEL_PACK
LEVEL_META_$(1) := $$(subst $$(DATA)/,$(BUILD)/levels/$(1)/data/,$$(PACKED_META))
ifneq ($$(words $$(wildcard $$(LEVEL_META_$(1)))),$$(words $$(LEVEL_META_$(1))))
$(BUILD)/levels/$(1)/data/packed.stamp: FORCE
endif
$(BUILD)/levels/$(1)/data/packed.stamp: $(BUILD)/packer $(TRDATA)/$(1).PHD $(TITLE_PCX)
	@mkdir -p $(BUILD)/levels/$(1)/data
	@rm -f $$@
	$(BUILD)/packer $(TRDATA) $(BUILD)/levels/$(1)/data $(1) --fx3
	@touch $$@ $$(LEVEL_META_$(1))
$$(LEVEL_META_$(1)): $(BUILD)/levels/$(1)/data/packed.stamp
	@test -f $$@
$(BUILD)/levels/$(1)/data/HOST.BIN: $(BUILD)/levels/$(1)/data/HOST.RAW $(BUILD)/levels/$(1)/data/HOST.json tools/hosttables.py
	$(PYTHON) tools/hosttables.py $(BUILD)/levels/$(1)/data/HOST.RAW $(BUILD)/levels/$(1)/data/HOST.json $$@
endef
$(foreach l,$(LEVELS),$(eval $(call LEVEL_PACK,$(l))))
endif

BANK_INCLUDES := $(addprefix $(BUILD)/,textures.inc texbank.inc olweapon.h hostbank.inc olhost.h levels.inc leveltable.inc)
ifneq ($(words $(wildcard $(BANK_INCLUDES))),7)
$(BUILD)/banks.stamp: FORCE
endif

ifeq ($(strip $(LEVELS)),)
$(BUILD)/banks.stamp: $(PACKED_META) $(if $(filter-out 0,$(FX3)),$(DATA)/HOST.BIN,) tools/mkbanks.py tools/hosttables.py
	@rm -f $@
	$(PYTHON) tools/mkbanks.py $(DATA) $(BUILD)/textures.inc $(if $(filter-out 0,$(FX3)),--fx3,)
	@touch $@ $(BANK_INCLUDES)
else
$(BUILD)/banks.stamp: $(foreach l,$(LEVELS),$(LEVEL_META_$(l)) $(BUILD)/levels/$(l)/data/HOST.BIN) $(BUILD)/config.inc tools/mkbanks.py tools/hosttables.py
	@rm -f $@
	$(PYTHON) tools/mkbanks.py --fx3-multi $(BUILD) $(foreach l,$(LEVELS),$(l)=levels/$(l)/data)
	@touch $@ $(BANK_INCLUDES)

# The other levels' GSU programs: the same source, each level's layout, the
# second assembled to lie at $41:8000 (gsu_N/config.inc's GSU_ORIGIN).
# Name these known targets explicitly: GNU make 3.81 can spin on the implicit
# intermediate-object chain during an incremental parallel multi-level build.
ifneq ($(strip $(MULTI_GSU)),)
$(MULTI_GSU:.bin=.o): $(BUILD)/gsu_%/gsu.o: $(GSU_SRC) $(GSU_INC) $(BUILD)/config.inc $(BUILD)/divtable.bin $(BUILD)/model.bin $(BUILD)/model_off.inc $(BUILD)/banks.stamp
	$(WLASFX) -I gsu -I $(BUILD)/gsu_$* -I $(BUILD) -o $@ $(GSU_SRC)

$(MULTI_GSU): $(BUILD)/gsu_%/gsu.bin: $(BUILD)/gsu_%/gsu.o
	@printf '[objects]\n%s\n' $< > $(BUILD)/gsu_$*/gsu.link
	$(WLALINK_FX) -r $(BUILD)/gsu_$*/gsu.link $@
endif
endif

$(BANK_INCLUDES): $(BUILD)/banks.stamp
	@test -f $@

ifneq ($(words $(wildcard $(BUILD)/model.bin $(BUILD)/model_off.inc)),2)
$(BUILD)/model.stamp: FORCE
endif

$(BUILD)/model.stamp: tools/mkmodel.py | $(BUILD)
	@rm -f $@
	$(PYTHON) tools/mkmodel.py $(BUILD)/model.bin
	@touch $@ $(BUILD)/model.bin $(BUILD)/model_off.inc

$(BUILD)/model.bin $(BUILD)/model_off.inc: $(BUILD)/model.stamp
	@test -f $@

$(BUILD)/gsu.o: $(GSU_SRC) $(GSU_INC) $(BUILD)/config.inc $(BUILD)/divtable.bin $(BUILD)/model.bin $(BUILD)/model_off.inc $(BUILD)/textures.inc | $(BUILD)
	$(WLASFX) -I gsu -I $(BUILD) -o $@ $(GSU_SRC)

$(GSU_BIN): $(BUILD)/gsu.o
	@printf '[objects]\n%s\n' $< > $(BUILD)/gsu.link
	$(WLALINK_FX) -r $(BUILD)/gsu.link $@

# --- sound CPU --------------------------------------------------------------
# The SPC700 driver, a 512-byte flat binary main.asm INCBINs and uploads at
# boot. A third assembler, for the same reason as the second.

$(BUILD)/spc.o: src/spc.asm | $(BUILD)
	$(WLASPC) -o $@ $<

$(BUILD)/spc.bin: $(BUILD)/spc.o
	@printf '[objects]\n%s\n' $< > $(BUILD)/spc.link
	$(WLALINK_FX) -r $(BUILD)/spc.link $@

# --- C side -----------------------------------------------------------------

# Distinct suffixes at every step, so the pattern rules cannot chain into
# themselves: c_x.raw -> c_x.asm -> c_x.o.
$(BUILD)/c_%.raw: src/%.c $(BUILD)/oltables.h src/fixed.h src/item.h \
                  src/level.h src/room.h src/nav.h src/nav_fx3.h src/enemy.h src/object.h src/common.h src/camera.h \
                  src/lara.h src/collision.h src/weapon.h src/weapon_kit.h src/sound.h src/save.h src/inventory.h src/settings.h $(BUILD)/olweapon.h $(BUILD)/olhost.h \
                  $(BUILD)/config.inc tools/tcclint.py | $(BUILD)
	@$(PYTHON) tools/tcclint.py $<
	$(TCC) $(CFLAGS_VARIANT) $(CFLAGS_EXTRA) -I $(BUILD) -I src -c $< -o $@

# Native FX3 uses the fixed C DB=$7E / D=0 ABI for external globals too.
# Collect every compiler-owned BSS declaration before rewriting any unit;
# this is independent of link order and also works on a clean build.
FX3_WRAM_GLOBALS := $(if $(and $(filter-out 0,$(FX3)),$(filter 0,$(C_MOTION))),$(BUILD)/wram_globals.json)
$(BUILD)/wram_globals.json: $(C_OBJS:.o=.raw) src/host_tables.asm tools/wramglobals.py
	$(PYTHON) tools/wramglobals.py --output $@ --direct-page src/host_tables.asm $(C_OBJS:.o=.raw)

# Makefile is a dependency because the bank a unit lands in is decided here.
$(BUILD)/c_%.asm: $(BUILD)/c_%.raw tools/tccfix.py Makefile $(FX3_WRAM_GLOBALS)
	@$(TCCOPT) $< > $@.opt 2>/dev/null || cp $< $@.opt
	$(PYTHON) tools/tccfix.py $@.opt $@ \
	  --bank $(C_BANK_$*) --slot $(C_SLOT_$*) \
	  --wrambank $(or $(C_WRAM_$*),$(C_WRAMBANK)) --bss-slot $(or $(C_BSS_$*),2) \
	  $(if $(and $(filter-out 0,$(FX3)),$(filter 0,$(C_MOTION))),--fast-sign-extend --fast-wide-shifts --wram-globals $(FX3_WRAM_GLOBALS),$(if $(and $(filter 0,$(FX3)),$(filter 0,$(C_MOTION))),--compact-sign-extend,))

$(BUILD)/c_%.o: $(BUILD)/c_%.asm hdr.asm src/host_tables.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/oltables.h: tools/mktables.py | $(BUILD)
	$(PYTHON) tools/mktables.py $@

$(BUILD)/libtcc.o: src/libtcc.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

# Room_query, hand written: the same bank as room.c, the same .BASE as
# tccfix.py gives it. snes.inc is for the multiplier's registers and the
# width macros, and it includes config.inc, hence the build dir.
$(BUILD)/room.o: src/room.asm hdr.asm snes.inc $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/math.o: src/math.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/weapon_aim.o: src/weapon_aim.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/weapon_anim.o: src/weapon_anim.asm hdr.asm $(BUILD)/textures.inc $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/item_motion.o: src/item_motion.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/item_anim.o: src/item_anim.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/wolf_state.o: src/wolf_state.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/wolf_update.o: src/wolf_update.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/bear_state.o: src/bear_state.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/bear_update.o: src/bear_update.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/raptor_state.o: src/raptor_state.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/raptor_update.o: src/raptor_update.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/contact_fx3.o: src/contact_fx3.asm hdr.asm $(BUILD)/config.inc $(BUILD)/texbank.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/nav_sector.o: src/nav_sector.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/nav_fx3.o: src/nav_fx3.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/enemy_move.o: src/enemy_move.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/enemy_target_gsu2.o: src/enemy_target_gsu2.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/nav_flood_gsu2.o: src/nav_flood_gsu2.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/nav_cache_gsu2.o: src/nav_cache_gsu2.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/enemy_update.o: src/enemy_update.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/collision_fx3.o: src/collision_fx3.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/render_sync.o: src/render_sync.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/sound.o: src/sound.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/camera_fx3.o: src/camera_fx3.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/trap_fx3.o: src/trap_fx3.asm hdr.asm $(BUILD)/config.inc $(BUILD)/hostbank.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/save_fx3.o: src/save_fx3.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/sound_ring.o: src/sound_ring.asm hdr.asm $(BUILD)/config.inc $(BUILD)/levels.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/inventory_fx3.o: src/inventory_fx3.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/inventory_text_cache.o: src/inventory_text_cache.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/shadow_fx3.o: src/shadow_fx3.asm hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/room_cache.o: src/room_cache.asm src/host_dp.inc snes.inc hdr.asm $(BUILD)/config.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

$(BUILD)/object_class_table.inc: tools/object_class_table.py src/object.c src/object.h src/enemy.c src/item.h $(BUILD)/config.inc | $(BUILD)
	$(PYTHON) $< $@ --fx3=$(if $(filter-out 0,$(FX3)),1,0)

$(BUILD)/object_class.o: src/object_class.asm hdr.asm $(BUILD)/config.inc $(BUILD)/object_class_table.inc
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -o $@ $<

ASM_OBJS := $(BUILD)/libtcc.o $(BUILD)/room.o $(BUILD)/math.o $(BUILD)/weapon_aim.o $(BUILD)/weapon_anim.o $(BUILD)/item_motion.o $(BUILD)/item_anim.o $(BUILD)/wolf_state.o $(BUILD)/wolf_update.o $(BUILD)/bear_state.o $(BUILD)/bear_update.o $(BUILD)/nav_sector.o $(BUILD)/nav_fx3.o $(BUILD)/enemy_move.o $(BUILD)/collision_fx3.o $(BUILD)/render_sync.o $(BUILD)/sound.o
ifeq ($(C_MOTION),0)
ASM_OBJS += $(BUILD)/object_class.o
ifeq ($(FX3),0)
ASM_OBJS += $(BUILD)/enemy_target_gsu2.o $(BUILD)/nav_flood_gsu2.o $(BUILD)/nav_cache_gsu2.o $(BUILD)/camera_fx3.o $(BUILD)/room_cache.o
endif
endif
ifneq ($(FX3),0)
ASM_OBJS += $(BUILD)/raptor_state.o $(BUILD)/raptor_update.o $(BUILD)/contact_fx3.o $(BUILD)/enemy_update.o $(BUILD)/camera_fx3.o $(BUILD)/trap_fx3.o \
            $(BUILD)/save_fx3.o $(BUILD)/sound_ring.o $(BUILD)/inventory_fx3.o
ifeq ($(C_MOTION),0)
ASM_OBJS += $(BUILD)/inventory_text_cache.o $(BUILD)/shadow_fx3.o $(BUILD)/room_cache.o
endif
endif

$(ASM_OBJS): src/host_tables.inc

# --- 65816 side -------------------------------------------------------------

# main.asm INCBINs gsu.bin, so the assembler needs it on the include path.
$(BUILD)/fx3maps.bin: tools/fx3.py | $(BUILD)
	$(PYTHON) $< $@

$(BUILD)/main.o: $(CPU_SRC) $(CPU_INC) src/host_dp.inc src/host_tables.asm src/host_tables.inc $(BUILD)/hostbank.inc $(BUILD)/config.inc $(GSU_BIN) $(MULTI_GSU) $(BUILD)/textures.inc $(BUILD)/texbank.inc $(BUILD)/levels.inc $(BUILD)/leveltable.inc $(BUILD)/spc.bin $(DATA)/SOUND.BIN $(DATA)/SNDTAB.BIN $(DATA)/PALETTE.BIN $(DATA)/HUD.BIN $(if $(filter-out 0,$(FX3)),$(BUILD)/fx3maps.bin,)
	$(WLA65816) $(ASFLAGS) -I . -I $(BUILD) -I $(DATA) -o $@ $(CPU_SRC)

$(ROM): $(BUILD)/main.o $(C_OBJS) $(ASM_OBJS) tools/budget.py tools/snesfix.py
	@printf '[objects]\n' > $(BUILD)/rom.link
	@for o in $(BUILD)/main.o $(C_OBJS) $(ASM_OBJS); do echo $$o >> $(BUILD)/rom.link; done
	$(WLALINK) -s -r $(BUILD)/rom.link $@
	@$(PYTHON) tools/budget.py $(BUILD)/$(TARGET).sym
	$(PYTHON) tools/snesfix.py $@

# --- misc -------------------------------------------------------------------

motioncheck: $(ROM)
	@test "$(C_MOTION)" = "0" || { echo "motioncheck requires C_MOTION=0 for the native ROM"; exit 1; }
	$(MAKE) BUILD=$(BUILD)_cmotion C_MOTION=1 LEVELS= LEVEL=$(LEVEL) all
	OL_REFERENCE_ROM=$(CURDIR)/$(BUILD)_cmotion/$(TARGET).sfc \
	OL_REFERENCE_SYMS=$(CURDIR)/$(BUILD)_cmotion/$(TARGET).sym \
	  $(MAKE) check TESTS='native item motion|native collision|native camera|wolves|bears|bats|raptors|frame contact|navigation sectors|per-creature navigation|floor geometry'

run: $(ROM)
	$(EMU) $(ROM)

# The opening frame's modelled cost, held against the recorded one. A
# renderer change that costs ten per cent used to be found later, by
# wondering; now it fails here until `make baseline` records it, and the
# record is tracked so the change shows in the diff. Only LEVEL1's opening
# view is recorded, so a `make check TICK=n`, a moved camera or another
# level skips it.
FRAME_TOL ?= 2
# Baseline names stay stable across the default changes. The 4bpp unlit default
# is frame.4bpp.unlit.baseline; frame.4bpp.baseline retains its optional shaded
# record, and the unsuffixed names retain the historical Mode 7 records.
BASELINE   = frame$(if $(filter-out 0,$(FX3)),.fx3,$(if $(filter-out 0,$(MODE7)),,.4bpp))$(if $(filter-out 0,$(SHADING)),,.unlit).baseline
VIEW       = $(CAMX),$(CAMY),$(CAMZ),$(CAMTY),$(ANGLE),$(ROOM),$(TICK)
OPENING    = 8215,2228,3584,2491,64,0,0

# Headless: boot the real ROM in TypeSNES and dump what the PPU produced.
# The runner is copied under the build directory's name, so two levels'
# checks can run at once: `make check` and `make BUILD=build_l2 LEVEL=LEVEL2
# check` do not share a file.
$(BUILD)/navmood.inc: tools/mknavmood.py ../../fixed/enemy.h | $(BUILD)
	$(PYTHON) tools/mknavmood.py ../../fixed/enemy.h $@

$(BUILD)/navcheck: tools/navcheck.cpp ../../fixed/nav.h $(BUILD)/navmood.inc | $(BUILD)
	$(CXX) -std=c++11 -O2 -Wall -Wextra -I $(BUILD) $< -o $@

# Upstream's getSpheres/collideSpheres and GBA's matrix code, extracted
# verbatim, run on the raw level: the blade-sphere oracle (FX3).
$(BUILD)/sphereupstream.inc: tools/mkspherecheck.py tools/mkenemystate.py ../../fixed/item.h ../../fixed/common.h ../../fixed/common.cpp | $(BUILD)
	PYTHONPATH=tools $(PYTHON) tools/mkspherecheck.py ../../fixed $@

$(BUILD)/spherecheck: tools/spherecheck.cpp $(BUILD)/sphereupstream.inc | $(BUILD)
	$(CXX) -O1 -std=c++11 -w -I../gba/packer -I../gba/packer/libimagequant -I $(BUILD) $< -o $@

$(BUILD)/bearstate.inc: tools/mkenemystate.py ../../fixed/enemy.h | $(BUILD)
	$(PYTHON) $< ../../fixed/enemy.h Bear $@

$(BUILD)/enemycheck: tools/enemycheck.cpp $(BUILD)/bearstate.inc | $(BUILD)
	$(CXX) -std=c++11 -O2 -Wall -Wextra -I $(BUILD) $< -o $@

$(BUILD)/raptorstate.inc: tools/mkenemystate.py ../../fixed/enemy.h | $(BUILD)
	$(PYTHON) $< ../../fixed/enemy.h Raptor $@

$(BUILD)/raptorcheck: tools/raptorcheck.cpp $(BUILD)/raptorstate.inc | $(BUILD)
	$(CXX) -std=c++11 -O2 -Wall -Wextra -I $(BUILD) $< -o $@

# FX3 with LEVEL1 first: nearcheck (below) too.
FIRST_LEVEL := $(if $(strip $(LEVELS)),$(firstword $(LEVELS)),$(LEVEL))
NEARCHECK   := $(if $(and $(filter-out 0,$(FX3)),$(filter LEVEL1,$(FIRST_LEVEL))),nearcheck)
check: $(DATA)/AIM.json $(DATA)/NAV.json $(BUILD)/enemycheck $(if $(filter-out 0,$(FX3)),$(BUILD)/navcheck $(BUILD)/raptorcheck $(BUILD)/spherecheck $(BUILD)/c_wolf.raw $(BUILD)/c_wolf.asm) $(NEARCHECK) sim
	@test -d "$(TYPESNES)" || { echo "TYPESNES=$(TYPESNES) not found"; exit 1; }
	@if [ "$(VIEW)" = "$(OPENING)" ] && [ "$(LEVEL)" = "LEVEL1" ]; then \
	   $(PYTHON) tools/framecheck.py $(BUILD)/gsu_frame.stats $(BASELINE) \
	     --tolerance $(FRAME_TOL); \
	 else echo "[openlara] frame model: not LEVEL1's opening view, not compared"; fi
	@OL_ROM=$(CURDIR)/$(ROM) \
	  OL_EXPECT=$(CURDIR)/$(BUILD)/gsu_frame.bin \
	  OL_EXPECT_SHADE=$(CURDIR)/$(BUILD)/gsu_shade.bin \
	  OL_FRAMES=$(FRAMES) \
	  OL_WALK=$(WALK) \
	  OL_TICK=$(TICK) \
	  OL_LEVEL=$(LEVEL) \
	  OL_BOOT_LEVEL=$(BOOT_LEVEL) \
	  OL_DATA=$(CURDIR)/$(DATA) \
	  OL_MODE7=$(MODE7) \
	  OL_FX3=$(FX3) \
	  OL_NAV_ORACLE=$(CURDIR)/$(BUILD)/navcheck \
	  OL_SPHERE_ORACLE=$(CURDIR)/$(BUILD)/spherecheck \
	  OL_LEVEL_PHD=$(TRDATA)/$(LEVEL).PHD \
	  OL_SPC=$(CURDIR)/$(BUILD)/spc.bin \
	  OL_SOUND_BIN=$(CURDIR)/$(DATA)/SOUND.BIN \
	  OL_SNDTAB=$(CURDIR)/$(DATA)/SNDTAB.BIN \
	  OL_SOUND_JSON=$(CURDIR)/$(DATA)/SOUND.json \
	  OL_SYMS=$(CURDIR)/$(BUILD)/$(TARGET).sym \
	  OL_OUT=$(CURDIR)/$(BUILD)/frame.png \
	  $(PYTHON) tools/run_typesnes.py --typesnes "$(TYPESNES)" \
	    --runner tools/typesnes_dump.ts --log "$(BUILD)/typesnes.log" -- \
	    $(if $(TESTS),-t "$(TESTS)")

# Pixel oracle for keyed and opaque spans, in both independent GSU cores.
keycheck: $(ROM)
	@$(WLALINK_FX) -s -r $(BUILD)/gsu.link $(BUILD)/gsu_sym.bin > /dev/null
	$(PYTHON) tools/keycheck.py $(ROM) $(BUILD)/gsu_sym.sym $(GSUSIM_FLAGS) \
	  $(if $(filter-out 0,$(SHADING)),--shading) --fixtures $(BUILD)/keycases.json
	@test -d "$(TYPESNES)" || { echo "TYPESNES=$(TYPESNES) not found"; exit 1; }
	@OL_ROM=$(CURDIR)/$(ROM) OL_KEY_CASES=$(CURDIR)/$(BUILD)/keycases.json \
	  $(PYTHON) tools/run_typesnes.py --typesnes "$(TYPESNES)" \
	    --runner tools/typesnes_key.ts --log "$(BUILD)/keycheck.log"

# Small cartridge regressions for the parity/performance paths added here.
# Optional OL_REFERENCE_ROM/SYMS enable full native/C inventory comparisons;
# the production ROM alone always checks triggers, floors and species wiring.
paritycheck: $(ROM)
	@OL_ROM=$(CURDIR)/$(ROM) OL_SYMS=$(CURDIR)/$(BUILD)/$(TARGET).sym \
	  OL_FX3=$(FX3) OL_LEVEL=$(LEVEL) OL_BOOT_LEVEL=$(BOOT_LEVEL) OL_DATA=$(CURDIR)/$(DATA) \
	  $(PYTHON) tools/run_typesnes.py --typesnes "$(TYPESNES)" \
	    --runner tools/typesnes_trigger.test.ts \
	    $(if $(filter-out 0,$(FX3)),--runner tools/typesnes_floor.test.ts --runner tools/typesnes_lion.test.ts --runner tools/typesnes_inventory_perf.test.ts --runner tools/typesnes_textcache.test.ts,$(if $(filter 0,$(C_MOTION)),--runner tools/typesnes_compact.test.ts)) \
	    $(if $(filter 0,$(C_MOTION)),--runner tools/typesnes_object_class.test.ts --runner tools/typesnes_navcount.test.ts --runner tools/typesnes_products.test.ts --runner tools/typesnes_division.test.ts --runner tools/typesnes_item_bounds.test.ts $(if $(filter LEVEL1,$(LEVEL)),--runner tools/typesnes_creature_move.test.ts)) \
	    $(if $(and $(filter-out 0,$(FX3)),$(filter 0,$(C_MOTION))),--runner tools/typesnes_navscale.test.ts --runner tools/typesnes_navwaypoint.test.ts --runner tools/typesnes_shadow.test.ts $(if $(filter LEVEL1,$(LEVEL)),--runner tools/typesnes_roomcache.test.ts)) \
	    $(if $(and $(filter 0,$(FX3)),$(filter 0,$(C_MOTION))),--runner tools/typesnes_enemy_target_gsu2.test.ts --runner tools/typesnes_navcache_gsu2.test.ts --runner tools/typesnes_gsu_overlap.test.ts $(if $(filter LEVEL1,$(LEVEL)),--runner tools/typesnes_creature_control.test.ts --runner tools/typesnes_roomcache.test.ts)) \
	    $(if $(filter-out 0,$(FX3_STAGE)),--runner tools/typesnes_staging.test.ts) \
	    --log "$(BUILD)/paritycheck.log" -- $(if $(TESTS),-t "$(TESTS)")

# Accept the opening frame as it now is. Commit the file with the change.
baseline: sim
	$(PYTHON) tools/framecheck.py $(BUILD)/gsu_frame.stats $(BASELINE) --record

# Where the cycles went. The interpreter models the instruction cache, so this
# is the tool that says whether a change made the frame shorter -- instruction
# counts do not, on a chip that charges five cycles a byte to fetch anything
# outside the 512-byte window. `wlalink -s` writes the symbol file it needs.
profile: $(ROM)
	@$(WLALINK_FX) -s -r $(BUILD)/gsu.link $(BUILD)/gsu_sym.bin > /dev/null
	$(PYTHON) tools/gsusim.py $(ROM) --scbr 4 --fb 0x1000 $(GSUSIM_FLAGS) \
	  --cam-x $(CAMX) --cam-y $(CAMY) --cam-z $(CAMZ) --cam-ty $(CAMTY) --angle $(ANGLE) --room $(ROOM) \
	  --tick $(TICK) \
	  --probe-x $(PROBEX) --probe-y $(PROBEY) --probe-z $(PROBEZ) \
	  --profile $(BUILD)/gsu_sym.sym --top $(TOP) \
	  --palette $(DATA)/PALETTE.BIN -o $(BUILD)/gsu_frame.png

TOP ?= 16
TICK ?= 0

# The matrix rotations, on their own, against floating point. A whole-frame
# comparison is a poor way to find a sign error in one of nine elements -- a
# wrong column still draws something, and something is what it sees.
# The host's per-entity channel, which the frame comparison cannot see: it
# stages the same phases the host computes, so it would agree with itself even
# if the renderer read one slot for every entity.
animcheck: $(ROM)
	$(PYTHON) tools/animcheck.py $(ROM) $(GSUSIM_FLAGS)

matcheck: $(ROM)
	@$(WLALINK_FX) -s -r $(BUILD)/gsu.link $(BUILD)/gsu_sym.bin > /dev/null
	$(PYTHON) tools/matcheck.py $(ROM) $(BUILD)/gsu_sym.sym

# Exact signed arithmetic and the real linked routine in two independent
# interpreters. The TypeSNES fixture uses the FX3 host register window.
matrixcheck: matcheck
	$(PYTHON) tools/matrix_exactcheck.py $(ROM) $(BUILD)/gsu_sym.sym
ifneq ($(FX3),0)
	OL_ROM=$(CURDIR)/$(ROM) OL_GSU_SYMS=$(CURDIR)/$(BUILD)/gsu_sym.sym \
	  $(PYTHON) tools/run_typesnes.py --typesnes "$(TYPESNES)" \
	  --runner tools/typesnes_matrix.test.ts --log "$(BUILD)/typesnes-matrix.log"
endif

weaponcheck: $(ROM)
	@$(WLALINK_FX) -s -r $(BUILD)/gsu.link $(BUILD)/gsu_sym.bin > /dev/null
	$(PYTHON) tools/weaponcheck.py $(BUILD)

# FX3's near projection (mesh.gsu TM_Near, PC_Near, ShearNear) against its
# own arithmetic, and TM_Near's side bits against the frames they must not
# change, on close views of LEVEL1's start: the cartridge's first level
# (NEARCHECK, above check, adds it there).
nearcheck: $(ROM)
	@$(WLALINK_FX) -s -r $(BUILD)/gsu.link $(BUILD)/gsu_sym.bin > /dev/null
	$(PYTHON) tools/nearcheck.py $(ROM) --sym $(BUILD)/gsu_sym.sym --level 0 \
	  --data $(if $(strip $(LEVELS)),$(BUILD)/levels/$(FIRST_LEVEL)/data,$(DATA))

# The view the oracles render: the settled follow camera at Lara's LEVEL1
# placement, which is where the ROM's camera is by the time `make check`
# samples it. Not behind her: the wanted view 1536 behind her is a sector
# inside the entrance wall, the trace stops it within 768 of her, and
# upstream's fallback then takes the cardinal candidate nearest the present
# view -- west, distH (1536 cos 10 degrees, 1513 on the port's sine table)
# to her left at her own Z, looking east (angle 64 of 256), and distV (263)
# above the target. CAMTY is the target's height, three quarters up her
# standing box, which the frame shears its centre line to: upstream's pitch.
# Override to look somewhere else: `make sim ROOM=12 CAMX=... `.
CAMX  ?= 8215
CAMY  ?= 2228
CAMZ  ?= 3584
CAMTY ?= 2491
ANGLE ?= 64
ROOM  ?= 0

# Where the host would have asked for a floor. Lara's eye at the level start,
# which is 1536 in front of the camera and 512 above her feet.
PROBEX ?= 9728
PROBEY ?= 2560
PROBEZ ?= 3584

# The engine port's oracle. src/*.c compiles two ways -- for the cartridge
# with 816-tcc, and for the desktop with OL_HOST_BUILD swapping the type
# widths -- so this runs the shipped code against libm rather than against a
# copy of it. Nothing gets ported out of src/fixed without landing here.
fixedcheck: $(BUILD)/fixedcheck
	./$<

$(BUILD)/fixedcheck: tools/fixedcheck.c src/math.c src/common.c src/fixed.h \
                     src/common.h $(BUILD)/oltables.h
	$(CC) -O1 -Wall -DOL_HOST_BUILD -I $(BUILD) -I src -o $@ \
	  tools/fixedcheck.c src/math.c src/common.c -lm

# GSU only: interpret the co-processor program and render its frame buffer.
# Much faster than a full machine and it isolates renderer bugs from host bugs.
# FX3: the level table names each level's program, banks and host tables.
SIM_LEVEL := $(if $(filter-out 0,$(FX3)),--level $(BOOT_LEVEL),)

sim: $(ROM)
	$(PYTHON) tools/gsusim.py $(ROM) --scbr 4 --fb 0x1000 $(GSUSIM_FLAGS) $(SIM_LEVEL) \
	  --cam-x $(CAMX) --cam-y $(CAMY) --cam-z $(CAMZ) --cam-ty $(CAMTY) --angle $(ANGLE) --room $(ROOM) \
	  --tick $(TICK) \
	  --probe-x $(PROBEX) --probe-y $(PROBEY) --probe-z $(PROBEZ) \
	  --palette $(DATA)/PALETTE.BIN --raw $(BUILD)/gsu_frame.bin \
	  --raw-shade $(BUILD)/gsu_shade.bin --stats $(BUILD)/gsu_frame.stats \
	  -o $(BUILD)/gsu_frame.png

clean:
	rm -rf $(BUILD)

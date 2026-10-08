# FX3 host tables (OLH2)

FX3's CPU and FX ROM windows are different. The [vendor specification,
§§1, 6 and 7](https://github.com/LimitedRunGames-Tech/snes-fx3/blob/main/docs/FX3.pdf)
allows simultaneous CPU/FX ROM and SRAM access, with 4 MiB of full-bank CPU
ROM at `$C0-$FF` but only 3 MiB at FX `$40-$6F`. CPU `$F0-$FF` must not be
translated into FX ROM banks. Engine code stays in FastROM; it is not copied
to WRAM or suspended to surrender ROM/SRAM ownership to the FX.

## Allocation and build

`FX3=1` produces a 4 MiB image with a Mode `$30`, type `$18`, size `$0C`
header. A level's `HOST.BIN` starts a CPU bank: file `$200000`, CPU
`$E00000`, for a one-level cartridge; from file `$300000`, CPU `$F00000`, the
GSU's blind last MiB, for several (`make LEVELS=...`). Where is the level
table's (`tools/mkbanks.py`, `leveltable.inc`): its first word, the host
bank, is what `olHostBind` reads -- it copies the level's 256-byte directory
into `olHostDirectory` first, and binds every table at that bank plus its
offset. GSU geometry and textures run from GSU `$45` to `$6F`, below them.

The C++ packer emits `HOST.RAW` and `HOST.json`. The latter supplies 32-bit
offsets and lengths for all tables; the old length word/footer in `HOST.RAW`
is not an FX3 addressing contract. `tools/hosttables.py` produces `HOST.BIN`,
and `tools/mkbanks.py` places it in 32 KiB linker chunks, including the exact
final partial chunk. FX3 `LEVEL.BIN` contains geometry, not a duplicate host
blob; its legacy host-offset field is zero.

GSU-2 retains its original 2 MiB image, one-bank host blob inside `LEVEL.BIN`,
and SRAM staging at `$710000`. The new metadata exports do not change that
ROM. Changing `FX3` in one build directory invalidates the packed assets as
well as assembly/C objects. Missing generated metadata also forces repacking.

## Directory

All integers are little-endian. Offsets are relative to `HOST.BIN`, not to
the cartridge image. The fixed directory is 256 bytes:

| Byte offset | Type | Meaning |
|---|---|---|
| 0 | 4 bytes | `OLH2` |
| 4 | u16 | Version: 5 (current-frame boxes, byte slopes, six-map ZONE) |
| 6 | u16 | Table count: 18 |
| 8 | u32 | Total image length, including directory/padding |
| 12 | u16 | Directory length: 256 |
| 14 | u16 | Reserved: 0 |
| 16 | 12 × u16 | Record counts, in the order below |
| 40 | 18 × 12 bytes | Table descriptors, in the order below |

Count order: `items, models, anims, trans, triggers, cmds, boxes, overlaps,
rooms, sectors, links, statics`.

Each descriptor is `u32 offset, u32 length, u16 flags, u16 reserved`.
The reserved word is zero. Flags are 1 for `SECTOR` and 0 for every other
table. Tables are ordered, non-overlapping, even-aligned, and each fits
wholly within a 64 KiB CPU bank. Padding between tables is permitted. The
last table ends exactly at the declared image length. Even an empty table
must have an address inside the allocation.

| Index | Table | Fixed record shape |
|---|---|---|
| 0 | ROOM | rooms × 10 |
| 1 | SECTOR | sectors × 4; mutable |
| 2 | LINK | links × 8 |
| 3 | MODEL | models × 16 |
| 4 | ANIM_BOX | union prefix, frame directory and shared boxes; see below |
| 5 | ANIM_SPEED | anims × 8 |
| 6 | ANIM | anims × 14 |
| 7 | TRANS | trans × 6 |
| 8 | CMD | cmds × 8 |
| 9 | TRIGGER | triggers × 16 |
| 10 | ITEM | items × 16 |
| 11 | BOX | boxes × 8 |
| 12 | OVERLAP | overlaps × 2 |
| 13 | ZONE | six-map directory and shared arrays; see below |
| 14 | STATIC | statics × 20 |
| 15 | STATIC_FIRST | (rooms + 1) × 2 |
| 16 | AIM | self-indexed compressed aim points |
| 17 | NAV | self-indexed compressed exact sector IDs |

The aggregate no longer has a 64 KiB limit. Individual tables still have a
65,536-byte limit because their runtime indices are 16-bit; that is a
remaining software constraint. Build-time validation checks fixed record
shapes against counts; the C++ packer validates the variable encodings.
The Python builder/parser additionally validates ZONE and ANIM_BOX's internal ranges.

### Current-frame boxes

ANIM_BOX retains its `anims × 12` union prefix for legacy callers. It is
followed by `anims × u16` record offsets, relative to the start of ANIM_BOX.
A zero offset means union fallback. A frame record contains `u16 tickCount`
(1–255) and `tickCount × u16` offsets to six signed-word bounds, in
`minX, maxX, minY, maxY, minZ, maxZ` order. Identical boxes share an offset,
including whole boxes in the union prefix. Records and boxes are even-aligned;
partial overlaps, mixed-type aliases, holes and unaccounted tails are rejected.
An out-of-range local frame falls back to that animation's union.

The packer resolves the original nearest-frame choice per tick, ties toward
the earlier pose, retaining `getFrames`' comparison against the original
absolute `frameEnd`. `FRAMES.json` exports raw source boxes/timing, not
expected runtime lookup results. Every valid model is included, including
Lara: LEVEL1/LEVEL2/LEVEL3A use 57,314/63,346/59,880 bytes for this table.
Version 4 has no frame directory and is rejected by the version-5 binder.

`ItemObj_frameBox` reads directly from the independent far pointer, including
bank-start and bank-end placements in CPU-only `$F8`. No frame boxes are
copied to WRAM. Wolf/bear/bat/raptor contact now follows the GBA/32X `FAST_HITMASK` bound
test and records contact before the unexpanded frame-box push. Its 128-byte
per-item latch is mutable. Wolves reuse this existing array; their attack
update no longer recomputes union contact after Lara has moved. Sixty-four
synthetic bank-boundary/eligibility/attack cases and 62 positions from original
attack/bite poses verify this migration on LEVEL1, LEVEL2 and LEVEL3A. The
old ROM fails the ghost-contact regression. Bear migration reuses the same
array, preserving running damage, latched attacks and the upright death hit.
Its 144 bank-boundary/eligibility/dead-target cases and 447 original-pose
ghost-hit positions pass on LEVEL1/LEVEL2, including both death animations.
Bat migration likewise reuses the latch, with 200 bank-boundary/eligibility/
dead-target cases and 124 original flight/attack ghost-contact positions.
The old ROM inflicts two unwanted damage in the new regression; both new
update entry points match 512 complete C snapshots apiece. No new assets,
RAM or host-directory version are needed. Legacy interaction readers and
GSU-2 still use their older union/contact paths.

`ItemObj_frameContact` fuses the rotated bound test and optional geometric
push without caching immutable boxes in WRAM. It returns the pre-push hit
bit and a separate pushed bit. Its 26-byte stack frame retains the box's
full CPU pointer, angle and full-width local coordinates; the native owner
loop needs another ten stack bytes. The executable C reference deliberately
uses the separate operations. Multi-owner comparisons exercise far `$7F`
Lara pointers and table-start/table-end placements in CPU-only `$F8/$F9`,
including all 24 mixed wolf/bear/bat/raptor three-species owner orders over
960 cases. The full C phase also matches 480 geometry/guard snapshots for
each migrated species.
Enemy room probes retain the original temporary zero radius through the
entire push, then restore the caller's radius, including rejected pushes.
No table layout, host-directory version or persistent RAM allocation changes.

The native `ItemObj_collideRoom` wrapper likewise executes from CPU FastROM.
Its 30-byte local frame retains the point coordinates and original movement
offsets; signed radius/trig products remain wide until the original shift.
It still calls the middle, front, left and right probes in order (two for
the existing front-only mode), followed by the unchanged C static response.
Zero radius skips only coordinate arithmetic, not the repeated queries or
non-middle slant rules. Tests independently place ROOM/LINK/STATIC/STATIC_FIRST
at both ends of CPU-only `$F8/$F9/$FA/$FB` and collision/item objects in mirror,
`$7E` and `$7F` WRAM. C objects fit one bank, as the linker already ensures:
816-tcc member-pointer additions do not carry out of the low word. This
software restriction does not reduce FX3's full-bank CPU ROM window.

The stalking-box predicate likewise reads packed BOX bounds in native code.
Its nine-bit min+max sum, scaled centre and target/enemy differences stay
32-bit; Lara's signed-angle quadrant is read through the full CPU pointer.
It adds no table, WRAM copy, local frame, PRNG roll or navigation cache.
All 131,072 original-predicate cases cover every angle and bound sum 0..510,
with BOX at both ends of CPU-only `$F8` and Lara in `$7E`/far `$7F`.
Another 2,048 C comparisons also preserve game state and ROM bytes.

The FX3 camera's native code stays in `$85` FastROM and reads the existing
`$87` sine table directly, with no WRAM code copy.
Sector walks still delegate portals and full-byte slopes to Room_query.
Their independent/C oracle places ROOM and LINK at both ends of CPU-only
`$F8/$F9` banks and uses mirrored/far Location pointers, including records
ending at `$7F:FFFF`. The 50-byte walker frame and 20-byte query result are
temporary stack storage, not a new persistent cache or allocation. The
original C versions remain available in C_MOTION builds.

Camera_move and the room-only resolver also execute there. The movement
frame is 28 temporary bytes (view plus RoomQuery); the resolver needs 20.
Two full room queries still run after the ordered signed-word x/y/z lerp,
with the original height test, conditional trace and third lookup. Inputs
may be mirrored/far or overlap camera fields; the trace source explicitly
retains its `$7E` WRAM bank. Exhaustive deltas and 2,048 C comparisons cover
bank-end records and every intermediate query/write order. The camera's
existing approximation caches retain their rules.

FX3's target offsets now match GBA's frame-interpolated bounds. CAMERA.BIN
keeps its camera/trigger prefix and appends `u16 animCount`, followed by four
`u16` stream offsets per animation: follow Y, middle Y, middle X, middle Z.
Offsets are relative to CAMERA.BIN, whose `olLevel.cameraAt/cameraBank`
descriptor follows level switches. Zero denotes an unavailable stream.
Each stream contains `u8 ticks`, `u8 bits`, `i16 minimum`, then little-endian
packed unsigned deltas. Constant streams have ticks=bits=0; equal scalar
sequences share records. Three final zero bytes keep the decoder's pair of
word loads inside the table. Invalid ticks select the first sample; invalid
animations/components and unavailable streams return zero.

The packer uses the original getFrames selection, signed interpolation and
GBA reciprocal rounding (including `$7FFF` for divisor two), then computes
the four offsets with full-width arithmetic. The CPU only extracts a sample
and adds the wrapped world coordinate. LEVEL1/LEVEL2 add 29,450/31,122 bytes,
fit the existing cartridge allocation and use no extra RAM. The raw-PHD
oracle extracts getBoundingBox(true) and the actual reciprocal table from
the shared engine. GSU-2 still uses animation unions; contact bounds retain
their separate nearest-frame lookup on FX3.

The link-time FastROM allocation guard covers CPU `*_native` sections by
name convention, including camera_fx3_native and future helpers. Only the
fixed native interrupt-vector header table is exempt from that convention.
Tests reject accidental WRAM/SlowROM placement, crossing a bank and reversed
section bounds. The accepted code banks are this engine's allocation, not
a reduction of FX3's full-bank CPU ROM window.

Per-frame animation commands now scan CMD directly in native code through
its independent CPU pointer. ANIM and MODEL may be in different banks;
the regression puts all three at both ends of `$F8/$F9/$FA`. The captured
command list/count survives an effect that changes the animation, while
subsequent frame comparisons see the updated item frame. Only that rare
animation-setting call saves six stack bytes. End-of-animation offset,
jump, empty-hands and kill commands still use their complete C handler;
there is no command-table copy or new persistent allocation. SOUND remains
unimplemented pending the APU driver, as before this dispatch optimization.

The logic PRNG has a separate 1 KiB immutable table in the engine's `$87`
FastROM bank: 256 exact uint32 products by `$41C64E6D`, generated as explicit
low/high uint16 words. The native code adds four shifted byte contributions
and `$3039`, retaining every carry below bit 32. No table copy, hardware
multiplier or new mutable allocation is involved; GSU-2 and C_MOTION omit
the table. Six long word reads, including the middle bytes of one product,
are verified with their operands explicitly relocated to `$F8:0000` and
`$F8:FC00`. These code patches are test-only, with immediate readback; the
emulator's real CPU ROM-write protection is unchanged.

The adjacent-room filter reads the existing 32-byte renderer portal records
through CPU FastROM. `OL_LEVEL_ADDRESS` is generated from the asset placement;
the relative renderer bank is added to its CPU bank, not passed to FX ROMBR.
This access is legal during FX execution under the vendor's concurrent-ROM
rules. The C reference binds the far pointer's two words explicitly because
816-tcc loses the bank on a computed int32-to-pointer cast.

The native Nav search likewise retains an overlap INDEX in Y, not a copied
list or a translated FX ROM bank. Geometry readers temporarily save that
index and use their independent zone/box pointers; tests place the three
tables independently at the start/end of CPU-only `$F8/$FA/$FB` banks.
The saved geometry cursor now advances directly from a box's floor word
to its adjacent overlap word after the unchanged height/weight guards.
All 324 complete C search snapshots stay exact; their native instruction
count falls from 181,965 to 172,936. Search setup is reused only inside one
five-step Nav_expand call, never across a flip or another creature. This
changes neither OLH2 nor the table layout, and consumes no new persistent WRAM.

### Floor and ceiling slopes

LINK remains eight bytes: below/above/portal/trigger bytes, then a floor
slant word and a ceiling slant word. Each FX3 word stores signed-byte X in
the low byte and signed-byte Z in the high byte, preserving TR1's original
`-128..127` range. Version 3's five-bit components are incompatible and are
rejected by the binder. GSU-2 retains the older encoding and range check.

`Room_query` applies the original quarter-scaled signed products. Magnitudes
can exceed 65,535, so wide slopes use two exact CPU byte products before
combining the scaled result; narrow slopes retain a short shift/add path.
Both multiplier reads respect its eight-cycle latency. NMI never uses the
multiplier. The change needs no extra LINK bytes or mutable workspace.

### Zone maps

ZONE starts with six `u16` byte offsets relative to the table, followed by
arrays of `boxes × u16` original zone IDs. Directory order is normal
ground-1, ground-2, fly, then alternate ground-1, ground-2, fly. Every array
is even-aligned and starts at or after byte 12. Identical whole arrays share
an offset; partial overlaps, holes and trailing unaccounted bytes are
rejected. The zero-box encoding is just the 12-byte directory, with all six
offsets equal to 12. Version 2's flat ground-1/fly layout is not compatible
with this interpretation, so runtime binding rejects it.

`Nav_setFlipped` normalizes its argument to 0/1 and caches the three active
array offsets. Search, zone checks and movement read the selected maps.
Initialization instead builds the original ordered union: a box belongs if
it shares the source box's zone in **either** normal or alternate geometry,
without duplicates and regardless of the current flip. Shared maps use a
single-array scan; distinct maps use a two-array scan in the same CPU bank.
Ground-2 is family 1, not a flying-family alias; only family 2 receives fly
waypoint/mood height adjustments. The flip word and three cached offsets
cost eight bytes in the explicit FX3 Nav allocation.

This is navigation flip selection, not alternate-room swapping. Room flips,
their triggers and water-creature controllers still need implementation.
TR1 water creatures use the fly family, not a separate fourth zone map.
GSU-2's two flat normal arrays and simplified flood are unchanged.

## Runtime binding and writable data

`olHostBind` in `src/host_tables.asm` runs from `$86` during boot, with
interrupts disabled. It validates the directory version, size, flags,
reserved words and all address ranges before copying sectors or publishing
`olHostTablesReady`. Invalid directories leave the ready flag clear; level
initialization does not consume partially bound pointers. Runtime range
validation does not duplicate the build-time record-shape checks.

Each table has its own four-byte compiler far pointer, `ol<Name>Data`.
`ol<Name>Base` aliases its low word for native routines, but the low word
alone is not a pointer or a validity test: an address at a bank's offset
zero is valid. The 18 pointers use 72 bytes in existing direct-page gaps:
`$28-$2F`, `$58-$5F`, and `$C0-$F7`. The two-byte ready flag is at `$F8`.
The compiler's `$30-$57` pseudo-registers and `$60-$BD` room workspace do
not overlap them. Link-budget checks enforce these boundaries.

Only `SECTOR` is copied through DMA channel 0 to `$7F0000`; its bound pointer
then refers to WRAM. Movable blocks can still change floor heights. Other
tables remain read-only ROM, including links, static geometry, aim and
navigation data. A zero-length sector table skips DMA entirely; a length of
65,536 uses DMA's zero count to copy the whole bank. The source bank and
length come from the bound descriptor, not baked-in per-level constants.

| Level | HOST.BIN | Mutable sectors in `$7F` | Free `$7F` | WRAM recovered |
|---|---:|---:|---:|---:|
| LEVEL1 | 160,918 | 16,104 | 49,432 | 45,386 |
| LEVEL2 | 166,889 | 17,196 | 48,340 | 45,209 |
| LEVEL3A | 185,068 | 27,364 | 38,172 | not a legacy-build comparison |

Lost Valley's real host image now exceeds 64 KiB. It contains 91 rooms,
6,841 sectors and 995 navigation boxes. Its `-39` slope previously failed
the five-bit packer check; the FX3 byte encoding removes that software
restriction, without enlarging either CPU or FX hardware address windows.

The existing entity snapshot/publication boundary remains necessary:
simultaneous hardware access does not make a multi-byte update atomic.

## Verification

Python tests cover aggregate images larger than 128 KiB, full-bank and
empty tables, malformed directories/manifests, linker chunk boundaries,
direct-page placement and the distinct CPU/FX address windows, plus shared
frame records and invalid frame offsets/counts/aliases.

Frame-contact cartridge checks compare every source tick (7,681 in Lost
Valley), 1,668 additional bank-start/end frame reads in CPU-only `$F8`,
8,463 original room-adjacency pairs and 1,200 full-width contact/push cases.
Another 480 complete native/C collision-phase snapshots cover 112 contacts
and 54 accepted pushes. Products/rotated coordinates and grown bounds stay
32-bit; only a validated push point and nonnegative edge distances narrow
to signed/unsigned words. Native readers/rotation/range checks run from
FastROM and retain C references. NMI never uses their math workspace.

The cartridge test `rebinds host tables across banks and CPU-only FX3 ROM`
relocates all 18 tables independently through `$E1-$F2`, both at bank starts
and at end-aligned offsets. It compares item loading, room/sector queries,
bounds, aim points and render snapshots; verifies normal CPU writes cannot
modify ROM; checks mutable floors, full-bank and empty DMA; and rejects
invalid directories. The zone fixture compares all six packed arrays with
an independent export of original level data, then moves shared/distinct
maps to the start/end of CPU-only bank `$F8`. It checks 54 complete ordered
union lists across all families, both flips and unsigned zone IDs.
`motioncheck` compares native code with actual C-reference cartridges,
including 1,404 complete Nav initialization snapshots over 39 lengths and
384 admission/reclamation snapshots. Another 864 original C++ queue/cell/
waypoint snapshots and 1,024 mood/PRNG snapshots exercise all three families,
both maps and flips during live searches. The C++ adapter reads the actual
six maps rather than duplicating the normal maps as alternate data.
The geometry audit compares every packed sector with raw source FloorData,
reading immutable ROM sectors so movable-block initialization cannot change
the expected input. Another 8,448 queries exercise all 256 signed slopes,
tile-boundary points, cache hits and three output-pointer banks; 1,409 cases
need a 17-bit intermediate product.
Live timing gates retain the four-slot
queue, five-tick drift bound, zero-drop requirement and sixteen-NMI render
publication deadline.
LEVEL1's 600-field wolf and mixed encounters pass. The current 3,600-field
three-wolf run also passes, but the mixed run fails at 1,799 ticks / one
drop after the navigation cursor cleanup (down from four). The remaining
drop is in a camera trace at field 1,736; earlier zero-drop mixed extended
results predate current-frame contact.
Lost Valley's new three-raptor
gate currently fails during early pursuit (282 ticks and 17 drops in 600
fields); larger-level timing is not established merely by fitting its data.
The additional three-bat room-14 workload passes its unchanged 600-field
gate at 296 ticks / zero drops, with all owners retained and 1,128 damage.

These are emulator/interpreter checks. They do not establish FX3 firmware
conversion costs or timing on a physical cartridge.

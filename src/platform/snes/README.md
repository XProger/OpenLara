# OpenLara — SNES / Super FX

A Super Nintendo target driven by the Super FX (GSU) co-processor.

## Current release: LEVEL1 and LEVEL2

The release target is parity with the **GBA alpha's first two levels**.
This pass polishes the existing FX3 port; the full-campaign backlog below
does not expand that scope. Physical FX3 qualification is still outstanding.

Build the two-level cartridge with:

```sh
make BUILD=build_fx3_ship FX3=1 LEVELS='LEVEL1 LEVEL2' -j8
```

## 2026-10-08: exact frame-interpolated camera bounds

FX3's camera now follows the **current interpolated animation bounds**, as
GBA does, instead of the union of every pose in the animation. This covers
follow height, the middle of a fixed/look target, and X/Z centering during
block pushes and climbs. Animation poses themselves still need matrix
interpolation; this change closes the separate camera-bounds gap.

The packer evaluates upstream's interpolation with its exact reciprocal
rounding, including `divTable[2] = $7FFF`, then stores four scalar streams
per animation in `CAMERA.BIN`. Identical streams share storage, constants
need one signed word, and varying values use only the bits their range
requires. The native reader and the C reference decode these immutable
CPU-ROM streams. No interpolation arithmetic or new persistent RAM is
needed at runtime. The two tables add **29,450 and 31,122 bytes** and fit the
existing 4 MiB cartridge. Native CPU code recovers 328 bytes; general BSS
still has 2,003 bytes free and resident host code 32.

The oracle extracts `getBoundingBox(true)`, `getFrames` and the reciprocal
table directly from the shared GBA engine and runs them on the original
PHD files. All **15,161 animation ticks / 60,644 offsets** agree exactly.
Native/C checks also cover widths zero through sixteen, bit alignments,
constant/missing streams, invalid indices, far/bank-wrapped item fields,
and wrapped world Y. Contact bounds keep their existing nearest-frame rule.
The GSU renderer and GSU-2 cartridge remain byte-identical.

The full gameplay runs pass **136 LEVEL1 checks and 98 LEVEL2 checks**,
including complete C comparisons, both underwater switches, block pushing,
title/passport flow, saves, level handoffs and sound. All 78 Python checks
pass. The opening retains **17.3 fps**, its original pixel CRC `ebabee54`,
and zero VRAM writes outside blanking. Nine near-camera renderer views
also pass; the full pipeline restores display by line 33.

Against the preceding $5636 two-level cartridge:

| LEVEL1 scene | Display flips, before → after | FPS, before → after | Maximum frame gap, before → after |
|---|---|---|---|
| Three wolves, 600 fields | 107 → 105 | 10.7 → 10.5 | 14 → 12 fields |
| Two wolves and a bear, 600 fields | 96 → 98 | 9.6 → 9.8 | 16 → 14 fields |
| Two wolves and a bear, 1,800 fields | 267 → 269 | 8.90 → 8.97 | 16 → 16 fields |
| Two wolves and a bear, 7,200 fields | 1,401 → 1,396 | 11.68 → 11.63 | 16 → 18 fields |
| Busy-period input replay, 2,160 fields | 314 → 316 | 8.72 → 8.78 | 16 → 16 fields |

All samples have zero dropped ticks. The long mixed run completes the same
3,599 ticks, spends 0.48% more engine clocks and loses 0.36% of displayed
throughput. Maximum logic lag rises from 4.5 to five ticks. This camera
correction does not resolve frame pacing. All
**1,079 input-replay states** retain exact items, three navigation contexts,
random state and queued input; camera/target states intentionally change.

The playable ROM is **`build_fx3_ship/openlara.sfc`**, checksum **$19D5**,
SHA-256 `876333c53969c3bedf621e9b53680b56b20afbccde8c30e1165f5888d51c4c34`.
It matches the independently built `build_fx3_camerapose/openlara.sfc`.
Validation and performance evidence is in `build_fx3_camerapose/evidence/`.
Physical FX3 qualification, animation pose interpolation, remaining
lighting/audio work and complete normal playthroughs remain outstanding.

## 2026-10-08: less CPU work in creature navigation

Native FX3 navigation reuses the starting corridor's already-copied bounds,
copies its fixed sixteen-byte workspace without a loop, and writes each
waypoint through the Nav pool's known WRAM bank. It retains the original
inside/outside tests, clipping order, path traversal and five-step search
budget. No extra RAM is used; bank $87 spends 38 ROM bytes, leaving 900 free.

Against the preceding $2DDC two-level cartridge:

| LEVEL1 scene | Display flips, before → after | FPS, before → after | Maximum frame gap, before → after |
|---|---|---|---|
| Three wolves, 600 fields | 105 → 107 | 10.5 → 10.7 | 13 → 14 fields |
| Three wolves, 1,800 fields | 287 → 289 | 9.57 → 9.63 | 15 → 14 fields |
| Two wolves and a bear, 600 fields | 97 → 96 | 9.7 → 9.6 | 14 → 16 fields |
| Two wolves and a bear, 1,800 fields | 267 → 267 | 8.9 → 8.9 | 17 → 16 fields |
| Two wolves and a bear, 7,200 fields | 1,384 → 1,401 | 11.53 → 11.68 | 17 → 16 fields |
| Busy-period input replay, 2,160 fields | 316 → 314 | 8.78 → 8.72 | 17 → 16 fields |

The long mixed run gains **1.23% displayed throughput**, retains all 3,599
ticks and spends 0.83% fewer engine clocks. All runs have zero dropped ticks.
Maximum logic lag falls from five to 4.5 ticks in the long mixed and input
runs. All **1,079 input-replay snapshots remain exact**. Smaller observation
windows do not all improve: the first mixed ten seconds lose one frame and
gain two fields in their worst pause. Publication timing remains discrete,
and further pacing work is still required.

The isolated waypoint comparison preserves **576 complete native/baseline/C
snapshots**, including clipping workspace, invalid and cyclic paths, signed
coordinate extremes, all three Nav pools and source points in either WRAM
bank. It uses 13,428,448 master clocks versus 14,089,836 before (**4.69% less**).
The existing GBA C++ oracle still matches 864 complete queues and waypoints.
The 49 selected LEVEL1 gameplay checks pass, including creature updates,
corpse expiry, save/load, level handoffs and the 17.3-fps opening pipeline.
Another 30 selected LEVEL2 checks pass against its own C reference where
applicable, including complete creature updates and underwater interactions.
The GSU image and opening pixel CRC remain unchanged; the rebuilt GSU-2
cartridge and LEVEL1 C reference are byte-identical to their predecessors.

`tools/typesnes_navwaypoint.test.ts` is included in `paritycheck`. Set
`OL_REFERENCE_ROM` / `OL_REFERENCE_SYMS` to a C oracle and
`OL_NAV_BASELINE_ROM` / `OL_NAV_BASELINE_SYMS` to the preceding native ROM
to run its before/after comparison. Other fixtures keep their independent
performance baselines. A single-level C oracle can now compare LEVEL2 of a
two-level cartridge: fixture setup selects its local index zero and verifies
the packed LevelID against the requested level's data, rejecting a wrong
reference before any gameplay comparison.

The frozen navigation checkpoint is **`build_fx3_navpolish/openlara.sfc`**, checksum **$5636**,
SHA-256 `fe03fd5f2319e48392890643855941d96da96407430b5ee8dc2b8a22f5cceb60`.
The independent LEVEL1/LEVEL2-selected builds produce the same cartridge.
Evidence is in `build_fx3_navpolish/evidence/`. These are emulator results;
physical FX3 qualification and the remaining parity work in TODO.md are
still outstanding.

## 2026-10-08: shorter pauses during creature encounters

Native staged FX3 now gives a ready render snapshot an **eight-field
publication deadline** under a logic backlog, down from sixteen. Once it
expires, the next logic tick boundary publishes the snapshot. A focused trace
found that the previous worst pause included a completed upload followed
by a long wait to start the next render. Queue priority, the four-tick cap,
controller samples, DMA ownership and the game update order are unchanged.
The C reference and non-staged pipelines keep their sixteen-field limit.

Against the preceding $2DE4 two-level cartridge:

| LEVEL1 scene | Display flips, before → after | FPS, before → after | Maximum frame gap, before → after |
|---|---|---|---|
| Three wolves, 600 fields | 105 → 105 | 10.5 → 10.5 | 15 → 13 fields |
| Three wolves, 1,800 fields | 285 → 287 | 9.50 → 9.57 | 16 → 15 fields |
| Two wolves and a bear, 600 fields | 97 → 97 | 9.7 → 9.7 | 14 → 14 fields |
| Two wolves and a bear, 1,800 fields | 267 → 267 | 8.9 → 8.9 | 23 → 17 fields |
| Two wolves and a bear, 7,200 fields | 1,384 → 1,384 | 11.53 → 11.53 | 23 → 17 fields |

The long mixed run retains **3,599 completed ticks, zero dropped ticks and
maximum logic lag five**. Its longest displayed pause falls from about
383 ms to 283 ms without sacrificing total throughput. The opening remains
17.3 fps with zero VRAM writes outside blanking. These are TypeSNES results;
physical FX3 qualification and further pacing work remain outstanding.

The short input replay preserves **300 exact gameplay snapshots**. A second
replay waits until field 1,701 before sending short pad taps into the busy
period: all **1,079 completed snapshots** match the previous ROM, including
items, navigation, camera, target, random state and input. That run publishes
with four ticks queued and reduces its worst frame gap from 24 to 17 fields.

Seven staging checks pass, including **2,500 publication decisions and
9,909 real NMI interruption boundaries**, immutable render snapshots, upload
source ownership and all 31,104 staged bytes. A deliberately saturated
queue now forces 22 publications within seven or eight observed field
increments, including counter rollover. Five live boot/pipeline/title/level
handoff checks and six LEVEL2 checks pass; the latter exercise swimming,
both underwater switches, pickups, doors, block pushing, effects and HUD.
The broader 32/71 gameplay audit below belongs to the preceding checkpoint.

The frozen two-level checkpoint is **`build_fx3_pacing_final/openlara.sfc`**, checksum
**$2DDC**, SHA-256
`745686c856d433d108bd9e85f5c7f55cb4bcc29638cff5ff1899f94f0344d827`.
The source build matches the isolated experiment byte for byte. Only one
host-code byte and two checksum bytes differ from $2DE4; the GSU, engine
code, assets and all memory budgets are unchanged. GSU-2 stays identical.
Evidence is in `build_fx3_pacing_final/evidence/`.

For a focused capture, set `OL_PROFILE_FOCUS=1700:2100` and
`OL_PROFILE_FIELDS=2160` when running the creature profiler. The observation
adds no emulated work: its complete tick/display timeline was checked
against the previous unfocused run. `OL_PROFILE_INPUT_START=1701` delays
the existing `OL_PROFILE_INPUT=pulses` replay into this workload; omitted,
it retains the original input pattern from field one.

## 2026-10-08: exact joint-rotation optimization

FX3's joint rotations keep both matrix elements in registers and reuse each
sine/cosine load across two products. The four signed products retain their
separate high-word truncations, wrapping, sums and doubling. This reduces
the rotation routine's modeled cycles by **18.1%**, with no extra RAM or ROM
and no change to its results. Padding preserves every later GSU address and
cache alignment. The opening frame falls from 3,647,219 to **3,630,587**
reference GSU cycles; its pixel CRC stays **ebabee54**.

Against the preceding $5546 two-level cartridge:

| LEVEL1 scene | Display flips, before → after | FPS, before → after | Maximum frame gap, before → after |
|---|---|---|---|
| Three wolves, 600 fields | 105 → 105 | 10.5 → 10.5 | 17 → 15 fields |
| Three wolves, 1,800 fields | 286 → 285 | 9.53 → 9.50 | 17 → 16 fields |
| Two wolves and a bear, 600 fields | 96 → 97 | 9.6 → 9.7 | 12 → 14 fields |
| Two wolves and a bear, 1,800 fields | 264 → 267 | 8.80 → 8.90 | 20 → 23 fields |
| Two wolves and a bear, 7,200 fields | 1,378 → 1,384 | 11.48 → 11.53 | 21 → 23 fields |
| Input replay, 600 fields | 95 → 97 | 9.5 → 9.7 | 11 → 13 fields |

All runs have **zero dropped ticks**. The changed renderer completion times
shift CPU/upload scheduling: the mixed scene gains throughput but its worst
pause and maximum logic lag grow (21 → 23 fields, four → five ticks). Wolf
pauses shorten while its 1,800-field total loses one frame. The opening
pipeline remains **17.3 fps**, with zero writes outside a VRAM blank. These
emulator results do not establish physical FX3 timing or solve frame pacing.

`make BUILD=build_fx3_matrix FX3=1 LEVELS='LEVEL1 LEVEL2' matrixcheck`
checks every byte angle and signed matrix edges in both gsusim and TypeSNES:
**24,576 cases in each**, including untouched columns, translations,
registers and memory guards. A separate comparison against $5546 matches
all 294,912 matrix elements exactly. Animation silhouettes, independent
arm/look poses and all nine near-camera views also pass. `animcheck` now
reads the multi-level cartridge's relocated host directory correctly.
The 604-field input replay preserves all **300 completed tick snapshots**
available in both runs; the previous build completes one additional sampled
tick within that observation window.

The **32 selected LEVEL1 gameplay/presentation checks** and complete
**71-check LEVEL2 suite** pass on this ROM, including real combat, inventory,
camera movement, level transitions, saves and both underwater switches.
The earlier 117-case LEVEL1 audit below belongs to the $5546 checkpoint;
this renderer-only follow-up does not claim to have repeated that full run.

The frozen rotation checkpoint is **`build_fx3_matrix/openlara.sfc`**, checksum
**$2DE4**, SHA-256
`2a688b6f1d92a4eb5b134e82dfd12193907c415e54aa3917c7a02c4d3454feae`.
Two independent two-level builds match. Only the two GSU rotation loops and
the cartridge checksum differ from $5546 (214 bytes); CPU code and packed
assets are identical. GSU-2 remains byte-identical. Evidence is in
`build_fx3_matrix/evidence/`.

Animation interpolation remains open: GBA blends joint matrices, and
precomputing all intermediate matrices would exceed this cartridge's
current data layout. This pass reduces the cost of the existing rotations;
it does not change animation sampling or camera bounds.

## 2026-10-08: GBA corpse lifetime and retrigger behavior

Wolves, bears and bats now finish their death animations and disappear after
the GBA's **300-update corpse countdown**. Finished bodies release navigation
and keep their final pose until expiry. Save/load preserves the remaining
countdown, including a bear saved during its longer death animation.

The existing item timer holds the countdown, so this adds **no RAM**. Floor
triggers preserve that word for dead enemies; they still refresh ordinary
object timers. Trigger activation now follows GBA's guard: only an admitted
actor with status NONE becomes ACTIVE. Repeated pads cannot reset or reveal
a corpse, and actors waiting for a navigation slot remain invisible.

All three creature types pass the complete countdown on **both levels**,
with repeated timed pads, save/load, navigation release, active-list integrity
and the renderer's hidden marker checked. The real LEVEL1 combat replay also
shoots a bat, observes its falling death and verifies that it disappears.
The native dispatcher matches **2,520 complete C snapshots**, including
expiry boundaries, admission failures and far Lara pointers. The input
replay retains all **301 baseline tick snapshots** over 604 fields.

The final audit passes **117 distinct LEVEL1 checks** (the complete normal
suite plus eight native/C comparisons) and the complete **71-check LEVEL2
suite**. LEVEL1 was split into targeted, remaining and encounter runs;
`release-audit/test-coverage.json` records every case and confirms that all
106 checks from the preceding full audit remain covered.

Performance against the original $A071 checkpoint:

| LEVEL1 scene | Display flips, before → now | FPS, before → now | Maximum frame gap, before → now |
|---|---|---|---|
| Three wolves, 600 fields | 105 → 105 | 10.5 → 10.5 | 13 → 17 fields |
| Three wolves, 1,800 fields | 284 → 286 | 9.47 → 9.53 | 16 → 17 fields |
| Two wolves and a bear, 600 fields | 95 → 96 | 9.5 → 9.6 | 15 → 12 fields |
| Two wolves and a bear, 1,800 fields | 260 → 264 | 8.67 → 8.80 | 23 → 20 fields |
| Two wolves and a bear, 7,200 fields | 1,366 → 1,378 | 11.38 → 11.48 | 23 → 21 fields |

All have zero dropped ticks. The long mixed run completes 3,600 ticks with
maximum logic lag four. Compared with the immediately preceding $85A2
polish build, the long run displays two fewer frames (1,380 → 1,378) while
its longest pause falls from 23 to 21 fields. These are emulator results;
the wolf-only worst pause still needs work. The opening scene retains
17.3 fps and the unchanged reference image. GSU-2 stays byte-identical.

The frozen corpse-lifetime checkpoint is **`build_fx3_corpse_final/openlara.sfc`**,
checksum **$5546**, SHA-256
`5a118787ad0b070749a1546bea53a9ab679dd5d02c5ae99d3e6c8adffc3628e2`.
Two independent two-level builds match it. General BSS still has 2,003 bytes
free and resident host code 32; C banks $84 and $88 have 81 and 50 bytes.
The C reference moves its small enemy dispatcher into $87 to fit the added
behavior. Evidence is in `build_fx3_corpse_final/release-audit/`.

## 2026-10-08: first-two-level performance checkpoint

Native FX3 avoids recomputing an unchanged health-bar width and shortens
small unsigned long products, including direct zero/unit results. Both
preserve the original arithmetic. Against the preceding $A071 build:

| LEVEL1 scene | Display flips, before → after | FPS, before → after | Maximum frame gap, before → after |
|---|---|---|---|
| Three wolves, 600 fields | 105 → 105 | 10.5 → 10.5 | 13 → 17 fields |
| Three wolves, 1,800 fields | 284 → 286 | 9.47 → 9.53 | 16 → 17 fields |
| Two wolves and a bear, 600 fields | 95 → 97 | 9.5 → 9.7 | 15 → 15 fields |
| Two wolves and a bear, 1,800 fields | 260 → 266 | 8.67 → 8.87 | 23 → 23 fields |
| Two wolves and a bear, 7,200 fields | 1,366 → 1,380 | 11.38 → 11.50 | 23 → 23 fields |
| Input replay, 600 fields | 95 → 96 | 9.5 → 9.6 | 12 → 10 fields |
| Settled idle, 600 fields | 173 → 173 | 17.3 → 17.3 | 4 → 4 fields |

These are actual TypeSNES display flips. The mixed samples share the same
two-minute run. Every run has zero dropped ticks; the long mixed run
completes 3,600 ticks and its maximum logic lag falls from five to four.
The short wolf scene retains its frame count but has a longer worst pause;
this is a modest throughput gain, not uniformly smoother frame delivery.
Resolution, rendering and the game clock are unchanged. All 299 captured
input-replay gameplay snapshots match the preceding build.

The health-bar check compares 5,805 complete C snapshots, including healing
in the inventory, oxygen and level resets. An unchanged bar takes 181
instructions instead of 360. Product checks cover over a million word
cases, 20,082 wide cases and 513 real NMI interruption boundaries. The full
native/C parity suite passes 75 checks; the Python suite passes 78. The
GSU-2 ROM, FX3 C oracle and GSU program remain byte-identical.

The same two-level ROM passes **106 LEVEL1 checks and 68 LEVEL2 checks**
(67 in the full LEVEL2 suite plus its new switch regression). The audit
covers title/start, END transitions, passport/death, save/load across levels
and power cycles, settings, sound, HUD and gameplay.
A new regression exercises both real LEVEL2 underwater switches from the
approach swim through their lever animations and door triggers. Animation,
matrix, weapon, near-plane and opening-frame oracles pass. Two stale test
inputs were corrected: the animation fixture now supplies camera target
height, and the camera replay counts an NMI-interrupted entry only once.
Camera arithmetic and per-tick expectations remain exact.

Explicit secondary GSU targets fix the macOS GNU make 3.81 incremental
parallel-build hang. Fresh builds, repeated builds and BOOT_LEVEL changes
produce the same two-level ROM. The HUD cache uses four bytes; general BSS
has 2,003 bytes free, resident host code 32, and the tightest C bank 136.
The actual placed room/model data in both levels fit the renderer's vertex,
face and matrix-stack limits; this audit does not include inventory-only
models or prove every rasterizer path.

At this checkpoint, remaining GBA differences included corpse expiry (now
addressed above), pose interpolation,
frame-interpolated camera bounds, lighting/water presentation and audio
coverage. These remain release work in TODO.md; this candidate is not yet
claimed to have complete GBA parity or hardware qualification.

The preserved two-level ROM is `build_fx3_polish/evidence/after/two-level.sfc`, checksum
**$85A2**, SHA-256 `dd9029514f85fd3cc2af1f4ad0e02a533e303ec109ccbc0b9871f98f1a62c2f7`.
Two independent builds match it. The single-level benchmark ROM is
`build_fx3_polish/openlara.sfc`, checksum **$44AE**. Profiles, variants,
audit results and frozen evidence are under `build_fx3_polish/evidence/`.

## 2026-10-08: another 25% in FX3 encounters

Native FX3 now displays **25–28% more frames** in all four encounter
benchmarks against the publication-scheduling build below. Resolution,
shading, creature updates and the 30 Hz game clock are unchanged.

| LEVEL1 scene | Display flips, before → after | FPS, before → after | Gain | Maximum frame gap |
|---|---|---|---|---|
| Three wolves, 600 fields | 82 → **105** | 8.2 → **10.5** | **28.0%** | 22 → **13 fields** |
| Two wolves and a bear, 600 fields | 74 → **95** | 7.4 → **9.5** | **28.4%** | 25 → **15 fields** |
| Two wolves and a bear, 1,800 fields | 208 → **260** | 6.93 → **8.67** | **25.0%** | 26 → **23 fields** |
| Two wolves and a bear, 7,200 fields | 1,068 → **1,366** | 8.90 → **11.38** | **27.9%** | 28 → **23 fields** |

These count actual NMI display flips in TypeSNES. The shorter mixed samples
are prefixes of the same two-minute run. Both long runs complete **3,599
logic ticks with zero drops**, with the same maximum five-tick lag at field
boundaries. Settled idle improves 17.1 → **17.3 fps**. Physical FX3 timing
still needs verification; the emulator does not model that hardware timing.

Completed frames can now enter the direct upload from NMI/IRQ without
waiting for a long logic call to return. The next render's camera and item
snapshot is prepared at a tick boundary, then launched after the last planar
DMA read; the remaining tilemap transfer can overlap it. Atomic claims
prevent nested interrupts from queuing or launching a job twice. WRAM
staging is used when the logic queue has headroom. A tick taking three or
more fields starts a four-field guard that suppresses staging and reserves
publication headroom. The sixteen-field publication deadline and existing
overload fallback remain intact.

CPU savings come from native common camera follow/handoff paths, dry-room
water queries, near WRAM query outputs, hoisted creature-scan values and a
shorter room-cache hash. Compiler postprocessing shortens bounded accesses
to verified WRAM globals and signed 9–14-bit shifts. Far pointers, aliases,
wrapped arithmetic and uncommon camera/water paths retain exact C behavior.
The camera handoff uses **39% fewer instructions** in its comparison matrix,
which validates 384 complete C snapshots. No new gameplay cache or reduced
update rate is used.

Validation includes **74 parity checks**, **28 focused runtime checks**,
**seven LEVEL2 checks**, **78 Python checks**, exhaustive arithmetic and
native/C comparisons. Upload tests cover 2,000
publication cases, 7,802 real NMI boundaries, 102 completed-frame claim
boundaries, 99 deferred-launch boundaries and 108 exact column transfers,
with zero torn or out-of-blank VRAM writes. Input replay preserves all **299
baseline gameplay snapshots** (four extra observation fields allow its last
tick to complete); the equal-duration replay also passes and improves
7.7 → **9.5 fps**. LEVEL2 water and blade checks pass with the default bank
layout, resolving the earlier bank-$84 capacity limitation.

Lost Valley's existing raptor overload improves **109 → 144 displayed
frames**, **873 → 881 ticks**, and **26 → 18 dropped ticks** over 1,800 fields.
It still fails the original timing gate; that gate has not been relaxed.

The pipeline adds a **two-byte prepared-entry slot**. General BSS retains
**2,007 bytes free**, and resident host code shrinks by nine bytes, leaving
**32 bytes free**. The existing 31,104-byte staging allocation and upper-WRAM
budgets are unchanged. Independent builds produce the same ROM, checksum
**$A071**. The GSU program, opening pixels, GSU-2 ROM and FX3 C oracle remain
byte-identical to the baseline.

The playable ROM is `build_fx3_perf_final/openlara.sfc`. Frozen before/after
ROMs, source snapshots, profiles, hashes, checks and rejected experiments
are in `build_fx3_perf_final/framerate_fx3_25/`; `comparison.json` records
all four unchanged 25% thresholds and the results. Reproduce the long run:

```sh
OL_PROFILE_FIELDS=7200 OL_PROFILE_SPECIES=7,7,8 \
  make BUILD=build_fx3_perf_final FX3=1 check TESTS='profiles the full tick'
```

## 2026-10-08: FX3 publication scheduling

Native FX3 can now publish a completed render with up to two logic ticks
queued. The main loop still runs a tick first, and defers publication from
three queued ticks to leave NMI headroom. Once the dropped-tick gauge is
nonzero, it returns to the original empty-queue policy; level initialization
resets that gauge. The sixteen-field deadline, controller FIFO, resolution,
image quality and direct/staged upload ownership are unchanged.

Actual TypeSNES display flips against the room-cache/product build below:

| LEVEL1 scene | Before → after | Maximum visible-frame gap |
|---|---|---|
| Three wolves, 600 fields | 65 → **82**, 6.5 → **8.2 fps** | 26 → **22 fields** |
| Two wolves and a bear, 600 fields | 61 → **74**, 6.1 → **7.4 fps** | 29 → **25 fields** |
| Two wolves and a bear, 1,800 fields | 163 → **208**, 5.43 → **6.93 fps** | 29 → **26 fields** |
| Two wolves and a bear, 7,200 fields | 967 → **1,068**, 8.06 → **8.90 fps** | 29 → **28 fields** |
| Settled idle, 600 fields | 150 → **171**, 15.0 → **17.1 fps** | 5 → **5 fields** |

The two-minute encounter keeps **3,599 logic ticks and zero drops** in both
builds. The 30-second result is the prefix of that final stress run. Its
larger gain (28%) reflects the heavier opening portion of the encounter;
the complete two-minute gain is 10%. At field boundaries, maximum
logic lag rises from four to five ticks: this trades some latency for
more displayed frames without discarding ticks or input. Short wolf/mixed
samples finish 299/298 ticks, within the unchanged five-tick drift gate. These are emulator
measurements; physical FX3 timing still needs verification.

The controller replay improves 5.7 → **7.7 fps**, with all **299 per-tick
snapshots exact**: queued input, items, three navigation contexts, camera,
target and PRNG. The publication fixture covers **400 queue/deadline/upload
cases and 1,344 real NMI boundaries**, including the overload fallback and
16-bit drop-counter boundaries. All four staging checks pass, including
108 exact column uploads with zero torn or out-of-blank VRAM writes.
The final build also passes 24 focused gameplay checks, five LEVEL2
water/blade checks and 21 memory-budget checks.

The change adds **16 resident host-code bytes**, leaving **23 bytes** in
that region, with no additional data allocation. General BSS retains
2,009 free bytes and upper WRAM 16,688. The GSU program, GSU-2 production
ROM and FX3 C reference ROM remain byte-identical. The playable ROM is
`build_fx3_perf_final/openlara.sfc`, checksum **$CE6D**.

Two experiments were rejected: admitting three queued ticks drops one tick
in the long LEVEL1 encounter; admitting two unconditionally increases
Lost Valley's existing raptor overload from 26 to 28 drops. The final
fallback keeps the original raptor result of **873 ticks / 26 drops** over
1,800 fields (110 → 109 displayed frames, maximum gap 30 → 31 fields).
Both original and final builds fail the existing raptor clock gate; this
pass does not fix that CPU overload or relax the gate. A fresh LEVEL2 build
also exposes a pre-existing bank-$84 capacity limit. Its five checks use
`C_BANK_camera_cold=6` to relocate cold camera helpers for validation; the
default bank assignment is unchanged.

Baseline and final ROMs, threshold experiments, profiles, source snapshots,
hashes and logs are in `build_fx3_perf_final/framerate_fx3_schedule/`.
Reproduce the final stress run with:

```sh
OL_PROFILE_FIELDS=7200 OL_PROFILE_SPECIES=7,7,8 \
  make BUILD=build_fx3_perf_final FX3=1 check TESTS='profiles the full tick'
```

For per-tick comparison, set `OL_PROFILE_INPUT=pulses` and
`OL_PROFILE_TICK_TRACE=1` on a baseline run, then set
`OL_PROFILE_REFERENCE_TRACE` to its profile JSON on the candidate run.
`make paritycheck` includes the publication/NMI fixture.

## 2026-10-08: FX3 room reuse and product carries

Native FX3 now keeps 32 exact room-query records instead of eight. The
table lives beside navigation data in `$7E:5800–$FFFF`, preserving general
BSS space and the existing two-creature admission policy. Full keys,
unaligned-grid bypass and mutable-floor invalidation are unchanged. A
64-entry experiment produced identical frame counts and CPU costs, so
the smaller table is retained.

The dense 16×16 product helper accumulates its middle terms with byte
additions and explicit carry propagation. This removes two helper calls
and temporary stack traffic without changing hardware multiplier waits
or the compiler ABI. The FX3 C reference keeps the preceding arithmetic.

Actual TypeSNES display flips against the preceding FX3 build:

| LEVEL1 scene | Before → after | Maximum visible-frame gap |
|---|---|---|
| Three wolves, 600 fields | 61 → **65**, 6.1 → **6.5 fps** | 26 → **26 fields** |
| Two wolves and a bear, 600 fields | 57 → **61**, 5.7 → **6.1 fps** | 29 → **29 fields** |
| Two wolves and a bear, 1,800 fields | 151 → **163**, 5.03 → **5.43 fps** | 29 → **29 fields** |
| Opening idle, 600 fields | 150 → **150**, 15.0 → **15.0 fps** | 5 → **5 fields** |

The longer encounter displays **8% more frames**. Short encounter samples
complete 299 ticks; the longer sample completes 898 → 899, with zero
dropped ticks in every run. Engine clocks per tick fall about 3–4%.
Resolution, image quality, the 30 Hz clock and performance gates remain
unchanged. These are emulator measurements, not FX3 hardware timings.

Validation covers **127 runtime checks**, **70 native/C parity checks**, over a million exact
compiler-product cases, **366 real NMI interruption boundaries**, and
**21 memory-budget checks**. Room-cache cases retain complete query
metadata, portal/seam behavior, collision rejection, far outputs and
mutation/reset semantics. Boot checks validate both allocations and every
unused WRAM gap; the memory report includes the new cache allocation.
The full run passed 126 cases and flagged the old unused-WRAM assertion;
after updating it for the explicit cache allocation, the boot recheck passed.

The change spends **768 additional WRAM bytes and 10 FastROM bytes**,
leaving **2,009 general BSS bytes** and **16,688 upper WRAM bytes** free.
The GSU-2 production ROM, FX3 C reference ROM and GSU program remain
byte-identical. The FX3 checksum is `$C7C7`.

The playable ROM is `build_fx3_perf_final/openlara.sfc`. Baseline ROM and
symbols, experiments, before/after profiles and verification logs are
retained in `build_fx3_perf_final/framerate_fx3_cache/`. Reproduce the
long encounter with:

```sh
OL_PROFILE_FIELDS=1800 OL_PROFILE_SPECIES=7,7,8 \
  make BUILD=build_fx3_perf_final FX3=1 check TESTS='profiles the full tick'
```

## 2026-10-08: arithmetic overlap and bounded division

GSU-2 now lets the renderer run during dense CPU word products, the
WRAM-only portion of `phd_atan`, and long division with at least four
quotient rounds. Each path retains its own ownership token, so nested
calls restore the caller's bus state. Room queries also recognize bank-zero
WRAM output pointers, with an exact `$1FEC` upper bound for the complete
20-byte record; outputs crossing into I/O retain serial access.

Native 32-bit division aligns the divisor against the dividend instead of
always shifting it to bit 31. This removes leading zero quotient rounds
while preserving full-width unsigned comparisons, signed minima, wrapped
overflow, remainders and the old zero-divisor behavior. The C oracle keeps
the original implementation. Across 47,888 paired arithmetic cases, CPU
master-clock cost falls **17.8% on GSU-2**, including its handoff overhead,
and **20.7% on FX3**.

Actual TypeSNES display flips compared with the preceding overlap pass:

| GSU-2 LEVEL1 scene | Before → after | Maximum visible-frame gap |
|---|---|---|
| Three wolves, 600 fields | 14 → **15**, 1.4 → **1.5 fps** | 66 → **56 fields** |
| Two wolves and a bear, 600 fields | 12 → **14**, 1.2 → **1.4 fps** | 76 → **58 fields** |
| Two wolves and a bear, 1,800 fields | 30 → **35**, 1.00 → **1.17 fps** | 80 → **68 fields** |

The longer encounter displays **17% more frames**. The samples retain
299/899 logic ticks and zero drops, at the same resolution, image quality
and 30 Hz logic clock. CPU engine clocks per tick increase 2.3–3.4% because
of the handoffs; overlapping that work with rendering improves presentation.
FX3's short samples change from 6.0/5.7 to **6.1/5.7 fps**, with maximum
gaps of 26/29 fields, within the existing gates. These are emulator results;
GSU-2 encounters still need further improvement for smooth play.

The new division fixture checks all 65,536 word inputs across signed and
unsigned quotient/remainder, plus full-width arithmetic against `BigInt`
and the retained runtime. Hardware product, angle and division checks inject
real NMI entry at **18,458 instruction boundaries**, including nested bus
ownership, short quotients, zero divisors and multiplier latency. The output
guard adds 32 exact bank/mirror-boundary/owner/mask cases. Existing gameplay
benchmarks continue auditing actual cartridge-RAM accesses and ownership.
The final ROM passes **55 gameplay checks**, **39 GSU-2 native/C parity
checks**, **69 FX3 parity checks**, **12 camera/collision comparisons** and
**six LEVEL2 checks**. Across the three GSU-2 encounter samples,
**1,649,205 CPU cartridge-RAM accesses have zero ownership violations**.

The change spends **81 resident code bytes**, leaving **13,372 free**
(348 / 5,624 / 6,148 / 1,199 / 53), with **260 BSS bytes free** and unchanged
stack limits. The 32-entry room cache is retained: a measured 64-entry
experiment produced the same frame counts with negligible CPU differences. Both
GSU program banks and both complete C reference ROMs remain byte-identical.
The GSU-2 ROM checksum is `$CF51`.

Before/after profiles, ROM hashes and final verification logs are in
`build_gsu2_perf/framerate_arithmetic/`. `make paritycheck` includes the
new arithmetic and interrupt fixtures. For a focused cost comparison, set
`OL_ROM`/`OL_SYMS` to the new ROM and `OL_BASELINE_ROM`/`OL_BASELINE_SYMS`
to the preceding ROM, then run:

```sh
python3 tools/run_typesnes.py --typesnes "$HOME/dev/typesnes" \
  --runner tools/typesnes_division.test.ts --log build_gsu2_perf/division.log
```

## 2026-10-08: room reuse and concurrent GSU rendering

GSU-2 now keeps 32 resolved room-query records in resident WRAM. Exact
world-sector keys allow both cache levels to answer before reading the
cartridge's room/sector tables. Every visited grid must be aligned;
unaligned grids bypass reuse, full keys reject hash collisions, and
`Room_reset` invalidates the records after geometry changes. FX3 retains
its eight records and creature-admission policy, with the earlier lookup.

During the enemy-avoidance scan and room-result slope calculations, the
CPU temporarily returns cartridge RAM to the GSU. These regions execute
entirely in WRAM. Cartridge output pointers retain serial access, and both
normal and early exits restore ownership. The desired owner changes before
SCMR so NMI restores the correct state. The host's existing direct-page ABI
now lives in `src/host_dp.inc`, shared with the resident handoff helpers.

Actual TypeSNES display flips compared with the collision pass below:

| GSU-2 LEVEL1 scene | Before → after | Maximum visible-frame gap | Engine master clocks/tick |
|---|---|---|---|
| Three wolves, 600 fields | 10 → **14**, 1.0 → **1.4 fps** | 82 → **66 fields** | 496,615 → **467,234** (−5.9%) |
| Two wolves and a bear, 600 fields | 8 → **12**, 0.8 → **1.2 fps** | 100 → **76 fields** | 508,492 → **481,960** (−5.2%) |
| Two wolves and a bear, 1,800 fields | 22 → **30**, 0.73 → **1.00 fps** | 104 → **80 fields** | 503,311 → **480,127** (−4.6%) |

The short samples complete 299 logic ticks and the long sample 899, all
with zero drops. The longer encounter displays **36% more frames**. The
resolution, image quality, 30 Hz logic clock and full-game performance gates
are unchanged. FX3 improves from 5.9/5.5 to **6.0/5.7 fps**, with maximum
gaps of 26/27 fields. These are emulator results; GSU-2 encounters still
have substantial visible pauses.

Validation includes **55 GSU-2 runtime checks**, **34 parity checks against
the retained C build**, **12 native/C camera and collision checks**, and
**six LEVEL2 checks**. FX3 passes **49 parity checks**, both creature
benchmarks, and seven cache comparisons against its preceding ROM. Both
C reference variants still build. The cache oracle covers 1,368 original
grid probes, 720 repeated portal/seam probes, hash eviction, unaligned
grids, mutations and failure outputs. Handoff tests inject real NMI entry
at **184 instruction boundaries**. The three GSU-2 benchmarks audit
**1,468,326 CPU cartridge-RAM accesses with zero ownership violations**.

Cache admission has a cold-path cost: the camera-query microbenchmark
uses 1,111,992 native instructions versus 999,209 C instructions when it
flushes before every query. Its expanded check preserves all exact cold
outputs and requires two reuses to repay admission; that sequence is
**8.1% cheaper**, with room-only resolution **24.7% cheaper**. Full-game
timing above remains the performance gate.

The records occupy **1,024 bytes plus six control bytes** in the linked,
copied WRAM image. Code and data together spend 1,498 resident bytes,
leaving **13,453 free** (418 / 5,624 / 6,148 / 1,199 / 64). General BSS
retains **260 bytes free** and stack limits are unchanged. Both GSU program
banks remain byte-identical; the opening framebuffer remains CRC `eb23d3da`
at 4,527,603 modeled GSU cycles. The GSU-2 ROM checksum is `$A1CE`.

Before/after JSON profiles, validation logs and ROM hashes are retained in
`build_gsu2_perf/framerate_overlap/`. Reproduce the new cache and interrupt
checks with `make BUILD=build_gsu2_perf paritycheck`; set
`OL_REFERENCE_ROM`/`OL_REFERENCE_SYMS` to a matching `C_MOTION=1` build for
the complete native/C parity set. The longer sample uses:

```sh
OL_PROFILE_FIELDS=1800 OL_PROFILE_SPECIES=7,7,8 \
  make BUILD=build_gsu2_perf check TESTS='profiles the full tick'
```

## 2026-10-08: full GSU-2 collision pass

GSU-2 now runs the complete room-collision wrapper, bridge-floor list and
closed-door collision scan in native 65816 code. The wrapper retains all
four ordered probes, zero-radius and front-only behavior, the C static-hit
response and every collision decision. The bridge helper preserves GSU-2's
existing sector and floor rules. The door scan re-reads Lara after each
push, so later contacts see her updated position. `C_MOTION=1` retains the
complete C implementations.

Compared with the camera/probe pass below, actual TypeSNES display flips are:

| GSU-2 LEVEL1 scene | Before → after | Maximum visible-frame gap | Engine master clocks/tick |
|---|---|---|---|
| Three wolves, 600 fields | 8 → **10**, 0.8 → **1.0 fps** | 108 → **82 fields** | 538,592 → **496,615** (−7.8%) |
| Two wolves and a bear, 600 fields | 7 → **8**, 0.7 → **0.8 fps** | 110 → **100 fields** | 551,282 → **508,492** (−7.8%) |
| Two wolves and a bear, 1,800 fields | 18 → **22**, 0.60 → **0.73 fps** | 118 → **104 fields** | 545,829 → **503,311** (−7.8%) |

The short samples retain 299 logic ticks and the longer sample 899, all with
zero dropped ticks. The longer sample displays **22% more frames** than the
preceding build. The same resolution, renderer, logic clock, scenes and
performance gates remain in use. Encounters still need further improvement
for smooth play; these measurements are from emulation.

Exact comparisons against the retained C build pass 2,048 bridge probes,
2,049 complete room-collision snapshots covering all seven outcomes,
1,024 point/room snapshots and 512 closed-door snapshots. The door fixture
includes list lengths 0, 1, 2, 8, 16, 31 and 32, duplicates, signed
world seams, far pointers and aliased Lara/item records; its 1,445 ordered
contacts include 269 pushes. Existing angle/side checks also pass. The
expanded FX3 bridge oracle accounts for its wrapped platform sectors.
The final cartridge passes **55 runtime checks**, **14 parity checks**,
**six focused native/C checks** and **six LEVEL2 water/diving/interaction
checks**. Both GSU-2 and FX3 C reference variants build successfully.

GSU-2 recovers another **3,080 resident code bytes**, leaving **14,951 free**
(514 / 5,624 / 7,550 / 1,199 / 64). General BSS still has **260 bytes free**,
and the stack boundaries are unchanged. Both GSU program banks are
byte-identical to the preceding build, and rebuilding FX3 produces the
same complete ROM. The GSU-2 ROM checksum is `$BBB2`.

Before/after profiles and comparison logs are retained under
`build_gsu2_perf/framerate_collision/`. To reproduce the focused comparisons,
set `OL_REFERENCE_ROM` and `OL_REFERENCE_SYMS` to a matching C reference and
run `make BUILD=build_gsu2_perf check TESTS='native collision'`.
The longer display sample is:

```sh
OL_PROFILE_FIELDS=1800 OL_PROFILE_SPECIES=7,7,8 \
  make BUILD=build_gsu2_perf check TESTS='profiles the full tick'
```

## 2026-10-08: shared raster updates and native GSU-2 camera/probes

Native GSU-2 now uses the existing exact native camera helpers and collision
point/side helpers. Camera code executes in its resident `$7F:8000` image;
the collision probes live in `$7F:0000`. `C_MOTION=1` retains the complete
C implementations. The normal GSU-2 bridge-floor and room-collision wrappers
are unchanged. The shared rasterizer loads each step before its accumulator,
then uses Super FX `SBK` to write back to that last RAM address. Fractional
carry, texture coordinates, clipping and shading are preserved; all six
cache-anchor alignments are checked by `keycheck`.

Actual TypeSNES buffer flips over the same 600 NTSC fields:

| GSU-2 LEVEL1 scene | Before → after | Maximum visible-frame gap | Engine master clocks/tick |
|---|---|---|---|
| Three wolves | 7 → 8 flips, **0.7 → 0.8 fps** | 124 → **108 fields** | 573,245 → **538,592** (−6.0%) |
| Two wolves and a bear | 6 → 7 flips, **0.6 → 0.7 fps** | 150 → **110 fields** | 585,885 → **551,282** (−5.9%) |

Both finish with 299 logic ticks and zero drops, within the existing drift
limit. The display/drop/drift/attack gates are unchanged. FX3 remains at
15 fps in the opening idle scene and 5.9/5.5 fps in the two creature scenes;
its wolf scene's longest gap falls from 28 to 27 fields. These are emulator
measurements, and the GSU-2 encounters still have long visible pauses.

The longer mixed-creature sample improves from **14 to 18 displayed flips
over 1,800 fields** (**0.47 → 0.60 fps**, 29% more frames), with the maximum
gap reduced from **162 to 118 fields**. It finishes 899 ticks with zero
drops. The old build fails the 0.6 fps gate at this duration; the new build
passes it without changing the threshold or the fixture's timing.

The renderer alone preserves every framebuffer and shade byte across **54
views** (nine views in each shaded/unshaded GSU-2, Mode 7 and FX3 variant).
Opening-frame modelled cycles fall from 4,578,386 to 4,527,603 on GSU-2,
7,292,064 to 7,067,172 on shaded GSU-2, and 3,683,104 to 3,647,219 in FX3's
reference cycle model. Savings across the views range from 0.1% to 6.0%.
`viewcheck.py --fx3` now supports the FX3 reference timing format and the
current opening camera. The six frame baselines retain their image CRCs.

The native camera comparisons include all 65,536 signed movement deltas,
131,864 angle/distance/alias cases, 2,048 complete movement snapshots, 384
follow/snap snapshots, 2,048 query cases and 5,376 independent/C traces.
Collision checks cover every angle, 4,896 complete side records and 1,024
point/room-collision snapshots against the pre-change C code. All **316
span/clip/dispatch fixtures** pass in both GSU cores. The Python suite has
**74 passing tests**, including `SBK` address, carry, flag and bank behavior.
The final GSU-2 build passes **55 runtime checks** and **14 parity checks**;
LEVEL2 adds **five passing water, swimming, key/door and block checks**.
The `C_MOTION=1` reference also builds successfully. The measured before/after
profiles are retained under `build_gsu2_perf/framerate/`.

GSU-2 now has **11,871 resident code bytes free** (1,825 / 5,624 / 3,159 /
1,199 / 64), **5,841 more** than before; general BSS has **260 bytes free**.
No memory or stack boundaries moved. FX3's CPU image is byte-identical.
Reproduce the shared-helper comparisons with `OL_REFERENCE_ROM` and
`OL_REFERENCE_SYMS` pointing at a matching GSU-2 C reference, then
`make BUILD=build_gsu2_perf check TESTS='native camera traces|native collision'`.
The fixture uses unused cartridge RAM for far records on GSU-2, preserving
its resident WRAM code. Per-variant image comparisons are saved as
`build_*/framerate-views.log`.

## 2026-10-07: frame rate and parity progress

The final native TypeSNES creature samples keep the 30 Hz logic clock and
existing queue/drift limits. Actual displayed buffer flips over 600 NTSC
fields give these results:

| Variant / active LEVEL1 scene | Displayed fps / flips | Ticks / dropped | CPU engine master clocks per tick |
|---|---|---|---|
| FX3 / three wolves | **5.9 fps / 59** | **299 / 0** | 602,466 → 561,815, **6.7% less** |
| FX3 / two wolves and a bear | **5.5 fps / 55** | **298 / 0** | 619,061 → 579,494, **6.4% less** |
| GSU-2 / three wolves | **0.1 → 0.7 fps**, 1 → 7 | **300 / 0**, previously 198 / 98 | 1,056,602 → 573,245, **45.7% less** |
| GSU-2 / two wolves and a bear | **0.1 → 0.6 fps**, 1 → 6 | **300 / 0**, previously 190 / 107 | 1,101,172 → 585,885, **46.8% less** |

The final FX3 opening idle view displays **150 flips / 15.0 fps**, up from
12.0 fps. Its wolf and mixed scenes have a maximum visible-frame gap of
**28 fields** each. The GSU-2 gaps are **124 / 150 fields**: logic now meets
the clock, but rendering remains too slow for smooth creature encounters.
The default native LEVEL1 checks require at least **5 displayed fps / at
most 30 fields** on FX3 and **0.6 fps / at most 180 fields** on GSU-2, with
zero drops, at most five ticks of drift, all three actors updating and
attacks occurring. These gates preserve the existing AI and render quality.

An earlier inventory sample improved from **6.6 to 10.3 fps** (66 → 103
flips, **56%**), with 300/301 tick-counter advances and zero drops. All
frame rates here are emulator measurements. The final FX3 build passes
**25 runtime checks and 8 changed parity checks**; earlier broad validation
passed **103 runtime and 58 parity checks**. Final GSU-2 validation passes
**50 runtime and 28 parity checks**, including both persistent pursuit
gates and completed-frame publication under backlog. The current Python
suite passes **70 checks**.

Native FX3's exact eight-entry `Room_query` cache uses **262 BSS bytes**,
activates with at least two admitted Nav owners, and invalidates through `Room_reset`.
World-sector keys distinguish points that clamp to the same hinted-room edge
but reach different portal sectors; unaligned grids use the complete resolver.
Ground movement retains its room/floor/avoidance checks and skips a redundant
navigation-box lookup only while staying in the same valid room sector.
`NB_Units` supplies exact full-width bounds and point coordinates. Native
FX3 shadow publication retains the full stable nearest-seven selection and all
packed records, with **528 complete native/C/independent snapshots**. Both
variants use a **256-byte ROM class table generated from the C specification**,
with all 65,536 type inputs checked against that specification.

Native FX3 inventory publication cuts the complete cold opening call from
50,396 to 31,495 instructions. An exact input cache cuts unchanged text
composition from **23,127 to 703 instructions**, or 81,097 to 2,659 CPU
cycles; FPS writes invalidate it, and shadows remain live. Both paths keep
uncached C versions for full record comparisons.

GSU-2/FX2 now uses native ground movement, the complete wolf update shell,
a **731-byte target-info helper**, a **378-byte shared flood**, exact bounds
and the same fast product helpers as native FX3. Its sector-codec optimization
adds **64 ROM bytes and no work RAM**; an exact 32-entry point-sector
cache uses **256 BSS bytes** and invalidates on `Room_reset` and
`navResetCache`. The cache retains NO_BOX results and keys sector, predictor
stride and sector count. Render completion is serviced between backlogged
logic ticks. Earlier compact helpers and generated classification recovered
**912 resident bytes**. Shared renderer changes save **0.77–1.66%** of
modelled cycles across nine FX2 views with identical framebuffer bytes.
Shaded and unshaded Mode 7 also preserve every pixel while saving about 0.9%.
With the FX3 pixel-loop changes, the opening render falls 2.36%, from
3,772,195 to 3,683,104 cycles; room 14 falls from 8,967,160 to 8,844,609.
The final FX3 build has **9,741 engine-ROM bytes**, **1,747 general-BSS
bytes** and **39 host-assembly bytes** free. GSU-2's five resident code
images have **6,030 bytes** free, plus **162 host-assembly bytes** and
**252 general-BSS bytes**; stack boundaries remain unchanged.

The FX3 parity additions cover lions/lionesses/pumas through the GBA's
inherited `Enemy` behavior, drawbridge/trapdoor cardinal extents and coordinate
seams, READY-gated COMBAT triggers after `Weapon_update`, and OBJECT triggers
from landed blocks, including their camera ordering. `make paritycheck`
runs more than ten focused fixture families, including room-cache, movement,
navigation scales, shadows and generated classification; optional
`OL_REFERENCE_ROM` and
`OL_REFERENCE_SYMS` enable native/C comparisons. `tools/run_typesnes.py`
keeps test sources and caches in a temporary directory and leaves the
TypeSNES checkout untouched. Focused arithmetic checks cover **1,070,458 exact
products**; GSU-2 navigation adds **48 flood cases and 9,297 target probes**.
These slices do not establish full GBA parity. Measurements in later
sections describe earlier passes unless explicitly updated.

## FX3 build variant

The experimental **FX3=1** build targets the [FX3 v1.0 specification](https://github.com/LimitedRunGames-Tech/snes-fx3/blob/main/docs/FX3.pdf)
with a new **216×144, 8bpp Mode 3 pipeline**, not the smaller Mode 7 renderer.
Run these from `src/platform/snes` (same toolchain and original game-data
requirements as the normal build):

```sh
make fx3
make BUILD=build_fx3 FX3=1
make BUILD=build_fx3 FX3=1 check
make BUILD=build_fx3 FX3=1 keycheck
make BUILD=build_fx3 FX3=1 paritycheck
make BUILD=build_fx3_sh FX3=1 SHADING=1 check
python3 -m unittest discover -s tools -p 'test_*.py'
```

The output is `build_fx3/openlara.sfc`. `FX3=0` remains the default, and
`FX3=1 MODE7=1` is rejected. Use separate build directories for variants;
switching a flag in one directory still invalidates both assembly and C.
Shading is optional; on FX3 it occupies each pixel's upper nibble rather
than selecting a palette for an entire 8×8 tile.

The FX3-specific changes are:

- FastROM header `$30`, cartridge type `$18` (with a battery: the save), VCR
  `$52`, and registers at `$7000`. The ROM is 4 MiB (size byte `$0C`); each
  level's CPU-only data -- host tables, its small tables, its APU image --
  lies in the full-bank FastROM window above the GSU's, from `$E00000` for
  one level and `$F00000` for several, where the level table says (see
  "Levels on one cartridge").
- `PLOT` targets the hardware's internal chunky buffer. Firmware commands
  3/4/5 clear its thirds; 0/1/2 convert them into the 256×160-layout planar
  buffer at `$710000`, of which 216×144 pixels are used. Ordinary `MERGE`
  operations are replaced with software byte concatenation.
- STOP completion is polled through R15, with IRQ masked even in emulators.
  Page zero of the GSU program is reserved, so a single high-byte read can
  distinguish every running PC from the hardware's stopped R15 value zero
  without a torn 16-bit read.
- All engine images execute directly from `$84-$89` FastROM, including math
  tables and native helpers (`$86-$89` were GSU-2's `$BC-$BF` numbering
  until a second level needed the GSU's window unbroken). Host tables have
  independent ROM pointers; only mutable sectors are copied to `$7F:0000`.
  LEVEL1/LEVEL2 use 16,104/17,196 bytes there, recovering 45,386/45,209 WRAM
  bytes from the old whole-blob copy. Framebuffer conversion cannot
  overwrite collision/animation data.
  The former `$7E` code regions freed 48,640 bytes. Per-creature Nav now uses
  **25,290 bytes at `$7E:5800`**, including a 512-byte exact-sector cache
  and eight bytes of flip/zone-offset state, leaving **23,350 bytes** unallocated there
  and below the stack. General BSS and stack bounds have not been enlarged.
  CPU logic does not suspend FX3 RAM
  access; the entity channel is still published only between render jobs.
- Two packed VRAM images occupy bytes `$0000–$797F` and `$7980–$F2FF`.
  Blank tile 972 occupies `$F300–$F33F`; the shared tilemap is `$F800–$FFFF`.
  DMA skips the unused two tile rows per source column. Forced-blank chunks
  upload seven columns from NMI and up to fourteen from the line-185 IRQ,
  continuing through V-Blank into the top letterbox. A frame normally uploads
  in two fields, including the map's view rows. The latest TypeSNES capture
  restores display by line 32 and records zero VRAM writes outside blanking.
  The 216×144 view is centred in the 256×224 display.

Native FX3 now defaults to `FX3_STAGE=1`: light scenes copy the completed
image into a **31,104-byte packed WRAM buffer at `$7F:8000–$F97F`**, then
render the next frame while that copy uploads. Expensive logic ticks restore
the original zero-copy serial path for two seconds; subsequent heavy ticks
renew the hold. Mutable sectors must fit the guarded lower 32 KiB of `$7F`.
Use `FX3_STAGE=0` for the serial path or larger sector tables; the `C_MOTION`
reference defaults to this unstaged path. The VRAM double buffer keeps a
completed frame visible until its final chunk and tilemap switch arrive.

**FX3 is a separate memory target, not a GSU-2 with a faster clock.** The
vendor specification §§1,6,7 permits simultaneous 65816/FX access to ROM and
FX SRAM, Mode `$30` FastROM, 3 MB of FX-addressable ROM and 4 MB on the CPU
side. GSU-2 RON/RAN ownership rules and its 2 MB hardware ceiling do not
apply to FX3. Shared data still needs publication boundaries to avoid races.
The [OLH2 host-table format](HOST_TABLES.md) removes the old aggregate 64 KiB
staging limit and uses the CPU's full ROM window, including CPU-only
`$F0-$FF` when tables extend there. Individual tables still fit one 64 KiB
bank because the engine uses 16-bit table indices. Geometry and texture pages
run from GSU `$45` to the window's end, `$6F`, unbroken, the code all below
them; the single C2P destination is an implementation choice, with the packed
WRAM copy allowing upload overlap. LEVEL2's
119-byte legacy source-bank headroom now applies only to GSU-2.

`budget.py` validates FX3 C/native engine sections in their FastROM allocation
and reports the ROM banks separately: **9,741 bytes free** across the six
native FX3 engine banks (2,860 / 1,314 / 1,875 / 1,710 / 109 / 1,873).
GSU-2 has **6,030 resident code bytes free** across its five images
(2,165 / 320 / 2,282 / 1,199 / 64).
CPU `*_native` sections inherit the guard automatically; the fixed native
interrupt-vector table is header data, not engine code. Regression tests
cover the previously missed camera section and invalid bank/range placements.
FX3's separate Nav allocation is link-checked against `$7E:5800-$FFFF`;
general BSS has another **1,747 bytes free**, with the stack still at
`$1C00-$1FFF`. The 72 bytes of independent table pointers and two-byte ready
flag occupy checked direct-page gaps; the cold binder runs from ROM `$86`.

FX3 now uses three independent Nav contexts and five search steps per update,
matching GBA/32X's limits. Weighted target-outward queues, per-creature
step/drop limits and 32-bit corridor clipping replace the shared flood.
Original normal/wild moods, species aggression, random target selection and
nearest/forced context admission drive wolves, bears, bats and the FX3 raptor
controller in progress.
All six original maps are now carried: ground-1, ground-2 and fly for both
normal and alternate geometry. Identical arrays share ROM storage losslessly.
Initialization builds the ordered normal/alternate reachability union; search
uses the currently selected flip without resetting live queues. Ground-2
retains ground height semantics. This does **not** implement room swapping,
flip triggers or water-creature movement/controllers; water navigation uses
the existing fly family, not a fourth map. Wolf, bear and bat now have their
original state controllers. Native 65816 initialization,
search, complete corridor clipping, target-info/steering shells and wolf
and bear/raptor updates retain executable C versions for differential checks. Warm context
acquisition and admission to an empty slot no longer enter C. Earlier inactive
owners still take precedence over later empty slots; reclamation and nearest/
forced eviction retain the complete C path. Search rejects
unchangeable visited cells before reading immutable zone/floor geometry;
32-cell initialization and zone-scan strides preserve every unused boxIndex
word and derive the compact zone-list count from its final write pointer.
Both single-map and union scans reuse successful comparisons' carry flags
for index/output increments, avoiding redundant carry clears. Cached sector
lookups use exact sector/depth/count
keys; collisions are misses, and level reset invalidates every entry.

FX3's compiler-output pass also replaces the exact 15-iteration sign-extension
idiom with a shorter sequence, preserving all flags and using two temporary
stack bytes. Logical 9–15-bit shift loops use byte-swap/mask plus the remaining
shifts, preserving the result, carry, overflow and final X/Y/N/Z. The runtime
orders word factors for shorter software loops and uses the CPU's 8×8
multiplier for dense zero-extended word products, with the required result
latency and the compiler's r0–r5 preservation. NMI/IRQ do not use the shared
multiply/divide registers. The compiler shift rewrites remain FX3-only;
both native variants now use the exact runtime product helpers. GSU-2's
`C_MOTION` oracle retains the original software products. Build budgets read
resolved placement records from `config.inc`, not unevaluated conditional
assignments in the Makefile.

Collision angle/side setup, rotated item contacts, room-probe wrappers, bridge
filtering and (until FX3's doors became sectors) ordered door pushes have native FX3 implementations, retaining C far pointers,
16-bit height wrapping and side padding. Mood distance/zone/random-box helpers
and attack selection preserve full-width coordinates and original thresholds.
The common uninjured, same-zone ATTACK path skips the C transition machinery
only when neither a mood change nor a cold target seed is possible. All other
transitions keep the full C path. The logic PRNG uses ten CPU 8×8 products for
the exact original modulo-32-bit LCG; its seed, return value and roll count
are unchanged. Signed head-offset products use magnitudes on the CPU word
multiplier and restore the complete product's sign before arithmetic shifting.
Render-state packing is also native on FX3: it preserves dirty/active selection,
invisible and frozen frames, model-local animation indices, coordinates and
the two reserved bytes in each record. Four word copies cover the CPU divider's
16-cycle latency. It writes only the existing WRAM staging array; the SRAM
handoff and render/upload ordering remain in the host loop.

Validation uses a local TypeSNES build with FX3 support. It checks the
uploaded bytes against `gsusim`, the displayed pixels and margins, both
tilemap layouts, independent ROM tables and mutable sectors, FastROM code addresses,
and IRQ-independent operation. The ordinary GSU-2 opening render remains
unchanged. **Real FX3 hardware has not been tested.** MesenCE-style FX3
emulation uses planar PLOT and no-op C2P commands; neither that nor the
standalone interpreter measures the firmware's real clear/conversion time.
`gsusim --fx3` therefore reports reference GSU cycles without claiming a
hardware frame rate. The earlier serial opening pipeline gave about 5
displayed fps; the final adaptive pipeline gives 15.0 fps as recorded above.

An earlier FX3 full cartridge milestone passed **67 LEVEL1 / 51 LEVEL2 checks**
(`build_fx3_camera_helpers_l1_full.log`, `build_fx3_camera_helpers_l2_full.log`), including
the frame/contact checks and both zero-drop LEVEL1 scenes. The three-wolf
and mixed wolf/bear scenes finish 300/297 ticks over 600 fields, both within
the unchanged five-tick drift bound.
The Lost Valley native/C behavior/geometry/memory suite passes 63 selected
checks (`build_fx3_camera_helpers_l3_matrix.log`), plus the separate complete
C camera/cache snapshots (`build_fx3_camera_helpers_l3_timing.log`); its three-raptor live
timing gate still fails as detailed below.
LEVEL1 also passes ten wolf checks and eight bear checks with C references,
including both direct and generic dispatchers (`build_fx3_wolf_contact_checks.log`,
`build_fx3_bear_contact_checks.log`). The earlier seven-check native/C run
also covers the verified PRNG/admission fixtures
(`build_fx3_prng_dispatch_l1_reference.log`). The CPU-only PRNG
fixture was also rechecked on LEVEL2 (`build_fx3_prng_dispatch_l2_verified.log`).
The preceding six-map milestone also passed **40 / 38 native/C-reference
checks**; the new Lost Valley differential runs add the raptor and expand
the mood/geometry coverage below. GSU-2's LEVEL1/LEVEL2 ROMs remain
byte-identical to the previous 40 / 27 cartridge-check baseline (plus 18
LEVEL1 native/C-reference checks). The checks compare 864 complete Nav snapshots
and 1,024 mood/PRNG/Nav snapshots against the actual shared C++ methods on
each level. They also check context ownership, coordinate seams and search
generation rollover, including signed floor-subtraction overflow in both
directions, all three zone families and distinct normal/alternate maps.
The C++ oracle now reads all six original maps and checks flip changes while
searches are queued. A separate cartridge fixture checks every packed zone
ID and 54 ordered union lists at CPU-only `$F8` bank boundaries.
All 65,536 inputs pass value/register/flag checks for sign
extension and each of eight wide-shift forms used by the compiled engine.
8,192 long/word products, 864 corridor containment cases, 225 full-width
steering vectors and 81 saturated distances pass. Differential tests cover
1,404 initialization snapshots over 39 lengths (including 0..33, 63..65 and 1023/1024),
all three slots and zone families, both maps and dense/sparse/invalid sources; 1,536 corridor
side/terminal clamps, 384 complete admission/reclamation snapshots,
384 ground-movement cases and 640 complete wolf
target-info/item/Lara/PRNG/Nav/contact snapshots through each of the direct
and generic dispatchers, including 168 nonzero warm-context steering cases
per entry. The current LEVEL1 checks use 31.6% / 31.4% of C's instructions.
Collision checks cover 8,192 C/independent rotated item contacts,
all 65,536 angles, 4,896 slope/side cases,
2,048 bridge probes and 1,024 point/full-room results,
including far pointers, signed boundaries and unchanged padding. Another
2,048 cases compare mood distance/zone/random-box helpers, and 768 compare
attack selection at every species aggression threshold. The logic PRNG agrees
with C and independent uint32 arithmetic for 8,192 seeds and 1,024 uninterrupted
rolls, using 14.4% of C's instructions. Another 512 complete general-BSS
snapshots check native render-state packing against C and an independent
byte-level oracle, including 20,267 serialized records, all 0..255 frames,
reserved bytes and every unused slot. It uses 9.4% of C's instructions.
Bear checks compare 19,712 decisions with the actual C++ method and 512
complete C update snapshots through each dispatcher (32.1% / 32.0% of C's
instructions in the current LEVEL1 checks).
Boot checks assert all new FastROM entry-point banks
and that only the explicit Nav allocation occupies former WRAM code space.
Both opening-frame/PPU
comparisons pass. Earlier renderer validation also covered 8 viewpoints,
matrix/fixed-point math and animation-channel/Lara silhouettes; the latter
reuse the original pose records after undoing the changed screen centre.
Opening/walking/stationary-target combat clock fixtures retain 30 Hz without
dropped ticks. The separate LEVEL1 FX3 three-live-creature profiling gate
runs by default and now passes its unchanged no-drop/bounded-drift limits.
Both the original three-wolf scene and a two-wolf/bear encounter run by default.
The profiles save bus-clock costs, per-field tick/drop/queue samples,
startup call/drop events and stack depth to
`build_fx3/profile_creatures.json` and `profile_creatures_7_7_8.json`, plus
matching final displayed PNGs. `OL_PROFILE_SPECIES=7,7,8` selects the mixed
scene alone; types 7/8/9 select original wolf/bear/bat actors for the
room-14 workload. LEVEL3A
adds type 19 and a three-raptor gate which currently fails during early
pursuit: 285 ticks and 14 drops over 600 fields. Its limits are unchanged.
Real hardware
timing remains unverified.

The measured LEVEL1 scene has three living wolves, original animation and
navigation, room collision, pursuit damage and an unfrozen renderer. Over
600 NTSC fields:

| build stage | completed logic ticks | dropped ticks | presentation after startup |
|---|---:|---:|---|
| before native hot paths | 118 | 178 | starved |
| native hot paths, old host loop | 245 | 50 | starved |
| native hot paths and fixed presentation | 216 | 79 | 4–5 fps |
| native corridor/targeting/wolf updates and arithmetic | 291 | 8 | 4–5 fps |
| collision, mood/PRNG and initialization pass | 298 | 1 | 4–5 fps |
| bounded logic priority before render-state publication | 299 | 0 | 3–6 fps, mostly 4–5 |
| bear/contact, cold zone scan and native render snapshots | 299 | 0 | 4–5 fps |
| independently banked ROM tables, native admission and bank-aware scans | 300 | 0 | 3–6 fps, mostly 4–5 |
| all six navigation maps and ordered normal/alternate unions | 300 | 0 | 3–6 fps, mostly 4–5 |
| current-frame wolf contact and guarded enemy pushes | 300 | 0 | 4–6 fps, mostly 5 |

The current scene spends 564,963 engine master clocks per tick (1.58 fields),
including renderer-state serialization; the three-creature tick itself
averages 593,497 inclusive clocks, with DMA/NMI work included. It completes
30 ticks in the first 60 fields and 60 by field 120, then reaches 300 at
field 600, with zero drops. The original 30 Hz gate
passes; neither its five-tick drift limit nor the four-slot queue was raised.
The first full creature tick costs 1,528,122 master clocks, including three Nav
initializations averaging 167,187 each; cold work remains in the measurement.
Render-state packing averages 37,344 clocks (previously about 210,000).
These are emulator measurements, not FX3 firmware timings. The
minimum CPU stack pointer across the short and extended scenes is `$1EB1`, within the unchanged
`$1C00-$1FFF` reservation.

The two-wolf/bear encounter finishes 297 ticks with zero drops over 600 fields,
mostly 4–5 displayed fps and all three original actors navigating and dealing
damage. Its three-tick endpoint backlog remains inside the unchanged drift
bound; it is not a 300-tick result. It spends 580,550 engine clocks per tick
(610,058 inclusive gameUpdate), with a first full creature tick of 1,506,928
clocks. Earlier,
moving tables to independently banked ROM needed native empty-slot admission,
32-cell bank-aware zone scans and adjacent-field room reads to stay within
the gate. Six-map support also exposed one startup drop; retaining the
initialization batch count and reusing successful comparisons' carry flags
removed redundant work and restored the no-drop result. The optimized scans
match all 1,404 complete C initialization snapshots.
Neither the sixteen-NMI publication bound nor the performance assertions changed.

Current extended 3,600-field runs distinguish the scenes: three wolves pass
at **1,800 ticks / zero drops**, but the mixed wolf/bear scene fails at
**1,799 ticks / one drop** (`build_fx3_camera_move_l1_extended.log`). It retains
all three Nav owners and deals 2,456 pursuit damage. The remaining drop is
at field 1,736 in Camera_updateFollow → Camera_move → Room_trace → traceZ.
The field-1,740 sample now meets the unchanged five-tick drift bound, but
the zero-drop assertion still fails. The preceding bear-contact/stalking/
camera builds dropped six/five/four. Native camera movement reduces the
mixed engine mean from 577,590 to 569,954 master clocks; inclusive gameUpdate
falls from 604,954 to 596,738. Camera updates average 49,285 clocks (57,123
before) and peak at 634,180. Three wolves average 559,104 engine clocks and
584,606 inclusive gameUpdate clocks over the same extended run.
The current long JSON traces and displayed PNGs are retained with
`profile_camera_move_` names in `build_fx3_floor_l1`; preceding bat/Nav
short/long traces keep their `profile_bat_nav_` names.
Earlier zero-drop mixed extended runs predate current-frame contact and
must not be presented as proof for this build.
`OL_PROFILE_CAMERA=1` adds read-only nested camera spans. A 2,160-field
mixed-scene probe preserves the original timeline and locates stalls inside
fallback `Room_trace`, `Camera_move` and `traceZ`; its JSON/PNG are retained
as `build_fx3_floor_l1/profile_camera_bear_stalk_2160.*`. The subsequent native
camera build is recorded in `profile_camera_native_3600.*` in the same directory.
It preserves the existing camera/cache rules; the four remaining drops occur
at fields 1,694/1,736/1,750/2,044 with the queue full, including camera work
and render-state publication.
Run both extended checks with
`OL_PROFILE_FIELDS=3600 make BUILD=build_fx3 FX3=1 check TESTS='profiles the full tick'`.
The counter includes a tick already in flight at the measurement boundary;
inclusive call samples contain one fewer complete new call than the tick
counter. The first new call installs the live scene before any creature update.

FX3 services completed render jobs between logic ticks instead of waiting
for an empty backlog; it does not insert an idle field while logic is queued.
If all four queue slots are occupied, render-state publication briefly gives
logic priority. A one-byte direct-page counter expires after sixteen NMIs,
forcing publication at the next poll even if the queue stays full. Any slack
publishes immediately. Completed uploads still proceed normally; the next
render starts only after STOP, upload and publication. This avoids the former
field-25 startup drop without discarding work or starving presentation. An
isolated, permanently saturated-queue test observes nine forced publications
and nine visible buffer flips in 240 fields, including counter rollover.
The new byte is at `$27`, below the compiler's direct-page register area;
general BSS, Nav and stack allocations do not grow.

The earlier between-tick scheduling change exposed a shared raster bug:
an empty-height portal clip could start a zero-line strip, whose
countdown wrapped to 65,535. A cold entry guard rejects empty/reversed height
windows without moving the hot loop relative to its cache alignment. Flat,
opaque and keyed cases, plus one-visible-row controls and framebuffer
canaries, pass in both independent GSU cores: 34 FX3 fixtures (also shaded),
62 GSU-2 and 62 Mode 7.

The rest of this document describes the original GSU-2 target unless an
FX3 exception is called out above.

**In progress (2026-09-06): FX3 Lost Valley and raptors.** Full signed-byte
floor/ceiling slopes now preserve the original geometry, including Lost
Valley's `-39` slope and 17-bit products. OLH2 version 5 keeps the same
eight-byte LINK records; GSU-2's five-bit format is unchanged. Lost Valley
builds with a 185,068-byte host image in independently banked CPU ROM and
only 27,364 mutable-sector bytes in WRAM. See [HOST_TABLES.md](HOST_TABLES.md).

The FX3 raptor has original state/PRNG decisions, 400-unit head targeting,
previous-tick 1/4-degree steering, animation-driven motion, 256-up/down
limits, three 100-point attack latches and two immediate lethal-hit deaths.
Its native state/update routines are 412/573 bytes in `$88` FastROM; complete
C references remain. Tests compare 20,480 decisions against the extracted
original C++ graph and 640 complete update snapshots against C (35.6% of
C's instruction count). Real Lost Valley animations pursue and bite in an
isolated arena, and both deaths stop without rerolling or restarting.

Current-frame boxes now remain in FX3 FastROM, including Lara's. Wolf/bear/bat/raptor
contact records the GBA/32X `FAST_HITMASK` bound result before pushing Lara
out of the unexpanded frame box, with the original adjacent-room/proximity
filter and room guard. The original room-51 encounter now damages Lara at
tick 418 without a forced attack state or larger radius. Native readers and
wide-arithmetic helpers match source-data oracles. The fused contact phase
reuses its frame box, trigonometry and pre-push local point; its native
owner loop matches 2,880 complete C contact snapshots: 480 per species and
960 mixed groups covering all 24 three-species owner orders. The original enemy
radius stays zero through the entire push, including its room guard, and is
restored afterward. There is no frame-box or portal copy in WRAM; only 128 bytes of
per-item contact latches were added by the original raptor pass. Wolf migration
reuses these latches and removes its late animation-union overlap test.
Sixty-four bank-boundary/contact/visibility/owner cases preserve single
50/100-point attacks and contact captured before pushing or later separation.
Another 62 positions from original wolf attack/bite poses correctly reject
animation-union ghost hits on all three levels. The preserved pre-change
ROM fails the synthetic ghost-hit check with 50 unwanted damage. The real
room-14 wolf still approaches and bites. This migration frees 24 ROM bytes
without adding RAM, assets or a host-format change.
Bear migration now reuses the same phase and latches for running damage,
200/400-point attacks and the single upright death hit. A living bear ignores
contact if Lara dies after the collision phase; an already-dying bear still
consumes its captured fall latch. All 144 bank-boundary/eligibility/attack
cases pass, as do 447 ghost-hit positions from original bear poses, including
both death animations. Both real death graphs and the room-17 encounter
remain intact. This frees another 33 ROM bytes with no new persistent RAM.
Bat flight/attack now consumes the same current-frame, pre-push latch. The
200 bank-boundary/eligibility/dead-target cases preserve two-point attacks,
flight decisions and contact captured before later separation. Another 124
positions from original flight/attack poses reject animation-union ghost
hits; the saved old ROM fails with two unwanted damage. Each of Bat_update
and Enemy_update matches 512 complete C item/Lara/contact/PRNG/three-Nav
snapshots, including 404 turns and 40 deaths. GSU-2 retains its union path
and rebuilds byte-for-byte. A three-live-bat version of the room-14 profile
passes the unchanged 600-field gate at 296 ticks / zero drops, with all
three Nav owners and 1,128 damage. It spends 652,560 engine clocks per tick,
including real contact pushes; its four-tick endpoint backlog is within
the five-tick bound, not a claim of 300 completed ticks
(`build_fx3_bat_nav_l1_bat_timing.log`). Whole-game update ordering and
multi-creature collision ordering are not yet full original-engine parity.

The newly exposed extended mixed-scene workload also exercises stalking.
Its native box predicate keeps all coordinate sums/subtractions 32-bit,
strict-zero quadrant rules and the original result/PRNG order. All 131,072
independent cases cover every angle and every 0..510 bound sum at CPU-only
`$F8` bank start/end, including far Lara pointers. Another 2,048 comparisons
match C and unchanged game state, using 178,388 / 1,247,226 instructions.
The 239-byte native routine replaces 1,334 C bytes, with no new RAM or local
stack frame. It lowered the extended mixed-scene engine mean from 605,993
to 589,805 master clocks; the later native camera pass is measured above.

FX3's camera sector walkers, coordinate math, query wrappers and movement
now run from `$85` FastROM too. The 1,628-byte section replaces 7,145 C bytes without
new persistent RAM or broader caching. The trace oracle compares 5,376 C
and independent results and every ordered query, with signed-coordinate
seams, full-width slopes, portals and CPU-only `$F8/$F9` room/link tables.
Another 131,864 coordinate cases cover every angle and signed distance;
2,840 matched C snapshots use 31.8% of the C instruction count. Query
failures, no-floor translation, far/overlapping outputs and 384 complete
follow/snap/fallback-cache snapshots also match C. Movement adds exhaustive
signed deltas and 2,048 complete C snapshots with ten far/alias layouts,
5,900 ordered full queries, 361 height traces and 261 room changes. These
comparisons use 61.4% of the C instruction count; the second room query and
post-trace lookup are not skipped. Both existing camera caches and the union
target height remain approximations, not full original camera parity. GSU-2 keeps
the unchanged C paths and rebuilds byte-for-byte.

This is **not yet a playable Lost Valley parity claim**. Three live raptors
still exceed the tick budget: with the actual push/room checks enabled,
the current 600-field run records 285 ticks and drops 14
(`build_fx3_camera_move_l3_checks.log`). Native camera movement reduces the
previous 17 drops, lowering engine mean from 674,653 to 666,021 clocks and
inclusive gameUpdate from 708,990 to 698,391. The initial field-60 drift is
still seven against the unchanged five-tick bound. The first C
contact implementation dropped 82; native helpers and checked word-width
arithmetic reduced that to 55, and fused/native contact reduced it to 43
(103,512 master clocks per contact tick versus 180,049 before fusion).
The navigation pass keeps its overlap cursor in Y, rejects identical
full weights early, reuses setup within each five-step batch, and removes
a duplicate waypoint-bounds read. The live Nav_getWaypoint mean falls from
60,433 to 48,154 master clocks; 864 original C++ snapshots match with the
five-step maximum explicitly observed. Another 324 complete C comparisons
cover all tested full-weight pairs, queue order and independent `$F8/$FA/$FB`
table-start/end placements. That earlier navigation pass added five ROM
bytes and no persistent RAM. The latest search also keeps the floor-word
cursor live through the height and weight tests, then reads the adjacent
overlap word without rebuilding its address. Search order and every queue/
cell snapshot stay exact; the same 324 cases fall from 181,965 to 172,936
native instructions (5.0% less). Bat contact and this cursor cleanup together
recover 61 more engine ROM bytes without changing persistent RAM.
The native room-collision wrapper reduces its mean cost from
79,529 to 61,825 master clocks while retaining the C probe order, zero-radius
side/slant semantics and static response. It frees 2,672 ROM bytes and cuts
its local frame from 96 to 30 bytes, with no new persistent RAM. Independent
CPU-only bank-boundary and signed-geometry fixtures compare every collision
decision with the complete C implementation. Per-frame command effects now
scan their original CPU-ROM list natively; end commands retain their C body.
Another 2,048 C snapshots exercise effect ordering and independent CPU-only
ANIM/MODEL/CMD banks, alongside all 14,964 real-animation advancement cases.
FX3 now constructs health before initial activation and rejects corpse
reactivation before it can change the active list or evict a Nav owner.
Living creatures that remain active can retry an evicted Nav slot without
being inserted into the active list twice. Both LEVEL1 gates still pass at
300/297 ticks / zero drops in the full regression suite after bear migration.
The logic PRNG now uses a 1 KiB immutable FastROM product table in place of
ten hardware multiplies: 66 rather than 178 native instructions per roll,
with the exact original uint32 state/return sequence. All 256 products and
131,072 seeds pass with verified operands pointing to CPU-only `$F8` bank
start/end tables. Native `Enemy_update` preserves admission/release and
visibility/dirty ordering before tail-calling the original species paths.
These changes add 330 net ROM bytes and no persistent RAM. The three-raptor
engine mean falls from 708,104 to 692,811 master clocks/tick (692,814 after
wolf migration); the first full
update still costs 2,218,066 clocks, including three cold Nav initializations.
No enlarged queue, relaxed drift gate or discarded search steps hide the
remaining failure. The current preview is
`build_fx3_raptor_l3a/openlara.sfc`; the earlier primary ROMs below have not
been replaced with this unfinished slice. Reproduce the remaining gaps with:

```sh
make BUILD=build_fx3_raptor_l3a FX3=1 LEVEL=LEVEL3A check TESTS='original Lost Valley|profiles the full tick'
```

**Previous parity work (2026-09-06):** the bear now follows its original state,
attack and death graph on both FX3 and GSU-2. Native bear decisions and update
ordering retain a complete C reference; a host oracle compiles the actual
shared C++ Bear method, including its PRNG order. FX3 additionally has exact
native rotated item contacts and a faster cold navigation zone scan. The
live timing gates and memory layouts are described above.
The preceding per-creature Nav slice also fixes two shared C++ navigation bugs: the
blocked-box mask tested an overlap-index bit (`$0400` instead of `$4000`),
and an emptied queue retained a stale tail that could prevent retargeting.
SNES box packing now converts inclusive PC maxima correctly, as GBA/32X do;
every packed box is checked against independent original bounds.

TR1 swan dives, fast dives and handstand
climbs now follow the original animation graph and collision rules. Hold L
during a forward jump to swan-dive; hold B+Up+L while hanging to handstand
onto the ledge. Dive entry requires free hands. A fast dive landing above
133 units/tick is fatal; water entry retains the original -45/-85 degree
pitch and doubles the entry speed. As with swimming, the renderer still
omits Lara's overall pitch; animation poses are drawn normally.

Bats now have their own controller: the original wake/fly/attack/falling/death
graph, animation-driven horizontal speed, 16-unit vertical pursuit toward
Lara's head, 20-degree steering and two-point contact damage. They resolve
room portals, respect floor/ceiling/wall limits and fall when killed. Contact
still uses animation-union boxes, not joint spheres; pitch and blood effects
are not rendered.

Bears now use the original WALK/STOP/HIND/RUN/HOWL/GROWL/BITE/ATTACK/EAT/DEATH
decisions, 500-unit head-offset targeting and previous-tick 0/2/5-degree
steering, including the close-reverse half-turn limit. Running contact deals
3 points per tick; bite and standing attacks latch one 200/400-point hit.
Upright deaths follow HOWL into DEATH and can deal one 200-point falling hit;
the other death route goes through STOP. Movement reads the real animation
speed (86 units/tick in the pursuit fixture) and shares ground collision,
with the bear's 256-up/256-down limit. Joint masks, head/tilt and blood remain
unported; GSU-2 still has its shared navigation approximation.

Wolves now use their original sleep/stop/walk/stalk/run/growl/howl/attack/bite
graph, animation-driven speed, 2/5-degree steering, head-offset distance and
front/behind attack decisions. Attacks latch one 50- or 100-point hit; lethal
damage selects one of the three death animations without restarting it each
tick. Ground movement resolves slopes/portals, enforces 256-up/1024-down
limits, rejects wrong zones and avoids other active creatures. GSU-2 retains
the shared navigation approximation; FX3 has the original aggression and
corridor waypoints. Joint hit masks, head/tilt rendering and blood remain.
The previous stationary, front-facing circling case has not been reverified
with FX3's new pathfinder; the room-14 pursuit fixture does approach and hit.

Navigation now keeps TR's exact sector-to-box IDs, including NO_BOX holes and
stacked rooms. The old X/Z rectangle scan chose the wrong ID for 1,265 LEVEL1
and 1,996 LEVEL2 sectors. A compact predictor/rank map fixes this using
2,300/2,109 bytes, and native point lookup shares collision's room cache.
The complete original maps are checked in the cartridge, not inferred from
box bounds. GSU-2's shared flood and simplified waypoints are still not full
Nav; the FX3 implementation uses the same exact sector map.

The low WRAM code image now starts at `$0600`, reclaiming 2,560 bytes while
keeping the full `$1C00-$1FFF` stack. Native item movement and bounding-box
copying recovered another 1,936 bytes after bank packing. Native animation
advancement/transitions and wolf decisions, plus keeping the boot-only math
checks in ROM, fund the wolf controller. Exact navigation and shared native
room lookup remove the old geometry scan/cache. Native bear decisions/update
and the smaller shared species table fit the bear without expanding WRAM.
At that earlier milestone, **1,816 bytes of resident code and 1,369 bytes
of BSS remained free**; current budgets appear above. The ROM-resident
self-test runs before the GSU starts, while the CPU still owns the ROM bus.
Link checks reject host/code/stack overlap and packed assets cannot enter
the reserved code banks `$3C-$3F`.

`make motioncheck` compares the native routines against a CPU-only C-reference
cartridge. LEVEL1 covers 4,550 movement cases, 279 boxes, 3,785 transition
boundaries, 14,769 frame/command/pending-state cases and 69,888 wolf decisions.
Bear validation adds 19,712 decisions against the actual C++ method and 512
complete C update snapshots, including target info, turns, attacks, death,
PRNG and (on FX3) the complete owning Nav context. The native GSU-2 bear
update uses 62.2% of C's instructions in that fixture.
The C reference (`C_MOTION=1`) lives in ROM with the GSU stopped; it is not a
playable GSU-2 build. Native movement, animation and wolf decisions use about
26%, 43% and 45% of their C counterparts' instructions on these fixtures,
respectively; these are isolated measurements, not whole-game speedups.
FX3 adds native ground movement (384 full-state cases, 39.6% of the C
instruction count) and the compiler/runtime checks described above. The
first native pass reduced the original 360-snapshot LEVEL1 Nav workload from
7,102 to 1,974 mean instructions; the current expanded 432-snapshot workload,
including extreme floors, averages 1,097 with a 4,682 maximum.
Neither is a whole-tick timing claim.
It also compares native room-aware navigation and sector lookup against C.
`NAV.json` retains the original IDs independently of the compressed map;
the tests cover every sector in LEVEL1/LEVEL2, coordinate seams, room edges,
all four predictor tags, 1..10-bit literals and ranks past a 16-bit bit offset.

The build supports macOS GNU make 3.81's multi-output generation, recovers
missing paired assets and repacks on `LEVEL` changes. `TEXTURES.json` selects
the level's pages, so stale pages from a larger level cannot enter its ROM.
The Python tests cover asset inventories and WRAM placement as well as the
renderer helpers; `make fixedcheck` retains the host-side math/box oracle.

Validation: GSU-2 passes 35 applicable LEVEL1 tests and 23 LEVEL2 tests;
FX3 passes 39/27, plus 31 Python unit tests and `fixedcheck`. C-reference
comparisons pass on all four cartridges; LEVEL2 covers 3,989 transition and
15,603 frame/command cases. The standard/FX3 opening images still match the
independent renderer. Fresh parallel builds, missing-NAV metadata recovery,
LEVEL1/LEVEL2 round-trips and same-directory FX3/GSU-2 switches reproduce
identical ROMs; an unchanged build is a no-op. The earlier FX3-only FastROM
relocation left GSU-2 byte-identical; this slice changes its packed box bounds
and blocked-box handling. Mode 7 passes 17 selected
boot/render/CPU/gameplay smoke checks. The standard walking fixture remains
at 30 Hz with no dropped ticks and samples 0.67 fields per tick (previously
about 1.0).

**Status: Lara is visible and controllable in LEVEL1.** Her current dry-land
slice walks, runs, turns, back-walks and hops, sidesteps, rolls, and jumps in
four directions through TR's packed animation graph. The target works out
which room the camera is in, walks the portals to find what else is visible,
renders it and the scenery standing in it, follows the floor under her feet,
refuses to let her through a wall, and drops her off ledges. Game time is
TR's own thirty ticks a second, queued by NMI and independent of how long a
frame takes to draw — see "The clock". What is here is
the machine underneath a port and the first playable slice of the game
standing on it. "Gaps against the GBA port" below is the honest list.

## The thing that makes this port different

Every other OpenLara target recompiles `src/fixed` with a C++ compiler. This
one cannot, and the reason is worth stating precisely because it is easy to get
wrong in both directions.

A 65816 C compiler does exist: `816-tcc`, shipped with
[PVSnesLib](https://github.com/alekmaul/pvsneslib) — a tcc 0.9.25 derivative
with a peephole pass (`816-opt`) behind it. It works, it is used here, and the
30 Hz game update already runs through it. What it is not is a C++
compiler — it rejects a `.cpp` file outright — and `src/fixed` is thoroughly
C++: `common.h` alone has around 110 struct member functions and 16 operator
overloads, `lara.h` another 73.

For the GSU there is no compiler of any kind. Sixteen registers, no stack, no
call instruction and a 512-byte instruction cache is not a target anybody has
written a backend for.

So the port is a rewrite: **C for game state on the 65816, hand written GSU
assembly for the renderer**, with the GBA port as the specification. The C++
stays the reference; `rasterizeF` here is a transliteration of
`src/platform/gba/rasterizer.h`, not a recompilation of it.

Where the line falls is set by measurement, not preference. A small fixed-point
edge-step routine compiles to 56 instructions of `816-tcc` output, 49 after
`816-opt`, including a call to a 32-bit multiply helper; the same work is about
12 GSU instructions, on a processor already six times faster. Anything
arithmetic-heavy belongs on the GSU. Control flow and game state can afford C.

### The compiler's type model

This shapes the port more than anything else about it:

| type | width |
|---|---|
| `char` | 1 |
| `short`, `int`, `long` | 2 |
| `long long` | 4 |
| pointer | 4 (far) |

There is no 32-bit `int`. OpenLara's `int32` has to become `long long`, and
every 32-bit multiply becomes a `jsr.l tcc__mull` library call — another reason
fixed-point maths wants to be on the other processor.

Nav exposed two more compiler constraints: long chains of 32-bit comparisons
can miscompile short-circuit branches, and a large function can accumulate
spill slots beyond the 255-byte stack-relative addressing range. Separate
guards and small clipping helpers retain the original decisions; a shared
50-byte CPU-only clipping workspace keeps Nav's largest local frame at 92
bytes. NMI never enters navigation. Cartridge tests compare the resulting
queues, waypoints and PRNG state, not only the host C translation.

Current-frame collision found the same wide-comparison issue in nearest-edge
selection. A stable minimum with separate guards preserves the original tie
order. Validate the wide rotated point first, then narrow its coordinates
and nonnegative edge distances: they fit signed/unsigned words respectively,
even for a 65,535-unit box. This reduces the push routine's local frame from
an unassemblable 352 bytes to 48. A computed int32-to-far-pointer cast also
lost the bank byte; the portal-reader C oracle now binds the two words
explicitly. Native/C and independent arithmetic tests cover both fixes.

### Getting compiled code to run at all

`816-tcc` emits its code, and its `.rodata`, into ROM. On an ordinary SNES that
is right. Here it is unusable: while the GSU holds the ROM bus the 65816 reads
dummy data from cartridge ROM, so the code the host executes *and* the
constants it reads have to be resident in WRAM before the GSU starts.

`tools/tccfix.py` retargets the compiler's output: every `SUPERFREE` section is
pinned into one ROM bank addressed through the WRAM code slot, and `.BASE $7B`
offsets the bank byte so that a section physically stored in ROM bank 4
resolves as `$7F:xxxx`. That matters because `816-tcc` emits `jsr.l` for every
call between C functions — without it, C calling C would jump into cartridge
ROM and read dummy data. `.BASE` is toggled back to `$00` around the
`.RAMSECTION`s, which otherwise inherit it and put `.bss` at bank `$FA`.

The boot code then moves the whole bank to `$7F:0000` with one DMA, and the
labels already point where the bytes ended up.

Two other conventions come with the compiler. Its code assumes 16-bit A and
index registers throughout. And `816-opt` shortens the compiler's long
addressing to plain absolute, which is only correct if the data bank register
points at the bank holding `.bss` — PVSnesLib's crt0 does `pea $7e7e / plb /
plb`; there is no crt0 here, so `ENTER_C` / `LEAVE_C` in `main.asm` do it
around every entry into C.

## Division of labour

The same shape as the 32X port — co-processor is the engine, host CPU is an
I/O servant:

| | |
|---|---|
| GSU, 21.4 MHz | clears and renders the frame buffer into game pak RAM |
| 65816, 3.58 MHz | hands the buses over, pumps the frame buffer into VRAM, reads the pad, and runs the game in C at thirty ticks a second |
| SPC700, 1.02 MHz | holds the level's samples and plays them on the S-DSP when told to, four port writes at a time; see "Sound" |

The catch that shapes every line of `main.asm`: while the GSU owns the ROM bus
the 65816 **cannot fetch from game pak ROM at all**. It reads dummy data, and
even its interrupt vectors come from the GSU as dummies pointing at WRAM
`$0100/$0104/$0108/$010C` (Book II 2-5). So everything the host runs after boot
lives in WRAM, copied there at reset from ROM bank 1.

## Memory map

**Cartridge** — LoROM (mode $20), Super FX **GSU-2**, 128 KB expansion RAM,
2 MB of ROM with 1.13 MB in use.

| ROM offset | CPU sees | GSU sees | contents |
|---|---|---|---|
| `$000000` | `$00:8000` | — | boot code, header, vectors |
| `$008000` | `$01:8000` | — | image of the WRAM-resident host assembly |
| `$010000` | `$02:8000` | `$41:0000` | GSU program + reciprocal table |
| `$020000` | `$04:8000` | — | image of the compiled C, copied to `$7F:0000` |
| `$030000` | | `$43:0000` | the level: room table, geometry, portals — 4 GSU banks |
| `$070000`+ | | `$47:0000`+ | packed 256×256 texture pages, one GSU bank each |
| after the pages | `$2E:8000` (LEVEL1) | — | `SOUND.BIN`, the sound CPU's image, read once at boot; `tools/mkbanks.py` places it |
| `$1E0000` | `$BC:8000` | — | boot-only arithmetic check; CPU-only C oracle also uses this bank when requested |
| `$1E8000` | — | — | resident-code image copied to `$7E:0600` |
| `$1F0000` | — | — | resident-code image copied to `$7E:5800` |
| `$1F8000` | — | — | resident-code image copied to `$7E:8000` |

`tools/mkbanks.py` places the last two and writes the bank bases into
`build/texbank.inc`, so neither the GSU code nor the packed data hard-codes
the other's layout.

The GSU reads ROM linearly in banks `$40-$5F`, which is why its program bank is
`$41` and its entry point is `$0000`.

### Which Super FX

**This is a GSU-2 cartridge, and it has to be.** No header field names the die,
so it is worth writing down what forces the choice and what pins it.

| revision | ROM | RAM | linear `$40-$5F` image |
|---|---|---|---|
| MARIO Chip 1 | 1 MB | 64 KB | **no** |
| GSU-1 | 1 MB | 64 KB | yes |
| GSU-2 | 2 MB | 128 KB | yes |

Two of those columns are load bearing:

- **The linear image.** Everything the GSU reads — its own program, the
  reciprocal table, the room, the texture pages — is addressed through banks
  `$40-$5F` as flat 64 KB banks. The original MARIO chip does not decode that
  image, so this port cannot run on one at all. That rules out Star Fox's die.
- **The windows.** 1 MB of ROM and 64 KB of RAM is what GSU-1 reaches. The
  texture pages for one level are already 704 KB, the game pak RAM map runs to
  `$1FFFF`, and neither is going to shrink. GSU-2 doubles both.

What actually selects it is the header: `$FFD6 = $15` (Super FX + RAM +
battery) with `$FFBD = $07` (1 Mbit of expansion RAM). 128 KB of work RAM is
more than the 100-pin package has pins to address, so the board can only be
carrying the wider part. **Dropping `$FFBD` to a smaller size would silently
retarget the cartridge at GSU-1** — it would still boot today, since the ROM is
exactly at GSU-1's 1 MB ceiling and nothing yet sets `RAMBR` to 1, and it would
stop booting the moment either grew. `make check` asserts the revision so that
cannot happen quietly.

Two things GSU-2 offers that this port does not take:

- **The second RAM bank.** `RAMBR` is set to 0 once at frame entry and never
  moved, so only `$70:0000-$FFFF` is in use. The other 64 KB is there for the
  engine data the gap analysis below is going to need, and costs a `RAMB` to
  reach.
- **The high-speed multiplier**, CFGR bit 5. The manual is explicit that MS0
  must be 0 at 21 MHz, and GSU-2 silicon has been measured ignoring that. It
  stays off regardless, because it was measured rather than argued about: the
  frame issues 14,695 `FMULT`/`LMULT`, and 7 cycles down to 3 saves 58,780 out
  of roughly 19 million. Three tenths of one percent, against a documented
  hardware constraint. This frame is bound by instruction fetch, not by the
  multiplier — see "Where a frame goes".

**Game pak RAM** (`$70:0000`, 128 KB)

| | |
|---|---|
| `$0000-$01FF` | GSU short-address window — LMS/SMS reach only 0–510, and that is where the rasterizer keeps its locals |
| `$0200-$0FFF` | vertex links and rasterizer work data |
| `$1000-$5FFF` | frame buffer 0 |
| `$6000-$AFFF` | frame buffer 1 |
| `$B000-$D9FF` | transformed vertices of the room being drawn — 10.5 KB, exactly 768 of them |
| `$DA00-$DC3F` | one near-plane flag for each of the 576 filed faces |
| `$DC40-$DFFF` | spare |
| `$E000-$E97F` | ordering-table heads, links and face offsets — 576 faces |
| `$EA00-$EC7F` | shade map 0 — tile map high bytes for frame buffer 0 |
| `$ED00-$EF7F` | shade map 1 |
| `$F000-$F2FF` | the portal walk: visible rooms, visited flags, clip rectangles |
| `$F300-$F315` | what the GSU stages from its probe — room, floor, neighbours, trigger, level-ready. The renderer's seed now; the C reads the host sector table instead |
| `$F400-$F4BF` | eight-level matrix stack |
| `$F500-$F5FF` | near-plane clipper buffers |
| `$F600-$F60D` | model-header scratch |
| `$F610-$F629` | Lara's saved object basis and weapon-node scratch |
| `$F700-$F705` | the position the host asks the GSU to probe |
| `$F710-$F729` | weapon state and two independent pistol arm records |
| `$F72A-$F731` | Lara's torso pitch/yaw and head pitch/yaw |
| `$F800-$FDFF` | renderer state for 128 entities, 12 bytes each |
| `$10000-$1F031` | level blob staged in RAM bank 1 for the 65816 C code — 61,490 bytes; 4,044 bytes below the blob limit, but only 2,826 below the source ROM bank limit including its room directory |

**WRAM** (`$7E:0000`, mirrored at `$00:0000-$1FFF`)

| | |
|---|---|
| `$0000-$00FF` | direct page variables |
| `$0100-$010F` | landing pads for the GSU's dummy vectors |
| `$0110-$05FF` | resident host assembly |
| `$0600-$1BFF` | C/native code and math tables, copied from ROM bank `$3D` |
| `$1C00-$1FFF` | stack |
| `$2000-$57FF` | C `.bss`, data bank `$7E` |
| `$5800-$7FFF` | resident C, copied from ROM bank `$3E` |
| `$8000-$FFFF` | resident C, copied from ROM bank `$3F` |

**WRAM bank `$7F`**

| | |
|---|---|
| `$0000-$7FFF` | resident compiled C code and its constants |
| `$8000-$FFFF` | resident compiled C/native math and constants |

On FX3 the engine executes from FastROM `$84-$89`,
and `$7F` holds only mutable sectors (16,104 bytes for LEVEL1). Immutable
host tables stay in independently banked ROM; see [OLH2](HOST_TABLES.md).
Math code/tables use `$87`; Nav, its sector cache and flip/zone offsets occupy
25,290 bytes of the former `$7E` code space, with 23,350 still unallocated. Native trig uses
long loads, so its data does not need a WRAM mirror.

**VRAM**, in words

| | |
|---|---|
| `$0000-$27FF` | frame buffer 0 characters |
| `$3000-$57FF` | frame buffer 1 characters |
| `$6000-$63FF` | tile map |

## Frame format and why it is 256×160

**This is the default again after the ninth renderer pass.** `make` builds the
full-resolution unlit frame; `MODE7=1` selects the faster 128×64 alternative
described next, and `SHADING=1` explicitly enables either format's lighting.
The change favours image detail now that geometry culling has recovered enough
frame time. It is still a real trade: after the eleventh pass, the unlit opening
view is 4.467 M modelled cycles here against 3.178 M in Mode 7.

The GSU's bitmap emulation offers three heights — 128, 160 and 192 dots — in
4, 16 or 256 colours. 256×160 in 16 colours is the pick:

```
chars      32 columns x 20 rows           = 640
char size  32 bytes (4bpp)
frame      640 * 32                       = 20480 bytes
```

Character numbering in 160-dot mode is column major, `CN = $14 * Xc + Yc`, so
the tile map's character numbers are that formula and nothing else, written
once at boot; flipping buffers just repoints `BG12NBA`. The attribute half of
each entry is not static — that is where shade lives, see below.

The resolution is chosen by the upload budget, not by taste. VRAM only accepts
writes during blanking, and DMA moves a byte every 8 master cycles, so an NTSC
V-Blank is worth about 6.4 KB. 20480 bytes is therefore four V-Blanks — a
15 fps ceiling on *presentation*, which is where Star Fox lived too. 256×192
would cost five.

That ceiling is not the current frame rate and should not be read as one; the
renderer is the bottleneck by a wide margin. See "Where a frame goes".

256 colours is not merely expensive, it is impossible, and it is worth writing
down because it looks like the obvious answer to the shading problem. An 8bpp
256×160 frame is 40960 bytes of characters. Two of them is 81920, and the SNES
has 65536 bytes of VRAM in total. A double-buffered 8bpp frame does not fit at
any speed, which takes the GSU's 256-colour mode — and with it the
freeze-high-nibble trick that would have given per-pixel shade — off the table
for this frame format.

When enabled, the shade term goes somewhere else entirely: see "Shading".

See "The Mode 7 frame" for the optional path that gives up half the resolution
to get eight bits a pixel and substantially more speed.

## The Mode 7 frame

Introduced as the default on 2026-09-04 and retained as the fast option with
`MODE7=1`. It renders 128x64 at a byte a pixel and lets the PPU's Mode 7
matrix show it at twice the size, 256x128 letterboxed. The
same view: the projection reads a table of 32768 / dz in place of 65536 /
dz, so every screen coordinate comes out halved and the vertex loop does
not change by a byte. What changes is what a pixel costs and what a pixel
can hold.

Mode 7 character data is chunky, one byte per pixel, so the GSU writes the
frame with `STB` and the bitmap machinery goes with it: no `PLOT`, no
plot cache, no read-modify-write of a partly filled character row at each
end of every span. A pixel is a ROM fetch and a RAM write. The
tiles run down the columns -- tile `(x >> 3) * 8 + (y >> 3)` -- which makes
the address `fb + (x >> 3) * 512 + y * 8 + (x & 7)`: a line is eight bytes
on from the one above it and everything else depends on x alone, so a
128-word table per frame holds each column's address on line 0 with the
buffer folded in. A span reads that table once, walks the current tile row
byte by byte, and adds 504 when it crosses each eight-pixel tile boundary.
The host's map names the tiles in the same order.

The constraints are the PPU's. A Mode 7 map entry is one byte, so 256 tiles
of 64 bytes is the entire character set: 128x64 is 128 tiles, two buffers
are 256, and 128x80 would not double-buffer. Mode 7 always lives in VRAM
words $0000-$3FFF, the map in the low bytes and the characters in the
high, so the upload is a DMA to $2119 alone with `VMAIN` incrementing on
the high byte -- a byte of frame is a word of VRAM -- 8 KB in two V-Blanks.
The flip is a vertical scroll: rows 0-7 of the map name buffer 0's
characters, rows 8-15 buffer 1's, `M7VOFS` moves 64 map pixels. The
letterbox is eight 64x64 sprites of one black colour, because the map
wraps and every tile of it is frame; sprites lie over BG1 in Mode 7 at
every priority. The sprite palettes are CG-RAM 128-255, which an 8-bit
pixel can also select -- the upper half of the shade rows, if they ever
exist, is shared with the letterbox.

At adoption, modelled against the 4bpp build on the same two views:

| view | 4bpp 256x160 unlit | Mode 7 128x64 unlit | Mode 7 lit | |
|---|---|---|---|---|
| opening, her west side | 8.75 M | 6.06 M | 6.54 M | 2.45 -> 3.27 fps, lit |
| behind her | 7.05 M | 4.85 M | 5.18 M | 3.04 -> 4.13 fps, lit |

A third of the frame unlit, which is the whole of the per-pixel and
per-scanline half the estimate said it would be; the lit frame gives eight
per cent of that back for the light, where the 4bpp frame's shade cost it
a third and then some (16.3 M at this view). What remains is geometry: the vertex
path is 1.45 M of the 6.08, the per-span gradient setup (`RFT_Recip`) is
the largest single routine at 0.57 M for some 2,750 spans averaging four
pixels, and `EdgeSkip` -- strips starting above a frame that is now 64
lines tall -- is 0.28 M from outside the window. The half-height buffer
this replaces was estimated at 2x; it is 1.44x, because halving pixels and
scanlines leaves the vertices, edges, clipper and per-face glue exactly
where they were. Those are the next levers, and they are independent of the
frame format: see "Where a frame goes" and TODO.md.

What the picture loses: half the resolution each way, and a fifth of the
vertical field of view (64 lines at the same focal length where 160 stood
in 256; the Mode 7 matrix can stretch 64 to 160 lines with `M7D` at 0.4,
non-square pixels and an uneven 2-3-2-3 row doubling, if the framing
matters more than the squareness). What it gains: eight bits a pixel. The
low nibble is the texel and the high nibble is the shade.

### The lit pixel

`SHADING=1` lights a span with the shade its left edge has
reached on this line: one `OR` a pixel, into the high nibble, from a
register set once a span. The palette is the same eight rows the 4bpp
frame's tile map selected -- CG-RAM row `g >> 2`, the ramp TR's own
lightmap gives, see "Shading" -- so a lit pixel is texel plus 16 times
row, and the letterbox sprites keep CG-RAM 128-255, which a row of 8 or
more would have taken. At four pixels a span the gradient across a span is
a fraction of a level, so per span is per pixel to the eye; the gradient
that matters, down the face, is the edge accumulator's, at its full 32
levels.

Three things made it fit. The edge accumulator carries `g << 10` rather
than `g << 8`, so its high byte holds the row in bits 4-6 and the span
setup is a `HIB` and an `AND` -- the GSU has no shift left, and a
doubling per bit would have been four bytes the window has not got. The
edge's starting shade travels in the edge record (`E_G0`, the record's two
spare bytes) and the edge block's words are reordered so `EA_Take`'s copy
loop carries it: the fifteen bytes that loaded it from the vertex are
gone. And the packer's clamp of every vertex shade to 3..28 leaves three
levels each side for the edge to drift into, so the per-span clamp the
4bpp path pays for is not needed here at all. Ten more bytes came from
`FROM R0` prefixes the window did not need -- R0 is the source unless a
prefix says otherwise. At adoption the tail of `RFT_Advance` still ran
from ROM for 45 K cycles a frame; the seventh pass below repacked the loop
and brought the whole advance inside the window.

Checked against the 4bpp lit frame at the same view, pixel against
character: 78% of lit pixels on the same palette row, 90% within one,
a mean difference of 0.37 of a row, and the same row histogram. The
`frame.4bpp.unlit.baseline` is the default frame (CRC `bc755bd1`), while
`frame.4bpp.baseline` retains the optional shaded record. The historical names
`frame.unlit.baseline` and `frame.baseline` retain the unlit and lit Mode 7
records so earlier performance notes remain comparable.

Not here: sixteen rows (a 16-row PALETTE.BIN, and the letterbox colour
moved to CG-RAM 255), and a gradient across the span, which wants two
registers the loop does not have.

Verified: `tools/gsusim.py --mode7` decodes the layout and counts a plot
as a byte stored into the buffer; the runner reads the high bytes of VRAM
words 0-$1FFF or $2000-$3FFF, by `fbShow`, and skips the shade map. `make
BUILD=build_m7 MODE7=1 check` is 13/13 with the frame the host uploaded
byte-identical to the interpreter's (CRC `c01930ff`, in
`frame.unlit.baseline`), and the ROM's own counter reads two whole frames
a second where the 4bpp build read one. One thing TypeSNES cannot show:
the top letterbox. A sprite at Y 240 wraps to the top of the screen on
hardware and TypeSNES draws none there (reported), so its picture has the
other buffer's lowest rows above the frame, where a real PPU has black.
The bottom band is black in both. bsnes (`make run`) is where the
letterbox is judged.

## Shading

**Off by default.** At 256×160 the 4bpp shade uses `SHADE_MAP`, only one palette
row per 8×8 character, and that coarse lighting does not suit the full-resolution
picture. `SHADING=1` remains available for comparison; its opening view is
7.604 M cycles against 4.853 M unlit. In Mode 7 it uses `SHADE_PIXEL`, per span,
and costs eight per cent of that frame. `SHADING=0` leaves every pixel on
CG-RAM row 0 in either format.

The flag is build-time, not runtime. A runtime switch would leave the
interpolation in the edge walkers whether or not anything read it, which is
most of the cost.

The GBA and 32X ports both light a pixel by indexing a 32×256 lightmap: shade
row from the vertex, column from the texel. A 4bpp frame buffer has no bits to
spare for the row, and 8bpp does not fit in VRAM twice (above), so the shade
term cannot live in the frame buffer at all.

It lives in the tile map instead. Bits 10-12 of a tile map entry select one of
eight background palettes, so CG-RAM holds eight progressively darker copies of
the sixteen colours and each 8×8 character picks a row. **Texel per pixel,
shade per character.**

The rasterizer interpolates `g` at its full 32 levels — down both edges,
across the span, and through the near-plane clipper alongside u and v — and
quantises with `g >> 2` only at the moment it writes a character. Interpolating
at 8 levels instead would band; interpolating at 32 puts the boundary in a
different place on each scanline, which reads as a gradient.

```
GSU   writes bits 2-4 of one byte per character, into game pak RAM
host  DMAs those 640 bytes over the tile map's high bytes, in the same
      V-Blank that flips BG12NBA
```

Bits 0-1 of that byte are character number bits 8-9, and with 640 characters
they are not optional, so the per-character write is a read-modify-write over a
map the host seeded at boot. Reconstructing `CN = $14 * Xc + Yc` on the GSU
instead would cost more in a loop that runs per span per scanline than the
read does.

Two things about the arithmetic are worth knowing before touching it:

- **The reciprocal table's error is one LSB of what it returns**, which is
  nothing at `h = 3` and everything at `h = 30000` — an ordinary height for a
  face with a vertex just past the near plane, where the true reciprocal is
  2.18 and the table gives 2. Edge accumulators therefore drift out of range,
  and `ShadeClamp` bounds them once per span.
- **The per-character step is a whole character wide and the span may not be.**
  A three-pixel span whose gradient was fitted over two pixels steps sixteen
  shade levels to reach its second character. The character loop saturates
  rather than masking blindly; masking put a single white character in the
  middle of a dark wall, which is exactly the artifact you would not think to
  look for.

Where it falls short of the GBA: eight rows against 32, per character against
per pixel, and — because the sixteen colours are k-means centroids rather than
a palette with room to move — the dark end crushes toward black where TR's own
lightmap would have kept some chroma. The ramp itself is TR's: each row's
brightness ratio comes from the level's lightmap, applied to the centroid, so
row 0 is the unshaded colour exactly.

### What it costs

| | SHADING=0 | SHADING=1 |
|---|---|---|
| GSU instructions per frame | 2,767,317 | 3,601,158 |
| modelled cycles per frame | 11.2 M | 16.5 M |
| frames per second, TypeSNES | 1.80 | 1.20 |
| colours on screen | 13 | 37 |

The frame rate falls by more than the instruction count rises, which is the
instruction cache talking: none of the shade path is inside the cached window —
`ShadeClamp` and `SpanShade` are 512 bytes the per-scanline path needs more —
so its instructions cost five cycles a byte where a cached one costs one. See
"The instruction cache".

Neither the packer nor the interpreter needs to know which way the flag is
set. The room format carries `g` either way — eight bytes a vertex — and the
palette always has its eight rows; with shading off nothing writes the shade
map, so every character keeps the row 0 the boot-time tile map gave it. That
means `SHADING=1` is a rebuild, never a re-pack, and `make check` compares the
same two things in both configurations.

## Colour-keyed textures

`RasterFTA` now implements the source-index-zero transparency used by GBA's
`rasterizeFTA`/`rasterizeGTA` and the keyed face types in the 32X renderer.
Room and mesh faces retain `TEX_ATTR_AKILL`; pickup billboard quads are keyed
too. Grates and foliage preserve the geometry behind their holes, and pickups
no longer have an opaque black rectangle around them. This works in the
default 4bpp, Mode 7, and FX3 builds, with or without shading.

The packer distinguishes a **transparent source index zero** from an opaque
source colour that quantizes to palette zero. Texture bytes are `0..15` for
opaque colours and `$80` for source zero. Face-page bit 6 selects keying;
bits 0–5 retain the page, and bit 7 still denotes a flat-coloured face. The
batch strips the key flag before selecting the texture ROM bank. The format
uses no extra texture or face bytes; rebuild the packer output with the ROM.

Opaque spans keep their original pixel loop. In 4bpp the marker's upper bit
is discarded; in 8bpp, palette entries 128–255 remain black, including when
shade bits are ORed in. Keyed spans test the marker **before** shading, skip
the write, and advance texture coordinates and destination normally. Their
edge walk, clipping, and gradient calculations are shared with opaque spans.

The optional 4bpp shade map remains a hardware limitation: a tile containing
both a foreground pixel and a hole can only have one palette. Keyed spans
use their left-edge shade and update a tile's palette only when an opaque
pixel is written. Entirely transparent tiles leave both background pixels
and background lighting untouched. Mode 7 and FX3 carry shade in each pixel
and do not have this tile-sharing limitation; shading is still per span,
not full per-pixel Gouraud interpolation.

`make keycheck` runs independent pixel-oracle fixtures through both `gsusim`
and TypeSNES. Use the same `BUILD`, `MODE7`, `FX3`, and `SHADING` flags as the
ROM. Across all six variants, 196 cases per emulator cover opaque black,
single-pixel and odd spans, tile boundaries, fractional/wrapping UVs, both
GSU-2 framebuffers, transparent shade preservation, and face/bank dispatch.
The cache anchor is realigned for every variant; the opening-frame model
cost rises by 0.46–0.81%, with unchanged opening-frame pixels. These are GSU
model costs, not measured FX3 hardware timings. In eight broader views,
only the two containing visible keyed geometry change; disabling keying
reproduces all eight original images exactly.

## Rooms and portals

The renderer draws the level, not a room. What decides which rooms is a portal
walk, and it runs on the GSU — not because that is elegant but because it is
the only place it can run. The level is in game pak ROM, and while the GSU
holds that bus the 65816 reads dummy data. **Anything that reads level data is
the co-processor's job.** The host names the room the camera is in and gets a
picture back.

```
LEVEL.BIN, one GSU bank aligned blob

  uint16 roomCount
  RoomEntry rooms[]        16 bytes: bank, address, portal list, world offset
  ... geometry and portal lists, packed into 64 KB GSU banks
```

Rooms never straddle a bank, because R14 is sixteen bits and wraps inside
ROMBR's bank; a record split over the boundary would read its own beginning.
LEVEL1 is 38 rooms, 8349 vertices, 7420 faces and 78 portals in 256 KB.

Vertices stay in their room's own frame. Rather than transform a room into the
world, **the camera moves into the room** — one subtraction per room instead of
one addition per vertex, and the matrix builder needs no change at all.

### The walk

Breadth-first from the camera's room, and the test on each portal is two
questions in the order that rejects most cheaply:

1. **Is the camera on the inside of it?** TR portal normals are axis-aligned
   unit vectors pointing out of the room that owns them, so `dot(n, corner -
   camera) < 0` is a way *out*. No transform, no projection — three signed
   adds, since a component is 0 or ±1.
2. **Does its screen box meet the rectangle this room was seen through?** The
   four corners go through the matrix and the projection, and the box that
   comes back is intersected with the parent's rectangle. That intersection
   becomes the child's rectangle.

The second question is what stops the walk fanning out through the whole
level. Testing against the whole frame instead — which is what the first
version did — found six rooms where four are visible, and each extra room is a
full transform of its vertices.

A portal that straddles the near plane is accepted without a box: part of it
is behind the camera, its projection means nothing, and guessing wrong there
makes rooms vanish at doorways. It inherits its parent's rectangle.

Rooms are drawn in reverse of the order the walk found them, so the camera's
own room goes on top. Each is depth-sorted within itself by the ordering
table; between rooms, the traversal order is the sort.

### The clip rectangle

The rasterizer used to clamp spans to `0..FRAME_WIDTH` and skip scanlines
outside `0..FRAME_HEIGHT`. Those constants are now `V_CLIPX0..V_CLIPY1`, set
per room from the rectangle its portal was seen through. That is what keeps a
room seen through a doorway inside the doorway, and it is nearly free: the
clamps already existed, they just had constants in them.

From the corridor in room 0, looking down it:

```
room  0   rect   0..256 x   0..160     the camera's own room
room  6   rect  99..156 x  25..75      through the far doorway
room  4   rect 139..141 x  59..75      through a doorway inside that one
room  2   rect 139..141 x  59..63
```

Two pixels wide, and it still costs a full vertex transform, because the
transform is per vertex whatever the rectangle. Culling a room whose rectangle
has collapsed to nothing much is an obvious next economy.

## Meshes and the matrix stack

Rooms are walls and floors; everything else standing in a level is a mesh. The
scenery ones are in, which is the half that needs no animation — door frames,
columns, the foliage hanging off the ledges in room 20.

One thing made it cheap, and one thing that looked like it made it cheap was
wrong.

**The packer emits a mesh in the room format, byte for byte** — a vertex
count, a face count, eight-byte vertices, the same face records. So
`TransformMesh` and `DrawMesh` do not know they are looking at a mesh, and the
whole geometry pipeline is reused without a line changed. Drawing one is:

```
MatrixPush
MX_X/Y/Z = where it stands;  MatrixTranslateRel
MatrixRotateY(its angle)
R_ADDR = the mesh;  TransformMesh;  DrawMesh
MatrixPop
```

The wrong idea, recorded because it was convincing: a static mesh is placed
with a *Y rotation*, and the camera matrix is a Y rotation too, so the two
compose into a single Y rotation — call `MatrixCamera` with `(yaw - angle)`
for a camera at `(roomCamera - position)` and skip the matrix stack entirely.
Two subtractions instead of thirty instructions.

It composes the **rotation** correctly and the **translation** wrongly. The
right answer is

```
v = M·Ry(A)·m + M·(P − c)
```

and the trick produces `M·Ry(A)·(P − c)` for the translation: the instance's
own rotation applied to its position. That is exactly right when the angle is
zero, which is why it survived — room 0's only static mesh is at angle 0, and
that is the view the regression test renders. Room 20's foliage is at 64 and
128, and it was being flung round the camera; the meshes that stayed visible
did so by accident.

So static meshes go through the matrix stack, which they did not strictly
need, and the win is that the one path models will use is exercised by the
regression test every run.

A TR mesh is a stream — centre, radius, flags, vertices, then either normals
or per-vertex intensities, then textured rectangles, textured triangles, and
the two coloured variants. Textured and flat-coloured faces now come across.
Flat faces carry a palette index instead of a texture page and branch to
`RasterF`; this matters especially for Lara, whose model otherwise has holes
where her torso, limbs, and clothing use coloured polygons. LEVEL1 has 8
static meshes worth carrying, 139 vertices and 75 textured faces between
them, placed 44 times.

They are drawn after the room's own geometry so they paint over its walls.
That is right for scenery standing in a room and wrong in general — one
ordering table across both would be the honest fix.

### The three rotations

X, Y and Z are the same operation on a different pair of columns — upstream's
`X_ROTXY`, which is worth keeping as one routine here rather than three copies
of it, since code size is what the instruction cache spends:

```
a' = a·cos − b·sin      b' = a·sin + b·cos
```

with the pairs, signs and order taken from `matrixRotateX/Y/Z_c` rather than
re-derived: Y mixes columns 0 and 2, X mixes 2 and 1, Z mixes 1 and 0. Two
byte offsets in scratch pick the pair and three entry points set them.

Angles on the GSU are a table index, 256 to the turn, not TR's 16 bits. The
sine table has 256 entries, so anything finer has nowhere to go, and the
packer narrows before shipping — with rounding, since truncating leans every
joint the same way and a chain of them adds up.

All three are checked against floating point by driving the routine on its own
in the interpreter: seed the twelve words at `M_00` with a matrix that has no
symmetry to hide a swapped column, call it, read the matrix back. Worst
element error is 3.4 of 32768, which is the two truncating `FMULT`s.

## Models

An entity is a model, and a model upstream is three tables walked in step:
`models` says where its nodes and its frame begin, a node says push/pop and a
translate, the frame says a packed rotation per mesh. Three pointers advancing
together is cheap on a CPU with 32-bit registers and expensive on one without,
so the packer flattens them into a single linear stream — which is the one
thing the GSU's ROM buffer is good at. `R14` walks forward and never chases a
pointer.

A header, then one record per mesh in draw order, then a table of the model's
animations. The frames themselves live elsewhere:

```
uint8  nodeCount
uint8  pad
uint16 frameStride  6 + 3 * nodeCount
uint16 animCount
uint16 animTable    where the table below starts, within the blob

uint16 meshAddr     within its bank                     <- nodeCount of these
uint8  meshBank     relative, or $FF to skip the mesh but keep the frame
uint8  flags        bit 0 pop, bit 1 push, bit 2 root, bit 3 billboard
uint8  radius       origin-centred mesh radius / 32; zero keeps it unconditionally
int16  x, y, z      translate, in the parent's frame

uint16 frameBase    this animation's frames             <- animCount of these
uint8  bank         and which bank they landed in
uint8  frameCount
```

and a frame is

```
int16  x, y, z      the root's translate, the only one that animates
uint8  aX, aY, aZ   per node, 256 to the turn, Y then X then Z
```

What varies between frames is deliberately not in the node record. A node's
translate is static in TR — only the root's animates — so a pose is three
bytes a node, and keeping the two apart is what makes animation fit at all: a
whole node stream per frame would cost 11 bytes a node against 3. LEVEL1's
3901 frames come to 166 KB; baked flat they would have been over 800.

**Frames are placed per animation, not per model.** Lara's come to 101 KB and
R14 is sixteen bits, so a model's frames cannot be one addressable run — but
an animation's always can, being a few dozen frames. Each gets its own bank in
the table. Capping the model to what fits a bank was the alternative and would
have quietly dropped two fifths of her animations.

`DrawModel` is then upstream's `drawNodes` with the tables already merged, and
the entity placements per room are the static-mesh placements with a model
stream on the end instead of a mesh.

### Choosing a frame

`GS_ITEM` carries twelve bytes per entity: which of the model's animations,
which frame of it, a position and an angle. All of it comes from the host and
**none of it is trusted** — the
GSU clamps the animation and the frame against the counts in the model's own
header, because an index
off the end reads a mesh address out of unrelated bytes and the cost of not
finding out is a hang rather than a wrong picture.

Nothing is masked or divided: the frame's address is `frameBase + frame *
frameStride`, one `UMULT`, which is why both indices are bytes and why the
packer refuses a model whose stride would not fit one.

The root's translate is picked out by a flag bit the packer sets rather than
by the loop counting iterations, so there is no first-iteration special case
in the inner loop.

### Sprites

TR draws a pickup as a **sprite**: a texture rectangle with a world size and
no geometry at all. There is no model, no node, no animation and no frame —
and every table on the other side is indexed by one of those. So a medipack
that is "just a texture" is a medipack the host cannot hold, the renderer
cannot place and the item code cannot animate. LEVEL1 drops seven entities on
that account: five small medipacks and two large.

Rather than a second path through all of it, `buildSpriteMesh` and
`buildSpriteModel` build the thing TR is describing — **one quad, one node,
one animation, one frame** — and append it after the level's own models, so
an index stays an index. The quad's corners are the sprite's `l/t/r/b`, its
texture coordinates are `u/v` and the size TR keeps in 256ths of a texel, and
its page is the same TR tile every other face reads, with `FACE_AKILL` set
so source-index-zero pixels preserve the background.

The billboard-specific difference in the renderer is a flag on the node:
`NODE_BILLBOARD` says **drop the rotation out of the matrix**. `DrawModel`
replaces the 3×3 with identity after the node's rotations, leaving the
translate column alone — so the quad's own axes become the camera's and it
faces the viewer wherever either of them stands. Nine stores, once per sprite.
It is checked from three sides in the simulator and from the front on the
cartridge: `make check`'s pickup test holds the whole picture still, confirms
it is still, and asserts the pixels that change when the medipack is taken.

Effects -- blood, ricochet, splash, bubble, smoke -- extend it on FX3: one
model whose node is a run of such quads, its frame choosing which (see
"Effects (FX3)"). The muzzle flash is a mesh, not a sprite: "Effects (FX3)".

**Room sprites** (FX3): the rooms' own sprites -- upstream's
`renderSprite` at a room vertex: in the Caves, pebbles and rocks on the
floor; LEVEL1 places 43, LEVEL2 81 -- are static instances of the same
one-quad meshes (one per sprite texture used), placed at their vertex with
the angle byte `$01`. TR stands every static on a quarter turn (the packer
refuses one that is not, on FX3), so no static has that byte, and the GSU's
`InstTurn`, in DrawRoomMeshes' place for `MatrixRotateY` (the same eight
bytes), reads it as "face the camera": the 3x3 to identity, which the room
path's yaw-only transform takes as yaw zero. The runner draws room 10 with
and without them (the placements' bank byte set to `INST_NONE` in a copy of
the ROM) and holds that they change the picture; `ROOMSPRITES.json` lists
where each landed in LEVEL.BIN.

## Entities on the host

The host had no idea what was in the level. It could walk around one, because
the GSU answered questions about the floor under the camera, but it could not
have named a single thing standing in a room — and everything above the
renderer is a question about those things. So the traffic goes both ways now:

```
down    GS_ITEMS   $71:0000   the level, as game logic asks about it
up      GS_ITEM    $70:F800   twelve bytes per entity, written by the host
```

**Down** is a flat list of what exists, in the shape game logic asks about
rather than the shape the renderer draws from — type, room, model, world
position, angle, and behind them the animation graph, the triggers and TR's
navigation boxes. It cannot be the renderer's copy: that one is per room and
holds bank addresses, and its positions are in each room's own frame. The
packer builds both from the same source and, crucially, **numbers the entities
once for both**. The renderer walks them room by room and the host walks them
in level order, so neither could have counted as it went; the slot each entity
owns is decided up front and read by both. Getting that wrong reads another
entity's state, silently.

**It is not copied.** It used to be — 22 KB into WRAM, which was most of the
room bank `$7E` had left. The host clears RAN before it calls into the game
rather than after, so game pak RAM is on this side for the whole of an update
and a far pointer reaches it. `.bss` went from 50 KB to 25, and the next table
the packer adds costs nothing here at all. The blob's first word is its own
length, which is also how the host knows it has arrived.

The GSU does not interpret any of it. It copies the packer's blob into game
pak RAM — every frame, because a "have I done this" flag would live in RAM
that comes up undefined, and the copy is under a tenth of a percent of a
frame. Cheaper than a bug.

**Up** is twelve bytes an entity — an animation, a frame, a position and an
angle. It carried only the first two until nothing in the world needed to
move; the renderer read where an entity stood out of ROM, which is exactly as
far as a creature could get. The host owns entity state now and the renderer
draws what it is told. Positions go up in world coordinates and the renderer
takes the room's origin off, which is one subtraction an entity rather than
one a vertex.

Both crossings happen in the main loop rather than in V-Blank. The GSU is
stopped there, so taking the RAM bus back costs nothing — there is nothing to
stall — and V-Blank is the window the host is already tight in.

`src/game.c` now keeps real per-entity state: type, room, model, position,
angle, and which animation at which frame — an `ItemObj` of exactly 32 bytes,
because an index into an array of structs is a multiply on this compiler and
a power of two is a shift.

### Walking the animation graph

An animation does not simply loop. TR says which animation follows and at
which frame, and for an idle that is itself from the top; for a jump it is the
landing. `olAdvanceItems` is that, which is `animProcess` minus the states
that would let anything *choose* a different answer:

```c
f = item->frameIndex + 1;
if (f >= olAnimFrames(a)) {
    f = olAnimNextFrame(a);
    a = olAnimNext(a);
}
```

and it runs once per 30 Hz tick: an animation's length is its tick count, and
the pose the renderer is shown is `frameIndex / frameRate`, both out of TR's
own animation records.

The graph arrives already in the host's terms. TR numbers frames globally and
chains by global frame number; the packer resolves each `nextFrameIndex` into
a frame local to the animation it lands in, so the cartridge needs neither a
divide per transition nor TR's `anims` table. 277 animations come to 2.2 KB;
the GSU-2 blob around them — entities, transitions, commands, triggers, the
navigation graph, the whole sector table, the static meshes she can walk
into, current-frame creature aim points and exact navigation IDs — is 61,490
bytes of the 65,534 the length word and the bank allow. LEVEL2 uses 62,405.
The source must also share a 64 KB ROM bank with the room directory:
actual expansion room is **2,826 bytes for LEVEL1 and only 119 for LEVEL2**.
The packer and `mkbanks.py` both reject a crossing; future table additions
need more compression or a revised staging layout on GSU-2. FX3 now uses
[independently banked ROM tables](HOST_TABLES.md), with only mutable sectors
in WRAM; neither the aggregate blob nor its source-bank limit applies there.

The aim table follows the static room index. It holds an animation count
and a relative offset per animation; zero retains the animation-union box.
Each creature animation has a tick count and one relative point offset per
game tick. Identical local XYZ points share six bytes, even across animations.
The packer resolves the shared engine's `getFrame()` choice, including ties
toward the earlier pose and its original `frameEnd` guard, before computing
the upper-third aim point. That leaves no frame division or box arithmetic
on the cartridge's creature-aiming path. LEVEL1's 1,457 creature ticks share
574 points in 7,020 bytes; LEVEL2's larger animation index makes it 7,072.

The navigation map follows the aim table; the blob's final word points to
it, leaving all earlier offsets unchanged. Each sector has a two-bit tag:
NO_BOX, copy the previous Z sector, copy the previous X row, or read a box ID.
Predictors stay within their room. Literals use 10 bits in LEVEL1 and 9 in
LEVEL2; a 16-bit rank per 32 sectors bounds the literal-count scan to 31 tags.
The longest predictor chains in those levels are 14/13 steps. This costs
2,300/2,109 bytes including the footer, with no decompressed work buffer.
`src/nav_sector.asm` reads it from the same staged bank as other host data
on GSU-2, or through its independent ROM pointer on FX3.

It was 65,225, with 309 bytes of headroom, until the sector table was
restructured. A sector carried six bytes and a slope stream two and a half
more: its two heights, the rooms below, above and through the wall, its
trigger, and four five-bit slants. Everything after the heights repeats —
a stacked room puts the same `roomBelow` on every sector over it, a ramp
puts the same slant on every sector along it — and LEVEL1's 4,026 sectors
carry 583 distinct combinations. So a sector is now four bytes, its heights
and a 16-bit link into a table of those combinations at eight bytes each,
and the whole thing is 20.8 KB where it was 34.2. A sector flag on the
roadmap costs 583 bytes now rather than 4,026, and the 13 KB that came back
is what the per-item and per-placement fields on it were waiting for. The
reader is `src/room.asm`; the record is described in `src/level.h`.

That blob outgrew what was left of game pak RAM bank 0, so it is staged into
**bank 1** — 64 KB the renderer never touches, on a cartridge that declares
128 KB because the GSU-2 package cannot address more anyway.

### Choosing

An entity has a `state` and a `goalState`, and when they differ it looks for a
way across. TR keeps that as two nested tables — an animation lists the states
reachable from it, and each of those lists the frame ranges it is reachable
from — and the packer flattens them into one list per animation, in TR's own
order so the first match still wins. 249 transitions for LEVEL1.

```c
if (goal != state && transCount) {
    while (n--) {
        if (transGoal[t] == goal && f >= transBegin[t] && f <= transEnd[t]) {
            a = transAnim[t]; f = transFrame[t]; state = animState[a];
            break;
        }
        t++;
    }
}
```

which is `ItemObj::animChange`, with the frame numbers already local.

### Commands

An animation also carries commands: move the entity, kill it, make a noise,
turn it round. TR packs them as a variable-length stream, so walking past one
means knowing its argument count; the packer unpacks them into fixed records
and the cartridge needs no such table.

They split the way upstream splits them — the ones that change where an entity
*is* run once at the end of the animation, the ones tied to a frame run when
that frame comes round. `OFFSET` moves the entity along the way it faces
rather than along +X, which is what makes a door slide correctly, and is the
first thing on this target to use the ported `phd_sin`/`phd_cos` for something
other than the camera.

`SOUND` fires on its frame at the item's position -- her footsteps, the
pull of a lever, a wolf's snarl -- through `Sound_playItem`, which is
upstream's `soundPlay` over the tables the boot copied in; see "Sound". Bit
14 and up of its argument are TR's land/water environment flags, masked off
as upstream masks them. `JUMP` and `EMPTY` landed with item physics and the
weapon state.

**A transition range legally ends one frame past its animation.** `animProcess`
advances the frame and *then* asks whether a transition applies, so the index
it tests is one beyond the frame just drawn, and TR's ranges are written for
that. A shut trapdoor is a one-frame idle whose transition range is 0..1 over
an animation that only has frame 0. Clamping that to 0..0 — which looks like
obvious hygiene — means the door can never open, and it took building the
trigger that opens it to find out.

## Triggers

The world responds now. Standing on a sector that TR marks with a trigger
activates whatever it names, and a dart emitter in room 6 of LEVEL1 goes from
shut to open in about a second — and stays open when you walk away.

**TR's `FloorData` never reaches the cartridge.** It is a bytecode — portals,
slants and triggers interleaved in one stream, terminated by a bit — and
walking it needs a parser on the side that is least able to have one. What the
host needs to know is "standing here, which entities does that activate", so
that is what a sector carries: two bytes, an index into a table of exactly
that, or `$FFFF`. 93 triggers for LEVEL1, resolved at pack time.

The GSU already worked out which sector the camera is in, for the floor; the
trigger rides along with it in the same staging block.

### Latching

It stays open. TR's activation is a five-bit mask per entity, and a trigger
carries the bits it sets:

```
switch    flags ^= mask
antipad   flags &= ~mask
otherwise flags |= mask

active when every bit of the mask is set
one-shot latches ONCE, and nothing can fire it again
```

which is `checkTrigger`'s ACTIVATE_OBJECT branch. The bits stay set, so
stepping off a pad leaves the door open — the difference between a door and a
door that follows you around — and a door wired to two floor pads opens on the
second one, which is what the mask is *for*.

What an entity does about being active is one dispatch:

```c
static void olItemUpdate(unsigned int i)
{
    ol_itemGoal[i] = (ol_itemFlags[i] & OL_ITEM_ACTIVE)
                   ? OL_STATE_OPEN : OL_STATE_SHUT;
}
```

which is upstream's `Door::update`, the same two lines. It is a **family, not
the general case**: doors, trapdoors, bridges and the dart emitter all use
state 0 for shut and 1 for open, and a floor trigger is overwhelmingly pointed
at one of those. A switch has to know Lara pulled it; a pickup has to
disappear; each is its own class in `object.h` with nothing here yet to reach
it. Only entities a trigger names are driven at all, so nothing else is being
told to be a door.

### The timer

How long it stays active. TR's is `Object::isActive`:

```
no timer      stays as it is
counting      decrement; at zero, latch expired
expired       inactive
```

and the trigger sets it **unconditionally, before checking whether the entity
is already active** — which is upstream's order and the entire reason a pad
keeps a dart emitter firing while you stand on it. Set it only on a fresh
activation and the emitter fires once and stops.

That is what the two halves of the trigger test are. Room 6's pads carry TR's
one-tick timer, so the emitter opens while the camera is on one and is asked
to shut the moment it steps off; room 2's trigger carries no timer, so what it
opens stays open. Both are facts about LEVEL1.

The countdown runs on the 30 Hz tick clock, and the figure a trigger carries
is converted the way upstream converts it — seconds times thirty, except the
literal one-tick case. The coupling to the frame rate is gone; see "The
clock".

### The types

Upstream's `checkTrigger` switches on the trigger's type before it fires
anything, and so does this one now. A **pad** fires only with her feet on
the floor -- `pos.y == roomFloor`, which the landing and the end of every
tick both establish -- so jumping over one does nothing, a fact TR levels
are built on. A **switch**, **key** or **pickup** trigger names the thing
used as its first entity, and that entity is *asked* rather than
activated: `Switch_use`, has the lever been pulled; `Key_use`, is the key
in; `Pickup_use`, has it been taken. Only then do the rest fire. Native FX3
COMBAT requires READY after `Weapon_update`; landed blocks fire OBJECT
triggers with the corresponding camera mode and ordering. GSU-2 COMBAT and
OBJECT, and DUMMY on both variants, return. Heavy creature triggers remain.

Two actions besides activating entities reach the host: **END** sets
`olLevelEnd` (upstream's `nextLevel`, with nothing yet to load), and
**SECRET** sets a bit of `olSecrets` (`gSaveGame.secrets`). The packer
keeps a trigger that has either even when it names no entity -- it used
to drop those -- and `OL_DUMP_TRIGGERS=1` lists every trigger with its
type, mask, timer, entities, actions and the sector it sits on. Cameras,
flip maps and the soundtrack are still parsed past.

### Active from the start

TR's item record carries two facts about activation, which the packer
always shipped and the host only now reads: a five-bit mask and a hidden
bit. The hidden bit is upstream's "once -> invisible": the thing is
INVISIBLE until a trigger shows it, and activation makes it ACTIVE (TR's
own rule, for everything; upstream's only does it for creatures through
`enable`). A full mask is "active from the start": `ItemObj::init` clears
the mask, sets `ITEM_FLAG_REVERSE` -- in `flags2`, since the first byte
is full -- and activates the thing, and `isActive` reads backwards from
then on, so a trigger that fills the mask switches it *off*. No item of
LEVEL1 or LEVEL2 sets either bit, so this is exercised by nothing here
and is written for the levels that do.

Creatures wake the same way, and only that way: a trigger names them, or
the level loaded them active. The five-sector proximity wake the port
carried until it had this is gone, and the runner's creature scene stands
her on the pad in room 13 that names the wolves before it expects one to
come.

## The camera follows her

She was the camera. Now the camera is derived from her, which is `camera.h`'s
`updateFollow`: put the view `targetDist` behind the target at the target's
own angle, and lerp toward it a tick at a time so it does not snap round
every time she turns.

And it is clipped, which is `getBestLocation` and the `trace`, `clip`,
`checkWall` and `checkHeight` behind it, all on `Room_query`. The wanted
view is traced from the target sector by sector and stopped at the first
wall, floor or ceiling; when that leaves it within 768 of her, upstream's
four cardinal candidates are traced too and the one nearest the present
view wins; and whatever is chosen is kept its radius off every wall. The
first thing it moved was the opening view: 1536 behind her at the level
start is a sector inside the entrance wall, and the camera had been there
since the port began. It sits 264 behind her now, its radius inside the
wall, and `make sim`'s default view followed it.

Three cuts, because a trace is two 32-bit divides and a dozen queries, the
fallback is eight traces, and upstream runs all of it every frame on a
processor that can. The divides are skipped on an axis with no delta,
which the flat follow geometry makes true of the vertical one always. The
fallback's answer is kept while its question is the same -- her sector,
her height band, her facing to a sixteenth of a turn -- and asked at most
once in eight ticks otherwise, because against a wall the question comes
every tick and eight cold traces is 100 K cycles. And the whole answer,
traced and clipped, is kept while *its* question is the same: the sector
and height band she is in and the sector and height band the view wants.
Standing still or running straight, that is every tick; a turn or a sector
boundary re-traces. Without that last one the camera cost 25 K cycles a
tick, the walking tick was 81 K, the main loop's idle share fell to 7% and
the clock slipped to 27 a second -- which is also how the tick's real
budget turned out to be a field and a half rather than two, the host
needing the rest. With it the camera costs 5 K standing and 6 K running.

**2026-09-23: the answer is upstream's again.** Keeping the traced and
clipped *place* froze the goal for the ten ticks she takes to cross a
sector: an oracle in the runner (`follows upstream exactly in the open`),
which replays upstream's arithmetic -- want = target - distH (sin, cos) of
her facing, a floored eighth of the way there a tick -- against the
cartridge's own view, found it matching only on the ticks she crossed a
boundary, a 60-unit sawtooth in between, 2,090 behind her running where
upstream settles nearer 1,870, and a view that stopped wherever she had
entered her last sector. The fallback had the same fault, cached as a place
while upstream's candidate moves with her, which held the opening view
still for half a second while she ran. Now only verdicts are kept -- clear,
fallback (and which of the four), or blocked -- and the goal is worked out
afresh each tick and clipped each tick. In the open the view is bit-exact
with upstream from the moment the line clears. A blocked line with no
fallback still keeps its hit point within the key: the one approximation
left.

Paying for that honestly took three things. The clip (six queries in
upstream) proves most of its answers from one: with the view between its
sector's floor and ceiling, clear of both by a slope's reach (|s|/4 a unit,
under 64|s| across the probe's 255), every probe that stays in the sector
reads clear and every snap stays in the sector column; only a probe that
crosses into the next sector is asked, at most one an axis. The last clip is
kept for the identical question, which standing still asks every tick. And
the 32-bit products of distance by sine are kept per angle -- running
straight asks the same one -- with a quarter turn needing none. The camera
is 125 K master clocks a tick walking against 114 K for the approximation.

**The height and the pitch.** Upstream sits the follow view ten degrees up
(`CAM_ANGLE_FOLLOW`, plus her own pitch when she swims): distH = 1536 cos
10 = 1512 across and distV = 267 above the target, and points the view down
at the target. The port had it level at the target's height, because the
view matrix is a yaw. Now the place is upstream's (1513 and 263 on the
port's coarser 1024-entry sine table) and the look-down is a shear: the
frame program projects the target -- its depth along the view from the
probe (her x and z, the target's) and its height from the host's R11,
`ol_camTgtY` -- through the same reciprocal and FMULT every vertex takes,
and moves the projection's centre line (`FR_CY`, which the four projection
sites read instead of `SCREEN_CY`) to where it lands. The target is where
upstream's pitch puts it, on the centre line; everything else differs from
a true rotation by 1 - cos 10 degrees, 1.5%. It is the pitch Doom and Build
use, and it costs the GSU two multiplies a frame.

**Combat and look.** Upstream's Camera::update has a mode, set each frame
before it runs and put back to FOLLOW after: `toCombat` while her guns are
drawing or ready, `s_checkLook` while Look is held in STOP or SURF_TREAD.
Combat aims the view 2048 away along her arms' aim (her head and torso with
no target), ten degrees down; look, 768 along her head and torso; both with
the target 256 higher and without the fallback. move()'s divisor is the one
subtle part: 8 is a shift, which floors, and 4 is a C divide, which
truncates, and the FX3 routine takes the speed from `camera+20` to keep
both. The pad's Y is Look; while it is held with nothing targeted the d-pad
turns her head -- two degrees a tick, clamped to -42/22 and +-44 degrees,
the torso with it -- and is spent there, so she stands still.

The load tick was the other casualty: `olLevelInit` and the first clip
together took it past eight fields and the NMI dropped one, so the tick
the level arrives in now ends after the init, and the next is the first
real one.

**The view and the probe point stopped being the same thing**, and that is the
structural half of this. The GSU took one position and used it for both -- the
view matrix and the floor under your feet -- because the two could not
disagree. They can now: the floor she walks on is under *her*, and the room
the renderer starts from is the one the *camera* is in. So there are two
handovers, and `GS_PROBE` is the second.

Which immediately found a bug that had been latent: the wall clamp compares
her position against the corner of the sector the query answered with, and if
those disagree the "clamp" is not a nudge but a teleport across the level. It
threw her four thousand units. Guarded now -- clamp only if she is actually
standing in the sector that was queried -- and the guard has outlived the
bug: `Room_query` answers in the same tick now, so the two can no longer be a
frame apart, but nothing about a guard that cheap is worth un-writing.

The camera **snaps** rather than lerping when a level loads: upstream inits
the view a hundred units behind the target and lerps from there, and the
first dozen frames are the view flying in, a frame the two renderers cannot
agree on. The snap arrives where that lerp would have settled -- traced and
clipped like any other tick's, since 2026-09-03; before that it took the
unclipped location, which at the LEVEL1 start is a sector inside the
entrance wall, and that was the first frame drawn.

Four things a reading against upstream on 2026-09-03 found and fixed, in
the order they show:

- **The yaw.** Upstream ends every update with `anglesFromVector` from the
  view to the target: the view *looks at her*. This set the yaw to her
  facing, so while the view swung round behind her after a turn, or
  whenever a wall had displaced it, she was not in the middle of the frame.
  `Camera_lookAtTarget` is upstream's; the pitch it also derives is not
  used, the renderer's view matrix being a yaw.
- **The target's height.** Upstream puts it three quarters up her bounding
  box and eases it a quarter of the gap a tick, so a crouch, a hang or a
  fall moves the camera; this had a constant 512 above her feet, set at
  once. `Camera_targetY` is upstream's formula on the animation's box. The
  view rides level with the target -- upstream looks ten degrees down at
  it from 267 above, and without pitch the target's own height is where a
  level view keeps her in the middle -- so the opening view rose 67 units.
- **The lerp.** Upstream's `>> 3` floors; a divide truncates, and from the
  negative side the view stopped seven units short. `floorShift`.
- **The opening view.** With the snap clipped, the fallback's candidate
  traces to the present view pass -- the old unclipped snap had put that
  view inside the wall and failed every one, which is the only reason the
  camera ever sat 264 behind her at the start. Upstream's rule gives the
  cardinal candidate nearest the present view, and at the LEVEL1 start that
  is 1536 to her west, looking east. `make check`'s opening view followed
  it, and `frame.baseline` with it.

The runner's camera test holds the port to those: the first published view
clipped and level with her chest, the yaw the direction to her at every
tick through a run down the corridor and a quarter turn on the spot, and
the view settled behind her within three degrees after each.

The new opening view costs more to draw than the old: it looks across the
corridor rather than down it, and she is in it, 1536 away, where the old
view had her inside the near plane. 8.77 M against 7.05 M for the view
from behind her, after a pass on what the side view exposed -- see "The
instruction cache".

## Lara

Lara is item 0 of every TR level and the host ignored that: the camera walked
around and the entity stood where the level had put her. She *is* the camera
now — position and angle, at her feet rather than her eye — which is what
makes "is Lara near this switch" a question with an answer. Every object that
has to be **used** rather than stood on was waiting behind that.

### She moves at the speed her animation says

This is the thing `lara.h` is built on, and it is not obvious until you look
for where TR writes a speed down: it does not. Every *animation* carries a
speed and an acceleration in 16.16, `ItemObj::move` evaluates them at the
frame she is on, and a walk differs from a run because the animations differ.

What stood here before was `items[0].x += sin(angle) * MOVE_SPEED`, with
`MOVE_SPEED` a number nobody chose, applied whatever she was doing. With that
in place a state machine is decoration — every state moves identically, so
which one she is in cannot matter. So the packer now carries `speed` and
`accel` per animation (eight bytes each, in a table of their own rather than
widened into the animation record, which is read six times a frame and wants
to stay a shift), and `ItemObj::move` is ported whole. Her run comes out at
**47 units a tick**, which is TR's, and nothing in this port says so.

### And she leaves the ground the same way

The air states — `COMPRESS`, `JUMP`, `FALL`, `LAND` — came across on the same
principle, and it is worth saying plainly because it is the second time it
paid: **nothing in this port decides how high she jumps.** TR's launch is an
*animation command*, `ANIM_CMD_JUMP`, carrying a vertical and a horizontal
speed; `ItemObj::animCmd` reads them and sets `ITEM_FLAG_GRAVITY`, and
`ItemObj::move` does the arc with upstream's own `GRAVITY`. What used to be
here was `ol_velY` and a `JUMP_SPEED` of `GRAVITY * 3` — three constants
nobody chose — and all three are gone.

What used to be left in `src/game.c` -- a floor-follow with a step limit, a
wall clamp per sector, a ceiling clamp -- is gone. Upstream's `c_` collision
handlers are what catch her now: see "Her collision".

The ceiling clamp had to learn the same lesson: **a clamp that moves her
without touching her speed pins her there for ever.** It had always just
pushed her down out of a low roof, which was fine while nothing pushed back;
the moment gravity owned her Y, a jump under a low ceiling put her above it
every step and the clamp put her back, and she hung at exactly
`ceiling + HEAD_ROOM + EYE_HEIGHT` until the test gave up. TR's ceiling
collision zeroes the upward speed, and that one line is the whole difference.

The walk test had to be corrected for it, and the correction is the
interesting part. It asserted she comes back down to **the Y she took off
from** — fine while the jump was purely vertical, and wrong the moment the
push came from `ANIM_CMD_JUMP`, which has a horizontal component. She now
travels while airborne, the corridor she lands in rises, and the number the
test read was two `STEP_LIMIT` steps of her *climbing onto higher ground*.
Nothing was broken; the test was measuring against a floor she had left. It
now asserts she is on the floor she is actually over.

The arc is TR's own thirty-three ticks now that a step *is* a tick: the push
from `ANIM_CMD_JUMP`, the height from upstream's `GRAVITY`, at the rate
upstream runs them. While a step was a rendered frame it was three steps and
a thousand units — the right shape squeezed through a fifteenfold conversion.
See "The clock" for why that era ended.

### Her ground states

`src/lara.c` is upstream's `updateState` for the playable dry-land slice:
STOP, WALK, RUN, the three turns, back-hop and back-walk, both sidesteps,
compress and the four directional jumps, forward/back falling, landing,
standing roll, swan/fast dives and death. Handstands extend the hanging
climb path. `s_rotate`, `s_checkWalk`, `s_checkDeath`,
`s_checkRoll` and `s_checkFront` sit under them. Upstream dispatches through a
table of member-function pointers built by the same macro that declares the
enum; this is a switch, because the other states have no handler here and an
arm that does nothing is what upstream's unimplemented handlers do too.

The controls follow TR's combinations: Down is the backward hop, L+Down is a
back-walk, L+Left/Right sidestep, Up+Down roll -- which a d-pad cannot press,
so SNES X sends both, as a face button does on upstream's NDS -- and A
followed by a direction chooses the jump. The animation graph still decides when each requested state
can begin. Lara's collision handlers normally set `extraL->moveAngle`; until
the full `CollisionInfo` path arrives, the SNES host carries that Lara-only
angle separately and applies the same 0, ±90 and 180 degree state offsets.

Nothing assigns her an animation. She asks for a goal state and TR's own
transition table grants it when she reaches a frame that offers one: holding
up takes STOP → RUN through animation 6, letting go takes RUN → anim 8, the
run-to-stop, which plays through and arrives at STOP. Standing still she ends
on **animation 103**, which is `ANIM_STAND_NORMAL` — where upstream's Lara
stands, arrived at by following the same chain rather than by being told.

### Her collision

`src/collision.c` is upstream's `CollisionInfo`, `ItemObj::collideRoom` and
the `c_` handler of every state this port has, read beside `item.h` and
`lara.h`. A tick ends with `collideRoom` asking four questions of
`Room_query` -- under her, and at her radius ahead, left and right, all
160 units above her head so a doorway lintel counts -- and classifying the
answer as upstream does: a wall in front, a wall to one side she slides
along, a ceiling, a floor-and-ceiling squeeze. The handler for her state
then does what upstream's does with it: a walk into a wall picks the
stand-still animation whose foot is forward; a run into a wall from more
than `LARA_SMASH_HEIGHT` splats; a step down more than 128 units chooses
the descending stride; a floor more than a step below starts the fall; a
steep floor starts the slide, forwards or backwards by which way she
faces; a landing at speed costs health by upstream's formula, and past 154
kills her.

`s_checkFront` is upstream's now too -- a throwaway probe through
`collideRoom`, on a second `CollisionInfo` because a struct copy is a
memcpy on this compiler -- and asks only its two points, since the left and
right ones can only answer CT_LEFT and CT_RIGHT. `COMPRESS` uses
`s_getFront`, the point 256 units ahead, as upstream does.

`Room::collideStatic` is in too, since the blob had room for its data: the
packer walks each room's static mesh placements, drops the plants and the
ones TR flags as having no collision, turns each mesh's collision box to
the quadrant the mesh faces, and ships the eleven LEVEL1 survivors with a
table of where each room's run starts. `collideRoom` translates the box to
her -- to her rather than to the world, because the world does not fit in
sixteen bits here and the distance to a mesh does -- and tests it against
her cylinder as upstream does, pushing her out and naming the side. The
first thing it stops her at is the rock face behind the entrance, which
she used to run straight through to the wall a sector behind it.

### Hang and climb

Upstream's `c_checkClimbUp`, `c_checkHang`, `c_hang`, `c_checkGrab`,
`c_checkSpace`, `c_checkSwing`, `alignAngle` and `alignWall` are across,
with the states they drive: HANG, REACH, HANG_UP, HANG_LEFT and HANG_RIGHT,
the REACH arms of JUMP and FALL_BACK, and STOP's four-unit nudge on
Up+Action that puts the front probe onto the ledge. Two and three clicks she
climbs, `ANIM_CLIMB_2` and `_3` through `STATE_HANG_UP`; up to seven and a
half she jumps for, at the speed `c_checkClimbUp` works out and the
animation's JUMP command spends in place of its own -- upstream's
`vSpeedHack`, the one place this port lets a command's speed be overridden.
A running jump with Action held goes to REACH and `c_jump` asks
`c_checkHang` on the way past a ledge. Three things are not here: the
handstand, the TR2 wall climbing behind `c_climbSide`, and the camera
angles the hang states set.

`getBoundingBox(true).minY` is the animation's box here rather than the
frame's, because the packer unions the frames' boxes over an animation; a
hang measures against the animation's highest hand. It has not mattered
yet. What is measured is the two-click climb, in room 10: `make check`'s
tenth test. The grab has nothing in LEVEL1 to catch -- the host table has
no four-to-seven click ledge whose sides are clean, which is TR1's level
design and not a gap -- so it waits for a level that has one.

The scan is in `src/room.asm` as `olStaticHit`, for the same reason
`Room_query` is: written in C, one placement cost the tick 3,900 cycles a
probe -- nine far reads and a `boxIntersect` through two pointers, at the
compiler's hundred cycles a read -- and the rock behind the entrance is in
the room she spends the whole opening in. The assembly walks the room's
run, rejects by distance, translates and tests, and hands C the box only
on a hit; the push-out and the naming stay upstream's, in C. It costs the
idle tick 2,000 cycles and the running one 2,700.

Frame ranges are the one translation. Upstream tests `frameIndex` against
global frame numbers; this port's is local to the animation. Lara's are
fixed data -- the walk is animation 1 at frame 22, the run animation 0 at
frame 0, the back-walk animation 41 at 598 -- so upstream's 29..47 on the
walk is 7..25 here, and only when the animation *is* the walk.
`OL_DUMP_ANIMS=1 ./build/packer` prints the table.

Not ported, each a slice of its own: wading and the TR2 wall climb, and
lava, which upstream also leaves for later. The lean (`angle.z`) is FX3's:
upstream's tilt into the run's turn and the swimming turn, its decays and
resets, drawn after her pitch through GS_ROLL and the GSU's `ItemTilt`.

### The water

Upstream's `updateWaterState`, `updateSurface` and `updateUnder`, `c_swim`,
`c_surf`, `c_checkWallUW`, `c_checkWallSurf` and `c_checkWaterOut` are
across, with the eleven states they drive: UW_TREAD, UW_SWIM, UW_GLIDE and
UW_DIVE under the water, SURF_TREAD, SURF_SWIM, SURF_BACK, SURF_LEFT and
SURF_RIGHT on it, WATER_OUT and DEATH_UW. `Lara_update` is upstream's
`update` now rather than its ground path: the water state first, then her
air, then one of the three updates, then animation, movement, collision
and the room.

Three things the water needed that the land did not. Her pitch: upstream's
`angle.x`, which ItemObj has no byte for, so it is a global
(`olLaraAngleX`) that `updateUnder` moves her along -- TR's own
arithmetic, a 1.14 sine against `vSpeed` and sixteen bits off, so the 200
she swims at is 50 units a tick -- and that the renderer does not draw:
she swims level in the picture while going where her nose points, and TR's
convention is Up for nose down. Her air: `oxygen`, which upstream aliases
to `ItemObj::timer` and the active list reads here, so a global too; a
minute of ticks, ten won back a tick out of the water, five health a tick
when it runs out. And `updateRoom`'s offset: at the surface the room query
goes her radius *down*, so the room she is in is the water she floats on
and `updateWaterState` can tell the surface from the air above it; under
water and on land it is her feet, as before.

The climb out is the one place the water reads a probe's room rather than
hers, and it is worth the paragraph because the level geometry makes it
look impossible. Rooms carry a wall sector round their edge, so the rim of
LEVEL2's pool (room 48) is room 43's column directly above the pool's west
*wall*, and a probe from room 48 at that column finds no floor.
`collideRoom` carries the room the middle probe resolved to -- room 43,
the middle probe being taken above her head, over the portal -- into the
front probe, exactly as TR's `GetCollisionInfo` does, and the rim is
there: one click above the surface, `h = -257`, `ANIM_WATER_OUT`.

What TR1 does not have, and so what is not here: wading. ANIM_WADE,
ANIM_SWIM_STAND, ANIM_SURF_OUT, ANIM_SURF_STAND and ANIM_UW_ROLL are past
her 160 animations, so the arms that would play them play what she has (a
stand), and `c_checkWallUW`'s stand-up-in-the-shallows arm is out: TR1
swims in the shallows. WADE stays a state of the water column, entered
from a shallow pool as upstream enters it and read by the collision
handlers' guards, because nothing about the column changed. Also not
here: the look camera at the surface, the 22-degree pitch upstream gives
the follow camera there (the view matrix has no pitch), the splash and the
breath.

It is measured on LEVEL2, the first scene that is: `make BUILD=build_l2
LEVEL=LEVEL2 check` runs the runner's eleventh Lara scene -- a drop into
the pool at (29696,-1800,22528), Down and Jump to the surface, Up and
Action west to the rim -- and skips the thirteen LEVEL1 ones; the runner
takes `OL_LEVEL` and each scene skips off its own level. The frame
baseline is LEVEL1's opening view and is compared only there. Each build
directory has its own copy of the runner, so the two levels' checks run
side by side.

On land it costs the tick one walk of the water column, 2.2 K cycles with
its sector index: 46.5 K idle and 63.3 K running, against 46 and 62
before. Under water it is the walking tick's collision plus the column
twice and the pitch arithmetic, not yet profiled -- the profile scene is
LEVEL1's. The cost that matters is code: `lara.c` went from 6.3 KB to about
13 under 816-tcc and `collision.c` grew 3 KB; `object.c` and `common.c`
moved to bank 4 to make room, and the three resident halves have 969, 3709
and 1011 bytes left. See "The structural one".

### A speed and an acceleration convert differently

They did, at least, while a logic step was a rendered frame worth about
fifteen ticks: a speed scaled by fifteen and an acceleration by fifteen
squared, and confusing the two made her turn reach in sixteen seconds a speed
TR reaches in half of one — the cap right, the climb fifteen times too slow,
and `STATE_TURN_FAST`, which exists precisely to be entered once the cap is
passed, never happened. The `turnSpeed` accumulator needed its decay
reordered and its sign hand-preserved on top of that, and every one of those
fixes was behaviour this port owned instead of TR's.

All of it went away when the clock did — see "The clock". A step is a tick
now, upstream's constants come across at their own values, and the decay and
the turn run in upstream's order with upstream's figures — `Lara_update`'s
tail says so in as many words. `OL_SPEED` and `OL_ACCEL` in `fixed.h` survive
as identity multipliers marking every conversion site, so a future PAL
scheduler or a slow-motion mode has one rate boundary to change rather than a
hunt through the constants for which kind each one is.

She is marked hidden rather than drawn: the camera is at her eye, so drawing
her costs a transform and a clip to put nothing on the screen. The marker is
an animation index of `$FF` in the channel the host already owns, so the
renderer skips her without a second mechanism.

## Using things

Upstream gives every object class a `collide(lara, cinfo)` and calls it
for each entity near her every tick; the class decides whether she is
close enough, moves her onto the spot, and plays the interaction. That is
`src/use.c` and `src/block.c`: the same methods, dispatched on the class,
over a list `olLevelInit` makes of the entities that have one --
switches, pickups, keyholes, puzzle holes, blocks. Doors stay
`Lara_collideItems`'s and creatures push their own way.

**Close enough is a box in the thing's frame.** Upstream's `Limit` tables
are here with upstream's numbers: a switch wants her 312 to 512 along its
facing and within 200 across, at its height, facing the same way within
thirty degrees; a pickup wants her within 256 behind it; a hole is the
switch's box; a block wants her 512 to 692 short of its centre, square
on. `Object_checkLimit` turns her offset into the thing's frame with the
same rotation `collideBounds` uses, the inverse of the one the OFFSET
command applies, and tests the box and the angle windows. The one pitch
that exists is hers under water, against the pickup's 25 degrees.

**The interaction is entered outright.** `ItemObj_animSkip` walks her
graph to the state wanted without moving her, and asks for the state
after it; `ItemObj_moveTo` puts her where the thing wants her, snapped on
land and swum onto at sixteen units a tick under water. Her hands are
BUSY from then (`olLaraWeaponState`, the corner of `extraL->weaponState`
the port has) until the animation frees them, which TR does two ways:
an EMPTY command at the end of the pull, the pick-up and the key, or a
HANDSFREE effect on a frame of letting go of a block. Both are in
`ItemObj_animCmd` now; before, every interaction would have refused her
after the first.

**The switch.** Standing at the lever with Action she is turned to its
facing, the lever is asked for its other state, and her pull is entered.
The lever plays, ends on its KILL command -- which leaves the status
INACTIVE, as upstream's does; the port's KILL used to clear the active
flag without unlinking, a cycle waiting for a re-trigger -- and the
SWITCH-type trigger under her feet asks `Switch_use` whether it has been
pulled. It has, once: the status goes back, a timed switch keeps its
countdown and springs back when `Switch_update` sees it expire, and the
rest of the trigger fires -- the door, and the lever itself, whose mask
`Switch_update` fills every tick regardless. Item 10 of LEVEL1 opens door
9 that way; the runner's trigger scene pulls it and waits for her hands.
The water switch is the same from a tread, after `moveTo` has swum her
onto it; it is coded and not yet exercised.

**The pickup.** She turns it to face her, steps a hundred units short of
it and bends; twenty-three frames before the bend ends, it is hers:
`Inventory_add` counts it -- keys and puzzle pieces by type, the two
medipacks and the lead bar, for the day there is a screen to use them
from -- `olPickups` counts it, and it goes INVISIBLE. Under water the
same from a tread, swum onto, fourteen frames before a shorter bend.

**The keyhole and the puzzle hole.** Action at the hole with the matching
key or piece in her pockets spends it, walks her 362 (or 327) onto the
spot and plays the use; the hole is ACTIVE from then, which is what the
KEY-type trigger in its sector asks `Key_use` for once her hands are
free, and the door beside it opens. A puzzle hole becomes its DONE type
when the piece is in, as upstream's does, though the piece is not drawn
in it: the model an entity is drawn with is the packer's and fixed. TR
opens the inventory here and lets you choose, and on FX3 so does this
(see "Inventory"); GSU-2 offers the key that fits. The runner's LEVEL2 key
scene picks up the silver key in room 14, turns it in the keyhole in room
5 (on FX3 through the ring's USE page), and watches door 9 open.

**The block.** A block is a floor. Upstream's `Block::updateFloor` rewrites
the sector it stands on so the room's floor is its top -- from the start,
in its constructor -- and rewrites it back when it moves; TR's floor data
is level data upstream can modify because it copied it into RAM. Here
the host's sector table lives in game pak RAM bank 1 as well, so the same
rewrite works on the same bytes, and `Room_reset` forgets what
`room.asm` remembered: the first time this port changes the world. In
LEVEL2's room 0 the corridor is exactly a block tall, so the raised floor
meets the ceiling and the sector becomes a wall, which is also upstream's
rule. Action at it, square on, aligns her to it (`Lara_alignWall`) and
takes hold; Up or Down from there, if the sector beyond is clear at the
block's base and its top (`Block_checkObstacles`, two `Room_query`s),
starts the push or the pull, hers and the block's together. The block's
push animation is 155 frames with the OFFSET and the KILL at its end; it
comes to rest INACTIVE, leaves the active list and is a floor again. The
runner's block scene reads the result the way she would, by standing
where it was and on top of where it is and asking the probe: the host
table can be read from the runner but not trusted while the GSU owns the
bus. Not here: the navigation boxes' blocked bit (upstream's TODO too)
and the heavy trigger a landed block fires.

**Trapdoors** are a class of their own now, off the door list: a door's
update and a bridge's floor. Shut, `getTrapDoorFloor` is a floor over the
two sectors it covers, applied by `Lara_itemFloorAt` from the bridge list
it joined (its box widened by the sector it opens toward); open, she
falls through. Coded against upstream's `getItemFloorCeiling`; LEVEL2 has
three and none is in a runner scene yet.

**Crystals.** Upstream's `Crystal` constructor hides every one ("disable
crystals for now": the PlayStation's save points, which the PC levels
carry); `level.c` does too, on both variants (LEVEL1 has one in room 20,
LEVEL2 two). They were drawn until 2026-10-06.

## Creatures

A wolf in room 14 wakes up, stands, turns to face you and comes at you.

`enemy.c` shares mood selection and a cached ground/flying-zone flood.
`wolf.c` and `wolf_state.asm` preserve Wolf's own state decisions, including
pending states across animation links, front/behind checks from a 375-unit
head offset, the 345/768/1536/3069 distance boundaries and random sleep/howl
decisions. The previous state's turn cap applies before the current state
updates it to 2 or 5 degrees. Close reverse turns halve that cap. Speeds come
from the animation; LEVEL1's run reaches 100 units/tick, not the old constant
13. ATTACK and BITE latch 50 and 100 damage once per attack, using rotated
animation-union bounds until joint spheres are ported.

`enemy_move.c` handles wolf/bear ground location: slopes, one-click steps,
four-click wolf drops or one-click bear drops, room portals, zone boundaries
and active-creature avoidance. `bear.c`, `bear_state.asm` and
`bear_update.asm` preserve the bear's original graph, target-info ordering,
turn caps, contact damage and two death routes. Both species read animation
speed; the bear no longer moves at a fixed 11 units/tick. On GSU-2,
shared navigation still lacks the original aggression-driven waypoints; in
particular, wolves can circle a stationary front-facing target. The original
state graph is present, but this is not yet full Enemy behavioral parity.
FX3 has the per-creature mood and corridor machinery described at the top.

`bat.c` owns the bat's state graph and motion. AWAKE requests FLY; contact
requests ATTACK, which deals two points each 30 Hz tick while the bounds
overlap. Losing contact requests FLY and resets the mood. Horizontal motion
comes from the original animation speed and acceleration, not the old
16-unit table entry. Vertical pursuit is capped at 16 units/tick and targets
Lara's animation-box top; turns are capped at 20 degrees. The shared flood
still replaces upstream's per-creature waypoint/aggression machinery.

Flying room queries resolve portals and prevent crossing solid columns,
floors or ceilings. A killed bat loses horizontal motion, falls under normal
gravity through CIRCLING, then reaches DEATH at the floor. The native fixtures
exercise the actual graph from awake, flying and attacking poses; the LEVEL1
scene also shoots an activated bat and checks its final grounded corpse.
Contact boxes are an approximation to upstream's joint-sphere hit mask;
bat pitch, blood, full aggression and enemy/enemy collision remain absent.

**Waking is two steps.** LEVEL1's wolves start asleep — state 8 — and there is
no transition from sleeping straight to walking. Asking for one leaves the
creature dreaming while it stares at you. SLEEP requests STOP and remembers
GROWL as its pending state; the animation graph grants each in order.

### Finding the way

A straight line is right until a wall is in it. TR's navigation graph is a set
of boxes — axis-aligned rectangles in sector units — and an
overlap list saying which box opens onto which. LEVEL1 has 619 and 3528, which
is 12 KB, and like everything else the packer stages it is read where it
lies.

Each sector explicitly names its box, or NO_BOX. Room identity matters:
layered rooms overlap in X/Z, and box rectangles also have shared boundaries
and holes. `navBoxAt` now uses the room's sector record and its packed original
ID instead of choosing the first rectangle containing an absolute sector.
No per-item X/Z cache survives to carry an answer onto a different floor.

Upstream keeps an incremental target-outward search per creature. Here one
complete breadth-first flood runs outward from Lara's box per zone kind and
records, for every reached box, the neighbour leading back toward her. The
search still lacks per-creature step/drop filtering and original corridor
waypoints, so a connected box is not yet a guarantee of a traversable route.

The flood stays inside her zone, which is what a creature could reach in the
first place. TR's zones say which boxes a given kind of creature may use, and
they are the difference between a wolf that waits on its side of the water and
one that walks into it — which is also why there are two floods, since a wolf
and a bat do not share a graph. In room 14 the flood connects 53 of the
level's 619 boxes, and it is rebuilt only when she changes box.

Both searches follow the overlap graph outward from their target. The shared
flood resets its source IDs along with its entries when the level is reset.

### Mood

What a creature has decided to do about her, which is `getMoodNormal` in
`enemy.h` with its own numbers:

```
asleep or stalking   hurt and unlucky, or cut off   -> escape
                     in her zone and within 3072    -> attack
                     in her zone                    -> stalk
attacking            hurt and unlucky               -> escape
                     cut off from her               -> asleep
escaping             in her zone and lucky          -> stalk
```

The rolls come out of upstream's own PRNG, ported with its seed and constants
intact — the *sequence* matters as much as the distribution, because a
creature that flinches has to do it for the same reason on every run or
nothing about one can be tested. `olMathCheck` pins its first three values
against constants, which is also a check on the 32-bit multiply underneath it.

A stalker hangs back two sectors and watches; an escaper heads away, which is
the one direction that needs no path.

### Sectors, and a truncation that had been there all along

**TR's world does not fit in sixteen bits.** LEVEL1 spans 98 sectors — about
100,000 units — and every position on this side is an `int`. That costs
nothing to anything working room-relative, which is the whole renderer and
most of the host, because a difference between two wrapped values is right
even when the values are not. It costs everything to a table indexed by
absolute position, and the navigation boxes are exactly that.

So navigation is done entirely in **sector units**: absolute, small, and the
only coordinate here that cannot wrap. A sector is worked out from the room
rather than the position — the packer ships each room's sector origin, the
offset within a room is small, and neither term can wrap, so the sum is exact.

The first working wolf walked *away* from Lara, because the comparison was
between an absolute box centre and a wrapped position and which was bigger
depended on where in the level it happened. That is the shape of every bug
this truncation will ever cause, and it had been latent since the packer was
written.

And it stops when it arrives, and bites. `Wolf::DIST_BITE` is 345 units and
`Wolf::DAMAGE_BITE` is 100 of Lara's 1000 — TR's own figures, paced at two
bites a second until the creature has a real attack animation to hang the
damage on: contact damage on all thirty ticks would end her in a third of a
second of standing still. At zero she is dead, which here means the
state machine requests `STATE_DEATH`; the camera/game-over behavior beyond
that state and its animation chain is not ported.

One range and one damage figure for everything, until each creature has a
class with its own.

### And she shoots back

Tap R to draw or holster the pistols; hold B/Action to fire once they are
ready. The shared GBA/32X controller's FREE, BUSY, DRAW, HOLSTER and READY
states now run on the SNES. PICK and DRAW use the cartridge's original
inclusive frame ranges, and each hand/thigh mesh swaps at the corresponding
animation boundary. Swimming requests holstering; busy hands cannot draw,
and death removes the arm overrides. Reaching and grabbing require free
hands; hanging, ledge climbs and climbing out of water mark them busy until
the release or animation command frees them.

Each arm raises, aims, fires, reloads and lowers independently. Aim angles
approach their targets by ten degrees per tick, and the GSU applies them
relative to Lara's object basis at the shoulder, preserving the body's
translated shoulder position. Each discharge is one damage per pistol;
both aimed pistols therefore deal two. The original nine FIRE reload frames
return the arm to AIM, and the following tick can fire again: ten ticks
between shots from a held pistol. The earlier shared cooldown fired every
nine ticks and applied only one bullet for both arms.

Torso and head now follow the pistol arms, using the shared engine's
`updateLook` order before movement and weapon animation. With both arms
aiming, each joint takes a quarter of their summed pitch/yaw; otherwise it
takes half. Losing the target eases the angles back toward centre, and death
clears them. The GSU applies each joint's extra Y-then-X rotation after its
original animation pose. The head inherits the torso, while aimed arms keep
their object-relative bases at the rotated shoulder positions.

The core uses upstream's 8192-unit range, 650-unit muzzle height and
60-degree acquisition limits. A wolf has six health, a bear twenty and a
bat one. Holding Action preserves the selected target; releasing it permits
fresh acquisition while the drawn guns continue to aim without firing.

Range is spherical and includes height. Only living, active creatures with
ACTIVE status can be targeted. `Weapon_getAimPoint` ports the shared GBA/32X
engine's `weaponGetAimPoint`: the upper third of the animation box, with its
X/Z centre rotated by the target's yaw. Acquisition uses that point for pitch,
yaw and visibility, while range still uses the item origins. Acquisition
subtracts both Lara's body angles and her torso angles. `weaponFindTargets`'s
rule selects the smallest absolute torso-relative yaw, with equal angles
favouring the last candidate in the active list. `Room_trace`, shared with the camera, rejects
intervening room walls, floors and ceilings. Endpoint checks reject solid sectors and
overlapping rooms without a connecting portal. Absolute room sectors reject
far-away coordinate aliases while allowing nearby targets across the signed
X/Z seam; the trace's sector loops now cross that seam correctly too.

This visibility path does not yet test static-mesh boxes. Shut doors it sees
on FX3, where they are walls in the sectors it walks (see "A door you cannot
walk through").

`Weapon_updateTargets` now also ports `weaponTrackTargets`'s retention rules.
Every READY tick updates the target's body-relative angles and visibility,
including reload ticks and ticks when Action is released. Within the body's
60-degree cone both arms acquire aim. Outside it, each arm can retain existing
aim up to 80 degrees of pitch; the right yaw arc is -60 to +170 degrees, and the
left is -170 to +60. Losing sight or leaving those limits clears arm aim
while retaining the target's identity. The guns stop firing until aim returns
inside the acquisition cone. Releasing Action permits another target; dead,
hidden or inactive targets release the lock on their next update.
A turned torso can select a new target outside that body cone; the same
independent arm retention rules then decide whether either gun can aim at it.

As upstream does, tracking allows a target to move beyond the initial
8192-unit acquisition range. The short-coordinate trace still limits the
target's sector difference to 31 on each horizontal axis, and its vertical
aim delta to -32767..32767; farther targets retain identity without aim.
These limits keep distant coordinate aliases from becoming visible targets.

**Death follows each species' graph.** The wolf sets animation 20, 21 or 22
of its model on its first dead update, using the original logic PRNG; merely
requesting its death state cannot reach those animations from every pose.
The bear instead follows its original STOP or HOWL transition into one of
two deaths, retaining the one-shot upright falling hit. Neither restarts a
death animation every tick. The bat runs its original falling/death state graph and resolves
its flying height, including the initially above-ceiling hanging origin.

Manual look and camera-item tracking, finite ammunition for the other guns,
muzzle flash, random spread, sphere hit tests, combat camera behavior and audio remain.
Creature aim points now follow `getBoundingBox(false)`'s selected source
frame. Their rotated positions drive acquisition, tracking and visibility,
so a moving pose moves the point both pistols aim at. Camera and collision
boxes still use animation unions; other model types retain the union aim
fallback. Joint/sphere hit testing remains unported.

The synthetic two-target fixture's paired discharge costs 56,908 cycles.
The 423-byte native aim-point helper retains its C reference, checked by
fixed cases and 256 randomized union boxes, plus every creature animation
tick at two yaw angles using the packed frame points. Unrotated frame-point
lookups average 2,093 CPU cycles in that fixture. The pistol arm controller uses 509 native
bytes, with the shared control flow retained in `weapon_anim.c` as a
reference. The look update and reset add 108 native bytes. Its two arm
records take 24 bytes, and the torso/head angles take eight more.

The host copies the weapon state, both arms and look angles to `$70:F710-$F731`
only at the stopped-GSU frame handoff. `gsu/weapon.gsu` selects the original
weapon poses and the four swappable meshes, with pointer tables generated
from `WEAPON.json`. Lara's model radius bounds arbitrary joint rotations
and both mesh sets. `make weaponcheck` checks 555 joint/frame combinations,
mesh addresses/radii, invalid poses, shoulder bases and 120 torso/head
matrices. The image oracle also checks that each arm and each look joint
alters the rendered picture independently, including holstered look decay.
A separate whole-machine check holds Action against two stationary living
targets for ten seconds, checking clock drift, zero dropped ticks and
continued rendering. GSU-2 records 300 completed ticks and FX3 299, with
60 shots and 60 damage in each; the sample can end with a tick in flight.
Enemy updates are frozen in that timing scene; moving creatures
and more simultaneous targets need their own measurements.

Sustained aiming exposed unnecessary slope arithmetic in `traceX`/`traceZ`.
When both endpoints occupy the same sector on that axis, the original loop
visits no boundary; an early return now preserves its room assignment without
the divides and multiplies. Other-axis and endpoint checks still run. The
land interaction scan also skips READY hands, preserving pending pickup and
puzzle animations and underwater use. Together these keep the combat scene
within the logic budget while leaving time for the GSU to finish frames.

### A bridge you can stand on

An entity can raise the floor as well as block it. TR's bridges come in flat
and two tilts, and `getBridgeFloor` reads the slope off the entity's angle and
the steepness off which tilt it is — so the four bridges over LEVEL1's cistern
are something to walk across rather than scenery to fall through.

It does **not** go into `ol_floor`. That global is written by the NMI whenever
the GSU stages a fresh probe, which can land in the middle of an update, so
anything computed into it is liable to be overwritten between the computing
and the using — which is exactly what happened, and read as the bridge simply
not working. `ol_floorEff` is the floor after the entities standing on it have
had their say, computed from one read of `ol_floor` and used by everything
downstream.

### A door you cannot walk through

Entities have a size now. TR puts a bounding box at the head of **every
frame**, and this port had been walking straight past it to get at the pose
behind it; one box per model, from its first frame, is what a shut door needs.

TR blocks a doorway by rewriting the floor data behind the door — level data
the GSU reads and the host cannot touch, since the channel only runs the other
way. So a door is a box here, which is what TR uses for most items anyway
(`ItemObj::collide`), and it is solid only while its state says shut. That
pair is the whole point:

```
[openlara] shut door: camX -16383 -> -16205     pushed to the edge of the box
[openlara] switch:    door flags $0 -> $5f      the lever in room 9
[openlara] open door:  camX -16383 -> -16383     the same spot, walkable
```

The box is rotated by quadrant, which is upstream's `boxRotateYQ` and not an
approximation of it -- it was one, briefly, and the two bugs that cost are
worth the telling because both are the shape a port fails in:

- **The quadrant convention is not the obvious one.** Quadrant 2 is the
  *identity*, and 0 negates both axes. A hand-rolled "odd quadrants swap X and
  Z" is right about the swap and wrong about the signs, which put LEVEL1's
  door in room 9 on the wrong side of its own hinge -- invisible on a
  symmetric box and obvious on a door.
- **`boxPushOut(a, b)` takes the obstacle first and the mover second**, and
  returns the vector to apply to the *second*. Upstream's only caller reads
  `boxPushOut(meshBox, objBox)` and moves the object, which is the one thing
  that says which way round it goes. Backwards, it returns a vector that
  pushes you further in.

Both were caught by `make fixedcheck`, which now covers the box helpers
against the identities they have to satisfy rather than against a second copy
of themselves: four quarter turns is where you started, a box pushed out of
another no longer overlaps it, and a translated box still contains a
translated point. They are pure functions, which makes them the easiest thing
in the port to get right and the easiest to get subtly wrong by writing
something that resembles them.

**FX3 does what TR does** (2026-10-06, `src/door.c`). Its host keeps the
sector table in WRAM (`$7F:0000`, copied at bind) and the level's own in
ROM, so upstream's `Door::action` ports as it is: while a door is shut, the
sector one along its facing in its own room and, through that sector's
portal, the sector at the door in the room beyond are walls -- `{-127, -127,
link 0}`, no floor, no ceiling, no rooms, no floor data -- and open, the
level's four bytes come back from ROM. Every door shuts at the load, as
upstream's constructor does; after that the update opens the sectors once a
door stands open while active and shuts them once it stands shut while not,
writing only when that changes, then forgets what `room.asm` and the camera
cached about sectors. Upstream's `Door::collide` is empty, and so is FX3's:
the box above is GSU-2's alone.

Everything that asks a sector sees the door now, where only her collision
saw the box: a creature's step (`Enemy_moveGround`'s floor test), the
camera's clip, and the guns' `Room_trace`. LEVEL1's bear used to walk
through door 28 into room 16 and be shot dead through it; now it stops at
the door, and her guns never lock on:

```
[openlara] door: bear 30 reached x 18367 (door line 18432) in 900 ticks, 0 in room 16, ...; her guns held it 0 of 650 ticks
           (the 10-06 ROM: x 24572, 790 ticks in room 16, health 0; her guns held it 121 ticks)
[openlara] shut door: camX -16383 -> -16283     her radius from the wall, on her side
[openlara] doors: 8 on LEVEL1, their sectors as their states say
```

Not here, nor upstream: the nav boxes (upstream's "TODO modify level.boxes"
-- a creature still plans through a shut door and is stopped at it) and
flipped rooms. Runner: "OpenLara FX3: doors", and the boot test checks every
door's sectors against `doorSectors`, a TypeScript port of the choice.

What Lara still is not: a body of her own. The box is the *entity's*; she is a
point with a radius, and nothing bumps into her. The reach test for a switch
is one radius and an angle window rather than TR's per-object limit box.
Keyholes want an item in an inventory; pickups in TR1 are sprites, which this
renderer has none of.

### Holding still

Entity state advancing on one side and standing still on the other is fatal to
the whole two-oracle arrangement: the interpreter has no host, so it cannot
reproduce an accumulator. The fix is `ol_animFreeze`, which holds every entity
on one frame — the game's own facility, which the inventory screen will want
anyway, rather than the harness reaching in to pin state.

Reaching in was tried first and does not work. The host fills its array in one
step and stages it in another, and a write from outside lands on whichever
side of that seam the field boundary happens to fall — so the two oracles
agreed at animation frames 0, 7 and 11 and disagreed by about 120 bytes at 3
and 4, deterministically, with no pattern until the seam was the suspect.

The lesson generalises past the test: **anything observed from outside this
bus has to be a function of state at a defined moment, not an accumulation
across a seam.** Save states will meet it next.

A record whose bank is `$FF` still moves the matrix. That matters: the node is
a joint whether or not the mesh hanging off it survived packing, and skipping
its transform folds every limb below it into its parent.

LEVEL1 comes to 42 models, 209 nodes and 53 entities placed -- plus the two
sprite models the packer synthesises and the seven pickups they carry, which
brings it to 44 and 60.

### Lara has no arms in her own model

Four of her fifteen meshes are empty in the file. That is TR being TR: the
game fills them at run time from whichever weapon model she is holding, which
is `ItemObj::meshSwap`, and the base model carries the holes. The packer fills
an empty joint on `ITEM_LARA` from the corresponding
`ITEM_LARA_PISTOLS` mesh. The renderer now also applies the runtime thigh
and hand swaps, so the guns move between her hips and hands during the
original drawing and holstering animations.

### Three bugs, all worth the telling

**A mesh lives in its own bank, and drawing one leaves `R_ROMB` pointing at
it.** Reading the *next* placement then read it from whichever bank the last
mesh happened to be in — which was the one holding the room table, so the
renderer found a vertex count of 38 followed by one of 0.

**A vertex count of zero is 65536 vertices**, because the loop decrements and
tests. That is a megabyte written from `TV_BASE` over the ordering table, the
frame buffer and everything else, and it is what turned a wrong bank into a
blank screen rather than a wrong picture. Bad data should draw nothing, not
take the machine with it; `TransformMesh` checks for it now.

The third is the composition trick above, found by replacing it with the
matrix stack and seeing room 20's mesh pixels go from 6265 to 41888 -- and
then checking, because more pixels is not the same as right pixels. The matrix
after each `MatrixRotateY` matches `Ry` computed in floating point to a unit,
and the translation matches an addition, which is what says the new path is
correct rather than merely different.

The first was found by A/B: patch `instCount` to zero in a copy of the ROM
image and render the same view twice. That trick is worth keeping — it
isolates "did the mesh pass draw this" from "is the camera inside a wall"
without rebuilding anything.

And one hypothesis that was wrong: the four disconnected quads the first
working render produced looked like inverted winding. Reversing it drew
**zero** pixels, which settled it — TR meshes wind the same way rooms do, and
four quads really was all that faced the camera from the base of a door frame.

## HUD

Upstream's `drawHUD` (`src/fixed/draw.h`): at the top right, the air bar
while she is at or under the surface, and below it -- or alone -- the health
bar, while she holds her guns ready or for forty ticks after a hit or a
medipack (a hundred at the level start). `renderBar` draws it 102 x 7: a
border of TR palette entries 19 and 17, five rows of the bar's colour
(8/11/8/6/24 health, 32/41/32/19/21 air), 27 where it is empty.

Drawing that into the GSU's frame would cost the renderer and the frame
would carry it into its CRC; it is sprites instead. Thirteen 8x8 sprites a
bar, 26 in all; the rest are parked below the picture at boot. The packer
(`HUD.BIN`) writes 23 characters -- the first cell with 0-7 columns filled,
a middle cell with 0-8, the last with 0-5 and the right border -- which
both bars share under two sprite palettes in the exact TR colours, because
sprites have CG-RAM 128-255 to themselves. On FX3 the 8bpp frame can use
those too, but only as `$80 | shade << 4`, colour 0 of a sprite palette,
which a sprite never shows.

The C (`olHudUpdate` in `src/game.c`) keeps upstream's timer by watching
her health rather than by setting it at every damage site, of which there
are several in native code; any change to her health is a `hit()` or a
medipack upstream, and both set forty. It writes 104 bytes of OAM image
and a dirty flag; the NMI moves them by DMA on channel 1 before the frame
chunk. Mode 7's sprite characters are its letterbox, so that build has no
HUD. While the inventory is open (FX3) the bars go and her health timer
stands still, as upstream's does; the one bar the ring shows, her health
under the title while a medipack is chosen, is the same sprites at
`INV_BAR_X`/`INV_BAR_Y`.

### The fade (FX3)

Upstream starts every level at `gBrightness = -128` and adds 16 a tick of
real time, `palSet` passing it to `palBright`: every channel of every colour
less `gBrightness >> 2`, clamped at zero. That subtraction is the PPU's own
colour math -- the main screen less the fixed colour, per channel, clamped --
so FX3 enables it for BG1 at boot (CGWSEL/CGADSUB) and the NMI writes the
amount to COLDATA every field: the load tick starts a count of 15 fields
(`olFadeFields`), and m left is `4 * ceil(m / 2)` -- 28 and 24 down to 4, two
fields each, then nothing, as upstream's first rendered frame after the load
is 28. Counting fields rather than ticks is upstream's own clock (its
`frames` are elapsed real ticks), and keeps the scenes that pin `ol_ticks`
from holding the picture dark. Not exact: OBJ palettes 0-3 never take colour
math, so the HUD's bars, which upstream darkens with the rest, come in at
full brightness.

## Inventory (FX3)

Upstream's `src/fixed/inventory.h`, whole in logic: Select opens the ring
while she is alive (the game does not move while it is open), Left/Right
spin it, Up/Down change page, Action shows an item and, for a medipack or
a gun, closes the ring and uses it (`Lara::useItem`); Action at a
keyhole or puzzle hole with anything in her pockets opens it on the USE
page with the hole's own item chosen, and what is picked there comes back
as `useSlot`, which the hole spends the tick after -- or answers with
SND_NO. `src/inventory.c` is the state machine (bank `$86`), with
upstream's slots, states, pages, counts and starting medipacks; GSU-2 has
no room and keeps its eleven counters and a hole that takes the key that
fits.

**The ring.** `src/inventory_draw.c` is upstream's `drawPage`/`drawSlot`
up to the draw: the view from `anglesFromVector`, then per item its turn
onto the ring, the push out, the quarter turn and, for the chosen one, the
lift, the pull and its spin -- every matrix call in upstream's 1.14, on the
PPU's multiplier -- and hands the GSU the finished model-view matrix per
item in its own 1.15, with the model (the packer's `INV.BIN`: per slot the
model stream, its bank, its first animation's rate and length) and its
pose. `gsu/mesh.gsu` `FrameBody` (at the end, so no tuned routine moved)
draws `InvRender` instead of the rooms while `GS_INV` has items: each
model under its matrix, on the whole frame. The projection gets a table of
its own at `$70:3200`, `RECIP_TAB` for the game and doubled for the ring,
and the host scales the ring's matrices across by 23/32: upstream's
framing, F 204.8 on 240 columns, on these 216. The chosen item comes as
close as upstream pulls it, 256 from the view for the passport: the near
plane is 64 (see "The near plane"), and `TM_Near`'s table is the doubled
one in a ring frame. (Until 2026-10-06 it stopped 240 short, past the old
plane at 512.)

**The text.** Upstream's `drawText` puts TR's font -- the GLYPHS sprite
sequence -- into the frame, and so does this, through the GSU: the VRAM
has 23 characters to spare, not a font's worth. The packer (`buildFont`)
trims each of the 110 glyphs to what it covers and stores its pixels in
`LEVEL.BIN`, a byte each, the colour in the low nibble, fitted to 15 by
weighted k-means in TR's RGB; `FONT.BIN` (CPU ROM) has the palette, the
level's upstream LevelID, and per glyph its address, size and corner.
`src/inventory_text.c` is `drawText`, `getTextWidth` and `charRemap` with
upstream's `char_width`/`char_map`, and `Inventory::draw`'s strings -- the
page title, the page arrows, the item's name (the level's key and puzzle
names included) and its count in the small digits -- laid out into a list
of glyph blocks for `GS_TEXT`. `TextRender` plots them with GETC under a
frozen high nibble: the frame is 8bpp, so pixels `$A1-$AF` reach sprite
palette 2, where the boot put the font's colours, and a zero byte is
transparent.

**The background.** Upstream's `copyBackground` greys the game's last
frame (`palGrayRemap`: eight TR greys by luminance) and draws the ring over
it. The ring's frames skip the clear, so on the first one the game's
picture is still in the plot buffer: `BgCapture` reads it back with RPIX,
turns each pixel into one of eight greys (`$B0-$B7`, CG-RAM 176-183) by the
packer's 256-byte remap, and keeps it at `$70:3400` -- the GSU-2 frame
buffers' space, which FX3 does not use -- and every frame after plots it
back before the models. TypeSNES models FX3's PLOT and RPIX on the planar
buffer, hardware on its chunky one; either way RPIX answers the last
frame's pixel.

**The passport and the death page.** Select while she is dead opens the
ring on the death page, straight to the passport shown at Load Game, as
upstream's `Lara::update` does (main.asm's `CheckRestart` stays GSU-2's).
Its pages turn with Left/Right on the options and death pages, the arrows
beside its name. Load Game lists the levels, and this cartridge has one:
Action on it starts the level again through the boot (`olRestartLevel`,
which sets DBR back to 0 for `RestartLevel`); Exit to Title boots the
title page (below). With a save in the battery RAM,
Load Game also offers Current Position, chosen first, and Save Game
writes one -- see "Saved games" below; the warning and OK are left for a
save that does not read back. `drawOptions` is upstream's: the box `renderFill`s its
interior at shade 25 and `renderBorder`s it in TR 14/10, the chosen
option framed in 15. The GSU's `RectRender` draws the rectangles --
colour 0 reads each pixel back and darkens it through a second packer
remap (a shaded texel five rows down, a grey a quarter as bright) --
before `TextRender` puts the labels on them.

**The passport's pages** (2026-10-06): upstream's `updateVisMask` picks
which of the passport's nodes are drawn as its pages turn -- covers and
spine always, the pages of the spread it is on and the one turning -- and
`drawSlot` draws every ring item under that `itemVisMask`, reset to all
whenever an item is chosen or put back. `inventory.c` resets it and asks
`Settings_visMask` (`settings.c`, beside the options pages: a 30-byte table
of upstream's ranges -- the C_MOTION oracle's `$BB` had no room for the if
chain) at every step of the passport's animation, and `inventory_draw.c`
puts it in each ring record's last word; on the
GSU, `InvRender` loads it into `MD_VIS` and `WeaponNodeFx3` -- already
called once a node, before the mesh is chosen -- shifts it right a node at a
time and, for a clear bit, sets the node's bank to `INST_NONE`: the joint's
transform stays, its mesh is not drawn, as upstream's `drawNodes`.
`FrameBody` sets it to all for every other frame; `ASR` keeps `$FFFF`
whole past sixteen nodes. Before it, the Save Game spread showed a printed
page from another spread over its right half. The game's frames pay 0.03%.
The ring's sounds are in ("Sound"); the options pages' settings are below
("Settings").

**A line under every ring, and a compiler bug.** From the day the ring
went in, every ring frame drew a one-pixel line across the whole frame at
row 60, and a stray glyph at the top on the pages without a title. Both
were 816-tcc: `if (state != SHOW || !count) return;` in `drawOptions`
returned only when the count was zero, so the options' border was drawn,
with no options, at height 0; `if (!s || !*s) return;` in `drawText` went
on to read a null title. See "Toolchain traps"; `tools/tcclint.py` now
refuses the shape at compile time.

## Settings (FX3)

Upstream's `gSettings` and its three options pages (`src/settings.c`,
bank `$88`). gameInit sets upstream's defaults -- rumble on, A/B as they
are, SFX and music on, gamma 0, the frame counter shown, no vsync -- and
`osLoadSettings` reads the battery RAM over them: `$70:3110`, "OLST", a
version, the seven fields; `osSaveSettings` writes them back whenever a
page changes one. The pages are upstream's `initOptions`, `onOption` and
`drawOptions`: Graphics (Gamma, a bar of 16 steps; Show FPS; VSync), Sound
(SFX, Music), Controls (Rumble, Swap A B). A bar is `renderBar`'s
BAR_OPTION -- TR 43 44 43 42 41 (FONT.BIN's tail, CG-RAM 188-191), the
border TR 19/17 (the HUD's palette), the empty part the one darkening the
GSU has (shade 25 for upstream's 27) -- and a switch On or Off, labels
right of the middle, as upstream lays them out.

What each does here. **SFX**: `Sound_play` sends nothing (`sound.asm`), as
the GBA's `sndPlaySample` returns. **Swap A B**: `game.c` trades the two
face buttons before anything reads them. **Gamma**: upstream's
`palGamma` over the whole palette, which here is CG-RAM: the load tick
keeps a copy of what the boot uploaded (the level's colours, HUD, font,
greys, borders, the bar, a title boot's picture), `olGammaApply`
(`save_fx3.asm`, on the CPU's multiplier: 29K cycles where 816-tcc took
two fields and held a tick back long enough to lose a key's release)
runs it through the gamma at the render sync, and the NMI sends it up 64
bytes a field, never in a field the HUD goes up in. **Show FPS**:
`drawFPS`, the counter at (2, 16) in TR's font -- upstream draws it over
every frame, the inventory's included -- through the GSU's `TextRender`
in game frames too (`FrameBody` calls it when `GS_TEXT` has glyphs;
`ItemsExchange` hands it `olInvText` when the counter changes, through
`save_fx3.asm`'s `olGameText`, since the host's WRAM image had 8 bytes
left). Music, Rumble and VSync are kept and change nothing: no music, no
rumble, and the frame rate is the GSU's.

**Checks.** `OpenLara FX3: settings`: with no settings in the battery,
upstream's defaults and the counter's digits in `GS_TEXT` over the game;
Graphics' gamma to 4 (CG-RAM 0-127 against `palGamma` of PALETTE.BIN,
colour for colour) and Show FPS off (the counter goes); her steps reach
the APU, then SFX off and they do not; Swap A B on, and B jumps her 630;
the battery's twelve bytes, and every setting and the gamma back after a
power cycle. Every other scene's cartridge is given settings with the
counter off (`OWN_SETTINGS` off), so no picture carries it.

## The title page (FX3)

Upstream's `startLevel(LVL_TR1_TITLE)`: the title screen, the ring over it
on `INV_PAGE_TITLE` -- the passport, controls, detail, sound and Lara's
Home -- tilted at the opening, Select and Jump not closing it, nothing else
moving; the passport's one page is Load Game, the cartridge's levels and,
with a save, Current Position, and its Action starts that level
(`nextLevel`). The GBA's background is `TITLE.SCR`, a 240 x 160 picture in
the title level's palette.

**The picture.** The packer (`writeTitle`) reads the DOS release's own
`TITLEH.PCX` (640 x 480, 205 colours), keeps rows 16-447 -- the black over
the logo and the copyright line go, which leaves the picture within a sixth
of the proportions 216 x 144 pixels of 8:7 show (the GBA's squeeze is an
eighth) -- and box-filters it to the FX3 frame. Every pixel is a CG-RAM
index as the title boot leaves CG-RAM: the level's 128 shade colours, the
HUD's, the font's, the greys and the borders where they are (1-191), and 64
colours of the picture's own, fitted by k-means to what those miss, at
192-255, which nothing else uses. 70% of LEVEL1's pixels land on colours
already there; rms error 4.8 of 255. `TITLE.BIN` is the 64 colours and the
31,104 indices; `mkbanks.py` puts it with the first level's small CPU-only
tables, inside one bank (`OL_TITLE_*` in `hostbank.inc`).

**The boot.** A power-on without a boot request is `BOOT_TITLE`
(`SelectLevel`): the first level, whose colours the picture was fitted to,
loads as ever, and the load tick opens the title page instead of starting
her (`game.c`; the ring pauses the game and draws no room).
`UploadPalette` puts the 64 colours at CG-RAM 192, and `CopyTitle` moves
the picture into `BG_BUF` with one MVN before `InitGSU`, both buses the
CPU's: four fields of the boot. `ItemsExchange` holds `BG_STATE` at 1 for a
title boot, so the GSU's `IR_Grey` plots the picture under every ring
frame and `BgCapture` never takes a game frame over it -- no GSU change.
The passport's level is a `BOOT_START` request and a restart; Exit to
Title (the options' or death page's passport) and the end page's Action
are a `BOOT_TITLE` request. A cartridge packed without `TITLEH.PCX` has an
empty `TITLE.BIN` and boots straight into its first level, as before.

**Not upstream's, or not yet.** Lara's Home: LEVEL1 and LEVEL2 do not carry
its model (upstream's title level does) and the cartridge has no gym, so
the ring leaves it out when the level has no model (`Inventory_hasModel`);
were it there, its Action would do nothing. The title music (no music).
The GBA's "slow cartridge" line. The ring's first-frame tilt is upstream's
(`pitch = 1024` at the open, its target never set, so it snaps level when
the opening ends).

**Checks.** `OpenLara FX3: the title page` powers on without a request: the
title page over the first level, its ring, CG-RAM 192-255 and `BG_BUF`
byte for byte against `TITLE.BIN`, the level held still for a second,
Select and Jump ignored, the frame on screen the picture wherever the ring
and text are not (95.7%, decoded from VRAM; `title.png` beside the ROM);
then the passport's first level starts and moves, and Exit to Title comes
back to the picture. Every other scene's cartridge is given a plain-start
boot request (`COLD_BOOT` off), so they boot their level as before.

## The four guns (FX3)

Upstream's `inventory.init()` hands her the pistols, the magnums, the uzis
and the shotgun, and `Lara::init` puts the shotgun on her back; so does this.
Choosing a gun on the ring is `changeWeapon`: the goal. `weaponUpdateState`
(`src/weapon_kit.c`, bank `$88`) holsters the gun in her hands, takes the
goal once her hands are FREE and draws it; choosing the gun already
holstered draws it (`WEAPON_NONE` for a tick, as upstream). R draws and
holsters whichever she has, as before.

**Parameters.** `weaponParams` whole: damage 1/2/1/4, spread 8/8/8/20
degrees, height 650 (the shotgun 500), reload 9/9/3/26 ticks, the flash's
timer 3/3/2 and distance down the hand 155/155/180, and the acquisition and
arm limits (the shotgun's 55/60/65 and -80..80). `weapon.c`'s targeting reads
them through the gun in her hands; ammunition is upstream's -1 for every gun.

**The pistol class.** The magnums and uzis share the pistols' four arm
animations (`animType`) and `weapon_anim.asm`'s controller, with the gun's
reload. **The shotgun** has its own three (AIM, DRAW, FIRE: WeaponArm
animations 4-6, one on both arms) and upstream's `weaponDrawShotgun`,
`weaponHolsterShotgun` and `weaponUpdateShotgun` in C: the hands swap at draw
frame 10, the reload sounds at fire frame 10, a blast every 26 ticks, the
torso following the right arm's aim and the head still (`updateLook`, in
`weapon_anim.asm`). `weaponFire` fires six pellets, each with its own two
`rand_logic` draws. Upstream tests each pellet against the target's joint
spheres, which this port has not got for creatures; a pellet hits when its
offset from the aim, carried out to the target's distance, passes within the
average of half its current-frame box's height and half its narrower width
(the pistols keep the port's hit-in-aim). Close up all six hit; at 8000 one
or two.

**Her meshes.** What `meshSwapPistols`, `meshSwapShotgun` and
`weaponUpdateTargets`' head leave her wearing is the host's to work out
(`Weapon_publish`, at the render sync, only when something changed) and the
GSU's only to read: `GS_KIT` (`$70:F648`) is a mesh index per node into a
table of 22 the packer lays out (`writeWeaponKit`, WEAPON.json's `kit`) --
her thighs with or without the holsters of the last pistol-class gun put
away, her hands with the gun of the moment or the shotgun's, her torso with
or without the shotgun on its back, her head or the uzis' gritted one while
Action is held. `WeaponNodeFx3` and `WeaponLookFx3` (end of `mesh.gsu`)
replace DrawModel's two weapon hooks byte for byte, so nothing tuned moved;
the latter adds `drawLaraNodes`' "additional torso animation for shotgun",
the shotgun frame's torso angles less `DEF_TORSO_ANGLE`, which the packer
tabulates and the host adds while the shotgun's animation runs. The opening
frame draws her with the shotgun on her back: 83 more pixels and 28 K cycles
for its mesh, 1.5 K for the hooks.

**Its sounds** share one region of the APU image, refilled when she changes
gun -- see Sound.


Upstream's `gameSave`/`gameLoad` (`src/fixed/game.h`) over the GBA's
`osSaveGame`/`osLoadGame`/`osCheckSave`, in `src/save.c` (bank `$87`) with
two block moves and a checksum in `src/save_fx3.asm`.

**Where.** The FX3 specification gives a board with a battery its own
cartridge type, `$18` (section 2), and a Super FX battery keeps the game
pak RAM -- the GSU's working memory, `$70-$71` -- whole: TypeSNES sizes it
from `$FFBD` (128 KB) for that reason. FX3 draws in bank `$71`, so the GSU-2
frame buffers' `$70:1000-$31FF` was nobody's; the save is there, 8,704
bytes. The runner watches every write to that window from power-on -- the
GSU's plots and reads-back, the ring's background, the host -- and only
the save lands in it.

**What.** Upstream saves each item's place, angles, animation, flags,
timer and states (`ItemObj::save`), Lara's extras (speeds, health, the
weapon and its arms, the water state, the camera's last fixed camera) and
the creatures' (health, speeds, mood), the inventory's counts and both
random seeds. Here an item is its whole 32-byte record -- a superset of
`ItemSave` that holds health, speeds and mood too -- and the per-item
arrays beside it (the creatures' pending state and turn cap); `extraL` is
the globals `lara.c`, `weapon.c`, `weapon_anim.c` and `contact_fx3.c` keep
for it; then the counts, the pickups, the darts in flight, the level's end
and secrets, the fixed cameras' ONCE bits (upstream keeps those in level
data and so loses them), the flip state and the seeds. One list of fields
in `save.c` serves the save, the load and the length, so their layouts
cannot part. LEVEL1: 2,898 bytes for its 80 entities.

Rooms are not saved, as upstream does not save them: the one thing in
them a game changes is a block's floor, and `Block::load` gives back the
sector the boot raised under it and raises the one it was saved on; so
does `Save_apply`. Navigation is not saved either: upstream makes a
creature's `extra` again in `activate()`, and a creature here takes a Nav
context again on its next update.

**The header**, sixteen bytes: `OLSV`, the version, the ROM's own checksum
(another build may number its entities differently), the entity count, the
payload's length and Fletcher sum, and a pending-load word outside the sum.
`Save_write` zeroes the magic first and writes it last, so a save the
power cuts short is no save rather than half of one, then reads the whole
thing back: `olSaveValid` is the answer, and the passport shows the
warning if it is not.

**Load.** Current Position marks the save pending and restarts the
cartridge, as Load Game of the level does (`startLevel`); the boot's
`Save_check` sees the mark and clears it, and the load tick, once
`olLevelInit` has made the level as any boot leaves it, puts the save over
it before the camera snaps to her. The runner saves a wolf mid-chase with
her pistols out, switches the cartridge off and on with only its battery
RAM carried over, loads, and finds every saved byte back at the next
`gameUpdate`; the wolf takes a Nav context and hunts on. LEVEL2 saves a
pushed block and finds both sectors right after the load.

**A compiler trap found on the way:** 816-tcc's `gen_opi` takes any
`VT_CONST` right operand as an immediate without looking at `VT_LVAL`, so
`x != *(uint16 *)0x80FFDE` compares `x` with `$FFDE`. A load of the same
expression is right; `save.c` reads the checksum through a function.
Nothing else in the tree puts a constant-address dereference on the right
of an operator.

## Levels on one cartridge (FX3)

The GBA's alpha is LEVEL1 and LEVEL2: its passport's Load Game lists the
two, Caves' END trigger loads City of Vilcabamba, and Vilcabamba's leads to
the alpha's end page. `make BUILD=build_fx3_multi FX3=1 LEVELS="LEVEL1
LEVEL2"` builds that cartridge.

**Where everything goes.** The GSU sees 48 banks of ROM. Below GSU `$45`:
the boot, the host image, both GSU programs (`$41:0000` and `$41:8000`) and
the C, `$84-$89` -- the FastROM banks GSU-2 numbers `$BC-$BF` moved down,
which left the window above unbroken. From `$45` each level's geometry and
texture pages in turn: LEVEL1 `$45-$58`, LEVEL2 `$59-$6E`, 84 of the 86
wla banks there. The CPU-only last MiB, `$F0-$FF`, holds each level's host
tables (bank-aligned), its small tables in their tail, its sphere data and
its APU image: to `$3F0000` of `$400000` (the title screen, with LEVEL1's
small tables, took one bank). Both levels fit with a GSU bank and a CPU
bank to spare; a third would not.

**One CPU program.** Where a level's data lies is its entry of the level
table (`tools/mkbanks.py`, `leveltable.inc`); main.asm's `SelectLevel`
copies it into `olLevel` before anything reads the level -- the host
tables' bind (`olHostBind` copies the 256-byte directory to
`olHostDirectory` and binds every table at the level's bank), the palette,
HUD, font palette, sound table and APU image uploads, the GSU's entry point.
What the program still fixes at assembly must agree across the levels, and
the generator refuses a cartridge where it does not: the pistols' animation
lengths, and the one level whose blades read sphere data (`trap_fx3.asm`
binds its bank).

**One GSU program per level.** The same source with each level's
`texbank.inc` -- its bank bases and pistol models are immediates in tuned
code -- the second assembled to lie at `$8000` (`GSU_ORIGIN`): 496 bytes
differ from the first, every one an address's high byte by `$80`, so its
cache alignment is the first's. Its entry is `$8100`; the R15 idle probe
still reads a high byte of zero only after STOP.

**Which level a boot plays.** A restart asks for one in the battery RAM,
which the reset leaves alone: `$70:3100`, "OLBR", the level, the kind of
boot (`Save_boot`). `SelectLevel` reads it and spends it; a power-on finds
none and opens the title page (above). A level's END trigger (`olLevelEnd`) asks
for the next level and restarts; past the last it opens upstream's end page
(`INV_PAGE_END`: the grey background and STR_ALPHA_END's lines, `GS_INV`'s
count `$8000` -- an inventory frame with no ring), whose Action goes to the
title page. The passport lists the cartridge's levels by their
LevelIDs; a save remembers its level (header version 2), and Current
Position from LEVEL1 boots LEVEL2 and loads it.

**Checks.** `make ... LEVELS="LEVEL1 LEVEL2" check` runs LEVEL1's suite on
the cartridge and `BOOT_LEVEL=1` LEVEL2's: every cartridge a scene loads is
given the boot request, and the fixtures that call in without booting copy
the level's entry as `SelectLevel` would. `OpenLara FX3: the cartridge's
levels` walks onto Caves' END trigger, saves in Vilcabamba, ends it on the
end page, goes to the title, and its Current Position loads Vilcabamba
exactly. Two literal
`$E00014` reads of the host directory (one in `contact_fx3.asm`, one in a
test) were found by the second level; the directory is read through
`olHostDirectory` now. So was a stale-RAM bug a one-level cartridge hid:
the reset leaves game pak RAM alone, and the first GSU frame runs before
any render sync, so it drew whatever the last boot had published -- after a
Load Game from the ring, the old ring's models through the new level's
banks, which trampled `GS_LEVEL_READY`. `InitGSU` now publishes gameInit's
hidden entity records and zeroes the ring, shadow, text and rectangle
counts before the first frame.

## Shadows (FX3)

Upstream's `drawShadow(item, 160)`, for her and every creature (Enemy sets
`ITEM_FLAG_SHADOW`): an octagon on the floor under the entity, sized from
its current frame box (160/1024 of it, 5/32 here) and turned with its yaw,
whose pixels `rasterizeS` darkens through lightmap row 26.
`Shadow_publish` (inventory_text.c, beside the font's tables) builds the
records at every render sync -- hers, from `ol_floorEff`, then the seven
nearest creatures still to be seen, from their `roomFloor`: slot, the
floor's depth below it, the box's middle and size -- and ItemsExchange
hands them to `GS_SHADOW`. None under the ring, or while `ol_animFreeze`
holds the drawn poses away from the host's (the renderers' byte-for-byte
comparison; Lara's hit animation stands down the same way). The GSU's
`ItemTilt`, now called for every entity DrawRoomItems draws, finds its
slot's record and `ShadowRender` draws it before the model: the octagon
through the entity's matrix -- after the yaw, before her pitch and lean --
then every covered pixel read back (RPIX), darkened by the font's second
remap (renderFill's), and plotted, inside the room's clip.

The renderer takes its reciprocal at 256-unit steps of depth, and its
pitch is a shear, so floor points a hand's width apart project to the same
line: projected its way, the octagon is a line or a step. `ShadowProject`
projects the corners at their exact depth instead (RECIP_TAB interpolated
at z / 128) and the octagon is moved to where the renderer's own
projection puts its centre -- under her feet, which it projected the same
way. Each edge is walked into per-line spans (`SH_XL`/`SH_XR`, in FX3's
unused shade-map RAM) with one FMULT a line. Nearer than 512 the model under it
is `TM_Near`'s, exact, so there the corners and the centre go through the
near projection (`ShadowCentre`, `ShadowProjectFx`, `SP_Near`, swapped in for
`ProjectPoint` and `ShadowProject` call for call) and the anchor offset is
zero; a corner short of 64 still means no shadow that frame, the octagon
being filled on the screen unclipped.

## Effects (FX3)

Upstream's `fxBlood`, `fxRicochet`, `fxSmoke`, `fxSplash` and `fxBubbles`
take an item from `ItemObj`'s free list and let `SpriteEffect` or `Bubble`
play its sprite sequence until it ends and puts it back. Here every entity
exists from the load, so the packer appends 24 hidden ones after everything
else (`appendFxPool`, beside the darts' pool); `src/fx.c` takes the first not
in use, retypes it and puts it on the active list, and the native active-list
walk hands the pool's slots straight to `Fx_update`. Upstream's pool is
`MAX_ITEMS` less the level; 24 covers a bat bleeding every tick of its attack
and both pistols at once, and when they are all in flight a new one is
dropped, as upstream's `add` returns NULL.

**Drawing.** All five kinds are one model (`buildFxModel`): a single node
marked billboard and `NODE_SEQUENCE`, whose mesh is every kind's sprite frames
as consecutive 54-byte one-quad meshes, and an animation per kind whose frame
f carries, in its Y angle byte, which quad of the run it is. On FX3 DrawModel's
billboard node goes through `BillboardNode`, which moves the mesh address on
by that byte times 54. The host plays the kind's animation with upstream's
frameIndex (rate 1), and the renderer clamps a frame past the end to frame 0,
as it does for any model, so nothing reads stray bytes.

**Where they come from**, as upstream: a creature's bite (`Enemy::bite`, the
wolf's attack and bite, the bear's bite, the bat's attack, the raptor's three)
bleeds at its jaw before the damage -- a call from the native state machines
(`wolf_state.asm`, `bear_state.asm`, `raptor_state.asm`) and from `bat.c`; a
shot that hits bleeds at the target's aim point (`Enemy::hit`'s `fxBlood`; the
joint spheres are still not tested, so an aimed shot always hits); a shot at
nothing is traced 16384 along the arm's aim, with `weaponFire`'s spread -- its
two `rand_logic` draws, which the creatures share -- and ricochets where the
line stops; a dart that strikes her bleeds, one that strikes the wall
ricochets, the emitter smokes as it fires; the swinging blade's cut bleeds at
her, offset by its four `rand_logic` draws (one call's arguments upstream,
whose order C++ leaves open: taken here left to right); she splashes ten
times where she falls into water; and `FX_LARA_BUBBLES` (her underwater
animations, `item_anim.asm`) breathes up to two bubbles from her head, which
wobble up to the surface and burst there.

**Joints.** The bite's position and the bubbles' are `getJoint(joint,
offset)`: upstream's int32 matrix chain to that node on the current keyframe.
The packer runs that chain for every game tick of the animations that can ask
(the creatures', and her eight underwater ones) at angle zero, keeps the
point in sixteenths (`FX.BIN`, per level in CPU ROM, the level descriptor's
new `fxAt/fxBank/fxSize`; the descriptor is 64 bytes now), and `Fx_joint`
turns it by the item's angles -- Z, X, then Y, as `matrixRotateYXZ` does to a
point; only she has a pitch and a lean -- on this side's 1024-entry sine. The
packer measures the result against the chain at sixty-four yaws (and her
pitches and leans): within five units; the runner checks 266 points against
upstream's own `getJoint` (`spherecheck --joint`, extracted verbatim): within
two.

**The muzzle flash** is upstream's `drawFlash`: after either hand's mesh while
that arm's flash runs, in the hand's frame 155 down it and 55 out, turned
`-90` degrees about X and by the shot's random roll about Z, the
`MUZZLE_FLASH` model's mesh. On FX3 DrawModel's `DrawMesh` call is
`DrawMeshFlash`, the same eight bytes. Upstream counts each arm's
`flash.timer` (3) down at the top of every `updateWeapon`; here the shot keeps
the tick it ends on instead, and `ItemsExchange` works the timer out once a
frame (`FlashTimer`, 1..3 while it runs) and retires a spent one, so no tick
pays for it.

**The update** of the four sprite kinds is native, beside the active-list
walk (`FX_Sprite`, render_sync.asm): its tick, frame and removal, and the
move along its angle on the PPU's 16x8 signed multiplier, which FX3 has free
(no Mode 7): `sin * hSpeed >> 14` from the product's top bytes, exact; the
runner holds it to fx.c's C tick by tick. A bubble asks a room every tick
(`Room_query`) and stays C.

**What it costs.** The pool's 24 records are not scanned or copied while no
effect is in flight and none has just gone (`olFxLive`, `olFxPending`: the
render sync and `ItemsExchange`'s DMA stop at `olSyncEnd`); with that, the
native active-list walk (`olUpdateItems`, `render_sync.asm`) and the flash's
tick-free timer, both three-creature gates are back to their numbers before
the slice (299/0, 296/0) with the creatures biting her. `fx.c` itself is 7 KB
of ROM in `$86`; the frame, +0.07% for the hand hook.

**Not here:** `SND_RICOCHET` is fired, but the APU image has no room for it
without dropping two of the level's animation sounds (the plan keeps 32 of
53; `SND_BUBBLE` is in it already, by her animations); explosions, sparkles
and the lava's particles (none in these levels); joint spheres for the hit
test; effects on GSU-2.

## Sound

The GBA reads its samples out of ROM and mixes them in software; the 32X
has no effects at all. Here the S-DSP mixes eight voices of BRR in hardware
and the sound CPU has 64 KB of RAM that no one else can touch, so the shape
is the co-processor one again: the level's samples go up once, at boot,
and playing one is four port writes.

**What goes up.** TR1 ships each level with 50-70 seconds of 22 kHz 8-bit
PCM -- LEVEL1 is 84 samples, 1.2 MB -- and the sound CPU has room for about
ten seconds of it at 11 kHz. So the packer (`packer/sound_SNES.h`) chooses:
only the ids this level can fire, which are the SOUND commands of its own
animations plus the ten the game calls by number; the ones the game names
first -- footsteps, landing, the pistols, the flinches, the hit, the scream
-- then the rest by how many animations name them. Each is decimated by an
integer factor (3 for the named set, 4 for the rest), cut at a second with
a 64-sample fade so it ends at zero (Caution 8), and BRR-encoded against the
DSP's own decoder, filter and shift chosen per block by least error. When
that does not fit, variants beyond two go, then the least important sound
takes a coarser rate, and only a sound already at 3675 Hz is dropped.
LEVEL1 keeps 32 of its 52 firable ids in 62 KB; `OL_DUMP_SOUNDS=1` prints
every decision and the encoder's SNR (20-44 dB). The result is `SOUND.BIN`,
the APU image from `$0400`: the source directory, a pitch word per source,
a silent block every unused entry points at, and the streams.

**The driver** (`src/spc.asm`, 483 bytes at `$0200`) spins on port 0. A
command is a source number, a volume, a pitch multiplier and a byte whose
top six bits the host advances every time so two commands never read alike;
the low two are the mode. UNIQUE retunes a voice already playing the source,
REPLAY restarts it, NEW takes another, STOP keys off -- upstream's
`sndPlaySample` modes, with the channel search done where the channels are.
A voice is free when its source byte says so or ENDX says its sample ran
out. The pitch is the sample's own word times the multiplier over 128, on
the SPC700's 8x8 multiplier. It echoes the command byte as its
acknowledgement, and the host waits for that echo -- briefly, bounded --
before the next.

**The boot** (`UploadSound` in `main.asm`) copies `SNDTAB.BIN` -- the
host's soundMap and infos, 640 bytes -- into WRAM, then talks the IPL ROM's
own protocol (Book I 3-3): the driver, the image in one or two halves, the
start address. An emulator without a sound CPU never answers `$BBAA`; that
wait is bounded and `ol_sndReady` stays clear, so the game runs silently
rather than hanging. The IPL moves about 48 KB/s, so a 62 KB image costs
1.3 s of black screen at power-on; the boot test reports the first tick's
field and allows three seconds.

**The host** (`src/sound.asm`, 724 bytes; `src/sound.c` is the C reference
the `C_MOTION` oracle build runs) is upstream's `soundPlay`: the id's info,
the chance roll, the distance from the camera target -- three squares on
the CPU's multiplier, `phd_sqrt`, `<< 2` off the volume -- the random gain,
`>> 9` to 0..63 and up one, the random pitch, the random variant, the mode.
The compiled unit came out at 2,344 bytes against 765 free in the GSU-2's
resident image, which is why it is assembly. `rand_draw` is its own PRNG,
as upstream's is, so that nothing here disturbs the logic PRNG the creature
oracles depend on; it is a 16-bit xorshift rather than the 32-bit LCG,
since its sequence decides nothing anyone can see.

**Where it is called** is where upstream calls it: the SOUND animation
command (natively on FX3, in `item.c` on GSU-2), `Weapon_fire` for the shot
and the wolf's or bear's flinch, `s_STATE_FALL` for the scream at 154 units
a tick and the landing and the water for its stop, the pistol arms for
draw and holster, and `Hole_collide` for a key that does not fit.

**The guns** (FX3) hold one region between them: she holds one gun at a
time, so the packer sets aside room for the largest gun's sounds -- the
shotgun's blast and reload at /4 -- and each gun takes the finest rate that
fits there (the pistols' and uzis' shots stay at /3). Upstream's five ids
keep their own infos; only the samples under the region's two sources
change. `GUNSND.BIN` (CPU ROM, `olLevel`'s `gun*`) has per gun the two
directory entries, two pitch words and the samples, and `olSndGun`
(`main.asm`) sends the driver back to the IPL with a STOP for source `$FF`
and uploads the three blocks when the gun in her hands changes (or a load
gives her another): about 90 ms, once. The image starts with the pistols'.
It cost LEVEL2 one animation sound (id 34); LEVEL1 keeps all it had.

**The ring** (FX3, 2026-10-06) has upstream's sounds -- SND_INV_SPIN,
CONTROLS, SHOW, HIDE, COMPASS, WEAPON and PAGE (HOME where the level has its
model) -- without taking a byte of the level's image: nothing in the level
sounds while the ring is open, so they borrow the room of its last samples,
just under the gun region. The packer encodes them at /4 (the longest down
to /6 until the set fits 8 KB: LEVEL1's 8,163 bytes), with their own
sources and infos past the guns', and carries them in `GUNSND.BIN` after the
guns' records. `olSndRing` (`src/sound_ring.asm`, `olSndGun`'s protocol)
uploads them as `Inventory_open` opens the ring, before its SND_INV_SHOW;
when it has closed, the tick that ends it puts SOUND.BIN's own bytes back
-- and the ring's directory entries back to the silent block -- before
what was chosen is used. Each way is about ten fields of IPL handshakes
inside that one tick, about one rendered frame at FX3's rates; the title
page pays it at the boot. The gun in her hands keeps its region.
SND_HEALTH, the medipack's chime, is the game's (`Lara::useItem`) and joins
the level's image with the named set, at /4: LEVEL1 lost ids 13 and 34 to
it (both had been at /6), LEVEL2 id 53, and its id 32 went to /6. With
`GUNSND.BIN` 14 KB larger, the two-level cartridge's CPU-only data now
ends exactly at the end of its 4 MiB.

**What is not there.** Music: TR1's is CD audio, the GBA ships it as
compressed samples, and here it would be a tracker rendition or nothing.
Room `soundSources`, the underwater loop. Pan:
upstream is mono, and so is this, though the DSP would give L/R for the
price of an `atan2` the camera already computes. The samples that did not
fit: the boot could load fewer at a higher rate, or more at a lower one --
the knobs are at the top of `sound_SNES.h`.

**Verifying.** TypeSNES runs the SPC700 and the S-DSP, so `make check`
compares the sound CPU's RAM with `spc.bin` and `SOUND.BIN` byte for byte
after boot, checks the DSP as the driver leaves it, then runs her for five
seconds and watches every voice every field: LARA_FEET's sources key on at
the sample's pitch within upstream's random band, both variants are heard,
and the pitch varies -- which is the assertion that caught the flags byte
arriving as zero.

## What the GSU tells the host

Less than it used to, and the story of why is the port growing up.

The bus problem stands and still shapes the target: **the level's geometry
and textures are in game pak ROM, and while the GSU holds that bus the 65816
reads dummy data.** What changed is where game logic's questions go. They
used to go to the co-processor — it probed one position per rendered frame,
staged the floor, the ceiling, the sector corner and the four neighbour
floors at `$70:F300`, and every piece of gameplay was rationed to that one
answer per frame, one frame old.

Now the answers come out of the staged blob itself. The packer ships the
whole sector table with the level — six bytes a sector: floor, ceiling,
`roomBelow`, `roomAbove`, a horizontal portal and the trigger, plus a packed
stream of 5-bit slants — and `Room_query` in `src/room.c` resolves an
arbitrary point at any moment of any tick. It exhausts FloorData's horizontal
portals the way `Room::getRoom` does, follows `roomBelow`/`roomAbove` by Y,
then walks to the bottom-most floor and the top-most ceiling, both
point-exact on their slants; the trigger comes from the *descended* sector,
which is exactly what the one-probe channel could not deliver and the
falling-floor test failed on. Doorways, stacked rooms and slopes all resolve
before Lara's state machine looks.

`src/game.c`'s `olQueryLara` refreshes the collision neighbourhood with it:
the sector under her and the four around it — those four cached until she
leaves the sector, since a neighbour cannot change until the centre does —
and the same query relinks her room and resolves the camera's.

The entity list is the other piece of level data the host holds outright
rather than asks about, staged by the GSU into `$71:0000` as it always was,
and it is why `src/game.c` can name what is standing in a room. See
"Entities on the host".

The walking is unchanged in spirit: `src/game.c` follows the floor with a
step limit and clamps out of walls, X and Z separately, which is what makes a
wall slide rather than stop her dead. A neighbour is blocked if it has no
floor at all, or if its floor is more than a step above this one — TR's Y
grows downward, so higher is smaller.

The four neighbour queries are each a **full room resolution**, not a lookup
in the current room's grid, and that is the point: a query that crosses a
doorway answers with the *next room's* floor rather than with a wall. Walking
between rooms needs none of TR's FloorData on the cartridge, because the
packer resolved all of it into the table.

What the GSU still stages at `$F300` is for the renderer, not for the game:
`StageProbes` resolves the probe point the host handed it and the result
seeds the render's starting room, falling back to the last room drawn when
the point has left the level entirely. Nothing on the C side collects it any
more. Two resolvers now stand on the two sides of the bus, answering from the
same packed facts, which makes each the other's oracle.

`RoomFind` — the GSU-side half — scans the room table and takes each room's
sector grid *less its border ring* — TR rooms share a ring of wall sectors
with their neighbours, and the full extent would let two rooms both claim a
doorway. X and Z decide which room; Y only breaks ties between rooms stacked
on the same ground, because a probe a step above or below a neighbour's floor
still has to resolve to that neighbour, or a doorway with a step in it reads
as a wall. `SectorAt` then indexes the grid: TR stores a floor as a signed
byte of 256 units, so the height in world units is that byte in the *high*
half of a word — which is what `GETBH` produces, sign and all, for nothing.

What the host-side table costs is bus discipline rather than staleness: the
blob lives in game pak RAM, so a tick takes RAN for its duration and a
running GSU parks at its next RAM access — see "The clock".

## Frame pipeline

```
host: SCBR, PBR, R3 = buffer, SCMR = RON|RAN, write R15   -> GSU starts
GSU:  clear -> render -> RPIX -> STOP                     -> IRQ
host: queue that buffer for upload, flip, restart the GSU
NMI:  RAN=0, DMA one $1400 chunk to VRAM, RAN=1           x4
      last chunk -> DMA the shade map over the tile map's
                    high bytes, then point BG1 at it
```

The shade map goes up in the same field as the flip and not before: the tile
map is shared by both frame buffers, so an early upload would re-shade the
frame still on screen.

The GSU keeps rendering the *other* buffer while the finished one uploads.
Taking `RAN` away parks the GSU at its next game pak RAM access and giving it
back releases it, so the only rendering time lost is V-Blank itself.

**FX3** (2026-10-06). Its frame goes chunky -> the firmware's C2P into one
planar buffer at `$71:0000` -> VRAM, in series: the next job is published
only once the upload is over. Measured in TypeSNES (a scene timing
`StartGSU`, `QueueUpload` and the last chunk) before this round: standing at
the start, render and C2P 3 fields, upload 6, publication 1 -- 10 fields,
6.0 fps; running, 5.2. The upload was four columns a V-Blank. Now:

- **The letterboxes are V-Blank.** The view is lines 41-184 of 224; the
  lines above and below show only the blank tile, so under forced blank
  they take VRAM writes. A V-timer IRQ at line 185 (`IrqFx3`, on the IRQ
  vector the GSU-2's STOP interrupt has) blanks and DMAs fourteen columns
  (16 KB, about 100 lines at 165 bytes a line, DRAM refresh included) on
  through V-Blank into the top letterbox; the NMI that arrives in the middle
  leaves the upload to it (`olIrqUpload`). A frame queued after line 185
  starts with the NMI's own chunk (seven columns, V-Blank and the top
  letterbox). The display comes back when the DMA is over, not on a timer
  -- by line 31 at worst, ten short of the view -- so a late chunk would
  black out the top of the view for a field, never lose a VRAM write. The
  flip sends the map's 18 view rows, the only ones the two buffers' maps
  differ in (the boot sends it whole). Two to three fields a frame, was
  seven. No HDMA: S-CPU v1 can hang when a general DMA ends early in an
  H-Blank while HDMA runs (the manual's Problem 1).
- **The STOP is seen when it happens.** The idle loop `WAI`ed for the next
  NMI; `IdleFx3` polls R15 a few hundred times a field while a job runs
  (not in a tight loop: TypeSNES puts the register window on the cartridge
  bus, and a spinning read starved the GSU).

- **Logic first.** A frame now costs the CPU ~0.9 field (the render sync,
  the exchange, 32 KB of DMA) and arrives twice as often, which the
  three-creature gates (logic at ~86% of the CPU) paid for in ticks: 298/1,
  294/2. Publication now waits for an empty tick queue (was: under three),
  the sixteen-field deadline still bounding it; the gates are back at
  299/0 and 296/0 and draw 4-8 fps where they drew 3-6.
- **Each tick its own keys.** The NMI queues every tick period's keys with
  the tick (`TickKeysPush`, eight slots), and the tick takes its own
  (`TickKeysPop`). Ticks that ran late used to share one accumulated state,
  so a release between two quick presses could vanish behind a long render
  sync -- the ring's is about five fields (816-tcc C: its matrices and its
  text), and with frames twice as frequent the settings scene's four Rights
  came out as two. `rot`/`trans` of the ring's matrices are native now
  (`inventory_fx3.asm`, exact against the C on 600 cases); the rest of
  that render sync is still C.

Standing 6.0 -> 12.0 fps, running 5.2 -> 9.2. Prompt publication also
exposed a latent bug: a frame published before the level's load tick has no
entities, and `ItemsExchange`'s piecewise DMA wrote `DAS2 = 0` -- 64 KB --
over all of game pak RAM bank `$70`, the battery save included. A zero count
now skips it (both variants).

Not done, and the next step: the render overlapping the upload. On the FX3
the render's `PLOT` goes to the firmware's own buffer and only the C2P to
`$71`, so the next job could render during the upload with its C2P held
back (built and measured: 15 fps standing, 13 running). TypeSNES plots
straight into `$71` and makes the C2P a no-op, so there the next frame
overwrote the one going up; it waits for the emulator to model the
firmware's buffer (TODO.md §1).

## The clock

Game logic runs at TR's own thirty ticks a second, and it does not care how
long the GSU takes to draw. The NMI queues one tick every second field; the
main loop drains the queue ahead of everything else it does:

```
NMI    every 2nd field: logicPending++, unless four are already waiting
main   while ticks are pending: TakeRamBus, gameUpdate in C, ReleaseRamBus
       when the GSU finishes a buffer: queue upload, gameRenderSync,
       ItemsExchange, flip, restart
```

The pieces that make it safe are the ones worth naming:

- **Taking RAN parks a running GSU only at its next game pak RAM access and
  leaves it the ROM bus**, so a tick can read the staged level mid-render and
  the renderer keeps fetching and plotting until its next writeback. The cost
  is real but bounded by the tick's own length.
- **`cpuOwnsRam` records the intended bus owner across NMI.** The NMI takes
  RAN for its own DMA chunk whatever else is happening; without the shadow it
  would hand RAM back to the GSU in the middle of a C update, after which
  every level-blob read returns dummy data. The owner is published before
  SCMR is touched, on take and on release both, so an NMI landing between the
  two stores restores the state the interrupted code needs.
- **The backlog is capped at four ticks, and FX3 publishes a frame only
  below three.** With three ticks queued, a finished frame waits (bounded
  at sixteen fields) while the ticks run: its publication -- the render
  sync, the exchange, the GSU's start -- used to land on the tick that then
  dropped at four. Upstream's order the other way round: every elapsed tick
  runs, and fewer frames are drawn. A debugger pause, or a scene whose
  ticks cost more than two fields each, must not become seconds of catch-up
  with the renderer starved of RAM. Under sustained overrun the cap means
  game time dilates rather than the machine spiralling — honest, and
  currently silent; a dropped-tick figure next to `fps` is still owed.
- **Renderer state is serialized only when a frame completes, and only for
  what can have changed.** Writing all sixty entities' twelve bytes on every
  tick spent most of the 65816's budget producing bytes no in-flight frame
  could observe, so `gameRenderSync` ran once per flip instead -- and the
  evening the renderer went from one frame a second to two, the walking
  test's clock slipped to 27 ticks: the sync was about 130 K cycles a
  frame, two fields of the host, and it had been hiding behind the frame
  rate. Now it writes Lara, the active list and whatever `ItemObj_touch`
  marked since the last frame -- leaving the list, a status change, the
  level load -- because an entity off the list has not moved. The copy into
  game pak RAM is an `MVN` of `itemsCount` records rather than a byte loop
  over all 128 slots. Every renderer speed-up from here on will ask this
  question of the host again; the profile's answer is `gameRenderSync` and
  `olItemToRenderer` beside `ItemsExchange`.
- **The pad is OR-latched per field into an accumulator** and consumed per
  tick, so a tap shorter than a tick is not lost and a tick that runs late
  sees everything pressed since the one before.

The consequences run through every section above. Upstream's constants come
across at their own per-tick values — `OL_FRAME_TICKS` is 1, and `OL_SPEED`/
`OL_ACCEL` survive only to mark the conversion sites for a future PAL
scheduler or a slow-motion mode. Animation time is counted in ticks, with the
pose the renderer is shown at `frameIndex / frameRate`. Trigger timers are
seconds times thirty, as upstream has them. A jump is thirty-three ticks
because nothing is left to make it anything else.

### Its gauges

Three counters, published by the NMI at every 60-field rollover into C
globals the runner finds by name: `ol_tps`, ticks completed in the last
second; `ol_ticksDropped`, every tick the backlog cap threw away; and
`ol_tickFields`, the field boundaries those ticks ran across. `make check`
prints them and asserts thirty a second and nothing dropped, in the opening
scene and in the walking scene with the wolves awake.

The third is an alarm rather than a measurement: a tick starts at the top of
a field, right after the NMI that queued it, so it only counts ticks that
outlast a field. The measurement is the profiler:

```
OL_PROFILE=walk make check TESTS=profile     # she runs, wolves awake
OL_PROFILE=idle make check TESTS=profile     # she stands in the entrance
make BUILD=build_fx3 FX3=1 check TESTS=profile # both live-creature 30 Hz gates
```

It wraps TypeSNES's CPU `step()`, histograms elapsed bus master clocks by
program counter and maps each to the nearest link symbol. This charges
address-dependent memory access, DMA and raster refresh stalls; opcode
cycles alone are not elapsed field time. WRAM and FX3 FastROM banks are both
included. The report contains flat self costs, selected inclusive JSL spans,
stack low-water mark and per-60-field tick/drop/presentation samples, saved
to `profile_<scene>.json` beside the cartridge. FX3 LEVEL1 runs the three-live-
creature regression by default, with no frozen animation, pinned waypoints,
forced mood or zero movement speed. It requires three Nav owners, pursuit
damage, bounded clock drift, no dropped ticks and continued presentation.
The older opcode-cycle figures below are historical, not the new bus-clock
measurements or an FX3 hardware timing claim.

The first thing it found was that walking past the first wolves ran the
game at six ticks a second. `boxAt` -- which navigation box a sector is
in -- was a scan of all 619 boxes through a long pointer, twice per awake
creature per tick: 160 ms. It was first cached per entity until its sector
changed; the exact-sector navigation slice later removed the scan/cache
entirely, since neither could distinguish stacked rooms. The second was that three loops over all sixty entities ran
every tick to find eight doors, twelve bridges and fourteen creatures,
which are now lists built at load. The third was the compiler: a table
stride that is not a power of two is a call to `tcc__mul`, and the blob
accessors in `level.h` spell their strides as shift-adds now. Where it
stands, after the collision handlers came across:

| scene | cycles per tick | fields per tick | CPU idle |
|---|---|---|---|
| standing in the entrance | 46,109 | 0.77 | 51% |
| running, wolves awake | 62,147 | 1.04 | 33% |

The fourth thing it found was the room query itself: four points a probe,
seven or eight calls a tick, 2,400 cycles each -- 32-bit pointer arithmetic
around a dozen byte reads, and a library multiply for the sector index. It
is 65816 assembly now, `src/room.asm`, the same algorithm statement for
statement with the reads as `lda.l $71xxxx,x`, the multiply on the CPU's
own `$4202`, and both caches in the direct page; the family that cost
20,300 cycles a tick costs about 3,500, and the profile lists its pieces by
label -- `RQ_Out`, `RQ_Mul` -- because the profiler attributes by nearest
link symbol. Before it, the walking scene stood at 70,494 cycles a tick and
the idle one at 52,960.

A tick every two fields keeps thirty a second while a tick is under two
fields long -- in principle. In practice the host wants a third of a field
for itself, the NMI's DMA chunks and pad reads and the frame handoffs, so
the clock starts slipping when ticks average a field and a half: the camera
clipper found that out at 81 K. The rest of the field is what the renderer
gets the RAM bus for. What remains is collision: `collideRoom` and the four
probes under it are a fifth of the walking tick.

## The rasterizers

`gsu/raster.gsu` walks a doubly linked vertex list down both edges at once and
fills the span between them, the same structure as `rasterizeF` on GBA. Three
things had to change, all forced by the chip:

1. **16-bit registers.** The ARM code's 16.16 edge accumulator does not fit.
   Screen X is 0–255, so 8.8 fits exactly, and eight fractional bits over a
   160-line polygon stays inside half a pixel.
2. **No divide.** OpenLara already dodges one with `divTable[i] = 65536 / i`;
   here that table lives in ROM and is read through the R14 ROM-buffer path,
   then multiplied with `LMULT`. The 32-bit product is shifted down a byte with
   `LOB`/`SWAP`/`MERGE`, which is the cheapest route on this instruction set.
   Large divisors are shifted into the table's 1–255 range and the reciprocal
   is scaled back afterward. `h == 2` is special-cased because `divTable[2]`
   is `$8000` and signed `LMULT` would read it as negative.
3. **Not enough registers.** Two edges plus loop state does not fit in sixteen,
   so the locals live in the `LMS`/`SMS` short-address window.

Flat spans are filled with `PLOT`, which advances R1 by itself — one
instruction per pixel plus loop overhead, and the two 8-pixel plot caches write
completed character blocks back to RAM in the background.

A fourth interpolant, `g`, rides alongside u and v but leaves the loop by a
different door: it is written per character, after the plot loop, by
`SpanShade`. See "Shading".

Room quads do not always split a long shared edge at identical vertices. With
16.16 edge steps, independently truncated slopes can then differ by one column.
Coverage is deliberately conservative on the right edge while texture
gradients retain the geometric width; the one-pixel overlap is hidden by the
back-to-front draw order and prevents the cleared frame from showing through.

### Textured spans, and what they cost

`RasterFT` adds affine U and V interpolation along both edges and across each
span. The trick that makes it affordable is the texture page layout: one
256x256 page of 8-bit texels fills a 64 KB GSU ROM bank, so the
texel address is exactly `(v << 8) | u` — and `MERGE`, which builds a word from
the high bytes of R7 and R8, computes that from two 8.8 accumulators in a
single instruction. The inner loop is eight instructions:

```
SFT_Loop:
    to r14
    merge           ; texel address, and the write starts the ROM read
    with r8
    add r10         ; U += dU
    getc            ; COLR <- texel
    plot            ; and R1 advances by itself
    with r7
    add r9          ; V += dV
    loop            ; LOOP consumes prefix state
    nop
```

The two accumulator updates happen after `MERGE`, so they affect the next
pixel. The `WITH` for V cannot be carried through `LOOP`: `LOOP` is an
instruction with a delay slot, not a prefix-preserving conditional branch.

The caveat is small polygons. Per-span setup is about 390 instructions: one
reciprocal lookup and two `MulRecip` calls to get du/dx and dv/dx. On a
124-pixel span that is a quarter of the cost; on an 8-pixel span it is
fifty-seven instructions per pixel. Sorting that out — a cheaper path for short
spans, or hoisting the gradients to the polygon — is the first optimisation
this renderer will want.

Traps worth knowing before editing any of it:

- A prefix (`FROM`/`TO`/`WITH`/`ALT`) survives only until the next non-prefix,
  non-branch instruction. Operand setup goes *before* the `FROM`/`TO` pair.
- Every branch executes the instruction after it. A multi-byte instruction in
  that slot has its operand bytes eaten from the branch target — keep it `NOP`.
- Branch displacements are a signed byte. Anything further needs `GFCALL` /
  `GFJMP`, which trampoline through R13.

## Geometry

`gsu/mesh.gsu` is the half of the renderer the GSU is actually good at.

`FMULT` is a signed 16x16 multiply returning the high word -- exactly a
fixed-point multiply with a 16-bit fraction -- so a 3x3 rotate is nine
instructions of real work. The same expression on the 65816 is nine calls to
`tcc__mull`. Matrix elements are 1.15, meaning 1.0 is `$7FFF` and `FMULT(v, m)`
comes back halved; the doubling is put back once after the three products are
summed rather than three times.

Projection reuses the reciprocal table: `dz = z >> 7` indexes it, so the focal
length is 128 pixels, and `FMULT(x, 65536/dz)` is `x / dz`. The shift is done
as `HIB` then a self-add rather than seven `LSR`s.

### The near plane (FX3)

That table has a floor. `65536 / dz` must fit the signed word `FMULT` reads,
so `dz` is 4 or more and the nearest depth it projects is 512 (`ZMIN`). On
GSU-2, 512 is also the clip plane, which leaves anything nearer than that
undrawn. Guns out at LEVEL1's start, the combat camera clips to the entrance
wall 257 from her, all of her is nearer than 512, and she was not drawn at all.
Upstream's plane is `VIEW_MIN_F`, 64.

FX3 clips at 64 (`ZNEAR`) and projects 64..511 out of line: `TM_Near` (the
vertex loop's arm, which used to flag such a vertex), `PC_Near` (ProjectClip's,
for clip vertices short of 512) and `ShearNear` (the frame's pitch, for a
target that close). Each reads the same table at `z >> 1` (32..255, 2048 at
most) and keeps the whole product with `LMULT`: `c * 65536 / (z >> 1) >> 10` is
`128 c / z` again, to two units of depth, and it meets the table's `128 c / 512`
at 512. `NEAR_SCALE` takes the product's top 22 bits with `LOB`/`SWAP`/`OR` and
two `ASR`s, and holds the result to ±8K: the range the table path reaches
(`128 * 32767 / 512`), which every edge difference downstream is sized for.
`tools/nearcheck.py` (`make check`, FX3 with LEVEL1 first) models nine close
views of the start and holds every near point to that arithmetic bit for bit:
within 1.5 pixels of exact `128 c / z` on the screen. All three hooks are
same-size replacements for what they displaced, `iwt r15` jumps (R13 is the
vertex loop's `LOOP` register) into code at the end of `mesh.gsu`, so no tuned
cache window moved: GSU-2's ROM is byte-identical, and FX3's raster section
moved 384 bytes, a multiple of its 16-byte lines.

Clipping at 64 made the first cut dearer, not cheaper. A face from beside or
behind the camera into the view (the floor under it, the walls either side)
now crossed the plane and went through the clipper, about 14 K cycles each in
the opening view, mostly to rasterize nothing. The bits OTBuild ANDs to drop a
face wholly off one side of the clip rectangle meant nothing for a near
vertex, so it carried bit 0 alone. Now a vertex at or behind the camera (z <= 0)
carries side bits too. "Past the right" for a vertex in front is
`128 x >= k z`, `k = x1 - SCREEN_CX`. At `z <= 0` with `k >= 0` that holds
whenever `x >= 0`, so the bit is the sign of x, and the same goes for the
other three sides with the signs of x, y and their own `k`.
`RoomClipFx3` (frame.gsu's `RoomClip` call, swapped) keeps `NK_MASK` to the
sides whose `k` has the right sign; the ring clears it. A vertex at 1..63 has
bit 0 alone. These are the half-spaces the in-front bits stand for, through
the camera, so a face wholly in one draws nothing -- exactly so for TM_Near's
vertices; the table's bucketed projection only pushes a vertex outward from
the centre, and for that to hide a drawn sliver the face needs a corner
within a few units of the camera itself. `nearcheck` models every view again
with the mask held at zero and requires the same frame. The
opening view went from 4.69 M modelled cycles before any of this, to 4.81 M
with the plane at 64, to 3.77 M with the side bits (`frame.fx3.unlit.baseline`).
The picture changed in its rightmost 22 columns: the wall the camera stands
beside, drawn rather than clipped away. Shadows follow the plane down to 64 (see Shadows);
`nearcheck`'s two shadow views seed `GS_SHADOW` and hold `SP_Near` to the same
arithmetic.

Backface culling evaluates `(x1-x0)(y2-y0) - (x2-x0)(y1-y0)` in 32 bits with
two `LMULT`s and an `SBC` borrow chain, because screen-space deltas multiply
past a word long before the sign stops mattering.

A quad's prev/next links never change, so `FaceLinksInit` writes them once and
each face only refreshes coordinates and UVs.

### Where a frame goes

**0.80 frames per second** in TypeSNES, which models GSU cycles — for a frame
that is now three or four rooms rather than one. A single room is about 2. `make check` counts flips of `fbShow` field by field and reports
it; the `fps` variable in the ROM is whole frames per 60 fields and cannot
tell two builds apart at this rate. The 15 fps figure elsewhere in this file
is the DMA ceiling on *presentation* and says nothing about how long the GSU
takes to fill a buffer.

Two numbers explain where the time goes, and they are not the ones instruction
counts point at:

| | |
|---|---|
| 5 cycles | every external ROM or RAM access at 21.4 MHz — **instruction fetch included** |
| ~5500 | spans per frame, averaging 8.7 pixels each |

The first one dominates. A two-byte instruction outside the cache costs ten
cycles before it does anything; inside, one per byte. Four fifths of this
frame used to be instruction fetch. See below.

The second is the older problem: per-span setup is a reciprocal lookup and two
`MulRecip` calls, and TR room geometry is mostly small polygons, so that setup
is amortised over nine pixels.

`make profile` says where the cycles are. `tools/gsusim.py` models CBR, the 32
cache-line flags and the fill cost, so its cycle count moves the way the
hardware does — it predicted this section's change to within a few per cent of
what TypeSNES then measured. It also reports the best 512-byte window it can
find, ranked by cycles *recoverable* rather than cycles contained, which is
the number that decides where the window goes.

### The instruction cache

The GSU fetches code at ROM speed unless the PC is inside 512 bytes starting at
CBR, where it is six times faster. `CACHE` points CBR at the block holding the
next instruction and clears every cache flag — **but only if CBR is not already
there**, so re-issuing it at the same place is free and issuing it somewhere
else throws the whole thing away.

The renderer used to `CACHE` seven different loops. Each one was locally
sensible and collectively they meant one 512-byte window wandering around a
5 KB program, so 24% of fetched bytes came from it. Now there is exactly one
window, and it holds the per-scanline path:

```
raster.gsu, in address order (as of the third pass, below; the first
pass's layout had EdgeStep and a per-scanline ScanClip in it)

  cold half     RasterF, SpanClampFlat, SpanFill, EdgeInit, EdgeSkip,
                SpanLeftClip, EdgeAdvance + EA_*, ShadeClamp, SpanShade
                                             -- per face, per strip, or never

  SpanFillT:    cache                        <-- CBR lands here
  the window    SFT_Loop, MulRecip, EdgeAdvanceX, DivTable, SpanBounds,
                RasterFT + RFT_*             -- 512 bytes of per-scanline path
```

Three things make it work:

- **Order by recoverable cycles per byte, not by call graph.** `SFT_Loop` is
  ten bytes that fetch a quarter of a million cycles' worth per frame, so it
  sits nearest the anchor; what is worth the least per byte is last, where it
  falls off the end. Whatever spills should be the code that minds least.
- **The cold halves of hot routines go out of line.** `SpanLeftClip` (the
  span that starts off the left of the screen) and, at the time, `SC_Skip`
  (the strip that starts above the top) were made branch-out-of-line. They
  cost a five-byte `GFJMP` each in the window and gave back seventy.
- **Nothing in the window may execute `CACHE`.** `SFT_Loop`, `ClipEmit` and
  `SpanFill` all used to. `ClipEmit` was the dangerous one: it runs per clipped
  vertex, mid-frame, and would have flushed the window for a six-iteration
  copy loop.

`ClearFrame` and `OTClear` keep theirs. `ClearFrame` runs once per frame
before any rasterising, so it costs one window rebuild — 2560 cycles — and
saves far more in its own loop. `OTClear` is the less obvious one: `DrawMesh`
runs it once per static-mesh instance and per model node, so its `CACHE`
rebuilds the rasterizer's window every mesh — and it still wins, because the
window it anchors also holds `OTBuild` and `OTDraw`, whose per-face loops are
hotter than the refault. Taking it out was measured: 29.2 M cycles became
31.7 M and the cache hit rate fell from 43% to 34%.

The result, on the same frame, byte-for-byte identical output:

| | before | after |
|---|---|---|
| fetched bytes served from cache | 24% | 67% |
| modelled cycles | 17.2 M | 11.2 M |
| TypeSNES, one room | 1.10 fps | **1.80 fps** |

A second pass on 2026-09-02, on the three-room opening frame, found two
more windows' worth and settled what the rest costs:

- **The model measures the wrong frame.** `StageItemList` copies the host's
  level blob into RAM bank 1 at 55 cycles a byte -- and only once, because
  the host clears `GS_LEVEL_READY` at boot and the GSU sets it after the
  copy. The interpreter starts from cleared RAM, so every `make sim` was
  the first frame after power-on, 2.85 M cycles dearer than any frame the
  game runs. It seeds the flag now; `--cold` gets the old frame back.
- **The vertex loop has a window.** `TM_Vertex`, `TransformPoint` and
  `ProjectPoint` are 450 contiguous bytes, run per vertex of every room and
  mesh, and nothing between them and `OTClear`'s `CACHE` moved CBR -- so
  they ran at ROM speed, 3 M cycles a frame of fetch, for want of one
  `CACHE` at the top of the loop. 2.1 M back for one byte.
- **`RomByteAt` inlined at the four per-face sites.** Three bytes of work
  behind an eight-byte far call to a routine outside every window: 0.33 M.

| | modelled cycles | from cache |
|---|---|---|
| before, the first frame | 28.4 M | 44% |
| the same, steady state | 25.6 M | |
| plus the vertex window | 23.4 M | 56% |
| plus the inlining | **23.1 M** | 57% |

(The opening view moved later that day when the camera learned to clip --
264 behind her instead of a sector inside the wall -- and that view models
at 19.1 M. Not a renderer change; the baseline was re-recorded with a note.)

What is still outside, per byte -- fetch cycles a frame over the routine's
size, which is the number that says whether it earns a place:

| | fetch cycles | bytes | per byte | |
|---|---|---|---|---|
| `DM_Vertex` | 3.88 M | 199 | 19.5 K | per face, between the two windows |
| `EdgeStep` and `ES_*` | 2.3 M | 286 | 8-11 K | per scanline, outside the raster window |
| `EA_Left`, `EA_Right`, `EA_Strip` | 1.7 M | 265 | 5-8 K | per scanline |
| `ScanClip` + `SC_Draw` | 0.68 M | 42 | 16 K | ten bytes past the window's end |
| `FaceVisible`, `CN_Edge`, `FaceLinksSet`, `EdgeInit`, `EdgeSkip` | 1.7 M | 500 | 1-6 K | per face |

And what is inside, by the same measure, is no colder: the `RasterFT` block
is the coldest resident at 9 K a byte and the span setup at 14-16 K, so the
outsiders that would fit displace as much as they recover. The window holds
the right 512 bytes; moving code around it is not where the next 10% is.

A third pass the same evening took the per-face setup, which was the next
30%, and it went the way the second pass said it would have to. Every face
ran `OTD_Face`, `DrawFace`, `DM_Vertex`, `FaceVisible` and the edge setup
uncached, about 3,650 cycles of fetch a face, because a window there would
have moved CBR twice a face -- 4,500 cycles of refill to save 3,650, which
is why it had been costed and refused twice. Batched, it pays: `OTDraw`
now sets faces up into slots in game pak RAM under one window -- the
`CACHE` at `OTD_Face`, per face, free when CBR is already there -- and
`BatchFlush` rasterizes the batch under the other when the 2.5 KB at
`$0200` is full or the mesh is done. Two refills per twenty-odd faces
rather than per face. A slot is the face's VertexLinks with the page, the
count, the topmost Y and its link's address tucked into the first link's
six spare bytes; `DM_Vertex` finds the top and writes the link cycle as the
vertices go by, so `FaceTop` and `FaceLinksSet` run only for the clipper's
faces. The RAM question the second pass left open answered itself: nothing
had used `$0280-$0FFF` since the face list moved out of it.

Then two things the ordering pass can do for free, because it already
walks every vertex of every face under its own window to find the depth.
A face whose deepest vertex is short of the near plane -- 402 of the
opening view's 1,116 -- has nothing for the clipper to keep, and it was
costing the whole clip to find that out; `OTBuild` does not file it. And
the cull is the same cross product whether it is made from the transformed
vertices or from a copy of them, so `OTBuild` makes it from the vertices
for every face it can -- one without a vertex short of the plane, whose
screen positions are real -- and a back-facing face is never copied at
all: 312 of the 714 left. What `DrawFace` sees is 402 faces, 394 of which
are drawn. `EdgeInit` also stopped zeroing twelve words that `EdgeStep`
rewrites before any scanline reads them.

| | modelled cycles | from cache |
|---|---|---|
| the opening view, this morning | 19.1 M | 62% |
| plus the batch | 15.9 M | 75% |
| plus dropping faces behind the near plane | 14.0 M | 77% |
| plus the cull in the ordering pass, and `EdgeInit` | **13.1 M** | 77% |

Same picture throughout, byte for byte; `frame.baseline` was re-recorded
at the end. One thing found on the way: CBR is the address *after* the
`CACHE` rounded down to sixteen, so what a window's last line holds
depends on where the linker put the file. The raster section is `ALIGN
16` now with a pad before `SpanFillT`, so its window starts on the anchor
and ends on `RFT_Advance`'s last byte whatever mesh.gsu does; `make
profile` shows `RFT_Advance` fetching in the hundreds when that holds.

What was outside then was the rasterizer's per-edge and per-scanline
code: `EdgeStep` and `ES_*` 1.6 M of fetch, `EA_Left`/`EA_Right`/
`EA_Strip` 1.3 M, `ScanClip` 0.6 M, `EdgeSkip` and `SpanLeftClip` 0.3 M
-- 4.5 M of a 13.1 M frame, and the "best window" the profiler proposed
at `EdgeSkip` would have thrashed against the span window every edge,
which is the trap above. The way out was the same one: precompute. An
edge is `EdgeStep`'s seven words and depends on nothing but its two
vertices, so the batch's VertexLinks grew to 32 bytes and carry the edge
that leaves each vertex along the chain that will walk it -- lines, the
far cell, the four steps -- filled in by `EdgePrepBatch` under a third
window once per batch, with its own copies of `DivTable` and `MulRecip`
so that they sit inside it. The two chains leave the top vertex in two
directions and a cell holds one edge, so each face gets a cell past its
last vertex: the top again, with the right chain's first edge, where
`EdgeInit` now starts the right chain. `EA_Left` and `EA_Right` are
copies of a record. 13.1 M became 11.9 M.

Then `ScanClip`, which asked "is this scanline on the screen" 4,000 times
a frame from inside the window and answered from ten bytes past its end,
became a test of the *strip* in `EA_Strip`: below the screen is the end,
above it is one `EdgeSkip` to the clip top or past the whole strip, and
the strip is cut at the clip bottom. That changed the picture, and the
comparison of the two frames is worth keeping: the old one has black
wedges wherever a wall or ceiling crossed the top of the screen. `SC_Skip`
skipped the lines above the screen and then returned "go get another
strip", and `EdgeAdvance` obliged with a strip the edges' heights had
already been charged for -- so a strip that started above the screen lost
whatever of it was below the clip top. It had been there since the
per-scanline clip went in, invisible in the opening view's CRC because
the baseline was recorded with it. 11.9 M became 11.8 M with 16% more
pixels, `frame.baseline` was re-recorded, and the six other views the
batch was checked against all gained the same kind of pixels and nothing
else.

| | modelled cycles | from cache |
|---|---|---|
| the third pass's end | 13.1 M | 77% |
| plus the edge pass | 11.9 M | 85% |
| plus the strip clip, and the corrected picture | **11.8 M** | 87% |

### The sixth pass: what draws nothing

The passes above made every face cheaper. This one (2026-09-04, late)
asked how many faces there were, with three switches added to
`tools/gsusim.py` for the purpose: `--count LABELS` counts entries to any
symbol, `--faces` histograms pixels per rasterized face, `--facebox`
classifies every face the ordering pass and `DrawFace` see against the
clip rectangle, and `--meshes` reports per `TransformMesh` call how many
vertices it transformed and how many faces it filed. The opening view:

| | |
|---|---|
| faces rasterized | 392 |
| of which drew no pixel at all | **285** |
| faces carrying 93% of the pixels | 17 |
| faces `DrawFace` saw wholly left of the clip rectangle | 187 of 406 |
| wholly above it | 15 |

A room seen from inside has half its faces off the screen, and every one
of them went through the vertex copy, the edge setup, the strip walk and
the scanlines to plot nothing. The GBA port rejects those in
`transformRoom`: a clip flag per vertex for each side of the viewport,
and a face whose vertices share one is never added. The same here:
`TM_Vertex` has screen X and Y in registers when the vertex is stored, so
it writes a word of flags into the transformed-vertex record's spare
slot (`TV_CLIP`: 2 left, 4 right, 8 above, 16 below, and 1 alone for a
vertex short of the near plane, whose screen point means nothing);
`OTB_Vertex` ANDs them over the face, and a non-zero result is a face
that is not filed. Byte-identical on seven of eight views. The eighth,
the bridges in room 14, lost seven pixels at the top-right corner --
which the classifier traced to three faces spanning x 76 to 134 at y -160
to -6, entirely above the screen, whose strips the old build had walked
into two rows of garbage. The reject removes the artefact.

Two more from the per-mesh counts. `OTDraw` walked all sixty-four depth
buckets for every mesh, whether it had filed sixty faces or none; the
ordering pass records the range it used (`F_BMIN`, `F_BMAX`) and the walk
covers that, and a mesh that filed nothing -- common now -- returns at
once. And in the room 14 view 49 of the 85 model nodes and 12 of the 24
static instances were built, transformed and filed nothing: the packer
ships a bounding radius per model (the largest frame box over all its
animations, in units of 32, in the header byte that was padding) and per
static placement (its visibility box, in the placement's spare byte), and
`SphereVisible` tests the sphere against the clip rectangle before a
node or an instance is built -- the transform, the radius projected at
the sphere's nearest depth plus two pixels, four compares. What the GBA's
`boxIsVisible` does with a box.

| view | before | clip flags | bucket range | sphere cull |
|---|---|---|---|---|
| the opening view | 6.54 M | 4.30 M | 4.18 M | **4.08 M** (5.25 fps) |
| behind her at the start | 5.43 M | 3.00 M | | **2.75 M** (7.8 fps) |
| room 14, six rooms and eighteen items | 14.9 M | 10.4 M | 9.73 M | **9.16 M** (2.3 fps) |
| room 2, the bats | 7.08 M | 5.25 M | | **5.03 M** |

What is left is the geometry that is on the screen, or nearly: in room
14, 1,469 of the 2,333 vertices transformed and 1,273 of the 2,069 faces
the ordering pass visits belong to the six rooms, and 352 of those faces
are filed. The vertex path -- `TM_Vertex`, `OTB_Vertex`, `DM_Vertex` --
is 30% of that view at about 450 cycles a vertex, and no per-vertex trim
is worth more than a few per cent of it. The lever that remains is
count: the packer grouping a room's faces and vertices by sector cell,
so that a cell whose vertices all share a clip flag skips its faces
without the ordering pass visiting them, and a cell's corners tested
before its vertices are transformed at all. A day's work on both sides
of the packer, for perhaps a third of the vertex path.

What was outside before this pass: the per-edge takes, `EA_Left`/`EA_Right`/`EA_Strip`,
1.4 M of fetch for 2,700 of them, and they are already copies -- the
window that holds the span path has no 200 bytes to give them. The
per-scanline path is the biggest bucket at about 3 M: `EdgeAdvanceX`'s
ten accumulators in RAM at three accesses each, and the per-span gradient
setup that was measured and kept. The rest is the vertex work, which is
cached and instruction-bound. At 1.8 frames a second the design is near
its floor for this frame size; a half-height buffer is the lever left,
and it is a decision about the picture, not the code.

### The seventh pass: do not multiply, load, or prefix twice

The Mode 7 frame made the vertex path the largest remaining bucket. Fixing
`gsusim.py --meshes` so its symbols and hooks are installed alongside
`--profile` made the split visible in the opening view: the two room calls
transform 555 vertices, the static call 40, and the fifteen model nodes 300.
Room geometry and static placements use only camera yaw (a static adds another
Y rotation), so `TransformRoom` now evaluates their sparse matrix with five `FMULT`s rather
than the general path's nine. It deliberately retains the `m11` multiply so
the `$7FFF` fixed-point rounding is identical. Model nodes still enter the
general path.

The transformed-vertex record is 14 bytes rather than 16. Its side flags
already had bit 0 reserved for the near plane, so keeping a second near word
made every transform store it and every face pass load or reconstruct the
same fact. `OTB_Vertex` now rejects an all-near face from the common flag and
`DM_Vertex` takes the clip decision from it. The packer emits face offsets at
the new stride.

Two instruction-stream cuts finish the pass. Redundant same-register
`FROM`/`TO` pairs become one-byte `WITH`s, and explicit default R0 source or
destination prefixes disappear; this removes more than 300 static prefix
bytes across the GSU program. In the Mode 7 textured loop the column table is
loaded once per span instead of once per pixel, with a small signed counter
walking a tile row and adding 504 at each boundary. The shared edge take
addresses `E_H` directly, and its corrupt-edge exit lives just before the
cache window. Together those trims leave `RFT_Advance` ending exactly at the
default build's 512-byte cache boundary.

The result is byte-for-byte identical on all eight `tools/viewcheck.py`
views:

| view | sixth pass | seventh pass | change |
|---|---:|---:|---:|
| opening | 4.079 M | **3.835 M** (5.58 fps) | -6.0% |
| behind Lara at the start | 2.754 M | **2.542 M** | -7.7% |
| turn at the start | 1.459 M | **1.316 M** | -9.8% |
| room 14, six rooms and eighteen items | 9.159 M | **8.615 M** (2.48 fps) | -5.9% |

Room 14 still spends about 30% in the transform, ordering and draw vertex
loops. The next large lever therefore remains reducing the number of room
vertices visited, not shaving another instruction from a pixel.

A fourth pass on 2026-09-03 found the floor was not where the third had
left it. The per-span setup was 90 instructions and a ROM round trip
where 40 would do: a far call to `DivTable` that switched ROMB away from
the texture page and back, two far calls to `MulRecip`, and every result
copied through RAM before the pixel loop reloaded it. The same
arithmetic to the bit, with the reciprocal from a copy of the table in
game pak RAM (`RECIP_TAB`, filled once a frame by `frame.gsu`), the two
multiplies inlined, and the results left in the registers the loop
reads, is half the instructions and no ROM access; ROMB is set once a
face. That and folding `SpanBounds`, `EdgeAdvanceX` and `SpanFillT` into
one straight-line loop (`Raster_Common`, `RFT_Line` to `RFT_Advance`,
nothing called on the way, the strip's line count in R3 and the span's
right end in R11) gave the window back 150 bytes, which is what the edge
takes needed: `EA_Left` and `EA_Right` were two 120-byte copies of the
same routine outside the window at 85% fetch, and one `EA_Take` with a
copy loop -- the two edge blocks have the record's own layout, next cell
first, so the block's first word says where to copy from -- is 80 bytes
inside it, with the strip clip beside it. 11.75 M became 8.72 M.

Then the vertex path, which was cached and instruction-bound and turned
out to be RAM-bound: `TransformPoint` loaded each coordinate from RAM
three times and stored and reloaded every result, `ProjectPoint` called
`DivTable` out of the vertex window through the ROM buffer, and the
seven stores went through `T_OUT` seven times. `TM_Vertex` has the same
arithmetic inline with the vertex in R1-R3, the camera point in R9-R11,
the cursor in R7 and LOOP counting, and the callable `TransformPoint`
and `ProjectPoint` stay for the portal test and the clipper. `OTB_Vertex`
and `DM_Vertex` keep their counters in R12, the running depth in R11 and
`ZMIN` in R3, and read the vertex's byte offset in the transformed array
-- the packer multiplies the index by `TV_SIZE` now -- instead of
doubling an index four times each. `MatrixRotate` anchors a window over
`MR_Row` and `MTR_Row`. 8.72 M became 7.71 M. (The packer has a third
face writer, `buildSpriteMesh`, and the pickup test is what found it
still writing indices: the eight geometry views have no sprite in them.)

Then the edge pass, where `EC_Loop` sign-extended a link and doubled it
five times when `MULT` on the byte is one instruction, and `MulRecipE`
was a call with the record cursor and the reciprocal going through RAM
between calls -- R11, R12 and R13 are free there; and the edge step,
which loaded ten accumulators after the span that the span setup had
loaded before it: each is stepped for the next line as it is loaded for
this one, with this line's uL and vL waiting in R12 and R13 until the
merges are done. 7.71 M became 7.41 M.

| | modelled cycles | from cache |
|---|---|---|
| the edge pass's end | 11.75 M | 87% |
| plus the span setup, the fused loop and the shared take | 8.72 M | 93% |
| plus the vertex path in registers and the packer's offsets | 7.71 M | 94% |
| plus `MULT` in the edge pass and the steps on the loads | 7.41 M | 93% |
| plus a window over the clipper and the left clip in the loop | **7.05 M** | 94% |

The last row came from the camera: once it was read against upstream
(see "The camera follows her") the settled opening view moved to her
side, looking across the corridor, and that view clips every wall face
against the near plane and starts every other span off the left of the
screen. The clipper -- `ClipNear`, `CN_Edge`, `ClipT`, the lerp, the
copy and the projection of what survives -- had run at ROM speed for
every such face, 0.9 M a frame in that view; it has a `CACHE` at
`ClipNear` now, `CN_Edge` walks a pointer and carries each vertex's
side of the plane round instead of computing both ends of every edge
from an index, the intersection and the lerp read `RECIP_TAB` with the
multiply inline, and the projection of the clipped vertices is
`TM_Vertex`'s in registers. `SpanLeftClip` is inline in the span path,
and the flat/textured choice is a stored jump target rather than a
flag tested per strip. The side view went from 10.30 M to 8.77 M; the
behind-her view above, which `frame.baseline` no longer records, is
what the player sees most and is the 7.05 M.

Byte-identical throughout: the opening view's CRC, and eight views of
LEVEL1 -- the entrance, the end of the walk, the climb in room 10 from
two sides, the falling floors in room 32 from two sides, and two turns
in room 0 -- compared frame and shade map against the ROM from before
the pass, in the `SHADING=1` build so that every arm is exercised.
Three things the pass leaned on and the next one should know:

- A `.DEFINE` of the cold half's size decides where the window's anchor
  lands: `RASTER_PAD` in raster.gsu, checked by `make profile`'s symbol
  file (`Raster_Common` must be a multiple of sixteen). The window is
  492 bytes of 512.
- Nothing reads R3 after `ClearFrame`, so the loops use it. Nothing
  reads R11 between a routine's link save and restore, so they use that
  too. `LMS` and `LDW` set no flags; `INC` then `DEC` does.
- The `SHADING=1` window is larger than 512 bytes and its tail runs from
  ROM. It builds and matches; it is not the build the frame rate is
  measured on.

What is left, in the order the profile points: the pixel loop at 1.05 M
(ten instructions and the ROM texel fetch; the plot cache write-back is
the rest), the span setup and step at 1.3 M (`RFT_Recip` and `RFT_Line`,
now mostly the RAM accesses that remain), the vertex path at 1.4 M, the
edge pass at 0.5 M, and 0.8 M of fetch in the cold half: the clipper
(a window there was costed at under 1% net once the refills it forces
on the setup window are counted), `EdgeSkip`, the per-face glue
between faces, and `RFT_EmptyStep` -- every face's first scanline has
both edges on the top vertex, so 660 spans a frame are empty by
construction and step U and V out of line. Past that it is the
half-height buffer: the per-scanline and per-pixel work is 3.3 M of
the 7.4 M, and that is a decision about the picture.

### The eighth pass: keep live values live

The next profile showed that the remaining face and edge loops were not short
of arithmetic; they were repeatedly spilling values that already had a free
register. The packed face format now writes the absolute game pak RAM address
`TV_BASE + vertex * TV_SIZE`, so both `OTB_Vertex` and `DM_Vertex` consume it
directly. `TransformMesh` and `TransformRoom` select separate cached loop
entries once per mesh, removing the RAM mode load and branch from every
vertex while sharing the projection and output half.

The same lifetime audit removed most of the short-RAM traffic in face setup.
`DM_Vertex` carries its link and clip cursors in R8/R9, its accumulated reject
mask in R3, its top Y in R4, and its constant middle-link word in R7; the shade
already loaded for the link is copied to the clip vertex instead of fetched
again. `OTBuild` keeps its outer face count in R3 and leaves R14 on the next
face throughout the cull instead of saving and restoring it on every exit.
The clipper keeps its component destination in R11, and `EdgeChain` carries
direction, output cell, height and record cursor in R1/R2/R3/R11. Copy-loop
increments that are unconditional now occupy `LOOP` or branch delay slots;
only single-byte instructions are used there.

Those code-size changes also required measuring all four raster layouts, not
only the default. Their `Raster_Common` anchors are aligned at `$2190`
(Mode 7 shaded), `$20E0` (Mode 7 unlit), `$20A0` (4bpp unlit), and `$2220`
(4bpp shaded). Correcting the last three one-byte phases made the legacy 4bpp
shaded build fall from 8.858 M to 7.603 M cycles by itself.

All eight comparison views remain byte-identical:

| view | seventh pass | eighth pass | change |
|---|---:|---:|---:|
| opening | 3.835 M | **3.536 M** (6.05 fps) | -7.8% |
| behind Lara at the start | 2.542 M | **2.345 M** | -7.7% |
| forward at the start | 4.058 M | **3.765 M** | -7.2% |
| turn at the start | 1.316 M | **1.238 M** | -5.9% |
| room 14, six rooms and eighteen items | 8.615 M | **7.993 M** (2.68 fps) | -7.2% |
| room 9 switch | 0.691 M | **0.651 M** | -5.7% |
| bridges | 2.345 M | **2.175 M** | -7.2% |
| room 2 bats | 4.729 M | **4.362 M** | -7.8% |

The original 4.079 M opening baseline is now 3.537 M, a 13.3% cycle reduction
across the two passes with the same `dce097eb` frame CRC. The Mode 7 unlit and
4bpp unlit baselines also retain their old CRCs at 3.363 M and 4.851 M cycles.
`make check` passes all 13 active whole-machine tests; `fixedcheck`, `matcheck`
and `animcheck` are clear. Room 14 still spends about 28% in the transform and
two face-vertex loops, so the next material gain remains sector-cell grouping
that avoids visiting room geometry, rather than another local spill removal.

### The ninth pass: cull articulated meshes by node

The model-level sphere cannot reject an actor that is partly visible even when
most of its articulated meshes are outside the room's portal rectangle. Each
11-byte node record now carries a conservative sphere centred on that node's
origin. The packer measures it from the actual mesh vertices, rounds the radius
up in units of 32, and writes zero when it cannot be represented safely. Flags
and radius are adjacent, so `DrawModel` gets both from one buffered ROM word.

The joint matrix must still be built because child nodes inherit it. After that
work, `SphereOriginVisible` tests the sphere and skips only the node's
`TransformMesh` and `DrawMesh`. Its centre is already the matrix translation
column, avoiding the nine zero multiplies of a general `TransformPoint`. A node
well inside a full-frame horizontal frustum takes a projection-free keep path;
if the accepted model's own centre meets the same condition, one flag disables
all of its redundant node tests. Narrow portal rectangles always take the
conservative projected-sphere path.

All eight comparison views remain byte-identical to the eighth pass:

| view | eighth pass | ninth pass | change |
|---|---:|---:|---:|
| opening | 3.536 M | **3.538 M** (6.05 fps) | +0.1% |
| behind Lara at the start | 2.345 M | **2.030 M** (10.5 fps) | -13.5% |
| forward at the start | 3.765 M | **3.455 M** | -8.2% |
| turn at the start | 1.238 M | **0.922 M** | -25.5% |
| room 14, six rooms and eighteen items | 7.993 M | **7.839 M** (2.73 fps) | -1.9% |
| room 9 switch | 0.651 M | **0.653 M** | +0.3% |
| bridges | 2.175 M | **2.176 M** | +0.0% |
| room 2 bats | 4.362 M | **4.302 M** | -1.4% |

The opening view is effectively flat, while the sum across the eight views is
1.151 M cycles lower, a 4.4% reduction. From the original 4.079 M opening
baseline the current 3.539 M Mode 7 frame is 13.2% lower and retains the
`dce097eb` CRC. The cold sphere code moved the ordering window, so an unreachable
38-byte pad restores its measured modulo-16 cache phase; the two hot vertex
loops retain the eighth pass's fetch counts. Mode 7 unlit and 4bpp unlit keep
their `c01930ff` and `7034f88f` CRCs at 3.365 M and 4.853 M cycles; 4bpp shaded
also keeps `7034f88f` at 7.604 M. `make check` passes all 13 active
whole-machine tests, and `fixedcheck`, `matcheck`, and `animcheck` are clear.

With that headroom established, the 256×160 4bpp path became the default again,
unlit because its one shade per 8×8 character is visibly too coarse at this
resolution. Its opening/behind/room-14 views cost 4.853 M, 2.896 M and 9.847 M
cycles; the optional unlit Mode 7 equivalents are 3.365 M, 1.962 M and 7.504 M.
This changes neither renderer—the Makefile now chooses full-resolution unlit;
`MODE7=1` asks for speed and `SHADING=1` asks for the experimental lighting.

### The tenth pass: pay for clipping only when clipped

`OTBuild` already ORed every transformed vertex's clip flags while deciding
whether to file a face. It now keeps that result in a byte-per-face table at
`$DA00`. `DrawFace` can therefore send a near-plane face directly to a cold
loader for camera X/Y/Z, UV and optional shade; its common loop copies only
the screen-space `VertexLink` fields. Unlit builds also compile out shade
loads and stores while retaining the same fixed record layouts.

Near vertices now branch around reciprocal lookup and both projection
multiplies. For vertices that pass the signed `z >= 512` test, the reciprocal
index is provably in the table's 4..254 range, so its old low and high clamps
are gone. In the 4bpp texel loop, the one-byte `PLOT` occupies `LOOP`'s delay
slot and still executes once for every pixel.

All eight 4bpp comparison views remain byte-identical:

| view | ninth pass | tenth pass | change |
|---|---:|---:|---:|
| opening | 4.853 M | **4.666 M** (4.59 fps) | -3.9% |
| behind Lara at the start | 2.896 M | **2.757 M** | -4.8% |
| forward at the start | 5.248 M | **5.065 M** | -3.5% |
| turn at the start | 1.591 M | **1.508 M** | -5.2% |
| room 14, six rooms and eighteen items | 9.847 M | **9.487 M** (2.26 fps) | -3.7% |
| room 9 switch | 0.836 M | **0.812 M** | -2.9% |
| bridges | 2.868 M | **2.745 M** | -4.3% |
| room 2 bats | 5.581 M | **5.355 M** | -4.0% |

The recorded opening frames keep their CRCs in all four layouts. 4bpp shaded
falls from 7.604 M to 7.438 M cycles, Mode 7 unlit from 3.365 M to 3.251 M,
and Mode 7 shaded from 3.539 M to 3.446 M. The optional unlit Mode 7
opening/behind/room-14 views are now 3.251 M, 1.888 M and 7.251 M cycles.

### The eleventh pass: cache the flat polygon loop

Flat-colour model faces were paying ROM instruction-fetch costs on every
scanline and updating four texture accumulators they never read. Their loop
now sits immediately before the shared edge walker. Each face chooses one
of two overlapping 512-byte cache windows: flat scanlines plus the walker,
or the walker plus textured scanlines. Neither changes the cache per strip.
Flat spans also put `PLOT` in `LOOP`'s delay slot, and skip the unused UV steps.

The 4bpp textured loop schedules both UV adds before `GETC`, giving the ROM
buffer more time to finish. The cycle model does not model that overlap, so
the figures below claim only the savings it measures. The instruction rules
are Book II's Super FX cache/ROM-buffer sections and Ch.9's `LOOP`/`PLOT`.

| opening view | tenth pass | eleventh pass | change |
|---|---:|---:|---:|
| 256×160 unlit (default) | 4,665,795 | 4,467,379 | -4.25% |
| 256×160 shaded | 7,437,966 | 7,178,622 | -3.49% |
| Mode 7 unlit | 3,251,249 | 3,177,965 | -2.25% |
| Mode 7 shaded | 3,446,499 | 3,361,161 | -2.48% |

These are modelled GSU cycles, not measured console frame rates. The default
opening estimate rises from 4.59 to 4.79 fps before host bus waits and uploads.
The gain depends on flat faces: room 14 saves 0.9%, room 2 saves 0.2%, and the
other five comparison views are unchanged. All eight views are byte-identical
in all four variants, including the 4bpp shade maps. The four baselines retain
their CRCs. Whole-machine checks pass for the default and both shaded layouts;
LEVEL1's 13 gameplay checks and LEVEL2's three pass, as do the fixed-point,
matrix and animation oracles.

The same pass fixed two gameplay bugs. The signed 16-bit angle check accepted
`-32768` after trying to negate it, allowing Lara to use a switch while facing
directly away. It now compares against the signed angle window. Pushable blocks
tested the floor in the wrong direction: unsupported blocks never acquired
gravity. They now fall while above the floor, stay active until landing, clamp
to the floor and clear vertical speed before restoring the occupied sector.
Both bugs have whole-machine regressions that failed before their fixes.

`tools/viewcheck.py` now returns failure when any view differs, checks the
simulator's exit status and compares shade-map bytes as well as the framebuffer.
Previously a palette-only regression passed unnoticed, and other differences
were printed with a successful exit status. Its harness regressions run with
`python3 -m unittest discover -s tools -p test_viewcheck.py`.

## Porting src/fixed

529 KB of C++ against a C compiler with no 32-bit `int`. It cannot be
recompiled, so it has to come across a file at a time -- and *come across*
rather than be rewritten, which is a distinction that took a correction to
learn. What sat here before was 1500 lines of `ol`-prefixed lookalikes:
functions that did what `animProcess` does without being it. They passed the
same tests and they were not a port, because nothing about them told the next
reader where to look upstream.

So the file layout is upstream's, name for name:

| here | from | what it is |
|---|---|---|
| `src/item.c` | `item.h` | `ItemObj`: the animation graph, commands, timers, an entity's floor |
| `src/room.c`, `src/room.asm` | `room.h` | `Room_query` — portals, vertical rooms, point-exact slants, in assembly for the cycles — and `checkTrigger`, TR's `FloorData` already resolved |
| `src/nav.c` | `nav.h` | the box graph, flooded from the target outward |
| `src/enemy.c` | `enemy.h` | moods, and the table of what a creature is |
| `src/object.c` | `object.h` | the type-to-class dispatch, and the classes it can reach |
| `src/level.c` | `level.h` | where the level's tables are |
| `src/lara.c` | `lara.h` | 22 dry-land movement/air/death state handlers |
| `src/camera.c` | `camera.h` | follow mode: where the view goes and how it gets there |
| `src/common.c` | `common.cpp` | the AABB helpers: intersect, contains, push out, translate, rotate by quadrant |
| `src/math.c` | `common.cpp` | `phd_sin`, `phd_cos`, `phd_atan`, `phd_sqrt`, `rand_logic` |
| `src/game.c` | -- | what is left: the camera, the pad, the host loop |

### Conventions

All forced by the compiler rather than chosen:

| | |
|---|---|
| `int32` | `long long`. There is no 32-bit int; every 32-bit operation is a library call |
| member functions | free functions taking the struct first: `ItemObj_update(item)` |
| operators | named functions; there is no overloading |
| references | pointers |
| names | ported functions keep their upstream names, so the two read side by side. Anything only this target has is prefixed `ol` |
| tables | the *level's* are not copied at all: the GSU stages them in game pak RAM and the host reads them where they lie, so `level.anims[i].frameEnd` becomes `olAnimFrames(i)` over a macro. Only the maths tables ride in the image |
| structs | sized to a power of two where they are indexed in a loop. `ItemObj` is 32 bytes for that reason alone: every index into an array of structs is a multiply on this compiler, and a power of two is a shift |

The tables row is why `tools/mktables.py` exists rather than a transcription of
`gSinCosTable`: upstream's is 4096 packed 32-bit entries, 16 KB, in ROM, which
is precisely where the host cannot read. The sine table here is 1024 int16
entries — 2 KB, 0.35 degrees, and `phd_sin` rounds rather than truncates so the
error is half a step instead of a whole one.

### What it costs, measured

The table below records the initial C implementation. The current cartridge
uses `src/math.asm` for sine, cosine, atan and square root: 265 bytes for all
four, down from 1,401 bytes of compiled helpers. Repacking bank 5 recovered
1,182 bytes of contiguous room before the combat additions.
Both tables are unchanged (2,306 bytes); `math.c` retains the C reference and
the PRNG. Atan computes its seven fractional quotient bits exactly in
16-bit arithmetic, and square root keeps the full 32-bit remainder.

The emulator checks every 16-bit trig angle, atan octants and square-root
boundaries. `mathcheck.c` also calls them through compiled C at boot. The
integer return convention is `tcc__r0`/`tcc__r1`; `tcc__r0h` belongs to far
pointers. The native code and tables stay in bank 5's WRAM image on GSU-2
and its `$85` FastROM alias on FX3. The boot checks moved to bank `$3F` so
the new weapon unit fits beside `game.c` in bank 4.

`phd_sin`, `phd_cos`, `phd_atan` and `phd_sqrt` are in, and `src/game.c` moves
the camera with them — the first thing out of that engine running on the
cartridge, and the walk test says it moves the camera to the same world unit
the placeholder did.

| | bytes |
|---|---|
| `.rodata` (both tables) | 2340 |
| `phd_atan` | 1442 |
| `phd_sqrt` | 624 |
| `phd_cos`, `phd_sin` | 311 |
| whole WRAM image so far | **6801 of 32768** |

`phd_atan` is 25 lines of C. **That is the number that decides the shape of the
port**: about 60 bytes of code per line where the line is 32-bit, about 20
where it is not. 32 KB is six hundred to sixteen hundred lines of C, another
32 KB is sitting unallocated at `$7F:8000`, and `src/fixed` is some fifteen
thousand lines. Paging code banks in behind a stopped GSU is not a
contingency, it is the plan, and what goes in which bank wants deciding before
the porting rather than after.

One thing that was tried and did not work: `phd_atan` rewritten to reduce its
ratio into sixteen bits, on the theory that `int32` is a library call and
`int16` is an instruction. It came out *larger* — 1518 bytes against 1442 —
because normalising a 32-bit pair down to 16 costs more shifts than the 32-bit
divide it saves. The rule is that `int16` is worth reaching for when a value is
*already* small, not when it has to be made small.

## Toolchain traps

These have cost real debugging time, so they are worth stating plainly.

**The manual swaps BLT and BGE; wla-superfx does not.** The instruction
pages (9.17 BGE, 9.20 BLT) give BLT = `$06` and BGE = `$07`. The hardware is
the other way round: `$06` is BGE (`S xor OV = 0`) and `$07` is BLT (`S xor
OV = 1`) -- Star Fox's own outcode routine only works that way, and bsnes,
snes9x, Mesen, fullsnes and wla-superfx agree. This port followed the manual
until 2026-09-26: `GBLT` expanded to `bge`, and TypeSNES and `tools/gsusim.py`
decoded the manual's way too, so all three agreed on four signed branches
that would have gone the wrong way on a cartridge. The "afternoon lost to
MatrixPush" once recorded here -- a `blt +` stack guard that seemed to
assemble to its opposite -- was that emulator swap, not the assembler. A
TypeSNES session found it with Star Fox; the macros now expand to the plain
mnemonics, gsusim decodes the hardware's way, and the frame CRC did not move.
Keep using `GBLT`/`GBGE` so the four sites read as what they mean.

**Prefixes are consumed by the next instruction.** `FROM`/`TO`/`WITH`/`ALT`
survive only until the next non-prefix, non-branch instruction, so operand
setup goes before the `FROM`/`TO` pair. `ROMB`, `RAMB`, `COLOR` and `CMODE`
all take Sreg, which defaults to R0; loading a value into R6 and then
executing `romb` sets the bank register from R0 instead, silently.

**`int` is sixteen bits, and so is a constant expression in one.** Upstream's
`LARA_TURN_ACCEL` converted for this frame rate is 92025, which a `#define`
computes at compile time, wraps to 26489, and hands over without a word. It
happened to still work, because the wrapped value was above the cap it gets
clamped against and the cap is what the behaviour depends on — which is worse
than failing, since nothing would have said so. Anything derived from
upstream's constants by a multiplier needs the product checked, not the
factors.

**`816-tcc` gets `a && f()` wrong when the right-hand side is a call.** The
mood code below returned "escape" from a branch guarded by a flag that was
provably zero, on every creature in the level, and the flag was read correctly
three lines above. Writing the short circuit out by hand fixes it:

```c
if (hurt) { if (rand_logic() < 0x800) return MOOD_ESCAPE; }   /* not && */
```

That took an afternoon to believe. Nothing in this port should put a call on
the right of `&&` or `||`.

**`816-tcc` gets `A || !x` wrong: the earlier operands' "true" is lost.**
An operand of an `||` chain, past the first, that is a logical not of
anything but a comparison -- `!x`, `!p->f`, `!(x & m)` -- makes the
backend resolve the earlier operands' true-jumps where evaluation goes on
as if they were false: `if (s != 9 || !n) return;` returns only when `n`
is 0, `A || !x || C` decides on `C`, and `while`, `return` and `?:` the
same. `x == 0` in its place, `!x || A`, `A || (B && !C)` and `A && !x`
compile right. It drew a line across every inventory frame for ten days
(2026-10-06), and also had a dead Lara keep hanging while Action was held
(`collision.c`) and `fireTrigger` reach past `itemsCount`. Five sites in
all, now split into two tests; `tools/tcclint.py` runs before every
compile and refuses the shape.

**`.bss` is not cleared, and neither is the direct page.** There is no crt0
here, so a global that is not written before it is read is whatever the
machine was last thinking. It has now caused three bugs: a matrix stack
pointer, every creature in the level deciding to run away, and two phantom
gunshots at power-on from an uninitialised pad accumulator. Initialise, or
expect the last one.

**An initialised static or global is a bug.** `816-tcc` puts it in a
`.data` pair: a ROM half holding the value and a RAM half holding the
variable, and there is no crt0 to copy one into the other. The RAM half
links at `$04:0000`, which mirrors the direct page, so the variable also
*aliases the host's own variables*: a `static unsigned int rsRoom = 0xFFFF`
in `room.c` shared an address with `keys`, and a camera cache in `game.c`
shared one with `fps` and `fbShow`. Every floor in the level read as a wall
and the runner reported nine frames a second. `tools/budget.py` now fails
the link when any unit has one; initialise in code, at level load.

**`RAMBR` banks every game pak RAM access, `LMS` and `SMS` included.** That
reads as obvious written down and is not while writing a routine: switching to
bank 1 to stage a table there sent the loop's own scratch variables to bank 1
too, where they read back as zero, so the copy decided its length was zero and
returned having moved nothing. A routine that changes `RAMBR` has to run out
of registers until it changes it back.

**Changing `ROMBR` does not reload the ROM buffer.** A write to R14 starts a
fetch. Code that switches from a texture bank back to the geometry bank must
rewrite R14 before its first `GETB`, even when the desired address is already
in R14.

## Building

```
brew install wla-dx        # wla-superfx for the GSU side
git clone .../pvsneslib    # 816-tcc, 816-opt, and its patched wla-65816
make PVSNESLIB=~/dev/pvsneslib      # -> build/openlara.sfc
make SHADING=1                      # with per-character shading, see above
```

`SHADING` is written into `build/config.inc`, which every object includes and
depends on, so changing it rebuilds what it has to and nothing else. No
`make clean` needed, and the file records what the ROM in `build` was built
with.

Changing it costs an extra second, on purpose. GNU make 3.81 — what macOS
ships — compares mtimes to the whole second, and a build of this size fits
inside one, so a config file written in the same second as the objects that
include it reads as up to date: `make SHADING=0` after `make SHADING=1` used
to leave the shading ROM in place, claiming to be the other one. The rule
sleeps before touching the file so it lands strictly ahead of everything
already built. Deleting the stale objects instead does not work — make stats
its targets once, at the start, so a file a recipe removes partway through is
still remembered as present.

Three assembler builds are in play and they never link against each other.
The GSU side uses whatever `wla-dx` is on `PATH` and produces a raw blob the
65816 side INCBINs; so does the SPC700 side (`wla-spc700`, `src/spc.asm` ->
`build/spc.bin`, 512 bytes). The 65816 side uses PVSnesLib's patched
`wla-65816` (v10.7a), because `816-tcc` emits `{WLA_FILENAME}` substitutions
that only that build understands, with the `-d -s -x` flags PVSnesLib passes.

`tools/snesfix.py` patches the header checksum afterwards. The header is hand
rolled rather than written with `.SNESHEADER` because a Super FX pak has to
declare its expansion RAM at `$FFBD`, which that directive has no field for.

## Verifying

There is no way to look at this target's output from a terminal, so there are
oracles instead.

```
make sim         # interpret just the GSU program  -> build/gsu_frame.png
make check       # boot the whole ROM headless     -> build/frame.png
make baseline    # accept the opening frame's modelled cost as the record
make profile     # where the cycles went, by routine
make fixedcheck  # the ported engine code against libm
make motioncheck # native motion/animation/bounds and wolf decisions vs CPU-only C ROM
make matcheck    # the matrix rotations against floating point
make animcheck   # the per-entity animation channel
make run         # bsnes+ or another emulator, for a human
```

The CPU/gameplay regressions can also run on their own:

```sh
make check TESTS='native maths|pistol combat'
make BUILD=build_fx3 FX3=1 check TESTS='native maths|pistol combat'
make check TESTS='Lara diving|bats|native item motion'
make BUILD=build_fx3 FX3=1 motioncheck
make BUILD=build_l2 LEVEL=LEVEL2 motioncheck
python3 -m unittest discover -s tools -p 'test_*.py'
```

They execute the shipped cartridge routines, including the 16-bit compiler's
calling convention. The combat fixture supplies small room tables to isolate
reload timing, active/hidden/dead targets, 3D range, aim limits, walls, floors,
ceilings, portals and wrapped coordinates. It also checks asymmetric boxes at
cardinal and diagonal rotations, negative midpoint rounding, wide bounds,
angle priority and active-list ties, Lara's pitch, and visibility to a rotated
aim point. Tracking checks cover held identity, release, visibility loss,
independent arm limits and reacquisition, invalid targets, range changes and
the coordinate seam. The retention regression fails on the preceding ROM;
256 randomized boxes independently check the native aim-point arithmetic.
The look fixture checks 400 signed arm-angle combinations, 960 decay ticks,
death/reset, torso-relative pitch/yaw acquisition and body-relative arm
retention. It includes targets across the angular seam and candidate ordering
that must preserve the preceding tick's arm flags.
The frame-aiming fixture reads raw source boxes and timing from `AIM.json`,
chooses the nearest eligible pose independently, and checks every tick in
all 51 creature animations at two rotations, including wrapped coordinates.
It also checks sparse entries, invalid ticks and invalid animation indices.
Full `make check` additionally exercises controller input, creature death,
sustained firing and the level's actual gameplay.

Validation for the current-frame creature aiming slice:

- LEVEL1 on GSU-2 and FX3: all 17 active checks pass. Four scenes are skipped:
  LEVEL2-only scenes and opt-in profiling.
- `make BUILD=build_l2 LEVEL=LEVEL2 check` and
  `make BUILD=build_fx3_l2 FX3=1 LEVEL=LEVEL2 check`: each 6 passed;
  covers water, the key/door, the pushable block and all three CPU fixtures.
- `make animcheck weaponcheck`: all clear on GSU-2 and FX3, including
  555 joint/frame cases and 120 look matrices each. These also exercise the
  ROM pointers relocated by the larger host table.
- Deleting the generated `AIM.json` and rebuilding under `make -j2`
  regenerates it and `LEVEL.BIN` together, byte-identical to the first pack.
- All six opening-frame baselines retain their CRCs, within 0.13% of their
  recorded costs. Both LEVEL1 cartridge frames match the GSU reference
  byte-for-byte, and both walking scenes sustain 30 logic ticks per second
  with no dropped ticks. This is emulator validation, not a hardware qualification.

`make sim` runs `tools/gsusim.py`, a Super FX interpreter covering the subset
the renderer uses and modelling its cycles, mapping GSU ROM banks the way the chip does so texture
reads come from where they really live — including the parts that are easy to get wrong: prefix
lifetime, delay slots, and the PLOT address calculation. It is fast and it
isolates renderer bugs from host bugs. It seeds the shade maps exactly as the
host does at boot, and renders its PNG through the tile map, because the frame
buffer read on its own is only half the picture.

The interpreter stops after 20 million instructions by default. `--limit N`
changes that guard; if it fires, the report includes PC, CBR, RAMBR, ROMBR,
all sixteen registers, and RAM windows around the live face/link pointers.
That turns a corrupt edge cycle or missing return into a useful state dump
instead of an apparently hung `make sim`.

It also writes what the frame cost — the modelled cycles, the cache hit rate
and a CRC of the picture — and `make check` holds that against the matching
`frame*.baseline` before it boots anything. A renderer change that moves the
opening frame's cycles by more than two per cent, or changes its picture,
fails there until `make baseline` records the new numbers, and the record is
tracked so the change lands in the diff beside the code that made it. That
is the whole point: a frame that got ten per cent dearer used to be found
weeks later, by wondering. The first thing it caught was the blob
restructure above — the GSU copies the blob into RAM every frame, 13 KB less
of it is 2.5% of the frame, and the check refused until that was recorded.
Only the opening view is recorded; `make check TICK=n` or a moved camera
skips the comparison and says so. `tools/framecheck.py`.

`make check` boots the real ROM headless in TypeSNES — a SNES emulator with
working Super FX support — and dumps what the PPU produced. Point `TYPESNES` at
your checkout (default `~/dev/typesnes`). It copies a runner into that tree,
runs it, and removes it again, leaving nothing behind. The per-test timeout is
raised to ten minutes, because a second of emulated SNES is about a frame and
the trigger test walks the camera around a level.

`make check` runs a tenth test in room 10, at a two-click step: it pins
Lara her radius off it, facing it, and holds Up+Action. She must pass
through `STATE_HANG_UP` and come to rest in STOP on the ledge, 512 above
where she stood and past the step's edge.

`make check` runs a ninth test on that rock: it turns Lara round at the
level start and holds Up. The rock's box ends at Z 3245 and the room's wall
is at 3072, so she stops at 3345 with the mesh and at about 3172 without
it -- 170 units that make it a test of the mesh rather than of the wall.

`make check` runs an eighth test on LEVEL1's falling floors, in room 32: it
stands her on one at the height of its own top -- which is upstream's test,
`getLara(pos)->pos.y != pos.y - 512` -- and requires the floor to pass through
`STATE_SHAKE`, `STATE_FALL` **and `STATE_DOWN`**: it notices her, shakes,
falls under `ItemObj::move`'s gravity and lands on the pit floor, because
`ItemObj_updateRoom` can ask `Room_query` for the sector below a falling item
at any point of its arc. This also pins the active-list head in WRAM:
it was once linked over the direct-page FPS counter, so NMI changed the head
and the floor updated once before stopping.

The trigger that starts it is the level's own, found by descent. LEVEL1's
trapdoors carry their trigger on room 32's sector; she walks on room 35's
floor *above* it, at 2048 — which is exactly `item.y - 512`, so upstream's
constant is right and the geometry lines up. `Room_query` **descends through
`roomBelow`** while the point is at or under the sector's floor, exactly as
TR's `Room::getSector` does, and the test asserts that the trigger under her
feet is the trapdoor's own. The one-probe GSU channel could not descend, this
floor could never activate, and the test once had to poke it by hand — the
gap that put the vertical descent at the top of the list it has now come off.

`make check` runs a seventh test on Lara herself: that she starts standing on
the animation TR's own chain arrives at, that holding up puts her in
`STATE_RUN` through the transition table rather than by assignment, that the
speed she runs at came out of the level file, that letting go plays the
run-to-stop animation through, and that holding a turn long enough breaks it
into `STATE_TURN_FAST`. The last of those is the one that catches a rate
converted as a speed when it is an acceleration — the failure that, in the
×15 era, left the cap right, the climb fifteen times too slow, and the state
that waits for the cap unreached.

Another cartridge test resets Lara in a flat four-way sector and drives the
dry-land combinations themselves: back-walk, back-hop, both sidesteps, roll,
and the three directional jumps. It counts completed C ticks rather than TV
fields, because restoring her flat-coloured polygons made a GSU frame more
expensive without changing the moveset. The roll assertion watches its stable
`STATE_ROLL_END`; `STATE_ROLL_START` can be entered and left inside one C
update and is not an observable field-boundary state.

`make check` runs a sixth test that stands next to a medipack in room 23,
holds the whole picture still, checks that it *is* still, and then takes the
medipack -- asserting on the pixels that change. Nothing else can say a sprite
was ever drawn: an entity carrying a model index and a hidden flag proves the
host's half and nothing about the renderer's. The camera has to be given long
enough to settle first, because the follow camera closes an eighth of the gap
per tick and needs a few dozen ticks before it stops creeping — until then it
redraws half the screen for reasons that have nothing to do with the
medipack.

`make check` runs a fifth test that stands the camera on a bridge over the
cistern in room 14 and requires an entity to be holding it up -- the room's own
floor is 800 units below. Room 14 costs half a second a frame, which is why it
waits four hundred fields for one update to come round.

`make check` runs a third test that walks the camera around LEVEL1's own
wiring: onto a pad in room 6 whose one-tick timer opens a dart emitter and
lets it shut again, onto a timerless trigger in room 2 that latches, and up to
the wall switch in room 9, where pulling it has to open the door it is wired
to. Every sector, entity and expected flag in it is a fact about the level
rather than about the port. It is the
only thing that exercises the whole chain the game runs on -- packer resolving
`FloorData`, GSU staging the sector, host reading it, state graph crossing --
and the sector and the entity are facts about LEVEL1 rather than about the
port, so it pins behaviour rather than merely exercising it.

`make check` runs a second test that holds Up for `WALK` fields and then
presses A, asserting that the camera moved along +Z, was never over no floor at
all, is still in a real room, left the ground when told to and came back down.
It is the only thing that exercises pad to C to GSU to collision and back, and
it is slow for the same reason everything is — a second of emulated SNES is a
frame. `WALK=0` skips it.

It finds entities **by what they are and where they stand**, not by their
index in the host's array. Which entities get a slot there depends on what the
packer could carry, and the day pickups gained a model every index past the
first one shifted: the creature test went on running and shot at a bear while
asserting a bat's health.

It finds `src/game.c`'s globals **by name**, out of the symbol file the link
writes. They used to be constants in the runner, and adding one variable to the
C silently moved every one of them: the test went on running, against whatever
had landed at the old offsets, and reported nonsense with confidence.

The host's own variables cannot be found that way — they live in a `.ENUM`,
which hands out addresses positionally and which wlalink puts in no symbol
file. The same trap caught them a second time: adding one word to the direct
page moved `fps` and `fbShow`, and the runner went on reading the old
addresses, which is how a frame-rate counter came back as zero and, worse, how
the frame comparison could have been reading the buffer that was *not* on
screen. The two the runner needs now sit at the top of that `.ENUM` with a
comment saying so, and their offsets are part of the contract.

The jump assertion is written to be rate-independent, because it was not.
A jump is a fixed number of *logic* ticks, and while a logic step was one
rendered frame, how many fields the arc spanned depended on how expensive the
view was. The runner used to hold A for a fixed count and start sampling
afterwards; the day models started drawing, the whole arc happened inside the
hold and the apex read as the ground. Ticks are wall-clocked by NMI now, but
the assertion stays pinned to ticks rather than fields — the shape that
survived both clocks.

`make check TICK=n` and `make sim TICK=n` render animation frame *n*, and must
agree there too. The clock is pinned rather than free-running for the frame
comparison: models choose a pose by masking `ol_ticks`, so a running clock
would put the emulator on whatever frame it reached and the interpreter on
frame zero. It is pinned one *below* the wanted frame, because the host
updates the game and then hands the GSU what the update produced.

**`make check` also asks the cartridge to check its own arithmetic**, which is
the half `fixedcheck` structurally cannot reach. That one builds `src/math.c`
for the desktop, where `int32` is a real 32-bit int; on the cartridge it is
`long long` and `int` is sixteen bits, and a differential test that compiles
the same source cannot see the difference. It hid a `phd_atan` that returned
zero for every vector pointing along -X — for as long as nothing asked one,
which was until a creature had to turn round. `olMathCheck` runs the eight
octants and the ends of the sine table against constants at boot and leaves a
bitmask; the runner requires it to be zero.

`make matcheck` drives `MatrixRotateX/Y/Z` on their own in the interpreter and
compares every element against the same rotation in floating point. A whole
frame is a poor way to find a sign error in one of nine matrix elements: a
wrong column still draws *something*, and something is what a frame comparison
sees. The seed matrix has no symmetry, because a matrix with any is one a
swapped column can survive.

`make animcheck` asks what the frame comparison cannot, from two views: room
14 has fourteen entities and several of one model, which is what the
per-entity slot is for, but their first animations are one frame long so a
frame index is invisible there; room 0 has Lara, whose first animation is a
walk, and nobody else. That comparison stages
one phase per entity and the host computes the same ones, so the two agree
byte for byte whether or not the renderer reads the slot belonging to the
entity it is drawing — read slot zero for everything and both sides would
ignore the index together. So it puts every other entity on a different
animation and requires the picture to change, which is only true if each
entity reads its own slot; changes the animation and the frame separately;
and stages an animation and a frame far past what any model has, to check that
the GSU clamps rather than reading a mesh address out of unrelated bytes. It
also isolates Lara from a background-only render and locks the resulting
front-view mask for two run frames, standing, and rolling. That catches lost
model faces and broken node transforms without making the test sensitive to
the room behind her.

`make check` also asserts what the cartridge is running on: that the emulator
resolved the header to a GSU-2 class die, and that the version code the ROM
itself read out of VCR at boot is not the MARIO chip's. Both are reported in
the log line.

Both currently agree byte for byte on the frame buffer *and* on the tile map
attributes, which is the property to preserve. `FRAMES` has to cover a whole
GSU frame plus its four-field upload; at the present frame rate that is 180
fields. Too small a value fails with `fbShow=0` and most of the frame
differing, which means nothing was ever uploaded — not that the renderer
regressed.

### One test at a time

`make check TESTS=walking` runs only the tests whose name matches, which
turns a two-minute run into fifteen seconds while one of them is red. The
walk/idle profiler is opt-in: `OL_PROFILE=walk make check TESTS=profile`.
FX3 LEVEL1's three-creature profile is instead part of the default suite;
its performance gate is currently red. See "Its gauges".

## What to take from PVSnesLib

Its runtime cannot be used here: it assumes it owns the machine, and its
library and interrupt handlers execute from cartridge ROM, which is exactly
what the GSU forbids while it holds the bus. Its separable parts are useful:

- **SNESMod** (`pvsneslib/snesmod/sm_spc_wla.asm`) — an SPC700 driver that runs
  entirely inside APU RAM and so is indifferent to who owns the cartridge bus.
  Not used: the effects needed a sample player and nothing more, and one
  fits in 483 bytes (`src/spc.asm`). SNESMod is the candidate for music, if
  a tracker rendition of the tracks is ever made.
- **Host tools** — `smconv` (IT module to SPC), `snesbrr` (WAV to BRR),
  `gfx4snes`, `snestools`. All host-side, all directly usable.
- The assembler is the same `wla-dx` family this target already builds with,
  so nothing has to be reconciled.

## What works

- Cartridge header, LoROM + Super FX mapping, 128 KB game pak RAM, checksum
- Boot, WRAM residency, dummy-vector landing pads
- GSU start/stop handshake and STOP → IRQ
- Double-buffered frame buffer, chunked V-Blank DMA, buffer flip
- Joypad read mapped onto OpenLara's `InputKey` bits, OR-latched per field
  and consumed per tick, so a tap shorter than a tick is not lost
- Frame counter and a measured fps figure in `fps`, reported by `make check`
- GSU: frame buffer clear, span fill, flat polygon rasterizer used by
  flat-coloured model faces, and an affine
  textured polygon rasterizer reading 8-bit texel pages straight out of ROM
  through the R14 ROM buffer
- GSU geometry: matrix build from a sine table, 3x3 + translate vertex
  transform, perspective projection, near-plane clipping, 32-bit backface
  culling, depth buckets, face assembly and dispatch
- **The whole level in ROM, and a portal walk over it.** 38 rooms, a room
  table the GSU indexes, per-room transforms, portal visibility with screen
  boxes, and a clip rectangle narrowed at every doorway. See "Rooms and
  portals"
- The shade term, behind `SHADING=1` and off by default: per-vertex `g` from
  the level's own lighting, interpolated down both edges, across each span and
  through the clipper, quantised to eight CG-RAM rows per 8x8 character and
  uploaded with the frame. See "Shading"
- SNES level/texture packer output for textured quads and triangles, using the
  original PC level data supplied through `TRDATA`
- **Walls that stop her, ledges she falls off, slopes she slides down.**
  Upstream's `CollisionInfo`, `collideRoom` and the `c_` handlers of every
  ported state, in `src/collision.c`. See "Her collision"
- **Fixed 30 Hz game time**, NMI-queued and independent of the renderer: the
  backlog capped, the RAM-bus owner correct across an NMI landing mid-update,
  and upstream's constants at their own per-tick values -- and measured:
  ticks per second, dropped ticks and a 65816 profiler. See "The clock"
- **`Room_query`: the level answered host-side.** Every sector's floor,
  ceiling, vertical neighbours, portal, trigger and slants ride in the staged
  blob, and game logic asks about arbitrary points as often as it likes —
  portal exhaustion, `roomBelow`/`roomAbove` descent, point-exact slopes,
  upstream's `getSector` shape. The GSU probe now serves the renderer alone
- **Static meshes, models and animation.** Scenery through the matrix stack;
  entities through a flattened model stream, with the matrix stack doing
  push/pop per node. Every model plays its own first animation, on a phase the
  host owns per entity. Lara and the level's 60 entities are on screen and
  moving. See "Models"
- **The level's entity list on the host**, staged by the GSU and parsed by the
  C: type, room, model, world position and angle for all 60 of LEVEL1's
  entities, plus a channel back the other way carrying an animation and a
  frame per entity
- **Every animation in the level**, 3901 frames across 42 models plus the two
  the packer synthesises for sprites, walked
  per entity along TR's own `nextAnim` chain, with the state graph -- 249
  transitions -- deciding when to leave it. See "Walking the animation graph"
- **Triggers, with TR's activation mask and timer.** Standing on a sector
  activates what TR says it activates, for as long as TR says: the packer
  resolves `FloorData` into a per-sector trigger index carried in the host
  sector table, and the host applies the mask, the one-shot bit and the
  countdown on the tick clock. See "Triggers"
- **Animation commands**: entities move, turn and switch themselves off where
  TR says they do
- **A third-person camera**, ported from `camera.h`: she is driven and the
  view follows her. See "The camera follows her"
- **A playable 41-state Lara slice**, ported from `lara.h`: stand, walk,
  run, turns, back-walk/hop, sidesteps, roll, directional jumps, fall,
  land and death; hang and climb; and the water -- the fall in, the dive,
  the swim, the surface and the climb out -- driven through TR's own
  transition table rather than by assignment, and moving at the speed and
  direction her state and *animation* carry. See "Lara"
- **The active list**, ported from `item.h`: `activate`, `deactivate`, and an
  update sweep that walks only what is on it. `isActive` is upstream's test of
  the trigger *mask* rather than of the flag, which is what makes an antipad
  switch something off without taking it off the list
- **A floor that lets go, and lands.** `TrapFloor` notices her standing on
  it, shakes, falls under `ItemObj::move`'s gravity and settles on the pit
  floor `ItemObj_updateRoom` asks for. LEVEL1 has two, in room 32
- **Lara as an item**, drawn, and **the things she can use**: switches
  she pulls, pickups she bends for, keys and puzzle pieces she carries to
  their holes, blocks she takes hold of and pushes; upstream's limit
  boxes, `animSkip` and `moveTo`, her hands freed by the animation. See
  "Using things"
- **Activation as TR defines it**: the item record's mask and hidden bit
  read (`ITEM_FLAG_REVERSE`, INVISIBLE), pads that fire only under her
  feet, switch, key and pickup triggers that ask before they fire, END and
  SECRET actions, and creatures woken by the triggers that name them. The
  KILL command leaves the status INACTIVE as upstream's does. See
  "Triggers"
- **Solid doors.** An entity carries its model's bounding box and a shut door
  blocks the way through; open it and the same spot is walkable
- **Bridges you can stand on**, flat and tilted, which is upstream's
  `getBridgeFloor` -- an entity raising the floor the GSU staged -- and
  trapdoors the same way, shut
- **A block that moves the floor with it**: the host sector table
  rewritten in game pak RAM, upstream's `updateFloor`
- **Creatures that decide**: upstream's moods -- asleep, stalking, attacking,
  escaping -- off upstream's PRNG, over shortest paths through TR's
  navigation boxes. They bite, they take damage, and they die. See
  "Creatures"
- **Sprite pickups.** TR's medipacks, keys and ammunition have no model at
  all, and everything downstream is indexed by one; the packer synthesises a
  billboard quad, a node, an animation and a frame per sprite type, and the
  renderer drops the rotation out of the matrix for it. Seven entities of
  LEVEL1 that were not there before. Stand at one with Action and she
  bends for it, which is upstream's `Pickup::collide`; the bag it goes
  into is a count per type until there is a screen. See "Sprites" and
  "Using things"
- **Sound effects.** A 483-byte SPC700 driver and the level's samples, chosen
  and BRR-encoded by the packer to fit 64 KB of APU RAM, uploaded at boot
  through the IPL protocol; upstream's `soundPlay` -- the level's soundMap
  and SoundInfo, the chance roll, distance from the camera target, random
  gain, pitch and variant -- natively on the 65816; the S-DSP mixes eight
  voices in hardware. SOUND animation commands, the pistol shot, a wolf or
  bear flinching, the scream, and a key that does not fit. See "Sound"
- **64 KB of resident C**, in two halves of WRAM bank $7F fed by two DMAs from
  two ROM banks. The 32 KB the port had was one LoROM bank, not a limit of
  the machine
- **The staged level read where it lies**, in game pak RAM, rather than copied
  into WRAM: 25 KB of `.bss` where it was 50
- C compiled with `816-tcc`, resident in WRAM, called thirty times a second
  while the GSU is mid-render — `src/game.c` owns the pad handling and the
  camera, and is the seam the rest of the engine grows through

## Gaps against the GBA port

The GBA target is the specification, so "what is missing" is best read as a
diff against it. Grouped by subsystem, with the GBA source that defines each
one, because that is what a port of it has to read.

### Renderer

| | GBA | SNES | |
|---|---|---|---|
| flat spans | `rasterizeF` | `RasterF`, model faces | rooms remain all textured |
| affine textured | `rasterizeFT` | `RasterFT` | done |
| Gouraud textured | `rasterizeGT` | per span into the pixel's high nibble | 8 rows against 32, per span (four pixels) against per pixel |
| colour-keyed textured | `rasterizeFTA`, `rasterizeGTA` | `RasterFTA`, including shaded spans | source-zero holes preserved; 4bpp foreground/background still share a tile palette |
| sprites | `rasterizeS`, `renderSprite` | colour-keyed pickup billboard models | no general sprite path; effects and HUD sprites are absent |
| glyphs and text | `renderGlyph`, `drawText` | FX3: TR's font through the GSU (`TextRender`), the inventory's strings | no menus, no subtitles; GSU-2 none |
| bars | `renderBar` | the HUD's sprites | health and air; the inventory's health bar (FX3) |
| background images | `renderBackground` | FX3: the inventory's grey copy of the game frame (`BgCapture`); the title screen (`TITLE.BIN`, `CopyTitle`) | credits |
| shadow blob | `renderShadow` | FX3: hers and the creatures' (`ShadowRender`) | the swinging blade's and darts'; GSU-2 none |
| static meshes | `faceAddMesh*`, `renderMesh` | scenery, through the room pipeline | **done** |
| models | `drawNodes`, `matrixFrame` | flattened node stream, `DrawModel` | **done** |
| animation | `animProcess`, `matrixFrameLerp` | every animation, per-entity, states, ranges and commands | no lerp; OFFSET, JUMP, KILL, SOUND, EMPTY and the effects act |
| mesh swap | `ItemObj::meshSwap` | applied once, in the packer | Lara's arms only, holstered pose |
| dynamic vertex lighting | `calcLightingDynamic/Static` | — | items lit by nearby lights |
| underwater transform | `transformRoomUW` | — | wave distortion and caustics |
| animated textures | `animTexturesShift` | — | water, lava |
| distance fog | `vg += fog` in `transformRoom` | — | cheap here: camera z is already to hand |
| viewport clipping | `setViewport` per portal | per-room clip rectangle | **done** |
| depth sort | one ordering table over everything | 64 buckets per room, traversal order between them | approximate; 576 faces and 768 vertices per room, with future overflows truncated by the packer |
| matrix stack | `matrixPush`/`matrixPop` | push, pop, translate-relative, rotate X/Y/Z | **done**, 8 levels, bounds checked |

### World

| | GBA source | |
|---|---|---|
| multiple rooms and portal traversal | `drawRooms`, `Room::checkPortal` | **done** |
| sector floor and ceiling heights | `Sector::getFloor` | **done** |
| floor data — triggers | `checkTrigger` | **done**, resolved at pack time |
| floor data — slopes and vertical portals | `room.h` | **done**, host-side: `Room_query` exhausts portals, descends `roomBelow`/`roomAbove` and evaluates slants point-exact |
| floor data — climbable walls, lava and material flags | `room.h` | — |
| room static meshes, drawn | `renderMesh` | **done** |
| their collision boxes | `Room::collideStatic` | **done**, the plants excepted as upstream excepts them |
| entity collision boxes | `ItemObj::collide` | doors as one box per model, solid while shut; switches, pickups, holes and blocks through their own `collide` | creatures' pushes, the rest of the classes |
| entity floors | `getItemFloorCeiling` | bridges, flat and tilted; trapdoors; blocks by the sector rewrite; FX3 drawbridges, including cardinal extents and signed seams | GSU-2 drawbridges and the falling-floor ceiling rule |
| creature behaviour | `enemy.h`, 40 KB | original wolf/bear/bat state graphs, animation motion, attacks and deaths; species ground limits; FX3 original moods/aggression, raptor and inherited lion/lioness/puma behavior | GSU-2 aggression/waypoints, other TR1 creatures, hit spheres and head tracking |
| Lara's weapons | `lara.h` weapon state | pistol draw/holster, independent arm poses and firing, torso/head tracking, range/aim limits and room visibility | no other weapons, spread, ammunition or flash |
| navigation | `nav.h`, 10 KB | exact sector IDs, native room-aware lookup, boxes and overlaps; FX3 three-context incremental Nav, corridor waypoints and all six ground-1/ground-2/fly maps with ordered normal/alternate initialization union | GSU-2 shared simplified flood; actual room flips and water-creature controllers/movement |
| room sprites, room lights | `out_GBA.h` room info | — |
| alternate rooms (flip maps) | `Room::modify` | — |
| more than one level, and switching between them | `startLevel`, `osLoadLevel` | — |
| room ambient light and light sources | `out_GBA.h` room info | — |

The question sectors raised — where does game logic get level data, when the
host cannot read ROM? — is answered and built: see "What the GSU tells the
host". `FloorData` turned out not to need walking on the cartridge at all: it
is a variable-length command list, so the packer walks it once and a sector
carries the answer. Triggers, slopes and vertical portals came across that
way; climbable-wall, lava and material flags remain.

### Runtime and game C rewrite

This target deliberately does not compile `src/fixed`: `816-tcc` cannot
compile the C++ and its native `int` and `long` are both 16 bits. The SNES
side is a semantic rewrite into small C modules for the 65816 and assembly for
the GSU. Host-build oracles catch algorithm drift, while cartridge tests are
still required for width, bank and bus-ownership bugs.

The rewrite is no longer just a movement demo, but it remains a narrow slice:

| subsystem | present on SNES | gap to `src/fixed` / GBA |
|---|---|---|
| main loop | fixed 30 Hz ticks, NMI-queued, input latched per field, ticks and drops counted, a profiler | overrun still dilates game time behind the capped backlog, now visibly; PAL would tick at 25 |
| Lara | dry-land movement and collision; hang/climb/handstand; swan/fast dives; water entry/swim/surface/climb-out/drown; pickup/switch/key/puzzle/block interactions; her pitch drawn (GS_ITEM byte 11); FX3: her lean (angle.z, GS_ROLL) | pose/special, reachable air-roll behavior, wading (TR2's); GSU-2's lean |
| items | animation transitions, active list, movement/gravity, basic box push, bridge and trapdoor floors, `animSkip`, `moveTo`, `animIsEnd`, the level's activation flags; drawn in the room the host has them in (GS_ITEM byte 10, the GSU's slot table) | no interpolation, joints/spheres, general room/entity collision, host-side room relinking for non-creatures, dynamic add/remove, runtime mesh swaps; serialization FX3 only (`src/save.c`) |
| objects | doors (FX3: walls in the sectors while shut, upstream's `Door::action`; GSU-2: a box), switches pulled, pickups bent for and counted by type, keyholes and puzzle holes, blocks pushed and pulled with the floor rewritten, trapdoors, bridges and the falling floor; FX3: drawbridges, dart emitters and darts, swinging blades cutting on upstream's spheres (`src/trap.c`, `src/trap_fx3.asm`) | gears, boulders, the other traps, lava, effects; GSU-2's drawbridges, traps and inventory screen; the DONE puzzle hole drawn |
| weapons | pistol draw/holster, independent arm poses/firing and mesh swaps, torso/head tracking, current-frame creature aim points, torso-relative acquisition, target lock and visibility; FX3: all four guns from the ring, the spread, ricochets, the muzzle flash, the shotgun's six pellets | sphere hits (a pistol-class shot at an aimed target always hits; pellets test the frame box); GSU-2's other guns |
| enemies | original wolf/bear/bat decisions, animation-driven motion/attacks/deaths; species ground limits; FX3 original moods and per-creature navigation, raptor and GBA's inherited lion/lioness/puma behavior | other TR1 creatures; GSU-2 aggression/waypoints, head tracking, hit spheres and flying-enemy collisions |
| camera | clipped follow camera: `getBestLocation`, `trace`, `clip`, exact against upstream; combat and look modes; every state's framing (targetAngle/targetDist, `center` on FX3); FX3: fixed cameras, camera targets and her look at them (`src/camera_fix.c`) | free, object and cinematic modes; GSU-2's fixed cameras |
| triggers | pad on the floor, switch/key/pickup asking their object, antipad, masks, timers/once, ACTIVATE_OBJECT, END and SECRET actions; FX3: ACTIVATE_CAMERA, CAMERA_TARGET, READY-gated COMBAT and landed-block OBJECT | soundtrack, heavy creature triggers, GSU-2 COMBAT/OBJECT (flip maps, FLOW and EFFECT are TODO upstream too) |
| game shell | health and air bars; Select after death restarts the level (GSU-2); FX3: the inventory ring -- main, item, options and death pages, medipacks and pistols used from it, keys and pieces offered to holes, the passport (Load Game starts a level or loads Current Position, Save Game into the battery RAM), TR's text and the grey background; the title page at power-on, LEVEL1 to LEVEL2 to the end page and back to the title; the settings pages (gamma, the frame counter, SFX, A/B swap); the ring's sounds and the passport's page mask | cutscenes, the full level order, the gym (no ROM: 4bpp pages first); GSU-2's save/load, title and settings |

The room query stopped being the shared dependency: `Room_query` serves
arbitrary points at 30 Hz, host-side, which is what camera clipping, Lara's
collision handlers and creature collision were all waiting on. What gates
them now is the porting itself — and the tick budget, since every handler
that comes across runs thirty times a second on a 3.58 MHz processor that
takes the RAM bus from the renderer to do it.

### Audio

Sound effects are in -- see "Sound" -- with the choices that section lists:
a per-level subset of TR's samples at 7350 or 5512 Hz, cut at a second, in
mono; no music. The GBA side is `sound.cpp`: ADPCM sample decode, a software
mixer, `sndPlayTrack`, `sndPlaySample`. What remains against it:

| | GBA | SNES |
|---|---|---|
| `sndPlaySample` | six software channels, samples read from ROM at 22 kHz | the S-DSP's eight voices; `src/spc.asm` finds the free one, or the one already playing the sample for UNIQUE/REPLAY |
| `soundPlay` | chance, distance, gain, pitch, variant | the same, natively (`src/sound.asm`); `rand_draw` is a 16-bit xorshift rather than upstream's LCG |
| `sndPlayTrack` | the ambient track and the cutscene music as compressed samples | — |
| `soundSources` | room ambience | — |
| the underwater sound, `SND_BUBBLE` | | — the water breath object and the loop are not carried |
| inventory sounds | | — the APU image has no room; they are the first the plan would drop |
| positional pan | mono, as here | the DSP has per-voice L/R for free; the camera yaw is to hand |
| every sample | | the packer keeps what fits: 32 of LEVEL1's 52 firable ids |

### Data pipeline

`packer/out_SNES.h` emits a palette, texture pages, and a level of room
geometry, portals, sectors, static mesh placements, models and entity
placements. `models`, `nodes` and `frameData` do not survive as three tables:
they are flattened into one node stream per model, which is the shape the
GSU's ROM buffer wants. See "Models". The GBA packer emits, additionally:

| | for |
|---|---|
| `floors` | done for geometry: triggers, slants and vertical connections ride per sector in the host table, the water flag per room; material and climb flags do not yet |
| `commands` | done: OFFSET, JUMP, KILL, SOUND, EMPTY and the effects act |
| `items` in TR's own shape | partly done: type, room, model, position and angle reach the host; flags and intensity do not |
| `objectTextures`, `spriteTextures`, `spriteSequences` | sprites, and UVs by reference rather than inlined |
| zones for flip state 1 | all six normal/alternate ground-1/ground-2/fly maps are carried and selected on FX3; GSU-2 keeps normal ground-1/fly only |
| `cameras`, `cameraFrames` | fixed cameras and cutscenes |
| `soundMap`, `soundInfos`, `soundData`, `soundOffsets` | done, as a subset: `SNDTAB.BIN` and the BRR image in `SOUND.BIN`, see "Sound"; `soundSources` not yet |
| `animTexData` | water and lava |
| room `ambient`, `lights`, `sprites`, `alternateRoom` | lighting, pickups, flip maps |

Every one of those is a prerequisite for the corresponding runtime feature.
The lightmap is the one exception: it is folded into the eight CG-RAM palette
rows at pack time rather than shipped.

Two sizing problems come with it:

- **Textures are 8bpp for a 16-colour target.** 11 tiles of LEVEL1 occupy
  704 KB, one 64 KB GSU bank each, and every texel holds a colour nibble plus
  the source-zero marker. 4bpp colour plus a separate mask would reduce it.
  The cost is that `(v << 8) | u` — one `MERGE` — stops being
  the texel address, so the inner loop grows a shift and a mask, or the pages
  get packed as two 4bpp halves per bank.
- **The 2 MB wall.** That is a GSU-2's entire ROM addressing range, and there
  is no wider part to move to — see "Which Super FX". LEVEL1 is 1.13 MB with
  its geometry and textures in, so one level fits comfortably and a second
  would not: 704 KB of that is texture pages at 8bpp for a 16-colour target.
  4bpp halves them. A multi-level cartridge needs that and real compression.

### The structural one: where the game code goes

Worth stating separately because it constrains **GSU-2**, not FX3. Nothing can
execute from cartridge ROM while GSU-2 holds that bus, so its active host code
is resident in WRAM. FX3 uses the FastROM layout documented at the top.

| | |
|---|---|
| `$7E:0110-$05FF` | host assembly, 1,102 bytes; 162 bytes free |
| `$7E:0600-$1BFF` | C/native code, ROM bank $3D; 64 bytes free |
| `$7E:1C00-$1FFF` | the stack, 1 KB: measured 421 bytes deep over a long scene, and the runner watches the floor |
| `$7E:2000-$57FF` | C/native `.bss`, 14,084 bytes used of 14,336; 252 free |
| `$7E:5800-$7FFF` | C/native code, ROM bank $3E; 1,199 free |
| `$7E:8000-$FFFF` | C/native code, ROM bank $3F, including sector decoder/cache and shared flood; 2,282 free |
| `$7F:0000-$7FFF` | C/native code, ROM bank 4, including target info, wolf update and the tcc runtime; 2,165 free |
| `$7F:8000-$FFFF` | C/native code, ROM bank 5; 320 free |

`make` prints what each holds and what is free after every link
(`tools/budget.py`), and fails the build if the host assembly crosses $0600
or any unit has initialised data -- see "Toolchain traps".

The 32 KB this said until recently was **one LoROM bank**, not a limit of the
machine, and the 64 KB after that was one WRAM bank. A unit is assigned to a
ROM bank and a WRAM bank in the Makefile, `tools/tccfix.py` derives the
`.BASE` that makes its long addresses resolve there, and the boot code DMAs
each image into place; the third one is fed from the top of the cartridge,
where neither the level nor the texture pages can grow into it, and confining
`.bss` to the lower half of `$7E` (SLOT 2 in `hdr.asm`) is what makes room
for it. The water slice (2026-09-04) cost 9.6 KB for some five hundred
lines of C, this compiler's rate, and left 5.7 KB free of the 96.

The same evening two more regions were found in `$7E` without moving a
byte of data. The `.bss` had 9.6 KB of headroom under `$8000`: 6 KB of it
is a code image now, `$6800-$7FFF` from ROM bank `$3E`, and the `.bss`
keeps `$2000-$67FF` with 3.5 KB to grow. And the stack had `$1000-$1FFF`
on no evidence: painted with a marker and run through the runner's
busiest scene, it reached 421 bytes deep, so it keeps `$1C00-$1FFF` and
`$1000-$1BFF` is a 3 KB image from ROM bank `$3D`, under it in the mirror
where the host assembly already runs. Two things about them. A wla slot
cannot be narrower than its 32 KB ROM bank, so the linker does not know
either limit; `tools/budget.py` reads every section's start and end out of
the symbol file and fails the build when a unit in either region ends past
it, since an overflow would land on the image above or on the stack and
nothing else would say so. And the runner asserts the sixty-four bytes
above the stack floor are still zero at the end of the boot and walking
scenes. `enemy.c` went to the first region -- the unit that will grow most
when the creatures come class by class, and the region a per-level creature
page would be paged into -- and `common.c` to the second. 14.8 KB of
resident C was free then, in five pieces of 0.9 to 5.1 KB.

The interaction slice (later the same day) spent it. `use.c` came out at
5.2 KB and `block.c` at 3.1, and item.c grew 3.6 KB for three helpers --
this compiler's rate again -- and no piece held either new unit. The
region over the `.bss` grew instead: the navigation flood kept a queue
per zone kind, 2 KB each, only so the next flood of that kind could clear
what the last one wrote, and a bit per box does the same in 256 bytes;
the `.bss` cap came down to `$5800` (SLOT 2 in `hdr.asm`, the DMA in
`main.asm`, `tools/budget.py`), and the region is 10 KB, holding
`enemy.c` and `use.c`. `budget.py` also reads the Makefile now for which
units those regions hold and refuses any of their sections wla placed
outside them at all -- it used to look only at sections that started
inside, and a unit that did not fit linked fine and landed on the image
above. That left 5.8 KB of resident C free, in five pieces of 0.4 to 1.9 KB, and
the `.bss` has 1.1 KB.

The pistol-core slice replaced the compiled trig/sqrt helpers with 265 bytes
of native code and moved the cold boot checks to bank `$3F`. This recovered
enough contiguous room for targeting and the shared visibility checks.
Those additions left 4,014 bytes of resident code free. The aiming slice
swaps `level.c` and `block.c` between banks 4/5 to make room for the rotated
aim point. That left 2,759 bytes after bank packing. The target-lock slice
replaces 1,622 bytes of compiled aim-point code with 338 native bytes, which
funded tracking with 3,111 bytes free. The pistol animation controller,
armed turning, climbing hand states and combat trace optimizations left
2,042 bytes. Torso/head tracking and torso-relative acquisition left 1,798.
Current-frame creature aiming left 1,684 bytes free. The dive/handstand slice
extended the low code image to `$0600` and moved the math tables there,
leaving 3,407. Native math followed its tables and `Room_pointOpen` moved to
the region above `.bss`, allowing the bat controller to fit with 655 bytes
remaining. Native item movement and bounding-box copying replace 2,311 bytes
of compiled C with 358 bytes of assembly, leaving 2,591 bytes at that point.
The wolf slice replaces 2,056 bytes of animation advancement/transition C
with 439 native bytes, and 1,797 bytes of wolf decisions with 555. The
1,443-byte arithmetic self-test now stays in ROM bank `$3C`; its sole caller
is `gameInit`, before the GSU starts. The wolf's C motion/targeting and shared
ground collision fit with **553 resident bytes free**, in pieces of
81 / 9 / 335 / 43 / 85. Pending-state/turn arrays leave 857 BSS bytes free.
The stack and data-region boundaries are unchanged.

Exact-sector navigation replaces the geometric scan and its 512-byte
per-item cache with a 308-byte packed-map reader. `Room_sectorAt` and
`navBoxAt` share the existing native room cache and sector multiply. The
result leaves **1,794 resident bytes free**, in pieces of 81 / 450 / 335 /
801 / 127, and **1,369 BSS bytes**. No bank or stack boundary moved.

The subsequent bear slice removed the reduced controller and now-unused
species-table fields. Native state/update routines use 725/603 bytes, fitting
both bear death routes and attack latches without any larger data/code slot.
GSU-2 then had **1,816 resident bytes free**, in pieces of
81 / 450 / 335 / 757 / 193. The current layout is in the table above.

`make motioncheck` builds the original C implementations with `C_MOTION=1`
and compares their outputs, including full signed ramps, gravity thresholds,
wrapped positions, transition boundaries, animation commands and wolf/bear state
decisions. It also checks 512 complete bear updates, with an additional host
oracle compiling the original C++ Bear method. Reference C lives in ROM bank `$3C`, so that cartridge is a
CPU-only test oracle, not a playable GSU-2 build.

Larger slices still need code-size work or paging: stop the GSU, DMA the next code
bank from ROM into a region, restart it. Cheap in isolation and expensive
per frame, so the partitioning wants deciding before the porting starts:
by level, since which creatures and objects exist is what changes between
levels, and the 10 KB region over the `.bss` is the natural first page.

## What to tackle next

The order is dependency-driven. The two that used to gate everything — the
fixed clock and the arbitrary-point room query — are built; see "The clock"
and "What the GSU tells the host". What follows is what they were for.

0. ~~**Instrument the clock.**~~ Done: `ol_tps`, `ol_ticksDropped`, the
   profiler, and the assertions. See "Its gauges". Everything below adds tick
   cost, and the walking scene has about 0.8 of a field to give.

1. **Finish the sector flags.** `roomBelow`/`roomAbove`, slants and the
   trigger are carried, and so is the room's water flag now (bit 7 of the
   width byte in the host room record; `Room_water` in `room.c` walks the
   column for the surface and the depth). Material and climbable flags are
   not, and go in the link record, where a byte costs 583 rather than
   4,026.

2. ~~**Port `CollisionInfo` and Lara's collision handlers.**~~ Done for the
   dry-land states, for hang and climb, and for the water: see "Her
   collision" and "The water". Wading is TR2's and stays out.

3. **Complete the camera.** ~~Implement `getBestLocation`~~ done; then look,
   combat, fixed/object and cinematic modes using the same arbitrary-point
   query service.

4. **Complete items, objects and inventory together.** ~~Keys, puzzles,
   blocks and the actual pickup interaction~~ are in, with the activation
   flags and the trigger types, and on FX3 the inventory screen (README
   "Inventory"); general entity collision, joints/spheres and room
   relinking remain, and the traps that are not open-and-shut.

5. **Add weapons and the remaining creatures.** Follow the wolf/bear/bat
   porting pattern: preserve each upstream state table, attack geometry,
   animation graph and random-roll order, with original-method oracles.

6. **Fill renderer/data gaps.** Colour-keyed polygons, generic sprites, HUD
   primitives, animation interpolation, dynamic lighting, animated textures,
   flip maps and full trigger actions. Replace hard room caps with a checked
   split/streaming policy before packing other levels.

7. **Make levels fit.** Convert texture storage to 4bpp, add compression and
   multi-level loading. GSU-2 has 1,816 resident-code bytes left, in pieces
   of at most 757 bytes; larger controllers still need paging or code-size
   work. FX3 executes from FastROM and now has independently banked CPU
   tables; geometry/texture packing, individual 16-bit table indices and
   multi-level loading still need work.

8. ~~**Add SPC700 audio**~~ Sound effects are in; music and the trigger
   soundtracks remain, and they are a data question first (see "Sound").

9. **Optimize the GSU renderer after correctness.** Room clip rectangles
   should reject vertex transforms earlier; profiles, simulator/frame diffs
   and real-emulator timing should continue to gate each change.

## Files

| | |
|---|---|
| `main.asm` | 65816 host: boot, PPU, DMA, input, GSU handoff, C entry, shade map seed and upload |
| `hdr.asm` | shared memory map; included by every object, compiler output too |
| `snes.inc` | hardware registers, frame configuration |
| `src/game.c` | camera, movement and collision, compiled to the 65816, resident in WRAM |
| `src/fixed.h` | the port of src/fixed: types, conventions, prototypes |
| `src/lara.c` | 41 state handlers ported from `lara.h` -- land, air, hang and climb, the water -- and `updateWaterState` |
| `src/collision.c` | `CollisionInfo`, `collideRoom` and the `c_` collision handlers, from `item.h` and `lara.h` |
| `src/object.c` | the class of a type, `Door`, `Switch`, `TrapFloor`, the dispatch of `ItemObj::update`, from `object.h` |
| `src/use.c` | the `collide` methods of the switch, the pickup and the holes, upstream's limit boxes, the inventory counts, `updateObjectsCollision` |
| `src/inventory.c` | FX3: upstream's inventory state machine -- slots, pages, states, counts, open/close, use, the USE page's hand-over to a hole |
| `src/inventory_draw.c` | FX3: the ring's matrices (`drawPage`/`drawSlot`) for the GSU's `InvRender` |
| `src/inventory_text.c` | FX3: `drawText` and the inventory's strings, as glyph blocks for the GSU's `TextRender` |
| `src/block.c` | `Block`: the sector rewrite, the obstacle checks, taking hold, the push and the pull |
| `src/room.c` | `checkTrigger` and `fireTrigger` over the packer's trigger table, the water column |
| `src/room.asm` | `Room_query` in 65816 assembly: the sector and link records, the portal walks, the slants; and `olStaticHit`, the static mesh scan |
| `tools/framecheck.py`, `frame*.baseline` | each frame format's opening-view cost, held against its record by `make check` |
| `tools/viewcheck.py` | eight views modelled on two ROMs and compared byte for byte: 4bpp by default, `--mode7` for the optional frame |
| `tools/keycheck.py`, `tools/typesnes_key.ts` | independent keyed/opaque pixel and face-dispatch oracles in both GSU cores, `make keycheck` |
| `tools/budget.py` | resolved-layout WRAM/FastROM budgets after every link; fails on initialised C data |
| `HOST_TABLES.md`, `tools/hosttables.py`, `src/host_tables.asm` | FX3's independently banked ROM directory, validation, pointer binding and mutable-sector DMA |
| `src/nav_fx3.c` / `.asm`, `src/enemy_nav.c` | FX3's independent Nav contexts, native hot paths and original moods |
| `src/enemy_move.c` / `.asm` | animation-driven ground movement; FX3 native implementation with C oracle |
| `src/bear.c`, `src/bear_state.asm`, `src/bear_update.asm` | original bear state/attack/death decisions and ordered updates, native on both targets with complete C references |
| `src/collision_fx3.asm` | FX3 native rotated item contact, full room-collision wrapper/probes, bridge/door filtering and side setup |
| `src/render_sync.asm` | FX3 native dirty/active/hidden/frozen render snapshots into the existing WRAM staging buffer |
| `tools/mkenemystate.py`, `tools/enemycheck.cpp` | extract and execute the actual shared C++ creature state method as a host oracle |
| `src/math.c`, `src/math.asm` | C reference and native trig/square root; shared tables and PRNG |
| `src/mathcheck.c` | boot-time math checks through the compiled C calling convention |
| `src/weapon.c`, `src/weapon_anim.c` / `.asm` | pistol draw/holster, independent arm animation, targeting and damage |
| `tools/mktables.py` | their tables, sized for WRAM rather than for ROM |
| `tools/fixedcheck.c` | the differential oracle for src/math.c, `make fixedcheck` |
| `tools/matcheck.py` | the matrix rotations against floating point, `make matcheck` |
| `tools/animcheck.py` | per-entity animation and Lara pose masks, `make animcheck` |
| `src/libtcc.asm` | 816-tcc runtime helpers; WRAM on GSU-2, FastROM and multiply shortcuts on FX3 |
| `tools/tccfix.py` | WRAM/FastROM placement and FX3's flag-preserving sign-extension rewrite |
| `gsu/frame.gsu` | GSU frame entry, buffer clear, room dispatch |
| `gsu/raster.gsu` | flat and textured span rasterizers, shade pass, reciprocal table |
| `gsu/mesh.gsu` | room table, portal walk, static meshes, models, matrix stack, transform, projection, culling, face setup |
| `tools/mkmodel.py` | sine table and the placeholder cube |
| `tools/mktexture.py` | placeholder texture page |
| `gsu/gsu.inc` | GSU register contract, call/return and far-jump macros, `GBLT`/`GBGE` |
| `tools/gsusim.py` | Super FX interpreter, cycle and cache model, profiler |
| `tools/typesnes_dump.ts` | headless whole-machine runner for `make check` |
| `tools/snesfix.py` | header checksum |
| `tools/mkdivtable.py` | reciprocal table |
| `packer/out_SNES.h` | level conversion: palette rows, texture pages, the room table, every room, and the model streams. `OL_DUMP_ITEMS`, `OL_DUMP_TRIGGERS`, `OL_DUMP_SECTORS=<room>`, `OL_DUMP_ROOMS`, `OL_DUMP_ANIMS`, `OL_DUMP_STATICS` print what a runner scene needs to know |
| `tools/mkbanks.py` | drops the level and the texture pages on GSU bank boundaries |

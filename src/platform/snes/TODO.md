# TODO — GBA parity and the SNES release

The GBA target is the specification (`src/fixed`, `src/platform/gba`). The
current release scope is its first two levels, with the same rules,
creatures, objects, sound, inventory and save behavior. The longer backlog
below also records full-campaign and other-target work; those entries are
not requirements for this release. This is the list, in dependency order,
with what each item costs
against the four budgets that gate everything here. Re-verified 2026-09-01
after the clock gauges, the third code bank and Lara's collision went in,
and again the same day after §0's three items went in -- see "Done: §0" at
the bottom, above the bug hunt that preceded it -- and on 2026-09-04 after
the water slice, which is what moved the resident-C row to the top of the
worries, and once more the same evening after the interaction slice, which
spent it. The pistol-core slice below also audits native math and rebalances
the boot checks to fit the new code. The 2026-09-05 aiming slice ports rotated
box centres and angle-priority acquisition, swapping the level/block banks.
The following target-lock slice adds per-tick retention and arm aim limits,
funded by native aim-point arithmetic.

## Current release scope: polish the GBA alpha's first two levels

The requested release is **LEVEL1 + LEVEL2 with GBA target parity**. Keep
improving framerate and polish the existing game. Full-campaign content,
GYM, later-level creatures and cutscenes are outside this release scope.
The release cartridge is the FX3 multi-level build, `build_fx3_ship`.

- [x] Run the complete gameplay suite at the $5546 checkpoint: 117 distinct
      LEVEL1 checks (normal suite plus eight native/C comparisons) and 71
      LEVEL2 checks. Includes both water switches and all three corpse types.
      Fix test bookkeeping without weakening gameplay expectations.
      The renderer-only $2DE4 follow-up passes 32 selected LEVEL1 checks
      and the complete 71-check LEVEL2 suite, plus the rotation/input oracles.
- [x] Verify title/start, both END transitions, passport/death navigation,
      save/load across levels and power cycles, settings, sound and HUD.
- [x] Improve measured throughput from the $A071 single-level baseline:
      mixed encounters 9.5 → 9.6, 8.67 → 8.80 and 11.38 → 11.48 fps in the
      final corpse build, with zero dropped ticks and 301 input snapshots
      exact. The long mixed frame gap falls 23 → 21 fields. The wolf
      scene's worst gap rises 13 → 17 fields; further smoothing remains.
- [x] Audit first-two-level GBA gaps; keep the remaining release work below.
- [x] Review placed room/model bounds and stack use against actual two-level
      data; pass the standard rendering oracles. Room faces peak at 546/576,
      vertices 620/768; matrix stack including muzzle flash at 5/8. This
      does not cover inventory-only models or all clipping/rasterizer paths.
- [x] Produce a reproducible two-level candidate, accurate release notes,
      complete validation evidence and explicit outstanding hardware checks.
- [ ] Verify the candidate on FX3 hardware before calling it hardware-qualified.

### Remaining first-two-level parity and polish

- [x] Match GBA's ten-second corpse expiry. Preserve death animation,
      navigation release, active-list and save behavior. Reuse the item
      timer without extra RAM, protect it from repeated pads and apply the
      original activation-status guard so triggers cannot reveal corpses.
      All three creature types pass on both levels, including a bear saved
      mid-death; the real combat replay verifies the bat's rendered removal.
- [x] Match GBA frame-interpolated camera bounds on FX3: follow height, fixed
      and look targets, and X/Z centering during pushes and climbs. Store
      exact offsets in packed ROM streams with no added RAM.
- [ ] Add animation pose interpolation; joint matrices still use sampled poses.
- [ ] Close the remaining lighting, animated-water/distortion and music/audio
      coverage gaps already itemized below, limited to LEVEL1 and LEVEL2.
- [ ] Further reduce long visible-frame pauses without dropping game ticks
      or weakening input/graphics checks. The eight-field publication bound
      reduces the long mixed gap 23 → 17 fields while retaining 11.53 fps,
      zero drops and maximum logic lag five. Wolf gaps fall 16 → 15 fields,
      with two more flips in 1,800 fields. A 17-field pause is still visible.
      The subsequent corridor optimization reaches 11.68 fps with a
      16-field long mixed gap and 4.5-tick maximum lag. Shorter windows still
      vary: the first mixed 600 fields lose one flip and gain two gap fields.
      The camera-parity checkpoint below subsequently reaches 11.63 fps
      with an 18-field long mixed gap, retaining 3,599 ticks and zero drops.
- [ ] Exercise a complete normal playthrough of both levels, in addition to
      the targeted gameplay scenes and transitions in the automated suite.

### Completed in the 2026-10-08 polish pass

- [x] Replace FX3's animation-union camera offsets with exact GBA interpolated
      follow height, fixed/look midpoint and X/Z centering. Preserve 15,161
      original animation ticks / 60,644 offsets across LEVEL1 and LEVEL2,
      and 1,079 input-replay gameplay states (camera/target intentionally
      differ). Spend 60,572 ROM data bytes, recover 328 CPU-code bytes,
      add no RAM, retain the 4 MiB cartridge and byte-identical GSU/GSU-2.
      Record the long mixed timing tradeoff: 1,401 → 1,396 displayed frames,
      maximum gap 16 → 18 fields, all 3,599 ticks and zero drops.
      Pass the full 136-check LEVEL1 and 98-check LEVEL2 gameplay runs,
      plus 78 Python checks, opening pixels/pipeline and nine near views.

- [x] Reduce exact native waypoint work: reuse the first corridor bounds,
      unroll the fixed workspace copy and use the Nav pool's known bank.
      Preserve 576 complete native/baseline/C states, the 864-case GBA Nav
      oracle and 1,079 busy-input snapshots. Pass 49 selected LEVEL1 and
      30 selected LEVEL2 checks. Gain 17 displayed frames in
      7,200 mixed fields and two in 1,800 wolf fields with zero dropped ticks.
      Spend 38 ROM bytes and no RAM; retain GSU, GSU-2 and LEVEL1 C-reference
      hashes. Verify single-level C fixtures against their packed LevelID
      when comparing the second level of a two-level cartridge.
- [x] Trace and shorten native staged FX3's publication deferral from sixteen
      fields to eight. Preserve the four-tick queue, input FIFO, update order,
      upload ownership and all graphics. Pass 2,500 publication cases and
      9,909 real NMI boundaries; retain 300 short and 1,079 busy-period input
      snapshots. Pass five live handoff and six LEVEL2 scenes. Spend no RAM
      or ROM; retain the C-reference timeout and exact GSU-2 ROM.
- [x] Reduce FX3 joint-rotation RAM traffic with identical signed arithmetic:
      24,576 cases in each of gsusim and TypeSNES, plus exact comparison of
      294,912 matrix elements against $5546. Preserve every GSU label and
      the opening pixels; spend no RAM or ROM. Routine cycles fall 18.1%,
      opening frame cycles 0.46%. Repair the animation oracle's multi-level
      host-directory lookup; keep pose/arm/look and near-plane checks exact.
- [x] Cache the exact health-bar width by health value, independently of its
      display timer; cover medipacks, inventory, oxygen and level resets in
      5,805 complete C snapshots. Spend four bytes, leave 2,003 BSS bytes.
- [x] Shorten native FX3 unsigned products and preserve the C ABI across
      more than a million product cases and 513 real NMI boundaries.
      Pass 75 parity and 78 Python checks; retain GSU-2/C-oracle ROM hashes.
- [x] Exercise both LEVEL2 underwater switches through their real approach,
      pull/return animation and door-trigger activation.
- [x] Fix incremental parallel multi-level builds on macOS make 3.81 with
      explicit secondary GSU targets. Fresh and incremental builds, including
      BOOT_LEVEL 0/1/0, yield the same $85A2 two-level cartridge.
- [x] Repair stale animation-camera input and NMI-interrupted camera replay
      bookkeeping. Keep exact arithmetic, consecutive ticks and the original
      GSU-2/Mode 7 silhouette expectations; review the complete FX3 poses.

## 2026-10-08: another 25% in FX3 encounters

- [x] Gain 25–28% in all four established encounter benchmarks: wolves
      8.2 → 10.5 fps; mixed 7.4 → 9.5 (600 fields), 6.93 → 8.67 (1,800),
      and 8.90 → 11.38 (7,200). Keep 3,599 long-run ticks, zero drops and
      the original maximum five-tick lag; preserve graphics and all input.
- [x] Claim completed frames in NMI/IRQ, prepare the next job at a tick
      boundary, and launch after its planar upload releases the buffer.
      Use measured logic headroom for staging and publication.
- [x] Shorten camera, room-query, creature-scan and compiler-generated
      WRAM/shift work with C parity, far-pointer and interrupt coverage.
      Pass 74 parity and 78 Python checks, retain all 299 input snapshots,
      and keep GSU-2, the C oracle and GSU code byte-identical.
- [x] Restore LEVEL2's default bank layout with passing water/blade checks.
      Spend two BSS bytes; retain 2,007 BSS and 32 resident-code bytes free.
- [ ] Finish resolving Lost Valley overload: raptor drops improve 26 → 18,
      but its unchanged timing gate still fails. Never discard extra ticks
      to increase displayed fps.
- [ ] Confirm the TypeSNES gains on physical FX3 hardware.

## 2026-10-08: FX3 publication scheduling

- [x] Admit at most two queued ticks when publishing, with the original
      empty-queue fallback after a dropped tick. Retain the sixteen-field
      deadline, input FIFO and completed-buffer ownership.
- [x] Improve wolves 6.5 → 8.2 fps, mixed wolves/bear 6.1 → 7.4, and
      settled idle 15.0 → 17.1. The two-minute encounter displays
      967 → 1,068 frames with 3,599 logic ticks and zero drops.
- [x] Preserve all 299 input-replay gameplay snapshots, pass 400 scheduling
      cases / 1,344 NMI boundaries, four staging checks, 24 focused runtime
      checks, five LEVEL2 checks and 21 budget checks. Spend 16 resident
      code bytes and no data bytes; GSU-2 and the C oracle stay identical.
- [ ] Reduce Lost Valley's existing three-raptor overload: both original
      and final schedules drop 26 ticks over 1,800 fields. Keep the original
      clock gate; do not trade additional logic loss for more displayed frames.
- [x] Resolve the LEVEL2 bank-$84 capacity limit: the later 25% pass above
      fits the default layout and passes the water/blade checks.
- [ ] Confirm the measured scheduling gains on physical FX3 hardware.

## 2026-10-08: FX3 room reuse and product carries

- [x] Expand FX3's exact room cache from eight to 32 entries in spare
      upper WRAM. Retain the two-creature policy, full-key checks and reset
      behavior; reject a 64-entry allocation after it gives no added gain.
- [x] Shorten native FX3 product accumulation with byte carries. Preserve
      exact results, multiplier waits and the CPU ABI across 366 real NMI
      boundaries and over a million arithmetic cases.
- [x] Wolves improve 6.1 → 6.5 fps and mixed wolves/bear 5.7 → 6.1.
      The 30-second mixed encounter improves 151 → 163 displayed frames
      (5.03 → 5.43 fps), with zero dropped ticks and a 29-field maximum gap.
      Opening idle retains 15.0 fps; all existing performance gates stay.
- [x] Pass 127 runtime, 70 native/C parity and 21 budget checks. Spend 768 additional
      WRAM bytes and 10 ROM bytes, retain 2,009 general BSS bytes free,
      and keep the GSU-2 production and FX3 C reference ROMs byte-identical.
- [ ] FX3 creature encounters still have visible pauses. Continue profiling
      CPU collision/navigation work and verify gains on actual FX3 hardware.

## 2026-10-08: arithmetic and further GSU overlap

- [x] Stop long-division normalization once the divisor reaches the
      dividend; preserve all signed/unsigned quotient, remainder and zero
      behavior. Paired arithmetic cost falls 17.8% on GSU-2, 20.7% on FX3.
- [x] Overlap dense products, angle calculations and longer divisions with
      the GSU. Preserve results and nested ownership across 18,458 injected
      NMI boundaries. Admit complete bank-zero WRAM room-query outputs.
- [x] Wolves improve 1.4 → 1.5 displayed fps, mixed wolves/bear 1.2 → 1.4.
      The longer mixed run improves 30 → 35 frames (1.00 → 1.17 fps), with
      its maximum gap 80 → 68 fields and no dropped logic ticks.
- [x] Spend 81 resident bytes, with no additional BSS or cache allocation.
      Keep 32 cache records after a 64-record experiment gives no gain.
- [x] Pass 55 gameplay checks, 39 GSU-2/69 FX3 parity checks, 12 focused
      camera/collision comparisons and six LEVEL2 checks. Audit 1,649,205
      encounter cartridge-RAM accesses with zero ownership violations.
- [ ] Further smooth GSU-2 encounters while retaining the original
      behavior, resolution, image quality and 30 Hz game clock.

## 2026-10-08: room reuse and GSU overlap

- [x] Share 32 resolved room records on GSU-2, in spare resident WRAM.
      Look up exact world keys before reading sectors; preserve unaligned
      grids, hash collisions, mutable-floor invalidation and far outputs.
- [x] Let the GSU render during WRAM-only enemy avoidance and room-result
      calculations. Preserve ownership across both exits and all 184 tested
      NMI interruption boundaries; audit actual gameplay RAM accesses.
- [x] Wolves improve 1.0 → 1.4 displayed fps and mixed wolves/bear
      0.8 → 1.2 over 600 fields. The 1,800-field mixed run improves
      22 → 30 flips (0.73 → 1.00 fps), with its maximum gap 104 → 80
      fields. All keep zero dropped ticks and the original performance gates.
- [x] Pass 55 runtime, 34 native/C parity, 12 focused camera/collision and
      six LEVEL2 checks; verify the cold-query cost is repaid by reuse.
      Retain 260 BSS bytes and 13,453 resident-image bytes free.
- [ ] GSU-2 encounters still need more rendering time for smooth play.
      Preserve behavior, image quality and the 30 Hz clock while targeting
      the remaining measured room-query, enemy and arithmetic costs.

## 2026-10-08: further collision work

- [x] Native GSU-2 complete room collision, bridge-floor lists and closed-door
      scan. Preserve exact C snapshots, probe/list order, far pointers,
      zero-radius/front-only responses and each push's effects on later doors.
- [x] Three wolves improve 0.8 → 1.0 displayed fps; mixed wolves/bear improve
      0.7 → 0.8 over 600 fields. Engine clocks per tick fall another 7.8%.
      Maximum visible gaps fall 108 → 82 and 110 → 100 fields respectively.
- [x] Longer mixed sample improves 18 → 22 flips over 1,800 fields, or
      0.60 → 0.73 fps, with a 104-field maximum gap and zero dropped ticks.
- [x] Recover another 3,080 resident bytes, with no additional BSS or stack
      allocation. Both GSU program banks and the complete FX3 ROM stay exact.
- [ ] Smooth GSU-2 creature encounters still require more CPU/GSU time.
      Room queries and enemy movement remain substantial measured costs;
      preserve the 30 Hz clock, original behavior, resolution and image quality.

## 2026-10-08: frame-rate follow-up

- [x] Share exact native camera and collision point/side helpers with GSU-2.
      Both 600-field creature samples keep 299 ticks/zero drops. Wolves
      improve from 0.7 to 0.8 displayed fps, mixed wolves/bear from 0.6 to
      0.7; maximum gaps fall from 124/150 to 108/110 fields. Engine clocks
      per tick fall about 6%; existing performance gates remain unchanged.
- [x] Extended mixed-creature run: 14 → 18 displayed flips over 1,800
      fields (0.47 → 0.60 fps), maximum gap 162 → 118 fields, 899 ticks
      and zero drops. The original build fails the existing FPS gate at
      this duration; the optimized build passes.
- [x] Replace raster accumulator stores with `SBK`, preserving carry and
      the last-RAM-address rule. All 54 images remain byte-identical across
      six variants; renderer cycle savings range from 0.1% to 6.0%. All
      316 span/clip/dispatch fixtures pass in both cores, plus 74 Python tests.
- [x] Compare native camera and collision helpers to the pre-change C code,
      including full-width coordinates, far/aliased records, exact query
      ordering, floor/ceiling/slant results and complete follow snapshots.
      Recover 5,841 resident code bytes and 8 BSS bytes on GSU-2.
- [ ] GSU-2 creature encounters still need substantially more rendering
      time: 0.7–0.8 fps in the short samples is not smooth play. Keep the
      30 Hz logic clock, original behavior, resolution and image quality.

## 2026-10-07: current frame-rate work

- [x] Adaptive native FX3 staging: the default `FX3_STAGE=1` overlaps light
      scenes through a 31,104-byte packed image at `$7F:8000–$F97F`; heavy
      ticks retain zero-copy serial uploads. Sector data has a 32 KiB guard;
      `FX3_STAGE=0` and the default C_MOTION oracle remain unstaged. NMI
      uploads seven columns, the line-185 IRQ up to fourteen. TypeSNES
      restores display by line 32 with zero off-blank VRAM writes.
- [x] Final native FX3 opening idle **12.0 → 15.0 fps** (150 displayed
      flips in 600 fields). An earlier inventory sample reached
      **6.6 → 10.3 fps** (66 → 103 flips, +56%), with 300/301 tick-counter
      advances and zero drops. These are TypeSNES measurements.
- [x] Final active-creature FX3 samples over 600 fields: three wolves
      **59 displayed flips / 5.9 fps**, mixed wolves/bear **55 / 5.5 fps**,
      **299 / 298 ticks with zero drops**. Engine master clocks per tick
      fall 602,466 → 561,815 (6.7%) and 619,061 → 579,494 (6.4%). Maximum
      visible-frame gaps are 28 fields in both scenes. The two LEVEL1
      fixtures require at least 5 displayed fps, gaps at most 30 fields,
      drift at most five ticks, zero drops, all three actors updating and
      attacks occurring.
- [x] Final native GSU-2 active-creature samples over 600 fields: wolves
      **1 → 7 displayed flips / 0.1 → 0.7 fps**, mixed **1 → 6 / 0.1 →
      0.6 fps**. Both finish **300 ticks with zero drops**, versus the
      former 198 / 98 dropped and 190 / 107 dropped. Engine master clocks
      per tick fall 1,056,602 → 573,245 (45.7%) and 1,101,172 → 585,885
      (46.8%); maximum gaps are 124 / 150 fields. Both pursuit scenes are
      now default native LEVEL1 checks, requiring at least 0.6 displayed
      fps, gaps at most 180 fields and the same drift/drop/update/attack
      conditions. Render completion is serviced between backlogged ticks.
- [ ] Improve GSU-2 creature rendering beyond 0.6–0.7 fps. The final CPU
      work meets the logic clock, but RAM-bus sharing still limits GSU
      progress; the current display gate does not establish smooth play.
- [x] Native FX3 exact eight-entry room-query cache: 262 BSS bytes, enabled
      with two or more admitted Nav owners, invalidated by `Room_reset`. World-sector
      keys fix clamped-edge portal aliases; unaligned room grids retain the
      full resolver. Ground movement skips a redundant box lookup only in
      the same valid room sector after complete movement checks. `NB_Units`
      preserves full-width navigation bounds and point coordinates.
- [x] Native FX3 full nearest-seven shadow selection/packing: 528 complete
      native/C/independent record snapshots, including order, wrapped
      arithmetic, floor/status filters and every unused record.
- [x] Generated object classification in both FX3 and GSU-2: 256 ROM bytes
      derived from the executable C specification, all 65,536 type inputs
      verified. GSU-2 recovers 460 resident bytes beyond the compact helpers'
      452, for 912 bytes recovered by those two changes.
- [x] Native inventory publication and exact text reuse: cold opening
      50,396 → 31,495 instructions; warm text 23,127 → 703 instructions,
      81,097 → 2,659 CPU cycles. C oracles compare complete active records;
      FPS output invalidates the text cache and shadows update independently.
- [x] GSU-2/FX2 compact CPU helpers: 0.75% less engine-tick work and 452
      resident bytes recovered; walking 299 ticks/zero drops. Shared
      renderer changes preserve all nine FX2 images while saving 0.77–1.66%
      of modelled cycles; both Mode 7 variants save about 0.9% with identical
      pixels. FX3's combined renderer changes save 2.36% in the
      opening view (3,772,195 → 3,683,104 cycles) and reduce room 14 from
      8,967,160 to 8,844,609.
- [x] Native GSU-2 ground movement, complete wolf update shell, 731-byte
      target-info helper, 378-byte shared flood and exact bounds preserve
      the existing C behavior. The sector-codec optimization adds 64 ROM bytes
      and no work RAM; a 32-entry exact point-sector cache uses 256 BSS
      bytes, retains NO_BOX and keys sector/stride/count. `Room_reset` and
      `navResetCache` invalidate it. Both native variants use exact fast
      products; GSU-2 `C_MOTION` retains the software oracle. Focused checks
      cover 1,070,458 products, 48 flood cases and 9,297 target probes.
- [x] FX3 parity slice: lions/lionesses/pumas use GBA's inherited `Enemy`
      behavior; drawbridge/trapdoor floors cover cardinal extents and seams;
      COMBAT uses READY after `Weapon_update`, and landed blocks fire OBJECT
      triggers with the corresponding camera mode and ordering.
- [x] `make paritycheck` runs more than ten focused fixture families,
      including room-cache/movement/nav-scale/shadow/classification, with optional
      `OL_REFERENCE_ROM`/`OL_REFERENCE_SYMS` for uncached C comparisons.
      `tools/run_typesnes.py` keeps temporary sources/caches outside the
      TypeSNES checkout and performs no dependency installation.
- [x] Final FX3 selected validation: 25 runtime checks and 8 changed parity
      checks pass. Earlier broad FX3 validation passed 103 runtime / 58
      parity checks; earlier GSU-2 broad validation passed 47 runtime
      checks. The current Python suite passes 70. The compact helper's
      65,536 inputs match registers, flags and ABI.
- [x] Final GSU-2 validation: **50 runtime / 28 parity checks pass**,
      including completed-frame publication under backlog and both pursuit
      display/drop/drift/update/attack gates. Full parity includes all seven
      wolf checks; focused exact fixtures remain separate from live timing.
- [x] Current memory headroom: FX3 engine ROM 9,741 bytes (2,860 / 1,314 /
      1,875 / 1,710 / 109 / 1,873), general BSS 1,747 and host assembly 39.
      GSU-2 resident code 6,030 bytes (2,165 / 320 / 2,282 / 1,199 / 64),
      general BSS 252 and host assembly 162. Stack boundaries remain unchanged.

The dated notes and budget figures below retain their earlier measurements.

## 0. The budgets

Every item below spends one of these. Check the price before the port.
These are the **GSU-2** budgets. FX3's independent access rules are tracked
separately below; do not make its feature work wait for GSU-2 WRAM packing.

| budget | where it stands | what says so |
|---|---|---|
| ROM | 2 MB is a GSU-2's whole address space; LEVEL1 alone is 1.13 MB at 8bpp | `mkbanks.py`, README "The 2 MB wall" |
| host level blob | LEVEL1: 61,490 of 65,534 bytes; LEVEL2: 62,405. Including the room directory in the source ROM bank leaves **2,826 / 119 bytes**, the tighter limit. Exact navigation IDs add 2,300/2,109 bytes; further tables need compression or revised staging. A LEVEL1 per-sector byte costs 4,026, a per-link byte 587, a per-item byte 60 | `out_SNES.h` hostBytes/source-bank check, `mkbanks.py`, README "Entities on the host" |
| resident C/native | 96 KB in three banks plus 15.5 KB in two regions in `$7E`: **6,030 bytes free** in the final native GSU-2 build (2,165 / 320 / 2,282 / 1,199 / 64). General `.bss` has **252 bytes** free, host assembly **162**. The earlier 2026-09-24 camera build had only 618 resident bytes free; native creature/navigation/product paths recover room without moving code/data/stack boundaries. Cold `olLevelInit` and `gameInit` still execute from ROM before gameplay. | `make` prints `tools/budget.py` after every link, and fails it when a unit in either small region is placed outside it |
| the tick | 63.6 K cycles running (1.07 fields) after the interaction slice, against 63.3 K before it; the object loop is skipped without Action held (it was 4.7 K a tick walking the usable list through far pointers, and the ten-second boot sample ended with one tick in flight at 28.9 measured Hz). The harness now judges the discrete count with that one endpoint tick allowed with bounded cumulative drift after startup and no dropped ticks. The budget is 1.5 fields, not 2: the host wants the rest, and the clock slipped at 81 K. The water tick itself is unprofiled: the profile scene is LEVEL1's | `OL_PROFILE=walk make check TESTS=profile` |
| the frame | the unlit 256×160 4bpp frame is the default after the eleventh pass: 4.55 M modelled for the opening view (4.70 fps) after pistol mesh swaps, 2.76 M behind her at the start (7.76 fps), and 9.40 M for the six-room, eighteen-item room-14 view (2.28 fps). `MODE7=1` remains the unlit speed option at 3.24 M in the opening view. The other view timings predate pistol mesh swaps. These estimates exclude host waits and upload time. Recorded in `frame.4bpp.unlit.baseline` and held by `make check` | `make sim`, `make baseline`, README "Frame format and why it is 256×160" |

### FX3 memory policy

Use the [FX3 vendor specification](https://github.com/LimitedRunGames-Tech/snes-fx3/blob/main/docs/FX3.pdf),
§§1,6,7, for this variant: simultaneous CPU/FX ROM and SRAM access, FastROM,
3 MB FX ROM and 4 MB CPU-side ROM. All C/native code and constants now run
in FastROM, not WRAM: `$84-$89`. Those six engine banks have **9,741 bytes
free** in the final native build. Moving the remaining three
images freed 48,640 WRAM bytes; Nav, its exact-sector cache and flip/zone-offset state now use 25,290
at `$7E:5800`, leaving 23,350 unallocated. The general BSS slot still ends at `$5800` (1,747 bytes
free) and the stack still starts at `$1C00`; new FX3 data has explicit slots.
The 4 MiB ROM now places independent host tables at CPU `$E00000` and above.
Only mutable sectors remain in `$7F`: 16,104/17,196 bytes on LEVEL1/LEVEL2,
recovering 45,386/45,209 bytes. Pointers and the ready flag use 74 bytes in
checked direct-page gaps. See [HOST_TABLES.md](HOST_TABLES.md).
Lost Valley now packs 185,068 host bytes across CPU banks and stages only
27,364 mutable sector bytes. Fitting its data does not establish its timing:
the three-raptor gate with actual contact pushes drops 14 ticks in 600 fields.

- [x] Separate FX3 ROM placement/budget checks and remove all engine-code DMAs.
- [x] Allocate freed `$7E` space for three independent Nav contexts and the
      clipping workspace; link-check this separately from general BSS.
- [x] Profile three active creature searches with the renderer running, using
      elapsed bus clocks across WRAM/FastROM, including DMA/raster stalls.
- [x] Fit that three-live-creature scene into 30 Hz with no dropped ticks.
      The default FX3 LEVEL1 gate passes unchanged: 300 completed ticks,
      zero dropped over 600 fields, 3–6 displayed fps (mostly 4–5). Cold
      admission/search, original movement and an unfrozen renderer remain
      in the trace. The four-slot queue and five-tick drift bound are unchanged.
      A bounded publication deferral lets urgent logic finish; its sixteen-NMI
      deadline also preserves presentation under a permanently full queue.
      Native hot paths alone reached 245 ticks while the old host loop
      starved presentation; do not use that as a playable-scene timing result.
- [x] Keep the original three-wolf gate and add a two-wolf/bear encounter to
      the default checks. Current wolf/mixed scenes finish 300/297 ticks in
      600 fields with zero drops, within the unchanged five-tick drift bound.
      Current 3,600-field runs pass for three wolves at 1,800/0, but the
      mixed scene still fails at 1,799 ticks / one drop after native camera
      movement (also one after navigation cursor cleanup). Earlier
      mixed zero-drop extended results do not cover the new contact path. Native rotated
      contacts, native admission, bank-aware zone/room scans and render-state
      packing fund banked tables and the bear's full controller without
      changing queue size, drift or publication limits. The counter includes
      a tick already in flight; inclusive samples have 299/1,799 full new calls.
- [x] Service FX3 render completion between backlogged ticks, and reject the
      empty-height raster clips that then exposed a wrapped strip countdown.
- [x] Lift the shared one-bank host-table format: OLH2 uses full offsets and
      independent CPU ROM banks. Only mutable sectors are staged in WRAM;
      LEVEL2's 119-byte legacy source-bank headroom no longer constrains FX3.
- [x] Extend CPU-table ROM packing past 2 MiB in a 4 MiB FX3 image, including
      verified CPU-only `$F0-$FF` access. The FX interpreter separately admits
      `$60-$6F`; it must not alias the CPU-only final MiB into FX ROM.
- [ ] Revisit geometry/texture allocation around the code reservation at
      file `$1E0000`, and widen individual table indices if a table exceeds
      64 KiB. These remaining software limits are not FX3 hardware ceilings.

- [x] Gauges: `ol_tps`, `ol_ticksDropped`, `ol_tickFields`, asserted by `make check`.
- [x] Profiler: the 65816 by symbol, from TypeSNES's CPU step.
- [x] Budget report and the initialised-data guard, at link.
- [x] **A GSU frame profile that survives changes**: `make sim` writes the
      modelled cycles and the picture's CRC, `make check` holds them against
      the matching tracked `frame*.baseline` and fails past 2%, `make baseline`
      records. `tools/framecheck.py`.
- [x] **The blob past 64 KB.** Not the trigger table -- 1.5 KB in all -- but
      the sector table: everything about a sector past its two heights
      repeats, 583 distinct combinations over 4,026 sectors, so a sector is
      four bytes and a 16-bit link into a table of those. 34.2 KB became
      20.8. Every item in §2 marked (blob) is unblocked.
- [x] **Room_query in 65816 assembly.** `src/room.asm`: the same algorithm
      statement for statement, the sector multiply on the CPU's own
      multiplier, the caches in the direct page. The family that was 20,300
      cycles a tick is about 3,500.

## 1. Renderer (`src/platform/gba/rasterizer.h`, `render.cpp`)

The frame is the elephant: nothing is playable at one frame a second,
whatever the tick does. Correctness is proven by `make check`'s CRC and the
interpreter; the profile decides what to touch.

### Speed, in the order the profile points
- [x] `make sim` modelled the first frame after power-on, with the 2.85 M
      cycle blob staging the game does once; it models the steady state now
      (`--cold` for the old one).
- [x] The vertex loop -- `TM_Vertex`, `TransformPoint`, `ProjectPoint`, 450
      contiguous bytes -- ran uncached: one `CACHE` at its top, 2.1 M back.
      `RomByteAt` inlined at its four per-face sites, 0.33 M. 23.09 M is the
      opening frame in `frame.baseline` and `make check` holds it there.
- [x] **Batch the per-face setup.** `OTDraw` sets faces up into 2.5 KB
      of slots at `$0200` under a window anchored at `OTD_Face`, and
      `BatchFlush` rasterizes the batch under the raster window: two
      refills per batch, not per face. 19.1 M -> 15.9 M. Then `OTBuild`
      drops faces wholly behind the near plane (402 of 1,116) and culls
      back-facing ones from the transformed vertices (312 of 714) before
      anything is copied: 13.1 M, 62% -> 77% of fetches from cache, the
      same picture. README "The instruction cache", third pass.
- [x] **Precompute the edges.** VertexLinks are 32 bytes and carry the
      edge out of each vertex; `EdgePrepBatch` fills them under a third
      window once per batch; `EA_Left`/`EA_Right` copy records. 13.1 M ->
      11.9 M. Then the strip clip in `EA_Strip` replaced `ScanClip` and
      found the truncation bug (faces crossing the top of the screen lost
      their lower part): 11.8 M with the corrected picture, baseline
      re-recorded. README "The instruction cache".
- [x] **The fourth pass** (2026-09-03): the span setup on a reciprocal
      table in RAM with the multiplies inlined and the results in the
      loop's registers; `SpanBounds`, `EdgeAdvanceX` and `SpanFillT` fused
      into one loop; one `EA_Take` in the window for both edges, and the
      strip clip beside it; then the vertex path in registers, the face
      loops on LOOP, the packer storing the transformed-vertex offset, and
      a window over the matrix rows; then MULT for the link in the edge
      pass with `MulRecipE` inlined, and the edge steps on the loads the
      span setup makes. 11.8 M -> 8.7 M -> 7.7 M -> 7.4 M, byte for byte
      the same picture on nine views. README "The instruction cache".
- [x] A window over the clipper, `CN_Edge` on a walking pointer, the
      left clip inline, the loop entry as a stored jump: what the side
      view the camera pass produced exposed. 10.3 M -> 8.8 M there,
      7.4 M -> 7.05 M behind her, both pictures byte-identical.
- [x] **The sixth pass** (2026-09-04, late; README "The sixth pass"):
      counted before cutting. 285 of the opening view's 392 rasterized
      faces drew no pixel; half of `DrawFace`'s faces lay wholly off the
      clip rectangle. Per-vertex clip flags in the transformed-vertex
      record (`TV_CLIP`, the GBA's `transformRoom` rule) reject those in
      the ordering pass; `OTDraw` walks only the bucket range a mesh
      used and returns at once for a mesh that filed nothing; a bounding
      radius per model and per static placement from the packer lets
      `SphereVisible` skip items and instances off the rectangle before
      a node is built. 6.54 M -> 4.08 M on the opening view, 5.43 -> 2.75
      behind her, 14.9 -> 9.16 in room 14; byte-identical on every view
      but one, where seven pixels of garbage from wholly-above strips
      went away. `tools/gsusim.py` grew `--count`, `--faces`, `--facebox`
      and `--meshes` to find all of it.
- [x] **The seventh pass** (2026-09-04; README "The seventh pass"):
      `--meshes` now installs its symbols and hooks alongside `--profile`,
      which showed rooms and statics owning most of the opening view's
      transformed vertices. Their
      yaw-only `TransformRoom` path takes five matrix multiplies rather than
      nine; model nodes keep the general path. `TV_CLIP`'s near bit replaces
      the duplicate near word, taking transformed vertices from 16 to 14
      bytes and removing repeat face checks. More than 300 redundant GSU
      register prefixes collapsed to `WITH` or the default R0, and the Mode 7
      column table is read once per span rather than per pixel; the compacted
      edge take and cold corrupt-edge exit put all of `RFT_Advance` inside the
      cache window. 4.08 M -> 3.83 M opening, 2.75 M -> 2.54 M behind her,
      9.16 M -> 8.61 M in room 14; all eight comparison views byte-identical.
- [x] **The eighth pass** (2026-09-04; README "The eighth pass"):
      face records carry absolute transformed-vertex addresses and the room/
      model transform choice is hoisted to separate cached loop entries.
      `DM_Vertex` keeps both output cursors, reject mask and top Y in registers;
      `OTBuild` keeps its face count and live R14 cursor; clip interpolation and
      `EdgeChain` keep their component, direction, cell, height and record
      cursors live too. Safe one-byte delay slots absorb copy increments. The
      four build variants have separately measured 16-byte cache padding.
      3.83 M -> 3.54 M opening, 2.54 M -> 2.35 M behind her, 8.61 M -> 7.99 M
      in room 14; all eight views and all three recorded build CRCs identical,
      with 13 active whole-machine tests passing.
- [x] **The ninth pass** (2026-09-04; README "The ninth pass"):
      each model node carries a conservative origin-centred mesh sphere. Once
      its joint matrix is built, `SphereOriginVisible` projects the matrix's
      translation column directly and skips `TransformMesh`/`DrawMesh` when
      the sphere cannot reach the room's portal rectangle. Child transforms
      are still applied. A full-frame fast keep and one per-model cull gate
      keep the common on-screen case cheap. The opening view stays 3.54 M,
      while behind/forward/turn fall 13.5% / 8.2% / 25.5% and room 14 falls
      1.9%; the sum across all eight views falls 4.4%, byte-identically.
- [x] **Restore the 4bpp default** after the ninth pass: `make` now selects
      the unlit 256×160 renderer; its per-character shading looked too coarse
      to ship by default. `MODE7=1` keeps the 128×64 fast build and `SHADING=1`
      remains opt-in. Format-specific baselines and pose masks cover both paths.
- [x] **The tenth pass** (2026-09-04; README "The tenth pass"):
      `OTBuild` records its per-face near flag in the free `$DA00-$DC3F`
      range, so ordinary `DrawFace` loops no longer copy camera-space triples
      into clip buffers. Near faces take a cold loader, near vertices skip the
      reciprocal and projection multiplies, and the proven in-front reciprocal
      range removes both clamps. Unlit builds omit shade traffic, and 4bpp
      `PLOT` fills `LOOP`'s one-byte delay slot. The eight default views fall
      2.9-5.2% with byte-identical output; all four recorded layout CRCs remain
      unchanged. `gsusim.py --limit` now turns a renderer hang into a register
      and pointer-adjacent RAM dump, and exposed a zero previous-link cycle
      while this pass was being built.
- [x] **The eleventh pass**: cache flat-colour scanlines together with the
      shared edge walker, selected once per face; remove their unused UV
      steps and put PLOT in the pixel loop's delay slot. The default opening
      frame drops 4.25%, room 14 drops 0.9%, and all eight views stay identical
      in all four layouts. The view checker now fails on mismatches and checks
      shade maps too. Half-turn interaction overflow and reversed block
      gravity were fixed with failing-then-passing whole-machine regressions.
- [x] **The FX3 pipeline, not the renderer** (2026-10-06, README "Frame
      pipeline"): measured, the frame was 3 fields of render and 6 of
      upload in series. The upload now runs in both letterboxes under forced
      blank (V-timer IRQ at line 185 + the NMI), 2-3 fields, and the idle
      loop polls the STOP: standing 6.0 -> 12.0 fps, running 5.2 -> 9.2.
      Fixed on the way: `ItemsExchange` DMAed 64 KB over bank `$70` (the
      save) when published with no entities. Publication waits for an
      empty tick queue (gates 299/0, 296/0); each tick takes its own keys
      from an NMI queue (lost releases behind long render syncs). Suites:
      FX3 L1 103/103 (with the new pipeline scene), motioncheck 71/71,
      keycheck 34/34, multi L1 104/104, multi L2 65/65, GSU-2 47/47 (its
      ROM changes only by the DAS guard). Scenes that pinned ol_ticks once
      a field now pin it at the render sync too: with frames twice as often
      a sync inside a ticked field drew the next pose.
- [ ] **The ring's render sync is ~5 fields** (`Inventory_publish`'s per-item
      matrices outside rot/trans, `toGsu` and the 23/32 scaling, the per-item
      32-bit divide; `Inventory_text` recomposing every string every frame):
      ~88K instructions. Native, or text recomposed only when it changes.
- [ ] **Overlap the FX3 render with the upload**: hold the job's C2P (the
      three `MERGE`s before STOP) on a byte the NMI clears when the upload
      ends, and publish at the STOP. Built and measured at 15 / 13 fps, and
      correct if the FX3's `PLOT` really goes to the firmware's internal
      buffer; TypeSNES plots into `$71` (C2P a no-op, the clear commands
      clear `$71`), so the next frame overwrote the one uploading there.
      Needs the emulator to model the firmware buffer first.
- [ ] The vertex path is still the wall: the transform, ordering and draw
      vertex loops are about a quarter of the room 14 view,
      and the rooms are most of the vertices. The lever is count, in the
      packer: a room's faces and vertices grouped by sector cell, the
      cell's corners tested against the frustum before its vertices are
      transformed, and its faces skipped when its vertices' flags share a
      side. The matrix path under a window (3%) is the smaller local lever.
- [ ] What is left that keeps the picture is small and view-dependent:
      `RFT_Wide` (a thousand wide spans in the side view) and
      `RFT_EmptyStep` into the window, each needing the strip's
      above-screen arm out, which was measured to cost the behind-her
      view more than it saved the side; `EdgeSkip` is too big for the window.
      `RF_Line` now has its own overlapping window (eleventh pass).
      Then it was the half-height buffer: the
      per-scanline and per-pixel work is 3.3 M of 7 M -- and the Mode 7
      frame below took it, at a quarter of the pixels rather than half.
      What it left is the geometry: the vertex path (1.45 M of 6.08),
      `RFT_Recip` (0.57 M: the per-span gradients, now over spans of four
      pixels), `EdgeSkip` (0.28 M, out of the window, more strips starting
      above a 64-line frame). The levers on those are the 2D column
      frustum before transform (the camera has no pitch) and coplanar
      quad merging on the free 256-texel wrap; neither depends on the
      frame format.
- [ ] `RasterF`'s flat spans PLOT uncached (`RF_Line`, out of line by
      design: a few faces a frame).
- [ ] (optional 4bpp shading, `MODE7=0 SHADING=1`.) Its raster window
      overflows 512 bytes and its tail runs
      from ROM: that build is 13-26% faster than before the pass on nine
      views and level on the pickup's room 23. It is not the measured default;
      if this look is revisited, the shade groups want the same treatment the
      U and V steps got. One thing the ten-view
      comparison turned up: the shading build from before the fourth
      pass drew 98 pixels on the side-of-the-corridor view that its own
      plain build did not (44,700 plots against 44,602), a divergence in
      its left-clip or span path that the rewrite removed -- the current
      shading build's frame is the plain build's, byte for byte, on all
      ten views. The old ROM is `build_sh_old/` if anyone wants the why.
- [ ] Reject vertex transforms against the room clip rectangle before
      transforming (README roadmap 9).
- [ ] 4bpp texture pages: halves the 704 KB of texel pages and the fetch per
      texel is the same `GETB`; the inner loop gains a nibble select. This is
      also the only route to a second level in ROM (§12).
- [ ] Per-room face/vertex caps (576/768) become a checked split policy
      rather than packer truncation, before any other level is packed.
- [ ] Depth sort across rooms: one ordering table, or a documented reason
      the per-room buckets in traversal order are enough.
- [x] **The Mode 7 frame** (2026-09-04, formerly the default; README "The Mode 7
      frame"): 128x64 at a byte a pixel, `STB` for `PLOT`, shown at 2x
      through the Mode 7 matrix, 8 KB in two V-Blanks, the flip a scroll,
      the letterbox eight sprites. 8.75 M -> 6.06 M unlit on the opening
      view, 7.05 M -> 4.85 M behind her: 1.44x, the whole of the per-pixel
      and per-scanline half and no more, because the vertices, edges,
      clipper and glue are untouched. Adopted the same day; after the
      ninth pass it remains available through
      `MODE7=1`, while the 4bpp frame is the default again. The picture is half
      the resolution and a fifth less vertical view (64 lines at the old
      focal length; `M7D` at 0.4 would stretch it to 160 with uneven
      rows). Unverified anywhere but by reasoning: the top letterbox,
      eight sprites at Y 240 relying on the hardware wrap TypeSNES lacks;
      verify in bsnes. If an emulator-proof letterbox
      is wanted, an HDMA window (W12SEL over BG1, WH0/WH1 per line) is the
      alternative.
- [x] **The lit pixel** (2026-09-04, optional with `SHADING=1`; README "The
      lit pixel"): the span's shade ORed into every pixel's high nibble,
      the palette's eight rows, 6.06 M -> 6.54 M (+8%), checked against the
      4bpp lit frame at the same view (78% same row, 90% within one).
      The edge accumulator carries g << 10, the edge's starting shade
      rides in the record (`E_G0`) and the edge block was reordered so one
      copy loop carries it; ten bytes of redundant `FROM R0` came out of
      the window. The seventh pass brought all of `RFT_Advance` into that
      window. Left: sixteen palette rows (packer + letterbox colour to
      CG-RAM 255); a gradient across the span (two registers the loop has
      not got).

### Missing rasterizers and passes
- [x] Colour-keyed textured spans (`rasterizeFTA`/`GTA` semantics): room/mesh
      AKILL flags and pickup billboards, across 4bpp, Mode 7 and FX3, lit or
      unlit. Source zero has its own marker so opaque colours quantized to
      black stay opaque. `make keycheck`: 196 pixel/dispatch cases across six
      variants, each in gsusim and TypeSNES. Shading keeps the existing
      per-span/per-tile approximations; full per-pixel Gouraud is still absent.
- [x] General sprites (`rasterizeS`, `renderSprite`), FX3 (2026-10-04):
      pickups are colour-keyed billboard models; the effects (blood, bubbles,
      ricochet, splash, smoke) one model whose frame picks the sprite (README
      "Effects (FX3)"); the rooms' sprites camera-facing static instances
      (README "Sprites"). Explosions remain (no level here has one); GSU-2:
      pickups only.
- [x] Glyphs and text (`renderGlyph`, `drawText`), FX3: TR's font through
      the GSU (`TextRender`, packer `buildFont`, `src/inventory_text.c`),
      2026-09-26. Menus beyond the ring, subtitles: not yet; GSU-2: none.
- [x] Bars (`renderBar`): health and air, as sprites (2026-09-23); the
      inventory's health bar (FX3, 2026-09-26).
- [ ] Background images (`renderBackground`): title, credits. The
      inventory's grey copy of the game frame is done (FX3, `BgCapture`).
- [x] Shadow blob (`renderShadow`), hers and the creatures', FX3
      (2026-09-26): README "Shadows". The swinging blade's and darts'
      (upstream's objects with ITEM_FLAG_SHADOW) remain; GSU-2 none.
- [ ] Animation interpolation (`matrixFrameLerp`): the pose between
      keyframes; at 1 fps invisible, at 15 fps the difference between TR and
      a flipbook.
- [x] Pistol thigh/hand mesh swaps at the draw/holster animation boundaries.
- [ ] General `ItemObj::meshSwap`: other weapons, the Midas hand.
- [x] Independent pistol arm poses and shoulder basis/aim overrides.
- [x] Per-node torso/head tracking for pistols, after the body animation.
- [ ] The other weapons' joint overrides; manual look and camera-item tracking.
- [ ] Dynamic vertex lighting (`calcLightingDynamic/Static`): the shade term
      exists (`SHADING=1`) but items are unlit by room lights; needs room
      `lights` (§2, blob).
- [ ] Underwater transform (`transformRoomUW`): wave distortion and caustics.
- [ ] Animated textures (`animTexturesShift`): water and lava surfaces;
      needs `animTexData` (§12).
- [ ] Distance fog (`vg += fog`): cheap, the camera z is to hand.
- [x] Gouraud: per span into the pixel, eight rows, in the Mode 7 frame
      (above). Per pixel across the span and sixteen rows are the two
      steps left to the GBA's 32 per pixel.

## 2. Level data on the host (`src/fixed/room.h`, `packer/out_SNES.h`)

- [x] Room flags: water per room (`ROOM_FLAG_WATER`), in bit 7 of the
      host room record's width byte (`olRoomWater`, and `room.asm` masks
      the width); `Room_water` in `room.c` is upstream's `getWaterLevel`
      and `getWaterDepth` in one walk of the column. LEVEL1 has no water
      room, so its blob and the frame are unchanged; LEVEL2 has ten.
      `OL_DUMP_ROOMS=1 ./build/packer ...`
      lists a level's rooms with the flag. Used by §3's water since
      2026-09-04.
- [ ] Lava per sector: upstream's own `checkTrigger` says "TODO lava"; do it
      when upstream does, or before with a bit in the portal byte (blob).
- [ ] Climbable walls: TR2+. Not TR1 parity. Drop from the list.
- [x] Static mesh collision boxes (`Room::collideStatic`): the packer
      ships the colliding placements, box pre-turned, with a per-room run
      table (20 bytes each, eleven in LEVEL1); the scan is `olStaticHit` in
      `room.asm` (3,900 cycles a probe from C, for one box), the push-out
      and naming in `collision.c`; the ninth `make check` test runs her
      into the rock behind the entrance.
- [x] Item flags in the host item record (2026-09-04, later): byte 3 was
      always the mask and the hidden bit, and `olLevelInit` reads it now
      by `ItemObj::init`'s rules -- hidden is INVISIBLE until triggered, a
      full mask is REVERSE (a bit in `flags2`) plus activated. No item of
      LEVEL1 or LEVEL2 sets either. LEVEL3A now packs on FX3; legacy GSU-2
      still rejects its wide slopes. `olWakeNearbyEnemies` is gone: creatures wake by the triggers
      that name them, as TR's do. Intensity is not carried.
- [x] Entity floors for trapdoors (`getTrapDoorFloor`, shut, over its two
      sectors, from the bridge list). Drawbridges and the falling floor's
      ceiling rule remain.
- [ ] Entity collision boxes per class (`ItemObj::collide` virtuals):
      doors as one box per model; switches, pickups, holes and blocks
      through their own `collide` in `use.c`/`block.c`. Boulders,
      creatures' pushes, Natla's pieces remain.
- [ ] Flip maps (`Room::modify`, `alternateRoom`): the renderer's room
      table needs the alternate rooms and a swap; the host needs the
      flip-1 zones for navigation (blob, and the level bank).
- [ ] Room lights, ambient (`out_GBA.h` room info): for §1's lighting. The
      decorative sprites are in (FX3, 2026-10-04).
- [ ] Cameras and camera frames (`cameras`, `cameraFrames`): fixed cameras
      and cutscenes (§8, §10).
- [ ] `objectTextures`/`spriteTextures`/`spriteSequences` by reference,
      so sprites can be general (§1).

## 3. Lara (`src/fixed/lara.h`)

Each slice is a set of `s_`/`c_` handlers plus what they call. Hang/climb
and water are the two that unlock levels: LEVEL1 needs neither past its
first rooms, LEVEL2 needs both.
- [x] **Hang and climb**: `c_checkHang`, `c_checkClimbUp`, `c_checkGrab`,
      `c_checkSpace`, `c_checkSwing`, `c_hang`; states HANG, REACH, HANG_UP,
      HANG_LEFT/RIGHT, CLIMB_2/3, JUMP_UP with the ledge grab and the
      `vSpeedHack`, `alignAngle`/`alignWall`. The two-click climb is the
      tenth `make check` test. The grab is untested: LEVEL1 has no
      four-to-seven click ledge with clean sides. Not here: `c_climbSide`
      (TR2 walls), the hang camera angles.
- [x] **Water** (2026-09-04): `updateWaterState` (ABOVE/WADE/SURFACE/UNDER),
      oxygen, `updateSurface`/`updateUnder`, `c_swim`, `c_surf`,
      `c_checkWallUW`/`Surf`, `c_checkWaterOut`; states UW_TREAD/SWIM/GLIDE/
      DIVE, SURF_TREAD/SWIM/BACK/LEFT/RIGHT, WATER_OUT, DEATH_UW; the swim
      constants; her pitch as `olLaraAngleX`, drawn since 2026-09-26.
      The eleventh Lara scene in the runner is the first on LEVEL2: `make
      BUILD=build_l2 LEVEL=LEVEL2 check`. Not here, because TR1 has no
      animation for them: wading (WADE is entered from a shallow pool as
      upstream enters it and the handlers' guards read it, but the arms
      that would play ANIM_WADE and its kin play a stand) and ROLL_UW.
      Also not here: the look camera at the surface, the splash, the
      breath, the camera's 22-degree pitch.
- [x] **Object interaction states** (2026-09-04, later): PICKUP,
      SWITCH_DOWN/UP, USE_KEY, USE_PUZZLE, BLOCK_PUSH/PULL/READY, with
      `moveTo`, `animSkip`, `animIsEnd`, upstream's limit boxes and
      `checkLimit`, and her hands (`olLaraWeaponState`) freed by the
      animation's EMPTY command or HANDSFREE effect. See README "Using
      things". Not here: USE_MIDAS/DEATH_MIDAS, the inventory screen a hole
      opens (the matching key is offered instead). The camera angles each
      handler sets are in since 2026-09-26.
- [x] SWAN_DIVE, FAST_DIVE, HANDSTAND: original input precedence, free-hand
      gate, transition above 131 units/tick, signed 95% drag, fatal fast-dive
      landing above 133, and distinct water-entry animations/pitch/speed.
      Hanging with Action+Up+Walk takes animation 159, including its 193-tick
      climb and shared finishing animation. Cartridge tests exercise both
      climbs through landing with free hands, and dive thresholds directly.
- [ ] POSE, SPECIAL, ROLL_AIR (check TR1 reachability before porting).
- [x] `c_STATE_STOP`'s Up+Action nudge (four units, for the climb check).
- [x] The lean (`angle.z`), FX3 (2026-09-26): `olLaraAngleZ` beside her
      pitch, upstream's tilt in the run and s_turnUW, decays per water
      state, clamps and resets; GS_ROLL and the GSU's ItemTilt (see the
      Done entry). GSU-2: no room in lara.c's bank.
- [x] Hit reactions (2026-09-24, FX3): `animHit` from a creature's push,
      `hitQuadrant`/`hitTimer`/`hitFrame`, SND_HIT, and the four HIT
      animations drawn over her own (`contact_fx3.c`). GSU-2 has no creature
      push to call it from; that is the gap, not this. `healthTimer` is the
      HUD's (2026-09-23). `restore` (Jump+Weapon revive) is a cheat, left out.
- [ ] `extraL` as a struct: turnSpeed, moveAngle, vSpeedHack, weapon state,
      camera -- today three globals.
- [ ] `updateObjectsCollision` in full: every item with ITEM_FLAG_COLLISION
      in the adjacent rooms, not only the door list.

## 4. Items (`src/fixed/item.h`)

- [ ] Room relinking on move for every item, not only Lara and the falling
      floor (`updateRoom` in `ItemObj::update`). The renderer half is done
      (2026-09-24): entities are drawn in the room the host has them in, so
      creatures (which do relink) are seen where they walk. Blocks and other
      movers still keep their load room on the host.
- [ ] Joints and spheres (`getJoint`, `collideSpheres`, `updateHitMask`):
      what weapons and creature attacks hit. Under FAST_HITMASK (GBA/32X)
      only the swinging blade asks spheres -- done 2026-09-26 (FX3); a
      creature's hit is its bounds there.
- [ ] Dynamic add and remove (`ItemObj::add`/`remove`, the free list):
      darts and the effects done as packer-made pools (FX3); Natla's
      missiles and dropped items remain -- the same pool shape fits.
- [ ] Per-item `collide` virtuals (§2 boxes).
- [x] Serialization for §10 (FX3, 2026-10-04): the whole 32-byte record
      and the per-item arrays, a superset of upstream's `ItemSave`/
      `LaraSave`/`EnemySave`; blocks re-floored as `Block::load` does.
- [x] Animation commands: SOUND (§11) and EMPTY.
- [ ] The `fx` effects table: FX_LARA_BUBBLES done (FX3, 2026-10-04);
      `fxDrawRightGun`, the flip map, the floor shake and the rest remain.
- [ ] `ItemObj::update`'s class dispatch for every TR1 type (§5) rather
      than the open-and-shut default.

## 5. Objects (`src/fixed/object.h`, 1,128 lines; one class per line of the ITEM_TYPES enum)

- [x] Keys and keyholes (KEY_HOLE_1..4), puzzle holes (PUZZLE_HOLE_1..4;
      the type becomes DONE but the piece is not drawn -- the renderer's
      model for an entity is the packer's), with counts per type for the
      inventory. The LEVEL2 key scene in the runner.
- [x] Trapdoors (TRAP_DOOR_1/2, LOD) as their own class: a door's update,
      a bridge's floor. FX3 cardinal extents, open states and signed
      coordinate seams have direct shared-GBA/native checks, including
      the drawbridge's two-sector floor; GSU-2 drawbridge remains.
- [x] Blocks (BLOCK_1..4): push and pull with her, the floor rewritten in
      the host sector table (game pak RAM). Not the box table's blocked
      bit (upstream's TODO too), not MOVING_BLOCK. FX3 landed blocks now
      fire OBJECT triggers with the original camera ordering. The LEVEL2
      block scene remains in the runner.
- [x] Darts and the emitter (2026-09-24, FX3): see the Done entry.
- [x] Swinging blade (2026-09-25, FX3): see the Done entry.
- [ ] Traps: spikes, boulder (rolls, kills), slam, sword, hammer
      handle/block, lightning, trap ceiling; each with its damage and Lara's
      death animation. (None is in LEVEL1 or LEVEL2.)
- [ ] Gears, the Midas hand, the scion pieces, MOVING_OBJECT, waterfall,
      the flame emitter (TR1's lava rooms), bubbles.
- [x] Doors: TR's real one (FX3, 2026-10-06, `door.c`): upstream's
      `Door::action` writes walls into the two sectors of the doorway while
      shut and the level's bytes back from ROM when open; creatures, the
      camera and the guns' trace see it. GSU-2 keeps the box (no room). The
      nav boxes and flipped rooms are upstream's TODOs too.
- [x] Switch in full: she pulls it, the lever's KILL makes it usable, its
      own trigger asks (`Switch_use`), timed switches spring back. The
      2026-10-08 two-level audit exercises both LEVEL2 underwater switches:
      swim to the use point, pull, return to tread and fire the real door
      trigger with the original activation mask.
- [x] Pickups in full: TR's 23-frame bend, counts per type; ammo counts
      and the medipacks' use wait for the inventory (§10).
- [ ] Cutscene actors (CUT_1..4, LARA_SPEC), Natla's and Adam's pieces.
- [x] Effects (FX3, 2026-10-04): blood (bites, hits, darts, the blade),
      ricochet (missed shots, darts at the wall), splash, bubbles, the dart
      emitter's smoke -- upstream's spawns, SpriteEffect and Bubble; README
      "Effects (FX3)". Explosion and sparkles remain (no level here has them);
      GSU-2: none.

## 6. Weapons (`src/fixed/lara.h` weapon state, `WeaponParams`)

- [x] Pistol core: held Action, nine FIRE reload frames, one damage per arm,
      8192-unit spherical range, pitch/yaw acquisition limits, living active
      targets only, and room-sector visibility through `Room_trace`.
      Endpoints and disconnected overlapping rooms are checked; room-sector
      origins distinguish coordinate aliases, and the shared trace now walks
      across the signed X/Z seam. Emulator fixtures cover these boundaries.
- [x] Inventory of weapons (FX3, 2026-10-05): all four guns from the start,
      as upstream's `inventory.init()`; ammunition is upstream's -1 for each
      (endless), and ammo pickups count into slots no page shows, as
      upstream's do. See the Done entry. GSU-2: the pistols only.
- [x] Pistol draw and holster: original animation ranges, mesh swaps (§1),
      FREE/BUSY/DRAW/HOLSTER/READY, R to draw/holster and B/Action to fire.
      Water requests holstering; busy hands cannot draw; death clears poses.
- [x] Rotated X/Z box centres and acquisition: `weaponGetAimPoint` and
      `weaponFindTargets`'s smallest-absolute-yaw rule, including active-list
      order for equal angles. Pitch/yaw and visibility use the aim point;
      range uses the item origins. Cartridge fixtures cover rotations,
      midpoint width/rounding, cone limits, visibility, pitch and ties.
- [x] Target lock and per-tick tracking: held Action preserves target identity,
      release permits a new acquisition while continuing to aim, and both arm aim flags follow the
      separate upstream retention arcs. Occlusion clears aim but preserves
      identity; lost aim returns only within the acquisition cone. A locked
      target without arm aim suspends fire. Dead/hidden/inactive targets
      release the lock; short-coordinate trace limits still apply.
- [x] Pistol arms: separate raise/fire/reload/lower animations and aim angles,
      ten-degree angle steps, one bullet per firing arm and ten-tick held
      cadence. Armed turns enter the upstream fast-turn state.
- [x] Pistol torso/head tracking before movement and weapon updates, signed
      arm averaging, return-to-centre decay and death/reset. Acquisition is
      relative to body plus torso; arm tracking remains body-relative.
- [x] Current-frame creature aim points: the packer resolves
      `getBoundingBox(false)`'s nearest-pose choice into shared local XYZ
      points. Acquisition, tracking and visibility read the current tick's
      point; camera/collision boxes and other model types retain unions.
- [ ] Combat camera, manual look and camera-item tracking.
- [x] Firing: the other weapons' rates/damage, the shotgun's six pellets
      (FX3, 2026-10-05). Before (2026-10-04): the spread's two `rand_logic`
      draws a shot, the traced ricochet at a miss, the muzzle flash
      (`drawFlash`); the ricochet's sound has no room in the APU image.
      A pellet's hit is a box stand-in for the joint spheres (next item).
- [ ] Hit test against creature joints/spheres (§4), not the item's box.
      Upstream (GBA too: FAST_HITMASK is contact only) traces every bullet
      with its spread against the target's spheres at its keyframe; here a
      pistol-class shot at an aimed target always hits, the shotgun's
      pellets test the frame box. Per-keyframe sphere tables for LEVEL1's
      wolf/bear/bat are 108 KB at 8 bytes a sphere (26/18/8 meshes, 246/339/
      126 keyframes), ~40 KB with byte offsets: ROM the two-level cartridge
      does not have (2026-10-06).
- [ ] Static-mesh visibility; the pistol trace checks the room sectors,
      floors, ceilings and portals. Shut doors (FX3, 2026-10-06) are walls
      in those sectors now, as upstream's are.

## 7. Creatures (`src/fixed/enemy.h`, 1,558 lines; `nav.h`)

Wolf, bear and bat have their original state graphs.
FX3's raptor graph/update and current-frame contact are implemented and
oracle-tested. The real room-51 encounter passes; Lost Valley timing still fails.
Upstream has a class per creature (README roadmap 5).

- [ ] The `Enemy` base in full: `updateMood` with aggression and timers,
      `getMoodNormal`/`getMoodEscape`, `updateLocation` (turn rate, speed,
      step/drop/jump limits per creature), `updateTargetInfo`, `flying`
      handling, `bite`, `hit` with hit masks and the injured flag, death
      by animation. FX3's corpse timer (`HIDE_CORPSES`) is complete above.
- [x] FX3 normal/wild moods, aggression, injured flag consumption, stalk/
      escape box selection and three-context nearest/forced admission.
      Cartridge mood, PRNG and Nav snapshots match the shared C++ methods.
- [x] Bat's wake/fly/attack/circling/death graph, animation-speed flight,
      16-unit vertical pursuit, 20-degree turn, two-point contact attack,
      room/column collision and falling death. FX3 follows its own 3D Nav
      waypoint; GSU-2 keeps the shared flying-zone flood. FX3 uses pre-push
      current-frame FAST_HITMASK bounds; GSU-2 retains animation unions.
      Joint-sphere contact is not implemented.
- [x] Wolf's original state decisions, pending-state latch, head-offset
      front/behind distances, previous-tick 2/5-degree steering, 50/100-point
      per-attack damage, three death animations, animation motion and ground
      step/drop/zone/portal/active-creature collision. Hit spheres, rendered
      head/tilt remain; FX3 now has original aggression/corridor navigation.
      FX3 attack contact uses current-frame FAST_HITMASK bounds captured
      before the original enemy push, with no late animation-union retest.
      GSU-2 keeps its shared pathfinder. Recheck the previous stationary,
      front-facing circling case; room-14 pursuit does approach and attack.
- [x] Bear's original state graph and PRNG order, 500-unit head targeting,
      previous-tick 0/2/5-degree turn caps and close-reverse half-turn,
      animation motion and 256-up/256-down ground limits. Running contact
      deals 3 points/tick; bite/standing attacks latch 200/400 once. Both
      original death routes terminate, with one 200-point upright fall hit.
      Native decisions/update fit both targets; shared C++ and complete C
      cartridge oracles cover them. FX3 now consumes current-frame pre-push
      contact; GSU-2 retains animation-union bounds. Joint masks outside
      FAST_HITMASK, head/tilt and blood still need work.
- [x] FX3 raptor state graph, PRNG consumption, 400-unit head targeting,
      previous-tick 1/4-degree turn caps, animation movement, 256-up/down,
      three 100-point attack latches and immediate random lethal-hit deaths.
      Original C++ decisions and complete C-update oracles pass.
- [x] FX3 current-frame raptor bounds/contact-before-push and original room
      adjacency. The real room-51 encounter reaches Lara and deals 100 damage
      at tick 418; 480 complete C collision-phase snapshots also pass.
- [x] FX3 current-frame wolf contact: 64 synthetic eligibility/push/attack
      cases and 62 original-pose ghost-contact positions pass on all three
      levels. Both 50/100-point attacks remain single-hit; room-14 pursuit
      still bites. The old ROM fails the ghost-damage regression. Mixed
      wolf/raptor owner orders and 480 full C phase snapshots per species
      retain the zero-radius room guard and existing FastROM data/latches.
- [x] FX3 current-frame bear contact, including running damage, latched
      200/400-point attacks and a dying upright bear's one fall hit. The
      old ROM fails the ghost-contact regression; 144 bank-boundary cases
      and 447 original-pose positions pass on LEVEL1/LEVEL2. Both native
      update entries match 512 complete C snapshots each.
- [x] FX3 current-frame bat contact: 200 bank-boundary/eligibility/dead-target
      cases, 124 original flight/attack ghost positions and 512 complete C
      snapshots per update entry preserve flight, two-point attacks and
      falling death. The saved old ROM fails the ghost-damage regression.
      All 24 mixed wolf/bear/bat/raptor owner orders match the C phase.
- [ ] Restore the mixed LEVEL1 extended timing gate: one drop in 3,600
      fields remains after native camera movement, down from four before
      navigation cursor cleanup.
      Cold camera fallback/trace work remains expensive; retain the four-slot
      queue, five-tick drift bound and sixteen-NMI publication deadline.
- [ ] Finish raptor live-scene timing.
      Full current-frame data is available for all valid models in FastROM.
      The three-raptor scene with real pushes drops 14 ticks; retain the four-slot queue,
      five-tick drift bound, five search steps and publication deadline.
- [ ] Remaining per-creature classes: crocodile land/water, gorilla,
      rat land/water, T-rex, GSU-2 lion/lioness/puma and raptor, mutant 1/2/3, centaur, mummy,
      Larson, Pierre, skateboard/skater, cowboy, Mr T, Natla, Adam. Each is
      a switch over its own states with attack ranges and damage.
- [ ] Ranged attacks (Larson, Pierre, mutants' fireballs, Natla): bullets,
      the `fx`, line of sight.
- [ ] Head tracking and hit spheres.
- [x] Exact sector-to-navigation-box IDs: two-bit room-local predictors,
      packed literals and a rank directory replace the ambiguous rectangle
      scan. `navBoxAt` shares native room/sector lookup with collision;
      `NAV.json` independently checks all 4,026/4,299 original sectors.
- [x] FX3 per-creature `Nav`: weighted incremental queues, step/drop limits,
      32-bit corridor clipping and five search steps per update. Shared
      blocked-mask and empty-queue tail bugs fixed in both C and C++.
- [x] FX3 ground-1/ground-2/fly maps for both normal and alternate geometry,
      original ordered initialization union and active flip selection, with
      ground-2 height semantics. Whole-map aliases are lossless; no extra
      mutable table copy. Water creatures use the fly family in TR1.
- [ ] Complete navigation integration: room swapping/flip triggers and
      water-creature controllers/movement. GSU-2 still needs all six maps,
      the per-creature implementation and its memory plan.
- [x] Creature activation by TR's rules: the triggers that name them and
      the item flags (§2), replacing `olWakeNearbyEnemies`. Heavy triggers
      (a creature standing on one) remain: creatures ask no floor.
- [x] FX3 uses GBA/32X's actual limits: `MAX_ENEMIES 3`, `NAV_STEPS 5`.
      The SNES `common.h` values 2/1 are old unused cuts, not GBA's defaults;
      GSU-2's shared flood does not use them either. Profile before raising
      the FX3 limits.

## 8. Camera (`src/fixed/camera.h`, 527 lines)

- [x] `getBestLocation` and clipping, on `Room_query`: trace, clip,
      checkWall, checkHeight, the four-direction fallback. Three cuts for
      the tick, README "The camera follows her": no divide on a level axis;
      the fallback cached by her sector, height band and facing, and at most
      once in eight ticks; the traced-and-clipped answer cached by her
      sector and the wanted view's. 5-6 K cycles a tick; the load tick ends
      after olLevelInit now.
- [x] Read against upstream (2026-09-03): the yaw is the direction from the
      view to the target, not her facing; the target is three quarters up
      her box and eased; the lerp floors; the snap arrives clipped. The
      opening view moved to where upstream's fallback puts it (her west
      side) and the baseline with it. A camera test in the runner.
- [x] FX3 native sector walkers, query wrapper and signed coordinate math.
      Full-width slopes/products, both ordered boundary probes, far pointers
      and existing cache behavior match C; 5,247 ROM bytes recovered. See
      the 2026-09-06 native camera pass below. This does not remove the
      approximation caches or close the remaining live timing failures.
- [x] FX3 native camera movement and room resolver. Every signed movement
      delta, ordered full room queries, far/overlapping inputs and complete
      follow/cache snapshots match C. This recovers another 270 payload
      bytes, with no persistent RAM or gameplay changes; live timing below.
- [x] **Exact follow** (2026-09-23): only the trace's verdict is cached,
      the goal is worked out and clipped every tick, and the fallback is
      kept as a direction. A runner oracle replays upstream's arithmetic
      against the cartridge's view, tick by tick: exact from the moment the
      line clears (it matched only on sector crossings before, a 60-unit
      sawtooth between, settling where she entered her last sector). One
      query proves most of the clip; the camera costs 125 K master clocks a
      walking tick against 114 K for the approximation. README "The camera
      follows her".
- [x] **Height and pitch** (2026-09-23): the view is distH/distV from the
      target at upstream's ten degrees (plus her pitch), and the GSU shears
      its projection centre to where the target projects -- upstream's
      look-down for two FMULTs a frame. The opening picture changed with it
      (frame baselines re-recorded: crc eb23d3da, FX3 2217f0ab).
- [ ] Interpolated follow target height: upstream uses getBoundingBox(true),
      while Camera_targetY still reads an animation union. The nearest-frame
      boxes used for contact are not a substitute for interpolated bounds.
- [ ] What is left of the caches: a blocked line with no fallback keeps
      its hit point within the sector key, and the fallback's "nearest the
      present view" is judged once in eight ticks. The sine table is 1024
      entries against upstream's 4096 (distV 263 for 267).
- [x] Combat and look (2026-09-23/24): `camera.mode`, chosen each tick as
      upstream's toCombat (guns drawing or ready, alive) and s_checkLook do;
      combat is 2048 away along her aim (or head + torso), look 768 along
      her head + torso at move() speed 4, which truncates where 8 floors.
      Look is the SNES Y button: the d-pad turns her head (updateLook's
      manual branch, `Lara_manualLook`) and her STOP and SURF_TREAD states
      hold still. The runner's upstream oracle covers draw, combat, holster
      and look: 379 ticks exact.
- [x] Fixed cameras, camera targets and lookAt (FX3, 2026-09-26; see the
      Done entry), and every state's targetAngle/targetDist/center (both).
- [ ] Modes: object (a heavy trigger's), cinematic (`cameraFrames`), FLYBY.
- [x] Camera triggers (§9): switch, target, one-shot, timer (FX3).
- [ ] Shake, the underwater and death behaviours.
- [x] **She vanishes when the camera comes close** (FX3, 2026-10-05): the
      GSU's near plane was `ZMIN` 512, the projection table's floor. FX3
      clips at upstream's 64 (`ZNEAR`) and projects 64..511 out of line
      (`TM_Near`, `PC_Near`, `ShearNear`); vertices behind the camera carry
      side bits so faces beside and behind it are dropped before the
      clipper. See the Done entry and README "The near plane (FX3)".
      GSU-2 keeps 512: its GSU program is unchanged.

## 9. Triggers (`src/fixed/room.h` checkTrigger)

- [x] Actions END (`olLevelEnd`, for the shell) and SECRET (`olSecrets`,
      a bit each), and the packer keeps a trigger that has either and no
      entity. ACTIVATE_CAMERA and CAMERA_TARGET on FX3 (2026-09-26).
      Remaining: SOUNDTRACK (music); FLOW, FLIP*, EFFECT are TODO upstream
      (GBA) too.
- [x] Trigger types: pad and antipad under her feet only, switch/key/pickup
      asking their first named object before firing. Native FX3 COMBAT
      requires READY after `Weapon_update`; landed blocks fire OBJECT
      triggers with the corresponding camera mode/order. DUMMY returns.
      Heavy triggers from creatures and GSU-2 COMBAT/OBJECT remain.
- [ ] Item activation flags at load (§2) and the `ITEM_FLAG_ACTIVE` seeding
      upstream does in `startLevel`.

## 10. Game shell (`src/fixed/inventory.h`, `game.h`, `level.h`)

- [x] Inventory (FX3, 2026-09-26): upstream's state machine, the main and
      item pages, medipacks and pistols used from it, keys and pieces
      offered to a hole on the USE page, the ring drawn by the GSU with TR's
      text and the grey background. README "Inventory".
- [x] The passport and the death page (FX3, 2026-09-26): Select while
      dead opens the death page; Load Game (this level) and Exit restart,
      Save Game shows the no-back-up-RAM warning; upstream's drawOptions
      box through the GSU's RectRender.
- [x] Inventory, the rest (FX3): the guns (2026-10-05), the settings
      pages, the ring's sounds (swapped into APU RAM while it is open:
      README "Sound") and the passport's page mask (upstream's
      itemVisMask, DrawModel's per-node test in WeaponNodeFx3), all
      2026-10-06. GSU-2: none.
- [x] Gameflow as the GBA's alpha has it (FX3, 2026-10-04): a level's END
      trigger boots the cartridge's next level; past the last, upstream's
      end page (`INV_PAGE_END`), whose Action goes to the title -- this
      cartridge's first level. The passport lists the cartridge's levels.
      README "Levels on one cartridge".
- [x] Title page (FX3, 2026-10-06): upstream's `INV_PAGE_TITLE` at power-on,
      over the first level held at its load, on TITLEH.PCX fitted to the
      frame (`TITLE.BIN`); its passport starts a level, Exit to Title and
      the end page come back to it. README "The title page". Lara's Home
      is left out (no model in LEVEL1/2, no gym on the cartridge).
- [ ] The full level order (`LEVEL_INFO`), cutscenes between levels, the
      gym; the title's music (no music at all).
- [x] Save and load (FX3, 2026-10-04): upstream's gameSave/gameLoad from
      the passport -- Save Game writes the battery RAM, Load Game offers
      Current Position -- in `src/save.c`; cartridge type $18 (FX3 with
      battery) keeps the game pak RAM, the save at `$70:1000`. See the Done
      entry and README "Saved games". GSU-2: no passport to save from (no
      inventory); its board would use `$78:0000` behind `BRAMR` ($3033,
      Book II 2-3).
- [x] Settings (FX3, 2026-10-06): upstream's gSettings and its three pages
      -- gamma (palGamma over CG-RAM), Show FPS (drawFPS through the GSU's
      TextRender, on by default as upstream), SFX, Swap A B; Music, Rumble
      and VSync kept, inert -- in the battery RAM. README "Settings".
- [x] HUD: health and air bars -- upstream's drawHUD and renderBar, as
      sprites (2026-09-23; see the Done entry). Pickup names and the ammo
      counter remain, and want glyphs.
- [x] Death: Select pressed after she dies restarts the level (2026-09-25),
      the one Load Game entry of upstream's death passport this cartridge
      has; see the Done entry. On FX3 it is upstream's death page itself
      (2026-09-26): the passport, its Load Game.
- [x] Fades (`gBrightness`), FX3 (2026-10-04): every level comes in from
      upstream's -128, 16 a tick -- `palBright`'s per-channel subtraction is
      the PPU's own colour math, BG1 less the fixed colour, which the NMI
      sets from a field count the load tick starts (28, 24 .. 4, 0, two
      fields a step). OBJ palettes 0-3 never take colour math, so the HUD's
      bars come in at full brightness where upstream darkens them too.
      GSU-2: none (no room in its resident C for the one store).
- [x] Input in full: look Y (2026-09-24), walk L, weapon R, action B,
      jump A, inventory Select (FX3, 2026-09-26), roll X (2026-10-04: a
      d-pad cannot press upstream's Up+Down, so X sends both, as the NDS
      build's face button does). Start is upstream's free-camera debug
      toggle; left unmapped.
- [ ] PAL: 50 Hz fields, a 25-tick clock or a 30-tick clock dropping every
      sixth field -- decide, then the `OL_SPEED`/`OL_ACCEL` sites.

## 11. Audio (README "Sound")

- [x] SPC700 driver: `src/spc.asm`, 483 bytes, a sample player on the
      S-DSP's eight voices with upstream's UNIQUE/REPLAY/NEW/STOP modes;
      uploaded through the IPL protocol at boot by `UploadSound` in
      main.asm, with the level's image after it. SNESMod was not needed.
- [x] Samples: the packer (`packer/sound_SNES.h`) keeps the ids the level
      can fire, ranked, decimated to 7350/5512/3675 Hz, cut at a second
      and BRR-encoded against the DSP's decoder; `SOUND.BIN` (62 KB,
      32 of LEVEL1's 52 ids; 32 of LEVEL2's 57) and `SNDTAB.BIN` (640
      bytes, to WRAM). `OL_DUMP_SOUNDS=1` prints the plan and SNR.
- [x] Playback: upstream's `soundPlay` natively (`src/sound.asm`, 724
      bytes; `sound.c` the C_MOTION reference): chance, distance from the
      camera target on the CPU's multiplier and `phd_sqrt`, random gain,
      pitch and variant. Mono, as the GBA is.
- [x] Integration: SOUND animation commands (§4) natively on FX3 and in
      item.c on GSU-2; the pistol shot and the wolf's/bear's flinch
      (`Weapon_fire`); the scream at 154 units a tick and its stop on
      landing or entering water; draw/holster in `weapon_anim.asm`; a key
      that does not fit (`Hole_collide`).
- [ ] `SND_RICOCHET` (fired by every missed shot, FX3): adding it to the plan
      drops two of the level's animation sounds (ids 13 and 34 on LEVEL1);
      decide, or find the bytes. (The guns' sounds took the other road:
      one region refilled at a change of gun, below.)
- [x] The guns' sounds (FX3, 2026-10-05): SND_PISTOLS/MAGNUMS/UZIS/SHOTGUN_SHOT
      and SND_SHOTGUN_RELOAD share one region of the APU image, refilled
      through the IPL when she changes gun (`GUNSND.BIN`, `olSndGun`). LEVEL2
      lost id 34 to it; LEVEL1 nothing.
- [x] The inventory's sounds (FX3, 2026-10-06): the ring's (108-115) in
      `GUNSND.BIN`, uploaded over the level's last samples while the ring
      is open (`sound_ring.asm`); SND_HEALTH (116) in the level's image.
- [ ] `soundSources` (room ambience), the underwater loop and breath:
      upstream (GBA) plays neither. Lara's hit sound (needs `hitTimer`, §3).
- [ ] Pan: the DSP's VOL(L)/VOL(R) from the camera yaw and `phd_atan`.
      Upstream is mono, so this is beyond parity.
- [ ] The boot: the IPL moves 48 KB/s, so the 62 KB image costs 1.3 s of
      black screen. A driver-side bulk loader (three data ports a
      handshake, `mov !abs+x,a` stores) would be about 1.5x faster; a
      smaller image proportionally. The runner's boot gate allows three
      seconds and reports the first tick's field.
- [ ] More of the level's sounds: the 64 KB is the wall. Lower rates
      buy time; a second bank of samples swapped at a room change would
      need the CPU to read ROM after boot, which GSU-2 forbids (FX3 could).
- [ ] `rand_draw` is a 16-bit xorshift in `sound.asm`; upstream's LCG is
      in `sound.c` for the reference. Nothing observable follows it.
- [ ] Music: TR1's is CD audio; the GBA ships tracks as compressed samples.
      A tracker rendition per track (SNESMod would play it), or none.
      FLOW/SOUNDTRACK triggers (§9) wait on it.

## 12. ROM, data pipeline and levels

- [ ] 4bpp texture pages (§1): 704 KB → 352 KB for LEVEL1.
- [ ] Compression for geometry the GSU reads in place -- it cannot, so the
      choice is: decompress into game pak RAM at level load (128 KB, of
      which the frame buffers and the host blob take most), or ship
      uncompressed and fit fewer levels.
- [x] Multi-level cartridge (FX3, 2026-10-04): `make LEVELS="LEVEL1
      LEVEL2"`, the GBA alpha's two. A level table (`mkbanks.py
      --fx3-multi`), one CPU program reading it (`olLevel`), one GSU
      program per level (the second at `$41:8000`), the boot request in
      the battery RAM, `BOOT_LEVEL` for `check`/`sim`. See the Done entry.
      The cartridge is ~95% full: a third level needs compression, or a
      level's sphere data and APU image moved, or a bigger budget than
      FX3's 3 MiB GSU window. The GBA's third is GYM (Lara alone: no class
      to port, 960 KB of GSU data, ~160 KB CPU-side). Re-packing textures
      as out_GBA.h's Atlas does will not make the room (measured
      2026-10-06): TR's tiles are 92/92/96% covered by LEVEL1/LEVEL2/GYM's
      object and sprite textures, and their rects, deduplicated across all
      three, need 28 pages against the 33 tiles -- 320 KB. GYM wants the
      4bpp pages below.
- [ ] **State the ceiling honestly.** 2 MB holds one LEVEL1 at 8bpp, two or
      three at 4bpp; TR1 is fifteen levels and four cutscenes. Full TR1 on a
      GSU-2 cartridge does not fit any ROM it can address. Options: a
      level-select build per cartridge, an emulator-only larger image, or
      decide that parity is "any level, one at a time".
- [ ] Pack every TR1 level through the packer and fix each refusal
      (room caps, slant range, 255-tick animations, 64 models, 320
      animations, 6 items per trigger -- each cap is a `#define` today and
      a level after LEVEL1 will hit one). LEVEL2 packs (2026-09-02): 94
      rooms, 576 KB in 9 banks, 13 texture pages, host table 53 KB, nothing
      dropped, 1.6 MB of the 2 MB; `ln -s <scratch> build_l2; make
      BUILD=build_l2 LEVEL=LEVEL2` builds it (the runner wants a relative
      build path) and `gsusim` draws Lara's start
      (room 90) coherently. Lara was TR item 93 there and the host addresses
      `items[0]`: the packer gives her slot 0 now whatever her TR index
      (`slotItem` is the inverse map, the host list is written in slot
      order; LEVEL1 unchanged byte for byte). Water can be tested in it.
      Booted in TypeSNES it runs at 30 ticks with the camera settling 1536
      behind her (z 11264), and its frame agrees with the interpreter's
      to within 105 bytes -- something in the upper middle of the view,
      one entity's worth, still to be found. The boot clock's finite-window
      assertion now accounts for one tick in flight at its endpoint, so 28.9
      measured Hz no longer hides this frame comparison. Two things it showed up: the
      load tick dropped one NMI tick every boot (the gauge now starts after
      the load), and the *tick* is over budget there -- 90 K cycles, 1.5
      fields, 137 ticks in 300 fields walking: `navFlood` 8%, `collideRoom`
      7%, `Lara_collideItems` 5%, `animProcess` 5% with more creatures
      awake. The flood no longer clears every box (only the ones the last
      flood of its kind reached); the rest is §7's per-creature navigation
      spread over ticks and §3's collision in assembly.
- [x] **Two more resident regions** (2026-09-04 evening): `$7E:6800-$7FFF`
      (6 KB, ROM bank $3E, slot 7; the `.bss` cap moved to `$6800` with
      3.5 KB spare) holds `enemy.c`; `$7E:1000-$1BFF` (3 KB, ROM bank $3D,
      slot 8) holds `common.c`, under a stack that was painted and measured
      at 421 bytes deep over the runner's busiest scene and now keeps
      `$1C00-$1FFF`. wla cannot bound a slot below its ROM bank, so
      `tools/budget.py` fails the build on an overflow of either region,
      and the runner asserts the 64 bytes above the stack floor stay zero
      at the end of the boot and walking scenes. 5.7 KB -> 14.8 KB free.
- [x] **The region over the `.bss` is 10 KB** (2026-09-04, later): the
      nav flood kept a 2 KB queue per zone kind only to clear what it
      wrote, and a bitmap does that in 256 bytes; the `.bss` cap is `$5800`
      (hdr.asm SLOT 2, the DMA in main.asm, budget.py). `use.c` lives
      there beside `enemy.c`. budget.py also refuses a section of either
      small region's units placed outside the region, which it did not
      look for before.
- [ ] **WRAM code paging behind a stopped GSU**: 6,030 resident bytes are free,
      but the largest piece is 2,282 bytes. A bank per
      level's creature and object set, DMA'd into the 10 KB region at
      level load, when the GSU is stopped anyway. Not per frame. The
      low host region has now been reclaimed through `$0600` (2,560 bytes);
      the remaining `$0110-$05FF` is reserved for host assembly.
- [x] **Native trig and square root**: 265 bytes of assembly replace 1,401
      bytes of compiled helpers; the 2,306-byte tables and PRNG stay the
      same. Native results are checked in TypeSNES, including integer-return
      high words, and the C reference remains under `make fixedcheck`.
- [x] **Native item movement and bounding boxes**: 358 bytes replace 2,311
      bytes of C; 1,936 bytes recovered after bank packing. `make motioncheck`
      builds the C reference as a separate cartridge and checks every level
      animation plus signed-ramp, gravity and coordinate-wrap boundaries.
- [x] **Native animation and wolf decisions**: 439/555 bytes replace
      2,056/1,797 bytes of C; the 1,443-byte boot-only math check stays in ROM
      before GSU startup. The C oracle occupies ROM bank `$3C` with the GSU
      stopped, keeping the recovered resident space available for gameplay.
- [ ] **Code size, continued**: 816-tcc spends 20 bytes a line, 60 where the line is
      32-bit. `collision.c` is 25.4 KB, `camera.c` 12.2 KB, `lara.c`
      12.9 KB. An audit of `int32` that could be `int` (angles, heights,
      most differences), and the hottest and largest routines in 65816
      assembly as `room.asm` already is (2.7 KB smaller than its C), would
      buy more than another region.

## 13. Latent, noted in the 2026-08-18 review, still open

- [ ] MatrixPush overflow desynchronises later pops (`gsu/mesh.gsu`).
- [ ] RoomsCollect indexes RL_VISITED with an unvalidated portal word.
- [ ] OTBuild has no OT_MAX cap.
- [ ] Sprite types with zero-size textures leave entities on a memset-zero
      model (`out_SNES.h`).
- [ ] The runner's field-boundary sampling: any test that reads a global
      after N fields can land mid-tick. Publish observed globals once, at
      the end (done for `ol_ticks`, `ol_floorEff`); wait for the second
      tick after pinning (done for the sector test); let three fields pass
      after the state a scene waits for before reading anything else (the
      water test, which first read her height between the state change to
      STOP and the collision pass that snaps it, in the same tick); read
      the position the tick publishes last, `ol_probe`, where a minimum
      over fields is wanted (the rock scene, which read her record inside
      the rock for one field once the tick's phase moved). Audit the rest.
      Two more of the family from the interaction slice: a scene that
      reads her hands or moves her on must wait for the interaction
      animation to end (its EMPTY command is at the end); and the host
      sector table in game pak RAM can be read from the runner but not
      trusted while the GSU owns the bus -- ask the probe instead.
- [ ] `EdgeSkip` walks a strip that starts wholly above the clip top into
      one or two rows of garbage at the top of the screen (seven pixels
      at the top-right of the bridges view in room 14, from three faces
      at y -160 to -6). Masked since the sixth pass rejects such faces
      before the rasterizer; a strip of a partly visible face that starts
      above may still be affected. Find the off-by-one.
- [x] Repair `make animcheck`'s stale pose fixture (2026-10-08). Initialize
      the target-height input R11 for the original level camera: zero had
      tilted the view and cropped Lara. Review all four complete FX3 poses
      and record expectations for its distinct rendering path. GSU-2 and
      Mode 7 pass with their original silhouette expectations; independent
      arm/head/torso and bounds checks also pass. No renderer code changed.
- [ ] Nothing has run on hardware. The uninitialised-WRAM bugs were fixed
      blind; a flash cart run of `make check`'s scenes is owed.

## Order

The dependency-driven order, restated with the new information:

1. ~~§0 Room_query in assembly and the blob restructure~~ -- done, and the
   §2/§3/§7 items they gated are open.
2. §1 renderer speed to ~10 fps -- nothing is playable until then, and every
   feature below is judged on a screen that moves. 2.8 fps modelled after
   the fourth pass; the picture-identical trims left are worth perhaps
   another 0.6 M, and the half-height buffer is the 2x after that.
3. ~~§3 hang/climb~~ done, ~~§8 camera clipping~~ done, ~~§3 water~~ done.
4. ~~**§12 the code banks, before any further slice.**~~ Two regions found
   (14.8 KB free now); paging by level and the code-size audit remain in
   §12 for when those are spent.
5. ~~§2 item flags; §5 the objects LEVEL2 needs (keys, trapdoors, blocks);
   §9 the trigger actions they use.~~ Done, with §3's interaction states.
6. **§12 paging or further code-size work before larger slices**: native
   math and the cold-check relocation made the pistol core fit, and swapping
   the level/block banks fitted the aiming slice. Native aim-point arithmetic
   funds target lock, and the native arm controller fits the pistol animations;
   dives/handstands reclaim the low host region and move the math tables.
   Bats then fit with 655 bytes left; native item movement/bounds recover
   another 1,936 bytes. Native animation/wolf decisions and ROM-resident boot
   checks then fund wolves. Exact navigation/shared room lookup and native
   bear decisions/update now leave 1,816 bytes. This is the GSU-2 constraint: FX3's engine executes from
   FastROM, with separate ROM and data budgets above.
   2026-09-23: load-only code runs cold from ROM with the ROM bus taken
   for the load tick (`olLevelInit`, 3.5 KB), and boot-only `gameInit`;
   after the HUD, 3,094 bytes are free. Other load-only paths are the next cheapest space.
7. §6 weapons and §7 creatures, class by class, LEVEL1's first. FX3 has
   all four guns and the near plane at 64 (2026-10-05), the title page
   and the settings pages (2026-10-06, §10).
8. §10 inventory and shell, ~~§11 audio~~ (effects done 2026-09-07; music
   is a data question), §12 a second level in ROM.

## Done: 2026-10-06 (evening), doors as upstream's, the ring's sounds and the passport's pages (FX3)

Continue toward GBA/32X parity.

- **Doors** (`src/door.c`, `$86`): upstream's `Door::action` -- while shut,
  the sector one along the door's facing in its own room and, through that
  sector's portal, the one at the door in the room beyond are walls
  (`{-127, -127, link 0}`); open, the level's four bytes come back from the
  ROM copy of the SECTOR table (directory entry 1 in the host bank). Every
  door shuts at the load (the constructor), the update opens/shuts on
  upstream's rule only when it changes (`olDoorOpen`, not saved: a load
  boots), then `Room_reset` and the camera's caches. The box push
  (`Lara_collideItems`, native and its C door fixture) is gone on FX3:
  upstream's `Door::collide` is empty. Found by the new scene: LEVEL1's
  bear walked through shut door 28 and was shot dead through it on the
  10-06 ROM (790 ticks in room 16); now it stops at x 18367 and the guns
  never lock on. The guns' trace and the camera see shut doors too.
  Runner: `doorSectors` (TS port of the choice), "OpenLara FX3: doors",
  the boot test checks every door's sectors, the switch scene is FX3-aware
  (she is held her radius from the wall, never through into room 10).
  GSU-2 keeps the box (6 bytes free) and is byte-identical.
- **The ring's sounds** (`src/sound_ring.asm` in `$86`, packer
  `sound_SNES.h`): SND_INV_* in `GUNSND.BIN` after the guns' records (12-byte
  header at 72), swapped over the level's last samples under the gun region
  while the ring is open; SND_HEALTH in the level's image at /4 (LEVEL1
  lost ids 13 and 34, LEVEL2 id 53). About ten fields of IPL each way,
  inside the opening/closing tick; scenes that read the game right after
  the ring closes wait for that tick to end (`closed()`).
- **The passport's pages** (`settings.c` Settings_visMask, a 30-byte table
  of upstream's updateVisMask ranges -- the C_MOTION oracle's `$BB` had 107
  bytes and the if chain cost 275; record word 15,
  GSU `MD_VIS` at `$F63E` shifted in `WeaponNodeFx3`): upstream's mask;
  the Save Game spread no longer shows another spread's printed page.
  Frame model +0.03%, CRC unchanged; GSU-2 byte-identical.
- **Suites, 2026-10-06 (evening, final code):** FX3 L1 101/101 (gates
  299/0, 296/0; frame CRC ebabee54, +0.03%), motioncheck 71/71 (the C door
  fixture is gone with the box), keycheck 34/34, multi L1 102/102, multi
  L2 65/65, GSU-2 47/47 (ROM byte-identical all session), tools 62 OK. The
  mask scene first raced the field boundary (frame stepped, mask not yet):
  it samples at gameUpdate's entry now. Nothing committed.
- **Measured, not done:** GYM by re-packing textures (no: 28 pages for
  the three levels' rects against 33 tiles, §12); bullets against joint
  spheres (§6: upstream tests every shot's ray against the target's
  spheres for its keyframe, the port's pistol-class shots always hit an
  aimed target -- but the tables are ~108 KB a level, 40 KB quantised, and
  the two-level cartridge's CPU-only data now ends at its last byte).

## Done: 2026-10-06 (later), the settings pages (FX3)

The options ring's Graphics, Sound and Controls did nothing.

- **settings.c** ($88; $87 in the C_MOTION oracle): upstream's gSettings
  defaults (rumble, no swap, SFX, music, gamma 0, FPS shown, no vsync),
  osLoad/SaveSettings at `$70:3110` ("OLST", version 1, seven bytes; not
  valid while written). inventory.c: `OPTION_BAR`/`OPTION_SWITCH`,
  `olInvOptValue`, onOption's Left/Right (bar 16 of 256, switch over),
  `Settings_option` for onDetail/onSound/onControls. inventory_text.c:
  `renderBar` (BAR_OPTION) and the switches; `INV_RECT_MAX` 24 (GS_RECT to
  `$ECA1`); OSTR_LEVEL is 16 now.
- **SFX**: native `Sound_play` returns at once (and sound.c's reference).
  **Swap A B**: game.c, at the keys. **Gamma**: CG-RAM copy taken at the
  load tick (`Settings_level`), `olGammaApply` (save_fx3.asm, multiplier)
  at the render sync, the NMI 64 bytes a field (`olCgramPiece`, skipped
  when the HUD goes up). 816-tcc's loop took 123K CPU cycles (+52K the
  first time) and the tick it ran in lost a key's release; native 29K, off
  the tick. **Show FPS**: `FrameBody` calls `TextRender` in a game frame
  when `GS_TEXT` has glyphs; `Settings_text` (render sync) composes the
  counter into `olInvText` when it changes, `olGameText` copies it; the
  ring's text carries it too (upstream's drawFPS is after the inventory).
- **Room.** inventory_draw.c moved to `$89`; host WRAM image had 8 bytes,
  so the NMI/ItemsExchange additions and the title's BG_STATE pin are
  three `jsl`s into save_fx3.asm (6 bytes left). FONT.BIN gains the bar's
  four colours after its glyph table (InitHud: CG-RAM 188-191).
- **Runner**: settings with the counter off planted beside the boot
  request (`OWN_SETTINGS` to skip); "OpenLara FX3: settings".
- **Suites, 2026-10-06 (later):** FX3 L1 97/97 (gates 300/0, 296/0; frame
  CRC ebabee54), motioncheck 72/72, GSU-2 47/47, multi L1 98/98, multi L2
  64/64, tools OK. GSU-2's GSU program (wla bank 2) unchanged all day.

## Done: 2026-10-06, the title page (FX3), and an 816-tcc `||` bug

The next GBA/32X parity milestone was the title page.

- **The picture.** Packer `writeTitle`: the DOS release's TITLEH.PCX, rows
  16-447 box-filtered to 216 x 144, every pixel a CG-RAM index -- the
  level's 128 colours, HUD, font, greys and borders as the title boot
  leaves them (1-191) plus 64 of its own by k-means at 192-255 (nothing
  else uses them). LEVEL1: 70% on existing colours, rms 4.8. `TITLE.BIN`
  (31,232 B) goes with the first level's small CPU tables, one bank
  (`OL_TITLE_ADDR/BANK/OFFSET/SIZE` in hostbank.inc); the multi cartridge's
  CPU-only data now runs to `$3F0000` (one bank left).
- **The boot.** No request = `BOOT_TITLE` (`SelectLevel`; a request of that
  kind forces level 0, and a cartridge without TITLE.BIN treats it as a
  start). `UploadPalette` writes the 64 colours, `CopyTitle` one MVN into
  `BG_BUF` before `InitGSU` (four fields of boot). The load tick opens
  `INV_PAGE_TITLE` (game.c); `ItemsExchange` pins `BG_STATE` = 1 in its
  ring-only block on a title boot, so `IR_Grey` plots the picture and
  `BgCapture` never runs: no GSU change at all.
- **The page.** inventory.c: `setPage(TITLE)` = passport, controls,
  detail, sound (+ Home if the level has the model: `Inventory_hasModel`),
  `pitch = 1024` at the open (upstream's, snapping level at the end of the
  opening), Select/Jump do not close it, the passport's Load Game starts
  the chosen level (`BOOT_START`); Exit to Title and the end page's Action
  are `BOOT_TITLE` (`restartKind`).
- **816-tcc: `A || !x`.** Every ring frame since 09-26 had a 1-px line
  across row 60 and, on title-less pages, a stray glyph: `drawOptions`'
  `if (state != SHOW || !count) return;` drew the empty options border, and
  `drawText`'s `if (!s || !*s) return;` read a null title. The backend
  resolves the earlier operands' true-jumps where the `!` operand's false
  path goes on (raw tcc output, not the optimizer). Probed shapes:
  broken = any `|| !<non-comparison>` past the first operand (`if`,
  `while`, value); fine = `x == 0`, `!x || A`, `A || (B && !C)`, `A && !x`.
  Five sites, all split: also `collision.c` (dead Lara kept hanging with
  Action held) and `room.c` fireTrigger (would touch `items[k]` past
  itemsCount). `tools/tcclint.py` runs before every 816-tcc compile and
  refuses the shape; `test_tcclint.py`. GSU-2 +23 B (6 / 27 left in
  `$7F:0000` / `$7E:8000`).
- **The chosen ring item comes as close as upstream's** (INV_SEL_MAX 240
  dropped): FX3's near plane is 64 and TM_Near reads the doubled table.
- **Found on the way, not bugs:** the ring's side items sit at the screen
  edges and the current one spins edge-on -- upstream's geometry (camera
  1456 from the ring centre, 12 degrees above it, items at 688).
- **Tools.** gsusim `--ram FILE` replays a frame from a captured game pak
  RAM image (the runner can dump one at `StartGSU`), which is how the ring
  was taken apart; `--meshes` tolerates labels that have gone.
- **Runner.** Every FX3 cartridge a scene loads gets a plain-start boot
  request unless `COLD_BOOT`; `fx3ShownFrame()` decodes the shown 8bpp
  frame from VRAM; `fx3Passport().load(true)` from the title. New scene
  "OpenLara FX3: the title page"; the levels scene now ends on the title
  and loads Vilcabamba from the title's passport; the saved-games scene's
  power cycle lands on the title too (the battery holds a spent request)
  and loads from there, and the boot's own spend of the planted request is
  not a stray write.
- **Crystals hidden** (both variants): upstream's Crystal constructor sets
  them invisible ("disable crystals for now"); level.c drew LEVEL1's one
  (room 20) and LEVEL2's two. New scene "OpenLara SNES/Super FX:
  crystals" (status and the renderer's $FF); the 10-05 ROM fails it.
- **Suites, 2026-10-06:** FX3 L1 96/96 (gates 300/0, 296/0), motioncheck
  72/72, GSU-2 46/46 + crystals, multi L1 95/95 + the changed scenes
  again (8/8), multi L2 64/64, tools 62 OK. GSU-2's ROM is no longer
  byte-identical (room.c, collision.c, level.c); its GSU program is.

## Done: 2026-10-05, the near plane at 64 (FX3)

With guns out at
LEVEL1's start the combat camera clips to the wall 257 from her and she
was not drawn at all: the GSU clipped at 512, where its reciprocal table
stops (65536 / dz must fit FMULT's signed word). Upstream clips at 64.

- **The plane.** `ZNEAR` 64 on FX3 (`ZMIN` 512 stays the table's floor and
  the sphere tests' "no projection to trust"): `ClipNear`/`ClipT`, and the
  sphere tests' "wholly behind". GSU-2 defines `ZNEAR` as `ZMIN`.
- **The projection, 64..511.** `TM_Near` (the vertex loop's near arm),
  `PC_Near` (ProjectClip, `dz < 4`; it replaced a clamp that could not fire)
  and `ShearNear` (frame.gsu's pitch, in the eight NOPs after `FrameBody`):
  the table at `z >> 1`, `LMULT`, `NEAR_SCALE` (>> 10, held to +-8K).
  Exact to 1.5 pixels on the screen. Same-size hooks, `iwt r15` jumps (R13
  is LOOP's): no tuned window moved; FX3's raster section moved 384.
- **Side bits behind the camera.** At z <= 0 the signs of x and y say which
  of the rectangle's half-spaces (through the camera) the vertex is in,
  for the sides `RoomClipFx3`'s `NK_MASK` admits (the ring clears it via
  `InvNear`); OTBuild's AND then drops a face wholly beside or behind the
  camera before the clipper. A first cut with `x >= z`-style planes was
  wrong behind the camera (a different half-space from the in-front
  vertices' bits) and changed a picture; the sign form is exact.
- **Cost.** The opening view: 4.69 M before, 4.81 M at 64 without the side
  bits, 3.77 M with them (baseline re-recorded, crc ebabee54: the wall
  beside the camera is drawn in the rightmost 22 columns). room14 2.40 M ->
  2.00 M, room9 1.02 M -> 0.83 M; views with the camera inside her model
  pay for drawing her.
- **Shadows** down to 64: `ShadowCentre`/`ShadowProjectFx`/`SP_Near` in
  place of `ProjectPoint`/`ShadowProject` (same-size calls); a corner short
  of 64 still drops the shadow (the octagon is filled unclipped).
- **Checks.** `tools/nearcheck.py` (`make check`, FX3 with LEVEL1 first):
  nine close views (two with a seeded shadow), every near point and shadow
  corner against its arithmetic bit for bit,
  every frame again with `NK_MASK` held at zero, the same bytes. Runner
  "OpenLara FX3: the near plane": guns out at the start, the view 257 from
  her, frozen, she covers 3,190 words of the frame (the 10-05 ROM before
  this: 0). GSU-2's ROM is byte-identical.

## Done: 2026-10-05, the four guns (FX3)

Upstream gives her
the pistols, magnums, uzis and shotgun from the start; this had the pistols.

- **Ring and change of gun.** `Inventory_init` adds the three; their slots
  were already upstream's. `Inventory_use` is `changeWeapon`
  (`Weapon_change`), and `weaponUpdateState` (`src/weapon_kit.c`) holsters,
  takes the goal from FREE, draws. `olInvDrawRequest` is gone.
- **weaponParams** for all four (`weapon_kit.c`), read by `weapon.c`'s
  targeting through `olWeaponParam`; `weapon_anim.asm` takes the reload from
  `olWeaponReload` (the uzis' 3). Magnums 2 damage, uzis flash 2 ticks at 180.
- **The shotgun**: its three animations as WeaponArm 4-6 (one on both arms),
  `weaponDraw/Holster/UpdateShotgun` in C, six pellets with two draws each, a
  box stand-in for the spheres (README "The four guns"), `updateLook`'s
  shotgun branch natively.
- **Meshes**: the packer's `writeWeaponKit` (WEAPON.json `kit`: 22 meshes,
  the shotgun's animations and per-frame torso deltas), `GS_KIT` at
  `$70:F648` (24 bytes), `Weapon_publish` at the render sync behind
  `olWeaponKitDirty`, `InitGSU` seeding it, `WeaponNodeFx3`/`WeaponLookFx3`
  at the end of `mesh.gsu` replacing DrawModel's two GFCALLs byte for byte
  (the raster section, ALIGN 16, moved 544 bytes: cache phases kept). The
  flash's distance down the hand is GS_KIT's too. She wears the shotgun on
  her back now: the FX3 baseline is re-recorded (crc 32d23bfd, 4,686,203;
  the old picture, CRC 2217f0ab, comes back with her torso her own, at
  +1.5 K cycles).
- **Sounds**: one APU region for the gun in her hands (see §11).
- **Banks**: `weapon_kit.c` $88; `block.c` to $85 and `camera_fix.c` to $84
  to make room; the C_MOTION oracle keeps them and puts `weapon_kit.c` in
  $87. FX3 $84 556 bytes free, $85 1,415, $88 3,012.
- **Save**: `olWeaponKit` in the payload, `SAVE_VERSION` 3; `Weapon_restore`
  after a load (its parameters, its meshes, its sounds).
- **Checks**: runner "OpenLara FX3: the four guns" (holster-change-draw for
  each gun, the shotgun's 33-frame draw with the swap at 10 and the torso
  table frame by frame, 12 draws a blast, the pellet oracle at 8000, 26-tick
  cadence and the reload sound, the magnums' 2, the uzis' 4-tick volleys and
  2-tick flash, the holster meshes on her thighs, the uzis' head, choosing
  the holstered gun); "OpenLara FX3: inventory" picks the shotgun on the ring
  and reads GS_KIT in game pak RAM and the APU's gun region (its directory,
  pitches, samples) and hears the blast and the reload from it. GSU-2's ROM
  differs from the 2026-10-04 build only by that build's stale
  `gameRenderSync` (+4) and a label; its GSU program is byte-identical.

## Done: 2026-10-04 (later), effects and the muzzle flash (FX3)

- **Pool.** 24 hidden entities after everything else (packer
  `appendFxPool`); `src/fx.c` `Fx_add` retypes the first free one. Upstream's
  `fxBlood`/`fxRicochet`/`fxSmoke`/`fxSplash`/`fxBubbles`, `SpriteEffect` and
  `Bubble`. LEVEL2 now uses 125 of the 128 entity slots.
- **One model, frame = sprite** (`buildFxModel`, `NODE_SEQUENCE`, the GSU's
  `BillboardNode`): every kind's frames are consecutive 54-byte quad meshes;
  the frame's Y angle byte says which.
- **Spawns:** bites (native wolf/bear/raptor state machines and `bat.c`, and
  the C references for C_MOTION), shots (blood at the aim point; a miss traced
  16384 along the arm's aim with upstream's two spread draws), darts (blood,
  ricochet, smoke), the blade (blood on its four draws), water entry (ten
  splashes), FX_LARA_BUBBLES (`item_anim.asm`).
- **Joints:** `FX.BIN` (new per-level CPU table; `OlLevelDesc` is 64 bytes, 32
  words, with `fxAt/fxBank/fxSize`): upstream's chain per tick at angle zero,
  turned on the host; the packer bounds it at 5 units over 64 yaws (and her
  pitch/lean), the runner at 2 against upstream's `getJoint` itself
  (`spherecheck --joint`, 266 points).
- **Muzzle flash:** `DrawMeshFlash` replaces DrawModel's `DrawMesh` call (8
  bytes for 8): after hand nodes 10/13 while `GS_FLASH` runs; the shot keeps
  an end tick and `ItemsExchange` (`FlashTimer`) derives upstream's timer.
  WEAPON.json's `flash`, texbank.inc's `FLASH_MESH_ADDR/BANK`.
- **Timing.** The first cut dropped a tick on 7,7,7 (298/1): the idle pool
  cost the render sync 12 K master clocks. Now the pool is skipped while idle
  (`olFxLive`/`olFxPending`/`olSyncEnd`: render sync and the DMA), the
  active-list walk is native (`olUpdateItems`, render_sync.asm; the 816-tcc
  loop was ~5.6 K a tick) and the flash costs no tick: 299/0 and 296/0, with
  the wolves biting her (and bleeding) throughout.
- **Native SpriteEffect** (`FX_Sprite`, render_sync.asm, from the native
  walk): the ten splashes' 816-tcc updates (int32 multiplies) held LEVEL2's
  ticks back long enough that the water scene's air bar missed its window;
  the move is the PPU's 16x8 multiplier now, `sin * hSpeed >> 14` exactly
  (the runner runs it against fx.c's C tick by tick, at a speed past a
  signed byte too, which it hands to the C).
- **gsusim** seeds hidden entities (TR's hidden bit) as animation `$FF`, as
  the host publishes them; the level table is 32 words an entry.
- **The fade-in** (FX3) came with it: see §10.
- **And the rooms' sprites** (FX3): 43 in LEVEL1, 81 in LEVEL2, as static
  instances marked to face the camera (`InstTurn`); README "Sprites".
- **Found on the way:** wla tracks REP/SEP textually, so a host routine placed
  after 8-bit-index code assembles `ldx #0` as two bytes: `FlashTimer` wrote
  255 into the left arm's flash until it said `.ACCU 16`/`.INDEX 16`.
- **Checks:** runner "OpenLara FX3: effects" -- pool lifecycle (blood frames
  00011112222 then gone at 12, ricochet 4, smoke 21, exhaustion), joints
  against upstream, shots (two draws, blood at the aim point, a ricochet
  16376 down LEVEL1's corridor and 512 off the wall behind), the GSU drawing
  three splash frames distinctly and nothing when hidden, both muzzle
  flashes, and on LEVEL2 ten splashes at the surface and bubbles rising 13 a
  tick to burst there; the fade's register sequence; room 10's sprites drawn.
  Final round: FX3 LEVEL1 91/91, FX3 LEVEL2 62/62, the two-level cartridge
  92/92 and 62/62, FX3 motioncheck 72/72 (its render-snapshot oracle models
  the pool's skip), GSU-2 46/46 with its ROM byte-identical, tools 58/58
  (`test_mkbanks` had a stale expectation from the multi-level layout: FX3's
  APU image is CPU-only and whole). FX3 opening frame CRC 2217f0ab unchanged
  (+1.44% against the recorded baseline, +0.07% of it this slice).

## Done: 2026-10-04, LEVEL1 and LEVEL2 on one cartridge (FX3)

- **Layout.** FX3's code moved from `$BC-$BF` to `$86-$89` (Makefile remap
  of the units' banks, `.BANK 6-9` in the native units, `budget.py`), so the
  GSU's window above `$45` is unbroken: LEVEL1 `$45-$58`, LEVEL2 `$59-$6E`.
  CPU-only data from `$F0`: host tables bank-aligned, the small tables in
  their tail, spheres and APU image bank-aligned, to `$3E0000`.
- **One CPU program.** `olLevel` (src/level.h), the level table's entry,
  replaces every per-level build constant: host bank (`olHostBind` copies
  the directory to `olHostDirectory`; `olHeader16` reads the copy), LEVEL.BIN
  bank (`Room_isAdjacent`), the camera/inventory/font tables, palette, HUD,
  sound table and image (main.asm boot uploads), GSU entry (StartGSU).
- **One GSU program per level**: same source, the level's `texbank.inc`; the
  second at `GSU_ORIGIN $8000` (gsu.inc), entry `$8100`. Byte diff against
  the first: 496 high bytes, each +$80.
- **Boot request** at `$70:3100` (save.c `Save_boot`, main.asm
  `SelectLevel`); save header version 2 carries the save's level; the
  passport lists every level by LevelID; END trigger -> next level or the
  end page (`Inventory_levelEnd`; `INV_GSU_FRAME`, InvRender masks it).
- **Found by the second level:** `contact_fx3.asm` read the animation count
  at a literal `$E00014`; so did two tests. gsusim seeded its entities from
  a literal `$200000`; it reads the level table now (`--level N`). And the
  first GSU frame after a reset drew the previous boot's publications
  (game pak RAM survives it): the passport's ring, then a cross-level load,
  trampled `GS_LEVEL_READY`. `InitGSU` publishes gameInit's hidden entity
  records and zero ring/shadow/text/rect counts before the first frame.
- **Phase-by-phase checks:** the code move alone (84/84, 58/58, frame and
  gates identical); the descriptor alone (84/84, 58/58, GSU-2 ROM identical,
  `$8156`); then the cartridge, final round: FX3 LEVEL1 84/84, FX3 LEVEL2
  58/58, GSU-2 LEVEL1 46/46, the two-level cartridge's LEVEL1 85/85 (with
  the gameflow scene) and LEVEL2 (`BOOT_LEVEL=1`) 58/58. Gates 299/0 and
  296/0 on both FX3 LEVEL1 ROMs; frame CRCs 2217f0ab / eb23d3da unchanged.

## Done: 2026-10-04, saved games (FX3)

- **Battery.** FX3's cartridge type with a battery is `$18` (the FX3
  specification, section 2); a Super FX battery keeps the game pak RAM
  whole, and TypeSNES sizes it from `$FFBD` (128 KB). FX3 draws in `$71`,
  so the GSU-2 frame buffers' `$70:1000-$31FF` was unused: a probe of every
  GSU and CPU write through boot, play and the ring found nothing there.
  The save lives in it (8,704 bytes).
- **Save** (`src/save.c`, bank `$BD`; `src/save_fx3.asm`, 81 bytes in
  `$BF`): one field list serves save, load and length -- the 32-byte item
  records, `olEnemyNext/Turn`, the active list, Lara's extraL globals, the
  arms and look, the weapon target, the inventory counts, pickups, darts in
  flight, level end, secrets, the fixed cameras' ONCE bits (now
  `olCamOnce`), flip state, both seeds. MVN per field, a Fletcher sum over
  words; the magic is written last and the save is read back (the passport
  shows the warning if not). LEVEL1: 2,898 bytes for 80 entities. The
  header carries the ROM's checksum, so another build's save is no save.
- **Load:** Current Position marks the header and restarts the cartridge
  (Load Game's path); `Save_check` (gameInit) sees the mark and clears it;
  the load tick runs `Save_apply` after `olLevelInit` and the water start,
  before the camera snaps: blocks give back the boot's raised sector, the
  fields come back, blocks raise their saved sectors, Nav maps follow the
  flip state. Creatures re-acquire Nav contexts on their next update, as
  upstream's `activate()` re-makes their extra.
- **Runner:** `OpenLara FX3: saved games` (LEVEL1) saves a wolf mid-chase
  with the pistols out, holds the battery RAM byte for byte to the game
  while the ring keeps it still, watches the window for any other writer
  from power-on, carries only the battery image into a second emulator,
  loads, and compares every field at the next `gameUpdate` -- exact; the
  wolf navigates and moves again, the pistols stay out. The LEVEL2 block
  scene saves after the push and loads: floor back where it stood, its top
  where it went. With `Save_apply`'s block floors disabled that scene fails
  (the left sector stays a wall).
- **816-tcc:** `gen_opi` (816-gen.c) takes any `VT_CONST` right operand as
  an immediate without checking `VT_LVAL`: `x != *(uint16 *)0x80FFDE`
  compiled to `sbc #$FFDE`. Loads are right; the checksum is read through
  a function. No other source has a constant-address dereference on the
  right of an operator (grep in the entry's session); a compiler fix would
  be `&& !(fr & VT_LVAL)` in gen_opi's constant test.
- C_MOTION FX3 links with `save.c` in `$BD` (its `$BE` had no room).

## Done: 2026-09-26, hardware corrections found by a TypeSNES session

- **GSU signed branches.** The manual's instruction pages (9.17, 9.20) swap
  BGE and BLT; hardware has `$06` = BGE, `$07` = BLT (Star Fox's outcode
  routine, bsnes, snes9x, Mesen, fullsnes, wla-superfx). TypeSNES and
  gsusim.py decoded the manual's way and gsu.inc's GBLT/GBGE were swapped to
  match, so four signed branches would have gone the wrong way on hardware.
  The macros now expand to the plain mnemonics and gsusim decodes `$06` as
  BGE; the frame CRC and cycle count are unchanged (eb23d3da, 4,615,832).
- **BG vertical scroll.** The PPU shows BG line VOFS + 1 on the first
  displayed line. FX3's frame sat on lines 39-182; BG1VOFS is -41 now
  (centred at 40, HUD sprites level with frame rows again), GSU-2's is -1
  (its frame's row 0 was off the top). MODE7=1's M7VOFS_FB0 is unchecked.
- The TypeSNES tree that models both (and VRAM writes only in V-Blank or
  forced blank, which the port's uploads pass: 1.1 M writes, none dropped)
  is what the suites run on; a TypeSNES HEAD from before it would fail.
- Still owed: a flash-cart run (§13); these three were all "right" on the
  one emulator the port was ever checked on.

## Done: 2026-09-26, shadows (FX3)

- Host: `Shadow_publish` at render sync -- hers and the seven nearest
  visible creatures': slot, the floor's depth (`ol_floorEff`/`roomFloor`),
  the frame box's middle and 5/32 of its size -- to `GS_SHADOW` through
  ItemsExchange (6 + 12n bytes). None under `ol_animFreeze`.
- GSU: DrawRoomItems calls `ItemTilt` for every entity (the 32-byte block:
  a far call, a branch over the pad); it finds the slot's record and
  `ShadowRender` builds the corners, then: TransformPoint per corner,
  `ShadowProject` (exact depth, RECIP_TAB interpolated) offset to the
  renderer's bucketed projection of the centre, edges into per-line spans,
  spans darkened (RPIX, remap, PLOT) inside the room's clip. A corner at
  the near plane or an edge taller than 255 lines: none that frame.
- Found on the way: GSU `from r0 / and r6` writes r0 -- a bit test needs
  `to` a scratch register. And the renderer's projection is bucketed at
  256 units of depth: fine for walls and her, not for a flat patch.
- The runner's lean test checks hers is published first and spans more
  than three pixels each way; the profile scenes publish four (her, three
  creatures) and hold both gates.
- Scheduling: FX3 now holds a finished frame's publication from three
  queued ticks (was four, where the NMI drops): the shadows' few
  thousand cycles tipped 7,7,7 into a dropped tick through one more frame
  in the window; with the hold both gates keep zero drops (fps dips a
  step in the heaviest stretch, upstream's own trade).

## Done: 2026-09-26, her lean (FX3)

- Upstream's `angle.z`: `s_rotate(LARA_TURN_FAST, 1)` in the run leans her
  1.5 degrees a tick into the turn, to 11; `s_turnUW` twice that; the land,
  surface and underwater updates take 1, 2 and 2 degrees back a tick (and
  clamp at 22 under water); a side wall eases it, the run's wall and every
  water-state change zero it. All in `lara.c`/`collision.c` under OL_FX3,
  as `olLaraAngleZ`.
- Drawn: ItemsExchange writes its top byte to GS_ROLL ($F732); the
  32-byte pitch block in DrawRoomItems became, byte for byte, a slot-0
  test and a far call to `ItemTilt` at the program's end (X by her pitch,
  then Z by her lean, each only when not zero) -- every tuned routine keeps
  its address. Frame model +275 cycles, CRC 2217f0ab. The runner's lean
  test runs her left: -2002 at most (11 degrees), GS_ROLL $F8, and
  straight again after.

## Done: 2026-09-26, her pitch drawn (both variants)

- Swimming, diving and climbing out, Lara's `angle.x` was moved along and
  never drawn: the GSU turned an entity by its yaw alone. Upstream draws
  `matrixRotateYXZ(angle.x, angle.y, angle.z)`; `DrawRoomItems` now turns X
  after Y by GS_ITEM byte 11 (the old pad), the top byte of her pitch, which
  ItemsExchange writes into her record after the block move -- the same
  completed-frame snapshot as her head and torso, eight bytes of host
  assembly, nothing per tick or per record. Zero skips the rotation, so the
  opening frame is byte-identical (eb23d3da / 2217f0ab).
- `gsusim.py --lara-pitch` renders her pitched; the LEVEL2 water scene checks
  every frame handed to the GSU while she swims up carries her pitch.
- The lean (`angle.z`) followed on FX3; see its entry.

## Done: 2026-09-26, the camera's framing, fixed cameras and camera targets

- **Framing by state (both variants):** upstream's handlers set
  `camera.targetAngle`, `targetDist` and `center` every tick -- hanging and
  sliding look down (-60/-45), a back jump and a reach swing the view round
  (135/85), pickups, levers, keys and puzzles frame her from the side at
  1024, blocks and climbing out centre on her box, the surface pitches 22
  down. `camera.askAngleX/Y/askDist/center` (appended to Camera), set in
  lara.c's handlers and spent by the next update, as Camera::update resets
  them. `center`'s rotated box middle is FX3's (GSU-2 has 28 bytes left in
  bank 5 after the rest).
- **Fixed cameras and targets (FX3):** CAMERA.BIN (the level's cameras, and
  per trigger its ACTIVATE_CAMERA/CAMERA_TARGET, placed by mkbanks after the
  sphere data) and `src/camera_fix.c` (bank $BE): upstream's checkCamera,
  checkTrigger's two arms with the camera ONCE bits, updateFixed (the camera's
  own place, clipped, the whole way at once, its timer to -1), lookAt (her
  head turns to the target two degrees a tick, torso with it, once per item
  through ITEM_FLAG_ANIMATED, and the native updateLook holds the head while
  camera.lastItem is set) and Camera::update's tail with lastFixed's snaps.
  The view's room is the camera's own; `Lara_toCamera` resolves from it.
- **Order:** upstream runs a trigger's camera part inside checkTrigger --
  from a landing, and at the end of Lara::update after updateWeapon's
  toCombat. The port's checkTrigger precedes the weapon update, so it only
  notes the trigger (and whether its type let the actions run); the camera
  update replays up to two with the modes upstream had at each: LOOK before
  both, COMBAT before the late one.
- **The gate:** the first cut cost the three-creature clock gate a tick
  (Camera_updateFollow +4.4 K master clocks, the tick +7.6 K). Everything new
  now sits behind `camera.ask` (a state asked this tick) and `olCamBusy` (any
  fixed-camera state not at rest): 300/0 and 296/0 again.
- **Packer fix:** a trigger list ending on a camera command takes its end bit
  from the camera's second word (upstream's FD_SET_END); buildTrigger read
  the first word's and parsed on into the next sector's data. FX3 now keeps
  LEVEL1's eight look-at-only triggers, which renumbers the rest (the sector
  query scene's trigger 4 is 12 there).
- Runner, `OpenLara FX3: camera triggers`: lever 52 cuts to camera 0 aimed at
  target 41 the tick it fires, exactly at the camera for exactly 90 ticks
  with the target's middle snapped, and back to her in one tick; the pull
  before it frames her at 1024 (distH 928); standing in room 4 she turns her
  head to target 4 two degrees a tick and holds it, and never again after
  stepping away and back. Found on the way: `lookAt`'s differences must be
  sixteen-bit (item 4 is past the z seam). Motioncheck's render-snapshot
  scene expected record byte 10 unchanged; it has been the room since
  2026-09-24.
- Approximations: a fixed camera's pitch is the GSU's projection shear,
  which steep views stretch; the fixed target's middle and lookAt's use the
  animation-union box. Not here: OBJECT (heavy triggers), cinematic frames,
  FLYBY, the camera's trigger speed (upstream's is overwritten before use).

## Done: 2026-09-26, blade spheres in play (FX3)

- **Scope:** GBA/32X/3DO define FAST_HITMASK (common.h), so `collideSpheres`
  runs only for TRAP_SWING_BLADE; creature and dart contact by bounds is
  their rule exactly. The blade now cuts on its spheres, as upstream's.
- **The fast path** (`Trap_sphereTest`, `src/trap_fx3.asm`): at pitch 0 the
  matrix chain's row 1 never sees her yaw, so each of her sphere centres has
  its yaw-0 y exactly, and x and z are the yaw-0 centre (kept in sixteenths)
  turned by the yaw's table cosine and sine to within a bound the packer
  proves -- every stored frame at all 4,096 table yaws, 1.5 s at pack time;
  LEVEL2's bound is 1 unit. The blades never turn, so their centres are
  stored exactly per distinct yaw (LEVEL2: one, $C000). A pair is a certain
  hit when its farthest possible distance is inside the radii, a certain miss
  when its nearest is not; a rotation-invariant reach test skips her spheres
  no blade sphere can meet before any product. 2,382 instructions a call.
- **The exact chain** (`Trap_collideSpheres`, native too): GBA's int32 matrix
  chain operation for operation -- rows keep e0-e2 in 16 bits (they stay
  within 16,384 and rounding), e3 in 32 -- for the undecided poses and a
  pitched Lara. 54,408 instructions a call (816-tcc's C was 326,552, 18
  fields); the C chain is gone and FX3 bank $BE got 7.7 KB back ($84 paid
  1.6 KB for both native paths). Products on the PPU's 16x8 multiplier
  ($211B/$211C/$2134): FX3 has no Mode 7.
- **Data:** SPHERE.BIN (164 -> 340 KB on LEVEL2) gains, per stored frame of
  hers, the yaw-0 centres, and the per-yaw blade tables, behind a fast-path
  header at 16410 (`packer/out_SNES.h` buildSphereData, which also asserts
  the headers sit in the data's first bank; `mkbanks.py` exports
  OL_SPHERE_BANK/SIZE to `hostbank.inc` for the assembly).
- **Proof:** `tools/spherecheck.cpp --table` re-proves the whole table with
  upstream's verbatim code in `make check` (1,928 frames x 4,096 yaws, 118 M
  centres within 1; 91 blade ticks exact). Runner: the 546 poses equal
  upstream's getSpheres and mask through the native chain (pitched ones
  included); 3,920 poses near the blade give 716 certain hits and 3,199
  certain misses, all upstream's, 5 undecided; walking 24 hits out to the
  edge of touching leaves 40 of 88 poses to the exact chain, which agrees.
  The swing scene now expects upstream's spheres: 57 cuts where the box
  reached her on 72. FX3 LEVEL2 58, FX3 78, GSU-2 46 passed.
- C_MOTION leaves the blade on its bounds (no native units there), which
  also let that reference ROM link again: the C chain had overfilled $BE.
- Possible refinement: the exact chain could take the blade's centres from
  its table and skip her row 1 at pitch 0 (y is exact) -- about half the
  products -- if the rare undecided pose's ~3 fields ever show.

## Done: 2026-09-26, the inventory (FX3)

- `src/inventory.c` (bank `$BC`): upstream's `Inventory` -- slots, states,
  pages, the opening/closing/spin/select/show animation targets, counts
  and upstream's starting medipacks, `useItem` for the medipacks and the
  pistols, `useSlot` handed to a hole the tick after the ring closes.
  Select opens it (alive); the game ticks stand still while it is open.
- The ring: `src/inventory_draw.c` builds each item's model-view matrix in
  upstream's 1.14 on the PPU multiplier; `INV.BIN` (packer) names the
  models; ItemsExchange copies the list to `GS_INV`; `mesh.gsu` FrameBody/
  InvRender draw it (own projection table at `$70:3200`, 23/32 across).
- Text: packer `buildFont` (110 trimmed glyphs, 15 k-means colours, pixels
  in `LEVEL.BIN`, `FONT.BIN` metrics in CPU ROM), `src/inventory_text.c`
  (`drawText` with upstream's tables and strings, level key/puzzle names
  by LevelID), GSU `TextRender` (GETC under a frozen high nibble, CG-RAM
  160-175). Title, arrows, name, count; the medipack health bar as HUD
  sprites.
- Background: GSU `BgCapture` reads the game's last frame back with RPIX on
  the ring's first frame, greys it by the packer's `palGrayRemap` table
  into `$70:3400`, and every ring frame plots it back.
- Keyholes/puzzle holes (FX3): Action opens the USE page with the hole's
  item chosen; the chosen one fits or SND_NO. The LEVEL2 key scene goes
  through the ring.
- Passport and death page: Select while dead opens INV_PAGE_DEATH; the
  passport's Load Game lists this level and restarts it (`olRestartLevel`:
  C runs with DBR $7E, `RestartLevel` needs 0), Exit does the same, Save
  Game shows upstream's failed-save warning worded for no back-up RAM.
  `drawOptions` via GSU `RectRender` (solid lines; colour 0 darkens by a
  second packer remap). The death runner test goes through it on FX3.
- Headroom: the three-creature gate (7,7,7) lost its tick to the extra
  render-sync work. ItemsExchange's 12-byte-per-entity MVN (56 clocks a
  byte) became a B->A DMA from the WRAM port in 128-byte pieces (8 a byte,
  NMI held off under a line each): ~47 K master clocks back per frame.
  The inventory's own per-tick hook is gated on its state inline.

## Done: 2026-09-25, death and restart (both variants)

- Upstream opens the inventory on Select, on the death page if she is dead;
  its passport's Load Game lists the levels, this one among them, from its
  start. This cartridge has one level and no menu, so a Select pressed after
  she dies restarts it -- `CheckRestart`/`RestartLevel` in main.asm, host
  assembly (GSU-2's C has no room). The press must begin after death, as
  `isKeyHit`: a latch arms on a dead tick with Select up.
- The restart is the boot, so every piece of level state -- host tables,
  mutable sectors, GSU staging and its slot table, creatures' Nav, sound --
  comes back the way power-on builds it. Before jumping: NMI and HDMA off,
  forced blank, GSU stopped (SFR = 0) and both buses the CPU's (SCMR), and
  the sound CPU sent back to its IPL ROM: `spc.asm`'s STOP for source $FF
  keys every voice off, re-enables the IPL ROM and jumps to `$FFC0`, so the
  boot's upload finds `$BBAA` as at power-on. About 2 s of black, the boot's.
- **The driver is full:** 509 of its 512 bytes. The next driver feature needs
  its layout reconsidered (the program and variables fit $0200-$03FF).
  Host assembly +147 bytes (231 free on GSU-2).
- Runner: `restarts the level on a Select pressed after she dies` on both
  variants -- alive, Select does nothing; held through death, nothing; a
  fresh press reboots; she is back at the start with 1000 health and the
  driver in sound RAM byte for byte again. GSU-2 46, FX3 78, FX3 LEVEL2 57,
  GSU-2 LEVEL2 29.

## Done: 2026-09-25, swinging blades (FX3)

- LEVEL2's three blades in room 61 (`src/trap.c`, which `dart.c` became):
  upstream's `TrapSwingBlade` -- SWING while active, back to STATIC when not
  (TR's data jumps from STATIC straight into the swing at frame 30 of 229;
  228 is the wind-down) -- and `collide`: ACTIVE and mid-swing, her room's
  neighbours within 4096, bounds, then the blood splash's four `rand_logic`
  draws (the creatures share that generator; the splash waits for effects)
  and hit(100). Health is tested once before the pass, as upstream, so a
  second blade after a fatal first still draws. Level init lists the blades
  (`olBlades`); her collision pass asks only when there are any.
- **Approximation:** the current-frame box of a blade mid-swing holds a lot
  of air, and GBA's spheres -- the blade is the one type FAST_HITMASK leaves
  them for -- would reject much of it. Standing 1000 along the sweep, 72
  ticks of 200 cut her here. See "Blade spheres" below for where that is.
- Runner: `OpenLara FX3: swinging blades` (LEVEL2) -- untriggered it hangs and
  cannot cut; triggered, every cut lands on exactly the ticks an independent
  replay of the nearest-pose box says it reaches her, each 100 and exactly
  four LCG steps, none after switching off. FX3 LEVEL2 57, FX3 77, GSU-2 45.

## Done: 2026-09-24, dart traps (FX3)

- LEVEL1's ten emitters (rooms 6, 24, 34) and LEVEL2's three fire. Upstream's
  `TrapDartEmitter` (`src/trap.c`): FIRE while active, a dart at the first
  frame of each 30-tick loop, 412 along `getBlockOffset` and 512 up, with
  SND_DART; IDLE and inactive takes it off the active list. `Dart`: flies on
  its animation's 130 a tick, `updateRoom`, removed when the floor under it
  reaches it (a wall's OL_NO_FLOOR read as upstream's WALL). `Dart::collide`
  from her collision pass: 50 a contact tick, the dart flying on, as upstream.
- `ItemObj::add` is a pool: the packer appends two hidden darts per emitter
  (`appendDartPool`, FX3 only), and add takes the first not in flight. Two,
  because a dart lives 31 ticks in room 6 (its floor ramps up to meet it)
  against the 30-tick loop. The entity-rooms slice is what lets them be drawn
  where they fly. The sound plan keeps 151 where there is a pool (sound 18,
  an unnamed animation sound, gave way on LEVEL1).
- ~~Approximation~~ **Correction (2026-09-25):** GBA, 32X and 3DO define
  FAST_HITMASK, and `collideSpheres` returns all-ones for everything but the
  swinging blade: a dart's hit IS its bounds there. 250 a pass (five contact
  ticks) is GBA's own rule, not an over-count.
- `EntBuild` reads ROM inline now (a call a field doubled it): frame model
  +0.78% GSU-2 / +1.35% FX3 against the baselines, pictures unchanged, all
  of it the one-time table build in the one frame the model renders.
- GSU-2: nothing (no resident room); its emitters keep the open-and-shut.
- Runner: `OpenLara FX3: darts` -- the pool, spawn point, 130 a tick, a fire
  every 30 ticks, removal exactly at the floor, 50 a contact tick with the
  dart flying on, and an idle emitter leaving the active list; and the real
  pad in room 6 in full emulation: two darts in flight, SND_DART's sample
  keyed on in the S-DSP. FX3 77 passed, GSU-2 45, FX3 LEVEL2 56.
- Found on the way: the trigger scene's step-off point is emitter 8's pad,
  in its line; a longer stay there now kills her, as upstream would.

## Done: 2026-09-24, entities drawn in the room they are in

- **The gap:** the GSU drew each room's entities from the static placement
  list the packer wrote for it, taking only the model address, bank and slot
  from it and everything else from the host. A creature was drawn only while
  the room it *started* in was being drawn: a wolf that chased her into a
  room its spawn room cannot be seen from vanished.
- **Now:** the host publishes the room in byte 10 of each 12-byte GS_ITEM
  record (`olItemToRenderer` in game.c, `render_sync.asm` on FX3), and
  `DrawRoomItems` draws every slot whose room byte is the room `RR_Room` is
  on (`R_INDEX`), as upstream's per-room item lists after `updateRoom`. The
  model comes from a slot table the GSU builds once per level (`EntBuild`,
  `ENT_TABLE` at `$DC40`, 4 bytes a slot, from the rooms' placement lists;
  the host clears `ENT_READY` at boot). No ROM or host-blob bytes: LEVEL2's
  first bank has 119 bytes of headroom, and a table there did not fit.
- **Cost:** the scan is a cached `loop` over the room bytes that leaves only
  to draw. The opening frame is pixel-identical on both variants (eb23d3da,
  2217f0ab); the model reads +0.97% / +1.46% because the one frame it renders
  is the first, which pays `EntBuild` (~77 K cycles) once -- the per-frame
  scan is cheaper than the old placement walk. Baselines not re-recorded.
  GSU-2 resident C: 58 bytes for the room byte (bank 4: 17 free).
- Runner: `draws an entity in the room it is in, not the room it started in`
  -- the room-14 wolf stood in her room and view appears (318 words GSU-2,
  84 FX3); named back to room 14 in the same place the picture returns
  exactly. GSU-2 45 passed, FX3 75, LEVEL2 29.
- Unblocks darts (§5): a spawned dart can live in a pool slot and fly
  through rooms. LEVEL1 has ten dart emitters (type 40) and nothing drives
  them; LEVEL2 has three more and three swinging blades (36).

## Done: 2026-09-24, Lara's hit reactions (FX3), and the suites' time

- Upstream's `collidePush(lara, cinfo, true)` tail (`animHit`): after a
  creature's push, her offset from before it, less the box centre turned back
  into the world, names the quadrant she was struck from; a box 256 or less
  tall (the bat) does not, nor a state with `cinfo.enemyHit` clear.
  `Lara_hitTick` (SND_HIT on the first frame, `hitFrame` to 34, cleared with
  the five-tick timer; death clears the quadrant) and `olLaraHitPublish`
  (after `gameRenderSync`: HIT_FRONT/LEFT/BACK/RIGHT = 125/127/126/128 at
  `hitFrame`, her own state untouched, as `Lara::draw`). GSU-2 has no
  creature push to hang this from and is byte-identical.
- **Priced against the three-creature gate, which has no headroom.** The
  first version (816-tcc, rotation and `phd_atan` at every push) cost that
  gate a tick; three wolves can push in one tick and a bear pushes every
  tick. Now native (`contact_fx3.asm`): the push only RECORDS its inputs in
  `olLaraHitIn[6]` (offset, box centre from the box `frameContact` leaves in
  r2/r2h on a push, both yaws) and marks the quadrant pending (`$FF`);
  `Lara_hitResolve` does the maths when her pose is published, five times a
  second. The last push wins either way, so it is exact. The tick call is
  guarded on the timer. Gates: 299/0 and 296/0, exactly the pre-slice
  numbers; all of it about 330 clocks a tick. C references in
  `contact_fx3.c` under C_MOTION; `make motioncheck` passes.
- Runner: `turns a creature's push into upstream's hit quadrant...` (1,500
  pushes against a JS replay of `animHit`, yaws scrambled between push and
  resolve; the tick's trace, clamp and death; the drawn animation per
  quadrant); the wolf frame-contact scene asserts only a performed push
  records one, and that the record through the push equals the direct one.
- Suite time: the sprite-pickup scene waited a fixed 5,400 fields for a
  camera that no longer eases per rendered frame (57 s); it now waits for two
  identical captures 120 fields apart (settles at 240; 7 s). GSU-2 `make
  check` 187 s, FX3 367 s. `OL_PROFILE_TOP=n` prints n profile rows.
- Lesson: an experiment that edits a source, builds, and restores it can
  leave the object and the source with the same mtime second, and make will
  not rebuild -- delete the object, and never restore before the check runs.

## Done: 2026-09-23, cold level load and the health and air bars

- **`olLevelInit` in ROM.** It runs once, on the tick that finds the
  staged level, and it was 3,464 bytes of resident WRAM. It now lives in ROM
  bank `$3C` beside `olMathCheck`. On that tick `UpdateDemo` takes the ROM
  bus as well as the RAM bus: RON = 0 parks the GSU at its next ROM access
  (Book II 2-9) and it resumes when RON returns. The bit is cleared in
  `scmrShadow` itself, before `SCMR`, so the NMI -- which rewrites `SCMR`
  from the shadow -- cannot hand ROM back mid-call; the real ROM vectors
  point at the same landing pads as the GSU's dummy ones. TypeSNES models
  both sides (dummy CPU reads under RON = 1, a GSU wait under RON = 0), so
  the unchanged 42-test suite is a real check. Resident C free: 634 ->
  4,098 bytes (before the HUD). The same trick fits anything else that runs only at load.
- **The HUD**: upstream's `drawHUD`/`renderBar` -- the air bar while she is
  at or under the surface, then the health bar while her guns are ready or
  for 40 ticks after her health last changed (100 at the level start;
  frozen while dead). Upstream sets `healthTimer` in `hit()` and the
  medipack path; watching the value catches every damage site, native ones
  included, without touching them. `packer/out_SNES.h` writes `HUD.BIN`:
  both bars' palettes in the exact TR colours (sprite palettes 0/1, CG-RAM
  128-159) and 23 characters (first cell 0-7 filled, middle 0-8, last 0-5)
  shared by both bars. `src/game.c` builds 26 OAM entries (thirteen 8x8
  sprites a bar at upstream's `FRAME_WIDTH - 108, 4`); the NMI copies them
  on DMA channel 1 before the frame chunk when `olHudDirty` is set. Sprite
  characters at VRAM `$6400` (OBJ base `$6000`, above the 64 characters the
  BG1 map covers) on GSU-2, `$79A0` (second name table, 154) after FX3's
  blank character. FX3's 8bpp frame emits `$80 | row << 4` for keyed
  source-zero texels, which is colour 0 of every sprite palette, so the
  boot test's all-black check over 128-255 now checks those sixteen on FX3
  (Mode 7, which has no HUD, keeps the full check). OAM Y n covers BG row
  n: both skip the first line. Cost: 1,046 bytes of C, 109 of `.bss`, 48 of
  host code, 104 bytes of DMA in V-Blank when a tick changed it.
- **`gameInit` in ROM** too (`src/game_init.c`, 551 bytes): it runs before
  the first StartGSU. Both moves are GSU-2 only (Makefile `ifeq ($(FX3),0)`);
  FX3 runs everything from FastROM already. Resident C free after the HUD:
  **3,094 bytes**.
- The HUD first rebuilt its 26 sprites every tick and cost 3.8% of FX3's
  whole CPU profile -- enough to fail the three-creature navigation gate
  (42 navigations of 897). It now rebuilds only when a bar appears, goes or
  changes width.
- **Lesson, now an assembly error**: the host's direct-page `.ENUM` has no
  linker check, and FX3's table pointers start at `$28` (`olRoomBase`). The
  one byte this slice added (`romTaken`, now GSU-2 only) pushed FX3's
  `renderHold` onto `olRoomBase`; every NMI rewrote the room table pointer
  and a collision probe put Lara at y = 0 in the lever scene. `main.asm`
  now fails assembly if the list passes `$28` on FX3 or `$30` (the tcc
  registers) on GSU-2 (`hostDpEnd`).
- Runner: `shows the health bar at the start, hides it, and shows it again
  on a hit` judges the PPU's own picture against CG-RAM -- a full bar at the
  start, all 26 sprites parked after 100 ticks, 75 of 100 columns for 40
  ticks after a 250-point hit; LEVEL2's water scene asserts the air bar on
  top once she is under.
- `make check`: GSU-2 43 passed (66 skipped), FX3 72 passed (39 skipped),
  LEVEL2 29 passed; every previous test unchanged. Frame CRC untouched
  (bc755bd1: the bars are sprites, not frame pixels). Boot at field 111.

## Done: 2026-09-07, sound effects on the SPC700

The one subsystem the README still called "nothing at all". README "Sound"
has the design; the numbers:

- `packer/sound_SNES.h`: LEVEL1 fires 52 of its 71 sound ids (the SOUND
  commands of its animations plus the ten the game names); 32 are kept in
  62,571 bytes of APU image -- footsteps, landing, pistols, hit, scream,
  climb and jump grunts, the wolf's three and the bear's two sounds -- at
  7350 Hz (the named set) or 5512 Hz, cut at a second. LEVEL2 keeps 32 of
  57 in 63,012. The encoder's SNR is 20-44 dB. `SNDTAB.BIN` is 640 bytes.
- `src/spc.asm`: 483 bytes. `src/sound.asm`: 724 bytes in the `$3F` image,
  after the C version came out at 2,344 and the compiled call sites alone
  took 130. Resident C free: 1,816 -> **634 bytes** (45 / 125 / 260 / 39
  / 165); `.bss` 1,369 -> 695. The GSU-2 image is now full for practical
  purposes: §12 first, before any further slice.
- Boot: the IPL protocol moves the image at 48 KB/s; the first tick moved
  from about 0.5 s to 1.83 s (field 110) on GSU-2 and 1.85 s on FX3. The
  runner's boot gate now judges the clock from the first tick and allows
  three seconds; twelve scenes that settled a fixed 120 fields from
  power-on wait for the entity list (`awaitLevel`) instead.
- Two runner tests: the sound CPU's RAM against `spc.bin` and `SOUND.BIN`
  byte for byte, the DSP as the driver leaves it, the port idle; then five
  seconds of running, every voice sampled every field: 13 commands, both
  footstep variants, pitches 845-1029 around the sample's 941. The pitch
  assertion caught a byte swap that zeroed the flags.
- `make check`: GSU-2 42 passed (66 skipped), FX3 71 passed (39 skipped), LEVEL2 28 passed;
  tools 58 Python tests; the C_MOTION oracle build links with the C
  reference in bank $3C. The frame is untouched (crc bc755bd1).

## In progress: 2026-09-06, FX3 native camera movement and ROM-layout guard

Camera_move and camRoomAt now join the existing native camera routines in
`$85` FastROM. Movement retains signed-word subtraction, arithmetic shift
by three and wrapped addition, including sequential x/y/z stores through
unaligned or overlapping inputs. Both full Room_query calls still run in
order; a failed room resolution keeps the input room. The floor/WALL height
test, conditional Room_trace and third room lookup retain their C behavior.
No query is skipped and no cache policy, persistent RAM or hardware protocol
changes. GSU-2 and C_MOTION retain the original C routines.

The independent shift fixture covers every signed delta on all three axes
over 65,536 calls, preserving four far/mirrored input layouts and both failed
queries. Its 263 paired C cases take 38,661 / 117,051 instructions. Another
2,048 complete C movement comparisons use ten far/alias layouts, ROOM/LINK
at both ends of CPU-only `$F8/$F9`, signed origins, full-byte slopes, wall
heights and portal/vertical room transitions. All 5,900 ordered queries,
361 height traces and 261 resolved-room changes agree, at 2,703,094 /
4,403,646 instructions. An independent byte-map simulation also verifies
the camera writes before the first query, including an input at camera+1.

The existing 2,048 query fixture now checks the room-only wrapper's resolved
or fallback room and unchanged output canaries. LEVEL1 takes 956,689 /
1,165,407 instructions. All 384 full follow/snap/cache snapshots still match
C on LEVEL1 and Lost Valley, with 97 cold entries and 270/335 fallback-cache
ticks. LEVEL1 falls from 1,676,617 to 1,529,028 native instructions; the C
reference stays at 2,413,399. The original 131,864 coordinate and 5,376
ordered-trace oracles also pass (`build_fx3_camera_move_l1_oracles.log`,
`build_fx3_camera_move_l3_checks.log`). The first follow run caught an
incorrect trace-source bank: WLA's bank-local WRAM label value cannot supply
its far bank via a shift. The pointer now explicitly carries `$7E`; the
failing log is retained as `build_fx3_camera_move_follow.log`.

The native camera section grows 346 bytes to 1,628 while the two C routines
remove 616 bytes: **270 payload bytes recovered**. Link padding additionally
shrinks by seven bytes, so the reported five-bank
headroom grows by 277 to **44,292 bytes**. Camera_move uses a temporary
28-byte view/query frame and camRoomAt a 20-byte query; the long mixed and
raptor scenes reach `$1EB1`, safely above the unchanged `$1C00` stack floor.
BSS remains 6,707 bytes, Nav 25,290, host state 677 and table pointers/ready
flag 74. No immutable tables move to WRAM or into FX ROMBR.

The size audit also found that budget.py's manually enumerated native list
omitted camera_fx3_native. Six new negative subcases reproduce the accepted
WRAM/SlowROM/cross-bank/reversed placements in
`build_fx3_camera_move_budget_before.log`. All CPU `*_native` sections now
inherit the engine's FastROM-allocation guard, alongside libtcc and roomq.
The fixed native interrupt-vector table is header data and explicitly exempt;
its positive regression prevents confusing it with executable engine code.
All 56 Python tests and fixedcheck pass. The allowed code-bank set remains
the current software allocation, not FX3's hardware address-space ceiling.
Vendor §§1,6,7 concurrent CPU/FX access and distinct ROM windows still apply.

The 3,600-field three-wolf scene passes at **1,800 ticks / zero drops**,
with 559,104 engine clocks/tick and 584,606 inclusive gameUpdate clocks.
The mixed scene remains **1,799 ticks / one drop**, but its engine mean
falls from 577,590 to 569,954, inclusive gameUpdate from 604,954 to 596,738,
and camera mean from 57,123 to 49,285. Its sole drop is still at field 1,736,
now inside Camera_move → Room_trace → traceZ → traceCheck → Room_query.
The unchanged field-1,740 drift sample is five. Owners `[26,27,30]` and
2,456 damage are unchanged (`build_fx3_camera_move_l1_extended.log`).
The preserved JSON/PNGs are `build_fx3_floor_l1/profile_camera_move_wolves_3600.*`
and `profile_camera_move_mixed_3600.*`; the prior bat/Nav traces remain.

Lost Valley improves from 282 ticks / 17 drops to **285 ticks / 14 drops**
over 600 fields, still failing seven-tick drift at field 60. Engine mean is
666,021 clocks (was 674,653), inclusive gameUpdate 698,391 (708,990), and
the first full tick 2,145,572 (2,155,510). Camera mean falls from 62,018 to
52,609. Owners `[32,34,42]` and 500 damage remain. The six camera oracles
pass in the same run, but its timing gate does not
(`build_fx3_camera_move_l3_checks.log`,
`build_fx3_floor_l3/profile_camera_move_raptors_600.*`). The four-slot queue,
five-tick drift bound, five Nav steps and sixteen-NMI publication deadline
are unchanged. These are emulator results, not physical FX3 timing proof;
the camera's approximation caches and union target height also remain.

## Earlier: 2026-09-06, FX3 bat frame contact and navigation cursors

Bat is now the fourth migrated current-frame contact species. Its existing
C controller consumes the collision phase's pre-push FAST_HITMASK latch
instead of recomputing animation-union overlap after navigation. The
20-degree steering, 16-unit flight, original state/animation graph,
two-point attacks and falling death are unchanged. Both native and C
collision phases admit type 9; GSU-2 retains its old union path.

The new cartridge regression covers 200 frame/ghost/eligibility/dead-target
cases at both ends of CPU-only `$F8`, including 80 pre-push contacts, 40
zero-radius pushes and eight two-point attacks. It also rejects 124 ghost
positions taken from original flight/attack frames. The saved pre-change
ROM fails the first attack ghost with health 998 instead of 1000
(`build_fx3_bat_nav_before_regression.log`). Both full update entries match
512 C item/Lara/contact/PRNG/three-Nav snapshots each, including 404 turns,
40 deaths and cold/warm contexts. Native/C instruction counts are
2,520,208 / 6,402,802 through Bat_update and 2,563,812 / 6,551,846 through
Enemy_update (`build_fx3_bat_nav_l1_fullbat.log`).

Lost Valley has no placed bat, so its original-animation bat scenes
are explicitly skipped, like the bear scenes there. Its model-independent
mixed phase still tests all four species: 960 complete C snapshots cover
all 24 three-species owner orders, 770 latches, 173 pushes, far `$7F` Lara,
CPU-only `$F8/$F9` frames and excluded unported types. Another 480 cases
per species retain 112 contacts, 54 pushes and 56 zero-radius room guards.
The first run's five failures were unavailable bat fixtures, not contact
differences (`build_fx3_bat_nav_l3_oracle.log`); the level guard is explicit.
`motioncheck` now selects bats as well as the other creature controllers.

Read-only profiling counted 30,487 Nav neighbor visits in the existing
600-field raptor scene. The native search now retains the box-floor cursor
through unchanged height/weight tests and advances to the adjacent overlap
word; zone lookup also avoids redundant X-to-Y setup. The overlap cursor
is still stack-preserved on every geometry exit. All 324 complete C queue/
cell cases retain search order, 506 changed weights and 92 empty queues,
with independent BOX/ZONE/OVERLAP placement at both ends of `$F8/$FA/$FB`.
The saved old ROM uses 181,965 native instructions for this oracle; the new
one uses 172,936, a 5.0% reduction (`build_fx3_nav_search_before.log`,
`build_fx3_bat_nav_l1_checks.log`). No Nav step, queue or cache policy changed.

The search frees 16 ROM bytes; the bat controller frees 55, and the two
native phase type checks cost ten: **61 net bytes recovered, 44,015 engine
ROM bytes free**. BSS remains 6,707 bytes, Nav 25,290, host state 677 and
direct-page pointers/ready flag 74. Immutable host tables stay in CPU FastROM,
never translated into FX ROMBR or copied into WRAM. Vendor FX3 §§1,6,7
concurrent ROM/SRAM and distinct CPU/FX address windows remain the policy.
GSU-2 rebuilds byte-for-byte at `$64FA` and all four bat scenes pass;
fixedcheck and all 54 Python tests pass. Primary ROMs remain unpromoted.

The final native/C Lost Valley matrix passes **69 selected checks**
(`build_fx3_bat_nav_l3_matrix.log`); Vilcabamba's full suite passes **52**
(`build_fx3_bat_nav_l2_full.log`), including the new bat regression. The
full LEVEL1 suite with C references passes **92 checks**
(`build_fx3_bat_nav_l1_full.log`). Its 600-field wolf/mixed scenes remain
300/297 ticks with zero drops; engine means are 564,963/580,550 clocks,
inclusive gameUpdate means 593,497/610,058. The short profiles are retained
as `build_fx3_floor_l1/profile_bat_nav_wolves_600.json` and
`profile_bat_nav_mixed_600.json` before extended runs overwrite the defaults.
The additional three-live-bat room-14 workload also passes unchanged at
**296 ticks / zero drops** over 600 fields, all owners `[1,2,3]` retained,
1,128 damage and `$1EBF` stack low (`build_fx3_bat_nav_l1_bat_timing.log`).
The engine mean is 652,560 clocks, inclusive gameUpdate 702,937 and
Bat_update 115,402. The profiler now includes that controller's read-only
span alongside wolf/bear/raptor; no model clock or gameplay state is altered.
The four-tick endpoint backlog fits the existing five-tick bound.

The final 3,600-field run keeps three wolves at **1,800 ticks / zero drops**
and improves the mixed wolf/bear encounter from four drops to **1,799 ticks /
one drop** (`build_fx3_bat_nav_l1_extended.log`). All mixed Nav owners
`[26,27,30]` remain and damage stays 2,456. The field-1,740 drift sample now
meets the unchanged five-tick bound; the zero-drop assertion still fails.
The sole drop occurs at field 1,736, inside Camera_updateFollow →
Camera_move → Room_trace → traceX. Mixed engine mean is 577,590 clocks
(previously 578,167), inclusive gameUpdate 604,954 (606,240), camera maximum
636,610 (644,050), and cold fallback maximum 517,650 (525,090). Both long
scenes' stack low is `$1EBF`. Their JSON/PNG traces are retained as
`build_fx3_floor_l1/profile_bat_nav_wolves_3600.*` and
`profile_bat_nav_mixed_3600.*`; the previous native-camera mixed trace remains
`profile_camera_native_3600.json`. The four-slot queue, five Nav steps and
sixteen-NMI publication deadline remain unchanged.

The unchanged three-raptor timing gate still fails at **282 ticks / 17 drops
in 600 fields**, with seven ticks of drift at field 60. The search cleanup
reduces the engine mean from 676,686 to 674,653 master clocks and waypoint
mean from 46,709 to 45,898; the first full tick falls from 2,171,106 to
2,155,510. All three owners `[32,34,42]` remain and damage stays 500.
The observed stack low is `$1EB1`, above the unchanged `$1C00` floor
(`build_fx3_bat_nav_l3_timing.log`). These small savings do not establish
the remaining 30 Hz or physical FX3 timing requirements.

## Earlier: 2026-09-06, FX3 native camera paths and live timing

The native stalking build's extended mixed encounter dropped five ticks.
Read-only profiling located the first bursts in the camera's sector walks
and fallback search. FX3 now executes traceX/traceZ, their query/check helper,
camQuery and Camera_getLocationForAngle_xz in `$85` FastROM. GSU-2 and
C_MOTION retain their complete C implementations. No new cache, persistent
RAM, WRAM code copy, hardware register protocol or gameplay limit was added.

The walkers keep signed 16-bit coordinate/difference wrap, full 32-bit
slopes and initial products, truncating division, arithmetic product shifts,
both ordered probes at each sector edge, invalid-query behavior and the
old room on a failed probe. Room_query still resolves portals and byte
slopes. The shared native walker check has a private JSL ABI; the public
traceX/Z interfaces and Room_trace's axis order/height clipping are unchanged.
All 5,376 independent/C cases pass, including 29,862 exact ordered queries,
486 room transitions, 615 seam rays and CPU-only `$F8/$F9` room/link tables
at bank start/end. Native/C instruction totals are 15,193,638 / 27,669,741
(`build_fx3_camera_helpers_checks.log`).

The coordinate helper keeps the rounded original sine table, both signed
products and output write order, including aliases with camera.targetDist
or camTarget.z. All 131,864 angle/distance/alias cases pass: every 16-bit
angle and every signed 16-bit distance, plus boundary/alias combinations.
The 2,840 matched C snapshots use 710,406 / 2,234,858 instructions (31.8%).
Another 2,048 query-output cases pass, including 121 failures that preserve
outputs, 850 no-floor translations and overlapping far output pointers.
The combined 1,282-byte native section replaces 6,529 C bytes: **5,247 bytes
recovered, 43,954 engine ROM bytes free**. BSS/Nav/direct-page bounds do not
change. The only new temporary storage is the walker's 50-byte stack frame
and the query wrapper's 20-byte RoomQuery, replacing larger C frames.

The walker-only build still failed the long gate: three wolves 1,800/0,
mixed 1,796/four drops over 3,600 fields. This intermediate result is in
`build_fx3_camera_native_l1_extended.log`; it does not verify the combined
native-camera build. The combined build still fails at **1,796 ticks / four
drops**, with three wolves passing at **1,800/0**. The mixed engine mean
falls from 589,805 to 578,167 clocks and inclusive gameUpdate from 618,542
to 606,240. Camera_updateFollow's maximum falls 1,022,438 → 644,050 and
getBestLocation's measured maximum 886,464 → 525,090. The four drops are at
fields 1,694/1,736/1,750/2,044; field 1,740 still has six ticks of drift,
above the unchanged five-tick bound. All owners `[26,27,30]` remain and Lara
takes 2,456 damage (`build_fx3_camera_helpers_l1_extended.log`,
`build_fx3_floor_l1/profile_camera_native_3600.json`). No timing limit changed.

All 384 complete LEVEL1 C follow/snap/cache snapshots pass, including 97
cold trace entries and 270 fallback-cache ticks, with the target item
unchanged: 1,676,617 / 2,413,399 instructions
(`build_fx3_camera_helpers_follow_checks.log`). GSU-2 rebuilds byte-for-byte
at `$64FA`; fixedcheck and all 54 Python tests pass.

The combined Lost Valley gate improves from 277/20 to **282 ticks / 17
drops over 600 fields**, but still fails the first drift sample. All three
raptor owners remain and Lara takes 500 damage. Engine mean 692,814 →
676,686 clocks; first gameUpdate 2,218,066 → 2,171,106. Its 384 complete C
camera snapshots also pass (97 cold entries, 335 fallback-cache ticks),
with 1,710,005 / 2,440,758 instructions
(`build_fx3_camera_helpers_l3_timing.log`). The broad native/C Lost Valley
matrix passes 63 selected checks (`build_fx3_camera_helpers_l3_matrix.log`).
Cold Nav initialization still averages 350,817 clocks per context; waypoint
work averages 46,709 per creature and the real contact phase 98,896 per
tick. Those measured costs and the remaining mixed-scene bursts are still
timing work, not a reason to reduce creature counts or collision checks.
Final full LEVEL1/LEVEL2 suites pass **67/51** checks, including independent
camera tests on both levels and both unchanged 600-field LEVEL1 timing
gates at 300/297 ticks, zero drops
(`build_fx3_camera_helpers_l1_full.log`, `build_fx3_camera_helpers_l2_full.log`).
Short wolf/mixed engine means are 565,404/581,059 clocks. The minimum stack
pointer across the short/extended encounters is `$1EB1`, above the unchanged
`$1C00` floor. Neither the primary GSU-2/FX3 ROMs nor the hardware-validation
status changed; these candidate ROMs remain unpromoted.

## Earlier: 2026-09-06, FX3 native stalking and extended pursuit timing

The extended test was needed: the bear-contact build passes both 600-field
gates, but its mixed 3,600-field run fails at 1,794 ticks / six drops. The
first drop is at field 1,690, and four of the first five drops occur inside
Camera_updateFollow. The mixed scene also calls the C stalking predicate
2,374 times, averaging 19,131 master clocks. This is a newly exposed
workload, not evidence that current-frame contact should be removed
(`build_fx3_bear_contact_l1_extended.log`).

Enemy_checkStalk now executes natively from `$BD` FastROM. Both box-byte
sums and centre-minus-target deltas remain full width, with inclusive
3072-unit bounds and strict-positive quadrant tests. Lara's signed-angle
quadrant uses her complete far pointer; the opposite-facing case checks
both full-width enemy deltas. There is no new local frame, persistent RAM,
table copy, cache, PRNG roll or altered navigation rule. C_MOTION retains
the original complete predicate and quadrant helper.

The original-predicate fixture passes 131,072 cases: every angle and bound
sum 0..510, inclusive edges, strict zeroes, all five decision outcomes,
signed-32-bit world coordinates, CPU-only `$F8` bank-start/end BOX tables
and far `$7F` Lara pointers. All subtractions are within defined signed-C
range. Another 2,048 C comparisons preserve coordinates, Lara, PRNG and ROM
bytes; matched native/C costs are 178,388 / 1,247,226 instructions (14.3%).
The complete 1,024 original C++ mood/PRNG/Nav snapshots also pass on LEVEL1
and Lost Valley (`build_fx3_stalk_checks.log`, `build_fx3_stalk_l3_checks.log`).
The 239-byte native routine replaces 1,334 C bytes: 1,095 bytes recovered,
38,707 engine bytes free. BSS/Nav/direct-page/stack allocations are unchanged.

The extended three-wolf scene passes at 1,800 ticks / zero drops. The mixed
scene improves to **1,795 ticks / five drops**, but still FAILS at the
field-1,740 drift sample (`build_fx3_stalk_l1_extended.log`). All three owners
remain and Lara takes 2,456 damage. The engine mean falls 605,993 → 589,805
master clocks; inclusive gameUpdate 637,640 → 618,542; stalking 19,131 →
4,051. Camera_updateFollow still peaks at 1,022,438 clocks. First four drops
remain at fields 1,690/1,734/1,740/1,780; the fifth is at 2,038 during bear
navigation. Camera trace/fallback cost is the next timing target, not a
reason to relax the queue/drift/presentation assertions.

Optional `OL_PROFILE_CAMERA=1` now adds read-only nested camera spans.
A separately staged 2,160-field mixed probe reproduces all five drops and
the exact 60-field timeline; its first 600 per-field tick/drop/queue samples
also match the ordinary run. The drop at 1,690 is in getBestLocation →
Room_trace, at 1,734 in Camera_move, and at 1,740 in Camera_move → Room_trace
→ traceZ. The other two occur outside a watched call and during bear Nav.
getBestLocation peaks at 886,464 clocks, Room_trace at 247,912 and traceZ at
136,358. All configured symbols are asserted present; probing does not
modify emulator registers or game state (`build_fx3_camera_probe.log`,
`build_fx3_floor_l1/profile_camera_bear_stalk_2160.json`). Camera code and
its existing cache policy remain unchanged.

Final full LEVEL1/LEVEL2 suites pass 64/48 checks; the full selected Lost
Valley native/C behavior/geometry/memory matrix passes 60
(`build_fx3_stalk_l1_full.log`, `build_fx3_stalk_l2_full.log`,
`build_fx3_stalk_l3_matrix.log`). Short LEVEL1 scenes remain 300/297 ticks
with zero drops over 600 fields. Those short passes do not override the
extended mixed-scene failure above. Fixedcheck also passes.

Lost Valley remains 277 ticks / 20 drops over 600 fields; its original
room-51 encounter still passes. GSU-2 rebuilds byte-identically at `$64FA`,
all 54 Python tests pass, and primary ROMs remain unpromoted. These are
emulator results; actual FX3 firmware/hardware timing remains unverified.

## Earlier pass: 2026-09-06, FX3 current-frame bear contact

Bears now join the existing wolf/raptor pre-update contact phase and consume
the captured current-frame FAST_HITMASK result. Frame boxes remain directly
readable in concurrent CPU FastROM, including the CPU-only final MiB; the
existing per-item latch is the only mutable contact state. The original
zero radius persists through the entire push/room guard. A living bear
ignores contact once Lara is dead, but an already-dying upright bear retains
its captured fall hit even if Lara died after collision. Running still deals
three damage per contact tick; bite/standing attacks latch 200/400 once, and
the reverse flag permits only one 200-point upright fall hit. GSU-2 keeps
its previous union-bound path.

The preserved old ROM deals three unwanted running-damage points outside
the current frame (`build_fx3_bear_contact_before.log`). The new 144-case
check covers CPU-only `$F8` bank start/end, running/bite/attack/death states,
push on/off, hidden/inactive/disabled/distant-room actors, dead targets and
targets killed between contact and update. It observes 48 pre-push contacts,
32 damaging updates, 24 zero-radius pushes and 12 consumed fall latches.
Another 447 positions derived from original running, attack and death poses
hit the old union routine but correctly cause no current-frame damage; both
death animations are included. The real room-17 encounter still deals its
first running contact at tick 11. All eight LEVEL1 bear checks pass
(`build_fx3_bear_contact_checks.log`), including original state decisions,
both attack/death graphs, movement limits and complete C updates.

Both native bear entries match 512 full C snapshots with captured contact,
item/Lara, PRNG, turn/pending state and the owning Nav context. The fixture
now resets Nav between cases so warm/cold ownership is deliberate. Direct
and generic calls use 2,380,517 / 7,407,255 and 2,416,245 / 7,561,523
instructions respectively, with 135 nonzero steering cases each. Shared
contact matches 240 mixed wolf/bear/raptor snapshots in all six owner orders
and 480 full geometry/room-guard snapshots per species. These include far
`$7F` Lara pointers and CPU-only `$F8/$F9` frame data; excluded bats retain
their previous path (`build_fx3_bear_contact_l3_checks.log`).
The expanded Lost Valley matrix passes all 59 selected native/C behavior,
geometry and memory checks (`build_fx3_bear_contact_l3_matrix.log`).
The mixed fixture now explicitly asserts all six species permutations,
not just owner-ID permutations; four focused phase checks pass with that
stronger coverage (`build_fx3_bear_contact_all_orders.log`). Original-pose
regressions also require ghost witnesses in every damage state and both
death animations (`build_fx3_bear_contact_original_l1.log`,
`build_fx3_bear_contact_original_l2.log`).

Full LEVEL1/LEVEL2 suites pass 63/47 checks
(`build_fx3_bear_contact_l1_full.log`, `build_fx3_bear_contact_l2_full.log`).
The three-wolf and mixed scenes finish 300/297 ticks over 600 fields with
zero drops and unchanged five-tick drift bounds. The mixed endpoint has
three ticks of backlog; passing the gate does not make it a 300-tick result.
Their engine means are 577,382 / 592,434 master clocks, inclusive gameUpdate
606,624 / 623,276, and first full updates 1,633,078 / 1,606,966. Stack low
remains `$1E70`. The subsequent extended run failed for the mixed scene at
1,794 ticks / six drops; see the native stalking pass above.

The extra species gate costs ten ROM bytes and the shorter bear update
frees 43: net -33, leaving 37,612 engine bytes free. BSS, Nav, direct-page,
stack and host-table allocations are unchanged. All 54 Python tests and
fixedcheck pass; GSU-2 rebuilds byte-identically at `$64FA`. Primary ROMs
remain unpromoted. Lost Valley's three-raptor gate remains exactly **277
ticks / 20 drops / 600 fields**, with a 692,814-clock engine mean and
2,218,066-clock first update (`build_fx3_bear_contact_l3_checks.log`). Queue,
drift, search-step and publication limits are unchanged. Bat migration,
room-list/whole-game ordering and the remaining timing failure are still
open; emulator timings do not establish real FX3 hardware performance.

## Earlier pass: 2026-09-06, FX3 current-frame wolf contact

Wolves now join the existing pre-update enemy contact phase. Like raptors,
they read current-frame boxes directly from concurrent CPU FastROM and
capture the FAST_HITMASK bound result before pushing Lara. The original
temporary zero radius remains in force through the complete room guard.
Both C_MOTION and the native Wolf_update consume this captured contact in
attack/bite states; their late animation-union retest is gone. GSU-2 keeps
its previous path. No table copy, new assets, host-version change or new
persistent RAM is needed. The extra species gate costs ten ROM bytes and
the shorter wolf update frees 34: net -24, leaving 37,579 engine bytes free.

The preserved previous ROM reproduces an actual ghost hit: state 6 deals
50 damage inside the union but outside the current frame
(`build_fx3_wolf_contact_before.log`). The corrected 64-case regression
covers CPU-only `$F8` bank-start/end tables, both attack states, push on/off,
visibility, inactivity, missing Nav ownership, dead targets and room
adjacency. It checks 16 single-hit attacks and eight pushes, including
contact retained after subsequent separation. Another 62 positions derived
from original TR1 attack/bite poses hit the old union routine but correctly
cause no current-frame damage. These pass on LEVEL1/LEVEL2/LEVEL3A
(`build_fx3_wolf_contact_original_l1.log`,
`build_fx3_wolf_contact_original_l2.log`,
`build_fx3_wolf_contact_original_poses.log`). The real room-14 wolf still
approaches Lara on the slopes and bites for 100.

Both direct and generic wolf entries match 640 complete C snapshots each,
including the latch and 168 nonzero steering cases per entry. They take
2,994,462 / 9,489,386 and 3,036,562 / 9,666,811 instructions respectively
(`build_fx3_wolf_contact_checks.log`). The shared phase matches 240 mixed
wolf/raptor snapshots in all six owner orders, plus 480 full geometry/guard
snapshots for each species (`build_fx3_wolf_contact_l3_checks.log`). Far Lara
pointers, CPU-only ROM tables and rejected pushes remain covered. The
69,888 original C++ wolf decisions still pass.

Full LEVEL1/LEVEL2 suites pass 62/46 checks
(`build_fx3_wolf_contact_l1_full.log`, `build_fx3_wolf_contact_l2_full.log`).
Both LEVEL1 live scenes finish 300 ticks / zero drops over 600 fields,
with unchanged queue, drift and publication limits. Current-frame contact
and pushes increase the three-wolf engine mean to 577,375 master clocks
(606,620 inclusive gameUpdate); the mixed encounter uses 569,744 / 597,558.
The first full updates cost 1,633,078 / 1,614,152 and stack low stays `$1E70`.
The earlier 3,600-field results predate wolf migration and need rerunning.
All 54 Python tests and fixedcheck pass; GSU-2 rebuilds byte-identically at
`$64FA`. Primary ROMs remain unpromoted.

The first expanded Lost Valley run exposed five invalid test selections:
real-bear animation fixtures had no bear model in that level, while 58
behavior checks passed (`build_fx3_wolf_contact_l3_matrix.log`). Those scenes
now explicitly require LEVEL1/LEVEL2; their model assertions remain strict,
and the asset-independent original bear state oracle still runs on Lost
Valley. All seven LEVEL1 bear/C checks and four LEVEL2 bear gameplay checks
remain enabled and pass (`build_fx3_wolf_contact_l1_bears.log`,
`build_fx3_wolf_contact_l2_bears.log`).
The corrected expanded Lost Valley native/C suite passes all 58 selected
behavior, geometry and memory checks (`build_fx3_wolf_contact_l3_verified.log`).

Lost Valley's three-raptor gate remains **277 ticks / 20 drops / 600 fields**
(`build_fx3_wolf_contact_l3_checks.log`), with a 692,814-clock engine mean,
725,452 inclusive gameUpdate and unchanged 2,218,066-clock first update.
The real room-51 encounter still bites at tick 418. Bear/bat contact,
whole-game/room-list collision ordering and the remaining timing failure
are still open. These emulator results do not establish real FX3 hardware
speed or full GBA/32X parity.

## Earlier pass: 2026-09-06, FX3 ROM-backed PRNG and creature dispatch

The exact logic LCG now expands its old seed into four bytes and reads a
256-entry uint32 constant-product table, instead of ten CPU hardware
multiplies. `mktables.py` generates explicit low/high uint16 words; the 1 KiB
table stays in concurrent CPU FastROM at `$BD`, never WRAM or FX-owned
storage. Shifted contributions preserve low-word carries and discard only
bits above bit 31. The original `$41C64E6D/$3039` constants, seed and 15-bit
return sequence are unchanged. There is no new local stack frame and no
arithmetic-register access. GSU-2 and C_MOTION omit the table entirely.

The 8,192 independent seeds plus 1,024 uninterrupted rolls match C/uint32:
608,256 native instructions versus 4,219,782 C (14.4%). Compared with the
previous native implementation, this is 66 rather than 178 instructions per
roll. A separate check verifies all 256 products and 131,072 full seeds,
covering every low and high word with the table at `$F8:0000/$F8:FC00`.
These are not all 2^32 seeds. The fixture explicitly edits the cloned ROM
operands and reads every edit back; the normal CPU write path stays
read-only. An initial attempt used ignored ROM stores and was discarded as
bank-relocation evidence. The corrected fixtures pass on all three levels
(`build_fx3_prng_dispatch_verified_fixtures.log`,
`build_fx3_prng_dispatch_l1_reference.log`, `build_fx3_prng_dispatch_l2_verified.log`).

`enemy_update.asm` replaces the generic C dispatch wrapper with a 187-byte
native entry. Inactive creatures release their Nav slot before deactivation;
failed admissions become invisible and dirty; admitted invisible creatures
become active and dirty before their species update. Dead-but-active
creatures still reach their death animation. Original item/Lara arguments
and return addresses survive the tail calls. There is no new persistent RAM
or local frame. C_MOTION keeps the complete original wrapper.

With species test doubles explicitly patched and read back, 840 C comparisons
match complete item/Nav/list/dirty/turn state and call order: 480 species
calls, 210 releases and 150 refusals, including item slots 1/127 and far Lara
arguments. They use 1,080,135 / 1,938,702 instructions. Real controller tests
also run through both the direct and generic entry: 640 wolf, 512 bear and
640 raptor snapshots per entry retain the original targets, steering, attacks,
death, PRNG and Nav state. The Lost Valley suite passes 47 behavior checks
(`build_fx3_prng_dispatch_l3_verified.log`), including the unchanged tick-418
room-51 encounter. Seven LEVEL1 C-reference checks pass; full LEVEL1/LEVEL2
suites pass 61/45 checks, with both LEVEL1 creature scenes at 300 ticks and
zero drops over 600 fields (`build_fx3_prng_dispatch_l1_final.log`,
`build_fx3_prng_dispatch_l2_final.log`).

The table/shorter PRNG adds 774 ROM bytes and native dispatch frees 444:
net +330, leaving 37,555 free across the five engine banks. BSS, Nav,
direct-page and stack-region bounds are unchanged. All 54 Python tests and
fixedcheck pass (`build_fx3_prng_dispatch_python.log`,
`build_fx3_prng_dispatch_fixedcheck.log`); the placement test now rejects a
WRAM-linked native dispatcher. GSU-2 LEVEL1 rebuilds byte-identically at
`$64FA` (`build_gsu2_prng_dispatch.log`). Primary ROMs remain unpromoted.

The same three-raptor clock gate still FAILS: **277 ticks / 20 drops / 600
fields**, up from 273/24 (`build_fx3_prng_dispatch_l3_final.log`). Engine work
averages 692,811 master clocks/tick (previously 708,104); inclusive gameUpdate
725,443, Raptor_update 128,417 and rand_logic 1,479 (previously 4,219).
First gameUpdate still costs 2,218,066 clocks; the three cold Nav_init calls
average 350,814. The first 60 fields still lose four ticks. Queue capacity,
five-tick drift, five search steps and publication deadline are unchanged;
stack low is `$1E70`. These emulator timings do not establish real FX3
hardware speed or full GBA/32X gameplay parity.

## Earlier pass: 2026-09-06, FX3 animation commands and activation order

The native animation dispatcher handles per-frame effects without entering
the C command loop. End-of-animation offset/jump/empty/kill commands tail-call
the unchanged C body with the original ABI; C_MOTION still uses that body
for both phases. Effects retain the original captured command list/count,
but later commands read any newly changed frame. NORMAL remains Lara-only
by full pointer identity, with model-relative animation 11 and the existing
animation-range/empty-record guards. ROTATE_180 and HANDSFREE keep their exact
word and byte semantics. (SOUND joined this native dispatch on 2026-09-07,
`AC_Sound`, calling `Sound_playItem`.) All tables stay in concurrent CPU FastROM; only a
NORMAL call uses six additional saved stack bytes.

An accompanying parity fix moves FX3 health construction into olLevelInit,
before full-mask items activate. ItemObj_activate now applies upstream
Enemy::activate's health <= 0 rejection before active-list insertion or Nav
admission. Re-triggering a corpse can no longer acquire a context or evict
a living owner. An already-active living creature still calls enable and
can retry a previously evicted slot, without duplicate list insertion.
Non-creature objects retain their normal activation, even
at zero health. GSU-2 retains its existing initialization/activation path.
The new checks fail on the preserved pre-change ROM (`build_fx3_activation_before.log`):
an initial creature reaches activation at zero health, and a dead wolf
mutates the active list/Nav. The final 720-case check passes on the corrected
build: 288 corpse rejections, 144 live admissions, 96 active-owner retries,
72 non-creature activations and 120 idempotent already-owned/non-creature
calls (`build_fx3_anim_fx_final_l3.log`).

The new command oracle covers 2,048 complete C snapshots: 144 NORMAL calls,
364 angle changes, 120 hands-free outcomes and 256 end-command lists. It
includes 255-command lists, noncanonical nonzero fx values, full-word frame
mismatches, invalid/empty animation destinations, far item pointers and
independent `$F8/$F9/$FA` bank-start/end tables. It takes 1,556,286 / 7,734,902
instructions. All 14,964 real-animation frame/command/pending-state snapshots
also match C (3,993,673 / 12,419,604 instructions). The combined Lost Valley
suite passes 44 behavior checks (`build_fx3_anim_fx_final_l3.log`), including
the unchanged original tick-418 encounter and all 640 complete Raptor_update
snapshots (now 3,202,106 / 9,242,939 instructions).
Full LEVEL1/LEVEL2 suites pass 60/44 checks
(`build_fx3_anim_fx_final_l1.log`, `build_fx3_anim_fx_final_l2.log`), with both
LEVEL1 creature scenes retaining 300 ticks and zero drops over 600 fields.

The dispatcher plus constructor/activation fix adds 182 engine ROM bytes:
37,885 remain free, with no new BSS, Nav allocation, direct-page slots or
stack-region changes. All 54 Python tests and fixedcheck pass
(`build_fx3_anim_fx_python.log`, `build_fx3_anim_fx_fixedcheck.log`), and
GSU-2 LEVEL1 rebuilds byte-identically at `$64FA`
(`build_gsu2_anim_fx_final.log`). Existing primary ROMs remain unpromoted.

The unchanged three-raptor timing gate still FAILS at **273 ticks / 24 drops /
600 fields** (`build_fx3_anim_fx_final_l3.log`), versus 271/25 before this pass.
Engine work averages 708,104 master clocks/tick (previously 719,488),
inclusive gameUpdate 742,168 and Raptor_update 131,703. ItemObj_animCmd now
averages 1,743 over 777 calls; its 34 end-command calls average 4,802 clocks.
The initial three Nav_init calls still average 350,823 and first gameUpdate
costs 2,246,102. The queue, drift, search-step and publication limits are
unchanged, and both startup and later drops remain visible. Stack low is
`$1E70`. These are emulator timings, not real-FX3 or end-to-end parity proof.

## Earlier pass: 2026-09-06, FX3 full room-collision wrapper

`ItemObj_collideRoom` now uses native 65816 coordinate setup and collision
decisions; C_MOTION retains the complete C body. The C static-mesh response
is still called after all requested probes. The original zero radius used
by enemy push guards skips only trigonometry/coordinate arithmetic: m/f/l/r
still execute in order, preserving each query's room hint and the distinct
non-middle stopOnSlant behavior. Front-only mode still omits only l/r.
Signed products stay 32-bit until shifting; all seven collision results,
movement-offset precedence, side padding and far-pointer banks are checked.

The native frame is 30 bytes versus C's 96, with no new BSS or direct-page
allocation. Replacing C setup/decisions and its private alignOffset frees
2,672 ROM bytes: engine banks now have 38,067 free, general BSS 7,629 free.
Code remains CPU FastROM during FX execution. No FX memory-map assumption,
ROM-to-WRAM copy, renderer handoff or table-format change was introduced.
The bank-seam fixture keeps C objects within one bank, including its last
valid address: 816-tcc wraps member-pointer addition at 64 KiB, whereas the
independent ROM tables still use full CPU-only `$F8/$F9/$FA/$FB` pointers.

The unchanged three-raptor gate still FAILS: **271 ticks / 25 drops / 600
fields** (`build_fx3_room_native_live.log`), versus 265/31 immediately before.
Mean room collision falls from 79,529 to 61,825 master clocks (22.3%).
Engine work averages 719,488 clocks/tick; inclusive gameUpdate 752,886,
Lara_update 221,396 and Enemy_collideLara 98,108. First gameUpdate remains
2,246,102, with three cold Nav_init calls averaging 350,823. Startup and
later drops remain visible; the queue, drift, five-step Nav and publication
deadlines are unchanged. This is emulator evidence, not hardware timing or
a playable Lost Valley claim. Existing primary ROMs remain unpromoted.

The new 2,048-scene C comparison covers all seven collision types, 293
zero-radius guards, 410 front-only probes, 721 static hits and 58 zero-radius
cases with distinct m/f floors. It compares every field/canary, the complete
probe argument sequence and successive room hints, including terminating
and cyclic portals, vertical links, bridges, extreme slopes, wrapped heights,
all quadrants and the C default arm. It uses 5,820,026 / 9,478,086 instructions.
The Lost Valley native/C suite passes 36 checks including this fixture and
the unchanged original tick-418 encounter (`build_fx3_room_native_l3_differential.log`).
Full LEVEL1/LEVEL2 suites pass 58/42 checks (`build_fx3_room_native_l1_full.log`,
`build_fx3_room_native_l2_full.log`), with both LEVEL1 live scenes at 300
ticks / zero drops / 600 fields. All 54 Python tests and fixedcheck pass
(`build_fx3_room_native_python.log`, `build_fx3_room_native_fixedcheck.log`).
GSU-2 LEVEL1 rebuilds byte-identically at `$64FA`
(`build_gsu2_room_native_unchanged.log`). Read-only inclusive counters now
also expose Room_collideStatic / olStaticHit in the profiler. The final
rerun is unchanged at 271/25; those two means are only 3,974 / 1,448 clocks
over 376 calls, so static response alone cannot close the remaining gap.

## Earlier pass: 2026-09-06, FX3 navigation hot path

The original ordered search now keeps its overlap cursor in Y across
visited neighbours. Only geometry/queue work saves and restores it (two
stack bytes); all exit paths restore the cursor. Overlap, zone and box
reads still use independent full CPU FastROM pointers, including CPU-only
banks, with no immutable-data copy in WRAM. Identical FULL cell weights
take an early exit; equal generations with differing blocked flags still
follow the original propagation/reopening rules. A cell pointer is saved
only when that neighbour could change, and the hot return branch is short.

Nav_expand reuses context, selected zone and signed step/drop setup within
its existing five-search batch. The public Nav_search entry initializes
them; the internal Nav_searchNext continues only that batch. There is no
cache across calls, retargets, creatures or flips. NMI preserves registers
and does not enter Nav or use its pseudo-register scratch. The first
waypoint box's bounds are no longer calculated twice. Total engine ROM grows
by five bytes: 35,395 free, general BSS still 7,629 free, all existing Nav,
direct-page and stack boundaries unchanged.

`build_fx3_nav_batch_oracles.log` passes 864 original C++ full Nav snapshots,
the new 324 direct C search snapshots, the original room-51 encounter at
tick 418 and 640 full Raptor_update snapshots. Mean waypoint instructions
in the original graph fixture fall from 1,184 to 1,004 (max 10,014 to 8,794).
The direct search fixture covers all pairs drawn from nine full weights,
all three pools, four counts through 1,024, both flips, independent
`$F8/$FA/$FB` bank-start/end table placements, duplicate/invalid neighbours,
signed floor overflows and both marker bits in the terminal overlap. Its
506 changed weights and 92 empty queues match C exactly; it takes
181,965 / 2,318,849 instructions. A subsequent observed-batch run executes
1,281 search steps across the 864 waypoints and reaches, but never exceeds,
the original five-step maximum (`build_fx3_nav_batch_live.log`).

The unchanged three-raptor gate still FAILS: **265 ticks / 31 drops / 600
fields**, versus 253/43 before this pass. Mean engine work is 742,851 master
clocks/tick (previously 778,887), inclusive gameUpdate 778,507 (816,168),
and Nav_getWaypoint 48,154 (60,433). The first tick still costs 2,262,290;
its three cold Nav initializations remain about 350,839 apiece, and drops
continue after startup. No queue, drift, search or publication allowance is
relaxed. The live renderer continues at 2–5 displayed fps after startup;
this remains emulator evidence, not real-FX3 or end-to-end Lost Valley proof.

Optional read-only `OL_PROFILE_NAV_BRANCHES=1` counters now accompany the
profile JSON. This run has 30,126 neighbour visits, 10,882 differing full
weights and 9,605 geometry checks. Nav_searchNext counts all logical search
bodies, including the first step that falls through from Nav_search: do NOT
add both inclusive timing series. The existing primary ROMs remain unpromoted.

The combined Lost Valley suite passes all 29 selected checks
(`build_fx3_nav_batch_l3_differential.log`); all 54 Python tests and host
fixedcheck pass (`build_fx3_nav_batch_python.log`, `build_fx3_nav_batch_fixedcheck.log`).
GSU-2 LEVEL1 rebuilds byte-identically at `$64FA`
(`build_gsu2_nav_batch_unchanged.log`).
Full LEVEL1/LEVEL2 suites pass 58/42 checks (`build_fx3_nav_batch_l1_full.log`,
`build_fx3_nav_batch_l2_full.log`), with both LEVEL1 live-creature scenes
still at 300 ticks / zero drops over 600 fields.

## Earlier pass: 2026-09-06, fused/native FX3 creature contact

The contact tick now averages 103,512 master clocks, down from 180,049 in
the initial native-helper build below. `ItemObj_frameContact` reads the
actor/Lara frame boxes once, tests the original strict vertical and inclusive
expanded bounds, then reuses the angle and full-width local point for the
optional unexpanded push. A native owner loop replaces the remaining C
iteration/filter/room-guard overhead. Its allocation-free no-raptor entry
is unchanged. The fused helper and owner loop use 26 and 10 stack bytes;
no persistent scratch or immutable table is added to WRAM. The full native
contact section is 1,600 bytes, and replacing the C owner loop recovers 177
bytes overall: 35,400 engine-ROM bytes remain free, BSS still has 7,629 free.

An upstream source re-read found a separate fidelity bug: Enemy::collide
sets the radius to zero around the ENTIRE collidePush, including its room
probes, then restores it. The previous port zeroed only the geometric box.
Both native and C implementations now keep the radius zero through the
room guard. Tests observe the actual call, requiring the creature pointer,
height 762, offset 0, gaps 32512/-384/0 and radius 0, with radius 100 restored
afterward. This is a source-contract fix, not a timing-fixture adjustment.

`build_fx3_frame_native_phase_checks.log` passes the original-data readers,
3,600 independent full-width fused/disabled/push cases (588,039 versus
1,224,746 instructions for the separate native-helper C paths), the original
room-51 encounter at tick 418 and 480 complete native/C collision snapshots.
Those 480 include 112 contacts, 54 accepted pushes and 56 observed zero-radius
room guards, using 445,082 / 1,631,909 instructions. Another 240 snapshots
exercise all six owner orders, invalid owners/statuses, a far-bank Lara,
CPU-only frame ROM, extreme radii and accepted/rejected pushes. This preserves
the current C owner's iteration order; matching upstream room-linked-list
collision ordering is still separate parity work.

The combined Lost Valley suite passes all 28 selected behavior, geometry,
memory and navigation checks (`build_fx3_frame_fusion_l3_differential.log`).
The 240 multi-owner cases report 191 final latches and 44 pushes, taking
526,250 / 1,918,702 instructions, with CPU-only `$F8/$F9` table placements.
All 54 Python tests and host fixedcheck pass (`build_fx3_frame_fusion_python.log`,
`build_fx3_frame_fusion_fixedcheck.log`). GSU-2 LEVEL1 still rebuilds byte-for-byte
identically at `$64FA` (`build_gsu2_frame_fusion_unchanged.log`).
Full LEVEL1/LEVEL2 suites pass 58/42 checks (`build_fx3_frame_fusion_l1_full.log`,
`build_fx3_frame_fusion_l2_full.log`), including both unchanged 600-field
LEVEL1 creature gates at 300 ticks / zero drops.

`build_fx3_frame_native_phase_live.log` still FAILS the unchanged real
three-raptor timing gate: **253 ticks / 43 drops / 600 fields**, improved
from 241/55 below. Mean engine work is 778,887 master clocks/tick, and the
inclusive gameUpdate mean is 816,168. Its first tick remains 2,338,070 with
three cold Nav admissions; dropping ticks is also a steady-state problem.
The four-slot queue, five-tick drift bound, five search steps and sixteen-NMI
publication deadline are unchanged. Rendering continues (2–5 displayed fps
after startup), and the final PNG visibly contains a raptor, but this is not
an end-to-end Lost Valley or real-FX3 hardware proof. Primary ROMs are not
promoted to this unfinished preview.

### Earlier current-frame contact implementation

OLH2 version 5 extends ANIM_BOX with tick maps to deduplicated six-word
current-frame boxes for every valid model, including Lara. The old union
prefix remains for unmigrated interactions/species. The table is immutable
FastROM: LEVEL1/LEVEL2/LEVEL3A use 57,314/63,346/59,880 bytes. The full host
images are 160,918/166,889/185,068 bytes; mutable sector staging is unchanged.
Build/parse tests validate all frame record/count/range/alias rules.

GBA/32X's FAST_HITMASK first tests current-frame bounds, records contact,
then pushes Lara out of the unexpanded creature box. The missing push was
the cause of the real room-51 raptor's permanent BITE loop: changing boxes
alone did not fix it. The runtime now performs that contact phase for enabled
raptors, filtering by original direct room portals and strict 4096-unit
proximity. Portals are read from the existing renderer ROM via FX3's CPU
mapping, not staged. The original push room guard accepts or restores Lara's
position; the contact latch precedes either result. Raptor_update consumes
the latch instead of recomputing overlap after Lara has moved. This fixes
the original placement regression: 100 damage at tick 418. Health, speed,
animation transitions and bite radius are not forced to make it pass.

Native FastROM readers, signed products, full-width range comparisons and
an empty-species fast path retain C references. A stable nearest-edge minimum
avoids 816-tcc's wide short-circuit bug; validated coordinates/edge distances
then safely narrow to signed/unsigned words. Push locals shrink from 352
(unassemblable) to 48 bytes. The C portal oracle explicitly constructs the
far pointer's two words to avoid the compiler's lost-bank cast. Engine ROM
has 35,223 bytes free; the sole persistent addition is a 128-byte contact
latch array. General BSS has 7,629 free; Nav/DP/stack bounds are unchanged.

Proof: `build_fx3_frame_contact_differential.log` passes all four tests:
7,681 raw-source frame selections, 1,668 CPU-only `$F8` bank-start/end reads,
8,463 source adjacency pairs, 1,200 full-width contact/push cases and 480
complete native/C collision snapshots (112 contacts, 54 accepted pushes,
825,830 / 1,625,618 instructions). The final combined suite
`build_fx3_frame_differential_final.log` passes 27 checks, including the
original encounter, three attack latches, both deaths, 640 complete
Raptor_update snapshots (3,293,599 / 9,242,939 instructions), original C++
states, slopes, Nav and host-table rebinding. An initial contact-fixture
coverage guard caught a deliberately blocked sector not being restored
between cases; restoring it produces the 54 accepted pushes above.

The first C collision loop caused one LEVEL1 mixed-scene drop. The native
empty-species gate restores both 600-field scenes to 300 ticks / zero drops
(`build_fx3_frame_l1_native_timing.log`). The final full LEVEL1/LEVEL2 suites
pass 58/42 checks (`build_fx3_frame_l1_final.log`, `build_fx3_frame_l2_final.log`).
All 54 Python tests and host `fixedcheck` pass; GSU-2 LEVEL1
rebuilds byte-identically at checksum `$64FA`.

The full LEVEL1 run also exposed an existing field-boundary setup race in
the long trigger test: an in-flight Lara room query restored room 2 after
the fixture teleported her to a room-9 door. Its setup now runs at actual
gameUpdate entries. Both FX3 and unchanged GSU-2 then push her 178 units
through the door's border into room 10 while shut, and zero while open;
switch direction and latch assertions remain unchanged. Focused proofs are
`build_fx3_frame_trigger_boundary.log` and `build_gsu2_frame_trigger_boundary.log`.

Still open: `build_fx3_raptor_contact_native_live.log` has **241 ticks / 55
drops / 600 fields**, versus 214/82 for the first frame-contact C path.
Mean engine work is 838,167 master clocks/tick; contact itself averages
180,049 including bound checks, pushes and room probes. The first tick is
2,338,070, with the unchanged three Nav initializations averaging 350,839.
Unlike the older no-push scene below, dropped ticks are not just a startup
problem. Keep the queue/drift/search/publication limits unchanged. This is
not an end-to-end Lost Valley or hardware timing proof, and the established
primary ROMs have not been promoted to this preview. Exact whole-game update
ordering, multi-creature collision ordering, other species' contact and
rendered hit/head/tilt effects remain parity work.

## Earlier baseline: Lost Valley byte slopes and FX3 raptor graph

The original LEVEL3A pack failed at FloorData 765: slope `(0,-39)` was
outside the host's signed five-bit encoding. FX3 OLH2 version 4 stores full
signed-byte X/Z pairs in the existing LINK words. The quarter-scaled query
arithmetic now handles 17-bit products exactly: narrow slopes use shift/add
with rounding folded into the accumulator; wide slopes combine two CPU
byte products, respecting the eight-cycle latency. No extra mutable data
is required, and GSU-2's encoding/ROM remain unchanged.

Lost Valley now builds: 91 rooms, 6,841 sectors, 995 Nav boxes, 64 placements
and **100,954 bytes of independently banked host tables**. Only 27,364 bytes
are staged in `$7F`, leaving 38,172. An audit exports raw original sectors
and FloorData, then checks immutable ROM records, not floors subsequently
changed by block initialization. All 8,448 signed-byte point/cache/far-pointer
queries pass, including 1,409 cases needing a 17-bit product. The first slope
implementation dropped one LEVEL1 mixed-scene tick; folding rounding into
the narrow accumulator restores both existing 300/0 gates without relaxed
limits.

Raptor decisions/update now have native `$BE` FastROM implementations
(412/606 bytes), complete C references and an extracted original C++ state
oracle. The controller preserves the original pending-state and random
ordering, 400-unit head offset, previous 1/4-degree steering and close-turn
test, one-click step/drop limits, three 100-point bite latches and immediate
death selection on the lethal pistol hit. Like the original `animSet`, death
selection preserves `nextState`; a final source comparison and regression
assertion caught and removed an unnecessary clear. Repeated corpse hits do not
reroll, and both real death animations finish inactive. The isolated real
animation pursuit reaches speed 99 and deals two bites in 420 updates.

Proofs: `build_fx3_raptor_l3a_differential.log` has 22 passing selected tests:
20,480 original C++ decisions; 640 complete C target-info/item/Lara/PRNG/Nav
snapshots; 1,024 original C++ moods; 2,560 C mood-helper and 960 aggression
edge cases; original navigation and geometry checks. The final native
updates use 34.4% of C's instruction count. An equal-high-word fast path in
the signed corridor comparisons and a 1 KiB immutable FastROM unit lookup
also pass the C/original-C++ suite (`build_fx3_raptor_l3a_fastbounds.log`).
They slightly reduce steady scene cost but do not solve the early deficit.
General BSS/Nav/stack allocations are unchanged; engine ROM has 39,153 free.
`build_fx3_raptor_l3a_final_differential.log` rechecks decisions, lethal-hit
pending-state preservation and all 640 complete updates after that final
fix: three tests pass (3,494,755 native / 10,167,213 C update instructions).

Two failing regressions were recorded at this earlier point (the contact
failure is fixed by the current-frame work above; timing remains open):

- `original Lost Valley`: item 42 starts at its original room-51 placement
  `(-15872,3328,-31232)`, approaches Lara, then remains in BITE with Lara
  undamaged. At 420 updates it is near `(-18354,3328,-31308)`. The original
  GBA/32X FAST_HITMASK uses `getBoundingBox(false)` and collision-phase hit
  state; this port still uses animation-union bounds during enemy update.
  Current-frame contact data/order is the next behavior task, not a larger
  arbitrary bite radius or a forced state transition.
- `profiles the full tick`, LEVEL3A, species `19,19,19`: **289 ticks / 11
  drops / 600 fields**, 400 pursuit damage and all three owners retained.
  Drops occur during the first 180 fields; the later trace returns to 30 Hz
  and about 4–5 displayed fps. First full tick is 2,362,440 master clocks,
  including three Nav initializations averaging 350,814. Final mean engine
  cost is 628,543, inclusive gameUpdate 655,414. See
  `build_fx3_raptor_l3a_fastbounds_live.log` and its profile JSON. The queue,
  drift bound, search work and render deadline have not been weakened.

LEVEL1's full suite passes 55 checks and LEVEL2's 39; focused final LEVEL1
geometry/live-clock/publication checks also pass, with both scenes at
300 ticks and zero drops. Full logs are `build_fx3_raptor_l1_final.log` and
`build_fx3_raptor_l2_final.log`; the last raptor-only fidelity fix is followed
by both live gates in `build_fx3_raptor_l1_final_timing.log`.
All 52 Python tests pass. GSU-2 LEVEL1 rebuilds
byte-identically (`$64FA`). The earlier primary FX3 ROMs remain the last
completed six-map milestone; the Lost Valley artifact is an unfinished
preview, not an end-to-end gameplay or hardware-timing proof.

## Done: 2026-09-06, six FX3 navigation maps and original alternate-zone union

FX3 now carries ground-1, ground-2 and fly maps for both normal and alternate
geometry. OLH2 version 3 gives ZONE a six-offset directory; identical whole
arrays share storage losslessly. The compiler, native readers and cold scan
use independent CPU ROM pointers, including CPU-only `$F8`. No navigation
table is mirrored in WRAM, and none of this extends the FX ROM window.

`Nav_init` now builds the actual original ordered union of normal and
alternate reachability, with no duplicate box indices. `Nav_setFlipped`
selects the active maps without resetting live searches. Ground-2 gets its
own step profile and ground height semantics rather than aliasing fly.
Room swapping, flip triggers and water-creature controllers/movement remain
open; TR1's water navigation uses the fly family, not a fourth zone map.
GSU-2 retains its existing two normal maps and shared flood.

The C++ oracle adapter previously duplicated the normal maps as alternate
data. It now reads all six source arrays, and 864 complete queue/cell/waypoint
snapshots cover real levels, mirrored/signed-extreme corridors, all families
and flip changes during active searches. The wolf, bear one-click drop and
fly profiles remain covered alongside ground-2. Another 768 original C++
mood/PRNG/Nav snapshots, 2,048 native/C mood-helper cases and 768 attack
selection cases check flip selection and family-specific height handling.
The cartridge also checks all 3,714/2,886 original zone IDs on LEVEL1/LEVEL2,
54 ordered union lists at both `$F8` bank boundaries, active-flip ground
collision and rejection of the old directory version.

Native initialization matches 1,404 complete C snapshots over 39 lengths,
all three slots/families, shared/distinct maps and dense/sparse/invalid sources.
It uses 1,445,466 instructions versus C's 14,685,924. The six-map change
initially exposed one mixed-encounter startup drop. Reusing the clear pass's
batch count and the successful comparisons' carry flags in both scans
recovers that margin without changing the four-slot queue, five-tick drift
gate or sixteen-NMI publication deadline.

Both 600-field scenes finish **300 ticks, zero drops**; both 3,600-field
scenes finish **1,800/0**, retaining all three owners and dealing 5,250/12,094
pursuit damage. Presentation remains mostly 4–5 fps. Short-run engine costs
are 529,362/560,243 master clocks per tick; inclusive gameUpdate means are
552,631/584,716, with all three cold admissions included in the first call.
The counter includes the tick already in flight; 299/1,799 complete new
calls are sampled. These remain emulator measurements, not FX3 hardware timing.

The six maps add **eight bytes** of flip/cached-offset state: Nav is 25,290
bytes, leaving 23,350 in former `$7E` code regions. Engine ROM has **42,038
bytes free**. General BSS, direct-page pointers, mutable-sector allocation
and stack bounds are unchanged; measured stack low-water is `$1E6A`.
LEVEL1/LEVEL2 host images are 62,966/63,605 bytes, up 1,250/974 bytes;
only their existing 16,104/17,196 mutable sector bytes occupy `$7F`.

Verification: **52/36 full LEVEL1/LEVEL2 cartridge checks**, **40/38 native/C
checks**, both long timing gates and all 52 Python tests pass. Shaded boot,
navigation and bear checks pass; shaded/unshaded saturated-queue tests each
force nine publications and nine visible flips in 240 fields. All 34
pixel/clip/dispatch fixtures pass in both independent GSU cores, shaded and
unshaded, as do fixedcheck and the shaded animation/pose oracle. Fresh GSU-2
ROMs remain byte-identical (`$64FA/$8119`), with 17 navigation/creature checks
rerun on LEVEL1. Rebuilt primary FX3 cartridges exactly match the tested
images (`$0814/$581E`; shaded LEVEL1 `$586E`), followed by a no-op build.
Opening frame CRCs are unchanged.

Next: remaining creature/object controllers, actual alternate rooms,
renderer/data gaps, inventory, audio and saves. This closes the six-map
navigation slice, not full GBA/32X parity.

## Done: 2026-09-06, bank-aware FX3 ROM tables and writable-sector staging

The FX3 port no longer inherits GSU-2's one-bank host blob or 2 MiB image.
The packer exports all 18 tables with 32-bit offsets/lengths; OLH2 places them
independently in CPU FastROM at `$E00000` and above, in a 4 MiB image.
`LEVEL.BIN` no longer embeds a duplicate host blob or reserves its old hole.
CPU-only `$F0-$FF` is supported without falsely mapping it into the FX's
3 MiB ROM window. The geometry/texture reservation below file `$1E0000`
and individual 16-bit table indices are still software limits, not hardware
ceilings. [HOST_TABLES.md](HOST_TABLES.md) specifies the format and ABI.

The cold `$BC` binder checks version/size, descriptor flags, alignment,
ordered non-overlapping ranges and whole-bank boundaries before publishing
readiness. Each table has an independent far pointer; offset zero is valid.
Only mutable sectors are mirrored in `$7F`, preserving movable-block floor
rewrites. A full-bank sector copy uses DMA count zero, while an empty table
skips DMA. Immutable collision, animation, aim and navigation tables remain
in ROM, readable while the FX runs. Snapshot publication still has its
existing ordering boundary.

LEVEL1/LEVEL2 host images are 61,716/62,631 bytes, with only 16,104/17,196
sector bytes in `$7F`: **45,386/45,209 WRAM bytes recovered**. The 18 pointers
and ready word use 74 bytes in checked direct-page gaps, without moving the
compiler registers or room workspace. General BSS has 7,757 bytes free,
Nav remains 25,282 bytes at `$7E:5800`, and the five engine ROM banks have
43,200 bytes free. Host code remains 677 bytes. Stack bounds stay
`$1C00-$1FFF`, with measured low-water `$1E68`.

Naive bank-aware reads introduced one dropped tick in the mixed encounter.
Native empty-slot admission preserves the complete C path for reclamation,
nearest/forced eviction and invalid owners. Nav initialization clears/scans
32 cells at a time and temporarily selects the zone table's data bank;
NMI preserves that register and does not borrow the routine's scratch state.
Adjacent room/static fields reuse the index register. Both 600-field live
scenes now reach **300 ticks, zero drops**, and both 3,600-field runs reach
**1,800/0**, retaining three owners and mostly 4–5 displayed fps. The counter
includes the tick already in flight at the sampling boundary; 299/1,799
complete new gameUpdate calls include all three cold creature admissions.
The four-slot queue, five-tick drift gate and sixteen-NMI publication deadline
are unchanged. Engine costs are 528,956/559,849 clocks per short-run tick;
these are emulator bus-clock measurements, not physical FX3 firmware timings.

Verification: **51/35 full LEVEL1/LEVEL2 checks**, **39/37 native/C checks**,
both long timing gates, shaded boot/bear checks, and shaded/unshaded saturated
publication checks pass. The latter still force nine publications and nine
visible flips in 240 fields. Bank fixtures relocate every table through
`$E1-$F2`, at bank starts and ends, test ROM read-only behavior and writable
floors, and cover invalid directories and full/empty sector DMA. Native
initialization matches 936 complete C Nav snapshots over 39 lengths; native
admission matches 384 complete Nav/item/turn/dirty snapshots, including earlier
inactive-owner precedence and invalid boxes. All 51 Python tests, fixedcheck,
and the FX3 shaded animation-channel/pose oracle pass. All 34 pixel/clip/
dispatch fixtures pass in both independent GSU cores, shaded and unshaded.

Fresh GSU-2 LEVEL1/LEVEL2 ROMs remain **byte-identical** (`$64FA/$8119`).
A same-directory GSU-2→FX3 rebuild reproduces the tested FX3 ROM, followed by
a no-op build. Current FX3 checksums are `$1ADF` (LEVEL1), `$70E3` (LEVEL2)
and `$6B39` (shaded LEVEL1). Opening frame CRCs are unchanged.

Next: missing ground-2/flip/water navigation data, remaining creature/object
controllers, renderer/data gaps, inventory, audio and saves. This removes a
memory-layout blocker; it does not establish full GBA/32X parity or physical
FX3 timing.

## Done: 2026-09-06, original bear controller and sustained mixed encounters

The bear's reduced STOP/WALK loop is replaced on **both FX3 and GSU-2** by
the original WALK/STOP/HIND/RUN/HOWL/GROWL/BITE/ATTACK/EAT/DEATH decisions.
Target info uses the 500-unit head offset, original front/height checks and
previous-tick 0/2/5-degree steering, including the close-reverse half-turn.
Pending states, injury/reverse consumption and exact random-roll order are
preserved, as are the base update's dead-target defaults. This does not invent
an EAT trigger that bypasses those defaults.

Running contact deals 3 points per tick; BITE/ATTACK latch one 200/400-point
hit until the animation leaves the attack. Both original death graphs run
through STOP or HOWL, without restarting the death animation. An upright fall
can hit for 200 once. Animation-driven ground movement now applies the bear's
256-up/256-down limit alongside the wolf's 256-up/1024-down rule. A real
LEVEL1 room-17 bear reaches Lara and damages her in eleven updates; the
synthetic pursuit reaches 86 units/tick and both running/bite damage.
Joint masks, head/tilt rendering and blood effects remain absent; contact is
still a rotated animation-union box, and GSU-2 keeps its shared pathfinder.

Native bear decisions/update occupy 725/603 bytes, with complete C versions
in the C_MOTION cartridge. `mkenemystate.py` extracts the actual shared C++
enums and updateState into a host oracle: 19,712 decision/PRNG cases match,
including health, flags, pending states and adjacent-byte canaries. Another
512 cases compare complete target-info/item/Lara/PRNG/Nav updates, with
135 nonzero FX3 turns and 174 on GSU-2. Native update instruction counts are
34.3% of C on FX3 LEVEL1, 34.2% on LEVEL2 and 62.2% on GSU-2 LEVEL1.

The two-wolf/bear performance fixture initially dropped two cold-start ticks.
FX3 now has a 396-byte native rotated-contact routine (8,192 C/independent
cases, far pointers and wrapped signed boundaries), eight-zone cold scan
strides (480 complete C Nav snapshots across all three slots and both zone
kinds), and a 255-byte native render snapshot packer. The latter preserves
dirty/active filtering, hidden/frozen frames, local animation indices,
coordinates, and reserved bytes; 512 complete general-BSS cases covering
20,267 records match C and an independent oracle. Four word copies cover
the CPU divider's 16-cycle latency; NMI/IRQ do not use its registers.
Contact and packing take 16.7% and 9.4% of their C instruction counts.

Both live fixtures now run by default, without a frozen renderer or forced
moods, and pass the unchanged limits: **299 completed ticks, zero dropped
over 600 fields; 1,799/0 over 3,600 fields**. Both retain all three Nav owners
and 4–5 displayed fps. The long runs deal 5,250 damage (wolves) and 12,094
(mixed). Ten-second engine costs are 527,107 / 558,054 master clocks per tick;
inclusive gameUpdate is 548,664 / 579,194. Snapshot packing falls from about
210,000 clocks to 37,108 / 36,936. Cold Nav averages 160,998 rather than about
265,000, and the first ticks still include all three admissions. The sixteen-
NMI publication deadline, four-slot queue and five-tick drift bound did not
change. A permanently full queue still gets nine forced publications and
nine visible flips in 240 fields, on both shaded and unshaded FX3.

FX3 follows the vendor's simultaneous-access/FastROM rules: all new native
code is in ROM, not a GSU-2 resident image. Its five engine banks have
**43,672 bytes free**. GSU-2 fits the bear after removing the old controller
and unused species-table fields, leaving **1,816 resident bytes free**
(81 / 450 / 335 / 757 / 193). General BSS stays 7,723 / 1,369 bytes free;
Nav remains 25,282 bytes, and the stack reservation is unchanged at
`$1C00-$1FFF`, with measured FX3 low-water `$1E68`. LEVEL1 checksums are
`$547A` (FX3) and `$64FA` (GSU-2); FX3 LEVEL2 is `$68E9`.

Full cartridge suites pass 50 LEVEL1 / 34 LEVEL2 checks on FX3 and 40 / 27
on GSU-2. Native/C-reference checks pass 37 / 35 on FX3 and 18 on GSU-2
LEVEL1. Shaded boot/bear/publication, 40 Python unit tests and fixedcheck
also pass. Fresh builds and a same-directory GSU-2→FX3 roundtrip reproduce
the primary ROMs byte for byte; the following no-op build does no work.
The final FX3-only optimizations leave the GSU-2 ROM unchanged.
Opening CRCs remain `e3068943`, `f3083c91` and `bc755bd1`. Profiles now keep
separate `profile_creatures` and `profile_creatures_7_7_8` JSON/PNG artifacts;
`OL_PROFILE_FIELDS=3600` runs both minute-long cases, while
`OL_PROFILE_SPECIES=7,7,8` selects only the mixed encounter.

Next: the one-bank host-table/staging limit, missing ground-2/flip/water
zones, remaining creatures, inventory, audio and saves. This is another
parity slice, not end-to-end parity. Real FX3 hardware timing remains
unverified; the interpreter/emulator cannot measure firmware C2P cost.

## Done: 2026-09-06, bounded FX3 render publication and a green live clock gate

The remaining tick drop was at field 25, after CPU render-state publication
started with all four queue slots occupied. The profile now records the
linked queue operand, every field's pending/tick/drop values, and startup
call/drop events; it no longer needs guessed direct-page offsets.

FX3 still polls render completion between ticks and starts uploads promptly.
Only publication of the next render snapshot is deferred when the queue is
full. A one-byte NMI countdown saturates at its expiration marker after
sixteen fields, forcing publication at the next main-loop poll even if logic
remains overloaded. Any queue slack publishes immediately. Neither the
30 Hz clock, four-slot backlog cap nor five-tick test drift limit changes.
STOP/upload/publication ordering and FX3's concurrent CPU/FX ROM/SRAM access
remain intact; this does not introduce GSU-2 ownership restrictions.

The original three-living-wolf workload now completes **299 updates with zero
dropped ticks over 600 fields**. It completes 27 in the first second, 59 in
the second, then remains one endpoint tick behind through the end. Original
animations, room collision, three cold Nav admissions, pursuit damage (400)
and the unfrozen renderer are all retained. Presentation is 3–6 fps across
one-second bins, mostly 4–5. Engine cost is 566,370 master clocks/tick
(1.58 fields); inclusive gameUpdate averages 563,389, with the first update
still costing 2,033,920. This is an emulator clock-gate pass, not a hardware
frame-rate claim.

An extended 3,600-field run retains the same cold start and also passes:
1,799 completed ticks, zero dropped, all three Nav owners, 5,250 pursuit
damage and 4–5 displayed fps after the startup bins. It averages 556,016
engine master clocks/tick (1.56 fields), with the same `$1E78` stack low.

A separate overload fixture deliberately fills the queue at every render
poll while executing real logic/NMI/render/upload work. In 240 fields it
observes nine forced publications and ten displayed-buffer flips (nine in
the shaded variant), with 15–16 field-number increments per deferral. It
also crosses the sixty-field counter rollover. This fixture cannot make
the live performance gate pass: it uses its own emulator and does not alter
the three-wolf scene.

The new byte is direct-page `$27`, below compiler registers at `$30`.
The five engine banks still have 41,770 bytes free, and general BSS, Nav and
stack bounds are unchanged. Live stack low-water remains `$1E78`. Fresh and
same-directory GSU-2→FX3 builds reproduce the primary ROMs byte-for-byte;
unchanged rebuilds do no work. FX3 LEVEL1 checksum is `$D580`. GSU-2 remains
byte-identical with checksum `$5E05`. Opening render CRCs remain `e3068943`,
`f3083c91` (shaded FX3) and `bc755bd1` (GSU-2).

Full cartridge suites pass 44 LEVEL1 / 30 LEVEL2 checks on FX3 and 35 / 23
on GSU-2. Native/C-reference suites pass 29 / 28. The shaded FX3 boot/frame
and bounded-publication checks both pass, as do 35 Python unit tests and
fixedcheck. The original ten-second gate and the extended minute-long gate
both run without relaxing their assertions.

Next: the remaining parity work, including the reduced bear controller,
host-table staging and missing zones, then other creatures, inventory,
audio and saves. Real FX3 firmware timing and full GBA/32X ROM builds remain
unverified.

## Done: 2026-09-06, FX3 collision, mood and deterministic arithmetic cost

Native FX3 collision wrappers now handle angle/side setup, room probes,
bridge floors and every ordered closed-door push. C_MOTION retains the
complete C implementations. Far pointers, side padding, signed word wrapping,
slant thresholds and the serial room-query workspace remain compatible.
The mood distance/zone/random-box helpers and attack selector are native too;
only an uninjured, same-zone attacker with a living target and seeded end box
can bypass the original C transition machinery. Other mood transitions and
cold target selection remain on the complete C path.

The exact logic PRNG uses ten CPU byte products for the original uint32 LCG,
waiting nine CPU cycles after each multiplier start. NMI/IRQ do not use those
registers. Negative head offsets multiply positive magnitudes then restore
the full product's sign before shifting, preserving floor rounding. Nav
rejects unchangeable visited cells before immutable geometry reads. Its
eight-cell clear handles zero/nonmultiple lengths without overwriting unused
boxIndex words; compact-list length comes from the final write pointer.
Code stays in FX3 FastROM under its simultaneous-access rules. GSU-2 retains
the original implementations, ROM/SRAM ownership rules and resident layout.

The unchanged three-live-wolf scene now completes 298 updates and drops one
tick over 600 fields, with 4–5 displayed fps and 400 pursuit damage. The drop
is in the first second; none follow. This improves the previous 291/8 result,
but remains 29.8 measured logic Hz, not a no-drop 30 Hz pass. The default
performance gate is still red. Engine cost falls from 663,331 to 567,315
master clocks/tick (1.59 fields). Inclusive gameUpdate averages 565,069;
the first update costs 2,033,572. Mood processing averages 5,430, wolf updates
104,639, Lara_update 137,496 and room collision 76,338. Render-state publish
still averages 208,919. These are emulator bus-clock measurements, not FX3
firmware/hardware timings.

Validation:

- 42 LEVEL1 and 30 LEVEL2 correctness checks; the separate LEVEL1 performance
  gate remains the sole failure. Native/C suites pass 29/28 checks. GSU-2
  passes 35/23, and shaded FX3 boot/frame comparison passes.
- All 65,536 angles, 4,896 side/slant/padding cases, 2,048 bridge probes,
  768 door scenes (including 69 multiple pushes) and 1,024 point/full-room
  collision cases agree with C. Fixtures include mirror/far-bank outputs.
- 8,192 PRNG seeds and 1,024 uninterrupted rolls agree with C and independent
  uint32 arithmetic, including the full return ABI. They use 38.9% of C's
  instructions. 2,048 mood-helper cases and 512 attack-selection boundary
  snapshots preserve results and exact random-number consumption.
- Existing 432 shared C++ Nav and 768 mood/PRNG/Nav snapshots still agree;
  20 initialization lengths and 1,536 corridor cases pass. The 640 complete
  wolf snapshots use 35.8% of C's instructions on LEVEL1, 35.7% on LEVEL2,
  including the 168 nonzero warm-context turns. Arithmetic/register/flag,
  movement, animation, bounds and ground-motion checks remain green.
- 35 Python tests and fixedcheck pass. Clean and same-directory GSU-2→FX3
  builds are byte-identical to their primary cartridges; unchanged builds
  are no-ops. The FX3 LEVEL1 checksum is `$C57F`; GSU-2 remains `$5E05`.

The five engine ROM banks have 41,770 bytes free; no WRAM allocation grew.
The live stack low-water mark is `$1E78`, inside the unchanged reservation.
Opening render CRCs remain `e3068943` (FX3), `f3083c91` (shaded FX3) and
`bc755bd1` (GSU-2).

Next: remove the last cold-start drop without changing the clock/backlog
limits or excluding admission/search work from the live trace. The three
initializations average 265,186 clocks each and cold admission adds about
36K each. Then return to the remaining parity work: host-table staging,
creature controllers/zones, inventory/audio/save support. Real FX3 firmware
timing and complete GBA/32X ROM builds remain unverified.

## Done: 2026-09-05, FX3 corridor, targeting and steering cost

Warm context acquisition, queue expansion, the full corridor and the
Enemy_target/Wolf_update shells now use native 65816 code in FX3 FastROM.
The C mood rules still run in their original order; the complete C versions
remain in reference cartridges. Waypoint deltas stay signed 32-bit and scale
both axes together before 16-bit steering. Wolf updates preserve the
previous state's turn limit, signed close-reverse radius test, head offset,
target-info flags, attack latch and single death-animation roll.

The software word multiplier shifts the smaller factor. Dense zero-extended
word products use up to four CPU 8×8 multiplies, observing the result latency
and preserving all r0–r5 halves. NMI/IRQ never use the shared arithmetic
registers. Exact 9–15-bit logical compiler loops now use byte-swap/mask and
remaining shifts, with the same final flags and registers. Four-cell Nav
clears preserve untouched boxIndex words and handle nonmultiple lengths.
These are FX3-only changes; GSU-2 keeps its original code and memory policy.

The unfrozen three-wolf scene improves from 216 ticks/79 dropped to
291 ticks/8 dropped over 600 fields, with 4–5 displayed fps and 400 damage
from real pursuit. This is 29.1 logic Hz, not the required no-drop 30 Hz.
Six ticks drop in the first second and two more by the fifth; the default
performance gate still fails. Engine cost is 663,331 master clocks/tick,
down from 902,598. Inclusive gameUpdate averages 681,885 (first tick
2,255,298), Enemy_target 60,825, Nav_getWaypoint 21,343 and Wolf_update
123,786. Stack low-water remains `$1E69` in the final trace. The five engine
ROM banks now have 38,669 bytes free; WRAM allocations are unchanged.

Both levels match 432 shared C++ Nav snapshots and 768 mood/PRNG/Nav
snapshots. Added differential tests cover 20 initialization lengths,
1,536 side/terminal clamps and 640 complete wolf snapshots including 168
nonzero warm-context turns; LEVEL1 wolf updates use 42.9% of C's instructions
(LEVEL2 42.7%). The 384 ground cases
use 39.6%. Each of eight emitted logical-shift forms and sign extension
passes all 65,536 inputs including final flags; 8,192 long/word products,
864 containment cases, 225 steering vectors and 81 distance edges pass.
Native/C suites pass 21/20 checks. Functional suites pass 41/29 on FX3 and
35/23 on GSU-2; the separate FX3 performance gate remains red. The shaded
FX3 boot/frame comparison also passes. 35 Python tests and fixedcheck pass.
Fresh and same-directory GSU-2→FX3 round-trip builds are byte-identical;
the unchanged build is a no-op. Standard GSU-2 remains checksum `$5E05`;
the FX3 LEVEL1 cartridge is `$10A4`.

Next: cold admission/search bursts, remaining C mood/room/collision work
and the 205K-master-clock render-state publish. Lara_update still averages
191K inclusive clocks (room collision 110K), and camera follow 71K. Keep
the three owners, original rules/PRNG, actual damage and active presentation.
Hardware FX3 firmware timing and full GBA/32X ROM builds remain unverified.

## Done: 2026-09-05, FX3 native hot paths and active-scene presentation

The new three-live-wolf workload uses real LEVEL1 geometry, original moods,
animation-driven movement and pursuit damage, without freezing the renderer
or pinning waypoints. The profiler now includes all FastROM banks and measures
elapsed bus clocks, including memory speed, DMA and raster refresh stalls.
It records selected inclusive call spans, flat self costs, tick/drop/render
samples, Nav owners and stack depth in `profile_creatures.json`. The default
FX3 LEVEL1 check now exposes the still-failing 30 Hz target explicitly.

Native FX3 Nav initialization/search, point/corridor helpers and wolf ground
movement retain C-reference implementations. A 512-byte sector cache has
exact sector/depth/count keys and explicit level-reset invalidation. Compiler
sign-extension loops are replaced with a flag-preserving sequence; runtime
multiply shortcuts preserve the compiler's scratch-register contract. GSU-2
keeps its original implementations and code placement. Build-budget validation
now reads resolved C_LAYOUT records from `config.inc`; parsing conditional
Makefile assignments had mistaken FX3-only item/bat placements for GSU-2's.

The initial profile completed 118 ticks in 600 fields, dropped 178 and
starved presentation. The native hot paths reached 245/50 but still starved
it. FX3 now services completed frames between ticks without adding a wait
while logic is queued. The resulting scene exposed a shared empty-height
portal-clip bug: a zero-line raster strip decremented to 65,535 and drew for
seconds. The new cold entry guard rejects empty/reversed vertical windows;
flat, opaque and keyed one-row controls verify that visible strips still draw.
The final live scene completes 216 ticks, drops 79, and sustains 4–5 displayed
fps. This is roughly 21.6 logic Hz, not the required 30. The engine costs
902,598 master clocks/tick (2.53 fields); stack low-water remains `$1E69`.

Nav and cache occupy 25,282 bytes, leaving 23,358 in former code regions;
general BSS has 7,723 free and the five engine ROM banks have 32,451 free.
432 complete C++ Nav snapshots and 768 mood/PRNG snapshots still match on
both levels. The old 360-snapshot Nav workload drops from 7,102 to 1,974 mean
CPU instructions; the expanded workload is 1,866 mean, 8,056 maximum. Native
ground movement uses 39.9% of C's instructions across 384 cases. All 65,536
sign-extension inputs and 4,096 integer products pass value/register/flag
checks. The runtime and compiler speedups do not establish the full budget.

The raster regression first failed on the pre-fix cartridge, then passed
34 FX3 (also shaded), 62 GSU-2 and 62 Mode 7 span/clip/dispatch fixtures in
both GSU cores. Correctness suites pass 41/29 FX3 checks and 35/23 GSU-2
checks; FX3 native/C-reference comparisons pass 19/18. The separate FX3
three-creature performance gate remains red. 34 Python tests and `fixedcheck`
pass. Fresh builds and a same-directory GSU-2/FX3 flag round-trip reproduce
the current ROMs byte for byte; the final unchanged build is a no-op.
Physical FX3/firmware timing and full GBA/32X ROM builds remain untested.

At this stage the next performance work was the C corridor/mood/steering and Lara/
camera collision cost, plus the 196–209K-master-clock render-state publish.
Keep three owners, original PRNG/state progression, non-starved presentation
and the no-drop clock target; do not reduce the workload to make it green.

## Done: 2026-09-05, FX3 per-creature navigation and moods

FX3 uses the WRAM its FastROM engine freed: three 8,240-byte Nav contexts
plus a 50-byte clipping workspace in an explicit `$7E:5800-$B8C1` BSS section.
General BSS and the stack have not moved. The old shared flood's 6,406-byte
data allocation disappears on FX3. There are 23,870 bytes left in former
code regions, 7,723 in general BSS, and 30,101 across the five engine ROM banks.

The original weighted search, five-step updates, step/drop filtering and
corridor clipping now produce each creature's waypoint in full 32-bit world
coordinates. Original normal/wild moods, aggression, random box selection,
injured flag handling and nearest/forced context admission drive the current
wolves, bears and bats; flying targets retain waypoint height. Animated/wild
item flags now survive packing. Ground-2, flipped and water-creature zones,
the full bear/other controllers and joint hit spheres remain unfinished.

Independent source bounds exposed truncated PC inclusive maxima in the SNES
box packer; it now uses GBA/32X's conversion. Two shared C++ Nav bugs were
also fixed: `$0400` selected an overlap-index bit rather than blocked bit
`$4000`, and a stale tail on an empty queue prevented a previous tail box
from being requeued as the next target. The room-14 wolf now routes and hits.

The host oracle includes `src/fixed/nav.h` and extracts the original Enemy
mood methods unchanged, rather than maintaining another transcription.
Each FX3 level checks 360 full queue/cell/waypoint snapshots and 768 original
normal/wild mood/PRNG/Nav snapshots, plus admission/release, generation
rollover, world-coordinate reconstruction and allocation canaries. Separate
guards avoid observed 816-tcc short-circuit miscompiles; small clipping
helpers reduce its largest Nav local frame from 896 to 92 bytes.

Verified: FX3 full suites 39 LEVEL1 / 27 LEVEL2, GSU-2 35/23; native/C-reference
suites 16/15 and 12/11 respectively. Mode 7 passes 17 selected checks;
31 Python tests and `fixedcheck` pass. Same-directory FX3/GSU-2 build
round-trips reproduce the respective ROMs and an unchanged build is a no-op.
The opening render comparisons and existing 30 Hz clock fixtures pass;
three active searches still need whole-tick profiling. The isolated LEVEL1
Nav fixture averages 7,102 CPU instructions, with a 61,922 maximum, not a
claim about real FX3 frame timing. The shared C++ fixes ran in the host
oracle; full GBA/32X ROM builds were not run. Hardware remains untested.

## Done: 2026-09-05, FX3-specific memory layout

The vendor specification permits simultaneous CPU/FX access to ROM and FX
SRAM. The remaining math/common, enemy/use and navigation/collision code
images now join the other engine code in FastROM: `$84/$85/$BD/$BE/$BF`.
Code and constants no longer need engine-image DMAs on FX3. The immutable
host level blob remains staged in `$7F` as an implementation choice; shared
entity data still has explicit publication boundaries.

This frees 48,640 bytes of former `$7E` code space for future allocations,
without changing the BSS/stack bounds, and exposes 51,458 bytes of free ROM
space in the five engine banks. Link checks budget FX3 independently and
reject accidental WRAM C/native placement. The 2 MB image and single-bank
host-table format remain port limitations, not FX3 hardware ceilings.

Verified: 35 LEVEL1 and 23 LEVEL2 full-suite checks, 12/11 native/C-reference
checks, and 26 Python unit tests. Boot asserts the relocated entry points
and that the former WRAM code regions stay unused. Same-directory
FX3/GSU-2 round-trips reproduce identical ROMs and then become a no-op;
the GSU-2 ROM is byte-identical to before this layout change. Real FX3
hardware and firmware conversion timing remain untested.

## Done: 2026-09-05, exact navigation sector identity

The old rectangle scan returned the wrong box for 1,265 of LEVEL1's 4,026
sectors and 1,996 of LEVEL2's 4,299: stacked rooms, shared bounds and NO_BOX
holes cannot be reconstructed from X/Z rectangles. Every original sector ID
now survives packing, in a 2,300/2,109-byte predictor/rank/literal map. It is
read in place; no decompression buffer or runtime ROM access is added.

The obsolete 512-byte per-item cache is gone. Native `navBoxAt` and
`Room_sectorAt` reuse collision's room cache and sector multiply, with C
versions retained for `make motioncheck`. Invalid rooms and empty dimensions
return NO_BOX; separate guards avoid an observed 816-tcc `||`/negation
miscompile in the reference. Flood reset also invalidates its remembered
source boxes. Resident code has 1,794 bytes free, BSS 1,369; stack unchanged.

The source ROM bank, not just the staged blob, is now the tight data budget:
the room directory plus blob leaves 2,826 bytes in LEVEL1 but only 119 in
LEVEL2. Both packing stages enforce it. The map is exact, but the shared
search, blocked-box/step/drop behavior and aggression/corridor waypoints
still need the full `Nav` port; this does not close creature navigation.

Verified: all 4,026/4,299 sector IDs, alternating stacked rooms, 2,871 codec
width/rank/hole/wide-offset cases and invalid headers; native point lookups
use 47.0%/42.5% of their C reference's instructions in LEVEL1/LEVEL2.
Full suites: 35 standard, 35 FX3, 23 LEVEL2; C-reference suites on all three,
16 Mode 7 smoke checks and `fixedcheck`. Fresh parallel builds, NAV metadata
recovery and LEVEL1/LEVEL2 round-trips reproduce identical ROMs. Standard
walking remains 30 Hz with zero drops (0.67 sampled fields/tick).

## Done: 2026-09-05, wolf state graph and native animation

Wolf now follows the original sleep/stop/walk/stalk/run/growl/howl/attack/bite
decisions, including the pending-state and previous-turn latches. Animation
speed replaces the constant walk; attacks do 50/100 damage once per attack.
All three PRNG-selected death animations finish without rerolling/restarting.
Ground queries enforce slopes, 256-up/1024-down limits, portal/zone boundaries
and active-creature avoidance. Contact still approximates the joint hit mask
with rotated union bounds. Shared navigation can circle a stationary
front-facing target; full aggression and per-creature waypoints remain open.

`item_anim.asm` preserves transition ordering, inclusive frame ranges,
commands, end links and pending-state clearing. `wolf_state.asm` retains its
C counterpart in `wolf.c` for the CPU-only `make motioncheck` cartridge.
LEVEL1 checks cover 3,785 transition boundaries, 14,769 frame/command cases
and 69,888 wolf decisions, in addition to the movement/box fixtures. The
wolf state test includes original random thresholds and confirms PRNG
consumption, not just the returned goal. Actual animation fixtures cover
ground limits, damage latches, death graphs and steering; the real room-14
slope and trigger scenes specify Lara's facing, which affects attack choice.

The boot-only math check moved to ROM `$3C`, before the CPU relinquishes ROM
to the GSU. Asset packing now explicitly reserves `$3C-$3F`. Resident code
has 553 bytes left; the two per-item state arrays leave 857 BSS bytes. No
stack reduction or runtime ROM access was introduced in the GSU-2 build.

Verified: 33 applicable LEVEL1 tests, 21 LEVEL2 and 33 FX3 (full suites plus
the added steering fixture), C-reference comparisons on all three, 17 Mode 7
smoke checks, 22 Python tests and `fixedcheck`. Fresh/incremental builds and
LEVEL/C-reference round-trips match bytewise; an unchanged build is a no-op.
The standard walking scene still runs at 30 logic ticks/sec with no drops.

## Done: 2026-09-05, native item motion and reliable incremental assets

`item_motion.asm` preserves `ItemObj_move`'s full 16.16 speed/acceleration
ramps, gravity impulse adjustment, terminal threshold, signed trig products
and 16-bit coordinate wrap. `ItemObj_getBoundingBox` copies the same six
animation-union bounds. Their C versions remain buildable with `C_MOTION=1`.
`make motioncheck` compares both actual cartridges: 4,550 movement cases and
279 boxes on LEVEL1 (also FX3), 4,902 and 305 on LEVEL2. Native movement uses
25.8–25.9% of the C instruction count on those fixtures; this is not a claim
about whole-game frame rate. 358 bytes replace 2,311 bytes of compiled code,
recovering 1,936 bytes after linker placement.

Multi-output generators use real stamps on GNU make 3.81. C intermediates
are retained so parallel recovery of a missing shared include cannot leave
the old make's implicit-rule traversal spinning. `LEVEL` and its source path
invalidate the packed assets. The packer writes `TEXTURES.json`; `mkbanks.py`
uses its exact page count, rejects missing/truncated pages and prevents ROM
bank overlap. A LEVEL2-to-LEVEL1 switch no longer includes two stale textures.

Verification: 26 standard LEVEL1 tests, 26 FX3 LEVEL1 tests and 15 LEVEL2
tests pass; 20 Python tests and `fixedcheck` pass. C/native comparison runs
pass separately on all three. A fresh parallel build matches the incremental
ROM; removing each paired asset group, switching LEVEL1/LEVEL2, and switching
C/native motion rebuild identically, followed by a no-op build. Mode 7 passes
its boot/render check and ten CPU/gameplay smoke checks.
The standard walking scene retains 30 Hz with zero dropped ticks and samples
0.63 fields/tick versus approximately 1.0 before native motion.

## Done: 2026-09-05, bat flight and falling death

`Bat_update` uses the original state graph, animation speed, vertical pursuit,
turn limit and contact damage. `Enemy_target` shares the existing cached
ground/flying navigation; ground creatures retain their earlier controller.
Four cartridge fixtures cover flying-zone splits, horizontal portals, failed
queries, wake/attack transitions, speed ramps, vertical separation, walls,
ceiling/floor limits and falling deaths from three live states. The real
LEVEL1 shooting scene now verifies a grounded bat corpse. The trigger test
asserts latched activation, not an AWAKE goal that correctly becomes FLY.

Contact remains a rotated animation-union box approximation. Per-creature
aggression/waypoints, hit spheres, blood, enemy collisions and pitch rendering
are still gaps. `Room_pointOpen` moves to the small upper region and native
math joins its tables in the low image, making contiguous room for the bat.

## Done: 2026-09-05, dives, handstands and resident code space

Swan/fast dives and handstands follow `src/fixed/lara.h`: free hands and
Walk select the dive, speed 132 requests FAST_DIVE, and a fast-dive impact
above 133 is fatal. Water entry keeps each dive's animation and pitch and
doubles its speed. The handstand runs all 193 ticks of animation 159 and
finishes through the shared climb tail. Fall/dive drag now multiplies at
32-bit width, avoiding 16-bit overflow at horizontal speeds above 344.

The `$3D` image starts at `$7E:0600`, recovering 2,560 bytes below the old
`$1000` base. Moving the trig tables and PRNG there leaves 3,407 bytes of
code space free after the new states. The stack still owns `$1C00-$1FFF`.
Budget tests cover host overlap, stack overlap, misplaced sections, crossing
`$8000`, and initialized-data detection in variant build directories. The
emulator checks the complete low WRAM image after boot and gameplay.

Validation: standard LEVEL1 20 applicable tests, LEVEL2 9, FX3 LEVEL1 20;
15 Python unit tests; `fixedcheck`; a Mode 7 build. Standard and FX3 opening
images still match their independent renderers. Clean/incremental ROMs
match byte for byte. The build uses real multi-output stamps compatible
with GNU make 3.81, invalidates on LEVEL/source changes, and discards a ROM
if its post-link budget/checksum checks fail.

## Done: 2026-09-05, current-frame creature aiming

- Creature aiming follows the shared engine's `getBoundingBox(false)` /
  `getFrame()` selection, including midpoint ties and the original final-frame
  guard. The packer resolves each game tick to an upper-third local XYZ
  point, sharing identical points across animations. Runtime yaw rotation
  and world-coordinate wrapping retain the existing native arithmetic.
- LEVEL1's 1,457 creature ticks share 574 points in 7,020 bytes; LEVEL2's
  larger animation index makes it 7,072 bytes. The tables follow the static
  room index without moving earlier host offsets. Animation unions remain
  for camera/collision boxes and the other model types' aim fallback.
- The cartridge oracle independently selects from raw source boxes and
  checks all 51 animations at every tick and two yaw angles, plus sparse
  entries, corrupt ticks and invalid animation indices. `AIM.json` and
  `LEVEL.BIN` are grouped packer outputs; a missing audit regenerates both
  deterministically under `make -j2`.
- All 46 active whole-machine checks pass across both levels on GSU-2 and
  FX3. Sustained combat records 300/299 completed ticks, 60 shots and 60
  damage over ten seconds, with no dropped ticks. Weapon/image oracles pass
  on both renderers; all six opening CRCs and frame costs are unchanged
  from the preceding slice. No baseline updates.
- The native aim helper grows from 338 to 423 bytes. Resident code has
  1,684 bytes free and `.bss` 1,115. Host data uses 59,190 bytes on LEVEL1
  and 60,296 on LEVEL2, leaving 6,344/5,238 under the blob limit. Combat
  camera, manual look, sphere hits, spread, flash and other weapons remain.

## Done: 2026-09-05, pistol torso/head tracking

- `updateLook` now runs before movement and weapon animation, using the
  preceding tick's arm angles/aim flags. It averages signed angles without
  overflow, returns toward centre after target loss and clears on death
  and initialization. The cartridge uses 108 native bytes with a C reference.
- Acquisition subtracts the torso's pitch/yaw as well as Lara's body angles.
  The selected target's arm aim remains body-relative, with independent
  retention from the preceding tick even across multiple candidates.
- The frame handoff publishes eight look bytes at `$70:F72A-$F731`, making
  34 weapon/look bytes in total. GSU torso/head rotations follow their
  original animation pose; the head inherits the torso and the aimed arms
  retain object-relative bases at their translated shoulders.
- The cartridge fixture checks 400 signed angle combinations, 960 decay
  ticks, reset/death, torso-relative cones/ranking and body-relative arm
  retention. `weaponcheck` adds 120 joint/matrix cases; the image oracle
  checks both look joints independently with holstered and aimed pistols.
  The close front-view poses were inspected visually.
- All 42 active whole-machine checks pass across LEVEL1/LEVEL2 on GSU-2
  and FX3. The sustained combat scene records 300/299 completed ticks in
  ten seconds, 60 shots and 60 damage on both, with no dropped ticks.
  Fixed/matrix/weapon/image oracles pass, including Mode 7 images. All six
  opening CRCs stay unchanged, with modelled/reference cost changes within
  0.13%; no baselines needed updating.
- Resident code has 1,798 bytes free, `.bss` 1,117 bytes. Manual look,
  camera-item tracking, combat camera, per-frame aim boxes, spread, sphere
  hits, flash and the other weapons remain.

## Done: 2026-09-05, pistol animations and controls

- R draw/holster and B/Action firing, with the shared weapon states and
  original PICK/DRAW/AIM/FIRE ranges. Each arm raises, fires, reloads and
  lowers independently; nine reload frames yield ten ticks between held
  shots. Releasing Action allows auto-aim to select another target.
- Ten-degree aim smoothing, asymmetric arm fire, holster angle decay, water
  holstering, busy/death guards and armed fast turning. The cartridge CPU
  fixture covers animation and mesh-swap boundaries, signed decay and aim
  smoothing across the angular seam. Reaching/grabbing requires FREE; ledge
  climbs, hanging and water-out occupy the hands until release/completion.
- Runtime thigh/hand mesh swaps, original arm poses and object-relative
  shoulder bases on the GSU. The frame handoff publishes 26 bytes at
  `$70:F710`. Packer metadata supplies pointers/ranges and a conservative
  radius for arbitrary arm rotations; no host blob growth.
- `weaponcheck` checks 555 joint/frame combinations and both arm bases.
  The image oracle checks independent arms; the reviewed holstered mesh
  change updates the six opening baselines and Lara's silhouette records.
- Same-sector trace early exits avoid unused slope arithmetic while retaining
  room and endpoint checks. READY hands skip ineligible land interactions.
  The ten-second combat fixture checks bounded clock drift, zero dropped
  ticks and continued GSU rendering under sustained Action. The boot clock
  uses the same queue-based drift bound; a 28/32 split across one-second
  gauges during frame handoff is valid when no game time is lost.
- The native controller is 509 bytes. There are 2,042 bytes of resident code
  and 1,125 bytes of `.bss` free. The first paired discharge costs 56,446 CPU
  cycles. Torso/head tracking, combat camera, flash, spread and sphere hits
  remain, along with finite ammunition and the other weapons.

## Done: 2026-09-05, target lock

- §6 `weaponUpdateTargets` / `weaponTrackTargets`: per-tick tracking through
  reload, held identity, release, separate right/left aim arcs, loss and
  reacquisition, and suspension of firing while a locked target is occluded.
  A target moving beyond the acquisition radius stays tracked, within this
  port's short trace limits. R remains the temporary direct-fire mapping.
- `weapon_aim.asm`: 338 bytes replace 1,622 compiled bytes, using the same
  rounded trig table and full-width rotation products. The C formula stays
  in `weapon.c`; 256 random ordered boxes plus the fixed angle/rounding cases
  pass on the cartridge. The two-target first discharge costs 62,136 CPU
  cycles, against 74,937 before. Tracking state takes six bytes of `.bss`.
- The lock test fails on the preceding ROM. It also exposed another 816-tcc
  short-circuit miscompile: a mixed `||` / `!` validity expression retained
  an ACTIVE target at zero health. Separate guards reject it correctly.
- GSU-2 and FX3: 16 active LEVEL1 checks and 5 LEVEL2 checks per build.
  Both sustained-fire scenes produce 300 ticks, 33 shots and no dropped
  ticks over ten seconds against two stationary targets. Renderer code is
  unchanged; hardware has not been tested.

## Done: 2026-09-05, aiming

- §6 rotated aim points and angle-priority acquisition, from the shared
  GBA/32X engine. The target's animation-box centre rotates by its yaw; the
  point drives both acquisition angles and the visibility trace. The active
  list chooses by absolute yaw and retains upstream's equal-angle order.
  Range remains spherical about the item origins, and the nine-tick cadence
  is unchanged. Target lock, arms and per-frame boxes remain above.
- The new target-choice fixture fails on the preceding ROM. The cartridge
  checks also cover cardinal/diagonal rotations, negative rounding, wide
  bounding-box arithmetic, the signed-coordinate seam and occluded rotated
  endpoints. LEVEL1: 16/16 active checks on GSU-2 and FX3; LEVEL2: 5/5 on
  each. The fixed-point, matrix and animation oracles pass.
- The two-target discharge fixture costs 74,937 CPU cycles on GSU-2,
  against 32,290 before. A ten-second whole-machine held-fire scene against
  two stationary targets gives 300 ticks, 33 shots and no dropped ticks on
  both builds; enemy updates are frozen for this timing case.
- The weapon unit grows from 3,610 to 4,814 bytes. `level.c` moves to bank 5
  and `block.c` to bank 4; the current bank budgets are above. The GSU program
  is byte-identical, and the opening-frame CRCs remain 7034f88f (GSU-2) and
  6cdda6ba (FX3). Hardware remains untested.

## Done: 2026-09-04, later

- §1 the sixth renderer pass, above: measured first (`gsusim.py`'s four
  new switches), then three picture-identical cuts -- per-vertex clip
  flags, the bucket range, the bounding spheres -- for 37.6% off the
  opening view and 38.6% off the heaviest view found. `frame.baseline`
  re-recorded twice; `make check` 13/13 LEVEL1 and 3/3 LEVEL2 after.
  Three toolchain facts: the profiler's symbol file is only rebuilt by
  `make profile`, so `make sim` after a mesh.gsu change profiles by stale
  addresses; a scratch word marked "free" in the define list may be
  aliased lower down (`F_UTMP` is `EC_DIR`); and zsh's `set -- $v` does
  not split, so the view list is a Python driver: `tools/viewcheck.py
  before.sfc build/openlara.sfc` models eight views on both ROMs and
  compares the raw frames byte for byte.

- §2, §3, §5, §9 the interaction slice: the activation flags read, the
  trigger types and two actions, and upstream's `collide` methods for
  the switch, the pickup, the holes and the block, with the Lara states
  they enter (README "Using things", "The types", "Active from the
  start"). `use.c` and `block.c` are new units; `item.c` grew `animSkip`,
  `animIsEnd`, `moveTo` and the trapdoor floor. Two semantic fixes on
  the way, both upstream's rules the port had approximated: the KILL
  command leaves the status INACTIVE rather than clearing the active
  flag in place (which left the entity on the list with the flag that
  guards a second `activate` clear -- a cycle waiting for a re-trigger),
  and the HANDSFREE effect and the EMPTY command both free her hands
  (the first was walked past). The proximity wake is gone.
- Runner: the trigger scene pulls the lever for real and waits for her
  hands; the pickup scene stands inside the limit box; the creature scene
  stands on the wolves' pad first; the rock scene reads the published
  probe; two LEVEL2 scenes, the key and the block. LEVEL1 13/13 (4 skipped), LEVEL2
  3/3, the frame byte-identical (crc dce097eb, 6.54 M).
- The cost: 8.3 KB for the two units and 4.5 KB of growth. The region
  over the `.bss` grew to 10 KB to take `use.c` (the nav queue bitmap,
  above), `object.c` moved to `$3F`, `block.c` to bank 5. 14.8 KB -> 5.8
  KB free. Two facts about the toolchain worth the keeping: a `static
  const` table is rodata and allowed (the ban is on initialised *data*),
  and wla places a bank's sections anywhere in its 32 KB, so a unit that
  does not fit a small region can land past it silently -- budget.py
  reads the Makefile for the assignment now.
- The tick: the object loop cost 4.7 K a tick when it ran every tick,
  which was the field the boot scene's 30 Hz gate lost; gated on Action
  (and the two states that wait on a frame), it costs nothing on a tick
  she does nothing on. Profile after: 63.6 K running, 1.07 fields, against 63.3 K before the slice.
- Packer: `OL_DUMP_ITEMS`, `OL_DUMP_TRIGGERS` (with each trigger's sector
  and world position) and `OL_DUMP_SECTORS=<room>` (the floor/ceiling
  grid), which is how the scenes were planned. LEVEL3A refuses to pack: a
  slant of -39 past the signed five bits of the host sector table (§12's
  "pack every level").

## Done: 2026-09-04

- §3 the water: `updateWaterState` and the eleven water states,
  `c_swim`, `c_surf`, the two wall checks and `c_checkWaterOut`, to the
  design under 2026-09-03 below, read against upstream once more as it
  went in. TR1's differences, each marked where it stands in the code:
  no wading animations (the WADE arms play a stand where upstream plays
  TR2's ANIM_WADE and its kin, and `c_checkWallUW`'s stand-up-in-the-
  shallows arm is out -- TR1 swims); `c_checkWaterOut` plays
  ANIM_WATER_OUT whatever the height, which is what upstream does last
  anyway; no look camera, no splash. Her pitch is a global she moves
  along and the renderer does not draw. The scene in the runner is the
  one planned -- room 43 over the pool in room 48, dropped at
  (29696,-1800,22528) facing west -- and the host sector table, decoded,
  corrected the plan in two places: the rim is one click *above* the
  surface, at -1536, and it is room 43's column over the pool's west
  *wall* (rooms carry a wall sector round their edge). That second fact
  is why the climb-out works at all: `collideRoom` carries the middle
  probe's room (43, the middle probe being taken above her head) into
  the front probe, as TR's `GetCollisionInfo` does, and finds the rim
  where her own room's sector is a wall. Trail from the run: fall, under
  at -754, swim, surface at -1279, west, out at (27548,-1541), stood at
  -1536. The runner takes `OL_LEVEL` and every scene skips off its own
  level; the frame baseline compares only on LEVEL1; each build has its
  own runner copy so the two levels' checks run side by side. `make
  check` 13/13 (1 skipped) on LEVEL1 with the frame byte-identical, 1/1
  (14 skipped) on LEVEL2.
- The cost, in the budget table above: `lara.c` 6.3 KB -> ~13 KB and
  `collision.c` +3 KB under 816-tcc; `object.c` and `common.c` to bank 4;
  969, 3709 and 1011 bytes free. §12's paging moved to step 4 of the
  order.
- §1 the Mode 7 frame, adopted as the default that afternoon and retained as
  the optional speed build after the ninth pass, and lit: the two items above.
  In numbers,
  the opening frame went 8.75 M (4bpp, unlit) -> 6.54 M (Mode 7, lit): a
  third faster and lit, where lighting the 4bpp frame cost 16.3 M. The
  regression on the way: every edit was held to the unlit CRCs of both
  frames (c01930ff, 7034f88f) until the shade itself went in. Three
  assembler facts the pass paid for: `AND Rn` takes R1-R15 (the mask
  cannot sit in R0); the Super FX has no shift left (`ADD Rn` to itself
  doubles); `.DSB 0` is an error (a 16-byte pad aligns the same as none).
- §12 two resident regions (above): 5.7 KB -> 14.8 KB free without
  moving a byte of data. The stack measurement is the fact to keep: 421
  bytes at the deepest, in a 4 KB region nobody had looked at. Budget
  before: 969 / 3709 / 1011; after: 918 / 3123 / 3709 / 5080 / 2006.
- The tick, from the profiler: idle 46,518 cycles (0.78 fields), walking
  63,301 (1.06), against 46 K and 62 K before -- `Room_water`,
  `Room_sectorAt` and `Lara_updateWaterState` are 2.2 K of it on land.
  The boot scene's `tickCost` gauge went from 0.20 to 1.00 fields on the
  same change: that gauge counts field boundaries a tick straddles, and
  the idle tick now ends just past one it used to end just short of. The
  cycles are the measure; the gauge is phase. A profile scene on LEVEL2
  is owed for the tick under water (the column twice, the pitch
  arithmetic, the swimming radius): estimated at the walking tick plus
  3-4 K, inside the budget.
- Found and left: `make animcheck`'s two stale silhouettes, in §13.

## Done: 2026-09-03

- §8 the camera read against upstream: four fixes, above, and the opening
  view moved -- see README "The camera follows her" for why the old one
  was an artefact of the unclipped snap. `frame.baseline` re-recorded for
  the new view; `make check` has a camera test now. The new view then
  drove one more renderer pass (§1 above): the clipper under a window,
  the left clip inline.
- §1 the fourth renderer pass, above: 11.8 M -> 7.4 M for the opening
  view, ~1.8 -> ~2.9 fps modelled, the picture byte-identical on the
  opening view and eight others (in the `SHADING=1` build, frame and
  shade map both). `frame.baseline` re-recorded three times; `make
  check` 12/12 after each. Three things learned: the "near its floor"
  verdict was about fetch, and the instruction-bound remainder was
  RAM-bound -- every value the span setup and the vertex loop passed to
  themselves went through RAM for want of looking at which registers
  were actually live. The packer has three face writers and the sprite
  one was missed until the pickup test caught it: the geometry views
  are not the whole picture. And the `RASTER_PAD` define is how the
  window's anchor stays aligned; recompute it when the cold half changes
  by reading the symbol file -- and mind that make 3.81 compares mtimes
  to the second, so a `sed` right after a build does not rebuild.
- §2 the room water flag and the water column, above, for the water slice
  that was started and set aside for the frame rate. The slice's design,
  for whoever picks it up: `olLaraWaterState`/`olLaraAngleX`/
  `olLaraSwimTimer`/`olLaraOxygen` as globals (oxygen not in `timer`, the
  active list reads that); `Lara_update` dispatching updateAbove/Surface/
  Under on the water state; `updateWaterState`, `c_swim`, `c_surf`,
  `c_checkWallUW/Surf`, `c_checkWaterOut` ported from upstream with the
  TR2-only animations (`ANIM_WADE`, `ANIM_SURF_OUT`, `ANIM_SWIM_STAND`,
  `ANIM_SURF_STAND`, `ANIM_UW_ROLL`: Lara has 160 animations in TR1)
  left out; the swim animations carry no speed of their own, so
  `ItemObj_move` needs no guard; `Lara_updateRoom` takes upstream's
  offset (her radius down at the surface). The test: LEVEL2 room 43
  (dry, x 25600..32768, y -3584..-1280, z 19456..26624) sits over the
  pool in room 48 (water, x 26624..32768, y -1280..256) with a one-sector
  ledge on its west side level with the surface: drop her in at
  (29696, -1800, 22528), expect UNDER, hold Down+A to surface, swim west
  facing $C000, press Action at the ledge for `WATER_OUT`. `make check`
  wants an `OL_LEVEL` for the runner and a frame check that skips when
  the level is not LEVEL1; neither exists yet.

## Done: 2026-09-02

- §1 the edges precomputed in the batch (32-byte VertexLinks, `EdgePrepBatch`
  under a third window, `EA_*` as copies), and the strip clip in
  `EA_Strip` replacing `ScanClip` -- which uncovered and fixed the
  truncation of faces crossing the top of the screen. 13.1 M -> 11.8 M,
  picture corrected, baseline re-recorded, six other views checked.
- §1 the per-face setup, batched: `OTDraw`/`DrawFace`/`BatchFlush` with
  the slots at `$0200`; the near-plane drop and the cull in `OTBuild`; the
  redundant `FaceVisible`, `FaceTop` and `FaceLinksSet` out of the
  unclipped path; `EdgeInit` down to four stores; the raster section
  aligned. Opening frame 19.1 M -> 13.1 M, crc unchanged, `frame.baseline`
  re-recorded, `make check` 12/12 -- after the host caught up: at two
  frames a second `gameRenderSync` (all sixty entities, ~130 K cycles a
  frame) cost the walking test its clock (27 tps). It syncs Lara, the
  active list and `ItemObj_touch`-marked entities now, and `ItemsExchange`
  is an MVN of `itemsCount` records: the walk profile's C went from 64.6 K
  to 57.7 K a tick (1.08 -> 0.97 fields) and the host's idle from 29% to
  39%, at two frames a second. The lesson of the evening was one bug:
  an edit that consumed a `lms r5, (F_BUCKET)` went unnoticed while R5
  happened to hold the value, and showed up as wrong depth buckets three
  edits later. The bisect that found it is the shape to reuse: build the
  two variants, dump the ordering table after the first mesh, diff.
- §8 camera clipping: `getBestLocation` and the trace behind it, with the
  three tick cuts above; the opening view moved out of the entrance wall
  and the baseline followed it. nav.c and enemy.c moved to bank $3F to make
  room for it in bank 5. Tick: idle 41.4 K -> 46.1 K, running 56.3 K ->
  62.1 K, and the discovery that the budget is 1.5 fields.
- §3 hang and climb: the handlers and helpers above, a test on the
  two-click step in room 10. Tick unchanged. It left the $7E:8000 bank 4 KB
  from full, so `lara.c` moved to bank 5: the three halves have 7.7, 10.9
  and 10.4 KB free.
- §2 static mesh collision: `Room::collideStatic` ported, its data in the
  blob (the first use of the headroom), the scan in assembly, a test on the
  rock behind the entrance. Tick: idle 39.3 K -> 41.3 K, running 53.5 K ->
  56.1 K.
- The interpreter modelled the power-on frame; now the steady state.
- `CACHE` at `TM_Vertex`: the transform loop and its two helpers in a window.
- `RomByteAt` inlined in `OTB_Face` and `DrawFace`.
- 28.4 M -> 23.1 M modelled, same picture; `frame.baseline` re-recorded.
- The per-face path costed by fetch per byte; the batching design written
  up as the next renderer item, with the RAM question it turns on.

## Done: §0, 2026-09-01

The three items that gated everything below §1, in one pass, each verified
by `make check` 10/10 and the three host oracles before the next went in.

- **`frame*.baseline`.** `tools/gsusim.py --stats` writes the modelled cycles,
  the cache hit rate and a CRC of the frame buffer; `tools/framecheck.py`
  compares them with the tracked record and refuses past `FRAME_TOL` (2%)
  or on a changed picture; `make baseline` records. The first thing it
  refused was the blob restructure below: 13 KB less to stage per frame is
  2.5% of the frame. Only the opening view is compared; other views say so
  and skip.
- **`src/room.asm`.** Room_query, Room_sector, the slants and Room_reset,
  called from C through 816-tcc's convention unchanged. The C read the blob
  through 32-bit pointer arithmetic and multiplied `sx * zSectors` in
  `tcc__mul`; the assembly reads `lda.l $71xxxx,x`, multiplies on $4202, and
  keeps both caches in the direct page at $60. Bank 4 is 2.7 KB smaller than
  it was with the C. Idle tick 52,960 -> 39,300 cycles; walking with the
  wolves awake 70,494 -> 53,472.
- **The link table.** A host sector is `int8 floor, int8 ceiling, uint16
  link`; a link is `roomBelow, roomAbove, portal, trigger, uint16
  floorSlants, uint16 ceilingSlants`, one per distinct combination, link 0
  being "nowhere, flat". The header grew a `linkCount` word (offset 24) and
  the room table starts at 26. LEVEL1: 583 links, 51,774 bytes, 13,760 free.
  The GSU stages the blob by its own length word and reads none of it past
  the entity list, so nothing on that side changed; `seed_items` in the
  interpreter finds the entities through `itemBase` as before.

## Done: the 2026-08-18 bug hunt

Findings from a four-way review of the untracked SNES target plus a full
oracle run. Every item is pinned to a verified file:line; the fix designs
below were checked against the surrounding code, not guessed.

**Executed 2026-09-01.** `make check` is 10/10, the three host oracles are
clear, `make SHADING=1 sim` assembles and runs the shade fix, and the host
code budget is now asserted at link time. Four items did not go in as
designed:

- **1.9 was wrong and is reverted.** Removing `OTClear`'s `cache` made the
  frame *slower* in gsusim: 29.2 M -> 31.7 M cycles, cache hits 43% -> 34%.
  The window it anchors also holds `OTBuild` and `OTDraw`, whose per-face
  loops are hotter than the SpanFillT refault. The `cache` stays, with the
  measurement in the comment and in the README.
- **1.6 needed the runner too.** Publishing `ol_ticks` last is in, but the
  sector test's one-tick wait then sampled a tick that had *started* before
  the pin was written. `sample()` now waits for the second completion.
- **1.11 goes before `InitShadeMaps`, not in `InitGSU`**: the shade-map seed
  at boot runs before `InitGSU` is called. The RON=0/RAN=0 SCMR write sits
  in the boot sequence ahead of both. The manual does document SCMR's reset
  default as 00H (Book II 2-3), so the item's "no manual guarantees" was
  wrong; the write stays as a stated assumption, cheap and explicit.
- **3.1 is two computation sites, not four**: the packer's second use reads
  the same variable. Both real sites now say `((n+1)/2)*5`.

### 1. Confirmed bugs — fix and verify

- [x] **Switch never animates** — `src/object.c:101`: `Switch_use` sets
      `ITEM_FLAG_ACTIVE` by hand, so `fireTrigger` (`src/room.c:244`) skips
      `ItemObj_activate` and the switch never joins the active list; the
      lever stays frozen and the spring-back timer is dead.
      Fix: replace `item->flags |= ITEM_FLAG_ACTIVE;` with
      `ItemObj_activate(item);` — it self-guards on the ACTIVE flag
      (`src/item.c:264`), and `Switch_update` behaves correctly once the
      switch is genuinely listed.
- [x] **Wolf death animation restarts every tick** — `src/enemy.c:131-139`:
      the `health == 0` branch calls `ItemObj_animSet` unconditionally and
      `animSet` (`src/item.c:155-174`) has no same-animation guard, so the
      corpse jitters on frames 0-1 forever and never leaves the active list.
      Fix at the call site (not in animSet — s_checkRoll relies on
      unconditional set): only call when
      `item->animIndex != olModelFirstAnim(item->model) + da`.
- [x] **`olRandSeed` never initialized** — `src/math.c:115`: garbage on real
      hardware; `olMathCheck` asserts the seed-0 sequence (12/30469/16649)
      and fails at boot. Add a `rand_reset()` (or make gameInit seed it)
      called at the top of `gameInit` before `olMathCheck`.
- [x] **`olLaraTurn` never initialized** — `src/lara.c:10`: on hardware Lara
      spins for the first second while the decay burns off garbage. Zero it
      in `gameInit` (also `olLaraMoveAngle` for the same sweep).
- [x] **First-frame garbage entity staging** — `src/game.c:76`:
      `ol_itemState_gsu` is uninitialized `.bss` and `ItemsExchange` ships it
      before the first `gameRenderSync`. Fill the whole array with `$FF` in
      `gameInit` — anim `$FF` is the renderer's hidden marker, so the first
      frame draws no entities instead of garbage.
- [x] **Torn tick counter** — `src/game.c:545`: `ol_ticks++` is the first
      statement of `gameUpdate`, so harness observers can sample a
      half-finished tick (this is what fails the sector-query test — the
      data and `Room_query` math are verified correct).
      Fix: compute `tick = ol_ticks + 1` locally, use it for the `&7`/`&15`
      pacing tests, and store `ol_ticks = tick` as the LAST statement of all
      three exit paths (no-items return ~:563, dead-Lara return ~:579, end).
      Safe: the runner pins `ol_ticks` by overwriting it per field, so the
      pinning path is unaffected; `enemy.c`'s `&15` pacing only phase-shifts.
- [x] **Bite test reads repurposed dx/dz** — `src/enemy.c:208-227`: when the
      path branch overwrites dx/dz with the next-box direction, the bite
      check compares *that* against DIST_BITE — a wolf across a box boundary
      can never bite; one parked on a box centre bites from 3072 units.
      Fix: recompute `lx = lara->x - item->x`, `lz = lara->z - item->z`
      immediately before the bite check and test those; leave dx/dz for
      steering (`phd_atan` below is intentionally the path direction).
- [x] **SpanShade underflow clamp dead** — `gsu/raster.gsu:1139`:
      `ibt r0, #$80` sign-extends to `$FF80`, so every out-of-range shade
      goes dark (row 31) and the bright arm is unreachable.
      Fix: `iwt r0, #$80` (SpanShade is outside the cache window; the byte
      is free). NOTE: the code is inside `.IF SHADING`; the unlit default does
      not assemble it, so verify explicitly with `make SHADING=1 sim`.
      gsusim.py reproduces the same sign extension, so the oracles agreed on
      the wrong picture — fixing the ROM alone keeps them agreeing.
- [x] **OTClear thrashes the cache window per mesh** — `gsu/mesh.gsu:3211`:
      `DrawMesh` (called per static-mesh instance and per model node) runs
      `OTClear`, whose `cache` moves CBR and forces SpanFillT to refault the
      ~507-byte window at 5 cyc/byte every mesh.
      Fix: delete only the `cache` line — KEEP `iwt r13, #OTC_Loop`
      (it is the LOOP target register, not cache setup). ClearFrame keeps
      its `cache` (genuinely once per frame).
- [x] **keysAccum wipe race** — `main.asm:678-680`: an NMI between the
      `lda keysAccum` and `stz keysAccum` ORs in fresh bits the `stz` then
      erases — a one-field tap vanishes. `sei` does NOT fix this (NMI is
      non-maskable). Fix: replace `stz keysAccum` with `trb keysAccum` —
      A still holds the snapshot after the `sta.l`, and TRB atomically
      clears exactly the sampled bits; bits arriving between the two
      instructions survive for the next tick.
- [x] **Boot writes pak RAM before SCMR is ever written** — `main.asm:157`
      (shade-map seed, SHADING=1) and `:578` (`GS_LEVEL_READY` clear) run
      before the first SCMR write (`StartGSU`); they rely on power-on RAN
      being clear, which no manual guarantees. Fix: one
      `sta GSU_SCMR` of the shadow with RAN=0 early in InitGSU.

### 2. Test + build hygiene

- [x] **Roll test constant** — `tools/typesnes_dump.ts`:
      `drive(PAD_UP | PAD_DOWN, 23, 3)` → count 12. The roll is anim 146
      (10 ticks, entered at frame 2) then anim 147 carries state 23; 3
      updates were 45 ticks in the ×15 era, now they are 3.
- [x] **Makefile C deps miss `src/lara.h`** — `Makefile:187-188`: editing a
      `LARA_*` constant rebuilds nothing. Add it to the `c_%.raw` rule.
- [x] **WRAM code-budget check exists only as a comment** —
      `main.asm:1136`: add a post-link assertion (grep the .sym for the
      host-code end symbol, require < $1000) or delete the claim.
- [x] **Stale Makefile comment** — `Makefile:276`: sim camera "matching
      gameInit()" — the values actually match the settled follow camera
      behind Lara's level placement (verified); reword.

### 3. Latent hardening (correct for LEVEL1, wrong for the next level)

- [x] **Slope-stream size formula vs emission** — `packer/out_SNES.h:1972`,
      `:2112`, and `src/level.c:68` all use `(n*5+1)/2`; the emission loop
      writes `ceil(n/2)*5`. Off by 2 for odd sector counts → every later
      table shifts. LEVEL1's 4026 is even by luck. Fix all four sites to
      `((n+1)/2)*5` (identical for even n — no repack needed to verify).
- [x] **768-vertex room cap warned but not clamped** —
      `packer/out_SNES.h:1498-1521`: emit-anyway overwrites the GSU ordering
      table. Truncating vertices would break face indices, so make it a
      hard `exit(1)` like the packer's other refusals.
- [x] **`(boxCentre - csx) << 10` overflows 16-bit at ≥32 sectors** —
      `src/enemy.c:208-209`: clamp the sector delta to ±31 before shifting
      (only the direction matters for steering).
- [x] **Zero-length blob copy loops 65,536 times** — `gsu/mesh.gsu` SIL_Copy
      (~:2116): guard the length word before the LOOP.
- [x] **`&&`/`||` with a call on the right** — `src/game.c:549`, `:633-645`
      (wall clamp × 4), `src/lara.c:153-155`: currently compile correctly
      (verified in build/*.asm.opt) but violate the port's own rule from the
      mood-code miscompile. Rewrite as nested ifs.
- [x] **One B press = switch AND pickup** — `src/game.c:587-590`: make
      `Lara_useSwitch` return whether it used something; gate `Lara_pickup`.
- [x] **Neighbour-floor cache ignores Y** — `src/game.c:522-536`: stale next
      to vertical portals. Add a coarse Y term (e.g. `y >> 8`) to the key.
- [ ] Deferred (unreachable with TR1 data, note only): MatrixPush overflow
      desynchronizes later pops (`gsu/mesh.gsu:1544`); RoomsCollect indexes
      RL_VISITED with an unvalidated portal word (`:2724`); OTBuild has no
      OT_MAX cap; sprite types with zero-size textures leave entities on a
      memset-zero model (`out_SNES.h:1006`).

### 4. Lying comments (fix or they cause the next bug)

- [x] `main.asm:720` — "The blob is 22 KB": it is 65,225 of 65,534 bytes
      (309 bytes headroom; README already corrected).
- [x] `main.asm:700-716` — entity list "copied once, whole": read in place.
- [x] `gsu/gsu.inc:80` — R3 "$0000 or $5000": actually $1000/$6000.
- [x] `gsu/raster.gsu:326` — "RasterF has no caller": DrawFace's flat path
      calls it for every flat-coloured model face. (Its follow-up — flat
      spans PLOT uncached — is a live performance TODO.)
- [x] `gsu/mesh.gsu:412-419` — model header doc showed the old
      frameMask/frameBase format and mismatched node byte counts.
- [x] `gsu/gsu.inc:100-106, 172` — scratch map "spare $140-$1FF" is full;
      GS_* ends $F314 not $F311.

### Verify (in order)

1. `make` — rebuild everything.
2. `make fixedcheck matcheck animcheck` — fast host oracles.
3. `make SHADING=1 sim` — explicitly assembles and exercises the optional
   shade path in the interpreter.
4. `make check` — expect 10/10: the sector test goes green with the tick
   publish fix (1.6), the moveset test with the roll count (2.1).
5. Optional: `make SHADING=1 check` for the full shaded comparison.

Bigger items already on the README roadmap, restated by this review:
tick-rate telemetry (`ticksDropped` beside `fps`), and the blob's 309-byte
headroom blocking roadmap item 1 without restructuring.

/** FX3 inventory's native matrix publication, against its executable C oracle.
 * Run via run_typesnes.py with OL_ROM/OL_SYMS, OL_REFERENCE_ROM/OL_REFERENCE_SYMS.
 * OL_INVENTORY_PROFILE_OUT optionally records instruction counts.
 */
import { describe, expect, it } from 'vitest';
import { readFileSync, writeFileSync } from 'fs';
import { SNESEmulator } from './emulator';

function symbols(path: string) {
    const map = new Map<string, number>();
    for (const line of readFileSync(path, 'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) map.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
    }
    return map;
}
function cartridge(rom: string, symPath: string) {
    const syms = symbols(symPath), emu = new SNESEmulator();
    emu.loadROM(new Uint8Array(readFileSync(rom)));
    const memory = emu.getMemory(), cpu = emu.getCPU();
    const symbol = (name: string) => {
        const key = syms.has(name) ? name : [...syms.keys()].find(k => k.endsWith('_' + name));
        if (!key) throw new Error(`missing ${name} in ${symPath}`);
        return syms.get(key)!;
    };
    const w8 = (at: number, value: number) => memory.write(at, value & 255);
    const w16 = (at: number, value: number) => { w8(at, value); w8(at + 1, value >>> 8); };
    const w32 = (at: number, value: number) => { w16(at, value); w16(at + 2, value >>> 16); };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    const bytes = (at: number, count: number) => Buffer.from(Array.from({ length: count }, (_, i) => memory.read(at + i)));
    const call = (name: string, words: number[] = []) => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E);
        words.forEach((word, i) => w16(0x1EE4 + i * 2, word));
        const at = symbol(name); cpu.PB = at >>> 16; cpu.PC = at & 65535;
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 500000) cpu.step();
        expect(steps, `${name} never returned`).toBeLessThan(500000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} corrupted ABI`).toEqual([0x1EE3, 0x7E, 0, 0, 4]);
        return steps;
    };
    const set = (state: Record<string, number>) => Object.entries(state).forEach(([n, v]) => w16(symbol(n), v));
    call('gameInit');
    const table = symbol('LevelTable'), entry = 64, selected = Number(process.env.OL_BOOT_LEVEL ?? 0);
    const count = (symbol('SECTIONEND_level_table') - table) / entry;
    for (let i = 0; i < entry; i++) w8(symbol('olLevel') + i, memory.read(table + selected * entry + i));
    for (let level = 0; level < count; level++) for (let byte = 0; byte < 2; byte++) {
        w8(symbol('olLevelHostBanks') + level * 2 + byte, memory.read(table + level * entry + byte));
        w8(symbol('olLevelIds') + level * 2 + byte, memory.read(table + level * entry + 6 + byte));
    }
    set({ olLevelIndex: selected, olLevelCount: count });
    call('olHostBind'); expect(u16(symbol('olHostTablesReady'))).toBe(1); call('olLevelInit');
    return { symbol, w8, w16, w32, u16, bytes, call, set };
}

const ROM = process.env.OL_ROM!, SYMS = process.env.OL_SYMS!;
const REF = process.env.OL_REFERENCE_ROM!, REF_SYMS = process.env.OL_REFERENCE_SYMS!;
const HAS_REF = Boolean(REF && REF_SYMS);

describe.skipIf(!HAS_REF)('OpenLara FX3: cached health bar', () => {
    it('preserves every health width, timer, inventory transition and level reset against C', () => {
        const pair = [cartridge(ROM, SYMS), cartridge(REF, REF_SYMS)];
        let cases = 0;
        const tick = (health: number, state: Record<string, number> = {}, slot = 20) => {
            const snapshots: Buffer[] = [];
            for (const g of pair) {
                g.w16(g.symbol('items') + 16, health);
                g.set({ olInvItemIndex: 0, ...state });
                g.w16(g.symbol('olInvItemsList'), slot);
                g.w8(g.symbol('olHudDirty'), 0);
                g.call('olHudUpdate');
                snapshots.push(Buffer.concat([
                    g.bytes(g.symbol('olHudOam'), 104), g.bytes(g.symbol('olHudDirty'), 1),
                    ...['olHudHealth', 'olHudTimer', 'olHudKey'].map(n => g.bytes(g.symbol(n), 2)),
                ]));
            }
            expect(snapshots[0], `HUD tick ${cases++}, health ${health}`).toEqual(snapshots[1]);
        };
        for (let health = 0; health <= 1000; health++) {
            tick(health, { olInvState: 0, olLaraWaterState: 0, olLaraWeaponState: 4 });
            tick(health); // unchanged health must take the same visible path
            tick(health, { olInvState: 4 }); // medipack shows the same width elsewhere
        }
        // Healing in the ring must not count down Lara's healthTimer. Closing
        // restores the game bar; each subsequent tick resumes the old timer.
        for (const health of [500, 1000, 0, -1, -32768, 1001, 2047, 32000, 32767, 750]) {
            tick(health, { olInvState: 4 }, 19);
            tick(health, { olInvState: 6 });
            tick(health, { olInvState: 4 }, 0);
            tick(health, { olInvState: 0, olLaraWeaponState: 0 });
            for (let frame = 0; frame < 45; frame++) tick(health);
        }
        for (let oxygen = 1800; oxygen >= 0; oxygen--) {
            tick(500, { olInvState: 0, olLaraWaterState: oxygen & 1 ? 2 : 3, olLaraOxygen: oxygen });
        }
        // Level start/load seeds these three presentation fields, even when
        // its health equals the previous level's cached value.
        for (const health of [1000, 500, 500, 0, 1000]) {
            for (const g of pair) g.set({ olHudHealth: health, olHudTimer: 100, olHudKey: 0xFFFF });
            for (let frame = 0; frame < 102; frame++) tick(health, { olInvState: 0, olLaraWaterState: 0 });
        }
        tick(1000, { olInvState: 0, olLaraWaterState: 0, olLaraWeaponState: 4 });
        const costs = pair.map(g => {
            let steps = 0;
            for (let i = 0; i < 100; i++) steps += g.call('olHudUpdate');
            return steps;
        });
        expect(costs[0], 'unchanged health should avoid the old divide').toBeLessThan(costs[1]);
        console.log(`[openlara] HUD: ${cases} exact C OAM/dirty/key/timer snapshots; 100 unchanged ticks ${costs.join('/')} instructions`);
    });
});

const boundaries = [0, 1, -1, 16383, 16384, -16383, -16384, -16385, 32767, -32768,
    0x3FFFFFFF, 0x40000000, 0x7FFFFFFF, -0x80000000, -0x40000001, -0x40000000,
    0x12345678, -0x12345678];
let random = 0x89ABCDEF;
function next() { random = (Math.imul(random, 1664525) + 1013904223) >>> 0; return random | 0; }
function packed(matrix: number[]) {
    return matrix.map((v, i) => {
        let n = i % 4 === 3 ? (v >> 14) << 16 >> 16 : Math.max(-32767, Math.min(32767, v << 1));
        if (i < 8) n = Math.floor(n * 23 / 32);
        return n & 65535;
    });
}

describe.skipIf(!HAS_REF)('OpenLara FX3: native inventory publication', () => {
    it('swaps full matrix columns and packs signed extrema exactly like the C cartridge', () => {
        const pair = [cartridge(ROM, SYMS), cartridge(REF, REF_SYMS)], output = 0x7FC600;
        let nativeSteps = 0, referenceSteps = 0;
        for (let test = 0; test < 512; test++) {
            const matrix = Array.from({ length: 12 }, (_, i) => test < boundaries.length ? boundaries[(test + i) % boundaries.length] : next());
            const want = matrix.slice();
            for (let row = 0; row < 12; row += 4) { want[row] = -matrix[row + 2] | 0; want[row + 2] = matrix[row]; }
            const snapshots: Buffer[] = [];
            for (const [index, g] of pair.entries()) {
                const at = g.symbol('olInvM'); matrix.forEach((v, i) => g.w32(at + i * 4, v));
                const steps = g.call('Inv_rotY90'); index ? referenceSteps += steps : nativeSteps += steps;
                const expected = Buffer.alloc(48); want.forEach((v, i) => expected.writeInt32LE(v, i * 4));
                expect(g.bytes(at, 48), `quarter turn ${test}`).toEqual(expected);
                matrix.forEach((v, i) => g.w32(at + i * 4, v));
                g.w16(output - 2, 0xA55A); g.w16(output + 24, 0x5AA5);
                const packSteps = g.call('Inv_pack', [output & 65535, output >>> 16]);
                index ? referenceSteps += packSteps : nativeSteps += packSteps;
                expect([g.u16(output - 2), g.u16(output + 24)]).toEqual([0xA55A, 0x5AA5]);
                expect(g.bytes(at, 48)).toEqual(Buffer.from(matrix.flatMap(v => [v & 255, v >>> 8 & 255, v >>> 16 & 255, v >>> 24 & 255])));
                expect(Array.from({ length: 12 }, (_, i) => g.u16(output + i * 2)), `pack ${test}`).toEqual(packed(matrix));
                snapshots.push(g.bytes(output, 24));
            }
            expect(snapshots[0]).toEqual(snapshots[1]);
        }
        console.log(`[openlara] inventory helpers: 512 exact native/C full-width matrix and packed records; ${nativeSteps}/${referenceSteps} instructions`);
    });

    it('publishes complete ring records and final matrices exactly like the C cartridge', () => {
        const pair = [cartridge(ROM, SYMS), cartridge(REF, REF_SYMS)];
        let nativeSteps = 0, referenceSteps = 0;
        for (let test = 0; test < 96; test++) {
            const snapshots: Buffer[] = [];
            for (const [index, g] of pair.entries()) {
                g.call('Inventory_init'); g.call('Inventory_open', [test & 1 ? 6 : 4, 0, 0, 0]);
                const count = g.u16(g.symbol('olInvItemsCount'));
                g.set({ olInvState: [1, 2, 4, 6, 7, 8, 9, 10, 11, 12][test % 10],
                    olInvHeight: [-1536, -256, 0, -1000][test % 4], olInvRadius: [0, 688, 344][test % 3],
                    olInvPitch: [0, 4096, -4096, 8192][test % 4], olInvRot: test * 1337 & 65535,
                    olInvRotItem: test * 2345 & 65535, olInvItemIndex: test % count,
                    olInvSelDist: [0, 256, 512][test % 3], olInvSelRotPre: test % 3 ? 0 : -3640,
                    olInvSelRotX: test % 4 ? 0 : 8192, olInvSelRotY: test % 5 ? 0 : 16384,
                    olInvFrameIndex: test % 15, olInvFrameTarget: test % 15 + 1, olInvVisMask: 0xFF00 | test });
                const output = g.symbol('olInvGsu'); for (let i = 0; i < 418; i++) g.w8(output + i, 0);
                const steps = g.call('Inventory_publish'); index ? referenceSteps += steps : nativeSteps += steps;
                snapshots.push(Buffer.concat([g.bytes(output, 418), g.bytes(g.symbol('olInvM'), 48)]));
            }
            expect(snapshots[0], `full ring ${test}`).toEqual(snapshots[1]);
        }
        console.log(`[openlara] inventory publish: 96 exact native/C olInvGsu and final olInvM snapshots; ${nativeSteps}/${referenceSteps} instructions`);
    });
});

it('profiles native ring publication across visible inventory states', () => {
    const g = cartridge(ROM, SYMS), samples: { name: string; items: number; instructions: number }[] = [];
    const sample = (name: string) => samples.push({ name, items: g.u16(g.symbol('olInvItemsCount')), instructions: g.call('Inventory_publish') });
    g.call('Inventory_open', [4, 0, 0, 0]); sample('opening');
    for (let i = 0; i < 16; i++) g.call('Inventory_update', [0, 1]); sample('ready-compass');
    g.set({ olInvItemIndex: 6 }); sample('ready-medipack');
    g.set({ olInvState: 9, olInvSelDist: 256, olInvSelRotX: -3640, olInvSelRotY: -16384 }); sample('show-medipack');
    g.call('Inventory_open', [6, 0, 0, 0]); for (let i = 0; i < 16; i++) g.call('Inventory_update', [0, 1]); sample('options-ready');
    console.log(`[openlara] inventory publication instructions: ${JSON.stringify(samples)}`);
    if (process.env.OL_INVENTORY_PROFILE_OUT) writeFileSync(process.env.OL_INVENTORY_PROFILE_OUT, JSON.stringify(samples, null, 2));
});

// Presentation is measured from the actual NMI buffer flips while the ring
// remains visible. Publication instruction counts alone do not measure FPS.
describe.skipIf(process.env.OL_INVENTORY_CADENCE !== '1')('OpenLara FX3: displayed ring cadence', () => {
    it('measures baseline and optimized Small Medi Pack rings over 600 fields', () => {
        const results: { build: string; fields: number; flips: number; fps: number; ticks: number; dropped: number }[] = [];
        for (const [build, rom, symPath] of [['baseline', process.env.OL_BASELINE_ROM!, process.env.OL_BASELINE_SYMS!], ['optimized', ROM, SYMS]]) {
            const syms = symbols(symPath), symbol = (name: string) => syms.get(name)!;
            const emu = new SNESEmulator(); emu.loadROM(new Uint8Array(readFileSync(rom)));
            const memory = emu.getMemory() as ReturnType<SNESEmulator['getMemory']> & { writeCartridgeRAM(at: number, value: number): void };
            [0x4F, 0x4C, 0x42, 0x52, 0, 0, 0, 0].forEach((v, i) => memory.writeCartridgeRAM(0x3100 + i, v));
            [0x4F, 0x4C, 0x53, 0x54, 1, 1, 0, 1, 1, 0, 0, 0].forEach((v, i) => memory.writeCartridgeRAM(0x3110 + i, v));
            const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
            const until = (test: () => boolean, limit = 600) => {
                for (let i = 0; i < limit && !test(); i++) emu.executeFrames(1);
                expect(test(), `${build}: visible ring state was never reached`).toBe(true);
            };
            until(() => u16(symbol('itemsCount')) > 0); emu.executeFrames(60);
            const pad = emu.getController() as unknown as { buttonState: number };
            pad.buttonState = 0x2000; emu.executeFrames(4); pad.buttonState = 0; emu.executeFrames(4);
            until(() => u16(symbol('olInvState')) === 4);
            pad.buttonState = 0x0100; emu.executeFrames(4); pad.buttonState = 0; emu.executeFrames(4);
            until(() => u16(symbol('olInvState')) === 4 && u16(symbol('olInvItemIndex')) === 6);
            emu.executeFrames(90);
            const ticks = u16(symbol('ol_ticks')), dropped = u16(symbol('ol_ticksDropped'));
            let buffer = memory.read(0x7E0002), flips = 0;
            for (let field = 0; field < 600; field++) {
                emu.executeFrames(1); const now = memory.read(0x7E0002);
                if (now !== buffer) { flips++; buffer = now; }
            }
            const record = { build, fields: 600, flips, fps: flips / 10,
                ticks: (u16(symbol('ol_ticks')) - ticks) & 65535,
                dropped: (u16(symbol('ol_ticksDropped')) - dropped) & 65535 };
            results.push(record);
            expect(u16(symbol('olInvState'))).toBe(4);
            expect(record.flips).toBeGreaterThan(0); expect(record.dropped).toBe(0);
            console.log(`[openlara] displayed inventory cadence: ${JSON.stringify(record)}`);
        }
        if (process.env.OL_INVENTORY_CADENCE_OUT) writeFileSync(process.env.OL_INVENTORY_CADENCE_OUT, JSON.stringify(results, null, 2));
    });
});

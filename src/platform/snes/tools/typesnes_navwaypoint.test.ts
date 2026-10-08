/** Complete waypoint state and linked cost against the preceding ROM and C. */
import { describe, it, expect } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

function machine(romPath: string, symbolPath: string) {
    const symbols = new Map<string, number>();
    for (const line of readFileSync(symbolPath, 'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) symbols.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
    }
    const sym = (name: string) => {
        const key = symbols.has(name) ? name : [...symbols.keys()].find(k => k.endsWith('_' + name));
        const address = symbols.get(key ?? '');
        if (address === undefined) throw new Error(`missing ${name} in ${symbolPath}`);
        return address;
    };
    const emulator = new SNESEmulator();
    emulator.loadROM(new Uint8Array(readFileSync(romPath)));
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    const bus = (emulator as unknown as { bus: { masterCycles: number } }).bus;
    const w8 = (at: number, value: number) => {
        if (at >= 0xE00000) (memory as unknown as { rom: Uint8Array }).rom[at - 0xC00000] = value & 255;
        else memory.write(at, value & 255);
    };
    const w16 = (at: number, value: number) => { w8(at, value); w8(at + 1, value >>> 8); };
    const w32 = (at: number, value: number) => { w16(at, value); w16(at + 2, value >>> 16); };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    const bytes = (at: number, size: number) => Buffer.from(Array.from({ length: size }, (_, i) => memory.read(at + i)));
    const call = (words: number[]) => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E);
        words.forEach((value, i) => w16(0x1EE4 + i * 2, value));
        w16(0x1EE4 + words.length * 2, 0xD00D); w16(0x1C00, 0xABCD);
        cpu.PB = sym('Nav_getWaypoint') >>> 16; cpu.PC = sym('Nav_getWaypoint') & 65535;
        const before = bus.masterCycles;
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 100000) cpu.step();
        expect(steps, 'waypoint did not return').toBeLessThan(100000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], 'waypoint C ABI').toEqual([0x1EE3, 0x7E, 0, 0, 4]);
        expect([u16(0x1EE4 + words.length * 2), u16(0x1C00)], 'stack canaries').toEqual([0xD00D, 0xABCD]);
        return { steps, clocks: bus.masterCycles - before };
    };
    return { sym, w8, w16, w32, u16, bytes, call };
}

const REF = process.env.OL_REFERENCE_ROM, REF_SYMS = process.env.OL_REFERENCE_SYMS;
// Separate from OL_BASELINE_ROM: other performance fixtures retain their own
// historical baselines, so selecting this comparison cannot weaken them.
const BEFORE = process.env.OL_NAV_BASELINE_ROM, BEFORE_SYMS = process.env.OL_NAV_BASELINE_SYMS;
describe.skipIf(!REF || !REF_SYMS || !BEFORE || !BEFORE_SYMS)('OpenLara FX3: waypoint copies', () => {
    it('preserves complete corridor state with far source points, all three pools and terminal paths', () => {
        const machines = [machine(process.env.OL_ROM!, process.env.OL_SYMS!),
            machine(BEFORE!, BEFORE_SYMS!), machine(REF!, REF_SYMS!)];
        const cost = machines.map(() => ({ steps: 0, clocks: 0 }));
        let cases = 0;
        for (let slot = 0; slot < 3; slot++) for (const sourceBank of [0x7E, 0x7F])
        for (let path = 0; path < 6; path++) for (let point = 0; point < 16; point++) {
            const sx = [0, 63, 128][slot], sz = [0, 31, 191][slot];
            const minX = sx * 1024, minZ = sz * 1024, maxX = minX + 2047, maxZ = minZ + 2047;
            const positions = [[minX, minZ], [maxX, maxZ], [minX + 512, minZ + 512],
                [minX - 1, minZ], [maxX + 1, maxZ], [minX, minZ - 1], [maxX, maxZ + 1],
                [minX - 1, minZ - 1], [maxX + 1, maxZ + 1], [minX + 1024, minZ + 1024],
                [-2147483648, 2147483647], [2147483647, -2147483648], [-1, 65536],
                [65535, 65536], [minX, maxZ], [maxX, minZ]];
            const source = point === 14 ? 65535 : point === 15 ? 7 : 0;
            const from = sourceBank * 65536 + 0x0520;
            const snapshots: Buffer[] = [];
            for (let m = 0; m < machines.length; m++) {
                const g = machines[m], n = g.sym('olNav') + slot * 8240;
                // Precomputed paths isolate the corridor from flood-fill cost;
                // the existing C++ Nav oracle covers live queue expansion.
                for (let i = 0; i < 8240; i++) g.w8(n + i, i * 43 + 17);
                g.w16(n - 2, 0xA55A); g.w16(n + 8240, 0x5AA5);
                g.w16(g.sym('olBoxCount'), 8); g.w16(g.sym('olBoxBase'), 0);
                const boxes = 0xFAFFC0; // last full eight-box table in its ROM bank
                g.w32(g.sym('olBoxData'), boxes);
                for (let i = 0; i < 8; i++) {
                    const x = sx + (path === 1 ? 0 : i), z = sz + (path === 5 ? i : 0);
                    [z, z + 2, x, x + 2].forEach((value, j) => g.w8(boxes + i * 8 + j, value));
                    g.w16(boxes + i * 8 + 4, i * 256 - 512);
                    g.w16(boxes + i * 8 + 6, path === 3 && i === 2 ? 0x4000 : 0);
                    g.w16(n + i * 8, i); g.w16(n + i * 8 + 2, 3);
                    g.w16(n + i * 8 + 4, path === 2 ? (i + 1) % 7 : path === 4 && i === 2 ? 65535 : i + 1);
                    g.w16(n + i * 8 + 6, 65535);
                }
                [8, slot, 3, 7, 7, 65535, 65535, 256, -1024, slot === 2 ? 16 : 0, 0x4000, slot]
                    .forEach((value, i) => g.w16(n + 8192 + i * 2, value));
                [minX + 7680, -32768, minZ + 1024].forEach((v, i) => g.w32(n + 8216 + i * 4, v));
                [positions[point][0], point * 8191 - 32768, positions[point][1]]
                    .forEach((v, i) => g.w32(from + i * 4, v));
                g.w16(from - 2, 0xBEEF); g.w16(from + 12, 0xCAFE);
                for (const [name, size] of [['olNavClipFrom', 12], ['olNavBounds', 16], ['olNavClipBounds', 16]] as const)
                    for (let i = 0; i < size; i++) g.w8(g.sym(name) + i, i * 29 + 61);
                g.w32(g.sym('olNavClipWP'), 0x7E0520); g.w16(g.sym('olNavClip'), 0xA53C);
                const result = g.call([n & 65535, 0x7E, source, from & 65535, sourceBank]);
                cost[m].steps += result.steps; cost[m].clocks += result.clocks;
                const wp = g.u16(g.sym('olNavClipWP')) | g.u16(g.sym('olNavClipWP') + 2) << 16;
                expect(wp).toBe(source >= 8 || source === 7 ? 0x7E0520 : n + 8228);
                expect([g.u16(n - 2), g.u16(n + 8240), g.u16(from - 2), g.u16(from + 12)])
                    .toEqual([0xA55A, 0x5AA5, 0xBEEF, 0xCAFE]);
                snapshots.push(Buffer.concat([g.bytes(n - 2, 8244), g.bytes(from - 2, 16),
                    g.bytes(g.sym('olNavClipFrom'), 12), g.bytes(g.sym('olNavBounds'), 16),
                    g.bytes(g.sym('olNavClipBounds'), 16), g.bytes(g.sym('olNavClip'), 2)]));
            }
            expect(snapshots[0], `baseline ${slot}/${sourceBank}/${path}/${point}`).toEqual(snapshots[1]);
            expect(snapshots[0], `C ${slot}/${sourceBank}/${path}/${point}`).toEqual(snapshots[2]);
            cases++;
        }
        expect(cost[0].steps).toBeLessThan(cost[1].steps);
        expect(cost[0].clocks).toBeLessThan(cost[1].clocks);
        console.log(`[openlara] waypoint copies: ${cases} exact native/baseline/C complete snapshots; ` +
            `inside/outside edges, signed extremes, cyclic/blocked/dead/finished/invalid paths, three pools and two source banks; ` +
            `${cost.map(c => c.steps).join('/')} instructions, ${cost.map(c => c.clocks).join('/')} master clocks`);
    });
});

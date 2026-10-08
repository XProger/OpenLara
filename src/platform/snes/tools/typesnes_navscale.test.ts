/** Exact full-width navigation scales and their linked native cost. */
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
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    const s32 = (at: number) => u16(at) | u16(at + 2) << 16;
    const bind = (name: string, at: number) => { w16(sym(name), at); w16(sym(name) + 2, at >>> 16); };
    const call = (name: string, words: number[]) => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E);
        words.forEach((value, i) => w16(0x1EE4 + i * 2, value));
        cpu.PB = sym(name) >>> 16; cpu.PC = sym(name) & 65535;
        const before = bus.masterCycles;
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 50000) cpu.step();
        expect(steps, `${name} did not return`).toBeLessThan(50000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} C ABI`).toEqual([0x1EE3, 0x7E, 0, 0, 4]);
        return { steps, clocks: bus.masterCycles - before };
    };
    return { sym, memory, w8, w16, u16, s32, bind, call };
}

const REFERENCE_ROM = process.env.OL_REFERENCE_ROM, REFERENCE_SYMS = process.env.OL_REFERENCE_SYMS;
const BASELINE_ROM = process.env.OL_BASELINE_ROM ?? REFERENCE_ROM;
const BASELINE_SYMS = process.env.OL_BASELINE_SYMS ?? REFERENCE_SYMS;
describe.skipIf(!REFERENCE_ROM || !REFERENCE_SYMS)('OpenLara FX3: navigation sector scales', () => {
    const native = () => machine(process.env.OL_ROM!, process.env.OL_SYMS!);
    const baseline = () => machine(BASELINE_ROM!, BASELINE_SYMS!);
    const reference = () => machine(REFERENCE_ROM!, REFERENCE_SYMS!);

    it('matches every sector byte and maximum borrow in packed box pairs at CPU-only bank boundaries', () => {
        const machines = [native(), baseline(), reference()];
        const cost = machines.map(() => ({ steps: 0, clocks: 0 }));
        let cases = 0;
        for (const base of [0xFA0000, 0xFAE000]) for (let test = 0; test < 2048; test++) {
            const box = (test * 97) & 1023, at = base + box * 8;
            // All 256 byte values occupy each bound, with zero and every
            // 64-sector carry boundary also paired with unrelated bounds.
            const minX = test & 255, maxX = (test >>> 8) & 1 ? 0 : (test * 73 + 19) & 255;
            const minZ = (test * 37 + 11) & 255, maxZ = (test >>> 8) & 2 ? ((test >>> 8) * 64) & 255 : (test * 53 + 7) & 255;
            const want = [minX * 1024, (maxX & 255) * 1024 - 1, minZ * 1024, maxZ * 1024 - 1];
            for (let m = 0; m < machines.length; m++) {
                const g = machines[m]; g.bind('olBoxData', base);
                g.w8(at, minZ); g.w8(at + 1, maxZ); g.w8(at + 2, minX); g.w8(at + 3, maxX);
                const bounds = g.sym('olNavBounds');
                g.w16(bounds - 2, 0xA55A); g.w16(bounds + 16, 0x5AA5);
                const result = g.call('Nav_bounds', [box]);
                cost[m].steps += result.steps; cost[m].clocks += result.clocks;
                expect(Array.from({ length: 4 }, (_, i) => g.s32(bounds + i * 4)), `bounds ${base.toString(16)}/${test}`).toEqual(want);
                expect([g.u16(bounds - 2), g.u16(bounds + 16)]).toEqual([0xA55A, 0x5AA5]);
            }
            cases++;
        }
        expect(cost[0].steps).toBeLessThan(cost[1].steps);
        console.log(`[openlara] nav bounds: ${cases} native/baseline/C exact packed pairs, all sector bytes and 32-bit maximum borrows, $FA bank start/end; ` +
            `${cost[0].steps}/${cost[1].steps}/${cost[2].steps} instructions, ${cost[0].clocks}/${cost[1].clocks}/${cost[2].clocks} master clocks`);
    });

    it('matches signed wrapped room-relative positions across all world sector bytes and output banks', () => {
        const machines = [native(), baseline(), reference()];
        const cost = machines.map(() => ({ steps: 0, clocks: 0 }));
        const signed = (v: number) => v << 16 >> 16;
        const locals = [-32768,-32767,-1024,-1,0,1,1024,32767];
        let cases = 0;
        for (const base of [0xF80000, 0xF8FB00]) for (let test = 0; test < 2048; test++) {
            const room = test & 127, at = base + room * 10;
            const sx = test & 255, sz = (test * 53 + 29) & 255;
            const ox = signed(test * 251 + 32760), oz = signed(test * 103 - 32760);
            const lx = locals[test >>> 8], lz = locals[(test >>> 5) & 7], y = locals[(test >>> 2) & 7];
            const x = signed(ox + lx), z = signed(oz + lz);
            const output = test & 1 ? 0x7F0540 : 0x7E0520;
            const want = [sx * 1024 + signed(x - ox), y, sz * 1024 + signed(z - oz)];
            for (let m = 0; m < machines.length; m++) {
                const g = machines[m]; g.bind('olRoomData', base);
                g.w8(at, sx); g.w8(at + 1, sz); g.w16(at + 2, ox); g.w16(at + 4, oz);
                g.w16(output - 2, 0xA55A); g.w16(output + 12, 0x5AA5);
                const result = g.call('Nav_point', [output & 65535, output >>> 16, room, x, y, z]);
                cost[m].steps += result.steps; cost[m].clocks += result.clocks;
                expect(Array.from({ length: 3 }, (_, i) => g.s32(output + i * 4)), `point ${base.toString(16)}/${test}`).toEqual(want);
                expect([g.u16(output - 2), g.u16(output + 12)]).toEqual([0xA55A, 0x5AA5]);
            }
            cases++;
        }
        expect(cost[0].steps).toBeLessThan(cost[1].steps);
        console.log(`[openlara] nav points: ${cases} native/baseline/C exact signed wrapped rooms, all sector bytes, $F8 bank start/end and $7E/$7F outputs; ` +
            `${cost[0].steps}/${cost[1].steps}/${cost[2].steps} instructions, ${cost[0].clocks}/${cost[1].clocks}/${cost[2].clocks} master clocks`);
    });
});

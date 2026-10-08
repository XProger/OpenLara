/** Exact packed-tag counting in the linked FX3 and relocated GSU-2 codec. */
import { describe, it, expect } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

const FX3 = process.env.OL_FX3 === '1';
function machine(romPath: string, symbolPath: string) {
    const symbols = new Map<string, number>();
    for (const line of readFileSync(symbolPath, 'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) symbols.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
    }
    const sym = (name: string) => {
        const address = symbols.get(name);
        if (address === undefined) throw new Error(`missing ${name} in ${symbolPath}`);
        return address;
    };
    const rom = new Uint8Array(readFileSync(romPath));
    const emulator = new SNESEmulator(); emulator.loadROM(rom);
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    const bus = (emulator as unknown as { bus: { masterCycles: number } }).bus;
    const w8 = (at: number, value: number) => {
        if (FX3 && at >= 0xE00000) (memory as unknown as { rom: Uint8Array }).rom[at - 0xC00000] = value & 255;
        else memory.write(at, value & 255);
    };
    const w16 = (at: number, value: number) => { w8(at, value); w8(at + 1, value >>> 8); };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    if (!FX3) for (let i = 0; i < 0x8000; i++) w8(0x7E8000 + i, rom[63 * 0x8000 + i]);
    cpu.E = 0; cpu.DB = 0x7E; cpu.D = 0;
    w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E);
    const call = (name: string, accumulator: number, words: number[] = [], shortReturn = false, flags?: number) => {
        cpu.P = 4 | (flags ?? ((accumulator & 1) | (accumulator & 2 ? 0x40 : 0)));
        cpu.A = accumulator; cpu.S = 0x1EE0;
        words.forEach((value, i) => w16(0x1EE4 + i * 2, value));
        const address = sym(name), returnBank = shortReturn ? address >>> 16 : 0x7E;
        cpu.PB = address >>> 16; cpu.PC = address & 65535;
        const before = bus.masterCycles;
        let steps = 0;
        while ((cpu.PB !== returnBank || cpu.PC !== 0x500) && steps++ < 10000) cpu.step();
        if (steps >= 10000 || cpu.S !== 0x1EE0 + (shortReturn ? 2 : 3) || cpu.DB !== 0x7E || cpu.D !== 0 ||
            cpu.E !== 0 || (cpu.P & 0x3C) !== 4) throw new Error(`${name} failed to return with the C register widths/banks`);
        return { steps, clocks: bus.masterCycles - before };
    };
    return { memory, cpu, sym, w8, w16, u16, call };
}

describe(`OpenLara ${FX3 ? 'FX3' : 'GSU-2'}: packed navigation tag counts`, () => {
    it('counts all 65536 tag words at every prefix with exact carries, pointers and source cursor', () => {
        const g = machine(process.env.OL_ROM!, process.env.OL_SYMS!);
        const masks = Array.from({ length: 8 }, (_, i) => g.u16(g.sym('Nav_PrefixMasks') + i * 2)).concat(65535);
        expect(masks).toEqual([0,3,15,63,255,1023,4095,16383,65535]);
        expect(g.sym('Nav_PrefixMasks') >>> 16).toBe(FX3 ? 0x89 : 0x7E);
        const preserved = ['tcc__r0','tcc__r0h','tcc__r1','tcc__r1h','tcc__r2','tcc__r2h','tcc__r3','tcc__r3h',
            'tcc__r4','tcc__r4h','tcc__r5','tcc__r5h','tcc__r9h'];
        preserved.forEach((name, i) => g.w16(g.sym(name), 0xA500 + i));
        g.cpu.X = 0xBEEF; g.cpu.Y = 0xCAFE;
        let cases = 0, instructions = 0, clocks = 0;
        for (let word = 0; word < 65536; word++) {
            let count = 0;
            for (let prefix = 0; prefix <= 8; prefix++) {
                if (prefix && (word >>> ((prefix - 1) * 2) & 3) === 3) count++;
                const result = g.call('Nav_CountLiterals', word & masks[prefix], [], true);
                if (g.cpu.A !== count || g.cpu.X !== 0xBEEF || g.cpu.Y !== 0xCAFE)
                    throw new Error(`tag word ${word.toString(16)} prefix ${prefix}: ${g.cpu.A} instead of ${count}`);
                instructions += result.steps; clocks += result.clocks; cases++;
            }
        }
        // Decouple incoming flags from input bits on carry-boundary words,
        // including the zero prefix that masks every source word to zero.
        for (const word of [0,1,2,3,0x5555,0xAAAA,0xFFFE,0xFFFF]) for (let prefix = 0; prefix <= 8; prefix++)
        for (const flags of [0,1,0x40,0x41]) {
            const want = Array.from({ length: prefix }, (_, i) => word >>> (i * 2) & 3).filter(t => t === 3).length;
            g.call('Nav_CountLiterals', word & masks[prefix], [], true, flags);
            expect([g.cpu.A,g.cpu.X,g.cpu.Y], `incoming flags ${flags} word ${word} prefix ${prefix}`)
                .toEqual([want,0xBEEF,0xCAFE]);
        }
        preserved.forEach((name, i) => expect(g.u16(g.sym(name)), name).toBe(0xA500 + i));
        expect(cases).toBe(589824);
        console.log(`[openlara] tag count: ${cases} linked word/prefix cases with varied incoming carry/overflow, ` +
            `288 independent flag-boundary cases, ` +
            `codec pointers/rank/remaining count and X/Y exact; ${instructions} instructions, ${clocks} master clocks`);
    });

    it.skipIf(!process.env.OL_BASELINE_ROM || !process.env.OL_BASELINE_SYMS)(
        'retains rank, mixed predictors and partial-word masks through the complete sector decoder', () => {
        const native = machine(process.env.OL_ROM!, process.env.OL_SYMS!);
        const baseline = machine(process.env.OL_BASELINE_ROM!, process.env.OL_BASELINE_SYMS!);
        const costs = [native, baseline].map(() => ({ steps: 0, clocks: 0 }));
        const patterns = [0,65535,0xAAAA,0x5555,0x3333,0xCCCC,0xEEEE,0xBBBB,
            ...Array.from({ length: 24 }, (_, i) => Math.imul(i + 17, 40503) & 65535)];
        let cases = 0;
        for (const group of [0,32]) for (let prefix = 0; prefix < 32; prefix++) for (const word of patterns) {
            const sector = group + prefix;
            const tags = Array(64).fill(3);
            for (let i = 0; i < 32; i++) tags[group + i] = word >>> ((i & 7) * 2) & 3;
            tags[sector] = 3;
            const wanted = group + tags.slice(group, sector).filter(t => t === 3).length;
            for (let m = 0; m < 2; m++) {
                const g = [native, baseline][m], map = FX3 ? 0xF8FF00 : 0x710200;
                g.w16(g.sym('olSectorCount'), 64); g.w16(g.sym('olBoxCount'), 1024);
                if (FX3) { g.w16(g.sym('olNavMapData'), map); g.w16(g.sym('olNavMapData') + 2, map >>> 16); }
                else { g.w16(0x710000, 4096); g.w16(0x710FFE, map & 65535); }
                g.w8(map, 0x4E); g.w8(map + 1, 6);
                for (let i = 0; i < 16; i++) g.w8(map + 2 + i,
                    tags[i * 4] | tags[i * 4 + 1] << 2 | tags[i * 4 + 2] << 4 | tags[i * 4 + 3] << 6);
                [0,32,64].forEach((rank, i) => g.w16(map + 18 + i * 2, rank));
                for (let i = 0; i < 52; i++) g.w8(map + 24 + i, 0);
                for (let id = 0; id < 64; id++) for (let bit = 0; bit < 6; bit++) {
                    const at = id * 6 + bit, address = map + 24 + (at >>> 3);
                    g.w8(address, g.memory.read(address) | (id >>> bit & 1) << (at & 7));
                }
                const result = g.call('navSectorBox', word, [sector,8]);
                expect(g.u16(g.sym('tcc__r0')), `sector ${sector}, word ${word.toString(16)}`).toBe(wanted);
                costs[m].steps += result.steps; costs[m].clocks += result.clocks;
            }
            cases++;
        }
        expect(costs[0].steps).toBeLessThan(costs[1].steps);
        expect(costs[0].clocks).toBeLessThan(costs[1].clocks);
        console.log(`[openlara] sector counts: ${cases} exact native/baseline full decodes, all 32 prefixes, mixed 0/1/2/3 tags, ` +
            `rank bases 0/32 and bank-end map; ${costs[0].steps}/${costs[1].steps} instructions, ` +
            `${costs[0].clocks}/${costs[1].clocks} master clocks`);
    });
});

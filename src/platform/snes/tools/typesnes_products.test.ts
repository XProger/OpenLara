/** Linked compiler products on FX3 and the five relocated GSU-2 code images.
 * Independent modulo arithmetic is always checked. Optional OL_BASELINE_ROM /
 * OL_BASELINE_SYMS compare costs with an unchanged cartridge helper runtime.
 */
import { describe, it, expect } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

const FX3 = process.env.OL_FX3 === '1';
type Product = 'tcc__mul' | 'tcc__mull';
function machine(romPath: string, symbolPath: string) {
    const symbols = new Map<string, number>();
    for (const line of readFileSync(symbolPath, 'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) symbols.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
    }
    const sym = (name: string) => {
        const at = symbols.get(name);
        if (at === undefined) throw new Error(`missing ${name} in ${symbolPath}`);
        return at;
    };
    const rom = new Uint8Array(readFileSync(romPath));
    const emulator = new SNESEmulator(); emulator.loadROM(rom);
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    const bus = (emulator as unknown as { bus: { masterCycles: number } }).bus;
    const w8 = (at: number, v: number) => memory.write(at, v & 255);
    const w16 = (at: number, v: number) => { w8(at, v); w8(at + 1, v >>> 8); };
    const w32 = (at: number, v: number) => { w16(at, v); w16(at + 2, v >>> 16); };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    if (!FX3) for (const [bank, at, size] of [[63, 0x7E8000, 0x8000], [62, 0x7E5800, 0x2800],
        [61, 0x7E0600, 0x1600], [4, 0x7F0000, 0x8000], [5, 0x7F8000, 0x8000]])
        for (let i = 0; i < size; i++) w8(at + i, rom[bank * 0x8000 + i]);
    if (FX3) for (let i = 0; i < sym('HostCodeEnd') - sym('HostCodeStart'); i++)
        w8(0x7E0100 + i, rom[0x8000 + i]);
    w8(0x4200, 0);
    const preserved = Array.from({ length: 6 }, (_, r) => [sym(`tcc__r${r}`), sym(`tcc__r${r}h`)]).flat();
    const canaries = preserved.map((_, i) => (i & 1 ? 0xA550 : 0x5AA0) + i);
    preserved.forEach((at, i) => w16(at, canaries[i]));
    w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E);
    w16(0x1EE4, 0xD00D); w16(0x1DC0, 0xABCD);
    const entries = { tcc__mul: sym('tcc__mul'), tcc__mull: sym('tcc__mull') };
    const operandA = sym('tcc__r9'), operandB = sym('tcc__r10');
    let instructions = 0, clocks = 0, calls = 0;
    const call = (name: Product, a: number, b: number, inject = -1) => {
        if (inject >= 0) for (let i = 0; i < 0x30; i++) w8(i, 0);
        w32(operandA, a); w32(operandB, b);
        cpu.E = 0; cpu.D = 0; cpu.DB = 0x7E; cpu.S = 0x1EE0;
        cpu.P = 4 | (a & 1) | (b & 2 ? 0x40 : 0); // independent incoming C/V
        cpu.A = 0xA5A5; cpu.X = 0xBEEF; cpu.Y = 0xCAFE;
        cpu.PB = entries[name] >>> 16; cpu.PC = entries[name] & 65535;
        const before = bus.masterCycles;
        let steps = 0, nmis = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps < 1000) {
            if (steps === inject) {
                cpu.setNMIEnabled(true); cpu.triggerNMI(); cpu.setNMIEnabled(false);
            }
            if (inject >= 0 && (cpu.PB << 16 | cpu.PC) === (sym('NmiHandler') & 65535)) nmis++;
            cpu.step(); steps++;
        }
        if (nmis !== (inject >= 0 ? 1 : 0)) throw new Error(`NMI count ${nmis} at boundary ${inject}`);
        if (steps === 1000 || cpu.S !== 0x1EE3 || cpu.DB !== 0x7E || cpu.D !== 0 || cpu.E !== 0 ||
            (cpu.P & 0x3C) !== 4 || u16(0x1EE4) !== 0xD00D || u16(0x1DC0) !== 0xABCD)
            throw new Error(`${name} ABI/stack failure for ${a >>> 0},${b >>> 0}`);
        for (let i = 0; i < preserved.length; i++) if (u16(preserved[i]) !== canaries[i])
            throw new Error(`${name} clobbered r${i >> 1}${i & 1 ? 'h' : ''} for ${a >>> 0},${b >>> 0}`);
        const actual = name === 'tcc__mul' ? cpu.A : (cpu.Y | cpu.X << 16) >>> 0;
        const expected = name === 'tcc__mul' ? Math.imul(a, b) & 65535 : Math.imul(a, b) >>> 0;
        if (actual !== expected) throw new Error(`${name} ${a >>> 0}*${b >>> 0}: ${actual} != ${expected}`);
        instructions += steps; clocks += bus.masterCycles - before; calls++;
        return actual;
    };
    return { call, cost: () => ({ instructions, clocks, calls }) };
}

const EDGES = [0,1,2,7,15,16,30,31,32,33,63,64,127,128,255,256,257,511,512,
    1023,1024,0x1FFF,0x4000,0x7FFF,0x8000,0x8001,0xFF00,0xFF01,0xFFFE,0xFFFF];
const WIDE = [...EDGES, 0x10000,0x10001,0x1FFFF,0x20000,0x7FFF0000,0x7FFFFFFE,0x7FFFFFFF,
    0x80000000,0x80000001,0xFFFF0000,0xFFFF0001,0xFFFFFFFE,0xFFFFFFFF];
function benchmarkCases() {
    const cases: [number, number][] = [];
    for (const a of WIDE) for (const b of WIDE) cases.push([a,b]);
    let seed = 0xB16B00B5;
    for (let i = 0; i < 8192; i++) {
        seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
        const a = i & 1 ? seed & 65535 : seed;
        seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
        const b = i & 1 ? seed & 65535 : seed;
        cases.push([a,b]);
    }
    return cases;
}
describe.skipIf(!process.env.OL_ROM || !process.env.OL_SYMS)
(`OpenLara ${FX3 ? 'FX3' : 'GSU-2'}: exact compiler products`, () => {
    const native = () => machine(process.env.OL_ROM!, process.env.OL_SYMS!);
    it('matches every word in both operand orders against zero, unit and signed extrema', () => {
        const g = native();
        for (let a = 0; a < 65536; a++) for (const b of [0,1,0x8000,0xFFFF]) {
            // Nonzero high words prove that the word helper depends only on
            // the low halves, including its zero/sign and swap shortcuts.
            g.call('tcc__mul', (a | (a ^ 0xA55A) << 16) >>> 0, (b | 0x5AA5 << 16) >>> 0);
            g.call('tcc__mul', (b | 0xA55A << 16) >>> 0, (a | (a ^ 0x5AA5) << 16) >>> 0);
        }
        for (const a of EDGES) for (const b of EDGES) g.call('tcc__mul', a, b);
        expect(g.cost().calls).toBe(524288 + EDGES.length ** 2);
        console.log(`[openlara] word products: ${JSON.stringify(g.cost())}; all 65,536 values against 0/1/8000/FFFF ` +
            'in both orders, dense edge matrix, every r0..r5 half and stack/banks/widths preserved');
    });
    it('matches every unsigned word with full32-bit carries and hardware/software boundaries', () => {
        const g = native();
        for (let a = 0; a < 65536; a++) for (const b of [0,1,32,0xFFFF]) {
            g.call('tcc__mull', a, b); g.call('tcc__mull', b, a);
        }
        for (const a of EDGES) for (const b of EDGES) g.call('tcc__mull', a, b);
        expect(g.cost().calls).toBe(524288 + EDGES.length ** 2);
        console.log(`[openlara] unsigned long products: ${JSON.stringify(g.cost())}; all 65,536 values against 0/1/32/FFFF ` +
            'in both orders plus dense-byte/cross-term carries and 0/1/2/31/32/33 dispatch edges');
    });
    it('matches signed and wide modulo products, equal-high swaps and seeded32-bit factors', () => {
        const g = native(), cases = benchmarkCases();
        for (const [a,b] of cases) {
            g.call('tcc__mull', a,b); g.call('tcc__mul', a,b);
        }
        expect(g.cost().calls).toBe(cases.length * 2);
        console.log(`[openlara] wide products: ${JSON.stringify(g.cost())}; signed 32-bit extrema, equal/high-word ` +
            'swap ties, min-int negations and reproducible dense 32-bit/unsigned 16-bit samples');
    });
    it.skipIf(!FX3)('preserves byte carries and both accumulator bytes at every real NMI boundary', () => {
        const g = native(); let boundaries = 0;
        for (const [a, b] of [[0,0xFFFF], [0xFFFF,0], [1,0xFFFF], [0xFFFF,1], [2,3], [2,0xFFFF],
            [33,41], [33,256], [256,33], [255,65535], [0x1234,0xBEEF], [65535,65535]]) {
            const before = g.cost().instructions;
            g.call('tcc__mull', a, b);
            const steps = g.cost().instructions - before;
            for (let at = 0; at < steps; at++) { g.call('tcc__mull', a, b, at); boundaries++; }
        }
        console.log(`[openlara] FX3 multiply: ${boundaries} real NMI boundaries; byte carries, full A, X:Y, r0..r5 and CPU ABI exact`);
    });
    it.skipIf(!process.env.OL_BASELINE_ROM || !process.env.OL_BASELINE_SYMS)
    ('compares exact native results and helper costs with the unchanged cartridge runtime', () => {
        const machines = [native(), machine(process.env.OL_BASELINE_ROM!, process.env.OL_BASELINE_SYMS!)];
        for (const [a,b] of benchmarkCases()) for (const name of ['tcc__mul','tcc__mull'] as const) {
            const outputs = machines.map(g => g.call(name,a,b));
            if (outputs[0] !== outputs[1]) throw new Error(`${name} native/reference difference for ${a},${b}`);
        }
        console.log(`[openlara] compiler product cost native/baseline: ${JSON.stringify(machines.map(g => g.cost()))}`);
    });
});

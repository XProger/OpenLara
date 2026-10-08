/** Execute the cartridge's joint rotations in the independent TypeSNES core. */
import { expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

it('keeps every GSU rotation angle exact at signed matrix boundaries', () => {
    const rom = readFileSync(process.env.OL_ROM!);
    const symbols = new Map<string, number>();
    for (const line of readFileSync(process.env.OL_GSU_SYMS!, 'utf8').split('\n')) {
        const match = line.trim().match(/^00:([0-9a-f]{4})\s+(\S+)$/i);
        if (match) symbols.set(match[2], parseInt(match[1], 16));
    }
    const sym = (name: string) => {
        const value = symbols.get(name);
        if (value === undefined) throw new Error(`missing GSU symbol ${name}`);
        return value;
    };
    const emulator = new SNESEmulator();
    emulator.loadROM(new Uint8Array(rom));
    const fx = emulator.getSuperFX(), memory = emulator.getMemory();
    const stop = rom.subarray(0x10000, 0x18000).indexOf(0);
    expect(stop).toBeGreaterThanOrEqual(0);
    const s16 = (v: number) => v << 16 >> 16;
    const sine = Array.from({ length: 256 }, (_, i) => rom.readInt16LE(0x10000 + sym('ModelData') + i * 2));
    const edges = [-32768, -32767, -16385, -16384, -1, 0, 1, 16383, 16384, 32766, 32767];
    const seeds = edges.map((_, shift) => Array.from({ length: 12 }, (_, i) => edges[(i + shift) % edges.length]));
    let random = 0x65816;
    for (let n = 0; n < 21; n++) seeds.push(Array.from({ length: 12 }, () => {
        random = (Math.imul(random, 1664525) + 1013904223) >>> 0;
        return s16(random >>> 8);
    }));
    const pairs: [string, number, number][] = [['MatrixRotateX', 2, 1], ['MatrixRotateY', 0, 2], ['MatrixRotateZ', 1, 0]];
    const preserved = [1, 2, 3, 4, 9, 12];
    const reg = (r: number, v: number) => {
        fx.writeRegister(0x7000 + r * 2, v & 255);
        fx.writeRegister(0x7001 + r * 2, v >>> 8 & 255);
    };
    let cases = 0;
    for (const [name, ca, cb] of pairs) for (let angle = 0; angle < 256; angle++) for (const seed of seeds) {
        fx.reset();
        for (let at = 0x70; at < 0xA8; at++) memory.writeCartridgeRAM(at, 0xA5);
        seed.forEach((v, i) => {
            memory.writeCartridgeRAM(0x80 + i * 2, v & 255);
            memory.writeCartridgeRAM(0x81 + i * 2, v >>> 8 & 255);
        });
        preserved.forEach(r => reg(r, 0xA500 + r));
        reg(0, angle); reg(11, stop);
        fx.writeRegister(0x7034, 0x41);
        fx.writeRegister(0x7039, 1);
        fx.writeRegister(0x703A, 0x18);
        reg(15, sym(name));
        fx.stepMaster(20000);
        expect(fx.isRunning(), `${name} did not return`).toBe(false);
        const diagnostic = fx.getDiagnostics();
        expect(diagnostic.lastError).toBeNull();
        expect(preserved.map(r => diagnostic.registers[r])).toEqual(preserved.map(r => 0xA500 + r));
        const expected = [...seed], s = sine[angle], c = sine[(angle + 64) & 255];
        for (let row = 0; row < 3; row++) {
            const a = seed[row * 4 + ca], b = seed[row * 4 + cb];
            expected[row * 4 + ca] = s16(2 * ((a * c >> 16) - (b * s >> 16)));
            expected[row * 4 + cb] = s16(2 * ((a * s >> 16) + (b * c >> 16)));
        }
        const actual = seed.map((_, i) => s16(memory.readCartridgeRAM(0x80 + i * 2) |
            memory.readCartridgeRAM(0x81 + i * 2) << 8));
        expect(actual, `${name} angle ${angle} seed ${seed}`).toEqual(expected);
        for (const start of [0x70, 0x98]) for (let i = 0; i < 16; i++)
            expect(memory.readCartridgeRAM(start + i), 'matrix guard').toBe(0xA5);
        cases++;
    }
    console.log(`[openlara] GSU matrices: ${cases} rotations, every angle, signed edges, preserved registers and matrix guards exact`);
});

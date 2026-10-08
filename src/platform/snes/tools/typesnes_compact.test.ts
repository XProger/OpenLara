/** Resident GSU-2 shared sign extension: real cartridge bytes and C ABI. */
import { expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { dirname, join } from 'path';
import { SNESEmulator } from './emulator';
const ROM = process.env.OL_ROM!, SYMS = process.env.OL_SYMS!;
function symbols() {
    const map = new Map<string, number>();
    for (const line of readFileSync(SYMS, 'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) map.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
    }
    return map;
}
it('preserves every input value, registers, stack and flags through the real resident helper', () => {
    const syms = symbols(), rom = new Uint8Array(readFileSync(ROM)), emu = new SNESEmulator();
    emu.loadROM(rom); const memory = emu.getMemory(), cpu = emu.getCPU();
    const w8 = (at: number, v: number) => memory.write(at, v & 255);
    const w16 = (at: number, v: number) => { w8(at, v); w8(at + 1, v >>> 8); };
    for (const [bank, at, size] of [[63, 0x7E8000, 0x8000], [62, 0x7E5800, 0x2800],
        [61, 0x7E0600, 0x1600], [4, 0x7F0000, 0x8000], [5, 0x7F8000, 0x8000]])
        for (let i = 0; i < size; i++) w8(at + i, rom[bank * 0x8000 + i]);
    const helpers = [...syms].filter(([name]) => name.endsWith('_signext_shared') && !name.startsWith('SECTION'));
    expect(helpers.length).toBeGreaterThan(0);
    const read = (at: number) => Array.from({ length: 23 }, (_, i) => memory.read(at + i));
    const original = readFileSync(join(dirname(ROM), 'c_camera.raw'), 'utf8');
    expect(original).toMatch(/ldy(?:\.w)? #15\n-\ncmp #\$8000\nror a\ndey\nbne -\n\+\n/);
    // The exact compiler idiom, checked against this build's raw source.
    const probe = 0x7E0520;
    [0xA0, 15, 0, 0xC9, 0, 0x80, 0x6A, 0x88, 0xD0, 0xF9, 0x6B].forEach((v, i) => w8(probe + i, v));
    // Exercise the replacement's additional JSL frame too: copy an actual
    // compiled call site rather than jumping directly into its callee.
    const helper = helpers[0][1], fastProbe = 0x7E0540;
    let caller = -1;
    const functions = [...syms].filter(([name]) => name.startsWith('SECTIONSTART_') && name.includes('text_'))
        .map(([name, start]) => [start, syms.get(name.replace('SECTIONSTART_', 'SECTIONEND_'))!]);
    for (const [start, end] of functions) {
        for (let at = start; at < end - 3 && caller < 0; at++) {
            if (memory.read(at) === 0x22 && (memory.read(at + 1) | memory.read(at + 2) << 8 |
                memory.read(at + 3) << 16) === helper) caller = at;
        }
    }
    expect(caller, 'compiled shared-helper JSL was absent').toBeGreaterThan(0);
    for (let i = 0; i < 4; i++) w8(fastProbe + i, memory.read(caller + i));
    w8(fastProbe + 4, 0x6B);
    for (const [name, at] of helpers) {
        expect(at >>> 16, `${name} must execute from resident WRAM`).toBeGreaterThanOrEqual(0x7E);
        expect(at >>> 16).toBeLessThanOrEqual(0x7F);
        expect(read(at), `${name} shares the identical proved body`).toEqual(read(helpers[0][1]));
    }
    let oldSteps = 0, fastSteps = 0;
    for (let value = 0; value < 65536; value++) for (const overflow of [0, 0x40]) {
        const states: number[][] = [];
        for (const [index, at] of [probe, fastProbe].entries()) {
            cpu.A = value; cpu.X = 0xBEEF; cpu.Y = 0xCAFE;
            cpu.E = 0; cpu.P = 4 | overflow | (value & 2 ? 1 : 0); cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
            w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E); cpu.PB = at >>> 16; cpu.PC = at & 65535;
            let steps = 0, minimum = cpu.S;
            while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 100) { cpu.step(); minimum = Math.min(minimum, cpu.S); }
            expect(steps).toBeLessThan(100); index ? fastSteps += steps : oldSteps += steps;
            const state = [cpu.A, cpu.X, cpu.Y, cpu.P, cpu.S, cpu.DB, cpu.D, cpu.E];
            expect(state, `value ${value} variant ${index}`).toEqual([value & 0x8000 ? 65535 : 0,
                0xBEEF, 0, 6 | overflow | (value >>> 14 & 1), 0x1EE3, 0x7E, 0, 0]);
            expect(minimum).toBe(index ? 0x1EDB : 0x1EE0); states.push(state);
        }
        expect(states[0]).toEqual(states[1]);
    }
    console.log(`[openlara] GSU-2 compact sign: ${helpers.length} identical resident helpers; all65536 A/X/Y/P/ABI states with both V flags; ${fastSteps}/${oldSteps} instructions`);
});

/** Native shadow publication vs the complete C_MOTION cartridge and a
 * separate byte-level oracle. OL_ROM/OL_SYMS select native FX3;
 * OL_REFERENCE_ROM/OL_REFERENCE_SYMS select the uncached C reference.
 */
import { describe, expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

const REF_ROM = process.env.OL_REFERENCE_ROM;
const REF_SYMS = process.env.OL_REFERENCE_SYMS;
const s16 = (v: number) => v << 16 >> 16;
const u16 = (v: number) => v & 65535;
const NONE = 32767, SLOT_COUNT = 128, LIST_MAX = 32, BYTES = 102;
type Item = { x: number; y: number; z: number; floor: number; status: number; anim: number; frame: number; health: number };
type Scene = { items: Item[]; list: number[]; boxes: number[][]; floor: number;
    count: number; state: number; freeze: number; fontAt: number; fontBank: number; fontSize: number };

function harness(romPath: string, symbolPath: string) {
    const names = new Map<string, number>();
    for (const line of readFileSync(symbolPath, 'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) names.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
    }
    const sym = (name: string) => {
        const at = names.get(name);
        if (at === undefined) throw new Error(`missing symbol ${name}`);
        return at;
    };
    const emu = new SNESEmulator();
    emu.loadROM(new Uint8Array(readFileSync(romPath)));
    const memory = emu.getMemory(), cpu = emu.getCPU();
    const byte = (at: number, v: number) => memory.write(at, v & 255);
    const word = (at: number, v: number) => { byte(at, v); byte(at + 1, v >>> 8); };
    const set = (name: string, v: number) => word(sym(name), v);
    const output = () => Array.from({ length: BYTES }, (_, i) => memory.read(sym('olShadow') + i));
    const call = () => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        word(0x1EE1, 0x04FF); byte(0x1EE3, 0x7E);
        const at = sym('Shadow_publish'), before = cpu.cycles;
        cpu.PB = at >>> 16; cpu.PC = at & 65535;
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 500000) cpu.step();
        expect(steps, 'Shadow_publish did not return').toBeLessThan(500000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], 'Shadow_publish corrupted the C ABI')
            .toEqual([0x1EE3, 0x7E, 0, 0, 4]);
        return { steps, clocks: cpu.cycles - before };
    };
    const prepare = (scene: Scene, seed: number[]) => {
        set('itemsCount', scene.count); set('olCreatureCount', scene.list.length);
        set('olInvState', scene.state); set('ol_animFreeze', scene.freeze); set('ol_floorEff', scene.floor);
        word(sym('olLevel') + 20, scene.fontAt); word(sym('olLevel') + 22, scene.fontBank);
        word(sym('olLevel') + 24, scene.fontSize);
        const font = (offset: number) => scene.fontBank * 65536 + u16(scene.fontAt + offset);
        byte(font(58), 0x48); word(font(62), 0xFEDC);
        scene.items.forEach((item, i) => {
            const at = sym('items') + i * 32;
            for (let j = 0; j < 32; j++) byte(at + j, 0);
            byte(at, i ? 7 : 0); word(at + 4, item.x); word(at + 6, item.y); word(at + 8, item.z);
            word(at + 12, item.anim); byte(at + 14, item.frame); word(at + 16, item.health);
            byte(at + 24, item.status); word(at + 26, item.floor);
        });
        scene.list.forEach((i, k) => byte(sym('olCreatures') + k, i));
        word(sym('olHostDirectory') + 20, 4);
        word(sym('olAnimBoxData'), 0x2000); word(sym('olAnimBoxData') + 2, 0x7F);
        const table = 0x7F2000;
        for (let i = 0; i < 0x160; i++) byte(table + i, 0);
        scene.boxes.slice(0, 4).forEach((box, i) => box.forEach((v, k) => word(table + i * 12 + k * 2, v)));
        [0, 0x60, 0x68, 0x6C].forEach((v, i) => word(table + 48 + i * 2, v));
        word(table + 0x60, 3);
        [0x100, 0x10C, 0x118].forEach((v, i) => word(table + 0x62 + i * 2, v));
        word(table + 0x68, 1); word(table + 0x6A, 0x124); word(table + 0x6C, 0);
        scene.boxes.slice(4).forEach((box, i) => box.forEach((v, k) => word(table + 0x100 + i * 12 + k * 2, v)));
        seed.forEach((v, i) => byte(sym('olShadow') + i, v));
    };
    return { prepare, call, output };
}

function scene(length = 3, variation = 0): Scene {
    const values = [-32768, -32767, -20000, -129, -1, 0, 1, 128, 20000, 32766, 32767];
    const delta = [-32768, -32767, -512, -1, 0, 1, 512, 32767];
    const x = values[variation % values.length], z = values[(variation * 3 + 5) % values.length];
    const items = Array.from({ length: SLOT_COUNT }, (_, i): Item => ({
        x: s16(x + (i ? delta[(i + variation) % delta.length] : 0)),
        z: s16(z + (i ? delta[(i * 3 + variation) % delta.length] : 0)),
        y: values[(i + variation * 3) % values.length],
        floor: (i + variation) % 7 === 0 ? NONE : values[(i * 5 + variation) % values.length],
        status: [0, 1, 2, 3, 0x20, 0x22, 0xE1, 0xE3][(i + variation) % 8],
        anim: (i + variation) % 4, frame: [0, 1, 2, 3, 255][(i * 3 + variation) % 5],
        health: i % 3 === 0 ? 0 : i % 3 === 1 ? -1 : 1000,
    }));
    return { items, list: Array.from({ length }, (_, i) => 1 + (variation * 17 + i * 19) % 127),
        boxes: Array.from({ length: 8 }, (_, i) => Array.from({ length: 6 }, (_, k) => values[(i * 5 + k * 3 + variation) % values.length])),
        floor: variation % 3 === 0 ? NONE : values[(variation * 7) % values.length],
        count: SLOT_COUNT, state: 0, freeze: 0, fontAt: 0x4000, fontBank: 0x7F, fontSize: 64 };
}

function expected(s: Scene, seed: number[]) {
    const bytes = [...seed];
    const word = (at: number, v: number) => { bytes[at] = v & 255; bytes[at + 1] = v >>> 8 & 255; };
    word(0, 0);
    if (!s.count || s.state || !s.fontSize || s.freeze) return bytes;
    word(2, 0x48); word(4, 0xFEDC);
    // Ranking the whole eligible list independently gives the same stable
    // nearest seven without mirroring the cartridge's insertion algorithm.
    const slots = s.list.map((i, order) => ({ i, order, it: s.items[i] }))
        .filter(({ it }) => (it.status & 3) !== 3 && it.floor !== NONE)
        .map(e => ({ ...e, distance: u16(Math.abs(s16(e.it.x - s.items[0].x)) + Math.abs(s16(e.it.z - s.items[0].z))) }))
        .sort((a, b) => a.distance - b.distance || a.order - b.order).slice(0, 7)
        .map(({ i, it }) => ({ i, floor: it.floor }));
    if (s.floor !== NONE) slots.unshift({ i: 0, floor: s.floor });
    slots.forEach(({ i, floor }, n) => {
        const item = s.items[i];
        let box = s.boxes[item.anim];
        if (item.anim === 1 && item.frame < 3) box = s.boxes[4 + item.frame];
        if (item.anim === 2 && item.frame === 0) box = s.boxes[7];
        [0xF800 + i * 12, floor - item.y, s16(box[1] + box[0]) >> 1,
            s16(box[5] + box[4]) >> 1, s16(s16(box[1] - box[0]) * 5) >> 5,
            s16(s16(box[5] - box[4]) * 5) >> 5].forEach((v, k) => word(6 + n * 12 + k * 2, v));
    });
    word(0, slots.length);
    return bytes;
}

describe.skipIf(process.env.OL_FX3 !== '1' || !REF_ROM || !REF_SYMS)('OpenLara FX3: native shadow publication', () => {
    const pair = () => ({ native: harness(process.env.OL_ROM!, process.env.OL_SYMS!),
        oracle: harness(REF_ROM!, REF_SYMS!) });
    const run = (p: ReturnType<typeof pair>, s: Scene, label: string, seed = Array.from({ length: BYTES }, (_, i) => (i * 7 + 0xA6) & 255)) => {
        p.native.prepare(s, seed); p.oracle.prepare(s, seed);
        const n = p.native.call(), c = p.oracle.call();
        expect(p.native.output(), `${label}: C cartridge`).toEqual(p.oracle.output());
        expect(p.native.output(), `${label}: independent oracle`).toEqual(expected(s, seed));
        return { n, c };
    };

    it('preserves complete output and unused records for every list length across all 128 slots', () => {
        const p = pair(), seen = new Set([0]);
        let native = 0, reference = 0;
        for (let length = 0; length <= LIST_MAX; length++) for (let v = 0; v < 16; v++) {
            const s = scene(length, length * 16 + v);
            s.list.forEach(i => seen.add(i));
            const { n, c } = run(p, s, `list ${length}, variation ${v}`);
            native += n.steps; reference += c.steps;
        }
        expect(seen.size).toBe(SLOT_COUNT);
        expect(native).toBeLessThan(reference / 3);
        console.log(`[openlara] native shadows: 528 complete102-byte native/C/independent snapshots; ` +
            `${native}/${reference} instructions (${(100 * native / reference).toFixed(1)}% of C)`);
    });

    it('preserves stable ties, wrapped distance overflow and every eligibility filter', () => {
        const p = pair(), s = scene(32, 1);
        s.items[0].x = 32767; s.items[0].z = -32768;
        s.list.forEach((slot, i) => {
            const item = s.items[slot];
            item.status = i < 24 ? i % 3 : 3;
            item.floor = i === 8 ? NONE : 1024;
            item.x = s16(s.items[0].x + (i % 4 === 0 ? -32768 : 100));
            item.z = s16(s.items[0].z + (i % 4 === 0 ? -32768 : -100));
        });
        run(p, s, 'equal-distance order and overflow to zero');
        s.list.reverse(); run(p, s, 'reversed source tie order');
        s.items[0].status = 3; s.floor = 1024; run(p, s, 'Lara shadow independent of item status');
        s.list.forEach(slot => s.items[slot].status = 3); run(p, s, 'all creatures invisible');
        s.list.forEach(slot => { s.items[slot].status = 0; s.items[slot].floor = NONE; });
        run(p, s, 'all creature floors missing');
    });

    it('preserves font headers, wrapped font addresses and all early exits', () => {
        const p = pair();
        const active = scene(7, 2);
        run(p, active, 'active publication');
        const seed = p.native.output();
        for (const change of [(s: Scene) => s.count = 0, (s: Scene) => s.state = 9,
            (s: Scene) => s.freeze = 1, (s: Scene) => s.fontSize = 0]) {
            const s = scene(7, 2); change(s); run(p, s, 'early exit over prior output', seed);
        }
        const wrapped = scene(7, 2); wrapped.fontAt = 0xFFF0;
        run(p, wrapped, 'fontAt offset wraps without changing bank');
        wrapped.fontAt = 0x7800; wrapped.fontBank = 0x7E;
        run(p, wrapped, 'independent WRAM font bank');
    });

    it('reduces publication work without dropping any of the selected shadows', () => {
        const p = pair(), samples = [];
        for (const length of [0, 3, 7, 32]) {
            const s = scene(length, 2);
            s.floor = 1024;
            s.list.forEach((slot, i) => {
                s.items[slot].status = 1; s.items[slot].floor = 1024;
                s.items[slot].x = i * 100; s.items[slot].z = 0;
            });
            const { n, c } = run(p, s, `publication with ${length} visible creatures`);
            expect(n.steps).toBeLessThan(c.steps / 3);
            expect(n.clocks).toBeLessThan(c.clocks / 2);
            samples.push({ creatures: length, nativeInstructions: n.steps, cInstructions: c.steps,
                nativeClocks: n.clocks, cClocks: c.clocks });
        }
        console.log(`[openlara] shadow publication work: ${JSON.stringify(samples)}`);
    });
});

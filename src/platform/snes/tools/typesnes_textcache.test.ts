/** Exact inventory text/rectangle parity against the uncached C_MOTION ROM.
 * OL_ROM/OL_SYMS select native FX3; OL_REFERENCE_ROM/OL_REFERENCE_SYMS
 * select its uncached oracle (OL_REF_ROM/OL_REF_SYMS also accepted).
 * Run with tools/run_typesnes.py; neither cartridge needs to boot.
 */
import { describe, expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

function symbols(path: string) {
    const result = new Map<string, number>();
    for (const line of readFileSync(path, 'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) result.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
    }
    return result;
}

const REF_ROM = process.env.OL_REFERENCE_ROM ?? process.env.OL_REF_ROM;
const REF_SYMS = process.env.OL_REFERENCE_SYMS ?? process.env.OL_REF_SYMS;

function harness(romPath: string, symbolPath: string) {
    const names = symbols(symbolPath);
    const sym = (name: string) => {
        const at = names.get(name);
        if (at === undefined) throw new Error(`missing symbol ${name} in ${symbolPath}`);
        return at;
    };
    const emulator = new SNESEmulator();
    emulator.loadROM(new Uint8Array(readFileSync(romPath)));
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    const byte = (at: number, value: number) => memory.write(at, value & 255);
    const word = (at: number, value: number) => { byte(at, value); byte(at + 1, value >>> 8); };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    const set = (name: string, value: number) => word(sym(name), value);
    const raw = (at: number, count: number) => Array.from({ length: count }, (_, i) => memory.read(at + i));
    const call = (name: string, args: number[] = []) => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        word(0x1EE1, 0x04FF); byte(0x1EE3, 0x7E);
        args.forEach((v, i) => word(0x1EE4 + i * 2, v));
        const at = sym(name), before = cpu.cycles;
        cpu.PB = at >>> 16; cpu.PC = at & 65535;
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 500000) cpu.step();
        expect(steps, `${name} did not return`).toBeLessThan(500000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} corrupted the C ABI`)
            .toEqual([0x1EE3, 0x7E, 0, 0, 4]);
        return { steps, cycles: cpu.cycles - before };
    };
    const font = (at: number, level: number, variant: number) => {
        for (let i = 0; i < 800; i++) byte(at + i, 0);
        byte(at + 56, level); byte(at + 57, 110); byte(at + 58, 0x40 + variant);
        word(at + 60, 0x6000 + variant * 256); word(at + 62, 0x7000 + variant * 256);
        for (let i = 0; i < 110; i++) {
            const r = at + 64 + i * 6;
            word(r, 0x1000 + i * 8 + variant * 1024);
            byte(r + 2, 6 + variant); byte(r + 3, 8);
            byte(r + 4, variant); byte(r + 5, -7);
        }
    };
    // Distinct immutable font contents behind different descriptor values.
    font(0x7F4000, 3, 0); font(0x7F4800, 7, 1); font(0x7E7800, 4, 2);
    const selectFont = (at: number, size = 800) => {
        word(sym('olLevel') + 20, at & 65535);
        word(sym('olLevel') + 22, at >>> 16);
        word(sym('olLevel') + 24, size);
    };
    for (let i = 0; i < 64; i++) byte(sym('olLevel') + i, 0);
    selectFont(0x7F4000);
    [2, 3, 4, 7].forEach((v, i) => word(sym('olLevelIds') + i * 2, v));
    set('olLevelIndex', 0); set('olLevelCount', 4);
    set('olInvState', 4); set('olInvPage', 4); set('olInvItemsCount', 3); set('olInvItemIndex', 0);
    [20, 21, 8].forEach((v, i) => byte(sym('olInvItemsList') + i, v));
    for (let i = 0; i < 26; i++) word(sym('olInvCounts') + i * 2, i === 20 ? 123 : 1);
    set('olInvNumKeys', 2); set('olInvFrameIndex', 0); set('olInvFrameTarget', 0);
    set('olInvPassportPage', 0); set('olInvOptionIndex', 0);
    set('olInvOptionsCount', 0); set('olInvOptionsHeight', 0);
    for (let i = 0; i < 8; i++) {
        byte(sym('olInvOptType') + i, 0); byte(sym('olInvOptStr') + i, 0);
        word(sym('olInvOptValue') + i * 2, 0);
    }
    for (let i = 0; i < 8; i++) byte(sym('olSettings') + i, 0);
    word(0x7E0000, 12);
    for (let i = 0; i < 520; i++) byte(sym('olInvText') + i, 0xA6);
    for (let i = 0; i < 146; i++) byte(sym('olInvRects') + i, i === 1 ? 0 : 0xB7);
    for (let i = 0; i < 102; i++) byte(sym('olShadow') + i, 0xC8);
    const output = () => {
        const count = u16(sym('olInvText')), rects = memory.read(sym('olInvRects'));
        expect(count).toBeLessThanOrEqual(64); expect(rects).toBeLessThanOrEqual(24);
        return { text: raw(sym('olInvText'), 8 + count * 8), rects: raw(sym('olInvRects'), 2 + rects * 6) };
    };
    return { sym, byte, word, set, u16, raw, call, output, selectFont };
}
type Harness = ReturnType<typeof harness>;

function pair() {
    const native = harness(process.env.OL_ROM!, process.env.OL_SYMS!);
    const oracle = harness(REF_ROM!, REF_SYMS!);
    const change = (f: (h: Harness) => void) => { f(native); f(oracle); };
    const compose = (label: string) => {
        const n = native.call('Inventory_text'), c = oracle.call('Inventory_text');
        expect(native.output(), label).toEqual(oracle.output());
        return { native: n, oracle: c };
    };
    return { native, oracle, change, compose };
}

describe.skipIf(!REF_ROM || !REF_SYMS)('OpenLara FX3: exact inventory text cache', () => {
    it('preserves every active record and cuts warm text composition instructions', () => {
        const p = pair();
        const shadows = p.native.raw(p.native.sym('olShadow'), 102);
        const cold = p.compose('cold main-page composition');
        const output = p.native.output();
        const warm = p.compose('unchanged main-page composition');
        expect(p.native.output()).toEqual(output);
        expect(warm.native.steps).toBeLessThan(cold.native.steps / 4);
        expect(warm.native.cycles).toBeLessThan(cold.native.cycles / 4);
        expect(p.native.raw(p.native.sym('olShadow'), 102), 'text cache changed shadow records').toEqual(shadows);
        console.log(`[openlara] inventory text cache: ${cold.native.steps} cold -> ${warm.native.steps} warm instructions; ` +
            `${cold.native.cycles} -> ${warm.native.cycles} CPU cycles; C ${warm.oracle.steps} instructions`);
    });

    it('matches state/page transitions, chosen items, counts and key arrows', () => {
        const p = pair();
        for (const state of [1, 4, 6, 7, 9, 8, 4, 10, 11, 12, 2, 3, 5, 0]) {
            p.change(h => h.set('olInvState', state));
            p.compose(`inventory state ${state}`); p.compose(`warm inventory state ${state}`);
        }
        p.change(h => h.set('olInvState', 4));
        for (const page of [0, 1, 2, 3, 4, 5, 6, 4]) {
            p.change(h => h.set('olInvPage', page)); p.compose(`inventory page ${page}`);
        }
        for (let slot = 0; slot < 26; slot++) {
            p.change(h => { h.byte(h.sym('olInvItemsList'), slot); h.word(h.sym('olInvCounts') + slot * 2, slot + 1); });
            p.compose(`selected slot ${slot}`);
        }
        for (const count of [-1, 0, 1, 2, 9, 10, 999, 32767]) {
            p.change(h => h.word(h.sym('olInvCounts') + 25 * 2, count)); p.compose(`item count ${count}`);
        }
        for (const keys of [0, 1, 0, 8]) {
            p.change(h => h.set('olInvNumKeys', keys)); p.compose(`key arrows ${keys}`);
        }
        p.change(h => { h.set('olInvItemIndex', 1); h.byte(h.sym('olInvItemsList') + 1, 1); });
        p.compose('different selected index and key name');
    });

    it('matches settings bars/switches, passport pages and saved-game level names', () => {
        const p = pair();
        p.change(h => {
            h.set('olInvState', 9); h.set('olInvPage', 6); h.byte(h.sym('olInvItemsList'), 22);
            h.set('olInvOptionsCount', 3); h.set('olInvOptionsHeight', 54);
            [2, 3, 3].forEach((v, i) => h.byte(h.sym('olInvOptType') + i, v));
            [6, 7, 8].forEach((v, i) => h.byte(h.sym('olInvOptStr') + i, v));
        });
        for (const value of [0, 1, 16, 128, 255, 256]) {
            p.change(h => {
                h.word(h.sym('olInvOptValue'), value); h.word(h.sym('olInvOptValue') + 2, value & 1);
                h.set('olInvOptionIndex', value % 3);
            });
            p.compose(`graphics settings ${value}`); p.compose(`warm graphics settings ${value}`);
        }
        p.change(h => h.byte(h.sym('olInvItemsList'), 21));
        for (const page of [6, 1, 4]) for (const passport of [0, 1, 2]) {
            p.change(h => {
                h.set('olInvPage', page); h.set('olInvPassportPage', passport);
                h.set('olInvOptionsCount', 4); h.set('olInvOptionsHeight', 72);
                for (let i = 0; i < 4; i++) {
                    h.byte(h.sym('olInvOptType') + i, 1); h.byte(h.sym('olInvOptStr') + i, 16 + i);
                }
            });
            p.compose(`passport page ${passport} on ring page ${page}`);
        }
        p.change(h => {
            h.set('olInvOptionsCount', 5); h.set('olInvOptionsHeight', 80);
            [5, 5, 5, 0, 1].forEach((v, i) => h.byte(h.sym('olInvOptType') + i, v));
            [1, 2, 3, 0, 4].forEach((v, i) => h.byte(h.sym('olInvOptStr') + i, v));
        });
        p.compose('save warning and OK option');
        p.change(h => { h.set('olInvOptionsCount', 1); h.set('olInvOptionsHeight', 18); h.byte(h.sym('olInvOptType'), 1); h.byte(h.sym('olInvOptStr'), 5); });
        p.compose('saved current position');
    });

    it('invalidates on every option-array element, level ID and descriptor change', () => {
        const p = pair();
        p.compose('baseline exact snapshot');
        const warm = p.compose('baseline cache hit').native.steps;
        const changed = (label: string, change: (h: Harness) => void) => {
            p.change(change);
            const cold = p.compose(label).native.steps;
            expect(cold, `${label} reused stale cached records`).toBeGreaterThan(warm);
            expect(p.compose(`warm ${label}`).native.steps).toBe(warm);
        };
        for (let i = 0; i < 8; i++) {
            changed(`option type ${i}`, h => h.byte(h.sym('olInvOptType') + i, 5));
            changed(`option string ${i}`, h => h.byte(h.sym('olInvOptStr') + i, 4));
            changed(`option value ${i}`, h => h.word(h.sym('olInvOptValue') + i * 2, 128 + i));
        }
        for (let i = 0; i < 4; i++) changed(`cartridge level ID ${i}`, h => h.word(h.sym('olLevelIds') + i * 2, 13 + i));
        changed('loaded LevelID', h => h.word(h.sym('olLevel') + 6, 7));
        changed('current level index', h => h.set('olLevelIndex', 1));
        changed('cartridge level count', h => h.set('olLevelCount', 3));
        changed('level descriptor reserved word', h => h.word(h.sym('olLevel') + 46, 1));
        changed('font address and level-specific name', h => h.selectFont(0x7F4800));
        changed('font bank', h => h.selectFont(0x7E7800));
        changed('font size', h => h.selectFont(0x7E7800, 799));
    });

    it('keys the settled-frame condition and FPS rendering without ring-motion churn', () => {
        const p = pair();
        p.change(h => { h.set('olInvFrameIndex', 1); h.set('olInvFrameTarget', 2); });
        p.compose('item animation moving');
        const warm = p.compose('same moving-item text').native.steps;
        p.change(h => { h.set('olInvFrameIndex', 9); h.set('olInvFrameTarget', 10); h.word(0x7E0000, 99); });
        expect(p.compose('animation and disabled FPS move without changing text').native.steps).toBe(warm);
        p.change(h => { h.set('olInvFrameIndex', 10); h.set('olInvFrameTarget', 10); });
        expect(p.compose('item settles and reveals its name').native.steps).toBeGreaterThan(warm);
        p.change(h => h.byte(h.sym('olSettings') + 5, 1));
        for (const fps of [0, 1, 9, 10, 12, 60, 999, 65535]) {
            p.change(h => h.word(0x7E0000, fps));
            p.compose(`FPS ${fps}`); p.compose(`warm FPS ${fps}`);
        }
        p.change(h => h.byte(h.sym('olSettings') + 5, 0)); p.compose('FPS disabled');
    });

    it('restores the ring after game FPS writes and fontless/level transitions', () => {
        const p = pair();
        p.compose('open ring');
        const ring = p.native.output(), warm = p.compose('unchanged ring').native.steps;
        for (const key of [0, 0x800C, 0x803C]) {
            p.change(h => h.call('Inventory_fpsText', [key]));
            expect(p.native.output()).toEqual(p.oracle.output());
            expect(p.compose(`reopen same ring after game text ${key}`).native.steps).toBeGreaterThan(warm);
            expect(p.native.output()).toEqual(ring);
        }
        p.change(h => h.selectFont(0x7F4000, 0));
        p.compose('fontless level'); p.compose('warm fontless level');
        expect(p.native.u16(p.native.sym('olInvText'))).toBe(0);
        p.change(h => h.call('Inventory_fpsText', [0x801E]));
        p.compose('ring after fontless FPS write');
        p.change(h => { h.selectFont(0x7F4800); h.byte(h.sym('olInvItemsList'), 1); });
        p.compose('new level key name and remaps'); p.compose('warm new level');
        // Shadow_publish remains independently executable while text is warm.
        p.change(h => { h.set('itemsCount', 0); h.set('olInvState', 0); }); p.compose('closed ring');
        p.change(h => h.call('Shadow_publish'));
        expect(p.native.u16(p.native.sym('olShadow'))).toBe(0);
        p.compose('warm closed-ring text');
        expect(p.native.u16(p.native.sym('olShadow'))).toBe(0);
    });
});

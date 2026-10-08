/**
 * Execute cartridge platform-floor routines against GBA's original methods.
 * Run with tools/run_typesnes.py and absolute OL_ROM/OL_SYMS paths. OL_SNES
 * overrides the source directory inferred from the ROM's build directory.
 * The host oracle is compiled from verbatim shared methods in a disposable
 * directory; no generated source or cache is written into either checkout.
 */
import { describe, expect, it } from 'vitest';
import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'fs';
import { execFileSync } from 'child_process';
import { tmpdir } from 'os';
import { dirname, join, resolve } from 'path';
import { SNESEmulator } from './emulator';

const ROM = process.env.OL_ROM!;
const SNES = process.env.OL_SNES ?? resolve(dirname(ROM), '..');
const ITEM_SIZE = 32;
const ITEM = { type: 0, room: 1, state: 2, x: 4, y: 6, z: 8, angleY: 10 };
const WALL = -127 * 256;
const symbols = new Map<string, number>();
for (const line of readFileSync(process.env.OL_SYMS!, 'utf8').split('\n')) {
    const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
    if (m) symbols.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
}
function sym(name: string) {
    const at = symbols.get(name);
    if (at === undefined) throw new Error(`missing cartridge symbol ${name}`);
    return at;
}
const ptr = (at: number) => [at & 65535, at >>> 16];
const signed = (v: number) => (v << 16) >> 16;
type Probe = { type: number; state: number; ix: number; iy: number; iz: number;
    angle: number; x: number; y: number; z: number; floor: number };

function gbaOracle(cases: Probe[]) {
    const source = readFileSync(resolve(SNES, '../../fixed/item.h'), 'utf8');
    const directory = mkdtempSync(join(tmpdir(), 'openlara-floor-oracle-'));
    try {
        const methods = ['int32 ItemObj::getBridgeFloor(', 'int32 ItemObj::getTrapDoorFloor(',
            'int32 ItemObj::getDrawBridgeFloor(', 'void ItemObj::getItemFloorCeiling('];
        const extracted = methods.map(signature => {
            const start = source.indexOf(signature);
            if (start < 0) throw new Error(`missing GBA oracle method ${signature}`);
            let end = source.indexOf('{', start), depth = 1;
            while (depth && ++end < source.length) {
                if (source[end] === '{') depth++;
                else if (source[end] === '}') depth--;
            }
            if (depth) throw new Error(`unterminated GBA oracle method ${signature}`);
            return source.slice(start, end + 1);
        }).join('\n\n');
        writeFileSync(join(directory, 'floorupstream.inc'), extracted);
        const executable = join(directory, 'floorcheck');
        execFileSync(process.env.CXX ?? 'c++', ['-std=c++11', '-O2', '-Wall', '-Wextra',
            '-I', directory, join(SNES, 'tools/floorcheck.cpp'), '-o', executable]);
        const input = cases.map(c => [c.type, c.state, c.ix, c.iy, c.iz, c.angle,
            c.x, c.y, c.z, c.floor].join(' ')).join('\n') + '\n';
        const rows = execFileSync(executable, { input, encoding: 'utf8', maxBuffer: 16 * 1024 * 1024 })
            .trim().split('\n').map(line => line.split(' ').map(Number));
        expect(rows.length, 'GBA oracle must answer every probe').toBe(cases.length);
        return rows;
    } finally {
        rmSync(directory, { recursive: true, force: true });
    }
}

function harness() {
    const emulator = new SNESEmulator();
    emulator.loadROM(new Uint8Array(readFileSync(ROM)));
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    const w16 = (at: number, value: number) => {
        memory.write(at, value & 255); memory.write(at + 1, (value >>> 8) & 255);
    };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    const s16 = (at: number) => signed(u16(at));
    const bytes = (at: number, count: number) => Array.from({ length: count }, (_, i) => memory.read(at + i));
    const call = (name: string, words: number[] = []) => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        w16(0x1EE1, 0x04FF); memory.write(0x1EE3, 0x7E);
        words.forEach((word, i) => w16(0x1EE4 + i * 2, word));
        const at = sym(name);
        cpu.PB = at >>> 16; cpu.PC = at & 65535;
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 500000) cpu.step();
        expect(steps, `${name} did not return`).toBeLessThan(500000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} corrupted the C ABI`)
            .toEqual([0x1EE3, 0x7E, 0, 0, 4]);
        return s16(sym('tcc__r0'));
    };
    const itemAt = (at: number, c: Probe) => {
        for (let i = -2; i < ITEM_SIZE + 2; i++) memory.write(at + i, (i * 29 + 91) & 255);
        memory.write(at + ITEM.type, c.type); memory.write(at + ITEM.state, c.state);
        w16(at + ITEM.x, c.ix); w16(at + ITEM.y, c.iy); w16(at + ITEM.z, c.iz);
        w16(at + ITEM.angleY, c.angle);
    };
    const outputAt = (at: number, floor: number) => {
        w16(at - 2, 0xA55A); w16(at, floor); w16(at + 2, 0x5AA5);
    };
    return { emulator, memory, w16, u16, s16, bytes, call, itemAt, outputAt };
}

function platformCases() {
    const cases: Probe[] = [];
    // Both coordinate signs, a negative sector seam and the int16 endpoints.
    const origins = [[-31744, -30721], [-1025, -1], [0, 1023], [2047, -2048], [31744, 30720]];
    for (const type of [41, 65, 66]) for (const state of [0, 1, 2])
        for (const angle of [0, 0x4000, 0x8000, 0xC000, 0x1234])
            for (const [ix, iz] of origins) for (let dx = -3; dx <= 3; dx++)
                for (let dz = -3; dz <= 3; dz++) for (const edge of [0, 1023]) {
                    const x = ((ix >> 10) + dx) * 1024 + edge;
                    const z = ((iz >> 10) + dz) * 1024 + 1023 - edge;
                    if (x < -32768 || x > 32767 || z < -32768 || z > 32767) continue;
                    const index = cases.length;
                    cases.push({ type, state, ix, iy: [-2048, 0, 1024][index % 3], iz, angle,
                        x, y: [-32768, -2049, -1, 0, 1024, 1025][index % 6], z,
                        floor: [-4096, -1, 0, 1024, 4096][index % 5] });
                }
    // Static flat/ramp platforms keep their existing own-sector coverage.
    for (const type of [68, 69, 70]) for (const angle of [0, 0x4000, 0x8000, 0xC000])
        for (const [ix, iz] of origins) for (const dx of [-1, 0, 1]) for (const dz of [-1, 0, 1])
            for (const edge of [0, 1, 511, 1023]) {
                const x = ((ix >> 10) + dx) * 1024 + edge;
                const z = ((iz >> 10) + dz) * 1024 + edge;
                if (x < -32768 || x > 32767 || z < -32768 || z > 32767) continue;
                cases.push({ type, state: 0, ix, iy: 0, iz, angle, x, y: -1024, z, floor: 4096 });
            }
    // The host stores only coordinate low words. Preserve the original GBA
    // world coordinate for this oracle while the CPU ABI packs it to int16:
    // e.g. 33784 crosses +32767 and reaches the cartridge as -31752.
    for (const type of [41, 65, 66]) for (const state of [0, 1, 2])
        for (const angle of [0, 0x4000, 0x8000, 0xC000])
            for (const [ix, iz] of [[32760, 0], [-32760, 0], [0, 32760], [0, -32760]])
                for (let dx = -3; dx <= 3; dx++) for (let dz = -3; dz <= 3; dz++) {
                    cases.push({ type, state, ix, iy: 1024, iz, angle,
                        x: ix + dx * 1024, y: 1024, z: iz + dz * 1024, floor: 4096 });
                }
    return cases;
}

describe('OpenLara FX3: GBA drawbridge and trapdoor floor parity', () => {
    it('bounds every encoded sector of a platform when its original world extent crosses the signed seam', () => {
        const h = harness();
        const coordinates = [-32768, -32760, -31745, -1025, -1, 0, 1023, 30720, 31744, 32767];
        let count = 0;
        for (const ix of coordinates) for (const iz of coordinates) for (const extent of [0, 1, 2]) {
            const item = [0x7E0520, 0x000520, 0x7FF100][count % 3];
            const c: Probe = { type: extent === 2 ? 41 : extent ? 65 : 68,
                state: 0, ix, iy: 1024, iz, angle: 0, x: ix, y: 0, z: iz, floor: 4096 };
            h.itemAt(item, c);
            const before = h.bytes(item - 2, ITEM_SIZE + 4);
            [32767, -32767, 32767, -32767].forEach((v, i) => h.w16(sym('olBridgeBox') + i * 2, v));
            const encodedSectors = (coordinate: number) => Array.from({ length: extent * 2 + 1 }, (_, i) =>
                signed(((coordinate >> 10) + i - extent) * 1024) >> 10);
            const x = encodedSectors(ix), z = encodedSectors(iz);
            const expected = [Math.min(...x), Math.max(...x), Math.min(...z), Math.max(...z)];
            h.call('ItemObj_extendPlatformBounds', [...ptr(item), extent]);
            expect([0, 2, 4, 6].map(offset => h.s16(sym('olBridgeBox') + offset)), `bounds ${count}`)
                .toEqual(expected);
            expect(h.bytes(item - 2, ITEM_SIZE + 4), `bounds ${count} mutated the item`).toEqual(before);
            count++;
        }
        console.log(`[openlara] platform bounds: ${count} encoded-sector extent checks across signed seams and far pointers`);
    });

    it('matches original helpers at all rotations, negative seams, open states and far pointers', () => {
        const h = harness(), cases = platformCases(), expected = gbaOracle(cases);
        let present = 0, raised = 0;
        for (let index = 0; index < cases.length; index++) {
            const c = cases[index], [height, floor] = expected[index];
            const item = [0x7E0520, 0x000520, 0x7FF100][index % 3];
            const output = [0x7FF180, 0x000580, 0x7E0580][index % 3];
            h.itemAt(item, c);
            const before = h.bytes(item - 2, ITEM_SIZE + 4);
            expect(h.call('ItemObj_getBridgeFloor', [...ptr(item), c.x, c.z]), `height probe ${index}`)
                .toBe(height);
            h.outputAt(output, c.floor);
            h.call('ItemObj_getItemFloorCeiling', [...ptr(item), c.x, c.y, c.z, ...ptr(output)]);
            expect(h.s16(output), `floor probe ${index}: ${JSON.stringify(c)}`).toBe(floor);
            expect(h.bytes(item - 2, ITEM_SIZE + 4), `probe ${index} mutated the item`).toEqual(before);
            expect([h.u16(output - 2), h.u16(output + 2)], `probe ${index} overwrote an output canary`)
                .toEqual([0xA55A, 0x5AA5]);
            if (height !== WALL) present++;
            if (floor !== c.floor) raised++;
        }
        expect(present).toBeGreaterThan(200); expect(raised).toBeGreaterThan(100);
        console.log(`[openlara] GBA platform helpers: ${cases.length} probes; ${present} covered points, ${raised} changed floors`);
    });

    it('applies both platform sectors in native Lara collision and preserves static own-sector scope', () => {
        const h = harness();
        const cases = platformCases().map(c => ({ ...c, floor: 4096 }));
        const expected = gbaOracle(cases);
        const item = sym('items') + ITEM_SIZE;
        h.w16(sym('olBridgeCount'), 1); h.memory.write(sym('olBridges'), 1);
        let covered = 0, rejected = 0;
        for (let index = 0; index < cases.length; index++) {
            const c = cases[index];
            // These are the same conservative +/-2 bounds produced at load.
            [c.ix >> 10, c.iz >> 10].flatMap(v => v - 2 < -32 || v + 2 > 31 ? [-32, 31] : [v - 2, v + 2])
                .forEach((v, i) => h.w16(sym('olBridgeBox') + i * 2, v));
            h.itemAt(item, c);
            const before = h.bytes(item - 2, ITEM_SIZE + 4);
            const output = [0x7FF180, 0x000580, 0x7E0580][index % 3];
            h.outputAt(output, c.floor);
            const ownSector = (c.ix >> 10) === (signed(c.x) >> 10) && (c.iz >> 10) === (signed(c.z) >> 10);
            const floor = c.type >= 68 && !ownSector ? c.floor : expected[index][1];
            h.call('Lara_itemFloorAt', [c.x, c.y, c.z, ...ptr(output)]);
            expect(h.s16(output), `Lara floor probe ${index}: ${JSON.stringify(c)}`).toBe(floor);
            expect(h.bytes(item - 2, ITEM_SIZE + 4), `Lara floor probe ${index} mutated the item`).toEqual(before);
            expect([h.u16(output - 2), h.u16(output + 2)]).toEqual([0xA55A, 0x5AA5]);
            if (floor !== c.floor) covered++; else rejected++;
        }
        expect(covered).toBeGreaterThan(200); expect(rejected).toBeGreaterThan(1000);
        // A platform below the room floor must not lower that floor; an empty
        // list or a bbox miss must preserve it without dereferencing items.
        const probe: Probe = { type: 41, state: 1, ix: 0, iy: 2048, iz: 0,
            angle: 0, x: 0, y: 0, z: 1024, floor: 1024 };
        h.itemAt(item, probe); h.outputAt(0x7FF180, probe.floor);
        [-2, 2, -2, 2].forEach((v, i) => h.w16(sym('olBridgeBox') + i * 2, v));
        h.call('Lara_itemFloorAt', [probe.x, probe.y, probe.z, ...ptr(0x7FF180)]);
        expect(h.s16(0x7FF180)).toBe(1024);
        h.w16(sym('olBridgeCount'), 0);
        h.call('Lara_itemFloorAt', [0, -32768, 0, ...ptr(0x7FF180)]);
        expect(h.s16(0x7FF180)).toBe(1024);
        console.log(`[openlara] native platform collision: ${cases.length} probes; ${covered} raised floors, ${rejected} preserved floors`);
    });

});

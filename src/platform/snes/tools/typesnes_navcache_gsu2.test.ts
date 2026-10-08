/** GSU-2 exact point-sector cache: resident machine code, uncached codec
 * oracle, original packed boxes, predictor strides and explicit invalidation.
 * OL_ROM/OL_SYMS select native GSU-2; OL_DATA defaults to its build/data.
 */
import { describe, expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { dirname, join } from 'path';
import { SNESEmulator } from './emulator';

const ROM = process.env.OL_ROM!, SYMS = process.env.OL_SYMS!;
const DATA = process.env.OL_DATA ?? join(dirname(ROM), 'data');
const signed = (v: number) => (v << 16) >> 16;
type Entry = number | { predictor: 1 | 2 };
type Room = { x: number; z: number; width: number; depth: number; boxes: number[] };

function harness() {
    const names = new Map<string, number>();
    for (const line of readFileSync(SYMS, 'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) names.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
    }
    const sym = (name: string) => {
        const at = names.get(name);
        if (at === undefined) throw new Error(`missing ${name} in ${SYMS}`);
        return at;
    };
    const rom = readFileSync(ROM), emulator = new SNESEmulator();
    emulator.loadROM(new Uint8Array(rom));
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    const bus = (emulator as unknown as { bus: { masterCycles: number } }).bus;
    const w8 = (at: number, value: number) => memory.write(at, value & 255);
    const w16 = (at: number, value: number) => { w8(at, value); w8(at + 1, value >>> 8); };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    const raw = (at: number, count: number) => Array.from({ length: count }, (_, i) => memory.read(at + i));
    const set = (name: string, value: number) => w16(sym(name), value);
    for (const [bank, at, size] of [[63, 0x7E8000, 0x8000], [62, 0x7E5800, 0x2800],
        [61, 0x7E0600, 0x1600], [4, 0x7F0000, 0x8000], [5, 0x7F8000, 0x8000]])
        for (let i = 0; i < size; i++) w8(at + i, rom[bank * 0x8000 + i]);
    const call = (name: string, args: number[] = []) => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E);
        args.forEach((v, i) => w16(0x1EE4 + i * 2, v));
        const at = sym(name), before = bus.masterCycles;
        cpu.PB = at >>> 16; cpu.PC = at & 65535;
        let steps = 0, codecCalls = 0, stackLow = cpu.S;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 500000) {
            if ((cpu.PB << 16 | cpu.PC) === sym('navSectorBox')) codecCalls++;
            cpu.step(); stackLow = Math.min(stackLow, cpu.S);
        }
        expect(steps, `${name} did not return`).toBeLessThan(500000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} corrupted the C ABI`)
            .toEqual([0x1EE3, 0x7E, 0, 0, 4]);
        expect(stackLow, `${name} exhausted the stack`).toBeGreaterThanOrEqual(0x1C00);
        return { result: u16(sym('tcc__r0')), steps, clocks: bus.masterCycles - before, codecCalls };
    };
    const level = readFileSync(join(DATA, 'LEVEL.BIN'));
    const host = level.readUInt16LE(2), size = level.readUInt16LE(host), blob = 0x710000;
    for (let i = 0; i < size; i++) w8(blob + i, level[host + i]);
    set('olRoomBase', 28); set('olRoomCount', u16(blob + 18));
    set('olSectorBase', 28 + u16(blob + 18) * 10);
    set('olSectorCount', u16(blob + 22)); set('olBoxCount', u16(blob + 14));
    const cache = sym('olNavSectorCacheGsu2');
    const tags = () => Array.from({ length: 32 }, (_, i) => u16(cache + i * 8));
    const reset = (name = 'Room_reset') => call(name);
    const map = (entries: Entry[], bits = 10, base = 0xE000, invalidate = true) => {
        const ranks = 2 + Math.ceil(entries.length / 4);
        const literals = ranks + (Math.ceil(entries.length / 32) + 1) * 2;
        const ids = entries.filter(e => typeof e === 'number' && e !== 65535) as number[];
        const data = Buffer.alloc(literals + Math.ceil(ids.length * bits / 8));
        data[0] = 0x4E; data[1] = bits;
        let rank = 0;
        entries.forEach((entry, i) => {
            if (!(i & 31)) data.writeUInt16LE(rank, ranks + (i >>> 5) * 2);
            const tag = typeof entry === 'number' ? entry === 65535 ? 0 : 3 : entry.predictor;
            data[2 + (i >>> 2)] |= tag << ((i & 3) * 2);
            if (tag !== 3) return;
            const box = entry as number;
            expect(box).toBeLessThan(1 << bits);
            for (let b = 0; b < bits; b++) {
                const bit = rank * bits + b;
                data[literals + (bit >>> 3)] |= (box >>> b & 1) << (bit & 7);
            }
            rank++;
        });
        data.writeUInt16LE(rank, literals - 2);
        expect(base + data.length).toBeLessThanOrEqual(size - 2);
        data.forEach((v, i) => w8(blob + base + i, v));
        w16(blob + size - 2, base); set('olSectorCount', entries.length); set('olBoxCount', 1024);
        if (invalidate) reset();
        return blob + base;
    };
    reset();
    return { sym, memory, w8, w16, u16, raw, set, call, cache, tags, reset, map, blob };
}

describe('OpenLara GSU-2: exact cached navigation point lookup', () => {
    it('matches every packed sector and cuts repeated decoding without caching raw codec calls', () => {
        const h = harness(), rooms: Room[] = JSON.parse(readFileSync(join(DATA, 'NAV.json'), 'utf8')).rooms;
        let sector = 0, holes = 0, rawClocks = 0, coldClocks = 0, warmClocks = 0;
        for (const room of rooms) for (const box of room.boxes) {
            const expected = h.call('navSectorBox', [sector, room.depth]);
            expect(expected.result, `original sector ${sector}`).toBe(box);
            const cold = h.call('navCachedSectorBoxGsu2', [sector, room.depth]);
            const warm = h.call('navCachedSectorBoxGsu2', [sector, room.depth]);
            expect([cold.result, warm.result], `cached sector ${sector}`).toEqual([box, box]);
            expect(cold.codecCalls).toBe(1); expect(warm.codecCalls).toBe(0);
            const records = h.raw(h.cache, 256);
            expect(h.call('navSectorBox', [sector, room.depth]).codecCalls).toBe(1);
            expect(h.raw(h.cache, 256), 'raw codec mutated cache records').toEqual(records);
            rawClocks += expected.clocks; coldClocks += cold.clocks; warmClocks += warm.clocks;
            if (box === 65535) holes++;
            sector++;
        }
        expect(sector).toBe(h.u16(h.sym('olSectorCount'))); expect(holes).toBeGreaterThan(0);
        expect(warmClocks).toBeLessThan(rawClocks);
        console.log(`[openlara] GSU-2 nav cache: ${sector} original sectors (${holes} NO_BOX), ` +
            `${rawClocks} raw / ${coldClocks} cold / ${warmClocks} warm CPU master clocks`);
    });

    it('preserves room identity, clamped points, invalid rooms and wrapped world seams', () => {
        const h = harness(), rooms: Room[] = JSON.parse(readFileSync(join(DATA, 'NAV.json'), 'utf8')).rooms;
        let first = 0, cases = 0;
        rooms.forEach((room, r) => {
            for (const [x, z] of [[-1025, -1], [-1, 0], [0, 0], [1023, 1023], [1024, 1024],
                [room.width * 1024 - 1, room.depth * 1024 - 1], [room.width * 1024, room.depth * 1024],
                [32767, -32768], [-32768, 32767]]) {
                const sx = Math.min(room.width - 1, Math.max(0, signed(x) >> 10));
                const sz = Math.min(room.depth - 1, Math.max(0, signed(z) >> 10));
                const expected = room.boxes[sx * room.depth + sz];
                const args = [r, signed(room.x + x), signed(room.z + z)];
                expect(h.call('Room_sectorAt', args).result).toBe(first + sx * room.depth + sz);
                expect(h.call('navBoxAt', args).result, `room ${r} point ${x}/${z}`).toBe(expected);
                expect(h.call('navBoxAt', args).codecCalls).toBe(0); cases++;
            }
            first += room.boxes.length;
        });
        for (const room of [rooms.length, 255, 65535])
            expect(h.call('navBoxAt', [room, 0, 0]).result).toBe(65535);
        const room = h.blob + h.u16(h.sym('olRoomBase'));
        h.w8(room + 8, 0x80); h.reset();
        expect(h.call('navBoxAt', [0, 0, 0]).result).toBe(65535);
        h.w8(room + 8, 8); h.w8(room + 9, 0); h.reset();
        expect(h.call('navBoxAt', [0, 0, 0]).result).toBe(65535);
        console.log(`[openlara] GSU-2 nav points: ${cases} room/clamp/seam cases, invalid rooms and empty water grids`);
    });

    it('keys same-slot collisions, NO_BOX, predictor stride and sector-count changes exactly', () => {
        const h = harness();
        const entries: Entry[] = Array.from({ length: 128 }, (_, i) => i === 2 ? 65535 : i + 7);
        h.map(entries);
        for (const i of [0, 32, 64, 96, 0, 32, 2, 2, 34, 2]) {
            const c = h.call('navCachedSectorBoxGsu2', [i, 8]), raw = h.call('navSectorBox', [i, 8]);
            expect(c.result).toBe(raw.result); expect(c.result).toBe(i === 2 ? 65535 : i + 7);
        }
        const predictors: Entry[] = Array(64).fill(65535);
        predictors[0] = 5; predictors[4] = 9; predictors[8] = { predictor: 2 }; predictors[9] = { predictor: 1 };
        h.map(predictors);
        for (const [i, depth, box] of [[8, 8, 5], [8, 4, 9], [8, 8, 5], [9, 4, 9], [9, 8, 5], [9, 8, 5]]) {
            expect(h.call('navSectorBox', [i, depth]).result).toBe(box);
            expect(h.call('navCachedSectorBoxGsu2', [i, depth]).result).toBe(box);
        }
        // Replacing the codec shape changes rank/literal offsets. The count
        // key must miss even without an explicit reset between these maps.
        h.map(Array(128).fill(17)); h.call('navCachedSectorBoxGsu2', [4, 8]);
        h.map(Array(64).fill(23), 10, 0xE000, false);
        const changed = h.call('navCachedSectorBoxGsu2', [4, 8]);
        expect(changed.result).toBe(23); expect(changed.codecCalls).toBe(1);
        for (const [sector, depth] of [[64, 8], [65535, 8], [0, 0]]) {
            const c = h.call('navCachedSectorBoxGsu2', [sector, depth]);
            expect(c.result).toBe(65535); expect(c.codecCalls).toBe(0);
        }
        console.log('[openlara] GSU-2 nav cache keys: same-slot replacement, NO_BOX, Z/X predictors, stride/count and invalid guards');
    });

    it('invalidates tag garbage, same-size rewrites and map rebinding through both reset entry points', () => {
        const h = harness();
        const tagState = () => h.tags().slice();
        for (const reset of ['Room_reset', 'navResetCache']) {
            for (let i = 0; i < 256; i++) h.w8(h.cache + i, (i * 71 + 19) & 255);
            const before = h.raw(h.cache, 256);
            h.reset(reset);
            expect(tagState(), `${reset} did not invalidate all records`).toEqual(Array(32).fill(65535));
            for (let i = 0; i < 256; i++) if (i % 8 >= 2)
                expect(h.memory.read(h.cache + i), `${reset} changed unused record ${i}`).toBe(before[i]);
            h.map(Array(128).fill(3));
            expect(h.call('navCachedSectorBoxGsu2', [7, 8]).result).toBe(3);
            h.map(Array(128).fill(29), 10, 0xE000, false);
            expect(h.call('navSectorBox', [7, 8]).result, 'raw codec reused an old ID').toBe(29);
            h.reset(reset);
            expect(h.call('navCachedSectorBoxGsu2', [7, 8]).result).toBe(29);
            h.map(Array(128).fill(31), 10, 0xE800, false);
            h.reset(reset);
            expect(h.call('navCachedSectorBoxGsu2', [7, 8]).result).toBe(31);
            expect(h.call('navCachedSectorBoxGsu2', [7, 8]).codecCalls).toBe(0);
        }
        console.log('[openlara] GSU-2 nav cache reset: all32 garbage tags, same-size map rewrites and footer rebinding through room/nav resets');
    });
});

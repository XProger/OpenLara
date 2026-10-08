/** GSU-2's shared Enemy_target, against target-compiled C and an independent oracle.
 * Run beside TypeSNES's emulator.ts with OL_FX3=0, OL_ROM/OL_SYMS and
 * optional OL_REFERENCE_ROM/OL_REFERENCE_SYMS (a GSU-2 C_MOTION=1 cartridge).
 * Calls use the five production WRAM code images and synthetic packed SRAM
 * tables. No boot, cartridge code patches, or source test doubles are used.
 */
import { describe, expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

const BOX_MAX = 1024, NO_BOX = 65535, ITEM_SIZE = 32;
const ITEM = { type: 0, room: 1, x: 4, z: 8, health: 16, mood: 23, flags2: 24 };
const TABLE = { room: 0x100, box: 0x400, overlap: 0x2400, zone: 0x4800, nav: 0x6000, size: 0x7000 };
const SRAM = 0x710000, CANARY_A = 0xA55A, CANARY_B = 0x5AA5;
const signed = (n: number) => n << 16 >> 16;
const word = (bytes: number[], at: number) => bytes[at] | bytes[at + 1] << 8;
const setWord = (bytes: number[], at: number, n: number) => { bytes[at] = n & 255; bytes[at + 1] = n >>> 8 & 255; };
const seedForRoll = (roll: number) => Number((BigInt(roll << 10) - 0x3039n) * 0xEEB9EB65n & 0xFFFFFFFFn);
// The LCG multiplier's inverse makes exact threshold rolls, rather than
// relying on a sample of seeds that happens to hit one side of a branch.
const ROLLS = [0, 255, 256, 2047, 2048, 32767];

function symbols(path: string) {
    const result = new Map<string, number>();
    for (const line of readFileSync(path, 'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) result.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
    }
    return result;
}

function cartridge(romPath: string, symbolPath: string) {
    const syms = symbols(symbolPath), rom = new Uint8Array(readFileSync(romPath));
    const sym = (name: string) => {
        const label = syms.has(name) ? name : [...syms.keys()].find(k => k.endsWith('_' + name));
        const address = syms.get(label ?? '');
        if (address === undefined) throw new Error(`missing ${name} in ${symbolPath}`);
        return address;
    };
    const has = (name: string) => syms.has(name) || [...syms.keys()].some(k => k.endsWith('_' + name));
    const emulator = new SNESEmulator(); emulator.loadROM(rom);
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    const bus = (emulator as unknown as { bus: { masterCycles: number } }).bus;
    const w8 = (at: number, v: number) => memory.write(at, v & 255);
    const w16 = (at: number, v: number) => { w8(at, v); w8(at + 1, v >>> 8); };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    const u32 = (at: number) => (u16(at) | u16(at + 2) << 16) >>> 0;
    const bytes = (at: number, size: number) => Array.from({ length: size }, (_, i) => memory.read(at + i));
    const words = (at: number, size: number) => Array.from({ length: size }, (_, i) => u16(at + i * 2));
    for (const [bank, at, size] of [[63, 0x7E8000, 0x8000], [62, 0x7E5800, 0x2800],
        [61, 0x7E0600, 0x1600], [4, 0x7F0000, 0x8000], [5, 0x7F8000, 0x8000]])
        for (let i = 0; i < size; i++) w8(at + i, rom[bank * 0x8000 + i]);
    const call = (name: string, args: number[] = []) => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        cpu.A = 0xA5A5; cpu.X = 0xBEEF; cpu.Y = 0xCAFE;
        w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E);
        args.forEach((v, i) => w16(0x1EE4 + i * 2, v));
        w16(0x1EE4 + args.length * 2, 0xD00D); w16(0x1C00, 0xABCD);
        cpu.PB = sym(name) >>> 16; cpu.PC = sym(name) & 65535;
        const before = bus.masterCycles;
        let steps = 0, rolls = 0, floods = 0;
        const random = sym('rand_logic'), flood = sym('navFlood');
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps < 700000) {
            const pc = cpu.PB * 65536 + cpu.PC;
            if (pc === random) rolls++;
            if (pc === flood) floods++;
            cpu.step(); steps++;
        }
        expect(steps, `${name} did not return`).toBeLessThan(700000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} C ABI`)
            .toEqual([0x1EE3, 0x7E, 0, 0, 4]);
        expect([u16(0x1EE4 + args.length * 2), u16(0x1C00)], `${name} stack canaries`)
            .toEqual([0xD00D, 0xABCD]);
        return { result: u16(sym('tcc__r0')), steps, clocks: bus.masterCycles - before, rolls, floods };
    };
    return { sym, has, memory, w8, w16, u16, u32, bytes, words, call, cacheSignature: '' };
}
type Cartridge = ReturnType<typeof cartridge>;
type Cache = { from: number[]; next: number[][]; reached: number; queue: number[]; wrote: number[][] };
type Scene = {
    name: string; type?: number; mood?: number; hurt?: boolean; health?: number; roll?: number;
    dx?: number; dz?: number; mine?: number; hers?: number; zones?: boolean;
    cache?: 'cold' | 'warm' | 'unreachable' | 'shared'; next?: number;
    sectorX?: number; sectorZ?: number; centreX?: number; centreZ?: number;
    wrapped?: boolean; external?: boolean; outputSwap?: boolean;
};
function initialCache(scene: Scene, kind: number): Cache {
    const from = [NO_BOX, NO_BOX], next = [Array(BOX_MAX).fill(NO_BOX), Array(BOX_MAX).fill(NO_BOX)];
    const wrote = [Array(BOX_MAX / 16).fill(0), Array(BOX_MAX / 16).fill(0)];
    const queue = Array.from({ length: BOX_MAX }, (_, i) => (0x5200 + i) & 65535);
    const mine = scene.mine ?? 0, hers = scene.hers ?? 0;
    if (scene.cache === 'warm' || scene.cache === 'unreachable') {
        from[kind] = hers;
        if (mine < BOX_MAX && scene.cache !== 'unreachable') next[kind][mine] = scene.next ?? hers;
    } else if (scene.cache === 'shared') {
        // A previous flood of this kind and an independent flood of the other
        // kind: only this kind's written bits may be cleared by the next flood.
        from[kind] = 2; from[1 - kind] = 1;
        for (const b of [2, 17, 1023]) { next[kind][b] = 2; wrote[kind][b >>> 4] |= 1 << (b & 15); }
        for (const b of [1, 7, 33]) { next[1 - kind][b] = 1; wrote[1 - kind][b >>> 4] |= 1 << (b & 15); }
    }
    return { from, next, wrote, queue, reached: 17 };
}

// Literal-only packed navigation maps avoid sharing a predictor encoder or
// geometric box lookup with the production implementation being checked.
function packedMap(ids: number[]) {
    const ranks = 2 + Math.ceil(ids.length / 4);
    const literals = ranks + (Math.ceil(ids.length / 32) + 1) * 2;
    const map = Buffer.alloc(literals + Math.ceil(ids.filter(v => v !== NO_BOX).length * 10 / 8) + 4);
    map[0] = 0x4E; map[1] = 10;
    let rank = 0;
    ids.forEach((id, i) => {
        if (!(i & 31)) map.writeUInt16LE(rank, ranks + (i >>> 5) * 2);
        if (id === NO_BOX) return;
        expect(id).toBeLessThan(BOX_MAX);
        map[2 + (i >>> 2)] |= 3 << ((i & 3) * 2);
        for (let bit = 0; bit < 10; bit++) {
            const at = rank * 10 + bit;
            map[literals + (at >>> 3)] |= (id >>> bit & 1) << (at & 7);
        }
        rank++;
    });
    map.writeUInt16LE(rank, literals - 2);
    return map;
}

function prepare(g: Cartridge, scene: Scene) {
    const kind = scene.type === 9 ? 1 : 0, mine = scene.mine ?? 0, hers = scene.hers ?? 0;
    const cache = initialCache(scene, kind);
    const boxCount = scene.cache === 'shared' ? BOX_MAX : Math.max(3,
        ...[mine, hers, scene.next ?? 0].filter(b => b !== NO_BOX).map(b => b + 1));
    const actor = scene.external ? 0x7E0200 : g.sym('items') + ITEM_SIZE;
    const lara = scene.external ? 0x7E0250 : g.sym('items');
    const tx = scene.outputSwap ? 0x710050 : 0x7E0400, tz = scene.outputSwap ? 0x7E0450 : 0x710080;
    const initialItems = Array.from({ length: 3 * ITEM_SIZE }, (_, i) => (i * 53 + 19) & 255);
    initialItems.forEach((v, i) => g.w8(g.sym('items') + i, v));
    const actorBytes = initialItems.slice(ITEM_SIZE, ITEM_SIZE * 2), laraBytes = initialItems.slice(0, ITEM_SIZE);
    const x = scene.wrapped ? 32760 : 1024, z = scene.wrapped ? -32760 : 1024;
    actorBytes[ITEM.type] = scene.type ?? 7; actorBytes[ITEM.room] = 0;
    actorBytes[ITEM.mood] = scene.mood ?? 2;
    actorBytes[ITEM.flags2] = 0xD5 | (scene.hurt ? 8 : 0);
    setWord(actorBytes, ITEM.x, x); setWord(actorBytes, ITEM.z, z);
    laraBytes[ITEM.type] = 0; laraBytes[ITEM.room] = 1;
    setWord(laraBytes, ITEM.x, signed(x + (scene.dx ?? 1024)));
    setWord(laraBytes, ITEM.z, signed(z + (scene.dz ?? 0)));
    setWord(laraBytes, ITEM.health, scene.health ?? 1000);
    actorBytes.forEach((v, i) => g.w8(actor + i, v)); laraBytes.forEach((v, i) => g.w8(lara + i, v));
    for (const at of [actor, lara]) if (scene.external) {
        g.w16(at - 2, CANARY_A); g.w16(at + ITEM_SIZE, CANARY_B);
    }
    for (const [at, value] of [[tx, 0x1357], [tz, 0x2468]]) {
        g.w16(at - 2, CANARY_A); g.w16(at, value); g.w16(at + 2, CANARY_B);
    }
    for (const [name, offset] of [['Room', TABLE.room], ['Box', TABLE.box],
        ['Overlap', TABLE.overlap], ['Zone', TABLE.zone]]) g.w16(g.sym(`ol${name}Base`), offset);
    g.w16(g.sym('olRoomCount'), 2); g.w16(g.sym('olSectorCount'), 128);
    g.w16(g.sym('olBoxCount'), boxCount); g.w16(SRAM, TABLE.size);
    g.w16(SRAM + TABLE.size - 2, TABLE.nav);
    for (let r = 0; r < 2; r++) {
        const at = SRAM + TABLE.room + r * 10;
        for (let i = 0; i < 10; i++) g.w8(at + i, 0);
        g.w8(at, r ? 0 : scene.sectorX ?? 20); g.w8(at + 1, r ? 0 : scene.sectorZ ?? 20);
        g.w16(at + 2, scene.wrapped ? 32760 : 0); g.w16(at + 4, scene.wrapped ? -32760 : 0);
        g.w16(at + 6, r * 64); g.w8(at + 8, 8); g.w8(at + 9, 8);
    }
    const zones = [Array(boxCount).fill(3), Array(boxCount).fill(7)];
    if (scene.zones === false && mine !== NO_BOX && hers !== NO_BOX && mine !== hers)
        zones[kind][mine]++;
    const overlaps: number[][] = Array.from({ length: boxCount }, () => []);
    overlaps[0] = [1]; overlaps[1] = [0, 2]; overlaps[2] = [1];
    let overlapAt = 0;
    for (let b = 0; b < boxCount; b++) {
        const at = SRAM + TABLE.box + b * 8;
        const cx = b === (scene.next ?? hers) ? scene.centreX ?? 22 : 22;
        const cz = b === (scene.next ?? hers) ? scene.centreZ ?? 21 : 21;
        [cz, cz, cx, cx].forEach((v, i) => g.w8(at + i, v)); g.w16(at + 4, 0);
        g.w16(at + 6, overlaps[b].length ? overlapAt : 0x4000);
        overlaps[b].forEach((v, i) => { g.w16(SRAM + TABLE.overlap + overlapAt * 2,
            v | (i === overlaps[b].length - 1 ? 0x8000 : 0)); overlapAt++; });
    }
    zones.flat().forEach((v, i) => g.w16(SRAM + TABLE.zone + i * 2, v));
    const map = packedMap(Array(64).fill(mine).concat(Array(64).fill(hers)));
    map.forEach((v, i) => g.w8(SRAM + TABLE.nav + i, v));
    // Warm-cache decisions cannot write a flood record, and every preceding
    // snapshot checked the entire cache. Reuse that checked state through the
    // mood matrix instead of doing thousands of redundant 6 KiB fixture writes.
    const cacheSignature = scene.cache === 'warm' ? `${kind}/${mine}/${hers}/${scene.next}` : '';
    if (!cacheSignature || cacheSignature !== g.cacheSignature) {
        cache.from.forEach((v, i) => g.w16(g.sym('navFrom') + i * 2, v));
        cache.next.flat().forEach((v, i) => g.w16(g.sym('navNext') + i * 2, v));
        cache.queue.forEach((v, i) => g.w16(g.sym('navQueue') + i * 2, v));
        cache.wrote.flat().forEach((v, i) => g.w16(g.sym('navWrote') + i * 2, v));
        g.w16(g.sym('navReached'), cache.reached);
    }
    g.cacheSignature = cacheSignature;
    g.w16(g.sym('olEnemyInZone'), 0xBEEF);
    const seed = seedForRoll(scene.roll ?? 32767);
    g.w16(g.sym('olRandSeed'), seed); g.w16(g.sym('olRandSeed') + 2, seed >>> 16);
    g.call('Room_reset');
    if (g.has('navInvalidateSectorsGsu2')) g.call('navInvalidateSectorsGsu2');
    return { actor, lara, tx, tz, actorBytes, laraBytes, initialItems, cache, zones, overlaps, seed, kind };
}
type Fixture = ReturnType<typeof prepare>;

function cloneCache(cache: Cache): Cache {
    return { from: cache.from.slice(), next: cache.next.map(a => a.slice()),
        wrote: cache.wrote.map(a => a.slice()), queue: cache.queue.slice(), reached: cache.reached };
}

function floodOracle(cache: Cache, kind: number, start: number, zones: number[],
                     overlaps: number[][], blocked = new Set<number>()) {
    cache.from[kind] = start;
    cache.wrote[kind].forEach((bits, group) => {
        for (let bit = 0; bit < 16; bit++) if (bits & 1 << bit) cache.next[kind][group * 16 + bit] = NO_BOX;
    });
    cache.wrote[kind].fill(0);
    if (start >= zones.length || start >= BOX_MAX) return;
    const queue = [start], zone = zones[start];
    cache.next[kind][start] = start; cache.wrote[kind][start >>> 4] |= 1 << (start & 15);
    for (let head = 0; head < queue.length; head++) {
        if (blocked.has(queue[head])) continue;
        for (const box of overlaps[queue[head]]) {
            if (box < zones.length && box < BOX_MAX && cache.next[kind][box] === NO_BOX && zones[box] === zone) {
                cache.next[kind][box] = queue[head]; cache.wrote[kind][box >>> 4] |= 1 << (box & 15);
                if (queue.length < BOX_MAX) queue.push(box);
            }
        }
    }
    queue.forEach((v, i) => cache.queue[i] = v); cache.reached = queue.length;
}

function oracle(scene: Scene, f: Fixture) {
    const actor = f.actorBytes.slice(), lara = f.laraBytes.slice();
    const cache = cloneCache(f.cache);
    const kind = f.kind, mine = scene.mine ?? 0, hers = scene.hers ?? 0;
    let dx = signed(word(lara, ITEM.x) - word(actor, ITEM.x));
    let dz = signed(word(lara, ITEM.z) - word(actor, ITEM.z));
    let tx = 0x1357, tz = 0x2468, result = 0, inZone = 0, seed = f.seed, rolls = 0, floods = 0;
    const random = () => { rolls++; seed = (Math.imul(seed, 0x41C64E6D) + 0x3039) >>> 0; return seed >>> 10 & 32767; };
    const finish = () => ({ actor, lara, cache, tx, tz, result, inZone, seed, rolls, floods });
    if (dx > 5120 || dx < -5120 || dz > 5120 || dz < -5120 || mine === NO_BOX || hers === NO_BOX) {
        actor[ITEM.mood] = 0; return finish();
    }
    inZone = Number(f.zones[kind][mine] === f.zones[kind][hers]);
    const dist = Math.max(Math.abs(dx), Math.abs(dz));
    let mood = actor[ITEM.mood];
    if (signed(word(lara, ITEM.health)) <= 0) mood = 0;
    else if (mood === 0 || mood === 1) {
        if (scene.hurt && (!inZone || random() < 2048)) mood = 3;
        else if (inZone) mood = dist < 3072 ? 2 : 1;
    } else if (mood === 2) {
        if (scene.hurt && random() < 2048) mood = 3;
        else mood = inZone ? 2 : 0;
    } else mood = inZone && random() < 256 ? 1 : 3;
    actor[ITEM.mood] = mood; actor[ITEM.flags2] &= 247;
    if (mood === 0 || (!kind && mood === 1 && dist < 2048)) return finish();
    if (mood === 3) { dx = signed(-dx); dz = signed(-dz); }
    else if (mine !== hers) {
        if (cache.from[kind] !== hers) {
            floods++; floodOracle(cache, kind, hers, f.zones[kind], f.overlaps);
        }
        const next = cache.next[kind][mine];
        if (next === NO_BOX) return finish();
        const roomX = scene.wrapped ? 32760 : 0, roomZ = scene.wrapped ? -32760 : 0;
        const csx = (scene.sectorX ?? 20) + (signed(signed(word(actor, ITEM.x)) - roomX) >> 10);
        const csz = (scene.sectorZ ?? 20) + (signed(signed(word(actor, ITEM.z)) - roomZ) >> 10);
        const configured = next === (scene.next ?? hers);
        const sx = (configured ? scene.centreX ?? 22 : 22) - csx;
        const sz = (configured ? scene.centreZ ?? 21 : 21) - csz;
        dx = Math.max(-31, Math.min(31, sx)) * 1024; dz = Math.max(-31, Math.min(31, sz)) * 1024;
    }
    tx = dx & 65535; tz = dz & 65535; result = 1;
    return finish();
}

function cacheSnapshot(g: Cartridge): Cache {
    return { from: g.words(g.sym('navFrom'), 2), next: [g.words(g.sym('navNext'), BOX_MAX),
            g.words(g.sym('navNext') + BOX_MAX * 2, BOX_MAX)], reached: g.u16(g.sym('navReached')),
            queue: g.words(g.sym('navQueue'), BOX_MAX), wrote: [g.words(g.sym('navWrote'), BOX_MAX / 16),
                g.words(g.sym('navWrote') + BOX_MAX / 8, BOX_MAX / 16)] };
}

function snapshot(g: Cartridge, f: Fixture, call: ReturnType<Cartridge['call']>) {
    return { actor: g.bytes(f.actor, ITEM_SIZE), lara: g.bytes(f.lara, ITEM_SIZE), cache: cacheSnapshot(g),
        tx: g.u16(f.tx), tz: g.u16(f.tz), result: call.result, inZone: g.u16(g.sym('olEnemyInZone')),
        seed: g.u32(g.sym('olRandSeed')), rolls: call.rolls, floods: call.floods };
}

const HAS_REFERENCE = !!(process.env.OL_REFERENCE_ROM && process.env.OL_REFERENCE_SYMS);
const CHECKED = HAS_REFERENCE ? 'independent/native/C' : 'independent/native';
describe.skipIf(process.env.OL_FX3 !== '0' || !process.env.OL_ROM || !process.env.OL_SYMS)
('OpenLara GSU-2: exact shared Enemy_target', () => {
    const pair = () => [cartridge(process.env.OL_ROM!, process.env.OL_SYMS!),
        ...(HAS_REFERENCE ? [cartridge(process.env.OL_REFERENCE_ROM!, process.env.OL_REFERENCE_SYMS!)] : [])];
    function runCases(scenes: Scene[]) {
        const machines = pair(), costs = machines.map(() => 0), rollCounts = machines.map(() => 0),
            floodCounts = machines.map(() => 0);
        for (const scene of scenes) {
            const outputs = machines.map((g, i) => {
                const f = prepare(g, scene), expected = oracle(scene, f);
                const ptr = (at: number) => [at & 65535, at >>> 16];
                const call = g.call('Enemy_target', [...ptr(f.actor), ...ptr(f.lara), ...ptr(f.tx), ...ptr(f.tz)]);
                costs[i] += call.steps; rollCounts[i] += call.rolls; floodCounts[i] += call.floods;
                const actual = snapshot(g, f, call);
                expect(actual, `${i ? 'C' : 'native'} ${scene.name}`).toEqual(expected);
                for (const at of [f.tx, f.tz]) expect([g.u16(at - 2), g.u16(at + 2)], `${scene.name} output canaries`)
                    .toEqual([CANARY_A, CANARY_B]);
                if (scene.external) {
                    expect(g.bytes(g.sym('items'), ITEM_SIZE * 3), `${scene.name} touched unrelated global items`).toEqual(f.initialItems);
                    for (const at of [f.actor, f.lara]) expect([g.u16(at - 2), g.u16(at + ITEM_SIZE)])
                        .toEqual([CANARY_A, CANARY_B]);
                } else {
                    expect(g.bytes(g.sym('items') + ITEM_SIZE * 2, ITEM_SIZE), `${scene.name} touched an unrelated item`)
                        .toEqual(f.initialItems.slice(ITEM_SIZE * 2));
                }
                return actual;
            });
            if (HAS_REFERENCE) expect(outputs[0], `${scene.name} native/C snapshot`).toEqual(outputs[1]);
        }
        if (HAS_REFERENCE) { expect(rollCounts[0]).toBe(rollCounts[1]); expect(floodCounts[0]).toBe(floodCounts[1]); }
        console.log(`[openlara] GSU-2 Enemy_target: ${scenes.length} ${CHECKED} exact snapshots; ` +
            `${rollCounts[0]} PRNG calls, ${floodCounts[0]} floods; ${costs.join('/')} instructions; ` +
            'all item bytes, far output canaries, seed and complete shared/private flood state');
        return costs;
    }

    it('preserves all three species moods, hurt rolls, live/dead Lara and threshold boundaries', () => {
        const scenes: Scene[] = [];
        for (const type of [7, 8, 9]) for (const mood of [0, 1, 2, 3]) for (const hurt of [false, true])
            for (const health of [1000, 0, -1]) for (const zones of [false, true]) for (const roll of ROLLS)
                for (const dx of [0, 2047, 2048, 3071, 3072, 5119, 5120, 5121, -5120, -5121]) {
                    // Different boxes give different zones; a warm cache isolates
                    // mood decisions from floods without replacing a callee.
                    scenes.push({ name: `mood/${type}/${mood}/${hurt}/${health}/${zones}/${roll}/${dx}`,
                        type, mood, hurt, health, zones, roll, dx, mine: 0, hers: 1, cache: 'warm',
                        external: scenes.length % 7 === 0, outputSwap: scenes.length % 2 === 0 });
                }
        runCases(scenes);
    });

    it('preserves same-box directions, cold and shared floods, holes and the maximum valid box', () => {
        const scenes: Scene[] = [];
        for (const type of [7, 8, 9]) {
            for (const dx of [-5121, -5120, -3072, -3071, -2048, -2047, -1, 0, 1, 2047, 2048, 3071, 3072, 5120, 5121])
                for (const dz of [-5121, -5120, -1, 0, 1, 5120, 5121]) scenes.push({
                    name: `same/${type}/${dx}/${dz}`, type, dx, dz, mine: 0, hers: 0,
                    mood: 2, hurt: true, roll: 2048, external: true, outputSwap: scenes.length % 2 === 0 });
            for (const cache of ['cold', 'warm', 'unreachable', 'shared'] as const)
                for (const zones of [false, true]) for (const mine of [0, 2, 1023, NO_BOX])
                    for (const hers of [1, NO_BOX]) scenes.push({ name: `flood/${type}/${cache}/${zones}/${mine}/${hers}`,
                        type, cache, zones, mine, hers, mood: 2, roll: 2048, dx: 3072,
                        external: scenes.length % 2 === 0, outputSwap: scenes.length % 3 === 0 });
            for (const sx of [0, 31, 32, 200, 255]) for (const cx of [0, 31, 32, 200, 255])
                for (const wrapped of [false, true]) scenes.push({ name: `clamp/${type}/${sx}/${cx}/${wrapped}`,
                    type, mood: 2, mine: 0, hers: 1, cache: 'warm', next: 2,
                    sectorX: sx, sectorZ: 255 - sx, centreX: cx, centreZ: 255 - cx, wrapped,
                    dx: 15, dz: -15, external: true, outputSwap: wrapped });
        }
        runCases(scenes);
    });

    it('preserves direct floods, queue order and repeated clearing through full 1024-box graphs', () => {
        type Graph = { name: string; count: number; edges: number[][]; zones: number[][];
            blocked?: number[]; blockable?: number[] };
        const chain = (n: number) => Array.from({ length: n }, (_, i) => [i - 1, i + 1].filter(b => b >= 0 && b < n));
        const graphs: Graph[] = [
            { name: 'self-duplicate-invalid', count: 1, edges: [[0, 0, 1024, 32767]], zones: [[2], [7]] },
            { name: 'ties-holes-blockable', count: 8,
                edges: [[1, 2, 1, 1024], [0, 3], [0, 3, 3], [1, 2, 4], [3], [], [], [32767]],
                zones: [Array(8).fill(2), Array(8).fill(7)], blocked: [5], blockable: [1, 3] },
            { name: 'mixed-zones', count: 48, edges: chain(48).map((a, i) => [...a, i, (i + 7) % 48]),
                zones: [Array.from({ length: 48 }, (_, i) => i >>> 3), Array.from({ length: 48 }, (_, i) => i % 3)] },
            { name: 'blocked', count: 48, edges: chain(48), zones: [Array(48).fill(2), Array(48).fill(7)],
                blocked: [1, 17, 33], blockable: [0, 16, 32] },
            { name: 'maximum', count: BOX_MAX,
                edges: Array.from({ length: BOX_MAX }, (_, i) => [i, (i + 1) % BOX_MAX, (i + 17) % BOX_MAX]),
                zones: [Array(BOX_MAX).fill(2), Array(BOX_MAX).fill(7)], blockable: [0, 511, 1023] },
            { name: 'maximum-mixed', count: BOX_MAX,
                edges: Array.from({ length: BOX_MAX }, (_, i) => [(i + 1) % BOX_MAX, (i + 2) % BOX_MAX, (i + 17) % BOX_MAX]),
                zones: [Array.from({ length: BOX_MAX }, (_, i) => i >>> 9),
                    Array.from({ length: BOX_MAX }, (_, i) => i % 2)], blocked: [17, 513] },
        ];
        const machines = pair(), totals = machines.map(() => 0), maxSteps = machines.map(() => 0),
            maxClocks = machines.map(() => 0);
        let cases = 0;
        for (const graph of graphs) {
            const fixtures = machines.map(g => prepare(g, { name: graph.name, cache: 'shared', mine: 0, hers: 1 }));
            const expected = fixtures.map(f => cloneCache(f.cache));
            machines.forEach(g => {
                g.w16(g.sym('olBoxCount'), graph.count);
                graph.zones.flat().forEach((v, i) => g.w16(SRAM + TABLE.zone + i * 2, v));
                let offset = 0;
                for (let box = 0; box < graph.count; box++) {
                    const blocked = graph.blocked?.includes(box), blockable = graph.blockable?.includes(box);
                    g.w16(SRAM + TABLE.box + box * 8 + 6, offset | (blocked ? 0x4000 : 0) | (blockable ? 0x8000 : 0));
                    const list = graph.edges[box].length ? graph.edges[box] : [32767];
                    list.forEach((neighbor, i) => {
                        g.w16(SRAM + TABLE.overlap + offset * 2, neighbor | (i === list.length - 1 ? 0x8000 : 0));
                        offset++;
                    });
                }
                expect(SRAM + TABLE.overlap + offset * 2).toBeLessThan(SRAM + TABLE.zone);
            });
            // Both kinds share the queue and keep independent bitmaps. Every
            // start reuses the preceding cache, including invalid starts that
            // clear written entries without changing queue or reached.
            for (const [kind, start] of [[0, 0], [1, graph.count - 1], [0, graph.count - 1],
                [1, 0], [0, graph.count], [1, NO_BOX], [0, 0], [1, 0]]) {
                const outputs = machines.map((g, index) => {
                    floodOracle(expected[index], kind, start, graph.zones[kind], graph.edges,
                        new Set(graph.blocked ?? []));
                    const call = g.call('navFlood', [kind, start]);
                    totals[index] += call.steps; maxSteps[index] = Math.max(maxSteps[index], call.steps);
                    maxClocks[index] = Math.max(maxClocks[index], call.clocks);
                    expect(call.rolls, `${graph.name} flood used PRNG`).toBe(0);
                    expect(call.floods).toBe(1);
                    const actual = cacheSnapshot(g);
                    expect(actual, `${index ? 'C' : 'native'} ${graph.name}/${kind}/${start}`).toEqual(expected[index]);
                    expect(g.bytes(fixtures[index].actor, ITEM_SIZE)).toEqual(fixtures[index].actorBytes);
                    expect(g.bytes(fixtures[index].lara, ITEM_SIZE)).toEqual(fixtures[index].laraBytes);
                    expect(g.u32(g.sym('olRandSeed'))).toBe(fixtures[index].seed);
                    expect(g.u16(g.sym('olEnemyInZone'))).toBe(0xBEEF);
                    return actual;
                });
                if (HAS_REFERENCE) expect(outputs[0], `${graph.name}/${kind}/${start} native/C flood`).toEqual(outputs[1]);
                cases++;
            }
        }
        console.log(`[openlara] GSU-2 navFlood: ${cases} ${CHECKED} exact full-cache snapshots; ` +
            'mixed zones, blocked/blockable, holes, self/ties/duplicates/invalid IDs and starts, repeated clearing, 1024-box BFS; ' +
            `${totals.join('/')} instructions; maxima ${maxSteps.join('/')} instructions, ` +
            `${maxClocks.join('/')} master clocks`);
    });
});

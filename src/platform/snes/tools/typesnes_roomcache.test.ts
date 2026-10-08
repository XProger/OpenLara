/** Exact resolved-query cache checks. Execute CPU call boundaries without
 * booting either cartridge. OL_REFERENCE_ROM/OL_REFERENCE_SYMS optionally
 * select the uncached reference; every oracle query first calls Room_reset.
 * Both cartridges use their immutable host tables, except explicit mutable
 * sector and synthetic-grid fixtures. No renderer or gameplay is bypassed
 * in the normal cartridge build by this test.
 */
import { describe, expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { dirname, join } from 'path';
import { SNESEmulator } from './emulator';

const ROM = process.env.OL_ROM!, SYMS = process.env.OL_SYMS!;
const REF_ROM = process.env.OL_REFERENCE_ROM ?? process.env.OL_REF_ROM ?? ROM;
const REF_SYMS = process.env.OL_REFERENCE_SYMS ?? process.env.OL_REF_SYMS ?? SYMS;
const DATA = process.env.OL_DATA ?? join(dirname(ROM), 'data');
const FX3 = process.env.OL_FX3 === '1';
const OUTPUT = FX3 ? 0x7F9000 : 0x71FF40;
const ptr = (at: number) => [at & 65535, at >>> 16];
const signed = (v: number) => (v << 16) >> 16;
type Query = [number, number, number, number];
const A: Query = [0, 7680, -8192, -32256];
const B: Query = [0, 8704, -8192, -32256];
const C: Query = [0, 7680, -8192, -31232];

function harness(romPath: string, symbolPath: string) {
    const names = new Map<string, number>();
    for (const line of readFileSync(symbolPath, 'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) names.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
    }
    const sym = (name: string) => {
        const at = names.get(name);
        if (at === undefined) throw new Error(`missing ${name} in ${symbolPath}`);
        return at;
    };
    const rom = readFileSync(romPath), emulator = new SNESEmulator();
    emulator.loadROM(new Uint8Array(rom));
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    const w8 = (at: number, value: number) => memory.write(at, value & 255);
    const w16 = (at: number, value: number) => { w8(at, value); w8(at + 1, value >>> 8); };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    const raw = (at: number, count: number) => Array.from({ length: count }, (_, i) => memory.read(at + i));
    const set = (name: string, value: number) => w16(sym(name), value);
    const address = (name: string, at: number) => {
        if (FX3) { set(name, at); w16(sym(name) + 2, at >>> 16); }
        else {
            expect(at >>> 16, 'GSU-2 host tables reside in cartridge RAM').toBe(0x71);
            set(name.replace(/Data$/, 'Base'), at);
        }
    };
    if (!FX3) for (const [bank, at, size] of [[63,0x7E8000,0x8000], [62,0x7E5800,0x2800],
        [61,0x7E0600,0x1600], [4,0x7F0000,0x8000], [5,0x7F8000,0x8000]])
        for (let i = 0; i < size; i++) w8(at + i, rom[bank * 0x8000 + i]);
    const call = (name: string, args: number[] = []) => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E);
        args.forEach((v, i) => w16(0x1EE4 + i * 2, v));
        const at = sym(name), before = cpu.cycles;
        const trace = new Map<string, number>();
        const watches = new Map(['RQ_CacheLookup', 'RQC_Miss', 'RQ_CacheStore', 'RQ_W1', 'RQ_Sector']
            .filter(n => names.has(n)).map(n => [sym(n), n]));
        cpu.PB = at >>> 16; cpu.PC = at & 65535;
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 500000) {
            const watch = watches.get(cpu.PB << 16 | cpu.PC);
            if (watch) trace.set(watch, (trace.get(watch) ?? 0) + 1);
            cpu.step();
        }
        expect(steps, `${name} did not return`).toBeLessThan(500000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} corrupted the C ABI`)
            .toEqual([0x1EE3, 0x7E, 0, 0, 4]);
        return { result: u16(sym('tcc__r0')), steps, cycles: cpu.cycles - before, trace };
    };
    // The first LEVEL1 host directory is independently mapped CPU ROM.
    // Only SECTOR is mutable; emulate olHostBind's pointers and exact copy.
    if (FX3) {
        const start = 0x200000;
        expect(rom.subarray(start, start + 4).toString()).toBe('OLH2');
        const table = (index: number) => ({
            offset: rom.readUInt32LE(start + 40 + index * 12),
            size: rom.readUInt32LE(start + 44 + index * 12),
        });
        for (const [name, index] of [['Room', 0], ['Sector', 1], ['Link', 2]] as const) {
            const t = table(index), at = name === 'Sector' ? 0x7F0000 : 0xE00000 + t.offset;
            address(`ol${name}Data`, at);
            if (name === 'Sector') for (let i = 0; i < t.size; i++) w8(at + i, rom[start + t.offset + i]);
        }
        set('olRoomCount', rom.readUInt16LE(start + 16 + 8 * 2));
        set('olSectorCount', rom.readUInt16LE(start + 16 + 9 * 2));
    } else {
        const level = readFileSync(join(romPath === ROM ? DATA : join(dirname(romPath), 'data'), 'LEVEL.BIN'));
        const host = level.readUInt16LE(2), size = level.readUInt16LE(host), blob = 0x710000;
        expect(size, 'fixture output overlaps staged tables').toBeLessThan(0xFF3E);
        for (let i = 0; i < size; i++) w8(blob + i, level[host + i]);
        const rooms = u16(blob + 18), sectors = u16(blob + 22);
        set('olRoomBase', 28); set('olRoomCount', rooms); set('olSectorCount', sectors);
        set('olSectorBase', 28 + rooms * 10); set('olLinkBase', 28 + rooms * 10 + sectors * 4);
    }
    const select = (mask: number) => {
        for (let i = 0; i < 3; i++) w16(sym('olNav') + i * 8240 + 8214, mask & (1 << i) ? i + 1 : 65535);
        call('RQ_CacheSelect');
    };
    const reset = (mask = 3) => {
        call('Room_reset');
        if (names.has('RQ_CacheSelect')) select(mask);
    };
    const query = (q: Query, out = OUTPUT) => {
        for (let i = -2; i < 22; i++) w8(out + i, 0xA5);
        const measured = call('Room_query', [...q, ...ptr(out)]);
        expect([u16(out - 2), u16(out + 20)], `query ${q} overwrote output canaries`).toEqual([0xA5A5, 0xA5A5]);
        return { ...measured, bytes: raw(out, 20), values: Array.from({ length: 10 }, (_, i) => signed(u16(out + i * 2))) };
    };
    const cacheBytes = names.has('olRoomQueryCache') ? sym('olRoomQueryCacheSlot') - sym('olRoomQueryCache') : 0;
    const tags = () => Array.from({ length: cacheBytes / 32 }, (_, i) => u16(sym('olRoomQueryCache') + i * 32));
    reset();
    return { sym, memory, gsu: emulator.getSuperFX(), w8, w16, u16, raw, set, address, call, select, reset, query, tags, cacheBytes };
}
type Harness = ReturnType<typeof harness>;
function pair() {
    const native = harness(ROM, SYMS), oracle = harness(REF_ROM, REF_SYMS);
    const compare = (q: Query, out = OUTPUT) => {
        oracle.reset();
        const expected = oracle.query(q, out), actual = native.query(q, out);
        expect(actual.result, `query result ${q}`).toBe(expected.result);
        expect(actual.bytes, `resolved metadata ${q}`).toEqual(expected.bytes);
        return { actual, expected };
    };
    return { native, oracle, compare };
}

function synthetic(h: Harness, origins: [number, number][] = [[31744, -2048], [31744, -2048], [31744, -2048]]) {
    const rooms = FX3 ? 0x7F9200 : 0x719200;
    const sectors = FX3 ? 0x7FA000 : 0x71A000, links = FX3 ? 0x7FB000 : 0x71B000;
    h.address('olRoomData', rooms); h.address('olSectorData', sectors); h.address('olLinkData', links);
    h.set('olRoomCount', 3); h.set('olSectorCount', 48);
    for (let r = 0; r < 3; r++) {
        const at = rooms + r * 10;
        h.w16(at, 0); h.w16(at + 2, origins[r][0]); h.w16(at + 4, origins[r][1]);
        h.w16(at + 6, r * 16); h.w8(at + 8, 4); h.w8(at + 9, 4);
        for (let s = 0; s < 16; s++) {
            const p = sectors + (r * 16 + s) * 4, link = r * 16 + s + 1, l = links + link * 8;
            h.w8(p, r === 0 ? 16 : r === 1 ? 8 + (s >>> 2) : 32 + s);
            h.w8(p + 1, r === 2 ? 8 : -16); h.w16(p + 2, link);
            h.w8(l, r === 1 ? 2 : 255); h.w8(l + 1, r === 2 ? 1 : 255);
            h.w8(l + 2, r === 0 ? 1 : 255); h.w8(l + 3, 10 + s);
            if (FX3) {
                h.w8(l + 4, (s % 5) - 2); h.w8(l + 5, (s % 3) - 1);
                h.w8(l + 6, 1 - (s % 3)); h.w8(l + 7, 2 - (s % 5));
            } else {
                h.w16(l + 4, (((s % 5) - 2) & 31) | ((((s % 3) - 1) & 31) << 5));
                h.w16(l + 6, ((1 - (s % 3)) & 31) | (((2 - (s % 5)) & 31) << 5));
            }
        }
    }
    for (let i = 0; i < 8; i++) h.w8(links + i, i < 4 ? 255 : 0);
    h.reset();
    return { rooms, sectors, links };
}

describe('OpenLara SNES: exact interleaved room-query cache', () => {
    it.skipIf(FX3)('grants RAM only for complete WRAM outputs, including the bank-zero mirror edge',()=>{
        const g=harness(ROM,SYMS), expected=g.query(A).bytes;
        const entry=g.sym('olGsuYieldRam');
        const u24=(at:number)=>g.u16(at)|g.memory.read(at+2)<<16;
        const owner=u24(entry+3), shadow=u24(entry+9);
        const write=g.gsu.writeCPU, writes:number[]=[];
        g.gsu.writeCPU=function(address,value) {
            if ((address&65535)===0x303A) writes.push(value);
            return write.call(this,address,value);
        };
        const outputs:[number,boolean][]=[[0x520,true],[0x1FD0,true],[0x1FEC,true],
            [0x1FED,false],[0x2000,false],[0x71FF40,false],[0x7E0520,true],[0x7FFFE0,true]];
        try {
            for (const [out,eligible] of outputs) for (const initial of [0,1]) for (const mode of [0x1D,0x05]) {
                g.w8(owner,initial); g.w8(shadow,mode); g.w8(0x303A,initial?mode&~8:mode);
                writes.length=0;
                const result=g.call('Room_query',[...A,...ptr(out)]);
                expect(result.result).toBe(1);
                expect(writes,`output ${out.toString(16)}, owner ${initial}, mode ${mode}`)
                    .toEqual(eligible&&initial&&(mode&8)?[mode,mode&~8]:[]);
                expect(g.memory.read(owner)).toBe(initial);
                expect(g.gsu.getDiagnostics().scmr).toBe(initial?mode&~8:mode);
                if (eligible || out===0x71FF40) expect(g.raw(out,20)).toEqual(expected);
            }
        } finally { g.gsu.writeCPU=write; }
        console.log('[openlara] room output overlap: 32 bank/mirror-edge/owner/mask cases, exact eligible records and serial cartridge/I-O outputs');
    });
    it.skipIf(FX3)('initializes every resident WRAM record and keeps general BSS out of the cache', () => {
        const p = pair(), n = p.native, cache = n.sym('olRoomQueryCache');
        expect(cache >>> 16).toBe(0x7E);
        expect(cache & 65535).toBeGreaterThanOrEqual(0x8000);
        expect(n.cacheBytes).toBe(32 * 32);
        for (let i = 0; i < n.cacheBytes; i++) n.w8(cache + i, i * 13 + 7);
        n.call('Room_reset');
        expect(n.tags()).toEqual(Array(n.cacheBytes / 32).fill(65535));
        expect(n.u16(n.sym('olRoomQueryCacheEnabled'))).toBe(1);
        p.compare(A); p.compare(B);
        const hit = p.compare(A).actual;
        expect(hit.trace.get('RQ_CacheLookup')).toBe(1);
        expect(hit.trace.get('RQC_Miss') ?? 0).toBe(0);
        n.call('Room_reset');
        expect(n.tags()).toEqual(Array(n.cacheBytes / 32).fill(65535));
        console.log(`[openlara] GSU-2 room cache: ${n.cacheBytes} bytes in resident WRAM, every tag reset, no general BSS allocation`);
    });

    it.skipIf(!FX3)('enables only with two or three admitted navigation owners and resets every hint tag', () => {
        const p = pair();
        const cache = p.native.sym('olRoomQueryCache'), nav = p.native.sym('olNav');
        expect(cache >>> 16).toBe(0x7E);
        expect(cache & 65535).toBeGreaterThanOrEqual(0x5800);
        expect(p.native.cacheBytes).toBe(32 * 32);
        expect(cache + p.native.cacheBytes).toBeLessThanOrEqual(0x7F0000);
        expect(cache >= nav + 3 * 8240 || cache + p.native.cacheBytes <= nav,
            'room records overlap live navigation contexts').toBe(true);
        for (let mask = 0; mask < 8; mask++) {
            p.native.reset(mask);
            expect(p.native.u16(p.native.sym('olRoomQueryCacheEnabled')), `owner occupancy ${mask}`)
                .toBe([0, 1, 2].filter(i => mask & (1 << i)).length >= 2 ? 1 : 0);
            let lookups = 0, stores = 0;
            for (const q of [A, B, A]) {
                const { actual } = p.compare(q);
                lookups += actual.trace.get('RQ_CacheLookup') ?? 0;
                stores += actual.trace.get('RQ_CacheStore') ?? 0;
            }
            if ([0, 1, 2].filter(i => mask & (1 << i)).length < 2) {
                expect(lookups).toBe(0); expect(stores).toBe(0);
                expect(p.native.tags()).toEqual(Array(p.native.cacheBytes / 32).fill(65535));
            } else { expect(lookups).toBe(3); expect(stores).toBe(2); }
            p.native.call('Room_reset');
            expect(p.native.tags()).toEqual(Array(p.native.cacheBytes / 32).fill(65535));
            expect(p.native.u16(p.native.sym('olRoomQueryCacheEnabled'))).toBe(0);
        }
        console.log('[openlara] room cache policy: all eight owner occupancies, disabled lookups/stores and reset tags');
    });

    it('distinguishes actual clamped portal sectors and restores complete metadata on secondary hits', () => {
        const p = pair();
        const first = p.compare(A).actual.values;
        p.compare(B);
        const different = p.compare(C).actual.values;
        expect(different).not.toEqual(first);
        const warm = p.compare(A).actual;
        expect(warm.values).toEqual(first);
        expect(warm.trace.get('RQ_CacheLookup')).toBe(1);
        expect(warm.trace.get('RQC_Miss') ?? 0).toBe(0);
        expect(warm.trace.get('RQ_W1') ?? 0).toBe(0);
        expect(warm.trace.get('RQ_CacheStore') ?? 0).toBe(0);
        expect(warm.trace.get('RQ_Sector') ?? 0, 'cache hit still read host sector tables').toBe(0);
        expect(first.slice(0, 5)).toEqual([6, 2560, -640, 7168, -32768]);
        expect(different.slice(0, 5)).toEqual([6, 2304, -512, 7168, -31744]);
        const d: Query = [0, 7680, -4096, 23040], e: Query = [0, 8704, -4096, 23040];
        const f: Query = [0, 7680, -4096, 24064];
        p.native.reset();
        const d0 = p.compare(d).actual.values;
        p.compare(e); expect(p.compare(f).actual.values).not.toEqual(d0);
        expect(p.compare(d).actual.values).toEqual(d0);
        // Fractional position is live, even when only the resolution is cached.
        for (const delta of [0, 1, 255, 511]) {
            p.compare(B);
            const q: Query = [A[0], A[1] - 512 + delta, A[2] + 31, A[3] - 512 + 1023 - delta];
            p.compare(q, [OUTPUT, 0x000520, 0x7E0520][delta % 3]);
        }
        console.log(`[openlara] room cache portal regressions: exact floors/ceilings/sectors/slants, ${warm.steps} instructions on a secondary hit`);
    });

    it('checks the complete key after collisions in the direct-mapped table', () => {
        const p = pair();
        const collider: Query = [A[0], signed(A[1] + (p.native.cacheBytes / 32) * 1024), A[2], A[3]];
        const first = p.compare(A).actual.bytes;
        p.compare(collider);
        const again = p.compare(A).actual;
        expect(again.trace.get('RQC_Miss')).toBe(1);
        expect(again.bytes).toEqual(first);
        expect(p.compare(A).actual.trace.get('RQ_Sector') ?? 0).toBe(0);
    });

    it('matches reset-per-query answers across original rooms, height bands, fractions and interleaving', () => {
        const p = pair();
        const rooms: { x: number; z: number; width: number; depth: number; sectors: number[][] }[] =
            JSON.parse(readFileSync(join(DATA, 'NAV.json'), 'utf8')).rooms;
        const points: Query[] = [];
        rooms.forEach((r, hint) => {
            for (const sector of [0, Math.floor(r.sectors.length / 2), r.sectors.length - 1]) {
                const sx = Math.floor(sector / r.depth), sz = sector % r.depth;
                for (const fraction of [0, 1, 511, 1023]) for (const height of [-1, 0, 1])
                    points.push([hint, signed(r.x + sx * 1024 + fraction),
                        signed(r.sectors[sector][0] * 256 + height * 256 + 37),
                        signed(r.z + sz * 1024 + 1023 - fraction)]);
            }
        });
        let hits = 0;
        for (let i = 0; i < points.length; i++) {
            const q = points[(i * 73) % points.length];
            const { actual } = p.compare(q, [OUTPUT, 0x000520, 0x7E0520][i % 3]);
            if ((actual.trace.get('RQ_CacheLookup') ?? 0) && !(actual.trace.get('RQC_Miss') ?? 0)) hits++;
            if (!(i % 7)) {
                p.compare(B);
                const repeated = p.compare(q).actual;
                if ((repeated.trace.get('RQ_CacheLookup') ?? 0) && !(repeated.trace.get('RQC_Miss') ?? 0)) hits++;
            }
        }
        expect(hits, 'interleaved geometry did not exercise secondary reuse').toBeGreaterThan(10);
        console.log(`[openlara] room cache geometry: ${points.length} original-grid probes with interleaving; ${hits} secondary hits`);
    });

    it('preserves portal and vertical resolution at signed world seams and every sector boundary', () => {
        const p = pair(); synthetic(p.native); synthetic(p.oracle);
        let cases = 0;
        for (const x of [31743, 31744, 32767, -32768, -32767, -31745, -31744, -30721, -30720])
            for (const z of [-2049, -2048, -1025, -1024, -1, 0, 1, 1023, 1024, 2048])
                for (const y of [-4097, -4096, -1, 0, 2047, 2048, 2049, 4095]) {
                    const q: Query = [cases % 3, x, y, z];
                    p.compare(q); p.compare([1, 31744, 0, -2048]); p.compare(q); cases++;
                }
        console.log(`[openlara] room cache seams: ${cases} exact portal/vertical/boundary probes, each repeated after interleaving`);
    });

    it('bypasses both reusable resolutions when any visited grid origin is unaligned', () => {
        const p = pair();
        for (const unaligned of [0, 1, 2]) {
            const origins: [number, number][] = [[0, 0], [0, 0], [0, 0]];
            origins[unaligned] = [100, 123];
            synthetic(p.native, origins); synthetic(p.oracle, origins);
            for (const mask of [0, 1, 3, 7]) {
                p.native.reset(mask);
                if (!FX3) p.native.set('olRoomQueryCacheEnabled', mask >= 3 ? 1 : 0);
                const left = p.compare([0, 1099, 0, 512]).actual.values;
                const right = p.compare([0, 1100, 0, 512]).actual.values;
                if (unaligned === 1) expect(left).not.toEqual(right);
                for (const q of [[0, 1099, 0, 512], [0, 1100, 0, 512], [0, 1099, 0, 512]] as Query[]) {
                    p.compare(q);
                    expect(p.native.u16(p.native.sym('olRoomQueryCacheUnaligned'))).toBe(1);
                    expect(p.native.u16(p.native.sym('rq_qcHint'))).toBe(65535);
                    expect(p.native.tags()).toEqual(Array(p.native.cacheBytes / 32).fill(65535));
                }
            }
        }
        console.log('[openlara] room cache unaligned grids: hinted, portal and floor-linked rooms bypass both caches under every policy');
    });

    it('invalidates mutable floors after Room_reset and preserves outputs and cache records on failures', () => {
        const p = pair(), n = synthetic(p.native), c = synthetic(p.oracle);
        const q: Query = [0, 32767, 0, -1025];
        const before = p.compare(q).actual.values;
        p.compare([1, 31744, 0, -2048]); p.compare(q);
        // Both queries resolve through room 1; its bottom floor is room 2.
        p.native.w8(n.sectors + 32 * 4, 60); p.oracle.w8(c.sectors + 32 * 4, 60);
        p.native.call('Room_reset'); expect(p.native.tags()).toEqual(Array(p.native.cacheBytes / 32).fill(65535));
        if (FX3) p.native.select(3);
        const after = p.compare(q).actual.values;
        expect(after[1]).not.toBe(before[1]);
        p.compare([1, 31744, 0, -2048]); expect(p.compare(q).actual.values).toEqual(after);
        const records = p.native.raw(p.native.sym('olRoomQueryCache'), p.native.cacheBytes);
        for (const invalid of [[65535, 0, 0, 0], [3, 0, 0, 0]] as Query[]) {
            const answer = p.compare(invalid).actual;
            expect(answer.result).toBe(0); expect(answer.bytes).toEqual(Array(20).fill(0xA5));
            expect(p.native.raw(p.native.sym('olRoomQueryCache'), p.native.cacheBytes)).toEqual(records);
        }
        p.native.set('olSectorCount', 0); p.oracle.set('olSectorCount', 0);
        const empty = p.compare(q).actual;
        expect(empty.result).toBe(0); expect(empty.bytes).toEqual(Array(20).fill(0xA5));
        expect(p.native.raw(p.native.sym('olRoomQueryCache'), p.native.cacheBytes)).toEqual(records);
        console.log('[openlara] room cache mutation/failure: changed floor survives cold/warm reuse; reset tags and output preservation verified');
    });
});

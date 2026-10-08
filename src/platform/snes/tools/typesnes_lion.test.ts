/**
 * FX3 Lion's inherited GBA Enemy behavior, executed from the built cartridge.
 * LEVEL1 supplies a wolf's real model/animation tables; only the packed item
 * type changes to male lion, female lion or puma. This does not claim to test
 * lion assets, which LEVEL1 does not contain.
 * Run with tools/run_typesnes.py and OL_ROM, OL_SYMS, OL_DATA, OL_FX3=1.
 * OL_LION_ASSETS=1 selects the separate original-species level/model suite.
 */
import { describe, expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { dirname, join } from 'path';
import { SNESEmulator } from './emulator';

const ROM = process.env.OL_ROM!;
const FX3 = process.env.OL_FX3 === '1';
const ACTUAL_ASSETS = process.env.OL_LION_ASSETS === '1';
const DATA = process.env.OL_DATA ?? join(dirname(ROM), 'data');
const ITEM_SIZE = 32;
const ITEM = { type: 0, room: 1, state: 2, goalState: 3, x: 4, y: 6, z: 8,
    angleY: 10, animIndex: 12, frameIndex: 14, flags: 15, health: 16,
    model: 22, mood: 23, flags2: 24, nextActive: 25, vSpeed: 28, hSpeed: 30 };
const NAV_SIZE = 8240;
const N = { zone: 8194, end: 8198, target: 8200, step: 8206, drop: 8208,
    speed: 8210, owner: 8214, pos: 8216 };
const HOST_COUNTS = ['items', 'models', 'anims', 'trans', 'triggers', 'cmds',
    'boxes', 'overlaps', 'rooms', 'sectors', 'links', 'statics'];
const SPECIES = [{ type: 12, health: 30, aggression: 0x7FFF },
    { type: 13, health: 25, aggression: 0x2000 },
    { type: 14, health: 40, aggression: 0x2000 }];

function readSymbols(path: string) {
    const table = new Map<string, number>();
    for (const line of readFileSync(path, 'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) table.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
    }
    return table;
}
const SYMS = readSymbols(process.env.OL_SYMS!);

function hostTable(memory: ReturnType<SNESEmulator['getMemory']>, symbol: (name: string) => number, name: string) {
    const at = symbol(`ol${name}Data`);
    return memory.read(at) | memory.read(at + 1) << 8 |
        memory.read(at + 2) << 16 | memory.read(at + 3) << 24;
}

function selectFixtureLevel(memory: ReturnType<SNESEmulator['getMemory']>, symbol: (name: string) => number,
                            write: (address: number, value: number) => void) {
    const table = symbol('LevelTable'), entry = 64;
    const count = (symbol('SECTIONEND_level_table') - table) / entry;
    const chosen = Number(process.env.OL_BOOT_LEVEL ?? 0);
    for (let i = 0; i < entry; i++) write(symbol('olLevel') + i, memory.read(table + chosen * entry + i));
    for (let l = 0; l < count; l++) for (let byte = 0; byte < 2; byte++) {
        write(symbol('olLevelHostBanks') + l * 2 + byte, memory.read(table + l * entry + byte));
        write(symbol('olLevelIds') + l * 2 + byte, memory.read(table + l * entry + 6 + byte));
    }
    for (const [name, value] of [['olLevelIndex', chosen], ['olLevelCount', count]] as const) {
        write(symbol(name), value); write(symbol(name) + 1, value >>> 8);
    }
}

type Observer = Record<string, (stack: number) => void>;
function lionCartridge(type: number) {
    const symbol = (name: string) => {
        const label = SYMS.has(name) ? name : [...SYMS.keys()].find(k => k.endsWith('_' + name));
        const address = SYMS.get(label ?? '');
        if (address === undefined) throw new Error(`missing cartridge symbol ${name}`);
        return address;
    };
    const emulator = new SNESEmulator();
    emulator.loadROM(new Uint8Array(readFileSync(ROM)));
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    // Explicit test-only table patches. The emulator's CPU ROM writes remain
    // read-only; the cartridge code itself is never patched or stubbed.
    const w8 = (at: number, value: number) => {
        if (at >= 0xE00000 && at <= 0xFFFFFF)
            (memory as unknown as { rom: Uint8Array }).rom[at - 0xC00000] = value & 255;
        else memory.write(at, value & 255);
    };
    const w16 = (at: number, value: number) => { w8(at, value); w8(at + 1, value >>> 8); };
    const w32 = (at: number, value: number) => { w16(at, value); w16(at + 2, value >>> 16); };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    const s16 = (at: number) => u16(at) << 16 >> 16;
    const u32 = (at: number) => (u16(at) | u16(at + 2) << 16) >>> 0;
    const bytes = (at: number, count: number) => Buffer.from(Array.from({ length: count }, (_, i) => memory.read(at + i)));
    const call = (name: string, words: number[] = [], observe: Observer = {}) => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E);
        words.forEach((word, i) => w16(0x1EE4 + i * 2, word));
        const at = symbol(name), watches = new Map(Object.entries(observe).map(([n, f]) => [symbol(n), f]));
        cpu.PB = at >>> 16; cpu.PC = at & 65535;
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 500000) {
            watches.get(cpu.PB << 16 | cpu.PC)?.(cpu.S);
            cpu.step();
        }
        expect(steps, `${name} never returned`).toBeLessThan(500000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} corrupted the C ABI`)
            .toEqual([0x1EE3, 0x7E, 0, 0, 4]);
    };
    const set = (values: Record<string, number>) => {
        for (const [name, value] of Object.entries(values)) w16(symbol(name), value);
    };
    const table = (name: string) => hostTable(memory, symbol, name);
    const ptr = (at: number) => [at & 65535, at >>> 16];
    call('gameInit'); selectFixtureLevel(memory, symbol, w8); call('olHostBind');
    expect(u16(symbol('olHostTablesReady'))).toBe(1);
    // Require the packer's fixture data. Synthetic tests select a real wolf;
    // original-assets tests additionally audit the model's actual species.
    expect(readFileSync(join(DATA, 'LEVEL.BIN')).length).toBeGreaterThan(0);
    const count = u16(symbol('olHostDirectory') + 16 + HOST_COUNTS.indexOf('items') * 2);
    const source = table('Item');
    const sourceType = ACTUAL_ASSETS ? type : 7;
    const slot = Array.from({ length: count }, (_, i) => i).find(i => i > 0 && memory.read(source + i * 16) === sourceType)!;
    expect(slot, `fixture must supply original type ${sourceType}`).toBeGreaterThan(0);
    const sourceModel = memory.read(source + slot * 16 + 2);
    for (let i = 1; i < count; i++) w8(source + i * 16 + 3,
        (memory.read(source + i * 16 + 3) & 0x40) | (i === slot ? 0x1F : 0));
    w8(source + slot * 16, type);
    const item = symbol('items'), lion = item + slot * ITEM_SIZE;
    const args = ptr(lion), laraArgs = ptr(item);
    const activation: number[] = [];
    call('olLevelInit', [], { ItemObj_activate: stack => {
        if (u32(stack + 4) === lion) activation.push(s16(lion + ITEM.health));
    } });
    const constructedHealth = s16(lion + ITEM.health);
    const constructedClass = (() => { call('ItemObj_class', [type]); return u16(symbol('tcc__r0')); })();
    const constructedOwners = [0, 1, 2].map(i => u16(symbol('olNav') + i * NAV_SIZE + N.owner));

    // Flat 8x8 room, one connected ground box. Keep the real animation tables.
    set({ olRoomCount: 1, olSectorCount: 64, olDoorCount: 0, olBridgeCount: 0,
        olStaticCount: 0, olUsableCount: 0, olCreatureCount: 1, itemsCount: slot + 1,
        olFirstActive: slot, olBoxCount: 1 });
    w8(symbol('olCreatures'), slot);
    const room = table('Room');
    for (let i = 0; i < 10; i++) w8(room + i, 0);
    w8(room + 8, 8); w8(room + 9, 8);
    for (let link = 0; link < 3; link++) for (let i = 0; i < 8; i++)
        w8(table('Link') + link * 8 + i, i < 4 ? 255 : 0);
    const sector = (x: number, z: number, floor: number) => {
        const at = table('Sector') + (x * 8 + z) * 4;
        w8(at, floor / 256); w8(at + 1, -8); w16(at + 2, 0);
    };
    for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++) sector(x, z, 2048);
    const box = table('Box');
    [0, 7, 0, 7].forEach((v, i) => w8(box + i, v)); w16(box + 4, 2048); w16(box + 6, 0);
    w16(table('Overlap'), 0x8000);
    // Literal nav map: all 64 sectors name box 0, without relying on the
    // packer's predictor encoding. Six independent zone maps name zone 0.
    const map = 0x7FC000, ranks = 2 + 16, literals = ranks + 6;
    w32(symbol('olNavMapData'), map);
    for (let i = 0; i < literals + 80; i++) w8(map + i, 0);
    w8(map, 0x4E); w8(map + 1, 10);
    for (let i = 0; i < 16; i++) w8(map + 2 + i, 255);
    [0, 32, 64].forEach((v, i) => w16(map + ranks + i * 2, v));
    const zones = 0x7FC100;
    w32(symbol('olZoneData'), zones);
    for (let i = 0; i < 6; i++) { w16(zones + i * 2, 12 + i * 2); w16(zones + 12 + i * 2, 0); }
    call('Nav_setFlipped', [0]); call('Nav_reset'); call('navInvalidateSectors'); call('Room_reset'); call('navResetCache');
    [item, lion].forEach((actor, index) => {
        w8(actor + ITEM.room, 0); w8(actor + ITEM.flags, 0x40); w8(actor + ITEM.flags2, index ? 1 : 0);
        w8(actor + ITEM.nextActive, 255); w16(actor + ITEM.x, 2560); w16(actor + ITEM.y, 2048);
        w16(actor + ITEM.z, index ? 1536 : 2560); w16(actor + ITEM.angleY, 0);
        w16(actor + ITEM.vSpeed, 0); w16(actor + ITEM.hSpeed, 0);
    });
    w16(item + ITEM.health, 1000); w16(lion + ITEM.health, constructedHealth); w8(lion + ITEM.mood, 2);
    call('ItemObj_animSet', [...laraArgs, 103]);
    const model = table('Model') + memory.read(lion + ITEM.model) * 16;
    const first = u16(model), animCount = u16(model + 2);
    const pose = (state: number) => {
        const animation = Array.from({ length: animCount }, (_, i) => first + i)
            .find(a => memory.read(table('Anim') + a * 14 + 9) === state)!;
        expect(animation, `wolf fixture has no animation for state ${state}`).toBeDefined();
        call('ItemObj_animSet', [...args, animation]);
    };
    if (ACTUAL_ASSETS) call('ItemObj_animSet', [...args, first]);
    else pose(3);
    const acquire = () => {
        call('Enemy_navAcquire', [...args, 1]);
        return u16(symbol('tcc__r0')) | u16(symbol('tcc__r0h')) << 16;
    };
    return { memory, w8, w16, w32, u16, s16, u32, bytes, call, set, table, symbol,
        slot, item, lion, args, laraArgs, pose, acquire, sector,
        activation, constructedHealth, constructedClass, constructedOwners, sourceModel, first };
}

describe.skipIf(!FX3 || ACTUAL_ASSETS)('OpenLara FX3: GBA Lion inherited controller parity', () => {
    it.each(SPECIES)('constructs type $type before activation and admits its ground navigation', ({ type, health }) => {
        const g = lionCartridge(type);
        g.call('Enemy_health', [type]); expect(g.s16(g.symbol('tcc__r0'))).toBe(health);
        expect(g.constructedHealth).toBe(health); expect(g.activation).toEqual([health]);
        expect(g.constructedClass).toBe(2); expect(g.constructedOwners).toContain(g.slot);
        const nav = g.acquire(); expect(nav).toBeGreaterThan(0);
        expect([g.u16(nav + N.owner), g.u16(nav + N.zone), g.s16(nav + N.step),
            g.s16(nav + N.drop), g.s16(nav + N.speed)]).toEqual([g.slot, 0, 256, -1024, 0]);
    });

    it.each(SPECIES)('uses type $type head offset and exactly one attack roll at aggression edges', ({ type, aggression }) => {
        const g = lionCartridge(type), nav = g.symbol('olNav');
        g.w32(g.symbol('olEnemyNav'), nav); g.w32(g.symbol('olNavEnemy'), g.lion);
        g.w32(g.symbol('olNavLara'), g.item);
        [0, 0x4000, 0x8000, 0xC000].forEach(angle => {
            g.w16(g.lion + ITEM.angleY, angle);
            [0, 2048, 0].forEach((v, i) => g.w32(g.symbol('olEnemyPos') + i * 4, v));
            const target = [angle === 0x4000 ? 3472 : angle === 0xC000 ? -3472 : 0, 2048,
                angle === 0 ? 3472 : angle === 0x8000 ? -3472 : 0];
            target.forEach((v, i) => g.w32(g.symbol('olTargetPos') + i * 4, v));
            g.call('Enemy_moodDistance');
            expect(g.s16(g.symbol('tcc__r0')), `type ${type} yaw ${angle}: 400-unit head offset`).toBe(3072);
        });
        let inverse = 1;
        for (let i = 0; i < 5; i++) inverse = Math.imul(inverse, 2 - Math.imul(0x41C64E6D, inverse));
        for (const roll of [0, 8191, 8192, 32766, 32767]) {
            const initial = Buffer.from(Array.from({ length: NAV_SIZE }, (_, i) => i * 43 + 17));
            initial.writeUInt16LE(0, N.zone);
            initial.forEach((v, i) => g.w8(nav + i, v));
            const target = [0x12345678, -45678, -0x1234567];
            target.forEach((v, i) => g.w32(g.symbol('olTargetPos') + i * 4, v));
            g.set({ olTargetBox: 1023 }); g.w8(g.lion + ITEM.mood, 2);
            g.w32(g.symbol('olRandSeed'), Math.imul((roll << 10) - 0x3039, inverse) >>> 0);
            let rolls = 0;
            g.call('Enemy_moodChoose', [], { rand_logic: () => rolls++ });
            const expected = Buffer.from(initial);
            if (roll < aggression) {
                target.forEach((v, i) => expected.writeInt32LE(v, N.pos + i * 4));
                expected.writeUInt16LE(1023, N.target);
            }
            expect(g.bytes(nav, NAV_SIZE), `type ${type} roll ${roll}`).toEqual(expected);
            expect(rolls).toBe(1); expect(g.u32(g.symbol('olRandSeed'))).toBe(roll << 10);
        }
    });

    it.each(SPECIES)('moves type $type over 256-up/1024-down limits and refuses their next step', ({ type }) => {
        const g = lionCartridge(type); g.acquire();
        for (const [floor, allowed] of [[1792, true], [1536, false], [3072, true], [3328, false]] as const) {
            for (let x = 0; x < 8; x++) for (let z = 3; z < 8; z++) g.sector(x, z, floor);
            g.pose(3); g.w16(g.lion + ITEM.x, 2560); g.w16(g.lion + ITEM.y, 2048);
            g.w16(g.lion + ITEM.z, 3071); g.w16(g.lion + ITEM.angleY, 0);
            g.call('Room_reset'); g.call('navResetCache');
            g.call('Enemy_moveGround', [...g.args, 100]);
            expect(g.s16(g.lion + ITEM.z) > 3071, `type ${type} floor ${floor}`).toBe(allowed);
            expect(g.s16(g.lion + ITEM.y)).toBe(allowed ? floor < 2048 ? floor : 2112 : 2048);
        }
    });

    it.each(SPECIES)('dispatches type $type without inventing attack/death states or contact damage', ({ type }) => {
        const g = lionCartridge(type), nav = g.acquire();
        g.w16(nav + N.end, 0); g.w16(nav + N.target, 0);
        const touch = g.symbol('olEnemyTouch') + g.slot;
        for (const hp of [1, 0, -1]) {
            g.pose(3); g.w16(g.lion + ITEM.health, hp); g.w8(g.lion + ITEM.goalState, 0xE3);
            g.w8(g.symbol('olEnemyNext') + g.slot, 0x7B); g.w8(touch, 1);
            g.w8(g.symbol('olEnemyTurn') + g.slot, 0);
            g.w32(g.symbol('olRandSeed'), 0x56781234);
            const yaw = g.u16(g.lion + ITEM.angleY), frame = g.memory.read(g.lion + ITEM.frameIndex);
            const calls = { dispatch: 0, target: 0, mood: 0, waypoint: 0, bites: 0, rolls: 0 };
            g.call('ItemObj_update', [...g.args, ...g.laraArgs], {
                Lion_update: () => calls.dispatch++, Enemy_target: () => calls.target++,
                Enemy_targetMood: () => calls.mood++, Nav_getWaypoint: () => calls.waypoint++,
                Fx_bite: () => calls.bites++, rand_logic: () => calls.rolls++,
            });
            expect(calls.dispatch).toBe(1); expect(calls.bites).toBe(0);
            expect(g.memory.read(g.lion + ITEM.goalState)).toBe(0xE3);
            expect(g.memory.read(g.symbol('olEnemyNext') + g.slot)).toBe(0x7B);
            expect(g.memory.read(g.symbol('olEnemyTurn') + g.slot)).toBe(0);
            expect(g.s16(g.item + ITEM.health)).toBe(1000); expect(g.memory.read(touch)).toBe(0);
            expect(g.u16(g.lion + ITEM.angleY)).toBe(yaw);
            expect(g.memory.read(g.lion + ITEM.frameIndex)).toBe(frame + 1);
            if (hp <= 0) {
                expect([calls.target, calls.mood, calls.waypoint, calls.rolls]).toEqual([0, 0, 0, 0]);
                expect(g.u32(g.symbol('olRandSeed'))).toBe(0x56781234);
            } else expect([calls.target, calls.mood, calls.waypoint]).toEqual([1, 1, 1]);
        }
    });

    it.each(SPECIES)('routes type $type nonlethal weapon hurt sound and keeps lethal hits silent', ({ type, health }) => {
        const g = lionCartridge(type);
        g.set({ olFxCount: 0, olWeaponTarget: g.slot, olSndGunLoaded: 0 });
        g.w16(g.symbol('olWeaponKit'), 0); g.call('Weapon_params');
        for (const hp of [health, 1]) {
            const sounds: number[] = [];
            g.w16(g.lion + ITEM.health, hp);
            const animation = g.u16(g.lion + ITEM.animIndex), frame = g.memory.read(g.lion + ITEM.frameIndex);
            g.call('Weapon_shot', [1, 0, 0], { Sound_playItem: stack => sounds.push(g.u16(stack + 4)) });
            expect(g.s16(g.lion + ITEM.health)).toBe(hp - 1);
            expect(g.memory.read(g.lion + ITEM.flags2) & 8).toBe(8);
            expect(sounds).toEqual(hp > 1 && type !== 14 ? [85] : []);
            expect([g.u16(g.lion + ITEM.animIndex), g.memory.read(g.lion + ITEM.frameIndex)]).toEqual([animation, frame]);
        }
    });
});

// HOST.RAW's immutable source records decide which species really occur in
// the supplied level. The animation audit ties their original model back to
// its original TR species; no absent animal is represented by another's art.
function actualSpecies() {
    if (!ACTUAL_ASSETS) return [];
    const host = JSON.parse(readFileSync(join(DATA, 'HOST.json'), 'utf8')) as {
        counts: { items: number }; tables: { ITEM: { offset: number } };
    };
    const raw = readFileSync(join(DATA, 'HOST.RAW'));
    const types = new Set(Array.from({ length: host.counts.items }, (_, i) => raw[host.tables.ITEM.offset + i * 16]));
    return SPECIES.filter(s => types.has(s.type));
}
const ACTUAL_SPECIES = actualSpecies();
describe.skipIf(!FX3 || !ACTUAL_ASSETS)('OpenLara FX3: original Lion level models', () => {
    it('contains original lion-family items and animations', () => {
        expect(ACTUAL_SPECIES.length, 'the actual-assets level contains no lion-family species').toBeGreaterThan(0);
        console.log(`[openlara] original lion-family fixture types: ${ACTUAL_SPECIES.map(s => s.type).join(', ')}`);
    });

    it.each(ACTUAL_SPECIES)('constructs and advances original type $type through the inherited controller', ({ type, health }) => {
        const g = lionCartridge(type);
        const audit = JSON.parse(readFileSync(join(DATA, 'FRAMES.json'), 'utf8')) as {
            animations: { anim: number; type: number; ticks: number }[];
        };
        const animation = audit.animations.find(a => a.anim === g.first)!;
        expect(animation, 'original model animation has no source audit').toBeDefined();
        expect(animation.type, 'the source model belongs to another species').toBe(type);
        expect(animation.ticks).toBeGreaterThan(1);
        expect(g.memory.read(g.lion + ITEM.type)).toBe(type);
        expect(g.memory.read(g.lion + ITEM.model)).toBe(g.sourceModel);
        expect(g.constructedHealth).toBe(health); expect(g.activation).toEqual([health]);
        expect(g.constructedClass).toBe(2); expect(g.constructedOwners).toContain(g.slot);
        const nav = g.acquire();
        expect([g.u16(nav + N.zone), g.s16(nav + N.step), g.s16(nav + N.drop), g.s16(nav + N.speed)])
            .toEqual([0, 256, -1024, 0]);
        g.w16(nav + N.end, 0); g.w16(nav + N.target, 0);
        for (const hp of [health, 0]) {
            g.call('ItemObj_animSet', [...g.args, g.first]);
            g.w16(g.lion + ITEM.health, hp); g.w8(g.lion + ITEM.mood, 2);
            g.w8(g.symbol('olEnemyNext') + g.slot, 0); g.w8(g.symbol('olEnemyTurn') + g.slot, 0);
            g.w8(g.symbol('olEnemyTouch') + g.slot, 1);
            const state = g.memory.read(g.lion + ITEM.state), goal = g.memory.read(g.lion + ITEM.goalState);
            const seed = 0x54321234; g.w32(g.symbol('olRandSeed'), seed);
            let dispatches = 0, targets = 0, bites = 0;
            g.call('ItemObj_update', [...g.args, ...g.laraArgs], {
                Lion_update: () => dispatches++, Enemy_target: () => targets++, Fx_bite: () => bites++,
            });
            expect([dispatches, targets, bites]).toEqual([1, hp > 0 ? 1 : 0, 0]);
            expect([g.memory.read(g.lion + ITEM.state), g.memory.read(g.lion + ITEM.goalState),
                g.u16(g.lion + ITEM.animIndex), g.memory.read(g.lion + ITEM.frameIndex)])
                .toEqual([state, goal, g.first, 1]);
            expect(g.memory.read(g.symbol('olEnemyTouch') + g.slot)).toBe(0);
            expect(g.s16(g.item + ITEM.health)).toBe(1000); expect(g.s16(g.lion + ITEM.health)).toBe(hp);
            if (!hp) expect(g.u32(g.symbol('olRandSeed'))).toBe(seed);
        }
    });

    it.each(ACTUAL_SPECIES)('uses original type $type for living and lethal weapon hits', ({ type, health }) => {
        const g = lionCartridge(type);
        g.set({ olFxCount: 0, olWeaponTarget: g.slot, olSndGunLoaded: 0 });
        g.w16(g.symbol('olWeaponKit'), 0); g.call('Weapon_params');
        for (const hp of [health, 1]) {
            g.w16(g.lion + ITEM.health, hp);
            const before = [g.u16(g.lion + ITEM.animIndex), g.memory.read(g.lion + ITEM.state),
                g.memory.read(g.lion + ITEM.goalState), g.memory.read(g.lion + ITEM.frameIndex)];
            const sounds: number[] = [];
            g.call('Weapon_shot', [1, 0, 0], { Sound_playItem: stack => sounds.push(g.u16(stack + 4)) });
            expect(g.s16(g.lion + ITEM.health)).toBe(hp - 1);
            expect(g.memory.read(g.lion + ITEM.flags2) & 8).toBe(8);
            expect(sounds).toEqual(hp > 1 && type !== 14 ? [85] : []);
            expect([g.u16(g.lion + ITEM.animIndex), g.memory.read(g.lion + ITEM.state),
                g.memory.read(g.lion + ITEM.goalState), g.memory.read(g.lion + ITEM.frameIndex)]).toEqual(before);
        }
    });
});

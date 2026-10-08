/** GSU-2 complete Wolf_update and distance against the frozen C cartridge. */
import { expect, it } from 'vitest';
import { readFileSync, existsSync } from 'fs';
import { dirname, join } from 'path';
import { SNESEmulator } from './emulator';
const ROM = process.env.OL_ROM!, DATA_DIR = process.env.OL_DATA ?? join(dirname(ROM), 'data');
const FX3 = process.env.OL_FX3 !== '0';
const WRAM_CODE_IMAGES = FX3 ? [] : [[63, 0x7E8000, 0x8000], [62, 0x7E5800, 0x2800],
    [61, 0x7E0600, 0x1600], [4, 0x7F0000, 0x8000], [5, 0x7F8000, 0x8000]];
const LEVEL_ENTRY = 64, BOOT_LEVEL = Number(process.env.OL_BOOT_LEVEL ?? 0);
const ITEM_SIZE = 32;
const ITEM = { type: 0, room: 1, state: 2, goalState: 3, x: 4, y: 6, z: 8,
 angleY: 10, animIndex: 12, frameIndex: 14, flags: 15, health: 16, model: 22,
 flags2: 24, roomFloor: 26, vSpeed: 28, hSpeed: 30 };
const HOST_COUNTS = ['items','models','anims','trans','triggers','cmds','boxes','overlaps','rooms','sectors','links','statics'];
function readSymbols(path: string) {
    const table = new Map<string, number>();
    if (path && existsSync(path)) {
        for (const line of readFileSync(path, 'utf8').split('\n')) {
            const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
            if (m) table.set(m[3], (parseInt(m[1], 16) << 16) | parseInt(m[2], 16));
        }
    }
    return table;
}

function fixtureWrite(memory: ReturnType<SNESEmulator['getMemory']>, address: number, value: number) {
    if (FX3 && address >= 0xE00000 && address <= 0xFFFFFF)
        (memory as unknown as { rom: Uint8Array }).rom[address - 0xC00000] = value & 255;
    else memory.write(address, value & 255);
}

function hostTable(memory: ReturnType<SNESEmulator['getMemory']>, symbol: (name: string) => number, name: string) {
    const word = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    if (FX3) {
        const at = symbol(`ol${name}Data`);
        return word(at) | word(at + 2) << 16;
    }
    return 0x710000 + (name === 'NavMap' ? word(0x710000 + word(0x710000) - 2) : word(symbol(`ol${name}Base`)));
}

function selectFixtureLevel(memory: ReturnType<SNESEmulator['getMemory']>, symbol: (name: string) => number,
                            write: (address: number, value: number) => void) {
    const table = symbol('LevelTable');
    const count = (symbol('SECTIONEND_level_table') - table) / LEVEL_ENTRY;
    const w16 = (a: number, v: number) => { write(a, v & 255); write(a + 1, v >> 8); };
    const r16 = (a: number) => memory.read(a) | memory.read(a + 1) << 8;
    for (let i = 0; i < LEVEL_ENTRY; i++) write(symbol('olLevel') + i, memory.read(table + BOOT_LEVEL * LEVEL_ENTRY + i));
    for (let l = 0; l < count; l++) {
        w16(symbol('olLevelHostBanks') + l * 2, r16(table + l * LEVEL_ENTRY));
        w16(symbol('olLevelIds') + l * 2, r16(table + l * LEVEL_ENTRY + 6));
    }
    w16(symbol('olLevelIndex'), BOOT_LEVEL);
    w16(symbol('olLevelCount'), count);
}

const SYMS = readSymbols(process.env.OL_SYMS!);
function movementCartridge(romPath = ROM, symbols = SYMS) {
    const sym = (name: string) => {
        const label = symbols.has(name) ? name : [...symbols.keys()].find(k => k.endsWith('_' + name));
        const address = symbols.get(label ?? '');
        if (address === undefined) throw new Error(`missing ${name} in ${romPath}'s symbols`);
        return address;
    };
    const rom = new Uint8Array(readFileSync(romPath));
    const emulator = new SNESEmulator();
    emulator.loadROM(rom);
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    const w8 = (a: number, v: number) => fixtureWrite(memory, a, v);
    const w16 = (a: number, v: number) => { w8(a, v); w8(a + 1, v >> 8); };
    const u16 = (a: number) => memory.read(a) | memory.read(a + 1) << 8;
    const s16 = (a: number) => (u16(a) << 16) >> 16;
    for (const [bank, dest, size] of WRAM_CODE_IMAGES) {
        for (let i = 0; i < size; i++) w8(dest + i, rom[bank * 0x8000 + i]);
    }
    const call = (name: string, words: number[] = [], observe?: { name: string; run: (stack: number) => void }) => {
        const label = symbols.has(name) ? name : [...symbols.keys()].find(k => k.endsWith('_' + name));
        expect(label, `missing routine ${name}`).toBeDefined();
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E);
        words.forEach((word, i) => w16(0x1EE4 + i * 2, word));
        const address = sym(label!);
        cpu.PB = address >>> 16; cpu.PC = address & 65535;
        const watch = observe ? sym(observe.name) : -1;
        w16(0x1EE4 + words.length * 2, 0xD00D); w16(0x1C00, 0xABCD);
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 500000) {
            if ((cpu.PB << 16 | cpu.PC) === watch) observe!.run(cpu.S);
            cpu.step();
        }
        expect(steps, `${name} never returned`).toBeLessThan(500000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} corrupted the C ABI`)
            .toEqual([0x1EE3, 0x7E, 0, 0, 4]);
        expect([u16(0x1EE4 + words.length * 2), u16(0x1C00)], `${name} damaged stack canaries`)
            .toEqual([0xD00D, 0xABCD]);
        return steps;
    };
    call('gameInit');
    expect(sym('olMathCheck') >>> 16, 'boot-only checks should not occupy resident WRAM').toBe(FX3 ? 0x86 : 0xBC);
    expect(u16(sym('ol_mathFail'))).toBe(0);
    const level = readFileSync(join(romPath === ROM ? DATA_DIR : join(dirname(romPath), 'data'), 'LEVEL.BIN'));
    const host = level.readUInt16LE(2);
    const size = FX3 ? 0 : level.readUInt16LE(host);
    const blob = FX3 ? 0x7F0000 : 0x710000;
    const loadTables = () => {
        if (FX3) {
            selectFixtureLevel(memory, sym, w8);
            const hostFile = (u16(sym('olLevel')) - 0xC0) << 16;
            const hostSize = new DataView(rom.buffer).getUint32(hostFile + 8, true);
            for (let i = 0; i < hostSize; i++) w8(0xC00000 + hostFile + i, rom[hostFile + i]);
            call('olHostBind');
            expect(u16(sym('olHostTablesReady'))).toBe(1);
        } else for (let i = 0; i < size; i++) w8(blob + i, level[host + i]);
    };
    loadTables();
    call('olLevelInit');
    const table = (name: string) => hostTable(memory, sym, name);
    const count = (name: string) => FX3 ? u16(sym('olHostDirectory') + 16 + HOST_COUNTS.indexOf(name) * 2) :
        level.readUInt16LE(host + ({ items: 2, anims: 6, overlaps: 16 } as Record<string, number>)[name]);
    const item = sym('items');
    const args = [item & 65535, item >>> 16];
    const set = (values: Record<string, number>) => {
        for (const [name, value] of Object.entries(values)) {
            w16(sym(name), value);
            if (FX3 && /^ol\w+Base$/.test(name)) w16(sym(name) + 2, value === 0 && name === 'olAnimAimBase' ? 0 : 0x7F);
        }
    };
    // Shared fixtures can name just ground/fly; zone-specific tests supply
    // all six original maps. FX3 stages synthetic maps in an explicit free
    // WRAM range, never overwrites a neighbouring immutable ROM table.
    const zoneMap = (input: number[][], deduplicate = true, flipped = 0) => {
        if (!FX3) {
            expect(input.length, 'GSU-2 supplies only ground and fly zones').toBe(2);
            input.forEach(map => expect(map.length).toBe(u16(sym('olBoxCount'))));
            input.flat().forEach((v, i) => w16(table('Zone') + i * 2, v));
            return;
        }
        const maps = input.length === 2 ? [input[0], input[0], input[1], input[0], input[0], input[1]] : input;
        expect(maps.length).toBe(6);
        maps.forEach(map => expect(map.length).toBe(u16(sym('olBoxCount'))));
        const address = 0x7FC000;
        w16(sym('olZoneData'), address & 65535); w16(sym('olZoneData') + 2, address >>> 16);
        const seen = new Map<string, number>();
        let offset = 12;
        maps.forEach((map, index) => {
            const key = map.join(','), old = deduplicate ? seen.get(key) : undefined;
            w16(address + index * 2, old ?? offset);
            if (old !== undefined) return;
            seen.set(key, offset);
            map.forEach((v, i) => w16(address + offset + i * 2, v));
            offset += map.length * 2;
        });
        expect((address & 65535) + offset).toBeLessThanOrEqual(65536);
        call('Nav_setFlipped', [flipped]);
    };
    const zoneAddress = (kind: number, flipped = 0) => table('Zone') + (FX3 ?
        u16(table('Zone') + (flipped * 3 + kind) * 2) : (kind ? u16(sym('olBoxCount')) * 2 : 0));
    const anim = (index: number, frame = 0) => call('ItemObj_animSetFrame', [...args, index, frame]);
    const state = () => memory.read(item + ITEM.state);
    const goal = () => memory.read(item + ITEM.goalState);
    const room = (index: number, water: boolean) => {
        const r = table('Room') + index * 10;
        for (let i = 0; i < 10; i++) w8(r + i, 0);
        w16(r + 6, index * 64); w8(r + 8, 8 | (water ? 128 : 0)); w8(r + 9, 8);
    };
    const sector = (index: number, x: number, z: number, floor: number, ceiling: number, link = 0) => {
        const s = table('Sector') + (index * 64 + x * 8 + z) * 4;
        w8(s, floor / 256); w8(s + 1, ceiling / 256); w16(s + 2, link);
    };
    // Fixture maps deliberately use literals only: no predictor encoder
    // shared with the packer, and no guessed geometric box membership.
    const navMap = (boxes: number[], bits = 10, base?: number) => {
        const ranks = 2 + Math.ceil(boxes.length / 4);
        const literals = ranks + (Math.ceil(boxes.length / 32) + 1) * 2;
        const ids = boxes.filter(b => b !== 65535);
        const data = Buffer.alloc(literals + Math.ceil(ids.length * bits / 8));
        data[0] = 0x4E; data[1] = bits;
        let rank = 0;
        boxes.forEach((box, i) => {
            if (!(i & 31)) data.writeUInt16LE(rank, ranks + (i >>> 5) * 2);
            if (box === 65535) return;
            expect(box).toBeLessThan(1 << bits);
            data[2 + (i >>> 2)] |= 3 << ((i & 3) * 2);
            for (let b = 0; b < bits; b++) {
                const bit = rank * bits + b;
                data[literals + (bit >>> 3)] |= (box >>> b & 1) << (bit & 7);
            }
            rank++;
        });
        data.writeUInt16LE(rank, literals - 2);
        const address = base === undefined ? table('NavMap') : blob + base;
        if (!FX3) {
            expect(address + data.length, 'fixture nav map overwrites footer').toBeLessThanOrEqual(blob + u16(blob) - 2);
            w16(blob + u16(blob) - 2, address - blob);
        } else {
            expect((address & 65535) + data.length, 'fixture nav map crosses a bank').toBeLessThanOrEqual(65536);
            w16(sym('olNavMapData'), address); w16(sym('olNavMapData') + 2, address >>> 16);
        }
        data.forEach((v, i) => w8(address + i, v));
        set({ olSectorCount: boxes.length });
        if (FX3) call('navInvalidateSectors');
    };
    const restoreLevel = () => {
        loadTables();
        call('olLevelInit'); call('Room_reset'); call('navResetCache');
    };
    // Keep the actual animation tables; replace only the room geometry with
    // a flat room/ledge so a transition has one cause. All links are empty.
    set({ olRoomCount: 1, olSectorCount: 128, olDoorCount: 0, olBridgeCount: 0,
        olStaticCount: 0, olUsableCount: 0, olCreatureCount: 0, itemsCount: 1, olFirstActive: 255 });
    for (let link = 0; link < 3; link++) {
        const p = table('Link') + link * 8;
        for (let i = 0; i < 8; i++) w8(p + i, i < 4 ? 255 : 0);
    }
    room(0, false);
    for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++) sector(0, x, z, 2048, -2048);
    navMap(Array(128).fill(0));
    const reset = () => {
        anim(103);
        w8(item + ITEM.room, 0); w8(item + ITEM.flags2, 0);
        w16(item + ITEM.x, 2560); w16(item + ITEM.y, 2048); w16(item + ITEM.z, 2560);
        w16(item + ITEM.angleY, 0); w16(item + ITEM.vSpeed, 0); w16(item + ITEM.hSpeed, 0);
        w16(item + ITEM.health, 1000);
        set({ olLaraTurn: 0, olLaraMoveAngle: 0, olLaraAngleX: 0, olLaraWaterState: 0,
            olLaraWeaponState: 0, olLaraOxygen: 1800 });
        call('Room_reset');
    };
    reset();
    return { memory, cpu, w8, w16, u16, s16, call, blob, table, count, item, args, set, anim, state, goal,
        room, sector, navMap, zoneMap, zoneAddress, restoreLevel, reset, symbol: sym };
}

type Cartridge = ReturnType<typeof movementCartridge>;
function wolfActor(g: Cartridge) {
    const slot = Array.from({ length: g.count('items') }, (_, i) => i)
        .find(i => i > 0 && g.memory.read(g.item + i * ITEM_SIZE + ITEM.type) === 7)!;
    expect(slot, 'the source level contains no wolf model').toBeGreaterThan(0);
    const item = g.item + slot * ITEM_SIZE, args = [item & 65535, item >>> 16];
    const model = g.table('Model') + g.memory.read(item + ITEM.model) * 16;
    const first = g.u16(model), count = g.u16(model + 2);
    const animation = (state: number) => {
        const index = Array.from({ length: count }, (_, i) => first + i)
            .find(a => g.memory.read(g.table('Anim') + a * 14 + 9) === state)!;
        expect(index, `the wolf model has no animation for state ${state}`).toBeDefined();
        return index;
    };
    const pose = (state: number) => {
        g.call('ItemObj_animSet', [...args, animation(state)]);
        g.w8(g.symbol('olEnemyNext') + slot, 0);
    };
    return { item, args, slot, pose, run: animation(3) };
}

function groundFixture(g: Cartridge) {
    const actor = wolfActor(g);
    g.set({ olBoxCount: 1, itemsCount: actor.slot + 1, olFirstActive: actor.slot, olCreatureCount: 1 });
    g.w8(g.symbol('olCreatures'), actor.slot);
    const box = g.table('Box');
    [0, 7, 0, 7].forEach((v, i) => g.w8(box + i, v)); g.w16(box + 4, 2048); g.w16(box + 6, 0);
    g.zoneMap([[0], [0]]); g.call('navResetCache');
    g.w8(actor.item + ITEM.room, 0); g.w8(actor.item + ITEM.flags2, 1);
    g.w8(actor.item + ITEM.flags, 0x40); g.w16(actor.item + ITEM.health, 6);
    g.w16(actor.item + ITEM.x, 2560); g.w16(actor.item + ITEM.y, 2048); g.w16(actor.item + ITEM.z, 1536);
    g.w16(actor.item + ITEM.angleY, 0); g.w16(actor.item + ITEM.hSpeed, 0); g.w16(actor.item + ITEM.vSpeed, 0);
    return actor;
}

function snapshot(g: Cartridge, actor: ReturnType<typeof wolfActor>) {
    return Buffer.from(Array.from({ length: ITEM_SIZE }, (_, i) => g.memory.read(actor.item + i))
        .concat(g.memory.read(g.symbol('olEnemyNext') + actor.slot)));
}

function resetSectorCache(g: Cartridge) {
    g.call('Room_reset');
    g.call(FX3 ? 'navInvalidateSectors' : 'navResetCache');
}

const REFERENCE = process.env.OL_REFERENCE_ROM;
const SKIP = FX3 || !REFERENCE;
function wolfPair() {
    const pair = [movementCartridge(), movementCartridge(REFERENCE!, readSymbols(process.env.OL_REFERENCE_SYMS!))];
    const actors = pair.map(groundFixture);
    for (const g of pair) g.set({ olEnemyInZone: 0 });
    expect(pair[0].symbol('Wolf_update') >>> 16).toBe(0x7F);
    expect(pair[0].symbol('Enemy_distance') >>> 16).toBe(0x7F);
    return { pair, actors };
}
function w32(g: Cartridge, name: string, value: number) {
    g.w16(g.symbol(name), value); g.w16(g.symbol(name) + 2, value >>> 16);
}
function seed(g: Cartridge) {
    const at = g.symbol('olRandSeed');
    return (g.u16(at) | g.u16(at + 2) << 16) >>> 0;
}
function fullSnapshot(g: Cartridge, actor: ReturnType<typeof wolfActor>) {
    const bytes = (at: number, count: number) => Array.from({ length: count }, (_, i) => g.memory.read(at + i));
    return Buffer.from(bytes(actor.item, ITEM_SIZE).concat(bytes(g.item, ITEM_SIZE),
        bytes(g.symbol('olEnemyNext') + actor.slot, 1), bytes(g.symbol('olEnemyTurn') + actor.slot, 1),
        bytes(g.symbol('olRandSeed'), 4), bytes(g.symbol('olEnemyInZone'), 2),
        bytes(g.symbol('navFrom'), 4), bytes(g.symbol('navNext'), 2 * 1024 * 2), bytes(g.symbol('navReached'), 2)));
}

it.skipIf(SKIP).each(['Wolf_update', 'Enemy_update'])(
    'matches frozen C complete wolf steering, targeting, attacks and death through %s', dispatch => {
    const { pair, actors } = wolfPair();
    const states = [1,2,3,5,6,7,8,9,11,12];
    const deltas = [-32768,-32767,-1537,-769,-375,-345,-1,0,1,344,375,768,1536,32766,32767];
    const angles = [0,1,31,32,16383,16384,16385,32767,32768,49151,49152,49153,65535];
    const instructions = [0,0];
    let turns = 0;
    for (let test = 0; test < 640; test++) {
        const snapshots: Buffer[] = [], targetInfo: number[][] = [];
        for (const [index, g] of pair.entries()) {
            const actor = actors[index];
            g.reset(); actor.pose(states[test % states.length]);
            g.w8(actor.item + ITEM.room, 0); g.w8(actor.item + ITEM.flags, 0x40);
            g.w8(actor.item + ITEM.flags2, 1 | (test & 4) | ((test >>> 1) & 8) | (test & 32));
            g.w8(actor.item + 23, (test >>> 2) & 3);
            g.w16(actor.item + ITEM.health, test % 13 ? 6 : 0);
            g.w16(actor.item + ITEM.x, 2560); g.w16(actor.item + ITEM.y, 2048); g.w16(actor.item + ITEM.z, 2560);
            const yaw = angles[(test * 7) % angles.length];
            g.w16(actor.item + ITEM.angleY, yaw);
            g.w16(actor.item + ITEM.hSpeed, [0,8,100,-8,-100,32767,-32768][test % 7]);
            g.w16(actor.item + ITEM.vSpeed, (test % 7 - 3) * 8);
            g.w16(g.item + ITEM.x, 2560 + deltas[test % deltas.length]);
            g.w16(g.item + ITEM.z, 2560 + deltas[(test * 7 + 3) % deltas.length]);
            g.w16(g.item + ITEM.y, 2048 + [-257,-256,-255,0,255,256,257][test % 7]);
            g.w16(g.item + ITEM.angleY, angles[(test * 3 + 1) % angles.length]);
            g.w16(g.item + ITEM.health, test % 17 ? 1000 : 0);
            g.w8(g.symbol('olEnemyTurn') + actor.slot, [0,2,5][test % 3]);
            g.w8(g.symbol('olEnemyNext') + actor.slot, [0,1,3,5,8,9][(test >>> 3) % 6]);
            w32(g, 'olRandSeed', (Math.imul(test + 1, 1664525) + 1013904223) >>> 0);
            g.set({ olEnemyInZone: 0xBEEF }); g.call('Room_reset');
            // One box keeps these original target-info cases independent of
            // flood state. A separate multi-box group exercises its mutation.
            let info: number[] = [];
            instructions[index] += g.call(dispatch, [...actor.args, ...g.args], {
                name: 'Wolf_state', run: stack => { info = [g.u16(stack + 8), g.u16(stack + 10)]; },
            });
            targetInfo.push(info); snapshots.push(fullSnapshot(g, actor));
            if (index === 0 && g.u16(actor.item + ITEM.angleY) !== yaw) turns++;
        }
        expect(targetInfo[0], `${dispatch} target info case ${test}`).toEqual(targetInfo[1]);
        expect(snapshots[0], `${dispatch} full state case ${test}, state ${states[test % states.length]}`).toEqual(snapshots[1]);
    }
    expect(turns).toBeGreaterThan(20); expect(instructions[0]).toBeLessThan(instructions[1]);
    console.log(`[openlara] GSU-2 ${dispatch}: 640 exact C target-info/item/Lara/mood/PRNG/flood snapshots; ` +
        `${turns} steering cases; ${instructions[0]}/${instructions[1]} instructions`);
});

it.skipIf(SKIP)('preserves signed distance saturation at every edge and deterministic full-width samples', () => {
    const { pair } = wolfPair();
    const edges = [-32768,-32767,-1537,-769,-375,-345,-1,0,1,344,375,768,1536,32766,32767];
    const cases = edges.flatMap(x => edges.map(z => [x,z]));
    let random = 0x54321234;
    for (let i = 0; i < 4096; i++) {
        random = (Math.imul(random, 1664525) + 1013904223) >>> 0;
        const x = random << 16 >> 16;
        random = (Math.imul(random, 1664525) + 1013904223) >>> 0;
        cases.push([x, random << 16 >> 16]);
    }
    const instructions = [0,0];
    for (const [dx,dz] of cases) {
        const expected = Math.min(32767, Math.max(Math.abs(dx), Math.abs(dz)) + (Math.min(Math.abs(dx), Math.abs(dz)) >> 1));
        for (const [index,g] of pair.entries()) {
            const before = seed(g);
            instructions[index] += g.call('Enemy_distance', [dx,dz]);
            expect(g.s16(g.symbol('tcc__r0')), `distance ${index ? 'C' : 'native'} ${dx}/${dz}`).toBe(expected);
            expect(seed(g)).toBe(before);
        }
    }
    expect(instructions[0]).toBeLessThan(instructions[1]);
    console.log(`[openlara] GSU-2 distance: ${cases.length} exact C/independent signed/saturated results; ` +
        `${instructions[0]}/${instructions[1]} instructions`);
});

it.skipIf(SKIP)('chooses all three finite death animations once and clears pending attacks', () => {
    const { pair, actors } = wolfPair();
    let inverse = 1;
    for (let i = 0; i < 5; i++) inverse = Math.imul(inverse, 2 - Math.imul(0x41C64E6D, inverse));
    const deaths = new Set<number>();
    for (const roll of [0,10922,10923,21845,21846,32767]) {
        const snapshots: Buffer[] = [];
        for (const [index,g] of pair.entries()) {
            const actor = actors[index]; g.reset(); actor.pose(3);
            g.w16(actor.item + ITEM.health, 0); g.w8(g.symbol('olEnemyNext') + actor.slot, 9);
            g.w8(actor.item + ITEM.flags2, 1); g.w8(actor.item + ITEM.flags, 0x40);
            w32(g, 'olRandSeed', Math.imul((roll << 10) - 0x3039, inverse) >>> 0);
            let rolls = 0;
            g.call('Wolf_update', [...actor.args, ...g.args], { name: 'rand_logic', run: () => rolls++ });
            const model = g.table('Model') + g.memory.read(actor.item + ITEM.model) * 16;
            const chosen = g.u16(actor.item + ITEM.animIndex) - g.u16(model);
            expect(chosen).toBe(20 + ((roll * 3) >> 15)); deaths.add(chosen);
            expect(rolls).toBe(1); expect(seed(g)).toBe(roll << 10);
            expect(g.memory.read(actor.item + ITEM.state)).toBe(11);
            expect(g.memory.read(g.symbol('olEnemyNext') + actor.slot)).toBe(0);
            g.call('Wolf_update', [...actor.args, ...g.args]);
            expect(seed(g), 'an existing corpse rerolled its death graph').toBe(roll << 10);
            expect(g.u16(actor.item + ITEM.animIndex) - g.u16(model)).toBe(chosen);
            snapshots.push(fullSnapshot(g, actor));
        }
        expect(snapshots[0], `death roll ${roll}`).toEqual(snapshots[1]);
    }
    expect([...deaths].sort()).toEqual([20,21,22]);
    console.log('[openlara] GSU-2 wolf deaths: six PRNG threshold edges select all three graphs once, clear pending state, and retain the corpse seed');
});

it.skipIf(SKIP)('uses the previous 0/2/5-degree turn limits and the close reverse half limit', () => {
    const { pair, actors } = wolfPair();
    for (const limit of [0,2,5]) for (const reverse of [false,true]) {
        const snapshots: Buffer[] = [];
        for (const [index,g] of pair.entries()) {
            const actor = actors[index]; g.reset(); actor.pose(reverse ? 3 : 2);
            g.w16(actor.item + ITEM.health, 6); g.w8(actor.item + 23, 2); g.w8(actor.item + ITEM.flags2, 1);
            g.w16(actor.item + ITEM.x, 2560); g.w16(actor.item + ITEM.y, 2048); g.w16(actor.item + ITEM.z, reverse ? 2560 : 1536);
            g.w16(actor.item + ITEM.angleY, 0); g.w16(actor.item + ITEM.hSpeed, reverse ? 100 : 8);
            g.w16(actor.item + ITEM.vSpeed, 0); g.w16(g.item + ITEM.x, reverse ? 2560 : 5000);
            g.w16(g.item + ITEM.z, reverse ? 2300 : 1536); g.w8(g.symbol('olEnemyTurn') + actor.slot, limit);
            w32(g, 'olRandSeed', 0x54321234);
            g.call('Wolf_update', [...actor.args, ...g.args]);
            const expected = limit ? reverse ? (65536 - (limit * 182 >> 1)) & 65535 : limit * 182 : 0;
            expect(g.u16(actor.item + ITEM.angleY), `${index ? 'C' : 'native'} previous ${limit}, reverse=${reverse}`).toBe(expected);
            snapshots.push(fullSnapshot(g, actor));
        }
        expect(snapshots[0], `previous turn ${limit}, reverse=${reverse}`).toEqual(snapshots[1]);
    }
    console.log('[openlara] GSU-2 wolf steering: six exact C previous-state 0/2/5-degree and close-reverse half-limit updates');
});

it.skipIf(SKIP)('preserves legacy rotated-union contact, clamped Lara damage and each attack latch', () => {
    const { pair, actors } = wolfPair();
    const scenarios = [{ name: 'inside', dx: 0, touching: true }, { name: 'edge', dx: -300, touching: true },
        { name: 'outside-edge', dx: -301, touching: false }, { name: 'far', dx: 2048, touching: false }];
    let count = 0;
    for (const state of [6,12]) for (const hp of [0,1,25,1000]) for (const pending of [0,3])
    for (const scenario of scenarios) {
        const snapshots: Buffer[] = [], infos: number[] = [];
        for (const [index,g] of pair.entries()) {
            const actor = actors[index]; g.reset(); actor.pose(state);
            const box = g.table('AnimBox') + g.u16(actor.item + ITEM.animIndex) * 12;
            [-200,200,-800,0,-300,300].forEach((v,i) => g.w16(box + i * 2,v));
            g.w16(g.table('AnimBox') + 103 * 12 + 4,-762);
            g.w16(actor.item + ITEM.health, 6); g.w8(actor.item + ITEM.flags2, 1); g.w8(actor.item + 23,2);
            g.w16(actor.item + ITEM.x,2560); g.w16(actor.item + ITEM.y,2048); g.w16(actor.item + ITEM.z,2560);
            g.w16(actor.item + ITEM.angleY,0); g.w16(actor.item + ITEM.hSpeed,0); g.w16(actor.item + ITEM.vSpeed,0);
            g.w16(g.item + ITEM.x,2560 + scenario.dx); g.w16(g.item + ITEM.z,2560); g.w16(g.item + ITEM.health,hp);
            g.w8(g.symbol('olEnemyNext') + actor.slot,pending); g.w8(g.symbol('olEnemyTurn') + actor.slot,0);
            w32(g,'olRandSeed',0x12345678);
            let info = 0;
            g.call('Wolf_update',[...actor.args,...g.args], { name: 'Wolf_state', run: stack => { info = g.u16(stack + 8); } });
            expect(Boolean(info & 16), `${index ? 'C' : 'native'} ${state}/${hp}/${pending}/${scenario.name}`).toBe(hp > 0 && scenario.touching);
            const expected = hp > 0 && !pending && scenario.touching ? Math.max(0,hp - (state === 6 ? 50 : 100)) : hp;
            expect(g.s16(g.item + ITEM.health)).toBe(expected);
            const after = g.s16(g.item + ITEM.health);
            g.call('Wolf_update',[...actor.args,...g.args]);
            expect(g.s16(g.item + ITEM.health),'an attack damaged Lara twice from one latch').toBe(after);
            infos.push(info); snapshots.push(fullSnapshot(g,actor));
        }
        expect(infos[0]).toBe(infos[1]); expect(snapshots[0],`legacy contact ${state}/${hp}/${pending}/${scenario.name}`).toEqual(snapshots[1]); count++;
    }
    console.log(`[openlara] GSU-2 wolf contact: ${count} exact C contact/health/latch snapshots, inclusive rotated-union edges and clamped 50/100-point damage`);
});

it.skipIf(SKIP)('preserves shared flood, seam, portal and invalid-box updates', () => {
    const { pair, actors } = wolfPair();
    const cases = ['seam','portal','old-hole','next-hole','step-up','blocked-step'] as const;
    let count = 0;
    for (const scenario of cases) for (const connected of [false,true]) for (const previousTurn of [0,2,5]) {
        const snapshots: Buffer[] = [];
        for (const [index,g] of pair.entries()) {
            const actor = actors[index]; g.reset(); actor.pose(3);
            g.set({ olRoomCount: 2, olBoxCount: 2, olCreatureCount: 1 }); g.room(0,false); g.room(1,false);
            for (let r = 0; r < 2; r++) for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++)
                g.sector(r,x,z,x >= 3 && scenario === 'step-up' ? 1792 : x >= 3 && scenario === 'blocked-step' ? 1536 : 2048,-2048);
            const link = g.table('Link');
            for (let n = 0; n < 2; n++) for (let i = 0; i < 8; i++) g.w8(link + n * 8 + i,i < 4 ? 255 : 0);
            g.w8(link + 10,1);
            if (scenario === 'portal') g.sector(0,3,2,2048,-2048,1);
            const boxes = Array.from({ length: 128 },(_,i) => i >= 64 || (i >>> 3) >= 3 ? 1 : 0);
            if (scenario === 'old-hole') boxes[18] = 65535;
            if (scenario === 'next-hole') boxes[26] = 65535;
            g.navMap(boxes); g.zoneMap([[0,connected ? 0 : 1],[0,0]]);
            [[0,8,0,3],[0,8,3,8]].forEach((bounds,b) => {
                const box = g.table('Box') + b * 8;
                bounds.forEach((v,i) => g.w8(box + i,v)); g.w16(box + 4,2048); g.w16(box + 6,b);
            });
            g.w16(g.table('Overlap'),0x8001); g.w16(g.table('Overlap') + 2,0x8000);
            g.w16(actor.item + ITEM.health,6); g.w8(actor.item + 23,2); g.w8(actor.item + ITEM.flags2,1);
            g.w16(actor.item + ITEM.x,3071); g.w16(actor.item + ITEM.y,2048); g.w16(actor.item + ITEM.z,2560);
            g.w16(actor.item + ITEM.angleY,16384); g.w16(actor.item + ITEM.hSpeed,100); g.w16(actor.item + ITEM.vSpeed,0);
            g.w16(g.item + ITEM.x,4000); g.w16(g.item + ITEM.z,2560);
            g.w8(g.symbol('olEnemyTurn') + actor.slot,previousTurn); w32(g,'olRandSeed',0x87654321);
            resetSectorCache(g);
            g.call('Wolf_update',[...actor.args,...g.args]); snapshots.push(fullSnapshot(g,actor));
            if (scenario === 'seam' && connected) expect(g.u16(g.symbol('navFrom'))).toBe(1);
        }
        expect(snapshots[0],`wolf geometry ${scenario}/${connected}/${previousTurn}`).toEqual(snapshots[1]); count++;
    }
    console.log(`[openlara] GSU-2 wolf geometry: ${count} exact C full-item/Lara/PRNG/flood seam/portal/step/hole snapshots`);
});

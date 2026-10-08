/** Native ground movement against the unchanged executable C movement. */
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

it.skipIf(!process.env.OL_REFERENCE_ROM)(FX3 ?
 'preserves same sectors, seams, portals, holes and flipped zones exactly like C' :
 'preserves same sectors, seams, portals, holes and ground/fly zones exactly like C', () => {
 const pair = [movementCartridge(), movementCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!))];
 const actors = pair.map(wolfActor);
 const cases = [
  {name:'same-sector',x:2560,z:2560,portal:false,hole:'none'},
  {name:'z-seam',x:2560,z:3071,portal:false,hole:'none'},
  {name:'x-seam',x:3071,z:2560,portal:false,hole:'none'},
  {name:'wrapped-seam',x:32767,z:32767,portal:false,hole:'none'},
  {name:'portal-same-sector',x:2560,z:2560,portal:true,hole:'none'},
  {name:'invalid-old-box',x:2560,z:2560,portal:false,hole:'old'},
  {name:'invalid-next-box',x:2560,z:3071,portal:false,hole:'next'},
 ];
 let count=0, fastCases=0, fallbackCases=0, nativeSteps=0, referenceSteps=0;
 const types = FX3 ? [7,8,12,13,14,19] : [0,1,7,8,9,12,14,19,34,255];
 for(const type of types) for(const flipped of FX3 ? [0,1] : [0]) for(const scenario of cases) for(const yaw of [0,16384,32768,49152]) {
  const snapshots: Buffer[]=[];
  for(const [index,g] of pair.entries()) {
   const a=actors[index];
   g.set({olRoomCount:2,olBoxCount:3,olCreatureCount:0,olBridgeCount:0,olDoorCount:0,itemsCount:g.count('items')});
   g.room(0,false);g.room(1,false);
   for(let r=0;r<2;r++)for(let x=0;x<8;x++)for(let z=0;z<8;z++)g.sector(r,x,z,2048,-2048);
   const link=g.table('Link');
   for(let n=0;n<2;n++)for(let i=0;i<8;i++)g.w8(link+n*8+i,i<4?255:0);
   g.w8(link+8+2,1); // horizontal portal into room1, whose link0 stops it
   if(scenario.portal)for(let x=0;x<8;x++)for(let z=0;z<8;z++)g.sector(0,x,z,2048,-2048,1);
   const boxes=Array.from({length:128},(_,i)=>i>=64?2:i%8>=3?1:0);
   if(scenario.hole==='old')boxes[18]=65535;
   if(scenario.hole==='next')boxes[19]=65535;
   g.navMap(boxes);
   for(let b=0;b<3;b++){
    [0,8,0,8].forEach((v,i)=>g.w8(g.table('Box')+b*8+i,v));g.w16(g.table('Box')+b*8+4,2048);g.w16(g.table('Box')+b*8+6,0);
   }
   g.zoneMap(FX3 ? [[0,1,0],[0,1,0],[0,0,0],[4,4,5],[4,4,5],[0,0,0]] : [[0,1,0],[0,0,0]],true,flipped);
   g.call('ItemObj_animSet',[...a.args,a.run]);
   g.w8(a.item+ITEM.type,type);g.w8(a.item+ITEM.room,0);g.w8(a.item+ITEM.flags2,1);
   g.w16(a.item+ITEM.x,scenario.x);g.w16(a.item+ITEM.y,2048);g.w16(a.item+ITEM.z,scenario.z);
   g.w16(a.item+ITEM.angleY,yaw);g.w16(a.item+ITEM.hSpeed,100);g.w16(a.item+ITEM.vSpeed,0);g.w16(a.item+ITEM.roomFloor,-1234);
   resetSectorCache(g);
   let lookup=0;
   const observe=index===0?{name:'EG_LookupBox',run:()=>lookup++}:undefined;
   const steps=g.call('Enemy_moveGround',[...a.args,182],observe);
   index?referenceSteps+=steps:nativeSteps+=steps;
   snapshots.push(snapshot(g,a));
   if(index===0){
    if(scenario.name==='same-sector')expect(lookup,'stable sector recomputed its box').toBe(0);
    if(scenario.portal||scenario.hole==='old')expect(lookup,'portal/invalid old box skipped the full check').toBe(1);
    lookup?fallbackCases++:fastCases++;
   }
  }
  expect(snapshots[0],`${type}/${flipped}/${scenario.name}/${yaw}`).toEqual(snapshots[1]); count++;
 }
 expect(fastCases).toBeGreaterThan(0);expect(fallbackCases).toBeGreaterThan(0);
 console.log(`[openlara] ${FX3 ? 'FX3' : 'GSU-2'} ground sectors: ${count} exact native/C seam/portal/hole/${FX3 ? 'flipped' : 'ground/fly'} snapshots; ` +
  `${fastCases} stable-sector / ${fallbackCases} fallback lookups; ${nativeSteps}/${referenceSteps} instructions`);
});

it.skipIf(!process.env.OL_REFERENCE_ROM)('matches original C ground movement across 384 animation, height, collision and turn boundaries', () => {
    const pair = [movementCartridge(), movementCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!))];
    const actors = pair.map(groundFixture);
    let nativeSteps = 0, referenceSteps = 0;
    for (let test = 0; test < 384; test++) {
        const snapshots: Buffer[] = [];
        for (const [index, g] of pair.entries()) {
            const actor = actors[index];
            actor.pose([1,2,3,5,6,9,11,12][test % 8]);
            const x = [2560,3071,-32768,32767][(test >>> 3) % 4];
            const z = [3071,2560,32767,-32768][(test >>> 5) % 4];
            const y = 2048 + [-1025,-1024,-65,-64,0,256,257,0][(test >>> 1) % 8];
            g.w16(actor.item + ITEM.x, x); g.w16(actor.item + ITEM.y, y); g.w16(actor.item + ITEM.z, z);
            g.w16(actor.item + ITEM.angleY, (test * 997) & 65535);
            g.w16(actor.item + ITEM.hSpeed, test & 1 ? 100 : 0);
            g.w16(actor.item + ITEM.vSpeed, (test % 7 - 3) * 8);
            g.w8(actor.item + ITEM.flags2, 1 | (test & 4)); g.w8(actor.item + ITEM.room, 0);
            g.w16(actor.item + ITEM.roomFloor, -1234);
            const otherSlot = 1, other = g.item + otherSlot * ITEM_SIZE;
            g.set({ olCreatureCount: 2 });
            g.w8(g.symbol('olCreatures'), actor.slot); g.w8(g.symbol('olCreatures') + 1, otherSlot);
            g.w8(other + ITEM.room, (test >>> 2) & 1); g.w16(other + ITEM.health, test % 5 ? 6 : 0);
            g.w8(other + ITEM.flags2, (test >>> 3) & 3);
            g.w16(other + ITEM.x, x + [-341,-340,-339,0,339,340,341][test % 7]);
            g.w16(other + ITEM.z, z + [-341,0,339][test % 3]);
            resetSectorCache(g);
            const steps = g.call('Enemy_moveGround', [...actor.args, [-32768,-1820,-364,0,364,1820,32767][test % 7]]);
            index ? referenceSteps += steps : nativeSteps += steps;
            snapshots.push(snapshot(g, actor));
        }
        expect(snapshots[0], `original ground movement case ${test}`).toEqual(snapshots[1]);
    }
    expect(nativeSteps).toBeLessThan(referenceSteps);
    console.log(`[openlara] ${FX3 ? 'FX3' : 'GSU-2'} ground boundaries: 384 exact C full-item/next-state snapshots; ` +
        `${nativeSteps}/${referenceSteps} instructions (${(nativeSteps / referenceSteps * 100).toFixed(1)}%)`);
});

it.skipIf(FX3 || !process.env.OL_REFERENCE_ROM)('retains the original GSU-2 bear-only narrow drop for every other item type', () => {
    const pair = [movementCartridge(), movementCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!))];
    const actors = pair.map(groundFixture);
    let count = 0;
    for (const type of [0,1,7,8,9,12,14,19,34,255]) {
        const wideDrop = type !== 8;
        for (const [floor, allowed] of [[1792,true], [1536,false], [2304,true], [2560,wideDrop],
            [3072,wideDrop], [3328,false], [-32512,false]] as const) {
            const snapshots: Buffer[] = [];
            for (const [index, g] of pair.entries()) {
                const actor = actors[index];
                g.set({ olCreatureCount: 0 });
                for (let x = 0; x < 8; x++) for (let z = 3; z < 8; z++) g.sector(0, x, z, floor, -2048);
                actor.pose(3); g.w8(actor.item + ITEM.type, type); g.w8(actor.item + ITEM.room, 0);
                g.w8(actor.item + ITEM.flags2, 1); g.w16(actor.item + ITEM.health, 6);
                g.w16(actor.item + ITEM.x, 2560); g.w16(actor.item + ITEM.y, 2048); g.w16(actor.item + ITEM.z, 3071);
                g.w16(actor.item + ITEM.angleY, 0); g.w16(actor.item + ITEM.hSpeed, 0); g.w16(actor.item + ITEM.vSpeed, 0);
                g.w16(actor.item + ITEM.roomFloor, -1234); resetSectorCache(g);
                g.call('Enemy_moveGround', [...actor.args, 100]);
                expect(g.s16(actor.item + ITEM.z) > 3071, `${index ? 'C' : 'native'} type ${type} floor ${floor}`).toBe(allowed);
                expect(g.u16(actor.item + ITEM.angleY)).toBe(allowed ? 100 : 200);
                expect(g.s16(actor.item + ITEM.y)).toBe(allowed ? floor < 2048 ? floor : 2112 : 2048);
                snapshots.push(snapshot(g, actor));
            }
            expect(snapshots[0], `GSU-2 type ${type} floor ${floor}`).toEqual(snapshots[1]); count++;
        }
    }
    console.log(`[openlara] GSU-2 ground drop rules: ${count} exact C full-item/next-state snapshots; ` +
        'bear alone uses256, all other type bytes use1024; 256-up/256-or1024-down boundaries');
});

it.skipIf(!process.env.OL_REFERENCE_ROM)('uses complete local-sector navigation when room origins are unaligned', () => {
    const pair = [movementCartridge(), movementCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!))];
    const actors = pair.map(groundFixture);
    const types = FX3 ? [7,8,12,13,14,19] : [0,1,7,8,9,12,14,19,34,255];
    let count = 0, accepted = 0, blocked = 0;
    for (const type of types) for (const axis of ['x', 'z'] as const)
    for (const origin of [511,-511,1023]) for (const connected of [false,true]) {
        const snapshots: Buffer[] = [];
        // Begin exactly on a LOCAL sector edge and move backwards over it.
        // These offsets keep both endpoints in the same WORLD 1024-unit tile.
        const boundary = origin + 3 * 1024;
        for (const [index, g] of pair.entries()) {
            const actor = actors[index];
            g.set({ olRoomCount: 1, olBoxCount: 2, olCreatureCount: 0, olDoorCount: 0, olBridgeCount: 0 });
            g.room(0, false);
            g.w16(g.table('Room') + (axis === 'x' ? 2 : 4), origin);
            for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++) g.sector(0, x, z, 2048, -2048);
            for (let b = 0; b < 2; b++) {
                const box = g.table('Box') + b * 8;
                [0,8,0,8].forEach((v, i) => g.w8(box + i, v));
                g.w16(box + 4, 2048); g.w16(box + 6, 0);
            }
            g.navMap(Array.from({ length: 64 }, (_, i) => (axis === 'x' ? i >>> 3 : i & 7) >= 3 ? 1 : 0));
            // Flying zones agree in both cases, so a wrong map cannot make
            // the disconnected ground movement pass the rollback oracle.
            g.zoneMap([[0, connected ? 0 : 1], [5,5]]);
            actor.pose(3); g.w8(actor.item + ITEM.type, type); g.w8(actor.item + ITEM.room, 0);
            g.w8(actor.item + ITEM.flags2, 1); g.w16(actor.item + ITEM.health, 6);
            g.w16(actor.item + ITEM.x, axis === 'x' ? boundary : 2560);
            g.w16(actor.item + ITEM.y, 2048); g.w16(actor.item + ITEM.z, axis === 'z' ? boundary : 2560);
            g.w16(actor.item + ITEM.angleY, axis === 'x' ? 49152 : 32768);
            // Type zero uses Lara's separate movement angle in ItemObj_move.
            g.set({ olLaraMoveAngle: axis === 'x' ? 49152 : 32768 });
            g.w16(actor.item + ITEM.hSpeed, 100); g.w16(actor.item + ITEM.vSpeed, 0);
            g.w16(actor.item + ITEM.roomFloor, -1234); resetSectorCache(g);
            let lookups = 0;
            g.call('Enemy_moveGround', [...actor.args, 100], index === 0 ?
                { name: 'EG_LookupBox', run: () => lookups++ } : undefined);
            const position = g.s16(actor.item + ITEM[axis]);
            expect(position < boundary, `${index ? 'C' : 'native'} ${type}/${axis}/${origin} connected=${connected}`).toBe(connected);
            expect((position ^ boundary) & 0xFC00, 'the fixture accidentally crosses a world tile').toBe(0);
            expect((position - origin) >> 10, 'local-sector crossing or rollback').toBe(connected ? 2 : 3);
            const yaw = axis === 'x' ? 49152 : 32768;
            expect(g.u16(actor.item + ITEM.angleY), `turn ${index ? 'C' : 'native'} ${type}/${axis}/${origin} connected=${connected}`)
                .toBe((yaw + (connected ? 100 : 200)) & 65535);
            if (index === 0) expect(lookups, 'an unaligned origin bypassed complete box/zone lookup').toBe(1);
            snapshots.push(snapshot(g, actor));
        }
        expect(snapshots[0], `unaligned origin ${type}/${axis}/${origin} connected=${connected}`).toEqual(snapshots[1]);
        connected ? accepted++ : blocked++; count++;
    }
    console.log(`[openlara] ${FX3 ? 'FX3' : 'GSU-2'} unaligned room origins: ${count} exact C full-item/next-state snapshots; ` +
        `${accepted} accepted local-sector crossings / ${blocked} zone rollbacks within unchanged world tiles; all use full lookup`);
});

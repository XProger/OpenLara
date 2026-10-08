/**
 * Call the cartridge's trigger code in TypeSNES over synthetic packed records.
 * Run beside TypeSNES's emulator.ts, with OL_ROM, OL_SYMS and OL_FX3=0/1.
 * No boot, source-code test doubles, or ROM writes are needed. The production
 * trigger table pointer is rebound to writable fixture data in WRAM/SRAM.
 */
import { describe, expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

const FX3 = process.env.OL_FX3 === '1';
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
const ITEM_SIZE = 32;
const ITEM = { type: 0, x: 4, y: 6, z: 8, flags: 15, health: 16, timer: 18,
    flags2: 24, nextActive: 25, roomFloor: 26, vSpeed: 28 };
const TRIGGER = { ACTIVATE: 0, PAD: 1, ANTIPAD: 6, COMBAT: 7, OBJECT: 5, DUMMY: 8 };
const ACTION = { END: 1, SECRET: 2, CAMERA: 4, TARGET: 8 };
const FLAG = { MASK: 0x1F, ONCE: 0x20, ACTIVE: 0x40, DRIVEN: 0x80 };
const WEAPON = { FREE: 0, BUSY: 1, DRAW: 2, HOLSTER: 3, READY: 4 };
const MODE = { FOLLOW: 0, COMBAT: 1, FIXED: 3, OBJECT: 4 };

function harness() {
    const rom = new Uint8Array(readFileSync(process.env.OL_ROM!));
    const emulator = new SNESEmulator();
    emulator.loadROM(rom);
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    if (!FX3) {
        // The same five code images main.asm stages before executing GSU-2.
        for (const [bank, dest, size] of [[63, 0x7E8000, 0x8000], [62, 0x7E5800, 0x2800],
            [61, 0x7E0600, 0x1600], [4, 0x7F0000, 0x8000], [5, 0x7F8000, 0x8000]])
            for (let i = 0; i < size; i++) memory.write(dest + i, rom[bank * 0x8000 + i]);
    }
    const w16 = (at: number, value: number) => {
        memory.write(at, value & 255); memory.write(at + 1, (value >>> 8) & 255);
    };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    const set = (name: string, value: number) => w16(sym(name), value);
    const get = (name: string) => u16(sym(name));
    const call = (name: string, words: number[] = []) => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        w16(0x1EE1, 0x04FF); memory.write(0x1EE3, 0x7E);
        words.forEach((word, i) => w16(0x1EE4 + i * 2, word));
        const at = sym(name);
        cpu.PB = at >>> 16; cpu.PC = at & 65535;
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 100000) cpu.step();
        expect(steps, `${name} did not return`).toBeLessThan(100000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} corrupted the C ABI`)
            .toEqual([0x1EE3, 0x7E, 0, 0, 4]);
    };
    const table = FX3 ? 0x7F1000 : 0x710100;
    const item = (slot: number, offset: number) => sym('items') + slot * ITEM_SIZE + offset;
    const bytes = (slot: number) => Array.from({ length: ITEM_SIZE }, (_, i) => memory.read(item(slot, i)));
    const trigger = (type = TRIGGER.COMBAT, mask = FLAG.MASK, timer = 2, once = 0,
                     actions = ACTION.END | ACTION.SECRET, slots = [1]) => {
        for (let i = 0; i < 16; i++) memory.write(table + i, 0);
        [type, slots.length, mask, once, timer, actions, 3].forEach((v, i) => memory.write(table + i, v));
        slots.forEach((v, i) => memory.write(table + 8 + i, v));
    };
    set('itemsCount', 3); set('olTriggerCount', 1); set('olFirstActive', 255);
    set('olLevelEnd', 0); set('olSecrets', 0); set('olLaraWeaponState', WEAPON.FREE);
    if (FX3) {
        w16(sym('olTriggerData'), table & 65535); w16(sym('olTriggerData') + 2, table >>> 16);
        const level = sym('olLevel'), cameras = 0x7F2000;
        for (let i = 0; i < 64; i++) memory.write(level + i, 0);
        w16(level + 8, cameras & 65535); w16(level + 10, cameras >>> 16); w16(level + 12, 16);
        w16(cameras, 1); w16(cameras + 2, 1);
        for (let i = 4; i < 16; i++) memory.write(cameras + i, 0);
        memory.write(cameras + 12, 255); memory.write(cameras + 15, 255);
        call('Camera_levelInit'); set('olLaraLooking', 0); set('olTrigLate', 0);
        w16(sym('camera') + 18, MODE.FOLLOW);
    } else set('olTriggerBase', table & 65535);
    for (let slot = 0; slot < 3; slot++) {
        for (let i = 0; i < ITEM_SIZE; i++) memory.write(item(slot, i), 0);
        // Door types avoid unrelated creature navigation during activation.
        memory.write(item(slot, ITEM.type), slot ? 57 : 0);
        memory.write(item(slot, ITEM.flags), slot ? FLAG.DRIVEN : FLAG.ACTIVE);
        memory.write(item(slot, ITEM.nextActive), 255);
        w16(item(slot, ITEM.health), 1000); w16(item(slot, ITEM.timer), 123);
    }
    trigger();
    const cameraRecord = (cam = 0, timer = 2, once = 1, target = 255) => {
        [cam, timer, once, target].forEach((v, i) => memory.write(0x7F200C + i, v));
    };
    const fallingBlock = () => {
        const data = 0x7F3000;
        const bind = (name: string, offset: number) => {
            w16(sym(`ol${name}Data`), (data + offset) & 65535);
            w16(sym(`ol${name}Data`) + 2, data >>> 16);
        };
        for (const [name, offset] of [['Room', 0x40], ['Sector', 0x100], ['Link', 0x200],
            ['Anim', 0x240], ['AnimSpeed', 0x260]] as const) bind(name, offset);
        for (let i = 0; i < 0x280; i++) memory.write(data + i, 0);
        set('olRoomCount', 1); set('olSectorCount', 16);
        memory.write(data + 0x48, 4); memory.write(data + 0x49, 4);
        for (let i = 0; i < 16; i++) memory.write(data + 0x100 + i * 4, 8);
        for (let i = 0; i < 4; i++) {
            memory.write(data + 0x200 + i, 255);
            memory.write(data + 0x208 + i, i === 3 ? 0 : 255);
        }
        const sector = data + 0x100 + 5 * 4;
        w16(sector + 2, 1); // sector (1, 1) names heavy trigger zero
        memory.write(data + 0x240, 100); // idle animation, no motion or commands
        memory.write(data + 0x241, 1);
        memory.write(item(2, ITEM.type), 48); // TR1's first movable block
        memory.write(item(2, ITEM.flags), FLAG.ACTIVE);
        memory.write(item(2, ITEM.flags2), 1);
        w16(item(2, ITEM.x), 1536); w16(item(2, ITEM.z), 1536);
        w16(item(2, ITEM.y), 1792); set('olFirstActive', 2);
        call('Room_reset');
        const tick = () => call('Block_update', [item(2, 0) & 65535, item(2, 0) >>> 16]);
        return { sector, tick };
    };
    return { memory, item, bytes, table, w16, u16, set, get, call, trigger, cameraRecord, fallingBlock };
}

describe.skipIf(!FX3)('OpenLara SNES: GBA combat trigger parity', () => {
    it('gates the entire action set on READY, including drawing and holstering', () => {
        const h = harness();
        for (const state of [WEAPON.FREE, WEAPON.BUSY, WEAPON.DRAW, WEAPON.HOLSTER, 5, 65535]) {
            h.set('olLaraWeaponState', state);
            const before = h.bytes(1);
            h.call('checkTrigger', [0]);
            expect(h.bytes(1), `weapon state ${state} activated an object`).toEqual(before);
            expect(h.get('olFirstActive')).toBe(255);
            expect(h.get('olLevelEnd')).toBe(0); expect(h.get('olSecrets')).toBe(0);
        }
        h.set('olLaraWeaponState', WEAPON.READY);
        // COMBAT does not inherit PAD's ground contact requirement.
        h.w16(h.item(0, ITEM.y), -256); h.w16(h.item(0, ITEM.roomFloor), 1024);
        h.call('checkTrigger', [0]);
        expect(h.memory.read(h.item(1, ITEM.flags))).toBe(FLAG.DRIVEN | FLAG.MASK | FLAG.ACTIVE);
        expect(h.memory.read(h.item(1, ITEM.flags2)) & 3).toBe(1);
        expect(h.u16(h.item(1, ITEM.timer))).toBe(60);
        expect(h.get('olFirstActive')).toBe(1);
        expect(h.memory.read(h.item(1, ITEM.nextActive))).toBe(255);
        expect(h.memory.read(sym('olItemDirty') + 1)).toBe(1);
        expect(h.get('olLevelEnd')).toBe(1); expect(h.get('olSecrets')).toBe(1 << 3);
    });

    it('retains mask accumulation, once latching, and the special one-tick timer', () => {
        const h = harness();
        h.set('olLaraWeaponState', WEAPON.READY);
        h.memory.write(h.item(1, ITEM.flags2), 3); // invisible before activation
        h.trigger(TRIGGER.COMBAT, 3, 3, 1);
        h.call('checkTrigger', [0]);
        expect(h.memory.read(h.item(1, ITEM.flags))).toBe(FLAG.DRIVEN | 3);
        expect(h.u16(h.item(1, ITEM.timer))).toBe(90);
        expect(h.get('olFirstActive')).toBe(255);
        h.trigger(TRIGGER.COMBAT, 28, 1, 1);
        h.call('checkTrigger', [0]);
        expect(h.memory.read(h.item(1, ITEM.flags))).toBe(255);
        expect(h.memory.read(h.item(1, ITEM.flags2)) & 3).toBe(1);
        expect(h.u16(h.item(1, ITEM.timer))).toBe(1);
        h.w16(h.item(1, ITEM.timer), 77);
        h.call('checkTrigger', [0]);
        expect(h.u16(h.item(1, ITEM.timer)), 'ONCE must skip timer refresh').toBe(77);
    });

    it('refreshes repeatable active timers without duplicating the active list', () => {
        const h = harness();
        h.set('olLaraWeaponState', WEAPON.READY);
        h.trigger(TRIGGER.COMBAT, FLAG.MASK, 255, 0, 0, [1, 255, 2]);
        h.call('checkTrigger', [0]);
        expect(h.get('olFirstActive')).toBe(2);
        expect(h.memory.read(h.item(2, ITEM.nextActive))).toBe(1);
        h.w16(h.item(1, ITEM.timer), 4); h.w16(h.item(2, ITEM.timer), 5);
        h.call('checkTrigger', [0]);
        expect(h.u16(h.item(1, ITEM.timer))).toBe(7650);
        expect(h.u16(h.item(2, ITEM.timer))).toBe(7650);
        expect(h.get('olFirstActive')).toBe(2);
        expect(h.memory.read(h.item(2, ITEM.nextActive))).toBe(1);
        expect(h.memory.read(h.item(1, ITEM.nextActive))).toBe(255);
        const before = h.bytes(1);
        h.call('checkTrigger', [1]);
        expect(h.bytes(1), 'out-of-range trigger must be ignored').toEqual(before);
    });

    it('preserves normal, grounded pad, antipad and inert trigger behavior', () => {
        const h = harness();
        h.w16(h.item(0, ITEM.y), 1);
        for (const type of [TRIGGER.PAD, TRIGGER.ANTIPAD, TRIGGER.OBJECT, TRIGGER.DUMMY]) {
            h.trigger(type);
            h.call('checkTrigger', [0]);
            expect(h.get('olLevelEnd')).toBe(0);
            expect(h.get('olFirstActive')).toBe(255);
        }
        h.trigger(TRIGGER.ACTIVATE);
        h.call('checkTrigger', [0]);
        expect(h.get('olFirstActive')).toBe(1);
        h.w16(h.item(0, ITEM.y), 0);
        h.trigger(TRIGGER.ANTIPAD, 3, 1, 0, 0);
        h.call('checkTrigger', [0]);
        expect(h.memory.read(h.item(1, ITEM.flags))).toBe(FLAG.DRIVEN | FLAG.ACTIVE | 28);
        expect(h.get('olFirstActive'), 'ANTIPAD must retain active-list membership').toBe(1);
    });

    it('checks the late combat trigger after real gameUpdate weapon transitions', () => {
        for (const drawing of [true, false]) {
            const h = harness();
            h.fallingBlock(); // supplies an empty dry room and an idle animation
            // This scene uses Lara's STOP state over the same sector trigger.
            h.memory.write(0x7F3249, 2);
            for (const [name, at] of [['Model', 0x7F3280], ['AnimBox', 0x7F3290],
                ['StaticFirst', 0x7F32A0]] as const) {
                h.w16(sym(`ol${name}Data`), at & 65535);
                h.w16(sym(`ol${name}Data`) + 2, at >>> 16);
                for (let i = 0; i < 16; i++) h.memory.write(at + i, 0);
            }
            [-128, 128, -768, 0, -128, 128].forEach((v, i) => h.w16(0x7F3290 + i * 2, v));
            h.memory.write(h.item(0, 2), 2); h.memory.write(h.item(0, 3), 2);
            h.w16(h.item(0, ITEM.x), 1536); h.w16(h.item(0, ITEM.y), 2048);
            h.w16(h.item(0, ITEM.z), 1536); h.w16(h.item(0, ITEM.roomFloor), 2048);
            h.memory.write(h.item(2, ITEM.flags), 0);
            h.memory.write(h.item(2, ITEM.flags2), 3);
            h.set('olFirstActive', 255);
            // Freeze other objects so their update cannot consume the action's
            // timer before the assertion; Lara and her weapons still update.
            h.set('ol_animFreeze', 1);
            h.call('Weapon_init');
            h.set('olLaraWeaponState', drawing ? WEAPON.DRAW : WEAPON.READY);
            for (const off of [0, 12]) {
                [drawing ? 2 : 0, drawing ? 10 : 4, 0, 0, 1, 1].forEach((v, i) =>
                    h.w16(sym('olWeaponArms') + off + i * 2, v));
            }
            h.trigger(TRIGGER.COMBAT, FLAG.MASK, 2, 0, ACTION.SECRET);
            h.set('ol_keys', drawing ? 0 : 0x80); // R requests holster
            h.call('Camera_snap', [h.item(0, 0) & 65535, h.item(0, 0) >>> 16]);
            h.call('gameUpdate');
            expect(h.get('ol_ticks'), 'the complete game tick must publish its result').toBe(1);
            expect(h.memory.read(h.item(0, 2))).toBe(2);
            expect(h.u16(h.item(0, ITEM.y))).toBe(2048);
            expect(h.get('ol_trigger'), 'Lara left the combat-trigger sector').toBe(0);
            expect(h.get('olLaraWeaponState')).toBe(drawing ? WEAPON.READY : WEAPON.HOLSTER);
            expect(h.get('olSecrets'), drawing ? 'newly ready guns did not fire the trigger' :
                'holstering guns fired the late trigger').toBe(drawing ? 1 << 3 : 0);
            expect(h.memory.read(h.item(1, ITEM.flags))).toBe(drawing ? FLAG.DRIVEN | FLAG.ACTIVE | FLAG.MASK : FLAG.DRIVEN);
            expect(h.u16(h.item(1, ITEM.timer))).toBe(drawing ? 60 : 123);
        }
    });
});

describe.skipIf(!FX3)('OpenLara FX3: GBA heavy trigger parity', () => {
    it('routes only OBJECT triggers to objects and excludes them from Lara', () => {
        const h = harness();
        h.set('olLaraWeaponState', WEAPON.READY);
        for (const type of [TRIGGER.ACTIVATE, TRIGGER.PAD, 2, 3, 4, TRIGGER.ANTIPAD,
            TRIGGER.COMBAT, TRIGGER.DUMMY]) {
            h.trigger(type);
            h.call('checkTriggerObject', [0]);
            expect(h.get('olFirstActive'), `object fired trigger type ${type}`).toBe(255);
            expect(h.get('olLevelEnd')).toBe(0); expect(h.get('olSecrets')).toBe(0);
        }
        h.trigger(TRIGGER.OBJECT);
        h.call('checkTrigger', [0]);
        expect(h.get('olFirstActive'), 'Lara fired a heavy trigger').toBe(255);
        h.set('olLaraWeaponState', WEAPON.FREE);
        h.w16(h.item(0, ITEM.y), -256); h.w16(h.item(0, ITEM.roomFloor), 1024);
        h.call('checkTriggerObject', [0]);
        expect(h.get('olFirstActive')).toBe(1);
        expect(h.u16(h.item(1, ITEM.timer))).toBe(60);
        expect(h.get('olLevelEnd')).toBe(1); expect(h.get('olSecrets')).toBe(1 << 3);
    });

    it('shares partial masks, repeatable timers and once latches with Lara triggers', () => {
        const h = harness();
        h.trigger(TRIGGER.OBJECT, 3, 3, 1);
        h.call('checkTriggerObject', [0]);
        expect(h.memory.read(h.item(1, ITEM.flags))).toBe(FLAG.DRIVEN | 3);
        expect(h.u16(h.item(1, ITEM.timer))).toBe(90);
        expect(h.get('olFirstActive')).toBe(255);
        h.trigger(TRIGGER.OBJECT, 28, 1, 0);
        h.call('checkTriggerObject', [0]);
        expect(h.memory.read(h.item(1, ITEM.flags))).toBe(FLAG.DRIVEN | FLAG.ACTIVE | FLAG.MASK);
        expect(h.u16(h.item(1, ITEM.timer))).toBe(1);
        h.trigger(TRIGGER.OBJECT, FLAG.MASK, 2, 1);
        h.call('checkTriggerObject', [0]);
        expect(h.memory.read(h.item(1, ITEM.flags))).toBe(255);
        expect(h.u16(h.item(1, ITEM.timer))).toBe(60);
        h.w16(h.item(1, ITEM.timer), 77);
        h.call('checkTriggerObject', [0]);
        expect(h.u16(h.item(1, ITEM.timer))).toBe(77);
    });

    it('fires from a real landed block after restoring its floor, never while airborne', () => {
        for (const type of [TRIGGER.OBJECT, TRIGGER.ACTIVATE]) {
            const h = harness();
            h.trigger(type);
            const { sector, tick } = h.fallingBlock();
            tick();
            expect(h.memory.read(h.item(2, ITEM.flags2)) & 4, 'unsupported block did not fall').toBe(4);
            expect(h.memory.read(h.item(1, ITEM.flags))).toBe(FLAG.DRIVEN);
            expect(h.get('olLevelEnd')).toBe(0);
            let ticks = 1;
            while ((h.memory.read(h.item(2, ITEM.flags)) & FLAG.ACTIVE) && ticks++ < 100) tick();
            expect(ticks, 'falling block did not land').toBeLessThan(100);
            expect(h.u16(h.item(2, ITEM.y))).toBe(2048);
            expect(h.u16(h.item(2, ITEM.vSpeed))).toBe(0);
            expect(h.memory.read(h.item(2, ITEM.flags2)) & 7).toBe(0);
            expect(h.memory.read(sector), 'landing did not restore the block floor').toBe(4);
            expect(h.get('olFirstActive')).toBe(type === TRIGGER.OBJECT ? 1 : 255);
            expect(h.get('olLevelEnd')).toBe(type === TRIGGER.OBJECT ? 1 : 0);
            expect(h.get('olSecrets')).toBe(type === TRIGGER.OBJECT ? 1 << 3 : 0);
            if (type === TRIGGER.OBJECT) expect(h.u16(h.item(1, ITEM.timer))).toBe(60);
        }
    });
});

describe.skipIf(!FX3)('OpenLara FX3: combat camera trigger ordering', () => {
    it('runs the camera precheck even when the combat action gate rejects', () => {
        const h = harness();
        h.trigger(TRIGGER.COMBAT, FLAG.MASK, 2, 0, ACTION.CAMERA | ACTION.TARGET);
        h.cameraRecord(0, 2, 1, 2);
        h.set('olCamLastIndex', 0); h.set('olCamTimer', 5);
        h.call('checkTrigger', [0]);
        expect(h.get('olFirstActive')).toBe(255);
        h.call('Camera_prepare');
        expect(h.u16(sym('camera') + 18)).toBe(MODE.FIXED);
        expect(h.get('olCamIndex')).toBe(0);
        expect(h.get('olCamTimer'), 'rejected action must not restart the timer').toBe(5);
        expect(h.get('olCamLookAt')).toBe(2);
        expect(h.memory.read(sym('olCamOnce'))).toBe(0);
    });

    it('does not arm a new fixed camera if READY becomes HOLSTER before replay', () => {
        const h = harness();
        h.trigger(TRIGGER.COMBAT, FLAG.MASK, 2, 0, ACTION.CAMERA);
        h.cameraRecord();
        h.set('olLaraWeaponState', WEAPON.READY);
        h.call('checkTrigger', [0]);
        expect(h.get('olFirstActive')).toBe(1);
        h.set('olLaraWeaponState', WEAPON.HOLSTER);
        h.call('Camera_prepare');
        expect(h.get('olCamIndex'), 'GBA records the camera index before suppressing COMBAT').toBe(0);
        expect(h.u16(sym('camera') + 18)).toBe(MODE.FOLLOW);
        expect(h.get('olCamTimer')).toBe(0);
        expect(h.memory.read(sym('olCamOnce'))).toBe(0);
    });

    it('applies the late combat mode before its camera precheck and actions', () => {
        const h = harness();
        h.trigger(TRIGGER.COMBAT, FLAG.MASK, 2, 0, ACTION.CAMERA | ACTION.TARGET);
        h.cameraRecord(0, 2, 1, 2);
        h.set('olCamLastIndex', 0); h.set('olCamTimer', 5);
        h.set('olLaraWeaponState', WEAPON.READY); h.set('olTrigLate', 1);
        h.call('checkTrigger', [0]); h.call('Camera_prepare');
        expect(h.u16(sym('camera') + 18)).toBe(MODE.COMBAT);
        expect(h.get('olCamTimer')).toBe(65535);
        expect(h.get('olCamLookAt')).toBe(255);
        expect(h.memory.read(sym('olCamOnce'))).toBe(0);
    });

    it('still arms normal fixed camera actions and latches their once flag', () => {
        const h = harness();
        h.trigger(TRIGGER.ACTIVATE, FLAG.MASK, 2, 0, ACTION.CAMERA | ACTION.TARGET);
        h.cameraRecord(0, 2, 1, 2);
        h.call('checkTrigger', [0]); h.call('Camera_prepare');
        expect(h.u16(sym('camera') + 18)).toBe(MODE.FIXED);
        expect(h.get('olCamTimer')).toBe(60);
        expect(h.get('olCamLookAt')).toBe(2);
        expect(h.memory.read(sym('olCamOnce'))).toBe(1);
    });

    it('retains heavy camera events after both Lara checks and enters OBJECT mode', () => {
        const h = harness();
        h.trigger(TRIGGER.DUMMY, FLAG.MASK, 2, 0, ACTION.CAMERA | ACTION.TARGET);
        h.cameraRecord(0, 2, 1, 2);
        h.call('checkTrigger', [0]); h.set('olTrigLate', 1); h.call('checkTrigger', [0]);
        h.trigger(TRIGGER.OBJECT, FLAG.MASK, 2, 0, ACTION.CAMERA | ACTION.TARGET);
        h.call('checkTriggerObject', [0]); h.call('Camera_prepare');
        expect(h.u16(sym('camera') + 18)).toBe(MODE.OBJECT);
        expect(h.get('olCamTimer')).toBe(60);
        expect(h.get('olCamLookAt')).toBe(2);
        expect(h.memory.read(sym('olCamOnce'))).toBe(1);
        h.call('Camera_tail', [1]);
        expect(h.u16(sym('camera') + 18), 'OBJECT mode should survive the camera tail').toBe(MODE.OBJECT);
    });

    it('skips the camera precheck while an object camera holds, even for rejected types', () => {
        const h = harness();
        h.trigger(TRIGGER.ACTIVATE, FLAG.MASK, 2, 0, ACTION.CAMERA | ACTION.TARGET);
        h.cameraRecord(0, 2, 1, 2);
        h.w16(sym('camera') + 18, MODE.OBJECT); h.set('olCamIndex', 0);
        h.set('olCamLastIndex', 0); h.set('olCamTimer', 5); h.set('olCamLookAt', 1);
        h.call('checkTriggerObject', [0]); h.call('Camera_prepare');
        expect(h.u16(sym('camera') + 18)).toBe(MODE.OBJECT);
        expect(h.get('olCamTimer')).toBe(5);
        expect(h.get('olCamLookAt')).toBe(1);
    });
});

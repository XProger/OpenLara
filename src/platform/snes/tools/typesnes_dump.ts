/**
 * Boot the built ROM in TypeSNES headless and dump the framebuffer as a PNG.
 *
 * TypeSNES lives in its own repository and its emulator core is TypeScript, so
 * this runs with that project's installed vitest in a disposable source tree.
 * `make check` uses tools/run_typesnes.py; the checkout stays unchanged.
 *
 * Environment:
 *   OL_ROM           path to the .sfc to boot
 *   OL_EXPECT        raw frame-buffer bytes produced by tools/gsusim.py
 *   OL_EXPECT_SHADE  its shade map, i.e. the tile map high bytes (optional)
 *   OL_FRAMES        how many SNES fields to run (default 30)
 *   OL_WALK          fields to spend walking in the second test (0 skips it)
 *   OL_TICK          the animation frame to pin models to, matching `make sim`
 *   OL_MODE7         1 for the MODE7=1 build: the frame is 8 KB of Mode 7
 *                    character data in VRAM's high bytes, not 20 KB of 4bpp
 *                    characters, and there is no shade map
 *   OL_LEVEL         which level the ROM carries (LEVEL1). The scenes are
 *                    level-specific by coordinate, so each runs on its own
 *                    level and skips on any other.
 *   OL_SYMS          the link's symbol file, for locating src/game.c's globals
 *   OL_OUT           where to write the PNG
 *   OL_PROFILE_TICK_TRACE  FX3: hash item/navigation/camera/input state after each tick
 *   OL_PROFILE_REFERENCE_TRACE  compare those hashes with a baseline profile
 *   OL_PROFILE_INPUT pulses  replay short pad taps during the profile
 *   OL_PROFILE_INPUT_START first  hold the pad neutral until this profile field (default 1)
 *   OL_PROFILE_FOCUS first:last  add read-only costs and frame handoffs for these fields
 */
import { describe, it, expect } from 'vitest';
import { readFileSync, writeFileSync, existsSync, readdirSync } from 'fs';
import { deflateSync, crc32 } from 'zlib';
import { dirname, join } from 'path';
import { execFileSync } from 'child_process';
import { createHash } from 'crypto';
import { SNESEmulator } from './emulator';
import { TIMING, REGION_NTSC } from './core/types';

const ROM = process.env.OL_ROM ?? '';
// The level's packed data: beside the ROM, or a multi-level cartridge's own
// directory for the level under test.
const DATA_DIR = process.env.OL_DATA ?? join(dirname(ROM), 'data');
const EXPECT = process.env.OL_EXPECT ?? '';
const EXPECT_SHADE = process.env.OL_EXPECT_SHADE ?? '';
const FRAMES = Number(process.env.OL_FRAMES ?? 30);
const WALK = Number(process.env.OL_WALK ?? 900);
// The animation frame the models are pinned to, matching `make sim TICK=`.
const TICK = Number(process.env.OL_TICK ?? 0);
const LEVEL = process.env.OL_LEVEL ?? 'LEVEL1';
const MODE7 = process.env.OL_MODE7 === '1';
const FX3 = process.env.OL_FX3 === '1';
// FX3 has independent concurrent ROM access; no engine image belongs in
// WRAM. GSU-2 must copy all five before handing its ROM bus to the GSU.
const WRAM_CODE_IMAGES = FX3 ? [] : [[63, 0x7E8000, 0x8000], [62, 0x7E5800, 0x2800],
    [61, 0x7E0600, 0x1600], [4, 0x7F0000, 0x8000], [5, 0x7F8000, 0x8000]];

const OUT = process.env.OL_OUT ?? 'frame.png';

// The title screen the packer made (packer writeTitle): 64 colours, then
// 216 x 144 CG-RAM indices. Empty when the pack found no TITLEH.PCX.
const TITLE_BIN = FX3 && existsSync(join(DATA_DIR, 'TITLE.BIN')) ? readFileSync(join(DATA_DIR, 'TITLE.BIN')) : Buffer.alloc(0);

const VRAM_TILEMAP = 0xc000;    // word $6000, as a byte address

// src/game.c's globals, located by name rather than by address. They used to
// be constants here, and adding a variable to the C silently moved every one
// of them -- the test still ran, against whatever happened to be at the old
// offsets, and reported nonsense with confidence.
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
const SYMS = readSymbols(process.env.OL_SYMS ?? '');

const HOST_COUNTS = ['items', 'models', 'anims', 'trans', 'triggers', 'cmds',
    'boxes', 'overlaps', 'rooms', 'sectors', 'links', 'statics'];
const HOST_TABLES = ['Room', 'Sector', 'Link', 'Model', 'AnimBox', 'AnimSpeed',
    'Anim', 'Trans', 'Cmd', 'Trigger', 'Item', 'Box', 'Overlap', 'Zone', 'Static',
    'StaticFirst', 'AnimAim', 'NavMap'];

// Explicit test-only ROM patching. CPU memory.write still refuses these
// writes, including while the FX is running; never weaken the bus model to
// accommodate fixtures that used to edit a WRAM staging copy.
function fixtureWrite(memory: ReturnType<SNESEmulator['getMemory']>, address: number, value: number) {
    if (FX3 && address >= 0xE00000 && address <= 0xFFFFFF)
        (memory as unknown as { rom: Uint8Array }).rom[address - 0xC00000] = value & 255;
    else memory.write(address, value & 255);
}

// Explicit code patching for isolated oracles only. The normal CPU write
// path remains read-only: a silently rejected store must never stand in for
// a relocated operand or an RTL test double. FX3's upper LoROM windows and
// full C0..FF CPU map both refer to the same cartridge image.
function fixtureCodeByte(memory: ReturnType<SNESEmulator['getMemory']>, address: number, value: number) {
    const bank = address >>> 16, low = address & 65535;
    const offset = bank >= 0xC0 && bank <= 0xFF ? address - 0xC00000 :
        bank >= 0x80 && bank <= 0xBF && low >= 0x8000 ? (bank - 0x80) * 0x8000 + low - 0x8000 : -1;
    const rom = (memory as unknown as { rom: Uint8Array }).rom;
    if (!FX3 || offset < 0 || offset >= rom.length) throw new Error(`invalid FX3 code fixture ${address.toString(16)}`);
    rom[offset] = value & 255;
    expect(memory.read(address), `code fixture did not take effect at ${address.toString(16)}`).toBe(value & 255);
}

function hostTable(memory: ReturnType<SNESEmulator['getMemory']>, symbol: (name: string) => number, name: string) {
    const word = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    if (FX3) {
        const at = symbol(`ol${name}Data`);
        return word(at) | word(at + 2) << 16;
    }
    return 0x710000 + (name === 'NavMap' ? word(0x710000 + word(0x710000) - 2) : word(symbol(`ol${name}Base`)));
}

// The level is up once the host has parsed its entity list. The boot is not
// a fixed number of fields: the sound upload through the IPL protocol takes
// 1.3 s for LEVEL1's image, then the level's staging. Scenes that used to
// settle for 120 fields from power-on read an empty entity list once the
// upload went in.
// The level a multi-level cartridge is asked to boot for these scenes (its
// index in the cartridge's level table); 0 for a single-level build.
const BOOT_LEVEL = Number(process.env.OL_BOOT_LEVEL ?? 0);
const LEVEL_ENTRY = 64;
// Every FX3 cartridge a scene loads boots that level: main.asm SelectLevel's
// request in the battery RAM ("OLBR", the level, a plain start), as a
// restart would leave it. A power-on without one opens the title page (the
// title scene sets COLD_BOOT around its loadROM). And settings.c's
// settings, as a player who turned the frame counter off would have left
// them ("OLST", version 1: rumble, swap, SFX, music, gamma, FPS, vsync), so
// that no scene's picture carries drawFPS's digits; the settings scene sets
// OWN_SETTINGS for upstream's defaults.
let COLD_BOOT = false, OWN_SETTINGS = false;
const SETTINGS_AT = 0x3110;
if (FX3) {
    const load = SNESEmulator.prototype.loadROM;
    SNESEmulator.prototype.loadROM = function (this: SNESEmulator, data: Uint8Array) {
        const result = load.call(this, data);
        const memory = this.getMemory() as unknown as { writeCartridgeRAM(o: number, v: number): void };
        if (!COLD_BOOT)
            [0x4F, 0x4C, 0x42, 0x52, BOOT_LEVEL & 255, BOOT_LEVEL >> 8, 0, 0]
                .forEach((v, i) => memory.writeCartridgeRAM(0x3100 + i, v));
        if (!OWN_SETTINGS)
            [0x4F, 0x4C, 0x53, 0x54, 1, 1, 0, 1, 1, 0, 0, 0]
                .forEach((v, i) => memory.writeCartridgeRAM(SETTINGS_AT + i, v));
        return result;
    };
}

// The FX3 frame on screen as the GSU drew it: 216 x 144 CG-RAM indices out
// of the 8bpp characters in VRAM -- 27 x 18 of them, column by column, the
// buffer fbShow (main.asm's direct page, $0002) names.
function fx3ShownFrame(memory: ReturnType<SNESEmulator['getMemory']>): Uint8Array {
    const base = memory.read(0x7e0002) ? 0x7980 : 0;
    const out = new Uint8Array(216 * 144);
    for (let tx = 0; tx < 27; tx++) for (let ty = 0; ty < 18; ty++) {
        const at = base + (tx * 18 + ty) * 64;
        for (let py = 0; py < 8; py++) {
            const planes = Array.from({ length: 8 }, (_, p) => memory.readVRAM(at + (p >> 1) * 16 + py * 2 + (p & 1)));
            for (let px = 0; px < 8; px++) {
                let v = 0;
                for (let p = 0; p < 8; p++) v |= ((planes[p] >> (7 - px)) & 1) << p;
                out[(ty * 8 + py) * 216 + tx * 8 + px] = v;
            }
        }
    }
    return out;
}

// What main.asm's SelectLevel does at boot (FX3), for the fixtures that call
// into the cartridge without booting it: the level table's entry as olLevel,
// and every level's host bank and LevelID.
function selectFixtureLevel(memory: ReturnType<SNESEmulator['getMemory']>, symbol: (name: string) => number,
                            write: (address: number, value: number) => void) {
    const table = symbol('LevelTable');
    const count = (symbol('SECTIONEND_level_table') - table) / LEVEL_ENTRY;
    const w16 = (a: number, v: number) => { write(a, v & 255); write(a + 1, v >> 8); };
    const r16 = (a: number) => memory.read(a) | memory.read(a + 1) << 8;
    // A single-level C oracle can compare the second level of a multi-level
    // cartridge. Its local index is zero; verify the actual packed LevelID
    // so a LEVEL1 reference cannot silently stand in for LEVEL2.
    const index = count === 1 ? 0 : BOOT_LEVEL;
    expect(Number.isInteger(index) && index >= 0 && index < count, 'fixture level index outside cartridge').toBe(true);
    const wantedId = readFileSync(join(DATA_DIR, 'FONT.BIN'))[56]; // mkbanks.py FONT_LEVEL
    expect(r16(table + index * LEVEL_ENTRY + 6), `fixture cartridge does not contain requested ${LEVEL}`).toBe(wantedId);
    for (let i = 0; i < LEVEL_ENTRY; i++) write(symbol('olLevel') + i, memory.read(table + index * LEVEL_ENTRY + i));
    for (let l = 0; l < count; l++) {
        w16(symbol('olLevelHostBanks') + l * 2, r16(table + l * LEVEL_ENTRY));
        w16(symbol('olLevelIds') + l * 2, r16(table + l * LEVEL_ENTRY + 6));
    }
    w16(symbol('olLevelIndex'), index);
    w16(symbol('olLevelCount'), count);
}

function awaitLevel(emulator: SNESEmulator) {
    const memory = emulator.getMemory();
    const at = sym('itemsCount');
    let t = 0;
    while ((memory.read(at) | (memory.read(at + 1) << 8)) === 0 && t < 600) {
        emulator.executeFrames(1);
        t++;
    }
    if (t >= 600) throw new Error('the level never came up in ten seconds');
}

function sym(name: string): number {
    const a = SYMS.get(name);
    if (a === undefined) throw new Error(`no symbol "${name}" in OL_SYMS`);
    return a;
}

// ItemObj is 32 bytes, so a field is `items + i*32 + offset`. The offsets are
// the struct's, in src/item.h; a power-of-two size is what keeps indexing a
// shift on the cartridge and makes it arithmetic here.
const ITEM_SIZE = 32;
const ITEM = {
    type: 0, room: 1, state: 2, goalState: 3,
    x: 4, y: 6, z: 8, angleY: 10,
    animIndex: 12, frameIndex: 14, flags: 15,
    health: 16, timer: 18, trigger: 20, model: 22, mood: 23,
    flags2: 24, nextActive: 25, roomFloor: 26, vSpeed: 28, hSpeed: 30,
};

// FX3's door.c, from the tables: upstream's Door::action shuts the sector
// one along the door's facing in its own room and, through that sector's
// portal as the level has it, the sector at the door in the room beyond.
// Host sector indices, and the level's own four bytes of each (ROM). TR's
// door types are 57..64.
const DOOR_FIRST = 57, DOOR_LAST = 64;
const DOOR_SHUT = [0x81, 0x81, 0, 0];
function doorSectors(memory: ReturnType<SNESEmulator['getMemory']>, symbol: (name: string) => number, item: number) {
    const u8 = (a: number) => memory.read(a);
    const word = (a: number) => u8(a) | u8(a + 1) << 8;
    const s16 = (v: number) => (v << 16) >> 16;
    const room = hostTable(memory, symbol, 'Room'), link = hostTable(memory, symbol, 'Link');
    const dir = symbol('olHostDirectory');
    const level = (word(dir + 54) + word(symbol('olLevel'))) * 65536 + word(dir + 52);
    const sectorAt = (r: number, x: number, z: number) => {
        const at = room + r * 10, xs = u8(at + 8) & 0x7F, zs = u8(at + 9);
        const dx = s16(x - word(at + 2)), dz = s16(z - word(at + 4));
        return word(at + 6) + (dx < 0 ? 0 : Math.min(xs - 1, dx >> 10)) * zs +
            (dz < 0 ? 0 : Math.min(zs - 1, dz >> 10));
    };
    const it = symbol('items') + item * ITEM_SIZE, a = word(it + ITEM.angleY);
    const x = word(it + ITEM.x), z = word(it + ITEM.z);
    const ox = a === 0x4000 ? -1024 : a === 0 || a === 0x8000 ? 0 : 1024;
    const oz = a === 0 ? -1024 : a === 0x8000 ? 1024 : 0;
    const first = sectorAt(u8(it + ITEM.room), x + ox, z + oz);
    const next = u8(link + word(level + first * 4 + 2) * 8 + 2);
    const sectors = next < word(symbol('olRoomCount')) ? [first, sectorAt(next, x, z)] : [first];
    return sectors.map(s => ({ s, level: [0, 1, 2, 3].map(k => u8(level + s * 4 + k)) }));
}

// flags2 carries upstream's two-bit ITEM_FLAG_STATUS in its low bits.
const ITEM_STATUS = 0x03;

const PAD_UP = 0x0800;
const PAD_DOWN = 0x0400;
const PAD_LEFT = 0x0200;
const PAD_RIGHT = 0x0100;
const PAD_A  = 0x0080;
const PAD_L  = 0x0020;
const PAD_B  = 0x8000;
const PAD_R  = 0x0010;
const PAD_Y  = 0x4000;
const PAD_X  = 0x0040;
const NO_FLOOR = 32767;

function png(width: number, height: number, rgba: (x: number, y: number) => number): Buffer {
    const raw = Buffer.alloc(height * (width * 3 + 1));
    let p = 0;
    for (let y = 0; y < height; y++) {
        raw[p++] = 0;
        for (let x = 0; x < width; x++) {
            const c = rgba(x, y) >>> 0;
            raw[p++] = c & 0xff;
            raw[p++] = (c >>> 8) & 0xff;
            raw[p++] = (c >>> 16) & 0xff;
        }
    }

    const chunk = (tag: string, data: Buffer) => {
        const len = Buffer.alloc(4);
        len.writeUInt32BE(data.length);
        const body = Buffer.concat([Buffer.from(tag, 'ascii'), data]);
        const crc = Buffer.alloc(4);
        crc.writeUInt32BE(crc32(body) >>> 0);
        return Buffer.concat([len, body, crc]);
    };

    const ihdr = Buffer.alloc(13);
    ihdr.writeUInt32BE(width, 0);
    ihdr.writeUInt32BE(height, 4);
    ihdr[8] = 8;    // bit depth
    ihdr[9] = 2;    // truecolour
    return Buffer.concat([
        Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
        chunk('IHDR', ihdr),
        chunk('IDAT', deflateSync(raw, { level: 9 })),
        chunk('IEND', Buffer.alloc(0)),
    ]);
}

describe('OpenLara SNES/Super FX: native maths', () => {
    it('matches the fixed-point reference through the 816-tcc ABI', () => {
        const rom = new Uint8Array(readFileSync(ROM));
        const emulator = new SNESEmulator();
        emulator.loadROM(rom);
        const memory = emulator.getMemory();
        const cpu = emulator.getCPU();
        // No game or GSU job is running. Reproduce the variant's boot copies.
        for (const [bank, dest, size] of WRAM_CODE_IMAGES) {
            for (let i = 0; i < size; i++) memory.write(dest + i, rom[bank * 0x8000 + i]);
        }
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 255);
            memory.write(a + 1, (v >>> 8) & 255);
        };
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        const call = (name: string, args: number[]): number => {
            cpu.E = 0; cpu.P = 4; cpu.D = 0; cpu.DB = 0x7E;
            cpu.S = 0x1C00;
            // A synthetic JSL return, followed by 32-bit stack arguments.
            w16(0x1C01, 0x04FF);
            memory.write(0x1C03, 0x7E);
            args.forEach((value, i) => {
                w16(0x1C04 + i * 4, value);
                w16(0x1C06 + i * 4, value >>> 16);
            });
            const address = sym(name);
            cpu.PB = address >>> 16; cpu.PC = address & 0xFFFF;
            let steps = 0;
            while ((cpu.PB !== 0x7E || cpu.PC !== 0x0500) && steps++ < 2000) cpu.step();
            if (steps >= 2000) throw new Error(`${name} did not return for ${args}`);
            if (cpu.S !== 0x1C03 || cpu.DB !== 0x7E || cpu.D !== 0 ||
                cpu.E !== 0 || (cpu.P & 0x3C) !== 4) {
                throw new Error(`${name} corrupted the C calling convention`);
            }
            // 816-tcc returns long long as two integer registers, r0/r1.
            // r0h is the bank half of a far pointer, not the integer high word.
            return (u16(sym('tcc__r0')) | (u16(sym('tcc__r1')) << 16)) >>> 0;
        };
        const same = (name: string, args: number[], expected: number) => {
            const actual = call(name, args);
            if (actual !== (expected >>> 0)) {
                throw new Error(`${name}(${args}) = ${actual}, expected ${expected >>> 0}`);
            }
        };
        // Every angle, including rounding and wrap boundaries; a nonzero
        // upper word must not affect a 16-bit TR angle.
        for (let a = 0; a < 65536; a++) {
            const sinIndex = (((a + 32) & 0xFFFF) >>> 6);
            const cosIndex = (((a + 0x4020) & 0xFFFF) >>> 6);
            same('phd_sin', [a | 0x12340000], Math.round(Math.sin(sinIndex * Math.PI / 512) * 16384));
            same('phd_cos', [a | 0xABCD0000], Math.round(Math.sin(cosIndex * Math.PI / 512) * 16384));
        }
        const atan = (x: number, y: number) => {
            let ax = Math.abs(x), ay = Math.abs(y);
            if (!ax && !ay) return 0;
            const swapped = ay > ax;
            if (swapped) [ax, ay] = [ay, ax];
            let a = Math.round(Math.atan(Math.floor(ay * 128 / ax) / 128) * 65536 / (2 * Math.PI));
            if (swapped) a = 0x4000 - a;
            if (y < 0) a = -a;
            if (x < 0) a = 0x8000 - a;
            return a & 0xFFFF;
        };
        const limits = [-32768, -32767, -16384, -256, -1, 0, 1, 127, 128, 255, 256, 16384, 32767];
        for (const x of limits) for (const y of limits) same('phd_atan', [x, y], atan(x, y));
        // Sample every quotient bin at maximum magnitude, including negative
        // axes and the X/Y swap. The grid above covers all octants.
        for (let y = 0; y <= 32768; y += 31) {
            for (const sign of [-1, 1]) {
                same('phd_atan', [-32768, sign * Math.min(y, 32767)], atan(-32768, sign * Math.min(y, 32767)));
                same('phd_atan', [sign * Math.min(y, 32767), -32768], atan(sign * Math.min(y, 32767), -32768));
            }
        }
        // Perfect-square boundaries exercise the restoring remainder; the
        // top of the uint32 range requires the trial's seventeenth bit.
        for (let root = 0; root <= 65535; root += 17) {
            for (const delta of [-1, 0, 1]) {
                const x = root * root + delta;
                if (x >= 0 && x <= 0xFFFFFFFF) same('phd_sqrt', [x], Math.floor(Math.sqrt(x)));
            }
        }
        for (const x of [0x7FFFFFFF, 0x80000000, 0xFFFFFFFE, 0xFFFFFFFF]) {
            same('phd_sqrt', [x], Math.floor(Math.sqrt(x)));
        }
        console.log('[openlara] native maths: every trig angle, atan octants and square-root boundaries agree');
    });
});

// The weapon scenes' harness: the cartridge's code, called directly, over a
// small synthetic room table so each geometric rejection has one cause.
function weaponHarness() {
        const rom = new Uint8Array(readFileSync(ROM));
        const emulator = new SNESEmulator();
        emulator.loadROM(rom);
        const memory = emulator.getMemory();
        const cpu = emulator.getCPU();
        // Load the actual cartridge code, but use a small synthetic room
        // table so each geometric rejection has one unambiguous cause.
        for (const [bank, dest, size] of WRAM_CODE_IMAGES) {
            for (let i = 0; i < size; i++) memory.write(dest + i, rom[bank * 0x8000 + i]);
        }
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 255); memory.write(a + 1, (v >>> 8) & 255);
        };
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        const call = (name: string, words: number[] = []) => {
            cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
            w16(0x1EE1, 0x04FF); memory.write(0x1EE3, 0x7E);
            words.forEach((word, i) => w16(0x1EE4 + i * 2, word));
            const a = sym(name);
            cpu.PB = a >>> 16; cpu.PC = a & 0xFFFF;
            let steps = 0;
            const before = cpu.cycles;
            while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 100000) cpu.step();
            expect(steps, `${name} never returned`).toBeLessThan(100000);
            expect(cpu.S, `${name} unbalanced the stack`).toBe(0x1EE3);
            expect([cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} corrupted the C calling convention`)
                .toEqual([0x7E, 0, 0, 4]);
            return cpu.cycles - before;
        };
        const blob = FX3 ? 0x7F0000 : 0x710000;
        const item = (i: number, f: number) => sym('items') + i * ITEM_SIZE + f;
        const hp = (i: number) => u16(item(i, ITEM.health));
        const tick = (keys = 0x20) => call('Weapon_update', [sym('items') & 0xFFFF, sym('items') >>> 16, keys]);
        const sector = (x: number, z: number, room = 0) => blob + 0x100 + (room * 256 + x * 16 + z) * 4;
        const place = (i: number, x: number, y: number, z: number, room = 0) => {
            for (let j = 0; j < ITEM_SIZE; j++) memory.write(item(i, j), 0);
            memory.write(item(i, ITEM.type), i ? 7 : 0);
            memory.write(item(i, ITEM.room), room);
            memory.write(item(i, ITEM.flags), 0x40);
            memory.write(item(i, ITEM.flags2), 1);
            memory.write(item(i, ITEM.nextActive), i === 2 ? 1 : 255);
            w16(item(i, ITEM.x), x); w16(item(i, ITEM.y), y); w16(item(i, ITEM.z), z);
            w16(item(i, ITEM.health), i ? 6 : 1000);
        };
        const reset = () => {
            for (const [name, value] of Object.entries({ olRoomBase: 0x40, olRoomCount: 2,
                olSectorBase: 0x100, olSectorCount: 512, olLinkBase: 0x900,
                olAnimBoxBase: 0xA00, olAnimAimBase: 0, olCreatureCount: 2, itemsCount: 3,
                olFirstActive: 2, olLaraAngleX: 0,
                olLaraWaterState: 0, olLaraWeaponState: 0 })) {
                w16(sym(name), value);
                if (FX3 && /^ol\w+Base$/.test(name)) w16(sym(name) + 2, value ? 0x7F : 0);
            }
            for (let room = 0; room < 2; room++) {
                const r = blob + 0x40 + room * 10;
                for (let j = 0; j < 10; j++) memory.write(r + j, 0);
                w16(r + 6, room * 256); memory.write(r + 8, 16); memory.write(r + 9, 16);
                for (let x = 0; x < 16; x++) for (let z = 0; z < 16; z++) {
                    w16(sector(x, z, room), 12); // floor 3072, ceiling 0
                    w16(sector(x, z, room) + 2, 0);
                }
            }
            // Two links: none, and a horizontal portal to room 1.
            for (let i = 0; i < 16; i++) memory.write(blob + 0x900 + i, i % 8 < 4 ? 255 : 0);
            memory.write(blob + 0x90A, 1);
            [-128, 128, -768, 0, -128, 128].forEach((v, i) => w16(blob + 0xA00 + i * 2, v));
            place(0, 4608, 3072, 2560);
            place(1, 4608, 3072, 2860); // 300 units: square must not wrap
            place(2, 4608, 3072, 3072); // 512 units: 16-bit square is zero
            memory.write(sym('olCreatures'), 1); memory.write(sym('olCreatures') + 1, 2);
            call('Room_reset'); call('Weapon_init');
            // Geometry fixtures begin with both pistols raised and ready.
            w16(sym('olLaraWeaponState'), 4);
            for (const off of [0, 12]) {
                w16(sym('olWeaponArms') + off, 0);
                w16(sym('olWeaponArms') + off + 2, 4);
                w16(sym('olWeaponArms') + off + 10, 1);
            }
        };
        const arms = () => [0, 12].map(off => Array.from({ length: 6 }, (_, i) =>
            u16(sym('olWeaponArms') + off + i * 2)));
        const phases = () => arms().map(a => a.slice(0, 2));
        const look = () => Array.from({ length: 4 }, (_, i) => u16(sym('olLaraLook') + i * 2));
        return { rom, emulator, memory, cpu, w16, u16, call, blob, item, hp, tick, sector, place, reset,
                 arms, phases, look };
}

describe('OpenLara SNES/Super FX: pistol combat', () => {
    it('draws, tracks, fires and holsters independent pistols with rotated aiming', () => {
        const { memory, w16, u16, call, blob, item, hp, tick, sector, place, reset, arms, phases, look } =
            weaponHarness();
        reset(); call('Weapon_init');
        tick(0x20);
        expect(u16(sym('olWeaponShots')), 'Action fired holstered guns').toBe(0);
        tick(0x80);
        expect(u16(sym('olLaraWeaponState'))).toBe(2);
        expect(phases()).toEqual([[1, 1], [1, 1]]);
        for (let i = 0; i < 6; i++) tick();
        expect(phases()).toEqual([[1, 7], [1, 7]]);
        expect(arms().map(a => a[5]), 'guns left the thighs before PICK ended').toEqual([0, 0]);
        tick();
        expect(phases()).toEqual([[2, 0], [2, 0]]);
        expect(arms().map(a => a[5])).toEqual([1, 1]);
        for (let i = 0; i < 10; i++) tick();
        expect(u16(sym('olLaraWeaponState')), 'DRAW ended before its last frame').toBe(2);
        tick();
        expect(u16(sym('olLaraWeaponState'))).toBe(4);
        expect(phases()).toEqual([[0, 0], [0, 0]]);
        for (let i = 0; i < 4; i++) tick();
        expect(phases()).toEqual([[0, 4], [0, 4]]);
        expect(u16(sym('olWeaponShots')), 'arms fired before being raised').toBe(0);
        tick();
        expect(phases()).toEqual([[3, 0], [3, 0]]);
        expect(u16(sym('olWeaponShots')), 'both pistols must discharge').toBe(2);
        expect(hp(1)).toBe(4);
        tick(0x80);
        expect(u16(sym('olLaraWeaponState'))).toBe(3);
        expect(phases(), 'holstering did not leave the firing animation').toEqual([[0, 4], [0, 4]]);
        for (let i = 0; i < 4; i++) tick();
        expect(arms().map(a => a.slice(0, 4))).toEqual([[0, 0, 0, 0], [0, 0, 0, 0]]);
        tick(); expect(phases()).toEqual([[2, 10], [2, 10]]);
        for (let i = 0; i < 10; i++) tick();
        expect(arms().map(a => a[5])).toEqual([1, 1]);
        tick(); expect(phases()).toEqual([[1, 7], [1, 7]]);
        expect(arms().map(a => a[5]), 'guns did not return to their thighs').toEqual([0, 0]);
        for (let i = 0; i < 7; i++) tick();
        expect(u16(sym('olLaraWeaponState'))).toBe(3);
        tick();
        expect(u16(sym('olLaraWeaponState'))).toBe(0);
        expect(arms()).toEqual([[65535, 0, 0, 0, 0, 0], [65535, 0, 0, 0, 0, 0]]);
        expect(u16(sym('olWeaponShots')), 'Action fired during holstering').toBe(2);
        for (const [state, water] of [[1, 0], [0, 2], [0, 3]]) {
            w16(sym('olLaraWeaponState'), state); w16(sym('olLaraWaterState'), water);
            tick(0x80);
            expect(u16(sym('olLaraWeaponState')), 'busy or swimming Lara drew pistols').toBe(state);
        }
        // Exact signed division during holster must land at zero on the
        // last AIM frame, including tiny negative angles and signed limits.
        for (let frame = 1; frame <= 4; frame++) {
            for (const angle of [-32768, -10921, -1821, -1, 0, 1, 1821, 10921, 32767]) {
                w16(sym('olLaraWeaponState'), 3);
                for (const off of [0, 12]) {
                    [0, frame, angle, angle, 1, 1].forEach((v, i) => w16(sym('olWeaponArms') + off + i * 2, v));
                }
                call('Weapon_animate', [0]);
                const expected = (angle - Math.trunc(angle / frame)) & 65535;
                expect(arms().map(a => a.slice(1, 4)), `holster angle ${angle}, frame ${frame}`)
                    .toEqual([[frame - 1, expected, expected], [frame - 1, expected, expected]]);
            }
        }

        // angleLerp takes the short signed arc across the angular seam,
        // then caps the step at ten degrees (ANGLE_1 is 182 in this port).
        for (const [angle, target] of [[-32000, 32000], [32000, -32000], [-1, 1], [1, -1],
            [0, -32768], [0, 32767], [-32768, 0], [0, 1821], [0, -1821]]) {
            w16(sym('olLaraWeaponState'), 4); w16(sym('olWeaponAim'), 3);
            w16(sym('olWeaponAimX'), target); w16(sym('olWeaponAimY'), target);
            for (const off of [0, 12]) {
                [0, 0, angle, angle, 0, 1].forEach((v, i) => w16(sym('olWeaponArms') + off + i * 2, v));
            }
            call('Weapon_animate', [0]);
            const delta = ((target - angle) << 16) >> 16;
            const expected = (angle + Math.max(-1820, Math.min(1820, delta))) & 65535;
            expect(arms().map(a => a.slice(2, 4)), `aim smoothing ${angle} -> ${target}`)
                .toEqual([[expected, expected], [expected, expected]]);
        }

        // updateLook reads the preceding tick's arms. Widen before adding:
        // signed overflow and negative odd sums distinguish it from averaging
        // each arm separately. The yaw pair differs from the pitch pair.
        reset(); w16(sym('olWeaponTarget'), 1);
        const lookAngles = [-32768, -32767, -16384, -8, -1, 0, 1, 7, 16384, 32767];
        for (const a of lookAngles) for (const b of lookAngles) for (const aiming of [0, 1, 2, 3]) {
            w16(sym('olWeaponAim'), aiming);
            w16(sym('olWeaponArms') + 4, a); w16(sym('olWeaponArms') + 16, b);
            w16(sym('olWeaponArms') + 6, a); w16(sym('olWeaponArms') + 18, ~b);
            const before = arms();
            call('Lara_updateLook');
            const shift = aiming === 3 ? 2 : 1;
            const x = ((a + b) >> shift) & 65535, y = ((a + ~b) >> shift) & 65535;
            expect(look(), `look angles ${a}, ${b}, aim ${aiming}`).toEqual([x, y, x, y]);
            expect(arms(), 'looking changed the arm controller').toEqual(before);
            expect([u16(sym('olWeaponTarget')), u16(sym('olWeaponAim'))]).toEqual([1, aiming]);
        }
        w16(sym('olWeaponTarget'), 255);
        if (FX3) {
            // An item the camera looked at last tick (camera.lastItem) holds
            // her head: no decay. At rest ($FF) it decays as below.
            [1234, 5678, 3000, -3000].forEach((v, i) => w16(sym('olLaraLook') + i * 2, v));
            w16(sym('olCamLastItem'), 4);
            call('Lara_updateLook');
            expect(look(), 'a looked-at item did not hold her head').toEqual([1234, 5678, 3000, -3000 & 65535]);
            w16(sym('olCamLastItem'), 255);
        }
        for (const angle of [...lookAngles, -7, 8]) {
            let x = angle, y = ~angle;
            // Use head values, not the stale torso, to return to centre.
            [1234, 5678, x, y].forEach((v, i) => w16(sym('olLaraLook') + i * 2, v));
            const decay = (v: number) => {
                const step = Math.abs(v) >> 3, delta = ((-v) << 16) >> 16;
                return ((v + Math.max(-step, Math.min(step, delta))) << 16) >> 16;
            };
            for (let n = 0; n < 80; n++) {
                x = decay(x); y = decay(y);
                call('Lara_updateLook');
                expect(look(), `look decay ${angle}, tick ${n}`)
                    .toEqual([x, y, x, y].map(v => v & 65535));
            }
        }
        for (const health of [0, -1, -32768]) {
            w16(item(0, ITEM.health), health); w16(sym('olWeaponTarget'), 1);
            [100, 200, 300, 400].forEach((v, i) => w16(sym('olLaraLook') + i * 2, v));
            call('Lara_updateLook'); expect(look(), 'dead Lara kept looking').toEqual([0, 0, 0, 0]);
        }
        w16(sym('olLaraLook'), 123); w16(sym('olLaraLook') + 6, -456);
        call('Weapon_init'); expect(look(), 'level initialization retained look angles').toEqual([0, 0, 0, 0]);

        // Turning left leaves only the left arm firing. Its reload continues
        // while the right arm lowers; the animations cannot share a timer.
        reset(); w16(item(1, ITEM.health), 1000); tick();
        w16(item(0, ITEM.angleY), 0x4000);
        for (let i = 0; i < 10; i++) tick();
        expect(phases()).toEqual([[0, 0], [3, 0]]);
        expect(arms().map(a => a[4])).toEqual([0, 1]);
        expect(u16(sym('olWeaponShots'))).toBe(3);
        expect(hp(1)).toBe(997);

        for (const [state, input] of [[6, 2], [7, 8]]) {
            for (const [weapons, water, walk, goal] of [[0, 0, 0, state], [4, 0, 0, 20],
                [4, 0, 0x40, state], [4, 1, 0, state]]) {
                reset();
                memory.write(item(0, ITEM.state), state); memory.write(item(0, ITEM.goalState), state);
                w16(sym('olLaraTurn'), 0); w16(sym('olLaraWaterState'), water);
                w16(sym('olLaraWeaponState'), weapons);
                call('Lara_updateState', [sym('items') & 65535, sym('items') >>> 16, input | walk]);
                expect(memory.read(item(0, ITEM.goalState)), 'armed turn ignored READY/walk/wade state').toBe(goal);
            }
        }

        for (const state of [3, 29]) { // forward jump and backward fall
            for (const weapons of [0, 1, 2, 3, 4]) {
                reset();
                memory.write(item(0, ITEM.state), state); memory.write(item(0, ITEM.goalState), state);
                w16(sym('olLaraWeaponState'), weapons);
                call('Lara_updateState', [sym('items') & 65535, sym('items') >>> 16, 0x20]);
                expect(memory.read(item(0, ITEM.goalState)), 'occupied hands reached for a ledge')
                    .toBe(weapons === 0 ? 11 : state);
            }
        }
        // The common grab guard protects both ground climbs and airborne
        // catches. CollisionSide occupies six bytes in the cartridge ABI.
        const grab = [...SYMS.keys()].find(name => name.endsWith('_c_checkGrab'));
        expect(grab, 'missing shared ledge-grab guard').toBeDefined();
        memory.write(sym('cinfo') + 50, 1); // CT_FRONT
        w16(sym('cinfo') + 14, 0); w16(sym('cinfo') + 20, 0); // level left/right edge
        for (const weapons of [0, 1, 2, 3, 4]) {
            w16(sym('olLaraWeaponState'), weapons);
            call(grab!, [0x20]);
            expect(u16(sym('tcc__r0')), 'ledge grab ignored occupied hands').toBe(weapons === 0 ? 0 : 1);
        }

        reset();
        const firstShotCycles = tick();
        expect([hp(1), hp(2)], 'equal aim angles prefer the last active-list candidate').toEqual([4, 6]);
        expect(u16(sym('olWeaponTarget'))).toBe(1);
        for (let i = 0; i < 9; i++) tick(i & 1 ? 0x20 : 0);
        expect(hp(1), 'rapid release/repress bypassed the firing animation').toBe(4);
        tick();
        expect(hp(1), 'held fire did not resume after nine reload frames').toBe(2);
        expect(u16(sym('olWeaponShots'))).toBe(4);
        for (let i = 0; i < 10; i++) tick();
        expect(hp(1)).toBe(0);
        for (let i = 0; i < 10; i++) tick();
        expect(hp(2), 'a dead target retained the next shot').toBe(4);

        for (const [label, change] of [
            ['no input', () => {}],
            ['busy hands', () => w16(sym('olLaraWeaponState'), 1)],
            ['underwater', () => w16(sym('olLaraWaterState'), 3)],
            ['surface swimming', () => w16(sym('olLaraWaterState'), 2)],
            ['dead Lara', () => w16(item(0, ITEM.health), 0)],
        ] as const) {
            reset(); change(); tick(label === 'no input' ? 0 : 0x20);
            expect(u16(sym('olWeaponShots')), label).toBe(0);
        }
        for (const status of [0, 2, 3]) {
            reset();
            memory.write(item(1, ITEM.flags2), status); w16(item(2, ITEM.health), 0);
            tick(); expect(hp(1), `ineligible item status ${status}`).toBe(6);
        }
        reset(); memory.write(item(1, ITEM.flags), 0); w16(item(2, ITEM.health), 0);
        tick(); expect(hp(1), 'inactive creature was shot').toBe(6);

        const blocked = (label: string, change: () => void) => {
            reset(); place(1, 4608, 3072, 4608); w16(item(2, ITEM.health), 0);
            change(); call('Room_reset'); tick();
            expect(hp(1), label).toBe(6);
            expect(u16(sym('olWeaponTarget')), label).toBe(255);
            expect(u16(sym('olWeaponShots')), 'a miss still fires').toBe(2);
            for (let i = 0; i < 8; i++) tick();
            expect(u16(sym('olWeaponShots')), 'a miss still reloads').toBe(2);
        };
        blocked('a solid sector blocks the shot', () => memory.write(sector(4, 3), 0x81));
        blocked('a raised floor blocks the shot', () => memory.write(sector(4, 3), 8));
        blocked('a low ceiling blocks the shot', () => memory.write(sector(4, 3) + 1, 11));
        blocked('a solid endpoint is not a target', () => memory.write(sector(4, 4), 0x81));
        blocked('coincident disconnected rooms', () => memory.write(item(1, ITEM.room), 1));
        blocked('spherical range rejects a diagonal outside 8192', () => {
            w16(item(1, ITEM.x), 10608); w16(item(1, ITEM.z), 8560);
        });
        blocked('one unit beyond the range boundary', () => w16(item(1, ITEM.z), 2560 + 8193));
        blocked('vertical range', () => w16(item(1, ITEM.y), -6000));
        blocked('yaw beyond the aim cone', () => w16(item(0, ITEM.angleY), 0x4000));
        blocked('exact half-turn yaw must not overflow abs', () => w16(item(0, ITEM.angleY), 0x8000));
        blocked('pitch beyond the aim cone', () => {
            w16(item(1, ITEM.z), 2860); w16(item(1, ITEM.y), 1000);
        });

        reset(); place(1, 4608, 3072, 4608, 1); w16(item(2, ITEM.health), 0);
        w16(sector(4, 3) + 2, 1); call('Room_reset'); tick();
        expect(hp(1), 'an open portal should permit the shot').toBe(4);
        reset(); place(1, 4608, 3072, 4608); place(2, 7680, 3072, 4608);
        memory.write(sector(4, 3), 0x81); call('Room_reset'); tick();
        expect([hp(1), hp(2)], 'a nearer occluded target stole the shot from a visible one').toEqual([6, 4]);
        reset(); place(1, 4608, 3072, 2560 + 8192); w16(item(2, ITEM.health), 0);
        tick(); expect(hp(1), 'the exact range boundary is inclusive').toBe(4);

        reset(); place(1, 5008, 3072, 3060); place(2, 4608, 3072, 4608);
        tick(); expect([hp(1), hp(2)], 'a nearer off-axis target stole the centred shot').toEqual([6, 4]);
        reset(); place(1, 5008, 3072, 3060); place(2, 4608, 3072, 4608);
        w16(sym('olLaraLook') + 2, 8192); tick();
        expect([hp(1), hp(2)], 'acquisition ignored torso-relative angle priority').toEqual([4, 6]);
        expect(u16(sym('olWeaponAimY')), 'tracking subtracted the torso from the arm angle')
            .toBeGreaterThan(6000);
        // A turned torso can select a target outside the body's acquisition
        // cone. Only arms already aiming may retain it in their wider arcs.
        for (const [bodyYaw, torsoYaw, expectedYaw, retained] of [
            [0, 16384, 16384, 1], [32768, -16384, -16384, 2],
            [-16384, 32767, -32768, 0],
        ]) for (const previous of [0, 1, 2, 3]) {
            reset(); place(1, 6656, 3072, 2560); w16(item(2, ITEM.health), 0);
            w16(item(0, ITEM.angleY), bodyYaw);
            w16(sym('olLaraLook') + 2, torsoYaw); w16(sym('olWeaponAim'), previous);
            tick(0);
            expect(u16(sym('olWeaponTarget')), 'torso cone failed to select the side target').toBe(1);
            expect(u16(sym('olWeaponAimY'))).toBe(expectedYaw & 65535);
            expect(u16(sym('olWeaponAim')), 'acquisition bypassed independent arm retention')
                .toBe(previous & retained);
        }
        reset(); place(1, 4608, 3072, 4608); w16(item(2, ITEM.health), 0);
        w16(sym('olLaraLook') + 2, 16384); tick(0);
        expect(u16(sym('olWeaponTarget')), 'a body-centred target bypassed the torso cone').toBe(255);
        reset(); place(1, 4608, 1000, 2860); w16(item(2, ITEM.health), 0);
        w16(sym('olLaraLook'), 8192); tick(0);
        expect(u16(sym('olWeaponTarget')), 'acquisition ignored torso pitch').toBe(1);
        expect(u16(sym('olWeaponAim')), 'torso pitch bypassed the body arm limit').toBe(0);
        // A rejected earlier candidate must not erase the previous tick's
        // arm flags before the better candidate is tracked. Active order is 2,1.
        reset(); place(1, 6656, 3072, 2560); place(2, 4608, 3072, 512);
        w16(sym('olLaraLook') + 2, 127 * 182); w16(sym('olWeaponAim'), 3); tick(0);
        expect(u16(sym('olWeaponTarget'))).toBe(1);
        expect(u16(sym('olWeaponAim')), 'discarded candidate changed the retained arm mask').toBe(1);
        reset(); place(1, 4608, 3072, 4608); place(2, 4608, 3072, 3072);
        tick(); expect([hp(1), hp(2)], 'equal angles were broken by distance').toEqual([4, 6]);
        reset(); place(1, 4608, 3072, 4608); place(2, 4608, 3072, 3072);
        w16(sym('olFirstActive'), 1);
        memory.write(item(1, ITEM.nextActive), 2); memory.write(item(2, ITEM.nextActive), 255);
        tick(); expect([hp(1), hp(2)], 'target ties ignored active-list order').toEqual([6, 4]);
        reset(); w16(sym('olFirstActive'), 255);
        tick(); expect([hp(1), hp(2)], 'a target absent from the active list was selected').toEqual([6, 6]);

        const aimBox = (x: number, z: number) => {
            // Give this target its own asymmetric animation box. The aim
            // direction must come from the box, while range uses item origins.
            w16(item(1, ITEM.animIndex), 1);
            [x - 128, x + 128, -768, 0, z - 128, z + 128]
                .forEach((v, i) => w16(blob + 0xA0C + i * 2, v));
        };
        reset(); place(1, 7608, 3072, 3560); w16(item(2, ITEM.health), 0);
        aimBox(-3000, 1500); tick();
        expect(hp(1), 'yaw used the origin instead of the aim point').toBe(4);
        reset(); place(1, 4608, 3072, 4608); w16(item(2, ITEM.health), 0);
        aimBox(3000, -1800); tick();
        expect(hp(1), 'an aim point outside the yaw cone was accepted').toBe(6);
        reset(); place(1, 4608, 1000, 2860); w16(item(2, ITEM.health), 0);
        aimBox(0, 2048); tick();
        expect(hp(1), 'pitch used the horizontal distance to the origin').toBe(4);
        reset(); place(1, 4608, 1000, 4608); w16(item(2, ITEM.health), 0);
        aimBox(0, -1900); tick();
        expect(hp(1), 'an aim point outside the pitch cone was accepted').toBe(6);
        reset(); place(1, 4608, 1000, 2860); w16(item(2, ITEM.health), 0);
        w16(sym('olLaraAngleX'), 0x2000); tick();
        expect(hp(1), 'acquisition ignored Lara\'s pitch').toBe(4);
        reset(); place(1, 4608, 3072, 2560 + 8193); w16(item(2, ITEM.health), 0);
        aimBox(0, -2048); tick();
        expect(hp(1), 'moving the aim point bypassed the origin range check').toBe(6);
        reset(); place(1, 4608, 3072, 2560 + 8192); w16(item(2, ITEM.health), 0);
        aimBox(0, 512); tick();
        expect(hp(1), 'the aim point incorrectly shortened the origin range').toBe(4);

        // Rotate a forward offset through a quarter turn into a neighbouring
        // sector. Visibility must follow that segment, including its endpoint.
        reset(); place(1, 4608, 3072, 4608); w16(item(2, ITEM.health), 0);
        aimBox(0, 1536); w16(item(1, ITEM.angleY), 0x4000);
        memory.write(sector(6, 4), 0x81); call('Room_reset'); tick();
        expect(hp(1), 'a solid rotated aim endpoint was visible').toBe(6);
        reset(); place(1, 4608, 3072, 4608); w16(item(2, ITEM.health), 0);
        aimBox(0, 2560); w16(item(1, ITEM.angleY), 0x4000);
        memory.write(sector(4, 3), 0x81); call('Room_reset'); tick();
        expect(hp(1), 'the trace followed the origin through a wall instead of the aim point').toBe(4);

        // Nearby rooms can straddle the signed X seam; rooms a whole 65536
        // apart can have the same stored coordinates. Both must be correct.
        const seam = (axis: 'x' | 'z' = 'x', reverse = false) => {
            reset();
            for (let room = 0; room < 2; room++) {
                memory.write(blob + 0x40 + room * 10 + (axis === 'x' ? 0 : 1), 30);
                w16(blob + 0x40 + room * 10 + (axis === 'x' ? 2 : 4), 30 * 1024);
            }
            const from = reverse ? 35200 : 32700;
            const to = reverse ? 32700 : 35200;
            place(0, axis === 'x' ? from : 2560, 3072, axis === 'z' ? from : 2560);
            place(1, axis === 'x' ? to : 2560, 3072, axis === 'z' ? to : 2560);
            w16(item(0, ITEM.angleY), (axis === 'x' ? 0x4000 : 0) + (reverse ? 0x8000 : 0));
            w16(item(2, ITEM.health), 0);
            call('Room_reset');
        };
        for (const axis of ['x', 'z'] as const) for (const reverse of [false, true]) {
            const label = `${reverse ? '-' : '+'}${axis} signed-coordinate seam`;
            seam(axis, reverse); tick(); expect(hp(1), label).toBe(4);
            seam(axis, reverse); memory.write(sector(2, 2), 0x81); call('Room_reset'); tick();
            expect(hp(1), `the trace skipped a wall at the ${label}`).toBe(6);
        }
        seam(); memory.write(item(1, ITEM.room), 1); memory.write(blob + 0x4A, 94);
        call('Room_reset'); tick(); expect(hp(1), 'distant room aliased a nearby target').toBe(6);

        // Tracking runs on reload ticks too. A better candidate entering
        // the crosshairs cannot steal a held target; release permits a new one.
        reset(); tick();
        w16(item(1, ITEM.x), 5000); w16(item(1, ITEM.z), 3072);
        tick();
        expect(u16(sym('olWeaponTarget')), 'reload forgot the held target').toBe(1);
        expect(u16(sym('olWeaponAim'))).toBe(3);
        expect(u16(sym('olWeaponAimY')), 'tracking did not update the aim angle').toBeGreaterThan(0);
        for (let i = 0; i < 9; i++) tick();
        expect([hp(1), hp(2)], 'a better-aligned target stole a held shot').toEqual([2, 6]);
        tick(0);
        expect(u16(sym('olWeaponTarget')), 'release did not select the better aim').toBe(2);
        expect(u16(sym('olWeaponAim'))).toBe(3);
        tick();
        expect(u16(sym('olWeaponTarget')), 'release did not permit a new target').toBe(2);
        expect(hp(2), 'reacquisition bypassed reload').toBe(6);

        const lock = () => {
            reset(); place(1, 4608, 3072, 4608); w16(item(2, ITEM.health), 0);
            tick(); expect(u16(sym('olWeaponTarget'))).toBe(1);
            expect(u16(sym('olWeaponAim'))).toBe(3);
        };
        const aim = () => u16(sym('olWeaponAim'));
        const yaw = (angle: number) => { w16(item(0, ITEM.angleY), angle); tick(); };
        lock();
        const bodyAim = [u16(sym('olWeaponAimX')), u16(sym('olWeaponAimY'))];
        w16(sym('olLaraLook'), 32767); w16(sym('olLaraLook') + 2, -32768); tick();
        expect([u16(sym('olWeaponAimX')), u16(sym('olWeaponAimY'))], 'held aim followed the torso')
            .toEqual(bodyAim);
        expect(aim(), 'tracking reapplied the torso acquisition cone').toBe(3);
        lock(); yaw(0xC000);
        expect(aim(), 'right arm lost a target inside its wider arc').toBe(1);
        yaw(0x4000);
        expect(aim(), 'a lost left arm reacquired outside the acquisition cone').toBe(0);
        yaw(0); expect(aim(), 'returning to the acquisition cone did not restore aim').toBe(3);
        lock(); yaw(0x4000);
        expect(aim(), 'left arm lost a target inside its wider arc').toBe(2);
        yaw(0x8000); expect(aim(), 'a half-turn overflow retained arm aim').toBe(0);
        expect(u16(sym('olWeaponTarget')), 'turning away discarded the target identity').toBe(1);
        yaw(0x4000); expect(aim(), 'lost aim returned in the wider tracking arc').toBe(0);
        yaw(0); expect(aim()).toBe(3);

        lock();
        const pitch = (u16(sym('olWeaponAimX')) << 16) >> 16;
        w16(sym('olLaraAngleX'), pitch - 70 * 182); tick();
        expect(aim(), 'pitch tracking stopped at the acquisition limit').toBe(3);
        w16(sym('olLaraAngleX'), pitch - 90 * 182); tick();
        expect(aim(), 'pitch tracking exceeded the arm limit').toBe(0);
        w16(sym('olLaraAngleX'), pitch - 70 * 182); tick();
        expect(aim(), 'lost pitch aim returned outside the acquisition cone').toBe(0);
        w16(sym('olLaraAngleX'), 0); tick(); expect(aim()).toBe(3);

        lock();
        memory.write(sector(4, 3), 0x81); call('Room_reset');
        const shots = u16(sym('olWeaponShots'));
        for (let i = 0; i < 18; i++) tick();
        expect(aim(), 'a wall failed to interrupt tracking').toBe(0);
        expect(u16(sym('olWeaponTarget')), 'occlusion switched away from the locked target').toBe(1);
        expect(u16(sym('olWeaponShots')), 'an occluded lock kept firing').toBe(shots);
        expect(hp(1), 'a tracked shot went through the wall').toBe(4);
        memory.write(sector(4, 3), 12); call('Room_reset'); yaw(0xC000);
        expect(aim(), 'clearing a wall reacquired outside the normal cone').toBe(0);
        yaw(0); expect(aim()).toBe(3);
        expect(hp(1), 'lowered arms skipped the raise animation').toBe(4);
        for (let i = 0; i < 4; i++) tick();
        expect(hp(1), 'raised arms did not resume firing').toBe(2);

        lock(); memory.write(item(1, ITEM.room), 1); tick();
        expect(aim(), 'tracking saw a disconnected overlapping room').toBe(0);
        expect(u16(sym('olWeaponTarget'))).toBe(1);
        lock(); w16(item(1, ITEM.z), 2560 + 8193); tick();
        expect(aim(), 'tracking incorrectly reapplied the acquisition range').toBe(3);
        lock(); memory.write(item(1, ITEM.room), 1); memory.write(blob + 0x4A, 64);
        call('Room_reset'); tick();
        expect(aim(), 'a distant room alias restored tracking').toBe(0);
        for (const axis of ['x', 'z'] as const) for (const reverse of [false, true]) {
            seam(axis, reverse); tick(); tick();
            expect(u16(sym('olWeaponTarget')), `tracking lost the ${axis} coordinate seam`).toBe(1);
            expect(aim()).toBe(3);
            memory.write(sector(2, 2), 0x81); call('Room_reset'); tick();
            expect(aim(), `tracking missed a wall across the ${axis} coordinate seam`).toBe(0);
        }
        for (const invalidate of [
            () => w16(item(1, ITEM.health), 0),
            () => memory.write(item(1, ITEM.flags2), 3),
            () => memory.write(item(1, ITEM.flags), 0),
        ]) {
            lock(); invalidate(); tick();
            expect(u16(sym('olWeaponTarget')), 'an invalid target retained its lock').toBe(255);
            expect(aim()).toBe(0);
        }
        for (const disable of [
            () => w16(item(0, ITEM.health), 0),
            () => w16(sym('olLaraWeaponState'), 1),
            () => w16(sym('olLaraWaterState'), 3),
            () => w16(sym('olLaraWaterState'), 2),
        ]) {
            lock(); disable(); tick();
            expect(u16(sym('olWeaponTarget')), 'disabling fire retained a lock').toBe(255);
            expect(aim()).toBe(0);
        }
        lock(); call('Weapon_init');
        expect(u16(sym('olWeaponTarget'))).toBe(255); expect(aim()).toBe(0);

        // Exercise the cartridge helper directly with boxes that expose
        // rotation signs, rounding and 16-bit midpoint overflow. Expected
        // points are fixed independently of the cartridge's math tables.
        const ordinaryBox = [-128, 384, -900, 0, -100, 600];
        const aimCases = [
            { yaw: 0, box: ordinaryBox, expected: [4736, 2472, 4858] },
            { yaw: 0x4000, box: ordinaryBox, expected: [4858, 2472, 4480] },
            { yaw: 0x8000, box: ordinaryBox, expected: [4480, 2472, 4358] },
            { yaw: 0xC000, box: ordinaryBox, expected: [4358, 2472, 4736] },
            { yaw: 0x2000, box: ordinaryBox, expected: [4875, 2472, 4694] },
            { yaw: 0, box: [-130, -127, -769, 0, -5, 0], expected: [4479, 2559, 4605] },
            { yaw: 0, box: [20000, 30000, -30000, 30000, -30000, -20000],
                expected: [29608, -6928, -20392] },
        ];
        for (const { yaw, box, expected } of aimCases) {
            reset(); place(1, 4608, 3072, 4608, 1); w16(item(1, ITEM.angleY), yaw);
            box.forEach((v, i) => w16(blob + 0xA00 + i * 2, v));
            const target = item(1, 0), point = 0x7E0540;
            const before = Array.from({ length: ITEM_SIZE }, (_, i) => memory.read(target + i));
            call('Weapon_getAimPoint', [target & 0xFFFF, target >>> 16, point & 0xFFFF, point >>> 16]);
            expect([0, 2, 4].map(i => (u16(point + i) << 16) >> 16), `aim point at yaw ${yaw}`)
                .toEqual(expected);
            expect(u16(point + 6), 'aim point lost its source room').toBe(1);
            expect(Array.from({ length: ITEM_SIZE }, (_, i) => memory.read(target + i)),
                'aim calculation mutated its target').toEqual(before);
        }
        reset(); place(1, 32700, 3072, 32700);
        ordinaryBox.forEach((v, i) => w16(blob + 0xA00 + i * 2, v));
        const target = item(1, 0), point = 0x7E0540;
        call('Weapon_getAimPoint', [target & 0xFFFF, target >>> 16, point & 0xFFFF, point >>> 16]);
        expect([0, 2, 4].map(i => (u16(point + i) << 16) >> 16), 'aim point crossed the signed seam incorrectly')
            .toEqual([-32708, 2472, -32586]);

        // Independent arithmetic oracle for the native helper: randomized
        // ordered boxes include the full signed-word range and arbitrary yaw.
        let seed = 0x7146AC19;
        const word = () => { seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0; return seed >>> 16; };
        const signed = (v: number) => (v << 16) >> 16;
        for (let n = 0; n < 256; n++) {
            const box = Array.from({ length: 3 }, () => [signed(word()), signed(word())].sort((a, b) => a - b)).flat();
            const pos = [signed(word()), signed(word()), signed(word())];
            const angle = word(), room = word() & 1;
            place(1, pos[0], pos[1], pos[2], room); w16(item(1, ITEM.angleY), angle);
            box.forEach((v, i) => w16(blob + 0xA00 + i * 2, v));
            const x = Math.floor((box[0] + box[1]) / 2), z = Math.floor((box[4] + box[5]) / 2);
            const theta = (((angle + 32) & 0xFFFF) >>> 6) * Math.PI * 2 / 1024;
            const s = Math.round(Math.sin(theta) * 16384), c = Math.round(Math.cos(theta) * 16384);
            const expected = [pos[0] + Math.floor((x * c + z * s) / 16384),
                pos[1] + box[2] + Math.trunc((box[3] - box[2]) / 3),
                pos[2] + Math.floor((z * c - x * s) / 16384)].map(signed);
            call('Weapon_getAimPoint', [target & 0xFFFF, target >>> 16, point & 0xFFFF, point >>> 16]);
            expect([0, 2, 4].map(i => signed(u16(point + i))), `random aim box ${n}`).toEqual(expected);
            expect(u16(point + 6)).toBe(room);
        }
        console.log('[openlara] pistol combat: draw/holster, independent arms, torso/head tracking, torso-relative acquisition, cadence, target lock, arm limits, rotated aiming, angle priority, eligibility, 3D range, occlusion, portals and coordinate seams agree');
        console.log(`[openlara] pistol combat: two-target discharge costs ${firstShotCycles} CPU cycles`);
    });

    it('aims at each creature animation frame using the original boxes', () => {
        const rom = new Uint8Array(readFileSync(ROM));
        const emulator = new SNESEmulator();
        emulator.loadROM(rom);
        const memory = emulator.getMemory(), cpu = emulator.getCPU();
        for (const [bank, dest, size] of WRAM_CODE_IMAGES) {
            for (let i = 0; i < size; i++) memory.write(dest + i, rom[bank * 0x8000 + i]);
        }
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 255); memory.write(a + 1, (v >>> 8) & 255);
        };
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        const signed = (v: number) => (v << 16) >> 16;
        const call = (name: string, words: number[] = []) => {
            cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
            w16(0x1EE1, 0x04FF); memory.write(0x1EE3, 0x7E);
            words.forEach((word, i) => w16(0x1EE4 + i * 2, word));
            const a = sym(name), before = cpu.cycles;
            cpu.PB = a >>> 16; cpu.PC = a & 65535;
            let steps = 0;
            while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 500000) cpu.step();
            expect(steps, `${name} never returned`).toBeLessThan(500000);
            expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} corrupted the C ABI`)
                .toEqual([0x1EE3, 0x7E, 0, 0, 4]);
            return cpu.cycles - before;
        };
        const dataDir = join(DATA_DIR);
        const level = readFileSync(join(dataDir, 'LEVEL.BIN'));
        const host = level.readUInt16LE(2), size = level.readUInt16LE(host);
        const blob = FX3 ? 0x7F0000 : 0x710000;
        if (FX3) { selectFixtureLevel(memory, sym, (a, v) => memory.write(a, v)); call('olHostBind'); }
        else for (let i = 0; i < size; i++) memory.write(blob + i, level[host + i]);
        call('olLevelInit'); // use the cartridge's real table-offset calculation
        const table = (name: string) => hostTable(memory, sym, name);
        expect(u16(table('AnimAim')), 'aim table does not start with the animation count')
            .toBe(FX3 ? u16(sym('olHostDirectory') + 20) : level.readUInt16LE(host + 6));
        const source = JSON.parse(readFileSync(join(dataDir, 'AIM.json'), 'utf8')) as {
            animations: { anim: number; type: number; begin: number; end: number; rate: number; boxes: number[][] }[];
        };
        expect(source.animations.length).toBeGreaterThan(0);
        const target = sym('items') + ITEM_SIZE, point = 0x7E0540;
        const pos = [32700, -900, -32700];
        pos.forEach((v, i) => w16(target + ITEM.x + i * 2, v));
        memory.write(target + ITEM.room, 1);
        const readPoint = () => [0, 2, 4].map(off => signed(u16(point + off)));
        let ticks = 0, moved = 0, cost = 0;
        for (const { anim, type, begin, end, rate, boxes } of source.animations) {
            memory.write(target + ITEM.type, type); w16(target + ITEM.animIndex, anim);
            let first: number[] | undefined;
            for (let tick = 0; tick <= end - begin; tick++) {
                // Choose the nearest source pose, ties toward the earlier.
                // getFrames permits the following pose only before its
                // original frameEnd; that field is a global frame number.
                const lower = Math.floor(tick / rate);
                const choices = [lower];
                if ((lower + 1) * rate < end) choices.push(lower + 1);
                choices.sort((a, b) => Math.abs(a * rate - tick) - Math.abs(b * rate - tick) || a - b);
                const box = boxes[choices[0]];
                expect(box, `missing source box for animation ${anim} tick ${tick}`).toBeDefined();
                const local = [Math.floor((box[0] + box[1]) / 2),
                    box[2] + Math.trunc((box[3] - box[2]) / 3), Math.floor((box[4] + box[5]) / 2)];
                memory.write(target + ITEM.frameIndex, tick);
                for (const yaw of [0, (anim * 331 + tick * 1223) & 65535]) {
                    w16(target + ITEM.angleY, yaw);
                    const theta = (((yaw + 32) & 65535) >>> 6) * Math.PI / 512;
                    const s = Math.round(Math.sin(theta) * 16384), c = Math.round(Math.cos(theta) * 16384);
                    const expected = [pos[0] + Math.floor((local[0] * c + local[2] * s) / 16384),
                        pos[1] + local[1], pos[2] + Math.floor((local[2] * c - local[0] * s) / 16384)].map(signed);
                    const cycles = call('Weapon_getAimPoint', [target & 65535, target >>> 16, point & 65535, point >>> 16]);
                    expect(readPoint(), `animation ${anim} tick ${tick} yaw ${yaw}`).toEqual(expected);
                    expect(u16(point + 6)).toBe(1);
                    if (yaw === 0) {
                        if (!first) first = expected;
                        else if (expected.some((v, i) => v !== first![i])) moved++;
                        cost += cycles;
                    }
                }
                ticks++;
            }
            memory.write(target + ITEM.frameIndex, 255); w16(target + ITEM.angleY, 0);
            call('Weapon_getAimPoint', [target & 65535, target >>> 16, point & 65535, point >>> 16]);
            expect(readPoint(), 'out-of-range tick did not use this animation\'s start').toEqual(first);
        }
        expect(moved, 'all creature aim points still use a single animation box').toBeGreaterThan(100);
        w16(target + ITEM.animIndex, 103); // Lara stand: a sparse-table fallback
        memory.write(target + ITEM.frameIndex, 128); w16(target + ITEM.angleY, 0);
        expect(u16(table('AnimAim') + 2 + 103 * 2)).toBe(0);
        const union = Array.from({ length: 6 }, (_, i) =>
            signed(u16(table('AnimBox') + 103 * 12 + i * 2)));
        call('Weapon_getAimPoint', [target & 65535, target >>> 16, point & 65535, point >>> 16]);
        expect(readPoint(), 'an animation without a point stream lost its union fallback').toEqual([
            pos[0] + Math.floor((union[0] + union[1]) / 2),
            pos[1] + union[2] + Math.trunc((union[3] - union[2]) / 3),
            pos[2] + Math.floor((union[4] + union[5]) / 2),
        ].map(signed));
        w16(target + ITEM.animIndex, 65535);
        call('Weapon_getAimPoint', [target & 65535, target >>> 16, point & 65535, point >>> 16]);
        expect(readPoint(), 'corrupt animation indexed beyond the aim table').toEqual(pos);
        expect(u16(point + 6)).toBe(1);
        console.log(`[openlara] frame aiming: ${ticks} ticks / ${source.animations.length} animations, ${moved} moving aim points; average unrotated lookup ${Math.round(cost / ticks)} CPU cycles`);
    });

    it.skipIf(LEVEL !== 'LEVEL1')('sustains the combat clock while firing at two stationary targets', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 255); memory.write(a + 1, (v >>> 8) & 255);
        };
        emulator.executeFrames(120);
        const wolves: number[] = [];
        for (let i = 1; i < u16(sym('itemsCount')) && wolves.length < 2; i++) {
            if (memory.read(sym('items') + i * ITEM_SIZE + ITEM.type) === 7) wolves.push(i);
        }
        expect(wolves.length, 'the level has no pair of wolf targets').toBe(2);
        // Keep the real scheduler, renderer, Lara update and room data. Only
        // enemy updates freeze, so both candidates stay alive and in the aim
        // cone for the entire sample instead of ending the load after six hits.
        w16(sym('ol_animFreeze'), 1);
        wolves.forEach((index, i) => {
            const a = sym('items') + index * ITEM_SIZE;
            memory.write(a + ITEM.room, 0);
            memory.write(a + ITEM.flags, 0x40);
            memory.write(a + ITEM.flags2, 1);
            memory.write(a + ITEM.nextActive, i === 0 ? wolves[1] : 255);
            w16(a + ITEM.x, 9728); w16(a + ITEM.y, 3072);
            w16(a + ITEM.z, 3900 + i * 150); w16(a + ITEM.angleY, 0);
            w16(a + ITEM.health, 1000);
        });
        w16(sym('olFirstActive'), wolves[0]);
        const pad = emulator.getController() as unknown as { buttonState: number };
        pad.buttonState = PAD_R;
        emulator.executeFrames(4);
        pad.buttonState = PAD_B;
        emulator.executeFrames(120); // finish drawing, then settle the gauges
        // Publication follows completed renders, not a fixed field count.
        // On GSU-2 the last drawing frame can still be in flight here.
        for (let fields = 0; fields < 360 && memory.readCartridgeRAM(0xF710) !== 4; fields++)
            emulator.executeFrames(1);
        const channel = Array.from({ length: 13 }, (_, i) => memory.readCartridgeRAM(0xF710 + i * 2) |
            (memory.readCartridgeRAM(0xF711 + i * 2) << 8));
        expect(channel[0], 'the GSU did not receive READY weapon state').toBe(4);
        expect([channel[6], channel[12]], 'the frame handoff lost the drawn meshes').toEqual([1, 1]);
        expect([channel[5], channel[11]], 'the frame handoff lost the aimed arm bases').toEqual([1, 1]);
        emulator.executeFrames(120); // a complete gauge window after that handoff
        const lookChannel = Array.from({ length: 4 }, (_, i) => memory.readCartridgeRAM(0xF72A + i * 2) |
            (memory.readCartridgeRAM(0xF72B + i * 2) << 8));
        expect(lookChannel, 'the frame handoff lost stable torso/head angles')
            .toEqual(Array.from({ length: 4 }, (_, i) => u16(sym('olLaraLook') + i * 2)));
        expect(lookChannel[0], 'the game tick never updated pistol look pitch').not.toBe(0);
        expect(lookChannel.slice(0, 2)).toEqual(lookChannel.slice(2));
        const ticksBefore = u16(sym('ol_ticks'));
        const shotsBefore = u16(sym('olWeaponShots'));
        const health = () => wolves.reduce((sum, i) => sum + u16(sym('items') + i * ITEM_SIZE + ITEM.health), 0);
        const healthBefore = health();
        const timeline: string[] = [];
        let rendered = 0;
        for (let second = 0; second < 10; second++) {
            emulator.executeFrames(60);
            const tps = u16(sym('ol_tps'));
            const dropped = u16(sym('ol_ticksDropped'));
            const ticks = (u16(sym('ol_ticks')) - ticksBefore) & 0xFFFF;
            // NMI issues 30 ticks per second. A renderer handoff can queue
            // ticks across the gauge boundary, so bound cumulative drift
            // by the four-entry queue plus one in-flight tick instead of
            // treating every one-second gauge as an independent clock.
            expect(Math.abs(ticks - (second + 1) * 30),
                'sustained targeting fell behind the logic clock').toBeLessThanOrEqual(5);
            expect(dropped, 'sustained targeting overflowed the tick queue').toBe(0);
            // fps is a stable direct-page ABI (see the boot test below).
            // Sampling each 60 fields observes each published gauge once.
            const fps = u16(0x7E0000);
            timeline.push(`${tps}/${dropped}/${fps}`);
            rendered += fps;
            if ((second + 1) % 5 === 0) {
                expect(rendered, 'sustained targeting starved the renderer').toBeGreaterThan(0);
                rendered = 0;
            }
        }
        const ticks = (u16(sym('ol_ticks')) - ticksBefore) & 0xFFFF;
        const shots = (u16(sym('olWeaponShots')) - shotsBefore) & 0xFFFF;
        const damage = healthBefore - health();
        // NMI issues exactly 300 ticks in 600 fields. A snapshot may have
        // up to four queued ticks (main.asm's cap) and one tick in flight.
        // The same bound is enforced at every second above, with no drops.
        expect(ticks).toBeGreaterThanOrEqual(295);
        expect(ticks).toBeLessThanOrEqual(305);
        expect(shots).toBeGreaterThanOrEqual(58);
        expect(shots).toBeLessThanOrEqual(62);
        expect(Math.abs(damage - shots), 'the clock scene fired misses instead of aiming').toBeLessThanOrEqual(1);
        console.log(`[openlara] combat clock: ${ticks} ticks, ${shots} shots, ${damage} damage; ticks/dropped/fps ${timeline.join(' ')}`);
    });
});

describe.skipIf(!FX3)('OpenLara FX3: the four guns', () => {
    // src/fixed/lara.h with upstream's inventory.init(): the pistols, magnums,
    // uzis and shotgun by weaponParams; weaponUpdateState's change of gun
    // through a holster; the shotgun's draw/aim/fire/holster, one animation
    // on both arms, with its six pellets; the meshes meshSwapPistols and
    // meshSwapShotgun leave her in, and the shotgun's extra torso turn, as
    // the GSU is handed them (weapon_kit.c Weapon_publish, GS_KIT).
    it('changes guns through the holster and fires each as upstream does', () => {
        const { memory, cpu, w16, u16, call, blob, item, hp, tick, place, reset, phases } = weaponHarness();
        const weapon = JSON.parse(readFileSync(join(DATA_DIR, 'WEAPON.json'), 'utf8'));
        const torsoTable: number[] = weapon.kit.torso;
        const [aimLast, drawLast] = weapon.kit.shotgun.map((a: { last: number }) => a.last);
        const kit = () => Array.from({ length: 5 }, (_, i) => u16(sym('olWeaponKit') + i * 2));
        const draw = () => {
            call('Weapon_publish');
            return Array.from({ length: 24 }, (_, i) => memory.read(sym('olWeaponKitDraw') + i));
        };
        // her meshes: thigh L, thigh R, torso, hand R, hand L, head
        const nodes = () => { const d = draw(); return [d[1], d[4], d[7], d[10], d[13], d[14]]; };
        const torso = () => { const d = draw(); return [0, 1, 2].map(i => ((d[16 + i * 2] | d[17 + i * 2] << 8) << 16) >> 16); };
        const state = () => u16(sym('olLaraWeaponState'));
        // What a tick called: Sound_playItem's ids, rand_logic's count.
        const traced = (keys: number) => {
            const sound = sym('Sound_playItem'), rand = sym('rand_logic');
            const out = { sounds: [] as number[], rands: 0 };
            const step = cpu.step;
            cpu.step = function (this: typeof cpu) {
                const pc = this.PB << 16 | this.PC;
                if (pc === sound) out.sounds.push(u16(this.S + 4));
                else if (pc === rand) out.rands++;
                return step.call(this);
            };
            try { tick(keys); } finally { cpu.step = step; }
            return out;
        };
        const settle = (what: () => boolean, keys = 0, limit = 80) => {
            for (let i = 0; i < limit && !what(); i++) tick(keys);
            expect(what(), 'the guns never settled').toBe(true);
        };
        // Current-frame boxes fall back to the union (contact_fx3.asm) with
        // a two-animation directory whose frame table is empty.
        const setup = () => {
            reset();
            w16(sym('olHostDirectory') + 20, 2);
            for (let i = 0; i < 4; i++) w16(blob + 0xA18 + i * 2, 0);
        };

        // Weapon_init: the pistols on her thighs, the shotgun on her back.
        setup();
        call('Weapon_init');
        expect(kit(), 'Lara::init: the pistols, the shotgun on her back').toEqual([0, 0, 0, 0, 0]);
        expect(nodes()).toEqual([5, 6, 18, 3, 4, 21]);
        expect(draw().slice(22, 24), 'the pistols\' flash, 155 down the hand').toEqual([155, 0]);

        // The fixture's pistols are out; the ring chooses the shotgun.
        setup();
        expect(nodes(), 'pistols drawn: her own thighs, the pistols in her hands').toEqual([1, 2, 18, 7, 8, 21]);
        call('Weapon_change', [3]);
        expect(kit().slice(0, 2), 'a new goal leaves the gun in her hands').toEqual([0, 3]);
        tick(0);
        expect(state(), 'a new goal holsters the gun in her hands').toBe(3);
        settle(() => state() === 0);
        expect(kit(), 'the pistols holstered, the shotgun still to come').toEqual([0, 3, 0, 0, 0]);
        expect(nodes()).toEqual([5, 6, 18, 3, 4, 21]);
        tick(0);
        expect([state(), kit()[0]], 'FREE takes the goal and draws it').toEqual([2, 3]);
        expect(phases(), 'ANIM_SHOTGUN_DRAW, both arms').toEqual([[5, 1], [5, 1]]);
        let sounds: number[] = [];
        for (let f = 2; f <= drawLast; f++) {
            const t = traced(0);
            sounds = sounds.concat(t.sounds);
            expect(phases()[0], `draw frame ${f}`).toEqual([5, f]);
            expect(kit()[3], `meshSwapShotgun at frame 10 (frame ${f})`).toBe(f >= 10 ? 1 : 0);
            expect(torso(), `the shotgun's torso turn at draw frame ${f}`)
                .toEqual(torsoTable.slice((aimLast + 1 + f) * 3, (aimLast + 2 + f) * 3));
        }
        expect(sounds, 'SND_DRAW as it leaves her back').toEqual([6]);
        expect(state(), 'the draw ends ready').toBe(4);
        expect(nodes(), 'the shotgun in her hands, off her back').toEqual([5, 6, 17, 19, 20, 21]);

        // Held Action: aim up, then the blast -- six pellets, two draws each.
        const shots0 = u16(sym('olWeaponShots'));
        tick();
        expect(phases()[0], 'aiming leaves the draw at AIM frame 1').toEqual([4, 1]);
        for (let f = 2; f <= aimLast; f++) tick();
        expect(phases()[0]).toEqual([4, aimLast]);
        let fire = traced(0x20);
        expect(fire.rands, 'six pellets, two rand_logic draws each').toBe(12);
        expect(u16(sym('olWeaponShots')) - shots0).toBe(1);
        expect(hp(1), 'four a pellet: dead by the second').toBe(0);
        // (a ricochet's SND_RICOCHET only where the effects pool is live)
        expect(fire.sounds.filter(id => id !== 10), 'the flinch, then the shotgun').toEqual([20, 45]);
        expect(phases()[0]).toEqual([6, 1]);
        sounds = [];
        let ticks = 0;
        while (u16(sym('olWeaponShots')) - shots0 < 2 && ticks < 60) {
            const t = traced(0x20);
            sounds = sounds.concat(t.sounds);
            ticks++;
        }
        expect(ticks, 'the shotgun\'s reload: 26 ticks a shot').toBe(26);
        expect(sounds[0], 'SND_SHOTGUN_RELOAD at fire frame 10').toBe(9);
        expect(hp(2), 'the second blast takes the next wolf').toBe(0);

        // Pellets at range: each passes its box or not, and only those hit.
        setup();
        place(1, 4608, 3072, 2560 + 8000); w16(item(1, ITEM.health), 1000);
        w16(item(2, ITEM.health), 0);
        [3, 3, 0, 1, 0].forEach((v, i) => w16(sym('olWeaponKit') + i * 2, v));
        call('Weapon_restore');
        for (const off of [0, 12]) [4, aimLast, 0, 0, 0, 0].forEach((v, i) => w16(sym('olWeaponArms') + off + i * 2, v));
        let seed = (u16(sym('olRandSeed')) | u16(sym('olRandSeed') + 2) << 16) >>> 0;
        const rand = () => {
            seed = (Math.imul(seed, 0x41C64E6D) + 0x3039) >>> 0;
            return (seed >>> 10) & 0x7FFF;
        };
        const length = (a: number, b: number) => {
            let x = Math.abs(a), y = Math.abs(b);
            if (x < y) [x, y] = [y, x];
            return x + ((y * 3) >> 3);
        };
        // d: 8000 ahead and 12 above her shoulder; R: (256 + 768) / 4
        const d = length(length(0, 8000), -12) >> 5;
        let hits = 0;
        for (let i = 0; i < 6; i++) {
            const ax = ((rand() - 0x4000) * 3640) >> 16, ay = ((rand() - 0x4000) * 3640) >> 16;
            if (d * (length(ax, ay) >> 4) <= 256 * 20) hits++;
        }
        fire = traced(0x20);
        console.log(`[openlara] shotgun at 8000: ${hits} of 6 pellets`);
        expect(fire.rands).toBe(12);
        expect(1000 - hp(1), 'only the pellets inside the box hit').toBe(4 * hits);

        // The magnums: the shotgun goes onto her back at holster frame 10.
        call('Weapon_change', [1]);
        sounds = [];
        for (let i = 0; i < 80 && state() !== 0; i++) sounds = sounds.concat(traced(0).sounds);
        expect([state(), kit()[3]], 'the shotgun holstered').toEqual([0, 0]);
        expect(sounds, 'SND_HOLSTER as it goes onto her back').toEqual([7]);
        expect(nodes()).toEqual([5, 6, 18, 3, 4, 21]);
        settle(() => state() === 4);
        expect(nodes(), 'the magnums in her hands').toEqual([1, 2, 18, 11, 12, 21]);
        place(1, 4608, 3072, 2860); w16(item(2, ITEM.health), 0);
        for (let i = 0; i < 8 && hp(1) === 6; i++) fire = traced(0x20);
        expect(hp(1), 'two a magnum shot, both arms').toBe(2);
        expect(fire.sounds.filter(id => id === 44).length, 'SND_MAGNUMS_SHOT per arm').toBe(2);
        expect(nodes()[5], 'the uzis\' head while Action is held').toBe(22);

        // The uzis: the magnums' holsters stay on her thighs.
        call('Weapon_change', [2]);
        settle(() => state() === 0);
        expect([kit()[2], kit()[4]], 'the magnums on her thighs, her own head back').toEqual([1, 0]);
        expect(nodes()).toEqual([9, 10, 18, 3, 4, 21]);
        settle(() => state() === 4);
        expect(nodes(), 'the uzis in her hands').toEqual([1, 2, 18, 15, 16, 21]);
        expect(draw().slice(22, 24), 'the uzis\' flash, 180 down the hand').toEqual([180, 0]);
        place(1, 4608, 3072, 2860); w16(item(1, ITEM.health), 1000);
        const before = u16(sym('olWeaponShots'));
        let first = -1;
        for (let i = 0; i < 24; i++) {
            const n = u16(sym('olWeaponShots'));
            tick();
            if (first < 0 && u16(sym('olWeaponShots')) > n) {
                first = i;
                expect(u16(sym('olWeaponFlash')) - u16(sym('ol_ticks')), 'the uzis\' flash: two ticks').toBe(3);
            }
        }
        // reloadTimer 3: a volley every fourth tick, both arms
        expect(u16(sym('olWeaponShots')) - before, 'the uzis\' cadence').toBe(2 * Math.ceil((24 - first) / 4));

        // Choosing the holstered gun draws it (changeWeapon's WEAPON_NONE).
        call('Weapon_change', [0]);
        settle(() => state() === 0);
        settle(() => state() === 4);
        expect(kit()[0]).toBe(0);
        tick(0x80);
        settle(() => state() === 0);
        call('Weapon_change', [0]);
        expect(kit().slice(0, 2), 'the same gun again: none, to be drawn').toEqual([4, 0]);
        tick(0);
        expect([state(), kit()[0]]).toEqual([2, 0]);
    });
});

describe('OpenLara SNES/Super FX', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('boots and renders', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);
        expect(existsSync(EXPECT), `expected frame not found: ${EXPECT}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));

        // Field by field, counting game updates. The `fps` variable in the ROM
        // is whole frames per 60 fields, which at this frame rate reads 1 or 0
        // and cannot tell two builds apart; this can.
        //
        // ol_ticks rather than the host's fbShow, because it is a C global and
        // so has a name in the link's symbol file. fbShow lives in a
        // RAMSECTION, which wlalink does not export, and the hard-coded
        // address this used to read went stale the moment a variable was
        // added above it -- silently, since a stale address still reads
        // *something*. The same trap the C globals fell into once already.
        // Hold every entity pose still, while leaving the fixed logic clock
        // alone: the clock is now the thing this test uses to prove gameplay
        // advances at 30 Hz even though rendering is much slower.
        const memory = emulator.getMemory();
        const ticksAddr = sym('ol_ticks');
        const freezeAddr = sym('ol_animFreeze');
        let ticks = 0;
        // Sample before the first field as well as after every field.  The
        // old `if (i)` discarded a real tick whenever boot completed during
        // field zero, then divided by all FRAMES fields; in a ten-second run
        // that turns 29.0 Hz worth of completed work into a 28.9 Hz report.
        let lastTick = memory.read(ticksAddr) |
                       (memory.read(ticksAddr + 1) << 8);
        const gauge = (name: string) =>
            memory.read(sym(name)) | (memory.read(sym(name) + 1) << 8);
        const timeline: string[] = [];
        let clockBase = 0;
        // The clock is judged from its first tick: the boot is a sound
        // upload through the IPL protocol (63 KB at about 48 KB/s) and the
        // level's staging, and neither is game time. Its length is reported.
        let firstTickField = -1;
        let baseField = -1;
        for (let i = 0; i < FRAMES; i++) {
            emulator.executeFrames(1);
            const t = memory.read(ticksAddr) | (memory.read(ticksAddr + 1) << 8);
            ticks += (t - lastTick) & 0xFFFF;
            lastTick = t;
            if (ticks > 0 && firstTickField < 0) firstTickField = i + 1;
            // One entry per second of the ROM's own gauges, so a dropped tick
            // can be placed: at the level's arrival, or somewhere it should
            // not be.
            if ((i + 1) % 60 === 0) {
                timeline.push(`${gauge('ol_tps')}/${gauge('ol_ticksDropped')}` +
                              `/${gauge('ol_tickFields')}`);
                // Allow level staging a second past the first tick to finish.
                // Thereafter bound cumulative drift by the four-entry NMI
                // queue plus one tick in flight. A frame handoff can split
                // completed ticks 28/32 across adjacent gauge windows.
                if (baseField < 0 && firstTickField >= 0 && i + 1 >= firstTickField + 60) {
                    baseField = i + 1;
                    clockBase = ticks;
                } else if (baseField >= 0) {
                    expect(Math.abs(ticks - clockBase - (i + 1 - baseField) / 2),
                        'the opening scene fell behind the logic clock').toBeLessThanOrEqual(5);
                }
            }

            // And hold every entity on one animation frame, through the
            // game's own freeze rather than by writing over its state. The
            // two renderer implementations cannot agree byte for byte while
            // entity state advances on one side and stands still on the
            // other, and reaching in to pin the state does not work: the host
            // fills its array in one step and stages it in another, and a
            // write from out here lands on whichever side of that seam the
            // field boundary happens to fall. Asking the game to hold still
            // has no such window.
            memory.write(freezeAddr, (TICK + 1) & 0xFF);
            memory.write(freezeAddr + 1, ((TICK + 1) >> 8) & 0xFF);
        }
        const logicHz = ticks / (FRAMES / 60);

        // The ROM's own gauges, published at every 60-field rollover: ticks
        // per second, ticks the backlog cap dropped, and what a tick costs
        // the renderer -- field boundaries straddled per tick, which is the
        // fraction of a field the 65816 holds the RAM bus for each one.
        const u16g = (name: string) =>
            memory.read(sym(name)) | (memory.read(sym(name) + 1) << 8);
        const tps = u16g('ol_tps');
        const dropped = u16g('ol_ticksDropped');
        const tickCost = tps ? u16g('ol_tickFields') / tps : 0;
        console.log(`[openlara] clock per second (ticks/dropped/fields): ` +
                    timeline.join(' '));
        expect(dropped, 'the NMI backlog cap dropped ticks in the opening scene')
            .toBe(0);

        const fb = emulator.getPPU().getFramebuffer();
        const { stride, width, height } = emulator.getPPU().getFrameGeometry();

        const gsu = emulator.getSuperFX().getDiagnostics();
        if (MODE7 || FX3) {
            // The opaque writer keeps the source-zero marker in bit 7.
            // All possible marker + shade indices must still display black.
            // The shade is ORed in as row << 4 (raster.gsu: AND #$70), so
            // in FX3's 8bpp frame those are colour 0 of each sprite
            // palette; colours 1-15 of palettes 0 and 1 are the HUD's.
            for (let i = 128; i < 256; i += (FX3 ? 16 : 1)) {
                expect(memory.readCGRAM(i * 2) | (memory.readCGRAM(i * 2 + 1) << 8),
                       `opaque source-zero palette entry ${i}`).toBe(0);
            }
        }
        // main.asm's direct page. These two sit at the top of its .ENUM
        // precisely so these constants can be relied on: a .ENUM's fields are
        // positional and wlalink exports none of them, so there is nothing to
        // resolve by name. See the comment on the block in main.asm.
        const fps = memory.read(0x7e0000) | (memory.read(0x7e0001) << 8);
        const fbShow = memory.read(0x7e0002);

        // Which Super FX die the cartridge is being run on, from both ends:
        // what the emulator decided from the header, and what the ROM itself
        // read back out of VCR at boot. This port needs the linear $40-$5F
        // ROM image, which the original MARIO chip does not decode, so a
        // version code of 1 here means the ROM should not have booted at all.
        const gsuVersion = memory.read(sym('ol_gsuVersion'));

        // Every mutable C global must live in WRAM. This caught an initialized
        // `.data` orphan at $04:0000, which aliases the host's direct-page FPS
        // counter and let NMI rewrite the active-item-list head once a second.
        expect(sym('olFirstActive') >>> 16,
               'olFirstActive did not link into WRAM').toBe(0x7e);

        if (FX3) {
            for (const [name, bank] of [['gameInit', 0x84], ['Lara_update', 0x85],
                ['phd_sin', 0x87], ['olSinTable', 0x87], ['Weapon_animate', 0x87],
                ['Enemy_target', 0x87], ['Wolf_update', 0x85], ['Nav_getWaypoint', 0x87], ['navSectorBox', 0x89],
                ['Enemy_checkZone', 0x87], ['Enemy_randomBox', 0x87], ['Enemy_moodDistance', 0x87], ['rand_logic', 0x87],
                ['Enemy_checkStalk', 0x87], ['traceX', 0x85], ['traceZ', 0x85],
                ['camQuery', 0x85], ['Camera_getLocationForAngle_xz', 0x85],
                ['Enemy_targetMood', 0x87], ['Enemy_moodChoose', 0x87], ['Bear_update', 0x88], ['Bear_state', 0x88],
                ['Raptor_update', 0x88], ['Raptor_state', 0x88], ['Raptor_hit', 0x88],
                ['CollisionInfo_setAngle', 0x89], ['CollisionInfo_setSide', 0x89], ['colPoint', 0x89],
                ['ItemObj_collideBounds', 0x89], ['gameRenderSync', 0x89],
                ['Lara_itemFloorAt', 0x89]] as const)
                expect(sym(name) >>> 16, `${name} retained a GSU-2 WRAM constraint`).toBe(bank);
            // Only the linked Nav and room-cache allocations may occupy
            // former code space. Check every gap too, so an unintended code
            // copy cannot hide between separate data allocations.
            const allocations = ['.bss.c_nav_fx3', 'room_query_cache']
                .filter(name => SYMS.has(`SECTIONSTART_${name}`))
                .map(name => [sym(`SECTIONSTART_${name}`), sym(`SECTIONEND_${name}`)])
                .sort((a, b) => a[0] - b[0]);
            const unused = [[0x7E0600, 0x7E1C00]];
            let free = 0x7E5800;
            for (const [start, end] of allocations) {
                expect(start, 'FX3 data overlaps an earlier allocation').toBeGreaterThanOrEqual(free);
                expect(end).toBeGreaterThanOrEqual(start);
                expect(end).toBeLessThanOrEqual(0x7F0000);
                unused.push([free, start]); free = end;
            }
            unused.push([free, 0x7F0000]);
            for (const [start, end] of unused) {
                let dirty = 0;
                for (let at = start; at < end; at++) dirty |= memory.read(at);
                expect(dirty, `FX3 still writes the retired image at ${start.toString(16)}`).toBe(0);
            }
        } else {
            // GSU-2 must preserve its full relocated code/table image.
            const lowImage = Buffer.alloc(0x1600);
            for (let i = 0; i < lowImage.length; i++) lowImage[i] = memory.read(0x7E0600 + i);
            expect(lowImage.equals(readFileSync(ROM).subarray(61 * 0x8000, 61 * 0x8000 + 0x1600)),
                   'the relocated WRAM code image was not preserved').toBe(true);
        }
        expect(sym('HostCodeEnd') & 65535).toBeLessThanOrEqual(0x0600);

        // What src/game.c handed the GSU this frame. Addresses come from the
        // link's symbol file (`wlalink -s`); when the picture disagrees with
        // the interpreter, the first question is always whether the two were
        // even asked to render the same view.
        const s16 = (a: number) => {
            const v = memory.read(a) | (memory.read(a + 1) << 8);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        // The C globals the GSU stages into over the cartridge bus. Reading
        // them back proves the round trip, and they are the first thing to
        // look at when the picture and the interpreter disagree.
        const handover = `room=${s16(sym('ol_room'))} ` +
                         `cam=(${s16(sym('ol_camX'))},${s16(sym('ol_camY'))},` +
                         `${s16(sym('ol_camZ'))}) tgtY=${s16(sym('ol_camTgtY'))} ` +
                         `angle=${s16(sym('ol_angle'))} floor=${s16(sym('ol_floor'))}`;
        let vramFrame: Buffer;
        if (MODE7) {
            // Mode 7 character n is the high bytes of VRAM words n*64..: buffer
            // 0 is characters 0-127, buffer 1 characters 128-255.
            const wordBase = fbShow ? 0x2000 : 0x0000;
            vramFrame = Buffer.alloc(0x2000);
            for (let i = 0; i < 0x2000; i++) {
                vramFrame[i] = memory.readVRAM((wordBase + i) * 2 + 1);
            }
        } else {
            const size = FX3 ? 0x7980 : 0x5000;
            const vramBase = fbShow ? (FX3 ? 0x7980 : 0x6000) : 0x0000;
            vramFrame = Buffer.alloc(size);
            for (let i = 0; i < size; i++) {
                vramFrame[i] = memory.readVRAM(vramBase + i);
            }
        }
        const expected = readFileSync(EXPECT);

        expect(width).toBe(256);
        expect(height).toBe(224);
        expect(gsu.lastError).toBeNull();

        // The port depends on GSU-2: the linear ROM image (GSU-1 and up), and
        // the 2 MB ROM / 128 KB RAM windows only the 112-pin part can address.
        // The header has no field for the die, so this is what pins it down.
        expect(gsu.revision, `expected a GSU-2 class die, got ${gsu.revisionName}`)
            .toMatch(FX3 ? /^fx3$/ : /^gsu-2/);
        if (FX3) {
            expect(gsuVersion).toBe(0x52);
            expect(gsu.cfgr & 0x80, 'FX3 must not depend on STOP IRQ').toBe(0x80);
            expect(gsu.scmr).toBe(0x1f);
            expect(gsu.scbr).toBe(0x40);
            expect(sym('gameUpdate') >>> 16).toBe(0x84);
            expect(sym('Lara_update') >>> 16).toBe(0x85);
            const rom = readFileSync(ROM);
            expect(rom[0x7fd5]).toBe(0x30);
            expect(rom[0x7fd6], 'FX3 with a battery (the save, src/save.c)').toBe(0x18);
            expect(rom.length).toBe(0x400000);
            expect(rom[0x7fd7]).toBe(0x0C);
            // Collision/animation data must survive repeated C2P clears.
            // Read-only tables stay in FastROM; writable sectors alone use
            // $7F, with $71 belonging to the FX3 firmware's packed output.
            // The level's host tables: where its level-table entry says.
            const hostFile = (memory.read(sym('olLevel')) - 0xC0) << 16, hostCpu = 0xC00000 + hostFile;
            const size = rom.readUInt32LE(hostFile + 8);
            const hostImage = Buffer.from(Array.from({ length: size }, (_, i) => memory.read(hostCpu + i)));
            expect(hostImage.equals(rom.subarray(hostFile, hostFile + size))).toBe(true);
            for (const [i, name] of HOST_TABLES.entries()) {
                const descriptor = hostFile + 40 + i * 12;
                const source = hostCpu + rom.readUInt32LE(descriptor);
                const length = rom.readUInt32LE(descriptor + 4);
                expect(hostTable(memory, sym, name), name).toBe(name === 'Sector' ? 0x7F0000 : source);
                if (name === 'Sector') {
                    // Floors move under blocks; a door's two sectors are
                    // walls while it is shut (door.c) and the level's open.
                    const doors = new Map<number, number[]>();
                    const count = memory.read(sym('itemsCount')) | memory.read(sym('itemsCount') + 1) << 8;
                    for (let i = 1; i < count; i++) {
                        const type = memory.read(sym('items') + i * ITEM_SIZE);
                        if (type < DOOR_FIRST || type > DOOR_LAST) continue;
                        const open = memory.read(sym('olDoorOpen') + i);
                        for (const d of doorSectors(memory, sym, i)) doors.set(d.s, open ? d.level : DOOR_SHUT);
                    }
                    expect(doors.size, 'no door shut a sector').toBeGreaterThan(0);
                    for (let k = 0; k < length; k++) {
                        const door = doors.get(k >> 2);
                        if (door) expect(memory.read(0x7F0000 + k), `door sector byte ${k}`).toBe(door[k & 3]);
                        else if (k % 4 !== 0)
                            expect(memory.read(0x7F0000 + k), `sector byte ${k}`).toBe(memory.read(source + k));
                    }
                } else if (length) {
                    const before = memory.read(source);
                    memory.write(source, before ^ 255);
                    expect(memory.read(source), `CPU wrote read-only ${name}`).toBe(before);
                }
            }
            // Both packed images must use the right tile indices, including
            // the shared blank tile outside the 27x18 frame.
            for (let y = 0; y < 32; y++) {
                for (let x = 0; x < 32; x++) {
                    const a = 0xf800 + (y * 32 + x) * 2;
                    const tile = memory.readVRAM(a) | (memory.readVRAM(a + 1) << 8);
                    expect(tile).toBe(x < 27 && y < 18 ? fbShow * 486 + x * 18 + y : 972);
                }
            }
        }
        expect(gsuVersion, 'ROM read a MARIO-chip version code out of VCR').not.toBe(1);
        expect(expected.length).toBe(vramFrame.length);
        // A tick may be in flight when the last field boundary is sampled.
        // Judge the discrete completion count with one tick of endpoint
        // quantisation instead of rounding the average up to a hard 29.0 Hz.
        // Cumulative drift and dropped ticks above still catch a clock
        // that actually falls behind after startup.
        expect(firstTickField, 'the game never reached its first tick').toBeGreaterThan(0);
        console.log(`[openlara] boot: first tick at field ${firstTickField}` +
                    ` (${(firstTickField / 60).toFixed(2)} s)`);
        expect(firstTickField, 'the boot took longer than three seconds').toBeLessThanOrEqual(180);
        const minTicks = Math.max(0, Math.ceil(29 * (FRAMES - firstTickField) / 60) - 1);
        expect(ticks, 'game logic is not advancing at a fixed 30 Hz')
            .toBeGreaterThanOrEqual(minTicks);
        expect(logicHz, 'game logic is running faster than 30 Hz')
            .toBeLessThanOrEqual(31);

        // "differs" on its own is a poor bug report for 20 KB of bitplanes.
        const diff = (a: Buffer, b: Buffer) => {
            let first = -1, n = 0;
            for (let i = 0; i < a.length; i++) {
                if (a[i] !== b[i]) { if (first < 0) first = i; n++; }
            }
            return first < 0 ? 'identical'
                : `${n}/${a.length} bytes differ, first at ${first} ` +
                  `(got ${a[first].toString(16)}, want ${b[first].toString(16)})`;
        };

        // Written before the comparison, so that a frame which differs is
        // there to be looked at next to the interpreter's; that is how the
        // LEVEL2 boot's one hundred differing bytes get found.
        writeFileSync(OUT, png(width, height, (x, y) => fb[y * stride + x]));

        expect(vramFrame.equals(expected),
               `uploaded VRAM differs from the standalone GSU render: ` +
               `${diff(vramFrame, expected)}; fbShow=${fbShow} ${handover}`).toBe(true);

        if (FX3) {
            // Check the PPU interpretation too: byte equality alone cannot
            // catch a wrong BG mode, scroll offset, palette or bitplane order.
            let wrongPixels = 0;
            for (let y = 0; y < 224; y++) {
                for (let x = 0; x < 256; x++) {
                    const sx = x - 20, sy = y - 40;
                    let index = 0;
                    if (sx >= 0 && sx < 216 && sy >= 0 && sy < 144) {
                        const tile = (sx >> 3) * 18 + (sy >> 3);
                        const a = tile * 64 + (sy & 7) * 2;
                        for (let p = 0; p < 8; p++) {
                            index |= ((expected[a + (p >> 1) * 16 + (p & 1)] >> (7 - (sx & 7))) & 1) << p;
                        }
                    }
                    const c = memory.readCGRAM(index * 2) | (memory.readCGRAM(index * 2 + 1) << 8);
                    const pixel = fb[y * stride + x];
                    if (((pixel & 255) >> 3) !== (c & 31) ||
                        (((pixel >>> 8) & 255) >> 3) !== ((c >> 5) & 31) ||
                        (((pixel >>> 16) & 255) >> 3) !== ((c >> 10) & 31)) wrongPixels++;
                }
            }
            expect(wrongPixels, 'FX3 PPU image is not the centred 216x144 render').toBe(0);
        }

        // The shade map is half the picture -- the frame buffer carries texel
        // indices and the tile map carries which sixteen colours they mean --
        // so comparing only the frame buffer would let the whole shade path
        // rot without a test noticing.
        let shadeInfo = '';
        if (!MODE7 && !FX3 && EXPECT_SHADE && existsSync(EXPECT_SHADE) &&
            readFileSync(EXPECT_SHADE).length > 0) {
            const expectedShade = readFileSync(EXPECT_SHADE);
            const vramShade = Buffer.alloc(expectedShade.length);
            for (let i = 0; i < expectedShade.length; i++) {
                vramShade[i] = memory.readVRAM(VRAM_TILEMAP + i * 2 + 1);
            }
            expect(vramShade.equals(expectedShade),
                   `uploaded tile map attributes differ from the GSU shade map: ` +
                   `${diff(vramShade, expectedShade)}`).toBe(true);
            const rows = new Set<number>();
            for (const b of vramShade) rows.add((b >> 2) & 7);
            shadeInfo = `  shadeRows=${[...rows].sort().join(',')}`;
        }

        let nonBlack = 0;
        const colours = new Set<number>();
        for (let y = 0; y < height; y++) {
            for (let x = 0; x < width; x++) {
                const px = fb[y * stride + x] >>> 0;
                colours.add(px);
                if ((px & 0x00ffffff) !== 0) nonBlack++;
            }
        }

        expect(nonBlack).toBeGreaterThan(0);

        // The stack has $7E:1C00-$1FFF -- measured 421 bytes deep at its
        // deepest over a long scene -- and the fifth C image sits directly
        // below it. Boot zeroed WRAM; sixty-four bytes still zero above the
        // floor say the stack stayed well clear of it.
        for (let a = 0x7E1C00; a < 0x7E1C40; a++) {
            expect(memory.read(a), `the stack reached $7E:${(a & 0xFFFF).toString(16)}`).toBe(0);
        }
        expect(colours.size).toBeGreaterThan(1);

        console.log(`[openlara] ${width}x${height}  ` +
                    `${gsu.revisionName} (VCR=${gsuVersion})  ${handover}  ` +
                    `fields=${FRAMES}  logic=${logicHz.toFixed(2)} Hz  ` +
                    `ticks=${tps}/s dropped=${dropped} ` +
                    `tickCost=${tickCost.toFixed(2)} fields  ` +
                    `render=${fps} fps (ROM counter)  ` +
                    `nonBlack=${nonBlack}/${width * height}  ` +
                    `colours=${colours.size}  ` +
                    `frameCRC=${(crc32(vramFrame) >>> 0).toString(16).padStart(8, '0')}` +
                    `${shadeInfo}  GSU=VRAM  -> ${OUT}`);
    }, 600_000);
});

describe('OpenLara SNES/Super FX: sector queries', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('resolves arbitrary sloped points and horizontal portals', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const s16 = (a: number) => { const v = u16(a); return v & 0x8000 ? v - 0x10000 : v; };
        const w8 = (a: number, v: number) => memory.write(a, v & 0xFF);
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };

        for (let i = 0; i < 120; i++) emulator.executeFrames(1);
        expect(u16(sym('itemsCount')), 'the host sector table never arrived')
            .toBeGreaterThan(0);

        // ol_ticks is published as the last thing a tick does, so a change
        // in it says a whole tick's results are in. But the first tick to
        // complete after the pin may have *started* before it, mid-field,
        // and queried the old position; only the second completion is a tick
        // that began after the pin was written. Wait for that one.
        const sample = (room: number, x: number, y: number, z: number) => {
            let tick = u16(sym('ol_ticks'));
            let completed = 0;
            for (let fields = 0; fields < 60; fields++) {
                w8(sym('items') + ITEM.room, room);
                w8(sym('items') + ITEM.state, 2);
                w8(sym('items') + ITEM.goalState, 2);
                w16(sym('items') + ITEM.x, x);
                w16(sym('items') + ITEM.y, y);
                w16(sym('items') + ITEM.z, z);
                w16(sym('items') + ITEM.animIndex, 103);
                w8(sym('items') + ITEM.frameIndex, 0);
                w8(sym('items') + ITEM.flags2, 0);
                w16(sym('items') + ITEM.vSpeed, 0);
                w16(sym('items') + ITEM.hSpeed, 0);
                emulator.executeFrames(1);
                const now = u16(sym('ol_ticks'));
                completed += (now - tick) & 0xFFFF;
                tick = now;
                if (completed >= 2) {
                    return { room: u16(sym('ol_room')),
                             floor: s16(sym('ol_floor')),
                             ceiling: s16(sym('ol_ceiling')),
                             trigger: u16(sym('ol_trigger')) };
                }
            }
            throw new Error('no fixed logic tick completed on the pinned position');
        };

        // Room 0 sector (1,3): floor base 7*256, slant (-2,+2),
        // ceiling base 5*256, slant (+2,+2). Two asymmetric interior points
        // prove this is a point query rather than one height per sector.
        const a = sample(0, 7168 + 256, 2000, 5120 + 768);
        const b = sample(0, 7168 + 768, 2000, 5120 + 256);
        expect(a.floor).toBe(2048);
        expect(b.floor).toBe(2560);
        expect(a.ceiling).toBe(1024);
        expect(b.ceiling).toBe(512);

        // This room-0 border sector carries a FloorData portal to room 6 and
        // room 6's sector owns trigger 4. The hint deliberately starts on the
        // other side of the portal.
        const portal = sample(0, 7680, 2560, 22016);
        expect(portal.room, 'the horizontal portal was not followed').toBe(6);
        // FX3 also keeps the eight look-at-only triggers (CAMERA_TARGET), which
        // come first in LEVEL1's list: the same sector's trigger is 12 there.
        expect(portal.trigger, 'the destination sector trigger was lost').toBe(FX3 ? 12 : 4);

        console.log(`[openlara] query: slope ${a.floor}/${a.ceiling} -> ` +
                    `${b.floor}/${b.ceiling}, portal room ${portal.room} ` +
                    `trigger ${portal.trigger}`);
    });
});

describe('OpenLara SNES/Super FX: triggers', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('opens what standing on a trigger names', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const itemAddr = (i: number, f: number) =>
            sym('items') + i * ITEM_SIZE + f;
        const itemB = (i: number, f: number) =>
            u8(itemAddr(i, f));
        const itemW = (i: number, f: number) =>
            u16(itemAddr(i, f));
        const sItemW = (i: number, f: number) => {
            const v = itemW(i, f);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const s16 = (a: number) => {
            const v = u16(a);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };
        const atGameTick = (setup: () => void) => {
            // A field boundary can interrupt Room_query/Lara_updateRoom.
            // Teleporting then lets the in-flight query restore the OLD room
            // after the fixture wrote the new coordinates. Mutate only at a
            // real gameUpdate entry, like the live creature timing fixture.
            const cpu = emulator.getCPU(), original = cpu.step;
            let applied = false;
            cpu.step = function () {
                if (!applied && (cpu.PB << 16 | cpu.PC) === sym('gameUpdate')) {
                    setup(); applied = true;
                }
                return original.call(this);
            };
            try {
                for (let i = 0; i < 60 && !applied; i++) emulator.executeFrames(1);
            } finally { cpu.step = original; }
            expect(applied, 'the trigger fixture never reached a safe setup boundary').toBe(true);
        };
        // Let it boot far enough for the GSU to have staged the entity list
        // and the host to have parsed it.
        for (let i = 0; i < 120; i++) emulator.executeFrames(1);
        expect(u16(sym('itemsCount')), 'no entity list to trigger against')
            .toBeGreaterThan(0);

        // Room 6 of LEVEL1 has four pad triggers in a row, each naming one
        // entity. This is the first of them, at the middle of its sector, and
        // the numbers are facts about the level rather than about the port.
        const TRIGGER_ITEM = 5;
        const before = itemB(TRIGGER_ITEM, ITEM.state);

        atGameTick(() => {
            memory.write(sym('items') + ITEM.room, 6);
            w16(sym('items') + ITEM.x, 7680);
            w16(sym('items') + ITEM.y, 2560);
            w16(sym('items') + ITEM.z, 22016);
        });

        // Long enough for the GSU to probe the new position, the host to
        // collect it, and the entity to walk its animation graph across.
        let opened = -1;
        for (let i = 0; i < 900 && opened < 0; i++) {
            emulator.executeFrames(1);
            if (itemB(TRIGGER_ITEM, ITEM.state) === 1) opened = i;
        }

        console.log(`[openlara] trigger: ol_trigger=${u16(sym('ol_trigger'))} ` +
                    `item ${TRIGGER_ITEM} state ${before} -> ` +
                    `${itemB(TRIGGER_ITEM, ITEM.state)} ` +
                    `after ${opened} fields, anim ` +
                    `${itemW(TRIGGER_ITEM, ITEM.animIndex)}`);

        expect(u16(sym('ol_trigger')), 'the sector under the camera has no trigger')
            .toBeLessThan(0xFFFF);
        expect(opened, 'the entity the trigger names never opened')
            .toBeGreaterThanOrEqual(0);


        // This one is a pad with TR's one-tick timer, which is what keeps a
        // dart emitter firing only while you stand on it. Step off and the
        // timer runs out, so it is asked to shut again -- the goal is what to
        // read, since the animation back takes thirty frames and a frame here
        // is half a second.
        atGameTick(() => {
            w16(sym('items') + ITEM.x, 7680);
            w16(sym('items') + ITEM.y, 2560);
            w16(sym('items') + ITEM.z, 22016 + 3072);
        });
        for (let i = 0; i < 300; i++) emulator.executeFrames(1);

        const away = u16(sym('ol_trigger'));
        console.log(`[openlara] stepped off: ol_trigger=${away} ` +
                    `item ${TRIGGER_ITEM} goal ` +
                    `${itemB(TRIGGER_ITEM, ITEM.goalState)} ` +
                    `flags $${itemB(TRIGGER_ITEM, ITEM.flags).toString(16)}`);

        expect(away, 'the camera should have left the trigger sector')
            .not.toBe(4);
        expect(itemB(TRIGGER_ITEM, ITEM.goalState),
               'a one-tick timer should have run out once nothing refreshed it')
            .toBe(0);

        // A trigger with no timer latches instead. Room 2 wakes three bats.
        // Their activation must persist; their goal state is owned by the
        // creature graph (AWAKE -> FLY), not by an open/closed door graph.
        const LATCH_ITEM = 1;
        atGameTick(() => {
            memory.write(sym('items') + ITEM.room, 2);
            w16(sym('items') + ITEM.x, 11776);
            w16(sym('items') + ITEM.y, -768);
            w16(sym('items') + ITEM.z, -25088);
        });
        for (let i = 0; i < 300; i++) emulator.executeFrames(1);
        const onLatch = u16(sym('ol_trigger'));

        atGameTick(() => {
            w16(sym('items') + ITEM.x, 11776);
            w16(sym('items') + ITEM.y, -768);
            w16(sym('items') + ITEM.z, -25088 + 2048);
        });
        for (let i = 0; i < 300; i++) emulator.executeFrames(1);

        console.log(`[openlara] latch: on=${onLatch} off=${u16(sym('ol_trigger'))} ` +
                    `item ${LATCH_ITEM} goal ` +
                    `${itemB(LATCH_ITEM, ITEM.goalState)} ` +
                    `flags $${itemB(LATCH_ITEM, ITEM.flags).toString(16)}`);

        expect(onLatch, 'room 2 should have had a trigger under the camera')
            .toBeLessThan(0xFFFF);
        expect(u16(sym('ol_trigger')), 'Lara never left the latching pad').not.toBe(onLatch);
        expect(itemB(LATCH_ITEM, ITEM.type)).toBe(9);
        expect(itemB(LATCH_ITEM, ITEM.flags) & 0x5F,
               'the bat lost its activation mask after leaving a no-timer trigger').toBe(0x5F);
        expect(itemB(LATCH_ITEM, ITEM.flags2) & ITEM_STATUS).toBe(1);
        expect(itemW(LATCH_ITEM, ITEM.timer)).toBe(0);

        // And a switch, which is the first thing in this port that has to be
        // *used* rather than stood on. Item 10 of LEVEL1 is a wall switch in
        // room 9 facing +Z, wired to door item 9 through a SWITCH-type
        // trigger -- so standing in front of it and pressing B opens the door
        // and pressing B again shuts it, which is what XOR means.
        const SWITCH_ITEM = 10, SWITCH_DOOR = 9;

        // Before it opens, it is solid. Door item 9 stands at
        // (-15872, 7680, -7680) turned a quarter, and its box in its own
        // frame is (x -500..500, z -561..-461). collideBounds rotates *her*
        // into that frame rather than the box into the world, so a world
        // offset of -500 in x maps to -500 in the item's z, which is where
        // the door is: about x -16372, and she carries 128 of her own.
        // Dropped into the middle of that, she has to be pushed out.
        const DOOR_MID_X = -16383;
        atGameTick(() => {
            memory.write(sym('items') + ITEM.room, 9);
            w16(sym('items') + ITEM.x, DOOR_MID_X);
            w16(sym('items') + ITEM.y, 7680);
            w16(sym('items') + ITEM.z, -7680);
            w16(sym('items') + ITEM.angleY, 0x8000);
        });
        for (let i = 0; i < 240; i++) emulator.executeFrames(1);

        const pushedTo = sItemW(0, ITEM.x);
        console.log(`[openlara] shut door: camX ${DOOR_MID_X} -> ${pushedTo}, health ${sItemW(0, ITEM.health)}, ` +
            `state ${itemB(0, ITEM.state)}, y ${sItemW(0, ITEM.y)}, room ${itemB(0, ITEM.room)}`);
        // The door's sectors, as door.c writes them (FX3): walls while shut.
        const doorBytes = () => doorSectors(memory, sym, SWITCH_DOOR).map(d =>
            [0, 1, 2, 3].map(k => u8(0x7F0000 + d.s * 4 + k)));
        if (FX3) {
            // Upstream's door is the wall past it (Door::action), not a box:
            // dropped one unit from that wall, she is put back her radius
            // from it on her own side -- never through it into room 10, as
            // the box's push could take her.
            expect(doorBytes(), 'a shut door should be walls in the sectors')
                .toEqual(doorSectors(memory, sym, SWITCH_DOOR).map(() => DOOR_SHUT));
            expect(itemB(0, ITEM.room), 'she went through the shut door').toBe(9);
            expect(pushedTo, 'she stands in the shut door\'s wall').toBeGreaterThanOrEqual(-16384 + 64);
        } else {
            // A valid push can cross the portal into room 10. Do not require
            // her to remain in room 9; the original displacement is the contract.
            expect(Math.abs(pushedTo - DOOR_MID_X),
                   'a shut door should have pushed the camera out of itself')
                .toBeGreaterThan(100);
        }

        // The switch item sits at the sector's centre with its lever on the
        // wall 475 along its facing (its model box is z 439..511), and
        // upstream's LIMIT_SWITCH wants her 312 to 512 along that facing,
        // facing the same way within thirty degrees: standing at the lever,
        // looking at it. Action then plays her pull, the lever's own
        // animation ends on its KILL, and the SWITCH-type trigger under her
        // feet asks the lever whether it has been pulled and opens the door.
        atGameTick(() => {
            w16(sym('items') + ITEM.x, -15872);
            w16(sym('items') + ITEM.y, 7680);
            w16(sym('items') + ITEM.z, -7680 + 350);
            w16(sym('items') + ITEM.angleY, 0);            // as the switch faces
            w16(sym('olLaraMoveAngle'), 0);
        });
        for (let i = 0; i < 240; i++) emulator.executeFrames(1);

        const doorBefore = itemB(SWITCH_DOOR, ITEM.flags);
        const STATE_SWITCH_DOWN = 40, STATE_SWITCH_UP = 41;

        // Exactly half a turn is -32768 as a signed 16-bit angle. Its
        // absolute value does not fit in 816-tcc's int: the old check left
        // it negative and accepted her facing directly away from the lever.
        const pad = emulator.getController() as unknown as { buttonState: number };
        w16(sym('items') + ITEM.angleY, 0x8000);
        w16(sym('olLaraMoveAngle'), 0x8000);
        pad.buttonState = PAD_B;
        for (let i = 0; i < 60; i++) {
            emulator.executeFrames(1);
            const st = itemB(0, ITEM.state);
            expect(st, 'she pulled the lever while facing directly away')
                .not.toBe(STATE_SWITCH_DOWN);
            expect(st, 'she pulled the lever while facing directly away')
                .not.toBe(STATE_SWITCH_UP);
        }
        pad.buttonState = 0;
        for (let i = 0; i < 12; i++) emulator.executeFrames(1);
        w16(sym('items') + ITEM.angleY, 0);
        w16(sym('olLaraMoveAngle'), 0);

        // Action is released the moment she takes hold: held through the
        // pull and past it, she would pull the lever again as soon as her
        // hands were free, and shut what she had opened -- which is TR.
        pad.buttonState = PAD_B;
        let pulled = -1;
        for (let i = 0; i < 120 && pulled < 0; i++) {
            emulator.executeFrames(1);
            const st = itemB(0, ITEM.state);
            if (st === STATE_SWITCH_DOWN || st === STATE_SWITCH_UP) pulled = st;
        }
        pad.buttonState = 0;
        let doorAfter = itemB(SWITCH_DOOR, ITEM.flags);
        let waited = 0;
        for (; waited < 900 && !(doorAfter & 0x40); waited++) {
            emulator.executeFrames(1);
            doorAfter = itemB(SWITCH_DOOR, ITEM.flags);
        }

        console.log(`[openlara] switch: she pulled in state ${pulled}; door flags ` +
                    `$${doorBefore.toString(16)} -> $${doorAfter.toString(16)} ` +
                    `after ${waited} fields, goal ${itemB(SWITCH_DOOR, ITEM.goalState)}, ` +
                    `switch flags $${itemB(SWITCH_ITEM, ITEM.flags).toString(16)} ` +
                    `status ${itemB(SWITCH_ITEM, ITEM.flags2) & ITEM_STATUS}, ` +
                    `hands ${u16(sym('olLaraWeaponState'))}`);

        // The *mask*, not ITEM_FLAG_ACTIVE. The flag says the entity is on the
        // update list and every entity starts there; what says a door is shut
        // is that not every trigger wired to it has fired, which is upstream's
        // own test in isActive.
        expect(doorBefore & 0x1F, 'the door should have started shut').toBe(0);
        expect(pulled, 'Action at the lever should have put her in a switch state')
            .toBeGreaterThan(0);
        expect(doorAfter & 0x40, 'pulling the switch should have opened the door')
            .not.toBe(0);
        // Her pull's EMPTY command, at the end of its 45 frames, gives her
        // hands back; without it every later pickup and lever would refuse
        // her. The lever opens the door before her pull is over.
        for (let i = 0; i < 300 && itemB(0, ITEM.state) !== 2; i++) emulator.executeFrames(1);
        expect(u16(sym('olLaraWeaponState')), 'her hands are still busy').toBe(0);

        // And once it is open, the same spot is walkable. This is the pair
        // that says the box is switched on and off by the door's own state
        // rather than always or never there.
        for (let i = 0; i < 600; i++) emulator.executeFrames(1);
        expect(itemB(SWITCH_DOOR, ITEM.state),
               'the door should have finished opening').toBe(1);
        if (FX3) {
            expect(u8(sym('olDoorOpen') + SWITCH_DOOR), 'door.c never opened its sectors').toBe(1);
            expect(doorBytes(), 'an open door should give the level its sectors back')
                .toEqual(doorSectors(memory, sym, SWITCH_DOOR).map(d => d.level));
        }

        atGameTick(() => {
            w16(sym('items') + ITEM.x, DOOR_MID_X);
            w16(sym('items') + ITEM.y, 7680);
            w16(sym('items') + ITEM.z, -7680);
        });
        for (let i = 0; i < 240; i++) emulator.executeFrames(1);

        const throughTo = sItemW(0, ITEM.x);
        console.log(`[openlara] open door: camX ${DOOR_MID_X} -> ${throughTo}`);
        expect(Math.abs(throughTo - DOOR_MID_X),
               'an open door should not have pushed the camera anywhere')
            .toBeLessThan(100);
    });
});

describe('OpenLara FX3: the frame pipeline', () => {
    // main.asm: the upload runs in both letterboxes under forced blank (the
    // V-timer IRQ at line 185, IrqFx3, and the NMI's chunk), and the idle
    // loop polls the GSU's STOP. Every VRAM write must land inside a blank,
    // the display must be back before the view's first line (41) with room
    // to spare, and standing at the start flips at least ten frames a second
    // (6.0 before, 12.0 after, in TypeSNES).
    it.skipIf(!FX3 || LEVEL !== 'LEVEL1')('uploads in the letterboxes and flips ten frames a second or more', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        emulator.executeFrames(120);
        const ppu = emulator.getPPU() as unknown as {
            scanline: number; videoMemoryWritable(): boolean; writeRegister(a: number, v: number): void };
        const write = ppu.writeRegister.bind(ppu);
        const back: number[] = [];
        let lost = 0, blanked = false;
        ppu.writeRegister = (a: number, v: number) => {
            if (a === 0x2100) {
                if (v & 0x80) blanked = true;
                else if (blanked) { back.push(ppu.scanline); blanked = false; }
            }
            if ((a === 0x2118 || a === 0x2119) && !ppu.videoMemoryWritable()) lost++;
            return write(a, v);
        };
        const done = sym('UploadFX3Cols@complete');
        let flips = 0;
        const cpu = emulator.getCPU(), step = cpu.step;
        cpu.step = function (this: typeof cpu) {
            if ((this.PB << 16 | this.PC) === done) flips++;
            return step.call(this);
        };
        try { emulator.executeFrames(600); } finally { cpu.step = step; ppu.writeRegister = write; }
        const latest = Math.max(...back.filter(l => l < 225));
        console.log(`[openlara] pipeline: ${flips} flips in 600 fields (${(flips / 10).toFixed(1)} fps), ` +
            `display back by line ${latest}, ${back.length} blanks, VRAM writes outside them ${lost}`);
        expect(lost, 'a VRAM write landed outside a blank').toBe(0);
        expect(latest, 'the display came back too close to the view (line 41)').toBeLessThanOrEqual(33);
        expect(flips, 'fewer than ten frames a second standing').toBeGreaterThanOrEqual(100);
    });
});

describe('OpenLara FX3: doors', () => {
    // Upstream's Door::action (door.c): a shut door is two walls in the
    // sectors -- past it in its own room, and at it in the room beyond --
    // which every query sees: her collision, a creature's step, the camera,
    // the guns' line of sight. LEVEL1's door 28 (DOOR_3, room 16 at x 18944
    // facing +X) shuts room 16's x 17920 and room 17's x 18944; room 17
    // keeps the bear (item 30) behind it. Pads 56-64 in room 21 wake it.
    it.skipIf(!FX3 || LEVEL !== 'LEVEL1')('a shut door holds the bear back, and the guns do not see through it', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        for (let i = 0; i < 120; i++) emulator.executeFrames(1);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const at = (i: number, f: number) => sym('items') + i * ITEM_SIZE + f;
        const s16 = (a: number) => (u16(a) << 16) >> 16;
        const w16 = (a: number, v: number) => { memory.write(a, v & 0xFF); memory.write(a + 1, (v >> 8) & 0xFF); };
        const atGameTick = (setup: () => void) => {
            const cpu = emulator.getCPU(), original = cpu.step;
            let applied = false;
            cpu.step = function () {
                if (!applied && (cpu.PB << 16 | cpu.PC) === sym('gameUpdate')) { setup(); applied = true; }
                return original.call(this);
            };
            try { for (let i = 0; i < 60 && !applied; i++) emulator.executeFrames(1); }
            finally { cpu.step = original; }
            expect(applied).toBe(true);
        };
        const find = (type: number, room: number, x: number, z: number) => {
            for (let i = 1; i < u16(sym('itemsCount')); i++)
                if (u8(at(i, ITEM.type)) === type && u8(at(i, ITEM.room)) === room &&
                    s16(at(i, ITEM.x)) === ((x << 16) >> 16) && s16(at(i, ITEM.z)) === ((z << 16) >> 16)) return i;
            const seen = Array.from({ length: u16(sym('itemsCount')) }, (_, i) => i)
                .filter(i => u8(at(i, ITEM.type)) === type)
                .map(i => `${i}: room ${u8(at(i, ITEM.room))} ${s16(at(i, ITEM.x))},${s16(at(i, ITEM.z))}`);
            throw new Error(`no item of type ${type} at (${x}, ${z}) in room ${room}; ${seen.join('; ')}`);
        };
        const BEAR = find(8, 17, 14848, 58880), DOOR = find(59, 16, 18944, 59904);
        const doorBytes = () => doorSectors(memory, sym, DOOR).map(d =>
            [0, 1, 2, 3].map(k => u8(0x7F0000 + d.s * 4 + k)));
        const shut = doorSectors(memory, sym, DOOR);
        expect(shut.length, 'door 28 leads nowhere').toBe(2);
        expect(u8(sym('olDoorOpen') + DOOR)).toBe(0);
        expect(doorBytes(), 'door 28 should start shut in the sectors').toEqual([DOOR_SHUT, DOOR_SHUT]);

        // Wake the bear from its pad, at the pad's own floor.
        const place = (room: number, x: number, y: number, z: number, angle: number) => atGameTick(() => {
            memory.write(at(0, ITEM.room), room);
            w16(at(0, ITEM.x), x); w16(at(0, ITEM.y), y); w16(at(0, ITEM.z), z);
            w16(at(0, ITEM.angleY), angle); w16(sym('olLaraMoveAngle'), angle);
        });
        place(21, 8704, 6656, 62976, 0);
        for (let i = 0; i < 6; i++) emulator.executeFrames(1);
        w16(at(0, ITEM.y), s16(sym('ol_floor')));
        for (let i = 0; i < 12; i++) emulator.executeFrames(1);
        expect(u8(at(BEAR, ITEM.flags)) & 0x40, 'the pad did not wake the bear').toBe(0x40);

        // Her, past the door in room 16, facing it and the bear beyond.
        place(16, 21504, 6656, 59904, 0xC000);
        // Sampled where a tick starts: a field can end inside one, between
        // a creature's provisional step and the restore that refuses it.
        let nearest = -32768, room16 = 0, target = 0, sampled = 0, ticks = 0, firing = false;
        const cpu = emulator.getCPU(), original = cpu.step;
        cpu.step = function () {
            if ((cpu.PB << 16 | cpu.PC) === sym('gameUpdate')) {
                ticks++;
                nearest = Math.max(nearest, s16(at(BEAR, ITEM.x)));
                if (u8(at(BEAR, ITEM.room)) === 16) room16++;
                if (firing) { sampled++; if (u16(sym('olWeaponTarget')) === BEAR) target++; }
            }
            return original.call(this);
        };
        const pad = emulator.getController() as unknown as { buttonState: number };
        try {
            for (let n = 0; n < 1800; n++) {
                if (n === 60) pad.buttonState = PAD_R;          // draw
                if (n === 64) pad.buttonState = 0;
                firing = n > 400 && n < 1700;                   // and fire at what she sees
                if (n > 400) pad.buttonState = firing ? PAD_B : 0;
                emulator.executeFrames(1);
            }
        } finally { cpu.step = original; }
        console.log(`[openlara] door: bear ${BEAR} reached x ${nearest} (door line 18432) in ${ticks} ticks, ` +
            `${room16} in room 16, health ${s16(at(BEAR, ITEM.health))}, ` +
            `mood ${u8(at(BEAR, ITEM.mood))}; her guns held it ${target} of ${sampled} ticks`);
        expect(nearest, 'the bear never came to the door').toBeGreaterThan(18432 - 1024);
        expect(nearest, 'the bear walked through a shut door').toBeLessThan(18432);
        expect(room16, 'the bear walked through a shut door').toBe(0);
        expect(target, 'her guns locked onto the bear through a shut door').toBe(0);
        expect(doorBytes()).toEqual([DOOR_SHUT, DOOR_SHUT]);
    });

    // Every door of the level: its sectors as door.c last wrote them, and
    // that as upstream's update rule wants it once the door has settled.
    it.skipIf(!FX3)('keeps every door\'s sectors as its state says', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        for (let i = 0; i < 60; i++) emulator.executeFrames(1);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        let doors = 0;
        for (let i = 1; i < (u8(sym('itemsCount')) | u8(sym('itemsCount') + 1) << 8); i++) {
            const it = sym('items') + i * ITEM_SIZE, type = u8(it);
            if (type < DOOR_FIRST || type > DOOR_LAST) continue;
            const open = u8(sym('olDoorOpen') + i);
            expect(open, `door ${i} in state ${u8(it + ITEM.state)}`).toBe(u8(it + ITEM.state) === 1 ? 1 : 0);
            for (const d of doorSectors(memory, sym, i))
                expect([0, 1, 2, 3].map(k => u8(0x7F0000 + d.s * 4 + k)), `door ${i} sector ${d.s}`)
                    .toEqual(open ? d.level : DOOR_SHUT);
            doors++;
        }
        console.log(`[openlara] doors: ${doors} on ${LEVEL}, their sectors as their states say`);
        expect(doors).toBeGreaterThan(0);
    });
});

describe('OpenLara SNES/Super FX: creatures', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('turns toward Lara and comes at her', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const itemAddr = (i: number, f: number) =>
            sym('items') + i * ITEM_SIZE + f;
        const itemB = (i: number, f: number) =>
            u8(itemAddr(i, f));
        const itemW = (i: number, f: number) =>
            u16(itemAddr(i, f));
        const sItemW = (i: number, f: number) => {
            const v = itemW(i, f);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const s16 = (a: number) => {
            const v = u16(a);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };
        for (let i = 0; i < 120; i++) emulator.executeFrames(1);

        // An entity is found by what it is and where it stands, not by its
        // index in the host's array. Which entities get a slot there depends
        // on what the packer could carry -- pickups only joined it once they
        // had a model to be drawn with -- so an index is stable only until
        // that changes, and it has changed once already.
        const findItem = (type: number, room: number, x: number, z: number) => {
            const n = u16(sym('itemsCount'));
            for (let i = 1; i < n; i++) {
                if (itemB(i, ITEM.type) !== type) continue;
                if (itemB(i, ITEM.room) !== room) continue;
                if (sItemW(i, ITEM.x) !== x) continue;
                if (sItemW(i, ITEM.z) !== z) continue;
                return i;
            }
            throw new Error(`no item of type ${type} at (${x}, ${z}) in room ${room}`);
        };

        // A wolf in room 14 of LEVEL1, facing +Z at angle 0. Put the camera
        // two and a half sectors along -Z from it: it has to turn most of the
        // way round to face her, and then close the distance.
        const WOLF = findItem(7, 14, 22016, -14848);
        const angleAt = (i: number) => itemW(i, ITEM.angleY);
        const zAt = (i: number) => sItemW(i, ITEM.z);

        // A creature sleeps until a trigger names it, as in TR. This wolf and
        // its neighbour are named by two pads in room 13 on the way in (the
        // packer's OL_DUMP_TRIGGERS=1 lists them: trigger 24 at world
        // (31232, 47616), which wraps to -17920 here). Stand her on that pad
        // at its own floor for a few ticks, then put her where the scene
        // wants her.
        const s16at = (a: number) => {
            const v = u16(a);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        memory.write(sym('items') + ITEM.room, 13);
        w16(sym('items') + ITEM.x, 31232);
        w16(sym('items') + ITEM.z, -17920);
        memory.write(sym('items') + ITEM.state, 2);
        memory.write(sym('items') + ITEM.goalState, 2);
        w16(sym('items') + ITEM.animIndex, 103);
        memory.write(sym('items') + ITEM.frameIndex, 0);
        for (let i = 0; i < 6; i++) emulator.executeFrames(1);
        w16(sym('items') + ITEM.y, s16at(sym('ol_floor')));
        for (let i = 0; i < 12; i++) emulator.executeFrames(1);
        expect(itemB(WOLF, ITEM.flags) & 0x40, 'the pad did not wake the wolf')
            .toBe(0x40);

        w16(sym('items') + ITEM.x, 22016);
        w16(sym('items') + ITEM.y, 6656);
        w16(sym('items') + ITEM.z, -17456);
        // Present Lara's back: Wolf::RUN can leap from this approach. The
        // front-facing branch brakes into STALK instead of proximity damage.
        w16(sym('items') + ITEM.angleY, 0x8000);
        memory.write(sym('items') + ITEM.room, 14);
        for (let i = 0; i < 200; i++) emulator.executeFrames(1);

        // Sampled through the run rather than only at the end, because a wolf
        // that has already arrived is no longer facing where it came from --
        // asserting on the final angle asserts that it failed to get there.
        const LARA_Z = -17456;
        // Running wolves arc around their target; compare with the current
        // bearing, not the -Z bearing from the original sleeping position.
        const facingError = () => {
            const dx = sItemW(0, ITEM.x) - sItemW(WOLF, ITEM.x);
            const dz = sItemW(0, ITEM.z) - zAt(WOLF);
            const bearing = Math.round(Math.atan2(dx, dz) * 32768 / Math.PI);
            return Math.abs(((angleAt(WOLF) - bearing) << 16) >> 16);
        };
        const distance = () => Math.max(Math.abs(sItemW(0, ITEM.x) - sItemW(WOLF, ITEM.x)),
            Math.abs(sItemW(0, ITEM.z) - zAt(WOLF)));
        const z0 = zAt(WOLF);
        let bestFacing = facingError(), nearest = distance();
        const states = new Set([itemB(WOLF, ITEM.state)]);

        // Observe the real 30 Hz state graph while the renderer runs. A leap
        // or bite may be shorter than a displayed frame, so sample frequently.
        for (let n = 0; n < 25; n++) {
            for (let i = 0; i < 200; i++) {
                emulator.executeFrames(1);
                if (i % 10) continue;
                bestFacing = Math.min(bestFacing, facingError());
                nearest = Math.min(nearest, distance());
                states.add(itemB(WOLF, ITEM.state));
            }
            if (nearest < 600 && sItemW(0, ITEM.health) < 1000) break;
        }
        const z1 = zAt(WOLF);

        console.log(`[openlara] mood: wolf ${WOLF} is ` +
                    `${['asleep','stalking','attacking','escaping'][itemB(WOLF, ITEM.mood)]}`);
        console.log(`[openlara] wolf: closest facing ${bestFacing} of 32768, ` +
                    `z ${z0} -> ${z1} (Lara at ${LARA_Z}), ` +
                    `states ${[...states]}, closest approach ${nearest}`);

        // Check the navigation implementation this cartridge actually uses:
        // independent weighted searches on FX3, shared floods on GSU-2.
        let reached = 0;
        if (FX3) {
            const roots = Array.from({ length: 3 }, (_, i) => sym('olNav') + i * 8240);
            const nav = roots.find(n => u16(n + 8214) === WOLF);
            expect(nav, 'the pursuing wolf lost its Nav context').toBeDefined();
            for (let b = 0; b < u16(sym('olBoxCount')); b++)
                if (u16(nav! + b * 8 + 2) & 0x7FFF) reached++;
        } else {
            reached = u16(sym('navReached'));
        }
        console.log(`[openlara] nav: ${FX3 ? 'wolf search visited' : 'flood connected'} ${reached} boxes ` +
                    `of ${u16(sym('olBoxCount'))}`);
        expect(reached, 'the navigation search connected nothing').toBeGreaterThan(1);

        expect(bestFacing, 'the wolf never turned to face her')
            .toBeLessThan(0x1000);
        expect(states.has(3), 'the wolf never reached its original RUN state').toBe(true);
        expect(Math.abs(z1 - LARA_Z), 'and never closed the distance')
            .toBeLessThan(Math.abs(z0 - LARA_Z));

        // Unlike the old reduced walker, a wolf can run past and circle.
        // Assert that it actually approached, not that it stopped forever.
        expect(nearest, 'it never reached contact range')
            .toBeLessThan(600);

        // Damage comes from ATTACK/BITE states at 50/100 per attack. The
        // isolated wolf fixture checks the per-attack latch exactly.
        const health = sItemW(0, ITEM.health);
        console.log(`[openlara] health: ${health} of 1000`);
        expect(health, 'the wolf reached her and did nothing')
            .toBeLessThan(1000);

        // Now the other way round. A bat in room 18, and a bat has one health
        // in TR -- so one shot is the whole fight, which is what makes it the
        // honest test of the death path. A wolf takes six, and long before the
        // sixth the other wolf in the room has wandered into the line of fire,
        // which is emergent and not something to assert on.
        const BAT = findItem(9, 18, 10752, -6656);
        const hpAt = (i: number) => sItemW(i, ITEM.health);

        // Target acquisition now follows upstream's ACTIVE-only rule. The
        // old test shot this sleeping bat without ever crossing its wake-up
        // pad. Stand on its real trigger (41) in room 18 first.
        w16(sym('items') + ITEM.health, 1000);
        memory.write(sym('items') + ITEM.room, 18);
        w16(sym('items') + ITEM.x, 11776);
        w16(sym('items') + ITEM.y, 6656);
        w16(sym('items') + ITEM.z, -2560);
        let batAwake = false;
        for (let i = 0; i < 240 && !batAwake; i++) {
            emulator.executeFrames(1);
            batAwake = !!(itemB(BAT, ITEM.flags) & 0x40) &&
                       (itemB(BAT, ITEM.flags2) & ITEM_STATUS) === 1;
        }
        expect(batAwake, 'the bat did not wake on its own floor trigger').toBe(true);
        emulator.executeFrames(3);

        // Stay on the pad in the open centre of the room, facing the bat
        // along -Z and slightly -X. The old firing position was inside the
        // room's solid north border. Its hanging pose also starts above the
        // ceiling: let it fly into the open corridor before a shot can hit.
        w16(sym('items') + ITEM.angleY, 0x8A00);

        const before = hpAt(BAT);

        const pad2 = emulator.getController() as unknown as { buttonState: number };
        pad2.buttonState = PAD_R;
        emulator.executeFrames(4);
        pad2.buttonState = 0;
        emulator.executeFrames(60);
        for (let shot = 0; shot < 6 && hpAt(BAT) > 0; shot++) {
            pad2.buttonState = PAD_B;
            for (let i = 0; i < 100; i++) emulator.executeFrames(1);
            pad2.buttonState = 0;
            for (let i = 0; i < 500; i++) emulator.executeFrames(1);
        }
        const after = hpAt(BAT);

        console.log(`[openlara] shooting: bat ${BAT} health ` +
                    `${before} -> ${after}, state ` +
                    `${itemB(BAT, ITEM.state)}`);

        expect(before, 'the bat should have started on upstream health').toBe(1);
        expect(FX3 ? [0, -0x4000] : [0], 'shooting it did nothing').toContain(after);

        // A dead bat stops moving horizontally and falls through its own
        // CIRCLING -> DEATH graph onto the floor.
        const restZ = sItemW(BAT, ITEM.z);
        for (let i = 0; i < 400; i++) emulator.executeFrames(1);
        expect(hpAt(BAT), 'it came back to life').toBe(FX3 ? -0x4000 : 0);
        expect(sItemW(BAT, ITEM.z), 'a dead bat kept flying')
            .toBe(restZ);
        expect(itemB(BAT, ITEM.state), 'the bat never reached its death animation').toBe(5);
        expect(sItemW(BAT, ITEM.y)).toBe(sItemW(BAT, ITEM.roomFloor));
        expect(itemB(BAT, ITEM.flags2) & 4, 'the corpse still has gravity').toBe(0);
        if (FX3) {
            expect(itemB(BAT, ITEM.flags2) & 3, 'the ten-second corpse never became invisible').toBe(3);
            expect(itemB(BAT, ITEM.flags) & 0x40, 'expired corpse remained active').toBe(0);
            expect(u8(sym('ol_itemState_gsu') + BAT * 12), 'expired corpse stayed in the rendered scene').toBe(255);
        }

        // Exact range and angle-priority selection live in the pistol combat
        // fixture, with valid room/height/status and a single 30 Hz update.
    });
});

describe('OpenLara SNES/Super FX: bridges', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('stands on a bridge instead of falling through it', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const itemB = (i: number, f: number) =>
            u8(sym('items') + i * ITEM_SIZE + f);
        const itemW = (i: number, f: number) =>
            u16(sym('items') + i * ITEM_SIZE + f);
        const sItemW = (i: number, f: number) => {
            const v = itemW(i, f);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const s16 = (a: number) => {
            const v = u16(a);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };
        const w8 = (a: number, v: number) => memory.write(a, v & 0xFF);

        for (let i = 0; i < 120; i++) emulator.executeFrames(1);

        // Item 16 of LEVEL1 is a flat bridge in room 14, at y 5120 over the
        // cistern. The room's own floor there is a long way below it; the
        // bridge is the only thing to stand on, and before this it was
        // scenery you fell through.
        // Room 14 costs half a second a frame, so this has to be long enough
        // for the GSU to probe the new position and the host to collect it --
        // and the wall clamp nudges a camera standing over a pit the whole
        // time, so which sector of the run it ends on is not worth pinning.
        // What is worth pinning is that an entity is holding it up at all.
        w16(sym('items') + ITEM.x, 25088);
        w16(sym('items') + ITEM.y, 5120);
        w16(sym('items') + ITEM.z, -12800);
        w8(sym('items') + ITEM.room, 14);
        for (let i = 0; i < 400; i++) emulator.executeFrames(1);

        const onBridge = s16(sym('ol_floorEff'));
        const onRoom = s16(sym('ol_floor'));
        const camOn = sItemW(0, ITEM.y);
        {
            const n = u16(sym('olBridgeCount'));
            const list: string[] = [];
            for (let k = 0; k < n; k++) {
                const idx = u8(sym('olBridges') + k);
                list.push(`${idx}:t${itemB(idx, ITEM.type)}@r${itemB(idx, ITEM.room)}` +
                          `(${sItemW(idx, ITEM.x) >> 10},${sItemW(idx, ITEM.z) >> 10})`);
            }
            console.log(`[openlara] bridge list: ${list.join(' ')}; lara r${itemB(0, ITEM.room)} ` +
                        `(${sItemW(0, ITEM.x) >> 10},${sItemW(0, ITEM.z) >> 10}) y=${camOn} ` +
                        `state=${itemB(0, ITEM.state)} floor=${onRoom} eff=${onBridge}`);
        }

        // One sector along is a tilted one, which is the same rule with a
        // slope: its floor has to sit between the flat bridge and the drop.
        w16(sym('items') + ITEM.x, 25088);
        w16(sym('items') + ITEM.y, 4864);
        w16(sym('items') + ITEM.z, -11776);
        for (let i = 0; i < 400; i++) emulator.executeFrames(1);
        const onTilt = s16(sym('ol_floorEff'));

        // Sample two points on that same tilted bridge. Its origin is exactly
        // halfway along the sector, so testing only there cannot distinguish
        // a real slope from code that accidentally asks getBridgeFloor for
        // the bridge's own position every time.
        const sampleTilt = (z: number) => {
            // Keep her on the requested point until two host updates have
            // consumed it. A door/wall push later in gameUpdate is allowed to
            // move Lara, and merely waiting 240 fields made the second sample
            // observe the room floor at that pushed position instead.
            let tick = u16(sym('ol_ticks'));
            let updates = 0;
            for (let i = 0; i < 600 && updates < 2; i++) {
                // This test arrives here after deliberately leaving Lara over
                // a pit.  Keep the movement state still as well as the point:
                // otherwise a newly implemented air/ground state can move the
                // GSU's next room-floor probe out from under this bridge while
                // the entity-floor half is sampling the requested point.
                w8(sym('items') + ITEM.state, 2);
                w8(sym('items') + ITEM.goalState, 2);
                w16(sym('items') + ITEM.x, 25088);
                w16(sym('items') + ITEM.y, 4608); // above both ends
                w16(sym('items') + ITEM.z, z);
                w8(sym('items') + ITEM.room, 14);
                w16(sym('items') + ITEM.angleY, 0);
                w16(sym('items') + ITEM.animIndex, 103);
                w8(sym('items') + ITEM.frameIndex, 0);
                w8(sym('items') + ITEM.flags2, 0);
                w16(sym('items') + ITEM.vSpeed, 0);
                w16(sym('items') + ITEM.hSpeed, 0);
                emulator.executeFrames(1);
                const next = u16(sym('ol_ticks'));
                if (next !== tick) {
                    tick = next;
                    updates++;
                }
            }
            expect(updates, 'the host never sampled the tilted bridge')
                .toBe(2);
            return s16(sym('ol_floorEff'));
        };
        const tiltA = sampleTilt(-12096);
        const tiltB = sampleTilt(-11456);

        console.log(`[openlara] bridge: on=${onBridge} (room ${onRoom}) ` +
                    `cam=${camOn}, tilt=${onTilt}, ends=${tiltA}/${tiltB}`);

        // An entity is the floor and the room is a long way below it. Only
        // the positive is asserted: the camera does not stay exactly where it
        // is put -- the wall clamp nudges it off a pit edge -- so "somewhere
        // with no bridge" is not a position a test can hold it at. That
        // nothing raises the floor in general is what every other test here
        // depends on already.
        expect(onBridge, 'a bridge should have been holding the camera up')
            .not.toBe(onRoom);
        expect(onBridge, 'and it should be above the room floor, not below')
            .toBeLessThan(onRoom);
        // Above the pit rather than exactly on the number: the floor and her
        // position are read a frame apart and the run has two bridge heights
        // in it, so equality is a race and "held up" is the property.
        expect(sItemW(0, ITEM.y), 'she should be standing above the pit')
            .toBeLessThan(onRoom);
        expect(tiltA, 'the near end of a tilted bridge has the wrong height')
            .toBe(5072);
        expect(tiltB, 'the far end of a tilted bridge has the wrong height')
            .toBe(4912);

    });
});

describe('OpenLara SNES/Super FX: walking', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('moves under the pad, stays on a floor, and stops at a wall', () => {
        if (WALK <= 0) return;
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const itemB = (i: number, f: number) =>
            u8(sym('items') + i * ITEM_SIZE + f);
        const itemW = (i: number, f: number) =>
            u16(sym('items') + i * ITEM_SIZE + f);
        const sItemW = (i: number, f: number) => {
            const v = itemW(i, f);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const s16 = (a: number) => {
            const v = memory.read(a) | (memory.read(a + 1) << 8);
            return v & 0x8000 ? v - 0x10000 : v;
        };

        // Settle: the first frame has to finish and be uploaded before the
        // GSU has staged anything for the game to act on.
        emulator.executeFrames(120);
        // Lara rather than the camera: the camera is derived from her now.
        for (let i = 0; i < 200; i++) emulator.executeFrames(1);
        const start = { x: sItemW(0, ITEM.x), y: sItemW(0, ITEM.y),
                        z: sItemW(0, ITEM.z) };
        expect(s16(sym('ol_floor')), 'the camera should start on a floor')
            .not.toBe(NO_FLOOR);

        // Hold Up. The Controller has no public setter -- it is built to be
        // driven by a keyboard -- so the harness writes the button state it
        // would have latched.
        const pad = emulator.getController() as unknown as { buttonState: number };
        pad.buttonState = PAD_UP;

        let offFloor = 0;
        for (let i = 0; i < WALK; i++) {
            emulator.executeFrames(1);
            if (s16(sym('ol_floor')) === NO_FLOOR) offFloor++;
        }
        pad.buttonState = 0;

        // Let her come to rest before testing the jump. Stable Y is not enough:
        // a run-to-stop animation can spend several expensive rendered frames
        // at one height while still being STATE_RUN.
        const ITEM_FLAG_GRAVITY = 0x04;
        let settled = false;
        for (let i = 0; i < 1800 && !settled; i++) {
            emulator.executeFrames(1);
            settled = itemB(0, ITEM.state) === 2 &&
                      sItemW(0, ITEM.hSpeed) === 0 &&
                      !(itemB(0, ITEM.flags2) & ITEM_FLAG_GRAVITY);
        }
        expect(settled, 'Lara did not finish her run-to-stop transition')
            .toBe(true);

        const sItem0 = (f: number) => sItemW(0, f);
        const walked = { x: sItemW(0, ITEM.x), y: sItemW(0, ITEM.y),
                         z: sItemW(0, ITEM.z) };

        // Jump. Y grows downward, so going up is going down in value, and
        // landing is coming back to where the floor puts you.
        //
        // Hold A+Up until the animation's JUMP command actually engages
        // gravity, then release. A short field-counted tap is not sufficient:
        // presses are latched, but the held direction intentionally is not,
        // and a rendered frame can span far more fields than the tap.
        //
        // The apex is sampled from the moment A goes down, and the loop ends
        // when she is back on the floor rather than after a fixed number of
        // fields. A jump is a fixed number of *logic* steps -- seven, at
        // GRAVITY against JUMP_SPEED -- and a logic step is one rendered
        // frame, so how many fields it spans depends on how expensive the
        // view is. A window sized for 3 fps missed the whole arc the day
        // models started drawing.
        pad.buttonState = PAD_A | PAD_UP;
        let apex = walked.y;
        let airborne = false;
        let landed = false;
        const jumpCap = Math.max(1800, WALK * 2);
        const arc: string[] = [];
        let lastY = walked.y;
        for (let i = 0; i < jumpCap && !landed; i++) {
            emulator.executeFrames(1);
            const gravity = !!(itemB(0, ITEM.flags2) & ITEM_FLAG_GRAVITY);
            if (!airborne && gravity) {
                airborne = true;
                pad.buttonState = 0;
            }
            const y = sItemW(0, ITEM.y);
            // One entry per step that actually moved her, which is what makes
            // this readable at a frame a second.
            if (y !== lastY) {
                if (arc.length < 24) {
                    arc.push(`${y}/v${sItem0(ITEM.vSpeed)}` +
                             `/f${s16(sym('ol_floorEff'))}/s${itemB(0, ITEM.state)}`);
                }
                lastY = y;
            }
            if (y < apex) apex = y;
            // On the floor she is *now* over, not the one she set off from.
            // TR's jump carries her forward -- the push comes out of
            // ANIM_CMD_JUMP and has a horizontal component -- so by the time
            // she comes down the ground under her has usually changed. The
            // old test compared against her pre-jump Y and failed the day the
            // jump stopped being purely vertical.
            if (airborne && !gravity) landed = true;
        }
        pad.buttonState = 0;
        console.log(`[openlara] jump arc: ${arc.join(' ')}`);

        const end = { x: sItemW(0, ITEM.x), y: sItemW(0, ITEM.y),
                      z: sItemW(0, ITEM.z) };
        const room = s16(sym('ol_room'));

        // The busy scene: wolves awake, the level's entities all updating,
        // her running. The clock has to keep thirty a second here too.
        const tpsWalk = u16(sym('ol_tps'));
        const droppedWalk = u16(sym('ol_ticksDropped'));
        const tickCostWalk = tpsWalk ? u16(sym('ol_tickFields')) / tpsWalk : 0;
        console.log(`[openlara] clock (walking): ticks=${tpsWalk}/s ` +
                    `dropped=${droppedWalk} tickCost=${tickCostWalk.toFixed(2)} fields`);
        expect(tpsWalk, 'the clock fell behind while walking')
            .toBeGreaterThanOrEqual(29);
        expect(droppedWalk, 'the backlog cap dropped ticks while walking').toBe(0);
        console.log(`[openlara] walk ${WALK} fields: ` +
                    `(${start.x},${start.y},${start.z}) -> ` +
                    `(${walked.x},${walked.y},${walked.z})  room=${room}  ` +
                    `offFloor=${offFloor}  jump apex=${apex} landed=${end.y}`);

        // The start angle is 180 degrees, so forward is -Z, and nothing else
        // was pressed.
        // She faces +Z at the start of LEVEL1 -- that is the angle the level
        // gives her, and it used to be the camera's ANGLE_180 instead because
        // the camera was the thing being driven.
        expect(walked.z, 'holding Up should have moved her along +Z')
            .toBeGreaterThan(start.z);
        expect(walked.x, 'nothing should have pushed it sideways')
            .toBe(start.x);
        expect(apex, 'pressing A should have left the ground')
            .toBeLessThan(walked.y);
        expect(airborne, 'the jump animation never engaged gravity').toBe(true);
        expect(landed, 'the jump did not return to the ground').toBe(true);
        // Back on the ground -- whichever ground she came down on, and within
        // a step of it. She does not land where she took off any more: the
        // jump's push comes from ANIM_CMD_JUMP and carries her forward, and
        // the corridor she comes down in rises, so the last thing she is doing
        // is walking *up* onto it a step at a time. What says gravity finished
        // with her is that it has let go, not that she is at an exact height.
        expect(itemB(0, ITEM.flags2) & ITEM_FLAG_GRAVITY,
               'she should not still be in the air').toBe(0);
        expect(Math.abs(end.y - s16(sym('ol_floorEff'))),
               'and gravity should have brought it back down')
            .toBeLessThanOrEqual(96);
        // Walking off the level would leave it over no floor at all; the wall
        // collision is what keeps that from happening.
        expect(offFloor, 'the camera left the floor while walking').toBe(0);
        // The busiest scene the runner has: the stack floor again (see the
        // boot test).
        for (let a = 0x7E1C00; a < 0x7E1C40; a++) {
            expect(memory.read(a), `the stack reached $7E:${(a & 0xFFFF).toString(16)}`).toBe(0);
        }

        // The entity list only reaches this side by being staged in game pak
        // RAM and copied back, so a count means the whole channel worked:
        // packer, GSU copy, host copy, and the C's parse of the blob.
        const s16at = (a: number) => {
            const v = u8(a) | (u8(a + 1) << 8);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        // The half tools/fixedcheck cannot reach: the ported maths run with the
        // compiler's own type widths rather than the desktop's. See
        // olMathCheck in src/game.c.
        expect(u16(sym('ol_mathFail')),
               'the cartridge disagreed with the maths it was built from')
            .toBe(0);

        const itemCount = u8(sym('itemsCount')) | (u8(sym('itemsCount') + 1) << 8);
        expect(itemCount, 'the host never received the entity list')
            .toBeGreaterThan(0);

        // Item 0 of LEVEL1 is Lara, in room 0. She is the camera now, which
        // is what makes "is Lara near this switch" answerable, so what pins
        // the parse is that she *follows* it -- at her feet rather than her
        // eye, which is the 512 units the camera rides above the floor.
        expect(itemB(0, ITEM.type), 'item 0 should be Lara').toBe(0);
        // Her room follows the camera's now, which is what makes a creature
        // able to work out whether it can reach her.
        expect(itemB(0, ITEM.room), 'Lara should be in the room the camera is')
            .toBe(u16(sym('ol_room')));

        // The camera rides above her feet and behind her, which is what the
        // follow camera is for.
        const drop = sItemW(0, ITEM.y) - s16(sym('ol_camY'));
        expect(drop, 'the camera should be above her feet').toBeGreaterThan(0);

        // And they animate. Compared against a snapshot rather than against
        // what olLoadItems would have set, because an entity whose animation
        // is one frame long never leaves frame zero and an entity that has
        // been round the loop can land back on where it began.
        const frames = Array.from({ length: itemCount },
                                  (_, i) => itemB(i, ITEM.frameIndex));
        const anims = Array.from({ length: itemCount },
                                 (_, i) => itemW(i, ITEM.animIndex));
        emulator.executeFrames(120);
        const later = Array.from({ length: itemCount },
                                 (_, i) => itemB(i, ITEM.frameIndex));
        expect(later.some((f, i) => f !== frames[i]),
               'no entity animated at all').toBe(true);

        // Every entity has to be on an animation its own model owns, which is
        // what says the chain landed somewhere real rather than merely
        // somewhere different.
        // The bound is the cartridge's own parse of the blob rather than a
        // number written here: the animation table grew when sprites got
        // synthetic animations, and a constant would have to be chased.
        const animCount = FX3 ? u16(sym('olHostDirectory') + 20) : (u16(sym('olTransBase')) - u16(sym('olAnimBase'))) / 14;
        expect(anims.every(a => a < animCount), 'an entity ran off the anim table')
            .toBe(true);

        console.log(`[openlara] lists: doors=${u16(sym('olDoorCount'))} ` +
                    `bridges=${u16(sym('olBridgeCount'))} ` +
                    `creatures=${u16(sym('olCreatureCount'))}`);
        console.log(`[openlara] items: ${itemCount}, ` +
                    `frames=${frames.slice(0, 6).join(',')} -> ` +
                    `${later.slice(0, 6).join(',')}, ` +
                    `anims=${anims.slice(0, 6).join(',')}`);
        expect(room, 'the camera should still be in a real room').toBeLessThan(38);
    }, 600_000);   // a second of emulated SNES is not a five second test
});

describe('OpenLara SNES/Super FX: pickups', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('draws a sprite pickup and takes it away', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const itemB = (i: number, f: number) =>
            u8(sym('items') + i * ITEM_SIZE + f);
        const itemW = (i: number, f: number) =>
            u16(sym('items') + i * ITEM_SIZE + f);
        const sItemW = (i: number, f: number) => {
            const v = itemW(i, f);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };

        // TR draws a pickup as a sprite: a texture with a world size and no
        // geometry. The packer builds it the one quad, one node, one animation
        // and one frame that everything downstream is indexed by, and marks
        // the node a billboard so the renderer drops the rotation out of the
        // matrix. What that buys is seven entities in LEVEL1 that were simply
        // not there before -- five small medipacks and two large.
        const MEDIKIT_SMALL = 93;
        const findItem = (type: number, room: number, x: number, z: number) => {
            const n = u16(sym('itemsCount'));
            for (let i = 1; i < n; i++) {
                if (itemB(i, ITEM.type) !== type) continue;
                if (itemB(i, ITEM.room) !== room) continue;
                if (sItemW(i, ITEM.x) !== x) continue;
                if (sItemW(i, ITEM.z) !== z) continue;
                return i;
            }
            throw new Error(`no item of type ${type} at (${x}, ${z}) in room ${room}`);
        };

        // The animation clock is pinned and every entity held on one frame,
        // exactly as the render test does it: the assertion below is that the
        // picture changed when the medipack was taken, and a bear walking
        // about in the background would change it for its own reasons.
        const ticksAddr = sym('ol_ticks');
        const freezeAddr = sym('ol_animFreeze');
        // Pinned once a field and at every render sync: a tick inside a field
        // makes ol_ticks 1 until the field ends, and a render sync landing
        // there (frequent since the letterbox upload) draws that pose.
        const hold = (fields: number) => {
            const cpu = emulator.getCPU(), step = cpu.step, sync = sym('gameRenderSync');
            cpu.step = function (this: typeof cpu) {
                if ((this.PB << 16 | this.PC) === sync) { w16(ticksAddr, 0); w16(freezeAddr, 1); }
                return step.call(this);
            };
            try {
                for (let i = 0; i < fields; i++) {
                    emulator.executeFrames(1);
                    w16(ticksAddr, 0);
                    w16(freezeAddr, 1);
                }
            } finally { cpu.step = step; }
        };

        hold(120);

        const PACK = findItem(MEDIKIT_SMALL, 23, 4608, 13824);

        // It is carried at all: an entity with no model was dropped by the
        // packer until sprites existed, so this is the whole feature in one
        // number. Model 42 and 43 of LEVEL1 are the two synthetic ones.
        const model = itemB(PACK, ITEM.model);
        console.log(`[openlara] pickup: item ${PACK} type ${MEDIKIT_SMALL}, ` +
                    `model ${model}, anim ${itemW(PACK, ITEM.animIndex)}`);
        expect(model, 'a pickup with no model is a pickup nothing can draw')
            .toBeGreaterThan(0);

        // Stand her at it, inside upstream's LIMIT_PICKUP: within 256
        // behind it, facing it. Action then walks her a hundred short of it
        // and plays the bend; twenty-three frames before the bend ends,
        // it is hers.
        w16(sym('items') + ITEM.x, 4608);
        w16(sym('items') + ITEM.y, 3840);
        w16(sym('items') + ITEM.z, 13824 - 200);
        w16(sym('items') + ITEM.angleY, 0);         // facing +Z, at it
        w16(sym('olLaraMoveAngle'), 0);
        memory.write(sym('items') + ITEM.room, 23);

        // Until the picture stops changing: the camera eases in at 30 Hz and
        // the frame follows it a render behind. Two identical captures 120
        // fields apart end the wait (the drift check below still judges the
        // result); 5400 fields, the old fixed wait, is the cap.
        let settleFields = 0;
        for (let last = emulator.getPPU().getFramebuffer().slice(); settleFields < 5400;) {
            hold(120); settleFields += 120;
            const now = emulator.getPPU().getFramebuffer().slice();
            if (now.every((v, i) => v === last[i])) break;
            last = now;
        }

        // Two captures of the same still picture, to establish that it is
        // one. The follow camera converges rather than arriving -- it closes
        // an eighth of the gap and integer division stops it once the gap is
        // under eight -- so this is what says the settling finished.
        const settled = emulator.getPPU().getFramebuffer().slice();
        hold(200);
        const before = emulator.getPPU().getFramebuffer().slice();
        let drift = 0;
        for (let i = 0; i < settled.length; i++) {
            if (settled[i] !== before[i]) drift++;
        }
        const shownBefore = u8(sym('ol_itemState_gsu') + PACK * 12);
        expect(shownBefore, 'the pickup should be on screen before it is taken')
            .not.toBe(0xFF);

        // B is the action button. Upstream gates this on her standing still,
        // empty-handed, above water and pressing it, and so does this.
        expect(u16(sym('olPickups')), 'nothing has been picked up yet').toBe(0);

        const pad = emulator.getController() as unknown as { buttonState: number };
        pad.buttonState = PAD_B;
        hold(60);
        pad.buttonState = 0;
        // The bend plays on: wait for it, then for the hidden marker.
        for (let i = 0; i < 300 && u16(sym('olPickups')) === 0; i++) hold(1);

        // Gameplay now advances at 30 Hz while the GSU owns ROM and renders
        // the previous snapshot.  Wait for the next CPU/GSU handoff instead
        // of assuming that an arbitrary number of fields straddled it.
        let syncFields = 0;
        while (u8(sym('ol_itemState_gsu') + PACK * 12) !== 0xFF &&
               syncFields < 1800) {
            hold(1);
            syncFields++;
        }

        const pictureChanged = () => {
            const frame = emulator.getPPU().getFramebuffer();
            for (let i = 0; i < before.length; i++) {
                if (before[i] !== frame[i]) return true;
            }
            return false;
        };
        let pictureFields = 0;
        while (!pictureChanged() && pictureFields < 1800) {
            hold(1);
            pictureFields++;
        }
        const after = emulator.getPPU().getFramebuffer().slice();

        console.log(`[openlara] pickup: status ${(itemB(PACK, ITEM.flags2) & ITEM_STATUS)}, ` +
                    `olPickups ${u16(sym('olPickups'))}, ` +
                    `renderer anim $${u8(sym('ol_itemState_gsu') + PACK * 12)
                                        .toString(16)}, ` +
                    `settle ${settleFields}, sync ${syncFields} fields, picture ${pictureFields} fields`);

        // Upstream's ITEM_FLAG_STATUS_INVISIBLE, and gSaveGame.pickups.
        expect((itemB(PACK, ITEM.flags2) & ITEM_STATUS), 'the pickup was not taken').toBe(3);
        expect(u16(sym('olPickups')), 'and was not counted').toBe(1);
        expect(u8(sym('ol_itemState_gsu') + PACK * 12),
               'a taken pickup should be hidden from the renderer').toBe(0xFF);

        // And the picture changed, which is the only thing that says the quad
        // was ever drawn. Everything else in the frame is held still, so the
        // difference is the medipack and nothing else.
        let diff = 0;
        for (let i = 0; i < before.length; i++) {
            if (before[i] !== after[i]) diff++;
        }
        console.log(`[openlara] pickup: ${diff} framebuffer words changed, ` +
                    `against ${drift} that drift on their own`);

        // If the picture were still moving on its own this would say nothing
        // about the sprite, so that is checked rather than assumed.
        expect(drift, 'the picture had not settled').toBe(0);
        expect(diff, 'taking it changed nothing on screen -- ' +
                     'the sprite was never drawn').toBeGreaterThan(0);

        // The other medipack is still there, which says the taking was of one
        // entity rather than of the type.
        const OTHER = findItem(MEDIKIT_SMALL, 27, 7680, 18944);
        expect((itemB(OTHER, ITEM.flags2) & ITEM_STATUS), 'taking one took them all').toBe(0);
    }, 600_000);
});

describe('OpenLara SNES/Super FX: entities', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('draws an entity in the room it is in, not the room it started in', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const w16 = (a: number, v: number) => { memory.write(a, v & 0xFF); memory.write(a + 1, (v >> 8) & 0xFF); };
        const s16 = (a: number) => ((u8(a) | u8(a + 1) << 8) << 16) >> 16;
        const item = (i: number) => sym('items') + i * ITEM_SIZE;
        // Everything held still, as the pickup scene does, so a changed
        // picture is the entity and nothing else.
        const hold = (fields: number) => {
            for (let i = 0; i < fields; i++) {
                emulator.executeFrames(1);
                w16(sym('ol_ticks'), 0); w16(sym('ol_animFreeze'), 1);
            }
        };
        const frame = () => emulator.getPPU().getFramebuffer().slice();
        const settle = () => {
            let last = frame();
            for (let n = 0; n < 40; n++) {
                hold(120);
                const now = frame();
                if (now.every((v, i) => v === last[i])) return now;
                last = now;
            }
            throw new Error('the picture never settled');
        };
        const differs = (a: Uint16Array | Uint8Array | number[], b: typeof a) => {
            let n = 0;
            for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) n++;
            return n;
        };
        const WOLF = 26, FAR = 14;
        const empty = settle();
        expect([u8(item(WOLF) + ITEM.type), u8(item(WOLF) + ITEM.room)], 'the room-14 wolf moved in the level data').toEqual([7, FAR]);

        // Stand it 2048 out, 20 degrees off her line so her own body does not
        // hide it, in her room: nothing of room 14 is in this view, so before
        // the relink it could never be drawn here.
        const lara = item(0), room = u8(lara + ITEM.room);
        const yaw = (u8(lara + ITEM.angleY) | u8(lara + ITEM.angleY + 1) << 8) * Math.PI / 32768 + 0.35;
        w16(item(WOLF) + ITEM.x, s16(lara + ITEM.x) + Math.round(Math.sin(yaw) * 2048));
        w16(item(WOLF) + ITEM.y, s16(lara + ITEM.y));
        w16(item(WOLF) + ITEM.z, s16(lara + ITEM.z) + Math.round(Math.cos(yaw) * 2048));
        memory.write(item(WOLF) + ITEM.room, room);
        const shown = settle();
        const drawn = differs(empty, shown);

        // The same place, its room byte back on room 14: the room decides.
        memory.write(item(WOLF) + ITEM.room, FAR);
        const gone = differs(empty, settle());
        console.log(`[openlara] entities: the room-14 wolf in room ${room} changes ${drawn} framebuffer words; ` +
            `named back to room 14 in the same place, ${gone}`);
        // The scene is frozen and settled, and the picture returns exactly
        // when only the room byte changes back, so the difference is the wolf.
        // FX3's 216x144 view draws it smaller (84 words against 318).
        expect(drawn, 'an entity moved into the view room was not drawn there').toBeGreaterThan(50);
        expect(gone, 'an entity was drawn in a room it is not in').toBe(0);
    }, 600_000);
});

describe.skipIf(!FX3 || LEVEL !== 'LEVEL1')('OpenLara FX3: darts', () => {
    it.skipIf(!SPC)('fires from the real pad in room 6 and plays SND_DART on the S-DSP', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        const w16 = (a: number, v: number) => { memory.write(a, v & 0xFF); memory.write(a + 1, (v >> 8) & 0xFF); };
        const plan = JSON.parse(readFileSync(SOUND_JSON, 'utf8')) as { ids: Record<string, { first: number }> };
        const dartSource = plan.ids['151']?.first;
        expect(dartSource, 'SND_DART is not in the sound plan').toBeDefined();
        emulator.executeFrames(120);
        // Onto emitter 5's pad (the trigger scene's), at a gameUpdate entry
        // so no in-flight room query puts her back.
        const cpu = emulator.getCPU(), original = cpu.step;
        let applied = false;
        cpu.step = function () {
            if (!applied && (cpu.PB << 16 | cpu.PC) === sym('gameUpdate')) {
                memory.write(sym('items') + ITEM.room, 6);
                w16(sym('items') + ITEM.x, 7680); w16(sym('items') + ITEM.y, 2560); w16(sym('items') + ITEM.z, 22016);
                applied = true;
            }
            return original.call(this);
        };
        try { for (let i = 0; i < 60 && !applied; i++) emulator.executeFrames(1); } finally { cpu.step = original; }
        expect(applied).toBe(true);
        const { dsp } = apuOf(emulator);
        let darts = 0, asked = false, played = false;
        for (let i = 0; i < 180; i++) {
            emulator.executeFrames(1);
            darts = Math.max(darts, u16(sym('olDartsLive')));
            if ((u16(sym('ol_sndLast')) >> 8) === 151) asked = true;
            if (dsp().voices.some(v => v.keyedOn && v.source === dartSource)) played = true;
        }
        const health = u16(sym('items') + ITEM.health);
        console.log(`[openlara] darts: on the pad, ${darts} in flight at most; SND_DART sent ${asked}, ` +
                    `source ${dartSource} keyed on ${played}; she is in the line, health ${health}`);
        expect(darts, 'standing on the pad fired no dart').toBeGreaterThan(0);
        expect(asked, 'the emitter never asked for SND_DART').toBe(true);
        expect(played, 'no voice played the dart sample').toBe(true);
        expect(health % 50, 'darts take 50 at a time').toBe(0);
        expect(health, 'she stood in the line and was never hit').toBeLessThan(1000);
    }, 600_000);

    it('fires from the pool on each FIRE loop, flies to the wall and takes 50 a contact tick', () => {
        const g = movementCartridge(); g.restoreLevel();
        const n = g.u16(g.symbol('itemsCount'));
        const at = (i: number) => g.item + i * ITEM_SIZE, argsOf = (i: number) => [at(i) & 65535, at(i) >>> 16];
        const pool = [...Array(n).keys()].filter(i => g.memory.read(at(i) + ITEM.type) === 39);
        const emitters = [...Array(n).keys()].filter(i => g.memory.read(at(i) + ITEM.type) === 40);
        expect([pool.length, emitters.length], 'two hidden darts per emitter').toEqual([20, 10]);
        pool.forEach(i => expect(g.memory.read(at(i) + ITEM.flags2) & 3, `dart ${i} starts hidden`).toBe(3));

        // Emitter 5, room 6, faces -90 degrees: upstream's getBlockOffset puts
        // the dart 412 along +x and 512 up, flying -x at its animation's 130.
        const E = 5, ex = g.s16(at(E) + ITEM.x), ey = g.s16(at(E) + ITEM.y), ez = g.s16(at(E) + ITEM.z);
        expect([g.memory.read(at(E) + ITEM.type), g.memory.read(at(E) + ITEM.room), g.u16(at(E) + ITEM.angleY)]).toEqual([40, 6, 0xC000]);
        g.w8(at(E) + ITEM.flags, g.memory.read(at(E) + ITEM.flags) | 0x1F);   // every trigger bit: active
        g.w16(at(E) + ITEM.timer, 0);
        g.call('ItemObj_activate', argsOf(E));

        // Every dart tracked by its slot: where it was spawned, how fast it
        // went, and that it was removed exactly when the floor under it rose
        // to meet it (a wall's floor being upstream's WALL).
        const flight = (lara: number[] | null, ticks: number) => {
            const born = new Map<number, number>(), last = new Map<number, number>();
            const lives: number[] = [], fires: number[] = [], hits: number[] = [];
            let speed = 0;
            if (lara) lara.forEach((v, i) => g.w16(g.item + ITEM.x + i * 2, v));
            else g.w16(g.item + ITEM.x, 0);
            g.w8(g.item + ITEM.room, 6); g.w16(g.item + ITEM.health, 1000);
            g.w16(g.symbol('cinfo') + 26, 100);
            for (let t = 0; t < ticks; t++) {
                const live = pool.filter(i => (g.memory.read(at(i) + ITEM.flags2) & 3) !== 3);
                g.call('ItemObj_update', [...argsOf(E), ...g.args]);
                for (const i of live) g.call('ItemObj_update', [...argsOf(i), ...g.args]);
                for (const i of pool) {
                    const flying = (g.memory.read(at(i) + ITEM.flags2) & 3) !== 3, x = g.s16(at(i) + ITEM.x);
                    if (flying && !live.includes(i)) {
                        expect([x, g.s16(at(i) + ITEM.y), g.s16(at(i) + ITEM.z), g.u16(at(i) + ITEM.angleY)],
                            'spawned where getBlockOffset says').toEqual([(ex + 412) << 16 >> 16, ey - 512, ez, 0xC000]);
                        born.set(i, t); fires.push(t);
                    } else if (flying) {
                        speed = ((last.get(i)! - x) << 16) >> 16;
                    } else if (live.includes(i)) {
                        const floor = g.s16(at(i) + 26), y = g.s16(at(i) + ITEM.y);
                        expect(y, `dart ${i} removed above its floor ${floor}`).toBeGreaterThanOrEqual(floor === 32767 ? -32512 : floor);
                        lives.push(t - born.get(i)!);
                    }
                    last.set(i, x);
                }
                const hp = g.s16(g.item + ITEM.health);
                if (g.u16(g.symbol('olDartsLive'))) g.call('Dart_collideLara', g.args);
                if (g.s16(g.item + ITEM.health) < hp) hits.push(t);
            }
            return { fires, lives, speed, hits };
        };
        const free = flight(null, 100);
        console.log(`[openlara] darts: fired at ${free.fires}, lived ${free.lives} ticks, ${free.speed} a tick`);
        expect(free.speed).toBe(130);
        expect(free.fires.length, 'fires once per 30-tick loop').toBe(4);
        expect(free.fires.slice(1).map((t, k) => t - free.fires[k])).toEqual([30, 30, 30]);
        expect(free.lives.length).toBeGreaterThanOrEqual(3);
        free.lives.forEach(l => expect(l, 'flew across the room').toBeGreaterThan(20));
        expect(free.hits).toEqual([]);

        // Standing in the line: 50 a contact tick, and the dart flies on
        // (upstream does not remove it on a hit).
        pool.forEach(i => { g.call('ItemObj_deactivate', argsOf(i)); g.w8(at(i) + ITEM.flags2, (g.memory.read(at(i) + ITEM.flags2) & ~3) | 3); });
        g.w16(g.symbol('olDartsLive'), 0);
        const hit = flight([(ex - 1536) << 16 >> 16, ey, ez], 40);
        console.log(`[openlara] darts: Lara in the line takes ${1000 - g.s16(g.item + ITEM.health)} over ticks ${hit.hits}`);
        expect(hit.hits.length, 'the dart passed through her').toBeGreaterThan(0);
        expect(g.s16(g.item + ITEM.health)).toBe(1000 - 50 * hit.hits.length);
        expect(hit.lives.length, 'a hit removed the dart').toBe(0);

        // Deactivated: IDLE, then off the active list; nothing more fires.
        g.w8(at(E) + ITEM.flags, g.memory.read(at(E) + ITEM.flags) & ~0x1F);
        const after = flight(null, 70);
        expect(after.fires.length, 'fired after deactivation').toBeLessThanOrEqual(1);
        expect(g.memory.read(at(E) + ITEM.flags) & 0x40, 'an idle emitter stays on the active list').toBe(0);
    });
});

// Upstream's effects (src/fx.c): a packer-made pool of hidden entities, one
// effects model whose animation is the kind and whose frame is the sprite.
describe.skipIf(!FX3)('OpenLara FX3: effects', () => {
    const FX = { splash: 0, bubble: 1, blood: 2, smoke: 3, ricochet: 4 };
    const TYPES = [153, 155, 158, 160, 164];
    const fxCartridge = () => {
        const g = movementCartridge(); g.restoreLevel(); g.call('Fx_levelInit');
        const first = g.u16(g.symbol('olFxFirst')), count = g.u16(g.symbol('olFxCount'));
        const at = (i: number) => g.item + i * ITEM_SIZE;
        const argsOf = (i: number) => [at(i) & 65535, at(i) >>> 16];
        const pool = [...Array(count).keys()].map(k => first + k);
        const live = () => pool.filter(i => (g.memory.read(at(i) + ITEM.flags2) & 3) !== 3);
        const anim0 = g.u16(at(first) + ITEM.animIndex);   // olLevelInit: the model's first
        return { g, first, count, at, argsOf, pool, live, anim0 };
    };

    it('takes effects from the pool and plays them as upstream SpriteEffect does', () => {
        const { g, first, count, at, argsOf, pool, live, anim0 } = fxCartridge();
        expect(count, 'the effects pool').toBe(24);
        expect(first + count, 'the pool is the last of the entities').toBe(g.u16(g.symbol('itemsCount')));
        pool.forEach(i => {
            expect(g.memory.read(at(i) + ITEM.type), `entity ${i} is not a pooled effect`).toBe(158);
            expect(g.memory.read(at(i) + ITEM.flags2) & 3, `effect ${i} starts hidden`).toBe(3);
        });
        const play = (kind: number, spawn: () => void, ticks: number) => {
            const before = live();
            spawn();
            const born = live().filter(i => !before.includes(i));
            expect(born.length, `kind ${kind} took one from the pool`).toBe(1);
            const i = born[0];
            expect(g.memory.read(at(i) + ITEM.type)).toBe(TYPES[kind]);
            expect(g.u16(at(i) + ITEM.animIndex) - anim0, 'the kind is the animation').toBe(kind);
            expect(g.memory.read(at(i) + ITEM.flags) & 0x40, 'not on the active list').toBe(0x40);
            const frames: number[] = [], xs: number[] = [];
            for (let t = 0; t < ticks; t++) {
                g.call('ItemObj_update', [...argsOf(i), ...g.args]);
                if ((g.memory.read(at(i) + ITEM.flags2) & 3) === 3) {
                    expect(g.memory.read(at(i) + ITEM.flags) & 0x40, 'removed but still active').toBe(0);
                    return { frames, xs, gone: t + 1 };
                }
                frames.push(g.memory.read(at(i) + ITEM.frameIndex)); xs.push(g.s16(at(i) + ITEM.x));
            }
            return { frames, xs, gone: 0 };
        };
        // fxBlood: timer 4, animated through its three frames, moving with
        // the speed and angle it was given (+x at 0x4000)
        const blood = play(FX.blood, () => g.call('Fx_blood', [0, 1000, -500, 2000, 0x4000, 20]), 40);
        expect(blood.frames).toEqual([0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2]);
        expect(blood.gone, 'blood lives three frames of four ticks').toBe(12);
        expect(blood.xs).toEqual(blood.xs.map((_, k) => 1000 + 20 * (k + 1)));
        // fxRicochet: a random frame, four ticks, not animated
        const ricochet = play(FX.ricochet, () => g.call('Fx_ricochet', [0, 1000, -500, 2000, 0]), 40);
        expect(ricochet.gone).toBe(4);
        expect(new Set(ricochet.frames).size).toBe(1);
        expect(ricochet.frames[0]).toBeLessThan(3);
        // fxSmoke: timer 3 over its seven frames
        const smoke = play(FX.smoke, () => g.call('Fx_smoke', [0, 1000, -500, 2000]), 60);
        expect(smoke.gone).toBe(21);
        expect(smoke.frames.filter((f, k) => k && f !== smoke.frames[k - 1]).length).toBe(6);
        console.log(`[openlara] effects: pool ${first}..${first + count - 1}; blood ${blood.frames.join('')}, ` +
            `ricochet frame ${ricochet.frames[0]} for 3 ticks, smoke gone at tick ${smoke.gone}`);

        // Published to the renderer as any entity is: the kind's local
        // animation in the low byte, the sprite frame in the high one.
        g.call('Fx_blood', [0, 1000, -500, 2000, 0, 0]);
        const b = live()[0];
        for (let t = 0; t < 5; t++) g.call('ItemObj_update', [...argsOf(b), ...g.args]);
        g.call('gameRenderSync');
        const rec = g.symbol('ol_itemState_gsu') + b * 12;
        expect([g.memory.read(rec), g.memory.read(rec + 1)], 'the record the GSU draws').toEqual([FX.blood, 1]);
        for (let t = 0; t < 7; t++) g.call('ItemObj_update', [...argsOf(b), ...g.args]);
        g.call('gameRenderSync');
        expect(g.memory.read(rec), 'a removed effect is not drawn').toBe(0xFF);

        // The native walk (olUpdateItems' FX_Sprite, render_sync.asm) against
        // fx.c's C, tick by tick: an odd angle at a byte's speed, and a speed
        // past a byte, which the native hands to the C.
        for (const speed of [97, -61, 300]) {
            const pair: number[] = [];
            for (let k = 0; k < 2; k++) {
                const before = live();
                g.call('Fx_blood', [0, 1000, -500, 2000, 0x2345, speed & 65535]);
                pair.push(live().filter(i => !before.includes(i))[0]);
            }
            const [c, n] = pair;
            const state = (i: number) => [g.s16(at(i) + ITEM.x), g.s16(at(i) + ITEM.z),
                g.memory.read(at(i) + ITEM.frameIndex), g.memory.read(at(i) + ITEM.flags2) & 3];
            for (let t = 0; t < 13; t++) {
                if ((g.memory.read(at(c) + ITEM.flags2) & 3) !== 3) g.call('ItemObj_update', [...argsOf(c), ...g.args]);
                if ((g.memory.read(at(n) + ITEM.flags2) & 3) !== 3) {
                    g.w16(g.symbol('olFirstActive'), n); g.w8(at(n) + ITEM.nextActive, 255);
                    g.call('olUpdateItems');
                }
                expect(state(n), `native and C apart at tick ${t}, speed ${speed}`).toEqual(state(c));
            }
            expect(state(n)[3], 'the native effect outlived its frames').toBe(3);
        }
        g.w16(g.symbol('olFirstActive'), 255);

        // The pool runs dry as upstream's free list does: nothing happens.
        pool.forEach(i => g.w8(at(i) + ITEM.flags2, g.memory.read(at(i) + ITEM.flags2) | 3));
        g.w16(g.symbol('olFxLive'), 0);
        for (let k = 0; k < count + 3; k++) g.call('Fx_smoke', [0, 1000, -500, 2000]);
        expect(live().length).toBe(count);
    });

    it('turns its joint points to within five units of upstream getJoint', () => {
        const { g, first, at } = fxCartridge();
        const source = JSON.parse(readFileSync(join(DATA_DIR, 'FRAMES.json'), 'utf8')) as {
            animations: { anim: number; type: number; ticks: number }[];
        };
        // FX.BIN's sites: the bites, and her head for FX_LARA_BUBBLES
        const sites = [[7, 6, 0, -14, 174], [8, 14, 0, 96, 335], [9, 4, 0, 16, 45], [19, 22, 0, 66, 318],
                       [0, 14, 0, 0, 50]];
        const fx = (g.u16(g.symbol('olLevel') + 48) | g.u16(g.symbol('olLevel') + 50) << 16) >>> 0;
        const out = 0x7E1F80;
        const cases: number[][] = [];
        let k = 0;
        sites.forEach(([type, joint, ox, oy, oz], site) => {
            const firstAnim = g.u16(fx + 16 + site * 6), anims = g.u16(fx + 18 + site * 6);
            const dir = g.u16(fx + 20 + site * 6);
            for (const a of source.animations.filter(a => a.type === type)) {
                if (!dir || a.anim < firstAnim || a.anim >= firstAnim + anims) continue;
                if (!g.u16(fx + dir + (a.anim - firstAnim) * 2)) continue;    // never asked
                for (let tick = 0; tick < a.ticks; tick += Math.max(1, a.ticks >> 2), k++) {
                    const yaw = [0, 0x4000, 0x9234, 0xE001][k % 4];
                    const pitch = type === 0 ? [0, -8192, 12000, -15500][(k >> 2) % 4] : 0;
                    const roll = type === 0 ? [0, 2002, -4004][k % 3] : 0;
                    cases.push([type, a.anim, tick, pitch, yaw << 16 >> 16, roll, joint, ox, oy, oz, site]);
                }
            }
        });
        const types = new Set(cases.map(c => c[0]));
        expect(types.has(7) && types.has(0), 'no wolf or Lara joints in FX.BIN').toBe(true);
        const oracle = execFileSync(process.env.OL_SPHERE_ORACLE!, [process.env.OL_LEVEL_PHD!, '0', '--joint'], {
            input: cases.map(c => c.slice(0, 10).join(' ')).join('\n') + '\n', maxBuffer: 16 * 1024 * 1024,
        }).toString().trim().split('\n').map(line => line.split(' ').map(Number));
        expect(oracle.length).toBe(cases.length);
        let worst = 0;
        cases.forEach(([type, anim, tick, pitch, yaw, roll, , , , , site], i) => {
            const it = type === 0 ? 0 : first;          // her, or a stand-in record
            g.w16(at(it) + ITEM.animIndex, anim); g.w8(at(it) + ITEM.frameIndex, tick);
            g.w16(at(it) + ITEM.angleY, yaw);
            g.w16(g.symbol('olLaraAngleX'), pitch); g.w16(g.symbol('olLaraAngleZ'), roll);
            g.call('Fx_joint', [at(it) & 65535, at(it) >>> 16, site,
                out & 65535, out >>> 16, (out + 2) & 65535, out >>> 16, (out + 4) & 65535, out >>> 16]);
            expect(g.u16(g.symbol('tcc__r0')), `no joint for ${cases[i]}`).toBe(1);
            const got = [g.s16(out), g.s16(out + 2), g.s16(out + 4)];
            got.forEach((v, c) => { worst = Math.max(worst, Math.abs(v - oracle[i][c])); });
            expect(got.map((v, c) => Math.abs(v - oracle[i][c]) <= 5),
                `type ${type} anim ${anim} tick ${tick} angles ${pitch},${yaw},${roll}: ${got} vs upstream ${oracle[i]}`)
                .toEqual([true, true, true]);
        });
        console.log(`[openlara] effects: ${cases.length} joint points (${[...types].join(',')}) within ${worst} of upstream getJoint`);
    });

    it('fires as weaponFire does: two spread draws, blood at a target, a ricochet at the wall', () => {
        const { g, at, live } = fxCartridge();
        const lara = g.item;
        const seed = () => (g.u16(g.symbol('olRandSeed')) | g.u16(g.symbol('olRandSeed') + 2) << 16) >>> 0;
        const lcg = (s: number) => (Math.imul(s, 0x41C64E6D) + 0x3039) >>> 0;
        // At nothing: two draws, then the line along her aim, traced. Room 0
        // runs 20,000 along z from her start: facing +z the line ends in the
        // air 16384 on; facing -z it meets the wall 512 behind her.
        const shoot = (yaw: number) => {
            g.w16(lara + ITEM.angleY, yaw);
            const s0 = seed(), before = live();
            g.w16(g.symbol('olWeaponTarget'), 0xFF); g.w16(g.symbol('olWeaponFireArm'), 0);
            g.call('Weapon_fire', [0]);
            expect(seed(), 'weaponFire draws aimX and aimY').toBe(lcg(lcg(s0)));
            const r = live().filter(i => !before.includes(i));
            expect(r.length, 'a miss leaves a ricochet').toBe(1);
            expect(g.memory.read(at(r[0]) + ITEM.type)).toBe(164);
            const dx = g.s16(at(r[0]) + ITEM.x) - g.s16(lara + ITEM.x), dz = g.s16(at(r[0]) + ITEM.z) - g.s16(lara + ITEM.z);
            const a = yaw * Math.PI / 32768;
            return { along: dx * Math.sin(a) + dz * Math.cos(a), across: dx * Math.cos(a) - dz * Math.sin(a),
                     y: g.s16(at(r[0]) + ITEM.y) - g.s16(lara + ITEM.y), room: g.memory.read(at(r[0]) + ITEM.room) };
        };
        const open = shoot(0), wall = shoot(0x8000);
        console.log(`[openlara] effects: from (${g.s16(lara + ITEM.x)},${g.s16(lara + ITEM.y)},${g.s16(lara + ITEM.z)}) ` +
            `room ${g.memory.read(lara + ITEM.room)}, a shot at nothing ricochets ${Math.round(open.along)} ahead ` +
            `(${Math.round(open.across)} aside) facing +z, ${Math.round(wall.along)} ahead facing -z`);
        for (const r of [open, wall]) {
            expect(r.along, 'the ricochet is not ahead of her').toBeGreaterThan(0);
            expect(r.along, 'past the line\'s 16384').toBeLessThanOrEqual(16384);
            expect(Math.abs(r.across) / r.along, 'outside the 8-degree spread').toBeLessThan(Math.tan(5 * Math.PI / 180));
        }
        if (LEVEL === 'LEVEL1') {
            [open, wall].forEach(r => expect(r.room).toBe(0));
            expect(open.along, 'the line is 16384 long').toBeGreaterThan(16000);
            expect(wall.along, 'the trace went through the wall').toBeLessThan(1100);
        }

        // At a living target in the arm's aim: blood at its aim point, in its room
        const n = g.u16(g.symbol('itemsCount'));
        const wolf = [...Array(n).keys()].find(i => g.memory.read(at(i) + ITEM.type) === 7)!;
        g.w16(at(wolf) + ITEM.health, 6);
        g.w16(g.symbol('olWeaponTarget'), wolf);
        const before = live();
        g.call('Weapon_fire', [1]);
        const born = live().filter(i => !before.includes(i));
        expect(born.length).toBe(1);
        expect(g.memory.read(at(born[0]) + ITEM.type), 'a hit bleeds').toBe(158);
        expect(g.memory.read(at(born[0]) + ITEM.room)).toBe(g.memory.read(at(wolf) + ITEM.room));
        expect(g.s16(at(wolf) + ITEM.health)).toBe(5);
        const out = 0x7E1F80;
        g.call('Weapon_getAimPoint', [at(wolf) & 65535, at(wolf) >>> 16, out & 65535, out >>> 16]);
        expect([g.s16(at(born[0]) + ITEM.x), g.s16(at(born[0]) + ITEM.y), g.s16(at(born[0]) + ITEM.z)],
            'not at the aim point').toEqual([g.s16(out), g.s16(out + 2), g.s16(out + 4)]);
    });

    it.skipIf(LEVEL !== 'LEVEL1')('draws the sprite the frame names, through the GSU', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const w16 = (a: number, v: number) => { memory.write(a, v & 0xFF); memory.write(a + 1, (v >> 8) & 0xFF); };
        const s16 = (a: number) => ((u8(a) | u8(a + 1) << 8) << 16) >> 16;
        const item = (i: number) => sym('items') + i * ITEM_SIZE;
        let freeze = 1;
        // Pinned at every render sync as well as once a field (the pickups
        // scene says why).
        const hold = (fields: number) => {
            const cpu = emulator.getCPU(), step = cpu.step, sync = sym('gameRenderSync');
            cpu.step = function (this: typeof cpu) {
                if ((this.PB << 16 | this.PC) === sync) { w16(sym('ol_ticks'), 0); w16(sym('ol_animFreeze'), freeze); }
                return step.call(this);
            };
            try {
                for (let i = 0; i < fields; i++) {
                    emulator.executeFrames(1);
                    w16(sym('ol_ticks'), 0); w16(sym('ol_animFreeze'), freeze);
                }
            } finally { cpu.step = step; }
        };
        const frame = () => emulator.getPPU().getFramebuffer().slice();
        const settle = () => {
            let last = frame();
            for (let n = 0; n < 40; n++) {
                hold(120);
                const now = frame();
                if (now.every((v, i) => v === last[i])) return now;
                last = now;
            }
            throw new Error('the picture never settled');
        };
        const differs = (a: ArrayLike<number>, b: ArrayLike<number>) => {
            let n = 0;
            for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) n++;
            return n;
        };
        settle();                   // and the load tick long finished
        const fx = u8(sym('olFxFirst')) | u8(sym('olFxFirst') + 1) << 8;
        expect(u8(item(fx) + ITEM.type)).toBe(158);
        // A splash 900 out in front of her and 500 up, in her room; the
        // freeze draws every entity on animation 0 (the splash's, for the
        // pool) at frame ol_animFreeze - 1, so the freeze picks the sprite.
        // It poses her too, so each frame is held against the picture
        // without the splash at the same freeze.
        const lara = item(0), yaw = (u8(lara + ITEM.angleY) | u8(lara + ITEM.angleY + 1) << 8) * Math.PI / 32768;
        w16(item(fx) + ITEM.x, s16(lara + ITEM.x) + Math.round(Math.sin(yaw) * 900));
        w16(item(fx) + ITEM.y, s16(lara + ITEM.y) - 500);
        w16(item(fx) + ITEM.z, s16(lara + ITEM.z) + Math.round(Math.cos(yaw) * 900));
        memory.write(item(fx) + ITEM.room, u8(lara + ITEM.room));
        const picture = (f: number, visible: boolean) => {
            freeze = f;
            memory.write(item(fx) + ITEM.flags2, (u8(item(fx) + ITEM.flags2) & ~3) | (visible ? 1 : 3));
            memory.write(sym('olItemDirty') + fx, 1);
            w16(sym('olFxLive'), visible ? 1 : 0);      // render sync's pool test (fx.c)
            w16(sym('olFxPending'), 1);
            return settle() as unknown as Uint16Array;
        };
        const shown: number[] = [], drawn: Uint16Array[] = [], bare: Uint16Array[] = [];
        for (const f of [1, 6, 11]) {
            bare.push(picture(f, false));
            drawn.push(picture(f, true));
            shown.push(differs(bare[bare.length - 1], drawn[drawn.length - 1]));
        }
        {
            const { stride, width, height } = emulator.getPPU().getFrameGeometry();
            writeFileSync(join(dirname(ROM), 'effects_splash.png'), png(width, height, (x, y) => drawn[1][y * stride + x]));
            writeFileSync(join(dirname(ROM), 'effects_splash_bare.png'), png(width, height, (x, y) => bare[1][y * stride + x]));
        }
        // where the two bare pictures agree, the splash's frames 0 and 5
        let between = 0;
        for (let i = 0; i < drawn[0].length; i++)
            if (bare[0][i] === bare[1][i] && drawn[0][i] !== drawn[1][i]) between++;
        const gone = differs(bare[0], picture(1, false));
        console.log(`[openlara] effects: a splash drawn ${shown.join('/')} words at frames 0/5/10, ` +
            `frames 0 and 5 differ in ${between}, hidden again ${gone}`);
        shown.forEach(n => expect(n, 'the effect was not drawn').toBeGreaterThan(10));
        expect(between, 'every frame drew the same sprite').toBeGreaterThan(10);
        expect(gone, 'a hidden effect was drawn').toBe(0);
    }, 600_000);
    it.skipIf(LEVEL !== 'LEVEL1')('draws each arm\'s muzzle flash for the three ticks after its shot', () => {
        // The timer as ItemsExchange works it out: a shot sets the arm's end
        // tick, and the flash shows while it is 1..3 ticks ahead.
        const { g } = fxCartridge();
        g.w16(g.symbol('olWeaponTarget'), 0xFF);
        for (const arm of [0, 12]) {
            g.w16(g.symbol('olWeaponFireArm'), arm);
            const ticks = g.u16(g.symbol('ol_ticks'));
            g.call('Weapon_fire', [0]);
            expect(g.u16(g.symbol('olWeaponFlash') + (arm ? 4 : 0)), 'flashTimer from this tick')
                .toBe((ticks + 1 + 3) & 65535);
        }

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const w16 = (a: number, v: number) => { memory.write(a, v & 0xFF); memory.write(a + 1, (v >> 8) & 0xFF); };
        let flash = [0, 0];
        // Held still, pistols drawn and aimed (READY, both arms on AIM's
        // last frame), the flash ends pinned against a frozen ol_ticks of 0.
        const hold = (fields: number) => {
            for (let i = 0; i < fields; i++) {
                emulator.executeFrames(1);
                w16(sym('ol_ticks'), 0); w16(sym('ol_animFreeze'), 1);
                w16(sym('olLaraWeaponState'), 4);
                for (const k of [0, 1]) {
                    const arm = sym('olWeaponArms') + k * 12;
                    w16(arm, 0); w16(arm + 2, 4); w16(arm + 4, 0); w16(arm + 6, 0); w16(arm + 8, 1); w16(arm + 10, 1);
                    w16(sym('olWeaponFlash') + k * 4, flash[k]); w16(sym('olWeaponFlash') + k * 4 + 2, 0x30);
                }
            }
        };
        const frame = () => emulator.getPPU().getFramebuffer().slice();
        const settle = () => {
            let last = frame();
            for (let n = 0; n < 40; n++) {
                hold(60);
                const now = frame();
                if (now.every((v, i) => v === last[i])) return now;
                last = now;
            }
            throw new Error('the picture never settled');
        };
        const differs = (a: ArrayLike<number>, b: ArrayLike<number>) => {
            let n = 0;
            for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) n++;
            return n;
        };
        const none = settle();
        flash = [2, 0];
        const right = settle();
        {
            const { stride, width, height } = emulator.getPPU().getFrameGeometry();
            writeFileSync(join(dirname(ROM), 'effects_flash.png'), png(width, height, (x, y) => right[y * stride + x]));
        }
        flash = [0, 2];
        const left = settle();
        // five ahead: not yet a flash. The scene pins ol_ticks to 0 once a
        // field, but a tick inside the field makes it 1 until then, and a
        // publication can land there (they are frequent since the letterbox
        // upload): four ahead is then three, inside the flash's own ticks.
        flash = [5, 5];
        const early = settle();
        flash = [0, 0];
        const after = settle();
        console.log(`[openlara] effects: muzzle flash ${differs(none, right)} words (right), ` +
            `${differs(none, left)} (left), ${differs(right, left)} between them; ${differs(none, early)} out of range`);
        expect(differs(none, right), 'no right-hand flash').toBeGreaterThan(10);
        expect(differs(none, left), 'no left-hand flash').toBeGreaterThan(10);
        expect(differs(right, left), 'both flashes at one hand').toBeGreaterThan(10);
        expect(differs(none, early), 'a flash outside its three ticks').toBe(0);
        expect(differs(none, after), 'the flash stayed').toBe(0);
        expect(u8(sym('olLaraWeaponState')), 'the weapon state did not hold').toBe(4);
    }, 600_000);
    it.skipIf(LEVEL !== 'LEVEL2')('splashes where she falls in and breathes bubbles that rise to the surface', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const s16 = (a: number) => (u16(a) << 16) >> 16;
        const w8 = (a: number, v: number) => memory.write(a, v & 0xFF);
        const w16 = (a: number, v: number) => { w8(a, v); w8(a + 1, v >> 8); };
        const item = (i: number) => sym('items') + i * ITEM_SIZE;
        // The water scene's pool: room 43 over room 48, surface at -1280.
        const ROOM = 43, X = 29696, Y = -1800, Z = 22528, SURFACE_Y = -1280;
        emulator.executeFrames(180);
        expect(u16(sym('itemsCount')), 'the host table never arrived').toBeGreaterThan(0);
        const first = u16(sym('olFxFirst')), count = u16(sym('olFxCount'));
        expect(count).toBe(24);
        const live = (type: number) => [...Array(count).keys()].map(k => first + k)
            .filter(i => (u8(item(i) + ITEM.flags2) & 3) !== 3 && u8(item(i) + ITEM.type) === type);
        for (let i = 0; i < 6; i++) {
            w8(item(0) + ITEM.room, ROOM);
            w8(item(0) + ITEM.state, 2); w8(item(0) + ITEM.goalState, 2);
            w16(item(0) + ITEM.x, X); w16(item(0) + ITEM.y, Y); w16(item(0) + ITEM.z, Z);
            w16(item(0) + ITEM.angleY, 0xC000);
            w16(item(0) + ITEM.animIndex, 103); w8(item(0) + ITEM.frameIndex, 0);
            w8(item(0) + ITEM.flags2, 0); w16(item(0) + ITEM.vSpeed, 0); w16(item(0) + ITEM.hSpeed, 0);
            w16(item(0) + ITEM.health, 1000);
            w16(sym('olLaraMoveAngle'), 0xC000); w16(sym('olLaraTurn'), 0);
            w16(sym('olLaraWaterState'), 0); w16(sym('olLaraAngleX'), 0);
            emulator.executeFrames(1);
        }
        const pad = emulator.getController() as unknown as { buttonState: number };
        // 1. in: ten splashes at the surface over her, outward, animated
        // (a field can end part-way through the tick that spawns them: keep
        // the largest set over the fields after the first is seen)
        let splashes: number[] = [];
        for (let f = 0, after = 0; f < 240 && after < 3; f++) {
            w16(item(0) + ITEM.health, 1000);
            emulator.executeFrames(1);
            const now = live(153);
            if (now.length > splashes.length) splashes = now;
            if (splashes.length) after++;
        }
        expect(splashes.length, 'no splash where she fell in').toBe(10);
        splashes.forEach(i => expect(s16(item(i) + ITEM.y), 'a splash off the surface').toBe(SURFACE_Y));
        const spread = new Set(splashes.map(i => u16(item(i) + ITEM.angleY))).size;
        emulator.executeFrames(30);
        const lasting = live(153).length;
        // 2. under: bubbles from her head, rising until the surface
        pad.buttonState = PAD_A;
        let seen = 0, highest = 0;
        const rise = new Map<number, number[]>();
        const born = new Set<number>(), ended = new Set<number>();
        for (let f = 0; f < 900; f++) {
            w16(item(0) + ITEM.health, 1000);
            w16(sym('olLaraOxygen'), 1800);
            emulator.executeFrames(1);
            const now = live(155);
            now.forEach(i => {
                if (!rise.has(i) || ended.has(i)) { rise.set(i, []); born.add(i); ended.delete(i); seen++; }
                rise.get(i)!.push(s16(item(i) + ITEM.y));
            });
            [...rise.keys()].filter(i => !now.includes(i) && !ended.has(i)).forEach(i => {
                ended.add(i); highest = Math.min(highest, ...rise.get(i)!);
            });
            if (ended.size >= 3) break;
        }
        pad.buttonState = 0;
        console.log(`[openlara] effects: ${splashes.length} splashes at y ${SURFACE_Y} in ${spread} directions, ` +
            `${lasting} still playing a second later; ${seen} bubbles, ${ended.size} burst, highest at y ${highest}`);
        expect(spread, 'the splashes all went one way').toBeGreaterThan(5);
        expect(lasting, 'a splash outlived its eleven frames').toBe(0);
        expect(seen, 'she breathed no bubbles').toBeGreaterThan(0);
        expect(ended.size, 'no bubble reached the surface').toBeGreaterThan(0);
        for (const i of ended) {
            const ys = rise.get(i)!;
            // sampled a field at a time: a tick every other sample
            expect(ys.slice(1).every((y, k) => y <= ys[k]) && ys[ys.length - 1] < ys[0],
                `bubble ${i} did not rise: ${ys}`).toBe(true);
            expect(ys[ys.length - 1], 'burst short of the surface').toBeLessThan(SURFACE_Y + 16 * 3);
        }
    }, 600_000);
});

describe.skipIf(!FX3 || LEVEL !== 'LEVEL1')('OpenLara FX3: the level fade', () => {
    it('fades the level in from upstream\'s -128 by 16 a tick, subtracting the fixed colour', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u16 = (a: number) => memory.read(a) | memory.read(a + 1) << 8;
        const ppu = emulator.getPPU() as unknown as { getDiagnostics(): { colorMath: {
            operation: string; layers: number; fixedColor: readonly number[] } } };
        const seen: number[] = [];
        let loaded = -1;
        for (let field = 0; field < 400 && (loaded < 0 || field < loaded + 60); field++) {
            emulator.executeFrames(1);
            const math = ppu.getDiagnostics().colorMath;
            if (loaded < 0 && u16(sym('itemsCount'))) loaded = field;
            if (loaded >= 0) {
                expect(math.operation).toBe('subtract');
                expect(math.layers & 1, 'BG1 is not in the colour math').toBe(1);
                expect(new Set(math.fixedColor).size, 'the channels fade apart').toBe(1);
                seen.push(math.fixedColor[0]);
            }
        }
        expect(loaded, 'the level never loaded').toBeGreaterThanOrEqual(0);
        // from the end of the load tick (it spans fields: olLevelInit, the snap)
        const lead = seen.findIndex(v => v !== 0);
        expect(lead, 'no fade at all').toBeGreaterThanOrEqual(0);
        seen.splice(0, lead);
        while (seen.length > 2 && seen[seen.length - 1] === 0 && seen[seen.length - 2] === 0) seen.pop();
        console.log(`[openlara] fade: ${lead} fields into the load tick, every channel less ${seen.join(',')}`);
        // 28 on the first field the NMI sees it, then down 4 every other
        // field -- a tick of real time -- to nothing, as palBright(-32 + 4k).
        const steps = seen.filter((v, k) => v !== 0 && (k === 0 || v !== seen[k - 1]));
        expect(steps).toEqual([28, 24, 20, 16, 12, 8, 4]);
        expect(seen.slice(1).every((v, k) => v <= seen[k]), 'the fade went back').toBe(true);
        expect(seen[seen.length - 1]).toBe(0);
        const lengths = steps.slice(1).map(v => seen.filter(s => s === v).length);
        expect(lengths.every(n => n === 2), `a step held for ${lengths} fields`).toBe(true);
    }, 600_000);
});

describe.skipIf(!FX3 || LEVEL !== 'LEVEL1')('OpenLara FX3: room sprites', () => {
    it('draws the rooms\' sprites as camera-facing quads where TR places them', () => {
        const placed = JSON.parse(readFileSync(join(DATA_DIR, 'ROOMSPRITES.json'), 'utf8')) as {
            sprites: { room: number; offset: number; x: number; y: number; z: number }[] };
        expect(placed.sprites.length, 'LEVEL1 has 43 room sprites').toBe(43);
        const bankBase = Number(/LEVEL_BANK_BASE \$([0-9A-Fa-f]+)/.exec(
            readFileSync(join(dirname(ROM), 'texbank.inc'), 'utf8'))![1].replace(/^/, '0x'));
        const rom = new Uint8Array(readFileSync(ROM));
        // every placement's angle byte is the marker, its bank a real one
        for (const s of placed.sprites) {
            const at = (bankBase - 0x40) * 0x10000 + s.offset;
            expect([rom[at + 3], rom[at + 2] !== 0xFF]).toEqual([1, true]);
        }
        const without = rom.slice();
        for (const s of placed.sprites) without[(bankBase - 0x40) * 0x10000 + s.offset + 2] = 0xFF;

        // Lara 1200 out from room 10's first sprite, facing it, held still
        const target = placed.sprites.find(s => s.room === 10)!;
        const view = (image: Uint8Array) => {
            const emulator = new SNESEmulator();
            emulator.loadROM(image);
            awaitLevel(emulator);
            const memory = emulator.getMemory(), cpu = emulator.getCPU();
            const w16 = (a: number, v: number) => { memory.write(a, v & 0xFF); memory.write(a + 1, (v >> 8) & 0xFF); };
            const lara = sym('items');
            const sx = target.x << 16 >> 16, sz = target.z << 16 >> 16;
            const original = cpu.step;
            let applied = 0;
            cpu.step = function () {
                if (applied < 4 && (cpu.PB << 16 | cpu.PC) === sym('gameUpdate')) {
                    memory.write(lara + ITEM.room, 10);
                    w16(lara + ITEM.x, sx + 1200); w16(lara + ITEM.y, target.y); w16(lara + ITEM.z, sz);
                    w16(lara + ITEM.angleY, 0xC000);    // facing -x, at the sprite
                    applied++;
                }
                return original.call(this);
            };
            try { for (let i = 0; i < 120 && applied < 4; i++) emulator.executeFrames(1); } finally { cpu.step = original; }
            expect(applied).toBe(4);
            let last = emulator.getPPU().getFramebuffer().slice();
            for (let n = 0; n < 30; n++) {
                for (let i = 0; i < 120; i++) { emulator.executeFrames(1); w16(sym('ol_animFreeze'), 1); }
                const now = emulator.getPPU().getFramebuffer().slice();
                if (now.every((v, i) => v === last[i])) {
                    expect(memory.read(lara + ITEM.room), 'she left room 10').toBe(10);
                    return { picture: now, geometry: emulator.getPPU().getFrameGeometry() };
                }
                last = now;
            }
            throw new Error('the picture never settled');
        };
        const shown = view(rom), bare = view(without);
        let differ = 0;
        for (let i = 0; i < shown.picture.length; i++) if (shown.picture[i] !== bare.picture[i]) differ++;
        const { stride, width, height } = shown.geometry;
        writeFileSync(join(dirname(ROM), 'room_sprites.png'), png(width, height, (x, y) => shown.picture[y * stride + x]));
        writeFileSync(join(dirname(ROM), 'room_sprites_bare.png'), png(width, height, (x, y) => bare.picture[y * stride + x]));
        console.log(`[openlara] room sprites: ${placed.sprites.length} placed; room 10's, seen from 1200 away, ` +
            `change ${differ} framebuffer words`);
        expect(differ, 'no room sprite was drawn').toBeGreaterThan(50);
    }, 600_000);
});

describe.skipIf(!FX3 || LEVEL !== 'LEVEL2')('OpenLara FX3: swinging blades', () => {
    it('computes upstream getSpheres and collideSpheres exactly for the blade against Lara', () => {
        const g = movementCartridge(); g.restoreLevel();
        const n = g.u16(g.symbol('itemsCount'));
        const at = (i: number) => g.item + i * ITEM_SIZE;
        const B = [...Array(n).keys()].find(i => g.memory.read(at(i) + ITEM.type) === 36)!;
        const source = JSON.parse(readFileSync(join(DATA_DIR, 'FRAMES.json'), 'utf8')) as {
            animations: { anim: number; type: number; ticks: number }[];
        };
        const bladeAnims = source.animations.filter(a => a.type === 36);
        const laraAnims = source.animations.filter(a => a.type === 0);
        expect(bladeAnims.length).toBe(3);
        expect(laraAnims.length).toBeGreaterThan(100);
        const cases: number[][] = [];
        let k = 0;
        for (const b of bladeAnims) for (let bf = 0; bf < b.ticks; bf++) {
            for (let r = 0; r < 6; r++, k++) {
                const l = laraAnims[(k * 37) % laraAnims.length];
                const lf = (k * 13) % l.ticks;
                const yaw = [0, 0x4000, 0x8000, 0xC000, 0x1234, 0xFEDC][k % 6] << 16 >> 16;
                const pitch = [0, 0, 0, 0x0800, -0x1000, 0x2AAA][(k >> 1) % 6];
                const bx = 22016, by = -1024, bz = 29184;
                cases.push([b.anim, bf, bx, by, bz, -16384,
                    l.anim, lf, bx + ((k * 211) % 1600) - 800, by - 900 + ((k * 97) % 1200),
                    bz + ((k * 173) % 3000) - 1500, yaw, pitch]);
            }
        }
        const oracle = execFileSync(process.env.OL_SPHERE_ORACLE!, [process.env.OL_LEVEL_PHD!, '0'], {
            input: cases.map(c => c.join(' ')).join('\n') + '\n', maxBuffer: 16 * 1024 * 1024,
        }).toString().trim().split('\n');
        expect(oracle.length).toBe(cases.length);
        const spheres = g.symbol('olSpheres');
        const s32 = (a: number) => (g.u16(a) | g.u16(a + 2) << 16) | 0;
        let hits = 0, steps = 0, compared = 0;
        cases.forEach((c, i) => {
            const [ba, bf, bx, by, bz, bay, la, lf, lx, ly, lz, lay, lp] = c;
            g.w16(at(B) + ITEM.animIndex, ba); g.w8(at(B) + ITEM.frameIndex, bf);
            [bx, by, bz].forEach((v, j) => g.w16(at(B) + ITEM.x + j * 2, v)); g.w16(at(B) + ITEM.angleY, bay);
            g.w16(g.item + ITEM.animIndex, la); g.w8(g.item + ITEM.frameIndex, lf);
            [lx, ly, lz].forEach((v, j) => g.w16(g.item + ITEM.x + j * 2, v)); g.w16(g.item + ITEM.angleY, lay);
            g.w16(g.symbol('olLaraAngleX'), lp);
            steps += g.call('Trap_collideSpheres', [at(B) & 65535, at(B) >>> 16, ...g.args]);
            const mask = (g.u16(g.symbol('tcc__r0')) | g.u16(g.symbol('tcc__r1')) << 16) >>> 0;
            const want = oracle[i].split(' ').map(Number);
            const got: number[] = [];
            for (const side of [0, 1]) {
                const count = side ? 15 : 3;
                got.push(count);
                for (let q = 0; q < count * 4; q++) got.push(s32(spheres + side * 16 * 16 + q * 4));
            }
            got.push(mask);
            expect(got, `case ${i}: ${c.join(' ')}`).toEqual(want);
            hits += mask ? 1 : 0; compared++;
        });
        console.log(`[openlara] spheres: ${compared} blade/Lara poses equal upstream's getSpheres and collideSpheres ` +
                    `(${hits} hit); ${Math.round(steps / compared)} instructions a call`);
        expect(hits).toBeGreaterThan(20);
        expect(compared - hits).toBeGreaterThan(20);

        // The fast path play asks first. Its data, proved against upstream's
        // own code: every stored frame of hers at all 4,096 table yaws within
        // the bound, the blade's centres exact.
        const table = execFileSync(process.env.OL_SPHERE_ORACLE!, [process.env.OL_LEVEL_PHD!, '0', '--table',
            join(DATA_DIR, 'SPHERE.BIN')]).toString().trim();
        console.log(`[openlara] spheres: ${table}`);
        expect(table).toMatch(/^table ok: \d+ frames x 4096 yaws \(\d+ centres\) within 1; 91 blade ticks exact over 1 yaws$/);
        // Its answers: certain ones must be upstream's, whatever the pose.
        const fast = (c: number[]) => {
            const [ba, bf, bx, by, bz, bay, la, lf, lx, ly, lz, lay, lp] = c;
            g.w16(at(B) + ITEM.animIndex, ba); g.w8(at(B) + ITEM.frameIndex, bf);
            [bx, by, bz].forEach((v, j) => g.w16(at(B) + ITEM.x + j * 2, v)); g.w16(at(B) + ITEM.angleY, bay);
            g.w16(g.item + ITEM.animIndex, la); g.w8(g.item + ITEM.frameIndex, lf);
            [lx, ly, lz].forEach((v, j) => g.w16(g.item + ITEM.x + j * 2, v)); g.w16(g.item + ITEM.angleY, lay);
            g.w16(g.symbol('olLaraAngleX'), lp);
            const n = g.call('Trap_sphereTest', [at(B) & 65535, at(B) >>> 16, ...g.args]);
            return { n, answer: g.u16(g.symbol('tcc__r0')) };
        };
        cases.forEach((c, i) => {
            const { answer } = fast(c), mask = Number(oracle[i].split(' ').pop());
            if (c[12] !== 0) expect(answer, `case ${i}: pitched, the exact chain decides`).toBe(2);
            else if (answer !== 2) expect(answer, `case ${i}: ${c.join(' ')}`).toBe(mask ? 1 : 0);
        });
        let seed = 0x2545F491;
        const rnd = (n: number) => { seed = (Math.imul(seed, 1103515245) + 12345) >>> 0; return (seed >>> 8) % n; };
        const near: number[][] = [];
        for (let k = 0; k < 4000; k++) {
            const b = bladeAnims[k % 3], l = laraAnims[rnd(laraAnims.length)];
            const bx = 22016, by = -1024, bz = 29184;
            // at this yaw the blade's spheres sweep x +80..+780 of its
            // origin, 1,300-1,900 above it, z within 1,200; hers run from
            // her feet to about 800 above them
            near.push([b.anim, rnd(b.ticks), bx, by, bz, k % 50 === 49 ? 0x1000 : -16384, l.anim, rnd(l.ticks),
                bx - 100 + rnd(1000), by - 1800 + rnd(1600), bz - 1200 + rnd(2400), rnd(65536) << 16 >> 16, 0]);
        }
        const upstream = execFileSync(process.env.OL_SPHERE_ORACLE!, [process.env.OL_LEVEL_PHD!, '0'], {
            input: near.map(c => c.join(' ')).join('\n') + '\n', maxBuffer: 64 * 1024 * 1024,
        }).toString().trim().split('\n').map(line => Number(line.split(' ').pop()));
        const tally = [0, 0, 0];
        let fastSteps = 0, strays = 0;
        near.forEach((c, i) => {
            const { n, answer } = fast(c);
            fastSteps += n;
            tally[answer]++;
            if (c[5] !== -16384) { expect(answer, `case ${i}: a yaw with no table`).toBe(2); strays++; return; }
            if (answer !== 2) expect(answer, `near case ${i}: ${c.join(' ')}`).toBe(upstream[i] ? 1 : 0);
        });
        const undecided = tally[2] - strays;
        console.log(`[openlara] spheres: fast path ${tally[1]} certain hits, ${tally[0]} certain misses, ` +
                    `${undecided} undecided of ${near.length - strays} poses near the blade; ` +
                    `${Math.round(fastSteps / near.length)} instructions a call`);
        expect(tally[1]).toBeGreaterThan(300);
        expect(tally[0]).toBeGreaterThan(300);
        expect(undecided / (near.length - strays), 'the bound should leave few poses undecided').toBeLessThan(0.05);

        // At the edge of touching: walk hits out along x a unit at a time to
        // where upstream's mask clears; the poses either side of that edge
        // are where the bound leaves the fast path undecided, and there the
        // exact chain must agree with upstream too.
        const seeds = near.filter((c, i) => upstream[i] && c[5] === -16384).slice(0, 24);
        const walk: number[][] = [];
        seeds.forEach(c => { for (let d = 0; d < 400; d++) walk.push([...c.slice(0, 8), c[8] + d, ...c.slice(9)]); });
        const walked = execFileSync(process.env.OL_SPHERE_ORACLE!, [process.env.OL_LEVEL_PHD!, '0'], {
            input: walk.map(c => c.join(' ')).join('\n') + '\n', maxBuffer: 64 * 1024 * 1024,
        }).toString().trim().split('\n').map(line => Number(line.split(' ').pop()));
        let edges = 0, edgeUndecided = 0;
        seeds.forEach((_, k) => {
            const run = walked.slice(k * 400, k * 400 + 400), edge = run.findIndex(m => m === 0);
            if (edge <= 0) return;
            for (let d = Math.max(0, edge - 3); d < Math.min(400, edge + 3); d++) {
                const c = walk[k * 400 + d], want = run[d];
                const { answer } = fast(c);
                edges++;
                if (answer !== 2) { expect(answer, `edge ${k} step ${d}: ${c.join(' ')}`).toBe(want ? 1 : 0); continue; }
                edgeUndecided++;
                if (edgeUndecided > 6) continue;         // the exact chain is 18 fields a call
                g.call('Trap_collideSpheres', [at(B) & 65535, at(B) >>> 16, ...g.args]);
                const mask = (g.u16(g.symbol('tcc__r0')) | g.u16(g.symbol('tcc__r1')) << 16) >>> 0;
                expect(mask, `edge ${k} step ${d}: the exact chain`).toBe(want);
            }
        });
        console.log(`[openlara] spheres: ${edges} poses at the edge of touching, ${edgeUndecided} left to the exact chain`);
        expect(edgeUndecided, 'poses at the edge should reach the exact chain').toBeGreaterThan(0);
    });

    it('swings when triggered, cuts 100 on each tick its spheres reach hers, and draws the splash rolls', () => {
        const g = movementCartridge(); g.restoreLevel();
        const n = g.u16(g.symbol('itemsCount'));
        const at = (i: number) => g.item + i * ITEM_SIZE, argsOf = (i: number) => [at(i) & 65535, at(i) >>> 16];
        const blades = [...Array(n).keys()].filter(i => g.memory.read(at(i) + ITEM.type) === 36);
        expect(blades.length).toBe(3);
        expect(g.u16(g.symbol('olBladeCount')), 'level init did not list the blades').toBe(3);
        const source = JSON.parse(readFileSync(join(DATA_DIR, 'FRAMES.json'), 'utf8')) as {
            animations: { anim: number; type: number; end: number; rate: number; ticks: number; boxes: number[][] }[];
        };
        // The box FX3 contact reads: the nearest source pose to the tick.
        const boxAt = (anim: number, tick: number, type = 36) => {
            const a = source.animations.find(x => x.anim === anim && x.type === type)!;
            const lower = Math.floor(tick / a.rate), choices = [lower];
            if ((lower + 1) * a.rate < a.end) choices.push(lower + 1);
            choices.sort((x, y) => Math.abs(x * a.rate - tick) - Math.abs(y * a.rate - tick) || x - y);
            return a.boxes[choices[0]];
        };
        const B = blades[1];
        const bx = g.s16(at(B) + ITEM.x), by = g.s16(at(B) + ITEM.y), bz = g.s16(at(B) + ITEM.z);
        expect(g.u16(at(B) + ITEM.angleY)).toBe(0xC000);
        // At yaw -90 her local x is dz and local z is -dx: -425 puts her in
        // the blade's thin z extent, 1000 along the sweep, so only the ends
        // of the swing reach her. Her feet 400 above the pivot.
        const lara = [bx + 425, by - 400, bz + 1000];
        lara.forEach((v, i) => g.w16(g.item + ITEM.x + i * 2, v));
        g.w8(g.item + ITEM.room, g.memory.read(at(B) + ITEM.room));
        g.w16(g.item + ITEM.health, 32000);
        const ci = g.symbol('cinfo'); g.w16(ci + 26, 100);
        const seed = () => (g.u16(g.symbol('olRandSeed')) | g.u16(g.symbol('olRandSeed') + 2) << 16) >>> 0;
        const lcg = (v: number) => (Math.imul(v, 0x41C64E6D) + 0x3039) >>> 0;
        const laraBox = boxAt(g.u16(g.item + ITEM.animIndex), g.memory.read(g.item + ITEM.frameIndex), 0);

        // A tick the current box reaches her is a cut when upstream's
        // collideSpheres says so for the same poses (tools/spherecheck.cpp).
        const spheresTouch = (poses: number[][]) => poses.length === 0 ? [] :
            execFileSync(process.env.OL_SPHERE_ORACLE!, [process.env.OL_LEVEL_PHD!, '0'], {
                input: poses.map(c => c.join(' ')).join('\n') + '\n', maxBuffer: 16 * 1024 * 1024,
            }).toString().trim().split('\n').map(line => Number(line.split(' ').pop()) !== 0);
        const run = (ticks: number) => {
            const states: number[] = [], hits: number[] = [], reach: number[] = [], poses: number[][] = [];
            const exact = g.u16(g.symbol('olSphExact'));
            for (let t = 0; t < ticks; t++) {
                g.call('ItemObj_update', [...argsOf(B), ...g.args]);
                const state = g.memory.read(at(B) + ITEM.state);
                if (states[states.length - 1] !== state) states.push(state);
                const box = boxAt(g.u16(at(B) + ITEM.animIndex), g.memory.read(at(B) + ITEM.frameIndex));
                const dy = lara[1] - by, dx = lara[0] - bx, dz = lara[2] - bz, px = dz, pz = -dx, r = 100;
                const boxReach = state === 2 && box[3] - laraBox[2] > dy && box[2] - laraBox[3] < dy &&
                    px >= box[0] - r && px <= box[1] + r && pz >= box[4] - r && pz <= box[5] + r;
                if (boxReach) {
                    reach.push(t);
                    poses.push([g.u16(at(B) + ITEM.animIndex), g.memory.read(at(B) + ITEM.frameIndex), bx, by, bz,
                        g.s16(at(B) + ITEM.angleY), g.u16(g.item + ITEM.animIndex), g.memory.read(g.item + ITEM.frameIndex),
                        lara[0], lara[1], lara[2], g.s16(g.item + ITEM.angleY), g.s16(g.symbol('olLaraAngleX'))]);
                }
                if (process.env.OL_BLADE_TRACE && t % 10 === 0) console.log(`[openlara] blade t${t} anim ${g.u16(at(B) + ITEM.animIndex)}.${g.memory.read(at(B) + ITEM.frameIndex)} state ${state} status ${g.memory.read(at(B) + ITEM.flags2) & 3} box ${box} px ${px} pz ${pz} dy ${dy} lara ${laraBox}`);
                const hp = g.s16(g.item + ITEM.health), before = seed();
                g.call('SwingBlade_collideLara', g.args);
                if (g.s16(g.item + ITEM.health) !== hp) {
                    hits.push(t);
                    expect(hp - g.s16(g.item + ITEM.health), 'a cut is 100').toBe(100);
                    expect(seed(), 'the splash makes four rand_logic draws').toBe(lcg(lcg(lcg(lcg(before)))));
                } else expect(seed(), 'no cut, no draw').toBe(before);
            }
            const touch = spheresTouch(poses);
            return { states, hits, want: reach.filter((_, i) => touch[i]), reach,
                     exact: g.u16(g.symbol('olSphExact')) - exact };
        };
        // Untriggered it hangs still and cannot cut.
        const idle = run(40);
        expect(idle.states).toEqual([0]);
        expect(idle.hits).toEqual([]);

        g.w8(at(B) + ITEM.flags, g.memory.read(at(B) + ITEM.flags) | 0x1F);
        g.w16(at(B) + ITEM.timer, 0);
        g.w8(at(B) + ITEM.flags2, (g.memory.read(at(B) + ITEM.flags2) & ~3) | 1);
        g.call('ItemObj_activate', argsOf(B));
        const swing = run(200);
        console.log(`[openlara] blades: states ${swing.states}, cuts at ${swing.hits.length} ticks ` +
                    `(${swing.hits.slice(0, 8)}...) of 200; the box reached her on ${swing.reach.length}, ` +
                    `the exact chain decided ${swing.exact}`);
        // TR's data goes from STATIC straight into the swing (229, frame 30);
        // 228 is the wind-down, back to rest.
        expect(swing.states[0], 'the trigger did not start the swing').toBe(2);
        expect(swing.hits.length, 'the swing never reached her').toBeGreaterThan(0);
        expect(swing.hits.length, 'it cut on every tick, not at the ends of the swing').toBeLessThan(150);
        expect(swing.hits, 'cuts on ticks upstream\'s spheres do not reach her').toEqual(swing.want);
        expect(swing.hits.length, 'the spheres should reject some of the box\'s air').toBeLessThan(swing.reach.length);

        // Off: back to STATIC at the end of the swing, and no more cuts.
        g.w8(at(B) + ITEM.flags, g.memory.read(at(B) + ITEM.flags) & ~0x1F);
        const stop = run(160);
        expect(stop.states[stop.states.length - 1], 'it never came back to rest').toBe(0);
        expect(g.u16(at(B) + ITEM.animIndex), 'not resting on STATIC').toBe(227);
        console.log(`[openlara] blades: switched off, ${stop.hits.length} cuts before it came to rest`);
        expect(stop.hits, 'cut when its spheres did not reach her').toEqual(stop.want);
    });
});

describe.skipIf(!FX3 || LEVEL !== 'LEVEL1')('OpenLara FX3: inventory', () => {
    // src/fixed/inventory.h: Select opens the ring (the game does not move
    // while it is open), Left/Right spin it, Action shows an item and, for a
    // medipack or a gun, closes the ring and uses it (Lara::useItem).
    // Upstream's inventory.init() hands out all four guns, ten small and
    // five large medipacks.
    const PAD_SELECT = 0x2000;
    const SLOT = { COMPASS: 10, PISTOLS: 11, SHOTGUN: 13, MAGNUMS: 15, UZIS: 17, MEDIKIT_BIG: 19,
                   MEDIKIT_SMALL: 20, PASSPORT: 21 };
    const STATE = { NONE: 0, READY: 4, SHOW: 9 };
    const boot = () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        emulator.executeFrames(60);
        const memory = emulator.getMemory();
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        const s16 = (a: number) => (u16(a) << 16) >> 16;
        const w16 = (a: number, v: number) => { memory.write(a, v & 0xFF); memory.write(a + 1, (v >> 8) & 0xFF); };
        const pad = emulator.getController() as unknown as { buttonState: number };
        const count = (slot: number) => s16(sym('olInvCounts') + slot * 2);
        const state = () => u16(sym('olInvState'));
        const press = (button: number, fields = 4) => {
            pad.buttonState = button; emulator.executeFrames(fields);
            pad.buttonState = 0; emulator.executeFrames(4);
        };
        const until = (what: () => boolean, fields = 240) => {
            for (let i = 0; i < fields && !what(); i++) emulator.executeFrames(1);
            return what();
        };
        // Closed and done: the tick that closes the ring also puts the
        // level's sounds back in APU RAM (sound_ring.asm), about ten fields
        // of IPL handshakes before what the ring chose is used.
        const closed = (fields = 400) => {
            if (!until(() => state() === STATE.NONE, fields)) return false;
            const t = u16(sym('ol_ticks'));
            return until(() => u16(sym('ol_ticks')) > t + 1, 60);
        };
        return { emulator, memory, u16, s16, w16, pad, count, state, press, until, closed };
    };

    it('opens on Select, freezes the game, spins to a medipack and heals her with it', () => {
        const g = boot();
        const it = sym('items');
        expect(g.count(SLOT.MEDIKIT_SMALL), 'upstream starts her with ten small medipacks').toBe(10);
        expect(g.count(SLOT.MEDIKIT_BIG), 'and five large').toBe(5);
        expect([g.count(SLOT.COMPASS), g.count(SLOT.PISTOLS), g.count(SLOT.SHOTGUN), g.count(SLOT.MAGNUMS),
                g.count(SLOT.UZIS), g.count(SLOT.PASSPORT)]).toEqual([1, 1, 1, 1, 1, 1]);
        g.w16(it + ITEM.health, 400);
        g.press(PAD_SELECT);
        expect(g.until(() => g.state() === STATE.READY), 'the ring never opened').toBe(true);
        // Let a frame or two of the ring reach the screen, and keep one to
        // look at next to the build.
        g.emulator.executeFrames(90);
        {
            const fb = g.emulator.getPPU().getFramebuffer();
            const { stride, width, height } = g.emulator.getPPU().getFrameGeometry();
            writeFileSync(join(dirname(ROM), 'inventory.png'), png(width, height, (x, y) => fb[y * stride + x]));
        }
        // The main page: compass, the four guns, large, small -- upstream's order.
        const list = [0, 1, 2, 3, 4, 5, 6].map(i => g.memory.read(sym('olInvItemsList') + i));
        expect(g.u16(sym('olInvItemsCount'))).toBe(7);
        expect(list).toEqual([SLOT.COMPASS, SLOT.PISTOLS, SLOT.SHOTGUN, SLOT.MAGNUMS, SLOT.UZIS,
                              SLOT.MEDIKIT_BIG, SLOT.MEDIKIT_SMALL]);
        // Frozen: she does not move and the world's clock does not either,
        // while ticks go on.
        const x0 = g.s16(it + ITEM.x), ticks0 = g.u16(sym('ol_ticks'));
        g.pad.buttonState = PAD_UP; g.emulator.executeFrames(2); g.pad.buttonState = 0;
        g.until(() => g.state() === STATE.READY);
        g.emulator.executeFrames(60);
        expect(g.s16(it + ITEM.x), 'she moved with the ring open').toBe(x0);
        expect(g.u16(sym('ol_ticks')) - ticks0, 'ticks stopped').toBeGreaterThan(20);
        const snap = (name: string) => {
            if (!process.env.OL_INV_SNAPS) return;
            const fb = g.emulator.getPPU().getFramebuffer();
            const { stride, width, height } = g.emulator.getPPU().getFrameGeometry();
            writeFileSync(join(dirname(ROM), `inv_${name}.png`), png(width, height, (x, y) => fb[y * stride + x]));
        };
        // Right from the compass wraps to the last item: the small medipack.
        g.press(PAD_RIGHT, 2);
        g.emulator.executeFrames(6); snap('spin');
        expect(g.until(() => g.state() === STATE.READY && g.u16(sym('olInvItemIndex')) === 6), 'the ring did not spin').toBe(true);
        g.emulator.executeFrames(60); snap('ready3');
        // Its text (inventory_text.c, the GSU's TextRender): the title, the
        // page's down arrows (LEVEL1 has no keys, so no up arrows), the count
        // in the small digits and the name -- 9 + 2 + 2 + 13 glyphs -- and
        // drawSlot's bar, her health under the title (renderBar at
        // ((FRAME_WIDTH - 104) / 2, 24): OAM 20 + 56, 40 + 24).
        // (Inventory_text zeroes the count at its entry: sample until it stands)
        g.until(() => g.u16(sym('olInvText')) === 26, 60);
        expect(g.u16(sym('olInvText')), 'INVENTORY, two arrows, 10, Small Medi Pack').toBe(26);
        const oam = sym('olHudOam');
        expect([g.memory.read(oam), g.memory.read(oam + 1)], 'the medipack health bar').toEqual([76, 64]);
        g.press(PAD_B, 2);
        g.until(() => g.state() === STATE.SHOW, 100); g.emulator.executeFrames(30); snap('show');
        expect(g.closed(), 'using it never closed the ring').toBe(true);
        console.log(`[openlara] inventory: health ${g.s16(it + ITEM.health)}, small ${g.count(SLOT.MEDIKIT_SMALL)}`);
        expect(g.s16(it + ITEM.health), 'the small medipack heals half her health').toBe(900);
        expect(g.count(SLOT.MEDIKIT_SMALL)).toBe(9);
        // The HUD's own bar again, for the forty ticks after the heal.
        expect([g.memory.read(sym('olHudOam')), g.memory.read(sym('olHudOam') + 1)]).toEqual([128, 44]);
        // Unfrozen again.
        const xa = g.s16(it + ITEM.x), za = g.s16(it + ITEM.z);
        g.pad.buttonState = PAD_UP; g.emulator.executeFrames(60); g.pad.buttonState = 0;
        expect(g.s16(it + ITEM.x) !== xa || g.s16(it + ITEM.z) !== za, 'she did not move after it closed').toBe(true);
    });

    // inventory_fx3.asm: the ring's matrix steps, natively, against the C
    // they replace (inventory_draw.c rot/trans, the C_MOTION oracle's):
    // every row r, x = (int)M[r + a], y = (int)M[r + b], M[r + a] =
    // (x c - y s) >> 14, M[r + b] = (y c + x s) >> 14; and M[r + 3] +=
    // M[r] x + M[r + 1] y + M[r + 2] z -- int32, wrapping as the C does.
    it('steps the ring\'s matrices natively, bit for bit as the C', () => {
        const g = movementCartridge();
        const at = g.symbol('olInvM');
        const s16 = (v: number) => (v << 16) >> 16;
        const read = () => Array.from({ length: 12 }, (_, i) =>
            (g.memory.read(at + i * 4) | g.memory.read(at + i * 4 + 1) << 8 |
             g.memory.read(at + i * 4 + 2) << 16 | g.memory.read(at + i * 4 + 3) << 24) | 0);
        const write = (m: number[]) => m.forEach((v, i) => { for (let k = 0; k < 4; k++) g.w8(at + i * 4 + k, (v >>> (8 * k)) & 255); });
        let seed = 12345;
        const rnd = () => { seed = (Math.imul(seed, 1103515245) + 12345) >>> 0; return seed >>> 8; };
        // olMulS16's domain (and so the C's, which multiplies with it too):
        // its second operand's high byte, rounded, must fit a signed byte --
        // below 32,640. The ring multiplies by sines and radii.
        const word = () => s16(rnd()) % 32640;
        let cases = 0;
        for (let t = 0; t < 300; t++) {
            const m = Array.from({ length: 12 }, () => (rnd() << 8 ^ rnd()) | 0);
            const [a, b] = [[0, 2], [2, 1], [1, 0]][t % 3];
            const angle = rnd() & 0xFFFF, sn = Math.round(Math.sin(angle * Math.PI / 32768) * 16384), cs = Math.round(Math.cos(angle * Math.PI / 32768) * 16384);
            const [sv, cv] = t % 5 === 0 ? [word(), word()] : [sn, cs];
            write(m);
            g.call('Inv_rot', [a, b, sv & 0xFFFF, cv & 0xFFFF]);
            const want = m.slice();
            for (let r = 0; r < 12; r += 4) {
                const x = s16(m[r + a]), y = s16(m[r + b]);
                want[r + a] = ((Math.imul(x, cv) - Math.imul(y, sv)) | 0) >> 14;
                want[r + b] = ((Math.imul(y, cv) + Math.imul(x, sv)) | 0) >> 14;
            }
            expect(read(), `Inv_rot case ${t}`).toEqual(want);
            const [x, y, z] = [word(), word(), word()];
            write(m);
            g.call('Inv_trans', [x & 0xFFFF, y & 0xFFFF, z & 0xFFFF]);
            const moved = m.slice();
            for (let r = 0; r < 12; r += 4)
                moved[r + 3] = (m[r + 3] + Math.imul(s16(m[r]), x) + Math.imul(s16(m[r + 1]), y) + Math.imul(s16(m[r + 2]), z)) | 0;
            expect(read(), `Inv_trans case ${t}`).toEqual(moved);
            cases += 2;
        }
        console.log(`[openlara] ring matrices: ${cases} native cases exact`);
    });

    // The ring's sounds (sound_ring.asm, packer sound_SNES.h): in APU RAM
    // only while it is open, over the level's last samples, under the gun
    // region; SOUND.BIN's own bytes and directory entries back as it closes.
    it('plays upstream\'s ring sounds while it is open, and has the level\'s back when it closes', () => {
        const g = boot();
        const plan = JSON.parse(readFileSync(join(DATA_DIR, 'SOUND.json'), 'utf8'));
        const gun = readFileSync(join(DATA_DIR, 'GUNSND.BIN')), image = readFileSync(join(DATA_DIR, 'SOUND.BIN'));
        const w = (o: number) => gun[o] | gun[o + 1] << 8;
        const [first, n, addr, bytes, dirAt, samplesAt] = [72, 74, 76, 78, 80, 82].map(w);
        expect([first, addr, bytes], 'GUNSND.BIN\'s ring against SOUND.json').toEqual([plan.ring.first, plan.ring.addr, plan.ring.bytes]);
        expect(n).toBe(Object.keys(plan.ring.ids).length);
        expect(addr + bytes, 'the ring ends where the gun region starts').toBe(plan.gun.addr);
        const { ram, dsp } = apuOf(g.emulator);
        const same = (at: number, from: Buffer, offset: number, k: number) => {
            for (let i = 0; i < k; i++) if (ram[at + i] !== from[offset + i]) return false;
            return true;
        };
        const levelHas = () => same(addr, image, addr - 0x400, bytes) && same(0x400 + first * 4, image, first * 4, n * 4);
        const gunRegion = Array.from(ram.subarray(plan.gun.addr, plan.gun.addr + plan.gun.size));
        expect(levelHas(), 'closed, APU RAM is the level\'s image').toBe(true);
        const heard = new Set<number>();
        const listen = (fields: number, until?: () => boolean) => {
            for (let i = 0; i < fields && !(until && until()); i++) {
                g.emulator.executeFrames(1);
                for (const v of dsp().voices) if (v.keyedOn && v.source >= first && v.source < first + n) heard.add(v.source);
            }
        };
        const source = (id: number) => plan.ring.ids[String(id)].source;
        g.pad.buttonState = 0x2000; listen(4); g.pad.buttonState = 0;
        listen(240, () => g.state() === STATE.READY);
        expect(g.state(), 'the ring never opened').toBe(STATE.READY);
        expect(g.u16(sym('ol_sndReady')), 'the driver did not come back').toBe(1);
        expect(same(addr, gun, samplesAt, bytes), 'the ring\'s samples').toBe(true);
        expect(same(0x400 + first * 4, gun, dirAt, n * 4), 'its directory entries').toBe(true);
        expect(same(0x600 + first * 2, gun, dirAt + n * 4, n * 2), 'its pitch words').toBe(true);
        expect(Array.from(ram.subarray(plan.gun.addr, plan.gun.addr + plan.gun.size)), 'the gun region').toEqual(gunRegion);
        g.pad.buttonState = PAD_LEFT; listen(2); g.pad.buttonState = 0;
        listen(120, () => g.state() === STATE.READY && g.u16(sym('olInvItemIndex')) === 1);
        listen(20);
        g.pad.buttonState = 0x2000; listen(4); g.pad.buttonState = 0;
        listen(300, () => g.state() === STATE.NONE);
        expect(g.state(), 'Select never closed the ring').toBe(STATE.NONE);
        // The state reads NONE the moment the update sets it; the upload
        // back follows in the same tick, about ten fields of IPL handshakes.
        const restoring = g.u16(sym('ol_ticks'));
        listen(60, () => g.u16(sym('ol_ticks')) > restoring + 1);
        console.log(`[openlara] ring sounds: ${n} sources from ${first}, ${bytes} B at $${addr.toString(16)}; ` +
            `heard ${[...heard].sort((a, b) => a - b)}`);
        expect([...heard].sort((a, b) => a - b), 'SND_INV_SHOW, SPIN and HIDE')
            .toEqual([source(111), source(108), source(112)].sort((a, b) => a - b));
        expect(g.u16(sym('ol_sndReady')), 'the driver did not come back').toBe(1);
        if (!levelHas()) {
            let at = -1;
            for (let i = 0; i < bytes && at < 0; i++) if (ram[addr + i] !== image[addr - 0x400 + i]) at = i;
            console.log(`[openlara] ring restore: samples ${same(addr, image, addr - 0x400, bytes)} (first difference +${at}: ` +
                `${ram[addr + at]} vs ${image[addr - 0x400 + at]}, ring ${gun[samplesAt + at]}), entries ${same(0x400 + first * 4, image, first * 4, n * 4)} ` +
                `${Array.from(ram.subarray(0x400 + first * 4, 0x400 + first * 4 + 8))}`);
        }
        expect(levelHas(), 'closed again, APU RAM is the level\'s image').toBe(true);
        expect(Array.from(ram.subarray(plan.gun.addr, plan.gun.addr + plan.gun.size)), 'the gun region').toEqual(gunRegion);
        // and the level's sounds play again: her footsteps, sources 0 and 1
        let steps = false;
        g.pad.buttonState = PAD_UP;
        for (let i = 0; i < 120 && !steps; i++) {
            g.emulator.executeFrames(1);
            for (const v of dsp().voices) if (v.keyedOn && v.source <= 1) steps = true;
        }
        g.pad.buttonState = 0;
        expect(steps, 'no footsteps after the ring closed').toBe(true);
    });

    // upstream's itemVisMask (updateVisMask): the passport's pages as they
    // turn, in every ring record's last word (inventory_draw.c), which the
    // GSU's DrawModel shifts a node at a time (WeaponNodeFx3).
    it('hands the GSU the passport\'s page mask as upstream computes it', () => {
        const g = boot();
        const ring = fx3Passport(g.emulator);
        const PM = (...n: number[]) => n.reduce((m, k) => m | 1 << k, 0);
        const upstream = (f: number) => PM(0, 1, 4) | (f <= 14 ? PM(2, 6) : f < 19 ? PM(2, 3, 6) : f === 19 ? PM(3, 6) :
                                                       f < 24 ? PM(3, 5, 6) : f < 29 ? PM(3, 5) : 0);
        const records = () => {
            const n = g.memory.readCartridgeRAM(0xDE40) | (g.memory.readCartridgeRAM(0xDE41) & 0x7F) << 8;
            return Array.from({ length: n }, (_, i) => g.memory.readCartridgeRAM(0xDE40 + 2 + i * 32 + 30) |
                g.memory.readCartridgeRAM(0xDE40 + 2 + i * 32 + 31) << 8);
        };
        expect(g.u16(sym('olInvVisMask')), 'the ring is not open yet').toBe(0);
        ring.open();                                // the options page's passport, at Save Game
        g.emulator.executeFrames(30);
        const frame = () => g.u16(sym('olInvFrameIndex'));
        expect(frame(), 'Save Game is frame 14 + 5').toBe(19);
        expect(g.u16(sym('olInvVisMask'))).toBe(upstream(19));
        expect(records().every(m => m === upstream(19)), `the ring's records ${records()}`).toBe(true);
        // Sampled where a tick starts: a field can end inside the tick
        // between the frame's step and the mask's (Settings_visMask).
        const seen = new Set<number>(), wrong: string[] = [];
        const cpu = g.emulator.getCPU(), step = cpu.step;
        cpu.step = function (this: typeof cpu) {
            if ((this.PB << 16 | this.PC) === sym('gameUpdate')) {
                seen.add(frame());
                if (g.u16(sym('olInvVisMask')) !== upstream(frame())) wrong.push(`${frame()}:${g.u16(sym('olInvVisMask'))}`);
            }
            return step.call(this);
        };
        try {
            g.pad.buttonState = PAD_LEFT; g.emulator.executeFrames(4); g.pad.buttonState = 0;
            g.emulator.executeFrames(60);
        } finally { cpu.step = step; }
        expect(wrong, 'frame:mask against upstream').toEqual([]);
        expect(frame(), 'Load Game is frame 14').toBe(14);
        expect(g.until(() => records().every(m => m === upstream(14)), 30), `the ring's records ${records()}`).toBe(true);
        g.pad.buttonState = 0x2000; g.emulator.executeFrames(4); g.pad.buttonState = 0;   // put it back
        expect(g.until(() => g.state() === STATE.READY), 'the passport did not go back').toBe(true);
        expect(g.u16(sym('olInvVisMask')), 'deselected, every node').toBe(0xFFFF);
        console.log(`[openlara] passport mask: Save Game ${upstream(19).toString(2)}, Load Game ${upstream(14).toString(2)}, ` +
            `frames ${[...seen].sort((a, b) => a - b)} on the way`);
    });

    it('draws the shotgun when it is chosen, and the GSU is handed her meshes', () => {
        const g = boot();
        g.press(PAD_SELECT);
        expect(g.until(() => g.state() === STATE.READY)).toBe(true);
        for (const index of [1, 2]) {
            g.press(PAD_LEFT, 2);
            expect(g.until(() => g.state() === STATE.READY && g.u16(sym('olInvItemIndex')) === index)).toBe(true);
        }
        expect(g.memory.read(sym('olInvItemsList') + 2)).toBe(SLOT.SHOTGUN);
        g.press(PAD_B, 2);
        expect(g.until(() => g.state() === STATE.NONE, 400)).toBe(true);
        expect(g.until(() => g.u16(sym('olLaraWeaponState')) === 4, 120), 'the shotgun never came out').toBe(true);
        const kit = Array.from({ length: 5 }, (_, i) => g.u16(sym('olWeaponKit') + i * 2));
        expect(kit, 'the shotgun in her hands').toEqual([3, 3, 0, 1, 0]);
        // ItemsExchange's copy, as the GSU reads it: her torso without the
        // shotgun on its back, the shotgun's hands
        expect(g.until(() => g.memory.readCartridgeRAM(0xF648 + 7) === 17, 60), 'GS_KIT never followed').toBe(true);
        expect([1, 4, 7, 10, 13, 14].map(n => g.memory.readCartridgeRAM(0xF648 + n))).toEqual([5, 6, 17, 19, 20, 21]);
        g.emulator.executeFrames(30);
        const fb = g.emulator.getPPU().getFramebuffer();
        const { stride, width, height } = g.emulator.getPPU().getFrameGeometry();
        writeFileSync(join(dirname(ROM), 'shotgun.png'), png(width, height, (x, y) => fb[y * stride + x]));

        // Its sounds (main.asm olSndGun): the APU's gun region, its two
        // directory entries and pitch words, as GUNSND.BIN has the shotgun's
        const plan = JSON.parse(readFileSync(join(DATA_DIR, 'SOUND.json'), 'utf8'));
        const gun = readFileSync(join(DATA_DIR, 'GUNSND.BIN'));
        const w = (o: number) => gun[o] | gun[o + 1] << 8;
        const rec = 8 + 3 * 16, bytes = w(rec + 12), from = w(rec + 14);
        const { ram, dsp } = apuOf(g.emulator);
        const apu = (at: number, n: number) => Array.from({ length: n }, (_, i) => ram[at + i]);
        expect(g.u16(sym('ol_sndReady')), 'the driver did not come back').toBe(1);
        expect(g.u16(sym('olSndGunLoaded'))).toBe(3);
        expect(apu(0x400 + plan.gun.shotSource * 4, 8), 'the region\'s directory entries')
            .toEqual(Array.from(gun.subarray(rec, rec + 8)));
        expect(apu(0x600 + plan.gun.shotSource * 2, 4), 'their pitch words')
            .toEqual(Array.from(gun.subarray(rec + 8, rec + 12)));
        expect(apu(plan.gun.addr, bytes).every((v, i) => v === gun[from + i]), 'the shotgun\'s samples').toBe(true);
        // and the blast plays from them, the reload after it
        let shot = false, reload = false;
        g.pad.buttonState = PAD_B;
        for (let i = 0; i < 120 && !(shot && reload); i++) {
            g.emulator.executeFrames(1);
            for (const v of dsp().voices) {
                if (v.keyedOn && v.source === plan.gun.shotSource) shot = true;
                if (v.keyedOn && v.source === plan.gun.reloadSource) reload = true;
            }
        }
        g.pad.buttonState = 0;
        expect([shot, reload], 'SND_SHOTGUN_SHOT and SND_SHOTGUN_RELOAD from the gun region').toEqual([true, true]);
    });

    it('closes on Select without using anything, and draws the pistols when they are chosen', () => {
        const g = boot();
        g.press(PAD_SELECT);
        expect(g.until(() => g.state() === STATE.READY)).toBe(true);
        g.press(PAD_SELECT);
        expect(g.until(() => g.state() === STATE.NONE), 'Select did not close it').toBe(true);
        expect(g.count(SLOT.MEDIKIT_SMALL), 'closing used something').toBe(10);
        expect(g.u16(sym('olLaraWeaponState')), 'closing drew a weapon').toBe(0);
        g.emulator.executeFrames(20);
        g.press(PAD_SELECT);
        expect(g.until(() => g.state() === STATE.READY)).toBe(true);
        g.press(PAD_LEFT, 2);
        expect(g.until(() => g.state() === STATE.READY && g.u16(sym('olInvItemIndex')) === 1)).toBe(true);
        g.press(PAD_B, 2);
        expect(g.until(() => g.state() === STATE.NONE, 400)).toBe(true);
        expect(g.until(() => g.u16(sym('olLaraWeaponState')) === 4, 120), 'the pistols never came out').toBe(true);
    });
});

// src/save.c's payload, in its order, for n entities. The runner holds the
// battery RAM to it both ways: what a save wrote is the game as it stood,
// and what a load leaves is what was saved.
const saveFields = (n: number): [string, number][] => [
    ['items', n * ITEM_SIZE], ['olEnemyNext', n], ['olEnemyTurn', n], ['olFirstActive', 2],
    ['olLaraTurn', 2], ['olLaraVSpeedHack', 2], ['olLaraMoveAngle', 2], ['olLaraWaterState', 2],
    ['olLaraAngleX', 2], ['olLaraAngleZ', 2], ['olLaraSwimTimer', 2], ['olLaraOxygen', 2],
    ['olLaraWeaponState', 2], ['olLaraLooking', 2],
    ['olLaraHitQuadrant', 1], ['olLaraHitTimer', 1], ['olLaraHitFrame', 1],
    ['olWeaponArms', 24], ['olLaraLook', 8],
    ['olWeaponTarget', 2], ['olWeaponAim', 2], ['olWeaponAimX', 2], ['olWeaponAimY', 2], ['olWeaponKit', 10],
    ['olInvCounts', 52], ['olInventory', 11], ['olPickups', 2], ['olDartsLive', 2],
    ['olLevelEnd', 2], ['olSecrets', 2], ['olCamOnce', 32], ['olCamLastIndex', 2], ['olFlipped', 2],
    ['olRandSeed', 4], ['olDrawSeed', 2],
];
const SAVE_AT = 0x1000, SAVE_DATA = 0x1010, SAVE_END = 0x3200, BOOT_AT = 0x3100;

// The game's side of the payload, field by field, as the battery RAM lays it out.
function saveImage(memory: ReturnType<SNESEmulator['getMemory']>, n: number): number[] {
    const out: number[] = [];
    for (const [name, size] of saveFields(n))
        for (let i = 0; i < size; i++) out.push(memory.read(sym(name) + i));
    return out;
}

// The first payload difference, named, or undefined.
function saveDiff(n: number, game: number[], saved: number[]): string | undefined {
    let at = 0;
    for (const [name, size] of saveFields(n)) {
        for (let i = 0; i < size; i++, at++)
            if (game[at] !== saved[at]) return `${name}+${i}: game ${game[at]}, saved ${saved[at]}`;
    }
    return game.length === saved.length ? undefined : `length ${game.length} against ${saved.length}`;
}

// The FX3 ring as a player drives it to the passport: Select, Down to the
// options page, Action on its first item; it opens at Save Game.
function fx3Passport(emulator: SNESEmulator) {
    const memory = emulator.getMemory();
    const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
    const pad = emulator.getController() as unknown as { buttonState: number };
    const press = (button: number) => {
        pad.buttonState = button; emulator.executeFrames(4);
        pad.buttonState = 0; emulator.executeFrames(4);
    };
    const until = (what: () => boolean, fields = 400) => {
        for (let i = 0; i < fields && !what(); i++) emulator.executeFrames(1);
        return what();
    };
    const state = () => u16(sym('olInvState'));
    const shown = () => state() === 9 && u16(sym('olInvFrameIndex')) === u16(sym('olInvFrameTarget'));
    const open = () => {
        press(0x2000);
        expect(until(() => state() === 4), 'the ring never opened').toBe(true);
        press(PAD_DOWN);
        expect(until(() => state() === 4 && u16(sym('olInvPage')) === 6), 'no options page').toBe(true);
        expect(memory.read(sym('olInvItemsList') + u16(sym('olInvItemIndex'))), 'not the passport').toBe(21);
        press(PAD_B);
        expect(until(shown), 'the passport never opened').toBe(true);
        expect(u16(sym('olInvPassportPage')), 'the options page\'s passport is not at Save Game').toBe(1);
    };
    // Save Game: Action, and the battery RAM against the game while the
    // ring, closing, still holds it still.
    const save = (writing?: (on: boolean) => void) => {
        open();
        const valid = u16(sym('olSaveValid'));
        writing?.(true);
        pad.buttonState = PAD_B;
        const wrote = until(() => u16(sym('olSaveValid')) === 1 && (valid === 0 || state() !== 9), 30);
        pad.buttonState = 0;
        writing?.(false);
        if (!wrote) {
            const words = Array.from({ length: 8 }, (_, i) =>
                memory.readCartridgeRAM(SAVE_AT + i * 2) | memory.readCartridgeRAM(SAVE_AT + i * 2 + 1) << 8);
            let s1 = 0, s2 = 0;
            for (let i = 0; i < words[5] >> 1; i++) {
                s1 = (s1 + (memory.readCartridgeRAM(SAVE_DATA + i * 2) | memory.readCartridgeRAM(SAVE_DATA + i * 2 + 1) << 8)) & 65535;
                s2 = (s2 + s1) & 65535;
            }
            const sum = (((s2 << 8) | (s2 >> 8)) & 65535) ^ s1 ^ 0x5A5A;
            const expected = saveFields(u16(sym('itemsCount'))).reduce((t, [, size]) => t + size, 0);
            throw new Error(`Save Game did not leave a valid save: header ${words.map(w => w.toString(16))}, ` +
                `sum ${sum.toString(16)}, rom ${u16(0x80FFDE).toString(16)}, length ${expected}, ` +
                `state ${state()} options ${u16(sym('olInvOptionsCount'))} page ${u16(sym('olInvPassportPage'))}`);
        }
        expect(state(), 'the ring closed before the save could be read').not.toBe(0);
        const n = u16(sym('itemsCount'));
        const saved = Array.from({ length: saveFields(n).reduce((a, [, size]) => a + size, 0) },
            (_, i) => memory.readCartridgeRAM(SAVE_DATA + i));
        const diff = saveDiff(n, saveImage(memory, n), saved);
        expect(diff, `the save is not the game: ${diff}`).toBeUndefined();
        expect(until(() => state() === 0), 'the ring did not close after saving').toBe(true);
        return saved;
    };
    // Load Game's Current Position: through the boot, and the game as the
    // load tick leaves it -- at the next gameUpdate, before anything moves.
    // From the title page the passport is the ring's first item, and its
    // only page is Load Game.
    const load = (fromTitle = false) => {
        if (fromTitle) {
            expect(state() === 4 && u16(sym('olInvPage')) === 0 && memory.read(sym('olInvItemsList') +
                   u16(sym('olInvItemIndex'))) === 21, 'not the title\'s passport').toBe(true);
            press(PAD_B);
        } else {
            open();
            press(PAD_LEFT);
        }
        expect(until(() => shown() && u16(sym('olInvPassportPage')) === 0), 'no Load Game page').toBe(true);
        const levels = u16(sym('olLevelCount'));
        expect([u16(sym('olInvOptionsCount')), u16(sym('olInvOptionIndex')),
                memory.read(sym('olInvOptStr') + levels + 1)], 'Load Game: the levels, a space, Current Position (chosen)')
            .toEqual([levels + 2, levels + 1, 5]);
        const cpu = emulator.getCPU(), step = cpu.step, entry = sym('gameUpdate');
        let empty = false, loaded: number[] | undefined;
        cpu.step = function (this: typeof cpu) {
            if (!loaded && (this.PB << 16 | this.PC) === entry) {
                const n = u16(sym('itemsCount'));
                if (n === 0) empty = true;
                else if (empty) loaded = saveImage(memory, n);
            }
            return step.call(this);
        };
        try {
            pad.buttonState = PAD_B;
            const back = until(() => loaded !== undefined, 900);
            if (!back) {
                const pc = cpu.PB << 16 | cpu.PC;
                let near = '', best = -1;
                for (const [name, a] of SYMS) if (a <= pc && a > best && a >>> 16 === pc >>> 16) { best = a; near = name; }
                throw new Error(`Current Position never came back through the boot: PC ${pc.toString(16)} (${near}+` +
                    `${(pc - best).toString(16)}), level ${u16(sym('olLevelIndex'))} kind ${u16(sym('olBootKind'))}, ` +
                    `items ${u16(sym('itemsCount'))}, ticks ${u16(sym('ol_ticks'))}, empty seen ${empty}, ` +
                    `request ${memory.readCartridgeRAM(0x3100).toString(16)}, tables ready ${memory.read(sym('olHostTablesReady'))}, ` +
                    `olLevel ${Array.from({ length: 6 }, (_, i) => u16(sym('olLevel') + i * 2).toString(16)).join(',')}, ` +
                    `directory ${u16(sym('olHostDirectory')).toString(16)} ${u16(sym('olHostDirectory') + 16)}, ` +
                    `saveLoad ${u16(sym('olSaveLoad'))} valid ${u16(sym('olSaveValid'))}, ` +
                    `GS_LEVEL_READY ${memory.readCartridgeRAM(0xF314)}`);
            }
        } finally {
            cpu.step = step;
            pad.buttonState = 0;
        }
        return loaded!;
    };
    return { memory, u16, pad, press, until, state, open, save, load };
}

describe.skipIf(!FX3 || LEVEL !== 'LEVEL1')('OpenLara FX3: saved games', () => {
    // src/fixed/game.h's gameSave and gameLoad, the GBA's osSaveGame into its
    // cartridge SRAM: here the FX3 board's battery (cartridge type $18) and
    // the game pak RAM it keeps, at $70:1000 (save.c). The save is made with a
    // wolf mid-chase and the pistols out; the cartridge is switched off and
    // on again with only the battery RAM carried over; the passport's Load
    // Game then offers Current Position, and choosing it puts every saved
    // byte back by the end of the load tick, and the chase goes on.
    it('saves a chase into battery RAM, keeps it through a power cycle and loads it back exactly', () => {
        const rom = new Uint8Array(readFileSync(ROM));
        const emulator = new SNESEmulator();
        emulator.loadROM(rom);
        expect(emulator.getROMInfo().sramSize, 'no battery: FX3 with one is cartridge type $18').toBe(0x20000);
        const memory = emulator.getMemory();
        const ram = memory as unknown as { writeCartridgeRAM(o: number, v: number): void; sram: Uint8Array };
        const writeRam = ram.writeCartridgeRAM;
        // From power-on, nothing but the save may write its window: not a
        // frame, not the ring's background, not the GSU's scratch. The boot
        // spending the request this harness planted (SelectLevel's zero over
        // "OL") is the boot's own.
        let saving = false, saveWrites = 0;
        const stray = new Set<number>();
        ram.writeCartridgeRAM = function (offset: number, value: number) {
            if ((offset === BOOT_AT || offset === BOOT_AT + 1) && value === 0) return writeRam.call(this, offset, value);
            if (offset >= SAVE_AT && offset < SAVE_END) {
                if (saving) saveWrites++; else stray.add(offset);
            }
            return writeRam.call(this, offset, value);
        };
        let battery: Uint8Array, saved: number[], wolf = 0, n = 0;
        const a = fx3Passport(emulator);
        try {
            awaitLevel(emulator);
            emulator.executeFrames(60);
            expect(a.u16(sym('olSaveValid')), 'a save in RAM no game wrote').toBe(0);
            const w16 = (at: number, v: number) => { memory.write(at, v & 255); memory.write(at + 1, v >> 8 & 255); };
            // The profile's room-14 wolf, woken at a gameUpdate boundary.
            n = a.u16(sym('itemsCount'));
            for (let i = 1; i < n && !wolf; i++) if (memory.read(sym('items') + i * ITEM_SIZE + ITEM.type) === 7) wolf = i;
            expect(wolf).toBeGreaterThan(0);
            const cpu = emulator.getCPU(), step = cpu.step;
            let placed = false;
            cpu.step = function (this: typeof cpu) {
                if (!placed && (this.PB << 16 | this.PC) === sym('gameUpdate')) {
                    placed = true;
                    const lara = sym('items'), w = lara + wolf * ITEM_SIZE;
                    memory.write(w + ITEM.nextActive, a.u16(sym('olFirstActive')) & 255);
                    w16(sym('olFirstActive'), wolf);
                    memory.write(lara + ITEM.room, 14);
                    w16(lara + ITEM.x, 22016); w16(lara + ITEM.y, 6656); w16(lara + ITEM.z, -17456);
                    w16(lara + ITEM.angleY, 0x8000);
                    memory.write(w + ITEM.room, 14); memory.write(w + ITEM.flags, 0x40);
                    memory.write(w + ITEM.flags2, 1); memory.write(w + ITEM.mood, 0);
                    w16(w + ITEM.x, 22016); w16(w + ITEM.y, 6656); w16(w + ITEM.z, -13824); w16(w + ITEM.health, 6);
                    memory.write(sym('olItemDirty') + wolf, 1);
                    [22016, 6144, -15920, 14, 0x8000, 0x8000, 1536].forEach((v, i) => w16(sym('camera') + i * 2, v));
                }
                return step.call(this);
            };
            try { a.until(() => placed, 60); } finally { cpu.step = step; }
            const wz0 = a.u16(sym('items') + wolf * ITEM_SIZE + ITEM.z);
            a.press(PAD_R);
            expect(a.until(() => a.u16(sym('olLaraWeaponState')) === 4, 120), 'the pistols never came out').toBe(true);
            const wzAt = () => a.u16(sym('items') + wolf * ITEM_SIZE + ITEM.z);
            expect(a.until(() => wzAt() !== wz0, 600), `the wolf never moved (state ${
                memory.read(sym('items') + wolf * ITEM_SIZE + ITEM.state)})`).toBe(true);
            emulator.executeFrames(20);

            saved = a.save(on => { saving = on; });
            const header = Array.from({ length: 8 }, (_, i) =>
                memory.readCartridgeRAM(SAVE_AT + i * 2) | memory.readCartridgeRAM(SAVE_AT + i * 2 + 1) << 8);
            const length = saveFields(n).reduce((t, [, size]) => t + size, 0);
            expect(header.filter((_, i) => i !== 6), 'OLSV, version 3, the ROM\'s checksum, entities, length, the level')
                .toEqual([0x4C4F, 0x5653, 3, a.u16(0x80FFDE), n, (length + 1) & ~1, a.u16(sym('olLevelIndex'))]);
            // The game goes on and leaves the save behind.
            a.pad.buttonState = PAD_UP; emulator.executeFrames(40); a.pad.buttonState = 0;
            emulator.executeFrames(20);
            expect(saveDiff(n, saveImage(memory, n), saved), 'the game stood still after the save').toBeDefined();
            battery = ram.sram.slice();
        } finally {
            ram.writeCartridgeRAM = writeRam;
        }
        expect([...stray].slice(0, 4).map(o => o.toString(16)), 'something else wrote the save\'s window').toEqual([]);
        expect(saveWrites, 'the save wrote nothing').toBeGreaterThan(saved!.length);

        // Off and on again, with the battery's RAM and nothing else.
        const again = new SNESEmulator();
        again.loadROM(rom);
        expect(again.importSRAM(battery!), 'the battery image did not fit the cartridge').toBe(true);
        const b = fx3Passport(again);
        awaitLevel(again);
        again.executeFrames(60);
        expect([b.u16(sym('olSaveValid')), b.u16(sym('olSaveLoad'))], 'the boot found no save, or loaded it unasked')
            .toEqual([1, 0]);
        expect(again.getMemory().read(sym('items') + ITEM.room), 'a power-on put her where the save was').not.toBe(14);
        // The battery's image holds the first boot's request, spent: this is
        // a power-on, and with a title screen it opens the title page, whose
        // passport offers Current Position (upstream's way back to a save).
        const title = TITLE_BIN.length > 0;
        if (title) expect([b.u16(sym('olBootKind')), b.u16(sym('olInvPage')), b.state()],
                          'a power-on with a save did not open the title page').toEqual([2, 0, 4]);
        const loaded = b.load(title);
        const diff = saveDiff(n, loaded, saved!);
        expect(diff, `the load is not the save: ${diff}`).toBeUndefined();
        expect(b.memory.readCartridgeRAM(BOOT_AT) | b.memory.readCartridgeRAM(BOOT_AT + 1) << 8,
            'the boot request was not spent').toBe(0);
        expect(b.u16(sym('olBootKind')), 'not booted as a load').toBe(1);
        expect(b.u16(sym('olSaveValid')), 'loading spent the save').toBe(1);

        // The chase goes on: the wolf owns a Nav context again and moves, her
        // pistols are out.
        const wz = b.u16(sym('items') + wolf * ITEM_SIZE + ITEM.z);
        again.executeFrames(90);
        const owners = [0, 1, 2].map(i => b.u16(sym('olNav') + i * 8240 + 8192 + 22));
        expect(owners, 'the loaded wolf never navigated again').toContain(wolf);
        expect(b.u16(sym('items') + wolf * ITEM_SIZE + ITEM.z), 'the loaded wolf stands still').not.toBe(wz);
        expect(b.u16(sym('olLaraWeaponState')), 'her pistols went back in the holster').toBe(4);
        console.log(`[openlara] save: ${saved!.length} bytes for ${n} entities, ${saveWrites} battery writes, ` +
            `none elsewhere in $70:1000-$31FF; loaded exactly after a power cycle, wolf ${wolf} hunting again`);
    }, 600_000);
});

// A cartridge carrying more than one level (make LEVELS="LEVEL1 LEVEL2").
const LEVELS_ON_CARTRIDGE = FX3 && SYMS.has('SECTIONEND_level_table') ?
    (sym('SECTIONEND_level_table') - sym('LevelTable')) / LEVEL_ENTRY : 1;

describe.skipIf(!FX3 || LEVEL !== 'LEVEL1' || LEVELS_ON_CARTRIDGE < 2)('OpenLara FX3: the cartridge\'s levels', () => {
    // Upstream's gameflow as the GBA's alpha has it: LEVEL1's END trigger is
    // nextLevel(LEVEL2), a load through startLevel; LEVEL2's leads to
    // LVL_TR1_3A, for which the alpha opens its end page, whose Action goes to
    // the title. Here a level is one boot of the cartridge (main.asm
    // SelectLevel), and so is the title page, over the first level. A save
    // remembers its level: the title's Load Game, Current Position, boots
    // LEVEL2 and loads it.
    it('ends LEVEL1 into LEVEL2, saves there, ends on the end page, and loads LEVEL2 back from the title', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const ring = fx3Passport(emulator), memory = ring.memory, u16 = ring.u16;
        const w16 = (a: number, v: number) => { memory.write(a, v & 255); memory.write(a + 1, v >> 8 & 255); };
        awaitLevel(emulator);
        emulator.executeFrames(30);
        const table = sym('LevelTable');
        const hostItems = (level: number) => u16((u16(table + level * LEVEL_ENTRY) << 16) + 16);
        expect([u16(sym('olLevelIndex')), u16(sym('olLevelCount'))], 'a power-on plays the first level').toEqual([0, 2]);
        expect([u16(sym('olLevelIds')), u16(sym('olLevelIds') + 2)], 'Caves, then City of Vilcabamba').toEqual([2, 3]);
        expect(u16(sym('itemsCount'))).toBe(hostItems(0));
        const atGameTick = (setup: () => void) => {
            const cpu = emulator.getCPU(), step = cpu.step;
            let done = false;
            cpu.step = function (this: typeof cpu) {
                if (!done && (this.PB << 16 | this.PC) === sym('gameUpdate')) { done = true; setup(); }
                return step.call(this);
            };
            try { ring.until(() => done, 60); } finally { cpu.step = step; }
            expect(done).toBe(true);
        };
        const booted = (level: number, fields = 900) => ring.until(() =>
            u16(sym('olLevelIndex')) === level && u16(sym('itemsCount')) === hostItems(level) && u16(sym('ol_ticks')) > 4, fields);

        // 1. LEVEL1's END trigger (room 11, its last corridor; the packer's
        //    OL_DUMP_TRIGGERS names it): she steps onto it and the next level
        //    is on its way through the boot.
        const audit = JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8')) as
            { rooms: { x: number; z: number; depth: number; sectors: number[][] }[] };
        const room = audit.rooms[11], floor = room.sectors[1 * room.depth + 4][0] * 256;
        const ex = room.x + 1024 + 512, ez = room.z + 4 * 1024 + 512;
        atGameTick(() => {
            const lara = sym('items');
            memory.write(lara + ITEM.room, 11);
            w16(lara + ITEM.x, ex); w16(lara + ITEM.y, floor); w16(lara + ITEM.z, ez);
            w16(lara + ITEM.angleY, 0);
            [ex, floor - 1024, ez - 1536, 11, 0, 0, 1536].forEach((v, i) => w16(sym('camera') + i * 2, v));
        });
        expect(booted(1), 'LEVEL1\'s END trigger did not boot LEVEL2').toBe(true);
        expect(u16(sym('olLevel') + 6), 'not City of Vilcabamba').toBe(3);
        expect(u16(sym('olBootKind')), 'a level start').toBe(0);
        emulator.executeFrames(60);
        const fb = emulator.getPPU().getFramebuffer();
        const colours = new Set<number>();
        for (let i = 0; i < fb.length; i += 97) colours.add(fb[i]);
        expect(colours.size, 'LEVEL2 never reached the screen').toBeGreaterThan(8);

        // 2. A save in LEVEL2, then its END: the alpha's end page, the
        //    background and upstream's five lines, no ring.
        ring.pad.buttonState = PAD_UP; emulator.executeFrames(30); ring.pad.buttonState = 0;
        emulator.executeFrames(10);
        const saved = ring.save();
        const n2 = u16(sym('itemsCount'));
        atGameTick(() => w16(sym('olLevelEnd'), 1));
        expect(ring.until(() => u16(sym('olInvPage')) === 2 && ring.state() === 4, 30), 'no end page').toBe(true);
        emulator.executeFrames(60);
        expect(memory.readCartridgeRAM(0xDE40) | memory.readCartridgeRAM(0xDE41) << 8, 'the ring drew items')
            .toBe(0x8000);
        // Read between render syncs: Inventory_text resets the count it is building.
        expect(ring.until(() => u16(sym('olInvText')) > 60, 60),
               'the end page\'s lines are not drawn').toBe(true);
        if (process.env.OL_INV_SNAPS) {
            const { stride, width, height } = emulator.getPPU().getFrameGeometry();
            const shot = emulator.getPPU().getFramebuffer();
            writeFileSync(join(dirname(ROM), 'inv_end.png'), png(width, height, (x, y) => shot[y * stride + x]));
        }
        ring.press(PAD_B);
        expect(ring.until(() => u16(sym('olBootKind')) === 2 && u16(sym('olLevelIndex')) === 0 &&
                                u16(sym('olInvPage')) === 0 && ring.state() === 4, 900),
               'the end page\'s Action did not go to the title').toBe(true);

        // 3. The title's Load Game lists both levels and Current Position;
        //    it boots LEVEL2 and puts the save back, exactly.
        const loaded = ring.load(true);
        expect(u16(sym('olLevelIndex')), 'Current Position did not boot the save\'s level').toBe(1);
        const diff = saveDiff(n2, loaded, saved);
        expect(diff, `the load is not the save: ${diff}`).toBeUndefined();
        console.log(`[openlara] levels: LEVEL1's END booted LEVEL2 (${hostItems(1)} entities); a save there, ` +
            `the end page, the title, and its Current Position loaded LEVEL2 exactly (${saved.length} bytes)`);
    }, 600_000);
});

describe.skipIf(!FX3 || LEVEL !== 'LEVEL1' || TITLE_BIN.length === 0)('OpenLara FX3: the title page', () => {
    // Upstream's startLevel(LVL_TR1_TITLE): the title screen with the ring
    // over it -- INV_PAGE_TITLE's passport, controls, detail, sound and
    // Lara's Home -- and nothing else moving; Select and Jump do not close
    // it, and its passport's Load Game starts a level. Here a power-on
    // opens it over the first level, held at its load (game.c), with the
    // picture in BG_BUF (main.asm CopyTitle) and its colours at CG-RAM
    // 192-255. Exit to Title, from a level's passport, comes back to it.
    it('opens the ring on the title screen at power-on; its passport starts the level, Exit to Title returns', () => {
        const emulator = new SNESEmulator();
        COLD_BOOT = true;
        try { emulator.loadROM(new Uint8Array(readFileSync(ROM))); } finally { COLD_BOOT = false; }
        const ring = fx3Passport(emulator), memory = ring.memory, u16 = ring.u16;
        const atTitle = () => u16(sym('olBootKind')) === 2 && u16(sym('olInvPage')) === 0 && ring.state() === 4;
        expect(ring.until(atTitle, 900), 'no title page at power-on').toBe(true);
        expect(u16(sym('olLevelIndex')), 'the title is not over the first level').toBe(0);
        // Lara's Home only where the level has its model (LEVEL1 does not)
        const inv = readFileSync(join(DATA_DIR, 'INV.BIN')), home = inv[25 * 8 + 2] !== 0xFF;
        expect(Array.from({ length: u16(sym('olInvItemsCount')) }, (_, i) => memory.read(sym('olInvItemsList') + i)),
               'the title\'s ring: passport, controls, detail, sound (home)').toEqual([21, 24, 22, 23, ...(home ? [25] : [])]);
        for (let i = 0; i < 64; i++)
            expect(memory.readCGRAM(0x180 + i * 2) | memory.readCGRAM(0x181 + i * 2) << 8, `title colour ${i}`)
                .toBe(TITLE_BIN.readUInt16LE(i * 2));
        const pixels = TITLE_BIN.subarray(128);
        let copied = 0;
        for (let i = 0; i < pixels.length; i++) if (memory.readCartridgeRAM(0x3400 + i) === pixels[i]) copied++;
        expect(copied, 'BG_BUF is not the title screen').toBe(pixels.length);

        // Held still: her pose and place, through the ring's whole opening
        // and a second of it standing
        const lara = sym('items');
        const pose = () => [ITEM.x, ITEM.y, ITEM.z, ITEM.angleY, ITEM.animIndex].map(o => u16(lara + o))
            .concat(memory.read(lara + ITEM.frameIndex));
        const before = pose(), ticks = u16(sym('ol_ticks'));
        emulator.executeFrames(60);
        expect(u16(sym('ol_ticks')), 'the clock stopped').toBeGreaterThan(ticks + 20);
        expect(pose(), 'the level moved under the title').toEqual(before);
        // Select and Jump put a ring away -- not this one
        ring.press(0x2000);
        ring.press(PAD_A);
        emulator.executeFrames(20);
        expect(atTitle(), 'Select or Jump closed the title\'s ring').toBe(true);

        // On screen: the picture wherever the ring and its text are not
        const frame = fx3ShownFrame(memory);
        let same = 0;
        for (let i = 0; i < frame.length; i++) if (frame[i] === pixels[i]) same++;
        {
            const { stride, width, height } = emulator.getPPU().getFrameGeometry();
            const shot = emulator.getPPU().getFramebuffer();
            writeFileSync(join(dirname(ROM), 'title.png'), png(width, height, (x, y) => shot[y * stride + x]));
        }
        expect(same / frame.length, 'the title screen is not what the frame shows').toBeGreaterThan(0.6);

        // The passport, Load Game: the cartridge's levels (and Current
        // Position, if the battery RAM holds a save); the first level
        ring.press(PAD_B);
        const shown = () => ring.state() === 9 && u16(sym('olInvFrameIndex')) === u16(sym('olInvFrameTarget'));
        expect(ring.until(shown), 'the title\'s passport never opened').toBe(true);
        const levels = u16(sym('olLevelCount'));
        expect(u16(sym('olInvPassportPage')), 'not Load Game').toBe(0);
        expect(u16(sym('olInvOptionsCount')), 'Load Game\'s options')
            .toBe(levels + (u16(sym('olSaveValid')) ? 2 : 0));
        for (let n = 0; n < 4 && u16(sym('olInvOptionIndex')) !== 0; n++) ring.press(PAD_UP);
        expect(u16(sym('olInvOptionIndex'))).toBe(0);
        ring.press(PAD_B);
        const playing = () => u16(sym('olBootKind')) === 0 && u16(sym('olLevelIndex')) === 0 &&
            u16(sym('itemsCount')) > 0 && ring.state() === 0 && u16(sym('ol_ticks')) > 30;
        expect(ring.until(playing, 900), 'the passport did not start the level').toBe(true);
        const start = pose();
        ring.pad.buttonState = PAD_UP;
        emulator.executeFrames(30);
        ring.pad.buttonState = 0;
        expect(pose().slice(0, 3), 'she does not move in the level the title started').not.toEqual(start.slice(0, 3));

        // Exit to Title: the options' passport, two pages on
        ring.open();
        ring.press(PAD_RIGHT);
        expect(ring.until(() => shown() && u16(sym('olInvPassportPage')) === 2), 'no Exit to Title page').toBe(true);
        ring.press(PAD_B);
        expect(ring.until(() => u16(sym('olBootKind')) === 2 && u16(sym('itemsCount')) > 0 && atTitle(), 900),
               'Exit to Title did not come back to the title page').toBe(true);
        copied = 0;
        for (let i = 0; i < pixels.length; i++) if (memory.readCartridgeRAM(0x3400 + i) === pixels[i]) copied++;
        expect(copied, 'BG_BUF is not the title screen again').toBe(pixels.length);
        console.log(`[openlara] title: the ring over the title screen at power-on (${(same * 100 / frame.length).toFixed(1)}% ` +
            `of the frame is the picture), held still; its passport started level 0 and Exit to Title came back`);
    }, 600_000);
});

describe.skipIf(!FX3 || LEVEL !== 'LEVEL1')('OpenLara FX3: settings', () => {
    // Upstream's options pages and gSettings (settings.c): with no settings
    // in the battery RAM, upstream's defaults -- the frame counter shown
    // (drawFPS, the GSU's TextRender over game frames). The Graphics page's
    // gamma bar is palGamma over CG-RAM, its Show FPS switch the counter;
    // Sound's SFX switch silences Sound_play; Controls' Swap A B trades the
    // face buttons. Each change is kept in the battery RAM and comes back
    // after a power cycle.
    it('shows the frame counter, and the options pages set gamma, FPS, SFX and swap, kept through a power cycle', () => {
        const rom = new Uint8Array(readFileSync(ROM));
        const emulator = new SNESEmulator();
        OWN_SETTINGS = true;
        try { emulator.loadROM(rom); } finally { OWN_SETTINGS = false; }
        const ring = fx3Passport(emulator), memory = ring.memory, u16 = ring.u16;
        const cart = (o: number) => memory.readCartridgeRAM(o);
        const set = () => Array.from({ length: 7 }, (_, i) => memory.read(sym('olSettings') + i));
        awaitLevel(emulator);
        emulator.executeFrames(150);
        expect(set(), 'upstream\'s defaults: rumble, no swap, SFX, music, gamma 0, FPS, no vsync')
            .toEqual([1, 0, 1, 1, 0, 1, 0]);
        // drawFPS: its digits in GS_TEXT, which FrameBody draws over the game
        const fps = memory.read(0x7e0000) | memory.read(0x7e0001) << 8;
        expect(ring.until(() => cart(0xEA00) === String(memory.read(0x7e0000) | memory.read(0x7e0001) << 8).length, 120),
               `no frame counter over the game (fps ${fps}, GS_TEXT ${cart(0xEA00)})`).toBe(true);
        expect(cart(0xEA08), 'the counter is not at upstream\'s x 2').toBeLessThan(8);
        const snap = (name: string) => {
            const { stride, width, height } = emulator.getPPU().getFrameGeometry();
            const shot = emulator.getPPU().getFramebuffer();
            writeFileSync(join(dirname(ROM), name), png(width, height, (x, y) => shot[y * stride + x]));
        };
        emulator.executeFrames(30);
        snap('settings_fps.png');

        // the options page's ring: passport, controls, detail, sound
        const page = (slot: number) => {
            ring.press(0x2000);
            expect(ring.until(() => ring.state() === 4), 'the ring never opened').toBe(true);
            ring.press(PAD_DOWN);
            expect(ring.until(() => ring.state() === 4 && u16(sym('olInvPage')) === 6), 'no options page').toBe(true);
            const at = () => memory.read(sym('olInvItemsList') + u16(sym('olInvItemIndex')));
            for (let n = 0; n < 4 && at() !== slot; n++) {
                ring.press(PAD_LEFT);
                ring.until(() => ring.state() === 4);
            }
            expect(at(), `slot ${slot} is not on the options page`).toBe(slot);
            ring.press(PAD_B);
            expect(ring.until(() => ring.state() === 9 && u16(sym('olInvFrameIndex')) === u16(sym('olInvFrameTarget'))),
                   `slot ${slot} never came forward`).toBe(true);
        };
        const close = () => {
            ring.press(0x2000);                 // put it back (Select: A may be swapped)
            expect(ring.until(() => ring.state() === 4), 'the item did not go back').toBe(true);
            ring.press(0x2000);
            expect(ring.until(() => ring.state() === 0), 'the ring did not close').toBe(true);
            const t = u16(sym('ol_ticks'));             // and the level's sounds are back
            expect(ring.until(() => u16(sym('ol_ticks')) > t + 1, 60), 'the closing tick never ended').toBe(true);
        };
        const value = (i: number) => u16(sym('olInvOptValue') + i * 2);

        // Graphics (22): gamma 0 -> 4 with four Rights, then Show FPS off
        page(22);
        expect([u16(sym('olInvOptionsCount')), value(0), value(1), value(2)], 'Gamma 0, Show FPS on, VSync off')
            .toEqual([3, 0, 1, 0]);
        for (let n = 0; n < 4; n++) ring.press(PAD_RIGHT);
        expect([value(0), set()[4]], 'four Rights: 64 of 256, gamma 4').toEqual([64, 4]);
        expect(ring.until(() => u16(sym('olInvRects')) % 256 >= 15, 30), 'the bar is not drawn').toBe(true);
        expect(ring.until(() => u16(sym('olGammaDirty')) === 0 && u16(sym('olCgramLeft')) === 0, 30),
               'CG-RAM never took the gamma').toBe(true);
        const gammaTable = Array.from({ length: 32 }, (_, c) => Math.min(31, c + Math.floor(((c * c >> 2) - c) * 64 / 1024)));
        const gamma = (v: number) => gammaTable[v & 31] | gammaTable[v >> 5 & 31] << 5 | gammaTable[v >> 10 & 31] << 10;
        const pal = readFileSync(join(DATA_DIR, 'PALETTE.BIN'));
        for (let i = 0; i < 128; i++)
            expect(memory.readCGRAM(i * 2) | memory.readCGRAM(i * 2 + 1) << 8, `colour ${i} under gamma 4`)
                .toBe(gamma(pal.readUInt16LE(i * 2)));
        emulator.executeFrames(20);
        snap('settings_graphics.png');
        ring.press(PAD_DOWN);
        ring.press(PAD_LEFT);
        expect([u16(sym('olInvOptionIndex')), value(1), set()[5]], 'Show FPS off').toEqual([1, 0, 0]);
        close();
        expect(ring.until(() => cart(0xEA00) === 0, 60), 'the counter stayed').toBe(true);

        // Sound (23): her steps reach the APU; SFX off, and they send nothing
        const steps = () => {
            const seq = u16(sym('olSndSeq'));
            ring.pad.buttonState = PAD_UP; emulator.executeFrames(90); ring.pad.buttonState = 0;
            emulator.executeFrames(10);
            return u16(sym('olSndSeq')) !== seq;
        };
        expect(steps(), 'her steps sent nothing with SFX on').toBe(true);
        page(23);
        ring.press(PAD_RIGHT);
        expect(set()[2], 'SFX still on').toBe(0);
        close();
        expect(steps(), 'a sound went to the APU with SFX off').toBe(false);

        // Controls (24): Swap A B, and B (upstream's jump now) lifts her
        page(24);
        ring.press(PAD_DOWN);
        ring.press(PAD_RIGHT);
        expect(set()[1], 'Swap A B still off').toBe(1);
        close();
        const lara = sym('items');
        // standing: the steps above can leave her running when the ring froze her
        expect(ring.until(() => memory.read(lara + ITEM.state) === 2, 120), 'she never stood still').toBe(true);
        const y0 = (u16(lara + ITEM.y) << 16) >> 16;
        let top = y0;
        ring.pad.buttonState = PAD_B;
        for (let f = 0; f < 40; f++) { emulator.executeFrames(1); top = Math.min(top, (u16(lara + ITEM.y) << 16) >> 16); }
        ring.pad.buttonState = 0;
        emulator.executeFrames(60);
        expect(y0 - top, 'B did not jump with A and B swapped').toBeGreaterThan(64);

        // kept: "OLST", version 1, the seven fields; and after a power cycle
        expect(Array.from({ length: 12 }, (_, i) => cart(SETTINGS_AT + i)), 'the battery\'s settings')
            .toEqual([0x4F, 0x4C, 0x53, 0x54, 1, 1, 1, 0, 1, 4, 0, 0]);
        const battery = (memory as unknown as { sram: Uint8Array }).sram.slice();
        const again = new SNESEmulator();
        OWN_SETTINGS = true;
        try { again.loadROM(rom); } finally { OWN_SETTINGS = false; }
        expect(again.importSRAM(battery)).toBe(true);
        const b = fx3Passport(again);
        awaitLevel(again);
        again.executeFrames(60);
        expect(Array.from({ length: 7 }, (_, i) => b.memory.read(sym('olSettings') + i)), 'the settings after a power cycle')
            .toEqual([1, 1, 0, 1, 4, 0, 0]);
        expect(b.until(() => b.u16(sym('olCgramLeft')) === 0, 60) &&
               (again.getMemory().readCGRAM(2) | again.getMemory().readCGRAM(3) << 8) === gamma(pal.readUInt16LE(2)),
               'the gamma did not come back at the load').toBe(true);
        console.log(`[openlara] settings: the counter over the game by default (fps ${fps}); gamma 4 over CG-RAM, ` +
            `FPS off, SFX off (her steps silent), A/B swapped (B jumps ${y0 - top}), all kept through a power cycle`);
    }, 600_000);
});

describe.skipIf(!FX3 || LEVEL !== 'LEVEL1')('OpenLara FX3: lean and shadow', () => {
    // Upstream's angle.z: the run's s_rotate(LARA_TURN_FAST, 1) leans her
    // 1.5 degrees a tick into the turn, to 11, and updateAbove takes one
    // back every tick -- so she settles at 11 while the turn is held and
    // straightens once it is let go. The GSU turns her by it (GS_ROLL,
    // mesh.gsu ItemTilt) after the pitch.
    it('casts her shadow, leans into a running turn, no further than eleven degrees, and straightens after it', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        emulator.executeFrames(60);
        const memory = emulator.getMemory();
        const s16 = (a: number) => ((memory.read(a) | (memory.read(a + 1) << 8)) << 16) >> 16;
        const pad = emulator.getController() as unknown as { buttonState: number };
        const TILT_MAX = 11 * 182;
        // The shadows (drawShadow(item, 160)): hers first, then the nearest
        // creatures', published with the frame's entities and drawn by the
        // GSU's ShadowRender under each, an octagon of the floor darkened
        // -- the last one drawn a few pixels each way from here.
        {
            const pts = [...Array(16).keys()].map(i => s16(0x70F7C0 + 2 * i));
            const xs = pts.filter((_, i) => !(i & 1)), ys = pts.filter((_, i) => i & 1);
            const n = s16(sym('olShadow'));
            console.log(`[openlara] shadow: ${n} published, hers at slot $${(s16(sym('olShadow') + 6) & 0xFFFF).toString(16)} ` +
                        `depth ${s16(sym('olShadow') + 8)}, last corners ${pts.join(',')}`);
            expect(n, 'no shadow published').toBeGreaterThanOrEqual(1);
            expect(s16(sym('olShadow') + 6) & 0xFFFF, 'hers is not first').toBe(0xF800);
            expect(Math.max(...xs) - Math.min(...xs), 'the shadow has no width').toBeGreaterThan(3);
            expect(Math.max(...ys) - Math.min(...ys), 'the shadow has no depth').toBeGreaterThan(3);
            if (process.env.OL_INV_SNAPS) {
                const fb = emulator.getPPU().getFramebuffer();
                const { stride, width, height } = emulator.getPPU().getFrameGeometry();
                writeFileSync(join(dirname(ROM), 'stand.png'), png(width, height, (x, y) => fb[y * stride + x]));
            }
        }
        let min = 0, max = 0, roll = 0;
        pad.buttonState = PAD_UP | PAD_LEFT;
        for (let i = 0; i < 120; i++) {
            emulator.executeFrames(1);
            const z = s16(sym('olLaraAngleZ'));
            min = Math.min(min, z); max = Math.max(max, z);
            if (memory.read(0x70F732) !== 0) roll = memory.read(0x70F732);
        }
        if (process.env.OL_INV_SNAPS) {
            const fb = emulator.getPPU().getFramebuffer();
            const { stride, width, height } = emulator.getPPU().getFrameGeometry();
            writeFileSync(join(dirname(ROM), 'lean.png'), png(width, height, (x, y) => fb[y * stride + x]));
        }
        console.log(`[openlara] lean: ${min}..${max} running left, GS_ROLL seen $${roll.toString(16)}`);
        expect(min, 'she did not lean into the turn').toBeLessThan(-TILT_MAX / 2);
        expect(min, 'past eleven degrees').toBeGreaterThanOrEqual(-TILT_MAX);
        expect(max, 'she leant the wrong way').toBe(0);
        expect(roll, 'the renderer never got her lean').toBeGreaterThan(0x80);
        pad.buttonState = PAD_UP;
        let straight = false;
        for (let i = 0; i < 120 && !straight; i++) { emulator.executeFrames(1); straight = s16(sym('olLaraAngleZ')) === 0; }
        pad.buttonState = 0;
        expect(straight, 'she stayed leant after the turn').toBe(true);
    });
});

describe.skipIf(!FX3 || LEVEL !== 'LEVEL1')('OpenLara FX3: camera triggers', () => {
    // LEVEL1's lever item 52 (room 33, facing -X) opens the doors of room 24
    // through trigger 90, which also switches to fixed camera 0 for three
    // seconds aimed at camera-target item 41 (CAMERA.BIN: packer/out_SNES.h).
    // Upstream (room.h checkCamera/checkTrigger, camera.h update/updateFixed)
    // puts the view at the camera the tick the lever fires, keeps it there
    // while she stands on the lever's sector until the timer runs out, and
    // snaps back to following (lastFixed: speed 1) the tick after.
    const boot = () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        emulator.executeFrames(30);
        const memory = emulator.getMemory();
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        const s16 = (a: number) => (u16(a) << 16) >> 16;
        const w16 = (a: number, v: number) => { memory.write(a, v & 0xFF); memory.write(a + 1, (v >> 8) & 0xFF); };
        const cpu = emulator.getCPU();
        const atGameTick = (setup: () => void) => {
            const original = cpu.step;
            let applied = false;
            cpu.step = function () {
                if (!applied && (cpu.PB << 16 | cpu.PC) === sym('gameUpdate')) { setup(); applied = true; }
                return original.call(this);
            };
            try { for (let i = 0; i < 60 && !applied; i++) emulator.executeFrames(1); }
            finally { cpu.step = original; }
            expect(applied).toBe(true);
        };
        const place = (room: number, x: number, y: number, z: number, facing: number) => atGameTick(() => {
            const it = sym('items');
            memory.write(it + ITEM.room, room);
            w16(it + ITEM.x, x); w16(it + ITEM.y, y); w16(it + ITEM.z, z);
            w16(it + ITEM.angleY, facing); w16(sym('olLaraMoveAngle'), facing);
        });
        // A row per tick, read at each gameUpdate entry: the tick before it
        // is whole.
        const rows: Record<string, number>[] = [];
        const record = () => {
            const original = cpu.step;
            cpu.step = function () {
                if ((cpu.PB << 16 | cpu.PC) === sym('gameUpdate')) {
                    const it = sym('items'), look = sym('olLaraLook');
                    rows.push({
                        cx: s16(sym('ol_camX')), cy: s16(sym('ol_camY')), cz: s16(sym('ol_camZ')),
                        room: u16(sym('ol_camRoom')), yaw: u16(sym('ol_angle')),
                        tx: s16(sym('camTarget')), ty: s16(sym('camTarget') + 2), tz: s16(sym('camTarget') + 4),
                        fixed: u16(sym('olCamFixedView')), busy: u16(sym('olCamBusy')),
                        timer: s16(sym('olCamTimer')), lookAt: u16(sym('olCamLookAt')),
                        lastItem: u16(sym('olCamLastItem')), state: memory.read(it + ITEM.state),
                        lx: s16(it + ITEM.x), lz: s16(it + ITEM.z), room0: memory.read(it + ITEM.room),
                        headX: s16(look + 4), headY: s16(look + 6), torsoY: s16(look + 2),
                        distH: s16(sym('camera') + 14),
                    });
                }
                return original.call(this);
            };
            return () => { cpu.step = original; };
        };
        return { emulator, memory, u16, s16, w16, place, rows, record,
                 pad: emulator.getController() as unknown as { buttonState: number } };
    };
    const camera = (i: number) => {
        const d = readFileSync(join(DATA_DIR, 'CAMERA.BIN'));
        return { x: d.readInt16LE(4 + i * 8), y: d.readInt16LE(6 + i * 8), z: d.readInt16LE(8 + i * 8), room: d[10 + i * 8] };
    };
    const yawTo = (dx: number, dz: number) => Math.round(Math.atan2(dx, dz) / (2 * Math.PI) * 65536) & 0xFFFF;
    const angDiff = (a: number, b: number) => { const d = ((a - b) & 0xFFFF); return d >= 0x8000 ? 0x10000 - d : d; };

    it('cuts to the lever\'s fixed camera, aimed at its target, for its three seconds, then follows again', () => {
        const g = boot();
        const it = sym('items'), target = it + 41 * ITEM_SIZE, lever = it + 52 * ITEM_SIZE;
        expect(g.memory.read(lever + ITEM.type), 'item 52 is not the lever').toBe(55);
        expect(g.memory.read(target + ITEM.type), 'item 41 is not a camera target').toBe(169);
        const cam = camera(0);
        // Standing at the lever as LIMIT_SWITCH wants her: 350 along its
        // facing (-X), facing it.
        const lx = g.s16(lever + ITEM.x) - 350, ly = g.s16(lever + ITEM.y), lz = g.s16(lever + ITEM.z);
        g.place(g.memory.read(lever + ITEM.room), lx, ly, lz, 0xC000);
        g.emulator.executeFrames(90);
        expect(g.u16(sym('olCamBusy')), 'a fixed camera before the lever was pulled').toBe(0);
        const stop = g.record();
        try {
            g.pad.buttonState = PAD_B;
            for (let i = 0; i < 90 && g.memory.read(it + ITEM.state) !== 40; i++) g.emulator.executeFrames(1);
            g.pad.buttonState = 0;
            g.emulator.executeFrames(420);
        } finally { stop(); }
        const rows = g.rows;
        expect(rows.some(r => r.state === 40), 'she never pulled the lever').toBe(true);
        const first = rows.findIndex(r => r.fixed);
        expect(first, 'the lever never switched to its camera').toBeGreaterThan(0);
        let n = 0;
        while (first + n < rows.length && rows[first + n].fixed) n++;
        const fixedRows = rows.slice(first, first + n), after = rows[first + n];
        console.log(`[openlara] camera trigger: fixed from tick ${first} for ${n} ticks at ` +
                    `(${fixedRows[0].cx},${fixedRows[0].cy},${fixedRows[0].cz}) room ${fixedRows[0].room} yaw ${fixedRows[0].yaw}; ` +
                    `after, (${after.cx},${after.cy},${after.cz}) room ${after.room}, she at (${after.lx},${after.lz})`);
        // Her pull asks upstream's framing before the cut (STATE_SWITCH_DOWN:
        // targetAngle -25, +80 degrees, targetDist 1024): distH 1024 cos 25.
        // (The tick she enters the state ran the previous state's handler.)
        const pulling = rows.slice(0, first).filter((r, k, a) => r.state === 40 && k > 0 && a[k - 1].state === 40);
        expect(pulling.length, 'no follow ticks while she pulled').toBeGreaterThan(5);
        expect(pulling.every(r => r.distH >= 925 && r.distH <= 931),
               `the pull did not frame her at 1024: distH ${[...new Set(pulling.map(r => r.distH))]}`).toBe(true);
        // Timer 3: three seconds of 30 ticks, the first the tick it fired.
        expect(n, 'the fixed camera did not hold for its timer').toBe(90);
        const tx = g.s16(target + ITEM.x), ty = g.s16(target + ITEM.y), tz = g.s16(target + ITEM.z);
        for (const [k, r] of fixedRows.entries()) {
            expect([r.cx, r.cy, r.cz, r.room], `fixed tick ${k}: not at camera 0`).toEqual([cam.x, cam.y, cam.z, cam.room]);
            expect([r.tx, r.tz], `fixed tick ${k}: not aimed at item 41`).toEqual([tx, tz]);
            // phd_atan is upstream's table lookup (0.4 degrees off atan2 here)
            expect(angDiff(r.yaw, yawTo(tx - cam.x, tz - cam.z)), `fixed tick ${k}: yaw ${r.yaw}`).toBeLessThanOrEqual(128);
            expect(r.room0, 'she left the lever\'s room').toBe(g.memory.read(lever + ITEM.room));
        }
        // lastFixed: the target's height snaps to the target's middle -- its
        // y plus the middle of its animation's box -- the first tick, then
        // eases by a quarter toward it, which does not move.
        const frames = JSON.parse(readFileSync(join(DATA_DIR, 'FRAMES.json'), 'utf8')) as {
            animations: { anim: number; boxes: number[][] }[] };
        const boxes = frames.animations.find(a => a.anim === g.u16(target + ITEM.animIndex))!.boxes;
        const minY = Math.min(...boxes.map(b => b[2])), maxY = Math.max(...boxes.map(b => b[3]));
        const middle = ty + Math.floor((minY + maxY) / 2);
        expect(fixedRows[0].ty, 'the target height did not snap on the cut').toBe(middle);
        expect(fixedRows.every(r => r.ty === middle), 'the target height drifted off the target').toBe(true);
        // The timer ran out while she stood there: back to following in one
        // tick (lastFixed again: speed 1), near her and out of room 24.
        expect(after.fixed).toBe(0);
        const dist = Math.hypot(after.cx - after.lx, after.cz - after.lz);
        expect(dist, 'the view did not come back to her in one tick').toBeLessThan(1700);
        expect(after.room, 'the view stayed in the camera\'s room').not.toBe(cam.room);
        // Standing on the lever's sector, checkCamera still asks every tick
        // (and finds the camera over); stepped off, the state comes to rest.
        g.place(g.memory.read(lever + ITEM.room), lx - 1024, ly, lz, 0xC000);
        g.emulator.executeFrames(30);
        expect(g.u16(sym('olCamBusy')), 'the fixed-camera state never came to rest').toBe(0);
        // And the doors the lever opens did open (trigger 90's items).
        for (const door of [35, 36])
            expect(g.memory.read(it + door * ITEM_SIZE + ITEM.flags) & 0x40, `door ${door} is not active`).not.toBe(0);
    });

    it('turns her head to a camera target the first time she stands where a trigger names it, and never again', () => {
        const g = boot();
        const it = sym('items'), target = it + 4 * ITEM_SIZE;
        expect(g.memory.read(target + ITEM.type), 'item 4 is not a camera target').toBe(169);
        // Room 4's triggers 4-11 (sectors x 1-4, z 4-5) name item 4, up the
        // slope ahead; this is the middle of the first. Facing +Z she can
        // turn her head to it (44 degrees of yaw, 22 down to 42 up).
        const room = 4, x = 75776 + 1024 + 512 - 65536, z = 27648 + 4 * 1024 + 512, y = 12 * 256;
        g.place(room, x, y, z, 0);
        const stop = g.record();
        try { g.emulator.executeFrames(120); } finally { stop(); }
        let rows = g.rows.splice(0);
        const looking = rows.filter(r => r.lookAt === 4 || r.lastItem === 4);
        expect(looking.length, 'she never looked at the camera target').toBeGreaterThan(20);
        expect(g.memory.read(target + ITEM.flags2) & 0x20, 'the target was not marked looked-at (ANIMATED)').not.toBe(0);
        // lookAt: half the angle to the target, turned two degrees a tick.
        // Wrapped positions: differences in sixteen bits (item 4 is past the
        // seam at z 32767).
        const tx = g.s16(target + ITEM.x), tz = g.s16(target + ITEM.z);
        const d16 = (v: number) => (v << 16) >> 16;
        const wantY = (d16(yawTo(d16(tx - x), d16(tz - z)))) >> 1;
        const heads = rows.map(r => r.headY);
        const settled = heads[heads.length - 1];
        console.log(`[openlara] camera target: head yaw ${heads.slice(0, 12).join(',')}... settled ${settled} ` +
                    `(half the yaw to it: ${wantY}); view ${Math.round(Math.hypot(rows[rows.length - 1].cx - x, rows[rows.length - 1].cz - z))} away`);
        expect(Math.abs(settled - wantY), 'her head did not settle on the target').toBeLessThanOrEqual(3 * 182);
        for (let k = 1; k < heads.length; k++) {
            const step = heads[k] - heads[k - 1];
            expect(Math.abs(step), `head step ${k}`).toBeLessThanOrEqual(364);
        }
        expect(rows.every(r => r.torsoY === r.headY), 'her torso did not turn with her head').toBe(true);
        // Upstream's look camera, at the follow distance while an item holds it.
        const last = rows[rows.length - 1];
        expect(last.distH, 'the look camera is not at the follow distance').toBeGreaterThan(1000);

        // Away and back: the target is spent, so she does not look again, and
        // her head comes back to rest.
        g.place(room, x + 3 * 1024, y - 512, z - 3 * 1024, 0);
        g.emulator.executeFrames(90);
        g.place(room, x, y, z, 0);
        const stop2 = g.record();
        try { g.emulator.executeFrames(120); } finally { stop2(); }
        rows = g.rows.splice(0);
        expect(rows.every(r => r.lookAt === 255), 'she looked at a spent target again').toBe(true);
        // upstream's decay, angleDec(h, |h| >> 3), stops short of zero
        expect(Math.abs(rows[rows.length - 1].headY), 'her head did not come back to rest').toBeLessThan(8);
    });
});

describe.skipIf(LEVEL !== 'LEVEL1')('OpenLara SNES/Super FX: death', () => {
    it.skipIf(!SPC)('restarts the level on a Select pressed after she dies, sound and all', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        const s16 = (a: number) => (u16(a) << 16) >> 16;
        const w16 = (a: number, v: number) => { memory.write(a, v & 0xFF); memory.write(a + 1, (v >> 8) & 0xFF); };
        const lara = sym('items');
        const pad = emulator.getController() as unknown as { buttonState: number };
        const PAD_SELECT = 0x2000;
        const settleLevel = () => { awaitLevel(emulator); for (let i = 0; i < 600 && u16(sym('ol_ticks')) < 20; i++) emulator.executeFrames(1); };
        const atGameTick = (setup: () => void) => {
            const cpu = emulator.getCPU(), original = cpu.step;
            let applied = false;
            cpu.step = function () {
                if (!applied && (cpu.PB << 16 | cpu.PC) === sym('gameUpdate')) { setup(); applied = true; }
                return original.call(this);
            };
            try { for (let i = 0; i < 60 && !applied; i++) emulator.executeFrames(1); } finally { cpu.step = original; }
            expect(applied).toBe(true);
        };
        settleLevel();
        const start = [s16(lara + ITEM.x), s16(lara + ITEM.y), s16(lara + ITEM.z), memory.read(lara + ITEM.room)];
        expect(u16(sym('ol_sndReady'))).toBe(1);

        // Alive, Select does nothing here (upstream opens the inventory).
        const rebooted = (fields: number) => {
            let last = u16(sym('ol_ticks'));
            for (let i = 0; i < fields; i++) {
                emulator.executeFrames(1);
                const t = u16(sym('ol_ticks'));
                if (t < last) return true;
                last = t;
            }
            return false;
        };
        if (FX3) {
            // FX3 has upstream's death page: Select opens the ring on the
            // passport, shown at Load Game, whose one option is this level;
            // Action on it starts the level again through the boot.
            atGameTick(() => { w16(lara + ITEM.x, start[0] + 512); w16(lara + ITEM.health, 0); });
            expect(rebooted(30)).toBe(false);
            pad.buttonState = PAD_SELECT; emulator.executeFrames(4); pad.buttonState = 0;
            let shown = false;
            for (let i = 0; i < 400 && !shown; i++) {
                emulator.executeFrames(1);
                shown = u16(sym('olInvState')) === 9 && u16(sym('olInvFrameIndex')) === u16(sym('olInvFrameTarget'));
            }
            expect(shown, 'the death page never showed the passport').toBe(true);
            expect(u16(sym('olInvPage')), 'not the death page').toBe(1);
            expect(memory.read(sym('olInvItemsList') + u16(sym('olInvItemIndex'))), 'not the passport').toBe(21);
            expect([u16(sym('olInvPassportPage')), u16(sym('olInvOptionsCount'))], 'Load Game, the levels')
                .toEqual([0, u16(sym('olLevelCount'))]);
            emulator.executeFrames(20);
            if (process.env.OL_INV_SNAPS) {
                const fb = emulator.getPPU().getFramebuffer();
                const { stride, width, height } = emulator.getPPU().getFrameGeometry();
                writeFileSync(join(dirname(ROM), 'inv_death.png'), png(width, height, (x, y) => fb[y * stride + x]));
            }
            pad.buttonState = PAD_B;        // Action
            expect(rebooted(200), 'Load Game did not restart the level').toBe(true);
            pad.buttonState = 0;
        } else {
        pad.buttonState = PAD_SELECT;
        expect(rebooted(60), 'Select restarted a living Lara').toBe(false);

        // Held through her death: not a press after it.
        atGameTick(() => { w16(lara + ITEM.x, start[0] + 512); w16(lara + ITEM.health, 0); });
        expect(rebooted(120), 'a Select held since before her death restarted').toBe(false);
        expect(s16(lara + ITEM.health)).toBe(0);
        pad.buttonState = 0;
        expect(rebooted(30)).toBe(false);

        // Pressed after it: back through the boot.
        pad.buttonState = PAD_SELECT;
        expect(rebooted(60), 'Select after her death did not restart').toBe(true);
        pad.buttonState = 0;
        }
        settleLevel();
        expect(s16(lara + ITEM.health), 'she did not come back').toBe(1000);
        expect([s16(lara + ITEM.x), s16(lara + ITEM.y), s16(lara + ITEM.z), memory.read(lara + ITEM.room)],
               'not at the level start').toEqual(start);
        expect(u16(sym('ol_sndReady')), 'the sound CPU did not come back through its IPL').toBe(1);
        const driver = readFileSync(SPC);
        const { ram } = apuOf(emulator);
        expect(Buffer.from(ram.subarray(0x0200, 0x0200 + driver.length)).equals(driver),
               'the driver was not uploaded again').toBe(true);
        console.log(`[openlara] death: ${FX3 ? 'the death page\'s Load Game' : 'Select'} restarted the level; she stands at ${start} with ` +
                    `${s16(lara + ITEM.health)} health and the sound driver re-uploaded`);
    }, 600_000);
});

describe('OpenLara SNES/Super FX: Lara', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('stands, runs, and turns through her own state graph', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const s16 = (a: number) => { const v = u16(a); return v & 0x8000 ? v - 0x10000 : v; };
        const itemB = (i: number, f: number) =>
            u8(sym('items') + i * ITEM_SIZE + f);
        const itemW = (i: number, f: number) =>
            u16(sym('items') + i * ITEM_SIZE + f);
        const sItem = (i: number, f: number) => s16(sym('items') + i * ITEM_SIZE + f);

        // Upstream's Lara::State, which is TR's own numbering.
        const STATE_WALK = 0, STATE_RUN = 1, STATE_STOP = 2;
        const STATE_TURN_RIGHT = 6, STATE_TURN_LEFT = 7, STATE_TURN_FAST = 20;
        const LARA_ANIM_RUN = 0, LARA_ANIM_STAND_NORMAL = 103;

        const pad = emulator.getController() as unknown as { buttonState: number };
        const run = (fields: number, buttons: number) => {
            pad.buttonState = buttons;
            for (let i = 0; i < fields; i++) emulator.executeFrames(1);
            pad.buttonState = 0;
        };

        run(200, 0);

        // She starts standing. Her model's animation zero is ANIM_RUN --
        // upstream's own enum -- so an entity left on its first animation
        // starts her running on the spot; the level load sets ANIM_STAND.
        console.log(`[openlara] lara: rest state ${itemB(0, ITEM.state)} ` +
                    `anim ${itemW(0, ITEM.animIndex)} ` +
                    `hSpeed ${sItem(0, ITEM.hSpeed)}`);
        expect(itemB(0, ITEM.state), 'she should be standing still').toBe(STATE_STOP);
        expect(itemW(0, ITEM.animIndex), 'and not on ANIM_RUN')
            .not.toBe(LARA_ANIM_RUN);
        // The level load puts her on ANIM_STAND, which is 11; TR's own
        // nextAnim chain then leads to ANIM_STAND_NORMAL and settles there,
        // which is where upstream's Lara stands too. Asserting the animation
        // the chain *arrives* at is asserting that the chain ran.
        expect(itemW(0, ITEM.animIndex), 'the stand chain did not settle where TR does')
            .toBe(LARA_ANIM_STAND_NORMAL);

        // Holding up takes her through the state graph into a run: STOP names
        // RUN as a goal, and the transition table is what grants it. Nothing
        // sets her speed -- the run animation carries it, which is upstream's
        // whole movement model and the reason this test exists.
        const z0 = sItem(0, ITEM.z);
        let sawRun = false, bestSpeed = 0;
        for (let n = 0; n < 12; n++) {
            run(100, PAD_UP);
            if (itemB(0, ITEM.state) === STATE_RUN) sawRun = true;
            bestSpeed = Math.max(bestSpeed, Math.abs(sItem(0, ITEM.hSpeed)));
        }
        const z1 = sItem(0, ITEM.z);

        console.log(`[openlara] lara: running state ${itemB(0, ITEM.state)} ` +
                    `anim ${itemW(0, ITEM.animIndex)} ` +
                    `fastest hSpeed ${bestSpeed}, z ${z0} -> ${z1}`);

        expect(sawRun, 'holding up never got her into STATE_RUN').toBe(true);
        // TR's run animation tops out at 50 units a tick. Nothing here says
        // so; the number comes out of the level file through ItemObj::move.
        expect(bestSpeed, 'her speed should come from the run animation')
            .toBeGreaterThan(20);
        expect(bestSpeed, 'and should be TR\'s, not something invented')
            .toBeLessThanOrEqual(60);
        expect(z1 - z0, 'she did not actually go anywhere').toBeGreaterThan(1000);

        // Letting go stops her, through the graph rather than by assignment.
        // Long, because stopping is TR's own deceleration rather than an
        // assignment: she has to reach a frame of the run that offers the
        // transition, then play the whole run-to-stop animation through.
        const trail: string[] = [];
        for (let n = 0; n < 20; n++) {
            run(100, 0);
            trail.push(`${itemB(0, ITEM.state)}/${itemW(0, ITEM.animIndex)}` +
                       `.${itemB(0, ITEM.frameIndex)}g${itemB(0, ITEM.goalState)}`);
            if (itemB(0, ITEM.state) === STATE_STOP) break;
        }
        console.log(`[openlara] lara: stopping ${trail.join(' ')}`);
        expect(itemB(0, ITEM.state), 'she never stopped').toBe(STATE_STOP);

        // And turning is its own pair of states, with the fast turn reached by
        // holding one long enough for turnSpeed to build past LARA_TURN_SLOW.
        const a0 = itemW(0, ITEM.angleY);
        let sawTurn = false, sawTurnFast = false;
        pad.buttonState = PAD_LEFT;
        for (let n = 0; n < 1000; n++) {
            emulator.executeFrames(1);
            const st = itemB(0, ITEM.state);
            if (st === STATE_TURN_LEFT) sawTurn = true;
            if (st === STATE_TURN_FAST) sawTurnFast = true;
        }
        pad.buttonState = 0;
        const a1 = itemW(0, ITEM.angleY);

        console.log(`[openlara] lara: turning saw TURN_LEFT ${sawTurn}, ` +
                    `TURN_FAST ${sawTurnFast}, angle ${a0} -> ${a1}, ` +
                    `turnSpeed ${s16(sym('olLaraTurn'))}, ` +
                    `state ${itemB(0, ITEM.state)} goal ${itemB(0, ITEM.goalState)} ` +
                    `anim ${itemW(0, ITEM.animIndex)}.${itemB(0, ITEM.frameIndex)}`);

        expect(sawTurn, 'holding left never got her into STATE_TURN_LEFT').toBe(true);
        expect(a1, 'and never turned her').not.toBe(a0);
        // Upstream builds turnSpeed by LARA_TURN_ACCEL a step and breaks into
        // the fast turn once it passes LARA_TURN_SLOW.
        expect(sawTurnFast, 'and never broke into the fast turn').toBe(true);
    }, 600_000);
});

describe('OpenLara SNES/Super FX: Lara moveset', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('backs, sidesteps, rolls, and jumps in the requested direction', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const s16 = (a: number) => {
            const v = u16(a);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const w8 = (a: number, v: number) => memory.write(a, v & 0xFF);
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };
        const itemAddr = (f: number) => sym('items') + f;
        const state = () => u8(itemAddr(ITEM.state));
        const x = () => s16(itemAddr(ITEM.x));
        const z = () => s16(itemAddr(ITEM.z));

        const pad = emulator.getController() as unknown as { buttonState: number };
        const advance = (count: number, buttons: number,
                         sample?: () => void) => {
            let tick = u16(sym('ol_ticks'));
            let updates = 0;
            pad.buttonState = buttons;
            for (let fields = 0; fields < count * 180 + 180 && updates < count; fields++) {
                emulator.executeFrames(1);
                const next = u16(sym('ol_ticks'));
                if (next !== tick) {
                    tick = next;
                    updates++;
                    if (sample) sample();
                }
            }
            pad.buttonState = 0;
            expect(updates, `only ${updates}/${count} game updates completed`)
                .toBe(count);
        };

        const X0 = 9728, Y0 = 3072, Z0 = 4608;
        const STATE_STOP = 2, ANIM_STAND_NORMAL = 103;

        const putAtRest = () => {
            w8(itemAddr(ITEM.room), 0);
            w8(itemAddr(ITEM.state), STATE_STOP);
            w8(itemAddr(ITEM.goalState), STATE_STOP);
            w16(itemAddr(ITEM.x), X0);
            w16(itemAddr(ITEM.y), Y0);
            w16(itemAddr(ITEM.z), Z0);
            w16(itemAddr(ITEM.angleY), 0);
            w16(itemAddr(ITEM.animIndex), ANIM_STAND_NORMAL);
            w8(itemAddr(ITEM.frameIndex), 0);
            w8(itemAddr(ITEM.flags2), 0);
            w16(itemAddr(ITEM.vSpeed), 0);
            w16(itemAddr(ITEM.hSpeed), 0);
            w16(itemAddr(ITEM.health), 1000);
            w16(sym('olLaraTurn'), 0);
            w16(sym('olLaraMoveAngle'), 0);
        };

        // Let the first GSU frame stage the level, then consume any input that
        // was latched before a reset. Room 0's (9728,3072,4608) sector and all
        // four neighbours are flat, so every direction below is genuinely
        // available rather than forced through a wall by the test.
        for (let i = 0; i < 200; i++) emulator.executeFrames(1);
        expect(u16(sym('itemsCount')), 'the level never reached the host')
            .toBeGreaterThan(0);

        const reset = () => {
            pad.buttonState = 0;
            putAtRest();
            advance(2, 0);
            putAtRest();
            advance(2, 0);
        };

        const drive = (buttons: number, wanted: number, count: number) => {
            let saw = false;
            const startX = x(), startZ = z();
            let minX = startX, maxX = startX;
            let minZ = startZ, maxZ = startZ;
            const trail: string[] = [];
            advance(count, buttons, () => {
                if (state() === wanted) saw = true;
                minX = Math.min(minX, x()); maxX = Math.max(maxX, x());
                minZ = Math.min(minZ, z()); maxZ = Math.max(maxZ, z());
                trail.push(`${state()}/${u16(itemAddr(ITEM.animIndex))}.` +
                           `${u8(itemAddr(ITEM.frameIndex))}:` +
                           `${s16(itemAddr(ITEM.hSpeed))}@` +
                           `${u16(sym('olLaraMoveAngle')).toString(16)}=` +
                           `${x()},${z()}`);
            });
            expect(saw, `never reached Lara state ${wanted}: ${trail.join(' ')}`)
                .toBe(true);
            return { startX, startZ, minX, maxX, minZ, maxZ,
                     trail: trail.join(' ') };
        };

        reset();
        let m = drive(PAD_L | PAD_RIGHT, 21, 4);
        expect(m.maxX, 'L+Right sidestep moved in the wrong direction')
            .toBeGreaterThan(m.startX);
        expect(u16(sym('olLaraMoveAngle')), 'right moveAngle should be +90')
            .toBe(0x4000);

        reset();
        m = drive(PAD_L | PAD_LEFT, 22, 4);
        expect(m.minX, 'L+Left sidestep moved in the wrong direction')
            .toBeLessThan(m.startX);
        expect(u16(sym('olLaraMoveAngle')), 'left moveAngle should be -90')
            .toBe(0xC000);

        reset();
        m = drive(PAD_L | PAD_DOWN, 16, 5);
        expect(m.minZ, `L+Down back-walk moved Lara forwards: ${m.trail}`)
            .toBeLessThan(m.startZ);

        reset();
        m = drive(PAD_DOWN, 5, 6);
        expect(m.minZ, 'Down back-hop moved Lara forwards')
            .toBeLessThan(m.startZ);

        reset();
        // STATE_ROLL_START is assigned by animSet and may transition to
        // STATE_ROLL_END again inside the same C update. Sampling it at a
        // field boundary made the oracle depend on GSU render time, so assert
        // the stable second half of the roll instead: anim 146 is ten ticks,
        // entered at frame 2, and anim 147 after it carries state 23. Twelve
        // ticks reach it with margin (three did when a tick was fifteen).
        // A real pad cannot hold Up and Down at once: SNES X is upstream's
        // IN_UP | IN_DOWN, as a face button is on its NDS (main.asm PadMap).
        m = drive(PAD_X, 23, 12);
        expect(m.maxZ, 'X did not roll her forward')
            .toBeGreaterThan(m.startZ);

        reset();
        m = drive(PAD_A | PAD_LEFT, 27, 14);
        expect(m.minX, 'A+Left jump did not launch left')
            .toBeLessThan(m.startX);

        reset();
        m = drive(PAD_A | PAD_RIGHT, 26, 14);
        expect(m.maxX, 'A+Right jump did not launch right')
            .toBeGreaterThan(m.startX);

        reset();
        m = drive(PAD_A | PAD_DOWN, 25, 14);
        expect(m.minZ, 'A+Down jump did not launch backwards')
            .toBeLessThan(m.startZ);

        console.log('[openlara] moveset: back, back-hop, sidesteps, roll, ' +
                    'and three directional jumps all followed moveAngle');
    }, 600_000);
});

// Execute the compiled game routines at call boundaries. This isolates exact
// state/threshold rules from field sampling, with the cartridge's real TR1
// animations and 816-tcc integer widths. The return PC and scratch data stay
// below the relocated code at $0600; the host loop is not running here.
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
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 500000) {
            if ((cpu.PB << 16 | cpu.PC) === watch) observe!.run(cpu.S);
            cpu.step();
        }
        expect(steps, `${name} never returned`).toBeLessThan(500000);
        expect([cpu.S, cpu.DB, cpu.D, cpu.E, cpu.P & 0x3C], `${name} corrupted the C ABI`)
            .toEqual([0x1EE3, 0x7E, 0, 0, 4]);
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
        const maps = input.length === 2 ? [input[0], input[0], input[1], input[0], input[0], input[1]] : input;
        expect(maps.length).toBe(6);
        maps.forEach(map => expect(map.length).toBe(u16(sym('olBoxCount'))));
        if (!FX3) {
            maps[0].concat(maps[2]).forEach((v, i) => w16(table('Zone') + i * 2, v));
            return;
        }
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
        else if (symbols.has('navInvalidateSectorsGsu2')) call('navInvalidateSectorsGsu2');
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

describe.skipIf(!FX3)('OpenLara FX3: floor geometry', () => {
    it.skipIf(!SYMS.has('Room_waterCold'))('matches dry-water shortcuts and near query stores against their far and C paths', () => {
        const g = movementCartridge();
        const bytes = (at: number, n: number) => Array.from({ length: n }, (_, i) => g.memory.read(at + i));
        let waterCases = 0, queryCases = 0, fast = 0;
        const outputs = [[0x001F80, 0x001F82], [0x7E1F80, 0x7E1F82],
            [0x7F9000, 0x7F9002], [0x7E1200, 0x7E1200]];
        for (const layout of ['dry', 'water', 'stacked', 'empty-x', 'empty-z']) {
            g.set({ olRoomCount: 3, olSectorCount: 192 });
            for (let room = 0; room < 3; room++) {
                g.room(room, layout === 'water' || room === 2);
                for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++)
                    g.sector(room, x, z, 2048, -2048, layout === 'stacked' && room < 2 ? room + 1 : 0);
            }
            g.w8(g.table('Link') + 8, 1); g.w8(g.table('Link') + 16, 2);
            if (layout === 'empty-x') g.w8(g.table('Room') + 8, 0);
            if (layout === 'empty-z') g.w8(g.table('Room') + 9, 0);
            for (const room of [0, 1, 2, 3, 65535]) for (const x of [-32768, -1, 0, 1023, 2560, 8192, 32767]) {
                for (const [level, depth] of outputs) {
                    const args = [room, x, x ^ 0x7FFF, level & 65535, level >>> 16, depth & 65535, depth >>> 16];
                    const run = (name: string) => {
                        g.call('Room_reset'); g.w16(level, 0x1234); g.w16(depth, 0x5678);
                        g.call(name, args, { name: 'RW_Dry', run: () => { fast++; } });
                        return [g.u16(level), g.u16(depth)];
                    };
                    const expected = run('Room_waterCold');
                    expect(run('Room_water'), `${layout} ${room} ${x} ${level.toString(16)}`).toEqual(expected);
                    waterCases++;
                }
            }
        }
        expect(fast).toBeGreaterThan(0);
        g.room(0, false); g.set({ olRoomCount: 1 });
        for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++) g.sector(0, x, z, 2048, -2048);
        for (const out of [0x001F80, 0x7E1200, 0x7E1F80, 0x7E1FEC, 0x7FFFE0, 0x7EFFEC, 0x7EFFF0])
        for (const x of [-32768, -1, 0, 1023, 4096, 8192, 32767]) {
            // Bank $7F always takes the retained far-store path. Sentinel
            // bytes prove that the fast path writes exactly twenty bytes.
            for (let sx = 0; sx < 8; sx++) for (let sz = 0; sz < 8; sz++)
                g.sector(0, sx, sz, 2048, -2048);
            g.w8(out - 1, 0xA5); g.w8(out + 20, 0x5A);
            g.call('Room_reset');
            g.call('Room_query', [0, x, 1024, x ^ 0x7FFF, 0x9000, 0x7F]);
            const expected = bytes(0x7F9000, 20);
            g.call('Room_reset');
            g.call('Room_query', [0, x, 1024, x ^ 0x7FFF, out & 65535, out >>> 16]);
            expect(bytes(out, 20), `query output ${out.toString(16)}`).toEqual(expected);
            expect([g.memory.read(out - 1), g.memory.read(out + 20)]).toEqual([0xA5, 0x5A]);
            queryCases++;
        }
        console.log(`[openlara] room outputs: ${waterCases} dry/water/stacked/empty/far/aliased C matches, ` +
            `${fast} native dry returns, ${queryCases} exact near/far/bank-crossing queries and canaries`);
    });

    it('preserves every original signed-byte slope, vertical link and portal', () => {
        const g = movementCartridge(); g.restoreLevel();
        type Sector = [number, number, number, number, number];
        const audit: { floors: number[]; rooms: { sectors: Sector[] }[] } =
            JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8'));
        let index = 0, wide = 0;
        const signed = (v: number) => (v & 128) ? (v & 255) - 256 : v & 255;
        // Level initialization lets movable blocks edit the staged floor.
        // Audit the immutable source descriptor, not those gameplay writes.
        const dir = g.u16(g.symbol('olLevel')) << 16;
        const source = dir + (g.u16(dir + 0x34) | g.u16(dir + 0x36) << 16);
        for (const room of audit.rooms) for (const [floor, ceiling, below, above, floorIndex] of room.sectors) {
            const sector = source + index * 4;
            const link = g.table('Link') + g.u16(sector + 2) * 8;
            let p = floorIndex, fs = 0, cs = 0, portal = 255;
            if (p > 0 && p < audit.floors.length) {
                let cmd = audit.floors[p++];
                if ((cmd & 31) === 2) {
                    fs = audit.floors[p++];
                    cmd = cmd & 0x8000 ? 0 : audit.floors[p++];
                }
                if ((cmd & 31) === 3) {
                    cs = audit.floors[p++];
                    cmd = cmd & 0x8000 ? 0 : audit.floors[p++];
                }
                if ((cmd & 31) === 1) portal = audit.floors[p];
            }
            expect([signed(g.memory.read(sector)), signed(g.memory.read(sector + 1)),
                g.memory.read(link), g.memory.read(link + 1), g.memory.read(link + 2),
                g.u16(link + 4), g.u16(link + 6)], `original sector ${index}`)
                .toEqual([floor, ceiling, below, above, portal, fs, cs]);
            for (const value of [signed(fs), signed(fs >>> 8), signed(cs), signed(cs >>> 8)])
                if (value < -16 || value > 15) wide++;
            index++;
        }
        expect(index).toBe(g.u16(g.symbol('olSectorCount')));
        if (LEVEL === 'LEVEL3A') expect(wide, 'Lost Valley no longer exercises its wide slopes').toBeGreaterThan(0);
        console.log(`[openlara] original floor geometry: ${index} exact sectors, ${wide} slopes outside signed five-bit range`);
    });

    it('evaluates the full signed-byte slope range without truncating 17-bit products', () => {
        const g = movementCartridge(), link = g.table('Link');
        const word = (v: number) => (v << 16) >> 16;
        const signed = (v: number) => (v << 24) >> 24;
        const points = [0, 1, 2, 255, 256, 511, 512, 767, 768, 1022, 1023];
        let cases = 0, overflows = 0;
        for (let value = -128; value <= 127; value++) {
            const slopes = [value, signed(value * 73 + 19), signed(value * 19 + 41), signed(value * 37 + 83)];
            slopes.forEach((s, i) => g.w8(link + 4 + i, s));
            g.call('Room_reset');
            for (const [i, dx] of points.entries()) {
                const dz = points[(i * 7 + 3) % points.length];
                const [sx, sz, cx, cz] = slopes;
                // Direct transcription of Sector::getFloor/getCeiling's
                // signed 32-bit expressions, before the SNES height wrap.
                const products = [sx * (sx < 0 ? dx : dx - 1023),
                    sz * (sz < 0 ? dz : dz - 1023), cx * (cx < 0 ? dx - 1023 : dx),
                    cz * (cz < 0 ? dz : dz - 1023)];
                if (products.some(p => Math.abs(p) > 65535)) overflows++;
                const floor = word(2048 - Math.floor(products[0] / 4) - Math.floor(products[1] / 4));
                const ceiling = word(-2048 - Math.floor(products[2] / 4) + Math.floor(products[3] / 4));
                // Cold then cached point answers, with mirror and far-bank
                // output pointers. Neither canaries nor any output padding
                // may be overwritten by an intermediate wide product.
                for (const out of [0x7E0520, 0x0520, 0x7F9000]) {
                    for (let b = -2; b < 22; b++) g.w8(out + b, 0xA5);
                    g.call('Room_query', [0, 2048 + dx, 0, 2048 + dz, out & 65535, out >>> 16]);
                    expect(g.u16(g.symbol('tcc__r0'))).toBe(1);
                    expect(Array.from({ length: 10 }, (_, f) => g.s16(out + f * 2)), `slope ${value}, point ${dx}/${dz}`)
                        .toEqual([0, floor, ceiling, 2048, 2048, 255, ...slopes]);
                    expect([g.u16(out - 2), g.u16(out + 20)]).toEqual([0xA5A5, 0xA5A5]);
                    cases++;
                }
            }
        }
        expect(overflows).toBeGreaterThan(100);
        console.log(`[openlara] full-byte slopes: ${cases} exact point/cache/far-pointer answers, ${overflows} 17-bit product cases`);
    });
});

// Controller-only fixtures explicitly pin a target. FX3's original mood
// layer otherwise samples aggression and keeps an older waypoint; immediate
// retargeting is not a valid assumption for a steering/vertical-motion test.
function pinCreatureTarget(g: ReturnType<typeof movementCartridge>, item: number) {
    if (!FX3) return;
    const owner = (item - g.item) / ITEM_SIZE;
    const turn = g.memory.read(g.symbol('olEnemyTurn') + owner);
    g.call('Enemy_navAcquire', [item & 65535, item >>> 16, 1]);
    const nav = g.u16(g.symbol('tcc__r0')) | g.u16(g.symbol('tcc__r0h')) << 16;
    expect(nav).toBeGreaterThanOrEqual(0x7E5800);
    g.w8(g.symbol('olEnemyTurn') + owner, turn);
    g.call('navBoxAt', [g.memory.read(g.item + ITEM.room), g.s16(g.item + ITEM.x), g.s16(g.item + ITEM.z)]);
    const box = g.u16(g.symbol('tcc__r0'));
    g.w16(nav + 8198, box); g.w16(nav + 8200, box);
    const head = g.memory.read(item + ITEM.type) === 9 ?
        g.s16(g.table('AnimBox') + g.u16(g.item + ITEM.animIndex) * 12 + 4) : 0;
    g.call('Nav_point', [(nav + 8216) & 65535, 0x7E, g.memory.read(g.item + ITEM.room),
        g.s16(g.item + ITEM.x), g.s16(g.item + ITEM.y) + head, g.s16(g.item + ITEM.z)]);
    g.w8(item + ITEM.mood, 2);
}

describe('OpenLara SNES/Super FX: navigation sectors', () => {
    it.skipIf(!FX3)('rebinds host tables across banks and CPU-only FX3 ROM, staging only mutable sectors', () => {
        const g = movementCartridge(); g.restoreLevel();
        // The fixture builds its directory at $E00000 whatever bank the level
        // keeps its own in (a multi-level cartridge's are higher), and points
        // olLevel's host bank there, as SelectLevel would for such a level.
        const directory = 0xE00000, level = g.u16(g.symbol('olLevel')) << 16;
        const w32 = (a: number, v: number) => { g.w16(a, v); g.w16(a + 2, v >>> 16); };
        const read = (a: number, n: number) => Buffer.from(Array.from({ length: n }, (_, i) => g.memory.read(a + i)));
        const source = HOST_TABLES.map((name, i) => {
            const offset = g.u16(level + 40 + i * 12) | g.u16(level + 42 + i * 12) << 16;
            const length = g.u16(level + 44 + i * 12) | g.u16(level + 46 + i * 12) << 16;
            return { name, descriptor: directory + 40 + i * 12, data: read(level + offset, length) };
        });
        read(level, 256).forEach((v, k) => g.w8(directory + k, v));
        g.w16(g.symbol('olLevel'), directory >>> 16);
        const originalItems = read(g.item, g.count('items') * ITEM_SIZE);
        const install = (endAligned: boolean) => {
            let end = 0;
            source.forEach(({ descriptor, data }, i) => {
                // E1..F2: STATIC_FIRST/AIM/NAV deliberately lie beyond the
                // FX's 3 MiB window, but inside the CPU's 4 MiB window.
                const low = endAligned ? ((65536 - data.length) & ~1) : 0;
                const offset = (i + 1) * 65536 + low;
                data.forEach((v, k) => g.w8(directory + offset + k, v));
                // A word reader may mask off the byte just after its table.
                g.w8(directory + offset + data.length, 0xA5);
                w32(descriptor, offset);
                end = offset + data.length;
            });
            w32(directory + 8, end);
            g.call('olHostBind');
            expect(g.u16(g.symbol('olHostTablesReady'))).toBe(1);
            source.forEach(({ name, descriptor, data }) => {
                const offset = g.u16(descriptor) | g.u16(descriptor + 2) << 16;
                const address = name === 'Sector' ? 0x7F0000 : directory + offset;
                expect(g.table(name), name).toBe(address);
                expect(read(address, data.length), name).toEqual(data);
                if (name !== 'Sector' && data.length) {
                    g.memory.write(address, data[0] ^ 255);
                    expect(g.memory.read(address), `${name} accepted a CPU ROM write`).toBe(data[0]);
                }
            });
            g.call('olLevelInit');
            expect(read(g.item, originalItems.length)).toEqual(originalItems);
            g.call('Room_reset');
        };
        const points = JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8')).rooms as
            { x: number; z: number; width: number; depth: number; boxes: number[] }[];
        for (const endAligned of [false, true]) {
            install(endAligned);
            let sector = 0;
            points.forEach((room, r) => room.boxes.forEach((box, i) => {
                g.call('navSectorBox', [sector++, room.depth]);
                expect(g.u16(g.symbol('tcc__r0'))).toBe(box);
                if (i === 0) {
                    g.call('Room_sectorAt', [r, room.x + 512, room.z + 512]);
                    expect(g.u16(g.symbol('tcc__r0'))).toBe(sector - 1);
                }
            }));
            // Both complete native and C consumers run against the rebinding.
            g.call('ItemObj_getBoundingBox', [...g.args, 0x520, 0x7E]);
            expect(read(0x7E0520, 12)).toEqual(read(g.table('AnimBox') + g.u16(g.item + ITEM.animIndex) * 12, 12));
            g.call('gameRenderSync');
            g.call('Weapon_getAimPoint', [...g.args, 0x540, 0x7E]);
        }
        // Movable floors live in WRAM, without changing the ROM source.
        const sectorROM = directory + (g.u16(directory + 52) | g.u16(directory + 54) << 16);
        const floor = g.memory.read(sectorROM);
        g.memory.write(g.table('Sector'), floor ^ 255);
        expect(g.memory.read(g.table('Sector'))).toBe(floor ^ 255);
        expect(g.memory.read(sectorROM)).toBe(floor);

        // DMA count $0000 means 64 KiB. An empty table must skip it.
        install(false);
        for (let i = 0; i < 65536; i++) g.w8(0xE20000 + i, (i ^ (i >>> 8) ^ 0xA7) & 255);
        w32(directory + 56, 65536); g.w16(directory + 34, 16384);
        g.call('olHostBind');
        expect(g.u16(g.symbol('olHostTablesReady'))).toBe(1);
        expect(read(0x7F0000, 65536)).toEqual(read(0xE20000, 65536));
        w32(directory + 56, 0); g.w16(directory + 34, 0);
        g.memory.write(0x7F0000, 0x5A); g.memory.write(0x7FFFFF, 0xA5);
        g.call('olHostBind');
        expect([g.memory.read(0x7F0000), g.memory.read(0x7FFFFF)]).toEqual([0x5A, 0xA5]);

        for (const [address, value, wide] of [[directory, 0, false],
            [directory + 4, 2, false], [directory + 4, 3, false],
            [directory + 4, 4, false], [directory + 4, 6, false], [directory + 8, 255, true],
            [directory + 8, 0x200001, true], [directory + 40, 255, true],
            [directory + 40, 0x200000, true], [directory + 48, 1, false],
            [directory + 50, 1, false], [directory + 56, 65537, true],
            [directory + 52, 0x2FFFF, true]]) {
            const before = read(address, wide ? 4 : 2);
            if (wide) w32(address, value); else g.w16(address, value);
            g.call('olHostBind');
            expect(g.u16(g.symbol('olHostTablesReady')), `accepted malformed descriptor at ${address.toString(16)}`).toBe(0);
            before.forEach((v, k) => g.w8(address + k, v));
        }
        console.log(`[openlara] FX3 host tables: ${source.length} independent banks, CPU-only F0..F2, ` +
            'bank-start/end readers, full/empty sector DMA and malformed-directory rejection');
    });

    it('matches every original sector box, room identity and wrapped point coordinate', () => {
        const g = movementCartridge(); g.restoreLevel();
        const ref = process.env.OL_REFERENCE_ROM ? movementCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS ?? '')) : undefined;
        ref?.restoreLevel();
        const audit: { rooms: { x: number; z: number; width: number; depth: number; boxes: number[] }[] } =
            JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8'));
        let sector = 0, calls = 0, steps = 0, pointSteps = 0, referenceSteps = 0;
        const tags = new Set<number>(), points = new Map<string, Set<number>>();
        const firstRoom = new Map<string, { r: number; box: number }>();
        let alternations = 0;
        const nav = g.table('NavMap');
        const check = (name: string, args: number[], expected: number, context: string) => {
            const n = g.call(name, args); steps += n; calls++;
            expect(g.u16(g.symbol('tcc__r0')), context).toBe(expected);
            if (name === 'navBoxAt' || name === 'Room_sectorAt') {
                pointSteps += n;
                if (ref) {
                    referenceSteps += ref.call(name, args);
                    expect(ref.u16(ref.symbol('tcc__r0')), `C ${context}`).toBe(expected);
                }
            }
        };
        audit.rooms.forEach((room, r) => {
            const start = sector;
            room.boxes.forEach((box, s) => {
                tags.add(g.memory.read(nav + 2 + (sector >>> 2)) >>> ((sector & 3) * 2) & 3);
                check('navSectorBox', [sector++, room.depth], box, `room ${r} sector ${s}`);
                const x = room.x + Math.floor(s / room.depth) * 1024 + 512;
                const z = room.z + (s % room.depth) * 1024 + 512;
                check('navBoxAt', [r, x, z], box, `room ${r} point ${x},${z}`);
                const key = `${x},${z}`, ids = points.get(key) ?? new Set<number>();
                ids.add(box); points.set(key, ids);
                const other = firstRoom.get(key);
                if (!other) firstRoom.set(key, { r, box });
                else if (other.box !== box && alternations < 32) {
                    for (const sample of [other, { r, box }, other, { r, box }])
                        check('navBoxAt', [sample.r, x, z], sample.box, `same point, room changed to ${sample.r}`);
                    alternations++;
                }
            });
            // Exact boundaries, either side of the signed-coordinate seam,
            // and the room-edge clamping contract shared with collision.
            for (const [x, z] of [[-1, -1], [0, 0], [1023, 1023], [1024, 1024],
                [room.width * 1024 - 1, room.depth * 1024 - 1],
                [room.width * 1024, room.depth * 1024]]) {
                const sx = Math.min(room.width - 1, Math.max(0, x >> 10));
                const sz = Math.min(room.depth - 1, Math.max(0, z >> 10));
                check('Room_sectorAt', [r, room.x + x, room.z + z], start + sx * room.depth + sz,
                    `room ${r} sector boundary ${x},${z}`);
                check('navBoxAt', [r, room.x + x, room.z + z], room.boxes[sx * room.depth + sz],
                    `room ${r} boundary ${x},${z}, start=${start}`);
            }
        });
        expect(sector).toBe(g.u16(g.symbol('olSectorCount')));
        const bounds: number[][] = JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8')).boxBounds;
        expect(bounds.length).toBe(g.u16(g.symbol('olBoxCount')));
        const boxBase = g.table('Box');
        bounds.forEach((b, i) => {
            const at = boxBase + i * 8;
            expect([0,1,2,3].map(k => g.memory.read(at + k)).concat([g.s16(at + 4), g.u16(at + 6)]),
                `original navigation bounds ${i}`).toEqual([b[0] >> 10, (b[1] + 1) >> 10,
                    b[2] >> 10, (b[3] + 1) >> 10, b[4], b[5] & 65535]);
            expect(g.memory.read(at + 1)).toBeGreaterThan(g.memory.read(at));
            expect(g.memory.read(at + 3)).toBeGreaterThan(g.memory.read(at + 2));
        });
        expect([...tags].sort()).toEqual([0, 1, 2, 3]);
        const ambiguous = [...points.values()].filter(ids => ids.size > 1).length;
        expect(ambiguous, 'fixture lacks overlapping room geometry').toBeGreaterThan(0);
        expect(alternations, 'room cache was not exercised across stacked rooms').toBe(32);
        for (const room of [audit.rooms.length, 255, 65535])
            check('navBoxAt', [room, 0, 0], 65535, 'invalid room');
        check('navSectorBox', [sector, 8], 65535, 'invalid sector');
        check('navSectorBox', [0, 0], 65535, 'empty room depth');
        const resize = (width: number, depth: number) => {
            for (const cart of [g, ref]) {
                if (!cart) continue;
                const room = cart.table('Room');
                cart.w8(room + 8, width); cart.w8(room + 9, depth); cart.call('Room_reset');
            }
        };
        resize(0x80, 8);
        check('navBoxAt', [0, 0, 0], 65535, 'zero-width water room');
        resize(8, 0);
        check('navBoxAt', [0, 0, 0], 65535, 'zero-depth room');
        console.log(`[openlara] navigation: ${sector} original sectors, ${ambiguous} ambiguous X/Z positions, ` +
            `${calls} native/point lookups, ${(steps / calls).toFixed(0)} mean instructions` +
            (ref ? `; point lookup ${(pointSteps / referenceSteps * 100).toFixed(1)}% of C instructions` : ''));
    });

    it('decodes every literal width, rank boundary, NO_BOX hole and wide bit offset', () => {
        const g = movementCartridge(); g.set({ olBoxCount: 1024 });
        let cases = 0;
        for (let bits = 1; bits <= 10; bits++) {
            const boxes = Array.from({ length: 259 }, (_, i) => i % 9 === 0 ? 65535 : (i * 67 + 13) & ((1 << bits) - 1));
            g.navMap(boxes, bits);
            boxes.forEach((box, i) => {
                g.call('navSectorBox', [i, 7]); cases++;
                expect(g.u16(g.symbol('tcc__r0')), `${bits}-bit literal ${i}`).toBe(box);
            });
        }
        // More than 65536 literal bits: exercise carry before bit->byte
        // conversion without increasing the real cartridge's host blob.
        const boxes = Array.from({ length: 6801 }, (_, i) => (i * 37 + 1023) & 1023);
        g.navMap(boxes, 10, 0x200);
        for (let i = 6520; i < boxes.length; i++) {
            g.call('navSectorBox', [i, 7]); cases++;
            expect(g.u16(g.symbol('tcc__r0')), `wide literal bit offset ${i * 10}`).toBe(boxes[i]);
        }
        for (const [magic, bits] of [[0, 10], [0x4E, 0], [0x4E, 11]]) {
            g.w8(g.blob + 0x200, magic); g.w8(g.blob + 0x201, bits);
            g.call('navSectorBox', [0, 7]);
            expect(g.u16(g.symbol('tcc__r0')), 'invalid codec header').toBe(65535);
        }
        // Resetting entries without invalidating their source used to make
        // a later request for the same target reuse an empty flood.
        if (!FX3) {
            g.w16(g.symbol('navFrom'), 0); g.w16(g.symbol('navFrom') + 2, 1);
            g.set({ navReached: 17 }); g.w16(g.symbol('navNext') + 2, 0);
            g.call('navResetCache');
            expect([g.u16(g.symbol('navFrom')), g.u16(g.symbol('navFrom') + 2),
                g.u16(g.symbol('navReached')), g.u16(g.symbol('navNext') + 2)])
                .toEqual([65535, 65535, 0, 65535]);
        }
        console.log(`[openlara] navigation codec: ${cases} 1..10-bit/rank/hole/wide-offset cases`);
    });
});

describe.skipIf(!FX3)('OpenLara FX3: per-creature navigation', () => {
    const NAV_SIZE = 8240, CELLS = 8192;
    const N = { count: CELLS, zone: CELLS + 2, weight: CELLS + 4, end: CELLS + 6,
        target: CELLS + 8, head: CELLS + 10, tail: CELLS + 12, step: CELLS + 14,
        drop: CELLS + 16, speed: CELLS + 18, mask: CELLS + 20, owner: CELLS + 22,
        pos: CELLS + 24, waypoint: CELLS + 36 };

    it('preserves every sign-extension and wide-shift value, register and flag in the compiler rewrites', () => {
        // Assemble the actual before/after fragments retained by this build,
        // not a hand-transcribed alternative implementation of the rewrite.
        const root = dirname(ROM);
        const slowSource = readFileSync(join(root, 'c_wolf.raw'), 'utf8')
            .match(/ldy(?:\.w)? #15\n-\ncmp #\$8000\nror a\ndey\nbne -\n\+\n/)?.[0];
        const fastSource = readFileSync(join(root, 'c_wolf.asm'), 'utf8')
            .match(/-\ntay\nand #\$8000\nbeq tccs_\{WLA_FILENAME\}_signext_\d+_positive\n[\s\S]*?\n\+\n/)?.[0];
        expect(slowSource, 'missing original sign-extension idiom').toBeDefined();
        expect(fastSource, 'the native build did not apply the rewrite').toBeDefined();
        const assemble = (source: string) => {
            const labels = new Map<string, number>(), fixups: [number, string][] = [], code: number[] = [];
            const simple: Record<string, number> = { tay: 0xA8, tya: 0x98, 'asl a': 0x0A,
                'ror a': 0x6A, 'lsr a': 0x4A, xba: 0xEB, pha: 0x48, pla: 0x68, dey: 0x88 };
            for (const raw of source.trim().split('\n')) {
                const line = raw.replace(/\.w\b/, '');
                if (line === '-' || line === '+' || line.endsWith(':')) {
                    labels.set(line.replace(/:$/, ''), code.length); continue;
                }
                if (simple[line] !== undefined) { code.push(simple[line]); continue; }
                const immediate = line.match(/^(ldy|lda|cmp|and|ora) #(.+)$/);
                if (immediate) {
                    const value = Number(immediate[2].replace('$', '0x'));
                    code.push(({ ldy: 0xA0, lda: 0xA9, cmp: 0xC9, and: 0x29, ora: 0x09 })[immediate[1]]!, value & 255, value >>> 8);
                    continue;
                }
                const branch = line.match(/^(beq|bne|bra|bcc) (.+)$/);
                expect(branch, `unsupported probe instruction ${line}`).not.toBeNull();
                code.push(({ beq: 0xF0, bne: 0xD0, bra: 0x80, bcc: 0x90 })[branch![1]]!, 0);
                fixups.push([code.length - 1, branch![2]]);
            }
            for (const [offset, label] of fixups) {
                expect(labels.has(label), `missing probe label ${label}`).toBe(true);
                const delta = labels.get(label)! - offset - 1;
                expect(Math.abs(delta)).toBeLessThan(128); code[offset] = delta & 255;
            }
            return code.concat(0x6B); // RTL to the fixture's unchanged caller
        };
        const symbols = new Map(SYMS).set('signSlow', 0x7E0520).set('signFast', 0x7E0580);
        const g = movementCartridge(ROM, symbols);
        const variants = [['signSlow', slowSource!], ['signFast', fastSource!]] as const;
        for (const [name, source] of variants) assemble(source).forEach((v, i) => g.w8(g.symbol(name) + i, v));
        let slowSteps = 0, fastSteps = 0;
        for (let value = 0; value < 65536; value++) {
            const overflow = value & 1 ? 0x40 : 0;
            for (const [name] of variants) {
                g.cpu.A = value; g.cpu.X = 0xBEEF;
                const steps = g.call(name, [], { name, run: () => { g.cpu.P |= overflow; } });
                if (name === 'signSlow') slowSteps += steps; else fastSteps += steps;
                expect([g.cpu.A, g.cpu.X, g.cpu.Y, g.cpu.P], `${name} input ${value}`)
                    .toEqual([value & 0x8000 ? 65535 : 0, 0xBEEF, 0, 6 | overflow | (value >>> 14 & 1)]);
            }
        }
        expect(fastSteps).toBeLessThan(slowSteps / 4);
        console.log(`[openlara] sign extension: all 65536 inputs preserve A/X/Y, stack and flags (including C/V); ` +
            `${fastSteps} instructions vs ${slowSteps}`);
        const slowShifts = new Map<string, string>(), fastShifts = new Map<string, string>();
        for (const file of readdirSync(root).filter(name => /^c_.*\.raw$/.test(name))) {
            for (const m of readFileSync(join(root, file), 'utf8').matchAll(/ldy(?:\.w)? #(9|1[0-5])\n-\n(asl|lsr) a\ndey\nbne -\n\+\n/g))
                slowShifts.set(`${m[2]}:${m[1]}`, m[0]);
            for (const m of readFileSync(join(root, file), 'utf8').matchAll(/ldy(?:\.w)? #(9|1[0-4])\n-\ncmp #\$8000\nror a\ndey\nbne -\n\+\n/g))
                slowShifts.set(`sar:${m[1]}`, m[0]);
            for (const m of readFileSync(join(root, file.replace(/\.raw$/, '.asm')), 'utf8')
                .matchAll(/-\nxba\nand #\$00FF\ncmp #\$0080\nbcc tccs_\{WLA_FILENAME\}_sar_\d+\nora #\$FF00\ntccs_\{WLA_FILENAME\}_sar_\d+:\n((?:cmp #\$8000\nror a\n){1,6})ldy #0\n\+\n/g))
                fastShifts.set(`sar:${8 + m[1].trim().split('\n').length / 2}`, m[0]);
            for (const m of readFileSync(join(root, file.replace(/\.raw$/, '.asm')), 'utf8')
                .matchAll(/-\nxba\nand #\$(FF00|00FF)\n((?:(?:asl|lsr) a\n){1,7})ldy #0\n\+\n/g)) {
                const op = m[1] === 'FF00' ? 'asl' : 'lsr';
                const count = 8 + m[2].trim().split('\n').length;
                fastShifts.set(`${op}:${count}`, m[0]);
            }
        }
        expect(fastShifts.size, 'no native wide-shift rewrites').toBeGreaterThan(0);
        let shiftSlow = 0, shiftFast = 0;
        for (const [key, fast] of fastShifts) {
            expect(slowShifts.has(key), `no original compiler idiom for ${key}`).toBe(true);
            const [op, bits] = key.split(':'), count = Number(bits);
            assemble(slowShifts.get(key)!).forEach((v, i) => g.w8(g.symbol('signSlow') + i, v));
            assemble(fast).forEach((v, i) => g.w8(g.symbol('signFast') + i, v));
            for (let value = 0; value < 65536; value++) for (const name of ['signSlow', 'signFast']) {
                g.cpu.A = value; g.cpu.X = 0xBEEF;
                const overflow = value & 1 ? 0x40 : 0;
                const steps = g.call(name, [], { name, run: () => { g.cpu.P |= overflow; } });
                if (name === 'signSlow') shiftSlow += steps; else shiftFast += steps;
                const result = op === 'asl' ? (value << count) & 65535 : op === 'sar' ?
                    ((value << 16 >> 16) >> count) & 65535 : value >>> count;
                const carry = value >>> (op === 'asl' ? 16 - count : count - 1) & 1;
                expect([g.cpu.A, g.cpu.X, g.cpu.Y, g.cpu.P], `${name} ${key} input ${value}`)
                    .toEqual([result, 0xBEEF, 0, 6 | overflow | carry]);
            }
        }
        expect(shiftFast).toBeLessThan(shiftSlow / 3);
        console.log(`[openlara] wide shifts: ${[...fastShifts.keys()].sort().join(', ')}; all 65536 inputs/flags each, ` +
            `${shiftFast} instructions vs ${shiftSlow}`);
    });

    it('matches integer products, signed corridor helpers and exact cache keys', () => {
        const g = movementCartridge();
        const word = (v: number) => [v & 65535, v >>> 16];
        const w32 = (at: number, v: number) => { g.w16(at, v); g.w16(at + 2, v >>> 16); };
        const values = [-2147483648,-2147483647,-65536,-32768,-1,0,1,32767,32768,65535,65536,2147483647];
        let products = 0;
        for (const a of values) for (const b of values) {
            for (const name of ['Nav_min', 'Nav_max']) {
                g.call(name, [...word(a), ...word(b)]);
                const result = g.u16(g.symbol('tcc__r0')) | g.u16(g.symbol('tcc__r1')) << 16;
                expect(result).toBe(name === 'Nav_min' ? Math.min(a, b) : Math.max(a, b));
            }
            for (const v of values) {
                g.call('Nav_clamp', [...word(v), ...word(a), ...word(b)]);
                expect(g.u16(g.symbol('tcc__r0')) | g.u16(g.symbol('tcc__r1')) << 16)
                    .toBe(Math.min(Math.max(v, a), b));
            }
        }
        const boundSets = [[-2147483648,2147483647,-2147483648,2147483647],
            [-32768,32767,-65536,65536], [0,0,0,0], [65535,65536,32767,32768],
            [7,-7,-9,9], [-9,9,7,-7]];
        for (const bounds of boundSets) {
            bounds.forEach((v, i) => w32(g.symbol('olNavBounds') + i * 4, v));
            for (const x of values) for (const z of values) {
                w32(g.symbol('olNavClipFrom'), x); w32(g.symbol('olNavClipFrom') + 8, z);
                g.call('Nav_inside');
                expect(g.u16(g.symbol('tcc__r0')), `inside ${x},${z} / ${bounds}`)
                    .toBe(Number(x >= bounds[0] && x <= bounds[1] && z >= bounds[2] && z <= bounds[3]));
            }
        }
        // Call the final native steering stage with its real caller layout.
        // Both axes must scale together; -32768 is outside the accepted
        // [-32767,+32767] range too. Include full signed-32-bit extremes.
        const directions = [...values, -32767, -32769, 32769];
        for (const dx of directions) for (const dz of directions) {
            w32(g.symbol('tcc__r2'), dx); w32(g.symbol('tcc__r3'), dz);
            g.call('ET_Fits', [0,0,0,0,0x520,0x7E,0x524,0x7E]);
            let x = dx, z = dz;
            while (Math.max(Math.abs(x), Math.abs(z)) > 32767) { x >>= 1; z >>= 1; }
            expect([g.s16(0x7E0520), g.s16(0x7E0524)]).toEqual([x, z]);
        }
        const distances = [-32768,-32767,-1024,-1,0,1,1024,32766,32767];
        for (const dx of distances) for (const dz of distances) {
            g.call('Enemy_distance', [dx, dz]);
            const x = Math.abs(dx), z = Math.abs(dz);
            expect(g.u16(g.symbol('tcc__r0'))).toBe(Math.min(32767, Math.max(x, z) + (Math.min(x, z) >> 1)));
        }
        let seed = 0xB16B00B5;
        for (let i = 0; i < 8192; i++) {
            seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
            const a = i < values.length * values.length ? values[Math.floor(i / values.length)] : i < 4096 ? seed : seed & 65535;
            seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
            const b = i < values.length * values.length ? values[i % values.length] : i < 4096 ? seed : seed & 65535;
            for (let r = 0; r <= 5; r++) w32(g.symbol(`tcc__r${r}`), 0xA55A0000 + r);
            w32(g.symbol('tcc__r9'), a); w32(g.symbol('tcc__r10'), b);
            g.call('tcc__mull');
            expect((g.cpu.Y | g.cpu.X << 16) >>> 0, `long product ${i}`).toBe(Math.imul(a, b) >>> 0);
            w32(g.symbol('tcc__r9'), a); w32(g.symbol('tcc__r10'), b);
            g.call('tcc__mul');
            expect(g.cpu.A, `word product ${i}`).toBe(Math.imul(a, b) & 65535);
            for (let r = 0; r <= 5; r++)
                expect((g.u16(g.symbol(`tcc__r${r}`)) | g.u16(g.symbol(`tcc__r${r}`) + 2) << 16) >>> 0)
                    .toBe(0xA55A0000 + r);
            products++;
        }
        // Same-slot collisions, NO_BOX and replacing a same-size map must not
        // resurrect geometry guesses or return a stale cached sector ID.
        g.set({ olBoxCount: 1024 });
        g.navMap(Array.from({ length: 128 }, (_, i) => i === 2 ? 65535 : i + 7));
        for (const i of [0,64,0,64,1,65,1,2,2]) {
            g.call('navCachedSectorBox', [i, 8]);
            expect(g.u16(g.symbol('tcc__r0'))).toBe(i === 2 ? 65535 : i + 7);
        }
        g.navMap(Array(128).fill(3));
        for (const [sector, depth, expected] of [[0,8,3], [2,8,3], [128,8,65535], [0,0,65535], [0,8,3]]) {
            g.call('navCachedSectorBox', [sector, depth]);
            expect(g.u16(g.symbol('tcc__r0'))).toBe(expected);
        }
        console.log(`[openlara] native helpers: ${products} long/word products, 864 corridor/225 steering/81 distance edges and exact sector cache invalidation`);
    });

    it('preserves all original zone maps and unions alternate reachability across CPU-only ROM banks', () => {
        const g = movementCartridge(); g.restoreLevel();
        const audit: { zones: number[][] } = JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8'));
        const count = g.u16(g.symbol('olBoxCount'));
        for (let flip = 0; flip < 2; flip++) for (let kind = 0; kind < 3; kind++) {
            const address = g.zoneAddress(kind, flip);
            expect(Array.from({ length: count }, (_, i) => g.u16(address + i * 2))).toEqual(audit.zones[flip * 3 + kind]);
        }
        const n = g.symbol('olNav'), size = 65;
        const maps = [Array.from({ length: size }, (_, i) => i % 5),
            Array.from({ length: size }, (_, i) => i % 3),
            Array.from({ length: size }, (_, i) => i & 1 ? 65535 : 0x8000)];
        maps.push(maps[0].map((_, i) => Math.floor(i / 3) % 5), maps[1],
            maps[2].map((_, i) => i % 3 ? 65535 : 0x8000));
        g.set({ olBoxCount: size }); g.zoneMap(maps);
        const source = g.table('Zone');
        const bytes = Math.max(...Array.from({ length: 6 }, (_, i) => g.u16(source + i * 2))) + size * 2;
        const data = Array.from({ length: bytes }, (_, i) => g.memory.read(source + i));
        let cases = 0;
        for (const offset of [0, 65536 - bytes]) {
            const address = 0xF80000 + offset; // CPU-only ROM, beyond the FX window
            data.forEach((v, i) => g.w8(address + i, v));
            g.w16(g.symbol('olZoneData'), offset); g.w16(g.symbol('olZoneData') + 2, 0xF8);
            g.memory.write(address, data[0] ^ 255);
            expect(g.memory.read(address)).toBe(data[0]);
            for (const flipped of [0, 1, 0xFFFF]) {
                g.call('Nav_setFlipped', [flipped]);
                expect(g.u16(g.symbol('olFlipped'))).toBe(Number(!!flipped));
                for (let kind = 0; kind < 3; kind++) {
                    expect(g.u16(g.symbol('olZoneOffsets') + kind * 2))
                        .toBe(g.u16(address + ((flipped ? 3 : 0) + kind) * 2));
                    for (const start of [0, 32, 64]) {
                        for (let i = 0; i < NAV_SIZE; i++) g.w8(n + i, 0xA5);
                        g.w16(n + N.step, [256, 512, 20480][kind]);
                        g.call('Nav_init', [n & 65535, 0x7E, start]);
                        const wanted = Array.from({ length: size }, (_, i) => i).filter(i =>
                            maps[kind][i] === maps[kind][start] || maps[kind + 3][i] === maps[kind + 3][start]);
                        expect(g.u16(n + N.zone)).toBe(kind);
                        expect(g.u16(n + N.count)).toBe(wanted.length);
                        expect(wanted.map((_, i) => g.u16(n + i * 8))).toEqual(wanted);
                        for (let i = wanted.length; i < size; i++) expect(g.u16(n + i * 8)).toBe(0xA5A5);
                        cases++;
                    }
                }
            }
        }
        console.log(`[openlara] zones: ${count * 6} original IDs, ${cases} ordered union lists, ` +
            'ground-1/ground-2/fly, both flips, shared/distinct arrays and F8 bank boundaries');
    });

    it('matches the original C++ Nav queues, weighted paths and corridor waypoints', () => {
        const g = movementCartridge(); g.restoreLevel();
        const root = g.symbol('olNav'), end = g.symbol('SECTIONEND_.bss.c_nav_fx3');
        expect(root).toBeGreaterThanOrEqual(0x7E5800);
        expect(end).toBeLessThanOrEqual(0x7F0000);
        const scratch = 0x7E0520;
        const ptr = (at: number) => [at & 65535, at >>> 16];
        const w32 = (at: number, v: number) => { g.w16(at, v); g.w16(at + 2, v >> 16); };
        type Box = [number, number, number, number, number, number];
        type Graph = { boxes: Box[]; overlaps: number[]; zones: number[][] };
        const count = g.u16(g.symbol('olBoxCount'));
        const boxes = g.table('Box');
        const overlaps = g.table('Overlap');
        const audit: { zones: number[][] } = JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8'));
        const original: Graph = {
            boxes: Array.from({ length: count }, (_, b) => [
                ...Array.from({ length: 4 }, (_, i) => g.memory.read(boxes + b * 8 + i)),
                g.s16(boxes + b * 8 + 4), g.u16(boxes + b * 8 + 6)] as Box),
            overlaps: Array.from({ length: g.count('overlaps') }, (_, i) => g.u16(overlaps + i * 2)),
            zones: audit.zones,
        };
        let cases = 0, instructions = 0, maxCall = 0, maxBatch = 0, searchSteps = 0;
        // Long winding corridors, mirrored through all four quadrants and
        // crossing 16-bit coordinate seams. Blocked is bit 14; a legitimate
        // overlap index containing bit 10 must not block the corridor.
        const corridor = (mirror: number): Graph => {
            const links: number[] = [], b: Box[] = [];
            for (let i = 0; i < 24; i++) {
                const row = Math.floor(i / 6), col = row & 1 ? 5 - i % 6 : i % 6;
                const x = mirror & 1 ? 130 - col * 2 : 62 + col * 2;
                const z = mirror & 2 ? 193 - row * 2 : 31 + row * 2;
                const offset = links.length;
                const near = [i - 1, i + 1].filter(k => k >= 0 && k < 24);
                links.push(...near.map((k, j) => k | (j === near.length - 1 ? 0x8000 : 0)));
                b.push([z, z + 2, x, x + 2, [0, 256, 512, 1536, -1024, 0][Math.floor(i / 4)], offset]);
            }
            b[12][5] |= 0x4000;
            while (links.length < 0x400) links.push(0x8000);
            b[13][5] = links.length;
            links.push(12, 0x800E);
            return { boxes: b, overlaps: links, zones: [
                b.map((_, i) => i === 7 ? 2 : 1), b.map((_, i) => i < 12 ? 7 : 8), b.map(() => 3),
                b.map((_, i) => i < 12 ? 5 : 6), b.map((_, i) => i % 4 ? 9 : 10),
                b.map((_, i) => i % 3 ? 65535 : 0x8000)] };
        };
        // Subtracting these signed floors overflows a 16-bit temporary in
        // both directions. The true +/-65535 deltas are outside every
        // supported step/drop profile, not the wrapped +/-1 stair steps.
        const extremeFloors = corridor(0);
        extremeFloors.boxes.forEach((b, i) => { b[4] = i & 1 ? -32768 : 32767; });
        // Retain the original bear's one-click drop profile as well as the
        // newly supported two-click ground family; wolves and flyers run in
        // both passes, including flips while their search queues are live.
        for (const [graphIndex, graph] of [original, ...[0, 1, 2, 3].map(corridor), extremeFloors].entries())
        for (const [groundStep, groundDrop] of [[256, -256], [512, -1024]]) {
            for (let i = 0; i < NAV_SIZE * 3; i++) g.w8(root + i, 0);
            g.w16(root - 2, 0xA55A); g.w16(end, 0x5AA5);
            const c = graph.boxes.length;
            const boxBase = 0x2000, overlapBase = boxBase + c * 8;
            const zoneBase = overlapBase + graph.overlaps.length * 2;
            g.set({ olBoxCount: c, olBoxBase: boxBase, olOverlapBase: overlapBase, olZoneBase: zoneBase });
            graph.boxes.forEach((b, i) => {
                b.slice(0, 4).forEach((v, j) => g.w8(g.blob + boxBase + i * 8 + j, v));
                g.w16(g.blob + boxBase + i * 8 + 4, b[4]); g.w16(g.blob + boxBase + i * 8 + 6, b[5]);
            });
            graph.overlaps.forEach((v, i) => g.w16(g.blob + overlapBase + i * 2, v));
            g.zoneMap(graph.zones);
            const lines = [`${c} ${graph.overlaps.length}`, ...graph.boxes.map(b => b.join(' ')),
                graph.overlaps.join(' '), ...graph.zones.map(z => z.join(' '))];
            const snapshots: Buffer[] = [];
            for (const [slot, step, drop, speed] of [[0, 256, -1024, 0], [1, groundStep, groundDrop, 0], [2, 20480, -20480, 16]]) {
                const n = root + slot * NAV_SIZE;
                g.w16(n + N.step, step); g.w16(n + N.drop, drop); g.w16(n + N.speed, speed);
                g.call('Nav_init', [...ptr(n), 0]);
                expect(g.u16(n + N.mask), 'blocked is bit 14, not overlap-index bit 10').toBe(0x4000);
                lines.push(`init ${slot} 0 ${step} ${drop} ${speed}`);
            }
            for (let tick = 0; tick < 72; tick++) {
                if (tick % 18 === 0) {
                    const flipped = Math.floor(tick / 18) & 1;
                    g.call('Nav_setFlipped', [flipped]);
                    lines.push(`flip 0 ${flipped}`);
                }
                const slot = tick % 3, n = root + slot * NAV_SIZE;
                const source = graphIndex ? tick < 36 ? 0 : 23 : (tick * 47) % c;
                const target = graphIndex ? tick < 36 ? 23 : 0 : (tick * 29 + 7) % c;
                if (graphIndex === 0 || tick < 3 || tick >= 36 && tick < 39) {
                    const b = graph.boxes[target], pos = [(b[2] + b[3]) * 512, b[4] - 128, (b[0] + b[1]) * 512];
                    g.w16(n + N.target, target); pos.forEach((v, i) => w32(n + N.pos + i * 4, v));
                    lines.push(`target ${slot} ${target} ${pos.join(' ')}`);
                }
                const b = graph.boxes[source];
                const from = [(b[2] + b[3]) * 512 + (tick % 5 - 2) * 513,
                    b[4] - 64, (b[0] + b[1]) * 512 + (tick % 7 - 3) * 257];
                from.forEach((v, i) => w32(scratch + i * 4, v));
                let batch = 0;
                const steps = g.call('Nav_getWaypoint', [...ptr(n), source, ...ptr(scratch)],
                    SYMS.has('Nav_searchNext') ? { name: 'Nav_searchNext', run: () => { batch++; } } : undefined);
                expect(batch, `graph ${graphIndex} tick ${tick}: search-step budget`).toBeLessThanOrEqual(5);
                maxBatch = Math.max(maxBatch, batch); searchSteps += batch;
                instructions += steps; maxCall = Math.max(maxCall, steps);
                lines.push(`waypoint ${slot} ${source} ${from.join(' ')}`);
                const raw = Buffer.from(Array.from({ length: NAV_SIZE }, (_, i) => g.memory.read(n + i)));
                if (raw.readUInt16LE(N.head) === 65535)
                    expect(raw.readUInt16LE(N.tail), 'empty queue retains a tail that cannot be retargeted').toBe(65535);
                snapshots.push(Buffer.concat([raw.subarray(0, N.owner), raw.subarray(N.pos)]));
                cases++;
            }
            const expected = execFileSync(process.env.OL_NAV_ORACLE!, [],
                { input: lines.join('\n') + '\n', timeout: 30000, maxBuffer: 16 * 1024 * 1024 });
            const size = NAV_SIZE - 2;
            expect(expected.length).toBe(snapshots.length * size);
            snapshots.forEach((got, i) => {
                const want = expected.subarray(i * size, (i + 1) * size);
                const mismatch = got.findIndex((v, k) => v !== want[k]);
                if (mismatch >= 0) console.log(`[openlara] Nav mismatch ${graphIndex}/${groundStep}/${groundDrop}/${i}: ` +
                    `waypoint ${[0,4,8].map(k => got.readInt32LE(size - 12 + k))} / ` +
                    `${[0,4,8].map(k => want.readInt32LE(size - 12 + k))}; ` +
                    `source ${lines.filter(l => l.startsWith('waypoint'))[i]}`);
                expect(mismatch, `graph ${graphIndex} profile ${groundStep}/${groundDrop} tick ${i}, byte ${mismatch}: ` +
                    `got ${got.readUInt16LE(Math.max(0, mismatch & ~1))}, want ${want.readUInt16LE(Math.max(0, mismatch & ~1))}`)
                    .toBe(-1);
            });
            expect([g.u16(root - 2), g.u16(end)], 'navigation wrote outside its dedicated region').toEqual([0xA55A, 0x5AA5]);
        }
        if (SYMS.has('Nav_searchNext')) expect(maxBatch, 'no full five-step batch was exercised').toBe(5);
        console.log(`[openlara] FX3 Nav: ${cases} complete queue/cell/waypoint snapshots match original C++; ` +
            `${Math.round(instructions / cases)} mean instructions, ${maxCall} max; ` +
            `${searchSteps} search steps, maximum ${maxBatch}/5 per waypoint`);
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C search generations and queue order across independent CPU-only bank boundaries', () => {
        const native = movementCartridge();
        const reference = movementCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const weights = [0,1,2,0x7FFE,0x7FFF,0x8000,0x8001,0x8002,0xFFFF];
        const profiles = [[256,-256], [512,-1024], [20480,-20480], [32767,-32768]];
        const floors = [-32768,-20480,-1024,-256,0,256,512,20480,32767];
        let nativeSteps = 0, referenceSteps = 0, changed = 0, empty = 0;
        for (let test = 0; test < 324; test++) {
            const variant = Math.floor(test / 81), count = [8,33,129,1024][variant];
            const slot = Math.floor(test / 3) % 3, current = (test * 17) % count;
            const start = [0,127,0x3FF8][test % 3], kind = test % 3, flip = (test >>> 2) & 1;
            const cw = weights[test % 9], nw = weights[Math.floor(test / 9) % 9];
            // Duplicates, invalid nonterminal IDs and a last word with BOTH
            // high bits set exercise cursor restoration and marker decoding.
            const links = [current, count + 1, 0x4001, ...Array.from({ length: 13 }, (_, i) =>
                i % 4 ? (i * 97 + test) % count : current)];
            links[links.length - 1] = ((current + 1) % count) | (test & 1 ? 0xC000 : 0x8000);
            const snapshots: Buffer[] = [];
            for (const g of [native, reference]) {
                const n = g.symbol('olNav') + slot * NAV_SIZE;
                for (let i = 0; i < NAV_SIZE; i++) g.w8(n + i, i * 43 + test);
                g.w16(n - 2, 0xA55A); g.w16(n + NAV_SIZE, 0x5AA5);
                g.set({ olBoxCount: count });
                const atEnd = !!(test & 1);
                const boxBase = 0xFA0000 + (atEnd ? 65536 - count * 8 : 0);
                const overlapSize = (start + links.length) * 2;
                const overlapBase = 0xF80000 + (atEnd ? 65536 - overlapSize : 0);
                const bind = (name: string, address: number) => {
                    g.w16(g.symbol(name), address); g.w16(g.symbol(name) + 2, address >>> 16);
                };
                bind('olBoxData', boxBase); bind('olOverlapData', overlapBase);
                for (let i = 0; i < count; i++) {
                    g.w16(boxBase + i * 8, 0x0402); g.w16(boxBase + i * 8 + 2, 0x0402);
                    g.w16(boxBase + i * 8 + 4, variant === 0 ? 0 : floors[(i + test) % floors.length]);
                    g.w16(boxBase + i * 8 + 6, i === current ? start : [0,0x4000,0x8000,0xC000][i % 4]);
                    g.w16(n + i * 8, i); g.w16(n + i * 8 + 2, nw);
                    g.w16(n + i * 8 + 4, (i + 1) % count); g.w16(n + i * 8 + 6, 65535);
                }
                links.forEach((v, i) => g.w16(overlapBase + (start + i) * 2, v));
                g.zoneMap(Array.from({ length: 6 }, (_, map) => Array.from({ length: count }, (_, i) =>
                    variant < 2 || i % 3 ? (map & 1 ? 0x8000 : 65535) : 7)), false, flip);
                const zoneSize = 12 + count * 12;
                const zones = Array.from({ length: zoneSize }, (_, i) => g.memory.read(g.table('Zone') + i));
                const zoneBase = 0xFB0000 + (atEnd ? 65536 - zoneSize : 0);
                zones.forEach((v, i) => g.w8(zoneBase + i, v)); bind('olZoneData', zoneBase);
                g.call('Nav_setFlipped', [flip]);
                const queued = variant & 1 ? (current + 2) % count : 65535;
                g.w16(n + current * 8 + 2, cw); g.w16(n + current * 8 + 6, queued);
                g.w16(n + N.zone, kind); g.w16(n + N.head, current);
                g.w16(n + N.tail, queued === 65535 ? current : queued);
                g.w16(n + N.step, profiles[variant][0]); g.w16(n + N.drop, profiles[variant][1]);
                g.w16(n + N.mask, [0,0x4000,0x8000,0xC000][test % 4]);
                const before = Buffer.from(Array.from({ length: NAV_SIZE }, (_, i) => g.memory.read(n + i)));
                const zone = g.u16(g.zoneAddress(kind, flip) + current * 2);
                const steps = g.call('Nav_search', [n & 65535,0x7E,zone]);
                const after = Buffer.from(Array.from({ length: NAV_SIZE }, (_, i) => g.memory.read(n + i)));
                snapshots.push(after);
                expect([g.u16(n - 2), g.u16(n + NAV_SIZE)], `search ${test} canaries`).toEqual([0xA55A,0x5AA5]);
                if (g === native) {
                    nativeSteps += steps;
                    for (let i = 0; i < count; i++) changed += before.readUInt16LE(i * 8 + 2) !== after.readUInt16LE(i * 8 + 2) ? 1 : 0;
                    empty += after.readUInt16LE(N.head) === 65535 ? 1 : 0;
                } else referenceSteps += steps;
            }
            expect(snapshots[0], `search ${test}: weights ${cw.toString(16)}/${nw.toString(16)}, bank end=${test & 1}`)
                .toEqual(snapshots[1]);
        }
        expect(changed).toBeGreaterThan(20); expect(empty).toBeGreaterThan(20);
        expect(nativeSteps).toBeLessThan(referenceSteps);
        console.log(`[openlara] native search: 324 exact C queue/cell snapshots, all full-weight pairs, ` +
            `three pools, independent $F8/$FA/$FB bank boundaries, ${changed} changed weights, ${empty} empty queues; ` +
            `${nativeSteps}/${referenceSteps} instructions`);
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C corridor sides and terminal clamps at signed coordinate and flag boundaries', () => {
        const native = movementCartridge();
        const reference = movementCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const values = [-2147483648,-65537,-1,0,512,1024,65535,2147483647];
        const bounds = [[0,1023,0,1023],[-2048,3071,-1024,2047],[65536,68607,196608,199679],[0,511,1024,1535]];
        const names = ['Nav_clipMinX','Nav_clipMaxX','Nav_clipMinZ','Nav_clipMaxZ','Nav_finish','Nav_deadEnd'];
        const sizes = [...Array.from({ length: 34 }, (_, i) => i), 63, 64, 65, 1023, 1024];
        let initCases = 0, nativeInitSteps = 0, referenceInitSteps = 0;
        for (const count of sizes) for (let slot = 0; slot < 3; slot++)
        for (let kind = 0; kind < 3; kind++) for (let pattern = 0; pattern < 4; pattern++) {
            const snapshots: Buffer[] = [];
            for (const g of [native, reference]) {
                const root = g.symbol('olNav') + slot * 8240;
                for (let i = 0; i < 8240; i++) g.w8(root + i, i * 43 + 17);
                g.w16(root - 2, 0xA55A); g.w16(root + 8240, 0x5AA5);
                g.set({ olBoxCount: count }); g.w16(root + 8206, [256, 512, 20480][kind]);
                g.zoneMap(Array.from({ length: 6 }, (_, map) => Array.from({ length: count }, (_, i) =>
                    pattern === 0 ? 0 : pattern === 1 ? Math.floor(i / (map + 1)) % 3 :
                    pattern === 2 ? ((i + map) % 5 ? 65535 : 0x8000) : i)), true, slot & 1);
                // All hits, sparse/interleaved hits, unsigned zone IDs and an
                // invalid source. Include every 0..31 tail and all three pools.
                const steps = g.call('Nav_init', [root & 65535, 0x7E,
                    !count || pattern === 3 && slot === 2 ? 65535 : count - 1]);
                if (g === native) nativeInitSteps += steps; else referenceInitSteps += steps;
                snapshots.push(Buffer.from(Array.from({ length: 8244 }, (_, i) => g.memory.read(root - 2 + i))));
                expect([g.u16(root - 2), g.u16(root + 8240)], `Nav_init ${count} canaries`).toEqual([0xA55A, 0x5AA5]);
            }
            expect(snapshots[0], `Nav_init ${count}/${slot}/${kind}/${pattern} full cells/compact union`).toEqual(snapshots[1]);
            initCases++;
        }
        expect(nativeInitSteps).toBeLessThan(referenceInitSteps);
        let cases = 0;
        for (const name of names) for (let test = 0; test < 256; test++) {
            const snapshots: Buffer[] = [];
            for (const g of [native, reference]) {
                const w32 = (at: number, v: number) => { g.w16(at, v); g.w16(at + 2, v >>> 16); };
                const root = g.symbol('olNav'), wp = 0x7E0520;
                w32(g.symbol('olNavClipWP'), wp); g.w16(wp - 2, 0xA55A); g.w16(wp + 12, 0x5AA5);
                [values[test % 8], -123456, values[(test >>> 1) % 8]].forEach((v, i) => w32(g.symbol('olNavClipFrom') + i * 4, v));
                [values[(test * 3 + 1) % 8], 654321, values[(test * 7 + 2) % 8]].forEach((v, i) => w32(wp + i * 4, v));
                bounds[test % bounds.length].forEach((v, i) => w32(g.symbol('olNavBounds') + i * 4, v));
                bounds[(test + 1) % bounds.length].forEach((v, i) => w32(g.symbol('olNavClipBounds') + i * 4, v));
                g.w16(g.symbol('olNavClip'), test >>> 3);
                [123456,-32768,234567].forEach((v, i) => w32(root + 8216 + i * 4, v));
                g.w16(root + 8194, test % 3);
                g.w16(g.table('Box') + 4, test & 2 ? 32767 : -32768);
                g.call(name, [root & 65535, 0x7E, 0]);
                const bytes = (at: number, size: number) => Array.from({ length: size }, (_, i) => g.memory.read(at + i));
                snapshots.push(Buffer.from(bytes(wp - 2, 16).concat(bytes(g.symbol('olNavClipBounds'), 16),
                    bytes(g.symbol('olNavClip'), 2), name.startsWith('Nav_clip') ? bytes(g.symbol('tcc__r0'), 2) : [])));
            }
            expect(snapshots[0], `${name} signed/flag case ${test}`).toEqual(snapshots[1]);
            cases++;
        }
        console.log(`[openlara] native corridor: ${initCases} exact C initialization snapshots (${sizes.length} lengths, ` +
            `three slots and zone families, default/alternate unions, dense/sparse/invalid sources), ${nativeInitSteps}/${referenceInitSteps} instructions; ` +
            `${cases} side/terminal clamps including signed-32-bit edges and all clip flags`);
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C and independent unsigned arithmetic for the complete logic PRNG state', () => {
        const native = movementCartridge();
        const ref = movementCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const seeds = [0,1,255,256,65535,65536,0x7FFFFFFF,0x80000000,0xFFFFFFFF,0xFF00FF00,0x00FF00FF];
        let random = 0x816F3, nativeSteps = 0, refSteps = 0, sequence = 0;
        for (let test = 0; test < 9216; test++) {
            random = (Math.imul(random, 1664525) + 1013904223) >>> 0;
            const seed = test < seeds.length ? seeds[test] : test < 8192 ? random : sequence;
            const expected = (Math.imul(seed, 0x41C64E6D) + 0x3039) >>> 0;
            for (const g of [native, ref]) {
                const at = g.symbol('olRandSeed');
                if (test <= 8192) { g.w16(at, seed); g.w16(at + 2, seed >>> 16); }
                const steps = g.call('rand_logic');
                if (g === native) nativeSteps += steps; else refSteps += steps;
                expect([(g.u16(at) | g.u16(at + 2) << 16) >>> 0,
                    g.u16(g.symbol('tcc__r0')), g.u16(g.symbol('tcc__r1'))], `PRNG seed ${seed.toString(16)}`)
                    .toEqual([expected, (expected >>> 10) & 32767, 0]);
            }
            sequence = expected;
        }
        console.log(`[openlara] native PRNG: 8192 seeds and 1024 uninterrupted rolls match C/uint32, including full return ABI; ` +
            `${nativeSteps}/${refSteps} instructions (${(100 * nativeSteps / refSteps).toFixed(1)}%)`);
    });

    it('keeps PRNG byte products exact in CPU-only ROM with full low/high word coverage', () => {
        const g = movementCartridge(), table = g.symbol('olRandProducts'), entry = g.symbol('rand_logic');
        const products = Buffer.from(Array.from({ length: 1024 }, (_, i) => g.memory.read(table + i)));
        expect(table >>> 16, 'PRNG products must be CPU FastROM, not a WRAM copy').toBeGreaterThanOrEqual(0x80);
        for (let i = 0; i < 256; i++) expect(products.readUInt32LE(i * 4)).toBe(Math.imul(i, 0x41C64E6D) >>> 0);
        // Observe actual instructions, then relocate only their long ROM
        // operands. Each fixture retains the exact linked routine/ABI.
        const reads = new Map<number, number>(), step = g.cpu.step;
        g.cpu.step = function () {
            const pc = this.PB << 16 | this.PC;
            if (g.memory.read(pc) === 0xBF) {
                const address = g.u16(pc + 1) | g.memory.read(pc + 3) << 16;
                if (address >= table && address < table + 4) reads.set(pc, address - table);
            }
            return step.call(this);
        };
        try { g.call('rand_logic'); } finally { g.cpu.step = step; }
        expect(reads.size).toBe(6);
        const write = g.memory.write, arithmeticWrites: number[] = [];
        g.memory.write = function (address: number, value: number) {
            if ((address >>> 16 & 0x40) === 0 && (address & 65535) >= 0x4202 && (address & 65535) <= 0x4206)
                arithmeticWrites.push(address);
            return write.call(this, address, value);
        };
        let cases = 0, steps = 0;
        const at = g.symbol('olRandSeed');
        try {
            for (const destination of [0xF80000, 0xF8FC00]) {
                products.forEach((v, i) => g.w8(destination + i, v));
                for (const [pc, offset] of reads) {
                    for (let i = 0; i < 3; i++) fixtureCodeByte(g.memory, pc + 1 + i, (destination + offset) >>> (i * 8));
                }
                // An odd affine permutation covers all 16-bit high words
                // while every low word is exercised. This is 131072 full
                // seeds, not an assertion that all 2^32 seeds were tested.
                for (let low = 0; low < 65536; low++) {
                    const high = (Math.imul(low, 40503) + (destination & 65535) + 0xA55A) & 65535;
                    const seed = (high << 16 | low) >>> 0;
                    const expected = (Math.imul(seed, 0x41C64E6D) + 0x3039) >>> 0;
                    g.w16(at, low); g.w16(at + 2, high);
                    steps += g.call('rand_logic');
                    expect([g.u16(at), g.u16(at + 2), g.u16(g.symbol('tcc__r0')), g.u16(g.symbol('tcc__r1'))],
                        `PRNG ${seed.toString(16)} with table at ${destination.toString(16)}`)
                        .toEqual([expected & 65535, expected >>> 16, expected >>> 10 & 32767, 0]);
                    cases++;
                }
            }
        } finally {
            g.memory.write = write;
            for (const [pc, offset] of reads)
                for (let i = 0; i < 3; i++) fixtureCodeByte(g.memory, pc + 1 + i, (table + offset) >>> (i * 8));
        }
        expect(arithmeticWrites, 'lookup PRNG unexpectedly used hardware multiplication/division').toEqual([]);
        expect(cases).toBe(131072);
        expect(entry >>> 16).toBeGreaterThanOrEqual(0x80);
        console.log(`[openlara] FX3 PRNG table: all 256 uint32 products, ${cases} full-state/return cases with complete ` +
            `low/high word coverage at $F8 bank start/end; ${steps} instructions, no arithmetic-register writes`);
    });

    it('matches original stalking quadrants and C across CPU-only box banks and full-width coordinates', () => {
        const native = movementCartridge();
        const ref = process.env.OL_REFERENCE_ROM ? movementCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS!)) : undefined;
        const deltas = [-3073,-3072,-3071,-1,0,1,3071,3072,3073,-65536,65536,-2147000000,2147000000];
        const offsets = [-2147000000,-65536,-1,0,1,65536,2147000000];
        const quad = (dx: number, dz: number) => dz > 0 ? dx > 0 ? 2 : 1 : dx > 0 ? 3 : 0;
        const w32 = (g: typeof native, at: number, v: number) => { g.w16(at, v); g.w16(at + 2, v >>> 16); };
        const bytes = (g: typeof native, at: number, n: number) =>
            Array.from({ length: n }, (_, i) => g.memory.read(at + i));
        const outcomes = new Set<string>();
        let cases = 0, cCases = 0, nativeSteps = 0, matchedSteps = 0, referenceSteps = 0;
        for (const low of [0,65536 - 32]) for (let angle = 0; angle < 65536; angle++) {
            const base = 0xF80000 + low, box = angle & 3;
            const sx = angle % 511, sz = (angle * 37 + 11) % 511;
            const bounds = [sz >>> 1,(sz + 1) >>> 1,sx >>> 1,(sx + 1) >>> 1];
            const dx = deltas[angle % deltas.length], dz = deltas[(angle >>> 4) % deltas.length];
            const tx = sx * 512 - dx, tz = sz * 512 - dz;
            // Clamp positions, not the original predicate. Every subtraction
            // remains defined signed-32-bit arithmetic for the C oracle.
            const ex = Math.max(-2147483648, Math.min(2147483647, tx + offsets[(angle >>> 2) % offsets.length]));
            const ez = Math.max(-2147483648, Math.min(2147483647, tz + offsets[(angle >>> 7) % offsets.length]));
            const qLara = ((angle << 16 >> 16) >> 14) + 2, qBox = quad(dx, dz);
            let expected = false, outcome = 'distance';
            if (Math.max(Math.abs(dx), Math.abs(dz)) <= 3072) {
                if (qLara === qBox) outcome = 'facing';
                else if (Math.abs(qLara - qBox) !== 2) { expected = true; outcome = 'side'; }
                else { expected = qLara !== quad(ex - tx, ez - tz); outcome = expected ? 'opposite-clear' : 'opposite-enemy'; }
            }
            outcomes.add(outcome);
            for (const g of ref && !(angle & 63) ? [native,ref] : [native]) {
                const lara = angle & 128 ? 0x7FFFE0 : g.item;
                w32(g, g.symbol('olBoxData'), base);
                bounds.forEach((v, i) => g.w8(base + box * 8 + i, v));
                w32(g, g.symbol('olNavLara'), lara); g.w16(lara + ITEM.angleY, angle);
                w32(g, g.symbol('olTargetPos'), tx); w32(g, g.symbol('olTargetPos') + 8, tz);
                w32(g, g.symbol('olEnemyPos'), ex); w32(g, g.symbol('olEnemyPos') + 8, ez);
                const snapshot = () => bytes(g, g.symbol('olTargetPos'), 12).concat(
                    bytes(g, g.symbol('olEnemyPos'), 12), bytes(g, g.symbol('olRandSeed'), 4),
                    bytes(g, lara, ITEM_SIZE), bytes(g, base, 32));
                const before = !(angle & 63) ? snapshot() : undefined;
                const steps = g.call('Enemy_checkStalk', [box]);
                expect(g.u16(g.symbol('tcc__r0')), `${g === native ? 'native' : 'C'} stalking ${low}:${angle}`).toBe(+expected);
                if (before) expect(snapshot(), 'stalking predicate mutated game state or CPU ROM').toEqual(before);
                if (g === native) {
                    nativeSteps += steps;
                    if (ref && !(angle & 63)) matchedSteps += steps;
                } else { referenceSteps += steps; cCases++; }
            }
            cases++;
        }
        expect(cases).toBe(131072); expect(outcomes.size).toBe(5);
        if (ref) { expect(cCases).toBe(2048); expect(matchedSteps).toBeLessThan(referenceSteps); }
        console.log(`[openlara] native stalking: ${cases} original-predicate cases, every angle and 0..510 bound sum, ` +
            `signed-32-bit deltas, strict-zero quadrants, CPU-only $F8 bank start/end and far Lara; ${nativeSteps} instructions` +
            (ref ? `; ${cCases} complete C/preserved-state checks, ${matchedSteps}/${referenceSteps} instructions` : ''));
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C mood distance, zone boundaries and random box consumption', () => {
        const native = movementCartridge();
        const ref = movementCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const values = [-2147483648,-262145,-65537,-32768,-3073,-3072,-1,0,1,3071,3072,32767,65536,262144,2147483647];
        const boxBounds = [[0,1,0,1],[63,65,193,196],[193,255,63,65],[0,0,0,0]];
        const bytes = (g: typeof native, at: number, n: number) =>
            Buffer.from(Array.from({ length: n }, (_, i) => g.memory.read(at + i)));
        const w32 = (g: typeof native, at: number, v: number) => { g.w16(at, v); g.w16(at + 2, v >>> 16); };
        let nativeSteps = 0, refSteps = 0;
        for (const g of [native, ref]) {
            w32(g, g.symbol('olNavEnemy'), g.item); w32(g, g.symbol('olEnemyNav'), g.symbol('olNav'));
            g.set({ olBoxCount: 4, olBoxBase: 0x2000, olZoneBase: 0x2100 });
            boxBounds.forEach((b, i) => b.forEach((v, j) => g.w8(g.blob + 0x2000 + i * 8 + j, v)));
            g.zoneMap([[0,1,0,1], [0,0,1,1], [2,2,2,2], [3,3,4,4], [5,6,5,6], [7,8,7,8]]);
            for (let i = 0; i < 1024; i++) g.w16(g.symbol('olNav') + i * 8, i * 53 + 17);
        }
        for (let test = 0; test < 2560; test++) {
            const snapshots: Buffer[][] = [];
            for (const g of [native, ref]) {
                g.w8(g.item, test < 2048 ? [7,8,9,0][test & 3] : 19); g.w16(g.item + ITEM.angleY, test * 1937);
                const root = g.symbol('olNav');
                [0,8].forEach((axis, i) => {
                    const from = values[(test + i * 3) % values.length];
                    // Full-width positions, but defined C arithmetic: negating
                    // INT_MIN or overflowing the signed distance sum is not
                    // an oracle contract. Legal world deltas are much smaller.
                    const delta = values[1 + (((test >>> 2) + i * 7) % (values.length - 2))];
                    const to = Math.max(-2147483648, Math.min(2147483647, from + delta));
                    w32(g, g.symbol('olEnemyPos') + axis, from);
                    w32(g, g.symbol('olTargetPos') + axis, to);
                });
                let steps = g.call('Enemy_moodDistance');
                const results = [bytes(g, g.symbol('tcc__r0'), 2)];
                g.set({ olEnemyBox: (test >>> 2) & 3 }); g.w16(root + 8194, test % 3);
                g.call('Nav_setFlipped', [test >>> 3 & 1]);
                const b = boxBounds[(test >>> 4) & 3];
                const edges = [b[2] * 1024 - 1,b[2] * 1024,b[2] * 1024 + 1,b[3] * 1024 - 2,
                    b[3] * 1024 - 1,b[3] * 1024,values[(test >>> 3) % values.length]];
                w32(g, g.symbol('olEnemyPos'), edges[test % edges.length]);
                w32(g, g.symbol('olEnemyPos') + 8, test & 2 ? b[0] * 1024 + 1 : b[1] * 1024 - 1);
                steps += g.call('Enemy_checkZone', [test % 11 === 0 ? 65535 : (test >>> 4) & 3]);
                results.push(bytes(g, g.symbol('tcc__r0'), 2));
                g.w16(root + 8192, [0,1,2,3,17,511,1024][test % 7]);
                w32(g, g.symbol('olRandSeed'), Math.imul(test + 1, 1664525) + 1013904223);
                steps += g.call('Enemy_randomBox');
                results.push(bytes(g, g.symbol('tcc__r0'), 2), bytes(g, g.symbol('olRandSeed'), 4));
                if (g === native) nativeSteps += steps; else refSteps += steps;
                snapshots.push(results);
            }
            expect(snapshots[0], `mood helpers ${test}: distance/zone/random box/PRNG`).toEqual(snapshots[1]);
        }
        console.log(`[openlara] native mood helpers: 2560 exact C signed-32-bit distance/zone/random box/PRNG cases; ` +
            `${nativeSteps}/${refSteps} instructions (${(100 * nativeSteps / refSteps).toFixed(1)}%)`);
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C attack selection at every species aggression edge and full-width target boundary', () => {
        const native = movementCartridge();
        const ref = movementCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const rolls = [0,1023,1024,8191,8192,16383,16384,32767];
        const coords = [-2147483648,-65537,-32768,-1,0,32767,65536,2147483647];
        const initial = Buffer.from(Array.from({ length: NAV_SIZE }, (_, i) => i * 43 + 17));
        const w32 = (g: typeof native, at: number, v: number) => { g.w16(at, v); g.w16(at + 2, v >>> 16); };
        let inverse = 1, count = 0;
        for (let i = 0; i < 5; i++) inverse = Math.imul(inverse, 2 - Math.imul(0x41C64E6D, inverse));
        for (const type of [7,8,9,19,123]) for (const zone of [0,1,2]) for (const roll of rolls) for (let edge = 0; edge < 8; edge++) {
            const target = [coords[edge], edge * 16000 - 56000, coords[7 - edge]];
            const height = [-32768,-762,-1,0,1,500,762,32767][edge];
            const targetBox = [0,1,1023,65535][edge & 3];
            const want = Buffer.from(initial); want.writeUInt16LE(zone, N.zone);
            if (roll < (type === 7 ? 0x2000 : type === 8 || type === 19 ? 0x4000 : 0x400)) {
                target.forEach((v, i) => want.writeInt32LE(i === 1 && zone === 2 ? v + height : v, N.pos + i * 4));
                want.writeUInt16LE(targetBox, N.target);
            }
            const seed = Math.imul((roll << 10) - 0x3039, inverse) >>> 0;
            for (const g of [native, ref]) {
                const root = g.symbol('olNav'), enemy = g.item + 2 * ITEM_SIZE;
                initial.forEach((v, i) => g.w8(root + i, v)); g.w16(root + N.zone, zone);
                w32(g, g.symbol('olEnemyNav'), root); w32(g, g.symbol('olNavEnemy'), enemy);
                w32(g, g.symbol('olNavLara'), g.item);
                g.w8(enemy + ITEM.mood, 2); g.w8(enemy, type); g.w16(g.item + ITEM.animIndex, 0);
                g.w16(g.table('AnimBox') + 4, height);
                target.forEach((v, i) => w32(g, g.symbol('olTargetPos') + i * 4, v));
                g.set({ olTargetBox: targetBox }); w32(g, g.symbol('olRandSeed'), seed);
                g.call('Enemy_moodChoose');
                const got = Buffer.from(Array.from({ length: NAV_SIZE }, (_, i) => g.memory.read(root + i)));
                expect(got, `attack selection ${count}: type ${type}, zone ${zone}, roll ${roll}`).toEqual(want);
                const at = g.symbol('olRandSeed');
                expect((g.u16(at) | g.u16(at + 2) << 16) >>> 0, 'attack must consume exactly one roll').toBe(roll << 10);
            }
            count++;
        }
        console.log(`[openlara] native attack selection: ${count} exact C/independent target and PRNG snapshots at aggression/height/32-bit edges`);
    });

    it('matches original normal/wild moods, aggression rolls and navigation targets', () => {
        const g = movementCartridge(), root = g.symbol('olNav');
        const ptr = (at: number) => [at & 65535, at >>> 16];
        const w32 = (at: number, v: number) => { g.w16(at, v); g.w16(at + 2, v >> 16); };
        const boxes: number[][] = [], links: number[] = [], ground: number[] = [], fly: number[] = [];
        for (let x = 0; x < 4; x++) for (let z = 0; z < 4; z++) {
            const b = x * 4 + z, p = links.length;
            const adjacent = [[x - 1, z], [x + 1, z], [x, z - 1], [x, z + 1]]
                .filter(([a, c]) => a >= 0 && a < 4 && c >= 0 && c < 4).map(([a, c]) => a * 4 + c);
            links.push(...adjacent.map((v, i) => v | (i === adjacent.length - 1 ? 0x8000 : 0)));
            boxes.push([193 + z * 2, 195 + z * 2, 63 + x * 2, 65 + x * 2, 2048, p]);
            ground.push(x < 2 ? 1 : 2); fly.push(3);
        }
        g.set({ olBoxCount: 16, olBoxBase: 0x2000, olOverlapBase: 0x2100, olZoneBase: 0x2200,
            itemsCount: 2 });
        boxes.forEach((b, i) => {
            b.slice(0, 4).forEach((v, j) => g.w8(g.blob + 0x2000 + i * 8 + j, v));
            g.w16(g.blob + 0x2000 + i * 8 + 4, b[4]); g.w16(g.blob + 0x2000 + i * 8 + 6, b[5]);
        });
        links.forEach((v, i) => g.w16(g.blob + 0x2100 + i * 2, v));
        const zoneMaps = [ground, ground.map((_, i) => i % 4), fly,
            ground.map((_, i) => i % 4 < 2 ? 5 : 6), ground.map((_, i) => i & 1), fly.map((_, i) => i < 8 ? 7 : 8)];
        g.zoneMap(zoneMaps);
        const room = g.table('Room');
        g.w8(room, 63); g.w8(room + 1, 193); g.w16(room + 2, 63 * 1024); g.w16(room + 4, 193 * 1024);
        g.navMap(Array.from({ length: 64 }, (_, i) => Math.floor(i / 16) * 4 + Math.floor(i % 8 / 2)));
        g.call('Room_reset');
        const lines = [`16 ${links.length}`, ...boxes.map(b => b.join(' ')), links.join(' '), ...zoneMaps.map(z => z.join(' '))];
        const snapshots: Buffer[] = [], descriptions: string[] = [];
        const item = g.item + ITEM_SIZE, args = [...ptr(item), ...g.args, 0x520, 0x7E, 0x524, 0x7E];
        const seedAt = g.symbol('olRandSeed');
        const head = g.s16(g.table('AnimBox') + 103 * 12 + 4);
        let inverse = 1;
        for (let i = 0; i < 5; i++) inverse = Math.imul(inverse, 2 - Math.imul(0x41C64E6D, inverse));
        const rolls = [0, 255, 256, 1023, 1024, 2047, 2048, 8191, 8192, 16383, 16384, 32767];
        for (let test = 0; test < 1024; test++) {
            const flipped = test >>> 6 & 1;
            g.call('Nav_setFlipped', [flipped]); lines.push(`flip 0 ${flipped}`);
            const type = test < 768 ? [7, 8, 9][test % 3] : 19, source = test * 7 % 16, dest = (test * 11 + 5) % 16;
            const mood = test >>> 2 & 3, hurt = test >>> 4 & 1, animated = test >>> 5 & 1;
            const next = test & 2 ? dest : 65535;
            const pos = [(boxes[source][2] * 1024) + 512, 2048, boxes[source][0] * 1024 + 512];
            const target = [boxes[dest][2] * 1024 + 512, 2048, boxes[dest][0] * 1024 + 512];
            const angle = test % 4 * 16384, health = test % 11 === 0 ? 0 : 1000;
            const aggression = type === 7 ? 8192 : type === 8 || type === 19 ? 16384 : 1024;
            const step = type === 9 ? 20480 : 256, drop = type === 9 ? -20480 : type === 7 ? -1024 : -256;
            const speed = type === 9 ? 16 : 0;
            const seed = Math.imul((rolls[test % rolls.length] << 10) - 0x3039, inverse) >>> 0;
            for (let i = 0; i < NAV_SIZE; i++) g.w8(root + i, 0);
            g.w16(root + N.owner, 1); g.w16(root + N.step, step); g.w16(root + N.drop, drop); g.w16(root + N.speed, speed);
            g.call('Nav_init', [...ptr(root), source]);
            g.w16(root + N.end, source); g.w16(root + N.target, next);
            pos.forEach((v, i) => w32(root + N.pos + i * 4, v));
            for (const [at, p] of [[item, pos], [g.item, target]] as const) {
                g.w8(at + ITEM.room, 0);
                [ITEM.x, ITEM.y, ITEM.z].forEach((f, i) => g.w16(at + f, p[i]));
                g.w16(at + ITEM.angleY, angle);
            }
            g.w8(item + ITEM.type, type); g.w8(item + ITEM.mood, mood);
            g.w8(item + ITEM.flags, 0x40); g.w8(item + ITEM.flags2, 1 | hurt * 8 | animated * 32);
            g.w16(g.item + ITEM.health, health); w32(seedAt, seed);
            const offset = type === 7 ? 375 : type === 8 ? 500 : type === 19 ? 400 : 0;
            const trig = (a: number) => g.s16(g.symbol('olSinTable') + (((a + 32) & 65535) >>> 6) * 2);
            const dx = Math.abs(target[0] - pos[0] - (trig(angle) * offset >> 14));
            const dz = Math.abs(target[2] - pos[2] - (trig(angle + 16384) * offset >> 14));
            const dist = Math.max(dx, dz) + (Math.min(dx, dz) >> 1);
            lines.push(`mood 0 ${source} ${dest} ${next} ${mood} ${hurt} ${animated} ${aggression} ` +
                `${pos.join(' ')} ${target.join(' ')} ${angle} ${health} ${head} ${dist} ${seed} ${step} ${drop} ${speed}`);
            g.call('Enemy_target', args);
            expect(g.u16(g.symbol('tcc__r0'))).toBe(1);
            let vx = (g.u16(root + N.waypoint) | g.u16(root + N.waypoint + 2) << 16) - pos[0];
            let vz = (g.u16(root + N.waypoint + 8) | g.u16(root + N.waypoint + 10) << 16) - pos[2];
            while (Math.max(Math.abs(vx), Math.abs(vz)) > 32767) { vx >>= 1; vz >>= 1; }
            expect([g.s16(0x7E0520), g.s16(0x7E0524)], `waypoint steering vector ${test}`).toEqual([vx, vz]);
            const header = Buffer.alloc(6);
            header.writeUInt16LE(g.memory.read(item + ITEM.mood));
            header.writeUInt32LE((g.u16(seedAt) | g.u16(seedAt + 2) << 16) >>> 0, 2);
            const raw = Buffer.from(Array.from({ length: NAV_SIZE }, (_, i) => g.memory.read(root + i)));
            snapshots.push(Buffer.concat([header, raw.subarray(0, N.owner), raw.subarray(N.pos)]));
            descriptions.push(`type=${type} source=${source} dest=${dest} mood=${mood} hurt=${hurt} wild=${animated} roll=${rolls[test % rolls.length]}`);
        }
        const expected = execFileSync(process.env.OL_NAV_ORACLE!, [],
            { input: lines.join('\n') + '\n', timeout: 30000, maxBuffer: 16 * 1024 * 1024 });
        const size = NAV_SIZE + 4;
        expect(expected.length).toBe(snapshots.length * size);
        snapshots.forEach((got, i) => {
            const want = expected.subarray(i * size, (i + 1) * size);
            const mismatch = got.findIndex((v, k) => v !== want[k]);
            expect(mismatch, `mood ${i}: ${descriptions[i]}, byte ${mismatch}; ` +
                `got ${got.readUInt16LE(Math.max(0, mismatch & ~1))}, want ${want.readUInt16LE(Math.max(0, mismatch & ~1))}`)
                .toBe(-1);
        });
        console.log(`[openlara] FX3 moods: ${snapshots.length} normal/wild mood, PRNG and full Nav snapshots match original C++`);
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C empty-slot admission, reclamation precedence and invalid-box rejection', () => {
        const native = movementCartridge();
        const reference = movementCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const patterns = [[65535,65535,65535], [2,65535,4], [2,3,65535], [2,3,4], [1,3,65535], [65535,3,4]];
        const initial = Buffer.from(Array.from({ length: NAV_SIZE * 3 }, (_, i) => i * 37 + 11));
        const bytes = (g: typeof native, at: number, n: number) => Buffer.from(Array.from({ length: n }, (_, i) => g.memory.read(at + i)));
        let cases = 0;
        for (const owners of patterns) for (const type of [7,8,9,255]) for (const forced of [0,1])
        for (const status of [0,1,2,3]) for (const valid of [false,true]) {
            const snapshots: Buffer[] = [], slots: number[] = [];
            for (const g of [native, reference]) {
                const root = g.symbol('olNav');
                initial.forEach((v, i) => g.w8(root + i, v));
                owners.forEach((v, i) => g.w16(root + i * NAV_SIZE + N.owner, v));
                g.set({ olBoxCount: valid ? 8 : 0, itemsCount: 6 });
                g.navMap(Array(128).fill(0)); g.call('Room_reset');
                g.zoneMap(Array.from({ length: 6 }, () => Array.from({ length: valid ? 8 : 0 }, (_, i) => i & 1)));
                [512,2048,512,0].forEach((v, i) => g.w16(g.symbol('camera') + i * 2, v));
                for (let i = 1; i <= 5; i++) {
                    const at = g.item + i * ITEM_SIZE;
                    for (let k = 0; k < ITEM_SIZE; k++) g.w8(at + k, 0);
                    g.w8(at, i === 1 ? type : 7);
                    g.w8(at + ITEM.flags, i === 2 && status === 0 ? 0 : 0x40);
                    g.w8(at + ITEM.flags2, i === 2 ? status : 1);
                    g.w16(at + ITEM.x, 512 + i * 1024); g.w16(at + ITEM.y, 2048); g.w16(at + ITEM.z, 512);
                    g.w8(g.symbol('olEnemyTurn') + i, 0x70 + i); g.w8(g.symbol('olItemDirty') + i, 0);
                }
                g.call('Enemy_navAcquire', [(g.item + ITEM_SIZE) & 65535, 0x7E, forced]);
                const pointer = g.u16(g.symbol('tcc__r0')) | g.u16(g.symbol('tcc__r0h')) << 16;
                slots.push(pointer ? (pointer - root) / NAV_SIZE : -1);
                snapshots.push(Buffer.concat([bytes(g, root, NAV_SIZE * 3), bytes(g, g.item + ITEM_SIZE, 5 * ITEM_SIZE),
                    bytes(g, g.symbol('olEnemyTurn') + 1, 5), bytes(g, g.symbol('olItemDirty') + 1, 5)]));
            }
            expect(slots[0], `admission case ${cases}`).toBe(slots[1]);
            expect(snapshots[0], `admission ${owners} / type ${type} / forced ${forced} / status ${status} / valid ${valid}`)
                .toEqual(snapshots[1]);
            cases++;
        }
        console.log(`[openlara] native admission: ${cases} complete C Nav/item/turn/dirty snapshots, including empty/occupied/reclaimed slots`);
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C creature dispatch, admission and release before tail-calling the species', () => {
        const native = movementCartridge();
        const reference = movementCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const names = ['Enemy_navAcquire','Enemy_navRelease','ItemObj_deactivate',
            'Wolf_update','Bear_update','Bat_update','Raptor_update'];
        const actors = ['Wolf_update','Bear_update','Bat_update','Raptor_update'];
        const bytes = (g: typeof native, at: number, n: number) =>
            Buffer.from(Array.from({ length: n }, (_, i) => g.memory.read(at + i)));
        const traces: string[][] = [[], []], originals = [native.cpu.step, reference.cpu.step];
        let cases = 0, nativeSteps = 0, referenceSteps = 0, dispatched = 0, released = 0, refused = 0;
        for (const [index, g] of [native, reference].entries()) {
            g.set({ olBoxCount: 1, itemsCount: 128, olRoomCount: 1 });
            [0,8,0,8].forEach((v, i) => g.w8(g.table('Box') + i, v));
            g.w16(g.table('Box') + 4, 2048); g.w16(g.table('Box') + 6, 0);
            g.w16(g.table('Overlap'), 0x8000); g.navMap(Array(128).fill(0)); g.zoneMap([[0],[0]]);
            [512,2048,512,0].forEach((v, i) => g.w16(g.symbol('camera') + i * 2, v));
            // This tests the dispatcher independently of the downstream
            // controllers, whose complete C/original-engine suites remain.
            for (const name of actors) fixtureCodeByte(g.memory, g.symbol(name), 0x6B);
            const watched = new Map(names.map(name => [g.symbol(name), name]));
            g.cpu.step = function () {
                const name = watched.get(this.PB << 16 | this.PC);
                if (name) {
                    const item = g.u16(this.S + 4) | g.u16(this.S + 6) << 16;
                    const args = name === 'Enemy_navAcquire' ? [g.u16(this.S + 8)] : actors.includes(name) ?
                        [g.u16(this.S + 8),g.u16(this.S + 10)] : [];
                    const slot = (item - g.item) / ITEM_SIZE;
                    traces[index].push(`${name}:${item.toString(16)}:${args.join(',')}:` +
                        `${g.memory.read(item + ITEM.flags2)}:${g.memory.read(g.symbol('olItemDirty') + slot)}`);
                }
                return originals[index].call(this);
            };
        }
        try {
            for (const type of [7,8,9,19,65]) for (const status of [0,1,2,3])
            for (const hp of [-1,0,20]) for (const timer of [0,1,300])
            for (const ownership of [0,1,2,3,4,5,6]) for (const slot of [1,127]) {
                const snapshots: Buffer[] = [];
                for (const [index, g] of [native, reference].entries()) {
                    traces[index].length = 0;
                    const root = g.symbol('olNav'), item = g.item + slot * ITEM_SIZE;
                    const lara = slot === 1 ? g.item : 0x7FFE00;
                    for (let i = 0; i < ITEM_SIZE * 128; i++) g.w8(g.item + i, i * 13 + 7);
                    for (let i = 0; i < NAV_SIZE * 3; i++) g.w8(root + i, i * 37 + 11);
                    for (let i = 0; i < 128; i++) {
                        g.w8(g.symbol('olItemDirty') + i, 0); g.w8(g.symbol('olEnemyTurn') + i, i * 19 + 31);
                    }
                    for (const owner of [slot,2,3,4]) {
                        const p = g.item + owner * ITEM_SIZE;
                        g.w8(p, owner === slot ? type : 7); g.w8(p + ITEM.room, owner === slot && ownership === 6 ? 2 : 0);
                        g.w8(p + ITEM.flags, 0x40); g.w8(p + ITEM.flags2, owner === slot ? status | 0xBC : 1);
                        g.w16(p + ITEM.health, owner === slot ? hp : 6);
                        g.w16(p + ITEM.timer, timer);
                        g.w16(p + ITEM.x, owner === slot ? ownership === 4 ? 7168 : 512 : 512 + (owner - 2) * 1024);
                        g.w16(p + ITEM.y, 2048); g.w16(p + ITEM.z, 512);
                        g.w8(p + ITEM.nextActive, owner === slot ? 2 : owner < 4 ? owner + 1 : 255);
                    }
                    g.w16(lara + ITEM.health, 1000); g.set({ olFirstActive: slot });
                    const owners = ownership < 3 ? [2,3,4].map((v, i) => i === ownership ? slot : v) :
                        ownership === 3 || ownership === 6 ? [65535,65535,65535] : [2,3,4];
                    owners.forEach((v, i) => g.w16(root + i * NAV_SIZE + N.owner, v));
                    g.call('Room_reset');
                    const steps = g.call('Enemy_update', [item & 65535,item >>> 16,lara & 65535,lara >>> 16]);
                    if (index) referenceSteps += steps; else nativeSteps += steps;
                    snapshots.push(Buffer.concat([bytes(g, g.item, ITEM_SIZE * 128), bytes(g, root, NAV_SIZE * 3),
                        bytes(g, g.symbol('olItemDirty'), 128), bytes(g, g.symbol('olEnemyTurn'), 128),
                        bytes(g, g.symbol('olFirstActive'), 2)]));
                }
                const description = `type ${type}, status ${status}, health ${hp}, timer ${timer}, ownership ${ownership}, item ${slot}`;
                expect(snapshots[0], description).toEqual(snapshots[1]);
                expect(traces[0], description).toEqual(traces[1]);
                const controller = traces[0].find(call => actors.some(name => call.startsWith(name + ':')));
                if (status === 2) { expect(controller).toBeUndefined(); released++; }
                else if (controller) {
                    expect(controller.startsWith((type === 7 ? 'Wolf' : type === 9 ? 'Bat' : type === 19 ? 'Raptor' : 'Bear') + '_update:')).toBe(true);
                    dispatched++;
                } else refused++;
                cases++;
            }
        } finally { native.cpu.step = originals[0]; reference.cpu.step = originals[1]; }
        expect(cases).toBe(2520); expect(dispatched).toBeGreaterThan(0); expect(refused).toBeGreaterThan(0);
        expect(released).toBe(630); expect(nativeSteps).toBeLessThan(referenceSteps);
        console.log(`[openlara] native creature dispatch: ${cases} complete C Nav/item/list/dirty/turn and ordered-call snapshots; ` +
            `${dispatched} species calls, ${released} releases, ${refused} refusals, real/far Lara arguments; ${nativeSteps}/${referenceSteps} instructions`);
    });

    it('constructs creature health before level-start activation', () => {
        const g = movementCartridge(); g.restoreLevel();
        const health = new Map([[7,6],[8,20],[9,1],[19,20]]);
        const source = g.table('Item'), count = g.count('items');
        const selected = Array.from({ length: count }, (_, i) => i)
            .find(i => i > 0 && health.has(g.memory.read(source + i * 16)))!;
        expect(selected).toBeGreaterThan(0);
        // Force one actual creature's initial activation. Others retain
        // their models/coordinates, but cannot crowd out this init oracle.
        for (let i = 1; i < count; i++) {
            const at = source + i * 16 + 3;
            g.w8(at, (g.memory.read(at) & 0x40) | (i === selected ? 0x1F : 0));
        }
        const seen: number[] = [];
        g.call('olLevelInit', [], { name: 'ItemObj_activate', run: stack => {
            const item = g.u16(stack + 4) | g.u16(stack + 6) << 16;
            if (!health.has(g.memory.read(item))) return;
            const value = g.s16(item + ITEM.health);
            expect(value, 'activation saw an unconstructed creature').toBe(health.get(g.memory.read(item)));
            seen.push((item - g.item) / ITEM_SIZE);
        } });
        expect(seen).toEqual([selected]);
        for (let i = 0; i < count; i++) expect(g.s16(g.item + i * ITEM_SIZE + ITEM.health), `initial health ${i}`)
            .toBe(i === 0 ? 1000 : health.get(g.memory.read(g.item + i * ITEM_SIZE)) ?? 0);
        expect(g.memory.read(g.item + selected * ITEM_SIZE + ITEM.flags) & 0x40).toBe(0x40);
        expect([0,1,2].map(i => g.u16(g.symbol('olNav') + i * NAV_SIZE + N.owner))).toContain(selected);
        console.log(`[openlara] FX3 constructor order: all ${count} item health values ready at load, ` +
            `actual creature ${selected} alive before activation and admitted to Nav`);
    });

    it('rejects dead-creature activation before list insertion or navigation eviction', () => {
        const g = movementCartridge(), root = g.symbol('olNav'), item = g.item + ITEM_SIZE;
        const bytes = (at: number, n: number) => Buffer.from(Array.from({ length: n }, (_, i) => g.memory.read(at + i)));
        g.set({ olBoxCount: 1, olRoomCount: 1, itemsCount: 6 });
        g.navMap(Array(128).fill(0)); g.zoneMap([[0],[0]]);
        [512,2048,512,0].forEach((v, i) => g.w16(g.symbol('camera') + i * 2, v));
        const snapshot = () => Buffer.concat([bytes(g.item, ITEM_SIZE * 6), bytes(root, NAV_SIZE * 3),
            bytes(g.symbol('olFirstActive'), 2), bytes(g.symbol('olItemDirty'), 128), bytes(g.symbol('olEnemyTurn'), 128)]);
        let dead = 0, alive = 0, reacquired = 0, objects = 0, alreadyActive = 0;
        for (const type of [7,8,9,19,65]) for (const hp of [-32768,-1,0,1,20,32767])
        for (const active of [false,true]) for (const status of [0,1,2,3]) for (const ownership of ['empty','others','self']) {
            const initialOwners = ownership === 'empty' ? [65535,65535,65535] : ownership === 'self' ? [1,3,4] : [2,3,4];
            for (let i = 0; i < NAV_SIZE * 3; i++) g.w8(root + i, i * 37 + 11);
            for (let i = 0; i < 3; i++) g.w16(root + i * NAV_SIZE + N.owner, initialOwners[i]);
            for (let i = 1; i < 6; i++) {
                const at = g.item + i * ITEM_SIZE;
                for (let k = 0; k < ITEM_SIZE; k++) g.w8(at + k, 0);
                g.w8(at, i === 1 ? type : 7); g.w8(at + ITEM.room, 0);
                g.w8(at + ITEM.flags, i === 1 ? active ? 0x40 : 0 : 0x40);
                g.w8(at + ITEM.flags2, i === 1 ? status : 1);
                g.w16(at + ITEM.health, i === 1 ? hp : 6);
                g.w16(at + ITEM.x, 512 + (i - 1) * 1024); g.w16(at + ITEM.y, 2048); g.w16(at + ITEM.z, 512);
                g.w8(at + ITEM.nextActive, i === 1 ? active ? 2 : 255 : i < 4 ? i + 1 : 255);
            }
            for (let i = 0; i < 128; i++) {
                g.w8(g.symbol('olItemDirty') + i, i * 19 + 31); g.w8(g.symbol('olEnemyTurn') + i, i * 13 + 11);
            }
            g.set({ olFirstActive: active ? 1 : 2 }); g.call('Room_reset');
            const before = snapshot();
            g.call('ItemObj_activate', [item & 65535, item >>> 16]);
            if ((type !== 65 && hp <= 0) || (active && (type === 65 || ownership === 'self'))) {
                expect(snapshot(), `activation changed state: type ${type}, hp ${hp}, active ${active}, status ${status}, ownership ${ownership}`)
                    .toEqual(before);
                if (type !== 65 && hp <= 0) dead++; else alreadyActive++;
            } else {
                expect(g.u16(g.symbol('olFirstActive'))).toBe(1);
                expect(g.memory.read(item + ITEM.flags) & 0x40).toBe(0x40);
                expect(g.memory.read(item + ITEM.nextActive)).toBe(2);
                const owners = [0,1,2].map(i => g.u16(root + i * NAV_SIZE + N.owner));
                if (type === 65) {
                    expect(owners).toEqual(initialOwners); objects++;
                } else { expect(owners).toContain(1); if (active) reacquired++; else alive++; }
            }
        }
        expect([dead,alive,reacquired,objects,alreadyActive]).toEqual([288,144,96,72,120]);
        console.log(`[openlara] FX3 activation: ${dead} corpse rejections preserve complete Nav/list/item state, ` +
            `${alive} live admissions, ${reacquired} active-owner retries without duplicate insertion, ` +
            `${objects} non-creature activations, ${alreadyActive} idempotent calls`);
    });

    it('owns three independent contexts, admits nearer creatures and safely rebases generations', () => {
        const g = movementCartridge(), root = g.symbol('olNav');
        g.set({ olBoxCount: 1, itemsCount: 6 });
        const box = g.table('Box');
        [0,8,0,8].forEach((v, i) => g.w8(box + i, v)); g.w16(box + 4, 2048); g.w16(box + 6, 0);
        g.w16(g.table('Overlap'), 0x8000);
        g.zoneMap([[0], [0]]);
        const camera = g.symbol('camera');
        [512,2048,512,0].forEach((v, i) => g.w16(camera + i * 2, v));
        for (let slot = 1; slot <= 5; slot++) {
            const item = g.item + slot * ITEM_SIZE;
            g.w8(item, 7); g.w8(item + ITEM.room, 0);
            g.w8(item + ITEM.flags, 0x40); g.w8(item + ITEM.flags2, 1);
            g.w16(item + ITEM.x, [0,1536,2560,3584,4608,768][slot]);
            g.w16(item + ITEM.y, 2048); g.w16(item + ITEM.z, 512); g.w16(item + ITEM.health, 6);
        }
        const acquire = (slot: number, forced = 0) => {
            const item = g.item + slot * ITEM_SIZE;
            g.call('Enemy_navAcquire', [item & 65535, 0x7E, forced]);
            return g.u16(g.symbol('tcc__r0')) | g.u16(g.symbol('tcc__r0h')) << 16;
        };
        const owners = () => [0,1,2].map(i => g.u16(root + i * NAV_SIZE + N.owner));
        const contexts = [1,2,3].map(slot => acquire(slot));
        expect(new Set(contexts).size).toBe(3); expect(owners()).toEqual([1,2,3]);
        for (let slot = 1; slot <= 3; slot++) {
            const item = g.item + slot * ITEM_SIZE;
            const steps = g.call('Enemy_navAcquire', [item & 65535, 0x7E, 0]);
            expect(g.u16(g.symbol('tcc__r0')) | g.u16(g.symbol('tcc__r0h')) << 16).toBe(contexts[slot - 1]);
            expect(steps, 'warm context lookup repeated cold admission work').toBeLessThan(32);
        }
        contexts.forEach((at, i) => g.w16(at + N.weight, 37 + i));
        expect(acquire(2)).toBe(contexts[1]); expect(g.u16(contexts[1] + N.weight)).toBe(38);
        expect(acquire(4), 'farther creature stole a nearer context').toBe(0);
        expect(owners()).toEqual([1,2,3]);
        expect(acquire(5)).toBe(contexts[2]); expect(owners()).toEqual([1,2,5]);
        expect(g.memory.read(g.item + 3 * ITEM_SIZE + ITEM.flags2) & 3).toBe(3);
        expect(g.u16(contexts[0] + N.weight)).toBe(37);
        expect(g.u16(contexts[1] + N.weight)).toBe(38);
        expect(acquire(4, 1), 'forced activation could not take the farthest context').toBe(contexts[1]);
        const item = g.item + ITEM_SIZE;
        g.call('Enemy_navRelease', [item & 65535, 0x7E]);
        expect(owners()).toEqual([65535,4,5]);
        expect(acquire(3)).toBe(contexts[0]);
        const n = contexts[0], scratch = 0x7E0520;
        g.w16(n + N.weight, 0x7FFF); g.w16(n + N.end, 65535); g.w16(n + N.target, 0);
        for (let i = 0; i < 12; i++) g.w8(n + N.pos + i, g.memory.read(scratch + i));
        g.call('Nav_getWaypoint', [n & 65535, 0x7E, 0, scratch & 65535, 0x7E]);
        expect([g.u16(n + N.weight), g.u16(n + 2), g.u16(n + N.owner)]).toEqual([1,1,3]);
        console.log('[openlara] FX3 Nav pool: independent owners, nearest/forced admission, release and generation rebase');
    });

    it('reconstructs full world coordinates and resets owners without touching the stack', () => {
        const g = movementCartridge(); g.restoreLevel();
        const audit = JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8'));
        const scratch = 0x7E0520;
        for (const [r, room] of audit.rooms.entries()) {
            for (const [dx, dz] of [[0, 0], [1023, 1024], [(room.width - 1) * 1024, (room.depth - 1) * 1024]]) {
                const x = room.x + dx, z = room.z + dz;
                g.call('Nav_point', [scratch & 65535, 0x7E, r, x, -1234, z]);
                const actual = [0, 4, 8].map(i => g.u16(scratch + i) | g.u16(scratch + i + 2) << 16);
                expect(actual, `world coordinate in room ${r}`).toEqual([x, -1234, z]);
            }
        }
        const root = g.symbol('olNav');
        for (let i = 0; i < 3; i++) g.w16(root + i * NAV_SIZE + N.owner, i + 1);
        g.call('Nav_reset');
        expect([0, 1, 2].map(i => g.u16(root + i * NAV_SIZE + N.owner))).toEqual([65535, 65535, 65535]);
    });
});

describe.skipIf(!SYMS.has('CS_Classified'))('OpenLara SNES/Super FX: native collision', () => {
    const reference = () => {
        const symbols = readSymbols(process.env.OL_REFERENCE_SYMS!);
        // Pre-native GSU-2 cartridges kept this collision workspace static.
        for (const name of ['Room','Trigger','SlantX','SlantZ','FrontOnly']) {
            const legacy = [...symbols.keys()].find(k => k.endsWith(`_col${name}`));
            if (!symbols.has(`olCol${name}`) && legacy)
                symbols.set(`olCol${name}`, symbols.get(legacy)!);
        }
        return movementCartridge(process.env.OL_REFERENCE_ROM!, symbols);
    };
    it.skipIf(!FX3 || !process.env.OL_REFERENCE_ROM)('matches C rotated item contacts across signed boundaries and far pointers', () => {
        const native = movementCartridge(), ref = reference();
        const edges = [-32768,-32767,-1025,-1001,-900,-763,-762,-501,-500,-1,0,1,499,500,761,762,899,1000,1024,32766,32767];
        const angles = [0,1,31,32,16383,16384,16385,32767,32768,49151,49152,65503,65504,65535];
        const word = (v: number) => v << 16 >> 16;
        let nativeSteps = 0, referenceSteps = 0, contacts = 0;
        for (let test = 0; test < 8192; test++) {
            const angle = test & 1 ? (test * 7919) & 65535 : angles[(test >>> 1) % angles.length];
            const radius = [0,100,341,-1,32767,-32768][(test >>> 2) % 6];
            const height = [762,0,1,-1,32767,-32768][(test >>> 4) % 6];
            const box = test & 4 ? [-500,500,-762,120,-900,1000]
                : Array.from({ length: 6 }, (_, i) => edges[(test * (i + 1) + i * 3) % edges.length]);
            const delta = [edges[test % edges.length], edges[(test * 7 + 3) % edges.length], edges[(test * 11 + 5) % edges.length]];
            const anchor = [edges[(test * 3) % edges.length], 32760, -32760];
            const before = Buffer.alloc(ITEM_SIZE, 0xA5), target = Buffer.alloc(ITEM_SIZE, 0x5A);
            before.writeUInt16LE(0, ITEM.animIndex); before.writeUInt16LE(angle, ITEM.angleY);
            [ITEM.x,ITEM.y,ITEM.z].forEach((at, i) => {
                before.writeInt16LE(anchor[i], at); target.writeInt16LE(word(anchor[i] + delta[i]), at);
            });
            const index = ((angle + 32) & 65535) >>> 5 & 0x7FE;
            const s = native.s16(native.symbol('olSinTable') + index);
            const c = native.s16(native.symbol('olSinTable') + ((index + 512) & 0x7FE));
            const px = word((Math.imul(delta[0], c) - Math.imul(delta[2], s)) >> 14);
            const pz = word((Math.imul(delta[2], c) + Math.imul(delta[0], s)) >> 14);
            const expected = word(box[3] + height) > delta[1] && box[2] < delta[1] &&
                px >= word(box[0] - radius) && px <= word(box[1] + radius) &&
                pz >= word(box[4] - radius) && pz <= word(box[5] + radius) ? 1 : 0;
            contacts += expected;
            for (const g of [native, ref]) {
                const item = [0x7E0580,0x000580,(FX3 ? 0x7FF100 : 0x71F900)][test % 3], lara = item + ITEM_SIZE;
                before.forEach((v, i) => g.w8(item + i, v)); target.forEach((v, i) => g.w8(lara + i, v));
                box.forEach((v, i) => g.w16(g.table('AnimBox') + i * 2, v));
                const steps = g.call('ItemObj_collideBounds', [item & 65535,item >>> 16,lara & 65535,lara >>> 16,radius,height]);
                if (g === native) nativeSteps += steps; else referenceSteps += steps;
                expect(g.u16(g.symbol('tcc__r0')), `contact ${test}: angle ${angle}, box ${box}, delta ${delta}, radius/height ${radius}/${height}`)
                    .toBe(expected);
                expect(Buffer.from(Array.from({ length: ITEM_SIZE }, (_, i) => g.memory.read(item + i)))).toEqual(before);
                expect(Buffer.from(Array.from({ length: ITEM_SIZE }, (_, i) => g.memory.read(lara + i)))).toEqual(target);
            }
        }
        expect(contacts, 'fixtures never made contact').toBeGreaterThan(50);
        expect(nativeSteps).toBeLessThan(referenceSteps);
        console.log(`[openlara] native item contacts: 8192 C/independent rotated/far-pointer/wrapped-boundary cases, ${contacts} hits; ` +
            `${nativeSteps} CPU instructions vs C ${referenceSteps} (${(100 * nativeSteps / referenceSteps).toFixed(1)}%)`);
    });
    const ptr = (at: number) => [at & 65535, at >>> 16];
    const bytes = (g: ReturnType<typeof movementCartridge>, at: number, n: number) =>
        Buffer.from(Array.from({ length: n }, (_, i) => g.memory.read(at + i)));
    const word = (v: number) => (v << 16) >> 16;

    it('sets every angle without changing adjacent collision fields or far-pointer banks', () => {
        const g = movementCartridge();
        const initial = Buffer.from(Array.from({ length: 62 }, (_, i) => i * 43 + 17));
        const addresses = [0x7E0520, 0x000520, (FX3 ? 0x7FF100 : 0x71F900)];
        for (const ci of addresses) initial.forEach((v, i) => g.w8(ci - 2 + i, v));
        for (let angle = 0; angle < 65536; angle++) {
            const ci = addresses[angle % addresses.length];
            g.call('CollisionInfo_setAngle', [...ptr(ci), angle]);
            expect([g.u16(ci + 46), g.u16(ci + 48)], `angle ${angle}`)
                .toEqual([angle, ((angle + 8192) & 65535) >>> 14]);
        }
        for (const ci of addresses) {
            const got = bytes(g, ci - 2, 62);
            initial.copy(got, 48, 48, 52); // only angle/quadrant are outputs
            expect(got).toEqual(initial);
        }
        console.log('[openlara] native collision: all 65536 angles, WRAM/mirror/far-bank pointers and adjacent-field canaries');
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C slope classes, signed stop thresholds and side padding', () => {
        const native = movementCartridge(), ref = reference();
        const slopes = [-32768, -32767, -3, -2, -1, 0, 1, 2, 3, 32767];
        const floors = [-32768, -32767, -513, -1, 0, 1, 512, 32767];
        let count = 0, nativeSteps = 0, refSteps = 0;
        const exercise = (sx: number, sz: number, floor: number, middle: number, stop: number) => {
            const ci = count % 3 === 0 ? (FX3 ? 0x7FF100 : 0x71F900) : count & 1 ? 0x520 : 0x7E0520;
            const side = count % 3 === 1 ? (FX3 ? 0x7FF150 : 0x71F950) : count & 1 ? 0x7E0580 : 0x580;
            const ceiling = word(count * 733 + 19);
            const snapshots: Buffer[] = [];
            for (const g of [native, ref]) {
                for (let i = -2; i < 60; i++) g.w8(ci + i, i * 23 + 91);
                for (let i = -2; i < 8; i++) g.w8(side + i, i * 41 + 33);
                g.w8(ci + 53, stop);
                const steps = g.call('CollisionInfo_setSide', [...ptr(ci), ...ptr(side), floor, ceiling, sx, sz, middle]);
                if (g === native) nativeSteps += steps; else refSteps += steps;
                snapshots.push(Buffer.concat([bytes(g, ci - 2, 62), bytes(g, side - 2, 10)]));
                const slant = sx === 0 && sz === 0 ? 0 : word(Math.abs(sx)) < 3 && word(Math.abs(sz)) < 3 ? 1 : 2;
                const expectedFloor = !middle && stop && slant === 2 && floor ? floor < 0 ? -32767 : 512 : floor;
                expect([g.s16(side), g.s16(side + 2), g.memory.read(side + 4)], `side ${count}`)
                    .toEqual([expectedFloor, ceiling, slant]);
            }
            expect(snapshots[0], `side ${count} full C snapshot`).toEqual(snapshots[1]);
            count++;
        };
        for (let i = 0; i < 4096; i++) exercise((i & 31) - 16, ((i >>> 5) & 31) - 16,
            floors[i % floors.length], (i >>> 10) & 1, (i >>> 11) & 1);
        for (const sx of slopes) for (const sz of slopes) for (const floor of floors)
            exercise(sx, sz, floor, count & 1 ? 0 : 65535, count & 2 ? 255 : 0);
        console.log(`[openlara] native collision: ${count} exact C side/class/padding snapshots; ` +
            `${nativeSteps}/${refSteps} instructions (${(100 * nativeSteps / refSteps).toFixed(1)}%)`);
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C bridge lists at sector seams, signed heights and far floor outputs', () => {
        const native = movementCartridge(), ref = reference();
        const coordinates = [-32768, -31745, -2048, -1025, -1024, -1, 0, 1, 1023, 1024, 2047, 31744, 32767];
        const heights = [-32768, -1024, -1, 0, 1, 512, 2048, 32767];
        let nativeSteps = 0, refSteps = 0, changed = 0;
        for (let test = 0; test < 2048; test++) {
            const x = coordinates[test % coordinates.length], z = coordinates[(test * 7 + 3) % coordinates.length];
            const y = heights[(test >>> 3) % heights.length], floor = heights[test % heights.length];
            const output = test % 3 === 0 ? (FX3 ? 0x7FF100 : 0x71F900) : test & 1 ? 0x520 : 0x7E0520;
            const count = test % 5;
            const bridges = Array.from({ length: count }, (_, n) => ({
                x: word(x + (n & 1 ? 1024 : 0)), z: word(z - (n === 3 ? 1024 : 0)),
                y: heights[(test + n * 3) % heights.length], type: [68, 69, 70, 65, 66, 41][(test + n) % 6],
                state: ((test >>> 4) + n) % 3,
                angle: [0, 0x4000, 0x8000, 0xC000, 0x1234][(test + n * 2) % 5],
            }));
            const extent = (b: typeof bridges[number]) => b.type === 41 ? 2 : b.type === 65 || b.type === 66 ? 1 : 0;
            const bounds = count ? [Math.min(...bridges.map(b => (b.x >> 10) - extent(b))),
                Math.max(...bridges.map(b => (b.x >> 10) + extent(b))),
                Math.min(...bridges.map(b => (b.z >> 10) - extent(b))),
                Math.max(...bridges.map(b => (b.z >> 10) + extent(b)))] : [32767, -32767, 32767, -32767];
            let expected = floor;
            for (const b of bridges) {
                let dx = (b.x >> 10) - (x >> 10), dz = (b.z >> 10) - (z >> 10);
                if (FX3 && [41,65,66].includes(b.type)) {
                    // Platform extents cross the signed 16-bit world seam;
                    // ordinary bridge sectors retain the exact sector match.
                    dx = ((dx + 32) & 63) - 32;
                    dz = ((dz + 32) & 63) - 32;
                }
                let h: number;
                if (FX3 && b.type === 41) {
                    const covers = b.angle === 0 ? dx === 0 && (dz === -1 || dz === -2) :
                        b.angle === 0x8000 ? dx === 0 && (dz === 1 || dz === 2) :
                        b.angle === 0x4000 ? dz === 0 && (dx === -1 || dx === -2) :
                        b.angle === 0xC000 ? dz === 0 && (dx === 1 || dx === 2) : false;
                    if (b.state !== 1 || !covers) continue;
                    h = b.y;
                } else if (FX3 && (b.type === 65 || b.type === 66)) {
                    const covers = dx === 0 && dz === 0 ||
                        dx === 0 && dz === 1 && b.angle === 0 ||
                        dx === 0 && dz === -1 && b.angle === 0x8000 ||
                        dx === 1 && dz === 0 && b.angle === 0x4000 ||
                        dx === -1 && dz === 0 && b.angle === 0xC000;
                    if (b.state !== 0 || !covers) continue;
                    h = b.y;
                } else {
                    if (dx !== 0 || dz !== 0) continue;
                    const ramp = (b.angle === 0 ? 1024 - x : b.angle === 0x8000 ? x : b.angle === 0x4000 ? z : 1024 - z) & 1023;
                    h = word(b.y + (b.type === 68 ? 0 : ramp >>> (b.type === 69 ? 2 : 1)));
                }
                if ((!FX3 || h !== -32512) && y <= h && h <= expected) expected = h;
            }
            const snapshots: Buffer[] = [];
            for (const g of [native, ref]) {
                g.set({ olBridgeCount: count });
                bounds.forEach((v, i) => g.w16(g.symbol('olBridgeBox') + i * 2, v));
                bridges.forEach((b, n) => {
                    const at = g.item + (n + 1) * ITEM_SIZE;
                    for (let i = 0; i < ITEM_SIZE; i++) g.w8(at + i, i * 29 + n);
                    g.w8(g.symbol('olBridges') + n, n + 1);
                    g.w8(at, b.type); g.w8(at + ITEM.state, b.state);
                    g.w16(at + ITEM.x, b.x); g.w16(at + ITEM.y, b.y);
                    g.w16(at + ITEM.z, b.z); g.w16(at + ITEM.angleY, b.angle);
                });
                const before = bytes(g, g.item + ITEM_SIZE, count * ITEM_SIZE);
                g.w16(output - 2, 0xA55A); g.w16(output, floor); g.w16(output + 2, 0x5AA5);
                const steps = g.call('Lara_itemFloorAt', [x, y, z, ...ptr(output)]);
                if (g === native) nativeSteps += steps; else refSteps += steps;
                expect(g.s16(output), `bridge ${test}`).toBe(expected);
                expect(bytes(g, g.item + ITEM_SIZE, count * ITEM_SIZE), `bridge ${test} mutated an item`).toEqual(before);
                expect([g.u16(output - 2), g.u16(output + 2)]).toEqual([0xA55A, 0x5AA5]);
                snapshots.push(bytes(g, output - 2, 6));
            }
            expect(snapshots[0], `bridge ${test} C output`).toEqual(snapshots[1]);
            if (expected !== floor) changed++;
        }
        expect(changed, 'bridge fixtures never raised a floor').toBeGreaterThan(100);
        console.log(`[openlara] native collision: 2048 exact C bridge probes (${changed} raised floors); ` +
            `${nativeSteps}/${refSteps} instructions (${(100 * nativeSteps / refSteps).toFixed(1)}%)`);
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C point probes and complete room collision on slopes, portals, bridges and missing geometry', () => {
        const native = movementCartridge(), ref = reference();
        let nativeSteps = 0, refSteps = 0;
        for (let test = 0; test < 1024; test++) {
            const snapshots: Buffer[] = [];
            for (const g of [native, ref]) {
                g.set({ olRoomCount: 2, olColRoom: test % 13 === 0 ? 65535 : 0, olColTrigger: 1234,
                    olColSlantX: -321, olColSlantZ: 456, olBridgeCount: test & 1 });
                g.room(0, false); g.room(1, false);
                for (let room = 0; room < 2; room++) {
                    const link = g.table('Link') + room * 8;
                    for (let i = 0; i < 4; i++) g.w8(link + i, 255);
                    if (room === 0 && test % 7 === 0) g.w8(link, 1); // horizontal portal
                    g.w8(link + 4, (((test & 31) - 16) & 15) | ((((test >>> 5) & 31) - 16) & 15) << 4);
                    g.w8(link + 5, (test * 37) & 255);
                    g.w16(link + 6, test * 17);
                    for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++)
                        g.sector(room, x, z, test % 11 === 0 ? -32512 : 2048 + room * 256,
                            test % 17 === 0 ? -32512 : -2048, room);
                }
                const bridge = g.item + ITEM_SIZE;
                g.w8(g.symbol('olBridges'), 1); g.w8(bridge, 68 + test % 3);
                g.w16(bridge + ITEM.x, 2560); g.w16(bridge + ITEM.z, 2560);
                g.w16(bridge + ITEM.y, 1024); g.w16(bridge + ITEM.angleY, (test & 3) << 14);
                [2,2,2,2].forEach((v, i) => g.w16(g.symbol('olBridgeBox') + i * 2, v));
                g.call('Room_reset');
                const ci = test & 1 ? 0x520 : 0x7E0520, side = ci + 8;
                for (let i = -2; i < 60; i++) g.w8(ci + i, i * 29 + 31);
                g.w8(ci + 53, test & 2 ? 1 : 0);
                const x = 2048 + (test * 73 & 1023), z = 2048 + (test * 419 & 1023);
                const y = [-32768,-2048,-1,0,1024,2048,32767][test % 7];
                const py = [0,1024,2048,32767,-32768][test % 5];
                let steps = g.call('colPoint', [...ptr(ci), ...ptr(side), x, y, z, py, word(y - 762), test & 4]);
                snapshots.push(Buffer.concat([bytes(g, ci - 2, 62), ...['olColRoom','olColTrigger','olColSlantX','olColSlantZ']
                    .map(name => bytes(g, g.symbol(name), 2))]));
                // Compare the complete native wrapper with its retained C body.
                g.w8(g.item + ITEM.room, 0); g.w16(g.item + ITEM.x, x); g.w16(g.item + ITEM.y, py); g.w16(g.item + ITEM.z, z);
                g.call('CollisionInfo_setAngle', [...ptr(ci), test * 137]);
                g.w16(ci + 26, [100,128,256][test % 3]);
                g.w16(ci + 28, [256,384,32767][test % 3]); g.w16(ci + 30, -256); g.w16(ci + 32, 0);
                g.w16(ci + 40, x - 17); g.w16(ci + 42, py); g.w16(ci + 44, z + 23);
                steps += g.call('ItemObj_collideRoom', [...g.args, ...ptr(ci), 762, test & 8 ? 128 : 0]);
                if (g === native) nativeSteps += steps; else refSteps += steps;
                snapshots.push(bytes(g, ci - 2, 62));
            }
            expect(snapshots[0], `probe ${test} full C result/workspace`).toEqual(snapshots[2]);
            expect(snapshots[1], `collideRoom ${test} full C result`).toEqual(snapshots[3]);
        }
        console.log(`[openlara] native collision: 1024 exact C point + complete room-collision snapshots; ` +
            `${nativeSteps}/${refSteps} instructions (${(100 * nativeSteps / refSteps).toFixed(1)}%)`);
    });

    it.skipIf(FX3 || !process.env.OL_REFERENCE_ROM)('matches C ordered closed-door contacts and pushes with far or aliased Lara records', () => {
        const native = movementCartridge(), ref = reference();
        let nativeSteps = 0, refSteps = 0, hits = 0, contacts = 0, moved = 0;
        for (let test = 0; test < 512; test++) {
            const snapshots: Buffer[] = [], traces: number[][][] = [];
            const count = [0,1,2,8,16,31,32][test % 7];
            const origin = [-32768,-1024,0,32767][(test >>> 2) % 4];
            for (const g of [native, ref]) {
                const lara = [g.item,0x000580,0x71F950,g.item + ITEM_SIZE * 3][test % 4];
                g.set({ olDoorCount: count });
                [-256,256,-762,0,-128,128].forEach((v, i) => g.w16(g.table('AnimBox') + i * 2, v));
                for (let slot = 0; slot <= 32; slot++) {
                    const at = g.item + slot * ITEM_SIZE;
                    for (let i = 0; i < ITEM_SIZE; i++) g.w8(at + i, i * 13 + test);
                    g.w8(at + ITEM.room, (slot + test) % 3);
                    g.w8(at + ITEM.state, (slot + test) % 5 ? 0 : 1);
                    g.w16(at + ITEM.x, word(origin + [-257,-128,-1,0,128,257,4096][slot % 7]));
                    g.w16(at + ITEM.y, 2048); g.w16(at + ITEM.z, word(origin + (slot % 3 - 1) * 64));
                    g.w16(at + ITEM.angleY, (slot * 7919 + test * 1021) & 65535);
                    g.w16(at + ITEM.animIndex, 0);
                }
                for (let i = 0; i < count; i++)
                    g.w8(g.symbol('olDoors') + i, test & 8 ? 1 + (i * 7 % 32) : 1 + (i % 4));
                for (let i = 0; i < ITEM_SIZE; i++) g.w8(lara + i, i * 29 + test);
                g.w8(lara + ITEM.room, test % 3); g.w8(lara + ITEM.state, 0);
                g.w16(lara + ITEM.x, origin); g.w16(lara + ITEM.y, 2048);
                g.w16(lara + ITEM.z, origin); g.w16(lara + ITEM.animIndex, 0);
                const expected: number[] = [];
                for (let i = 0; i < count; i++) {
                    const slot = g.memory.read(g.symbol('olDoors') + i), at = g.item + slot * ITEM_SIZE;
                    if (g.memory.read(at + ITEM.room) === test % 3 && !g.memory.read(at + ITEM.state))
                        expected.push(slot);
                }
                const before = bytes(g, lara, ITEM_SIZE), trace: number[][] = [];
                const steps = g.call('Lara_collideItems', ptr(lara), {
                    name: 'ItemObj_collideBounds', run: stack => {
                        const item = g.u16(stack + 4) | g.u16(stack + 6) << 16;
                        expect(g.u16(stack + 8) | g.u16(stack + 10) << 16).toBe(lara);
                        trace.push([(item - g.item) / ITEM_SIZE, g.u16(stack + 12), g.u16(stack + 14),
                            g.u16(lara + ITEM.x), g.u16(lara + ITEM.z)]);
                    }
                });
                expect(trace.map(t => t[0]), `door ${test} list order/filtering`).toEqual(expected);
                for (const t of trace) expect(t.slice(1,3)).toEqual([128,762]);
                const result = g.u16(g.symbol('tcc__r0'));
                expect([0,1]).toContain(result);
                traces.push(trace);
                snapshots.push(Buffer.concat([bytes(g, g.item, ITEM_SIZE * 33), bytes(g, lara, ITEM_SIZE),
                    Buffer.from([result])]));
                if (g === native) {
                    nativeSteps += steps; hits += result; contacts += trace.length;
                    if (!bytes(g, lara, ITEM_SIZE).equals(before)) moved++;
                } else refSteps += steps;
            }
            expect(traces[0], `door ${test} C coordinates after sequential pushes`).toEqual(traces[1]);
            expect(snapshots[0], `door ${test} C item/Lara/result snapshots`).toEqual(snapshots[1]);
        }
        expect(hits).toBeGreaterThan(100); expect(contacts).toBeGreaterThan(500);
        expect(moved).toBeGreaterThan(100); expect(nativeSteps).toBeLessThan(refSteps);
        console.log(`[openlara] native door collision: 512 complete C snapshots, ${contacts} ordered bounds calls, ` +
            `${hits} hits and ${moved} pushes; ${nativeSteps}/${refSteps} instructions`);
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C full room decisions, ordered zero-radius probes and far-pointer seams', () => {
        const native = movementCartridge(), ref = reference();
        const slopes = [-128,-17,-3,-2,0,1,2,3,16,127];
        const radii = [0,100,128,256,-1,32767,-32768];
        const types = new Set<number>();
        let nativeSteps = 0, refSteps = 0, statics = 0, zero = 0, frontOnly = 0, slopeSides = 0;
        for (let test = 0; test < 2049; test++) {
            const snapshots: Buffer[] = [], traces: number[][][] = [];
            const ceilingOnly = test === 2048;
            const originX = [-32768,0,28672,-4096][test % 4];
            const originZ = [0,28672,-32768,-4096][(test >>> 2) % 4];
            const x = word(originX + [2048,2049,3071,3072,32767,-1,0,2560][(test >>> 4) % 8]);
            const z = word(originZ + [2560,2048,3071,3072,-1,32767,0,2049][(test >>> 7) % 8]);
            const py = test % 4 ? 2048 : [-32768,32767,-2048,0,1024][(test >>> 2) % 5];
            const height = ceilingOnly || test % 4 ? 762 : [0,1,-1,32767,-32768][(test >>> 2) % 5];
            const yOffset = [0,128,-128,32767,-32768][(test >>> 3) % 5];
            const radius = radii[test % radii.length];
            const onlyFront = test % 5 === 0 ? test & 1 ? 1 : 65535 : 0;
            const angle = (test * 7919) & 65535;
            const quadrant = test % 13 ? ((angle + 8192) & 65535) >>> 14 : [4,5,65535][test % 3];
            // Flat, bridge-free cases pin equality in the separate > then
            // >= front-ceiling tests, which random heights seldom reach.
            const equalCeiling = (test & 31) === 1 && test % 7 !== 0 && test % 23 !== 0;
            for (const g of [native, ref]) {
                // Independent CPU-only banks, each table at its first or last
                // valid byte. Mutable sectors stay WRAM. Like the linked C
                // objects, ci must fit one bank: 816-tcc member arithmetic
                // wraps the low pointer word instead of carrying its bank.
                if (FX3) for (const [name, bank, size] of [['Room',0xF8,40], ['Link',0xF9,128],
                    ['Static',0xFA,40], ['StaticFirst',0xFB,10]] as const) {
                    const address = bank * 65536 + (test & 1 ? 65536 - size : 0);
                    g.w16(g.symbol(`ol${name}Data`), address);
                    g.w16(g.symbol(`ol${name}Data`) + 2, address >>> 16);
                }
                if (FX3) {
                    g.w16(g.symbol('olSectorData'), 0xA000); g.w16(g.symbol('olSectorData') + 2, 0x7F);
                }
                g.set({ olRoomCount: 4, olSectorCount: 256, olColFrontOnly: onlyFront,
                    olBridgeCount: test & 2 ? 1 : 0, olStaticCount: test & 4 ? 2 : 0 });
                for (let room = 0; room < 4; room++) {
                    g.room(room, false);
                    g.w16(g.table('Room') + room * 10 + 2, originX);
                    g.w16(g.table('Room') + room * 10 + 4, originZ);
                    for (let k = 0; k < 4; k++) {
                        const link = g.table('Link') + (room * 4 + k) * 8;
                        for (let i = 0; i < 4; i++) g.w8(link + i, 255);
                        // Both a terminating chain and a cycle (bounded by
                        // room count) exercise the next probe's room hint.
                        if (test % 11 === 0) g.w8(link + 2, room < 2 ? room + 1 : 255);
                        if (test % 17 === 0) g.w8(link + 2, room === 0 ? 1 : 0);
                        if (test % 19 === 0) {
                            g.w8(link, room < 3 ? room + 1 : 255);
                            g.w8(link + 1, room ? room - 1 : 255);
                        }
                        g.w8(link + 3, test + room * 7 + k);
                        for (let axis = 0; axis < 4; axis++) g.w8(link + 4 + axis,
                            test & 8 ? slopes[(test + room + k + axis * 3) % slopes.length] : 0);
                    }
                    for (let sx = 0; sx < 8; sx++) for (let sz = 0; sz < 8; sz++) {
                        const floors = [2048,1024,4096,-32512];
                        const floor = test & 16 ? floors[(sx + sz + test + room) % 4] : 2048;
                        const ceiling = test % 23 === 0 ? -32512 : test % 7 === 0 ? 1536 : -2048;
                        g.sector(room, sx, sz, floor, ceiling, room * 4 + (sx + sz) % 4);
                    }
                }
                const bridge = g.item + ITEM_SIZE;
                g.w8(g.symbol('olBridges'), 1); g.w8(bridge, 68 + test % 3);
                g.w16(bridge + ITEM.x, x); g.w16(bridge + ITEM.z, z);
                g.w16(bridge + ITEM.y, word(py - 256)); g.w16(bridge + ITEM.angleY, (test & 3) << 14);
                [x >> 10,x >> 10,z >> 10,z >> 10].forEach((v, i) => g.w16(g.symbol('olBridgeBox') + i * 2, v));
                for (let room = 0; room <= 4; room++) g.w16(g.table('StaticFirst') + room * 2, room && (test & 4) ? 2 : 0);
                for (let n = 0; n < 2; n++) {
                    const mesh = g.table('Static') + n * 20;
                    [0,word(x + (n ? 60 : 6500)),py,word(z + 40),-128,128,-1024,0,-128,128]
                        .forEach((v, i) => g.w16(mesh + i * 2, v));
                }
                const ci = [0x0520,(FX3 ? 0x7FF100 : 0x71F900),0x7F0000 - 58][test % 3];
                const item = [g.item,0x0580,0x7FFFE0][(test >>> 2) % 3];
                for (let i = -2; i < 60; i++) g.w8(ci + i, i * 29 + test);
                for (let i = 0; i < ITEM_SIZE; i++) g.w8(item + i, i * 13 + test);
                g.w8(item + ITEM.room, test % 29 ? 0 : 255);
                g.w16(item + ITEM.x, x); g.w16(item + ITEM.y, word(py - yOffset)); g.w16(item + ITEM.z, z);
                g.w16(ci + 26, radius);
                g.w16(ci + 28, [32512,256,384,32767,-32768][(test >>> 2) % 5]);
                g.w16(ci + 30, [-384,-256,-32768,32767][(test >>> 5) % 4]);
                g.w16(ci + 32, [0,-762,32767,-32768][(test >>> 4) % 4]);
                if (equalCeiling) {
                    g.w16(ci + 28, 32512); g.w16(ci + 30, -32512);
                    g.w16(ci + 32, word(-2048 - word(py - height)));
                }
                g.w16(ci + 40, word(x - 17)); g.w16(ci + 42, word(py + 33)); g.w16(ci + 44, word(z + 23));
                g.w16(ci + 46, angle); g.w16(ci + 48, quadrant); g.w8(ci + 53, test & 1 ? 255 : 0);
                if (ceilingOnly) {
                    // Pin CT_CEILING without a later side/front decision.
                    // GSU-2's packed slopes otherwise miss this branch in
                    // the random geometry above, unlike FX3's byte slopes.
                    g.set({ olRoomCount: 1, olBridgeCount: 0, olStaticCount: 0 });
                    g.room(0, false);
                    g.w16(g.table('StaticFirst'), 0); g.w16(g.table('StaticFirst') + 2, 0);
                    for (let k = 0; k < 8; k++) g.w8(g.table('Link') + k, k < 4 ? 255 : 0);
                    for (let sx = 0; sx < 8; sx++) for (let sz = 0; sz < 8; sz++)
                        g.sector(0, sx, sz, 4096, 1536, 0);
                    g.w8(item + ITEM.room, 0);
                    g.w16(item + ITEM.x, 2560); g.w16(item + ITEM.y, word(2048 - yOffset));
                    g.w16(item + ITEM.z, 2560);
                    g.w16(ci + 26, 100); g.w16(ci + 28, 32512); g.w16(ci + 30, -32512);
                    g.w16(ci + 32, word(1536 - word(2048 - height) + 256));
                    g.w16(ci + 46, 0); g.w16(ci + 48, 0);
                }
                const beforeItem = bytes(g, item, ITEM_SIZE), beforeLeftRight = bytes(g, ci + 14, 12);
                const trace: number[][] = [];
                g.call('Room_reset');
                const steps = g.call('ItemObj_collideRoom', [...ptr(item), ...ptr(ci), height, yOffset], {
                    name: 'colPoint', run: stack => {
                        const side = g.u16(stack + 8) | g.u16(stack + 10) << 16;
                        trace.push([side - ci, ...Array.from({ length: 6 }, (_, i) => g.u16(stack + 12 + i * 2)),
                            g.u16(g.symbol('olColRoom'))]);
                    }
                });
                traces.push(trace);
                if (ceilingOnly) expect(g.memory.read(ci + 50), 'isolated ceiling response').toBe(8);
                expect(trace.map(p => p[0]), `room ${test} probe order`).toEqual(onlyFront ? [2,8] : [2,8,14,20]);
                if (onlyFront) expect(bytes(g, ci + 14, 12), `room ${test} touched omitted sides`).toEqual(beforeLeftRight);
                if (!radius) for (const point of trace) expect(point.slice(1, 6)).toEqual(
                    [x & 65535,word(py - height - 160) & 65535,z & 65535,py & 65535,word(py - height) & 65535]);
                expect(bytes(g, item, ITEM_SIZE), `room ${test} changed the item`).toEqual(beforeItem);
                snapshots.push(Buffer.concat([bytes(g, ci - 2, 62), ...['olColRoom','olColTrigger',
                    'olColSlantX','olColSlantZ','olColFrontOnly'].map(name => bytes(g, g.symbol(name), 2))]));
                if (g === native) {
                    nativeSteps += steps; types.add(g.memory.read(ci + 50));
                    if (g.memory.read(ci + 57)) statics++;
                    if (!radius) { zero++; if (g.s16(ci + 2) !== g.s16(ci + 8)) slopeSides++; }
                    if (onlyFront) frontOnly++;
                } else refSteps += steps;
            }
            expect(traces[0], `room ${test} C probe coordinates/middle/room hints`).toEqual(traces[1]);
            expect(snapshots[0], `room ${test} complete C decisions/workspace/canaries`).toEqual(snapshots[1]);
        }
        expect(zero).toBeGreaterThan(250); expect(frontOnly).toBeGreaterThan(400);
        expect(statics).toBeGreaterThan(30); expect(slopeSides).toBeGreaterThan(20);
        expect([...types].sort((a,b) => a-b), 'fixtures must reach every collision decision').toEqual([0,1,2,4,8,16,32]);
        expect(nativeSteps).toBeLessThan(refSteps);
        console.log(`[openlara] native full room collision: 2049 complete C snapshots; ${zero} zero-radius, ` +
            `${frontOnly} front-only, ${statics} static hits, ${slopeSides} distinct zero-radius side floors; ` +
            `types ${[...types].sort((a,b) => a-b)}; ${nativeSteps}/${refSteps} instructions`);
    });
});

describe('OpenLara SNES/Super FX: native item motion', () => {
    const reference = () => process.env.OL_REFERENCE_ROM ? movementCartridge(
        process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS ?? '')) : undefined;

    it.skipIf(!FX3 || !process.env.OL_REFERENCE_ROM)('matches C render snapshots with dirty, active, hidden and frozen items', () => {
        const native = movementCartridge(), ref = reference()!;
        const counts = [0,1,2,3,60,90,127,128], freezes = [0,0,0,0,1,2,255,256,257,65535];
        const rates = [1,2,3,4,5,7,16,255];
        let nativeSteps = 0, referenceSteps = 0, published = 0;
        for (let test = 0; test < 512; test++) {
            const count = counts[test % counts.length], freeze = freezes[(test >>> 3) % freezes.length];
            for (const g of [native, ref]) {
                // the effects pool (fx.c): idle, it is not published
                const live = test % 3 === 0 ? 1 : 0, pending = test % 7 === 0 ? 1 : 0;
                g.set({ itemsCount: count, ol_animFreeze: freeze, olFxLive: live, olFxPending: pending });
                const fxFirst = g.u16(g.symbol('olFxFirst')), fxCount = g.u16(g.symbol('olFxCount'));
                const end = count && !(live | pending) && fxCount && fxFirst < count ? fxFirst : count;
                const items = g.symbol('items'), out = g.symbol('ol_itemState_gsu'), dirty = g.symbol('olItemDirty');
                const anims = g.table('Anim');
                const models = g.table('Model');
                for (let a = 0; a < 8; a++) g.w8(anims + a * 14 + 1, rates[a]);
                for (let m = 0; m < 16; m++) g.w16(models + m * 16, m * 31 + 300);
                for (let i = 0; i < 128 * ITEM_SIZE; i++) g.w8(items + i, i * 57 + test * 29);
                for (let i = 0; i < 128 * 12; i++) g.w8(out + i, i * 43 + test * 17);
                for (let i = 0; i < 128; i++) {
                    const p = items + i * ITEM_SIZE;
                    g.w16(p + ITEM.animIndex, (i + (test >>> 6)) & 7);
                    g.w8(p + ITEM.model, (i + test) & 15);
                    g.w8(p + ITEM.frameIndex, test + i * 3);
                    g.w8(p + ITEM.flags, (i + test) % 3 ? 0xBF : 0xFF);
                    g.w8(p + ITEM.flags2, 0xBC | ((i + test) & 3));
                    g.w8(dirty + i, (i + test) % 5 ? 0 : (test & 1 ? 0x80 : 1));
                }
                // Compare the complete general BSS region, including every
                // input item, unused output slot and reserved record byte.
                const base = 0x7E2000, size = 0x3800;
                const before = Buffer.from(Array.from({ length: size }, (_, i) => g.memory.read(base + i)));
                const expected = Buffer.from(before);
                expected.writeUInt16LE(end, g.symbol('olSyncEnd') - base);
                if (count) expected.writeUInt16LE(0, g.symbol('olFxPending') - base);
                for (let i = 0; i < end; i++) {
                    const p = items + i * ITEM_SIZE - base, q = out + i * 12 - base;
                    if (i && !freeze && !before[dirty + i - base] && !(before[p + ITEM.flags] & 0x40)) continue;
                    expected[dirty + i - base] = 0;
                    const hidden = (before[p + ITEM.flags2] & 3) === 3;
                    const a = before.readUInt16LE(p + ITEM.animIndex);
                    expected[q] = hidden ? 255 : freeze ? 0 : (a - (before[p + ITEM.model] * 31 + 300)) & 255;
                    expected[q + 1] = hidden ? 0 : freeze ? (freeze - 1) & 255 : Math.floor(before[p + ITEM.frameIndex] / rates[a]);
                    before.copy(expected, q + 2, p + ITEM.x, p + ITEM.angleY + 2);
                    expected[q + 10] = before[p + ITEM.room];   // the room it is drawn in (2026-09-24)
                    if (g === native) published++;
                }
                const steps = g.call('gameRenderSync');
                if (g === native) nativeSteps += steps; else referenceSteps += steps;
                const after = Buffer.from(Array.from({ length: size }, (_, i) => g.memory.read(base + i)));
                const mismatch = after.findIndex((v, i) => v !== expected[i]);
                expect(mismatch, `${g === native ? 'native' : 'C'} render snapshot ${test}, ` +
                    `count ${count}, freeze ${freeze}, BSS byte ${mismatch}`).toBe(-1);
            }
        }
        expect(nativeSteps).toBeLessThan(referenceSteps);
        console.log(`[openlara] native render snapshots: 512 C/independent full-BSS cases, ${published} records; ` +
            `${nativeSteps}/${referenceSteps} CPU instructions (${(100 * nativeSteps / referenceSteps).toFixed(1)}%)`);
    });

    it('matches 16-bit movement for every animation and extreme signed speed ramps', () => {
        const g = movementCartridge(), ref = reference();
        const s16 = (v: number) => (v << 16) >> 16;
        const s32 = (at: number) => g.u16(at) | g.u16(at + 2) << 16;
        const readItem = (cart: typeof g) => Buffer.from(
            Array.from({ length: ITEM_SIZE }, (_, i) => cart.memory.read(cart.item + i)));
        let seed = 0x1965816, count = 0, nativeSteps = 0, referenceSteps = 0;
        const random = () => seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
        const exercise = (anim: number, frame: number, gravity: boolean, type: number,
                          speed: number, accel: number) => {
            const initial = Buffer.from(Array.from({ length: ITEM_SIZE }, () => random() >>> 24));
            initial[ITEM.type] = type;
            initial.writeUInt16LE(anim, ITEM.animIndex); initial[ITEM.frameIndex] = frame;
            initial[ITEM.flags2] = (initial[ITEM.flags2] & ~4) | (gravity ? 4 : 0);
            // Exercise both sides of the terminal threshold and signed wrap.
            initial.writeInt16LE([-32768, -1, 0, 127, 128, 129, 32767][count % 7], ITEM.vSpeed);
            const moveAngle = random() & 65535;
            const expected = Buffer.from(initial);
            const ramp = (speed + Math.imul(accel, frame)) | 0;
            let h = ramp >> 16;
            if (gravity) {
                const previous = (ramp - accel) | 0;
                h = s16(initial.readInt16LE(ITEM.hSpeed) - (previous >> 16) + h);
                let v = initial.readInt16LE(ITEM.vSpeed);
                v = s16(v + (v < 128 ? 6 : 1));
                expected.writeInt16LE(v, ITEM.vSpeed);
                expected.writeInt16LE(s16(initial.readInt16LE(ITEM.y) + v), ITEM.y);
            }
            expected.writeInt16LE(h, ITEM.hSpeed);
            const angle = type === 0 ? moveAngle : initial.readUInt16LE(ITEM.angleY);
            const trig = (a: number) => g.s16(g.symbol('olSinTable') + (((a + 32) & 65535) >>> 6) * 2);
            expected.writeInt16LE(s16(initial.readInt16LE(ITEM.x) + (h * trig(angle) >> 14)), ITEM.x);
            expected.writeInt16LE(s16(initial.readInt16LE(ITEM.z) + (h * trig(angle + 16384) >> 14)), ITEM.z);
            for (const cart of [g, ref]) {
                if (!cart) continue;
                initial.forEach((v, i) => cart.w8(cart.item + i, v));
                const at = cart.table('AnimSpeed') + anim * 8;
                cart.w16(at, speed); cart.w16(at + 2, speed >> 16);
                cart.w16(at + 4, accel); cart.w16(at + 6, accel >> 16);
                cart.w16(cart.symbol('olLaraMoveAngle'), moveAngle);
                const steps = cart.call('ItemObj_move', cart.args);
                if (cart === g) nativeSteps += steps; else referenceSteps += steps;
                expect(readItem(cart), `${cart === g ? 'native' : 'C'} case ${count}: ` +
                    `anim=${anim} f=${frame} g=${gravity} speed=${speed} accel=${accel}`).toEqual(expected);
            }
            count++;
        };
        const totalAnims = g.count('anims');
        for (let a = 0; a < totalAnims; a++) {
            const at = g.table('AnimSpeed') + a * 8;
            const speed = s32(at), accel = s32(at + 4);
            const last = g.memory.read(g.table('Anim') + a * 14) - 1;
            for (const frame of new Set([0, 1, Math.max(0, last), 255])) {
                for (const gravity of [false, true]) for (const type of [0, 9])
                    exercise(a, frame, gravity, type, speed, accel);
            }
        }
        const extremes = [0, 1, -1, 32767, 32768, -32768, 65535, 65536, -65536, 0x7FFFFFFF, -0x80000000];
        for (const speed of extremes) for (const accel of extremes) {
            for (const gravity of [false, true])
                exercise(0, count & 255, gravity, count & 1 ? 0 : 255, speed, accel);
        }
        console.log(`[openlara] native item motion: ${count} exact movement cases; ${nativeSteps} CPU instructions` +
            (ref ? ` vs C ${referenceSteps} (${(100 * nativeSteps / referenceSteps).toFixed(1)}%)` : ' (integer oracle)'));
        if (ref) expect(nativeSteps).toBeLessThan(referenceSteps);
    });

    it('matches every animation transition and its inclusive frame boundaries', () => {
        const g = movementCartridge(), ref = reference();
        const anims = g.table('Anim');
        const trans = g.table('Trans');
        let count = 0;
        for (let a = 0; a < g.count('anims'); a++) {
            const record = anims + a * 14, state = g.memory.read(record + 9);
            const first = g.u16(record + 6), total = g.memory.read(record + 8);
            const goals = new Set([state, 255]), frames = new Set([0, 255]);
            for (let t = first; t < first + total; t++) {
                goals.add(g.memory.read(trans + t * 6));
                for (const edge of [1, 2]) {
                    const frame = g.memory.read(trans + t * 6 + edge);
                    for (const delta of [-1, 0, 1]) if (frame + delta >= 0 && frame + delta <= 255)
                        frames.add(frame + delta);
                }
            }
            for (const goal of goals) for (const frame of frames) {
                const initial = Buffer.alloc(ITEM_SIZE, 0xA5);
                initial[ITEM.type] = 7; initial[ITEM.state] = state; initial[ITEM.goalState] = goal;
                initial.writeUInt16LE(a, ITEM.animIndex); initial[ITEM.frameIndex] = frame;
                const expected = Buffer.from(initial);
                if (goal !== state) for (let t = first; t < first + total; t++) {
                    const p = trans + t * 6;
                    if (g.memory.read(p) !== goal || frame < g.memory.read(p + 1) || frame > g.memory.read(p + 2)) continue;
                    const dest = g.u16(p + 4);
                    expected[ITEM.frameIndex] = g.memory.read(p + 3);
                    expected.writeUInt16LE(dest, ITEM.animIndex);
                    expected[ITEM.state] = g.memory.read(anims + dest * 14 + 9);
                    break; // first match wins, including overlapping ranges
                }
                for (const cart of [g, ref]) {
                    if (!cart) continue;
                    initial.forEach((v, i) => cart.w8(cart.item + i, v));
                    cart.w8(cart.symbol('olEnemyNext'), expected[ITEM.state]);
                    cart.call('ItemObj_animChange', cart.args);
                    expect(Buffer.from(Array.from({ length: ITEM_SIZE }, (_, i) => cart.memory.read(cart.item + i))),
                        `transition ${count}: anim=${a} frame=${frame} goal=${goal}`).toEqual(expected);
                    expect(cart.memory.read(cart.symbol('olEnemyNext')), 'animChange consumed a pending state before animProcess')
                        .toBe(expected[ITEM.state]);
                }
                count++;
            }
        }
        console.log(`[openlara] native item animation: ${count} transition cases, inclusive edges and unchanged goal/pending state`);
    });

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C frame advancement, commands and pending states for every animation', () => {
        const g = movementCartridge(), ref = reference()!;
        const anims = g.table('Anim');
        const trans = g.table('Trans');
        const cmds = g.table('Cmd');
        const globals = ['olLaraWeaponState', 'olLaraVSpeedHack', 'olItemDirty'];
        let count = 0, nativeSteps = 0, referenceSteps = 0;
        for (let a = 0; a < g.count('anims'); a++) {
            const record = anims + a * 14, state = g.memory.read(record + 9);
            const first = g.u16(record + 6), total = g.memory.read(record + 8);
            const nextState = g.memory.read(anims + g.u16(record + 2) * 14 + 9);
            const goals = new Set([state, nextState, 255]);
            const frames = new Set([0, 254, 255, (g.memory.read(record) - 1) & 255]);
            for (let t = first; t < first + total; t++) {
                goals.add(g.memory.read(trans + t * 6));
                for (const edge of [1, 2]) {
                    const frame = g.memory.read(trans + t * 6 + edge);
                    for (const delta of [-2, -1, 0]) frames.add((frame + delta) & 255);
                }
            }
            const firstCmd = g.u16(record + 10), totalCmds = g.memory.read(record + 12);
            for (let c = firstCmd; c < firstCmd + totalCmds; c++) if (g.memory.read(cmds + c * 8) === 6)
                frames.add((g.u16(cmds + c * 8 + 2) - 1) & 255);
            // Last item slot checks the array-index calculation; slot zero
            // checks pointer-identity effects independently of item.type.
            for (const slot of [0, 1, 127]) for (const goal of goals) for (const frame of frames) {
                const initial = Buffer.alloc(ITEM_SIZE, 0);
                initial[ITEM.type] = slot === 0 ? 0 : 7;
                initial[ITEM.state] = state; initial[ITEM.goalState] = goal;
                initial[ITEM.model] = 0;
                initial.writeUInt16LE(a, ITEM.animIndex); initial[ITEM.frameIndex] = frame;
                initial[ITEM.flags] = 0x5F; initial[ITEM.flags2] = 9;
                initial.writeInt16LE(32760, ITEM.x); initial.writeInt16LE(-32760, ITEM.y);
                initial.writeInt16LE(-6144, ITEM.z); initial.writeUInt16LE(count * 7919 & 65535, ITEM.angleY);
                initial.writeInt16LE(133, ITEM.vSpeed); initial.writeInt16LE(-24, ITEM.hSpeed);
                const pending = [0, state, nextState, goal][count & 3];
                const snapshots: unknown[] = [];
                for (const cart of [g, ref]) {
                    const item = cart.item + slot * ITEM_SIZE;
                    initial.forEach((v, i) => cart.w8(item + i, v));
                    cart.w8(cart.symbol('olEnemyNext') + slot, pending);
                    cart.set({ olLaraWeaponState: 3, olLaraVSpeedHack: count & 1 ? 73 : 0, olItemDirty: 0 });
                    const steps = cart.call('ItemObj_animProcess', [item & 65535, item >>> 16]);
                    if (cart === g) nativeSteps += steps; else referenceSteps += steps;
                    snapshots.push({
                        item: Buffer.from(Array.from({ length: ITEM_SIZE }, (_, i) => cart.memory.read(item + i))),
                        pending: cart.memory.read(cart.symbol('olEnemyNext') + slot),
                        globals: globals.map(name => cart.u16(cart.symbol(name)))
                    });
                }
                expect(snapshots[0], `process ${count}: slot=${slot} anim=${a} frame=${frame} goal=${goal} next=${pending}`)
                    .toEqual(snapshots[1]);
                count++;
            }
        }
        console.log(`[openlara] native item animation: ${count} exact C frame/command/pending-state cases; ` +
            `${nativeSteps} CPU instructions vs C ${referenceSteps} (${(100 * nativeSteps / referenceSteps).toFixed(1)}%)`);
        expect(nativeSteps).toBeLessThan(referenceSteps);
    });

    it.skipIf(!FX3 || !process.env.OL_REFERENCE_ROM)('matches C ordered frame effects and end commands in independent CPU-only banks', () => {
        const native = movementCartridge(), ref = reference()!;
        const bytes = (g: typeof native, at: number, n: number) => Buffer.from(
            Array.from({ length: n }, (_, i) => g.memory.read(at + i)));
        const ptr = (at: number) => [at & 65535, at >>> 16];
        const word = (v: number) => v << 16 >> 16;
        let nativeSteps = 0, referenceSteps = 0, normalCalls = 0, rotations = 0, frees = 0, ends = 0;
        for (let test = 0; test < 2048; test++) {
            const fx = test % 8 === 0 ? 0 : [1,2,255,256,65535][test % 5];
            const count = fx ? [0,1,2,7,31,255][test % 6] : [0,1,2,7][(test >>> 3) % 4];
            const start = [0,1,64,65][(test >>> 3) % 4], initialFrame = [0,1,254,255][(test >>> 2) % 4];
            const firstAnim = [0,50,308,309,65535][(test >>> 4) % 5];
            const model = [0,1,63][(test >>> 6) % 3], anim = 3;
            const initial = Buffer.from(Array.from({ length: ITEM_SIZE }, (_, i) => i * 13 + test));
            initial[ITEM.type] = [0,7,19,255][test % 4]; initial[ITEM.model] = model;
            initial[ITEM.frameIndex] = initialFrame; initial.writeUInt16LE(anim, ITEM.animIndex);
            initial.writeUInt16LE(test * 7919 & 65535, ITEM.angleY);
            initial.writeInt16LE(32760, ITEM.x); initial.writeInt16LE(-32760, ITEM.y);
            initial.writeInt16LE(-6144, ITEM.z);
            const commands = Array.from({ length: count }, (_, c) => {
                const op = [0,1,2,3,4,5,6,6,6,255][(test + c) % 10];
                const a0 = op === 6 ? [initialFrame,0,255,256,-1][(test + c * 3) % 5] :
                    [-32768,32767,-1,0,512][(test + c) % 5];
                return [op,a0,[0,2,12,1,-1,256,32767,-32768][(test + c * 5) % 8],word(test * 419 + c * 7919)];
            });
            // NORMAL can change the current frame; later effects must see
            // that new frame, but must still come from the captured old list.
            if (fx && count >= 3 && test & 1) {
                commands[0] = [6,initialFrame,2,0];
                commands[1] = [6,0,0,0];
                commands[2] = [6,initialFrame,12,0];
            }
            const snapshots: Buffer[] = [], calls: number[][] = [];
            for (const g of [native, ref]) {
                for (const [name, bank, size] of [['Anim',0xF8,320 * 14], ['Model',0xF9,64 * 16],
                    ['Cmd',0xFA,320 * 8]] as const) {
                    const at = bank * 65536 + (test & 1 ? 65536 - size : 0);
                    g.w16(g.symbol(`ol${name}Data`), at); g.w16(g.symbol(`ol${name}Data`) + 2, at >>> 16);
                }
                for (let a = 0; a < 320; a++) {
                    const record = g.table('Anim') + a * 14;
                    for (let i = 0; i < 14; i++) g.w8(record + i, 0);
                    g.w8(record, test % 17 === 0 && a !== anim ? 0 : 16);
                    g.w8(record + 9, a * 29 + 7);
                }
                g.w16(g.table('Anim') + anim * 14 + 10, start);
                g.w8(g.table('Anim') + anim * 14 + 12, count);
                g.w16(g.table('Model') + model * 16, firstAnim);
                commands.forEach(([op,a0,a1,a2], c) => {
                    const at = g.table('Cmd') + (start + c) * 8;
                    g.w16(at, op | 0xA500); g.w16(at + 2, a0); g.w16(at + 4, a1); g.w16(at + 6, a2);
                });
                // End commands can dirty an item slot, so use only real
                // slots there. Frame effects also accept unrelated far items.
                const choices = [g.item,g.item + ITEM_SIZE,g.item + 127 * ITEM_SIZE,0x520,0x7FFFE0];
                const item = choices[(test >>> 1) % (fx ? choices.length : 3)];
                initial.forEach((v, i) => g.w8(item + i, v));
                const before = bytes(g, item, ITEM_SIZE);
                g.set({ olLaraWeaponState: 3, olLaraVSpeedHack: test & 4 ? 73 : 0 });
                for (let i = 0; i < 128; i++) g.w8(g.symbol('olItemDirty') + i, i * 19 + test);
                const seen: number[] = [];
                const steps = g.call('ItemObj_animCmd', [...ptr(item), fx], {
                    name: 'ItemObj_animSet', run: stack => seen.push(g.u16(stack + 8))
                });
                calls.push(seen);
                snapshots.push(Buffer.concat([bytes(g, item, ITEM_SIZE), bytes(g, g.symbol('olItemDirty'), 128),
                    bytes(g, g.symbol('olLaraWeaponState'), 2), bytes(g, g.symbol('olLaraVSpeedHack'), 2)]));
                if (g === native) {
                    nativeSteps += steps; normalCalls += seen.length;
                    if (g.u16(item + ITEM.angleY) !== before.readUInt16LE(ITEM.angleY)) rotations++;
                    if (!g.u16(g.symbol('olLaraWeaponState'))) frees++;
                    if (!fx) ends++;
                } else referenceSteps += steps;
            }
            expect(calls[0], `effect ${test} original list / NORMAL order`).toEqual(calls[1]);
            expect(snapshots[0], `effect ${test}, fx ${fx}, count ${count} full C snapshot`).toEqual(snapshots[1]);
        }
        expect(normalCalls).toBeGreaterThan(100); expect(rotations).toBeGreaterThan(100);
        expect(frees).toBeGreaterThan(100); expect(ends).toBe(256);
        expect(nativeSteps).toBeLessThan(referenceSteps);
        console.log(`[openlara] native animation commands: 2048 full C snapshots, ${normalCalls} NORMAL calls, ` +
            `${rotations} angle changes, ${frees} hands-free outcomes, ${ends} end-command lists; ` +
            `${nativeSteps}/${referenceSteps} instructions, independent $F8/$F9/$FA bank-start/end tables`);
    });

    it('copies every animation bounding box without changing adjacent bytes', () => {
        const g = movementCartridge(), ref = reference();
        const scratch = 0x7E0540;
        let count = 0;
        for (let anim = 0; anim < g.count('anims'); anim++) {
            const at = g.table('AnimBox') + anim * 12;
            const expected = Array.from({ length: 12 }, (_, i) => g.memory.read(at + i));
            for (const cart of [g, ref]) {
                if (!cart) continue;
                cart.w16(cart.item + ITEM.animIndex, anim);
                for (let i = -2; i < 14; i++) cart.w8(scratch + i, 0xA5);
                cart.call('ItemObj_getBoundingBox', [...cart.args, scratch & 65535, scratch >>> 16]);
                expect(Array.from({ length: 12 }, (_, i) => cart.memory.read(scratch + i))).toEqual(expected);
                expect([cart.u16(scratch - 2), cart.u16(scratch + 12)]).toEqual([0xA5A5, 0xA5A5]);
            }
            count++;
        }
        console.log(`[openlara] native item motion: ${count} bounding boxes, six signed words and intact canaries`);
    });
});

describe('OpenLara SNES/Super FX: bears', () => {
    // These two levels contain placed bears and their real animation data.
    // Lost Valley does not; the asset-independent state oracle below still
    // runs there. Keep fixture assertions strict on the supported levels.
    const bearLevel = LEVEL === 'LEVEL1' || LEVEL === 'LEVEL2';
    it.skipIf(!FX3 || !bearLevel)('uses current-frame bear contact for running, attacks and the upright death hit', () => {
        const g = bearCartridge(), count = g.u16(g.symbol('olHostDirectory') + 16 + HOST_COUNTS.indexOf('anims') * 2);
        const dir = count * 12, record = count * 14, box = record + 4, size = box + 12;
        const ci = g.symbol('cinfo');
        const scenarios = ['ghost','touch','moved-after-contact','invisible','inactive','no-owner',
            'dead-target','other-room','target-died-after-contact'];
        let checks = 0, captured = 0, hits = 0, pushes = 0, falls = 0;
        for (const low of [0,65536 - size]) for (const state of [3,6,7,9])
        for (const push of [0,1]) for (const scenario of scenarios) {
            g.reset(); g.pose(state); g.call('Nav_reset');
            g.w8(g.bear + ITEM.room, 0); g.w8(g.bear + ITEM.flags, 0x40);
            g.w8(g.bear + ITEM.flags2, 1 | (state === 9 ? 16 : 0));
            g.w8(g.bear + ITEM.nextActive, 255); g.set({ olFirstActive: g.slot });
            [2560,2048,2560].forEach((v, i) => g.w16(g.bear + ITEM.x + i * 2, v));
            g.w16(g.bear + ITEM.health, 20); g.w16(g.bear + ITEM.angleY, 0);
            g.w16(g.bear + ITEM.hSpeed, 0); g.w16(g.bear + ITEM.vSpeed, 0);
            g.call('Enemy_navAcquire', [...g.bearArgs, 1]);
            if (state === 9) g.w16(g.bear + ITEM.health, -1);
            if (scenario === 'no-owner') g.call('Nav_reset');
            g.w8(g.bear + ITEM.flags2, (scenario === 'invisible' ? 3 : scenario === 'inactive' ? 2 : 1) | (state === 9 ? 16 : 0));
            g.w8(g.bear + ITEM.room, scenario === 'other-room' ? 1 : 0);
            const base = 0xF80000 + low;
            for (let i = 0; i < size; i++) g.w8(base + i, 0);
            for (let a = 0; a < count; a++) [-512,512,-800,0,-512,512].forEach((v, i) => g.w16(base + a * 12 + i * 2, v));
            [-100,100,-762,0,-100,100].forEach((v, i) => g.w16(base + 103 * 12 + i * 2, v));
            g.w16(base + dir + g.animation(state) * 2, record); g.w16(base + record, 1); g.w16(base + record + 2, box);
            [-32,32,-800,0,-32,32].forEach((v, i) => g.w16(base + box + i * 2, v));
            g.w16(g.symbol('olAnimBoxData'), base); g.w16(g.symbol('olAnimBoxData') + 2, base >>> 16);
            const startX = 2560 + (scenario === 'ghost' ? 200 : 0);
            [startX,2048,2560].forEach((v, i) => g.w16(g.item + ITEM.x + i * 2, v));
            g.w16(g.item + ITEM.health, scenario === 'dead-target' ? 0 : 1000);
            g.w8(g.symbol('olEnemyTouch') + g.slot, 0);
            for (let i = 0; i < 58; i++) g.w8(ci + i, 0);
            g.w16(ci + 26, 100); g.w8(ci + 55, push);
            [startX,2048,2560].forEach((v, i) => g.w16(ci + 40 + i * 2, v));
            g.call('Enemy_collideLara', g.args);
            const touching = ['touch','moved-after-contact','target-died-after-contact'].includes(scenario);
            expect(g.memory.read(g.symbol('olEnemyTouch') + g.slot), `${scenario} contact, state ${state}, push ${push}`).toBe(+touching);
            expect(g.s16(ci + 26)).toBe(100);
            if (touching && push) {
                expect(g.s16(g.item + ITEM.x), 'bear contact did not push to the nearest original face').toBe(2528); pushes++;
            } else expect(g.s16(g.item + ITEM.x)).toBe(startX);
            if (touching) captured++;
            if (scenario === 'moved-after-contact') g.w16(g.item + ITEM.x, 3760);
            if (scenario === 'target-died-after-contact') g.w16(g.item + ITEM.health, 0);
            let info = 0;
            g.call('ItemObj_update', [...g.bearArgs, ...g.args], {
                name: 'Bear_state', run: stack => { info = g.u16(stack + 8); },
            });
            const targetDead = scenario === 'dead-target' || scenario === 'target-died-after-contact';
            const damage = state === 3 ? 3 : state === 7 ? 400 : 200;
            const expected = targetDead ? 0 : 1000 - (touching ? damage : 0);
            expect(g.s16(g.item + ITEM.health), `${scenario} damage, state ${state}, push ${push}`).toBe(expected);
            expect(Boolean(info & 16), `${scenario} captured hitMask, state ${state}`)
                .toBe(touching && (!targetDead || state === 9));
            if (touching && !targetDead) hits++;
            if (state === 9 && touching) {
                expect(g.memory.read(g.bear + ITEM.flags2) & 16, 'upright fall latch was not consumed').toBe(0); falls++;
            }
            if (touching && state !== 3) {
                g.call('ItemObj_update', [...g.bearArgs, ...g.args]);
                expect(g.s16(g.item + ITEM.health), 'one bear attack/fall damaged twice').toBe(expected);
            }
            checks++;
        }
        expect([checks,captured,hits,pushes,falls]).toEqual([144,48,32,24,12]);
        console.log(`[openlara] bear frame contact: ${checks} current-frame/ghost/eligibility/dead-target cases at CPU-only $F8 bank start/end; ` +
            `${captured} pre-push contacts, ${hits} damaging updates, ${pushes} zero-radius pushes, ${falls} single-use fall latches`);

        // Independent raw TR1 poses distinguish genuine frame misses from
        // the larger union. Include both actual death animations, not just
        // a forced state on the first death record.
        const real = bearCartridge();
        const source = JSON.parse(readFileSync(join(DATA_DIR, 'FRAMES.json'), 'utf8')) as {
            animations: { anim: number; type: number; ticks: number; rate: number; boxes: number[][] }[];
        };
        let ghosts = 0;
        const deathAnims = new Set<number>();
        const ghostsByAnim = new Map<number, number>(), ghostStates = new Set<number>();
        for (const frames of source.animations.filter(a => a.type === 8)) {
            const anim = frames.anim, state = real.memory.read(real.table('Anim') + anim * 14 + 9);
            if (![3,6,7,9].includes(state)) continue;
            if (state === 9) deathAnims.add(anim);
            const union = Array.from({ length: 6 }, (_, i) => real.s16(real.table('AnimBox') + anim * 12 + i * 2));
            const cx = (union[0] + union[1]) >> 1, cz = (union[4] + union[5]) >> 1;
            const points = [[union[0] - 100,cz],[union[1] + 100,cz],[cx,union[4] - 100],[cx,union[5] + 100]];
            for (let pose = 0; pose < frames.boxes.length && pose * frames.rate < frames.ticks; pose++) {
                const box = frames.boxes[pose];
                for (const [dx,dz] of points) {
                    if (dx >= box[0] - 100 && dx <= box[1] + 100 && dz >= box[4] - 100 && dz <= box[5] + 100) continue;
                    real.reset(); real.call('ItemObj_animSet', [...real.bearArgs, anim]);
                    real.w8(real.symbol('olEnemyNext') + real.slot, 0);
                    real.w8(real.bear + ITEM.frameIndex, pose * frames.rate);
                    real.w8(real.bear + ITEM.room, 0); real.w8(real.bear + ITEM.flags, 0x40);
                    real.w8(real.bear + ITEM.flags2, 1 | (state === 9 ? 16 : 0));
                    real.w16(real.bear + ITEM.health, state === 9 ? -1 : 20); real.w16(real.bear + ITEM.angleY, 0);
                    real.w16(real.bear + ITEM.hSpeed, 0); real.w16(real.bear + ITEM.vSpeed, 0);
                    [2560,2048,2560].forEach((v, i) => real.w16(real.bear + ITEM.x + i * 2, v));
                    [2560 + dx,2048,2560 + dz].forEach((v, i) => real.w16(real.item + ITEM.x + i * 2, v));
                    real.w16(real.item + ITEM.health, 1000);
                    real.w8(real.symbol('olEnemyTouch') + real.slot, 0); real.w8(real.symbol('olEnemyTurn') + real.slot, 0);
                    const ci = real.symbol('cinfo'); real.w16(ci + 26, 100); real.w8(ci + 55, 0);
                    real.call('Enemy_navAcquire', [...real.bearArgs, 1]);
                    real.call('ItemObj_collideBounds', [...real.bearArgs, ...real.args, 100, -real.s16(real.table('AnimBox') + 103 * 12 + 4)]);
                    expect(real.u16(real.symbol('tcc__r0')), `original bear union witness ${anim}:${pose}`).toBe(1);
                    real.call('Enemy_collideLara', real.args);
                    expect(real.memory.read(real.symbol('olEnemyTouch') + real.slot), `original bear frame witness ${anim}:${pose}`).toBe(0);
                    real.call('ItemObj_update', [...real.bearArgs, ...real.args]);
                    expect(real.s16(real.item + ITEM.health), `original-pose bear ghost hit ${anim}:${pose}`).toBe(1000);
                    ghostsByAnim.set(anim, (ghostsByAnim.get(anim) ?? 0) + 1); ghostStates.add(state);
                    ghosts++;
                }
            }
        }
        expect(ghosts, 'real bear poses did not distinguish frame contact from the union').toBeGreaterThan(10);
        expect(deathAnims.size, 'did not exercise both original bear death animations').toBe(2);
        expect([...ghostStates].sort((a, b) => a - b)).toEqual([3,6,7,9]);
        for (const anim of deathAnims)
            expect(ghostsByAnim.get(anim) ?? 0, `no ghost-contact witness for death animation ${anim}`).toBeGreaterThan(0);
        console.log(`[openlara] bear original poses: ${ghosts} raw-source running/attack/death positions reject animation-union ghost damage; both death animations checked`);
    });

    it.skipIf(LEVEL !== 'LEVEL1')('pursues Lara from its original LEVEL1 room and position', () => {
        const g = movementCartridge();
        g.restoreLevel();
        const bear = g.item + 30 * ITEM_SIZE, args = [bear & 65535, bear >>> 16];
        expect([g.memory.read(bear), g.memory.read(bear + ITEM.room), g.s16(bear + ITEM.x),
            g.s16(bear + ITEM.y), g.s16(bear + ITEM.z), g.u16(bear + ITEM.angleY)])
            .toEqual([8,17,14848,6656,-6656,49152]);
        const query = 0x7E0520;
        g.call('Room_query', [17,13312,6656,-6656,query & 65535,query >>> 16]);
        expect(g.u16(g.symbol('tcc__r0')), 'encounter target is outside the room geometry').toBe(1);
        expect(g.s16(query + 2)).not.toBe(32767);
        g.anim(103);
        g.w8(g.item + ITEM.room, g.u16(query)); g.w16(g.item + ITEM.x, 13312);
        g.w16(g.item + ITEM.y, g.s16(query + 2)); g.w16(g.item + ITEM.z, -6656);
        g.w16(g.item + ITEM.health, 1000); g.w16(g.item + ITEM.hSpeed, 0); g.w16(g.item + ITEM.vSpeed, 0);
        g.w8(bear + ITEM.flags, 0x40);
        g.w8(bear + ITEM.flags2, (g.memory.read(bear + ITEM.flags2) & ~3) | 1);
        g.w16(bear + ITEM.health, 20);
        const seen = new Set<number>();
        let tick = 0;
        while (tick++ < 420 && g.s16(g.item + ITEM.health) === 1000) {
            g.call('Lara_update', [...g.args, 0]);
            seen.add(g.memory.read(bear + ITEM.state));
            g.call('ItemObj_update', [...args, ...g.args]);
        }
        console.log(`[openlara] bear room 17: ${tick} ticks, states ${[...seen]}, ` +
            `bear (${g.s16(bear + ITEM.x)},${g.s16(bear + ITEM.y)},${g.s16(bear + ITEM.z)}), ` +
            `Lara health ${g.s16(g.item + ITEM.health)}`);
        expect(g.s16(g.item + ITEM.health), 'the original bear never reached Lara').toBeLessThan(1000);
        expect(Math.abs(g.s16(bear + ITEM.x) - 14848), 'bear never moved from its placement').toBeGreaterThan(256);
    });

    it('matches the shared C++ bear states, flags, attacks and exact random consumption', () => {
        const g = movementCartridge();
        const ref = process.env.OL_REFERENCE_ROM ? movementCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS ?? '')) : undefined;
        const mult = 0x41C64E6D;
        let inverse = 1;
        for (let i = 0; i < 5; i++) inverse = Math.imul(inverse, 2 - Math.imul(mult, inverse));
        const rolls = [0, 79, 80, 81, 767, 768, 769, 1535, 1536, 1537, 32767];
        const cases: number[][] = [];
        for (let state = 0; state <= 10; state++) for (let mood = 0; mood < 4; mood++)
        for (const info of [0,1,4,5,16,17,20,21])
        for (const dist of [0,599,600,601,767,768,769,1023,1024,1025,2047,2048,2049,32767])
        for (const next of [0,1,4,5]) {
            const n = cases.length, roll = rolls[n % rolls.length];
            const seed = Math.imul((roll << 10) - 0x3039, inverse) >>> 0;
            cases.push([state, (state + 3) % 11, mood, info, dist, next,
                n % 17 === 0 ? -1 : n % 13 === 0 ? 0 : 20,
                [1000,401,201,3,1,0,-1][(n >>> 2) % 7], 0xA5 | ((n >>> 2) & 3) << 3, 11, seed]);
        }
        const oracle = execFileSync(join(dirname(ROM), 'enemycheck'), {
            input: cases.map(c => c.join(' ')).join('\n') + '\n', maxBuffer: 4 * 1024 * 1024,
        });
        expect(oracle.length).toBe(cases.length * 16);
        let nativeSteps = 0, referenceSteps = 0;
        for (let n = 0; n < cases.length; n++) {
            const [state, goal, mood, info, dist, next, health, target, flags, turn, seed] = cases[n];
            const expected = Array.from({ length: 7 }, (_, i) => oracle.readUInt16LE(n * 16 + i * 2));
            const slot = n & 1 ? 1 : 127;
            const before = Buffer.alloc(ITEM_SIZE, 0xA5);
            before[ITEM.type] = 8; before[ITEM.state] = state; before[ITEM.goalState] = goal;
            before[ITEM.mood] = mood; before[ITEM.flags2] = flags; before.writeInt16LE(health, ITEM.health);
            const after = Buffer.from(before); after[ITEM.flags2] = expected[4];
            for (const cart of [g, ref]) {
                if (!cart) continue;
                const item = cart.item + slot * ITEM_SIZE, seedAt = cart.symbol('olRandSeed');
                before.forEach((v, i) => cart.w8(item + i, v));
                cart.w16(cart.item + ITEM.health, target);
                cart.w8(cart.symbol('olEnemyNext') + slot, next); cart.w8(cart.symbol('olEnemyTurn') + slot, turn);
                cart.w16(seedAt, seed); cart.w16(seedAt + 2, seed >>> 16);
                const steps = cart.call('Bear_state', [item & 65535, item >>> 16, info, dist]);
                if (cart === g) nativeSteps += steps; else referenceSteps += steps;
                expect([cart.u16(cart.symbol('tcc__r0')), cart.memory.read(cart.symbol('olEnemyNext') + slot),
                    cart.memory.read(cart.symbol('olEnemyTurn') + slot), cart.u16(cart.item + ITEM.health),
                    cart.memory.read(item + ITEM.flags2), cart.u16(seedAt), cart.u16(seedAt + 2)],
                    `bear state ${n}: ${cases[n].join('/')}`).toEqual(expected);
                expect(Buffer.from(Array.from({ length: ITEM_SIZE }, (_, i) => cart.memory.read(item + i))))
                    .toEqual(after);
            }
        }
        console.log(`[openlara] bear decisions: ${cases.length} exact original C++ goal/next/turn/damage/flags/PRNG cases; ` +
            `${nativeSteps} CPU instructions` + (ref ? ` vs C ${referenceSteps}` : ''));
    });

    it.skipIf(!bearLevel)('walks, runs and bites through its original animation graph', () => {
        const g = bearCartridge();
        g.pose(0); // the placed model starts standing; exercise WALK as well
        g.w16(g.item + ITEM.z, 6500); g.w16(g.item + ITEM.health, 6000);
        const seen = new Set([g.bearState()]), damage = new Set<number>();
        let fastest = 0;
        for (let tick = 0; tick < 420; tick++) {
            const before = g.bearState(), hp = g.s16(g.item + ITEM.health);
            g.step(); seen.add(g.bearState());
            fastest = Math.max(fastest, g.s16(g.bear + ITEM.hSpeed));
            const hit = hp - g.s16(g.item + ITEM.health);
            if (hit) {
                expect([3,6,7]).toContain(before);
                expect([3,200,400]).toContain(hit);
                damage.add(hit);
            }
        }
        expect([...seen]).toEqual(expect.arrayContaining([0,1,3,6]));
        expect(fastest, 'bear still uses the old fixed eleven-unit movement').toBeGreaterThan(11);
        expect([...damage], 'pursuit never reached an animated bite').toContain(200);
        console.log(`[openlara] bear pursuit: states ${[...seen]}, top speed ${fastest}, damage ${[...damage]}`);
    });

    it.skipIf(!bearLevel)('latches bite/attack damage and follows both death graphs with one upright fall hit', () => {
        for (const [state, damage] of [[6,200], [7,400]]) {
            const g = bearCartridge();
            g.pose(state); g.w16(g.bear + ITEM.z, g.s16(g.item + ITEM.z));
            let hits = 0, ticks = 0;
            while (g.bearState() === state && ticks++ < 100) {
                const hp = g.s16(g.item + ITEM.health);
                g.step();
                if (g.s16(g.item + ITEM.health) !== hp) {
                    expect(hp - g.s16(g.item + ITEM.health)).toBe(damage); hits++;
                }
            }
            expect(hits, `state ${state} did not latch one hit`).toBe(1);
            expect(ticks).toBeLessThan(100);
        }
        const animations = new Set<number>();
        for (const state of [0,1,2,3,4]) {
            const g = bearCartridge(), upright = state === 2 || state === 4;
            g.pose(state); g.w16(g.bear + ITEM.health, -1);
            if (FX3) g.w16(g.bear + ITEM.timer, 300);
            g.w16(g.item + ITEM.z, 6500);
            const trail = [g.bearState()];
            for (let i = 0; i < 240 && g.bearState() !== 9; i++) {
                g.step();
                if (trail[trail.length - 1] !== g.bearState()) trail.push(g.bearState());
            }
            expect(g.bearState(), `death graph ${state}: ${trail}`).toBe(9);
            expect(Boolean(g.memory.read(g.bear + ITEM.flags2) & 16)).toBe(upright);
            const animation = g.u16(g.bear + ITEM.animIndex);
            animations.add(animation);
            g.w16(g.item + ITEM.x, g.s16(g.bear + ITEM.x));
            g.w16(g.item + ITEM.z, g.s16(g.bear + ITEM.z));
            g.step();
            expect(g.s16(g.item + ITEM.health)).toBe(upright ? 800 : 1000);
            expect(g.memory.read(g.bear + ITEM.flags2) & 16).toBe(0);
            g.step();
            expect(g.s16(g.item + ITEM.health)).toBe(upright ? 800 : 1000);
            expect(g.u16(g.bear + ITEM.animIndex), 'death animation restarted').toBe(animation);
        }
        expect(animations.size).toBe(2);
        console.log('[openlara] bear attacks: one 200/400-point hit; both original deaths, one 200-point upright fall');
    });

    it.skipIf(!bearLevel)('uses previous-tick steering limits and the bear-specific 256-unit drop limit', () => {
        const g = bearCartridge();
        g.pose(0); g.w16(g.bear + ITEM.hSpeed, 30);
        g.w16(g.item + ITEM.x, 6500); g.w16(g.item + ITEM.z, 1536);
        pinCreatureTarget(g, g.bear);
        g.w8(g.symbol('olEnemyTurn') + g.slot, 0);
        g.step(); expect(g.u16(g.bear + ITEM.angleY)).toBe(0);
        g.step(); expect(g.u16(g.bear + ITEM.angleY)).toBe(182 * 2);
        g.pose(3); g.step(); expect(g.u16(g.bear + ITEM.angleY)).toBe(182 * 4);
        g.step(); expect(g.u16(g.bear + ITEM.angleY)).toBe(182 * 9);
        g.pose(3); g.w16(g.bear + ITEM.x, 2560); g.w16(g.bear + ITEM.z, 2560);
        g.w16(g.bear + ITEM.angleY, 0); g.w16(g.bear + ITEM.hSpeed, 100);
        g.w16(g.item + ITEM.x, 2560); g.w16(g.item + ITEM.z, 2300);
        pinCreatureTarget(g, g.bear);
        g.step(); expect(g.u16(g.bear + ITEM.angleY)).toBe((65536 - 182 * 5 / 2) & 65535);
        for (const height of [-1024,-257,-256,-65,-64,0,256,257]) {
            g.pose(1); g.w16(g.bear + ITEM.x, 2560); g.w16(g.bear + ITEM.z, 2560);
            g.w16(g.bear + ITEM.y, 2048 + height); g.w16(g.bear + ITEM.angleY, 0);
            g.w16(g.bear + ITEM.hSpeed, 0);
            g.call('Enemy_moveGround', [...g.bearArgs, 182]);
            const blocked = height < -256 || height > 256;
            expect(g.s16(g.bear + ITEM.y), `ground delta ${height}`)
                .toBe(blocked ? 2048 + height : height < -64 ? 2048 + height + 64 : 2048);
            expect(g.u16(g.bear + ITEM.angleY)).toBe(blocked ? 364 : 182);
        }
        console.log('[openlara] bear movement: previous 0/2/5-degree limits, close-reverse half turn, 256-up/256-down');
    });

    it.skipIf(!bearLevel || !process.env.OL_REFERENCE_ROM).each(FX3 ? ['Bear_update','Enemy_update'] : ['Bear_update'])(
        'matches complete C bear updates across target info, navigation, attacks and death through %s', (dispatch) => {
        const native = bearCartridge();
        const reference = bearCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const deltas = [-32768,-32767,-2049,-1025,-769,-600,-500,-1,0,1,499,500,599,768,1024,2048,32767];
        const angles = [0,1,31,32,16383,16384,16385,32767,32768,49151,49152,49153,65535];
        let nativeSteps = 0, referenceSteps = 0, turns = 0;
        for (let test = 0; test < 512; test++) {
            const snapshots: Buffer[] = [], infos: number[][] = [];
            for (const g of [native, reference]) {
                g.reset(); g.pose(test % 10);
                g.w8(g.bear + ITEM.room, 0); g.w8(g.bear + ITEM.flags, 0x40);
                g.w8(g.bear + ITEM.flags2, 1 | (test & 4) | ((test >>> 1) & 24) | (test & 32));
                g.w8(g.bear + ITEM.mood, (test >>> 2) & 3);
                g.w16(g.bear + ITEM.health, test % 13 ? 20 : test & 1 ? -1 : 0);
                if (FX3) g.w16(g.bear + ITEM.timer, 300);
                g.w16(g.bear + ITEM.x, 2560); g.w16(g.bear + ITEM.z, 2560);
                g.w16(g.bear + ITEM.y, 2048 + [-257,-256,0,256,257][test % 5]);
                const angle = angles[(test * 7) % angles.length];
                g.w16(g.bear + ITEM.angleY, angle);
                g.w16(g.bear + ITEM.hSpeed, [0,8,100,-8,-100,32767,-32768][test % 7]);
                g.w16(g.bear + ITEM.vSpeed, (test % 7 - 3) * 8);
                g.w16(g.item + ITEM.x, 2560 + deltas[test % deltas.length]);
                g.w16(g.item + ITEM.z, 2560 + deltas[(test * 7 + 3) % deltas.length]);
                g.w16(g.item + ITEM.y, 2048 + [-257,-256,-255,0,255,256,257][test % 7]);
                g.w16(g.item + ITEM.health, test % 17 ? 1000 : 0);
                g.w8(g.symbol('olEnemyTurn') + g.slot, [0,2,5][test % 3]);
                g.w8(g.symbol('olEnemyNext') + g.slot, [0,1,4,5][(test >>> 3) & 3]);
                if (FX3) g.w8(g.symbol('olEnemyTouch') + g.slot, (test >>> 2) & 1);
                const seed = (Math.imul(test + 1, 1664525) + 1013904223) >>> 0;
                g.w16(g.symbol('olRandSeed'), seed); g.w16(g.symbol('olRandSeed') + 2, seed >>> 16);
                g.call('Room_reset'); g.call('navResetCache');
                if (FX3) g.call('Nav_reset');
                if (FX3 && (test & 1)) {
                    g.call('Enemy_navAcquire', [...g.bearArgs, 1]);
                    g.w8(g.symbol('olEnemyTurn') + g.slot, [0,2,5][test % 3]);
                }
                let info: number[] = [];
                const steps = g.call(dispatch, [...g.bearArgs, ...g.args], {
                    name: 'Bear_state', run: stack => { info = [g.u16(stack + 8), g.u16(stack + 10)]; },
                });
                if (g === native) {
                    nativeSteps += steps;
                    if (g.u16(g.bear + ITEM.angleY) !== angle) turns++;
                } else referenceSteps += steps;
                infos.push(info);
                const bytes = (at: number, size: number) => Array.from({ length: size }, (_, i) => g.memory.read(at + i));
                snapshots.push(Buffer.from(bytes(g.bear, ITEM_SIZE).concat(bytes(g.item, ITEM_SIZE),
                    bytes(g.symbol('olEnemyTurn') + g.slot, 1), bytes(g.symbol('olEnemyNext') + g.slot, 1),
                    bytes(g.symbol('olRandSeed'), 4), bytes(g.symbol('olEnemyInZone'), 2),
                    FX3 ? bytes(g.symbol('olNav'), 8240).concat(bytes(g.symbol('olEnemyTouch') + g.slot, 1)) : [])));
            }
            expect(infos[0], `bear target info ${test}`).toEqual(infos[1]);
            expect(snapshots[0], `complete bear update ${test}, state ${test % 10}`).toEqual(snapshots[1]);
        }
        expect(turns, 'the oracle never exercised steering').toBeGreaterThan(20);
        expect(nativeSteps).toBeLessThan(referenceSteps);
        console.log(`[openlara] native bear update (${dispatch}): 512 exact C target-info/item/Lara/PRNG${FX3 ? '/Nav' : ''} snapshots; ` +
            `${turns} nonzero turns; ${nativeSteps} CPU instructions vs C ${referenceSteps} (${(100 * nativeSteps / referenceSteps).toFixed(1)}%)`);
    });
});

describe.skipIf(!FX3)('OpenLara FX3: frame contact', () => {
    it('reads every original current-frame box directly from banked CPU ROM', () => {
        const g = movementCartridge();
        const source = JSON.parse(readFileSync(join(DATA_DIR, 'FRAMES.json'), 'utf8')) as {
            animations: { anim: number; type: number; begin: number; end: number; rate: number; ticks: number; boxes: number[][] }[];
        };
        const actor = g.item + ITEM_SIZE, args = [actor & 65535, actor >>> 16];
        let ticks = 0, changed = 0;
        const readBox = () => {
            g.call('ItemObj_frameBox', args);
            const p = g.u16(g.symbol('tcc__r0')) | g.u16(g.symbol('tcc__r0h')) << 16;
            expect(p).toBeGreaterThanOrEqual(0xE00000); expect(p).toBeLessThan(0x1000000);
            return Array.from({ length: 6 }, (_, i) => g.s16(p + i * 2));
        };
        for (const a of source.animations) {
            g.w16(actor + ITEM.animIndex, a.anim);
            const union = Array.from({ length: 6 }, (_, i) => g.s16(g.table('AnimBox') + a.anim * 12 + i * 2));
            for (let tick = 0; tick < a.ticks; tick++) {
                const lower = Math.floor(tick / a.rate), choices = [lower];
                if ((lower + 1) * a.rate < a.end) choices.push(lower + 1);
                choices.sort((x, y) => Math.abs(x * a.rate - tick) - Math.abs(y * a.rate - tick) || x - y);
                const box = a.boxes[choices[0]];
                g.w8(actor + ITEM.frameIndex, tick);
                expect(readBox(), `frame box ${a.anim}:${tick}`).toEqual(box);
                changed += box.some((v, i) => v !== union[i]) ? 1 : 0; ticks++;
            }
            g.w8(actor + ITEM.frameIndex, 255);
            expect(readBox(), `invalid frame ${a.anim}:255 did not use the union`).toEqual(union);
        }
        expect(source.animations.some(a => a.type === 0)).toBe(true);
        expect(changed).toBeGreaterThan(1000);
        const descriptor = g.symbol('olHostDirectory') + 40 + 4 * 12;
        const size = g.u16(descriptor + 4) | g.u16(descriptor + 6) << 16;
        const payload = Array.from({ length: size }, (_, i) => g.memory.read(g.table('AnimBox') + i));
        let relocated = 0;
        for (const low of [0, 65536 - size]) {
            const base = 0xF80000 + low;
            payload.forEach((v, i) => g.w8(base + i, v));
            g.w16(g.symbol('olAnimBoxData'), base); g.w16(g.symbol('olAnimBoxData') + 2, base >>> 16);
            for (const a of source.animations) for (const tick of [0, Math.floor(a.ticks / 2), a.ticks - 1]) {
                const lower = Math.floor(tick / a.rate), choices = [lower];
                if ((lower + 1) * a.rate < a.end) choices.push(lower + 1);
                choices.sort((x, y) => Math.abs(x * a.rate - tick) - Math.abs(y * a.rate - tick) || x - y);
                g.w16(actor + ITEM.animIndex, a.anim); g.w8(actor + ITEM.frameIndex, tick);
                expect(readBox(), `CPU-only frame box ${low}/${a.anim}:${tick}`).toEqual(a.boxes[choices[0]]);
                expect(g.u16(g.symbol('tcc__r0h'))).toBe(0xF8); relocated++;
            }
        }
        console.log(`[openlara] frame boxes: ${ticks} raw-source nearest-pose checks, ${changed} differ from unions; ` +
            `${relocated} CPU-only $F8 bank-start/end reads, no WRAM copy`);
    });

    it('matches original adjacent-room lists through the FX3 renderer ROM mapping', () => {
        const g = movementCartridge(); g.restoreLevel();
        const { rooms } = JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8')) as {
            rooms: { portals: number[] }[];
        };
        let checks = 0;
        for (let room = 0; room < rooms.length; room++) for (let other = 0; other < rooms.length + 2; other++) {
            g.call('Room_isAdjacent', [room, other]);
            expect(g.u16(g.symbol('tcc__r0')), `room adjacency ${room}/${other}`)
                .toBe(room === other || rooms[room].portals.includes(other) ? 1 : 0);
            checks++;
        }
        g.call('Room_isAdjacent', [65535, 65535]); expect(g.u16(g.symbol('tcc__r0'))).toBe(0);
        console.log(`[openlara] frame contact: ${checks} raw-source room adjacency checks, no WRAM portal copy`);
    });

    it('retains full-width rotation, vertical edges and nearest-face push ties', () => {
        const g = movementCartridge(), actor = g.item + ITEM_SIZE, args = [actor & 65535, actor >>> 16];
        const word = (v: number) => v << 16 >> 16;
        // Explicit union fallback gives controlled extremes without replacing
        // the runtime lookup or the original-data test above.
        g.w16(actor + ITEM.animIndex, 0); g.w16(g.item + ITEM.animIndex, 1);
        g.w8(actor + ITEM.frameIndex, 255); g.w8(g.item + ITEM.frameIndex, 255);
        const boxes = [[-200,200,-800,10,-900,900], [-32768,32767,-32768,32767,-32768,32767],
            [-1,1,-1,1,-1,1], [0,0,0,0,0,0], [-200,200,-600,-500,-200,200]];
        const edges = [-32768,-32767,-1000,-901,-900,-201,-200,-1,0,1,200,201,900,901,32767];
        let hits = 0, pushes = 0, fusedSteps = 0, separateSteps = 0;
        for (let test = 0; test < 1200; test++) {
            const a = boxes[test % boxes.length], b = boxes[(test * 3 + 1) % boxes.length];
            a.concat(b).forEach((v, i) => g.w16(g.table('AnimBox') + i * 2, v));
            const pos = [32700,-300,-32700];
            const d = [edges[test % edges.length], edges[(test * 7 + 5) % edges.length], edges[(test * 11 + 3) % edges.length]];
            const yaw = [0,32,16384,32768,49152,8192,65535][test % 7];
            const radius = [0,100,341,-1,32767,-32768][(test >>> 2) % 6];
            const theta = (((yaw + 32) & 65535) >>> 6) * Math.PI / 512;
            const s = Math.round(Math.sin(theta) * 16384), c = Math.round(Math.cos(theta) * 16384);
            let px = Math.floor((d[0] * c - d[2] * s) / 16384), pz = Math.floor((d[2] * c + d[0] * s) / 16384);
            const hit = a[3] - b[2] > d[1] && a[2] - b[3] < d[1] &&
                px >= a[0] - radius && px <= a[1] + radius && pz >= a[4] - radius && pz <= a[5] + radius;
            pos.forEach((v, i) => { g.w16(actor + ITEM.x + i * 2, v); g.w16(g.item + ITEM.x + i * 2, v + d[i]); });
            g.w16(actor + ITEM.angleY, yaw);
            const boundsSteps = g.call('ItemObj_collideFrameBounds', [...args, ...g.args, radius]);
            expect(g.u16(g.symbol('tcc__r0')), `frame contact ${test}`).toBe(hit ? 1 : 0); hits += +hit;
            const push = px >= a[0] && px <= a[1] && pz >= a[4] && pz <= a[5];
            if (push) {
                const distances = [px - a[0], a[1] - px, pz - a[4], a[5] - pz];
                const face = distances.indexOf(Math.min(...distances));
                if (face < 2) px = a[face]; else pz = a[face + 2];
            }
            const pushSteps = g.call('ItemObj_pushFrame', [...args, ...g.args]);
            expect(g.u16(g.symbol('tcc__r0')), `push entry ${test}`).toBe(+push);
            const want = push ? [word(pos[0] + Math.floor((px * c + pz * s) / 16384)),
                word(pos[2] + Math.floor((pz * c - px * s) / 16384))] : [word(pos[0] + d[0]), word(pos[2] + d[2])];
            expect([g.s16(g.item + ITEM.x), g.s16(g.item + ITEM.z)], `frame push ${test}`).toEqual(want);
            for (const enabled of [0,1,-1]) {
                pos.forEach((v, i) => g.w16(g.item + ITEM.x + i * 2, v + d[i]));
                const steps = g.call('ItemObj_frameContact', [...args, ...g.args, radius, enabled]);
                expect(g.u16(g.symbol('tcc__r0')), `fused contact ${test}/${enabled}`)
                    .toBe(hit ? enabled && push ? 3 : 1 : 0);
                expect([g.s16(g.item + ITEM.x), g.s16(g.item + ITEM.z)], `fused push ${test}/${enabled}`)
                    .toEqual(hit && enabled ? want : [word(pos[0] + d[0]), word(pos[2] + d[2])]);
                if (enabled === 1) { fusedSteps += steps; separateSteps += boundsSteps + (hit ? pushSteps : 0); }
            }
            pushes += +push;
        }
        expect(hits).toBeGreaterThan(50); expect(pushes).toBeGreaterThan(50);
        console.log(`[openlara] frame contact: 1200 full-width bound/push cases, ${hits} hits and ${pushes} pushes`);
        if (SYMS.has('CF_Rotate')) expect(fusedSteps).toBeLessThan(separateSteps);
        console.log(`[openlara] fused frame contact: 3600 exact bound/push/disabled cases, ` +
            `${fusedSteps}/${separateSteps} instructions vs separate native-helper C paths`);
    });

    it("turns a creature's push into upstream's hit quadrant, timer and drawn animation", () => {
        const g = movementCartridge(), actor = g.item + ITEM_SIZE, args = [actor & 65535, actor >>> 16];
        const ci = g.symbol('cinfo'), q = g.symbol('olLaraHitQuadrant');
        const timer = g.symbol('olLaraHitTimer'), frame = g.symbol('olLaraHitFrame');
        const word = (v: number) => v << 16 >> 16;
        const atan = (x: number, y: number) => {
            let ax = Math.abs(x), ay = Math.abs(y);
            if (!ax && !ay) return 0;
            const swapped = ay > ax;
            if (swapped) [ax, ay] = [ay, ax];
            let a = Math.round(Math.atan(Math.floor(ay * 128 / ax) / 128) * 65536 / (2 * Math.PI));
            if (swapped) a = 0x4000 - a;
            if (y < 0) a = -a;
            if (x < 0) a = 0x8000 - a;
            return a & 0xFFFF;
        };
        // The actor's union box (frame 255) is anim 0's, written in place.
        g.w16(actor + ITEM.animIndex, 0); g.w8(actor + ITEM.frameIndex, 255);
        const boxes = [[-200,200,-800,10,-900,900], [-100,300,-257,0,-50,700], [-100,300,-256,0,-50,700],
            [-512,-12,-900,0,40,520], [-32768,32767,-800,0,-32768,32767]];
        // A push means she stood inside the box, so offsets are box-sized;
        // phd_atan is exact over signed words, which is all that can reach it.
        const offsets = [-9000,-5000,-901,-300,-1,0,1,150,700,4095,9000];
        let hits = 0, skipped = 0;
        const quadrants = new Set<number>();
        for (let test = 0; test < 1500; test++) {
            const a = boxes[test % boxes.length];
            a.forEach((v, i) => g.w16(g.table('AnimBox') + i * 2, v));
            const yaw = [0,32,16384,32768,49152,8192,65535,12345][test % 8];
            const facing = [0,16384,32768,49152,8191,8192,57344,40000,1][(test >>> 3) % 9];
            const enemyHit = (test >>> 1) % 7 ? 1 : 0;
            const pos = [[1000,0,-2000],[32000,-300,-32000],[-4096,0,4096]][test % 3];
            const d = [offsets[test % offsets.length], offsets[(test * 7 + 3) % offsets.length]];
            pos.forEach((v, i) => g.w16(actor + ITEM.x + i * 2, v));
            g.w16(actor + ITEM.angleY, yaw); g.w16(g.item + ITEM.angleY, facing);
            g.w8(ci + 56, enemyHit); g.w8(q, 0); g.w8(timer, 0);
            const x = word(pos[0] + d[0]), z = word(pos[2] + d[1]);
            g.call('Lara_enemyHit', [...args, x & 65535, z & 65535]);
            if (!enemyHit || a[3] - a[2] <= 256) {
                expect([g.memory.read(q), g.memory.read(timer)], `no hit ${test}`).toEqual([0, 0]); skipped++;
                continue;
            }
            const theta = (((yaw + 32) & 65535) >>> 6) * Math.PI / 512;
            const s = -Math.round(Math.sin(theta) * 16384), c = Math.round(Math.cos(theta) * 16384);
            const cx = (a[0] + a[1]) >> 1, cz = (a[4] + a[5]) >> 1;
            const dx = word(x - pos[0]) - Math.floor((cx * c - cz * s) / 16384);
            const dz = word(z - pos[2]) - Math.floor((cz * c + cx * s) / 16384);
            const want = ((facing - atan(dz, dx) + 0x8000 + 0x2000) & 0xFFFF) >>> 14;
            // The push records and marks it pending; the publish resolves it.
            expect([g.memory.read(q), g.memory.read(timer)], `hit pending ${test}`).toEqual([0xFF, 5]);
            g.w16(actor + ITEM.angleY, yaw ^ 0x5A5A); g.w16(g.item + ITEM.angleY, facing ^ 0x1234);
            g.call('Lara_hitResolve');
            expect(g.memory.read(q), `hit quadrant ${test}: yaw ${yaw} facing ${facing} d ${dx},${dz}`).toBe(want + 1);
            g.call('Lara_hitResolve');
            expect(g.memory.read(q), 'resolving twice changed it').toBe(want + 1);
            quadrants.add(want); hits++;
        }
        expect([...quadrants].sort()).toEqual([0,1,2,3]);
        expect(skipped).toBeGreaterThan(100);

        // Lara::updateCollision's tail: the hit sound on the first frame,
        // hitFrame to 34 at most, both cleared when the timer runs out; dead
        // clears the quadrant (updateObjectsCollision) but not the timer.
        g.w16(g.item + ITEM.health, 1000);
        g.w8(q, 3); g.w8(timer, 5); g.w8(frame, 0);
        const trace: number[][] = [];
        for (let i = 0; i < 6; i++) {
            g.call('Lara_hitTick');
            trace.push([g.memory.read(q), g.memory.read(timer), g.memory.read(frame)]);
        }
        expect(trace).toEqual([[3,4,1],[3,3,2],[3,2,3],[3,1,4],[0,0,0],[0,0,0]]);
        g.w8(q, 1); g.w8(timer, 3); g.w8(frame, 33);
        for (let i = 0; i < 2; i++) g.call('Lara_hitTick');
        expect([g.memory.read(q), g.memory.read(timer), g.memory.read(frame)]).toEqual([1, 1, 34]);
        g.w16(g.item + ITEM.health, 0);
        g.call('Lara_hitTick');
        expect([g.memory.read(q), g.memory.read(timer), g.memory.read(frame)]).toEqual([0, 0, 0]);

        // Lara::draw: ANIM_HIT_FRONT/LEFT/BACK/RIGHT at hitFrame, over what
        // gameRenderSync published; nothing while there is no quadrant.
        const source = JSON.parse(readFileSync(join(DATA_DIR, 'FRAMES.json'), 'utf8')) as {
            animations: { anim: number; rate: number }[];
        };
        const rate = (anim: number) => source.animations.find(a => a.anim === anim)?.rate ??
            g.memory.read(g.table('Anim') + anim * 14 + 1);
        const state = g.symbol('ol_itemState_gsu');
        g.w16(g.symbol('ol_animFreeze'), 0);
        for (const [quadrant, anim] of [[1,125],[2,127],[3,126],[4,128]]) for (const f of [0,1,17,34]) {
            g.w8(state, 11); g.w8(state + 1, 0xAA); g.w8(q, quadrant); g.w8(frame, f);
            g.call('olLaraHitPublish');
            expect([g.memory.read(state), g.memory.read(state + 1)], `hit draw ${quadrant}/${f}`)
                .toEqual([anim, Math.floor(f / rate(anim))]);
        }
        g.w8(state, 11); g.w8(state + 1, 0xAA); g.w8(q, 0);
        g.call('olLaraHitPublish');
        expect([g.memory.read(state), g.memory.read(state + 1)]).toEqual([11, 0xAA]);
        console.log(`[openlara] hit reactions: ${hits} pushes named a quadrant (all four), ${skipped} short boxes or ` +
            `no enemyHit left none; timer, 34-frame clamp, death and the drawn HIT animations match upstream`);
    });
});

describe.skipIf(!FX3)('OpenLara FX3: raptors', () => {
    it.skipIf(LEVEL !== 'LEVEL3A')('reaches Lara from its original Lost Valley room and placement', () => {
        const g = movementCartridge(); g.restoreLevel();
        const actor = g.item + 42 * ITEM_SIZE, args = [actor & 65535, actor >>> 16];
        expect([g.memory.read(actor), g.memory.read(actor + ITEM.room), g.s16(actor + ITEM.x),
            g.s16(actor + ITEM.y), g.s16(actor + ITEM.z), g.u16(actor + ITEM.angleY)])
            .toEqual([19,51,-15872,3328,-31232,49152]);
        // restoreLevel isolates table loading; gameUpdate normally applies
        // species constructor health immediately after olLevelInit.
        g.call('Enemy_health', [19]); expect(g.u16(g.symbol('tcc__r0'))).toBe(20);
        g.w16(actor + ITEM.health, 20);
        const query = 0x7E0520;
        g.call('Room_query', [51,-17920,3328,-31232,query & 65535,query >>> 16]);
        expect(g.u16(g.symbol('tcc__r0'))).toBe(1); expect(g.s16(query + 2)).not.toBe(32767);
        g.anim(103); g.w8(g.item + ITEM.room, g.u16(query));
        g.w16(g.item + ITEM.x, -17920); g.w16(g.item + ITEM.z, -31232);
        g.w16(g.item + ITEM.y, g.s16(query + 2)); g.w16(g.item + ITEM.health, 1000);
        g.w8(actor + ITEM.flags, 0x40);
        g.w8(actor + ITEM.flags2, (g.memory.read(actor + ITEM.flags2) & ~3) | 1);
        const seen = new Set<number>(); let tick = 0;
        while (tick++ < 420 && g.s16(g.item + ITEM.health) === 1000) {
            g.call('Lara_update', [...g.args, 0]); seen.add(g.memory.read(actor + ITEM.state));
            g.call('ItemObj_update', [...args, ...g.args]);
        }
        console.log(`[openlara] Lost Valley room 51 raptor: ${tick} ticks, states ${[...seen]}, ` +
            `position (${g.s16(actor + ITEM.x)},${g.s16(actor + ITEM.y)},${g.s16(actor + ITEM.z)}) ` +
            `room ${g.memory.read(actor + ITEM.room)} mood ${g.memory.read(actor + ITEM.mood)}, ` +
            `Lara (${g.s16(g.item + ITEM.x)},${g.s16(g.item + ITEM.y)},${g.s16(g.item + ITEM.z)}), health ${g.s16(g.item + ITEM.health)}`);
        expect(g.s16(g.item + ITEM.health), 'the originally placed raptor never reached Lara').toBeLessThan(1000);
        expect(Math.abs(g.s16(actor + ITEM.x) + 15872)).toBeGreaterThan(256);
    });

    it('matches original C++ states, attack latches and exact random consumption', () => {
        const g = movementCartridge();
        const ref = process.env.OL_REFERENCE_ROM ? movementCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS ?? '')) : undefined;
        let inverse = 1;
        for (let i = 0; i < 5; i++) inverse = Math.imul(inverse, 2 - Math.imul(0x41C64E6D, inverse));
        const rolls = [0,255,256,257,8191,8192,8193,32767], cases: number[][] = [];
        for (let state = 0; state <= 9; state++) for (let mood = 0; mood < 4; mood++)
        for (const info of [0,1,4,5,16,17,20,21])
        for (const dist of [0,679,680,681,1535,1536,1537,32767])
        for (const next of [0,1,3,6]) for (const goal of [3,(state + 2) % 9]) {
            const n = cases.length;
            const seed = Math.imul((rolls[(n >>> 1) % rolls.length] << 10) - 0x3039, inverse) >>> 0;
            cases.push([state, goal, mood, info, dist, next, n % 17 ? 20 : n & 1 ? -1 : 0,
                [1000,101,100,99,1,0,-1][(n >>> 2) % 7], 0xA5 | ((n >>> 3) & 3) << 3, 11, seed]);
        }
        const oracle = execFileSync(join(dirname(ROM), 'raptorcheck'), {
            input: cases.map(c => c.join(' ')).join('\n') + '\n', maxBuffer: 4 * 1024 * 1024,
        });
        expect(oracle.length).toBe(cases.length * 16);
        let nativeSteps = 0, referenceSteps = 0;
        for (let n = 0; n < cases.length; n++) {
            const [state, goal, mood, info, dist, next, health, target, flags, turn, seed] = cases[n];
            const want = Array.from({ length: 7 }, (_, i) => oracle.readUInt16LE(n * 16 + i * 2));
            const slot = n & 1 ? 1 : 127, before = Buffer.alloc(ITEM_SIZE, 0xA5);
            before[ITEM.type] = 19; before[ITEM.state] = state; before[ITEM.goalState] = goal;
            before[ITEM.mood] = mood; before[ITEM.flags2] = flags; before.writeInt16LE(health, ITEM.health);
            for (const cart of [g, ref]) {
                if (!cart) continue;
                const actor = cart.item + slot * ITEM_SIZE, seedAt = cart.symbol('olRandSeed');
                before.forEach((v, i) => cart.w8(actor + i, v)); cart.w16(cart.item + ITEM.health, target);
                cart.w8(cart.symbol('olEnemyNext') + slot, next); cart.w8(cart.symbol('olEnemyTurn') + slot, turn);
                cart.w16(seedAt, seed); cart.w16(seedAt + 2, seed >>> 16);
                const steps = cart.call('Raptor_state', [actor & 65535, actor >>> 16, info, dist]);
                if (cart === g) nativeSteps += steps; else referenceSteps += steps;
                expect([cart.u16(cart.symbol('tcc__r0')), cart.memory.read(cart.symbol('olEnemyNext') + slot),
                    cart.memory.read(cart.symbol('olEnemyTurn') + slot), cart.u16(cart.item + ITEM.health),
                    cart.memory.read(actor + ITEM.flags2), cart.u16(seedAt), cart.u16(seedAt + 2)],
                    `raptor state ${n}: ${cases[n].join('/')}`).toEqual(want);
                expect(Buffer.from(Array.from({ length: ITEM_SIZE }, (_, i) => cart.memory.read(actor + i))))
                    .toEqual(before);
            }
        }
        console.log(`[openlara] raptor decisions: ${cases.length} original C++ goal/next/turn/damage/flags/PRNG cases; ` +
            `${nativeSteps} instructions` + (ref ? ` vs C ${referenceSteps}` : ''));
    });

    it.skipIf(LEVEL !== 'LEVEL3A')('pursues and bites through the actual Lost Valley animation graph', () => {
        const g = raptorCartridge();
        g.pose(2); g.w16(g.item + ITEM.z, 6500); g.w16(g.item + ITEM.health, 6000);
        const seen = new Set([g.raptorState()]);
        let fastest = 0, hits = 0;
        for (let tick = 0; tick < 420; tick++) {
            const before = g.raptorState(), hp = g.s16(g.item + ITEM.health);
            g.step(); seen.add(g.raptorState()); fastest = Math.max(fastest, g.s16(g.raptor + ITEM.hSpeed));
            const damage = hp - g.s16(g.item + ITEM.health);
            if (damage) { expect([4,7,8]).toContain(before); expect(damage).toBe(100); hits++; }
        }
        expect([...seen]).toEqual(expect.arrayContaining([1,2,3]));
        expect(fastest).toBeGreaterThan(20); expect(hits).toBeGreaterThan(1);
        console.log(`[openlara] Lost Valley raptor pursuit: states ${[...seen]}, top speed ${fastest}, ${hits} bites`);
    });

    it.skipIf(LEVEL !== 'LEVEL3A')('latches all three attacks and selects both deaths on the lethal weapon hit', () => {
        for (const state of [4,7,8]) {
            const g = raptorCartridge();
            g.pose(state); g.w16(g.raptor + ITEM.z, g.s16(g.item + ITEM.z) - 400);
            let hits = 0, ticks = 0;
            while (g.raptorState() === state && ticks++ < 120) {
                const hp = g.s16(g.item + ITEM.health); g.step();
                if (g.s16(g.item + ITEM.health) !== hp) { expect(hp - g.s16(g.item + ITEM.health)).toBe(100); hits++; }
            }
            expect(hits, `attack ${state} did not latch exactly one bite`).toBe(1);
            expect(ticks).toBeLessThan(120);
        }
        const deaths = new Set<number>();
        for (const seed of [0,1,2,3,0x7FFFFFFF,0xFFFFFFFF]) {
            const g = raptorCartridge(); g.pose(3);
            const seedAt = g.symbol('olRandSeed'), next = g.symbol('olEnemyNext') + g.slot;
            g.w16(seedAt, seed); g.w16(seedAt + 2, seed >>> 16); g.w8(next, 6);
            g.set({ olWeaponTarget: g.slot, olWeaponShots: 0 });
            for (let shot = 0; shot < 19; shot++) g.call('Weapon_fire', [1]);
            expect(g.s16(g.raptor + ITEM.health)).toBe(1); expect(g.raptorState()).toBe(3);
            expect((g.u16(seedAt) | g.u16(seedAt + 2) << 16) >>> 0).toBe(seed);
            g.call('Weapon_fire', [1]);
            const rolled = (Math.imul(seed, 0x41C64E6D) + 0x3039) >>> 0;
            const anim = g.u16(g.raptor + ITEM.animIndex);
            expect(anim).toBe(g.first + 9 + ((rolled >>> 24) & 1)); deaths.add(anim);
            expect(g.s16(g.raptor + ITEM.health)).toBe(0); expect(g.raptorState()).toBe(0);
            expect(g.memory.read(g.raptor + ITEM.flags2) & 8).toBe(8);
            expect(g.memory.read(next), 'original lethal animSet must preserve nextState').toBe(6);
            g.call('Weapon_fire', [1]); g.call('Raptor_hit', [...g.raptorArgs, 100]);
            expect((g.u16(seedAt) | g.u16(seedAt + 2) << 16) >>> 0).toBe(rolled);
            expect(g.u16(g.raptor + ITEM.animIndex)).toBe(anim);
            for (let tick = 0; tick < 301; tick++) g.step();
            expect(g.u16(g.raptor + ITEM.animIndex), 'corpse restarted its death animation').toBe(anim);
            expect(g.memory.read(g.raptor + ITEM.flags) & 0x40, 'death did not remove the active actor').toBe(0);
            expect(g.s16(g.item + ITEM.health), 'dead raptor dealt contact damage').toBe(1000);
        }
        expect(deaths.size).toBe(2);
        console.log('[openlara] raptor attacks: three single-hit latches; both lethal-hit deaths, one PRNG roll, inactive corpses');
    });

    it.skipIf(LEVEL !== 'LEVEL3A')('uses previous-tick 1/4-degree steering and a 256-unit drop limit', () => {
        const g = raptorCartridge();
        g.pose(2); g.w16(g.raptor + ITEM.hSpeed, 30);
        g.w16(g.item + ITEM.x, 6500); g.w16(g.item + ITEM.z, 1536);
        pinCreatureTarget(g, g.raptor); g.w8(g.symbol('olEnemyTurn') + g.slot, 0);
        g.step(); expect(g.u16(g.raptor + ITEM.angleY)).toBe(0);
        g.step(); expect(g.u16(g.raptor + ITEM.angleY)).toBe(182);
        g.pose(3); g.step(); expect(g.u16(g.raptor + ITEM.angleY)).toBe(364);
        g.step(); expect(g.u16(g.raptor + ITEM.angleY)).toBe(1092);
        for (const height of [-1024,-257,-256,-65,-64,0,256,257]) {
            g.pose(1); g.w16(g.raptor + ITEM.x, 2560); g.w16(g.raptor + ITEM.z, 2560);
            g.w16(g.raptor + ITEM.y, 2048 + height); g.w16(g.raptor + ITEM.angleY, 0);
            g.w16(g.raptor + ITEM.hSpeed, 0); g.call('Enemy_moveGround', [...g.raptorArgs, 182]);
            const blocked = height < -256 || height > 256;
            expect(g.s16(g.raptor + ITEM.y), `raptor ground delta ${height}`)
                .toBe(blocked ? 2048 + height : height < -64 ? 2048 + height + 64 : 2048);
            expect(g.u16(g.raptor + ITEM.angleY)).toBe(blocked ? 364 : 182);
        }
    });
});

describe.skipIf(!FX3 || LEVEL !== 'LEVEL3A' || !process.env.OL_REFERENCE_ROM)('OpenLara FX3: raptors native', () => {
    it('matches ordered multi-owner contacts with far Lara pointers and CPU-only frame ROM', () => {
        const native = raptorCartridge();
        const reference = raptorCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const permutations = [[1,2,3], [1,3,2], [2,1,3], [2,3,1], [3,1,2], [3,2,1]];
        const speciesOrders = new Set<string>();
        const edges = [-4096,-400,-180,-60,0,60,180,400,4095,4096];
        const clone = (g: typeof native, at: number, n: number) => Array.from({ length: n }, (_, i) => g.memory.read(at + i));
        const payloads = [native, reference].map(g => {
            const desc = g.symbol('olHostDirectory') + 40 + 4 * 12;
            return clone(g, g.table('AnimBox'), g.u16(desc + 4) | g.u16(desc + 6) << 16);
        });
        let contacts = 0, moved = 0, nativeSteps = 0, referenceSteps = 0;
        for (let test = 0; test < 960; test++) {
            const snapshots: Buffer[] = [];
            for (const [index, g] of [native, reference].entries()) {
                g.reset(); g.set({ itemsCount: 4 });
                // Full table at each end of FX3 CPU-only banks; no alias
                // through the FX's smaller ROMBR range is valid here.
                const payload = payloads[index], base = test & 1 ? 0xFA0000 - payload.length : 0xF80000;
                if (test < 2) payload.forEach((v, i) => g.w8(base + i, v));
                g.w16(g.symbol('olAnimBoxData'), base); g.w16(g.symbol('olAnimBoxData') + 2, base >>> 16);
                const lara = test % 3 ? g.item : 0x7FFFE0; // final complete item in bank $7F
                if (lara !== g.item) clone(g, g.item, ITEM_SIZE).forEach((v, i) => g.w8(lara + i, v));
                const origin = test % 11 ? 2560 : 32700;
                [origin,2048,origin].forEach((v, i) => g.w16(lara + ITEM.x + 2 * i, v));
                g.w16(lara + ITEM.health, test % 17 ? 1000 : -1);
                const prototype = clone(g, g.raptor, ITEM_SIZE);
                for (let owner = 1; owner <= 3; owner++) {
                    const item = g.item + owner * ITEM_SIZE;
                    prototype.forEach((v, i) => g.w8(item + i, v));
                    g.call('ItemObj_animSetFrame', [item & 65535,item >>> 16,
                        g.animation([1,3,4,7,8][(test + owner) % 5]), (test * owner) % 20]);
                    // Every three-species combination and every owner order.
                    // Unported types remain excluded. Geometry is a separate
                    // current-frame fixture, irrespective of species type.
                    const species = [[7,8,19],[7,8,9],[7,9,19],[8,9,19]][Math.floor(test / 240)];
                    g.w8(item + ITEM.type, test % 13 || owner !== 2 ? species[owner - 1] : 123);
                    g.w8(item + ITEM.flags2, test % 19 ? 1 : owner);
                    g.w8(item + ITEM.room, test % 23 || owner !== 3 ? 0 : 1);
                    g.w16(item + ITEM.x, origin + edges[(test + owner) % edges.length]);
                    g.w16(item + ITEM.y, 2048 + (test % 7 ? 0 : 4096));
                    g.w16(item + ITEM.z, origin + edges[(test * 3 + owner * 2) % edges.length]);
                    g.w16(item + ITEM.angleY, test * 1337 + owner * 8192);
                }
                permutations[test % 6].forEach((owner, i) =>
                    g.w16(g.symbol('olNav') + i * 8240 + 8214, test % 29 || i !== 1 ? owner : [4,65535][test & 1]));
                for (let i = 0; i < 128; i++) g.w8(g.symbol('olEnemyTouch') + i, (i + test) & 1);
                const ci = g.symbol('cinfo');
                for (let i = 0; i < 58; i++) g.w8(ci + i, 0);
                g.w16(ci + 26, [0,100,341,32767,-1,-32768][test % 6]);
                g.w16(ci + 40, origin); g.w16(ci + 42, 2048); g.w16(ci + 44, origin);
                g.w8(ci + 55, (test >>> 1) & 1); g.w16(ci + 46, test * 1777);
                g.sector(0, 2, 2, test % 31 ? 2048 : -127 * 256, -2048);
                g.call('Room_reset');
                const steps = g.call('Enemy_collideLara', [lara & 65535,lara >>> 16]);
                snapshots.push(Buffer.from(clone(g, g.item, ITEM_SIZE * 4).concat(clone(g, lara, ITEM_SIZE),
                    clone(g, ci, 58), clone(g, g.symbol('olEnemyTouch'), 128))));
                if (g === native) {
                    nativeSteps += steps;
                    if (test % 13) speciesOrders.add(permutations[test % 6].map(owner =>
                        g.memory.read(g.item + owner * ITEM_SIZE + ITEM.type)).join('/'));
                    contacts += permutations[test % 6].filter(owner => g.memory.read(g.symbol('olEnemyTouch') + owner)).length;
                    moved += g.s16(lara + ITEM.x) !== origin || g.s16(lara + ITEM.z) !== origin ? 1 : 0;
                } else referenceSteps += steps;
            }
            expect(snapshots[0], `multi-owner contact ${test}, order ${permutations[test % 6]}`).toEqual(snapshots[1]);
        }
        expect(contacts).toBeGreaterThan(20); expect(moved).toBeGreaterThan(5);
        expect(speciesOrders.size, 'mixed phase did not exercise all 24 species orders').toBe(24);
        expect(nativeSteps).toBeLessThan(referenceSteps);
        console.log(`[openlara] wolf/bear/bat/raptor multi-owner contact: 960 exact C snapshots, all 24 species orders, excluded unported types, ` +
            `far $7F Lara and $F8/$F9 frame data; ${contacts} latches, ${moved} pushes, ${nativeSteps}/${referenceSteps} instructions`);
    });

    it.each([19,7,8,9])('matches the complete C collision phase including contact before push and room rejection for type %i', (type) => {
        const native = raptorCartridge();
        const reference = raptorCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const edges = [-4096,-1000,-400,-201,-1,0,1,200,400,1000,4095,4096];
        let nativeSteps = 0, referenceSteps = 0, contacts = 0, moved = 0, roomGuards = 0;
        for (let test = 0; test < 480; test++) {
            const snapshots: Buffer[] = [];
            for (const g of [native, reference]) {
                g.reset(); g.pose([1,3,4,7,8][test % 5]);
                g.call('navResetCache');
                g.w16(g.raptor + ITEM.x, 2560); g.w16(g.raptor + ITEM.z, 2560);
                g.w16(g.raptor + ITEM.y, 2048);
                g.w8(g.raptor + ITEM.room, 0); g.w16(g.raptor + ITEM.angleY, (test * 1337) & 65535);
                g.w8(g.raptor + ITEM.flags2, test % 19 ? 1 : [0,2,3][test % 3]);
                g.w8(g.raptor + ITEM.type, type);
                g.w16(g.raptor + ITEM.health, test % 23 ? 20 : 0);
                if (test % 17) g.call('Enemy_navAcquire', [...g.raptorArgs, 1]);
                const x = 2560 + edges[test % edges.length], z = 2560 + edges[(test * 7 + 2) % edges.length];
                g.w16(g.item + ITEM.x, x); g.w16(g.item + ITEM.z, z);
                g.w16(g.item + ITEM.y, 2048 + [-257,-256,0,256,257][test % 5]);
                g.w16(g.item + ITEM.health, test % 29 ? 1000 : 0);
                const ci = g.symbol('cinfo');
                for (let i = 0; i < 58; i++) g.w8(ci + i, 0);
                g.w16(ci + 26, 100); g.w16(ci + 40, x); g.w16(ci + 42, g.s16(g.item + ITEM.y)); g.w16(ci + 44, z);
                g.w8(ci + 55, (test >>> 2) & 1); g.w16(ci + 46, 0x8000);
                g.w8(g.symbol('olEnemyTouch') + g.slot, 0);
                g.w8(g.table('Sector') + (2 * 8 + 2) * 4, test % 31 === 0 ? 0x81 : 8); // restore valid/rejected room guard
                g.call('Room_reset');
                const steps = g.call('Enemy_collideLara', g.args, {
                    name: 'ItemObj_collideRoom', run: stack => {
                        // Enemy::collide temporarily sets radius=0 around
                        // the entire source collidePush, not just its box.
                        expect(g.s16(ci + 26), `enemy room-guard radius ${test}`).toBe(0);
                        expect(g.u16(stack + 4) | g.u16(stack + 6) << 16).toBe(g.raptor);
                        expect([g.u16(stack + 12), g.s16(stack + 14)]).toEqual([762,0]);
                        expect([g.s16(ci + 28), g.s16(ci + 30), g.s16(ci + 32)])
                            .toEqual([32512,-384,0]);
                        if (g === native) roomGuards++;
                    },
                });
                expect(g.s16(ci + 26), `restored enemy contact radius ${test}`).toBe(100);
                const read = (p: number, n: number) => Array.from({ length: n }, (_, i) => g.memory.read(p + i));
                snapshots.push(Buffer.from(read(g.item, ITEM_SIZE).concat(read(g.raptor, ITEM_SIZE),
                    read(ci, 58), read(g.symbol('olEnemyTouch'), 128))));
                if (g === native) {
                    nativeSteps += steps;
                    contacts += g.memory.read(g.symbol('olEnemyTouch') + g.slot) ? 1 : 0;
                    moved += g.s16(g.item + ITEM.x) !== x || g.s16(g.item + ITEM.z) !== z ? 1 : 0;
                } else referenceSteps += steps;
            }
            expect(snapshots[0], `complete type ${type} contact phase ${test}`).toEqual(snapshots[1]);
        }
        expect(contacts).toBeGreaterThan(20); expect(moved).toBeGreaterThan(5);
        expect(roomGuards).toBeGreaterThan(5);
        expect(nativeSteps).toBeLessThan(referenceSteps);
        console.log(`[openlara] creature contact phase (type ${type}): 480 exact C item/Lara/collision/latch snapshots, ` +
            `${contacts} contacts, ${moved} pushes, ${roomGuards} zero-radius room guards; ` +
            `${nativeSteps}/${referenceSteps} instructions`);
    });

    it.each(['Raptor_update','Enemy_update'])(
        'matches complete C updates across targeting, steering, navigation, attacks and death through %s', (dispatch) => {
        const native = raptorCartridge();
        const reference = raptorCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const deltas = [-32768,-32767,-2049,-1537,-769,-681,-400,-1,0,1,399,400,679,680,768,1536,2048,32767];
        const angles = [0,1,31,32,16383,16384,16385,32767,32768,49151,49152,49153,65535];
        const states = [0,1,2,3,4,6,7,8];
        let nativeSteps = 0, referenceSteps = 0, turns = 0;
        for (let test = 0; test < 640; test++) {
            const snapshots: Buffer[] = [], infos: number[][] = [];
            for (const g of [native, reference]) {
                g.reset(); g.pose(states[test % states.length]);
                g.w8(g.raptor + ITEM.room, 0); g.w8(g.raptor + ITEM.flags, 0x40);
                g.w8(g.raptor + ITEM.flags2, 1 | (test & 4) | ((test >>> 1) & 24) | (test & 32));
                g.w8(g.raptor + ITEM.mood, (test >>> 2) & 3);
                g.w16(g.raptor + ITEM.health, test % 13 ? 20 : test & 1 ? -1 : 0);
                g.w16(g.raptor + ITEM.timer, 300);
                g.w16(g.raptor + ITEM.x, 2560); g.w16(g.raptor + ITEM.z, 2560);
                g.w16(g.raptor + ITEM.y, 2048 + [-257,-256,0,256,257][test % 5]);
                const angle = angles[(test * 7) % angles.length];
                g.w16(g.raptor + ITEM.angleY, angle);
                g.w16(g.raptor + ITEM.hSpeed, [0,8,100,-8,-100,32767,-32768][test % 7]);
                g.w16(g.raptor + ITEM.vSpeed, (test % 7 - 3) * 8);
                g.w16(g.item + ITEM.x, 2560 + deltas[test % deltas.length]);
                g.w16(g.item + ITEM.z, 2560 + deltas[(test * 7 + 3) % deltas.length]);
                g.w16(g.item + ITEM.y, 2048 + [-257,-256,-255,0,255,256,257][test % 7]);
                g.w16(g.item + ITEM.health, test % 17 ? 1000 : 0);
                g.w8(g.symbol('olEnemyTurn') + g.slot, [0,1,4][test % 3]);
                g.w8(g.symbol('olEnemyNext') + g.slot, [0,1,3,6][(test >>> 3) & 3]);
                g.w8(g.symbol('olEnemyTouch') + g.slot, (test >>> 2) & 1);
                const seed = (Math.imul(test + 1, 1664525) + 1013904223) >>> 0;
                g.w16(g.symbol('olRandSeed'), seed); g.w16(g.symbol('olRandSeed') + 2, seed >>> 16);
                g.call('Room_reset'); g.call('navResetCache');
                if (test & 1) {
                    g.call('Enemy_navAcquire', [...g.raptorArgs, 1]);
                    g.w8(g.symbol('olEnemyTurn') + g.slot, [0,1,4][test % 3]);
                }
                let info: number[] = [];
                const steps = g.call(dispatch, [...g.raptorArgs, ...g.args], {
                    name: 'Raptor_state', run: stack => { info = [g.u16(stack + 8), g.u16(stack + 10)]; },
                });
                if (g === native) {
                    nativeSteps += steps;
                    if (g.u16(g.raptor + ITEM.angleY) !== angle) turns++;
                } else referenceSteps += steps;
                infos.push(info);
                const bytes = (at: number, size: number) => Array.from({ length: size }, (_, i) => g.memory.read(at + i));
                snapshots.push(Buffer.from(bytes(g.raptor, ITEM_SIZE).concat(bytes(g.item, ITEM_SIZE),
                    bytes(g.symbol('olEnemyTurn') + g.slot, 1), bytes(g.symbol('olEnemyNext') + g.slot, 1),
                    bytes(g.symbol('olEnemyTouch') + g.slot, 1), bytes(g.symbol('olRandSeed'), 4),
                    bytes(g.symbol('olEnemyInZone'), 2), bytes(g.symbol('olNav'), 8240))));
            }
            expect(infos[0], `raptor target info ${test}`).toEqual(infos[1]);
            expect(snapshots[0], `complete raptor update ${test}, state ${states[test % states.length]}`).toEqual(snapshots[1]);
        }
        expect(turns, 'the oracle never exercised steering').toBeGreaterThan(20);
        expect(nativeSteps).toBeLessThan(referenceSteps);
        console.log(`[openlara] native raptor update (${dispatch}): 640 exact C target-info/item/Lara/PRNG/Nav snapshots; ` +
            `${turns} nonzero turns; ${nativeSteps}/${referenceSteps} instructions (${(100 * nativeSteps / referenceSteps).toFixed(1)}%)`);
    });
});

function raptorCartridge(romPath = ROM, symbols = SYMS) {
    const g = movementCartridge(romPath, symbols);
    let slot = -1;
    for (let i = 1; i < g.count('items'); i++)
        if (g.memory.read(g.item + i * ITEM_SIZE + ITEM.type) === 19) { slot = i; break; }
    expect(slot, 'the test level has no actual raptor model').toBeGreaterThan(0);
    const raptor = g.item + slot * ITEM_SIZE, args = [raptor & 65535, raptor >>> 16];
    const model = g.table('Model') + g.memory.read(raptor + ITEM.model) * 16;
    const first = g.u16(model), count = g.u16(model + 2), anims = g.table('Anim');
    const animation = (state: number) => {
        for (let a = first; a < first + count; a++) if (g.memory.read(anims + a * 14 + 9) === state) return a;
        throw new Error(`raptor model lacks state ${state}`);
    };
    const pose = (state: number) => {
        g.call('ItemObj_animSet', [...args, animation(state)]); g.w8(g.symbol('olEnemyNext') + slot, 0);
    };
    g.set({ olBoxCount: 1, itemsCount: slot + 1, olFirstActive: slot, olCreatureCount: 1 });
    g.w8(g.symbol('olCreatures'), slot);
    const box = g.table('Box');
    [0,7,0,7].forEach((v, i) => g.w8(box + i, v)); g.w16(box + 4, 2048); g.w16(box + 6, 0);
    g.zoneMap([[0], [0]]); g.call('navResetCache');
    g.w8(raptor + ITEM.room, 0); g.w8(raptor + ITEM.flags2, 1 | 32);
    g.w8(raptor + ITEM.flags, 0x40); g.w8(raptor + ITEM.nextActive, 255);
    g.w8(raptor + ITEM.mood, 0); g.w16(raptor + ITEM.health, 20);
    g.w16(raptor + ITEM.x, 2560); g.w16(raptor + ITEM.y, 2048);
    g.w16(raptor + ITEM.z, 1536); g.w16(raptor + ITEM.angleY, 0);
    g.w16(raptor + ITEM.hSpeed, 0); g.w16(raptor + ITEM.vSpeed, 0);
    // Controller tests isolate hit timing from Lara movement; the real-level
    // test above exercises her complete collision/push path. A contact latch
    // must be supplied by that phase, not recomputed after the push.
    g.w16(g.symbol('cinfo') + 26, 100);
    g.w8(g.symbol('cinfo') + 55, 0);
    g.call('Enemy_navAcquire', [...args, 1]);
    const step = () => {
        g.call('Enemy_collideLara', g.args);
        return g.call('ItemObj_update', [...args, ...g.args]);
    };
    const raptorState = () => g.memory.read(raptor + ITEM.state);
    return { ...g, raptor, raptorArgs: args, slot, first, count, pose, raptorState, step, animation };
}

function bearCartridge(romPath = ROM, symbols = SYMS) {
    const g = movementCartridge(romPath, symbols);
    let slot = -1;
    for (let i = 1; i < g.count('items'); i++)
        if (g.memory.read(g.item + i * ITEM_SIZE + ITEM.type) === 8) { slot = i; break; }
    expect(slot, 'the test level has no bear model').toBeGreaterThan(0);
    const bear = g.item + slot * ITEM_SIZE, args = [bear & 65535, bear >>> 16];
    const model = g.table('Model') + g.memory.read(bear + ITEM.model) * 16;
    const first = g.u16(model), count = g.u16(model + 2), anims = g.table('Anim');
    const animation = (state: number) => {
        for (let a = first; a < first + count; a++) if (g.memory.read(anims + a * 14 + 9) === state) return a;
        throw new Error(`bear model lacks state ${state}`);
    };
    const pose = (state: number) => {
        g.call('ItemObj_animSet', [...args, animation(state)]);
        g.w8(g.symbol('olEnemyNext') + slot, 0);
    };
    g.set({ olBoxCount: 1, itemsCount: slot + 1, olFirstActive: slot, olCreatureCount: 1 });
    g.w8(g.symbol('olCreatures'), slot);
    const box = g.table('Box');
    [0,7,0,7].forEach((v, i) => g.w8(box + i, v));
    g.w16(box + 4, 2048); g.w16(box + 6, 0);
    g.zoneMap([[0], [0]]);
    g.call('navResetCache');
    g.w8(bear + ITEM.room, 0); g.w8(bear + ITEM.flags2, 1 | (FX3 ? 32 : 0));
    g.w8(bear + ITEM.flags, 0x40); g.w8(bear + ITEM.nextActive, 255);
    g.w8(bear + ITEM.mood, 0); g.w16(bear + ITEM.health, 20);
    g.w16(bear + ITEM.x, 2560); g.w16(bear + ITEM.y, 2048);
    g.w16(bear + ITEM.z, 1536); g.w16(bear + ITEM.angleY, 0);
    g.w16(bear + ITEM.hSpeed, 0); g.w16(bear + ITEM.vSpeed, 0);
    if (FX3) {
        // Acquire while alive so the original death graphs retain their
        // context and fall contact. Real Lara_update scenes keep pushing.
        g.w16(g.symbol('cinfo') + 26, 100); g.w8(g.symbol('cinfo') + 55, 0);
        g.call('Enemy_navAcquire', [...args, 1]);
    }
    const step = () => {
        if (FX3) g.call('Enemy_collideLara', g.args);
        return g.call('ItemObj_update', [...args, ...g.args]);
    };
    const bearState = () => g.memory.read(bear + ITEM.state);
    return { ...g, bear, bearArgs: args, slot, first, count, pose, bearState, step, animation };
}

function wolfCartridge(romPath = ROM, symbols = SYMS) {
    const g = movementCartridge(romPath, symbols);
    let slot = -1;
    for (let i = 1; i < g.count('items'); i++)
        if (g.memory.read(g.item + i * ITEM_SIZE + ITEM.type) === 7) { slot = i; break; }
    expect(slot, 'the test level has no wolf model').toBeGreaterThan(0);
    const wolf = g.item + slot * ITEM_SIZE, args = [wolf & 65535, wolf >>> 16];
    const model = g.table('Model') + g.memory.read(wolf + ITEM.model) * 16;
    const first = g.u16(model), count = g.u16(model + 2);
    const anims = g.table('Anim');
    expect(g.memory.read(wolf + ITEM.frameIndex), 'Wolf constructor did not start one frame before the last')
        .toBe(g.memory.read(anims + first * 14) - 2);
    const animation = (state: number) => {
        for (let a = first; a < first + count; a++) if (g.memory.read(anims + a * 14 + 9) === state) return a;
        throw new Error(`wolf model lacks state ${state}`);
    };
    const pose = (state: number) => {
        g.call('ItemObj_animSet', [...args, animation(state)]);
        g.w8(g.symbol('olEnemyNext') + slot, 0);
    };
    const step = () => {
        if (FX3) g.call('Enemy_collideLara', g.args);
        return g.call('ItemObj_update', [...args, ...g.args]);
    };
    const state = () => g.memory.read(wolf + ITEM.state);
    g.set({ olBoxCount: 1, itemsCount: slot + 1, olFirstActive: slot, olCreatureCount: 1 });
    g.w8(g.symbol('olCreatures'), slot);
    const box = g.table('Box');
    [0, 7, 0, 7].forEach((v, i) => g.w8(box + i, v));
    g.w16(box + 4, 2048); g.w16(box + 6, 0);
    g.zoneMap([[0], [0]]);
    g.call('navResetCache');
    g.w8(wolf + ITEM.room, 0); g.w8(wolf + ITEM.flags2, 1);
    g.w8(wolf + ITEM.flags, 0x40); g.w8(wolf + ITEM.nextActive, 255);
    g.w8(wolf + ITEM.mood, 0); g.w16(wolf + ITEM.health, 6);
    g.w16(wolf + ITEM.x, 2560); g.w16(wolf + ITEM.y, 2048);
    g.w16(wolf + ITEM.z, 1536); g.w16(wolf + ITEM.angleY, 0);
    g.w16(wolf + ITEM.hSpeed, 0); g.w16(wolf + ITEM.vSpeed, 0);
    if (FX3) {
        // Isolated controllers still need the original pre-update contact
        // phase. Actual Lara_update tests retain its real push/room guard.
        g.w16(g.symbol('cinfo') + 26, 100); g.w8(g.symbol('cinfo') + 55, 0);
        g.call('Enemy_navAcquire', [...args, 1]);
    }
    return { ...g, wolf, wolfArgs: args, slot, first, count, pose, wolfState: state, step, animation };
}

describe.skipIf(!FX3 || !['LEVEL1', 'LEVEL2'].includes(LEVEL))('OpenLara FX3: corpse expiry', () => {
    it.each([7, 8, 9])('hides type %i after the GBA countdown, preserving death, saves and the active list', (type) => {
        const g = movementCartridge(); g.restoreLevel();
        const slot = Array.from({ length: g.count('items') }, (_, i) => i)
            .find(i => g.memory.read(g.item + i * ITEM_SIZE) === type)!;
        expect(slot, `no type ${type} in ${LEVEL}`).toBeGreaterThan(0);
        const item = g.item + slot * ITEM_SIZE, args = [item & 65535, item >>> 16];
        const status = () => g.memory.read(item + ITEM.flags2) & 3;
        const owner = () => [0, 1, 2].some(i => g.u16(g.symbol('olNav') + i * 8240 + 8214) === slot);
        const active = () => {
            const result: number[] = [];
            for (let n = g.u16(g.symbol('olFirstActive')); n !== 255; n = g.memory.read(g.item + n * ITEM_SIZE + ITEM.nextActive)) {
                expect(n).toBeLessThan(g.u16(g.symbol('itemsCount')));
                expect(result, 'cycle in the active list').not.toContain(n);
                result.push(n);
            }
            return result;
        };
        // Use original level geometry and animations. A shot starts the
        // countdown; touching a corpse again must not extend its lifetime.
        const trigger = g.table('Trigger');
        for (let i = 0; i < 16; i++) g.w8(trigger + i, 0);
        [1, 1, 31, 0, 17].forEach((v, i) => g.w8(trigger + i, v));
        g.w8(trigger + 8, slot);
        g.w8(item + ITEM.flags, (g.memory.read(item + ITEM.flags) | 0x80) & ~0x20);
        g.w16(item + ITEM.health, 1);
        g.call('ItemObj_activate', args);
        g.set({ olWeaponTarget: slot, olWeaponFireArm: 0 });
        g.call('Weapon_fire', [1]);
        expect(g.s16(item + ITEM.health)).toBe(0);
        expect(g.s16(item + ITEM.timer), 'lethal shot did not start GBA HIDE_CORPSES').toBe(300);
        let ended = -1, pose: number[] | undefined;
        const shape = () => Array.from({ length: 11 }, (_, i) => g.memory.read(item + ITEM.x + i));
        for (let tick = 0; tick < 300; tick++) {
            g.call('Enemy_update', [...args, ...g.args]);
            g.call('fireTrigger', [0, 0]);
            expect(g.s16(item + ITEM.timer), `countdown at ${tick}`).toBe(299 - tick);
            expect(status(), `hidden before 300 updates: ${tick}`).not.toBe(3);
            expect(active()).toContain(slot);
            if (g.s16(item + ITEM.health) === -0x4000) {
                expect(status()).toBe(2);
                expect(owner(), 'finished corpse still owns navigation').toBe(false);
                if (ended < 0) { ended = tick; pose = shape(); }
                else expect(shape(), 'finished death pose moved').toEqual(pose);
            }
            if (tick === 122) {
                const health = g.s16(item + ITEM.health), phase = status();
                const list = active();
                g.call('Save_write'); expect(g.u16(g.symbol('tcc__r0')), 'corpse save is invalid').toBe(1);
                g.w16(item + ITEM.timer, 1); g.w16(item + ITEM.health, 1); g.w8(item + ITEM.flags2, 1);
                g.call('Nav_reset'); g.call('Save_apply');
                expect([g.s16(item + ITEM.timer), g.s16(item + ITEM.health), status()]).toEqual([177, health, phase]);
                expect(active(), 'save/load lost the corpse or another active item').toEqual(list);
                g.call('Weapon_fire', [1]);
                expect(g.s16(item + ITEM.timer), 'a repeated hit reset the dead timer').toBe(177);
            }
        }
        expect(ended, `type ${type} death never finished: state ${g.memory.read(item + ITEM.state)}, ` +
            `anim ${g.u16(item + ITEM.animIndex)}.${g.memory.read(item + ITEM.frameIndex)}`).toBeGreaterThanOrEqual(0);
        g.call('gameRenderSync');
        const render = g.symbol('ol_itemState_gsu') + slot * 12;
        expect(g.memory.read(render), 'body disappeared before expiry').not.toBe(255);
        const rest = active().filter(i => i !== slot);
        g.call('Enemy_update', [...args, ...g.args]);
        expect([g.s16(item + ITEM.timer), status(), g.memory.read(item + ITEM.flags) & 0x40]).toEqual([-1, 3, 0]);
        expect(active(), 'expiry damaged another active item').toEqual(rest);
        expect(owner()).toBe(false);
        expect(g.memory.read(g.symbol('olItemDirty') + slot)).toBe(1);
        g.call('ItemObj_activate', args);
        expect(active(), 'trigger resurrected an expired corpse').toEqual(rest);
        g.call('fireTrigger', [0, 0]);
        expect([g.s16(item + ITEM.timer), status()], 'floor pad reset or revealed an expired corpse').toEqual([-1, 3]);
        expect(active(), 'floor pad reinserted an expired corpse').toEqual(rest);
        g.call('gameRenderSync');
        expect(g.memory.read(render), 'expired corpse was still published to the GSU').toBe(255);
        console.log(`[openlara] corpse ${type}/${slot}: death finished at ${ended + 1}, 300 visible updates then hidden; saved 177 ticks, Nav/list/dirty publication exact`);
    });
});

describe('OpenLara SNES/Super FX: wolves', () => {
    it.skipIf(!FX3)('uses current-frame wolf contact before push and rejects animation-union ghost hits', () => {
        const g = wolfCartridge(), count = g.u16(g.symbol('olHostDirectory') + 16 + HOST_COUNTS.indexOf('anims') * 2);
        const dir = count * 12, record = count * 14, box = record + 4;
        const size = box + 12, ci = g.symbol('cinfo');
        let checks = 0, hits = 0, pushes = 0;
        const scenarios = ['ghost','touch','moved-after-contact','invisible','inactive','no-owner','dead-target','other-room'];
        for (const low of [0,65536 - size]) for (const state of [6,12]) for (const push of [0,1]) for (const scenario of scenarios) {
            g.reset(); g.pose(state); g.call('Nav_reset');
            g.w8(g.wolf + ITEM.room, 0); g.w8(g.wolf + ITEM.flags, 0x40); g.w8(g.wolf + ITEM.flags2, 1);
            g.w8(g.wolf + ITEM.nextActive, 255); g.set({ olFirstActive: g.slot });
            [2560,2048,2560].forEach((v, i) => g.w16(g.wolf + ITEM.x + i * 2, v));
            g.w16(g.wolf + ITEM.health, 6); g.w16(g.wolf + ITEM.angleY, 0);
            g.w16(g.wolf + ITEM.hSpeed, 0); g.w16(g.wolf + ITEM.vSpeed, 0);
            g.call('Enemy_navAcquire', [...g.wolfArgs, 1]);
            if (scenario === 'no-owner') g.call('Nav_reset');
            g.w8(g.wolf + ITEM.flags2, scenario === 'invisible' ? 3 : scenario === 'inactive' ? 2 : 1);
            g.w8(g.wolf + ITEM.room, scenario === 'other-room' ? 1 : 0);
            const base = 0xF80000 + low;
            for (let i = 0; i < size; i++) g.w8(base + i, 0);
            // One exact frame differs deliberately from the old full-
            // animation union. All other animations retain a union fallback.
            for (let a = 0; a < count; a++) [-512,512,-800,0,-512,512].forEach((v, i) => g.w16(base + a * 12 + i * 2, v));
            [-100,100,-762,0,-100,100].forEach((v, i) => g.w16(base + 103 * 12 + i * 2, v));
            g.w16(base + dir + g.animation(state) * 2, record); g.w16(base + record, 1); g.w16(base + record + 2, box);
            [-32,32,-800,0,-32,32].forEach((v, i) => g.w16(base + box + i * 2, v));
            g.w16(g.symbol('olAnimBoxData'), base); g.w16(g.symbol('olAnimBoxData') + 2, base >>> 16);
            const startX = 2560 + (scenario === 'ghost' ? 200 : 0);
            [startX,2048,2560].forEach((v, i) => g.w16(g.item + ITEM.x + i * 2, v));
            g.w16(g.item + ITEM.health, scenario === 'dead-target' ? 0 : 1000);
            g.w8(g.symbol('olEnemyTouch') + g.slot, 0);
            for (let i = 0; i < 58; i++) g.w8(ci + i, 0);
            g.w16(ci + 26, 100); g.w8(ci + 55, push); g.w8(ci + 56, 1);
            [startX,2048,2560].forEach((v, i) => g.w16(ci + 40 + i * 2, v));
            g.w8(g.symbol('olLaraHitQuadrant'), 0); g.w8(g.symbol('olLaraHitTimer'), 0);
            g.call('Enemy_collideLara', g.args);
            const touching = scenario === 'touch' || scenario === 'moved-after-contact';
            // collidePush(lara, cinfo, true): only a performed push names a quadrant.
            expect(g.memory.read(g.symbol('olLaraHitTimer')), `${scenario} hit timer, push ${push}`).toBe(touching && push ? 5 : 0);
            expect(g.memory.read(g.symbol('olLaraHitQuadrant')) !== 0).toBe(touching && push === 1);
            if (touching && push) {
                // The push hands frameContact's box over in r2; the direct
                // entry looks it up itself. Both must record the same.
                const record = () => Array.from({ length: 6 }, (_, i) => g.u16(g.symbol('olLaraHitIn') + i * 2));
                const viaPush = record();
                for (let i = 0; i < 12; i++) g.w8(g.symbol('olLaraHitIn') + i, 0xEE);
                g.call('Lara_enemyHit', [...g.wolfArgs, startX, 2560]);
                expect(record(), `${scenario} push record, state ${state}`).toEqual(viaPush);
            }
            expect(g.memory.read(g.symbol('olEnemyTouch') + g.slot), `${scenario} contact, state ${state}, push ${push}`).toBe(+touching);
            expect(g.s16(ci + 26)).toBe(100);
            if (touching && push) {
                expect(g.s16(g.item + ITEM.x), 'wolf contact did not apply the original nearest-face push').toBe(2528); pushes++;
            } else expect(g.s16(g.item + ITEM.x)).toBe(startX);
            if (scenario === 'moved-after-contact') g.w16(g.item + ITEM.x, 3760);
            g.call('ItemObj_update', [...g.wolfArgs, ...g.args]);
            const expected = scenario === 'dead-target' ? 0 : 1000 - (touching ? state === 6 ? 50 : 100 : 0);
            expect(g.s16(g.item + ITEM.health), `${scenario} damage, state ${state}, push ${push}`).toBe(expected);
            if (touching) {
                g.call('ItemObj_update', [...g.wolfArgs, ...g.args]);
                expect(g.s16(g.item + ITEM.health), 'wolf contact damaged twice from one attack latch').toBe(expected); hits++;
            }
            checks++;
        }
        expect([checks,hits,pushes]).toEqual([64,16,8]);
        console.log(`[openlara] wolf frame contact: ${checks} current-frame/ghost/visibility/owner/dead-target cases at CPU-only $F8 bank start/end; ` +
            `${hits} single-hit attacks, ${pushes} zero-radius pushes; pre-push contact survives later separation`);
        // Also find witnesses in the unmodified TR1 attack poses, not just
        // the synthetic extreme above. Raw-source boxes determine which
        // union edges must miss the current frame; the old bounds routine
        // independently confirms that these used to count as contacts.
        const real = wolfCartridge();
        const source = JSON.parse(readFileSync(join(DATA_DIR, 'FRAMES.json'), 'utf8')) as {
            animations: { anim: number; type: number; ticks: number; rate: number; boxes: number[][] }[];
        };
        let originalGhosts = 0;
        for (const state of [6,12]) {
            const anim = real.animation(state), frames = source.animations.find(a => a.anim === anim && a.type === 7)!;
            expect(frames).toBeDefined();
            const union = Array.from({ length: 6 }, (_, i) => real.s16(real.table('AnimBox') + anim * 12 + i * 2));
            const cx = (union[0] + union[1]) >> 1, cz = (union[4] + union[5]) >> 1;
            const points = [[union[0] - 100,cz],[union[1] + 100,cz],[cx,union[4] - 100],[cx,union[5] + 100]];
            for (let pose = 0; pose < frames.boxes.length && pose * frames.rate < frames.ticks; pose++) {
                const box = frames.boxes[pose];
                for (const [dx,dz] of points) {
                    if (dx >= box[0] - 100 && dx <= box[1] + 100 && dz >= box[4] - 100 && dz <= box[5] + 100) continue;
                    real.reset(); real.pose(state);
                    real.w8(real.wolf + ITEM.frameIndex, pose * frames.rate);
                    real.w8(real.wolf + ITEM.room, 0); real.w8(real.wolf + ITEM.flags, 0x40); real.w8(real.wolf + ITEM.flags2, 1);
                    real.w16(real.wolf + ITEM.health, 6); real.w16(real.wolf + ITEM.angleY, 0);
                    real.w16(real.wolf + ITEM.hSpeed, 0); real.w16(real.wolf + ITEM.vSpeed, 0);
                    [2560,2048,2560].forEach((v, i) => real.w16(real.wolf + ITEM.x + i * 2, v));
                    [2560 + dx,2048,2560 + dz].forEach((v, i) => real.w16(real.item + ITEM.x + i * 2, v));
                    real.w16(real.item + ITEM.health, 1000);
                    real.w8(real.symbol('olEnemyTouch') + real.slot, 0); real.w8(real.symbol('olEnemyTurn') + real.slot, 0);
                    const ci = real.symbol('cinfo'); real.w16(ci + 26, 100); real.w8(ci + 55, 0);
                    real.call('Enemy_navAcquire', [...real.wolfArgs, 1]);
                    real.call('ItemObj_collideBounds', [...real.wolfArgs, ...real.args, 100, -real.s16(real.table('AnimBox') + 103 * 12 + 4)]);
                    expect(real.u16(real.symbol('tcc__r0')), `original union witness ${anim}:${pose}`).toBe(1);
                    real.call('Enemy_collideLara', real.args);
                    expect(real.memory.read(real.symbol('olEnemyTouch') + real.slot), `original frame witness ${anim}:${pose}`).toBe(0);
                    real.call('ItemObj_update', [...real.wolfArgs, ...real.args]);
                    expect(real.s16(real.item + ITEM.health), `original-pose ghost hit ${anim}:${pose}`).toBe(1000);
                    originalGhosts++;
                }
            }
        }
        expect(originalGhosts, 'no real attack pose distinguished current-frame contact from the union').toBeGreaterThan(10);
        console.log(`[openlara] wolf original poses: ${originalGhosts} raw-source attack/bite positions hit the old animation union but correctly cause no current-frame damage`);
    });

    it.skipIf(LEVEL !== 'LEVEL1')('approaches Lara on the real room-14 slopes', () => {
        const g = movementCartridge();
        g.restoreLevel();
        const wolf = g.item + 26 * ITEM_SIZE, args = [wolf & 65535, wolf >>> 16];
        expect([g.memory.read(wolf), g.memory.read(wolf + 1)]).toEqual([7, 14]);
        g.w16(wolf + ITEM.health, 6);
        g.w8(wolf + ITEM.flags2, 1); g.w8(wolf + ITEM.flags, 0x40);
        g.w8(g.item + ITEM.room, 14); g.w16(g.item + ITEM.x, 22016);
        g.w16(g.item + ITEM.y, 6656); g.w16(g.item + ITEM.z, -17456);
        g.w16(g.item + ITEM.health, 1000);
        g.w16(g.item + ITEM.angleY, 0x8000);
        g.anim(103);
        const trail: string[] = [];
        let nearest = 32767, closest = '';
        for (let tick = 0; tick < 420 && g.s16(g.item + ITEM.health) === 1000; tick++) {
            g.call('Lara_update', [...g.args, 0]);
            g.call('Wolf_update', [...args, ...g.args], { name: 'Wolf_state', run: stack => {
                const distance = Math.max(Math.abs(g.s16(wolf + ITEM.x) - g.s16(g.item + ITEM.x)),
                    Math.abs(g.s16(wolf + ITEM.z) - g.s16(g.item + ITEM.z)));
                if (distance < nearest) {
                    nearest = distance;
                    closest = `t${tick} d${distance} info=${g.u16(stack + 8)} headDist=${g.u16(stack + 10)} ` +
                        `state=${g.memory.read(wolf + ITEM.state)} anim=${g.u16(wolf + ITEM.animIndex)}.` +
                        `${g.memory.read(wolf + ITEM.frameIndex)} angle=${g.u16(wolf + ITEM.angleY)} y=${g.s16(wolf + ITEM.y)}`;
                }
            } });
            if (tick % 30 === 0) trail.push(`${tick}:${g.memory.read(wolf + ITEM.state)}/` +
                `${g.memory.read(wolf + ITEM.goalState)}@${g.s16(wolf + ITEM.x)},${g.s16(wolf + ITEM.y)},${g.s16(wolf + ITEM.z)}`);
            if (FX3 && process.env.OL_NAV_TRACE && tick % 60 === 0) {
                const n = g.symbol('olNav');
                console.log(`[openlara] wolf Nav t${tick}: mood=${g.memory.read(wolf + ITEM.mood)} ` +
                    `end/target=${g.u16(n + 8198)}/${g.u16(n + 8200)} ` +
                    `pos/wp=${[8216,8224,8228,8236].map(k => g.u16(n + k) | g.u16(n + k + 2) << 16)} ` +
                    `turn=${g.memory.read(g.symbol('olEnemyTurn') + 26)} angle=${g.u16(wolf + ITEM.angleY)}`);
            }
        }
        console.log(`[openlara] wolf slopes: ${trail.join(' ')}; Lara ` +
            `${g.s16(g.item + ITEM.x)},${g.s16(g.item + ITEM.y)},${g.s16(g.item + ITEM.z)}, health ${g.s16(g.item + ITEM.health)}; closest ${closest}`);
        expect(trail.length).toBeGreaterThan(0);
        expect(g.s16(g.item + ITEM.health), 'wolf did not attack on the real sloped floor').toBeLessThan(1000);
    });

    it('wakes, stalks, runs and attacks through the original animation graph', () => {
        const g = wolfCartridge();
        g.w16(g.item + ITEM.z, 6500);
        const seen = new Set([g.wolfState()]);
        let fastest = 0, contactHits = 0;
        for (let tick = 0; tick < 420 && g.s16(g.item + ITEM.health) === 1000; tick++) {
            const before = g.wolfState(), hp = g.s16(g.item + ITEM.health);
            g.step(); seen.add(g.wolfState());
            fastest = Math.max(fastest, g.s16(g.wolf + ITEM.hSpeed));
            if (g.s16(g.item + ITEM.health) < hp) {
                expect([6, 12], 'damage occurred outside a wolf attack').toContain(before);
                contactHits++;
            }
        }
        // Original per-creature mood/waypoint rolls may select RUN directly
        // after GROWL. STALK's decisions are covered exhaustively below.
        expect([...seen], `wake/pursuit states: ${[...seen]}`).toEqual(expect.arrayContaining(FX3 ? [8, 1, 9, 3] : [8, 1, 9, 5, 3]));
        expect(fastest, 'the wolf still uses the old constant 13-unit walk').toBeGreaterThan(13);
        expect(contactHits, `no attack reached Lara: states ${[...seen]}, z=${g.s16(g.wolf + ITEM.z)}`).toBe(1);
        expect(g.s16(g.item + ITEM.health)).toBeGreaterThanOrEqual(900);
        console.log(`[openlara] wolf graph: ${[...seen]}, peak animation speed ${fastest}, first attack hit`);
    });

    it('latches one 50/100-point hit per attack and chooses all three deaths without restarting', () => {
        const g = wolfCartridge();
        for (const state of [6, 12]) {
            g.pose(state); g.w16(g.item + ITEM.health, 1000);
            g.w8(g.wolf + ITEM.flags2, 1); g.w16(g.wolf + ITEM.health, 6);
            let ticks = 0;
            while (g.wolfState() === state && ticks++ < 100) {
                g.w16(g.wolf + ITEM.x, g.u16(g.item + ITEM.x));
                g.w16(g.wolf + ITEM.z, g.u16(g.item + ITEM.z));
                g.w16(g.wolf + ITEM.y, 2048);
                g.step();
            }
            expect(ticks, 'attack animation never exited').toBeLessThan(100);
            expect(g.s16(g.item + ITEM.health), `state ${state} did not latch exactly one hit`).toBe(state === 6 ? 950 : 900);
        }
        // The first roll after these seeds falls into each third of 0..32767.
        const seeds = [0, 3, 1];
        const deaths = new Set<number>();
        for (const seed of seeds) {
            g.pose(3); g.w16(g.wolf + ITEM.health, 0); g.w8(g.wolf + ITEM.flags2, 1);
            if (FX3) g.w16(g.wolf + ITEM.timer, 300); // equivalent to the lethal hit above the controller
            g.w8(g.wolf + ITEM.flags, 0x40); g.set({ olFirstActive: g.slot });
            const seedAt = g.symbol('olRandSeed'); g.w16(seedAt, seed); g.w16(seedAt + 2, 0);
            g.step();
            const chosen = g.u16(g.wolf + ITEM.animIndex), afterSeed = g.u16(seedAt) | g.u16(seedAt + 2) << 16;
            expect(g.wolfState()).toBe(11); deaths.add(chosen - g.first);
            let ticks = 0;
            while ((g.memory.read(g.wolf + ITEM.flags) & 0x40) && ticks++ < (FX3 ? 301 : 120)) {
                g.step();
                expect(g.u16(seedAt) | g.u16(seedAt + 2) << 16, 'death rerolled on a later tick').toBe(afterSeed);
            }
            if (FX3) expect(ticks, 'GBA corpse countdown did not expire exactly').toBe(300);
            else expect(ticks, 'corpse never left the active list').toBeLessThan(120);
            expect(g.memory.read(g.wolf + ITEM.flags) & 0x40).toBe(0);
        }
        expect([...deaths].sort()).toEqual([20, 21, 22]);
        console.log('[openlara] wolf attacks: one 50/100-point hit; three finite death graphs deactivate without rerolling');
    });

    it('respects steps, drops, walls, zones, portals and other active creatures', () => {
        const g = wolfCartridge();
        for (const x of [-32768, -32767, -1, 0, 1, 32767]) for (const z of [-32768, -32767, -1, 0, 1, 32767]) {
            g.call('Enemy_distance', [x, z]);
            expect(g.u16(g.symbol('tcc__r0'))).toBe(Math.min(32767,
                Math.max(Math.abs(x), Math.abs(z)) + (Math.min(Math.abs(x), Math.abs(z)) >> 1)));
        }
        const prepare = () => {
            g.pose(3); g.w16(g.wolf + ITEM.x, 2560); g.w16(g.wolf + ITEM.y, 2048);
            g.w16(g.wolf + ITEM.z, 3071); g.w16(g.wolf + ITEM.angleY, 0);
            g.w8(g.wolf + ITEM.room, 0); g.call('Room_reset'); g.call('navResetCache');
        };
        const move = () => g.call('Enemy_moveGround', [...g.wolfArgs, 100]);
        for (const [floor, allowed] of [[1792, true], [1536, false], [3072, true], [3328, false], [-32512, false]] as const) {
            for (let x = 0; x < 8; x++) for (let z = 3; z < 8; z++) g.sector(0, x, z, floor, -2048);
            prepare(); move();
            expect(g.s16(g.wolf + ITEM.z) > 3071, `floor ${floor} step/drop decision`).toBe(allowed);
            expect(g.u16(g.wolf + ITEM.angleY), 'blocked movement must apply double turn').toBe(allowed ? 100 : 200);
            expect(g.s16(g.wolf + ITEM.y)).toBe(allowed ? floor < 2048 ? floor : 2112 : 2048);
        }
        for (let x = 0; x < 8; x++) for (let z = 3; z < 8; z++) g.sector(0, x, z, 2048, -2048);
        const boxes = g.table('Box');
        [0, 2, 0, 7, 0, 8, 0, 0, 3, 7, 0, 7, 0, 8, 0, 0].forEach((v, i) => g.w8(boxes + i, v));
        g.set({ olBoxCount: 2 });
        g.navMap(Array.from({ length: 128 }, (_, s) => s % 8 < 3 ? 0 : 1));
        g.zoneMap([[0, 1], [7, 7]]);
        const zones = g.zoneAddress(0);
        prepare(); move(); expect(g.s16(g.wolf + ITEM.z), 'crossed into a disconnected ground zone').toBe(3071);
        g.w16(zones + 2, 0); prepare(); move(); expect(g.s16(g.wolf + ITEM.z)).toBeGreaterThan(3071);
        if (FX3) {
            // Ground motion must use the active ground-1 map, not ground-2,
            // fly or an old normal-map offset. Cache reset precedes each
            // explicit flip; changing the flip itself does not reset Nav.
            g.zoneMap([[0, 1], [2, 2], [7, 7], [0x8000, 0x8000], [3, 4], [8, 9]]);
            for (const flipped of [0, 1, 0, 1]) {
                prepare(); g.call('Nav_setFlipped', [flipped]); move();
                expect(g.s16(g.wolf + ITEM.z) > 3071, `ground motion flip ${flipped}`).toBe(!!flipped);
                expect(g.u16(g.wolf + ITEM.angleY)).toBe(flipped ? 100 : 200);
            }
            g.zoneMap([[0, 0], [7, 7]]);
        }
        // A horizontal portal commits both the new position and room.
        g.set({ olRoomCount: 2 }); g.room(1, false);
        for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++) g.sector(1, x, z, 2048, -2048);
        g.w8(g.table('Link') + 10, 1);
        g.sector(0, 2, 3, 2048, -2048, 1);
        prepare(); move(); expect(g.memory.read(g.wolf + ITEM.room)).toBe(1);
        g.sector(0, 2, 3, 2048, -2048);
        const otherSlot = g.slot === 1 ? 2 : 1, other = g.item + otherSlot * ITEM_SIZE;
        g.set({ olCreatureCount: 2 }); g.w8(g.symbol('olCreatures') + 1, otherSlot);
        g.w8(other + ITEM.room, 0); g.w16(other + ITEM.health, 6); g.w8(other + ITEM.flags2, 1);
        g.w16(other + ITEM.x, 2560); g.w16(other + ITEM.z, 3200);
        prepare(); move();
        expect(g.s16(g.wolf + ITEM.z), 'ran through another active creature').toBe(3071);
        expect(g.u16(g.wolf + ITEM.angleY)).toBe(182 * 16);
        g.w16(other + ITEM.health, 0); prepare(); move();
        expect(g.s16(g.wolf + ITEM.z), 'a corpse blocked navigation').toBeGreaterThan(3071);
        console.log('[openlara] wolf ground motion: 256-up/1024-down limits, zone/portal checks and active-creature avoidance');
    });

    it.skipIf(!FX3 || !process.env.OL_REFERENCE_ROM)('matches C ground movement across animation, height, collision and turn boundaries', () => {
        const native = wolfCartridge();
        const reference = wolfCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        let nativeSteps = 0, referenceSteps = 0;
        for (let test = 0; test < 384; test++) {
            const snapshots: Buffer[] = [];
            for (const g of [native, reference]) {
                const state = [1,2,3,5,6,9,11,12][test % 8];
                g.pose(state);
                const x = [2560,3071,-32768,32767][(test >>> 3) % 4];
                const z = [3071,2560,32767,-32768][(test >>> 5) % 4];
                const y = 2048 + [-1025,-1024,-65,-64,0,256,257,0][(test >>> 1) % 8];
                g.w16(g.wolf + ITEM.x, x); g.w16(g.wolf + ITEM.y, y); g.w16(g.wolf + ITEM.z, z);
                g.w16(g.wolf + ITEM.angleY, (test * 997) & 65535);
                g.w16(g.wolf + ITEM.hSpeed, test & 1 ? 100 : 0);
                g.w16(g.wolf + ITEM.vSpeed, (test % 7 - 3) * 8);
                g.w8(g.wolf + ITEM.flags2, 1 | (test & 4));
                g.w8(g.wolf + ITEM.room, 0);
                g.w16(g.wolf + ITEM.roomFloor, -1234);
                const otherSlot = 1, other = g.item + otherSlot * ITEM_SIZE;
                g.set({ olCreatureCount: 2 });
                g.w8(g.symbol('olCreatures'), g.slot); g.w8(g.symbol('olCreatures') + 1, otherSlot);
                g.w8(other + ITEM.room, (test >>> 2) & 1);
                g.w16(other + ITEM.health, test % 5 ? 6 : 0);
                g.w8(other + ITEM.flags2, (test >>> 3) & 3);
                g.w16(other + ITEM.x, x + [-341,-340,-339,0,339,340,341][test % 7]);
                g.w16(other + ITEM.z, z + [-341,0,339][test % 3]);
                g.call('Room_reset'); g.call('navInvalidateSectors');
                const steps = g.call('Enemy_moveGround', [...g.wolfArgs, [-32768,-1820,-364,0,364,1820,32767][test % 7]]);
                if (g === native) nativeSteps += steps; else referenceSteps += steps;
                snapshots.push(Buffer.from(Array.from({ length: ITEM_SIZE }, (_, i) => g.memory.read(g.wolf + i))
                    .concat(g.memory.read(g.symbol('olEnemyNext') + g.slot))));
            }
            expect(snapshots[0], `ground movement ${test}`).toEqual(snapshots[1]);
        }
        console.log(`[openlara] native ground motion: 384 exact C animation/height/collision/turn cases; ` +
            `${nativeSteps} CPU instructions vs C ${referenceSteps} (${(nativeSteps / referenceSteps * 100).toFixed(1)}%)`);
        expect(nativeSteps).toBeLessThan(referenceSteps);
    });

    it.skipIf(!FX3 || !process.env.OL_REFERENCE_ROM).each(['Wolf_update','Enemy_update'])(
        'matches C complete wolf updates across steering, targeting, attacks and death through %s', (dispatch) => {
        const native = wolfCartridge();
        const reference = wolfCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const states = [1,2,3,5,6,7,8,9,11,12];
        const deltas = [-32768,-32767,-1537,-769,-375,-345,-1,0,1,344,375,768,1536,32766,32767];
        const angles = [0,1,31,32,16383,16384,16385,32767,32768,49151,49152,49153,65535];
        let nativeSteps = 0, referenceSteps = 0, turns = 0;
        for (let test = 0; test < 640; test++) {
            const snapshots: Buffer[] = [], targetInfo: number[][] = [];
            for (const g of [native, reference]) {
                g.reset(); g.pose(states[test % states.length]);
                g.w8(g.wolf + ITEM.room, 0); g.w8(g.wolf + ITEM.flags, 0x40);
                g.w8(g.wolf + ITEM.flags2, 1 | (test & 4) | ((test >>> 1) & 8) | (test & 32));
                g.w8(g.wolf + ITEM.mood, (test >>> 2) & 3);
                g.w16(g.wolf + ITEM.health, test % 13 ? 6 : 0);
                g.w16(g.wolf + ITEM.timer, 300);
                g.w16(g.wolf + ITEM.x, 2560); g.w16(g.wolf + ITEM.z, 2560);
                g.w16(g.wolf + ITEM.y, 2048);
                g.w16(g.wolf + ITEM.angleY, angles[(test * 7) % angles.length]);
                g.w16(g.wolf + ITEM.hSpeed, [0,8,100,-8,-100,32767,-32768][test % 7]);
                g.w16(g.wolf + ITEM.vSpeed, (test % 7 - 3) * 8);
                g.w16(g.item + ITEM.x, 2560 + deltas[test % deltas.length]);
                g.w16(g.item + ITEM.z, 2560 + deltas[(test * 7 + 3) % deltas.length]);
                g.w16(g.item + ITEM.y, 2048 + [-257,-256,-255,0,255,256,257][test % 7]);
                g.w16(g.item + ITEM.angleY, angles[(test * 3 + 1) % angles.length]);
                g.w16(g.item + ITEM.health, test % 17 ? 1000 : 0);
                g.w8(g.symbol('olEnemyTurn') + g.slot, [0,2,5][test % 3]);
                g.w8(g.symbol('olEnemyNext') + g.slot, [0,1,3,5,8,9][(test >>> 3) % 6]);
                g.w8(g.symbol('olEnemyTouch') + g.slot, (test >>> 2) & 1);
                const seed = (Math.imul(test + 1, 1664525) + 1013904223) >>> 0;
                g.w16(g.symbol('olRandSeed'), seed); g.w16(g.symbol('olRandSeed') + 2, seed >>> 16);
                g.call('Room_reset'); g.call('Nav_reset');
                // Cold admission resets max turn to zero. Also exercise
                // warm contexts, otherwise these snapshots would never
                // compare the nonzero previous-state steering calculation.
                if (test & 1) {
                    g.call('Enemy_navAcquire', [...g.wolfArgs, 1]);
                    g.w8(g.symbol('olEnemyTurn') + g.slot, [0,2,5][test % 3]);
                }
                let info: number[] = [];
                const steps = g.call(dispatch, [...g.wolfArgs, ...g.args], {
                    name: 'Wolf_state', run: stack => { info = [g.u16(stack + 8), g.u16(stack + 10)]; },
                });
                if (g === native) {
                    nativeSteps += steps;
                    if (g.u16(g.wolf + ITEM.angleY) !== angles[(test * 7) % angles.length]) turns++;
                } else referenceSteps += steps;
                targetInfo.push(info);
                const bytes = (at: number, size: number) => Array.from({ length: size }, (_, i) => g.memory.read(at + i));
                snapshots.push(Buffer.from(bytes(g.wolf, ITEM_SIZE).concat(bytes(g.item, ITEM_SIZE),
                    bytes(g.symbol('olEnemyTurn') + g.slot, 1), bytes(g.symbol('olEnemyNext') + g.slot, 1),
                    bytes(g.symbol('olRandSeed'), 4), bytes(g.symbol('olEnemyInZone'), 2), bytes(g.symbol('olNav'), 8240),
                    bytes(g.symbol('olEnemyTouch') + g.slot, 1))));
            }
            expect(targetInfo[0], `wolf target info ${test}`).toEqual(targetInfo[1]);
            expect(snapshots[0], `complete wolf update ${test}: state ${states[test % states.length]}`)
                .toEqual(snapshots[1]);
        }
        console.log(`[openlara] native wolf update (${dispatch}): 640 exact C target-info/item/Lara/PRNG/Nav snapshots; ` +
            `${turns} nonzero steering cases; ${nativeSteps} CPU instructions vs C ${referenceSteps} (${(nativeSteps / referenceSteps * 100).toFixed(1)}%)`);
        expect(turns, 'the complete-update oracle did not exercise warm-context steering').toBeGreaterThan(20);
        expect(nativeSteps).toBeLessThan(referenceSteps);
    });

    it('uses the previous state turn limit and halves it for close reverse turns', () => {
        const g = wolfCartridge();
        g.pose(2); g.w16(g.item + ITEM.x, 5000); g.w16(g.item + ITEM.z, 1536);
        pinCreatureTarget(g, g.wolf);
        g.w8(g.symbol('olEnemyTurn') + g.slot, 0);
        g.step(); expect(g.u16(g.wolf + ITEM.angleY)).toBe(0);
        g.step(); expect(g.u16(g.wolf + ITEM.angleY)).toBe(182 * 2);
        g.pose(3); g.step(); expect(g.u16(g.wolf + ITEM.angleY)).toBe(182 * 4);
        g.step(); expect(g.u16(g.wolf + ITEM.angleY)).toBe(182 * 9);
        g.pose(3); g.w16(g.wolf + ITEM.x, 2560); g.w16(g.wolf + ITEM.z, 2560);
        g.w16(g.wolf + ITEM.angleY, 0); g.w16(g.wolf + ITEM.hSpeed, 100);
        g.w16(g.item + ITEM.x, 2560); g.w16(g.item + ITEM.z, 2300);
        pinCreatureTarget(g, g.wolf);
        g.step(); expect(g.u16(g.wolf + ITEM.angleY)).toBe((65536 - 182 * 5 / 2) & 65535);
        g.w16(g.item + ITEM.health, 0); g.step();
        expect(g.memory.read(g.wolf + ITEM.mood), 'continued pursuing a dead target').toBe(0);
        console.log('[openlara] wolf steering: previous-tick 0/2/5-degree limits, close reverse half-turn and dead-target mood');
    });

    it('matches the original wolf decisions across states, distance edges, moods and random thresholds', () => {
        const g = movementCartridge();
        const ref = process.env.OL_REFERENCE_ROM ? movementCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS ?? '')) : undefined;
        const mult = 0x41C64E6D;
        let inverse = 1;
        for (let i = 0; i < 5; i++) inverse = Math.imul(inverse, 2 - Math.imul(mult, inverse));
        const rolls = [0, 31, 32, 383, 384, 32767];
        let count = 0, nativeSteps = 0, referenceSteps = 0;
        for (let state = 0; state <= 13; state++) for (let mood = 0; mood < 4; mood++)
        for (let info = 0; info < 32; info++) for (const dist of [0, 344, 345, 346, 768, 769, 1535, 1536, 1537, 3068, 3069, 3070, 32767])
        for (const pending of [0, 7, 9]) {
            const slot = count & 1 ? 1 : 127;
            const roll = rolls[count % rolls.length];
            const seed = Math.imul((roll << 10) - 0x3039, inverse) >>> 0;
            let afterSeed = seed;
            const random = () => { afterSeed = (Math.imul(afterSeed, mult) + 0x3039) >>> 0; return afterSeed >>> 10 & 32767; };
            const health = count % 19 === 0 ? -1 : count % 17 === 0 ? 0 : 6;
            const target = [1000, 73, 0, -1][(count >>> 1) & 3];
            let next = pending, turn = 11, hp = target;
            const goal = (state + 3) % 14;
            // Direct transcription of src/fixed/enemy.h: Wolf::updateState.
            const decide = () => {
                if (health <= 0) return goal;
                switch (state) {
                    case 1: return next || 2;
                    case 2:
                        turn = 2;
                        if (mood !== 0) { next = 0; return 5; }
                        if (random() < 0x20) { next = 8; return 1; }
                        break;
                    case 3:
                        turn = 5;
                        if ((info & 1) && dist < 1536) {
                            if (dist <= 768 && !(info & 2)) { next = 0; return 6; }
                            next = 5; return 9;
                        }
                        if (mood === 1 && dist < 3069) { next = 5; return 9; }
                        if (mood === 0) return 9;
                        break;
                    case 5:
                        turn = 2;
                        if (mood === 3) return 3;
                        if (dist < 345 && (info & 4) && hp > 0) return 12;
                        if (dist > 3069) return 3;
                        if (mood === 2) {
                            if (!(info & 1) || dist > 1536 || (info & 2)) return 3;
                            break;
                        }
                        if (random() < 0x180) { next = 7; return 9; }
                        if (mood === 0) return 9;
                        break;
                    case 8:
                        if (mood === 3 || (info & 8)) next = 9;
                        else if (random() < 0x20) next = 2;
                        else break;
                        return 1;
                    case 9:
                        if (next) return next;
                        if (mood === 3) return 3;
                        if (dist < 345 && (info & 4) && hp > 0) return 12;
                        return mood === 1 ? 5 : mood === 0 ? 1 : 3;
                    case 6: case 12:
                        if (!next && (info & 16)) {
                            if (hp > 0) hp = Math.max(0, hp - (state === 6 ? 50 : 100));
                            next = state === 6 ? 3 : 9;
                        }
                        return 3;
                }
                return goal;
            };
            const expectedGoal = decide();
            const initial = Buffer.alloc(ITEM_SIZE, 0xA5);
            initial[ITEM.type] = 7; initial[ITEM.state] = state; initial[ITEM.goalState] = goal;
            initial[ITEM.mood] = mood; initial.writeInt16LE(health, ITEM.health);
            for (const cart of [g, ref]) {
                if (!cart) continue;
                const item = cart.item + slot * ITEM_SIZE, seedAt = cart.symbol('olRandSeed');
                initial.forEach((v, i) => cart.w8(item + i, v));
                cart.w16(cart.item + ITEM.health, target);
                cart.w8(cart.symbol('olEnemyNext') + slot, pending); cart.w8(cart.symbol('olEnemyTurn') + slot, 11);
                cart.w16(seedAt, seed); cart.w16(seedAt + 2, seed >>> 16);
                const steps = cart.call('Wolf_state', [item & 65535, item >>> 16, info, dist]);
                if (cart === g) nativeSteps += steps; else referenceSteps += steps;
                const actual = [cart.u16(cart.symbol('tcc__r0')), cart.memory.read(cart.symbol('olEnemyNext') + slot),
                    cart.memory.read(cart.symbol('olEnemyTurn') + slot), cart.s16(cart.item + ITEM.health),
                    (cart.u16(seedAt) | cart.u16(seedAt + 2) << 16) >>> 0];
                expect(actual, `${cart === g ? 'native' : 'C'} wolf decision ${count}: state=${state}, ` +
                    `mood=${mood}, info=${info}, dist=${dist}, next=${pending}, random=${roll}`)
                    .toEqual([expectedGoal, next, turn, hp, afterSeed]);
                expect(Buffer.from(Array.from({ length: ITEM_SIZE }, (_, i) => cart.memory.read(item + i))))
                    .toEqual(initial);
            }
            count++;
        }
        console.log(`[openlara] wolf decisions: ${count} cases match the original state graph; ${nativeSteps} CPU instructions` +
            (ref ? ` vs C ${referenceSteps} (${(100 * nativeSteps / referenceSteps).toFixed(1)}%)` : ' (independent oracle)'));
        if (ref) expect(nativeSteps).toBeLessThan(referenceSteps);
    });
});

describe('OpenLara SNES/Super FX: Lara diving', () => {
    it('uses TR1 dive controls, animation transitions, drag and landing thresholds', () => {
        const g = movementCartridge();
        const { item, args, w8, w16, u16, s16, call } = g;
        for (const weapons of [0, 1, 2, 3, 4]) for (const input of [0, 0x20, 0x40, 0x60]) {
            g.reset(); w8(item + ITEM.state, 3); w8(item + ITEM.goalState, 52);
            g.set({ olLaraWeaponState: weapons });
            call('Lara_updateState', [...args, input]);
            expect(g.goal(), `weapon ${weapons}, input ${input}`).toBe(
                weapons ? 3 : input & 0x40 ? 52 : input & 0x20 ? 11 : 3);
        }
        for (const goal of [8, 2, 1]) {
            g.reset(); w8(item + ITEM.state, 3); w8(item + ITEM.goalState, goal);
            call('Lara_updateState', [...args, 0x60]);
            expect(g.goal(), 'dive input replaced an already chosen landing').toBe(goal);
        }
        // The original jump graph must actually grant the new state.
        g.reset(); g.anim(77, 1); // airborne forward jump
        expect(g.state()).toBe(3);
        const seen = new Set<number>();
        for (let n = 0; n < 60 && !seen.has(52); n++) {
            call('Lara_updateState', [...args, 0x40]); call('ItemObj_animProcess', args); seen.add(g.state());
        }
        expect(seen.has(52), 'Walk never reached the swan-dive animation').toBe(true);
        for (const speed of [131, 132]) {
            w8(item + ITEM.state, 52); w8(item + ITEM.goalState, 52); w16(item + ITEM.vSpeed, speed);
            call('Lara_updateState', [...args, 0]);
            expect(g.goal()).toBe(speed === 131 ? 52 : 53);
        }
        for (let n = 0; n < 60 && g.state() !== 53; n++) {
            call('Lara_updateState', [...args, 0]); call('ItemObj_animProcess', args);
        }
        expect(g.state(), 'the original dive graph never entered FAST_DIVE').toBe(53);
        for (const speed of [-1000, -346, -1, 0, 1, 100, 346, 1000]) {
            w16(item + ITEM.hSpeed, speed);
            call('Lara_updateState', [...args, 5]); // Up+Down must not start a standing roll in the air
            expect(s16(item + ITEM.hSpeed), `drag at ${speed}`).toBe(Math.trunc(speed * 95 / 100) | 0);
            expect(g.state()).toBe(53);
        }
        for (const state of [3, 52, 53]) for (const speed of [133, 134, 140, 141, 154, 155]) {
            g.reset(); w8(item + ITEM.state, state); w16(item + ITEM.vSpeed, speed);
            w16(sym('cinfo'), 65535); w16(sym('cinfo') + 2, 0);
            call('c_checkLanding', args);
            const damage = state === 53 && speed > 133 ? 1000 : speed <= 140 ? 0 :
                speed > 154 ? 1000 : Math.trunc((speed - 140) ** 2 * 1000 / 196);
            expect(u16(item + ITEM.health), `state ${state} landed at ${speed}`).toBe(1000 - damage);
            expect(u16(sym('tcc__r0'))).toBe(damage === 1000 ? 1 : 0);
        }
        console.log('[openlara] diving: input priority, occupied hands, original animation graph, signed drag and fatal landing boundaries agree');
    });

    it('preserves swan/fast-dive animations, pitch and speed on entering water', () => {
        const g = movementCartridge();
        g.set({ olRoomCount: 2 }); g.room(0, false); g.room(1, true);
        const links = g.table('Link');
        g.w8(links + 9, 0);      // link 1: room above is dry room 0
        g.w8(links + 16, 1);     // link 2: room below is water room 1
        for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++) {
            g.sector(0, x, z, 0, -2048, 2); g.sector(1, x, z, 2048, 0, 1);
        }
        const animations = g.table('Anim');
        for (const state of [52, 53]) {
            g.reset();
            let anim = -1;
            for (let a = 0; a < 160; a++) if (g.memory.read(animations + a * 14 + 9) === state) { anim = a; break; }
            expect(anim).toBeGreaterThanOrEqual(0); g.anim(anim);
            g.w8(g.item + ITEM.room, 1); g.w8(g.item + ITEM.flags2, 4); // gravity
            g.w16(g.item + ITEM.y, 512); g.w16(g.item + ITEM.vSpeed, 140);
            g.set({ olLaraOxygen: 200 });
            g.call('Lara_updateWaterState', g.args);
            expect(g.u16(sym('olLaraWaterState'))).toBe(3);
            expect(g.s16(sym('olLaraAngleX'))).toBe(-182 * (state === 52 ? 45 : 85));
            expect(g.u16(g.item + ITEM.vSpeed)).toBe(280);
            expect(g.memory.read(g.item + ITEM.flags2) & 4).toBe(0);
            expect(g.u16(sym('olLaraOxygen'))).toBe(1800);
            expect(g.u16(g.item + ITEM.animIndex), 'a dive was replaced with the ordinary water-fall animation').not.toBe(112);
            for (let n = 0; n < 90 && g.state() !== 35; n++) g.call('ItemObj_animProcess', g.args);
            expect(g.state(), 'the dive never entered its underwater animation').toBe(35);
        }
        console.log('[openlara] diving: original swan/fast water transitions retain -45/-85 degree pitch and doubled entry speed');
    });

    it('climbs a hanging ledge with a handstand only while Walk is held', () => {
        const g = movementCartridge();
        for (let x = 0; x < 8; x++) for (let z = 4; z < 8; z++) g.sector(0, x, z, 1024, -2048);
        for (const walk of [0, 0x40]) {
            g.reset(); g.anim(96, 21);
            const minY = g.s16(g.table('AnimBox') + 96 * 12 + 4);
            g.w16(g.item + ITEM.y, 1024 - minY); g.w16(g.item + ITEM.z, 4096 - 101);
            g.set({ olLaraWeaponState: 1 });
            const states = new Set<number>();
            const animations = new Set<number>();
            for (let n = 0; n < 360; n++) {
                g.call('Lara_update', [...g.args, g.state() === 10 ? 0x21 | walk : 0]);
                states.add(g.state());
                animations.add(g.u16(g.item + ITEM.animIndex));
                if (g.state() === 2 && n > 2) break;
            }
            expect([...states], `climb states, Walk=${walk}`).toContain(walk ? 54 : 19);
            // Both climbs finish through animation 102 (state HANG_UP).
            // Their long climb animations, 97 and 159, are distinct.
            expect(animations.has(walk ? 97 : 159), 'the wrong climb animation played').toBe(false);
            expect(g.state(), `climb never returned to STOP: ${[...states]}; anim ` +
                `${g.u16(g.item + ITEM.animIndex)}.${g.memory.read(g.item + ITEM.frameIndex)}, goal ${g.goal()}`).toBe(2);
            expect(g.s16(g.item + ITEM.y), 'the climb did not reach the upper floor').toBe(1024);
            expect(g.s16(g.item + ITEM.z)).toBeGreaterThanOrEqual(4096);
            expect(g.u16(sym('olLaraWeaponState')), 'climb animation retained busy hands').toBe(0);
        }
        console.log('[openlara] handstand: Action+Up+Walk selects the original animation, clears the ledge and frees the hands');
    });
});

function batCartridge(romPath = ROM, symbols = SYMS) {
    const g = movementCartridge(romPath, symbols);
    let slot = -1;
    for (let i = 1; i < g.count('items'); i++) {
        if (g.memory.read(g.item + i * ITEM_SIZE + ITEM.type) === 9) { slot = i; break; }
    }
    expect(slot, 'the test level has no bat model').toBeGreaterThan(0);
    const bat = g.item + slot * ITEM_SIZE;
    const args = [bat & 65535, bat >>> 16];
    const model = g.table('Model') + g.memory.read(bat + ITEM.model) * 16;
    const first = g.u16(model), count = g.u16(model + 2);
    const anims = g.table('Anim');
    const animation = (state: number) => {
        for (let a = first; a < first + count; a++) {
            if (g.memory.read(anims + a * 14 + 9) === state) return a;
        }
        throw new Error(`bat model lacks state ${state}`);
    };
    const pose = (state: number) => g.call('ItemObj_animSet', [...args, animation(state)]);
    const state = () => g.memory.read(bat + ITEM.state);
    const step = () => {
        if (FX3) g.call('Enemy_collideLara', g.args);
        return g.call('ItemObj_update', [...args, ...g.args]);
    };
    const head = () => g.s16(g.item + ITEM.y) +
        g.s16(g.table('AnimBox') + g.u16(g.item + ITEM.animIndex) * 12 + 4);
    // One reachable flying box, with the real animations and no synthetic
    // animation speeds or transitions. Navigation still runs on the CPU.
    g.set({ olBoxCount: 1, itemsCount: slot + 1, olFirstActive: slot });
    const box = g.table('Box');
    [0, 7, 0, 7].forEach((v, i) => g.w8(box + i, v));
    g.w16(box + 4, 2048); g.w16(box + 6, 0);
    g.zoneMap([[0], [0]]);
    g.call('navResetCache');
    g.w8(bat + ITEM.room, 0); g.w8(bat + ITEM.flags2, 1);
    g.w8(bat + ITEM.flags, 0x40); g.w8(bat + ITEM.nextActive, 255);
    g.w8(bat + ITEM.mood, 2); g.w16(bat + ITEM.health, 1);
    g.w16(bat + ITEM.x, 2560); g.w16(bat + ITEM.y, head() - 512);
    g.w16(bat + ITEM.z, 1536); g.w16(bat + ITEM.angleY, 0);
    g.w16(bat + ITEM.hSpeed, 0); g.w16(bat + ITEM.vSpeed, 0);
    pose(1);
    if (FX3) {
        // Controller-only scenes retain contact while disabling position
        // pushes. Real Lara_update still enables the original enemy push.
        g.w16(g.symbol('cinfo') + 26, 100); g.w8(g.symbol('cinfo') + 55, 0);
        g.call('Enemy_navAcquire', [...args, 1]);
    }
    return { ...g, slot, bat, batArgs: args, pose, batState: state, step, head, animation };
}

// These original-animation scenes need a placed bat, present in the first
// two fixture levels but not Lost Valley. Its mixed contact oracle above
// still covers type 9 independently of the available creature models.
describe.skipIf(LEVEL !== 'LEVEL1' && LEVEL !== 'LEVEL2')('OpenLara SNES/Super FX: bats', () => {
    it.skipIf(!FX3)('uses current-frame bat contact before pushes, flight decisions and two-point attacks', () => {
        const g = batCartridge(), ci = g.symbol('cinfo');
        const count = g.count('anims'), dir = count * 12, record = dir + count * 2, box = record + 4, size = box + 12;
        const scenarios = ['ghost','touch','moved-after-contact','invisible','inactive','no-owner',
            'dead-target','other-room','target-died-after-contact','dead-bat'];
        let cases = 0, contacts = 0, pushes = 0, attacks = 0;
        for (const low of [0,65536 - size]) for (const push of [0,1]) for (const state of [3,2,1,4,5]) for (const scenario of scenarios) {
            g.reset(); g.pose(state); g.call('Nav_reset');
            g.w8(g.bat + ITEM.flags, 0x40); g.w8(g.bat + ITEM.flags2, 1);
            g.w8(g.bat + ITEM.room, 0); g.w8(g.bat + ITEM.mood, 2);
            g.w8(g.bat + ITEM.nextActive, 255); g.set({ olFirstActive: g.slot });
            [2560,1536,2560].forEach((v, i) => g.w16(g.bat + ITEM.x + i * 2, v));
            g.w16(g.bat + ITEM.health, 1); g.w16(g.bat + ITEM.angleY, 0);
            g.w16(g.bat + ITEM.hSpeed, 0); g.w16(g.bat + ITEM.vSpeed, 0);
            g.call('Enemy_navAcquire', [...g.batArgs, 1]);
            if (scenario === 'no-owner') g.call('Nav_reset');
            if (scenario === 'dead-bat') {
                g.w16(g.bat + ITEM.health, 0); g.w16(g.bat + ITEM.timer, 300);
            }
            g.w8(g.bat + ITEM.flags2, scenario === 'invisible' ? 3 : scenario === 'inactive' ? 2 : 1);
            g.w8(g.bat + ITEM.room, scenario === 'other-room' ? 1 : 0);
            const base = 0xF80000 + low;
            for (let i = 0; i < size; i++) g.w8(base + i, 0);
            for (let a = 0; a < count; a++) [-512,512,-512,512,-512,512].forEach((v, i) => g.w16(base + a * 12 + i * 2, v));
            [-100,100,-762,0,-100,100].forEach((v, i) => g.w16(base + 103 * 12 + i * 2, v));
            g.w16(base + dir + g.animation(state) * 2, record); g.w16(base + record, 1); g.w16(base + record + 2, box);
            [-32,32,-80,80,-32,32].forEach((v, i) => g.w16(base + box + i * 2, v));
            g.w16(g.symbol('olAnimBoxData'), base); g.w16(g.symbol('olAnimBoxData') + 2, base >>> 16);
            const x = 2560 + (scenario === 'ghost' ? 200 : 0);
            [x,2048,2560].forEach((v, i) => g.w16(g.item + ITEM.x + i * 2, v));
            g.w16(g.item + ITEM.health, scenario === 'dead-target' ? 0 : 1000);
            g.w8(g.symbol('olEnemyTouch') + g.slot, 0);
            for (let i = 0; i < 58; i++) g.w8(ci + i, 0);
            g.w16(ci + 26, 100); g.w8(ci + 55, push);
            [x,2048,2560].forEach((v, i) => g.w16(ci + 40 + i * 2, v));
            g.call('Enemy_collideLara', g.args);
            const touching = ['touch','moved-after-contact','target-died-after-contact','dead-bat'].includes(scenario);
            expect(g.memory.read(g.symbol('olEnemyTouch') + g.slot), `${scenario} bat contact, state ${state}`).toBe(+touching);
            expect(g.s16(ci + 26)).toBe(100);
            expect(g.s16(g.item + ITEM.x), `${scenario} nearest-face push, state ${state}`).toBe(touching && push ? 2528 : x);
            if (touching) contacts++; if (touching && push) pushes++;
            if (scenario === 'moved-after-contact') g.w16(g.item + ITEM.x, 3760);
            if (scenario === 'target-died-after-contact') g.w16(g.item + ITEM.health, 0);
            g.call('ItemObj_update', [...g.batArgs, ...g.args]);
            const targetDead = scenario === 'dead-target' || scenario === 'target-died-after-contact';
            const damage = touching && state === 3 && !targetDead && scenario !== 'dead-bat' ? 2 : 0;
            expect(g.s16(g.item + ITEM.health), `${scenario} bat damage, state ${state}, push ${push}`).toBe(targetDead ? 0 : 1000 - damage);
            if (damage) attacks++;
            // The dispatched controller consumes the pre-push latch even
            // when later collision/movement separates Lara from the bat.
            if (!['inactive','other-room','dead-bat'].includes(scenario)) {
                if (state === 3 && (!touching || targetDead)) {
                    expect(g.memory.read(g.bat + ITEM.goalState)).toBe(2);
                    expect(g.memory.read(g.bat + ITEM.mood)).toBe(0);
                } else if (state === 2 && touching && !targetDead)
                    expect(g.memory.read(g.bat + ITEM.goalState)).toBe(3);
            }
            cases++;
        }
        expect([cases,contacts,pushes,attacks]).toEqual([200,80,40,8]);
        console.log(`[openlara] bat frame contact: ${cases} frame/ghost/eligibility/dead-target cases at CPU-only $F8 bank start/end; ` +
            `${contacts} pre-push contacts, ${pushes} zero-radius pushes, ${attacks} two-point attacks`);

        const real = batCartridge();
        const source = JSON.parse(readFileSync(join(DATA_DIR, 'FRAMES.json'), 'utf8')) as {
            animations: { anim: number; type: number; ticks: number; rate: number; boxes: number[][] }[];
        };
        let ghosts = 0; const states = new Set<number>();
        for (const frames of source.animations.filter(a => a.type === 9)) {
            const anim = frames.anim, state = real.memory.read(real.table('Anim') + anim * 14 + 9);
            if (![2,3].includes(state)) continue;
            const union = Array.from({ length: 6 }, (_, i) => real.s16(real.table('AnimBox') + anim * 12 + i * 2));
            const cx = (union[0] + union[1]) >> 1, cz = (union[4] + union[5]) >> 1;
            const points = [[union[0] - 100,cz],[union[1] + 100,cz],[cx,union[4] - 100],[cx,union[5] + 100]];
            for (let pose = 0; pose < frames.boxes.length && pose * frames.rate < frames.ticks; pose++) for (const [dx,dz] of points) {
                const box = frames.boxes[pose];
                if (dx >= box[0] - 100 && dx <= box[1] + 100 && dz >= box[4] - 100 && dz <= box[5] + 100) continue;
                real.reset(); real.call('ItemObj_animSet', [...real.batArgs, anim]);
                real.w8(real.bat + ITEM.frameIndex, pose * frames.rate);
                real.w8(real.bat + ITEM.room, 0); real.w8(real.bat + ITEM.flags, 0x40); real.w8(real.bat + ITEM.flags2, 1);
                real.w16(real.bat + ITEM.health, 1); real.w16(real.bat + ITEM.angleY, 0);
                real.w16(real.bat + ITEM.hSpeed, 0); real.w16(real.bat + ITEM.vSpeed, 0);
                [2560,1536,2560].forEach((v, i) => real.w16(real.bat + ITEM.x + i * 2, v));
                [2560 + dx,2048,2560 + dz].forEach((v, i) => real.w16(real.item + ITEM.x + i * 2, v));
                real.w16(real.item + ITEM.health, 1000); real.w8(real.symbol('olEnemyTouch') + real.slot, 0);
                real.call('Enemy_navAcquire', [...real.batArgs, 1]);
                real.call('ItemObj_collideBounds', [...real.batArgs, ...real.args, 100, -real.s16(real.table('AnimBox') + 103 * 12 + 4)]);
                expect(real.u16(real.symbol('tcc__r0')), `bat union witness ${anim}:${pose}`).toBe(1);
                real.call('Enemy_collideLara', real.args);
                expect(real.memory.read(real.symbol('olEnemyTouch') + real.slot), `bat frame witness ${anim}:${pose}`).toBe(0);
                real.call('ItemObj_update', [...real.batArgs, ...real.args]);
                expect(real.s16(real.item + ITEM.health), `original bat pose ghost hit ${anim}:${pose}`).toBe(1000);
                if (state === 2) expect(real.memory.read(real.bat + ITEM.goalState), `ghost attack transition ${anim}:${pose}`).toBe(2);
                ghosts++; states.add(state);
            }
        }
        expect(ghosts).toBeGreaterThan(10); expect([...states].sort()).toEqual([2,3]);
        console.log(`[openlara] bat original poses: ${ghosts} raw-source flight/attack positions reject animation-union ghost contact`);
    });

    it.skipIf(!FX3 || !process.env.OL_REFERENCE_ROM).each(['Bat_update','Enemy_update'])(
        'matches complete C bat updates across contact, flying navigation, steering and death through %s', (dispatch) => {
        const native = batCartridge();
        const reference = batCartridge(process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const deltas = [-32768,-32767,-2049,-1025,-769,-101,-100,-1,0,1,100,101,768,1024,2048,32766,32767];
        const angles = [0,1,31,32,16383,16384,16385,32767,32768,49151,49152,49153,65535];
        let nativeSteps = 0, referenceSteps = 0, turns = 0, deaths = 0;
        for (let test = 0; test < 512; test++) {
            const snapshots: Buffer[] = [];
            for (const g of [native, reference]) {
                g.reset(); g.pose(1 + test % 5);
                g.w8(g.bat + ITEM.room, 0); g.w8(g.bat + ITEM.flags, 0x40);
                g.w8(g.bat + ITEM.flags2, 1 | (test & 4) | ((test >>> 1) & 8) | (test & 32));
                g.w8(g.bat + ITEM.mood, (test >>> 2) & 3);
                g.w16(g.bat + ITEM.health, test % 13 ? 1 : test & 1 ? -1 : 0);
                g.w16(g.bat + ITEM.timer, 300);
                g.w16(g.bat + ITEM.x, 2560); g.w16(g.bat + ITEM.z, 2560);
                g.w16(g.bat + ITEM.y, [-2049,-2048,-2047,512,1286,1536,2047,2048,2049][test % 9]);
                const angle = angles[(test * 7) % angles.length];
                g.w16(g.bat + ITEM.angleY, angle);
                g.w16(g.bat + ITEM.hSpeed, [0,8,16,-8,-16,32767,-32768][test % 7]);
                g.w16(g.bat + ITEM.vSpeed, (test % 7 - 3) * 8);
                g.w16(g.item + ITEM.x, 2560 + deltas[test % deltas.length]);
                g.w16(g.item + ITEM.z, 2560 + deltas[(test * 7 + 3) % deltas.length]);
                g.w16(g.item + ITEM.y, 2048 + [-763,-762,-761,0,761,762,763][test % 7]);
                g.w16(g.item + ITEM.health, test % 17 ? 1000 : 0);
                const seed = (Math.imul(test + 1, 1664525) + 1013904223) >>> 0;
                g.w16(g.symbol('olRandSeed'), seed); g.w16(g.symbol('olRandSeed') + 2, seed >>> 16);
                g.call('Room_reset'); g.call('Nav_reset');
                if (test & 1) g.call('Enemy_navAcquire', [...g.batArgs, 1]);
                // A captured contact is a phase input, not a fresh query
                // after steering, movement or a now-dead Lara.
                g.w8(g.symbol('olEnemyTouch') + g.slot, (test >>> 2) & 1);
                const steps = g.call(dispatch, [...g.batArgs, ...g.args]);
                if (g === native) {
                    nativeSteps += steps;
                    if (g.u16(g.bat + ITEM.angleY) !== angle) turns++;
                    if (g.s16(g.bat + ITEM.health) <= 0) deaths++;
                } else referenceSteps += steps;
                const bytes = (at: number, size: number) => Array.from({ length: size }, (_, i) => g.memory.read(at + i));
                snapshots.push(Buffer.from(bytes(g.bat, ITEM_SIZE).concat(bytes(g.item, ITEM_SIZE),
                    bytes(g.symbol('olEnemyTurn') + g.slot, 1), bytes(g.symbol('olEnemyNext') + g.slot, 1),
                    bytes(g.symbol('olEnemyTouch') + g.slot, 1), bytes(g.symbol('olRandSeed'), 4),
                    bytes(g.symbol('olEnemyInZone'), 2), bytes(g.symbol('olNav'), 3 * 8240))));
            }
            expect(snapshots[0], `complete bat update ${test}, state ${1 + test % 5}`).toEqual(snapshots[1]);
        }
        expect(turns, 'the complete bat oracle never exercised steering').toBeGreaterThan(20);
        expect(deaths).toBe(40); expect(nativeSteps).toBeLessThan(referenceSteps);
        console.log(`[openlara] FX3 bat update (${dispatch}): 512 exact C item/Lara/contact/PRNG/three-Nav snapshots; ` +
            `${turns} turns, ${deaths} deaths; ${nativeSteps}/${referenceSteps} instructions`);
    });

    it('uses flying zones, crosses room portals and refuses invalid room queries', () => {
        const g = batCartridge();
        g.pose(2); g.w16(g.bat + ITEM.hSpeed, 16);
        g.set({ olBoxCount: 2 });
        const boxes = g.table('Box');
        [0, 7, 0, 3, 0, 8, 0, 0, 0, 7, 4, 7, 0, 8, 1, 0]
            .forEach((v, i) => g.w8(boxes + i, v));
        g.navMap(Array.from({ length: 128 }, (_, s) => (s % 64) < 32 ? 0 : 1));
        g.zoneMap([[0, 1], [7, 7]]);
        const overlap = g.table('Overlap');
        g.w16(overlap, 0x8001); g.w16(overlap + 2, 0x8000);
        g.w16(g.item + ITEM.x, 4608); g.call('navResetCache');
        if (FX3) {
            // First decision seeds the creature's current box; the second
            // successful aggression roll targets Lara's. Both are original.
            for (let i = 0; i < 2; i++) { g.call('rand_reset'); g.step(); }
            const n = g.symbol('olNav');
            expect([g.u16(n + 8194), g.u16(n + 8198), g.u16(n + 4)])
                .toEqual([2,1,1]);
        } else {
            g.step();
            expect(g.u16(sym('navFrom') + 2), 'bat did not flood the flying zone').toBe(1);
            expect(g.u16(sym('navNext') + 2048), 'flying flood did not connect across the ground-zone split').toBe(1);
        }
        expect(g.memory.read(g.bat + ITEM.mood)).toBe(2);
        g.w16(g.zoneAddress(2), 8); g.call('navResetCache'); g.step();
        expect(g.memory.read(g.bat + ITEM.mood), 'bat pursued across disconnected flying zones').toBe(0);

        g.set({ olRoomCount: 2 }); g.room(1, false);
        for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++) g.sector(1, x, z, 2048, -2048);
        const links = g.table('Link');
        g.w8(links + 8 + 2, 1); // horizontal portal to room 1
        g.sector(0, 2, 3, 2048, -2048, 1); g.call('Room_reset');
        g.w16(g.bat + ITEM.x, 2560); g.w16(g.bat + ITEM.z, 3071);
        g.w16(g.bat + ITEM.angleY, 0); g.w16(g.bat + ITEM.hSpeed, 0);
        g.step();
        expect(g.memory.read(g.bat + ITEM.room), 'flight never resolved the portal').toBe(1);
        expect(g.s16(g.bat + ITEM.z)).toBeGreaterThan(3071);
        g.w8(g.bat + ITEM.room, 255); g.call('Room_reset');
        const before = [ITEM.x, ITEM.y, ITEM.z].map(f => g.s16(g.bat + f));
        g.step();
        expect([ITEM.x, ITEM.y, ITEM.z].map(f => g.s16(g.bat + f)), 'a failed room query moved the bat').toEqual(before);
        console.log('[openlara] bats: flying-zone navigation, portal crossing and failed-query rollback');
    });

    it('wakes through the original graph, flies at animation speed and pursues Lara vertically', () => {
        const g = batCartridge();
        const states = new Set<number>();
        for (let i = 0; i < 120 && g.batState() !== 2; i++) {
            g.step(); states.add(g.batState());
        }
        expect(g.batState(), `wake states: ${[...states]}`).toBe(2);
        g.pose(2); g.w16(g.bat + ITEM.angleY, 0); g.w16(g.bat + ITEM.hSpeed, 0);
        g.w16(g.bat + ITEM.x, 2560); g.w16(g.bat + ITEM.z, 1536);
        for (const delta of [-512, 512, -8, 8]) {
            const y = g.head() + delta;
            g.w16(g.bat + ITEM.y, y); g.w16(g.bat + ITEM.z, 1536);
            pinCreatureTarget(g, g.bat);
            g.step();
            expect(g.s16(g.bat + ITEM.y) - y, `vertical pursuit from ${delta}`)
                .toBe(-Math.sign(delta) * Math.min(16, Math.abs(delta)));
            const a = g.u16(g.bat + ITEM.animIndex), frame = g.memory.read(g.bat + ITEM.frameIndex);
            const speeds = g.table('AnimSpeed') + a * 8;
            const s32 = (at: number) => g.u16(at) | g.u16(at + 2) << 16;
            const speed = (s32(speeds) + s32(speeds + 4) * frame) >> 16;
            expect(g.s16(g.bat + ITEM.hSpeed)).toBe(speed);
            expect(g.s16(g.bat + ITEM.z) - 1536, 'motion used a fixed creature-table speed').toBe(speed);
        }
        g.w16(g.bat + ITEM.angleY, 0x8000); g.w16(g.bat + ITEM.hSpeed, 16);
        g.step();
        expect(Math.abs(((g.u16(g.bat + ITEM.angleY) - 0x8000) << 16) >> 16)).toBe(20 * 182);
        console.log('[openlara] bats: real wake/flight graph, animation speed, 16-unit vertical pursuit and 20-degree steering');
    });

    it('attacks only in contact, deals two points per attack tick and breaks off', () => {
        const g = batCartridge();
        const contact = () => {
            g.w16(g.bat + ITEM.x, g.s16(g.item + ITEM.x));
            g.w16(g.bat + ITEM.z, g.s16(g.item + ITEM.z));
            g.w16(g.bat + ITEM.y, g.head() + 256);
        };
        g.pose(2); contact();
        for (let n = 0; n < 60 && g.batState() !== 3; n++) { contact(); g.step(); }
        expect(g.batState(), 'contact never entered the actual attack graph').toBe(3);
        g.w16(g.item + ITEM.health, 1000);
        for (let n = 0; n < 8; n++) { contact(); g.step(); }
        expect(g.u16(g.item + ITEM.health), 'bat used wolf damage or a global 16-tick cooldown').toBe(984);
        g.w16(g.bat + ITEM.y, g.head() - 1024);
        g.step();
        expect(g.memory.read(g.bat + ITEM.goalState)).toBe(2);
        expect(g.memory.read(g.bat + ITEM.mood)).toBe(0);
        expect(g.u16(g.item + ITEM.health), 'a bat overhead damaged Lara without contact').toBe(984);
        g.pose(3); contact(); g.w16(g.item + ITEM.health, 1); g.step();
        expect(g.u16(g.item + ITEM.health), 'contact damage underflowed health').toBe(0);
        contact(); g.step(); expect(g.u16(g.item + ITEM.health)).toBe(0);
        console.log('[openlara] bats: attack transition, two-point contact ticks, vertical separation and break-off');
    });

    it('rejects walls, clamps flight and falls to a stationary corpse when killed', () => {
        const g = batCartridge();
        g.pose(2);
        // The next forward step crosses into a solid sector.
        g.w16(g.bat + ITEM.z, 3071); g.w16(g.bat + ITEM.angleY, 0);
        g.w16(g.item + ITEM.z, 4096); g.w16(g.bat + ITEM.hSpeed, 0);
        g.sector(0, 2, 3, -127 * 256, -127 * 256); g.call('Room_reset');
        g.step(); expect(g.s16(g.bat + ITEM.z), 'flight crossed a solid wall').toBe(3071);
        g.sector(0, 2, 3, 2048, -2048); g.call('Room_reset');
        // A target beyond the column cannot pull the bat through its limits.
        for (const [y, target, expected] of [[-2040, -4096, -2048], [2040, 4096, 2048]]) {
            g.w16(g.bat + ITEM.y, y); g.w16(g.item + ITEM.y, target);
            pinCreatureTarget(g, g.bat);
            g.step(); expect(g.s16(g.bat + ITEM.y)).toBe(expected);
        }
        for (const killedState of [1, 2, 3]) {
            g.pose(killedState); g.w8(g.bat + ITEM.flags2, 1); g.w8(g.bat + ITEM.flags, 0x40);
            g.w8(g.bat + ITEM.nextActive, 255); g.set({ olFirstActive: (g.bat - g.item) / ITEM_SIZE });
            g.w16(g.bat + ITEM.health, 0); g.w16(g.bat + ITEM.y, 0);
            if (FX3) g.w16(g.bat + ITEM.timer, 300);
            g.w16(g.bat + ITEM.vSpeed, 0); g.w16(g.bat + ITEM.hSpeed, 100);
            const x = g.s16(g.bat + ITEM.x), z = g.s16(g.bat + ITEM.z);
            g.step();
            expect(g.memory.read(g.bat + ITEM.flags2) & 4).toBe(4);
            expect(g.memory.read(g.bat + ITEM.goalState)).toBe(4);
            expect(g.s16(g.bat + ITEM.y)).toBe(6);
            const seen = new Set<number>();
            for (let n = 0; n < 180; n++) {
                g.step(); seen.add(g.batState());
                expect([g.s16(g.bat + ITEM.x), g.s16(g.bat + ITEM.z)]).toEqual([x, z]);
                expect(g.s16(g.bat + ITEM.y)).toBeLessThanOrEqual(2048);
            }
            expect(seen.has(4), `no falling animation from state ${killedState}`).toBe(true);
            expect(g.batState(), `no death animation from state ${killedState}`).toBe(5);
            expect(g.s16(g.bat + ITEM.y)).toBe(2048);
            expect(g.memory.read(g.bat + ITEM.flags2) & 4).toBe(0);
            expect(g.s16(g.bat + ITEM.hSpeed)).toBe(0);
        }
        console.log('[openlara] bats: solid walls, ceiling/floor limits and falling death graph from awake/fly/attack');
    });
});

describe('OpenLara SNES/Super FX: crystals', () => {
    // Upstream's Crystal constructor hides every one ("disable crystals for
    // now"): the PlayStation's save points, which the PC levels carry --
    // LEVEL1 one in room 20, LEVEL2 two. level.c does the same at load, and
    // the renderer is handed them as hidden ($FF), so nothing draws them.
    it('hides the crystals as upstream does', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        emulator.executeFrames(30);
        const memory = emulator.getMemory();
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        const n = u16(sym('itemsCount')), items = sym('items');
        const crystals = Array.from({ length: n }, (_, i) => i).filter(i => memory.read(items + i * ITEM_SIZE + ITEM.type) === 83);
        expect(crystals.length, 'no crystal in the level').toBeGreaterThan(0);
        for (const i of crystals) {
            expect(memory.read(items + i * ITEM_SIZE + ITEM.flags2) & ITEM_STATUS, `crystal ${i}'s status`).toBe(3);
            expect(memory.read(sym('ol_itemState_gsu') + i * 12), `crystal ${i} handed to the renderer`).toBe(0xFF);
        }
        console.log(`[openlara] crystals: ${crystals.length} hidden (${crystals.join(', ')})`);
    }, 600_000);
});

describe('OpenLara SNES/Super FX: static meshes', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('stops at the rock behind the entrance instead of walking through it', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };
        const sItemW = (f: number) => {
            const v = u16(sym('items') + f);
            return v & 0x8000 ? v - 0x10000 : v;
        };

        // LEVEL1, room 0. The packer's first colliding static is the rock
        // face behind Lara's start -- id 38, 3200 wide and 138 deep, its
        // box spanning world Z 3107..3245 -- and the room's own wall is at
        // 3072, one sector behind it. She starts at Z 3584. Without the
        // mesh she runs to the wall, about 3172; with it she stops at its
        // face plus her radius, 3345. The two are 170 units apart, which is
        // what makes this a test of the mesh and not of the wall.
        const MESH_MAX_Z = 3245;
        const WALL_Z = 3072;
        const LARA_RADIUS = 100;

        emulator.executeFrames(120);
        for (let i = 0; i < 200; i++) emulator.executeFrames(1);
        const start = { x: sItemW(ITEM.x), z: sItemW(ITEM.z) };
        expect(u8(sym('items') + ITEM.room), 'Lara should start in room 0').toBe(0);
        expect(start.z, 'Lara should start in front of the rock')
            .toBeGreaterThan(MESH_MAX_Z + LARA_RADIUS);

        // Turn her round and hold Up: she runs at the rock.
        w16(sym('items') + ITEM.angleY, 0x8000);
        w16(sym('olLaraMoveAngle'), 0x8000);
        const pad = emulator.getController() as unknown as { buttonState: number };
        // The nearest she gets is read from the probe the tick publishes
        // last, not from her record: a field boundary can fall between the
        // move and the collision pass that puts her back, and her record
        // then shows her inside the rock for that field (TODO.md, section
        // 13).
        const probeZ = () => {
            const v = u16(sym('ol_probe') + 4);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        pad.buttonState = PAD_UP;
        let nearest = start.z;
        for (let i = 0; i < 240; i++) {
            emulator.executeFrames(1);
            const z = probeZ();
            if (z < nearest) nearest = z;
        }
        pad.buttonState = 0;
        const end = { x: sItemW(ITEM.x), z: sItemW(ITEM.z) };

        console.log(`[openlara] static: z ${start.z} -> nearest ${nearest}, ` +
                    `end ${end.z}; x ${start.x} -> ${end.x}; ` +
                    `rock face ${MESH_MAX_Z}, wall ${WALL_Z}`);
        expect(nearest, 'she never moved toward the rock').toBeLessThan(start.z - 100);
        expect(nearest, 'she reached the room wall behind the rock: the mesh did not stop her')
            .toBeGreaterThan(WALL_Z + 200);
        expect(nearest, 'she walked into the rock')
            .toBeGreaterThanOrEqual(MESH_MAX_Z + LARA_RADIUS - 8);
    }, 600_000);
});

describe('OpenLara SNES/Super FX: climbing', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('climbs a two-click ledge on Up+Action', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const w8 = (a: number, v: number) => memory.write(a, v & 0xFF);
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };
        const sItemW = (f: number) => {
            const v = u16(sym('items') + f);
            return v & 0x8000 ? v - 0x10000 : v;
        };

        // LEVEL1 room 10, from the host sector table: sector (7,6) is flat
        // at 7424 and its +Z neighbour (7,7) is flat at 6912, two clicks
        // up, with 2304 of headroom. World Z of the step is -8192 (wrapped);
        // she stands her radius off it, facing +Z, so the front probe lands
        // on the ledge, and presses Up+Action. c_checkClimbUp should pick
        // ANIM_CLIMB_2, put her in STATE_HANG_UP, and the animation's own
        // OFFSET command should lift her the 512 onto the ledge.
        const ROOM = 10, X = -19968, Y = 7424, Z = -8290;
        const LEDGE_Y = 6912, STEP_Z = -8192;
        const STATE_STOP = 2, STATE_HANG_UP = 19;
        const ANIM_STAND = 103;

        emulator.executeFrames(120);
        expect(u16(sym('itemsCount')), 'the host table never arrived').toBeGreaterThan(0);

        // Pin her there for a couple of ticks, so the relink and the floor
        // under her are hers before the pad does anything.
        for (let i = 0; i < 6; i++) {
            w8(sym('items') + ITEM.room, ROOM);
            w8(sym('items') + ITEM.state, STATE_STOP);
            w8(sym('items') + ITEM.goalState, STATE_STOP);
            w16(sym('items') + ITEM.x, X);
            w16(sym('items') + ITEM.y, Y);
            w16(sym('items') + ITEM.z, Z);
            w16(sym('items') + ITEM.angleY, 0);
            w16(sym('items') + ITEM.animIndex, ANIM_STAND);
            w8(sym('items') + ITEM.frameIndex, 0);
            w8(sym('items') + ITEM.flags2, 0);
            w16(sym('items') + ITEM.vSpeed, 0);
            w16(sym('items') + ITEM.hSpeed, 0);
            w16(sym('olLaraMoveAngle'), 0);
            w16(sym('olLaraTurn'), 0);
            emulator.executeFrames(1);
        }
        expect(u16(sym('ol_room')), 'she is not in room 10').toBe(ROOM);
        expect(sItemW(ITEM.y), 'she did not settle on the low floor').toBe(Y);

        const pad = emulator.getController() as unknown as { buttonState: number };
        pad.buttonState = PAD_R;
        emulator.executeFrames(4);
        pad.buttonState = 0;
        emulator.executeFrames(60);
        expect(u16(sym('olLaraWeaponState')), 'pistols did not finish drawing at the ledge').toBe(4);
        pad.buttonState = PAD_UP | PAD_B;
        for (let i = 0; i < 30; i++) {
            emulator.executeFrames(1);
            expect(u8(sym('items') + ITEM.state), 'firing with drawn pistols started a climb')
                .not.toBe(STATE_HANG_UP);
        }
        expect(sItemW(ITEM.y), 'armed Lara climbed onto the ledge').toBe(Y);
        pad.buttonState = PAD_R;
        emulator.executeFrames(4);
        pad.buttonState = 0;
        emulator.executeFrames(90);
        expect(u16(sym('olLaraWeaponState')), 'holstering did not free her hands').toBe(0);
        pad.buttonState = PAD_UP | PAD_B;
        const states = new Set<number>();
        const trail: string[] = [];
        let lastState = -1;
        let done = false;
        let busy = false;
        for (let i = 0; i < 480 && !done; i++) {
            if (i === 90) pad.buttonState = 0;
            emulator.executeFrames(1);
            const st = u8(sym('items') + ITEM.state);
            states.add(st);
            if (st === STATE_HANG_UP && u16(sym('olLaraWeaponState')) === 1) busy = true;
            if (st !== lastState) {
                trail.push(`${st}@${sItemW(ITEM.y)}`);
                lastState = st;
            }
            if (i > 90 && st === STATE_STOP && sItemW(ITEM.y) === LEDGE_Y &&
                sItemW(ITEM.hSpeed) === 0) done = true;
        }
        pad.buttonState = 0;

        console.log(`[openlara] climb: states ${trail.join(' ')}; ` +
                    `end (${sItemW(ITEM.x)},${sItemW(ITEM.y)},${sItemW(ITEM.z)}) ` +
                    `room ${u16(sym('ol_room'))}`);
        expect(states.has(STATE_HANG_UP), 'Up+Action at the ledge did not start a climb').toBe(true);
        expect(busy, 'climbing did not occupy her hands').toBe(true);
        expect(sItemW(ITEM.y), 'she did not end up on the ledge').toBe(LEDGE_Y);
        expect(sItemW(ITEM.z), 'she is not over the ledge').toBeGreaterThan(STEP_Z);
        expect(u8(sym('items') + ITEM.state), 'she did not come to rest').toBe(STATE_STOP);
        expect(u16(sym('olLaraWeaponState')), 'the climb animation did not free her hands').toBe(0);
    }, 600_000);
});

describe('OpenLara SNES/Super FX: falling floors', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('shakes, lets go, and falls to the floor below', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const itemB = (i: number, f: number) =>
            u8(sym('items') + i * ITEM_SIZE + f);
        const itemW = (i: number, f: number) =>
            u16(sym('items') + i * ITEM_SIZE + f);
        const sItem = (i: number, f: number) => {
            const v = itemW(i, f);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const s16 = (a: number) => { const v = u16(a); return v & 0x8000 ? v - 0x10000 : v; };
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };

        // Upstream's TrapFloor::State.
        const STATE_STATIC = 0, STATE_SHAKE = 1, STATE_FALL = 2, STATE_DOWN = 3;
        const ITEM_TRAP_FLOOR = 35;

        for (let i = 0; i < 200; i++) emulator.executeFrames(1);

        // LEVEL1's two are in room 32, model 9, animations 223 to 226 -- one
        // per state and in upstream's own order.
        const n = u16(sym('itemsCount'));
        let PIT = -1;
        for (let i = 1; i < n; i++) {
            if (itemB(i, ITEM.type) !== ITEM_TRAP_FLOOR) continue;
            if (sItem(i, ITEM.x) !== -29184) continue;
            if (sItem(i, ITEM.z) !== 8704) continue;
            PIT = i;
        }
        expect(PIT, 'no falling floor at (-29184, 8704) in LEVEL1')
            .toBeGreaterThan(0);

        const y0 = sItem(PIT, ITEM.y);
        expect(itemB(PIT, ITEM.state), 'it should start static').toBe(STATE_STATIC);

        // Upstream's test is `getLara(pos)->pos.y != pos.y - 512` -- she has
        // to be standing *on* it, at the height of its own top. Its sector is
        // hers too, which upstream gets from the room and this port has to
        // check itself.
        w16(sym('items') + ITEM.x, -29184);
        w16(sym('items') + ITEM.y, y0 - 512);
        w16(sym('items') + ITEM.z, 8704);
        memory.write(sym('items') + ITEM.room, 35);

        // Keep her on top until a fixed tick consumes the placement. The
        // room-35 sector descends into room 32 at this Y; that lower sector
        // owns both the trigger and the 3840-unit pit floor.
        w16(sym('items') + ITEM.x, -29184);
        w16(sym('items') + ITEM.y, y0 - 512);
        w16(sym('items') + ITEM.z, 8704);

        console.log(`[openlara] trapfloor: item ${PIT} at y ${y0}, ` +
                    `lara y ${sItem(0, ITEM.y)}, ` +
                    `its trigger ${itemW(PIT, ITEM.trigger)}, ` +
                    `probe trigger ${u16(sym('ol_trigger'))}`);

        // The shake is thirty-six 30 Hz animation ticks. Rendering this room
        // remains expensive, but the state progression no longer depends on
        // it. The trail says whether it is progressing or stuck.
        const seen = new Set<number>();
        const trail: string[] = [];
        let lowest = y0;
        for (let n2 = 0; n2 < 250; n2++) {
            // Held on top of it while it is still static: upstream's floor
            // deactivates the moment she is not, and nothing else here keeps
            // her there -- the floor-follow is working off a probe that has
            // her in the room below.
            if (itemB(PIT, ITEM.state) === STATE_STATIC) {
                w16(sym('items') + ITEM.y, sItem(PIT, ITEM.y) - 512);
                w16(sym('items') + ITEM.x, -29184);
                w16(sym('items') + ITEM.z, 8704);
            }
            // Every field, not every sixtieth: the fall is about forty
            // fields long and a coarser sample can miss the whole of it.
            for (let i = 0; i < 60; i++) {
                emulator.executeFrames(1);
                seen.add(itemB(PIT, ITEM.state));
            }
            if ((n2 & 31) === 0) {
                trail.push(`${itemB(PIT, ITEM.state)}/` +
                           `${itemW(PIT, ITEM.animIndex)}.${itemB(PIT, ITEM.frameIndex)}`);
            }
            lowest = Math.max(lowest, sItem(PIT, ITEM.y));
            if (itemB(PIT, ITEM.state) === STATE_DOWN) break;
        }

        console.log(`[openlara] trapfloor: trail ${trail.join(' ')}`);
        console.log(`[openlara] trapfloor: item ${PIT} saw states ` +
                    `[${[...seen].sort().join(',')}], y ${y0} -> ` +
                    `${sItem(PIT, ITEM.y)} (lowest ${lowest}), ` +
                    `roomFloor ${sItem(PIT, ITEM.roomFloor)}, ` +
                    `vSpeed ${sItem(PIT, ITEM.vSpeed)}`);

        // It notices her and lets go of STATE_STATIC -- which is the whole of
        // upstream's first arm: the height test, the sector test, and the
        // transition the animation graph offers toward STATE_SHAKE.
        expect(seen.has(STATE_SHAKE), 'it never shook').toBe(true);

        // The active list must keep advancing it through the shake. This used
        // to stop after one update because olFirstActive was accidentally
        // linked over the host's FPS counter and NMI rewrote it once a second.
        expect(seen.has(STATE_FALL), 'it shook but never let go').toBe(true);

        expect(u16(sym('ol_trigger')),
               'vertical descent did not find the lower room trigger')
            .toBe(itemW(PIT, ITEM.trigger));
        expect(seen.has(STATE_DOWN), 'it fell but never landed').toBe(true);
        expect(sItem(PIT, ITEM.roomFloor), 'it found the wrong pit floor')
            .toBe(3840);
        expect(sItem(PIT, ITEM.y), 'it did not stop on the pit floor')
            .toBe(3840);
    }, 600_000);
});

// ---------------------------------------------------------------------------
// Camera follow/clip behavior in the full cartridge.
// ---------------------------------------------------------------------------
describe.skipIf(!SYMS.has('CT_CheckHit'))('OpenLara SNES/Super FX: native camera traces', () => {
    // GSU-2 executes from $7F WRAM. Far fixture records belong in the
    // unused end of cartridge RAM, past the staged host blob, not in code.
    const far = (address: number) => FX3 ? address : 0x71FF00 + (address & 255);
    const cameraCartridge = (romPath = ROM, symbols = SYMS) => {
        const cart = movementCartridge(romPath, symbols);
        if (!FX3) expect(cart.u16(0x710000), 'camera fixture scratch overlaps host tables')
            .toBeLessThanOrEqual(0xFF00);
        return cart;
    };
    it.skipIf(!FX3 || !SYMS.has('Lara_toCamera') || !process.env.OL_REFERENCE_ROM)(
        'matches the complete camera handoff, mode precedence and room hint against C', () => {
        const g = cameraCartridge(), ref = cameraCartridge(
            process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const fields: [string, number][] = [['camera', 32], ['camTarget', 8], ['olLaraLook', 8],
            ...['ol_camX','ol_camY','ol_camZ','ol_camTgtY','ol_angle','ol_camRoom',
                'olCamSX','olCamSY','olCamSZ','olCamHint','olCamBusy','olCamFixedView',
                'olCamLastFixed','olCamLastIndex','olCamLastItem','olCamIndex','olCamLookAt']
                .map(name => [name, 2] as [string, number])];
        const snapshot = (cart: typeof g) => fields.map(([name, size]) =>
            [name, Array.from({ length: size }, (_, i) => cart.memory.read(cart.symbol(name) + i))]);
        let cases = 0, nativeSteps = 0, cSteps = 0;
        for (const health of [-32768, 0, 1, 1000]) for (const weapon of [0, 2, 4, 5])
        for (const looking of [0, 1]) for (const mode of [0, 2, 3]) for (const count of [0, 1]) {
            for (const cart of [g, ref]) {
                cart.reset(); cart.call('Camera_init');
                cart.set({ itemsCount: count, olLaraLooking: looking, olLaraWeaponState: weapon,
                    olCamBusy: Number(mode === 3 || (cases >>> 1 & 1)), olCamIndex: mode === 3 ? 0 : 65535,
                    olCamTimer: 10, olCamSX: 32767, olCamSY: 32767, olCamSZ: 32767, olCamHint: 65535 });
                cart.w16(cart.item + ITEM.health, health);
                cart.w16(cart.symbol('camera') + 18, mode);
                const steps = cart.call('Lara_toCamera');
                if (cart === g) nativeSteps += steps; else cSteps += steps;
            }
            expect(snapshot(g), `camera handoff ${health}/${weapon}/${looking}/${mode}/${count}`).toEqual(snapshot(ref));
            for (const cart of [g, ref]) cart.call('Lara_toCamera'); // warm room/cache path
            expect(snapshot(g), `warm camera handoff case ${cases}`).toEqual(snapshot(ref));
            cases++;
        }
        expect(nativeSteps).toBeLessThan(cSteps);
        console.log(`[openlara] camera handoff: ${cases * 2} full C snapshots, empty/live/dead, look/combat/fixed precedence and warm room hints; ${nativeSteps}/${cSteps} instructions`);
    });

    it.skipIf(!FX3)('matches all interpolated camera offsets against original GBA frame code', () => {
        const g = cameraCartridge();
        const reference = process.env.OL_REFERENCE_ROM ? cameraCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS!)) : undefined;
        const animations: { anim: number; type: number; ticks: number }[] =
            JSON.parse(readFileSync(join(DATA_DIR, 'FRAMES.json'), 'utf8')).animations;
        const cases = animations.flatMap(a => Array.from({ length: a.ticks }, (_, tick) => ({ ...a, tick })));
        const output = execFileSync(process.env.OL_SPHERE_ORACLE!, [process.env.OL_LEVEL_PHD!, '0', '--camera'], {
            input: cases.map(c => `${c.type} ${c.anim} ${c.tick}`).join('\n') + '\n', encoding: 'utf8', maxBuffer: 4 * 1024 * 1024 });
        const want = output.trim().split('\n').filter(line => /^-?\d+ -?\d+ -?\d+ -?\d+$/.test(line))
            .map(line => line.split(' ').map(Number));
        expect(want.length).toBe(cases.length);
        const local = (base: number, offset: number) => (base & 0xFF0000) | ((base + offset) & 65535);
        let checks = 0, paired = 0, nativeSteps = 0, cSteps = 0;
        cases.forEach((c, index) => {
            for (const cart of [g, reference && (c.tick === 0 || c.tick === c.ticks - 1 || c.tick === (c.ticks >> 1)) ? reference : undefined]) {
                if (!cart) continue;
                const item = [cart.item, 0x000520, 0x7F9000, 0x7EFFF9][index & 3];
                const y = (index * 677 + 32760) << 16 >> 16;
                cart.w16(local(item, ITEM.animIndex), c.anim);
                cart.w8(local(item, ITEM.frameIndex), c.tick);
                cart.w16(local(item, ITEM.y), y);
                const before = Array.from({ length: ITEM_SIZE }, (_, k) => cart.memory.read(local(item, k)));
                for (let component = 0; component < 4; component++) {
                    const steps = cart.call('Camera_boxOffset', [item & 65535, item >>> 16, component]);
                    expect(cart.s16(cart.symbol('tcc__r0')), `GBA camera ${c.type}/${c.anim}.${c.tick} component ${component}`)
                        .toBe(want[index][component]);
                    if (cart === g) checks++; else { paired++; cSteps += steps; }
                    if (cart === g && reference && (c.tick === 0 || c.tick === c.ticks - 1 || c.tick === (c.ticks >> 1))) nativeSteps += steps;
                }
                cart.call('Camera_targetY', [item & 65535, item >>> 16]);
                expect(cart.s16(cart.symbol('tcc__r0'))).toBe((y + want[index][0]) << 16 >> 16);
                expect(Array.from({ length: ITEM_SIZE }, (_, k) => cart.memory.read(local(item, k)))).toEqual(before);
            }
        });
        if (reference) expect(nativeSteps).toBeLessThan(cSteps);
        console.log(`[openlara] interpolated camera: ${cases.length} complete GBA ticks, ${checks} offsets, ` +
            `${paired} C comparisons (${nativeSteps}/${cSteps} instructions), far/wrapped item fields and target Y exact`);
    }, 600_000);

    it.skipIf(!FX3)('decodes every camera bit width and bit position with invalid-index guards', () => {
        const g = cameraCartridge();
        const pair = [g];
        if (process.env.OL_REFERENCE_ROM) pair.push(cameraCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS!)));
        let checks = 0;
        for (let bits = 0; bits <= 16; bits++) {
            const values = Array.from({ length: 33 }, (_, i) => bits ? ((i * 257 + i * i * 37) & (2 ** bits - 1)) : 0);
            const packed = Buffer.alloc(4 + Math.ceil(values.length * bits / 8) + 3);
            packed[0] = bits ? values.length : 0; packed[1] = bits; packed.writeInt16LE(-32768, 2);
            values.forEach((v, i) => { for (let b = 0; b < bits; b++) if (v & 2 ** b) packed[4 + ((i * bits + b) >>> 3)] |= 1 << ((i * bits + b) & 7); });
            for (const cart of pair) {
                const base = 0x7F9000, item = 0x7EFFF9;
                cart.w16(cart.symbol('olLevel') + 8, base); cart.w16(cart.symbol('olLevel') + 10, base >>> 16);
                cart.w16(cart.symbol('olLevel') + 12, 0x1000);
                cart.w16(base, 0); cart.w16(base + 2, 0); cart.w16(base + 4, 1);
                for (let component = 0; component < 4; component++) cart.w16(base + 6 + component * 2, 14);
                packed.forEach((v, i) => cart.w8(base + 14 + i, v));
                cart.w16(0x7E0005, 0); // animIndex, item + 12 wraps in its bank
                for (const tick of [...values.keys(), 33, 255]) {
                    cart.w8(0x7E0007, tick);
                    for (let component = 0; component <= 4; component++) {
                        cart.call('Camera_boxOffset', [item & 65535, item >>> 16, component]);
                        const expected = component === 4 ? 0 : ((-32768 + values[tick < values.length ? tick : 0]) << 16 >> 16);
                        expect(cart.s16(cart.symbol('tcc__r0')), `packed ${bits}/${tick}/${component}`).toBe(expected); checks++;
                    }
                }
                cart.w16(0x7E0005, 1);
                cart.call('Camera_boxOffset', [item & 65535, item >>> 16, 0]);
                expect(cart.u16(cart.symbol('tcc__r0'))).toBe(0);
                cart.w16(0x7E0005, 0); cart.w16(base + 6, 0);
                cart.call('Camera_boxOffset', [item & 65535, item >>> 16, 0]);
                expect(cart.u16(cart.symbol('tcc__r0'))).toBe(0);
                cart.w16(cart.symbol('olLevel') + 12, 0);
                cart.call('Camera_boxOffset', [item & 65535, item >>> 16, 0]);
                expect(cart.u16(cart.symbol('tcc__r0'))).toBe(0);
            }
        }
        console.log(`[openlara] packed camera: ${checks} scalar checks, widths 0..16, all reachable bit alignments, missing animation/stream/table and invalid tick/component guards`);
    });

    it.skipIf(!FX3 || !SYMS.has('Camera_floorShift'))('preserves camera word arithmetic, far field wrapping and target yaw against C', () => {
        const g = cameraCartridge();
        const ref = process.env.OL_REFERENCE_ROM ? cameraCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS ?? '')) : undefined;
        const word = (v: number) => v << 16 >> 16;
        let shifts = 0, heights = 0, bearings = 0, paired = 0;
        for (let v = 0; v < 65536; v++) for (const shift of [0, 1, 2, 3, 15, 16]) {
            g.call('Camera_floorShift', [v, shift]);
            const expected = shift < 16 ? word(v) >> shift : 0;
            expect(g.s16(g.symbol('tcc__r0')), `floorShift ${word(v)}/${shift}`).toBe(expected);
            if (ref && !(v & 255)) {
                ref.call('floorShift', [v, shift]);
                expect(ref.s16(ref.symbol('tcc__r0'))).toBe(expected); paired++;
            }
            shifts++;
        }
        const local = (base: number, offset: number) => (base & 0xFF0000) | ((base + offset) & 65535);
        for (let i = 0; i < 65536; i++) {
            const min = word(i * 2 + 31), max = word(i), y = word(i * 677 + 32760);
            const offset = word(max + ((min - max) * 3 >> 2));
            const expected = word(y + offset);
            for (const cart of [g, ref && i % 127 === 0 ? ref : undefined]) {
                if (!cart) continue;
                const item = [cart.item,0x000520,0x7F9000,0x7EFFF9][i & 3];
                const table = i & 4 ? 0x7FFFF8 : 0x7F8000, anim = i & 15;
                // Constant packed streams isolate full-width GBA offset plus
                // the port's wrapped world Y. Interpolation is checked above.
                const tableBase = table === 0x7FFFF8 ? 0x7FFE00 : table;
                cart.w16(cart.symbol('olLevel') + 8, tableBase);
                cart.w16(cart.symbol('olLevel') + 10, tableBase >>> 16);
                cart.w16(cart.symbol('olLevel') + 12, 256);
                cart.w16(tableBase, 0); cart.w16(tableBase + 2, 0);
                cart.w16(tableBase + 4, 16);
                cart.w16(tableBase + 6 + anim * 8, 134);
                cart.w16(tableBase + 134, 0); cart.w16(tableBase + 136, offset);
                cart.w16(local(item, ITEM.animIndex), anim);
                cart.w16(local(item, ITEM.y), y);
                cart.call('Camera_targetY', [item & 65535, item >>> 16]);
                expect(cart.s16(cart.symbol('tcc__r0')), `target Y ${i}, item ${item.toString(16)}`).toBe(expected);
                if (cart === ref) paired++;
            }
            heights++;
        }
        for (let i = 0; i < 4096; i++) {
            const results: number[] = [];
            for (const cart of [g, ref]) {
                if (!cart) continue;
                const camera = cart.symbol('camera'), target = cart.symbol('camTarget');
                const x = word(i * 977), z = word(i * 613 + 32760);
                const dx = i % 7 ? word(i * 31 + 32767) : 0;
                const dz = i % 11 ? word(i * 47 - 32768) : 0;
                cart.w16(camera, x); cart.w16(camera + 4, z); cart.w16(camera + 8, 0xBEEF);
                cart.w16(target, x + dx); cart.w16(target + 4, z + dz);
                cart.call('Camera_lookAtTarget');
                results.push(cart.u16(camera + 8));
                if (!dx && !dz) expect(cart.u16(camera + 8)).toBe(0xBEEF);
                expect([cart.s16(camera),cart.s16(camera + 4),cart.s16(target),cart.s16(target + 4)])
                    .toEqual([x,z,word(x + dx),word(z + dz)]);
            }
            if (ref) { expect(results[0], `target yaw ${i}`).toBe(results[1]); paired++; }
            bearings++;
        }
        console.log(`[openlara] camera arithmetic: ${shifts} signed shifts, ${heights} wrapped target heights, ` +
            `${bearings} target bearings, ${paired} C comparisons, far/bank-wrapped inputs and ABI exact`);
    }, 600_000);

    it('matches every signed camera movement delta and preserves far input records', () => {
        const g = cameraCartridge();
        const ref = process.env.OL_REFERENCE_ROM ? cameraCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS ?? '')) : undefined;
        const word = (v: number) => (v << 16) >> 16;
        const edges = new Set([1,7,8,32767,32768,65528,65534,65535]);
        let nativeSteps = 0, comparedSteps = 0, cSteps = 0, cCases = 0;
        for (let i = 0; i < 65536; i++) {
            const view = [word(i * 313 + 32760),word(i * 223 - 32768),word(i * 197 + 31),
                i ^ 0x5A5A,0x6000,0x3456,1536];
            // Each axis independently covers all 65,536 signed deltas.
            const wanted = [word(view[0] + i),word(view[1] + i * 257 + 1),word(view[2] - i - 3),65535];
            const expected = view.map((v, a) => a < 3 ? word(v + Math.floor(word(wanted[a] - v) / 8)) : v);
            expected[3] = 65535;
            const compare = !(i & 255) || edges.has(i);
            for (const cart of [g, compare ? ref : undefined]) {
                if (!cart) continue;
                const camera = cart.symbol('camera'), to = [0x7E0520,0x0520,far(0x7FFFF8),far(0x7F9000)][i & 3];
                view.forEach((v, a) => cart.w16(camera + a * 2, v));
                wanted.forEach((v, a) => cart.w16(to + a * 2, v));
                cart.w16(to - 2, 0xA55A);
                const queries: number[][] = [];
                const steps = cart.call('Camera_move', [to & 65535,to >>> 16], {
                    name: 'Room_query', run: stack => queries.push([cart.u16(stack + 4),
                        cart.s16(stack + 6),cart.s16(stack + 8),cart.s16(stack + 10)]) });
                expect(Array.from({ length: 7 }, (_, a) => a < 3 ? cart.s16(camera + a * 2) : cart.u16(camera + a * 2)),
                    `${cart === g ? 'native' : 'C'} camera shift ${i}`).toEqual(expected);
                expect(queries, `invalid-room camera query order ${i}`).toEqual([
                    [65535,...expected.slice(0,3)],[65535,...expected.slice(0,3)]]);
                expect(Array.from({ length: 4 }, (_, a) => a < 3 ? cart.s16(to + a * 2) : cart.u16(to + a * 2))).toEqual(wanted);
                expect(cart.u16(to - 2)).toBe(0xA55A);
                if (cart === g) { nativeSteps += steps; if (compare && ref) comparedSteps += steps; }
                else { cSteps += steps; cCases++; }
            }
        }
        if (ref) expect(comparedSteps).toBeLessThan(cSteps);
        console.log(`[openlara] native camera movement shifts: 65536 cases cover every signed delta on all three axes, ` +
            `two ordered failed queries and far input preservation; ${nativeSteps} instructions; ${cCases} C cases ${comparedSteps}/${cSteps}`);
    }, 600_000);

    it.skipIf(!process.env.OL_REFERENCE_ROM)('matches C camera movement, ordered room queries and aliased inputs across target memory banks', () => {
        const g = cameraCartridge(), ref = cameraCartridge(
            process.env.OL_REFERENCE_ROM!, readSymbols(process.env.OL_REFERENCE_SYMS!));
        const word = (v: number) => (v << 16) >> 16;
        const bytes = (cart: typeof g, at: number, size: number) => Buffer.from(
            Array.from({ length: size }, (_, i) => cart.memory.read(at + i)));
        const peekCamera = (cart: typeof g) => Array.from({ length: 7 }, (_, a) => a < 3 ?
            cart.s16(cart.symbol('camera') + a * 2) : cart.u16(cart.symbol('camera') + a * 2));
        const heights = [-32768,-2049,-2048,-1,0,1536,1791,1792,2047,2048,2049,32767];
        const deltas = [-32768,-32767,-4096,-1025,-1024,-9,-8,-1,0,1,7,8,1023,1024,4096,32767];
        let cases = 0, nativeSteps = 0, cSteps = 0, queries = 0, heightTraces = 0, changedRooms = 0;
        for (let variant = 0; variant < 8; variant++) {
            const origin = [0,31744,-32768,-4096][variant & 3];
            for (const cart of [g, ref]) {
                cart.set({ olRoomCount: 3, olSectorCount: 192 });
                if (FX3) for (const [name, at] of [['Room', variant & 1 ? 0xF8FFE2 : 0xF80000],
                    ['Link', variant & 1 ? 0xF9FFD0 : 0xF90000]] as const) {
                    cart.w16(cart.symbol(`ol${name}Data`), at); cart.w16(cart.symbol(`ol${name}Data`) + 2, at >>> 16);
                }
                for (let link = 0; link < 6; link++) for (let b = 0; b < 8; b++)
                    cart.w8(cart.table('Link') + link * 8 + b, b < 4 ? 255 : 0);
                [-31,47,3,-5].forEach((v, i) => cart.w8(cart.table('Link') + 12 + i, v));
                cart.w8(cart.table('Link') + 2 * 8 + 2, 1);
                cart.w8(cart.table('Link') + 3 * 8, 1);
                cart.w8(cart.table('Link') + 4 * 8 + 1, 2);
                for (let room = 0; room < 3; room++) {
                    cart.room(room, false);
                    cart.w16(cart.table('Room') + room * 10, origin);
                    cart.w16(cart.table('Room') + room * 10 + 2, origin);
                    for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++) {
                        const choice = variant && room === 0 ? (x * 5 + z * 3 + variant) % 13 : 0;
                        cart.sector(room, x, z, choice === 5 ? -32512 : [2048,4096,-2048][room],
                            [-2048,2048,-4096][room], choice < 5 ? choice : 0);
                    }
                }
            }
            for (let i = 0; i < 256; i++) {
                const results: { view: number[]; target: Buffer; input: Buffer; probes: number[][] }[] = [];
                for (const cart of [g, ref]) {
                    cart.call('Room_reset');
                    const camera = cart.symbol('camera'), target = cart.symbol('camTarget');
                    const style = i % 10;
                    const to = [0x7E0520,0x0520,far(0x7FFFF8),far(0x7F9000),camera,camera + 1,camera + 2,camera + 4,camera + 6,target][style];
                    [word(origin + 2560 + (i % 7) * 97),heights[i % heights.length],
                        word(origin + 2560 + (i % 5) * 179),i % 3,0x6000,0x3456,1536]
                        .forEach((v, a) => cart.w16(camera + a * 2, v));
                    [word(origin + 2048 + i % 1024),heights[(i * 7 + 3) % heights.length],
                        word(origin + 3072 - i % 1024),i % 3].forEach((v, a) => cart.w16(target + a * 2, v));
                    [word(origin + 2560 + deltas[i % deltas.length]),heights[(i * 5 + 1) % heights.length],
                        word(origin + 2560 + deltas[(i * 7 + 3) % deltas.length]),i % 17 ? 0 : 65535]
                        .forEach((v, a) => cart.w16(to + a * 2, v));
                    // Simulate the first three writes independently at byte
                    // granularity: an unaligned/aliased input can change
                    // under the next axis before the first query occurs.
                    const expected = new Map<number, number>();
                    const phys = (at: number) => at < 0x2000 ? at + 0x7E0000 : at;
                    for (const [at, size] of [[camera,14],[to,8]]) for (let b = 0; b < size; b++)
                        expected.set(phys(at + b), cart.memory.read(at + b));
                    const get = (at: number) => expected.get(phys(at))! | expected.get(phys(at + 1))! << 8;
                    for (let a = 0; a < 3; a++) {
                        const at = camera + a * 2, v = word(get(at) + Math.floor(word(get(to + a * 2) - get(at)) / 8));
                        expected.set(phys(at), v & 255); expected.set(phys(at + 1), v >>> 8 & 255);
                    }
                    const lerped = Array.from({ length: 7 }, (_, a) => a < 3 ? word(get(camera + a * 2)) : get(camera + a * 2));
                    const firstQuery = [get(to + 6),...lerped.slice(0,3),...lerped];
                    const probes: number[][] = [], inputBefore = bytes(cart, to, 8), targetBefore = bytes(cart, target, 8);
                    const steps = cart.call('Camera_move', [to & 65535,to >>> 16], { name: 'Room_query', run: stack =>
                        probes.push([cart.u16(stack + 4),cart.s16(stack + 6),cart.s16(stack + 8),cart.s16(stack + 10),...peekCamera(cart)]) });
                    expect(probes[0], `first camera query ${variant}/${i}, alias ${style}`).toEqual(firstQuery);
                    if (style < 4 || style === 9) expect(bytes(cart, to, 8), 'camera movement changed an independent input').toEqual(inputBefore);
                    expect(bytes(cart, target, 8), 'camera movement changed the trace source').toEqual(targetBefore);
                    results.push({ view: peekCamera(cart),target: bytes(cart, target, 8),input: bytes(cart, to, 8),probes });
                    if (cart === g) {
                        nativeSteps += steps; queries += probes.length;
                        if (probes.length > 2) heightTraces++;
                        if (probes[0][0] !== probes[1][0]) changedRooms++;
                    } else cSteps += steps;
                }
                expect(results[0], `complete camera move ${variant}/${i}, alias ${i % 10}`).toEqual(results[1]);
                cases++;
            }
        }
        expect(heightTraces).toBeGreaterThan(10); expect(changedRooms).toBeGreaterThan(0);
        expect(nativeSteps).toBeLessThan(cSteps);
        console.log(`[openlara] native camera movement: ${cases} exact C snapshots, ${queries} ordered queries, ` +
            `${heightTraces} height traces, ${changedRooms} room changes, ten far/alias layouts and target-specific tables; ${nativeSteps}/${cSteps} instructions`);
    }, 600_000);

    it.skipIf(!process.env.OL_REFERENCE_ROM)('preserves complete C follow, snap and fallback-cache snapshots on original geometry', () => {
        const g = cameraCartridge(), ref = cameraCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS ?? ''));
        const rooms: { x: number; z: number; width: number; depth: number; sectors: number[][] }[] =
            JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8')).rooms;
        const fields: [string, number][] = [['camera',32],['camTarget',8],['tcTo',8],['clipIn',8],['clipOut',8],
            ...['fbValid','fbSX','fbSY','fbSZ','fbAngle','fbTick','fbLastTick','fbDir','fbRoom',
                'tcValid','tcMode','tcClear','tcSX','tcSY','tcSZ','tcWX','tcWY','tcWZ','clipValid',
                'pitchValid','pitchKey','pitchDist'].map(name => [name,2] as [string,number])];
        const snapshot = (cart: typeof g) => fields.map(([name, size]) => [name, Buffer.from(
            Array.from({ length: size }, (_, i) => cart.memory.read(cart.symbol(name) + i)))]);
        const itemBytes = (cart: typeof g) => Buffer.from(
            Array.from({ length: ITEM_SIZE }, (_, i) => cart.memory.read(cart.item + i)));
        for (const cart of [g, ref]) { cart.restoreLevel(); cart.call('Camera_init'); cart.anim(103); }
        let nativeSteps = 0, cSteps = 0, fallbackTicks = 0, coldTraces = 0;
        for (let i = 0; i < 384; i++) {
            const r = (i >>> 5) % rooms.length, room = rooms[r], sx = (r * 3 + 1) % room.width, sz = (r * 7 + 1) % room.depth;
            const name = (i & 31) ? 'Camera_updateFollow' : 'Camera_snap';
            const x = room.x + sx * 1024 + 512 + ((i & 31) - 16) * 29;
            const z = room.z + sz * 1024 + 512 + ((i & 31) - 16) * 17;
            const y = room.sectors[sx * room.depth + sz][0] * 256 - (i % 5) * 128;
            for (const cart of [g, ref]) {
                cart.w8(cart.item + ITEM.room, r);
                for (const [offset, value] of [[ITEM.x,x],[ITEM.y,y],[ITEM.z,z],[ITEM.angleY,i * 977]])
                    cart.w16(cart.item + offset, value);
                const before = itemBytes(cart);
                const steps = cart.call(name, cart.args, { name: 'Camera_getBestLocation', run: () => {
                    if (cart === g) coldTraces++;
                } });
                if (cart === g) nativeSteps += steps; else cSteps += steps;
                expect(itemBytes(cart), `${name} changed its target item`).toEqual(before);
            }
            expect(snapshot(g), `${name} full camera/cache case ${i}, room ${r}`).toEqual(snapshot(ref));
            if (g.u16(g.symbol('fbValid'))) fallbackTicks++;
        }
        expect(coldTraces).toBeGreaterThan(0); expect(fallbackTicks).toBeGreaterThan(0);
        expect(nativeSteps).toBeLessThan(cSteps);
        console.log(`[openlara] native camera follow: 384 complete C snapshots, ${coldTraces} cold trace entries, ` +
            `${fallbackTicks} fallback-cache ticks; ${nativeSteps}/${cSteps} instructions`);
    }, 600_000);

    it('matches every angle and signed distance with C camera coordinates and aliased far outputs', () => {
        const g = cameraCartridge();
        const ref = process.env.OL_REFERENCE_ROM ? cameraCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS ?? '')) : undefined;
        const word = (v: number) => (v << 16) >> 16;
        const distances = [0,1,-1,256,-256,1536,-1536,32767,-32768];
        let cases = 0, cCases = 0, nativeSteps = 0, comparedSteps = 0, cSteps = 0;
        const exercise = (angle: number, distance: number, style: number, compare: boolean) => {
            const x = word(cases * 719 + 32760), z = word(cases * 379 + 7);
            const sine = (a: number) => g.s16(g.symbol('olSinTable') + (((a + 32) & 65535) >>> 6) * 2);
            const s = sine(angle), c = sine(angle + 16384);
            for (const cart of [g, compare ? ref : undefined]) {
                if (!cart) continue;
                const camera = cart.symbol('camera'), target = cart.symbol('camTarget');
                const normal = [0x7E0520,0x0520,far(0x7FFFFE),far(0x7F9000)][style & 3];
                const outX = style === 4 ? camera + 14 : style === 5 ? target + 4 : normal;
                const outZ = style === 6 ? outX : style === 7 ? target : 0x7E0560;
                const expected = new Map<number, number>();
                const phys = (at: number) => at < 0x2000 ? at + 0x7E0000 : at;
                const put = (at: number, v: number) => {
                    expected.set(phys(at), v & 255); expected.set(phys(at + 1), v >>> 8 & 255);
                };
                for (const [at, size] of [[camera,18],[target,8],[outX,2],[outZ,2]]) {
                    for (let b = -2; b < size; b++) {
                        cart.w8(at + b, 0xA5); expected.set(phys(at + b), 0xA5);
                    }
                }
                for (const [at, value] of [[camera + 14,distance],[target,x],[target + 4,z]]) {
                    cart.w16(at, value); put(at, value);
                }
                const readExpected = (at: number) => word(expected.get(phys(at))! | expected.get(phys(at + 1))! << 8);
                put(outX, word(x - (distance * s >> 14)));
                // distH as it was on entry: both products precede the writes.
                put(outZ, word(readExpected(target + 4) - (distance * c >> 14)));
                const steps = cart.call('Camera_getLocationForAngle_xz',
                    [angle, outX & 65535, outX >>> 16, outZ & 65535, outZ >>> 16]);
                if (cart === g) { nativeSteps += steps; if (compare && ref) comparedSteps += steps; }
                else { cCases++; cSteps += steps; }
                for (const [at, value] of expected) expect(cart.memory.read(at),
                    `${cart === g ? 'native' : 'C'} angle ${angle} distance ${distance} alias ${style} byte ${at.toString(16)}`).toBe(value);
            }
            cases++;
        };
        for (let a = 0; a < 65536; a++) exercise(a, distances[a % distances.length], a & 3, !(a & 63));
        for (let d = 0; d < 65536; d++) exercise((d * 149 + 31) & 65535, word(d), d & 3, !(d & 63));
        for (const a of [0,31,32,33,16383,16384,32767,32768,65503,65504,65535])
            for (const d of distances) for (let style = 0; style < 8; style++) exercise(a, d, style, true);
        if (ref) expect(comparedSteps).toBeLessThan(cSteps);
        console.log(`[openlara] native camera coordinates: ${cases} angle/distance/alias cases, ${nativeSteps} instructions; ` +
            `${cCases} C snapshots ${comparedSteps}/${cSteps} instructions`);
    }, 600_000);

    it('matches C query outputs, failure preservation and overlapping pointer write order', () => {
        const g = cameraCartridge(); g.restoreLevel();
        const ref = process.env.OL_REFERENCE_ROM ? cameraCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS ?? '')) : undefined;
        ref?.restoreLevel();
        const rooms: { x: number; z: number; width: number; depth: number; sectors: number[][] }[] =
            JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8')).rooms;
        const word = (v: number) => (v << 16) >> 16;
        let nativeSteps = 0, cSteps = 0, failures = 0, translated = 0, nativeRoomSteps = 0, cRoomSteps = 0;
        let nativeWarmSteps = 0, cWarmSteps = 0, nativeRoomWarmSteps = 0, cRoomWarmSteps = 0;
        for (let i = 0; i < 2048; i++) {
            const r = i % rooms.length, room = rooms[r], sx = (i * 3) % room.width, sz = (i * 5) % room.depth;
            const args = [i % 17 ? r : 65535, word(room.x + sx * 1024 + (i & 1023)),
                word(room.sectors[sx * room.depth + sz][0] * 256 + ((i % 7) - 3) * 256),
                word(room.z + sz * 1024 + ((i * 271) & 1023))];
            g.call('Room_reset');
            g.call('Room_query', [...args, far(0x7F9000) & 65535, far(0x7F9000) >>> 16]);
            const result = g.u16(g.symbol('tcc__r0'));
            const rawFloor = g.s16(far(0x7F9002));
            const values = [rawFloor === 32767 ? -32512 : rawFloor, g.s16(far(0x7F9004)), g.u16(far(0x7F9000))];
            if (!result) failures++; else if (rawFloor === 32767) translated++;
            const base = [0x7E0520,0x0520,far(0x7FFFF8)][i % 3];
            const pointers = i % 4 === 0 ? [base,base,base] : i % 4 === 1 ? [base,base + 1,base + 2] : [base,base + 2,base + 4];
            const expected = Buffer.alloc(10, 0xA5);
            if (result) values.forEach((v, n) => expected.writeUInt16LE(v & 65535, pointers[n] - base + 2));
            for (const cart of [g, ref]) {
                if (!cart) continue;
                cart.call('Room_reset');
                for (let b = -2; b < 8; b++) cart.w8(base + b, 0xA5);
                for (let probe = 0; probe < 3; probe++) {
                    const steps = cart.call('camQuery', [...args, ...pointers.flatMap(at => [at & 65535, at >>> 16])]);
                    if (probe === 0) { if (cart === g) nativeSteps += steps; else cSteps += steps; }
                    else { if (cart === g) nativeWarmSteps += steps; else cWarmSteps += steps; }
                    expect(cart.u16(cart.symbol('tcc__r0')), `query result ${i}, reuse ${probe}`).toBe(result);
                    expect(Buffer.from(Array.from({ length: 10 }, (_, b) => cart.memory.read(base - 2 + b))),
                        `${cart === g ? 'native' : 'C'} query ${i}, reuse ${probe}, ${args}, outputs ${pointers}`).toEqual(expected);
                }
                cart.call('Room_reset');
                for (let probe = 0; probe < 3; probe++) {
                    const steps = cart.call('camRoomAt', args);
                    if (probe === 0) { if (cart === g) nativeRoomSteps += steps; else cRoomSteps += steps; }
                    else { if (cart === g) nativeRoomWarmSteps += steps; else cRoomWarmSteps += steps; }
                    expect(cart.u16(cart.symbol('tcc__r0')), `resolved/fallback camera room ${i}, reuse ${probe}`).toBe(result ? values[2] : args[0]);
                    expect(Buffer.from(Array.from({ length: 10 }, (_, b) => cart.memory.read(base - 2 + b))),
                        `room-only resolver changed output canaries ${i}, reuse ${probe}`).toEqual(expected);
                }
            }
        }
        expect(failures).toBeGreaterThan(0); expect(translated).toBeGreaterThan(0);
        // Cache admission adds work on a cold miss. Retain that measurement
        // and require two reuses to repay it; flushing before every call
        // cannot measure the benefit of caching interleaved gameplay probes.
        if (ref) {
            expect(nativeSteps + nativeWarmSteps).toBeLessThan(cSteps + cWarmSteps);
            expect(nativeRoomSteps + nativeRoomWarmSteps).toBeLessThan(cRoomSteps + cRoomWarmSteps);
        }
        console.log(`[openlara] native camera queries: 2048${ref ? ' C' : ''} cases, ${failures} failures preserve outputs, ` +
            `${translated} no-floor translations; cold ${nativeSteps}/${cSteps}, two reuses ${nativeWarmSteps}/${cWarmSteps} instructions; ` +
            `room resolver cold ${nativeRoomSteps}/${cRoomSteps}, two reuses ${nativeRoomWarmSteps}/${cRoomWarmSteps}`);
    });

    it('matches C and independent sector walks with slopes, portals, signed seams and far pointers', () => {
        const g = cameraCartridge();
        const ref = process.env.OL_REFERENCE_ROM ? cameraCartridge(
            process.env.OL_REFERENCE_ROM, readSymbols(process.env.OL_REFERENCE_SYMS ?? '')) : undefined;
        type Loc = [number, number, number, number];
        const word = (v: number) => (v << 16) >> 16;
        let nativeSteps = 0, referenceSteps = 0, cases = 0, hits = 0, clears = 0, seam = 0;
        let queryCount = 0, changedRooms = 0, firstHits = 0, secondHits = 0;
        const outcomes = new Set<string>();
        const read = (cart: typeof g, at: number, size: number) => Buffer.from(
            Array.from({ length: size }, (_, i) => cart.memory.read(at + i)));
        const exercise = (name: string, from: Loc, initialTo: Loc, index: number) => {
            const queries: number[][] = [];
            const query = (room: number, x: number, y: number, z: number) => {
                const args = [room, word(x), word(y), word(z)]; queries.push(args);
                const out = far(0x7F9000);
                g.call('Room_query', [...args, out & 65535, out >>> 16]);
                if (!g.u16(g.symbol('tcc__r0'))) { outcomes.add('invalid'); return undefined; }
                return [g.u16(out), g.s16(out + 2) === 32767 ? -32512 : g.s16(out + 2), g.s16(out + 4)];
            };
            const to = [...initialTo] as Loc;
            const axis = (primary: number) => {
                const d = word(to[primary] - from[primary]);
                if (!d) { outcomes.add('zero'); return 1; }
                if (!((from[primary] ^ to[primary]) & 0xFC00)) {
                    outcomes.add('same-sector'); to[3] = from[3]; return 1;
                }
                const slope = [0,1,2].map(a => a === primary ? (d < 0 ? -1024 : 1024) :
                    Math.trunc(word(to[a] - from[a]) * 1024 / d));
                const p = [...from] as Loc;
                p[primary] = word(d < 0 ? p[primary] & ~1023 : p[primary] | 1023);
                const offset = word(p[primary] - from[primary]);
                for (let a = 0; a < 3; a++) if (a !== primary)
                    p[a] = word(p[a] + (Math.imul(slope[a], offset) >> 10));
                for (let visits = 0; word(d < 0 ? p[primary] - to[primary] : to[primary] - p[primary]) > 0; visits++) {
                    expect(visits, 'independent sector walk did not converge').toBeLessThan(33);
                    for (let side = 0; side < 2; side++) {
                        const probe = [...p]; probe[primary] = word(probe[primary] + (side ? (d < 0 ? -1 : 1) : 0));
                        const q = query(p[3], probe[0], probe[1], probe[2]);
                        if (!q) continue;
                        if (probe[1] > q[1] || probe[1] < q[2]) {
                            for (let a = 0; a < 4; a++) to[a] = p[a];
                            if (side) secondHits++; else firstHits++;
                            outcomes.add(side ? 'second-probe' : 'first-probe'); return 0;
                        }
                        if (p[3] !== q[0]) changedRooms++;
                        p[3] = q[0];
                    }
                    for (let a = 0; a < 3; a++)
                        p[a] = word(p[a] + (a === primary || d > 0 ? slope[a] : -slope[a]));
                }
                to[3] = p[3]; return 1;
            };
            g.call('Room_reset');
            let result: number;
            if (name !== 'Room_trace') result = axis(name === 'traceX' ? 0 : 2);
            else {
                const dx = word(Math.abs(word(to[0] - from[0]))), dz = word(Math.abs(word(to[2] - from[2])));
                result = axis(dz > dx ? 0 : 2);
                if (!axis(dz > dx ? 2 : 0)) result = 0;
                else {
                    const dy = word(to[1] - from[1]);
                    if (dy) {
                        const q = query(to[3], to[0], to[1], to[2]);
                        if (q) {
                            let h = q[1];
                            if (to[1] <= h || from[1] >= h) {
                                h = q[2]; if (to[1] >= h || from[1] <= h) h = -32512;
                            }
                            if (h !== -32512) {
                                outcomes.add('height-intersection'); to[1] = h; h = word(h - from[1]);
                                to[0] = word(from[0] + Math.trunc(word(to[0] - from[0]) * h / dy));
                                to[2] = word(from[2] + Math.trunc(word(to[2] - from[2]) * h / dy));
                                result = 0;
                            }
                        }
                    }
                }
            }
            const source = [0x7E0520, 0x0520, far(0x7FFFE0)][index % 3];
            const dest = [far(0x7FFFF8), 0x0560, 0x7E0560][(index >>> 2) % 3];
            for (const cart of [g, ref]) {
                if (!cart) continue;
                cart.call('Room_reset');
                for (const at of [source, dest]) for (let b = -2; b < 8; b++) cart.w8(at + b, 0xA5);
                from.forEach((v, i) => cart.w16(source + i * 2, v));
                initialTo.forEach((v, i) => cart.w16(dest + i * 2, v));
                const before = read(cart, source - 2, 10), actualQueries: number[][] = [];
                const steps = cart.call(name, [source & 65535, source >>> 16, dest & 65535, dest >>> 16], {
                    name: 'Room_query', run: stack => actualQueries.push([cart.u16(stack + 4),
                        cart.s16(stack + 6), cart.s16(stack + 8), cart.s16(stack + 10)]) });
                if (cart === g) nativeSteps += steps; else referenceSteps += steps;
                const context = `${cart === g ? 'native' : 'C'} ${name} case ${cases}: ${from} -> ${initialTo}`;
                expect(cart.u16(cart.symbol('tcc__r0')), context).toBe(result);
                expect([cart.s16(dest), cart.s16(dest + 2), cart.s16(dest + 4), cart.u16(dest + 6)], context).toEqual(to);
                expect(actualQueries, `${context} changed probe order/coordinates`).toEqual(queries);
                expect(read(cart, source - 2, 10), `${context} changed source`).toEqual(before);
                expect(cart.u16(dest - 2), `${context} overwrote canary`).toBe(0xA5A5);
            }
            if (result) clears++; else hits++;
            if (Math.abs(from[0] - initialTo[0]) > 32768 || Math.abs(from[2] - initialTo[2]) > 32768) seam++;
            cases++; queryCount += queries.length;
        };
        const fractions = [0,1,511,512,1022,1023];
        const deltas = [0,1,-1,1023,-1023,1024,-1024,2048,-2048,4096,-4096,32767,-32768];
        const heights = [-32768,-2049,-2048,-1,0,2048,2049,32767];
        for (let variant = 0; variant < 8; variant++) {
            const origin = [0,31744,-32768,-4096][variant & 3];
            for (const cart of [g, ref]) {
                if (!cart) continue;
                cart.set({ olRoomCount: 3, olSectorCount: 192 });
                // Independent CPU-only banks, including tables ending exactly
                // at $FFFF. The FX cannot address either $F8 or $F9.
                if (FX3) for (const [name, at] of [['Room', variant & 1 ? 0xF8FFE2 : 0xF80000],
                    ['Link', variant & 1 ? 0xF9FFD0 : 0xF90000]] as const) {
                    cart.w16(cart.symbol(`ol${name}Data`), at); cart.w16(cart.symbol(`ol${name}Data`) + 2, at >>> 16);
                }
                for (let link = 0; link < 6; link++) for (let b = 0; b < 8; b++)
                    cart.w8(cart.table('Link') + link * 8 + b, b < 4 ? 255 : 0);
                [-31,47,3,-5].forEach((v, i) => cart.w8(cart.table('Link') + 12 + i, v));
                cart.w8(cart.table('Link') + 2 * 8 + 2, 1); // horizontal portal
                cart.w8(cart.table('Link') + 3 * 8, 1);     // below
                cart.w8(cart.table('Link') + 4 * 8 + 1, 2); // above
                for (let room = 0; room < 3; room++) {
                    cart.room(room, false);
                    cart.w16(cart.table('Room') + room * 10, origin);
                    cart.w16(cart.table('Room') + room * 10 + 2, origin);
                    for (let x = 0; x < 8; x++) for (let z = 0; z < 8; z++) {
                        const choice = variant && room === 0 ? (x * 5 + z * 3 + variant) % 13 : 0;
                        cart.sector(room, x, z, choice === 5 ? -32512 : [2048,4096,-2048][room],
                            [-2048,2048,-4096][room], choice < 5 ? choice : 0);
                    }
                }
            }
            for (let i = 0; i < 192; i++) {
                const from: Loc = [word(origin + 2048 + fractions[i % 6]), heights[(i >>> 2) % heights.length],
                    word(origin + 2048 + fractions[(i * 5 + 1) % 6]), i % 31 === 0 ? 65535 : 0];
                const to: Loc = [word(from[0] + deltas[i % deltas.length]),
                    i % 3 ? from[1] : heights[(i * 5 + 3) % heights.length],
                    word(from[2] + deltas[(i * 3 + 4) % deltas.length]), 2];
                for (const name of ['traceX','traceZ','Room_trace']) exercise(name, from, to, cases);
            }
        }
        // Real geometry supplies the original level's linked rooms/slants;
        // the independent walker asks point queries, never calls traceX/Z.
        g.restoreLevel(); ref?.restoreLevel();
        const rooms: { x: number; z: number; width: number; depth: number; sectors: number[][] }[] =
            JSON.parse(readFileSync(join(DATA_DIR, 'NAV.json'), 'utf8')).rooms;
        for (let i = 0; i < 256; i++) {
            const r = i % rooms.length, room = rooms[r], sx = (i * 5 + 1) % room.width, sz = (i * 7 + 1) % room.depth;
            const from: Loc = [word(room.x + sx * 1024 + fractions[i % 6]),
                word(room.sectors[sx * room.depth + sz][0] * 256 - 512),
                word(room.z + sz * 1024 + fractions[(i + 2) % 6]), r];
            const to: Loc = [word(from[0] + deltas[i % deltas.length]), word(from[1] + ((i % 5) - 2) * 511),
                word(from[2] + deltas[(i * 7 + 3) % deltas.length]), r];
            for (const name of ['traceX','traceZ','Room_trace']) exercise(name, from, to, cases);
        }
        expect([...outcomes]).toEqual(expect.arrayContaining(['invalid','zero','same-sector',
            'first-probe','second-probe','height-intersection']));
        expect(changedRooms).toBeGreaterThan(0); expect(seam).toBeGreaterThan(0);
        expect(hits).toBeGreaterThan(0); expect(clears).toBeGreaterThan(0);
        if (ref) expect(nativeSteps).toBeLessThan(referenceSteps);
        console.log(`[openlara] native camera traces: ${cases} independent${ref ? '/C' : ''} cases, ${queryCount} exact ordered probes; ` +
            `${hits} hits (${firstHits}/${secondHits} boundary sides), ${changedRooms} room changes, ${seam} seam rays; ` +
            `${nativeSteps}/${referenceSteps} instructions`);
    }, 600_000);
});

describe('OpenLara SNES/Super FX: camera', () => {
    it.skipIf(LEVEL !== 'LEVEL1')('arrives clipped, keeps her in the middle of the frame, and settles behind her', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const s16 = (a: number) => { const v = u16(a); return v & 0x8000 ? v - 0x10000 : v; };
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };
        const sItemW = (f: number) => s16(sym('items') + f);

        // TR angles: 65536 to the turn, 0 along +Z, a quarter turn along +X.
        const angTo = (dx: number, dz: number) =>
            Math.round((Math.atan2(dx, dz) / (2 * Math.PI)) * 65536) & 0xFFFF;
        const angDiff = (a: number, b: number) => {
            let d = (a - b) & 0xFFFF;
            if (d >= 0x8000) d -= 0x10000;
            return Math.abs(d);
        };
        const DEG = 182;

        // What the camera publishes, and where she is, read together after a
        // tick has completed so the two belong to the same tick.
        const ticksAddr = sym('ol_ticks');
        let lastTick = -1;
        const sample = () => {
            const t = u16(ticksAddr);
            if (t === lastTick) return null;
            lastTick = t;
            return {
                cx: s16(sym('ol_camX')), cy: s16(sym('ol_camY')), cz: s16(sym('ol_camZ')),
                yaw: u16(sym('ol_angle')),
                lx: sItemW(ITEM.x), ly: sItemW(ITEM.y), lz: sItemW(ITEM.z),
                facing: u16(sym('items') + ITEM.angleY),
            };
        };
        // The view has her in the middle of the frame when its yaw is the
        // direction from it to her: upstream's anglesFromVector at the end
        // of every update. Four degrees covers a mid-tick read.
        const centred = (v: NonNullable<ReturnType<typeof sample>>) =>
            angDiff(v.yaw, angTo(v.lx - v.cx, v.lz - v.cz));
        const dist = (v: NonNullable<ReturnType<typeof sample>>) =>
            Math.hypot(v.lx - v.cx, v.lz - v.cz);

        // The level arrives during the load tick, which publishes the
        // snapped view and its tick count last: wait for that tick to end,
        // and for the one after it, so the read is not the pre-load view.
        let t = 0;
        while (u16(sym('itemsCount')) === 0 && t < 600) { emulator.executeFrames(1); t++; }
        expect(u16(sym('itemsCount')), 'the host table never arrived').toBeGreaterThan(0);
        for (let seen = 0, t0 = u16(ticksAddr); seen < 2 && t < 1200; t++) {
            emulator.executeFrames(1);
            if (u16(ticksAddr) !== t0) { seen++; t0 = u16(ticksAddr); }
        }

        // The wanted view, 1536 behind her, is a sector inside the entrance
        // wall. The first view published must already be the traced and
        // clipped one, not that -- upstream arrives there at speed 1 -- and
        // level with the target, three quarters up her box.
        const first = sample()!;
        console.log(`[openlara] camera: first view (${first.cx},${first.cy},${first.cz}) ` +
                    `yaw ${first.yaw}, she is at (${first.lx},${first.ly},${first.lz})`);
        expect(first.cz, 'the first view is inside the entrance wall')
            .toBeGreaterThanOrEqual(3072 + 255);
        // Upstream's height: the target three quarters up her box (about
        // 580 above her feet standing) and the view distV = 1536 sin 10
        // degrees = 267 above that, looking down at it.
        expect(first.ly - first.cy, 'the view is not above her head as upstream puts it')
            .toBeGreaterThanOrEqual(780);
        expect(first.ly - first.cy, 'the view is too high')
            .toBeLessThanOrEqual(900);
        expect(centred(first), 'the first view does not look at her')
            .toBeLessThanOrEqual(4 * DEG);

        const pad = emulator.getController() as unknown as { buttonState: number };
        let worstCentred = 0, worstStep = 0, prevYaw = first.yaw;
        let last = first;
        const watch = (fields: number, tag: string, each?: (i: number) => void) => {
            for (let i = 0; i < fields; i++) {
                if (each) each(i);
                emulator.executeFrames(1);
                const v = sample();
                if (!v) continue;
                last = v;
                const c = centred(v);
                const step = angDiff(v.yaw, prevYaw);
                prevYaw = v.yaw;
                if (c > worstCentred) worstCentred = c;
                if (step > worstStep) worstStep = step;
                if (c > 4 * DEG || step > 24 * DEG) {
                    console.log(`[openlara] camera ${tag}: field ${i} view (${v.cx},${v.cz}) ` +
                                `yaw ${v.yaw} she (${v.lx},${v.lz}) facing ${v.facing} ` +
                                `centred ${c} step ${step}`);
                }
            }
        };

        // She runs down the corridor; the entrance wall stops blocking the
        // view behind her and the camera swings from her side to her back.
        pad.buttonState = PAD_UP;
        watch(150, 'running');
        pad.buttonState = 0;
        watch(150, 'stopping');

        let v = last;
        const behind = angDiff(v.yaw, v.facing);
        console.log(`[openlara] camera: after the run view (${v.cx},${v.cy},${v.cz}) yaw ${v.yaw} ` +
                    `she (${v.lx},${v.lz}) facing ${v.facing}: behind by ${behind}, ` +
                    `${Math.round(dist(v))} away; worst centred ${worstCentred}, step ${worstStep}`);
        expect(behind, 'the view did not settle behind her').toBeLessThanOrEqual(3 * DEG);
        expect(dist(v), 'the view is too close').toBeGreaterThanOrEqual(1200);
        expect(dist(v), 'the view is too far').toBeLessThanOrEqual(1700);

        // A quarter turn on the spot, written rather than pressed so that it
        // is exact: she faces -X, the view wants her east side, and swings
        // there round her rather than through her.
        const FACING = 49152;
        watch(300, 'turned', (i) => {
            if (i < 6) w16(sym('items') + ITEM.angleY, FACING);
        });

        v = last;
        const behind2 = angDiff(v.yaw, v.facing);
        console.log(`[openlara] camera: after the turn view (${v.cx},${v.cy},${v.cz}) yaw ${v.yaw} ` +
                    `she (${v.lx},${v.lz}) facing ${v.facing}: behind by ${behind2}, ` +
                    `${Math.round(dist(v))} away; worst centred ${worstCentred}, step ${worstStep}`);
        expect(v.facing, 'she did not keep the facing she was given').toBe(FACING);
        expect(behind2, 'the view did not settle behind her after the turn').toBeLessThanOrEqual(3 * DEG);
        expect(dist(v), 'the view is too close after the turn').toBeGreaterThanOrEqual(1200);

        expect(worstCentred, 'she left the middle of the frame').toBeLessThanOrEqual(4 * DEG);
        expect(worstStep, 'the view jumped').toBeLessThanOrEqual(24 * DEG);
    }, 600_000);
});

describe.skipIf(!FX3 || LEVEL !== 'LEVEL1')('OpenLara FX3: the near plane', () => {
    // The GSU's projection table stops at 512 of depth -- 65536 / dz has to
    // fit FMULT's signed word -- and GSU-2 clips there. Guns out at LEVEL1's
    // start, the combat camera clips to the entrance wall a few hundred units
    // from her, all of her is nearer than 512, and until 2026-10-05 nothing of
    // her was drawn. FX3 clips at upstream's 64 and projects 64..511 out of
    // line (mesh.gsu TM_Near/PC_Near/ShearNear; tools/nearcheck.py holds the
    // arithmetic). Held frozen, the frame with her and without her must differ
    // by her, and the frame with her must come back the same.
    it('draws her when the combat camera has clipped to within 512 of her', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        awaitLevel(emulator);
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const s16 = (a: number) => (u16(a) << 16) >> 16;
        const w16 = (a: number, v: number) => { memory.write(a, v & 0xFF); memory.write(a + 1, (v >> 8) & 0xFF); };
        const pad = emulator.getController() as unknown as { buttonState: number };
        emulator.executeFrames(30);
        pad.buttonState = PAD_R;                 // draw: the combat camera
        emulator.executeFrames(4);
        pad.buttonState = 0;
        emulator.executeFrames(150);

        const cam = sym('camera'), lara = sym('items');
        const away = () => Math.hypot(s16(cam) - s16(lara + ITEM.x), s16(cam + 4) - s16(lara + ITEM.z));
        const drawnAt = away(), weapon = u16(sym('olLaraWeaponState')), mode = u16(cam + 18);

        let freeze = 1;
        // Pinned at every render sync as well as once a field (the pickups
        // scene says why).
        const hold = (fields: number) => {
            const cpu = emulator.getCPU(), step = cpu.step, sync = sym('gameRenderSync');
            cpu.step = function (this: typeof cpu) {
                if ((this.PB << 16 | this.PC) === sync) { w16(sym('ol_ticks'), 0); w16(sym('ol_animFreeze'), freeze); }
                return step.call(this);
            };
            try {
                for (let i = 0; i < fields; i++) {
                    emulator.executeFrames(1);
                    w16(sym('ol_ticks'), 0); w16(sym('ol_animFreeze'), freeze);
                }
            } finally { cpu.step = step; }
        };
        const frame = () => emulator.getPPU().getFramebuffer().slice();
        const settle = () => {
            let last = frame();
            for (let n = 0; n < 40; n++) {
                hold(60);
                const now = frame();
                if (now.every((v, i) => v === last[i])) return now;
                last = now;
            }
            throw new Error('the picture never settled');
        };
        const differs = (a: ArrayLike<number>, b: ArrayLike<number>) => {
            let n = 0;
            for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) n++;
            return n;
        };
        const status = u8(lara + ITEM.flags2) & 3;
        const picture = (shown: boolean) => {
            memory.write(lara + ITEM.flags2, (u8(lara + ITEM.flags2) & ~3) | (shown ? status : 3));
            memory.write(sym('olItemDirty'), 1);
            return settle();
        };
        const her = picture(true), bare = picture(false), again = picture(true);
        const frozenAt = away();
        {
            const { stride, width, height } = emulator.getPPU().getFrameGeometry();
            writeFileSync(join(dirname(ROM), 'near_plane.png'), png(width, height, (x, y) => her[y * stride + x]));
            writeFileSync(join(dirname(ROM), 'near_plane_bare.png'), png(width, height, (x, y) => bare[y * stride + x]));
        }
        const shown = differs(her, bare), drift = differs(her, again);
        console.log(`[openlara] near plane: guns out (state ${weapon}, camera mode ${mode}) the view is ` +
            `${Math.round(drawnAt)} from her, ${Math.round(frozenAt)} frozen; she covers ${shown} words, ` +
            `${drift} differ when she is shown again`);
        expect(drawnAt, 'the combat camera did not come within 512 of her').toBeLessThan(512);
        expect(frozenAt, 'the frozen view is not the near one').toBeLessThan(512);
        expect(shown, 'she was not drawn').toBeGreaterThan(1000);
        expect(drift, 'the frame with her did not come back').toBe(0);
    }, 600_000);
});

describe('OpenLara SNES/Super FX: camera against upstream', () => {
    // Upstream's follow camera (src/fixed/camera.h) in the open is
    // arithmetic: want = target - 1536 (sin, cos) of her facing, the view
    // moves a floored eighth of the way there each tick. With the line from
    // her clear and the clip a no-op the port must match it bit for bit --
    // it did only on the tick she crossed a sector until 2026-09-23, when
    // the traced *place* was cached, not just the verdict. Read at each
    // gameUpdate entry, so every row is one whole tick; the port's own sine
    // table, so this tests the camera and not the trigonometry.
    it.skipIf(LEVEL !== 'LEVEL1')('follows upstream exactly in the open and carries the fallback with her', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const s16 = (a: number) => { const v = u16(a); return v & 0x8000 ? v - 0x10000 : v; };
        awaitLevel(emulator);
        emulator.executeFrames(30);
        const cam = sym('camera'), tgt = sym('camTarget'), it0 = sym('items');
        const rows: { tick: number, vx: number, vz: number, tx: number, tz: number, lx: number, lz: number, f: number,
                      dh: number, ws: number, look: number, lookY: number }[] = [];
        const cpu = emulator.getCPU(), original = cpu.step;
        let interruptedEntries = 0;
        cpu.step = function () {
            if ((cpu.PB << 16 | cpu.PC) === sym('gameUpdate')) {
                // cpu.step may service NMI before executing the instruction
                // at this PC. RTI then visits the same entry a second time;
                // that is still one logic tick, not a stationary camera step.
                const tick = u16(sym('ol_ticks'));
                if (rows.length && rows[rows.length - 1].tick === tick) interruptedEntries++;
                else rows.push({ tick, vx: s16(cam), vz: s16(cam + 4), tx: s16(tgt), tz: s16(tgt + 4),
                            lx: s16(it0 + ITEM.x), lz: s16(it0 + ITEM.z), f: u16(it0 + ITEM.angleY),
                            dh: s16(cam + 14), ws: u16(sym('olLaraWeaponState')),
                            look: u16(sym('olLaraLooking')),
                            lookY: s16(sym('olLaraLook') + 2) + s16(sym('olLaraLook') + 6) });
            }
            return original.call(this);
        };
        const pad = emulator.getController() as unknown as { buttonState: number };
        try {
            pad.buttonState = PAD_UP;       // down the entrance corridor
            emulator.executeFrames(150);
            pad.buttonState = 0;
            emulator.executeFrames(120);
            // Guns out, nothing to aim at: upstream's combat camera, 2048
            // away along her facing, then follow again once holstered.
            pad.buttonState = PAD_R;
            emulator.executeFrames(4);
            pad.buttonState = 0;
            emulator.executeFrames(150);
            pad.buttonState = PAD_R;
            emulator.executeFrames(4);
            pad.buttonState = 0;
            emulator.executeFrames(150);
            // Look (Y): the d-pad is her head's, and the camera's.
            pad.buttonState = PAD_Y | PAD_LEFT;
            emulator.executeFrames(60);
            pad.buttonState = PAD_Y;
            emulator.executeFrames(60);
            pad.buttonState = 0;
            emulator.executeFrames(120);
        } finally { cpu.step = original; }

        for (let i = 1; i < rows.length; i++)
            expect(rows[i].tick, 'camera replay skipped a logic tick').toBe((rows[i - 1].tick + 1) & 65535);

        const tab = (i: number) => s16(sym('olSinTable') + i * 2);
        // targetDist split by upstream's ten-degree pitch, as the cartridge
        // worked it out with its own table: distH across, distV up.
        const distH = rows[0].dh, distV = s16(cam + 16);
        const scale = (v: number, d: number) => Math.floor(d * v / 16384);
        const lerp = (v: number, goal: number) => v + Math.floor((goal - v) / 8);
        const lerp4 = (v: number, goal: number) => v + Math.trunc((goal - v) / 4);  // upstream's d /= 4
        const want = (r: typeof rows[0]) => {
            const a = r.look ? (r.f + r.lookY) & 0xFFFF : r.f;
            return { x: r.tx - scale(tab(((a + 32) & 0xFFFF) >> 6), r.dh),
                     z: r.tz - scale(tab(((a + 0x4020) & 0xFFFF) >> 6), r.dh) };
        };

        // 1. The wall behind her blocks the line; upstream's fallback picks
        //    her west side, 1536 away, and re-derives it from her every tick.
        let k = 0, fallback = 0;
        for (; k + 1 < rows.length && rows[k + 1].vx === rows[0].vx; k++, fallback++) {
            const a = rows[k], b = rows[k + 1];
            expect(b.vz, `fallback tick ${k}: the view does not follow her along the corridor`)
                .toBe(lerp(a.vz, b.tz));
        }
        expect(fallback, 'the fallback phase is missing: the start has changed').toBeGreaterThanOrEqual(8);
        expect(rows[0].vx, 'the fallback is not her west side').toBe(rows[0].lx - distH);
        expect(distH, 'distH is not 1536 cos 10 degrees').toBeGreaterThanOrEqual(1510);
        expect(distH, 'distH is not 1536 cos 10 degrees').toBeLessThanOrEqual(1514);
        // Upstream's -267 on its 4096-entry table; the port's 1024 entries
        // are 0.35 degrees apart and read sin(-10) as -0.171: -263.
        expect(distV, 'distV is not -1536 sin 10 degrees').toBeGreaterThanOrEqual(-268);
        expect(distV, 'distV is not -1536 sin 10 degrees').toBeLessThanOrEqual(-262);

        // 2. Past the entrance the line clears: from then on every tick,
        //    running, stopping and settling, is upstream's arithmetic.
        let firstExact = -1, exact = 0;
        const bad: string[] = [];
        for (k = fallback; k + 1 < rows.length; k++) {
            const a = rows[k], b = rows[k + 1], w = want(b);
            const step = b.look ? lerp4 : lerp;
            const ok = b.vx === step(a.vx, w.x) && b.vz === step(a.vz, w.z);
            if (firstExact < 0) { if (ok) firstExact = k; else continue; }
            if (ok) exact++;
            else if (bad.length < 6) bad.push(`tick ${k}${b.look ? ' look' : ''}: view (${b.vx},${b.vz}) upstream (${step(a.vx, w.x)},${step(a.vz, w.z)})`);
        }
        // The combat ticks: her facing, 2048 split by ten degrees.
        const combat = rows.filter(r => r.ws === 2 || r.ws === 4);
        const combatH = new Set(combat.map(r => r.dh));
        expect(combat.length, 'the pistols were never drawn').toBeGreaterThan(60);
        expect([...combatH].every(d => d >= 2014 && d <= 2020), `combat distH ${[...combatH]} is not 2048 cos 10`).toBe(true);
        const settledCombat = combat[combat.length - 1];
        const combatDist = Math.hypot(settledCombat.lx - settledCombat.vx, settledCombat.lz - settledCombat.vz);
        expect(combatDist, 'the combat view did not settle 2048 away').toBeGreaterThanOrEqual(1990);
        // Looking: she stays facing where she was, her head swings to
        // upstream's 44-degree clamp (torso with it), the view 768 away.
        const looking = rows.filter(r => r.look);
        expect(looking.length, 'Look never took the camera').toBeGreaterThan(50);
        expect(new Set(looking.map(r => r.f)).size, 'she turned while looking').toBe(1);
        expect(Math.min(...looking.map(r => r.lookY)), 'her head did not reach the clamp')
            .toBe(-2 * 44 * 182);
        expect(new Set(looking.map(r => r.dh)).size === 1 && looking[0].dh === 768,
               `look distH ${[...new Set(looking.map(r => r.dh))]}`).toBe(true);
        const last = rows[rows.length - 1];
        const dist = Math.hypot(last.lx - last.vx, last.lz - last.vz);
        console.log(`[openlara] camera vs upstream: fallback ${fallback} ticks, exact from tick ` +
                    `${firstExact}, ${exact} exact ticks (${combat.length} in combat, ${looking.length} looking, settled ` +
                    `${Math.round(combatDist)} away), settled ${Math.round(dist)} behind her; ` +
                    `${interruptedEntries} interrupted entries observed once`);
        expect(firstExact, 'the view never matched upstream in the open').toBeGreaterThanOrEqual(0);
        expect(firstExact, 'the line took too long to clear').toBeLessThanOrEqual(40);
        expect(bad, `the view left upstream's path in the open: ${bad.join('; ')}`).toEqual([]);
        expect(dist, 'the view did not settle 1536 behind her').toBeLessThanOrEqual(1560);
    }, 600_000);
});

describe('OpenLara SNES/Super FX: profile', () => {
    it.skipIf(FX3 || LEVEL !== 'LEVEL1')('services completed GSU-2 renders while logic ticks are queued', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        for (let i = 0; i < 180; i++) emulator.executeFrames(1);
        const memory = emulator.getMemory(), cpu = emulator.getCPU();
        const loopPC = sym('MainLoop') & 65535, framePC = sym('MainLoop@frame') & 65535;
        expect(memory.read(loopPC)).toBe(0xA5);
        const pendingAt = 0x7E0000 | memory.read(loopPC + 1);
        expect([memory.read(framePC), memory.read(framePC + 2), memory.read(framePC + 4)])
            .toEqual([0xA5, 0xD0, 0xA5]);
        const busyAt = 0x7E0000 | memory.read(framePC + 1);
        const doneAt = 0x7E0000 | memory.read(framePC + 5);
        const queuePC = sym('QueueUpload') & 65535, startPC = sym('StartGSU') & 65535;
        let field = 0, armed = false, queued = false, began = 0, publications = 0;
        let shown = memory.read(0x7E0002), flips = 0;
        const orig = cpu.step;
        cpu.step = function (this: typeof cpu) {
            const pc = this.PB << 16 | this.PC;
            // Hold a full queue only after a real GSU STOP, with its previous
            // upload finished. Real ticks, bus handoffs and DMA still run.
            // The old tick-first loop never reached @frame in this state.
            if (pc === loopPC && memory.read(doneAt) && !memory.read(busyAt)) {
                if (!armed) { armed = true; began = field; }
                memory.write(pendingAt, 4);
            }
            if (pc === queuePC && armed) {
                expect(memory.read(pendingAt), 'render waited for an empty logic queue').toBeGreaterThan(0);
                expect(memory.read(0x3030) & 0x20, 'publication reused a running GSU frame').toBe(0);
                queued = true;
            }
            if (pc === startPC && armed && queued) {
                expect(memory.read(doneAt)).toBe(0);
                expect(memory.read(busyAt)).toBe(1);
                expect(field - began, 'completed render starved behind queued ticks').toBeLessThanOrEqual(4);
                publications++; armed = false; queued = false;
            }
            return orig.call(this);
        };
        try {
            for (field = 1; field <= 240; field++) {
                emulator.executeFrames(1);
                const next = memory.read(0x7E0002);
                if (next !== shown) { flips++; shown = next; }
            }
        } finally { cpu.step = orig; }
        expect(publications, 'GSU-2 did not publish completed renders under backlog').toBeGreaterThanOrEqual(5);
        expect(flips, 'queued GSU-2 frames did not reach displayed VRAM').toBeGreaterThanOrEqual(5);
        console.log(`[openlara] GSU-2 backlog: ${publications} real completed renders published with ticks queued, ${flips} visible flips`);
    });

    it.skipIf(!FX3 || LEVEL !== 'LEVEL1')('bounds render deferral even with a permanently full logic queue', () => {
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        for (let i = 0; i < 180; i++) emulator.executeFrames(1);
        const memory = emulator.getMemory(), cpu = emulator.getCPU();
        const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
        const loopPC = sym('MainLoop') & 65535, framePC = sym('MainLoop@frame') & 65535;
        const holdPC = sym('MainLoop@holdRender') & 65535, publishPC = sym('MainLoop@publish') & 65535;
        const updatePC = sym('UpdateDemo') & 65535;
        const deadlineFields = SYMS.has('FX3PublishPriority') && SYMS.has('StageFX3Frame') ? 8 : 16;
        // Read linked direct-page operands, and refuse changed instruction
        // formats rather than silently relying on old .ENUM addresses.
        expect(memory.read(loopPC)).toBe(0xA5);
        const pendingAt = 0x7E0000 | memory.read(loopPC + 1);
        expect([memory.read(holdPC), memory.read(holdPC + 1), memory.read(holdPC + 2)])
            .toEqual([0xA9, 0x80 + deadlineFields, 0x85]);
        const holdAt = 0x7E0000 | memory.read(holdPC + 3);
        expect([memory.read(updatePC), memory.read(updatePC + 1), memory.read(updatePC + 2)]).toEqual([0xC2, 0x30, 0xA5]);
        const frameAt = 0x7E0000 | memory.read(updatePC + 3);
        const waits: { start: number; end: number; startCounter: number; endCounter: number }[] = [];
        let field = 0, armed = false, shown = memory.read(0x7E0002), flips = 0;
        let hold: { field: number; counter: number } | undefined;
        const orig = cpu.step;
        cpu.step = function (this: typeof cpu) {
            const pc = this.PB << 16 | this.PC;
            // An isolated overload fixture, NOT the live clock gate below:
            // force the queue full at every frame poll while still executing
            // real logic, NMI, render, conversion and upload work unchanged.
            if (pc === framePC) { memory.write(pendingAt, 4); armed = true; }
            if (pc === holdPC && armed) hold = { field, counter: u16(frameAt) };
            if (pc === publishPC && armed) {
                expect(memory.read(pendingAt)).toBe(4);
                expect(memory.read(holdAt), 'a full queue published before its deadline').toBe(0x80);
                expect(hold, 'publication was never deferred').toBeDefined();
                const age = field - hold!.field;
                // Eight NMIs can straddle seven field-number increments;
                // finishing the current real logic tick permits two more.
                expect(age, 'publication bypassed its logic headroom').toBeGreaterThanOrEqual(deadlineFields - 1);
                expect(age, 'a full queue starved render publication').toBeLessThanOrEqual(deadlineFields + 2);
                waits.push({ start: hold!.field, end: field, startCounter: hold!.counter, endCounter: u16(frameAt) });
                hold = undefined;
            }
            return orig.call(this);
        };
        try {
            for (field = 1; field <= 240; field++) {
                emulator.executeFrames(1);
                const next = memory.read(0x7E0002);
                if (next !== shown) { flips++; shown = next; }
            }
        } finally {
            cpu.step = orig;
        }
        expect(waits.length, 'too few forced publications').toBeGreaterThanOrEqual(5);
        expect(waits.some(w => w.endCounter < w.startCounter), 'the deadline never crossed frameIndex rollover').toBe(true);
        expect(flips, 'publication did not reach displayed VRAM').toBeGreaterThanOrEqual(5);
        console.log(`[openlara] saturated queue: ${waits.length} forced publications, ${flips} visible buffer flips; ` +
            `wait fields ${waits.map(w => w.end - w.start).join('/')}, including counter rollover`);
    });

    // FX3 LEVEL1 runs both original gates; Lost Valley adds three raptors.
    // Native GSU-2 also runs both pursuit gates; its historical walk/idle
    // profiles remain opt-in via OL_PROFILE. Attribute
    // elapsed bus clocks (not just opcode cycles) in WRAM and FastROM, and
    // save both flat self costs and selected inclusive JSL spans.
    const nativeGsu2 = !FX3 && SYMS.has('navCachedSectorBoxGsu2') && SYMS.has('WU_Info');
    const profileSpecies = process.env.OL_PROFILE_SPECIES ? [process.env.OL_PROFILE_SPECIES] :
        FX3 && LEVEL === 'LEVEL3A' ? ['19,19,19'] :
        (FX3 || nativeGsu2) && (!process.env.OL_PROFILE || process.env.OL_PROFILE === 'creatures') ? ['7,7,7', '7,7,8'] : ['7,7,7'];
    it.skipIf((!process.env.OL_PROFILE && !FX3 && !nativeGsu2) || (LEVEL !== 'LEVEL1' && !(FX3 && LEVEL === 'LEVEL3A'))).each(profileSpecies)(
        'profiles the full tick and sustains three active creatures [%s]', (selectedSpecies) => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);
        const scene = process.env.OL_PROFILE ?? 'creatures';
        const fields = Number(process.env.OL_PROFILE_FIELDS ?? 600);
        const inputStartField = Number(process.env.OL_PROFILE_INPUT_START ?? 1);
        expect(Number.isInteger(inputStartField) && inputStartField >= 1 && inputStartField <= fields).toBe(true);
        // Retain the original three-wolf workload and add two wolves/bear.
        // Overrides can select other supported real species combinations.
        const species = selectedSpecies.split(',').map(Number);
        expect(species).toHaveLength(3);
        for (const type of species) expect(FX3 ? [7,8,9,19] : [7,8,9]).toContain(type);
        const valley = LEVEL === 'LEVEL3A';
        const profileTag = scene === 'creatures' && selectedSpecies !== '7,7,7' ?
            `${scene}_${species.join('_')}` : scene;

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 255); memory.write(a + 1, v >> 8 & 255);
        };
        const pad = emulator.getController() as unknown as { buttonState: number };
        // The existing emulator exposes its timing bus as a runtime field.
        // CPU.cycles is opcode work, not elapsed time: the bus also charges
        // address-dependent memory accesses, DMA and raster refresh stalls.
        const bus = (emulator as unknown as { bus: { masterCycles: number } }).bus;
        const fieldClocks = TIMING[REGION_NTSC].linesPerFrame * TIMING[REGION_NTSC].cyclesPerLine;

        // Settle: the level has to arrive and the first frame be uploaded.
        for (let i = 0; i < 180; i++) emulator.executeFrames(1);
        expect(u16(sym('itemsCount')), 'the entity list never arrived').toBeGreaterThan(0);

        const cpu = emulator.getCPU();
        const clockHist = new Map<number, number>();
        // Optional read-only attribution of a particular slow stretch. Keep
        // the complete run and its gates unchanged, including warmup/input.
        const focusRange = process.env.OL_PROFILE_FOCUS?.split(':').map(Number);
        if (focusRange) {
            expect(focusRange).toHaveLength(2);
            expect(focusRange.every(Number.isInteger)).toBe(true);
            expect(focusRange[0]).toBeGreaterThanOrEqual(1);
            expect(focusRange[1]).toBeGreaterThanOrEqual(focusRange[0]);
            expect(focusRange[1]).toBeLessThanOrEqual(fields);
        }
        const focusHist = new Map<number, number>();
        const focusEvents: unknown[] = [];
        const focusWatch = new Map((focusRange ? ['StageFX3Frame', 'QueueUploadFX3Staged',
            'MainLoop@publish', 'PrepareFX3Start', 'StartQueuedFX3Frame@launch',
            'UploadFX3Cols@complete'] : []).filter(name => SYMS.has(name))
            .map(name => [sym(name) & 65535, name]));
        // Optional read-only camera spans expose cold fallback/trace bursts;
        // they must not change the emulated clocks or gameplay timeline.
        const cameraWatches = process.env.OL_PROFILE_CAMERA ? ['Camera_getBestLocation',
            'Camera_locationForAngle', 'Camera_getLocationForAngle_xz', 'Camera_clip', 'Camera_move',
            'Camera_targetY', 'Camera_pitch', 'Camera_followAngleX', 'Camera_lookAtTarget', 'floorShift',
            'Room_trace', 'traceX', 'traceZ', 'traceCheck', 'camQuery'] : [];
        if (process.env.OL_PROFILE_LARA) cameraWatches.push('Lara_updateWaterState',
            'Lara_updateState', 'Lara_updateRoom', 'Lara_updateCollision', 'Lara_collideObjects',
            'ItemObj_animProcess', 'ItemObj_move', 'angleDec');
        const watched = new Map(['gameUpdate', 'gameRenderSync', 'Enemy_update', 'Enemy_target', 'Wolf_update', 'Bear_update', 'Bat_update', 'Raptor_update', 'ItemObj_collideBounds', 'Nav_init',
            'Nav_getWaypoint', 'Nav_search', 'Nav_searchNext', 'Enemy_moveGround', 'navBoxAt', 'Lara_update',
            'ItemObj_collideRoom', 'Camera_updateFollow', 'Weapon_update', 'Enemy_navAcquire',
            'Enemy_navAcquireCold', 'Enemy_targetMood', 'Shadow_publish', 'Lara_itemFloorAt', 'Lara_collideItems',
            'CollisionInfo_setSide', 'colPoint', 'Room_query', 'Room_collideStatic', 'olStaticHit', 'ItemObj_animCmd',
            'ItemObj_animCmdEnd', 'Enemy_checkZone', 'Enemy_checkStalk',
            'Enemy_checkEscape', 'Enemy_setNextBox', 'Enemy_randomBox', 'Enemy_moodDistance', 'rand_logic',
            'Enemy_collideLara', 'ItemObj_frameContact', 'ItemObj_collideFrameBounds', 'ItemObj_pushFrame', 'ItemObj_frameBox', 'Frame_rotate', 'Frame_inBounds']
            .concat(cameraWatches)
            .flatMap(name => {
                const key = SYMS.has(name) ? name : [...SYMS.keys()].find(k => k.endsWith('_' + name));
                return key ? [[sym(key), name] as const] : [];
            }));
        if (cameraWatches.length) expect([...watched.values()]).toEqual(expect.arrayContaining(cameraWatches));
        type Span = { name: string; stack: number; end: number; begin: number };
        const spans: Span[] = [];
        const calls = new Map<string, number[]>();
        // Derive the queue address from the linked host instruction, not a
        // copied .ENUM offset. The opcode assertion catches a changed load
        // format before a profiler can silently sample the wrong variable.
        const loopPC = sym('MainLoop') & 65535;
        expect(memory.read(loopPC), 'MainLoop no longer starts with LDA direct-page').toBe(0xA5);
        const pendingAt = 0x7E0000 | memory.read(loopPC + 1);
        const dropPC = sym('NmiHandler@dropped') & 65535;
        const startupEvents: { field: number; event: string; clocks: number; elapsed?: number;
            pending: number; ticks: number; active?: string[] }[] = [];
        const fieldSamples: number[][] = [];
        let field = 0;
        let stackLow = cpu.S, setup = false;
        const creatures: number[] = [];
        const setScene = () => {
            if (scene !== 'creatures') return;
            // Real boxes, animation graphs and room collision: LEVEL1 room
            // 14 wolves/bear, or Lost Valley room 51 and actual raptor models.
            // No animation freeze, forced mood, waypoint pin or zero speed.
            for (const type of species) {
                const previous = creatures.length;
                for (let i = 1; i < u16(sym('itemsCount')); i++) {
                    if (memory.read(sym('items') + i * ITEM_SIZE + ITEM.type) === type && !creatures.includes(i)) {
                        creatures.push(i); break;
                    }
                }
                expect(creatures.length, `level lacks another type ${type} for the requested encounter`).toBe(previous + 1);
            }
            const active: number[] = [];
            for (let i = u16(sym('olFirstActive')); i !== 255;) {
                expect(active.length).toBeLessThan(128);
                if (!creatures.includes(i)) active.push(i);
                i = memory.read(sym('items') + i * ITEM_SIZE + ITEM.nextActive);
            }
            const list = [...creatures, ...active];
            w16(sym('olFirstActive'), list[0]);
            list.forEach((i, j) => memory.write(sym('items') + i * ITEM_SIZE + ITEM.nextActive, list[j + 1] ?? 255));
            const lara = sym('items');
            memory.write(lara + ITEM.room, valley ? 51 : 14);
            w16(lara + ITEM.x, valley ? -18944 : 22016); w16(lara + ITEM.y, valley ? 3328 : 6656);
            w16(lara + ITEM.z, valley ? -31232 : -17456);
            w16(lara + ITEM.angleY, valley ? 0x4000 : 0x8000); w16(lara + ITEM.health, 30000);
            creatures.forEach((i, j) => {
                const a = sym('items') + i * ITEM_SIZE;
                memory.write(a + ITEM.room, valley ? 51 : 14); memory.write(a + ITEM.flags, 0x40);
                memory.write(a + ITEM.flags2, 1 | (species[j] === 8 ? 32 : 0)); memory.write(a + ITEM.mood, 0);
                w16(a + ITEM.x, valley ? [-17920,-16896,-15872][j] : j === 1 ? 23040 : 22016);
                w16(a + ITEM.y, valley ? 3328 : j === 1 ? 6528 : 6656);
                w16(a + ITEM.z, valley ? -31232 : [-14848, -16896, -13824][j]);
                w16(a + ITEM.health, species[j] === 7 ? 6 : species[j] === 8 || species[j] === 19 ? 20 : 1);
                memory.write(sym('olItemDirty') + i, 1);
            });
            const camera = sym('camera');
            (valley ? [-20480,2816,-31232,51,0x4000,0x4000,1536] :
                [22016,6144,-15920,14,0x8000,0x8000,1536]).forEach((v, i) => w16(camera + i * 2, v));
        };
        const orig = cpu.step;
        const fx = emulator.getSuperFX(), readCPU = fx.readCPU, writeCPU = fx.writeCPU;
        const ramViolations: string[] = [];
        let auditedRamAccesses = 0, yieldCalls = 0, reclaimCalls = 0;
        let ramGrants = 0, ramReclaims = 0;
        const auditRam = (address: number, operation: string) => {
            const bank = address >>> 16, offset = address & 65535;
            if (bank !== 0x70 && bank !== 0x71 &&
                !((bank <= 0x3F || bank >= 0x80 && bank <= 0xBF) && offset >= 0x6000 && offset <= 0x7FFF)) return;
            auditedRamAccesses++;
            const state = fx.getDiagnostics();
            if (state.running && state.scmr & 8 && ramViolations.length < 8)
                ramViolations.push(`${operation} ${address.toString(16)} at ${(cpu.PB<<16|cpu.PC).toString(16)}`);
        };
        if (!FX3 && SYMS.has('olGsuYieldRam')) {
            fx.readCPU = function(address, openBus) { auditRam(address,'read'); return readCPU.call(this,address,openBus); };
            fx.writeCPU = function(address, value) {
                auditRam(address,'write');
                const pc = cpu.PB<<16|cpu.PC;
                if ((address & 65535) === 0x303A) {
                    if (pc >= sym('olGsuYieldRam') && pc < sym('olGsuReclaimRam')) ramGrants++;
                    if (pc >= sym('olGsuReclaimRam') && pc < sym('RQ_CacheReset')) ramReclaims++;
                }
                return writeCPU.call(this,address,value);
            };
        }
        const navBranches = process.env.OL_PROFILE_NAV_BRANCHES ? new Map(
            ['Nav_search','Nav_searchNext','NS_Loop','NS_Different','NS_Geometry','NS_Open','NS_Append']
                .filter(name => SYMS.has(name)).map(name => [sym(name),name])) : undefined;
        const navBranchCounts = new Map<string, number>();
        // Optional schedule regression: hash item/navigation/camera state at the
        // C tick's return, independent of which field displayed a new frame.
        // Only WRAM is observed; these reads cannot acknowledge device IRQs.
        const traceTicks = !!process.env.OL_PROFILE_TICK_TRACE || !!process.env.OL_PROFILE_REFERENCE_TRACE;
        if (traceTicks) expect(FX3, 'tick-state traces require the FX3 navigation contexts').toBe(true);
        const tickStates: Record<string, number | string>[] = [];
        let traceStartTick = 0;
        const stateHash = (name: string, size: number) => createHash('sha256').update(
            Buffer.from(Array.from({ length: size }, (_, i) => memory.read(sym(name) + i)))).digest('hex');
        cpu.step = function (this: typeof cpu) {
            const pc = (this.PB << 16) | this.PC;
            const focused = focusRange && field >= focusRange[0] && field <= focusRange[1];
            const focusEvent = focused && (focusWatch.get(pc) || (pc === sym('gameUpdate') && 'gameUpdate'));
            if (focusEvent) focusEvents.push({ field, event: focusEvent, clocks: bus.masterCycles - clocks0,
                pending: memory.read(pendingAt), ticks: (u16(sym('ol_ticks')) - ticks0) & 65535,
                loadHold: SYMS.has('olFrameLoadHold') ? u16(sym('olFrameLoadHold')) : 0,
                staged: SYMS.has('olFrameStaged') ? memory.read(sym('olFrameStaged')) : 0 });
            if (pc === SYMS.get('olGsuYieldRam')) yieldCalls++;
            if (pc === SYMS.get('olGsuReclaimRam')) reclaimCalls++;
            const navBranch = navBranches?.get(pc);
            if (navBranch) navBranchCounts.set(navBranch, (navBranchCounts.get(navBranch) ?? 0) + 1);
            if (pc === dropPC) startupEvents.push({ field, event: 'drop', clocks: bus.masterCycles - clocks0,
                pending: memory.read(pendingAt), ticks: (u16(sym('ol_ticks')) - ticks0) & 65535,
                active: spans.map(s => s.name) });
            // Fixture writes occur only at a gameUpdate call boundary.
            if (!setup && pc === sym('gameUpdate')) {
                // The field observation can begin midway through a boot tick.
                // Its return is counted by the clock gauge, but it predates
                // this fixture and has no captured call span. Number replay
                // ticks from the actual scene boundary; leave timing totals
                // and all gameplay state untouched.
                traceStartTick = u16(sym('ol_ticks'));
                setScene(); setup = true;
            }
            const name = watched.get(pc);
            if (name && !spans.some(s => s.name === name && s.stack === this.S)) {
                const ret = (u16(this.S + 1) + 1) & 65535;
                spans.push({ name, stack: this.S, end: ret | memory.read(this.S + 3) << 16,
                    begin: bus.masterCycles });
            }
            const clockBefore = bus.masterCycles;
            const r = orig.call(this);
            clockHist.set(pc, (clockHist.get(pc) ?? 0) + bus.masterCycles - clockBefore);
            if (focused) focusHist.set(pc, (focusHist.get(pc) ?? 0) + bus.masterCycles - clockBefore);
            stackLow = Math.min(stackLow, this.S);
            // A JML tail call shares its caller's return and stack. Finish
            // every matching span, not just the innermost one.
            while (spans.length) {
                const top = spans[spans.length - 1];
                if ((this.PB << 16 | this.PC) !== top.end || this.S !== top.stack + 3) break;
                if (traceTicks && top.name === 'gameUpdate') tickStates.push({
                    tick: (u16(sym('ol_ticks')) - traceStartTick) & 65535, keys: u16(sym('ol_keys')),
                    items: stateHash('items', u16(sym('itemsCount')) * ITEM_SIZE),
                    nav: stateHash('olNav', 3 * 8240), camera: stateHash('camera', 32),
                    target: stateHash('camTarget', 8), random: stateHash('olRandSeed', 4),
                });
                if (field <= 120 && ['gameUpdate', 'gameRenderSync'].includes(top.name))
                    startupEvents.push({ field, event: top.name, clocks: bus.masterCycles - clocks0,
                        elapsed: bus.masterCycles - top.begin, pending: memory.read(pendingAt),
                        ticks: (u16(sym('ol_ticks')) - ticks0) & 65535 });
                const costs = calls.get(top.name) ?? [];
                costs.push(bus.masterCycles - top.begin); calls.set(top.name, costs); spans.pop();
            }
            return r;
        };

        const ticks0 = u16(sym('ol_ticks'));
        const cycles0 = cpu.cycles;
        const dropped0 = u16(sym('ol_ticksDropped'));
        const clocks0 = bus.masterCycles;
        const timeline: number[][] = [];
        const rendererTimeline: unknown[] = [];
        // Count completed displayed-buffer changes, independently of the
        // FPS gauge's sampling phase. An upload spans multiple fields, so
        // a field-end sample cannot miss two flips within one field.
        let displayedFlips = 0, displayedBuffer = memory.read(0x7E0002);
        let lastDisplayedFlip = 0, bucketFlips = 0, maximumDisplayGap = 0;
        const displayedTimeline: number[][] = [];
        const displayedFlipFields: number[] = [];
        try {
            for (let i = 0; i < fields; i++) {
                field = i + 1;
                pad.buttonState = scene === 'walk' ? PAD_UP : 0;
                if (process.env.OL_PROFILE_INPUT === 'pulses') {
                    pad.buttonState = 0;
                    if (field >= inputStartField) {
                        const phase = (field - inputStartField + 1) % 120;
                        pad.buttonState = phase === 20 ? PAD_LEFT : phase === 23 ? PAD_RIGHT :
                            phase === 42 ? PAD_UP : phase >= 80 && phase < 88 ? PAD_UP | PAD_L :
                            phase === 101 ? PAD_Y : 0;
                    }
                }
                emulator.executeFrames(1);
                const shown = memory.read(0x7E0002);
                if (shown !== displayedBuffer) {
                    displayedFlips++; bucketFlips++; displayedBuffer = shown;
                    maximumDisplayGap = Math.max(maximumDisplayGap, field - lastDisplayedFlip);
                    lastDisplayedFlip = field; displayedFlipFields.push(field);
                }
                fieldSamples.push([field, (u16(sym('ol_ticks')) - ticks0) & 65535,
                    (u16(sym('ol_ticksDropped')) - dropped0) & 65535, memory.read(pendingAt)]);
                if ((i + 1) % 60 === 0) {
                    displayedTimeline.push([field, displayedFlips, bucketFlips]);
                    bucketFlips = 0;
                    timeline.push([i + 1, (u16(sym('ol_ticks')) - ticks0) & 65535,
                        (u16(sym('ol_ticksDropped')) - dropped0) & 65535, u16(0x7E0000)]);
                    // Read-only emulator diagnostics, not CPU register reads
                    // that could acknowledge IRQs or alter a running job.
                    rendererTimeline.push({ field: i + 1,
                        gsu: emulator.getSuperFX().getDiagnostics(),
                        host: Array.from({ length: 27 }, (_, a) => memory.read(0x7E0000 + a)),
                    });
                }
            }
        } finally {
            pad.buttonState = 0;
            cpu.step = orig;
            fx.readCPU = readCPU; fx.writeCPU = writeCPU;
        }
        if (!FX3 && SYMS.has('olGsuYieldRam')) {
            expect(ramViolations,'CPU accessed cartridge RAM while granted to the running GSU').toEqual([]);
            expect(auditedRamAccesses).toBeGreaterThan(1000);
            expect(yieldCalls).toBeGreaterThan(100);
            // A cartridge output skips yield and reclaims with token zero;
            // count actual handoffs, not these deliberately asymmetric calls.
            // Observation can end between a grant and its matching reclaim.
            expect(ramGrants).toBeGreaterThan(100);
            expect(Math.abs(ramGrants-ramReclaims)).toBeLessThanOrEqual(1);
            console.log(`[openlara] GSU overlap: ${yieldCalls}/${reclaimCalls} yield/reclaim calls, ` +
                `${ramGrants}/${ramReclaims} actual grants/reclaims, ` +
                `${auditedRamAccesses} CPU cartridge-RAM accesses, zero ownership violations`);
        }
        // Include partial holds at the beginning and end of observation;
        // a stall must not escape the gap metric at a window boundary.
        maximumDisplayGap = Math.max(maximumDisplayGap, fields - lastDisplayedFlip);
        const displayedFPS = displayedFlips * 60 / fields;
        const ticks = (u16(sym('ol_ticks')) - ticks0) & 0xFFFF;
        const total = cpu.cycles - cycles0;
        const clocks = bus.masterCycles - clocks0;

        // The link's symbols, as a sorted table for nearest-below lookup.
        // The host assembly is linked at bank $01 (its ROM image) and runs
        // at $00:0100 -- so a bank-0 counter below $1000 is looked up as $01.
        const table: [number, string][] = [];
        for (const [name, addr] of SYMS) {
            if (name.startsWith('__local') || name.startsWith('SECTION')) continue;
            if (name.startsWith('tcc__r') || name.startsWith('ol_')) continue;
            const bank = addr >>> 16;
            if (![0x7e, 0x7f, 0x01, 0x00, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0xBC, 0xBD, 0xBE, 0xBF].includes(bank)) continue;
            table.push([addr, name.replace(/^tccs_.*\/c_(\w+)\.asm_/, '$1: ')]);
        }
        table.sort((a, b) => a[0] - b[0]);
        const lookup = (pc: number) => {
            let key = pc;
            if ((pc >>> 16) === 0 && (pc & 0xffff) < 0x1000) key = 0x010000 | (pc & 0xffff);
            let lo = 0, hi = table.length - 1, best = -1;
            while (lo <= hi) {
                const mid = (lo + hi) >> 1;
                if (table[mid][0] <= key) { best = mid; lo = mid + 1; } else { hi = mid - 1; }
            }
            if (best < 0 || (table[best][0] >>> 16) !== (key >>> 16)) return `?${pc.toString(16)}`;
            return table[best][1];
        };

        const perSym = new Map<string, number>();
        const perBank = new Map<string, number>();
        for (const [pc, c] of clockHist) {
            const s = lookup(pc);
            perSym.set(s, (perSym.get(s) ?? 0) + c);
            const bank = pc >>> 16;
            const b = bank === 0x7f ? 'C $7F' : bank === 0x7e ? 'C $7E'
                    : bank >= 0x80 ? `C ROM $${bank.toString(16).toUpperCase()}`
                    : bank === 0 && (pc & 0xffff) < 0x1000 ? 'host asm' : `bank ${bank.toString(16)}`;
            perBank.set(b, (perBank.get(b) ?? 0) + c);
        }
        const cInC = [...perBank].filter(([name]) => name.startsWith('C ')).reduce((n, [, c]) => n + c, 0);
        const pct = (c: number) => (100 * c / clocks).toFixed(1).padStart(5);

        console.log(`[openlara] profile (${profileTag}): ${fields} fields, ${ticks} ticks, ` +
                    `${total} CPU cycles = ${(total / fields).toFixed(0)}/field; ` +
                    `C ${pct(cInC)}% = ${ticks ? (cInC / ticks).toFixed(0) : '?'} master clocks/tick ` +
                    `= ${ticks ? (cInC / ticks / fieldClocks).toFixed(2) : '?'} fields/tick; stack low $${stackLow.toString(16)}`);
        console.log(`[openlara] profile banks: ` +
                    [...perBank.entries()].sort((a, b) => b[1] - a[1])
                        .map(([b, c]) => `${b} ${pct(c)}%`).join('  '));
        const top = [...perSym.entries()].sort((a, b) => b[1] - a[1]).slice(0, Number(process.env.OL_PROFILE_TOP ?? 32));
        const focusSym = new Map<string, number>();
        for (const [pc, c] of focusHist) {
            const name = lookup(pc);
            focusSym.set(name, (focusSym.get(name) ?? 0) + c);
        }
        const focusTop = [...focusSym].sort((a, b) => b[1] - a[1]);
        if (focusRange) console.log(`[openlara] focus fields ${focusRange.join('..')}: ${JSON.stringify(focusTop.slice(0, 24))}`);
        for (const [s, c] of top) {
            console.log(`[openlara] profile ${pct(c)}%  ${c.toString().padStart(9)}  ${s}`);
        }
        const callStats = Object.fromEntries([...calls].map(([name, values]) => [name, {
            count: values.length, mean: Math.round(values.reduce((a, b) => a + b, 0) / values.length),
            max: Math.max(...values), first: values[0],
        }]));
        console.log(`[openlara] profile inclusive master clocks: ${JSON.stringify(callStats)}`);
        if (navBranches) console.log(`[openlara] profile Nav branches: ${JSON.stringify(Object.fromEntries(navBranchCounts))}`);
        console.log(`[openlara] creature display: ${displayedFlips} actual NMI buffer flips over ${fields} fields = ` +
            `${displayedFPS.toFixed(2)} FPS; visible 60-field buckets ${displayedTimeline.map(v => v[2]).join('/')}; ` +
            `maximum display gap ${maximumDisplayGap} fields (including observation edges)`);
        console.log(`[openlara] profile clock (fields/ticks/dropped/fps): ${timeline.map(v => v.join('/')).join(' ')}`);
        if (FX3 && process.env.OL_INV_SNAPS) {
            // the scene's shadows, hers and the creatures' (inventory_text.c)
            const fb = emulator.getPPU().getFramebuffer();
            const { stride, width, height } = emulator.getPPU().getFrameGeometry();
            writeFileSync(join(dirname(ROM), `profile_${profileTag}.png`), png(width, height, (x, y) => fb[y * stride + x]));
            console.log(`[openlara] profile shadows: ${u16(sym('olShadow'))} published`);
        }
        const owners = FX3 ? [0,1,2].map(i => u16(sym('olNav') + i * 8240 + 8214)) : [];
        const health = 30000 - u16(sym('items') + ITEM.health);
        const ppu = emulator.getPPU(), framebuffer = ppu.getFramebuffer();
        const { width, height, stride } = ppu.getFrameGeometry();
        writeFileSync(join(dirname(OUT), `profile_${profileTag}.png`),
            png(width, height, (x, y) => framebuffer[y * stride + x]));
        writeFileSync(join(dirname(OUT), `profile_${profileTag}.json`), JSON.stringify({
            scene, species, fields, ticks, clocks, engineClocks: cInC, stackLow, timeline, callStats, owners,
            damage: health, functions: top, rendererTimeline, callSamples: Object.fromEntries(calls),
            startupEvents, fieldSamples, displayedFlips, displayedFPS, displayedTimeline,
            displayedFlipFields, maximumDisplayGap, navBranches: Object.fromEntries(navBranchCounts),
            ...(focusRange ? { focus: { fields: focusRange, functions: focusTop, events: focusEvents } } : {}),
            ...(traceTicks ? { tickStates, traceStartOffset: (traceStartTick - ticks0) & 65535,
                inputPattern: process.env.OL_PROFILE_INPUT ?? 'none', inputStartField } : {}),
        }, null, 2));
        if (process.env.OL_PROFILE_REFERENCE_TRACE) {
            const reference = JSON.parse(readFileSync(process.env.OL_PROFILE_REFERENCE_TRACE, 'utf8'));
            expect(reference.inputPattern).toBe(process.env.OL_PROFILE_INPUT ?? 'none');
            expect(reference.inputStartField ?? 1).toBe(inputStartField);
            expect(reference.species).toEqual(species);
            expect(reference.fields).toBe(fields);
            const compared = Math.min(tickStates.length, reference.tickStates.length);
            expect(compared).toBeGreaterThanOrEqual(fields / 2 - 5);
            for (let i = 0; i < compared; i++)
                expect(tickStates[i], `observed gameplay state at tick ${i + 1}`).toEqual(reference.tickStates[i]);
            console.log(`[openlara] scheduler parity: ${compared} exact tick snapshots, all items, three Nav contexts, camera, target, PRNG and queued input`);
        }
        expect(stackLow, 'active creatures exhausted the unchanged stack').toBeGreaterThanOrEqual(0x1C00);
        if (scene === 'creatures' && nativeGsu2 && fields >= 600 &&
            ['7,7,7', '7,7,8'].includes(species.join(','))) {
            expect(displayedFPS, 'GSU-2 creatures starved displayed frames').toBeGreaterThanOrEqual(0.6);
            expect(maximumDisplayGap, 'GSU-2 creatures held the displayed frame for over three seconds')
                .toBeLessThanOrEqual(180);
            expect(health, 'the active GSU-2 pursuit never reached an attack').toBeGreaterThan(0);
            for (const [type, name] of [[7, 'Wolf_update'], [8, 'Bear_update']] as const) {
                const count = species.filter(t => t === type).length;
                if (count) expect(callStats[name]?.count, 'living GSU-2 creatures stopped updating')
                    .toBeGreaterThanOrEqual((ticks - 1) * count);
            }
            for (const [field, completed, dropped] of timeline) {
                expect(Math.abs(completed - field / 2), `GSU-2 creatures fell behind at field ${field}`)
                    .toBeLessThanOrEqual(5);
                expect(dropped, 'GSU-2 creatures overflowed the tick queue').toBe(0);
            }
        }
        if (scene === 'creatures' && FX3) {
            if (LEVEL === 'LEVEL1' && SYMS.has('Nav_searchNext') && fields >= 600 &&
                ['7,7,7', '7,7,8'].includes(species.join(','))) {
                expect(displayedFPS, 'three creatures averaged fewer than five displayed frames per second')
                    .toBeGreaterThanOrEqual(5);
                expect(maximumDisplayGap, 'three creatures held the displayed frame for over half a second')
                    .toBeLessThanOrEqual(30);
            }
            expect(owners.slice().sort((a, b) => a - b)).toEqual(creatures.slice().sort((a, b) => a - b));
            expect(callStats.Nav_getWaypoint.count, 'three living creatures did not all navigate').toBeGreaterThanOrEqual((ticks - 1) * 3);
            expect(health, 'the active pursuit never reached an attack').toBeGreaterThan(0);
            for (const [field, , , fps] of timeline) {
                if (field >= 120)
                    expect(fps, `three creatures starved frame presentation at field ${field}`).toBeGreaterThan(0);
            }
            for (const [field, completed, dropped] of timeline) {
                expect(Math.abs(completed - field / 2), `three creatures fell behind at field ${field}`).toBeLessThanOrEqual(5);
                expect(dropped, 'three active creatures overflowed the tick queue').toBe(0);
            }
        }
    }, 600_000);
});

describe('OpenLara SNES/Super FX: water', () => {
    it.skipIf(!FX3 || LEVEL !== 'LEVEL2')('swims onto both underwater switches and completes their real trigger animations', () => {
        for (const switchIndex of [31, 32]) {
            const emulator = new SNESEmulator();
            emulator.loadROM(new Uint8Array(readFileSync(ROM)));
            awaitLevel(emulator); emulator.executeFrames(30);
            const memory = emulator.getMemory(), cpu = emulator.getCPU();
            const r8 = (a: number) => memory.read(a);
            const r16 = (a: number) => r8(a) | r8(a + 1) << 8;
            const w16 = (a: number, v: number) => { memory.write(a, v & 255); memory.write(a + 1, v >>> 8 & 255); };
            const lara = sym('items'), lever = lara + switchIndex * ITEM_SIZE;
            expect(r8(lever + ITEM.type)).toBe(56);
            const initialState = r8(lever + ITEM.state);
            const trigger = r16(lever + ITEM.trigger);
            const table = hostTable(memory, sym, 'Trigger');
            const entry = table + trigger * 16;
            const targets = Array.from({ length: r8(entry + 1) }, (_, i) => r8(entry + 8 + i));
            expect(targets.length).toBeGreaterThan(0);
            let setup = false;
            const step = cpu.step;
            cpu.step = function () {
                if (!setup && (cpu.PB << 16 | cpu.PC) === sym('gameUpdate')) {
                    // Face west, 400 units short of the actual use point.
                    // Action must swim her into place; do not snap the moveTo.
                    memory.write(lara + ITEM.room, r8(lever + ITEM.room));
                    w16(lara + ITEM.x, r16(lever + ITEM.x) - 508);
                    w16(lara + ITEM.y, r16(lever + ITEM.y));
                    w16(lara + ITEM.z, r16(lever + ITEM.z));
                    w16(lara + ITEM.angleY, r16(lever + ITEM.angleY));
                    w16(lara + ITEM.animIndex, 108);
                    memory.write(lara + ITEM.frameIndex, 0);
                    memory.write(lara + ITEM.state, 13); memory.write(lara + ITEM.goalState, 13);
                    memory.write(lara + ITEM.flags2, 0);
                    for (const f of [ITEM.vSpeed, ITEM.hSpeed]) w16(lara + f, 0);
                    w16(lara + ITEM.health, 1000);
                    w16(sym('olLaraWaterState'), 3); w16(sym('olLaraOxygen'), 1800);
                    w16(sym('olLaraWeaponState'), 0); w16(sym('olLaraAngleX'), 0);
                    w16(sym('olLaraTurn'), 0); w16(sym('olLaraMoveAngle'), r16(lever + ITEM.angleY));
                    setup = true;
                }
                return step.call(this);
            };
            const pad = emulator.getController() as unknown as { buttonState: number };
            let activated = false, pulled = false, finished = false, activationX = 0;
            try {
                pad.buttonState = PAD_B;
                for (let field = 0; field < 600 && !finished; field++) {
                    emulator.executeFrames(1);
                    if (!setup) continue;
                    if (!activated && (r8(lever + ITEM.flags2) & 3) === 1) {
                        activated = true; activationX = r16(lara + ITEM.x);
                        pad.buttonState = 0;
                    }
                    if (r8(lara + ITEM.state) === 40) pulled = true; // STATE_SWITCH_DOWN
                    finished = activated && (r8(lever + ITEM.flags2) & 3) === 0 && r8(lara + ITEM.state) === 13;
                }
            } finally { cpu.step = step; pad.buttonState = 0; }
            expect(activated, `water switch ${switchIndex} never entered its active list`).toBe(true);
            expect(activationX, 'moveTo must finish at the 108-unit use offset').toBe((r16(lever + ITEM.x) - 108) & 65535);
            expect(pulled, 'Lara never played the underwater pull').toBe(true);
            expect(finished, `water switch ${switchIndex} never returned to underwater tread`).toBe(true);
            expect(r8(lever + ITEM.state)).not.toBe(initialState);
            for (const index of targets) {
                if (index === switchIndex || index === 255) continue;
                const target = lara + index * ITEM_SIZE;
                expect(r8(target + ITEM.flags) & 31, `switch ${switchIndex} did not fire target ${index}`).toBe(31);
            }
            console.log(`[openlara] water switch ${switchIndex}: swam 400 units onto the lever, pull/tread complete, trigger ${trigger} targets ${targets}`);
        }
    }, 600_000);

    it.skipIf(LEVEL !== 'LEVEL2')('falls in, surfaces, swims to the rim and climbs out', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const w8 = (a: number, v: number) => memory.write(a, v & 0xFF);
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };
        const sItemW = (f: number) => {
            const v = u16(sym('items') + f);
            return v & 0x8000 ? v - 0x10000 : v;
        };

        // LEVEL2 room 43 (dry, x 25600..32768, y -3584..-1280, z 19456..26624)
        // sits over the pool in room 48 (water, x 26624..32768, y -1280..256).
        // Rooms carry a wall sector round their edge, so the pool's water is
        // x 27648..31744 and room 43's column over the pool's west wall, x
        // 26624..27648, is the rim: dry floor one click above the surface,
        // at -1536 (the host sector table says so; see TODO.md).
        //
        // Drop her over the pool facing west and expect the fall to become a
        // dive; hold Down and Jump to pitch her up and swim to the surface;
        // hold Up and Action to swim west until the rim stops her and
        // c_checkWaterOut lifts her onto it; expect her standing there, dry.
        const ROOM = 43, POOL = 48, X = 29696, Y = -1800, Z = 22528;
        const SURFACE_Y = -1280, RIM_Y = -1536, POOL_WEST_WALL_X = 27648;
        const FACING_WEST = 0xC000;
        const STATE_STOP = 2, STATE_SURF_TREAD = 33;
        const WS_ABOVE = 0, WS_SURFACE = 2, WS_UNDER = 3;
        const ANIM_STAND = 103;

        emulator.executeFrames(180);
        expect(u16(sym('itemsCount')), 'the host table never arrived').toBeGreaterThan(0);

        for (let i = 0; i < 6; i++) {
            w8(sym('items') + ITEM.room, ROOM);
            w8(sym('items') + ITEM.state, STATE_STOP);
            w8(sym('items') + ITEM.goalState, STATE_STOP);
            w16(sym('items') + ITEM.x, X);
            w16(sym('items') + ITEM.y, Y);
            w16(sym('items') + ITEM.z, Z);
            w16(sym('items') + ITEM.angleY, FACING_WEST);
            w16(sym('items') + ITEM.animIndex, ANIM_STAND);
            w8(sym('items') + ITEM.frameIndex, 0);
            w8(sym('items') + ITEM.flags2, 0);
            w16(sym('items') + ITEM.vSpeed, 0);
            w16(sym('items') + ITEM.hSpeed, 0);
            w16(sym('items') + ITEM.health, 1000);
            w16(sym('olLaraMoveAngle'), FACING_WEST);
            w16(sym('olLaraTurn'), 0);
            w16(sym('olLaraWaterState'), WS_ABOVE);
            w16(sym('olLaraAngleX'), 0);
            emulator.executeFrames(1);
        }
        expect(u16(sym('ol_room')), 'she is not in room 43').toBe(ROOM);

        const pad = emulator.getController() as unknown as { buttonState: number };
        const state = () => u8(sym('items') + ITEM.state);
        const water = () => u16(sym('olLaraWaterState'));
        const trail: string[] = [];
        let last = '';
        const run = (buttons: number, fields: number, until: () => boolean) => {
            pad.buttonState = buttons;
            let i = 0;
            for (; i < fields; i++) {
                // The pool has neighbours with teeth; this scene is about
                // the water, not about surviving them.
                w16(sym('items') + ITEM.health, 1000);
                emulator.executeFrames(1);
                const now = `${state()}/${water()}`;
                if (now !== last) {
                    trail.push(`${now}@(${sItemW(ITEM.x)},${sItemW(ITEM.y)})`);
                    last = now;
                }
                if (until()) break;
            }
            return i < fields;
        };
        const where = () => `(${sItemW(ITEM.x)},${sItemW(ITEM.y)},${sItemW(ITEM.z)}) ` +
                            `room ${u16(sym('ol_room'))} state ${state()} water ${water()}`;
        // A field boundary can fall inside a tick, between the state change
        // `run` stops on and the collision pass that follows it in the same
        // tick; let go and let the tick finish before reading anything.
        const settle = () => { pad.buttonState = 0; emulator.executeFrames(3); };

        // 1. the fall, and the water closing over her
        const under = run(0, 240, () => water() === WS_UNDER);
        settle();
        const diveY = sItemW(ITEM.y);
        expect(under, `she never went under: ${trail.join(' ')} ${where()}`).toBe(true);
        expect(u16(sym('ol_room')), 'she is not in the pool room').toBe(POOL);
        // Under, the air bar comes first: palette 1, at the top.
        expect(u8(sym('olHudOam') + 1), 'no air bar under water').toBe(FX3 ? 44 : 4);
        expect(u8(sym('olHudOam') + 3) & 0x0E, 'the top bar is not the air bar').toBe(2);

        // 2. nose up and swim: Down pitches her up, Jump is the stroke. Each
        // frame handed to the GSU carries her pitch's top byte in her
        // record's byte 11 (ItemsExchange, main.asm, after the block move
        // that also writes the byte), which the renderer turns X by. Where
        // the emulator routes cartridge-RAM writes through memory.write
        // (FX3), the last write of each hand-over must be her pitch then;
        // on both, the record polled each field must show her pitched.
        const mem = memory as unknown as { write: (a: number, v: number) => void };
        const originalWrite = mem.write;
        const handed: { got: number, want: number }[] = [];
        mem.write = function (a: number, v: number) {
            if (a === 0x70F800) handed.push({ got: -1, want: -1 });      // a hand-over begins
            if (a === 0x70F800 + 11 && handed.length)
                handed[handed.length - 1] = { got: v & 255, want: (u16(sym('olLaraAngleX')) >> 8) & 255 };
            return originalWrite.call(this, a, v);
        };
        const polled = new Set<number>();
        let surfaced = false;
        try {
            surfaced = run(PAD_DOWN | PAD_A, 900, () => {
                polled.add(u8(0x70F800 + 11));
                return water() === WS_SURFACE;
            });
        } finally { mem.write = originalWrite; }
        const stale = handed.find(h => h.got !== h.want);
        console.log(`[openlara] water: ${handed.length} hand-overs seen while she swam, record pitches ${[...polled].join(',')}`);
        expect(stale, `the renderer was handed a stale pitch: ${JSON.stringify(stale)}`).toBeUndefined();
        // GSU-2 renders this pool at 0-1 fps: no frame is handed over during
        // the swim at all, so only FX3 can show her drawn pitched here.
        if (FX3) expect([...polled].filter(p => p !== 0).length, 'she was never drawn pitched under water').toBeGreaterThan(3);
        settle();
        expect(surfaced, `she never surfaced: ${trail.join(' ')} ${where()}`).toBe(true);
        expect(sItemW(ITEM.y), 'she is not floating at the surface').toBe(SURFACE_Y + 1);
        run(0, 90, () => state() === STATE_SURF_TREAD);
        settle();
        expect(state(), `she is not treading water: ${where()}`).toBe(STATE_SURF_TREAD);
        expect(u16(sym('olLaraOxygen')), 'her air is not coming back').toBeGreaterThan(0);

        // 3. west to the rim with Action held, and out
        const out = run(PAD_UP | PAD_B, 1200, () => water() === WS_ABOVE);
        settle();
        expect(out, `she never climbed out: ${trail.join(' ')} ${where()}`).toBe(true);
        expect(u16(sym('olLaraWeaponState')), 'climbing out of water did not occupy her hands').toBe(1);
        expect(sItemW(ITEM.x), 'she left the water somewhere other than the rim')
            .toBeLessThan(POOL_WEST_WALL_X + 200);

        // 4. the climb-out animation plays through to a stand on the rim
        run(0, 240, () => state() === STATE_STOP && sItemW(ITEM.hSpeed) === 0);
        settle();

        console.log(`[openlara] water: ${trail.join(' ')}; under at y ${diveY}; ` +
                    `end ${where()}`);
        expect(state(), `she did not come to a stand: ${where()}`).toBe(STATE_STOP);
        expect(sItemW(ITEM.y), 'she is not standing on the rim').toBe(RIM_Y);
        expect(sItemW(ITEM.x), 'she is not on the rim').toBeLessThan(POOL_WEST_WALL_X);
        expect(u16(sym('ol_room')), 'she is not back in the dry room').toBe(ROOM);
        expect(water(), 'she is not out of the water').toBe(WS_ABOVE);
        expect(u16(sym('olLaraWeaponState')), 'the water-out animation did not free her hands').toBe(0);
    }, 600_000);

    it.skipIf(LEVEL !== 'LEVEL2')('picks up the key, turns it in the keyhole, and the door opens', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const w8 = (a: number, v: number) => memory.write(a, v & 0xFF);
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };
        const itemB = (i: number, f: number) => u8(sym('items') + i * ITEM_SIZE + f);
        const sItemW = (i: number, f: number) => {
            const v = u16(sym('items') + i * ITEM_SIZE + f);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const findItem = (type: number, room: number, x: number, z: number) => {
            const n = u16(sym('itemsCount'));
            for (let i = 1; i < n; i++) {
                if (itemB(i, ITEM.type) !== type) continue;
                if (itemB(i, ITEM.room) !== room) continue;
                if (sItemW(i, ITEM.x) !== x) continue;
                if (sItemW(i, ITEM.z) !== z) continue;
                return i;
            }
            throw new Error(`no item of type ${type} at (${x}, ${z}) in room ${room}`);
        };
        const stand = (room: number, x: number, y: number, z: number, angle: number) => {
            for (let i = 0; i < 6; i++) {
                w8(sym('items') + ITEM.room, room);
                w8(sym('items') + ITEM.state, 2);
                w8(sym('items') + ITEM.goalState, 2);
                w16(sym('items') + ITEM.x, x);
                w16(sym('items') + ITEM.y, y);
                w16(sym('items') + ITEM.z, z);
                w16(sym('items') + ITEM.angleY, angle);
                w16(sym('items') + ITEM.animIndex, 103);
                w8(sym('items') + ITEM.frameIndex, 0);
                w8(sym('items') + ITEM.flags2, 0);
                w16(sym('items') + ITEM.vSpeed, 0);
                w16(sym('items') + ITEM.hSpeed, 0);
                w16(sym('items') + ITEM.health, 1000);
                w16(sym('olLaraMoveAngle'), angle);
                w16(sym('olLaraTurn'), 0);
                w16(sym('olLaraWaterState'), 0);
                w16(sym('olLaraAngleX'), 0);
                emulator.executeFrames(1);
            }
            emulator.executeFrames(30);
        };
        const pad = emulator.getController() as unknown as { buttonState: number };
        const state = () => u8(sym('items') + ITEM.state);
        const trail: string[] = [];
        let last = -1;
        const run = (buttons: number, fields: number, until: () => boolean) => {
            pad.buttonState = buttons;
            let i = 0;
            for (; i < fields; i++) {
                w16(sym('items') + ITEM.health, 1000);
                emulator.executeFrames(1);
                if (state() !== last) { trail.push(`${state()}`); last = state(); }
                if (until()) break;
            }
            pad.buttonState = 0;
            return i < fields;
        };

        emulator.executeFrames(180);
        expect(u16(sym('itemsCount')), 'the host table never arrived').toBeGreaterThan(0);

        // LEVEL2's silver key (KEY_ITEM_1) lies in room 14; the keyhole
        // (KEY_HOLE_1) is in room 5 facing -X, its sector carrying a
        // KEY-type trigger that opens the door beside it (packer dump:
        // trigger 23, items 10 9). Upstream's numbers: she must be within
        // 256 behind a pickup, and 312 to 512 in front of a hole facing it.
        // Positions past 32767 wrap in the host's sixteen bits.
        const KEY = findItem(129, 14, 64000 - 65536, 22016);
        const HOLE = findItem(137, 5, 55808 - 65536, 29184);
        const DOOR = findItem(58, 5, 55808 - 65536, 28160);
        const STATE_PICKUP = 39, STATE_USE_KEY = 42;

        // 1. the key
        stand(14, 64000 - 65536, 512, 22016 - 200, 0);
        const bent = run(PAD_B, 120, () => state() === STATE_PICKUP);
        expect(bent, `Action at the key did not bend her: ${trail.join(' ')}`).toBe(true);
        run(0, 600, () => u16(sym('olPickups')) > 0);
        expect(itemB(KEY, ITEM.flags2) & ITEM_STATUS, 'the key was not taken').toBe(3);
        // FX3 keeps upstream's per-slot counts (src/inventory.c), the key
        // at SLOT_KEY_ITEM_1; GSU-2 its own eleven bytes, the key first.
        const keys = () => FX3 ? u16(sym('olInvCounts') + 2 * 1) : u8(sym('olInventory') + 0);
        expect(keys(), 'the key is not in her inventory').toBe(1);
        // The bend is taken twenty-three frames before it ends, and its
        // EMPTY command -- her hands back -- is at the end: let it finish.
        run(0, 300, () => state() === 2);
        expect(u16(sym('olLaraWeaponState')), 'the bend should have freed her hands').toBe(0);

        // 2. the keyhole
        expect(itemB(DOOR, ITEM.goalState), 'the door should start shut').toBe(0);
        // Facing -X, the hole's wall is at its -X side and so is she.
        stand(5, 55808 - 65536 - 380, 0, 29184, 0xC000);
        // FX3 has the ring: Action at the hole opens it on the USE page with
        // the hole's key chosen (upstream Hole::apply, Inventory::open), and
        // Action held there shows it, closes the ring and hands it back as
        // useSlot, which the hole spends the tick after.
        if (FX3) {
            const opened = run(PAD_B, 60, () => u16(sym('olInvState')) !== 0);
            expect(opened, 'Action at the keyhole did not open the ring').toBe(true);
            expect(u16(sym('olInvPage')), 'not the USE page').toBe(3);
            const slot = u8(sym('olInvItemsList') + u16(sym('olInvItemIndex')));
            expect(slot, 'the hole\'s key is not the one chosen').toBe(1);
            if (process.env.OL_INV_SNAPS) {
                run(0, 40, () => false);
                const fb = emulator.getPPU().getFramebuffer();
                const { stride, width, height } = emulator.getPPU().getFrameGeometry();
                writeFileSync(join(dirname(ROM), 'inv_use.png'), png(width, height, (x, y) => fb[y * stride + x]));
            }
        }
        const used = run(PAD_B, FX3 ? 400 : 120, () => state() === STATE_USE_KEY);
        expect(used, `Action at the keyhole did not use the key: ${trail.join(' ')}, ` +
                     `hands ${u16(sym('olLaraWeaponState'))}, she is at ` +
                     `(${sItemW(0, ITEM.x)},${sItemW(0, ITEM.y)},${sItemW(0, ITEM.z)}) anim ` +
                     `${u16(sym('items') + ITEM.animIndex)}`).toBe(true);
        expect(keys(), 'the key was not spent').toBe(0);
        const opened = run(0, 900, () => itemB(DOOR, ITEM.goalState) === 1);

        console.log(`[openlara] key: states ${trail.join(' ')}; hole status ` +
                    `${itemB(HOLE, ITEM.flags2) & ITEM_STATUS}, door flags ` +
                    `$${itemB(DOOR, ITEM.flags).toString(16)} goal ${itemB(DOOR, ITEM.goalState)}, ` +
                    `hands ${u16(sym('olLaraWeaponState'))}`);
        expect(opened, 'the door did not open for the key').toBe(true);
        expect(u16(sym('olLaraWeaponState')), 'her hands are still busy').toBe(0);
    }, 600_000);

    it.skipIf(LEVEL !== 'LEVEL2')('takes hold of a block and pushes it a sector', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const u16 = (a: number) => u8(a) | (u8(a + 1) << 8);
        const w8 = (a: number, v: number) => memory.write(a, v & 0xFF);
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };
        const itemB = (i: number, f: number) => u8(sym('items') + i * ITEM_SIZE + f);
        const sItemW = (i: number, f: number) => {
            const v = u16(sym('items') + i * ITEM_SIZE + f);
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const findItem = (type: number, room: number, x: number, z: number) => {
            const n = u16(sym('itemsCount'));
            for (let i = 1; i < n; i++) {
                if (itemB(i, ITEM.type) !== type) continue;
                if (itemB(i, ITEM.room) !== room) continue;
                if (sItemW(i, ITEM.x) !== x) continue;
                if (sItemW(i, ITEM.z) !== z) continue;
                return i;
            }
            throw new Error(`no item of type ${type} at (${x}, ${z}) in room ${room}`);
        };
        const pad = emulator.getController() as unknown as { buttonState: number };
        const state = () => u8(sym('items') + ITEM.state);
        const trail: string[] = [];
        let last = -1;
        const run = (buttons: number, fields: number, until: () => boolean) => {
            pad.buttonState = buttons;
            let i = 0;
            for (; i < fields; i++) {
                w16(sym('items') + ITEM.health, 1000);
                emulator.executeFrames(1);
                if (state() !== last) { trail.push(`${state()}`); last = state(); }
                if (until()) break;
            }
            pad.buttonState = 0;
            return i < fields;
        };

        emulator.executeFrames(180);
        expect(u16(sym('itemsCount')), 'the host table never arrived').toBeGreaterThan(0);

        // LEVEL2's one pushable block stands in room 0, a corridor one
        // sector wide along z (OL_DUMP_SECTORS=0: row x1, floors at 1024,
        // ceilings at 0 with the ends taller). The block is at sector z 4
        // with z 5 clear beyond it. Upstream's LIMIT_BLOCK puts her 512 to
        // 692 short of its centre, facing it square, at its own height.
        const BLOCK = findItem(48, 0, 60928 - 65536, 29184);
        const idleAnim = u16(sym('items') + BLOCK * ITEM_SIZE + ITEM.animIndex);
        const STATE_BLOCK_PUSH = 36, STATE_BLOCK_READY = 38;
        const BLOCK_X = 60928 - 65536, BLOCK_Z = 29184, BLOCK_Y = 1024;

        // The host sector table is in game pak RAM, which the runner can
        // read but not trust while the GSU owns the bus. What a block does
        // to the floor is asked the way she asks it: stand somewhere and
        // read the floor the probe reports for her feet.
        const settle = () => { pad.buttonState = 0; emulator.executeFrames(3); };
        const stand = (x: number, y: number, z: number, angle: number) => {
            for (let i = 0; i < 6; i++) {
                w8(sym('items') + ITEM.room, 0);
                w8(sym('items') + ITEM.state, 2);
                w8(sym('items') + ITEM.goalState, 2);
                w16(sym('items') + ITEM.x, x);
                w16(sym('items') + ITEM.y, y);
                w16(sym('items') + ITEM.z, z);
                w16(sym('items') + ITEM.angleY, angle);
                w16(sym('items') + ITEM.animIndex, 103);
                w8(sym('items') + ITEM.frameIndex, 0);
                w8(sym('items') + ITEM.flags2, 0);
                w16(sym('items') + ITEM.vSpeed, 0);
                w16(sym('items') + ITEM.hSpeed, 0);
                w16(sym('items') + ITEM.health, 1000);
                w16(sym('olLaraMoveAngle'), angle);
                w16(sym('olLaraTurn'), 0);
                w16(sym('olLaraWaterState'), 0);
                w16(sym('olLaraAngleX'), 0);
                emulator.executeFrames(1);
            }
            settle();
        };
        const floorAt = (x: number, y: number, z: number) => {
            stand(x, y, z, 0);
            const v = u16(sym('ol_floor'));
            return v & 0x8000 ? v - 0x10000 : v;
        };
        const NO_FLOOR_HERE = 32767;

        // Its sector is rewritten from the start: Block's constructor
        // raises the floor to its top, and here that meets the ceiling --
        // the corridor is exactly a block tall -- so upstream's updateFloor
        // makes it no floor at all, a wall.
        const under = floorAt(BLOCK_X, BLOCK_Y, BLOCK_Z);
        const beyond = floorAt(BLOCK_X, BLOCK_Y, BLOCK_Z + 1024);
        console.log(`[openlara] block: floor under it ${under}, beyond it ${beyond}`);
        expect(under, 'the block should have made its sector a wall').toBe(NO_FLOOR_HERE);
        expect(beyond, 'the sector beyond it should be floor').toBe(BLOCK_Y);

        stand(BLOCK_X, BLOCK_Y, BLOCK_Z - 600, 0);
        expect(sItemW(0, ITEM.y), 'she is not standing at the block\'s height').toBe(BLOCK_Y);

        // 1. Action: she takes hold
        const held = run(PAD_B, 120, () => state() === STATE_BLOCK_READY);
        expect(held, `Action at the block did not take hold: ${trail.join(' ')}`).toBe(true);
        expect(sItemW(0, ITEM.z), 'alignWall should have put her radius off the block')
            .toBe(BLOCK_Z - 512 - 100);

        // 2. Up with Action held: the push, hers and the block's together
        const pushed = run(PAD_B | PAD_UP, 240, () => state() === STATE_BLOCK_PUSH);
        expect(pushed, `Up did not start the push: ${trail.join(' ')}`).toBe(true);
        expect(itemB(BLOCK, ITEM.flags) & 0x40, 'the block should be active while it moves').toBe(0x40);

        // 3. the block travels a sector -- its push animation is 155
        //    frames with the OFFSET and the KILL at its end -- and comes to
        //    rest as a floor again
        const moved = run(PAD_B | PAD_UP, 900, () =>
            sItemW(BLOCK, ITEM.z) === BLOCK_Z + 1024 && !(itemB(BLOCK, ITEM.flags) & 0x40));
        console.log(`[openlara] block: states ${trail.join(' ')}; block at ` +
                    `(${sItemW(BLOCK, ITEM.x)},${sItemW(BLOCK, ITEM.y)},${sItemW(BLOCK, ITEM.z)}) ` +
                    `flags $${itemB(BLOCK, ITEM.flags).toString(16)} status ` +
                    `${itemB(BLOCK, ITEM.flags2) & ITEM_STATUS}; she is at ` +
                    `(${sItemW(0, ITEM.x)},${sItemW(0, ITEM.y)},${sItemW(0, ITEM.z)}) state ${state()}`);
        expect(moved, 'the block did not arrive a sector on and stop').toBe(true);
        // Her push carries her the sector too; let it finish.
        run(0, 300, () => state() === 2);
        const followedTo = sItemW(0, ITEM.z);
        const left = floorAt(BLOCK_X, BLOCK_Y, BLOCK_Z);
        const top = floorAt(BLOCK_X, BLOCK_Y - 1024, BLOCK_Z + 1024);
        console.log(`[openlara] block: she followed to z ${followedTo}; floor where it ` +
                    `stood ${left}, on top of it now ${top}`);
        expect(left, 'the sector it left should have its floor back').toBe(BLOCK_Y);
        expect(top, 'the sector it reached should be a floor at its top').toBe(BLOCK_Y - 1024);
        // Her push animation carries her by its own speed, so where it
        // leaves her is a sector on to within the rounding of a 16.16
        // speed against the tick the button was released on.
        expect(Math.abs(followedTo - (BLOCK_Z - 512 - 100 + 1024)),
               'she should have followed it a sector').toBeLessThanOrEqual(8);

        if (FX3) {
            // A block is a floor, so its place is the one piece of a save
            // outside the entity records: upstream's Block::load gives back
            // the sector the boot raised and raises the one it was saved on.
            // Saved where it was pushed to, loaded through the boot: the
            // sector it left is floor again, the one it reached its top.
            const ring = fx3Passport(emulator);
            const n = u16(sym('itemsCount'));
            const saved = ring.save();
            const loaded = ring.load();
            const diff = saveDiff(n, loaded, saved);
            expect(diff, `the load is not the save: ${diff}`).toBeUndefined();
            expect([sItemW(BLOCK, ITEM.x), sItemW(BLOCK, ITEM.z)], 'the block is not where it was saved')
                .toEqual([BLOCK_X, BLOCK_Z + 1024]);
            const leftAgain = floorAt(BLOCK_X, BLOCK_Y, BLOCK_Z);
            const topAgain = floorAt(BLOCK_X, BLOCK_Y - 1024, BLOCK_Z + 1024);
            console.log(`[openlara] block: saved and loaded, floor where it stood ${leftAgain}, on top of it ${topAgain}`);
            expect(leftAgain, 'loading left the boot\'s block in the sector it was pushed from').toBe(BLOCK_Y);
            expect(topAgain, 'loading did not raise the sector it was pushed to').toBe(BLOCK_Y - 1024);
        }

        // An active block above a lower floor must fall, then stop exactly
        // on it. Reuse the now-empty original sector and put the block a
        // click above it, as after losing its support. The normal push above
        // never changes height and cannot catch a reversed floor comparison.
        const blockAddr = sym('items') + BLOCK * ITEM_SIZE;
        w16(blockAddr + ITEM.z, BLOCK_Z);
        w16(blockAddr + ITEM.y, BLOCK_Y - 256);
        w16(blockAddr + ITEM.animIndex, idleAnim);
        w8(blockAddr + ITEM.frameIndex, 0);
        w8(blockAddr + ITEM.state, 0);
        w8(blockAddr + ITEM.goalState, 0);
        w16(blockAddr + ITEM.vSpeed, 0);
        w16(blockAddr + ITEM.hSpeed, 0);
        w8(blockAddr + ITEM.flags2, 1); // ACTIVE status, initially no gravity
        w8(blockAddr + ITEM.nextActive, u16(sym('olFirstActive')));
        w8(blockAddr + ITEM.flags, itemB(BLOCK, ITEM.flags) | 0x40);
        w16(sym('olFirstActive'), BLOCK);
        let sawFalling = false;
        const landed = run(0, 180, () => {
            if ((itemB(BLOCK, ITEM.flags2) & 4) && sItemW(BLOCK, ITEM.y) < BLOCK_Y)
                sawFalling = true;
            return !(itemB(BLOCK, ITEM.flags) & 0x40);
        });
        expect(sawFalling, 'an unsupported block never started falling').toBe(true);
        expect(landed, 'the falling block never came to rest').toBe(true);
        expect(sItemW(BLOCK, ITEM.y), 'the block did not stop on the floor').toBe(BLOCK_Y);
        expect(sItemW(BLOCK, ITEM.vSpeed), 'the landed block kept its fall speed').toBe(0);
        // Here its top meets the ceiling, so restoring the original block
        // makes this sector a wall again, exactly as it was before the push.
        expect(floorAt(BLOCK_X, BLOCK_Y, BLOCK_Z),
               'the landed block did not restore its occupied sector').toBe(under);
    }, 600_000);
});

// ---------------------------------------------------------------------------
// Sound: the SPC700 driver and the level's samples, uploaded at boot through
// the IPL protocol; then her footsteps, keyed on the S-DSP by the animation's
// SOUND commands. TypeSNES runs the sound CPU and the DSP for real, so the
// driver's voice bookkeeping is exercised by the actual hardware protocol.
// ---------------------------------------------------------------------------

const SPC = process.env.OL_SPC ?? '';
const SOUND_BIN = process.env.OL_SOUND_BIN ?? '';
const SNDTAB = process.env.OL_SNDTAB ?? '';
const SOUND_JSON = process.env.OL_SOUND_JSON ?? '';

function apuOf(emulator: SNESEmulator) {
    const apu = emulator.getAPU() as unknown as {
        spc700: { getRAM(): Uint8Array };
        getDSP(): { getDiagnostics(): {
            flags: number; mainVolume: readonly [number, number]; directory: number;
            voices: { voice: number; keyedOn: boolean; pitch: number; source: number;
                      volume: readonly [number, number] }[];
        } };
    };
    return { ram: apu.spc700.getRAM(), dsp: () => apu.getDSP().getDiagnostics() };
}

describe('OpenLara SNES/Super FX: sound', () => {
    // The HUD (src/game.c olHudUpdate, packer HUD.BIN): upstream's
    // drawHUD as thirteen 8x8 sprites a bar. Judged on the PPU's own
    // picture, against the colours CG-RAM holds for the bar's palette.
    it.skipIf(MODE7)('shows the health bar at the start, hides it, and shows it again on a hit', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);
        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u8 = (a: number) => memory.read(a);
        const w16 = (a: number, v: number) => {
            memory.write(a, v & 0xFF);
            memory.write(a + 1, (v >> 8) & 0xFF);
        };
        const HUD_X = FX3 ? 128 : 148, HUD_Y = FX3 ? 44 : 4;
        // The first displayed line is BG row 1 and OAM line 1 alike, so a
        // sprite's Y is the frame row it covers.
        const X0 = HUD_X, Y0 = HUD_Y;
        const rgb = (i: number) => {
            const c = memory.readCGRAM(i * 2) | (memory.readCGRAM(i * 2 + 1) << 8);
            const e = (v: number) => (v << 3) | (v >> 2);
            return e(c & 31) | (e((c >> 5) & 31) << 8) | (e((c >> 10) & 31) << 16);
        };
        const pixel = (x: number, y: number) => {
            const fb = emulator.getPPU().getFramebuffer();
            const { stride } = emulator.getPPU().getFrameGeometry();
            return (fb[y * stride + x] >>> 0) & 0xFFFFFF;
        };
        const hex = (v: number) => v.toString(16).padStart(6, '0');
        // Where the bar's pixels are not what a bar `w` wide would draw.
        const mismatches = (w: number) => {
            const bad: string[] = [];
            const want = (x: number, y: number, c: number) => {
                const got = pixel(X0 + x, Y0 + y), exp = rgb(128 + c);
                if (got !== exp) bad.push(`(${x},${y}) ${hex(got)}!=${hex(exp)}`);
            };
            for (let x = 1; x <= 100; x += 11) { want(x, 0, 7); want(x, 6, 8); }
            for (let y = 1; y <= 5; y++) {
                want(0, y, 7);
                want(101, y, 8);
                for (const x of [0, w - 1, w, 99]) {
                    if (x < 0 || x > 99) continue;
                    want(1 + x, y, x < w ? y : 6);
                }
            }
            return bad;
        };
        const hidden = () => {
            let n = 0;
            for (let i = 0; i < 26; i++) if (u8(sym('olHudOam') + i * 4 + 1) === 0xF0) n++;
            return n;
        };

        awaitLevel(emulator);
        emulator.executeFrames(20);
        // A distinct palette: the five rows, the empty fill and both borders.
        const colours = new Set([1, 2, 3, 5, 6, 7, 8].map(c => rgb(128 + c)));
        expect(colours.size, 'the health bar palette is not in CG-RAM').toBeGreaterThanOrEqual(5);
        let bad = mismatches(100);
        expect(bad, `the full health bar is not on screen at the start: ${bad.slice(0, 6).join(' ')}`)
            .toEqual([]);

        // healthTimer starts at 100 ticks: gone by 240 fields, all 26 parked.
        emulator.executeFrames(240);
        expect(hidden(), 'the health bar outlived its hundred ticks').toBe(26);
        bad = mismatches(100);
        expect(bad.length, 'the health bar is still being drawn').toBeGreaterThan(20);

        // A 250-point hit: (750 << 8) / 1000 = 192, 192 * 100 >> 8 = 75.
        w16(sym('items') + ITEM.health, 750);
        emulator.executeFrames(8);
        bad = mismatches(75);
        expect(bad, `the bar did not come back 75 wide: ${bad.slice(0, 6).join(' ')}`).toEqual([]);
        emulator.executeFrames(100);
        expect(hidden(), 'the hit bar outlived its forty ticks').toBe(26);
        console.log(`[openlara] hud: health bar ${X0},${Y0} full at the start, ` +
                    `hidden after 100 ticks, 75/100 for 40 ticks after a 250-point hit`);
    }, 600_000);

    it.skipIf(!SPC)('uploads the driver and the level\'s samples to the sound CPU at boot', () => {
        expect(existsSync(ROM), `ROM not found: ${ROM}`).toBe(true);
        for (const f of [SPC, SOUND_BIN, SNDTAB]) expect(existsSync(f), `not found: ${f}`).toBe(true);
        const driver = readFileSync(SPC);
        const image = readFileSync(SOUND_BIN);
        const table = readFileSync(SNDTAB);

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        // Past the boot -- the upload, then the level's staging -- and into
        // the first second of play.
        let booted = 0;
        for (; booted < 600 && u16(sym('ol_ticks')) === 0; booted++) emulator.executeFrames(1);
        expect(booted, 'the game never reached its first tick').toBeLessThan(600);
        emulator.executeFrames(60);

        expect(u16(sym('ol_sndReady')), 'the driver never answered the boot handshake').toBe(1);

        // The host's tables, copied into WRAM before the upload.
        const at = sym('olSndTable');
        const wram = Buffer.alloc(table.length);
        for (let i = 0; i < table.length; i++) wram[i] = memory.read(at + i);
        expect(wram.equals(table), 'SNDTAB.BIN did not reach olSndTable').toBe(true);

        // The driver at $0200 and the image at $0400, byte for byte.
        const { ram, dsp } = apuOf(emulator);
        expect(Buffer.from(ram.subarray(0x0200, 0x0200 + driver.length)).equals(driver),
               'the driver in sound CPU RAM differs from spc.bin').toBe(true);
        expect(Buffer.from(ram.subarray(0x0400, 0x0400 + image.length)).equals(image),
               'the sample image in sound CPU RAM differs from SOUND.BIN').toBe(true);
        expect(image.length, 'the image runs into the echo bytes').toBeLessThanOrEqual(0xFF00 - 0x0400);

        // The DSP as the driver leaves it: main volume up, echo and mute off,
        // the directory at $0400. A level may already have played something
        // -- an object's animation has SOUND commands too -- so the port is
        // judged by the handshake: it echoes the last command, or zero.
        const d = dsp();
        expect(d.flags & 0xE0, 'FLG: reset/mute/echo').toBe(0x20);
        expect(d.mainVolume).toEqual([127, 127]);
        expect(d.directory).toBe(0x0400);
        const sent = u16(sym('ol_sndCount'));
        expect(memory.read(0x002140), 'port 0 should echo the last command')
            .toBe(sent ? u16(sym('olSndSeq')) & 0xFF : 0);
        if (!sent) expect(d.voices.some(v => v.keyedOn), 'a voice is playing before any command').toBe(false);
        console.log(`[openlara] sound: boot at field ${booted}, ${sent} command(s) in the first second` +
                    (sent ? `, last id ${u16(sym('ol_sndLast')) >> 8} source ${u16(sym('ol_sndLast')) & 255}` : ''));
    }, 600_000);

    it.skipIf(!SPC || LEVEL !== 'LEVEL1')('plays her footsteps as she runs, at the sample\'s own pitch', () => {
        expect(existsSync(SOUND_JSON), `not found: ${SOUND_JSON}`).toBe(true);
        const plan = JSON.parse(readFileSync(SOUND_JSON, 'utf8')) as {
            ids: Record<string, { first: number; count: number; pitch: number; randomPitch: number }>;
            sources: { source: number }[];
        };
        const feet = plan.ids['0'];
        expect(feet, 'LEVEL1 packs no footsteps').toBeDefined();

        const emulator = new SNESEmulator();
        emulator.loadROM(new Uint8Array(readFileSync(ROM)));
        const memory = emulator.getMemory();
        const u16 = (a: number) => memory.read(a) | (memory.read(a + 1) << 8);
        for (let i = 0; i < 600 && u16(sym('ol_ticks')) === 0; i++) emulator.executeFrames(1);
        emulator.executeFrames(120);
        expect(u16(sym('ol_sndReady'))).toBe(1);
        const { dsp } = apuOf(emulator);

        // Run forward for five seconds, watching every voice every field.
        const pad = emulator.getController() as unknown as { buttonState: number };
        pad.buttonState = PAD_UP;
        const sentBefore = u16(sym('ol_sndCount'));
        const heard = new Map<number, Set<number>>();   // source -> pitches
        const ids = new Set<number>();
        let maxVolume = 0;
        for (let i = 0; i < 300; i++) {
            emulator.executeFrames(1);
            ids.add(u16(sym('ol_sndLast')) >> 8);
            for (const v of dsp().voices) {
                if (!v.keyedOn) continue;
                if (!heard.has(v.source)) heard.set(v.source, new Set());
                heard.get(v.source)!.add(v.pitch);
                maxVolume = Math.max(maxVolume, v.volume[0], v.volume[1]);
            }
        }
        pad.buttonState = 0;

        expect(u16(sym('ol_sndCount')), 'no command reached the driver while she ran')
            .toBeGreaterThan(sentBefore);
        expect(ids.has(0), 'the run never fired LARA_FEET').toBe(true);
        const feetSources = new Set<number>();
        for (let k = 0; k < feet.count; k++) feetSources.add(feet.first + k);
        const played = [...heard.keys()];
        expect(played.some(s => feetSources.has(s)),
               `no footstep source among ${played.join(',')}`).toBe(true);
        // Every source keyed on is one the packer placed, and every footstep
        // plays within upstream's random pitch band of the sample's rate.
        for (const [source, pitches] of heard) {
            expect(source, 'a voice plays a source outside the plan').toBeLessThan(plan.sources.length);
            if (!feetSources.has(source)) continue;
            for (const p of pitches) {
                const lo = Math.floor(feet.pitch * (feet.randomPitch ? 115 : 128) / 128) - 1;
                const hi = Math.ceil(feet.pitch * (feet.randomPitch ? 141 : 128) / 128) + 1;
                expect(p, `footstep pitch ${p} outside ${lo}..${hi}`).toBeGreaterThanOrEqual(lo);
                expect(p, `footstep pitch ${p} outside ${lo}..${hi}`).toBeLessThanOrEqual(hi);
            }
        }
        expect(maxVolume, 'the footsteps played silently').toBeGreaterThan(0);
        // Upstream varies a footstep's pitch by up to a tenth and picks a
        // variant at random: a dozen steps all alike means the flags never
        // reached the host (they did not, once: a stray byte swap).
        const sent = u16(sym('ol_sndCount')) - sentBefore;
        const feetHeard = played.filter(s => feetSources.has(s));
        const feetPitches = new Set<number>();
        for (const s of feetHeard) for (const p of heard.get(s)!) feetPitches.add(p);
        if (sent >= 10) {
            if (feet.randomPitch) expect(feetPitches.size, 'the footsteps never varied in pitch').toBeGreaterThan(1);
            if (feet.count > 1) expect(feetHeard.length, 'only one footstep variant was ever heard').toBeGreaterThan(1);
        }
        console.log(`[openlara] sound: ${sent} commands, ` +
                    `sources ${played.join(',')}, ids ${[...ids].join(',')}, ` +
                    `footstep pitches ${[...feetPitches].join(',')}`);
    }, 600_000);
});

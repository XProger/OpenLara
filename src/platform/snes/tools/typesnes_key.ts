/** Run keycheck.py's synthetic spans in the independent, buffered GSU core. */
import { expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { crc32 } from 'zlib';
import { SNESEmulator } from './emulator';

const fixtures = JSON.parse(readFileSync(process.env.OL_KEY_CASES!, 'utf8'));
const original = new Uint8Array(readFileSync(process.env.OL_ROM!));
for (const [offset, hex] of fixtures.patches) original.set(Buffer.from(hex, 'hex'), offset);

for (const c of fixtures.cases) {
    it(c.name, () => {
        const rom = new Uint8Array(original);
        for (const [offset, hex] of c.patches) rom.set(Buffer.from(hex, 'hex'), offset);
        rom.set(Buffer.from(c.wrapper, 'hex'), 0x17F00);
        const emulator = new SNESEmulator();
        emulator.loadROM(rom);
        const memory = emulator.getMemory();
        const io = (offset: number, value: number) => emulator.writeIO(fixtures.hostBase + offset, value);
        for (let i = 0; i < 0x20000; i++) memory.writeCartridgeRAM(i, 0xA6);
        for (const [a, value] of c.words) {
            memory.writeCartridgeRAM(a, value & 255);
            memory.writeCartridgeRAM(a + 1, value >>> 8);
        }
        for (const [reg, value] of Object.entries(c.regs)) {
            const a = Number(reg) * 2;
            io(a, Number(value) & 255);
            io(a + 1, Number(value) >>> 8);
        }
        io(0x34, 0x41); // PBR
        io(0x38, c.scbr);
        io(0x3A, c.scmr);
        io(0x39, 1); // fast GSU clock
        io(0x1E, 0);
        io(0x1F, 0x7F); // start the fixture's setup trampoline
        emulator.getSuperFX().stepMaster(1000000);
        const d = emulator.getSuperFX().getDiagnostics();
        expect(d.lastError).toBeNull();
        expect(d.running, 'span must terminate').toBe(false);
        expect(d.pbr, 'fixture must actually execute').toBe(0x41);
        expect(d.rombr).toBe(c.rombr);
        expect(d.registers[12], 'LOOP must exhaust the span').toBe(0);
        for (const [a, count, crc] of c.checks) {
            const bytes = Uint8Array.from({ length: count }, (_, i) => memory.readCartridgeRAM(a + i));
            expect(crc32(bytes) >>> 0, `RAM $${a.toString(16)}`).toBe(crc);
        }
    });
}

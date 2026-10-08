/** Native generated object class maps against the real target-compiled C. */
import { expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';
const FX3 = process.env.OL_FX3 !== '0';

function symbols(path: string) {
    const map = new Map<string, number>();
    for (const line of readFileSync(path, 'utf8').split('\n')) {
        const match = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (match) map.set(match[3], parseInt(match[1], 16) * 65536 + parseInt(match[2], 16));
    }
    return map;
}

function cartridge(romPath: string, symbolPath: string) {
    const emulator = new SNESEmulator();
    const rom = new Uint8Array(readFileSync(romPath));
    emulator.loadROM(rom);
    const memory = emulator.getMemory(), cpu = emulator.getCPU(), syms = symbols(symbolPath);
    if (!FX3) {
        // Production's five resident code images; no cartridge ROM code or
        // classification data is read while the GSU owns the ROM bus.
        for (const [bank, at, size] of [[63, 0x7E8000, 0x8000], [62, 0x7E5800, 0x2800],
            [61, 0x7E0600, 0x1600], [4, 0x7F0000, 0x8000], [5, 0x7F8000, 0x8000]])
            for (let i = 0; i < size; i++) memory.write(at + i, rom[bank * 0x8000 + i]);
    }
    const symbol = (name: string) => {
        const address = syms.get(name);
        if (address === undefined) throw new Error(`missing ${name} in ${romPath}`);
        return address;
    };
    const write16 = (address: number, value: number) => {
        memory.write(address, value & 255); memory.write(address + 1, value >>> 8);
    };
    const read16 = (address: number) => memory.read(address) | memory.read(address + 1) << 8;
    const entry = symbol('ItemObj_class'), result = symbol('tcc__r0');
    const call = (type: number) => {
        cpu.E = 0; cpu.P = 4 | (type & 1) | (type & 0x40);
        cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        cpu.A = 0xA5A5; cpu.X = 0xBEEF; cpu.Y = 0xCAFE;
        write16(0x1EE1, 0x04FF); memory.write(0x1EE3, 0x7E);
        write16(0x1EE4, type); write16(0x1EE6, 0xD00D); write16(0x1EA0, 0xABCD);
        write16(result, 0xFFFE);
        cpu.PB = entry >>> 16; cpu.PC = entry & 65535;
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps < 1000) {
            cpu.step(); steps++;
        }
        if (steps === 1000 || cpu.S !== 0x1EE3 || cpu.DB !== 0x7E || cpu.D !== 0 ||
            cpu.E !== 0 || (cpu.P & 0x3C) !== 4 || read16(0x1EE4) !== type ||
            read16(0x1EE6) !== 0xD00D || read16(0x1EA0) !== 0xABCD)
            throw new Error(`type ${type} damaged the C ABI in ${romPath}`);
        return { result: read16(result), steps };
    };
    return { memory, symbol, call };
}

it.skipIf(!process.env.OL_REFERENCE_ROM)('classifies all65536 unsigned types exactly like the executable C', () => {
    const native = cartridge(process.env.OL_ROM!, process.env.OL_SYMS!);
    const reference = cartridge(process.env.OL_REFERENCE_ROM!, process.env.OL_REFERENCE_SYMS!);
    expect(native.symbol('ItemObj_classTableEnd') - native.symbol('ItemObj_classTable')).toBe(256);
    if (!FX3) {
        expect(native.symbol('ItemObj_class') >>> 16).toBe(0x7E);
        expect(native.symbol('ItemObj_classTable') >>> 16).toBe(0x7E);
    }
    const none = reference.call(65535).result;
    const nativeInstructions = [0, 0], referenceInstructions = [0, 0];
    let validClasses = 0;
    for (let type = 0; type < 65536; type++) {
        const fast = native.call(type), c = reference.call(type), bucket = type < 256 ? 0 : 1;
        if (fast.result !== c.result)
            throw new Error(`type ${type}: native class ${fast.result}, C class ${c.result}`);
        nativeInstructions[bucket] += fast.steps; referenceInstructions[bucket] += c.steps;
        if (type < 256) {
            const stored = native.memory.read(native.symbol('ItemObj_classTable') + type);
            if (stored !== c.result) throw new Error(`generated table differs from C at type ${type}`);
            validClasses += Number(stored !== none);
        }
    }
    expect(validClasses).toBe(FX3 ? 66 : 53);
    expect(nativeInstructions[0]).toBeLessThan(referenceInstructions[0]);
    expect(nativeInstructions[1]).toBeLessThan(referenceInstructions[1]);
    console.log(`[openlara] object classes: all65536 native/C inputs and C ABI exact; ` +
        `byte inputs ${nativeInstructions[0]}/${referenceInstructions[0]}, ` +
        `high guard ${nativeInstructions[1]}/${referenceInstructions[1]} native/C instructions`);
});

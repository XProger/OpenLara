/** FX3 adaptive framebuffer ownership, run with tools/run_typesnes.py. */
import { describe, it, expect } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

const ROM = process.env.OL_ROM ?? '';
const symbols = new Map<string, number>();
for (const line of readFileSync(process.env.OL_SYMS ?? '', 'utf8').split('\n')) {
    const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
    if (m) symbols.set(m[3], parseInt(m[1], 16) * 65536 + parseInt(m[2], 16));
}
function sym(name: string) {
    const address = symbols.get(name);
    if (address === undefined) throw new Error(`missing staging symbol ${name}`);
    return address;
}
// The host image's bank-1 link addresses run from WRAM in CPU bank zero.
function code(name: string) {
    const address = sym(name);
    return address >>> 16 === 1 && (address & 65535) < 0x8000 ? address & 65535 : address;
}
const COLUMNS = 27, COLUMN_BYTES = 1152, SRAM_STRIDE = 1280;
const PACKED_BYTES = COLUMNS * COLUMN_BYTES;
const DP = 0x7E0000;
const HOST = { fbDraw: DP + 13, gsuDone: DP + 14, dmaBusy: DP + 15, dmaBuffer: DP + 16,
    dmaSrc: DP + 18, dmaLeft: DP + 22 };

function machine() {
    const emulator = new SNESEmulator();
    emulator.loadROM(new Uint8Array(readFileSync(ROM)));
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    const u8 = (a: number) => memory.read(a);
    const u16 = (a: number) => u8(a) | u8(a + 1) << 8;
    const w16 = (a: number, value: number) => {
        memory.write(a, value & 255); memory.write(a + 1, value >>> 8 & 255);
    };
    const bytes = (at: number, count: number) => Buffer.from(Array.from({ length: count }, (_, i) => u8(at + i)));
    const packed = () => Buffer.concat(Array.from({ length: COLUMNS }, (_, c) =>
        bytes(0x710000 + c * SRAM_STRIDE, COLUMN_BYTES)));
    return { emulator, memory, cpu, u8, u16, w16, bytes, packed };
}

describe('OpenLara FX3: adaptive staging ownership', () => {
    it('claims each completed frame once across main-loop, IRQ and nested NMI handoffs', () => {
        const g = machine(), rom = readFileSync(ROM), fx = g.emulator.getSuperFX();
        for (let i = 0; i < sym('HostCodeEnd') - sym('HostCodeStart'); i++)
            g.memory.write(0x7E0100 + i, rom[0x8000 + i]);
        g.memory.write(0x4200, 0);
        const pending = DP | g.u8(code('MainLoop') + 1);
        const probe = (entry: string, inject = -1) => {
            fx.writeRegister(0x7030, 0);
            fx.writeRegister(0x701E, 0); fx.writeRegister(0x701F, 0); fx.writeRegister(0x7030, 0);
            for (let i = 0; i < 0x28; i++) g.memory.write(DP + i, 0);
            g.memory.write(pending, 4); g.memory.write(pending - 1, 1);
            g.w16(sym('olFrameLoadHold'), 120); g.w16(sym('ol_ticksDropped'), 0);
            g.memory.write(sym('olFrameStaged'), 0); g.memory.write(sym('olIrqUpload'), 0);
            g.cpu.E = 0; g.cpu.P = 0x24; g.cpu.D = 0; g.cpu.DB = 0; g.cpu.S = 0x1C00;
            g.w16(0x1C01, 0x04FF); g.cpu.PB = 0; g.cpu.PC = code(entry);
            let steps = 0, queues = 0, nmis = 0;
            const end = entry === 'MainLoop@frame' ? code('MainLoop@idle') : 0x0500;
            while (g.cpu.PC !== end && steps < 10000) {
                if (steps === inject) {
                    g.cpu.setNMIEnabled(true); g.cpu.triggerNMI(); g.cpu.setNMIEnabled(false);
                    g.cpu.step(); steps++;
                }
                const pc = g.cpu.PB << 16 | g.cpu.PC;
                if (pc === code('NmiHandler')) nmis++;
                if (pc === code('QueueUploadFX3Staged')) queues++;
                g.cpu.step(); steps++;
            }
            expect(steps).toBeLessThan(10000);
            expect(nmis).toBe(inject < 0 ? 0 : 1);
            expect(queues, `${entry} boundary ${inject} queued the same render twice`).toBe(1);
            expect(g.u8(HOST.gsuDone)).toBe(1);
            expect(g.u8(HOST.dmaBusy)).toBe(1);
            expect([g.cpu.S, g.cpu.D, g.cpu.DB, g.cpu.P & 0x3C])
                .toEqual([entry === 'MainLoop@frame' ? 0x1C00 : 0x1C02, 0, 0, 0x24]);
            return steps;
        };
        let boundaries = 0;
        for (const entry of ['MainLoop@frame', 'TryQueueFX3Frame']) {
            const steps = probe(entry);
            for (let at = 0; at < steps; at++) { probe(entry, at); boundaries++; }
        }
        console.log(`[openlara] completed-frame claim: ${boundaries} main/IRQ instruction boundaries with real nested NMI; one queue per render`);
    });

    it('starts a prepared snapshot exactly once across every nested NMI boundary', () => {
        const g = machine(), rom = readFileSync(ROM);
        for (let i = 0; i < sym('HostCodeEnd') - sym('HostCodeStart'); i++)
            g.memory.write(0x7E0100 + i, rom[0x8000 + i]);
        g.memory.write(0x4200, 0);
        const snapshot = [0xABCD, 0x1234, 0x0040, 0xFEDC, 0x8001, 0x7FFF];
        const probe = (claim: number, busy: number, staged: number, inject = -1) => {
            g.emulator.getSuperFX().writeRegister(0x7030, 0); // stop the preceding isolated launch
            for (let i = 0; i < 0x28; i++) g.memory.write(DP + i, 0);
            g.memory.write(HOST.gsuDone, claim); g.memory.write(HOST.dmaBusy, busy);
            g.memory.write(sym('olFrameStaged'), staged);
            g.w16(sym('olFrameLoadHold'), 0); // no unrelated completed-frame claim
            g.memory.write(sym('olIrqUpload'), 0);
            g.memory.write(sym('olHudDirty'), 0);
            g.w16(DP + 22, 1); // interrupt can finish the last DMA column
            g.w16(sym('olFrameEntry'), 0x3456);
            snapshot.forEach((v, i) => {
                g.emulator.getSuperFX().writeRegister(0x700C + i * 2, v & 255);
                g.emulator.getSuperFX().writeRegister(0x700D + i * 2, v >>> 8);
            });
            // Live camera can advance after publication. Launch must use
            // the register snapshot prepared for this rendered frame.
            g.w16(sym('ol_camX'), 0x7777); g.w16(sym('ol_camY'), 0x2222);
            g.cpu.E = 0; g.cpu.P = 0x24; g.cpu.D = 0; g.cpu.DB = 0;
            g.cpu.S = 0x1C00; g.cpu.X = 0xBEEF; g.cpu.Y = 0xCAFE;
            g.w16(0x1C01, 0x04FF);
            g.cpu.PB = 0; g.cpu.PC = code('StartQueuedFX3Frame');
            let steps = 0, starts = 0, nmis = 0;
            while (g.cpu.PC !== 0x0500 && steps < 10000) {
                if (steps === inject) {
                    g.cpu.setNMIEnabled(true); g.cpu.triggerNMI(); g.cpu.setNMIEnabled(false);
                    g.cpu.step(); steps++; // enter NMI before counting an interrupted launch
                }
                const pc = g.cpu.PB << 16 | g.cpu.PC;
                if (pc === code('NmiHandler')) nmis++;
                if (pc === code('StartQueuedFX3Frame@launch')) {
                    starts++;
                    expect(g.u8(HOST.dmaBusy) === 0 || staged === 1 || g.u16(HOST.dmaLeft) === 0,
                        'started rendering into a direct upload still in flight').toBe(true);
                    expect(g.emulator.getSuperFX().getDiagnostics().registers.slice(6, 12)).toEqual(snapshot);
                    expect(g.cpu.A).toBe(0x3456);
                }
                g.cpu.step(); steps++;
            }
            expect(steps).toBeLessThan(10000);
            expect(nmis).toBe(inject < 0 ? 0 : 1);
            expect(starts).toBe(claim === 3 && (!busy || staged || inject >= 0) ? 1 : 0);
            expect(g.u8(HOST.gsuDone)).toBe(starts ? 0 : claim);
            expect([g.cpu.S, g.cpu.D, g.cpu.DB, g.cpu.X, g.cpu.Y, g.cpu.P & 0x3C])
                .toEqual([0x1C02, 0, 0, 0xBEEF, 0xCAFE, 0x24]);
            return steps;
        };
        let cases = 0, boundaries = 0;
        for (const claim of [0, 1, 3]) for (const busy of [0, 1]) for (const staged of [0, 1]) {
            const steps = probe(claim, busy, staged); cases++;
            for (let at = 0; at < steps; at++) { probe(claim, busy, staged, at); boundaries++; }
        }
        console.log(`[openlara] deferred launch: ${cases} ownership cases and ${boundaries} nested NMI boundaries; one start, immutable camera registers, direct-buffer ownership and CPU ABI preserved`);
    });

    it('reserves logic headroom and keeps upload ownership across publication decisions and NMI entry', () => {
        const g = machine(), rom = readFileSync(ROM);
        for (let i = 0; i < sym('HostCodeEnd') - sym('HostCodeStart'); i++)
            g.memory.write(0x7E0100 + i, rom[0x8000 + i]);
        g.memory.write(0x4200, 0);
        const loop = code('MainLoop'), hold = code('MainLoop@holdRender');
        expect(g.u8(loop)).toBe(0xA5);
        expect([g.u8(hold), g.u8(hold + 1), g.u8(hold + 2)]).toEqual([0xA9, 0x88, 0x85]);
        const pending = DP | g.u8(loop + 1), timer = DP | g.u8(hold + 3);
        // logicPhase immediately precedes logicPending in the shared host ABI.
        const probe = (queued: number, deadline: number, staged: number, busy: number, dropped: number, inject = -1, load = 0) => {
            for (let i = 0; i < 0x28; i++) g.memory.write(DP + i, 0);
            g.memory.write(pending, queued); g.memory.write(pending - 1, 1);
            g.memory.write(timer, deadline); g.memory.write(HOST.dmaBusy, busy);
            g.memory.write(HOST.gsuDone, 1);
            g.memory.write(sym('olFrameStaged'), staged);
            g.w16(sym('olFrameLoadHold'), load);
            g.w16(sym('olTickHead'), 0); g.w16(sym('olTickTail'), 0);
            g.w16(sym('ol_ticksDropped'), dropped);
            g.cpu.E = 0; g.cpu.P = 0x24; g.cpu.D = 0; g.cpu.DB = 0; g.cpu.S = 0x1C00;
            g.cpu.PB = 0; g.cpu.PC = code('MainLoop@uploaded');
            let steps = 0, nmis = 0;
            while (![code('MainLoop@publish'), code('MainLoop@idle')].includes(g.cpu.PB << 16 | g.cpu.PC) && steps < 1000) {
                if (steps === inject) {
                    g.cpu.setNMIEnabled(true); g.cpu.triggerNMI(); g.cpu.setNMIEnabled(false);
                }
                if ((g.cpu.PB << 16 | g.cpu.PC) === code('NmiHandler')) nmis++;
                g.cpu.step(); steps++;
            }
            expect(steps).toBeLessThan(1000);
            expect(nmis).toBe(inject < 0 ? 0 : 1);
            expect([g.cpu.S, g.cpu.D, g.cpu.DB, g.cpu.E, g.cpu.P & 0x3C]).toEqual([0x1C00, 0, 0, 0, 0x24]);
            const publish = g.cpu.PC === code('MainLoop@publish');
            if (inject < 0) {
                expect(publish).toBe((queued < (dropped ? 1 : load >= 128 ? 2 : 4) || deadline === 0x80));
                expect(g.u8(pending), 'publication consumed a logic tick').toBe(queued);
                expect(g.u8(timer)).toBe(!publish && deadline === 0 ? 0x88 : deadline);
            } else {
                expect(g.u8(pending)).toBe(Math.min(4, queued + 1));
                if (publish) expect(g.u8(pending) <= (dropped ? 1 : g.u8(sym('olFrameLoadHold')) >= 128 ? 2 : 4) || g.u8(timer) === 0x80,
                    'NMI left a publication without queue headroom or an expired deadline').toBe(true);
            }
            expect(g.u16(sym('ol_ticksDropped'))).toBe((dropped + Number(inject >= 0 && queued === 4)) & 65535);
            expect(g.u8(sym('olFrameStaged'))).toBe(staged);
            expect(g.u8(HOST.dmaBusy)).toBe(busy);
            expect(g.u16(sym('olTickHead')), 'publication consumed an input sample').toBe(0);
            expect(g.u16(sym('olTickTail'))).toBe(inject >= 0 && queued < 4 ? 2 : 0);
            return steps;
        };
        let cases = 0, boundaries = 0;
        for (const dropped of [0, 1, 255, 256, 65535]) for (const load of [0, 1, 120, 128, 135])
        for (let queued = 0; queued <= 4; queued++) for (const timer of [0, 0x90, 0x88, 0x81, 0x80])
            for (const staged of [0, 1]) for (const busy of [0, 1]) {
                probe(queued, timer, staged, busy, dropped, -1, load); cases++;
            }
        for (const dropped of [0, 1, 255, 256, 65535]) for (const load of [0, 1, 120, 128, 135])
        for (let queued = 0; queued <= 4; queued++) for (const timer of [0, 0x90, 0x88, 0x81, 0x80]) {
            const steps = probe(queued, timer, 0, 0, dropped, -1, load);
            for (let at = 0; at < steps; at++) { probe(queued, timer, 0, 0, dropped, at, load); boundaries++; }
        }
        console.log(`[openlara] FX3 publication: ${cases} queue/deadline/ownership cases, ${boundaries} real NMI boundaries; three queued ticks when healthy, one after a costly tick, empty queue after overload, input/tick ownership preserved`);
    });

    it('renews only heavy tick costs, preserves the gauge value, and saturates the field countdown at zero', () => {
        const g = machine(), hold = sym('olFrameLoadHold'), mode = sym('olFrameStaged');
        g.memory.write(0x4200, 0);
        const call = (name: string, accumulator: number, byteAccumulator: boolean) => {
            g.cpu.E = 0; g.cpu.P = byteAccumulator ? 0x24 : 4;
            g.cpu.D = 0; g.cpu.DB = 0; g.cpu.S = 0x1C00; g.cpu.A = accumulator;
            g.w16(0x1C01, 0x04FF); g.memory.write(0x1C03, 0x7E); // synthetic JSL return
            g.cpu.PB = code(name) >>> 16; g.cpu.PC = code(name) & 65535;
            let steps = 0;
            while ((g.cpu.PB !== 0x7E || g.cpu.PC !== 0x0500) && steps++ < 100) g.cpu.step();
            expect(steps, `${name} did not return`).toBeLessThan(100);
            expect([g.cpu.S, g.cpu.D, g.cpu.DB, g.cpu.E, g.cpu.P & 0x3C])
                .toEqual([0x1C03, 0, 0, 0, byteAccumulator ? 0x24 : 4]);
        };
        g.memory.write(mode, 0xA5);
        for (const previous of [0, 77, 120, 127, 128, 135]) for (const cost of [0, 1, 2, 3, 59]) {
            g.w16(hold, previous);
            call('StageFX3TickCost', cost, false);
            expect(g.cpu.A, 'backpressure changed the tick gauge accumulator').toBe(cost);
            expect(g.u16(hold)).toBe(cost >= 3 ? 131 : cost === 2 && previous < 128 ? 120 : previous);
            expect(g.u8(mode), 'countdown renewal changed a queued frame source').toBe(0xA5);
        }
        g.w16(hold, 131);
        for (let field = 1; field <= 132; field++) {
            call('StageFX3Field', 0xBC00, true);
            expect(g.u16(hold)).toBe(Math.max(0, 131 - field));
            expect(g.cpu.A & 0xFF00, 'A8 countdown changed the accumulator high byte').toBe(0xBC00);
            expect(g.u8(mode), 'field countdown changed a queued frame source').toBe(0xA5);
        }
        console.log('[openlara] staging policy: 0/1-field ticks leave the hold, 2 renew 120 fields, 3+ renew a four-field burst guard, countdown saturates, widths and gauge exact');
    });

    it('uses WRAM staging only with the measured logic headroom and retains the selected upload source', () => {
        const g = machine(), rom = readFileSync(ROM);
        for (let i = 0; i < sym('HostCodeEnd') - sym('HostCodeStart'); i++)
            g.memory.write(0x7E0100 + i, rom[0x8000 + i]);
        g.memory.write(0x4200, 0);
        const pending = DP | g.u8(code('MainLoop') + 1), entry = code('FX3WantStaging');
        let cases = 0;
        for (let load = 0; load <= 135; load++) for (let queued = 0; queued <= 4; queued++)
        for (const dropped of [0, 1, 256]) {
            g.w16(sym('olFrameLoadHold'), load); g.w16(sym('ol_ticksDropped'), dropped);
            g.memory.write(pending, queued); g.memory.write(sym('olFrameStaged'), 0xA5);
            g.cpu.E = 0; g.cpu.P = 0x24; g.cpu.D = 0; g.cpu.DB = 0; g.cpu.S = 0x1C00;
            g.cpu.A = 0xBC00; g.cpu.PB = entry >>> 16; g.cpu.PC = entry & 65535;
            g.w16(0x1C01, 0x04FF);
            let steps = 0;
            while (g.cpu.PC !== 0x0500 && steps++ < 100) g.cpu.step();
            expect(steps).toBeLessThan(100);
            expect(g.cpu.A).toBe(0xBC00 | Number(load === 0 || load < 128 && queued === 0 && dropped === 0));
            expect([g.cpu.S, g.cpu.D, g.cpu.DB, g.cpu.P & 0x3C]).toEqual([0x1C02, 0, 0, 0x24]);
            expect([g.u16(sym('olFrameLoadHold')), g.u16(sym('ol_ticksDropped')), g.u8(pending), g.u8(sym('olFrameStaged'))])
                .toEqual([load, dropped, queued, 0xA5]);
            cases++;
        }
        console.log(`[openlara] adaptive staging: ${cases} load/queue/overload combinations, upload source and accumulator width preserved`);
    });

    it('copies every used planar byte, skips unused rows, and preserves both staging canaries', () => {
        const g = machine(), stage = sym('olFrameStage');
        expect(stage).toBe(0x7F8000);
        g.memory.write(0x4200, 0);  // isolated routine, no interrupts or render
        for (let c = 0; c < COLUMNS; c++) for (let i = 0; i < SRAM_STRIDE; i++)
            g.memory.write(0x710000 + c * SRAM_STRIDE + i,
                i < COLUMN_BYTES ? (Math.imul(c + 1, 73) + i * 29 + (i >>> 8)) & 255 : 0xDE);
        const source = g.bytes(0x710000, COLUMNS * SRAM_STRIDE), expected = g.packed();
        for (let i = 0; i < PACKED_BYTES; i++) g.memory.write(stage + i, 0xCD);
        for (let i = 1; i <= 16; i++) g.memory.write(stage - i, 0xA5);
        for (let i = 0; i < 16; i++) g.memory.write(stage + PACKED_BYTES + i, 0x5A);
        g.w16(sym('olFrameLoadHold'), 0);
        g.cpu.E = 0; g.cpu.P = 0x24; g.cpu.D = 0; g.cpu.DB = 0; g.cpu.S = 0x1C00;
        g.w16(0x1C01, 0x04FF); // synthetic JSR return in bank zero
        g.cpu.PB = code('StageFX3Frame') >>> 16;
        g.cpu.PC = code('StageFX3Frame') & 65535;
        let steps = 0;
        while ((g.cpu.PB !== 0 || g.cpu.PC !== 0x0500) && steps++ < 10000) g.cpu.step();
        expect(steps, 'packed staging routine did not return').toBeLessThan(10000);
        expect([g.cpu.S, g.cpu.E, g.cpu.P & 0x30]).toEqual([0x1C02, 0, 0x20]);
        expect(g.u8(sym('olFrameStaged'))).toBe(1);
        expect(g.bytes(stage, PACKED_BYTES)).toEqual(expected);
        expect(g.bytes(stage - 16, 16)).toEqual(Buffer.alloc(16, 0xA5));
        expect(g.bytes(stage + PACKED_BYTES, 16)).toEqual(Buffer.alloc(16, 0x5A));
        expect(g.bytes(0x710000, COLUMNS * SRAM_STRIDE)).toEqual(source);
        console.log(`[openlara] staging: all ${PACKED_BYTES} packed bytes, source padding and 32 canary bytes exact`);
    });

    it('keeps queued banks and strides fixed through direct, staged and direct uploads while the next frame changes', () => {
        const g = machine();
        const cart = g.memory as unknown as { writeCartridgeRAM(offset: number, value: number): void };
        const bootLevel = Number(process.env.OL_BOOT_LEVEL ?? 0);
        [0x4F, 0x4C, 0x42, 0x52, bootLevel & 255, bootLevel >>> 8, 0, 0]
            .forEach((v, i) => cart.writeCartridgeRAM(0x3100 + i, v));
        // Disable the FPS text so camera changes are the only intended new pixels.
        [0x4F, 0x4C, 0x53, 0x54, 1, 1, 0, 1, 1, 0, 0, 0]
            .forEach((v, i) => cart.writeCartridgeRAM(0x3110 + i, v));
        for (let i = 0; i < 600 && !g.u16(sym('itemsCount')); i++) g.emulator.executeFrames(1);
        expect(g.u16(sym('itemsCount')), 'the level never loaded').toBeGreaterThan(0);
        g.emulator.executeFrames(180);

        type Upload = { index: number; mode: number; expected: Buffer; columns: number; target: number; changedHold: boolean };
        let pending: Upload | undefined;
        const modes: number[] = [], completed: Upload[] = [];
        let watching = false, changedNext = false, overlappingStart = false, lost = 0;
        const ppu = g.emulator.getPPU() as unknown as {
            videoMemoryWritable(): boolean; writeRegister(address: number, value: number): void };
        const write = ppu.writeRegister.bind(ppu);
        ppu.writeRegister = (address, value) => {
            if ((address === 0x2118 || address === 0x2119) && !ppu.videoMemoryWritable()) lost++;
            return write(address, value);
        };
        const step = g.cpu.step;
        g.cpu.step = function (this: typeof g.cpu) {
            const pc = this.PB << 16 | this.PC;
            if (pending && g.u8(HOST.dmaBusy))
                expect(g.u8(sym('olFrameStaged')), 'an in-flight upload changed its source mode').toBe(pending.mode);
            if (pc === code('TryQueueFX3Frame') && !pending && !g.u8(HOST.dmaBusy)) {
                watching = true;
                g.w16(sym('ol_ticksDropped'), 1); // force the direct arm even with an empty queue
                g.w16(sym('olFrameLoadHold'), modes.length % 2 === 0 ? 120 : 0);
            }
            if (pc === code('StageFX3Frame')) {
                expect(g.u8(HOST.dmaBusy), 'staging replaced a copy still owned by the previous upload').toBe(0);
                watching = true; // ignore an upload whose staging began before instrumentation
                g.w16(sym('ol_ticksDropped'), 1);
                // Choose direct -> staged -> direct -> staged. The hold is
                // changed again after DMA begins, independently of this choice.
                g.w16(sym('olFrameLoadHold'), modes.length % 2 === 0 ? 120 : 0);
            }
            if (pc === code('QueueUploadFX3Staged') && watching) {
                expect(pending, 'a new frame replaced an unfinished upload').toBeUndefined();
                const mode = g.u8(sym('olFrameStaged'));
                const expected = g.packed();
                if (mode) expect(g.bytes(sym('olFrameStage'), PACKED_BYTES)).toEqual(expected);
                pending = { index: modes.length, mode, expected, columns: 0,
                    target: g.u8(HOST.fbDraw) ? 0x7980 : 0, changedHold: false };
                modes.push(mode);
            }
            if (pc === code('UploadFX3Cols@column') && pending) {
                expect(g.u8(0x4304), `upload ${pending.index} used the wrong source bank`).toBe(pending.mode ? 0x7F : 0x71);
                expect(g.u16(HOST.dmaSrc), `upload ${pending.index} column ${pending.columns} used the wrong stride`)
                    .toBe((pending.mode ? 0x8000 : 0) + pending.columns * (pending.mode ? COLUMN_BYTES : SRAM_STRIDE));
                expect(g.u8(HOST.dmaBuffer) ? 0x7980 : 0).toBe(pending.target);
                if (!pending.changedHold) {
                    g.w16(sym('olFrameLoadHold'), pending.mode ? 120 : 0);
                    pending.changedHold = true;
                }
                pending.columns++;
            }
            if (pc === code('StartQueuedFX3Frame@launch') && pending && g.u8(HOST.dmaBusy)) {
                expect(pending.mode || g.u16(HOST.dmaLeft) === 0,
                    'the direct SRAM upload released its buffer before its last column').toBeTruthy();
                if (pending.mode) overlappingStart = true;
            }
            if (pc === code('StartGSU') && pending && pending.mode && g.u8(HOST.dmaBusy)) {
                if (!changedNext) {
                    g.w16(sym('ol_angle'), g.u16(sym('ol_angle')) + 0x4000);
                    changedNext = true;
                }
            }
            if (pc === code('UploadFX3Cols@complete') && pending) {
                expect(pending.columns).toBe(COLUMNS);
                const actual = Buffer.from(Array.from({ length: PACKED_BYTES }, (_, i) => g.memory.readVRAM(pending!.target + i)));
                expect(actual, `upload ${pending.index} mixed bytes from the next rendered frame`).toEqual(pending.expected);
                if (pending.mode) expect(g.bytes(sym('olFrameStage'), PACKED_BYTES)).toEqual(pending.expected);
                completed.push(pending);
                pending = undefined;
            }
            return step.call(this);
        };
        try {
            for (let i = 0; i < 180 && completed.length < 4; i++) g.emulator.executeFrames(1);
        } finally {
            g.cpu.step = step; ppu.writeRegister = write;
        }
        expect(completed.length, 'four adaptive uploads never completed').toBeGreaterThanOrEqual(4);
        expect(modes.slice(0, 4)).toEqual([0, 1, 0, 1]);
        expect(overlappingStart, 'no staged upload overlapped a live render').toBe(true);
        expect(changedNext).toBe(true);
        const changed = completed[1].expected.reduce((n, v, i) => n + Number(v !== completed[2].expected[i]), 0);
        expect(changed, 'the next live render did not produce different pixels').toBeGreaterThan(100);
        expect(lost, 'a VRAM write landed outside forced blank').toBe(0);
        console.log(`[openlara] staging ownership: modes ${modes.slice(0, 4)}, 108 exact column banks/strides, ` +
                    `${changed} altered next-frame bytes, zero torn or out-of-blank VRAM writes`);
    }, 600000);
});

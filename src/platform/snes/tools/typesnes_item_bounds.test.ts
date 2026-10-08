/** Exact rotated item contacts on both native variants, with optional C oracles. */
import { describe, it, expect } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

const FX3 = process.env.OL_FX3 === '1';
const signed = (v: number) => v << 16 >> 16;
const ITEM_SIZE = 32;
function machine(romPath: string, symbolPath: string) {
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
    const rom = new Uint8Array(readFileSync(romPath)), emulator = new SNESEmulator();
    emulator.loadROM(rom);
    const memory = emulator.getMemory(), cpu = emulator.getCPU();
    const bus = (emulator as unknown as { bus: { masterCycles: number } }).bus;
    const w8 = (at: number, value: number) => {
        if (FX3 && at >= 0xE00000) (memory as unknown as { rom: Uint8Array }).rom[at - 0xC00000] = value & 255;
        else memory.write(at, value & 255);
    };
    const w16 = (at: number, value: number) => { w8(at, value); w8(at + 1, value >>> 8); };
    const u16 = (at: number) => memory.read(at) | memory.read(at + 1) << 8;
    const bytes = (at: number, count: number) => Buffer.from(Array.from({ length: count }, (_, i) => memory.read(at + i)));
    if (!FX3) for (const [bank, at, size] of [[63,0x7E8000,0x8000], [62,0x7E5800,0x2800],
        [61,0x7E0600,0x1600], [4,0x7F0000,0x8000], [5,0x7F8000,0x8000]])
        for (let i = 0; i < size; i++) w8(at + i, rom[bank * 0x8000 + i]);
    const sine = (angle: number) => {
        const index = ((angle + 32) & 65535) >>> 5 & 0x7FE;
        return [signed(u16(sym('olSinTable') + index)), signed(u16(sym('olSinTable') + ((index + 512) & 0x7FE)))];
    };
    const bind = (base: number, anim: number, box: number[]) => {
        w16(sym('olAnimBoxBase'), base);
        if (FX3) w16(sym('olAnimBoxData') + 2, 0xF8);
        const record = (base + anim * 12) & 65535;
        expect(record + 12, 'box fixture must fit its bank').toBeLessThanOrEqual(65536);
        box.forEach((value, i) => w16((FX3 ? 0xF80000 : 0x710000) + record + i * 2, value));
    };
    const call = (item: number, lara: number, radius: number, height: number) => {
        cpu.E = 0; cpu.P = 4; cpu.DB = 0x7E; cpu.D = 0; cpu.S = 0x1EE0;
        w16(0x1EE1, 0x04FF); w8(0x1EE3, 0x7E);
        const args = [item & 65535,item >>> 16,lara & 65535,lara >>> 16,radius,height];
        args.forEach((v, i) => w16(0x1EE4 + i * 2, v));
        w16(0x1C00, 0xABCD); w16(0x1EF0, 0xD00D);
        cpu.PB = sym('ItemObj_collideBounds') >>> 16; cpu.PC = sym('ItemObj_collideBounds') & 65535;
        const before = bus.masterCycles;
        let steps = 0;
        while ((cpu.PB !== 0x7E || cpu.PC !== 0x500) && steps++ < 50000) cpu.step();
        expect(steps, 'bounds routine did not return').toBeLessThan(50000);
        expect([cpu.S,cpu.DB,cpu.D,cpu.E,cpu.P & 0x3C]).toEqual([0x1EE3,0x7E,0,0,4]);
        expect([u16(0x1C00),u16(0x1EF0)]).toEqual([0xABCD,0xD00D]);
        return { value: u16(sym('tcc__r0')), steps, clocks: bus.masterCycles - before };
    };
    return { sym, w8, w16, bytes, sine, bind, call, nativeBounds: names.has('IB_Multiply') };
}

describe(`OpenLara ${FX3 ? 'FX3' : 'GSU-2'}: exact rotated item bounds`, () => {
    const machines = () => {
        const result = [machine(process.env.OL_ROM!, process.env.OL_SYMS!)];
        for (const [rom, syms] of [[process.env.OL_BASELINE_ROM,process.env.OL_BASELINE_SYMS],
            [process.env.OL_REFERENCE_ROM,process.env.OL_REFERENCE_SYMS]]) if (rom && syms) result.push(machine(rom,syms));
        return result;
    };
    type Case = { angle: number; box: number[]; delta: number[]; anchor: number[]; radius: number; height: number; anim: number; base: number };
    const oracle = (c: Case, sine: number[]) => {
        const [s,cos] = sine, [dx,dy,dz] = c.delta;
        const px = signed((Math.imul(dx,cos) - Math.imul(dz,s)) >> 14);
        const pz = signed((Math.imul(dz,cos) + Math.imul(dx,s)) >> 14);
        return +(signed(c.box[3] + c.height) > dy && c.box[2] < dy &&
            px >= signed(c.box[0] - c.radius) && px <= signed(c.box[1] + c.radius) &&
            pz >= signed(c.box[4] - c.radius) && pz <= signed(c.box[5] + c.radius));
    };
    function run(cases: Case[]) {
        const pair = machines(), costs = pair.map(() => ({ steps: 0,clocks: 0 }));
        const locations = [[0x7E0580,0x7E05C0], [0x000580,0x7FFF50], [0x7FFF00,0x0005C0], [0x710200,0x710240]];
        let hits = 0;
        cases.forEach((c, test) => {
            const want = oracle(c,pair[0].sine(c.angle)); hits += want;
            const itemBytes = Buffer.alloc(ITEM_SIZE,0xA5), laraBytes = Buffer.alloc(ITEM_SIZE,0x5A);
            itemBytes.writeUInt16LE(c.anim,12); itemBytes.writeUInt16LE(c.angle,10);
            [4,6,8].forEach((at, i) => {
                itemBytes.writeInt16LE(c.anchor[i],at);
                laraBytes.writeInt16LE(signed(c.anchor[i] + c.delta[i]),at);
            });
            const [item,lara] = locations[test % locations.length];
            pair.forEach((g, i) => {
                expect(g.sine(c.angle)).toEqual(pair[0].sine(c.angle));
                g.bind(c.base,c.anim,c.box);
                for (const [at,data] of [[item,itemBytes],[lara,laraBytes]] as const) {
                    g.w16(at - 2,0xA55A); g.w16(at + ITEM_SIZE,0x5AA5);
                    data.forEach((v,n) => g.w8(at + n,v));
                }
                const beforeItem = g.bytes(item - 2,ITEM_SIZE + 4), beforeLara = g.bytes(lara - 2,ITEM_SIZE + 4);
                const result = g.call(item,lara,c.radius,c.height);
                expect(result.value, `${test}/${i} angle ${c.angle} box ${c.box} delta ${c.delta} radius/height ${c.radius}/${c.height}`).toBe(want);
                expect(g.bytes(item - 2,ITEM_SIZE + 4)).toEqual(beforeItem);
                expect(g.bytes(lara - 2,ITEM_SIZE + 4)).toEqual(beforeLara);
                costs[i].steps += result.steps; costs[i].clocks += result.clocks;
            });
        });
        return { costs,hits,nativeBounds: pair.map(g => g.nativeBounds) };
    }

    it('matches every trig quantum, strict vertical and inclusive horizontal edges with untouched far inputs', () => {
        const edges = [-32768,-32767,-1025,-1001,-900,-763,-762,-501,-500,-1,0,1,499,500,761,762,899,1000,1024,32766,32767];
        const quantums = [-32,-1,0,31,32,63], radii = [0,100,341,-1,32767,-32768], heights = [762,0,1,-1,32767,-32768];
        const animations = FX3 ? [0,1,15,255,1023] : [0,1,15,255,1023,5461,32767,65535];
        const cases: Case[] = [];
        for (let quantum = 0; quantum < 1024; quantum++) for (const offset of quantums) {
            const test = cases.length, ordinary = test % 3 !== 0;
            cases.push({ angle: (quantum * 64 + offset) & 65535,
                box: ordinary ? [-500,500,-762,120,-900,1000] : Array.from({ length: 6 }, (_, i) => edges[(test * (i + 1) + i * 3) % edges.length]),
                delta: ordinary ? [edges[test % edges.length],0,edges[(test * 11 + 5) % edges.length]] :
                    [edges[test % edges.length],edges[(test * 7 + 3) % edges.length],edges[(test * 11 + 5) % edges.length]],
                anchor: [edges[(test * 3) % edges.length],32760,-32760],
                radius: ordinary ? 100 : radii[(test >>> 2) % radii.length], height: ordinary ? 762 : heights[(test >>> 4) % heights.length],
                anim: animations[test % animations.length],base: 0x0800 });
        }
        // The last complete box in the bank checks high/low byte fetches
        // without introducing an invalid record spanning two banks.
        for (const delta of edges) cases.push({ angle: 0,box: [-500,500,-762,120,-900,1000],
            delta: [0,delta,0],anchor: [32760,32760,-32760],radius: 0,height: 762,anim: 0,base: 0xFFF4 });
        const result = run(cases);
        expect(result.hits).toBeGreaterThan(500);
        result.costs.forEach((cost, i) => {
            if (i && !result.nativeBounds[i]) expect(result.costs[0].steps).toBeLessThan(cost.steps);
        });
        console.log(`[openlara] item bounds: ${cases.length} independent/${result.costs.length} cartridge cases, all 1,024 trig quantums/halfsteps, ` +
            `${result.hits} contacts, signed/wrapped bounds and ${FX3 ? 'CPU-only' : 'SRAM'} bank-end boxes; ` +
            `instructions ${result.costs.map(c => c.steps).join('/')}, master clocks ${result.costs.map(c => c.clocks).join('/')}`);
    });

    it('adds or subtracts full signed products before shifting at negative rounding seams', () => {
        const g = machine(process.env.OL_ROM!,process.env.OL_SYMS!), cases: Case[] = [];
        for (let q = 0; q < 1024 && cases.length < 512; q++) for (const dx of [-257,-63,-1,1,63,257])
        for (const dz of [-257,-63,-1,1,63,257]) {
            const angle = q * 64, [s,c] = g.sine(angle);
            const px = signed((Math.imul(dx,c) - Math.imul(dz,s)) >> 14), pz = signed((Math.imul(dz,c) + Math.imul(dx,s)) >> 14);
            const wrongX = signed((Math.imul(dx,c) >> 14) - (Math.imul(dz,s) >> 14));
            const wrongZ = signed((Math.imul(dz,c) >> 14) + (Math.imul(dx,s) >> 14));
            if (px === wrongX && pz === wrongZ) continue;
            cases.push({ angle,box: [px,px,-1,1,pz,pz],delta: [dx,0,dz],anchor: [32760,32760,-32760],
                radius: 0,height: 0,anim: q % 3,base: 0x0800 });
        }
        expect(cases.length).toBeGreaterThanOrEqual(512);
        const result = run(cases);
        expect(result.hits).toBe(cases.length);
        console.log(`[openlara] item product rounding: ${cases.length} contacts rejected by separately shifted terms match ` +
            `${result.costs.length} linked cartridges and independent full-product arithmetic`);
    });
});

/** Execute linked 32-bit compiler division and remainder helpers. */
import { describe, expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

const FX3 = process.env.OL_FX3 === '1';
const NAMES = ['tcc__divdi3','tcc__moddi3','tcc__udivdi3','tcc__umoddi3'] as const;
type Operation = typeof NAMES[number];
function machine(romPath: string, symbolPath: string) {
    const names = new Map<string, number>();
    for (const line of readFileSync(symbolPath,'utf8').split('\n')) {
        const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
        if (m) names.set(m[3],parseInt(m[1],16)*65536+parseInt(m[2],16));
    }
    const sym = (name: string) => {
        const at = names.get(name);
        if (at === undefined) throw new Error(`missing ${name}`);
        return at;
    };
    const rom = readFileSync(romPath), emu = new SNESEmulator();
    emu.loadROM(new Uint8Array(rom));
    const memory = emu.getMemory(), cpu = emu.getCPU();
    const bus = (emu as unknown as { bus: { masterCycles: number } }).bus;
    const w8 = (at: number, v: number) => memory.write(at,v&255);
    const w16 = (at: number, v: number) => { w8(at,v); w8(at+1,v>>>8); };
    const w32 = (at: number, v: number) => { w16(at,v); w16(at+2,v>>>16); };
    const u16 = (at: number) => memory.read(at)|memory.read(at+1)<<8;
    const u32 = (at: number) => (u16(at)|u16(at+2)<<16)>>>0;
    if (!FX3) for (const [bank,at,size] of [[63,0x7E8000,0x8000],[62,0x7E5800,0x2800],
        [61,0x7E0600,0x1600],[4,0x7F0000,0x8000],[5,0x7F8000,0x8000]])
        for (let i=0;i<size;i++) w8(at+i,rom[bank*0x8000+i]);
    const preserved = [2,3,4,5].flatMap(r=>[sym(`tcc__r${r}`),sym(`tcc__r${r}h`)]);
    preserved.forEach((at,i)=>w16(at,0xA550+i));
    w16(0x1EE1,0x04FF); w8(0x1EE3,0x7E); w16(0x1ED0,0xBEEF); w16(0x1EEC,0xCAFE);
    let calls=0, instructions=0, clocks=0;
    const call = (name: Operation, a: number, b: number) => {
        w32(0x1EE4,a); w32(0x1EE8,b);
        cpu.E=0; cpu.P=4|(a&1)|(b&2?0x40:0); cpu.D=0; cpu.DB=0x7E; cpu.S=0x1EE0;
        cpu.A=0xABCD; cpu.X=0x1234; cpu.Y=0xFEED;
        const at=sym(name); cpu.PB=at>>>16; cpu.PC=at&65535;
        const before=bus.masterCycles; let steps=0;
        while ((cpu.PB!==0x7E || cpu.PC!==0x500) && steps<2000) { cpu.step(); steps++; }
        if (steps===2000 || cpu.S!==0x1EE3 || cpu.D!==0 || cpu.DB!==0x7E || cpu.E!==0 ||
            (cpu.P&0x3C)!==4 || u16(0x1ED0)!==0xBEEF || u16(0x1EEC)!==0xCAFE ||
            u32(0x1EE4)!==(a>>>0) || u32(0x1EE8)!==(b>>>0)) throw new Error(`${name} ABI failure ${a}/${b}`);
        for (let i=0;i<preserved.length;i++) if (u16(preserved[i])!==0xA550+i)
            throw new Error(`${name} clobbered r${2+(i>>>1)}${i&1?'h':''}`);
        const unsigned=name.includes('__u'), modulo=name.includes('mod');
        const left=unsigned?BigInt(a>>>0):BigInt(a|0), right=unsigned?BigInt(b>>>0):BigInt(b|0);
        // Preserve the old runtime's deterministic zero-divisor result too.
        const expected=Number(BigInt.asUintN(32,right===0n ? modulo?left:left<0n?1n:-1n : modulo?left%right:left/right));
        const actual=(u16(sym('tcc__r0'))|u16(sym('tcc__r1'))<<16)>>>0;
        if (actual!==expected) throw new Error(`${name} ${a>>>0}/${b>>>0}: ${actual} != ${expected}`);
        calls++; instructions+=steps; clocks+=bus.masterCycles-before;
        return actual;
    };
    return {call,cost:()=>({calls,instructions,clocks})};
}
const EDGES = [0,1,2,3,7,15,16,31,32,63,127,128,255,256,257,1023,1024,
    0x7FFF,0x8000,0xFFFF,0x10000,0x10001,0x1FFFF,0x7FFFFFFE,0x7FFFFFFF,
    0x80000000,0x80000001,0xFFFF0000,0xFFFFFFFE,0xFFFFFFFF];
function cases() {
    const result: [number,number][]=[];
    for (const a of EDGES) for (const b of EDGES) result.push([a,b]);
    for (let bit=0;bit<32;bit++) for (const delta of [-1,0,1])
        for (const b of EDGES) result.push([((2**bit)+delta)>>>0,b]);
    let seed=0xB16B00B5;
    for (let i=0;i<8192;i++) {
        seed=(Math.imul(seed,1664525)+1013904223)>>>0; const a=seed;
        seed=(Math.imul(seed,1664525)+1013904223)>>>0;
        result.push([a,i&1 ? seed&65535 : seed]);
    }
    return result;
}
describe(`OpenLara ${FX3?'FX3':'GSU-2'}: exact compiler division`,()=>{
    const native=()=>machine(process.env.OL_ROM!,process.env.OL_SYMS!);
    it('preserves signed and unsigned quotient/remainder at every word and divisor boundary',()=>{
        const g=native(), divisors=[1,2,3,7,255,256,65535,0x80000000];
        for (let a=0;a<65536;a++) for (const name of NAMES)
            g.call(name,name.includes('__u')?a:(a<<16)>>16,divisors[a&7]);
        console.log(`[openlara] division words: ${JSON.stringify(g.cost())}; all 65,536 words, four operations, ABI and arguments preserved`);
    });
    it('matches full-width independent arithmetic, signed minima, power boundaries and zero divisors',()=>{
        const g=native();
        for (const [a,b] of cases()) for (const name of NAMES) g.call(name,a,b);
        console.log(`[openlara] division wide: ${JSON.stringify(g.cost())}; exact BigInt quotient/remainder, wrapping overflow and legacy zero behavior`);
    });
    it.skipIf(!process.env.OL_BASELINE_ROM || !process.env.OL_BASELINE_SYMS)
    ('matches the preceding runtime and reduces measured division cost',()=>{
        const pair=[native(),machine(process.env.OL_BASELINE_ROM!,process.env.OL_BASELINE_SYMS!)];
        for (const [a,b] of cases()) for (const name of NAMES)
            expect(pair[0].call(name,a,b)).toBe(pair[1].call(name,a,b));
        const cost=pair.map(g=>g.cost());
        expect(cost[0].clocks).toBeLessThan(cost[1].clocks);
        console.log(`[openlara] division native/baseline: ${JSON.stringify(cost)}`);
    });
});

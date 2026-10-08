/** Actual native RAM handoffs with NMI injected at every instruction boundary. */
import { describe, expect, it } from 'vitest';
import { readFileSync } from 'fs';
import { SNESEmulator } from './emulator';

const rom = readFileSync(process.env.OL_ROM!);
const symbols = new Map<string, number>();
for (const line of readFileSync(process.env.OL_SYMS!, 'utf8').split('\n')) {
    const m = line.trim().match(/^([0-9a-f]{2}):([0-9a-f]{4})\s+(\S+)$/i);
    if (m) symbols.set(m[3], parseInt(m[1],16) * 65536 + parseInt(m[2],16));
}
const sym = (name: string) => {
    const at = symbols.get(name);
    if (at === undefined) throw new Error(`missing ${name}`);
    return at;
};

function harness() {
    const emulator = new SNESEmulator(); emulator.loadROM(new Uint8Array(rom));
    const memory = emulator.getMemory(), cpu = emulator.getCPU(), gsu = emulator.getSuperFX();
    const w8 = (at: number, value: number) => memory.write(at, value & 255);
    const w16 = (at: number, value: number) => { w8(at,value); w8(at+1,value>>>8); };
    const u16 = (at: number) => memory.read(at) | memory.read(at+1)<<8;
    const u24 = (at: number) => u16(at) | memory.read(at+2)<<16;
    for (const [bank, at, size] of [[1,0x7E0100,sym('HostCodeEnd')-sym('HostCodeStart')],
        [63,0x7E8000,0x8000],[62,0x7E5800,0x2800],[61,0x7E0600,0x1600],
        [4,0x7F0000,0x8000],[5,0x7F8000,0x8000]])
        for (let i=0; i<size; i++) w8(at+i,rom[bank*0x8000+i]);
    // Read the linked long operands, so the fixture follows the shared DP
    // declaration without duplicating its offsets or weakening NMI itself.
    const entry = sym('olGsuYieldRam');
    expect([memory.read(entry),memory.read(entry+2),memory.read(entry+8)]).toEqual([0xE2,0xAF,0xAF]);
    const owner = u24(entry+3), shadow = u24(entry+9);
    const run = (name: string, initialOwner: number, scmr: number, token: number, inject = -1,
                 operands?: [number,number]) => {
        // NMI's idle path still executes the real pad, clock and bus restore.
        for (let i=0; i<0x30; i++) w8(i,0);
        w8(owner,initialOwner); w8(shadow,scmr);
        w8(0x303A,initialOwner ? scmr & ~8 : scmr);
        memory.write(0x4200,0);
        cpu.E=0; cpu.P=4; cpu.DB=0x7E; cpu.D=0; cpu.S=0x1EE0;
        cpu.A=token; cpu.X=0x1234; cpu.Y=0xFEDC;
        w16(0x1EE1,0x05EF); w8(0x1EE3,0x7E);
        const product=name==='tcc__mull', angle=name==='phd_atan', division=name.endsWith('di3');
        const preserved=(product?[0,1,2,3,4,5]:division?[2,3,4,5]:[])
            .flatMap(r=>[sym(`tcc__r${r}`),sym(`tcc__r${r}h`)]);
        preserved.forEach((at,i)=>w16(at,0xA550+i));
        if (product) {
            w16(sym('tcc__r9'),operands![0]); w16(sym('tcc__r9h'),0);
            w16(sym('tcc__r10'),operands![1]); w16(sym('tcc__r10h'),0);
        }
        if (angle) operands!.forEach((v,i)=>{ w16(0x1EE4+i*4,v); w16(0x1EE6+i*4,v<0?65535:0); });
        if (division) operands!.forEach((v,i)=>{ w16(0x1EE4+i*4,v); w16(0x1EE6+i*4,v>>>16); });
        const at=sym(name); cpu.PB=at>>>16; cpu.PC=at&65535;
        let steps=0, nmis=0;
        while ((cpu.PB!==0x7E || cpu.PC!==0x05F0) && steps<20000) {
            if (steps===inject) {
                // Inject exactly one edge. Keep periodic video NMIs disabled
                // while the shared harness advances through many cases.
                cpu.setNMIEnabled(true); cpu.triggerNMI(); cpu.setNMIEnabled(false);
            }
            if ((cpu.PB<<16|cpu.PC)===(sym('NmiHandler')&65535)) nmis++;
            cpu.step(); steps++;
        }
        expect(steps,`${name} did not return`).toBeLessThan(20000);
        expect([cpu.S,cpu.DB,cpu.D,cpu.E,cpu.P&0x3C]).toEqual([0x1EE3,0x7E,0,0,4]);
        if (product) {
            const expected=Math.imul(...operands!)>>>0;
            expect([cpu.X,cpu.Y]).toEqual([expected>>>16,expected&65535]);
        } else if (!angle&&!division) expect([cpu.X,cpu.Y]).toEqual([0x1234,0xFEDC]);
        expect(preserved.map(at=>u16(at))).toEqual(preserved.map((_,i)=>0xA550+i));
        if (angle) {
            const [x,y]=operands!, ax=Math.abs(x), ay=Math.abs(y);
            let expected=ax||ay ? u16(sym('olAtanTable')+2*Math.floor(Math.min(ax,ay)*128/Math.max(ax,ay))) : 0;
            if (ay>ax) expected=0x4000-expected;
            if (y<0) expected=-expected;
            if (x<0) expected=0x8000-expected;
            expect([u16(sym('tcc__r0')),u16(sym('tcc__r1'))]).toEqual([expected&65535,0]);
        }
        if (division) {
            const unsigned=name.includes('__u'), modulo=name.includes('mod');
            const [a,b]=operands!.map(v=>unsigned?BigInt(v>>>0):BigInt(v|0));
            const expected=Number(BigInt.asUintN(32,b===0n?modulo?a:a<0n?1n:-1n:modulo?a%b:a/b));
            expect([u16(sym('tcc__r0')),u16(sym('tcc__r1'))]).toEqual([expected&65535,expected>>>16]);
        }
        expect(nmis,`${name}: NMI count at boundary ${inject}`).toBe(inject>=0 ? 1 : 0);
        const yielding=name==='olGsuYieldRam';
        const released=initialOwner!==0 && (scmr&8)!==0;
        const expectedOwner=product||angle||division ? initialOwner : yielding ? released ? 0 : initialOwner : token ? 1 : initialOwner;
        expect(memory.read(owner),'NMI changed the desired bus owner').toBe(expectedOwner);
        expect(gsu.getDiagnostics().scmr,'SCMR did not follow the desired bus owner')
            .toBe(expectedOwner ? scmr&~8 : scmr);
        if (yielding) expect(cpu.A).toBe(released ? 1 : 0);
        return steps;
    };
    return {run};
}

describe('OpenLara GSU-2: CPU/GSU RAM overlap', () => {
    it('preserves RAM ownership and the CPU ABI across every NMI interruption boundary', () => {
        const h=harness(); let cases=0;
        for (const name of ['olGsuYieldRam','olGsuReclaimRam'])
            for (const owner of [0,1]) for (const shadow of [0x1D,0x0D,0x05])
                for (const token of [0,1]) {
                    const steps=h.run(name,owner,shadow,token);
                    for (let at=0; at<steps; at++) { h.run(name,owner,shadow,token,at); cases++; }
                }
        console.log(`[openlara] GSU overlap handoffs: ${cases} real NMI interruption boundaries, all owners/masks/tokens and CPU ABI preserved`);
    });
    it('preserves dense multiply and atan results with NMI at every arithmetic and handoff boundary',()=>{
        const h=harness(); let cases=0;
        const inputs: [string,[number,number][]][]=[
            ['tcc__mull',[[33,41],[33,256],[256,33],[0x1234,0xBEEF],[65535,65535]]],
            ['phd_atan',[[0,0],[0,32767],[32767,0],[-32768,32767],[32767,-32768],[-32768,-32768],[123,-45],[-123,45]]],
        ];
        for (const [name,values] of inputs) for (const pair of values) for (const owner of [0,1]) {
            const steps=h.run(name,owner,0x1D,0,-1,pair);
            for (let at=0;at<steps;at++) { h.run(name,owner,0x1D,0,at,pair); cases++; }
        }
        console.log(`[openlara] GSU arithmetic overlap: ${cases} real NMI boundaries, exact products/angles, registers and nested ownership preserved`);
    });
    it('preserves all division returns and ownership with NMI throughout short, long and zero-divisor paths',()=>{
        const h=harness(); let cases=0;
        for (const name of ['tcc__divdi3','tcc__moddi3','tcc__udivdi3','tcc__umoddi3'])
            for (const pair of [[65535,17],[0x80000000,0xFFFFFFFF],[32,0],[1,7]] as [number,number][])
                for (const owner of [0,1]) {
                    const steps=h.run(name,owner,0x1D,0,-1,pair);
                    for (let at=0;at<steps;at++) { h.run(name,owner,0x1D,0,at,pair); cases++; }
                }
        console.log(`[openlara] GSU division overlap: ${cases} real NMI boundaries, exact signed/unsigned quotient/remainder and nested ownership preserved`);
    });
});

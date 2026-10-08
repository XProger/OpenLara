#!/usr/bin/env python3
"""A small Super FX interpreter that runs the GSU frame program and dumps the
frame buffer as a PNG.

Purpose is the same as OpenLara's __GBA_WIN__ build: get the renderer under a
debugger on a desktop instead of guessing at a console. It models the subset
of the GSU the renderer uses, including the bits that are easy to get wrong --
prefix (ALT1/ALT2/B, Sreg/Dreg) lifetime, branch delay slots, and the PLOT
address calculation for the 160-dot 16-colour bitmap mode.

Cycles are modelled too, because on this chip they are not proportional to
instructions. Every external ROM or RAM access costs five cycles at 21.4 MHz,
and that includes fetching the instruction itself: a two-byte instruction
outside the 512-byte cache costs ten cycles before it does anything. So the
interpreter carries CBR, the 32 cache-line flags and the fill cost, which is
what makes the difference between "fewer instructions" and "faster" visible.

Deliberately not modelled: ROM/RAM buffering latency and pipeline overlap, and
the two 8-pixel plot caches are modelled for cycles but not for behaviour
(PLOT is a direct read-modify-write here, which produces the same buffer
contents as the hardware's cache-then-flush path).

It reads the whole cartridge image and maps GSU ROM banks the way the chip
does -- $00-$3F as LoROM windows, $40-$5F linearly -- so the program and its
texture pages are fetched from where they will really be.

A 20-million-instruction guard catches corrupt loops. If it fires, --limit's
failure report includes the GSU registers and RAM around the live face/link
pointers instead of leaving a renderer test running indefinitely.

Usage:
    gsusim.py build/openlara.sfc --scbr 4 --fb 0x1000 -o frame.png
"""

import argparse
import os
import re
import struct
import sys
import zlib
import fx3

FRAME_WIDTH = 256
FRAME_HEIGHT = 160
FRAME_COLS = FRAME_WIDTH // 8
FRAME_ROWS = FRAME_HEIGHT // 8
FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT // 2
RAM_SIZE = 128 * 1024

# The Mode 7 build (MODE7=1): 128x64, a byte a pixel, in Mode 7's character
# layout -- 8x8 tiles of 64 chunky bytes, sixteen to a row -- written with
# STB rather than PLOT. --mode7 switches the decoder, the size and what a
# plot is; see gsu/gsu.inc.
MODE7 = False
FX3 = False


def configure_mode7():
    global MODE7, FRAME_WIDTH, FRAME_HEIGHT, FRAME_COLS, FRAME_ROWS, FRAME_SIZE
    MODE7 = True
    FRAME_WIDTH = 128
    FRAME_HEIGHT = 64
    FRAME_COLS = FRAME_WIDTH // 8
    FRAME_ROWS = FRAME_HEIGHT // 8
    FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT


def configure_fx3():
    global FX3, FRAME_WIDTH, FRAME_HEIGHT, FRAME_COLS, FRAME_ROWS, FRAME_SIZE
    FX3 = True
    FRAME_WIDTH, FRAME_HEIGHT = fx3.WIDTH, fx3.HEIGHT
    FRAME_COLS, FRAME_ROWS, FRAME_SIZE = fx3.COLS, fx3.ROWS, fx3.FRAME_SIZE

SHADE_ADDR = {0x1000: 0xEA00, 0x6000: 0xED00}
SHADE_SIZE = FRAME_COLS * FRAME_ROWS

# Book II: an external ROM/RAM transfer is six master clocks at the standard
# GSU clock and five at the fast one. CLSR is 1 here, so five.
EXT_CYCLES = 5
CACHE_SIZE = 512
CACHE_LINE = 16
CACHE_LINES = CACHE_SIZE // CACHE_LINE
GSU_HZ = 21_400_000

# Fallback CG-RAM. The real one comes from the packer via --palette; without
# it the preview is a lie, which cost a debugging detour once already.
PALETTE = [0x0000, 0x1084, 0x2108, 0x318C, 0x4210, 0x5294, 0x6318, 0x739C,
           0x001F, 0x03E0, 0x7C00, 0x03FF, 0x7C1F, 0x7FE0, 0x56B5, 0x7FFF]


def seed_shade_maps(ram):
    """What main.asm's InitShadeMaps leaves in game pak RAM at boot.

    The renderer only ever rewrites bits 2-4 of a tile map high byte; bits 0-1
    are character number bits 8-9, and the frame is 640 characters, so they
    matter. Seeding them here is what makes the interpreter's shade map
    comparable with the one the real machine uploads.
    """
    for base in SHADE_ADDR.values():
        for yc in range(FRAME_ROWS):
            for xc in range(FRAME_COLS):
                ram[base + yc * FRAME_COLS + xc] = (0x14 * xc + yc) >> 8


class Halt(Exception):
    pass


def rom_addr(bank, addr):
    """GSU view of cartridge ROM (Book II 2-3)."""
    if bank <= 0x3F:
        return ((bank << 15) | (addr & 0x7FFF))
    if 0x40 <= bank <= (0x6F if FX3 else 0x5F):
        return (((bank - 0x40) << 16) | addr)
    return None


class GSU:
    def __init__(self, program, scbr=0, r3=0, trace=False,
                 pbr=0x41, entry=0x0000):
        self.rom = bytearray(program)
        self.pbr = pbr
        self.ram = bytearray(RAM_SIZE)
        seed_shade_maps(self.ram)
        self.r = [0] * 16
        self.r[3] = r3
        self.fb = r3                # where the frame buffer is, for the plot count
        self.pc = entry
        self.pending = None          # branch target waiting on its delay slot

        self.sreg = 0
        self.dreg = 0
        self.alt1 = False
        self.alt2 = False
        self.b = False

        self.z = self.s = self.cy = self.ov = False

        self.rombr = pbr
        self.rom_buffer = 0

        self.scbr = scbr
        self.colr = 0
        self.por = 0
        self.rambr = 0
        self.last_ram_addr = 0     # register/RAM transfer address, for SBK

        self.trace = trace
        self.steps = 0
        self.plots = 0
        self.fx3_commands = []

        # --- cycles ---------------------------------------------------------
        # CBR comes up at 0000H with every cache flag clear, which is what the
        # GSU does when the CPU clears the GO flag (Book II, cache section).
        self.cycles = 0
        self.icycles = 0            # cycles charged by the instruction in flight
        self.ibytes = 0             # of which, bytes fetched from outside the cache
        self.cbr = 0
        self.cache_valid = [False] * CACHE_LINES
        self.cache_ram = bytearray(CACHE_SIZE)
        self.fetch_ext = 0          # bytes fetched from ROM
        self.fetch_hit = 0          # bytes fetched from cache
        self.cache_fills = 0
        self.cache_flushes = 0
        self.profile = None         # {pc: [instructions, cycles]} when profiling
        self.counters = None        # {pc: hits} for --count
        self.face_entries = None    # {pc} for --faces: a face starts here
        self.face_plots = []        # plots at each face start, for --faces

        # The two 8-pixel plot caches, for cycles only; the plot itself writes
        # straight through, which lands the same bytes in game pak RAM.
        self.plot_block = -1
        self.plot_pending = 0

    # -- helpers ------------------------------------------------------------

    def rom_read(self, bank, addr):
        off = rom_addr(bank, addr)
        return self.rom[off] if off is not None and off < len(self.rom) else 0

    def fetch(self):
        """One instruction byte, through the cache if the PC is inside it.

        The cache area is 512 bytes from CBR in 32 blocks of 16, each with its
        own flag. A miss loads the whole block at external speed; a hit is one
        cycle. This is the single largest term in a GSU frame, and the reason
        instruction counts are a poor proxy for time on this chip.
        """
        addr = self.pc
        self.pc = (addr + 1) & 0xFFFF

        offset = (addr - self.cbr) & 0xFFFF
        if offset < CACHE_SIZE:
            line = offset >> 4
            if not self.cache_valid[line]:
                base = (self.cbr + (line << 4)) & 0xFFFF
                for i in range(CACHE_LINE):
                    self.cache_ram[(line << 4) + i] = self.rom_read(
                        self.pbr, (base + i) & 0xFFFF)
                self.cache_valid[line] = True
                self.cache_fills += 1
                self.icycles += CACHE_LINE * EXT_CYCLES
                self.fetch_ext += 1
                self.ibytes += 1
            else:
                self.icycles += 1
                self.fetch_hit += 1
            return self.cache_ram[offset]

        self.icycles += EXT_CYCLES
        self.fetch_ext += 1
        self.ibytes += 1
        return self.rom_read(self.pbr, addr)

    def fetch16(self):
        lo = self.fetch()
        return lo | (self.fetch() << 8)

    def read_reg(self, n):
        return self.pc if n == 15 else self.r[n]

    def write_reg(self, n, v):
        v &= 0xFFFF
        if n == 15:
            self.pending = v
            return
        self.r[n] = v
        if n == 14:
            # Writing R14 kicks off a ROM fetch into the buffer that GETB and
            # GETC read, from the bank ROMBR selects.
            self.rom_buffer = self.rom_read(self.rombr, v)
            self.icycles += EXT_CYCLES

    def set_dreg(self, v):
        v &= 0xFFFF
        self.write_reg(self.dreg, v)

    def nz(self, v):
        self.z = (v & 0xFFFF) == 0
        self.s = (v & 0x8000) != 0

    def branch(self, cond, e):
        if e & 0x80:
            e -= 0x100
        if cond:
            self.pending = (self.pc + e) & 0xFFFF

    # -- plot ---------------------------------------------------------------

    def plot(self):
        """160-dot, 16-colour bitmap mode (Book II 2-8).

        CN = $14 * Xc + Yc, character is 32 bytes, planes 0/1 in the first 16
        bytes and 2/3 in the second 16, two bytes per pixel row.
        """
        x = self.r[1] & 0xFFFF
        y = self.r[2] & 0xFF
        colour = self.colr & (0xFF if FX3 else 0x0F)

        if not (self.por & 0x01) and colour == 0:
            self.r[1] = (x + 1) & 0xFFFF          # transparent: advance only
            return

        block = ((y << 5) + (x >> 3)) & 0xFFFF
        if block != self.plot_block:
            self.flush_plot()
            self.plot_block = block
        self.plot_pending |= 0x80 >> (x & 7)
        if self.plot_pending == 0xFF:
            self.flush_plot()

        if x < FRAME_WIDTH and y < FRAME_HEIGHT:
            xc = (x >> 3) & 0x1F
            yc = (y >> 3) & 0x1F
            cn = 0x14 * xc + yc
            planes = 8 if FX3 else 4
            base = (self.scbr * 0x400 + cn * planes * 8 + (y & 7) * 2) & 0x1FFFF
            bit = 7 - (x & 7)
            for plane in range(planes):
                addr = (base + (plane >> 1) * 16 + (plane & 1)) & 0x1FFFF
                if colour & (1 << plane):
                    self.ram[addr] |= 1 << bit
                else:
                    self.ram[addr] &= ~(1 << bit) & 0xFF
            self.plots += 1

        self.r[1] = (x + 1) & 0xFFFF

    def ram_addr(self, addr):
        return (((self.rambr & 1) << 16) | (addr & 0xFFFF)) & (RAM_SIZE - 1)

    def read_ram_word(self, addr):
        """Read a GSU word, whose high byte pairs backward at odd addresses."""
        self.last_ram_addr = addr & 0xFFFF
        low = self.ram_addr(addr)
        high = self.ram_addr(addr - 1 if addr & 1 else addr + 1)
        self.icycles += EXT_CYCLES
        return self.ram[low] | (self.ram[high] << 8)

    def write_ram_word(self, addr, value):
        self.last_ram_addr = addr & 0xFFFF
        low = self.ram_addr(addr)
        high = self.ram_addr(addr - 1 if addr & 1 else addr + 1)
        self.ram[low] = value & 0xFF
        self.ram[high] = (value >> 8) & 0xFF
        self.icycles += EXT_CYCLES

    def flush_plot(self):
        """Cycles for writing one 8-pixel character row back to game pak RAM.

        Four bit planes; a block that was only partly plotted has to be read
        before it can be merged, which doubles it.
        """
        if not self.plot_pending:
            return
        planes = 8 if FX3 else 4
        if self.plot_pending != 0xFF:
            self.icycles += planes * EXT_CYCLES
        self.icycles += planes * EXT_CYCLES
        self.plot_pending = 0

    def read_pixel(self):
        """Read one pixel after PLOT's direct writes (there is no cache here)."""
        self.flush_plot()           # RPIX is the barrier for pending plots
        self.icycles += 4 * EXT_CYCLES
        x = self.r[1] & 0xFF
        y = self.r[2] & 0xFF
        cn = 0x14 * (x >> 3) + (y >> 3)
        planes = 8 if FX3 else 4
        base = (self.scbr * 0x400 + cn * planes * 8 + (y & 7) * 2) & 0x1FFFF
        bit = 7 - (x & 7)
        colour = 0
        for plane in range(planes):
            addr = (base + (plane >> 1) * 16 + (plane & 1)) & 0x1FFFF
            colour |= ((self.ram[addr] >> bit) & 1) << plane
        return colour

    # -- execution ----------------------------------------------------------

    def step(self):
        pc0 = self.pc
        self.icycles = 0
        self.ibytes = 0
        op = self.fetch()
        alt1, alt2, b = self.alt1, self.alt2, self.b
        keep_prefix = False

        if self.trace:
            print(f"{pc0:04X}: {op:02X}  "
                  f"S={self.sreg} D={self.dreg} a1={int(alt1)} a2={int(alt2)}")

        s = self.read_reg(self.sreg)

        if op == 0x00:                                  # STOP
            raise Halt()
        elif op == 0x01:                                # NOP
            pass
        elif op == 0x02:                                # CACHE
            # "If CBR != (R15 & FFF0H), set CBR to the address following CACHE
            # and clear all cache flags." Re-issuing it at the same place is
            # free; issuing it somewhere else throws the whole cache away,
            # which is what makes a second CACHE inside a per-span path so
            # expensive.
            target = self.pc & 0xFFF0
            if self.cbr != target:
                self.cbr = target
                self.cache_valid = [False] * CACHE_LINES
                self.cache_flushes += 1
        elif op == 0x03:                                # LSR
            self.cy = (s & 1) != 0
            v = s >> 1
            self.set_dreg(v)
            self.nz(v)
        elif op == 0x04:                                # ROL
            v = ((s << 1) | (1 if self.cy else 0)) & 0xFFFF
            self.cy = (s & 0x8000) != 0
            self.set_dreg(v)
            self.nz(v)
        elif 0x05 <= op <= 0x0F:                        # branches
            e = self.fetch()
            cond = {
                0x05: True,
                0x06: self.s == self.ov,                # BGE (the manual's 9.17
                0x07: self.s != self.ov,                # BLT  and 9.20 swap these)
                0x08: not self.z,                       # BNE
                0x09: self.z,                           # BEQ
                0x0A: not self.s,                       # BPL
                0x0B: self.s,                           # BMI
                0x0C: not self.cy,                      # BCC
                0x0D: self.cy,                          # BCS
                0x0E: not self.ov,                      # BVC
                0x0F: self.ov,                          # BVS
            }[op]
            self.branch(cond, e)
            keep_prefix = True
        elif 0x10 <= op <= 0x1F:                        # TO Rn / MOVE Rn,Sreg
            n = op & 0x0F
            if b:
                self.write_reg(n, s)
            else:
                self.dreg = n
                keep_prefix = True
        elif 0x20 <= op <= 0x2F:                        # WITH Rn
            n = op & 0x0F
            self.sreg = self.dreg = n
            self.b = True
            keep_prefix = True
        elif 0x30 <= op <= 0x3B:                        # STW/STB (Rn)
            addr = self.r[op & 0x0F]
            self.last_ram_addr = addr & 0xFFFF
            if alt1:
                self.ram[self.ram_addr(addr)] = s & 0xFF
                self.icycles += EXT_CYCLES
                # In the Mode 7 frame a pixel is a byte store into the buffer.
                if MODE7 and self.fb <= (addr & 0xFFFF) < self.fb + FRAME_SIZE:
                    self.plots += 1
            else:
                self.write_ram_word(addr, s)
        elif op == 0x3C:                                # LOOP
            self.r[12] = (self.r[12] - 1) & 0xFFFF
            self.nz(self.r[12])
            if self.r[12] != 0:
                self.pending = self.r[13]
        elif op == 0x3D:                                # ALT1
            self.b = False
            self.alt1 = True
            keep_prefix = True
        elif op == 0x3E:                                # ALT2
            self.b = False
            self.alt2 = True
            keep_prefix = True
        elif op == 0x3F:                                # ALT3
            self.b = False
            self.alt1 = self.alt2 = True
            keep_prefix = True
        elif 0x40 <= op <= 0x4B:                        # LDW/LDB (Rn)
            addr = self.r[op & 0x0F]
            self.last_ram_addr = addr & 0xFFFF
            if alt1:
                v = self.ram[self.ram_addr(addr)]
                self.icycles += EXT_CYCLES
            else:
                v = self.read_ram_word(addr)
            self.set_dreg(v)
        elif op == 0x4C:                                # PLOT / RPIX
            if alt1:
                v = self.read_pixel()                   # direct PLOT writes mean
                self.set_dreg(v)                        # there is no cache to flush
                self.nz(v)
            else:
                self.plot()
        elif op == 0x4D:                                # SWAP
            v = ((s >> 8) | (s << 8)) & 0xFFFF
            self.set_dreg(v)
            self.nz(v)
        elif op == 0x4E:                                # COLOR / CMODE
            if alt1:
                self.por = s & 0x1F
            else:
                hi = s & 0xF0
                lo = (s >> 4) & 0x0F if (self.por & 0x04) else s & 0x0F
                if self.por & 0x08:
                    self.colr = (self.colr & 0xF0) | lo
                else:
                    self.colr = hi | lo
        elif op == 0x4F:                                # NOT
            v = (~s) & 0xFFFF
            self.set_dreg(v)
            self.nz(v)
        elif 0x50 <= op <= 0x5F:                        # ADD / ADC
            n = op & 0x0F
            rhs = n if alt2 else self.read_reg(n)
            carry = 1 if (alt1 and self.cy) else 0
            v = s + rhs + carry
            self.cy = v > 0xFFFF
            self.ov = ((s ^ v) & (rhs ^ v) & 0x8000) != 0
            self.set_dreg(v)
            self.nz(v)
        elif 0x60 <= op <= 0x6F:                        # SUB / SBC / CMP
            n = op & 0x0F
            rhs = n if (alt2 and not alt1) else self.read_reg(n)
            borrow = (0 if self.cy else 1) if (alt1 and not alt2) else 0
            v = s - rhs - borrow
            self.cy = v >= 0                            # set when no borrow
            self.ov = ((s ^ rhs) & (s ^ (v & 0xFFFF)) & 0x8000) != 0
            self.nz(v)
            if not (alt1 and alt2):                     # ALT3 = CMP, no write
                self.set_dreg(v)
        elif op == 0x70:                                # MERGE
            if FX3:
                command = self.r[0]
                if command not in range(6):
                    raise RuntimeError(f"invalid FX3 command {command} at {pc0:04x}")
                self.fx3_commands.append(command)
                # Like MesenCE/TypeSNES: planar PLOT, so C2P is a no-op.
                # This is a functional oracle, NOT a firmware timing model.
                if command >= 3:
                    for col in range((command - 3) * 9, (command - 2) * 9):
                        for row in range(fx3.ROWS):
                            base = fx3.FB_BASE + col * fx3.COL_STRIDE + row * 64
                            for plane in range(8):
                                value = 255 if fx3.CLEAR_COLOUR & (1 << plane) else 0
                                for y in range(8):
                                    self.ram[base + (plane // 2) * 16 + y * 2 + (plane & 1)] = value
            else:
                v = ((self.r[7] & 0xFF00) | ((self.r[8] & 0xFF00) >> 8)) & 0xFFFF
                self.set_dreg(v)
                self.ov = (v & 0xC0C0) != 0
                self.s = (v & 0x8080) != 0
                self.cy = (v & 0xE0E0) != 0
                self.z = (v & 0xF0F0) != 0
        elif 0x71 <= op <= 0x7F:                        # AND / BIC
            n = op & 0x0F
            rhs = n if alt2 else self.read_reg(n)
            if alt1:
                rhs = (~rhs) & 0xFFFF
            v = s & rhs
            self.set_dreg(v)
            self.nz(v)
        elif 0x80 <= op <= 0x8F:                        # MULT / UMULT
            self.icycles += 1                           # CFGR MS0 = 0
            n = op & 0x0F
            rhs = n if alt2 else self.read_reg(n)
            a = s & 0xFF
            c = rhs & 0xFF
            if not alt1:
                a = a - 256 if a & 0x80 else a
                c = c - 256 if c & 0x80 else c
            v = (a * c) & 0xFFFF
            self.set_dreg(v)
            self.nz(v)
        elif op == 0x90:                                # SBK
            self.write_ram_word(self.last_ram_addr, s)
        elif 0x91 <= op <= 0x94:                        # LINK #n
            self.r[11] = (self.pc + (op & 0x0F)) & 0xFFFF
        elif op == 0x95:                                # SEX
            v = s & 0xFF
            v = v - 256 if v & 0x80 else v
            self.set_dreg(v)
            self.nz(v)
        elif op == 0x96:                                # ASR / DIV2
            self.cy = (s & 1) != 0
            v = (s >> 1) | (s & 0x8000)
            self.set_dreg(v)
            self.nz(v)
        elif op == 0x97:                                # ROR
            v = (s >> 1) | (0x8000 if self.cy else 0)
            self.cy = (s & 1) != 0
            self.set_dreg(v)
            self.nz(v)
        elif op == 0x9F:                                # FMULT / LMULT
            self.icycles += 7                           # 3 with CFGR MS0 = 1
            a = s - 0x10000 if s & 0x8000 else s
            c = self.r[6]
            c = c - 0x10000 if c & 0x8000 else c
            prod = (a * c) & 0xFFFFFFFF
            hi = (prod >> 16) & 0xFFFF
            lo = prod & 0xFFFF
            if alt1:                                    # LMULT
                self.r[4] = lo
                self.set_dreg(hi)
            else:                                       # FMULT
                self.set_dreg(hi)
            self.cy = (prod & 0x8000) != 0
            self.nz(hi)
        elif 0x98 <= op <= 0x9D:                        # JMP Rn
            self.pending = self.r[op & 0x0F]
            keep_prefix = True
        elif op == 0x9E:                                # LOB
            v = s & 0xFF
            self.set_dreg(v)
            self.z = v == 0
            self.s = (v & 0x80) != 0
        elif 0xA0 <= op <= 0xAF:                        # IBT / LMS / SMS
            n = op & 0x0F
            imm = self.fetch()
            if alt1:
                addr = imm * 2
                self.write_reg(n, self.read_ram_word(addr))
            elif alt2:
                addr = imm * 2
                v = self.read_reg(n)
                self.write_ram_word(addr, v)
            else:
                self.write_reg(n, imm - 256 if imm & 0x80 else imm)
        elif 0xB0 <= op <= 0xBF:                        # FROM Rn / MOVES
            n = op & 0x0F
            if b:
                v = self.read_reg(n)
                self.set_dreg(v)
                self.nz(v)
                self.ov = (v & 0x0080) != 0
            else:
                self.sreg = n
                keep_prefix = True
        elif op == 0xC0:                                # HIB
            v = (s >> 8) & 0xFF
            self.set_dreg(v)
            self.z = v == 0
            self.s = (v & 0x80) != 0
        elif 0xC1 <= op <= 0xCF:                        # OR / XOR
            n = op & 0x0F
            rhs = n if alt2 else self.read_reg(n)
            v = (s ^ rhs) if alt1 else (s | rhs)
            self.set_dreg(v)
            self.nz(v)
        elif 0xD0 <= op <= 0xDE:                        # INC Rn
            n = op & 0x0F
            v = (self.read_reg(n) + 1) & 0xFFFF
            self.write_reg(n, v)
            self.nz(v)
        elif op == 0xDF:                                # GETC / RAMB / ROMB
            if alt1 and alt2:                           # ROMB
                self.rombr = s & 0xFF
            elif alt2:                                  # RAMB
                self.rambr = s & 1
            else:                                       # GETC
                self.colr = self.rom_buffer & 0xFF
        elif 0xE0 <= op <= 0xEE:                        # DEC Rn
            n = op & 0x0F
            v = (self.read_reg(n) - 1) & 0xFFFF
            self.write_reg(n, v)
            self.nz(v)
        elif op == 0xEF:                                # GETB / GETBH / BL / BS
            byte = self.rom_buffer & 0xFF
            if alt1 and alt2:                           # GETBS, sign extended
                v = byte - 256 if byte & 0x80 else byte
            elif alt1:                                  # GETBH: byte -> high
                v = (byte << 8) | (s & 0x00FF)
            elif alt2:                                  # GETBL: byte -> low
                v = (s & 0xFF00) | byte
            else:                                       # GETB
                v = byte
            self.set_dreg(v)
        elif 0xF0 <= op <= 0xFF:                        # IWT / LM / SM
            n = op & 0x0F
            imm = self.fetch16()
            if alt1:
                self.write_reg(n, self.read_ram_word(imm))
            elif alt2:
                v = self.read_reg(n)
                self.write_ram_word(imm, v)
            else:
                self.write_reg(n, imm)
        else:
            raise RuntimeError(f"unimplemented opcode ${op:02X} at ${pc0:04X}")

        if not keep_prefix:
            self.sreg = self.dreg = 0
            self.alt1 = self.alt2 = self.b = False

        cost = max(1, self.icycles)
        self.cycles += cost
        self.steps += 1
        if self.profile is not None:
            slot = self.profile.setdefault(pc0, [0, 0, 0])
            slot[0] += 1
            slot[1] += cost
            slot[2] += self.ibytes

    def face_box(self, r_romb, tv_clip, v_clip):
        """--facebox: the face at R14 in bank R_ROMB, as (clip rect, list of
        (x, y, near) per vertex), read the way DrawFace reads it."""
        ram = self.ram
        w = lambda a: ram[a] | (ram[a + 1] << 8)
        sw = lambda a: w(a) - 0x10000 if w(a) & 0x8000 else w(a)
        bank = ram[r_romb]
        a = self.r[14]
        rd = lambda o: self.rom_read(bank, (a + o) & 0xFFFF)
        count = rd(1)
        verts = []
        for i in range(count):
            tv = rd(2 + i * 4) | (rd(3 + i * 4) << 8)
            verts.append((sw(tv), sw(tv + 2), w(tv + tv_clip) & 1))
        rect = tuple(sw(v_clip + k * 2) for k in range(4))
        return rect, verts

    def run(self, limit=200_000_000):
        # A branch target takes effect after its delay slot, so the pending
        # target is applied one instruction late.
        armed = None
        counters = self.counters
        faces = self.face_entries
        facebox = getattr(self, "facebox", None)
        mesh_hooks = getattr(self, "mesh_hooks", None)
        self.mesh_kind = "?"
        try:
            while self.steps < limit:
                if counters is not None and self.pc in counters:
                    counters[self.pc] += 1
                if facebox is not None and self.pc == facebox[0]:
                    self.facebox_log.append(self.face_box(*facebox[1:]))
                if facebox is not None and self.pc == self.facebox_all:
                    self.facebox_all_log.append(self.face_box(*facebox[1:]))
                if mesh_hooks is not None and self.pc in mesh_hooks:
                    n = mesh_hooks[self.pc]
                    if n in ("TransformMesh", "TransformRoom"):
                        self.mesh_log.append({"v": 0, "f": 0, "filed": 0,
                                              "kind": self.mesh_kind})
                    elif n in ("DRM_Inst", "DRI_Item", "DMD_Node", "RoomLoad"):
                        self.mesh_kind = {"DRM_Inst": "static", "DRI_Item": "item",
                                          "DMD_Node": "node", "RoomLoad": "room"}[n]
                    elif self.mesh_log and n in ("TM_Vertex", "TM_RoomVertex",
                                                  "OTB_Face", "OTB_File"):
                        key = {"TM_Vertex": "v", "TM_RoomVertex": "v",
                               "OTB_Face": "f", "OTB_File": "filed"}[n]
                        self.mesh_log[-1][key] += 1
                if faces is not None and self.pc in faces:
                    self.face_plots.append(self.plots)
                self.pending = None
                self.step()
                taken, self.pending = self.pending, None
                if armed is not None:
                    self.pc = armed
                armed = taken
        except Halt:
            return True
        return False


# ---------------------------------------------------------------------------


def decode_frame(ram, fb_offset):
    """Turn the frame buffer back into an index bitmap: 4bpp SNES character
    data, or the Mode 7 build's chunky tiles."""
    px = [[0] * FRAME_WIDTH for _ in range(FRAME_HEIGHT)]
    if FX3:
        for y in range(FRAME_HEIGHT):
            for x in range(FRAME_WIDTH):
                base = fx3.FB_BASE + (x // 8) * fx3.COL_STRIDE + (y // 8) * 64 + (y & 7) * 2
                px[y][x] = sum(((ram[base + (p // 2) * 16 + (p & 1)] >> (7 - (x & 7))) & 1) << p
                               for p in range(8))
        return px
    if MODE7:
        # Tiles run down the columns: fb + (x >> 3) * 512 + y * 8 + (x & 7).
        for y in range(FRAME_HEIGHT):
            row = fb_offset + y * 8
            for x in range(FRAME_WIDTH):
                px[y][x] = ram[row + (x >> 3) * 512 + (x & 7)]
        return px
    for yc in range(FRAME_HEIGHT // 8):
        for xc in range(FRAME_WIDTH // 8):
            cn = 0x14 * xc + yc
            ch = fb_offset + cn * 32
            for row in range(8):
                p0 = ram[ch + row * 2]
                p1 = ram[ch + row * 2 + 1]
                p2 = ram[ch + 16 + row * 2]
                p3 = ram[ch + 16 + row * 2 + 1]
                for col in range(8):
                    bit = 7 - col
                    v = (((p0 >> bit) & 1) |
                         (((p1 >> bit) & 1) << 1) |
                         (((p2 >> bit) & 1) << 2) |
                         (((p3 >> bit) & 1) << 3))
                    px[yc * 8 + row][xc * 8 + col] = v
    return px


def write_png(path, px, palette=None, shade=None):
    """Render the frame the way the PPU would.

    A pixel is four bits of texel; which sixteen colours those four bits mean
    comes from the tile map, per 8x8 character, bits 2-4 of the high byte.
    Reading the frame buffer alone and calling it the picture would drop the
    entire shade term.
    """
    rgb = []
    for c in (palette or PALETTE):
        r = (c & 0x1F) << 3
        g = ((c >> 5) & 0x1F) << 3
        b = ((c >> 10) & 0x1F) << 3
        rgb.append((r, g, b))

    rows = len(rgb) // 16

    # The Mode 7 frame is a byte a pixel into all of CG-RAM, and is shown at
    # twice the size; the PNG is written the way the PPU would show it.
    scale = 2 if MODE7 else 1
    while (MODE7 or FX3) and len(rgb) < 256:
        rgb.append((0, 0, 0))

    raw = bytearray()
    for y, row in enumerate(px):
        line = bytearray([0])
        for x, v in enumerate(row):
            bank = 0
            if MODE7 or FX3:
                c = rgb[v]
            else:
                if shade and rows > 1:
                    entry = shade[(y >> 3) * FRAME_COLS + (x >> 3)]
                    bank = ((entry >> 2) & 0x07) % rows
                c = rgb[bank * 16 + (v & 0x0F)]
            line += bytes(c) * scale
        for _ in range(scale):
            raw += line

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    png = (b"\x89PNG\r\n\x1a\n" +
           chunk(b"IHDR", struct.pack(">IIBBBBB",
                                      len(px[0]) * scale, len(px) * scale,
                                      8, 2, 0, 0, 0)) +
           chunk(b"IDAT", zlib.compress(bytes(raw), 9)) +
           chunk(b"IEND", b""))
    open(path, "wb").write(png)


def read_symbols(path):
    """Labels from a NO$SNES symbol file, bank 00 only -- the GSU blob."""
    syms = []
    for line in open(path):
        parts = line.split()
        if len(parts) == 2 and ":" in parts[0]:
            bank, addr = parts[0].split(":")
            # Macro-generated return labels would otherwise split a routine's
            # cycles across a dozen meaningless names.
            if bank == "00" and not parts[1].startswith("_"):
                syms.append((int(addr, 16), parts[1]))
    syms.sort()
    return syms


def report_profile(gsu, symfile, top):
    """Where the cycles went, by routine, and what a cache window would hold.

    Cycles rather than instructions, because the two rank differently: a
    routine outside the cache pays five cycles per byte just to be read.
    """
    import bisect

    syms = read_symbols(symfile)
    if not syms:
        print("  (no bank 00 symbols)")
        return
    addrs = [a for a, _ in syms]

    per_sym = {}
    for pc, (count, cycles, extbytes) in gsu.profile.items():
        i = bisect.bisect_right(addrs, pc) - 1
        name = syms[i][1] if i >= 0 else "?"
        slot = per_sym.setdefault(name, [0, 0, 0])
        slot[0] += count
        slot[1] += cycles
        slot[2] += extbytes

    total = gsu.cycles
    print(f"\n  {'cycles':>10} {'':>6} {'instr':>9} {'fetch':>9}  routine")
    for name, (count, cycles, ext) in sorted(per_sym.items(),
                                             key=lambda kv: -kv[1][1])[:top]:
        print(f"  {cycles:>10} {100 * cycles / total:5.1f}% {count:>9} "
              f"{ext * EXT_CYCLES:>9}  {name}")

    # Where to put the one window a single CACHE buys. The figure that matters
    # is not cycles contained but cycles *recoverable*: an uncached byte costs
    # EXT_CYCLES and a cached one costs 1, so a window is worth
    # (EXT_CYCLES - 1) per uncached byte fetched inside it, less the fills.
    by_pc = sorted(gsu.profile.items())
    pcs = [pc for pc, _ in by_pc]
    best = (0, 0)
    for i, start in enumerate(pcs):
        j = bisect.bisect_left(pcs, start + CACHE_SIZE)
        saved = sum(by_pc[k][1][2] for k in range(i, j)) * (EXT_CYCLES - 1)
        if saved > best[0]:
            best = (saved, start)
    saved, start = best
    i = bisect.bisect_right(addrs, start) - 1
    name = syms[i][1] if i >= 0 else "?"
    print(f"\n  best {CACHE_SIZE}-byte window starts at ${start:04X} ({name}): "
          f"would recover about {saved} cycles, {100 * saved / total:.0f}% of "
          f"the frame")


# Must match gsu/gsu.inc.
GS_PROBE = 0xF700
GS_ITEM = 0xF800
GS_LEVEL_READY = 0xF314
GS_ITEM_SIZE = 12
GS_ITEM_MAX = 128


def level_table(rom_path):
    """FX3: the cartridge's level table (tools/mkbanks.py's leveltable.inc
    beside the ROM), as [(host tables' file offset, LEVEL.BIN's GSU bank,
    GSU entry)] -- OlLevelDesc's first three words, 32 words an entry."""
    try:
        with open(os.path.join(os.path.dirname(os.path.abspath(rom_path)), "leveltable.inc")) as f:
            words = [int(w, 16) for w in re.findall(r"\$([0-9A-Fa-f]{4})", f.read())]
    except OSError:
        return []
    return [((words[i] - 0xC0) << 16, words[i + 1] - 0x80, words[i + 2])
            for i in range(0, len(words) - 31, 32)]


def seed_items(gsu, tick, level_bank, lara_pitch=0, host_offset=None):
    """Fill GS_ITEM from the level's own entity list.

    The renderer reads where an entity is from the host, so an interpreter
    with no host has to supply it -- and the level's placement list is exactly
    what the host starts from, so this reads the same table the GSU stages for
    it. Positions come across unchanged; every entity is held on animation 0
    at one frame, which is what src/game.c's ol_animFreeze does at the other
    end.
    """
    def rom16(a):
        return (gsu.rom_read(level_bank, a)
                | (gsu.rom_read(level_bank, a + 1) << 8))

    if FX3:
        import hosttables
        start = hosttables.ROM_OFFSET if host_offset is None else host_offset
        size = int.from_bytes(gsu.rom[start + 8:start + 12], "little")
        directory = hosttables.parse(gsu.rom[start:start + size])
        items = directory["counts"]["items"]
        io = start + directory["tables"]["ITEM"]["offset"]
        # This is a host-side fixture, not an extension of the FX ROM map.
        read = lambda a: gsu.rom[a]
        gsu.ram[GS_LEVEL_READY] = 1  # FX3 has no GSU-2 SRAM staging job
    else:
        host = rom16(2)
        items = rom16(host + 2)
        io = host + rom16(host + 20)
        read = lambda a: gsu.rom_read(level_bank, a)
    # Where the entity list is, from the header rather than added up here:
    # this offset was recomputed in three places and every table the packer
    # gained broke the two that were not being edited.

    for i in range(min(items, GS_ITEM_MAX)):
        o = io + i * 16
        p = GS_ITEM + i * GS_ITEM_SIZE
        # TR's hidden bit, in ITEM_FLAG_ONCE's slot: the host publishes such
        # an entity as animation $FF (gameRenderSync) -- the dart and
        # effect pools, until they are used.
        gsu.ram[p] = 0xFF if read(o + 3) & 0x20 else 0   # animation
        gsu.ram[p + 1] = tick & 0xFF        # frame
        for k in range(8):                  # x, y, z, angle
            gsu.ram[p + 2 + k] = read(o + 4 + k)
        gsu.ram[p + 10] = read(o + 1)       # the room it stands in
        gsu.ram[p + 11] = (lara_pitch >> 8) & 0xFF if i == 0 else 0   # pitch: hers only

    # Entity 0 is Lara. She used to be hidden -- the camera was at her eye, so
    # drawing her put nothing on screen -- and is not, now that the camera
    # follows her from behind.


# FX3's GS_KIT as Weapon_init publishes it (weapon.c Weapon_publish): her
# thighs in the pistols' holsters, the shotgun on her back, her own hands and
# head, no shotgun torso turn, the pistols' flash 155 down the hand.
GS_KIT = 0xF648
KIT_BOOT = {1: 5, 4: 6, 7: 18, 10: 3, 13: 4, 14: 21}


def seed_kit(gsu):
    for i in range(24):
        gsu.ram[GS_KIT + i] = 0
    for node, mesh in KIT_BOOT.items():
        gsu.ram[GS_KIT + node] = mesh
    gsu.ram[GS_KIT + 22] = 155


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom", help="cartridge image (.sfc)")
    ap.add_argument("--pbr", type=lambda v: int(v, 0), default=0x41,
                    help="GSU program bank")
    ap.add_argument("--entry", type=lambda v: int(v, 0), default=None,
                    help="GSU entry address within that bank ($0000; FX3 $0100, or "
                         "$8100 for a multi-level cartridge's second program)")
    ap.add_argument("--level", type=int, default=0,
                    help="FX3: which of the cartridge's levels (its level table, "
                         "leveltable.inc beside the ROM): its program, banks and host tables")
    ap.add_argument("--scbr", type=lambda v: int(v, 0), default=0,
                    help="SCBR the host would have written")
    ap.add_argument("--fb", type=lambda v: int(v, 0), default=0,
                    help="frame buffer offset handed over in R3")
    ap.add_argument("--angle", type=lambda v: int(v, 0), default=0,
                    help="rotation angle the host would have left in R8")
    ap.add_argument("--probe-x", type=lambda v: int(v, 0), default=None,
                    help="where the host would have asked for a floor; "
                         "defaults to the camera, which is what it was before "
                         "the camera followed Lara rather than being her")
    ap.add_argument("--probe-y", type=lambda v: int(v, 0), default=None)
    ap.add_argument("--probe-z", type=lambda v: int(v, 0), default=None)
    ap.add_argument("--level-bank", type=lambda v: int(v, 0), default=None,
                    help="GSU ROM bank the level image starts in; defaults to "
                         "LEVEL_BANK_BASE in the texbank.inc beside the ROM "
                         "(mkbanks.py), else $43")
    ap.add_argument("--tick", type=lambda v: int(v, 0), default=0,
                    help="animation phase the host would have staged for "
                         "every entity")
    ap.add_argument("--limit", type=lambda v: int(v, 0), default=20_000_000,
                    help="stop after this many instructions and dump GSU "
                         "state instead of waiting on a hung renderer")
    ap.add_argument("--room", type=lambda v: int(v, 0), default=0,
                    help="room index the host would have left in R6")
    ap.add_argument("--cam-x", type=lambda v: int(v, 0), default=0,
                    help="camera X the host would have left in R7")
    ap.add_argument("--cam-y", type=lambda v: int(v, 0), default=0,
                    help="camera Y the host would have left in R10")
    ap.add_argument("--cam-ty", type=lambda v: int(v, 0), default=None,
                    help="the height the view looks at, the host's R11: the "
                         "frame shears its centre line to it (default: the "
                         "camera's own Y, no shear)")
    ap.add_argument("--lara-pitch", type=lambda v: int(v, 0), default=0,
                    help="her angle.x (GS_ITEM byte 11 takes the top byte)")
    ap.add_argument("--cam-z", type=lambda v: int(v, 0), default=0,
                    help="camera Z the host would have left in R9")
    ap.add_argument("-o", "--out", default="frame.png")
    ap.add_argument("--raw", help="also write the raw 4bpp frame-buffer bytes")
    ap.add_argument("--raw-shade",
                    help="also write the shade map, i.e. the tile map high bytes")
    ap.add_argument("--palette", help="PALETTE.BIN from the packer")
    ap.add_argument("--stats", metavar="FILE",
                    help="write the modelled cycle count and the frame's CRC "
                         "as key=value lines, for tools/framecheck.py")
    ap.add_argument("--profile", metavar="SYMFILE",
                    help="attribute cycles to routines, using the .sym file "
                         "`wlalink -s` writes next to gsu.bin")
    ap.add_argument("--top", type=int, default=16,
                    help="how many routines --profile prints")
    ap.add_argument("--count", metavar="LABELS",
                    help="comma-separated labels (from --profile's symbol "
                         "file) whose entries to count: loop iterations")
    ap.add_argument("--facebox", action="store_true",
                    help="classify every face DrawFace sees by its screen "
                         "box against the clip rectangle (an experiment)")
    ap.add_argument("--meshes", action="store_true",
                    help="per TransformMesh/TransformRoom call: vertices "
                         "transformed, faces visited and filed, to find the "
                         "meshes that draw nothing")
    ap.add_argument("--faces", action="store_true",
                    help="histogram of pixels per rasterized face (a face "
                         "starts at RasterF or RasterFT)")
    ap.add_argument("--cold", action="store_true",
                    help="model the first frame after power-on, which stages "
                         "the host's level blob into RAM bank 1 (2.8 M cycles); "
                         "the default is every frame after it, which the host "
                         "has told the GSU not to stage again")
    ap.add_argument("--trace", action="store_true")
    ap.add_argument("--mode7", action="store_true",
                    help="the MODE7=1 build: a 128x64 chunky frame in Mode 7's "
                         "character layout, written with STB")
    ap.add_argument("--ram", metavar="FILE",
                    help="game pak RAM as a captured run left it (bank $70, then $71), "
                         "over everything seeded: a frame replayed as the cartridge ran it")
    ap.add_argument("--fx3", action="store_true",
                    help="FX3 v1.0 216x144 8bpp framebuffer (functional, not hardware timing)")
    args = ap.parse_args()
    if args.fx3 and args.mode7:
        ap.error("--fx3 and --mode7 are mutually exclusive")
    levels = []
    if args.fx3:
        configure_fx3()
        args.scbr, args.fb = 0x40, 0x10000
        levels = level_table(args.rom)
        if levels and not 0 <= args.level < len(levels):
            ap.error(f"--level {args.level}: the cartridge carries {len(levels)}")
        if args.entry is None:
            args.entry = levels[args.level][2] if levels else 0x100
    if args.mode7:
        configure_mode7()

    if args.entry is None:
        args.entry = 0x0000
    gsu = GSU(open(args.rom, "rb").read(), scbr=args.scbr, r3=args.fb,
              trace=args.trace, pbr=args.pbr, entry=args.entry)
    if args.profile:
        gsu.profile = {}
    label_addr = {}
    if args.profile and (args.count or args.faces or args.facebox or args.meshes):
        for a, n in read_symbols(args.profile):
            label_addr.setdefault(n, a)
    if args.count:
        gsu.counters = {}
        for n in args.count.split(","):
            if n not in label_addr:
                sys.exit(f"--count: no symbol {n}")
            gsu.counters[label_addr[n]] = 0
    if args.faces:
        gsu.face_entries = {label_addr[n] for n in ("RasterF", "RasterFT")
                            if n in label_addr}
    if args.meshes:
        gsu.mesh_hooks = {label_addr[n]: n for n in
                          ("TransformMesh", "TransformRoom", "TM_Vertex",
                           "TM_RoomVertex",
                           "OTB_Face", "OTB_File",
                           "DrawRoomMeshes", "DRM_Inst", "DRI_Item", "DMD_Node",
                           "RoomLoad") if n in label_addr}
        gsu.mesh_log = []
    if args.facebox:
        import re as _re
        defs = {}
        for f in ("gsu/gsu.inc", "gsu/mesh.gsu"):
            for line in open(f):
                m = _re.match(r"\.DEFINE\s+(\w+)\s+(?:\$([0-9A-Fa-f]+)|(\d+))", line)
                if m:
                    defs.setdefault(m.group(1), int(m.group(2), 16) if m.group(2)
                                                    else int(m.group(3)))
        gsu.facebox = (label_addr["DrawFace"], defs["R_ROMB"],
                       defs["TV_CLIP"], defs["V_CLIPX0"])
        gsu.facebox_log = []
        # and every face the ordering pass sees, before any rejection
        gsu.facebox_all = label_addr["OTB_Face"]
        gsu.facebox_all_log = []
    # The entity state the host writes before every run. There is no host
    # here, so the positions come from the level's own placement records --
    # which is what the host starts from too -- and every entity is held on
    # animation 0 at the same frame, as ol_animFreeze does at the other end.
    # There is no host here, so the probe point is given rather than derived.
    px = args.cam_x if args.probe_x is None else args.probe_x
    py = args.cam_y if args.probe_y is None else args.probe_y
    pz = args.cam_z if args.probe_z is None else args.probe_z
    for k, v in enumerate((px, py, pz)):
        gsu.ram[GS_PROBE + k * 2] = v & 0xFF
        gsu.ram[GS_PROBE + k * 2 + 1] = (v >> 8) & 0xFF

    level_bank = args.level_bank
    if level_bank is None and levels:
        level_bank = levels[args.level][1]
    if level_bank is None:
        level_bank = 0x43
        try:
            with open(os.path.join(os.path.dirname(os.path.abspath(args.rom)), "texbank.inc")) as f:
                m = re.search(r"LEVEL_BANK_BASE \$([0-9A-Fa-f]+)", f.read())
            if m:
                level_bank = int(m.group(1), 16)
        except OSError:
            pass
    seed_items(gsu, args.tick, level_bank, args.lara_pitch,
               levels[args.level][0] if levels else None)
    if args.fx3:
        seed_kit(gsu)

    # The host clears GS_LEVEL_READY once at boot and the GSU sets it after
    # the copy, so every frame but the first skips StageItemList. A model of
    # the frame rate is a model of those frames; the first one is --cold.
    if args.fx3 or not args.cold:
        gsu.ram[GS_LEVEL_READY] = 1
    # FX3's projection table ($3200, mesh.gsu FrameBody) is rewritten only
    # when the kind of frame changes; every game frame after the first finds
    # it holding RECIP_TAB's values under the PROJ_GAME tag.
    if args.fx3 and not args.cold:
        for i in range(256):
            v = 0x7FFF if i < 2 else min(65536 // i, 0x7FFF)     # tools/mkdivtable.py
            gsu.ram[0x3200 + i * 2] = v & 0xFF
            gsu.ram[0x3200 + i * 2 + 1] = v >> 8
        gsu.ram[0xF638] = 0x01                  # PROJ_MODE = PROJ_GAME, $A501
        gsu.ram[0xF639] = 0xA5

    if args.ram:
        image = open(args.ram, "rb").read()
        gsu.ram[:min(len(image), len(gsu.ram))] = image[:len(gsu.ram)]

    gsu.r[6] = args.room & 0xFFFF
    gsu.r[7] = args.cam_x & 0xFFFF
    gsu.r[8] = args.angle & 0xFFFF
    gsu.r[9] = args.cam_z & 0xFFFF
    gsu.r[10] = args.cam_y & 0xFFFF
    gsu.r[11] = (args.cam_y if args.cam_ty is None else args.cam_ty) & 0xFFFF
    if not gsu.run(limit=args.limit):
        regs = " ".join(f"R{i}={v:04X}" for i, v in enumerate(gsu.r))
        state = [f"GSU did not reach STOP within {args.limit} instructions; "
                 f"PC={gsu.pc:04X} CBR={gsu.cbr:04X} RAMBR={gsu.rambr} "
                 f"ROMBR={gsu.rombr:02X}", f"  {regs}"]
        for n in (2, 8, 9, 10):
            addr = gsu.ram_addr(gsu.r[n])
            base = max(0, min(len(gsu.ram) - 32, addr - 8))
            data = " ".join(f"{v:02X}" for v in gsu.ram[base:base + 32])
            state.append(f"  RAM around R{n} (${addr:05X}): "
                         f"${base:05X}: {data}")
        sys.exit("\n".join(state))

    print(f"STOP after {gsu.steps} instructions, {gsu.plots} pixels plotted")
    fetched = gsu.fetch_ext + gsu.fetch_hit
    if FX3:
        if gsu.fx3_commands != [3, 4, 5, 0, 1, 2]:
            # a replayed frame (--ram) starts wherever the run was: an
            # inventory frame, say, which never sets up the game's view
            if not args.ram:
                sys.exit(f"bad FX3 frame command sequence: {gsu.fx3_commands}")
            print(f"  FX3 frame commands: {gsu.fx3_commands}")
        print(f"  {gsu.cycles} reference GSU cycles; FX3 hardware timing is not modelled")
    else:
        print(f"  {gsu.cycles} cycles = {gsu.cycles / GSU_HZ * 1000:.0f} ms "
              f"= {GSU_HZ / gsu.cycles:.2f} fps if the GSU never waited")
    print(f"  fetch: {gsu.fetch_hit}/{fetched} bytes from cache "
          f"({100 * gsu.fetch_hit / max(1, fetched):.0f}%), "
          f"{gsu.cache_fills} line fills, {gsu.cache_flushes} flushes")

    if gsu.profile is not None:
        report_profile(gsu, args.profile, args.top)
    if gsu.counters is not None:
        names = {a: n for n, a in label_addr.items()}
        print("\n  entries:")
        for a, c in sorted(gsu.counters.items(), key=lambda kv: -kv[1]):
            print(f"  {c:>8}  {names[a]}")
    if getattr(gsu, "facebox", None) is not None:
        cats = {}
        for rect, verts in gsu.facebox_log:
            x0, x1, y0, y1 = rect
            if not verts:
                cat = "unreadable (stale symbols?)"
            elif any(n for _, _, n in verts):
                cat = "near (clipper)"
            else:
                xs = [x for x, _, _ in verts]
                ys = [y for _, y, _ in verts]
                if max(xs) < x0:      cat = "all left of clip"
                elif min(xs) > x1:    cat = "all right of clip"
                elif max(ys) < y0:    cat = "all above clip"
                elif min(ys) > y1:    cat = "all below clip"
                elif max(ys) == min(ys): cat = "zero height"
                elif max(xs) == min(xs): cat = "zero width"
                elif max(ys) - min(ys) <= 1 or max(xs) - min(xs) <= 1: cat = "one pixel wide or high"
                else:                 cat = "overlaps clip"
            cats[cat] = cats.get(cat, 0) + 1
        # The faces the flags reject that lie within two pixels of the
        # rectangle: where a rasterizer's rounding could still have drawn.
        edge = []
        for rect, verts in gsu.facebox_all_log:
            x0, x1, y0, y1 = rect
            if not verts or any(n for _, _, n in verts):
                continue
            xs = [x for x, _, _ in verts]
            ys = [y for _, y, _ in verts]
            side = None
            if max(xs) < x0:   side = "left"
            elif min(xs) >= x1: side = "right"
            elif max(ys) < y0: side = "above"
            elif min(ys) >= y1: side = "below"
            if side:
                edge.append((side, (min(xs), max(xs), min(ys), max(ys)), rect))
        if edge:
            print(f"\n  faces the flags reject, by side, with their extents "
                  f"(x0 x1 y0 y1) and the rectangle:")
            for side, ext, rect in sorted(edge):
                print(f"    {side:6s} {ext} in {rect}")
        print(f"\n  faces DrawFace saw: {len(gsu.facebox_log)}, clip rects "
              f"{sorted(set(r for r, _ in gsu.facebox_log))}")
        for c, n in sorted(cats.items(), key=lambda kv: -kv[1]):
            print(f"  {n:>5}  {c}")
    if getattr(gsu, "mesh_hooks", None) is not None:
        log = gsu.mesh_log
        print(f"\n  meshes transformed: {len(log)}")
        for kind in ("room", "static", "node", "?"):
            ms = [m for m in log if m["kind"] == kind]
            if not ms:
                continue
            empty = [m for m in ms if m["filed"] == 0]
            print(f"    {kind:7s} {len(ms):>4} meshes, {sum(m['v'] for m in ms):>5} vertices, "
                  f"{sum(m['f'] for m in ms):>5} faces visited, {sum(m['filed'] for m in ms):>4} filed; "
                  f"{len(empty)} filed nothing ({sum(m['v'] for m in empty)} vertices, "
                  f"{sum(m['f'] for m in empty)} faces wasted)")
    if gsu.face_entries is not None:
        starts = gsu.face_plots + [gsu.plots]
        sizes = [starts[i + 1] - starts[i] for i in range(len(starts) - 1)]
        bins = [(0, 0), (1, 1), (2, 3), (4, 7), (8, 15), (16, 31), (32, 63),
                (64, 127), (128, 1 << 30)]
        print(f"\n  faces rasterized: {len(sizes)}, pixels {sum(sizes)}")
        for lo, hi in bins:
            n = sum(1 for v in sizes if lo <= v <= hi)
            px = sum(v for v in sizes if lo <= v <= hi)
            if n:
                print(f"  {lo:>4}-{min(hi, 9999):<4} px: {n:>5} faces, {px:>6} pixels")
    pal = None
    if args.palette:
        raw = open(args.palette, "rb").read()
        pal = [raw[i * 2] | (raw[i * 2 + 1] << 8) for i in range(len(raw) // 2)]

    shade = None
    if MODE7 or FX3:
        # A byte a pixel: the shade, when there is one, is in the pixel.
        print("8bpp: no shade map")
    else:
        shade_base = SHADE_ADDR.get(args.fb, SHADE_ADDR[0x1000])
        shade = gsu.ram[shade_base:shade_base + SHADE_SIZE]
        used = sorted({(b >> 2) & 7 for b in shade})
        print(f"shade rows in use: {used}")

    write_png(args.out, decode_frame(gsu.ram, args.fb), pal, shade)
    print(f"wrote {args.out}")
    if args.raw:
        with open(args.raw, "wb") as f:
            f.write(fx3.packed_frame(gsu.ram) if FX3 else gsu.ram[args.fb:args.fb + FRAME_SIZE])
        print(f"wrote {args.raw}")
    if args.raw_shade:
        # The runner compares the shade map only when the file has bytes.
        with open(args.raw_shade, "wb") as f:
            f.write(shade if shade is not None else b"")
        print(f"wrote {args.raw_shade}")
    if args.stats:
        # What `make check` holds against the selected frame*.baseline: the price of the
        # frame, and a CRC of the picture so a change in one is told from a
        # change in the other. The CRC is over the same bytes the emulator
        # uploads, so it matches the runner's frameCRC when the two agree.
        raw = fx3.packed_frame(gsu.ram) if FX3 else bytes(gsu.ram[args.fb:args.fb + FRAME_SIZE])
        with open(args.stats, "w") as f:
            if FX3:
                f.write("target=fx3\n")
            f.write(f"cycles={gsu.cycles}\n")
            f.write(f"instructions={gsu.steps}\n")
            f.write(f"plots={gsu.plots}\n")
            f.write(f"cache_hit={100 * gsu.fetch_hit / max(1, fetched):.1f}\n")
            f.write(f"crc32={zlib.crc32(raw) & 0xFFFFFFFF:08x}\n")
        print(f"wrote {args.stats}")


if __name__ == "__main__":
    main()

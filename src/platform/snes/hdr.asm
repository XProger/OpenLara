; OpenLara - SNES / Super FX target
; Shared memory map. Included by main.asm, by the tcc runtime, and by every
; 816-tcc output file (the compiler emits `.include "hdr.asm"` itself).
;
; Slot numbering is not free: 816-tcc hard codes `.RAMSECTION ".bss" BANK $7e
; SLOT 2`, so slots 1-3 keep the meanings PVSnesLib gave them. Slots 4 to 9
; are this target's own.
;
; The unusual one is slot 4. On an ordinary SNES, compiled code lives in ROM.
; Here it cannot: while the GSU owns the ROM bus the 65816 reads dummy data
; from cartridge ROM, so anything the host executes -- or reads as constant
; data -- has to be resident in WRAM before the GSU starts.
; That applies to GSU-2, not FX3: its CPU and FX can access ROM/SRAM
; simultaneously, so the engine uses FastROM slot 0 on that variant.

.INCLUDE "config.inc"
.INCLUDE "src/host_tables.inc"

.MEMORYMAP
    DEFAULTSLOT 0
    SLOTSIZE $8000
    SLOT 0 $8000                ; ROM, the LoROM window in banks $00-$3F
    SLOT 1 $0000 $2000          ; low RAM: direct page, and 816-tcc expects it
    SLOT 2 $2000 $3800          ; WRAM bank $7E data; 816-tcc puts .bss here.
                                ; Ends at $57FF: a code image sits above it
                                ; (slot 7), and this is what makes a .bss
                                ; that grows into it a link error
    SLOT 3 $0000 $10000         ; global RAM
    SLOT 4 $0000 $8000          ; WRAM bank $7F: resident C code and rodata
    SLOT 5 $0100 $8000          ; WRAM-resident host assembly, runs at $00:0100
    SLOT 6 $8000 $8000          ; the upper half of a WRAM bank: $7F:8000 or
                                ; $7E:8000, the unit's .BASE says which
    SLOT 7 $5800 $8000          ; the fourth C image, $7E:5800-$7FFF, over
                                ; the .bss; ROM bank $3E. A slot cannot be
                                ; narrower than its ROM bank, so the 10 KB
                                ; limit is tools/budget.py's to enforce
    SLOT 8 $0600 $8000          ; the fifth, $7E:0600-$1BFF, under the stack,
                                ; which keeps $1C00-$1FFF; ROM bank $3D.
                                ; The $1600-byte limit is budget.py's as well
    SLOT 9 $5800 $A800          ; FX3-only per-creature data. Engine code is
                                ; in FastROM, so this region is free WRAM.
.ENDME

.ROMBANKSIZE $8000
.IFDEF FX3
.ROMBANKS 128                   ; FX3 CPU can read 4 MB; FX can read 3 MB
.ELSE
.ROMBANKS 64                    ; 2 MB / 16 Mbit -- the GSU-2 ceiling
.ENDIF

; OpenLara - SNES / Super FX target
; Room_query, in 65816 assembly.
;
; src/room.c is upstream's room.h, and the point resolution in it --
; Room_sector, the portal walks, the slants -- was the C that cost the most
; per line of anything on the host: 2,400 cycles a call, seven or eight
; calls a tick, a fifth of the CPU with the wolves awake. The algorithm is a
; dozen byte reads; the compiler spent the cycles on 32-bit pointer
; arithmetic to make each one, and on a library multiply for the sector
; index. This is the same algorithm, statement for statement, with the reads
; as long SRAM loads (GSU-2) or banked pointers (FX3), the hardware multiply, and
; state in the direct page. room.c keeps the triggers, which are not hot;
; room.h keeps the interface, which is unchanged: 816-tcc calls Room_query
; here exactly as it called the C.
;
; The calling convention is 816-tcc's. Arguments are pushed right to left as
; 16-bit words (a pointer is two of them, offset then bank), the call is a
; `jsr.l`, the result goes in tcc__r0 and the caller pops. A, X and Y are 16
; bits on both sides of the call; the data bank is $7E, which is where the C
; globals read below live; the direct page is $00. tcc__r0-r10 are scratch
; across a call and are used as such.
;
; The state that survives between calls is the C's two caches, in the direct
; page rather than .bss because a query touches every word of them and a
; direct-page operand is a cycle cheaper and a byte shorter than an absolute
; one. The cache occupies $60-$BD; FX3's table pointers use $C0-$F7.
; Nothing here has an initialiser -- Room_reset is
; the initialiser, at level load, as it was for the C.
;
; ROM bank 4: GSU-2 copies this to $7F:0000; FX3 executes from $84:8000.
; Both share room.c's region. .BASE must match tools/tccfix.py for that bank.

.INCLUDE "snes.inc"
.INCLUDE "hdr.asm"
.IFNDEF C_MOTION
.DEFINE RQ_WORLD_CACHE 1
.ENDIF

.DEFINE ROOM_NONE   $00FF       ; a sector's "no room that way"
.DEFINE WALL_H      $8100       ; -127 * 256: the floor byte that means no floor
.DEFINE NO_FLOOR    32767       ; OL_NO_FLOOR, src/fixed.h

.RAMSECTION ".roomq" BANK 0 SLOT 1 ORGA $60 FORCE PRIORITY 1000
; Room_sector's record of the room it last answered for: a collision probe
; asks about four points in one room and pays for the record once.
rq_rsRoom       dw
rq_rsX          dw
rq_rsZ          dw
rq_rsXS         dw
rq_rsZS         dw
rq_rsOff        dw
rq_rsSX         dw      ; the last answer's sector coordinates, $FFFF for none
rq_rsSZ         dw
rq_rsSector     dw      ; and the answer, so the same sector costs no multiply
; Room_query's record of the last resolution. Where a point ends up -- which
; room, which sector under and over it, which trigger -- depends only on the
; world sector it is in and the 256-unit band its Y is in: the vertical walks
; compare Y against floor and ceiling bytes, never against slants. So a
; second point in the same sector and band resolves identically, and only
; the slant arithmetic is repeated.
rq_qcHint       dw
rq_qcSX         dw
rq_qcSZ         dw
rq_qcY          dw      ; y & $FF00: the band
rq_qcRoom       dw
rq_qcTrigger    dw
rq_qcRX         dw
rq_qcRZ         dw
rq_qcFSX        dw
rq_qcFSZ        dw
rq_qcFloor      dw
rq_qcCeil       dw
rq_qcFsx        dw
rq_qcFsz        dw
rq_qcCsx        dw
rq_qcCsz        dw
; one call's temporaries
rq_hint         dw
rq_x            dw
rq_y            dw
rq_z            dw
rq_sector       dw
rq_lower        dw
rq_upper        dw
rq_next         dw
rq_n            dw
rq_sx           dw
rq_sz           dw
rq_dx           dw
rq_dz           dw
rq_d            dw
rq_a            dw
rq_b            dw
rq_tmp          dw
rq_floor        dw
rq_ceil         dw
; olStaticHit's
rq_dy           dw
rq_i            dw
rq_end          dw
.ENDS

.BASE $7B
.BANK 4 SLOT 4
.IFDEF FX3
.BASE $80
.BANK 4 SLOT 0
.ENDIF
.SECTION "roomq" SEMIFREE

.accu 16
.index 16
.16bit

; X <- the byte offset of host sector A's four-byte record:
;   int8 floor, int8 ceiling, uint16 link
; A 16-bit load at +0 reads both heights at once.
.MACRO SECTOR_X
    asl a
    asl a
    clc
    HOST_ADD_BASE olSectorBase
    tax
.ENDM

; X <- the byte offset of the link record the sector at X names:
;   uint8 roomBelow, roomAbove, portal, trigger
;   uint16 floorSlants, ceilingSlants
; Link 0 is the record that links nowhere and is flat, so there is no
; "none" to test for. See level.h.
.MACRO LINK_X
    HOST_LDA olSectorData, 2
    asl a
    asl a
    asl a
    clc
    HOST_ADD_BASE olLinkBase
    tax
.ENDM

; ---------------------------------------------------------------------------
; void Room_reset(void)
;
; Forget both records; at level load, when the blob's tables move.
; ---------------------------------------------------------------------------
Room_reset:
    lda #$FFFF
    sta.b rq_rsRoom
    sta.b rq_qcHint
.IFNDEF C_MOTION
    jsl RQ_CacheReset
.IFNDEF FX3
    jsl navInvalidateSectorsGsu2
.ENDIF
.ENDIF
    rtl

.IFNDEF C_MOTION
.IFDEF FX3
; A dry room with no room below has no water column. Preserve the complete
; C walk for water/stacked rooms and arbitrary output aliases. The common
; caller's outputs are two words in its own WRAM stack locals.
Room_water:
    lda 12,s
    beq +                       ; the compiler addresses stack locals in bank 0
    cmp #$007E
    bne RW_Cold
+   lda 16,s
    beq +
    cmp #$007E
    bne RW_Cold
+   tsc
    clc
    adc #18
    sta.b tcc__r0
    lda 10,s
    cmp.b tcc__r0
    bcc RW_Cold
    cmp #$1FFF
    bcs RW_Cold
    lda 14,s
    cmp.b tcc__r0
    bcc RW_Cold
    cmp #$1FFF
    bcs RW_Cold
    lda 4,s
    cmp.w olRoomCount
    bcs RW_Dry
    asl a
    sta.b tcc__r0
    asl a
    asl a
    clc
    adc.b tcc__r0
    clc
    adc #8
    tay
    lda [olRoomData],y
    bit #$0080
    bne RW_Cold
    bit #$007F
    beq RW_Dry
    bit #$FF00
    beq RW_Dry
    lda 8,s
    pha
    lda 8,s
    pha
    lda 8,s
    pha
    jsl Room_sectorAt
    pla
    pla
    pla
    lda.b tcc__r0
    asl a
    asl a
    inc a
    inc a
    tay
    lda [olSectorData],y
    asl a
    asl a
    asl a
    tay
    lda [olLinkData],y
    and #$00FF
    cmp #$00FF
    beq RW_Dry
RW_Cold:
    jml Room_waterCold
RW_Dry:
    lda 10,s
    tax
    lda #WALL_H
    sta.w 0,x
    lda 14,s
    tax
    lda #WALL_H
    sta.w 0,x
    rtl
.ENDIF

; Public point lookups share Room_query's room/sector cache and multiplier.
; This does not alter the separate resolved-floor/ceiling cache. C versions
; remain in room.c/nav.c for the CPU-only reference cartridge.
Room_sectorAt:
    jsr RS_Point
    sta.b tcc__r0
    rtl

navBoxAt:
    jsr RS_Point
    sta.b tcc__r0
    cmp #$FFFF
    beq +
    lda.b rq_rsZS
    pha
    pei (tcc__r0)
.IFDEF FX3
    jsr.l navCachedSectorBox
.ELSE
    jsr.l navCachedSectorBoxGsu2
.ENDIF
    pla
    pla
+   rtl

RS_Point:
    ; Skip our two-byte JSR return before the public function's arguments.
    lda 8,s
    sta.b rq_x
    lda 10,s
    sta.b rq_z
    lda 6,s
    cmp.w olRoomCount
    bcc +
    lda #$FFFF
    rts
+   jmp.w RQ_Sector
.ENDIF

; ---------------------------------------------------------------------------
; int Room_query(unsigned int roomHint, int x, int y, int z, RoomQuery* out)
;
; Resolve a world point from a room hint through horizontal and vertical
; portals. Floors and ceilings come from the bottom and top linked sectors
; and include TR's slants. Returns zero only for an invalid table or hint.
;
;   4,s  roomHint    6,s  x    8,s  y    10,s  z    12,s / 14,s  out
; ---------------------------------------------------------------------------
Room_query:
    lda 4,s
    sta.b rq_hint
    lda 6,s
    sta.b rq_x
    lda 8,s
    sta.b rq_y
    lda 10,s
    sta.b rq_z
    lda 12,s
    sta.b tcc__r1
    lda 14,s
    sta.b tcc__r1h
    ora.b tcc__r1
    beq RQ_FailNear
    lda.b rq_hint
    cmp.w olRoomCount
    bcs RQ_FailNear
    lda.w olSectorCount
    beq RQ_FailNear
.IFNDEF C_MOTION
    stz.w olRoomQueryCacheUnaligned
.ENDIF

.IFNDEF RQ_WORLD_CACHE
    lda.b rq_hint
    jsr RQ_Sector
    cmp #$FFFF
    beq RQ_FailNear
    sta.b rq_sector
.ENDIF

    ; World sectors distinguish points clamped into the same hinted-room
    ; edge that reach different sectors through a portal.
    lda.b rq_hint
    cmp.b rq_qcHint
    bne RQ_Resolve
.IFDEF RQ_WORLD_CACHE
    lda.b rq_x
    and #$FC00
.ELSE
    lda.b rq_rsSX
.ENDIF
    cmp.b rq_qcSX
    bne RQ_Resolve
.IFDEF RQ_WORLD_CACHE
    lda.b rq_z
    and #$FC00
.ELSE
    lda.b rq_rsSZ
.ENDIF
    cmp.b rq_qcSZ
    bne RQ_Resolve
    lda.b rq_y
    and #$FF00
    cmp.b rq_qcY
    bne RQ_Resolve
    jmp.w RQ_Answer

RQ_FailNear:                    ; the early outs above cannot reach RQ_Fail
    jmp.w RQ_Fail

RQ_Resolve:
.IFNDEF C_MOTION
    lda.w olRoomQueryCacheEnabled
    beq +
    jsl RQ_CacheLookup
    beq +
    jmp.w RQ_Answer
+
.ENDIF
.IFDEF RQ_WORLD_CACHE
    ; Exact world keys can answer before touching a host sector. A cached
    ; record was admitted only after checking every visited grid's alignment;
    ; Room_reset invalidates it whenever the level/geometry changes.
    lda.b rq_hint
    jsr RQ_Sector
    cmp #$FFFF
    beq RQ_FailNear
    sta.b rq_sector
.ENDIF
    lda.b rq_hint
    sta.b rq_qcHint
.IFDEF RQ_WORLD_CACHE
    lda.b rq_x
    and #$FC00
.ELSE
    lda.b rq_rsSX
.ENDIF
    sta.b rq_qcSX
.IFDEF RQ_WORLD_CACHE
    lda.b rq_z
    and #$FC00
.ELSE
    lda.b rq_rsSZ
.ENDIF
    sta.b rq_qcSZ
    lda.b rq_y
    and #$FF00
    sta.b rq_qcY

    ; FloorData portals are horizontal connections between overlapping room
    ; grids. Match Room::getRoom: exhaust them before choosing a stacked room
    ; from Y. Every walk is bounded by the room count so malformed data
    ; cannot cycle.
    lda.w olRoomCount
    sta.b rq_n
RQ_W1:
    lda.b rq_sector
    SECTOR_X
    LINK_X
    HOST_LDA olLinkData, 2              ; portal | trigger << 8
    and #$00FF
    cmp #ROOM_NONE
    beq RQ_W1Done
    cmp.w olRoomCount
    bcs RQ_W1Done
    jsr RQ_Sector
    cmp #$FFFF
    bne +
    jmp.w RQ_Forget
+   sta.b rq_sector
    dec.b rq_n
    bne RQ_W1
RQ_W1Done:

    ; Up through roomAbove while the point is above the sector's ceiling.
    lda.w olRoomCount
    sta.b rq_n
RQ_W2:
    lda.b rq_sector
    SECTOR_X
    HOST_LDA olSectorData, 0                ; floor | ceiling << 8
    and #$FF00                  ; the ceiling, in world units
    sta.b rq_tmp
    LINK_X
    HOST_LDA olLinkData, 0                ; roomBelow | roomAbove << 8
    xba
    and #$00FF
    cmp #ROOM_NONE
    beq RQ_W2Done
    cmp.w olRoomCount
    bcs RQ_W2Done
    sta.b rq_next
    lda.b rq_y
    sec
    sbc.b rq_tmp
    bvc +
    eor #$8000
+   bpl RQ_W2Done               ; y >= ceiling: under it, stay
    lda.b rq_next
    jsr RQ_Sector
    cmp #$FFFF
    bne +
    jmp.w RQ_Forget
+   sta.b rq_sector
    dec.b rq_n
    bne RQ_W2
RQ_W2Done:

    ; Down through roomBelow while the point is at or under the floor,
    ; exactly as TR's Room::getSector descends.
    lda.w olRoomCount
    sta.b rq_n
RQ_W3:
    lda.b rq_sector
    SECTOR_X
    HOST_LDA olSectorData, 0
    and #$00FF
    xba                         ; the floor, in world units
    sta.b rq_tmp
    LINK_X
    HOST_LDA olLinkData, 0
    and #$00FF                  ; roomBelow
    cmp #ROOM_NONE
    beq RQ_W3Done
    cmp.w olRoomCount
    bcs RQ_W3Done
    sta.b rq_next
    lda.b rq_y
    sec
    sbc.b rq_tmp
    bvc +
    eor #$8000
+   bmi RQ_W3Done               ; y < floor: above it, stay
    lda.b rq_next
    jsr RQ_Sector
    cmp #$FFFF
    bne +
    jmp.w RQ_Forget
+   sta.b rq_sector
    dec.b rq_n
    bne RQ_W3
RQ_W3Done:

    ; The sector's origin in the room the point resolved to, taken now: the
    ; walks below move Room_sector's record to other rooms.
    lda.b rq_rsRoom
    sta.b rq_qcRoom
    lda.b rq_rsSX
    sta.b rq_qcFSX
    lda.b rq_rsSZ
    sta.b rq_qcFSZ
    lda.b rq_rsX
    sta.b rq_qcRX
    lda.b rq_rsZ
    sta.b rq_qcRZ

    ; The bottom-most sector under the point, for the floor and the trigger.
    lda.b rq_sector
    sta.b rq_lower
    lda.w olRoomCount
    sta.b rq_n
RQ_W4:
    lda.b rq_lower
    SECTOR_X
    LINK_X
    HOST_LDA olLinkData, 0
    and #$00FF
    cmp #ROOM_NONE
    beq RQ_W4Done
    cmp.w olRoomCount
    bcs RQ_W4Done
    jsr RQ_Sector
    cmp #$FFFF
    bne +
    jmp.w RQ_Forget
+   sta.b rq_lower
    dec.b rq_n
    bne RQ_W4
RQ_W4Done:

    ; And the top-most over it, for the ceiling.
    lda.b rq_sector
    sta.b rq_upper
    lda.w olRoomCount
    sta.b rq_n
RQ_W5:
    lda.b rq_upper
    SECTOR_X
    LINK_X
    HOST_LDA olLinkData, 0
    xba
    and #$00FF
    cmp #ROOM_NONE
    beq RQ_W5Done
    cmp.w olRoomCount
    bcs RQ_W5Done
    jsr RQ_Sector
    cmp #$FFFF
    bne +
    jmp.w RQ_Forget
+   sta.b rq_upper
    dec.b rq_n
    bne RQ_W5
RQ_W5Done:

    ; FX3 retains both original signed bytes; GSU-2 keeps its ten-bit pair.
    lda.b rq_lower
    SECTOR_X
    HOST_LDA olSectorData, 0
    and #$00FF
    xba
    sta.b rq_qcFloor
    LINK_X
    HOST_LDA olLinkData, 2              ; portal | trigger << 8
    xba
    and #$00FF
    sta.b rq_qcTrigger
    HOST_LDA_NEXT olLinkData, 4, 2      ; Y still addresses portal/trigger
    sta.b rq_tmp
    jsr RQ_SignSlope
    sta.b rq_qcFsx
    lda.b rq_tmp
.IFDEF FX3
    xba
.ELSE
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
.ENDIF
    jsr RQ_SignSlope
    sta.b rq_qcFsz

    lda.b rq_upper
    SECTOR_X
    HOST_LDA olSectorData, 0
    and #$FF00
    sta.b rq_qcCeil
    LINK_X
    HOST_LDA olLinkData, 6              ; the ceiling's slants
    sta.b rq_tmp
    jsr RQ_SignSlope
    sta.b rq_qcCsx
    lda.b rq_tmp
.IFDEF FX3
    xba
.ELSE
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
.ENDIF
    jsr RQ_SignSlope
    sta.b rq_qcCsz
.IFNDEF C_MOTION
    lda.w olRoomQueryCacheUnaligned
    beq +
    lda #$FFFF
    sta.b rq_qcHint
    bra ++
+
    lda.w olRoomQueryCacheEnabled
    beq ++
    jsl RQ_CacheStore
++
.ENDIF

; ---------------------------------------------------------------------------
; Upstream's Sector::getFloor and getCeiling on the cached bases. Each slant
; term in the C is `slant * (slant < 0 ? d : d - 1023)`, arithmetically
; shifted right by two and subtracted (or, for the ceiling's Z, added). The
; product's sign is fixed by the selection -- the floor terms and the
; ceiling's Z term are never positive, the ceiling's X term never negative --
; so they are computed here as a magnitude on the two absolute values, and
; the arithmetic shift of a negative product, which rounds towards minus
; infinity, is (m + 3) >> 2 on the magnitude.
; ---------------------------------------------------------------------------
RQ_Answer:
.IFNDEF FX3
.IFNDEF C_MOTION
    ; Resolution is finished: slope arithmetic uses only WRAM and CPU I/O.
    ; Keep far cartridge outputs serial; normal stack/global WRAM outputs
    ; can be written while the GSU renders. All arguments are already in DP.
    lda.b tcc__r1h
    beq RQ_MirroredOutput
    cmp #$7E
    beq RQ_YieldOutput
    cmp #$7F
    beq RQ_YieldOutput
    bra RQ_SerialOutput
RQ_MirroredOutput:
    ; Bank-zero pointers commonly address the mirrored CPU stack. Require
    ; the entire 20-byte record to remain in WRAM, below the I/O window.
    lda.b tcc__r1
    cmp #$1FED
    bcc RQ_YieldOutput
RQ_SerialOutput:
    lda #0
    bra ++
RQ_YieldOutput:
    jsl olGsuYieldRam
++  pha                        ; ownership token, private to this query
.ENDIF
.ENDIF
    lda.b rq_x
    and #$03FF
    sta.b rq_dx
    lda.b rq_z
    and #$03FF
    sta.b rq_dz

    lda.b rq_qcFloor
    cmp #WALL_H
    bne +
    lda #NO_FLOOR
    sta.b rq_floor
    bra RQ_Ceil
+   sta.b rq_floor

    lda.b rq_qcFsx
    beq RQ_FloorZ
    bmi +
    lda.b rq_dx
    sec
    sbc #1023
    bra ++
+   lda.b rq_dx
++  sta.b rq_d
    lda.b rq_qcFsx
.IFDEF FX3
    jsr RQ_MulUp
.ELSE
    jsr RQ_Mul
    clc
    adc #3
    lsr a
    lsr a
.ENDIF
    clc
    adc.b rq_floor
    sta.b rq_floor
RQ_FloorZ:
    lda.b rq_qcFsz
    beq RQ_Ceil
    bmi +
    lda.b rq_dz
    sec
    sbc #1023
    bra ++
+   lda.b rq_dz
++  sta.b rq_d
    lda.b rq_qcFsz
.IFDEF FX3
    jsr RQ_MulUp
.ELSE
    jsr RQ_Mul
    clc
    adc #3
    lsr a
    lsr a
.ENDIF
    clc
    adc.b rq_floor
    sta.b rq_floor

RQ_Ceil:
    lda.b rq_qcCeil
    sta.b rq_ceil
    lda.b rq_qcCsx
    beq RQ_CeilZ
    bmi +
    lda.b rq_dx
    bra ++
+   lda.b rq_dx
    sec
    sbc #1023
++  sta.b rq_d
    lda.b rq_qcCsx
.IFDEF FX3
    jsr RQ_MulDown
.ELSE
    jsr RQ_Mul
    lsr a
    lsr a
.ENDIF
    sta.b rq_tmp
    lda.b rq_ceil
    sec
    sbc.b rq_tmp
    sta.b rq_ceil
RQ_CeilZ:
    lda.b rq_qcCsz
    beq RQ_Out
    bmi +
    lda.b rq_dz
    sec
    sbc #1023
    bra ++
+   lda.b rq_dz
++  sta.b rq_d
    lda.b rq_qcCsz
.IFDEF FX3
    jsr RQ_MulUp
.ELSE
    jsr RQ_Mul
    clc
    adc #3
    lsr a
    lsr a
.ENDIF
    sta.b rq_tmp
    lda.b rq_ceil
    sec
    sbc.b rq_tmp
    sta.b rq_ceil

    ; RoomQuery, in room.h's order: room, floor, ceiling, sectorX, sectorZ,
    ; trigger, floorSlopeX, floorSlopeZ, ceilingSlopeX, ceilingSlopeZ.
RQ_Out:
.IFDEF FX3
.IFNDEF C_MOTION
    ; Ordinary WRAM objects and stack locals can use indexed stores. Keep
    ; direct-page aliases, I/O mirrors and bank-crossing far outputs on
    ; the original path; that path rereads the far pointer after each store.
    lda.b tcc__r1
    cmp #$1000
    bcc RQ_OutFar
    lda.b tcc__r1h
    and #$00FF
    cmp #$007E
    beq RQ_OutWram
    cmp #0
    bne RQ_OutFar
    lda.b tcc__r1
    cmp #$1FED
    bcs RQ_OutFar
    bra RQ_OutNear
RQ_OutWram:
    lda.b tcc__r1
    cmp #$FFED
    bcs RQ_OutFar
RQ_OutNear:
    ldx.b tcc__r1
    lda.b rq_qcRoom
    sta.w 0,x
    lda.b rq_floor
    sta.w 2,x
    lda.b rq_ceil
    sta.w 4,x
    lda.b rq_qcFSX
    xba
    asl a
    asl a
    clc
    adc.b rq_qcRX
    sta.w 6,x
    lda.b rq_qcFSZ
    xba
    asl a
    asl a
    clc
    adc.b rq_qcRZ
    sta.w 8,x
    lda.b rq_qcTrigger
    sta.w 10,x
    lda.b rq_qcFsx
    sta.w 12,x
    lda.b rq_qcFsz
    sta.w 14,x
    lda.b rq_qcCsx
    sta.w 16,x
    lda.b rq_qcCsz
    sta.w 18,x
    bra RQ_Return
RQ_OutFar:
.ENDIF
.ENDIF
    ldy #0
    lda.b rq_qcRoom
    sta.b [tcc__r1],y
    ldy #2
    lda.b rq_floor
    sta.b [tcc__r1],y
    ldy #4
    lda.b rq_ceil
    sta.b [tcc__r1],y
    ldy #6
    lda.b rq_qcFSX
    xba
    asl a
    asl a                       ; sector << 10
    clc
    adc.b rq_qcRX
    sta.b [tcc__r1],y
    ldy #8
    lda.b rq_qcFSZ
    xba
    asl a
    asl a
    clc
    adc.b rq_qcRZ
    sta.b [tcc__r1],y
    ldy #10
    lda.b rq_qcTrigger
    sta.b [tcc__r1],y
    ldy #12
    lda.b rq_qcFsx
    sta.b [tcc__r1],y
    ldy #14
    lda.b rq_qcFsz
    sta.b [tcc__r1],y
    ldy #16
    lda.b rq_qcCsx
    sta.b [tcc__r1],y
    ldy #18
    lda.b rq_qcCsz
    sta.b [tcc__r1],y
RQ_Return:
.IFNDEF FX3
.IFNDEF C_MOTION
    pla
    jsl olGsuReclaimRam
.ENDIF
.ENDIF
    lda #1
    sta.b tcc__r0
    rtl

RQ_Forget:
    lda #$FFFF
    sta.b rq_qcHint
RQ_Fail:
    stz.b tcc__r0
    rtl

; ---------------------------------------------------------------------------
; int olStaticHit(unsigned int room, int px, int py, int pz, int radius,
;                 int height, AABBi* box)
;
; The scan in Room::collideStatic: the room's colliding static placements,
; each one's box translated to the point and tested against her cylinder --
; radius either side, height above, nothing below. Returns 1 with the
; translated box written for the first that intersects, 0 for none. The
; push-out and what to call the collision stay in C, on the hit, which is
; rare; the scan runs per probe, per tick, on every placement in the room,
; and from C it was 3,900 cycles for one of them.
;
; The record is level.h's: uint8 room, pad, int16 x, y, z, then the box in
; AABBi order. Signed compares throughout: sbc, then the overflow fix-up,
; then the sign.
;
;   4,s room   6,s px   8,s py   10,s pz   12,s radius   14,s height
;   16,s / 18,s box
; ---------------------------------------------------------------------------
olStaticHit:
    lda 16,s
    sta.b tcc__r1
    lda 18,s
    sta.b tcc__r1h
    lda 4,s
    cmp.w olRoomCount
    bcc +
SH_None:
    stz.b tcc__r0
    rtl
+   asl a
    clc
    HOST_ADD_BASE olStaticFirstBase
    tax
    HOST_LDA olStaticFirstData, 0                ; the room's first placement
    sta.b rq_i
    HOST_LDA_NEXT olStaticFirstData, 2, 2      ; next room's first placement
    sta.b rq_end
SH_Next:
    lda.b rq_i
    cmp.b rq_end
    bcs SH_None
    asl a
    asl a
    sta.b rq_tmp
    asl a
    asl a
    clc
    adc.b rq_tmp                ; i * 20
    clc
    HOST_ADD_BASE olStaticBase
    tax
    inc.b rq_i

    ; where it stands, relative to the point; further than five sectors on
    ; any axis is upstream's MAX_STATIC_MESH_RADIUS reject
    HOST_LDA olStaticData, 2
    sec
    sbc 6,s
    sta.b rq_dx
    bpl +
    eor #$FFFF
    inc a
+   cmp #5121
    bcs SH_Next
    HOST_LDA_NEXT olStaticData, 6, 4
    sec
    sbc 10,s
    sta.b rq_dz
    bpl +
    eor #$FFFF
    inc a
+   cmp #5121
    bcs SH_Next
    HOST_LDA_NEXT olStaticData, 4, -2
    sec
    sbc 8,s
    sta.b rq_dy
    bpl +
    eor #$FFFF
    inc a
+   cmp #5121
    bcs SH_Next

    ; boxIntersect(meshBox, objBox), objBox being -R..R, -H..0, -R..R:
    ; out when maxX <= -R, minX >= R, maxY <= -H, minY >= 0, and Z as X.
    ; SH_Next is out of an 8-bit branch's reach from here on, hence the
    ; trampoline.
    bra SH_Test
SH_Skip:
    jmp.w SH_Next
SH_Test:
    HOST_LDA olStaticData, 10             ; maxX + dx + R <= 0 ?
    clc
    adc.b rq_dx
    clc
    adc 12,s
    bvc +
    eor #$8000
+   bmi SH_Skip
    beq SH_Skip
    HOST_LDA_NEXT olStaticData, 8, -2     ; minX + dx - R >= 0 ?
    clc
    adc.b rq_dx
    sec
    sbc 12,s
    bvc +
    eor #$8000
+   bpl SH_Skip
    HOST_LDA_NEXT olStaticData, 18, 10    ; maxZ + dz + R <= 0 ?
    clc
    adc.b rq_dz
    clc
    adc 12,s
    bvc +
    eor #$8000
+   bmi SH_Skip
    beq SH_Skip
    HOST_LDA_NEXT olStaticData, 16, -2    ; minZ + dz - R >= 0 ?
    clc
    adc.b rq_dz
    sec
    sbc 12,s
    bvc +
    eor #$8000
+   bpl SH_Skip
    HOST_LDA_NEXT olStaticData, 14, -2    ; maxY + dy + H <= 0 ?
    clc
    adc.b rq_dy
    clc
    adc 14,s
    bvc +
    eor #$8000
.IFDEF FX3
+   bmi +
    bne ++
+   jmp.w SH_Skip
++
.ELSE
+   bmi SH_Skip
    beq SH_Skip
.ENDIF
    HOST_LDA_NEXT olStaticData, 12, -2    ; minY + dy >= 0 ?
    clc
    adc.b rq_dy
.IFDEF FX3
    bmi +
    jmp.w SH_Skip
+
.ELSE
    bpl SH_Skip
.ENDIF

    ; a hit: the box where it stands relative to the point, in AABBi order
.IFNDEF FX3
    ldy #0
.ENDIF
    HOST_LDA olStaticData, 8
    clc
    adc.b rq_dx
.IFDEF FX3
    ldy #0
.ENDIF
    sta.b [tcc__r1],y
.IFNDEF FX3
    ldy #2
.ENDIF
    HOST_LDA olStaticData, 10
    clc
    adc.b rq_dx
.IFDEF FX3
    ldy #2
.ENDIF
    sta.b [tcc__r1],y
.IFNDEF FX3
    ldy #4
.ENDIF
    HOST_LDA olStaticData, 12
    clc
    adc.b rq_dy
.IFDEF FX3
    ldy #4
.ENDIF
    sta.b [tcc__r1],y
.IFNDEF FX3
    ldy #6
.ENDIF
    HOST_LDA olStaticData, 14
    clc
    adc.b rq_dy
.IFDEF FX3
    ldy #6
.ENDIF
    sta.b [tcc__r1],y
.IFNDEF FX3
    ldy #8
.ENDIF
    HOST_LDA olStaticData, 16
    clc
    adc.b rq_dz
.IFDEF FX3
    ldy #8
.ENDIF
    sta.b [tcc__r1],y
.IFNDEF FX3
    ldy #10
.ENDIF
    HOST_LDA olStaticData, 18
    clc
    adc.b rq_dz
.IFDEF FX3
    ldy #10
.ENDIF
    sta.b [tcc__r1],y
    lda #1
    sta.b tcc__r0
    rtl

; ---------------------------------------------------------------------------
; Room_sector: A = room, rq_x / rq_z the point -> A = the host sector index,
; or $FFFF for a room with no sectors. Keeps the room record and the last
; answer, as the C did.
; ---------------------------------------------------------------------------
RQ_Sector:
    cmp.b rq_rsRoom
    beq RQ_SecKnown
    sta.b rq_rsRoom
    asl a
    sta.b rq_tmp
    asl a
    asl a
    clc
    adc.b rq_tmp                ; room * 10: the host room record
    clc
    HOST_ADD_BASE olRoomBase
    tax
    HOST_LDA olRoomData, 2
    sta.b rq_rsX
    HOST_LDA_NEXT olRoomData, 4, 2
    sta.b rq_rsZ
    HOST_LDA_NEXT olRoomData, 6, 2
    sta.b rq_rsOff
    HOST_LDA_NEXT olRoomData, 8, 2
    and #$007F                  ; bit 7 is the room's water flag
    sta.b rq_rsXS
    HOST_LDA_NEXT olRoomData, 9, 1
    and #$00FF
    sta.b rq_rsZS
    lda #$FFFF
    sta.b rq_rsSX
RQ_SecKnown:
.IFNDEF C_MOTION
    ; TR grids align to world sectors. A synthetic/unaligned grid can
    ; cross a different linked-room boundary inside that world sector.
    lda.b rq_rsX
    ora.b rq_rsZ
    and #$03FF
    beq +
    lda #1
    sta.w olRoomQueryCacheUnaligned
+
.ENDIF
    lda.b rq_rsXS
    beq RQ_SecNone
    lda.b rq_rsZS
    beq RQ_SecNone

    ; sx = clamp((x - rsX) >> 10, 0, xSectors - 1), on the sixteen-bit
    ; difference as the C had it: negative is 0, and a non-negative one
    ; shifts as its high byte over four.
    lda.b rq_x
    sec
    sbc.b rq_rsX
    bmi RQ_SecSX0
    xba
    and #$00FF
    lsr a
    lsr a
    cmp.b rq_rsXS
    bcc +
    lda.b rq_rsXS
    dec a
+   bra RQ_SecSX
RQ_SecSX0:
    lda #0
RQ_SecSX:
    sta.b rq_sx

    lda.b rq_z
    sec
    sbc.b rq_rsZ
    bmi RQ_SecSZ0
    xba
    and #$00FF
    lsr a
    lsr a
    cmp.b rq_rsZS
    bcc +
    lda.b rq_rsZS
    dec a
+   bra RQ_SecSZ
RQ_SecSZ0:
    lda #0
RQ_SecSZ:
    sta.b rq_sz

    lda.b rq_sx
    cmp.b rq_rsSX
    bne RQ_SecMiss
    lda.b rq_sz
    cmp.b rq_rsSZ
    bne RQ_SecMiss
    lda.b rq_rsSector
    rts

RQ_SecMiss:
    lda.b rq_sx
    sta.b rq_rsSX
    lda.b rq_sz
    sta.b rq_rsSZ
    ; sector = sectorOffset + sx * zSectors + sz. The multiply is the CPU's
    ; own: eight by eight, ready eight cycles after the second operand, and
    ; the three instructions between the write and the read take thirteen.
    ; Nothing in the NMI handler touches it.
    SETA8
    lda.b rq_sx
    sta.l WRMPYA
    lda.b rq_rsZS
    sta.l WRMPYB
    SETA16
    lda.b rq_rsOff
    clc
    adc.b rq_sz
    clc
    adc.l RDMPYL
    sta.b rq_rsSector
    rts

RQ_SecNone:
    lda #$FFFF
    rts

; A = the low packed slope field -> A = it, sign-extended.
RQ_SignSlope:
.IFDEF FX3
    and #$00FF
    bit #$0080
    beq +
    ora #$FF00
.ELSE
    and #$001F
    bit #$0010
    beq +
    ora #$FFE0
.ENDIF
+   rts

.IFDEF FX3
; Return floor((|slope| * |distance| + rounding) / 4). Full signed-byte
; slopes can produce a 17-bit product: never truncate it before the shift.
; Existing small slopes retain the short shift/add path. Wide slopes use
; s*(d>>2) + ((s*(d&3)+rounding)>>2), two exact 8x8 products whose sum fits
; a word even at slope=-128, distance=1023. NMI never uses the multiplier.
RQ_MulUp:
    bpl +
    eor #$FFFF
    inc a
+   sta.b rq_b
    cmp #65                    ; 64*1023+3 still fits 16 bits
    bcs RQ_WideUp
    lda.b rq_d
    bpl +
    eor #$FFFF
    inc a
+   sta.b rq_a
    lda #3                     ; fold rounding into the shift/add accumulator
    bra RQ_SmallSlope
RQ_MulDown:
    bpl +
    eor #$FFFF
    inc a
+   sta.b rq_b
    cmp #65
    bcs RQ_WideDown
    lda.b rq_d
    bpl +
    eor #$FFFF
    inc a
+   sta.b rq_a
    lda #0
RQ_SmallSlope:
-   lsr.b rq_b
    bcc +
    clc
    adc.b rq_a
+   asl.b rq_a
    ldx.b rq_b
    bne -
    lsr a
    lsr a
    rts
RQ_WideUp:
    ldx #3
    bra RQ_WideSlope
RQ_WideDown:
    ldx #0
RQ_WideSlope:
    stx.b rq_tmp
    lda.b rq_d
    bpl +
    eor #$FFFF
    inc a
+   sta.b rq_a
    sep #$20
    lda.b rq_b
    sta.l WRMPYA
    lda.b rq_a
    and #3
    sta.l WRMPYB
    rep #$20
    lda.b rq_a
    lsr a
    lsr a
    tax                        ; >=8 cycles since WRMPYB
    lda.l RDMPYL
    clc
    adc.b rq_tmp
    lsr a
    lsr a
    sta.b rq_tmp
    sep #$20
    txa
    sta.l WRMPYB
    rep #$20
    lda.b rq_tmp
    clc                        ; >=8 cycles since WRMPYB; WRMPYA is unchanged
    adc.l RDMPYL
    rts
.ELSE
; A = a slant, rq_d = a signed distance -> A = |slant| * |distance|.
; Five bits of multiplier at most and usually one or two set, so a shift-add
; with an early exit beats setting up the hardware multiplier twice for a
; ten-bit distance.
RQ_Mul:
    bpl +
    eor #$FFFF
    inc a
+   sta.b rq_b
    lda.b rq_d
    bpl +
    eor #$FFFF
    inc a
+   sta.b rq_a
    lda #0
-   lsr.b rq_b
    bcc +
    clc
    adc.b rq_a
+   asl.b rq_a
    ldx.b rq_b
    bne -
    rts
.ENDIF

.ENDS

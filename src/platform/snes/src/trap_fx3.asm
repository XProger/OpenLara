; OpenLara - SNES / Super FX target
; The swinging blade's collideSpheres against Lara, by the proven fast path.
;
; At pitch 0 the matrix chain's row 1 never sees her yaw, so each sphere's y
; is the yaw-0 one exactly; x and z are the yaw-0 centre turned by the yaw's
; table cosine and sine, to within the bound the packer proved over every
; stored frame and all 4,096 table yaws (packer/out_SNES.h buildSphereData;
; tools/spherecheck.cpp --table proves it again against upstream's code).
; The blade never turns: its centres are upstream's exactly, per yaw. So a
; pair is a certain hit when its farthest possible distance is inside the
; radii, a certain miss when its nearest is not, and otherwise undecided --
; then trap.c runs the exact chain (Trap_collideSpheres).
;
; unsigned int Trap_sphereTest(ItemObj* blade, ItemObj* lara):
;   0 no blade sphere meets hers, 1 one does, 2 undecided (or not covered:
;   pitch, a yaw or animation the tables do not hold).
;
; uint32 Trap_collideSpheres(ItemObj* blade, ItemObj* lara): upstream's
;   getSpheres for both -- GBA's int32 matrix chain exactly: 2^14 scale, the
;   4,096-entry table, matrixFrame's translate then Y, X, Z with its zero
;   skips, DECODE_ANGLES, the node stack -- into olSpheres[0] (blade) and
;   [1] (Lara), then collideSpheres' mask. Matrix rows keep e0-e2 in 16 bits
;   (they stay within 16,384 and rounding) and e3 in 32; every product is
;   16x16 on the PPU. The runner checks both against upstream's own code.
;
; Products on the PPU's 16x8 signed multiplier ($211B twice, $211C once,
; the 24-bit product at $2134-$2136 at once): FX3 has no Mode 7, so it is
; free. A 16x16 product is two: b = hi' * 256 + (int8)lo, where lo is b's
; low byte and hi' the high byte of b + $80.
;
; 816-tcc ABI: A/X/Y16, D0, DB $7E; arguments at 4,s after the JSL; the
; result in tcc__r0 (and A). Scratch: tcc__r0-r5, r9, r10 and olSphFast[]
; (trap.c). Only C calls this.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFDEF FX3
.IFNDEF C_MOTION
.INCLUDE "hostbank.inc"
.BASE $80
.BANK 4 SLOT 0
.SECTION "trap_fx3_native" SEMIFREE
.accu 16
.index 16
.16bit

.DEFINE SPH         (OL_SPHERE_BANK << 16)
.DEFINE SPH_BOUND   16410       ; u16 bound, u16 yaw count, 4 x {yaw, 0, u32 table}
.DEFINE SPH_LARA    16386       ; u16 type, count; u32 meshes; u16 first, anims
.DEFINE SPH_BLADE   16398

.DEFINE PPU_M7A     $00211B
.DEFINE PPU_M7B     $00211C
.DEFINE PPU_MPYL    $002134
.DEFINE PPU_MPYM    $002135

; olSphFast[] words (trap.c): the blade's spheres, her position subtracted,
; 8 bytes each {x, y, z, r}; then scratch.
.DEFINE SF_BLADE    olSphFast+0
.DEFINE SF_BCOUNT   olSphFast+32
.DEFINE SF_LCOUNT   olSphFast+34
.DEFINE SF_LMESH    olSphFast+36    ; her mesh record, bank 0 of the data
.DEFINE SF_X0       olSphFast+38
.DEFINE SF_Y0       olSphFast+40
.DEFINE SF_Z0       olSphFast+42
.DEFINE SF_RL       olSphFast+44
.DEFINE SF_X        olSphFast+46
.DEFINE SF_Z        olSphFast+48
.DEFINE SF_DX       olSphFast+50
.DEFINE SF_DY       olSphFast+52
.DEFINE SF_DZ       olSphFast+54
.DEFINE SF_R        olSphFast+56
.DEFINE SF_SUM      olSphFast+58    ; 32-bit
.DEFINE SF_R2       olSphFast+62    ; 32-bit
.DEFINE SF_AMB      olSphFast+66
.DEFINE SF_TMP      olSphFast+68
.DEFINE SF_I        olSphFast+70
.DEFINE SF_RHO      olSphFast+72    ; her sphere's reach from her origin, turned
; the exact chain's
.DEFINE SC_TOP      olSphFast+74    ; the current matrix, bytes into olSphStack
.DEFINE SC_CA       olSphFast+76    ; the rotation's two columns, row bytes
.DEFINE SC_CB       olSphFast+78
.DEFINE SC_C        olSphFast+80    ; cos, sin, -sin as hi' << 8 | lo
.DEFINE SC_S        olSphFast+82
.DEFINE SC_NS       olSphFast+84
.DEFINE SC_XS       olSphFast+86    ; the translation, split the same way
.DEFINE SC_YS       olSphFast+88
.DEFINE SC_ZS       olSphFast+90
.DEFINE SC_T        olSphFast+92
.DEFINE SC_T2       olSphFast+94
.DEFINE SC_HDR      olSphFast+96    ; the model's header in the data
.DEFINE SC_COUNT    olSphFast+98
.DEFINE SC_MESH     olSphFast+100   ; its mesh record, bank 0
.DEFINE SC_FRAME    olSphFast+102   ; its frame record, far
.DEFINE SC_OUT      olSphFast+106   ; bytes into olSpheres
.DEFINE SC_I        olSphFast+108
.DEFINE SC_PX       olSphFast+110   ; the item's position, angles, anim, frame
.DEFINE SC_PY       olSphFast+112
.DEFINE SC_PZ       olSphFast+114
.DEFINE SC_YAW      olSphFast+116
.DEFINE SC_PITCH    olSphFast+118
.DEFINE SC_ANIM     olSphFast+120
.DEFINE SC_FIDX     olSphFast+122
.DEFINE SC_VX       olSphFast+124   ; a translation or frame offset
.DEFINE SC_VY       olSphFast+126
.DEFINE SC_VZ       olSphFast+128
.DEFINE SC_ANGLO    olSphFast+130   ; a frame's packed angles, GBA's uint32
.DEFINE SC_ANGHI    olSphFast+132
.DEFINE SC_AX       olSphFast+134
.DEFINE SC_AY       olSphFast+136
.DEFINE SC_AZ       olSphFast+138
.DEFINE SX_BLADE    olSphFast+140   ; far pointers
.DEFINE SX_LARA     olSphFast+144
.DEFINE SX_MASK     olSphFast+148   ; 32-bit
.DEFINE SX_BIT      olSphFast+152   ; 32-bit
.DEFINE SX_AN       olSphFast+156
.DEFINE SX_BN       olSphFast+158
.DEFINE SX_I        olSphFast+160   ; bytes into olSpheres
.DEFINE SX_J        olSphFast+162
.DEFINE SX_RA       olSphFast+164

; int32 olMulS16(int a, int b): a * b, signed, in tcc__r0 (low word) and
; tcc__r1 (high). Two PPU products, b = hi' * 256 + (int8)lo as below; b
; must stay under 32640. The inventory's matrices use it (inventory_draw.c).
olMulS16:
    lda 6,s
    clc
    adc #$0080
    xba
    and #$00FF
    sta.b tcc__r2               ; hi'
    sep #$20
    lda 4,s
    sta.l PPU_M7A
    lda 5,s
    sta.l PPU_M7A
    lda 6,s
    sta.l PPU_M7B               ; a * (int8)lo
    rep #$20
    lda.l PPU_MPYL
    sta.b tcc__r0
    lda.l PPU_MPYM              ; bits 8-23: the high word is MPYH's sign
    xba
    and #$00FF
    bit #$0080
    beq +
    ora #$FF00
+   sta.b tcc__r1
    sep #$20
    lda.b tcc__r2
    sta.l PPU_M7B               ; a * hi', to go a byte up
    rep #$20
    lda.l PPU_MPYL
    xba
    and #$FF00
    clc
    adc.b tcc__r0
    sta.b tcc__r0
    lda.l PPU_MPYM
    adc.b tcc__r1
    sta.b tcc__r1
    lda.b tcc__r0
    rtl

.IF OL_SPHERE_SIZE == 0
Trap_sphereTest:
    lda #1
    sta.b tcc__r0
    rtl
Trap_collideSpheres:
    lda #$FFFF
    sta.b tcc__r0
    sta.b tcc__r1
    rtl
.ELSE
Trap_sphereTest:
    lda 4,s
    sta.b tcc__r0               ; the blade
    lda 6,s
    sta.b tcc__r0h
    lda 8,s
    sta.b tcc__r1               ; Lara
    lda 10,s
    sta.b tcc__r1h
    lda.w olLaraAngleX
    beq +
    jmp ST_Undecided            ; pitched (in water): the exact chain
+
    ; The blade's yaw table.
    ldy #10
    lda [tcc__r0],y
    sta.b tcc__r2
    lda.l SPH+SPH_BOUND+2
    beq ST_NoTable
    sta.w SF_TMP
    ldx #0
ST_Yaw:
    lda.l SPH+SPH_BOUND+4,x
    cmp.b tcc__r2
    beq ST_YawFound
    txa
    clc
    adc #8
    tax
    dec.w SF_TMP
    bne ST_Yaw
ST_NoTable:
    jmp ST_Undecided
ST_YawFound:
    lda.l SPH+SPH_BOUND+8,x
    sta.b tcc__r3               ; the table, far
    lda.l SPH+SPH_BOUND+10,x
    clc
    adc #OL_SPHERE_BANK
    sta.b tcc__r3h

    ; Its record: per anim {tickBase, ticks}, then ticks x count x 6 bytes.
    lda.l SPH+SPH_BLADE+2
    cmp #5
    bcs ST_NoTable              ; more spheres than olSphFast holds
    sta.w SF_BCOUNT
    ldy #12
    lda [tcc__r0],y
    sec
    sbc.l SPH+SPH_BLADE+8
    cmp.l SPH+SPH_BLADE+10
    bcs ST_NoTable
    asl a
    asl a
    tay
    lda [tcc__r3],y
    sta.b tcc__r2               ; tickBase
    iny
    iny
    lda [tcc__r3],y
    dec a
    sta.b tcc__r2h              ; last tick
    ldy #14
    lda [tcc__r0],y
    and #$00FF
    cmp.b tcc__r2h
    bcc +
    lda.b tcc__r2h              ; getSpheres clamps a frame past the end
+   clc
    adc.b tcc__r2
    sep #$20                    ; M7A = tick, M7B = count * 6
    sta.l PPU_M7A
    xba
    sta.l PPU_M7A
    lda.w SF_BCOUNT
    asl a
    adc.w SF_BCOUNT
    asl a
    sta.l PPU_M7B
    rep #$20
    lda.l SPH+SPH_BLADE+10
    asl a
    asl a
    clc
    adc.l PPU_MPYL
    clc
    adc.b tcc__r3
    sta.b tcc__r3               ; the tick's record

    ; D = blade - her, per axis: the blade's spheres relative to her.
    ldy #4
    lda [tcc__r0],y
    sec
    sbc [tcc__r1],y
    sta.b tcc__r4
    ldy #6
    lda [tcc__r0],y
    sec
    sbc [tcc__r1],y
    sta.b tcc__r4h
    ldy #8
    lda [tcc__r0],y
    sec
    sbc [tcc__r1],y
    sta.b tcc__r5
    lda.l SPH+SPH_BLADE+4       ; mesh records, bank 0
    clc
    adc #6
    tax                         ; its first radius
    stz.w SF_I                  ; byte offset in the tick's record
    ldy #0                      ; byte offset in SF_BLADE
ST_Blade:
    phy
    ldy.w SF_I
    lda [tcc__r3],y
    clc
    adc.b tcc__r4
    sta.w SF_TMP
    iny
    iny
    lda [tcc__r3],y
    clc
    adc.b tcc__r4h
    sta.b tcc__r2
    iny
    iny
    lda [tcc__r3],y
    clc
    adc.b tcc__r5
    sta.b tcc__r2h
    iny
    iny
    sty.w SF_I
    ply
    lda.w SF_TMP
    sta.w SF_BLADE,y
    lda.b tcc__r2
    sta.w SF_BLADE+2,y
    lda.b tcc__r2h
    sta.w SF_BLADE+4,y
    lda.l SPH,x
    sta.w SF_BLADE+6,y
    txa
    clc
    adc #16
    tax
    tya
    clc
    adc #8
    tay
    lsr a
    lsr a
    lsr a
    cmp.w SF_BCOUNT
    bcc ST_Blade

ST_Lara:
    ; Her frame: anim -> tick directory -> frame record -> yaw-0 centres.
    ldy #12
    lda [tcc__r1],y
    sec
    sbc.l SPH+SPH_LARA+8
    cmp.l SPH+SPH_LARA+10
    bcc +
    jmp ST_Undecided
+   asl a
    asl a
    sta.w SF_TMP
    lda.l SPH+SPH_LARA+2
    sta.w SF_LCOUNT
    asl a
    asl a
    asl a
    asl a
    clc
    adc.w SF_TMP
    clc
    adc.l SPH+SPH_LARA+4        ; directory slots follow the mesh records, bank 0
    tax
    lda.l SPH,x
    sta.b tcc__r0               ; the directory, far
    lda.l SPH+2,x
    clc
    adc #OL_SPHERE_BANK
    sta.b tcc__r0h
    lda [tcc__r0]
    dec a
    sta.w SF_TMP                ; last tick
    ldy #14
    lda [tcc__r1],y
    and #$00FF
    cmp.w SF_TMP
    bcc +
    lda.w SF_TMP
+   asl a
    asl a
    inc a
    inc a
    tay
    lda [tcc__r0],y
    sta.b tcc__r2
    iny
    iny
    lda [tcc__r0],y
    clc
    adc #OL_SPHERE_BANK
    sta.b tcc__r2h              ; the frame record, far
    ldy #10
    lda [tcc__r1],y             ; her yaw: the sine table entry
    lsr a
    lsr a
    and #$FFFC
    tax
    lda.l SPH,x                 ; cos
    clc
    adc #$0080
    and #$FF00
    sta.b tcc__r9               ; hi' in the high byte ...
    lda.l SPH,x
    and #$00FF
    ora.b tcc__r9
    sta.b tcc__r9               ; ... lo in the low
    lda.l SPH+2,x               ; sin
    clc
    adc #$0080
    and #$FF00
    sta.b tcc__r9h
    lda.l SPH+2,x
    and #$00FF
    ora.b tcc__r9h
    sta.b tcc__r9h
    lda.l SPH+SPH_BOUND
    sta.b tcc__r10              ; the bound
    lda.w SF_LCOUNT             ; centres after the angles
    asl a
    asl a
    clc
    adc #6
    adc.b tcc__r2
    sta.b tcc__r0               ; her centres, far
    lda.b tcc__r2h
    sta.b tcc__r0h
    lda.l SPH+SPH_LARA+4
    clc
    adc #6
    sta.w SF_LMESH              ; her first radius
    stz.w SF_AMB

ST_Sphere:
    ldx.w SF_LMESH
    lda.l SPH,x
    bpl +
    jmp ST_SphereNext
+   bne +
    jmp ST_SphereNext
+   sta.w SF_RL
    lda [tcc__r0]
    sta.w SF_X0
    ldy #2
    lda [tcc__r0],y
    sta.w SF_Y0
    ldy #4
    lda [tcc__r0],y
    sta.w SF_Z0
    ; Before turning it: y is exact, and turned, |x| and |z| stay within
    ; (|x0| + |z0|) / 16 plus the product's rounding, the floors and the
    ; bound. A sphere no blade sphere passes that for is a miss unturned.
    bpl +
    eor #$FFFF
    inc a
+   sta.w SF_TMP
    lda.w SF_X0
    bpl +
    eor #$FFFF
    inc a
+   clc
    adc.w SF_TMP
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc #4
    adc.b tcc__r10
    sta.w SF_RHO
    ldy #0
ST_Pre:
    lda.w SF_BLADE+6,y
    bmi ST_PreNext
    beq ST_PreNext
    clc
    adc.w SF_RL
    sta.w SF_R
    lda.w SF_BLADE+2,y
    sec
    sbc.w SF_Y0
    bpl +
    eor #$FFFF
    inc a
+   cmp.w SF_R
    bcs ST_PreNext
    lda.w SF_R
    clc
    adc.w SF_RHO
    sta.w SF_TMP
    lda.w SF_BLADE,y
    bpl +
    eor #$FFFF
    inc a
+   cmp.w SF_TMP
    bcs ST_PreNext
    lda.w SF_BLADE+4,y
    bpl +
    eor #$FFFF
    inc a
+   cmp.w SF_TMP
    bcc ST_Turn
ST_PreNext:
    tya
    clc
    adc #8
    tay
    lsr a
    lsr a
    lsr a
    cmp.w SF_BCOUNT
    bcc ST_Pre
    jmp ST_SphereNext
ST_Turn:
    stz.b tcc__r2
    stz.b tcc__r2h
    stz.b tcc__r3
    stz.b tcc__r3h
    stz.b tcc__r4
    stz.b tcc__r4h
    stz.b tcc__r5
    stz.b tcc__r5h
    ; x = (c x0 + s z0) >> 18, z = (c z0 - s x0) >> 18: r2/r3 are x's
    ; hi'/lo sums, r4/r5 z's.
    sep #$20
    lda.w SF_X0
    sta.l PPU_M7A
    lda.w SF_X0+1
    sta.l PPU_M7A
    ldx #tcc__r2
    lda.b tcc__r9+1
    jsr ST_MulAcc
    ldx #tcc__r3
    lda.b tcc__r9
    jsr ST_MulAcc
    lda.w SF_Z0
    sta.l PPU_M7A
    lda.w SF_Z0+1
    sta.l PPU_M7A
    ldx #tcc__r2
    lda.b tcc__r9h+1
    jsr ST_MulAcc
    ldx #tcc__r3
    lda.b tcc__r9h
    jsr ST_MulAcc
    ldx #tcc__r4
    lda.b tcc__r9+1
    jsr ST_MulAcc
    ldx #tcc__r5
    lda.b tcc__r9
    jsr ST_MulAcc
    rep #$20
    lda #0
    sec
    sbc.w SF_X0
    sep #$20
    sta.l PPU_M7A
    xba
    sta.l PPU_M7A
    ldx #tcc__r4
    lda.b tcc__r9h+1
    jsr ST_MulAcc
    ldx #tcc__r5
    lda.b tcc__r9h
    jsr ST_MulAcc
    rep #$20
    ldx #tcc__r2
    jsr ST_Combine
    sta.w SF_X
    ldx #tcc__r4
    jsr ST_Combine
    sta.w SF_Z

    ; Against each of the blade's spheres.
    ldy #0
ST_Pair:
    lda.w SF_BLADE+6,y
    bpl +
    jmp ST_PairNext
+   bne +
    jmp ST_PairNext
+   clc
    adc.w SF_RL
    sta.w SF_R
    lda.w SF_BLADE+2,y
    sec
    sbc.w SF_Y0
    bpl +
    eor #$FFFF
    inc a
+   cmp.w SF_R
    bcc +
    jmp ST_PairNext             ; dy >= r: apart whatever x and z are
+   sta.w SF_DY
    lda.w SF_R
    clc
    adc.b tcc__r10
    sta.w SF_TMP                ; r + bound
    lda.w SF_BLADE,y
    sec
    sbc.w SF_X
    bpl +
    eor #$FFFF
    inc a
+   cmp.w SF_TMP
    bcc +
    jmp ST_PairNext
+   sta.w SF_DX
    lda.w SF_BLADE+4,y
    sec
    sbc.w SF_Z
    bpl +
    eor #$FFFF
    inc a
+   cmp.w SF_TMP
    bcc +
    jmp ST_PairNext
+   sta.w SF_DZ
    phy
    ; r^2
    stz.w SF_SUM
    stz.w SF_SUM+2
    lda.w SF_R
    jsr ST_SqAdd
    lda.w SF_SUM
    sta.w SF_R2
    lda.w SF_SUM+2
    sta.w SF_R2+2
    ; the farthest: (dx + b)^2 + dy^2 + (dz + b)^2 < r^2 is a certain hit
    stz.w SF_SUM
    stz.w SF_SUM+2
    lda.w SF_DX
    clc
    adc.b tcc__r10
    jsr ST_SqAdd
    lda.w SF_DY
    jsr ST_SqAdd
    lda.w SF_DZ
    clc
    adc.b tcc__r10
    jsr ST_SqAdd
    jsr ST_SumBelowR2
    bcs +
    ply
    lda #1
    sta.b tcc__r0
    rtl
+   ; the nearest: max(dx - b, 0)^2 + dy^2 + max(dz - b, 0)^2 < r^2 is undecided
    stz.w SF_SUM
    stz.w SF_SUM+2
    lda.w SF_DX
    sec
    sbc.b tcc__r10
    bcs +
    lda #0
+   jsr ST_SqAdd
    lda.w SF_DY
    jsr ST_SqAdd
    lda.w SF_DZ
    sec
    sbc.b tcc__r10
    bcs +
    lda #0
+   jsr ST_SqAdd
    jsr ST_SumBelowR2
    bcs +
    lda #1
    sta.w SF_AMB
+   ply
ST_PairNext:
    tya
    clc
    adc #8
    tay
    lsr a
    lsr a
    lsr a
    cmp.w SF_BCOUNT
    bcs ST_SphereNext
    jmp ST_Pair

ST_SphereNext:
    lda.w SF_LMESH
    clc
    adc #16
    sta.w SF_LMESH
    lda.b tcc__r0
    clc
    adc #6
    sta.b tcc__r0
    dec.w SF_LCOUNT
    beq +
    jmp ST_Sphere
+   lda.w SF_AMB
    beq +
ST_Undecided:
    lda #2
+   sta.b tcc__r0
    rtl

; A8 = a signed multiplier byte, M7A loaded: the 24-bit product added to
; the 32-bit sum at direct page X. Returns A8; the carry survives the
; loads, XBA, AND, BIT and ORA between the two halves.
.accu 8
ST_MulAcc:
    sta.l PPU_M7B
    rep #$20
    lda.l PPU_MPYL
    clc
    adc.b 0,x
    sta.b 0,x
    lda.l PPU_MPYM              ; bits 8-23
    xba
    and #$00FF
    bit #$0080
    beq +
    ora #$FF00
+   adc.b 2,x
    sta.b 2,x
    sep #$20
    rts

; A16 = (hi' sum * 256 + lo sum) >> 18, the sums at direct page X and X+4.
.accu 16
ST_Combine:
    lda.b 0,x
    xba
    and #$FF00
    clc
    adc.b 4,x
    lda.b 1,x                   ; hi' sum bytes 1-2
    adc.b 6,x
    cmp #$8000
    ror a
    cmp #$8000
    ror a
    rts

; A16 = v, 0 <= v < 32640: SF_SUM += v * v.
ST_SqAdd:
    sta.w SF_TMP
    sep #$20
    sta.l PPU_M7A
    xba
    sta.l PPU_M7A
    lda.w SF_TMP                ; lo
    sta.l PPU_M7B
    rep #$20
    lda.l PPU_MPYL
    clc
    adc.w SF_SUM
    sta.w SF_SUM
    sep #$20
    lda.l PPU_MPYM+1            ; bits 16-23: a non-negative v times a
    rep #$20                    ; byte read signed can be negative
    and #$00FF
    bit #$0080
    beq +
    ora #$FF00
+   adc.w SF_SUM+2
    sta.w SF_SUM+2
    lda.w SF_TMP                ; hi' = high byte of v + $80, times 256
    clc
    adc #$0080
    xba
    sep #$20
    sta.l PPU_M7B
    rep #$20
    lda.l PPU_MPYL              ; v * hi', 24 bits, shifted up a byte
    xba
    and #$FF00
    clc
    adc.w SF_SUM
    sta.w SF_SUM
    lda.l PPU_MPYM
    adc.w SF_SUM+2
    sta.w SF_SUM+2
    rts

; Carry clear when SF_SUM < SF_R2 (unsigned 32-bit).
ST_SumBelowR2:
    lda.w SF_SUM+2
    cmp.w SF_R2+2
    bne +
    lda.w SF_SUM
    cmp.w SF_R2
+   rts

; ---------------------------------------------------------------------------
; The exact chain.
; ---------------------------------------------------------------------------
Trap_collideSpheres:
    lda 4,s
    sta.w SX_BLADE
    lda 6,s
    sta.w SX_BLADE+2
    lda 8,s
    sta.w SX_LARA
    lda 10,s
    sta.w SX_LARA+2
    lda.w SX_BLADE
    sta.b tcc__r0
    lda.w SX_BLADE+2
    sta.b tcc__r0h
    jsr SC_LoadItem
    stz.w SC_PITCH
    stz.w SC_OUT
    lda #SPH_BLADE
    sta.w SC_HDR
    jsr SC_Model
    sta.w SX_AN
    lda.w SX_LARA
    sta.b tcc__r0
    lda.w SX_LARA+2
    sta.b tcc__r0h
    jsr SC_LoadItem
    lda.w olLaraAngleX
    sta.w SC_PITCH
    lda #256
    sta.w SC_OUT
    lda #SPH_LARA
    sta.w SC_HDR
    jsr SC_Model
    sta.w SX_BN

    ; collideSpheres: a bit per blade sphere that meets any of hers
    stz.w SX_MASK
    stz.w SX_MASK+2
    lda #1
    sta.w SX_BIT
    stz.w SX_BIT+2
    stz.w SX_I
    lda.w SX_AN
    beq +
    lda.w SX_BN
    bne SX_Outer
+   jmp SX_Done
SX_Outer:
    ldx.w SX_I
    lda.w olSpheres+14,x        ; radius <= 0: skipped
    bmi SX_OuterSkip
    lda.w olSpheres+12,x
    bne +
SX_OuterSkip:
    jmp SX_OuterNext
+   sta.w SX_RA
    lda #256
    sta.w SX_J
SX_Inner:
    ldy.w SX_J
    lda.w olSpheres+14,y
    bmi SX_InnerNext
    lda.w olSpheres+12,y
    beq SX_InnerNext
    clc
    adc.w SX_RA
    sta.w SF_R
    stz.w SF_SUM
    stz.w SF_SUM+2
    ldx.w SX_I
    jsr SX_Axis                 ; x
    bcs SX_InnerNext
    inx
    inx
    inx
    inx
    iny
    iny
    iny
    iny
    jsr SX_Axis                 ; y
    bcs SX_InnerNext
    inx
    inx
    inx
    inx
    iny
    iny
    iny
    iny
    jsr SX_Axis                 ; z
    bcs SX_InnerNext
    lda.w SF_SUM
    sta.w SF_DX                 ; keep the sum while r^2 is formed
    lda.w SF_SUM+2
    sta.w SF_DY
    stz.w SF_SUM
    stz.w SF_SUM+2
    lda.w SF_R
    jsr ST_SqAdd
    lda.w SF_SUM
    sta.w SF_R2
    lda.w SF_SUM+2
    sta.w SF_R2+2
    lda.w SF_DX
    sta.w SF_SUM
    lda.w SF_DY
    sta.w SF_SUM+2
    jsr ST_SumBelowR2
    bcs SX_InnerNext
    lda.w SX_MASK               ; this sphere meets one of hers: no need
    ora.w SX_BIT                ; to ask the rest
    sta.w SX_MASK
    lda.w SX_MASK+2
    ora.w SX_BIT+2
    sta.w SX_MASK+2
    bra SX_OuterNext
SX_InnerNext:
    lda.w SX_J
    clc
    adc #16
    sta.w SX_J
    sec
    sbc #256
    lsr a
    lsr a
    lsr a
    lsr a
    cmp.w SX_BN
    bcs SX_OuterNext
    jmp SX_Inner
SX_OuterNext:
    asl.w SX_BIT
    rol.w SX_BIT+2
    lda.w SX_I
    clc
    adc #16
    sta.w SX_I
    lsr a
    lsr a
    lsr a
    lsr a
    cmp.w SX_AN
    bcs SX_Done
    jmp SX_Outer
SX_Done:
    lda.w SX_MASK+2
    sta.b tcc__r1
    lda.w SX_MASK
    sta.b tcc__r0
    rtl

; One axis of a pair: d = her centre (olSpheres+Y) - the blade's
; (olSpheres+X), 32-bit. Carry set when |d| >= r (they cannot meet);
; otherwise SF_SUM += d^2.
SX_Axis:
    lda.w olSpheres,y
    sec
    sbc.w olSpheres,x
    sta.w SC_T
    lda.w olSpheres+2,y
    sbc.w olSpheres+2,x
    bpl +
    eor #$FFFF                  ; negate the 32-bit difference
    sta.w SC_T2
    lda.w SC_T
    eor #$FFFF
    clc
    adc #1
    sta.w SC_T
    lda.w SC_T2
    adc #0
+   bne SX_Apart
    lda.w SC_T
    cmp.w SF_R
    bcs SX_Apart
    phx
    phy
    jsr ST_SqAdd
    ply
    plx
    clc
    rts
SX_Apart:
    sec
    rts

; tcc__r0 = a far item: its position, yaw, anim and frame for SC_Model.
SC_LoadItem:
    ldy #4
    lda [tcc__r0],y
    sta.w SC_PX
    ldy #6
    lda [tcc__r0],y
    sta.w SC_PY
    ldy #8
    lda [tcc__r0],y
    sta.w SC_PZ
    ldy #10
    lda [tcc__r0],y
    sta.w SC_YAW
    ldy #12
    lda [tcc__r0],y
    sta.w SC_ANIM
    ldy #14
    lda [tcc__r0],y
    and #$00FF
    sta.w SC_FIDX
    rts

; getSpheres(spheres, true) for the model at SC_HDR: its spheres into
; olSpheres+SC_OUT (int32 x, y, z, radius each). Returns A = the count
; (0 for an animation not its own).
SC_Model:
    ldx.w SC_HDR
    lda.l SPH+2,x
    sta.w SC_COUNT
    lda.w SC_ANIM
    sec
    sbc.l SPH+8,x
    cmp.l SPH+10,x
    bcc +
    lda #0
    rts
+   asl a
    asl a
    sta.w SC_T
    lda.l SPH+4,x
    sta.w SC_MESH
    lda.w SC_COUNT
    asl a
    asl a
    asl a
    asl a
    clc
    adc.w SC_T
    clc
    adc.w SC_MESH               ; its directory slot, bank 0
    tax
    lda.l SPH,x
    sta.b tcc__r0
    lda.l SPH+2,x
    clc
    adc #OL_SPHERE_BANK
    sta.b tcc__r0h
    lda [tcc__r0]
    dec a
    sta.w SC_T                  ; last tick
    lda.w SC_FIDX
    cmp.w SC_T
    bcc +
    lda.w SC_T
+   asl a
    asl a
    inc a
    inc a
    tay
    lda [tcc__r0],y
    sta.w SC_FRAME
    iny
    iny
    lda [tcc__r0],y
    clc
    adc #OL_SPHERE_BANK
    sta.w SC_FRAME+2
    ; matrixSetIdentity, matrixRotateYXZ(pitch, yaw, 0)
    stz.w SC_TOP
    ldx #0
-   stz.w olSphStack,x
    inx
    inx
    cpx #36
    bcc -
    lda #$4000
    sta.w olSphStack+0
    sta.w olSphStack+12+2
    sta.w olSphStack+24+4
    lda.w SC_PITCH
    sta.w SC_AX
    lda.w SC_YAW
    sta.w SC_AY
    stz.w SC_AZ
    jsr SC_RotYXZ
    ; matrixFrame(root offset, angles[0])
    lda.w SC_FRAME
    sta.b tcc__r2
    lda.w SC_FRAME+2
    sta.b tcc__r2h
    lda [tcc__r2]
    sta.w SC_VX
    ldy #2
    lda [tcc__r2],y
    sta.w SC_VY
    ldy #4
    lda [tcc__r2],y
    sta.w SC_VZ
    lda #0
    jsr SC_Frame
    stz.w SC_I
SC_Mesh:
    lda.w SC_I
    beq SC_Sphere
    ldx.w SC_MESH
    lda.l SPH+8,x               ; the node's flags: pop, push
    lsr a
    bcc +
    jsr SC_Pop
+   ldx.w SC_MESH
    lda.l SPH+8,x
    and #2
    beq +
    jsr SC_Push
+   ldx.w SC_MESH               ; matrixFrame(node offset, angles[i])
    lda.l SPH+10,x
    sta.w SC_VX
    lda.l SPH+12,x
    sta.w SC_VY
    lda.l SPH+14,x
    sta.w SC_VZ
    lda.w SC_I
    jsr SC_Frame
SC_Sphere:
    jsr SC_Push
    ldx.w SC_MESH
    lda.l SPH+0,x
    sta.w SC_VX
    lda.l SPH+2,x
    sta.w SC_VY
    lda.l SPH+4,x
    sta.w SC_VZ
    jsr SC_Translate
    ldx.w SC_TOP
    ldy.w SC_OUT
    lda.w SC_PX
    sta.w SC_VX
    jsr SC_Centre
    lda.w SC_PY
    sta.w SC_VX
    jsr SC_Centre
    lda.w SC_PZ
    sta.w SC_VX
    jsr SC_Centre
    ldx.w SC_MESH
    lda.l SPH+6,x               ; the radius, sign-extended
    sta.w olSpheres,y
    asl a
    lda #0
    adc #$FFFF
    eor #$FFFF
    sta.w olSpheres+2,y
    jsr SC_Pop
    lda.w SC_OUT
    clc
    adc #16
    sta.w SC_OUT
    lda.w SC_MESH
    clc
    adc #16
    sta.w SC_MESH
    inc.w SC_I
    lda.w SC_I
    cmp.w SC_COUNT
    bcs +
    jmp SC_Mesh
+   lda.w SC_COUNT
    rts

; olSpheres+Y = SC_VX + (the row at olSphStack+X's e3 >> 14), 32-bit;
; X and Y step to the next row and output word.
SC_Centre:
    lda.w olSphStack+8,x
    sta.w SC_T
    lda.w olSphStack+10,x
    asl.w SC_T
    rol a
    asl.w SC_T
    rol a                       ; bits 14-29: the offset, which fits 16 bits
    sta.w SC_T2
    asl a
    lda #0
    adc #$FFFF
    eor #$FFFF
    sta.w SC_T                  ; its sign word
    lda.w SC_VX
    asl a
    lda #0
    adc #$FFFF
    eor #$FFFF                  ; the position's
    pha
    lda.w SC_VX
    clc
    adc.w SC_T2
    sta.w olSpheres,y
    pla
    adc.w SC_T
    sta.w olSpheres+2,y
    txa
    clc
    adc #12
    tax
    iny
    iny
    iny
    iny
    rts

; matrixFrame: SC_VX-VZ the offset, A = i picks the frame's angles[i];
; translate, then rotate.
SC_Frame:
    asl a
    asl a
    clc
    adc #6
    tay
    lda.w SC_FRAME
    sta.b tcc__r2
    lda.w SC_FRAME+2
    sta.b tcc__r2h
    lda [tcc__r2],y
    sta.w SC_ANGLO
    iny
    iny
    lda [tcc__r2],y
    sta.w SC_ANGHI
    jsr SC_Translate
    lda.w SC_ANGHI              ; DECODE_ANGLES
    and #$3FF0
    asl a
    asl a
    sta.w SC_AX
    lda.w SC_ANGHI
    and #$000F
    xba
    asl a
    asl a
    asl a
    asl a
    sta.w SC_T
    lda.w SC_ANGLO
    and #$FC00
    lsr a
    lsr a
    lsr a
    lsr a
    ora.w SC_T
    sta.w SC_AY
    lda.w SC_ANGLO
    and #$03FF
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    sta.w SC_AZ
    ; fall into SC_RotYXZ

; matrixRotateYXZ(SC_AX, SC_AY, SC_AZ): Y on columns 0/2, X on 2/1, Z on
; 1/0, each skipped when zero.
SC_RotYXZ:
    lda.w SC_AY
    beq +
    stz.w SC_CA
    ldx #4
    stx.w SC_CB
    jsr SC_Rot
+   lda.w SC_AX
    beq +
    ldx #4
    stx.w SC_CA
    ldx #2
    stx.w SC_CB
    jsr SC_Rot
+   lda.w SC_AZ
    beq +
    ldx #2
    stx.w SC_CA
    stz.w SC_CB
    jsr SC_Rot
+   rts

; X_ROTXY on columns SC_CA, SC_CB of each row: a' = (a c - b s) >> 14,
; b' = (b c + a s) >> 14, both from the old. A = the angle.
SC_Rot:
    lsr a
    lsr a
    and #$FFFC
    tax
    lda.l SPH,x
    jsr SC_Split
    sta.w SC_C
    lda.l SPH+2,x
    pha
    jsr SC_Split
    sta.w SC_S
    pla
    eor #$FFFF
    inc a
    jsr SC_Split
    sta.w SC_NS
    lda.w SC_TOP
    clc
    adc #olSphStack&$FFFF
    pha
    clc
    adc.w SC_CA
    sta.b tcc__r0
    pla
    clc
    adc.w SC_CB
    sta.b tcc__r1
    ldy #0
SC_RotRow:
    stz.b tcc__r2
    stz.b tcc__r2h
    stz.b tcc__r3
    stz.b tcc__r3h
    stz.b tcc__r4
    stz.b tcc__r4h
    stz.b tcc__r5
    stz.b tcc__r5h
    lda (tcc__r0),y
    sep #$20
    sta.l PPU_M7A
    xba
    sta.l PPU_M7A
    ldx #tcc__r2                ; a' += a c
    lda.w SC_C+1
    jsr ST_MulAcc
    ldx #tcc__r3
    lda.w SC_C
    jsr ST_MulAcc
    ldx #tcc__r4                ; b' += a s
    lda.w SC_S+1
    jsr ST_MulAcc
    ldx #tcc__r5
    lda.w SC_S
    jsr ST_MulAcc
    rep #$20
    lda (tcc__r1),y
    sep #$20
    sta.l PPU_M7A
    xba
    sta.l PPU_M7A
    ldx #tcc__r2                ; a' += b (-s)
    lda.w SC_NS+1
    jsr ST_MulAcc
    ldx #tcc__r3
    lda.w SC_NS
    jsr ST_MulAcc
    ldx #tcc__r4                ; b' += b c
    lda.w SC_C+1
    jsr ST_MulAcc
    ldx #tcc__r5
    lda.w SC_C
    jsr ST_MulAcc
    rep #$20
    ldx #tcc__r2
    jsr SC_Combine14
    sta (tcc__r0),y
    ldx #tcc__r4
    jsr SC_Combine14
    sta (tcc__r1),y
    tya
    clc
    adc #12
    tay
    cpy #36
    bcs +
    jmp SC_RotRow
+   rts

; MATRIX_TRANS then e3 += it, per row: e3 += e0 x + e1 y + e2 z, with
; x, y, z = SC_VX-VZ.
SC_Translate:
    lda.w SC_VX
    jsr SC_Split
    sta.w SC_XS
    lda.w SC_VY
    jsr SC_Split
    sta.w SC_YS
    lda.w SC_VZ
    jsr SC_Split
    sta.w SC_ZS
    lda.w SC_TOP
    clc
    adc #olSphStack&$FFFF
    sta.b tcc__r0
    ldy #0
SC_TrRow:
    stz.b tcc__r2
    stz.b tcc__r2h
    stz.b tcc__r3
    stz.b tcc__r3h
    lda (tcc__r0),y
    sep #$20
    sta.l PPU_M7A
    xba
    sta.l PPU_M7A
    ldx #tcc__r2
    lda.w SC_XS+1
    jsr ST_MulAcc
    ldx #tcc__r3
    lda.w SC_XS
    jsr ST_MulAcc
    rep #$20
    iny
    iny
    lda (tcc__r0),y
    sep #$20
    sta.l PPU_M7A
    xba
    sta.l PPU_M7A
    ldx #tcc__r2
    lda.w SC_YS+1
    jsr ST_MulAcc
    ldx #tcc__r3
    lda.w SC_YS
    jsr ST_MulAcc
    rep #$20
    iny
    iny
    lda (tcc__r0),y
    sep #$20
    sta.l PPU_M7A
    xba
    sta.l PPU_M7A
    ldx #tcc__r2
    lda.w SC_ZS+1
    jsr ST_MulAcc
    ldx #tcc__r3
    lda.w SC_ZS
    jsr ST_MulAcc
    rep #$20
    iny
    iny
    iny
    iny                         ; e3
    lda.b tcc__r2               ; the sum: hi' part * 256 + lo part
    xba
    and #$FF00
    clc
    adc.b tcc__r3
    sta.w SC_T
    lda.b tcc__r2+1
    adc.b tcc__r3h
    sta.w SC_T2
    lda.w SC_T
    clc
    adc (tcc__r0),y
    sta (tcc__r0),y
    iny
    iny
    lda.w SC_T2
    adc (tcc__r0),y
    sta (tcc__r0),y
    iny
    iny
    cpy #36
    bcs +
    jmp SC_TrRow
+   rts

; A = v: hi' << 8 | lo, hi' the high byte of v + $80 (the PPU reads the
; multiplier byte signed, so lo's sign is borrowed from hi').
SC_Split:
    sta.w SC_T
    clc
    adc #$0080
    and #$FF00
    sta.w SC_T2
    lda.w SC_T
    and #$00FF
    ora.w SC_T2
    rts

; A = bits 14-29 of hi' sum * 256 + lo sum (direct page X, X+4): the
; product sum >> 14, which fits 16 bits.
SC_Combine14:
    lda.b 0,x
    xba
    and #$FF00
    clc
    adc.b 4,x
    sta.w SC_T
    lda.b 1,x
    adc.b 6,x
    asl.w SC_T
    rol a
    asl.w SC_T
    rol a
    rts

SC_Push:
    lda.w SC_TOP
    cmp #7*36
    bcs +                       ; as src/trap.c's C did: no deeper than 8
    clc
    adc #olSphStack&$FFFF
    tax
    clc
    adc #36
    tay
    lda #35
    mvn $7E, $7E
    lda.w SC_TOP
    clc
    adc #36
    sta.w SC_TOP
+   rts

SC_Pop:
    lda.w SC_TOP
    beq +
    sec
    sbc #36
    sta.w SC_TOP
+   rts
.ENDIF

.ENDS
.ENDIF
.ENDIF

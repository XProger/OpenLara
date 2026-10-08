; Native rotated bounds, collision probes and room wrappers on both variants.
; Complete C bodies remain in C_MOTION cartridges as an executable oracle.
; FX3 executes in FastROM; GSU-2 copies bounds/probes into resident WRAM.
; CollisionInfo, side and floor pointers retain the C far-pointer ABI.
; A/X/Y 16, D=0, DB=$7E, JSL. NMI never enters collision or its workspace.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF C_MOTION
.IFDEF FX3
.BASE $80
.BANK 9 SLOT 0                  ; $89 (was $BF)
.ELSE
.BASE $7B
.BANK 4 SLOT 4                  ; resident $7F:0000 code image
.ENDIF
.SECTION "collision_fx3_native" SEMIFREE
.accu 16
.index 16
.16bit

; Item/Lara far pointers at 4/8,s, radius 12,s, height 14,s. Preserve the
; original wrapped word boundaries, strict vertical and inclusive X/Z tests.
; Four signed products stay 32-bit until after subtraction/addition; shifting
; each factor separately would change contact at negative rounding seams.
ItemObj_collideBounds:
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    lda 8,s
    sta.b tcc__r2
    lda 10,s
    sta.b tcc__r2h
    ldy #12
    lda [tcc__r0],y
    asl a
    asl a
    sta.b tcc__r9
    asl a
    clc
    adc.b tcc__r9
    clc
    adc.w olAnimBoxBase
    sta.b tcc__r1
.IFDEF FX3
    lda.b olAnimBoxData+2
.ELSE
    lda #$71                   ; the legacy packed SRAM animation-box table
.ENDIF
    sta.b tcc__r1h
    ldy #6
    lda [tcc__r2],y
    sec
    sbc [tcc__r0],y
    eor #$8000
    sta.b tcc__r3
    lda [tcc__r1],y             ; maxY + height <= dy rejects contact
    clc
    adc 14,s
    eor #$8000
    cmp.b tcc__r3
    beq IB_No
    bcc IB_No
    ldy #4
    lda [tcc__r1],y             ; minY >= dy rejects contact
    eor #$8000
    cmp.b tcc__r3
    bcs IB_No
    lda [tcc__r2],y
    sec
    sbc [tcc__r0],y
    sta.b tcc__r3
    ldy #8
    lda [tcc__r2],y
    sec
    sbc [tcc__r0],y
    sta.b tcc__r3h
    ldy #10
    lda [tcc__r0],y
    clc
    adc #32
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    and #$07FE
    tax
    lda.l olSinTable,x
    sta.b tcc__r4
    txa
    clc
    adc #512
    and #$07FE
    tax
    lda.l olSinTable,x
    sta.b tcc__r4h
    jmp IB_RotateX
IB_No:
    lda #0
    sta.b tcc__r0
    rtl
IB_RotateX:
    lda.b tcc__r3
    ldy.b tcc__r4h
    jsr IB_Multiply
    sty.b tcc__r5
    stx.b tcc__r5h
    lda.b tcc__r3h
    ldy.b tcc__r4
    jsr IB_Multiply
    sty.b tcc__r9
    stx.b tcc__r9h
    lda.b tcc__r5
    sec
    sbc.b tcc__r9
    sta.b tcc__r5
    lda.b tcc__r5h
    sbc.b tcc__r9h
    sta.b tcc__r5h
    jsr IB_Shift
    ldy #0
    lda [tcc__r1],y
    sec
    sbc 12,s
    eor #$8000
    cmp.b tcc__r5h
    beq +
    bcs IB_No
+   ldy #2
    lda [tcc__r1],y
    clc
    adc 12,s
    eor #$8000
    cmp.b tcc__r5h
    bcc IB_No
    lda.b tcc__r3h
    ldy.b tcc__r4h
    jsr IB_Multiply
    sty.b tcc__r5
    stx.b tcc__r5h
    lda.b tcc__r3
    ldy.b tcc__r4
    jsr IB_Multiply
    tya
    clc
    adc.b tcc__r5
    sta.b tcc__r5
    txa
    adc.b tcc__r5h
    sta.b tcc__r5h
    jsr IB_Shift
    ldy #8
    lda [tcc__r1],y
    sec
    sbc 12,s
    eor #$8000
    cmp.b tcc__r5h
    beq +
    bcc +
    jmp IB_No
+   ldy #10
    lda [tcc__r1],y
    clc
    adc 12,s
    eor #$8000
    cmp.b tcc__r5h
    bcs +
    jmp IB_No
+   lda #1
    sta.b tcc__r0
    rtl

IB_Shift:
    lda.b tcc__r5
    asl a
    rol.b tcc__r5h
    asl a
    rol.b tcc__r5h
    lda.b tcc__r5h
    eor #$8000
    sta.b tcc__r5h              ; signed compare bias after extracting bits 14..29
    rts
IB_Multiply:
    sta.b tcc__r9
    tya
    sta.b tcc__r10
    eor.b tcc__r9
    php                        ; product sign survives the runtime multiply
    lda.b tcc__r9
    bpl +
    eor #$FFFF
    inc a
+   sta.b tcc__r9
    stz.b tcc__r9h
    lda.b tcc__r10
    bpl +
    eor #$FFFF
    inc a
+   sta.b tcc__r10
    stz.b tcc__r10h
    jsr.l tcc__mull             ; preserves all r0..r5 halves, returns X:Y
    plp
    bpl +
    tya
    eor #$FFFF
    clc
    adc #1
    tay
    txa
    eor #$FFFF
    adc #0
    tax
+   rts

CollisionInfo_setAngle:
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    lda 8,s
    ldy #46
    sta [tcc__r0],y
    clc
    adc #$2000
    asl a
    rol a
    rol a
    and #3
    ldy #48
    sta [tcc__r0],y
    rtl

; ci 4/6,s, side 8/10,s, floor 12,s, ceiling 14,s,
; slope X/Z 16/18,s, middle 20,s. Side padding is not an output.
CollisionInfo_setSide:
    stz.b tcc__r2
    lda 16,s
    ora 18,s
    beq CS_Classified
    inc.b tcc__r2
    lda 16,s
    bpl +
    eor #$FFFF
    inc a
+   bmi CS_LowX                 ; match 16-bit C abs(-32768)'s wrap
    cmp #3
    bcs CS_High
CS_LowX:
    lda 18,s
    bpl +
    eor #$FFFF
    inc a
+   bmi CS_Classified
    cmp #3
    bcc CS_Classified
CS_High:
    inc.b tcc__r2
CS_Classified:
    lda 12,s
    sta.b tcc__r3
    lda 20,s
    bne CS_Store
    lda.b tcc__r2
    cmp #2
    bne CS_Store
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    ldy #53
    lda [tcc__r0],y
    and #$00FF
    beq CS_Store
    lda.b tcc__r3
    beq CS_Store
    bmi +
    lda #512
    bra ++
+   lda #$8001
++  sta.b tcc__r3
CS_Store:
    lda 8,s
    sta.b tcc__r0
    lda 10,s
    sta.b tcc__r0h
    lda.b tcc__r3
    sta [tcc__r0]
    ldy #2
    lda 14,s
    sta [tcc__r0],y
    ldy #4
    lda.b tcc__r2
    sep #$20
    sta [tcc__r0],y
    rep #$20
    rtl

; Six local bytes: list index 1,s, signed sector X/Z 3/5,s.
; x 10,s, y 12,s, z 14,s, far floor output 16/18,s.
Lara_itemFloorAt:
    lda.w olBridgeCount
    bne +
    rtl
+   tsc
    sec
    sbc #6
    tcs
    lda 10,s
    jsr CF_Sector
    sta 3,s
    eor #$8000
    sta.b tcc__r2
    lda 14,s
    jsr CF_Sector
    sta 5,s
    eor #$8000
    sta.b tcc__r2h
    lda.w olBridgeBox
    eor #$8000
    cmp.b tcc__r2
    bcc +
    beq +
    jmp CF_Return
+   lda.w olBridgeBox+2
    eor #$8000
    cmp.b tcc__r2
    bcs +
    jmp CF_Return
+   lda.w olBridgeBox+4
    eor #$8000
    cmp.b tcc__r2h
    bcc +
    beq +
    jmp CF_Return
+   lda.w olBridgeBox+6
    eor #$8000
    cmp.b tcc__r2h
    bcs +
    jmp CF_Return
+   lda #0
    sta 1,s
CF_Loop:
    lda 1,s
    cmp.w olBridgeCount
    bcc +
    jmp CF_Return
+   tax
    lda.w olBridges,x
    and #$00FF
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #(items & $FFFF)
    tax
.IFDEF FX3
    lda.w 0,x
    and #$00FF
    cmp #41                    ; drawbridge: two forward sectors
    beq CF_Platform
    cmp #65                    ; closed trapdoors: own + rear sector
    beq CF_Platform
    cmp #66
    beq CF_Platform
.ENDIF
    lda.w 4,x
    jsr CF_Sector
    cmp 3,s
    bne CF_Next
    lda.w 8,x
    jsr CF_Sector
    cmp 5,s
    bne CF_Next
CF_Platform:
    lda 14,s
    pha
    lda 12,s
    pha
    lda #$7E
    pha
    phx
    jsr.l ItemObj_getBridgeFloor
    tsc
    clc
    adc #8
    tcs
    lda.b tcc__r0
.IFDEF FX3
    cmp #$8100                 ; WALL: no floor on a raised/open platform
    beq CF_Next
.ENDIF
    sta.b tcc__r2
    eor #$8000
    sta.b tcc__r2h
    lda 12,s
    eor #$8000
    cmp.b tcc__r2h
    bcc +
    bne CF_Next
+   lda 16,s
    sta.b tcc__r0
    lda 18,s
    sta.b tcc__r0h
    lda [tcc__r0]
    eor #$8000
    cmp.b tcc__r2h
    bcc CF_Next
    lda.b tcc__r2
    sta [tcc__r0]
CF_Next:
    lda 1,s
    inc a
    sta 1,s
    jmp CF_Loop
CF_Return:
    tsc
    clc
    adc #6
    tcs
    rtl

; Exact arithmetic word >> 10, independent of carry; X/Y are preserved.
CF_Sector:
    eor #$8000
    xba
    and #$00FC
    lsr a
    lsr a
    sec
    sbc #32
    rts

; The 20-byte RoomQuery is the entire local frame. Its floor/ceiling slots
; become relative results after the query; the slope slots remain intact.
; ci 24/26,s, side 28/30,s, x 32,s, cy 34,s, z 36,s,
; py 38,s, y 40,s, middle 42,s.
colPoint:
    tsc
    sec
    sbc #20
    tcs
    lda #$7E
    pha
    tsc
    clc
    adc #3
    pha
    lda 40,s
    pha
    lda 40,s
    pha
    lda 40,s
    pha
    lda.w olColRoom
    pha
    jsr.l Room_query
    tsc
    clc
    adc #12
    tcs
    lda.b tcc__r0
    bne CP_Query
    lda #$8100                 ; WALL
    sta 3,s
    sta 5,s
    lda #0
    sta 13,s
    sta 15,s
    lda #$FFFF
    sta.w olColTrigger
    bra CP_Side
CP_Query:
    lda 1,s
    sta.w olColRoom
    lda 11,s
    sta.w olColTrigger
    lda 3,s
    cmp #$7FFF                 ; OL_NO_FLOOR
    beq CP_NoFloor
    lda #$7E
    pha
    tsc
    clc
    adc #5
    pha
    lda 40,s
    pha
    lda 40,s
    pha
    lda 40,s
    pha
    jsr.l Lara_itemFloorAt
    tsc
    clc
    adc #10
    tcs
    lda 3,s
    sec
    sbc 38,s
    bra +
CP_NoFloor:
    lda #$8100
+   sta 3,s
    lda 5,s
    cmp #$8100
    beq CP_Side
    sec
    sbc 40,s
    sta 5,s
CP_Side:
    lda 42,s
    beq +
    lda 13,s
    sta.w olColSlantX
    lda 15,s
    sta.w olColSlantZ
+   lda 42,s
    pha
    lda 17,s
    pha
    lda 17,s
    pha
    lda 11,s
    pha
    lda 11,s
    pha
    lda 40,s
    pha
    lda 40,s
    pha
    lda 40,s
    pha
    lda 40,s
    pha
    jsr.l CollisionInfo_setSide
    tsc
    clc
    adc #38                    ; eighteen argument bytes + local RoomQuery
    tcs
    rtl

; Complete room-collision wrapper: keep the C probe order (including all
; four zero-radius probes), the front-only mode, and the C static response.
; Only the coordinate setup / final decisions move here. No persistent
; cache, FX ROM-bank conversion, or ROM/WRAM ownership handoff is involved.
; The 30-byte frame replaces C's 96 bytes. Item/ci remain full far pointers.
.DEFINE IR_LOCALS 30
.DEFINE IR_PX 1
.DEFINE IR_PY 3
.DEFINE IR_PZ 5
.DEFINE IR_Y 7
.DEFINE IR_CY 9
.DEFINE IR_FX 11
.DEFINE IR_FZ 13
.DEFINE IR_LX 15
.DEFINE IR_LZ 17
.DEFINE IR_RX 19
.DEFINE IR_RZ 21
.DEFINE IR_DX 23
.DEFINE IR_DY 25
.DEFINE IR_DZ 27
.DEFINE IR_R 29
ItemObj_collideRoom:
    tsc
    sec
    sbc #IR_LOCALS
    tcs
    lda IR_LOCALS+4,s
    sta.b tcc__r0
    lda IR_LOCALS+6,s
    sta.b tcc__r0h
    ldy #4
    lda [tcc__r0],y
    sta IR_PX,s
    ldy #6
    lda [tcc__r0],y
    clc
    adc IR_LOCALS+14,s
    sta IR_PY,s
    sec
    sbc IR_LOCALS+12,s
    sta IR_Y,s
    sec
    sbc #160
    sta IR_CY,s
    ldy #8
    lda [tcc__r0],y
    sta IR_PZ,s
    ldy #1
    lda [tcc__r0],y
    and #$FF
    sta.w olColRoom
    lda IR_LOCALS+8,s
    sta.b tcc__r0
    lda IR_LOCALS+10,s
    sta.b tcc__r0h
    ldy #26
    lda [tcc__r0],y
    sta IR_R,s
    lda #0
    ldy #34
    sta [tcc__r0],y
    iny
    iny
    sta [tcc__r0],y
    iny
    iny
    sta [tcc__r0],y
    sep #$20
    ldy #50
    sta [tcc__r0],y
    rep #$20
    lda IR_PX,s
    tax
    lda IR_PZ,s
    tay
    lda #2
    jsr IR_Point
    lda IR_LOCALS+8,s
    sta.b tcc__r0
    lda IR_LOCALS+10,s
    sta.b tcc__r0h
    lda.w olColTrigger
    sta [tcc__r0]
    sep #$20
    ldy #51
    lda.w olColSlantX
    sta [tcc__r0],y
    iny
    lda.w olColSlantZ
    sta [tcc__r0],y
    rep #$20
    lda IR_R,s
    bne IR_Radius
    ; Zero-radius push guards still visit m/f/l/r separately: room traversal
    ; is sequential, and non-middle stopOnSlant floors differ from m.floor.
    lda IR_PX,s
    sta IR_FX,s
    sta IR_LX,s
    sta IR_RX,s
    lda IR_PZ,s
    sta IR_FZ,s
    sta IR_LZ,s
    sta IR_RZ,s
    jmp IR_Coordinates
IR_Radius:
    ldy #48
    lda [tcc__r0],y
    sta.b tcc__r3
    ldy #46
    lda [tcc__r0],y
    clc
    adc #32
    ldx.b tcc__r3
    beq +
    cpx #2
    beq +
    clc
    adc #$4000
+   lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    and #$07FE
    tax
    lda.l olSinTable,x
    tay
    lda IR_R,s
    jsr IB_Multiply             ; signed R * sin/cos, full 32-bit result
    stx.b tcc__r4
    tya
    asl a
    rol.b tcc__r4
    asl a
    rol.b tcc__r4               ; exact low word of arithmetic >> 14
    lda IR_PX,s
    clc
    adc IR_R,s
    sta.b tcc__r1               ; X+R / X-R / Z+R / Z-R
    lda IR_PX,s
    sec
    sbc IR_R,s
    sta.b tcc__r1h
    lda IR_PZ,s
    clc
    adc IR_R,s
    sta.b tcc__r2
    lda IR_PZ,s
    sec
    sbc IR_R,s
    sta.b tcc__r2h
    lda.b tcc__r3
    beq IR_Q0
    cmp #1
    beq IR_Q1
    cmp #2
    beq IR_Q2
    ; The port's C default is quadrant 3, also for out-of-range inputs.
    lda.b tcc__r1h
    sta IR_FX,s
    sta IR_LX,s
    sta IR_RX,s
    lda.b tcc__r2h
    sta IR_LZ,s
    lda.b tcc__r2
    sta IR_RZ,s
    bra IR_FrontZ
IR_Q1:
    lda.b tcc__r1
    sta IR_FX,s
    sta IR_LX,s
    sta IR_RX,s
    lda.b tcc__r2
    sta IR_LZ,s
    lda.b tcc__r2h
    sta IR_RZ,s
IR_FrontZ:
    lda.b tcc__r4
    clc
    adc IR_PZ,s
    sta IR_FZ,s
    bra IR_Coordinates
IR_Q0:
    lda.b tcc__r1h
    sta IR_LX,s
    lda.b tcc__r1
    sta IR_RX,s
    lda.b tcc__r2
    bra IR_FrontX
IR_Q2:
    lda.b tcc__r1
    sta IR_LX,s
    lda.b tcc__r1h
    sta IR_RX,s
    lda.b tcc__r2h
IR_FrontX:
    sta IR_FZ,s
    sta IR_LZ,s
    sta IR_RZ,s
    lda.b tcc__r4
    clc
    adc IR_PX,s
    sta IR_FX,s
IR_Coordinates:
    ldy #40
    lda [tcc__r0],y
    sec
    sbc IR_PX,s
    sta IR_DX,s
    iny
    iny
    lda [tcc__r0],y
    sec
    sbc IR_PY,s
    sta IR_DY,s
    iny
    iny
    lda [tcc__r0],y
    sec
    sbc IR_PZ,s
    sta IR_DZ,s
    lda IR_FX,s
    tax
    lda IR_FZ,s
    tay
    lda #8
    jsr IR_Point
    lda.w olColFrontOnly
    bne IR_Static
    lda IR_LX,s
    tax
    lda IR_LZ,s
    tay
    lda #14
    jsr IR_Point
    lda IR_RX,s
    tax
    lda IR_RZ,s
    tay
    lda #20
    jsr IR_Point
IR_Static:
    lda IR_LOCALS+4,s
    sta.b tcc__r0
    lda IR_LOCALS+6,s
    sta.b tcc__r0h
    ldy #1
    lda [tcc__r0],y
    and #$FF
    sta.b tcc__r2
    lda IR_LOCALS+12,s
    pha
    lda IR_PZ+2,s
    pha
    lda IR_PY+4,s
    pha
    lda IR_PX+6,s
    pha
    lda IR_LOCALS+18,s
    pha
    lda IR_LOCALS+18,s
    pha
    pei (tcc__r2)
    jsr.l Room_collideStatic
    tsc
    clc
    adc #14
    tcs
    lda IR_LOCALS+8,s
    sta.b tcc__r0
    lda IR_LOCALS+10,s
    sta.b tcc__r0h
    ldy #2
    lda [tcc__r0],y
    cmp #$8100
    bne +
    lda #1                     ; CT_FRONT, restoring all original offsets
    jmp IR_Restore
+   eor #$8000
    sta.b tcc__r1
    ldy #4
    lda [tcc__r0],y
    eor #$8000
    cmp.b tcc__r1
    bcc +
    lda #32                    ; CT_FLOOR_CEILING
    jmp IR_Restore
+   eor #$8000
    bmi +
    ldy #36
    sta [tcc__r0],y
    ldy #50
    sep #$20
    lda #8                     ; CT_CEILING; later checks may replace it
    sta [tcc__r0],y
    rep #$20
+   ldy #28
    lda [tcc__r0],y
    eor #$8000
    sta.b tcc__r1
    iny
    iny
    lda [tcc__r0],y
    eor #$8000
    sta.b tcc__r1h
    iny
    iny
    lda [tcc__r0],y
    eor #$8000
    sta.b tcc__r2
    ldy #48
    lda [tcc__r0],y
    and #1
    sta.b tcc__r3
    ldy #8
    lda [tcc__r0],y
    eor #$8000
    cmp.b tcc__r1
    beq +
    bcs IR_Front
+   cmp.b tcc__r1h
    bcc IR_Front
    ldy #10
    lda [tcc__r0],y
    eor #$8000
    cmp.b tcc__r2
    bcc IR_Sides
    bne IR_Front
    lda #16                    ; CT_FRONT_CEILING: >= after the > check
    jmp IR_Restore
IR_Front:
    lda.b tcc__r3
    beq IR_FrontEven
    lda IR_DZ,s
    ldy #38
    sta [tcc__r0],y
    lda IR_PX,s
    tax
    lda IR_FX,s
    ldy #34
    bra IR_FrontAlign
IR_FrontEven:
    lda IR_DX,s
    ldy #34
    sta [tcc__r0],y
    lda IR_PZ,s
    tax
    lda IR_FZ,s
    ldy #38
IR_FrontAlign:
    jsr IR_Align
    sta [tcc__r0],y
    lda #1
    jmp IR_Type
IR_Sides:
    lda.w olColFrontOnly
    beq +
    jmp IR_Return
+   ldy #14
    lda [tcc__r0],y
    eor #$8000
    cmp.b tcc__r1
    beq +
    bcs IR_Left
+   cmp.b tcc__r1h
    bcs IR_RightTest
IR_Left:
    lda.b tcc__r3
    beq IR_LeftEven
    lda IR_FZ,s
    tax
    lda IR_LZ,s
    ldy #38
    bra IR_LeftAlign
IR_LeftEven:
    lda IR_FX,s
    tax
    lda IR_LX,s
    ldy #34
IR_LeftAlign:
    jsr IR_Align
    sta [tcc__r0],y
    lda #2
    bra IR_Type
IR_RightTest:
    ldy #20
    lda [tcc__r0],y
    eor #$8000
    cmp.b tcc__r1
    beq +
    bcs IR_Right
+   cmp.b tcc__r1h
    bcs IR_Return
IR_Right:
    lda.b tcc__r3
    beq IR_RightEven
    lda IR_FZ,s
    tax
    lda IR_RZ,s
    ldy #38
    bra IR_RightAlign
IR_RightEven:
    lda IR_FX,s
    tax
    lda IR_RX,s
    ldy #34
IR_RightAlign:
    jsr IR_Align
    sta [tcc__r0],y
    lda #4
    bra IR_Type
IR_Restore:
    sta.b tcc__r4
    lda IR_DX,s
    ldy #34
    sta [tcc__r0],y
    lda IR_DY,s
    iny
    iny
    sta [tcc__r0],y
    lda IR_DZ,s
    iny
    iny
    sta [tcc__r0],y
    lda.b tcc__r4
IR_Type:
    ldy #50
    sep #$20
    sta [tcc__r0],y
    rep #$20
IR_Return:
    tsc
    clc
    adc #IR_LOCALS
    tcs
    rtl

; A=side offset, X/Z point in X/Y. Additional JSR return adds two bytes
; to every local/argument offset. Side pointer addition carries its bank.
IR_Point:
    sta.b tcc__r3
    stx.b tcc__r2
    sty.b tcc__r2h
    lda IR_LOCALS+10,s
    sta.b tcc__r0
    clc
    adc.b tcc__r3
    sta.b tcc__r1
    lda IR_LOCALS+12,s
    sta.b tcc__r0h
    adc #0
    sta.b tcc__r1h
    lda.b tcc__r3
    cmp #2
    beq +
    lda #0
    bra ++
+   lda #1
++  pha
    lda IR_Y+4,s
    pha
    lda IR_PY+6,s
    pha
    pei (tcc__r2h)
    lda IR_CY+10,s
    pha
    pei (tcc__r2)
    pei (tcc__r1h)
    pei (tcc__r1)
    pei (tcc__r0h)
    pei (tcc__r0)
    jsr.l colPoint
    tsc
    clc
    adc #20
    tcs
    rts

; alignOffset(A, X), preserving Y/ci. Comparing signed sector-aligned
; words is equivalent to the C arithmetic shifts, including -32768.
IR_Align:
    sta.b tcc__r1
    txa
    and #$FC00
    eor #$8000
    sta.b tcc__r2
    lda.b tcc__r1
    and #$FC00
    eor #$8000
    cmp.b tcc__r2
    beq IR_Aligned
    bcc IR_AlignLower
    lda.b tcc__r1
    and #1023
    eor #$FFFF                  ; -(a + 1)
    rts
IR_AlignLower:
    lda.b tcc__r1
    and #1023
    sta.b tcc__r1
    lda #1025
    sec
    sbc.b tcc__r1
    rts
IR_Aligned:
    lda #0
    rts

.IFNDEF FX3
; GSU-2's closed-door list uses box collisions rather than rewritten
; sectors. Preserve its order and re-read Lara after every push. Six local
; bytes hold the index, hit result and current item; Lara is at 10/12,s.
Lara_collideItems:
    tsc
    sec
    sbc #6
    tcs
    lda #0
    sta 1,s
    sta 3,s
LI_Loop:
    lda 1,s
    cmp.w olDoorCount
    bcc +
    jmp LI_Return
+   tax
    lda.w olDoors,x
    and #$FF
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #(items & $FFFF)
    tax
    lda 10,s
    sta.b tcc__r0
    lda 12,s
    sta.b tcc__r0h
    ldy #1
    lda [tcc__r0],y
    eor.w 1,x
    and #$FF
    bne LI_Next
    lda.w 2,x
    and #$FF                   ; STATE_CLOSE = 0
    bne LI_Next
    txa
    sta 5,s
    lda #762                   ; LARA_HEIGHT
    pha
    lda #128                   ; SOLID_RADIUS
    pha
    pei (tcc__r0h)
    pei (tcc__r0)
    lda #$7E
    pha
    phx
    jsr.l ItemObj_collideBounds
    tsc
    clc
    adc #12
    tcs
    lda.b tcc__r0
    beq LI_Next
    lda #128
    pha
    lda 14,s
    pha
    lda 14,s
    pha
    lda #$7E
    pha
    lda 13,s
    pha
    jsr.l ItemObj_collidePush
    tsc
    clc
    adc #10
    tcs
    lda #1
    sta 3,s
LI_Next:
    lda 1,s
    inc a
    sta 1,s
    jmp LI_Loop
LI_Return:
    lda 3,s
    sta.b tcc__r0
    tsc
    clc
    adc #6
    tcs
    rtl
.ENDIF

.ENDS
.ENDIF

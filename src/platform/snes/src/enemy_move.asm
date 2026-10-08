; Native Enemy_moveGround; enemy_move.c is the executable C oracle.
; Same animation/movement/query order, signed wrapped positions, step/drop,
; active-creature avoidance and blocked-move rollback. No extra persistent RAM.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF C_MOTION
.IFDEF FX3
.BASE $80
.BANK 9 SLOT 0                  ; $89 (was $BF)
.ELSE
.BASE $3F
.BANK 63 SLOT 6                ; resident $7E:8000 code image
.ENDIF
.SECTION "enemy_move_native" SEMIFREE
.accu 16
.index 16
.16bit

Enemy_moveGround:
    tsc
    sec
    sbc #32
    tcs
    ; 1..20,s RoomQuery; 21/23/25 old XYZ; 27 room; 29 box; 31 index.
    ; Original arguments are now 36/38 item pointer and 40 turn.
    lda 36,s
    tax
    lda.w 4,x
    sta 21,s
    lda.w 6,x
    sta 23,s
    lda.w 8,x
    sta 25,s
    lda.w 1,x
    and #$00FF
    sta 27,s
    lda 25,s
    pha
    lda 23,s
    pha
    lda 31,s
    pha
    jsr.l navBoxAt
    pla
    pla
    pla
    lda.b tcc__r0
    sta 29,s
    lda 38,s
    pha
    lda 38,s
    pha
    jsr.l ItemObj_animProcess
    pla
    pla
    lda 38,s
    pha
    lda 38,s
    pha
    jsr.l ItemObj_move
    pla
    pla
    lda #$7E
    pha
    tsc
    clc
    adc #3                     ; address of RoomQuery at original 1,s
    pha
    lda 40,s
    tax
    lda.w 8,x
    pha
    lda.w 6,x
    pha
    lda.w 4,x
    pha
    lda 37,s
    pha
    jsr.l Room_query
    tsc
    clc
    adc #12
    tcs
    lda.b tcc__r0
    bne +
    jmp EG_Blocked
+   lda 3,s
    cmp #32767
    bne +
    jmp EG_Blocked
+   eor #$8000
    sta.b tcc__r0
    lda 5,s
    cmp #$8100
    bne +
    jmp EG_Blocked
+   eor #$8000
    cmp.b tcc__r0
    bcc +
    jmp EG_Blocked
+   lda 36,s
    tax
    lda.w 6,x
    sec
    sbc 3,s
    eor #$8000                  ; preserve C's wrapped signed-16 delta
    cmp #$8101                  ; h > 256
    bcc +
    jmp EG_Blocked
+   sta.b tcc__r0
    lda.w 0,x
    and #$00FF
.IFDEF FX3
    cmp #7
    beq EG_WideDrop
    cmp #12
    bcc EG_BearDrop
    cmp #15
    bcs EG_BearDrop              ; bear / raptor retain Enemy's one-click drop
.ELSE
    cmp #8
    beq EG_BearDrop              ; GSU2: only bear has the one-click drop
    bra EG_WideDrop              ; original C allows 1024 for all other types
.ENDIF
EG_WideDrop:
    lda.b tcc__r0
    cmp #$7C00                  ; wolf / Lion: h < -1024
    bra EG_DropChecked
EG_BearDrop:
    lda.b tcc__r0
    cmp #$7F00                  ; bear: h < -256
EG_DropChecked:
    bcs +
    jmp EG_Blocked
+   cmp #$7FC0                  ; h < -64
    bcs EG_Floor
    lda.w 6,x
    clc
    adc #64
    bra EG_SetY
EG_Floor:
    lda 3,s
EG_SetY:
    sta.w 6,x
    lda.w 30,x
    bne +
    jmp EG_NewBox
+   lda #0
.IFDEF FX3
    sta.b tcc__r5              ; loop-only scratch: no nested calls below
    lda 36,s
    sta.b tcc__r4
    tay                       ; keep this item and room through the scan
    lda 1,s
    sta.b tcc__r3
.ELSE
    sta 31,s
.ENDIF
.IFNDEF FX3
    ; The avoidance scan reads only WRAM items and the creature list.
    ; RoomQuery's now-dead ceiling-slope slot holds the ownership token.
    jsl olGsuYieldRam
    sta 17,s
.ENDIF
EG_Other:
.IFDEF FX3
    ldx.b tcc__r5
    cpx.w olCreatureCount
    bcc +
    jmp EG_OtherDone
+   lda.w olCreatures,x
    and #$00FF
    asl a
    tax
    lda.l EG_ItemPointers,x
.ELSE
    lda 31,s
    cmp.w olCreatureCount
    bcc +
    jmp EG_OtherDone
+   tax
    lda.l olCreatures,x
    and #$00FF
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #(items & $FFFF)
.ENDIF
.IFDEF FX3
    cmp.b tcc__r4
.ELSE
    cmp 36,s
.ENDIF
    beq EG_Next
    tax
    lda.w 1,x
    and #$00FF
.IFDEF FX3
    cmp.b tcc__r3
.ELSE
    cmp 1,s
.ENDIF
    bne EG_Next
    lda.w 16,x
    beq EG_Next
    bmi EG_Next
    lda.w 24,x
    and #3
    cmp #1
    bne EG_Next
.IFNDEF FX3
    lda 36,s
    tay
.ENDIF
    lda.w 4,x
    sec
    sbc.w 4,y
    bpl +
    eor #$FFFF
    inc a
+   cmp #340
    bcs EG_Next
    sta.b tcc__r0
    lda.w 8,x
    sec
    sbc.w 8,y
    bpl +
    eor #$FFFF
    inc a
+   cmp #340
    bcs EG_Next
    sta.b tcc__r1
    cmp.b tcc__r0
    bcc EG_XMajor
    lda.b tcc__r0
    lsr a
    clc
    adc.b tcc__r1
    bra EG_Distance
EG_XMajor:
    lsr a
    clc
    adc.b tcc__r0
EG_Distance:
    cmp #340
    bcs EG_Next
    lda #1456                   ; ANGLE_1 * 8
    sta 40,s
.IFNDEF FX3
    lda 17,s
    jsl olGsuReclaimRam
.ENDIF
    jmp EG_Blocked
EG_Next:
.IFDEF FX3
    inc.b tcc__r5
.ELSE
    lda 31,s
    inc a
    sta 31,s
.ENDIF
    jmp EG_Other
EG_OtherDone:
.IFNDEF FX3
    lda 17,s
    jsl olGsuReclaimRam
.ENDIF
EG_NewBox:
    lda 36,s
    tax
    ; The immutable nav map assigns one box to a room's whole sector.
    ; After the complete room/floor/avoidance checks, a move remaining in
    ; the same room and 1024-unit X/Z tile cannot change that box or zone
    ; when the room origin is sector-aligned. Read that room's origins:
    ; Room_query's last sector cache can belong to a vertical floor room.
    ; Invalid boxes, seams, portals and unaligned grids use the full lookup.
    lda 29,s
    cmp.w olBoxCount
    bcs EG_LookupBox
    lda 1,s
    cmp 27,s
    bne EG_LookupBox
    lda.w 4,x
    eor 21,s
    sta.b tcc__r0
    lda.w 8,x
    eor 25,s
    ora.b tcc__r0
    and #$FC00
    bne EG_LookupBox
    lda 1,s                    ; exact resolved room, already equal to old room
    asl a
    sta.b tcc__r0
    asl a
    asl a
    clc
    adc.b tcc__r0              ; room * 10
    clc
    HOST_ADD_BASE olRoomBase
    tax
    HOST_LDA olRoomData, 2
    sta.b tcc__r0
    HOST_LDA_NEXT olRoomData, 4, 2
    ora.b tcc__r0
    and #$03FF
    beq +
    lda 36,s
    tax
    bra EG_LookupBox
+
    jmp EG_Accept
EG_LookupBox:
    lda.w 8,x
    pha
    lda.w 4,x
    pha
    lda 5,s
    pha
    jsr.l navBoxAt
    pla
    pla
    pla
    lda.b tcc__r0
    cmp #$FFFF
    beq EG_Blocked
    asl a
    clc
.IFDEF FX3
    adc.w olZoneOffsets        ; current flip's ground-1 array
.ENDIF
    HOST_ADD_BASE olZoneBase
    tax
    HOST_LDA olZoneData, 0
    sta.b tcc__r1
    lda 29,s
    cmp #$FFFF
    beq EG_Blocked
    asl a
    clc
.IFDEF FX3
    adc.w olZoneOffsets
.ENDIF
    HOST_ADD_BASE olZoneBase
    tax
    HOST_LDA olZoneData, 0
    cmp.b tcc__r1
    bne EG_Blocked
EG_Accept:
    lda 36,s
    tax
    lda 1,s
    and #$00FF
    sta.b tcc__r0
    lda.w 1,x
    and #$FF00
    ora.b tcc__r0
    sta.w 1,x
    lda 3,s
    sta.w 26,x
    lda 40,s
    bra EG_Turn
EG_Blocked:
    lda 36,s
    tax
    lda 21,s
    sta.w 4,x
    lda 23,s
    sta.w 6,x
    lda 25,s
    sta.w 8,x
    lda 40,s
    asl a
EG_Turn:
    clc
    adc.w 10,x
    sta.w 10,x
    tsc
    clc
    adc #32
    tcs
    rtl
.IFDEF FX3
; All byte-valued creature indices use the same wrapped item-array offset.
; This immutable table avoids five shifts and an add for every list probe.
EG_ItemPointers:
    .REPEAT 256 INDEX creatureIndex
        .DW ((items + creatureIndex * 32) & $FFFF)
    .ENDR
.ENDIF

.ENDS
.ENDIF

; Native camera helpers and sector walkers. camera.c is the C_MOTION reference.
; No new cache or persistent RAM: locals live on the caller's stack.
; FX3 executes in FastROM, GSU-2 in its existing resident WRAM image.
; 816-tcc ABI: A/X/Y 16, D=0, DB=$7E, JSL. Argument layouts are at each entry;
; traceX/traceZ take two four-byte Location pointers.
; Coordinates/differences wrap at 16 bits, slopes/products retain 32 bits.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF C_MOTION
.IFDEF FX3
.BASE $80
.BANK 5 SLOT 0
.ELSE
.BASE $7A
.BANK 5 SLOT 6                ; resident $7F:8000 code image
.ENDIF
.SECTION "camera_fx3_native" SEMIFREE
.accu 16
.index 16
.16bit

.IFDEF FX3
; game.c's camera handoff, retaining its complete C body as an oracle.
; The trigger preparation, mode precedence, follow update and room probe
; run in the same order. Locals: RoomQuery 1..20, room hint 21..22.
Lara_toCamera:
    lda.w itemsCount
    bne +
    rtl
+   lda.w olCamBusy
    beq LTC_Mode
    jsl Camera_prepare
    bra LTC_Follow
LTC_Mode:
    lda.w olLaraLooking
    beq LTC_Combat
    lda #2
    sta.w camera+18
    bra LTC_Follow
LTC_Combat:
    lda.w items+16
    beq LTC_Follow
    bmi LTC_Follow
    lda.w olLaraWeaponState
    cmp #2
    beq +
    cmp #4
    bne LTC_Follow
+   lda #1
    sta.w camera+18
LTC_Follow:
    pea $007E
    pea (items & $FFFF)
    jsl Camera_updateFollow
    pla
    pla
    lda.w camera
    sta.w ol_camX
    lda.w camera+2
    sta.w ol_camY
    lda.w camera+4
    sta.w ol_camZ
    lda.w camTarget+2
    sta.w ol_camTgtY
    lda.w camera+8
    sta.w ol_angle
    tsc
    sec
    sbc #22
    tcs
    lda.w camera
    jsr CUF_Sector
    sta 1,s
    lda.w camera+2
    jsr CUF_Height
    sta 3,s
    lda.w camera+4
    jsr CUF_Sector
    sta 5,s
    lda.w olCamBusy
    beq LTC_ItemRoom
    lda.w olCamFixedView
    beq LTC_ItemRoom
    lda.w camera+6
    bra LTC_Hint
LTC_ItemRoom:
    lda.w items+1
    and #$00FF
LTC_Hint:
    sta 21,s
    cmp.w olCamHint
    bne LTC_Query
    lda 1,s
    cmp.w olCamSX
    bne LTC_Query
    lda 3,s
    cmp.w olCamSY
    bne LTC_Query
    lda 5,s
    cmp.w olCamSZ
    beq LTC_Return
LTC_Query:
    lda 1,s
    sta.w olCamSX
    lda 3,s
    sta.w olCamSY
    lda 5,s
    sta.w olCamSZ
    lda 21,s
    sta.w olCamHint
    pea 0
    tsc
    clc
    adc #3
    pha
    lda.w camera+4
    pha
    lda.w camera+2
    pha
    lda.w camera
    pha
    lda 31,s
    pha
    jsl Room_query
    tsc
    clc
    adc #12
    tcs
    lda.b tcc__r0
    beq LTC_NoRoom
    lda 1,s
    sta.w camera+6
    bra LTC_SetRoom
LTC_NoRoom:
    lda 21,s
LTC_SetRoom:
    sta.w ol_camRoom
LTC_Return:
    tsc
    clc
    adc #22
    tcs
    rtl

; Common follow mode. Triggered/fixed/look/combat cameras and per-state
; requests retain the complete C path. Stack-only scratch and the same
; trace/fallback caches; no skipped query or changed camera policy.
Camera_updateFollow:
    ; The common caller follows the first item. Preserve the C access
    ; order for arbitrary far/aliased target records by taking its path.
    lda 6,s
    cmp #$007E
    bne CUF_Cold
    lda 4,s
    cmp #(items & $FFFF)
    bne CUF_Cold
    lda.w olCamBusy
    ora.w camera+30             ; ask
    ora.w camera+18             ; mode
    beq CUF_Begin
CUF_Cold:
    jml Camera_updateFollowCold
.DEFINE CUF_SIZE 30
.DEFINE CUF_WANT 1
.DEFINE CUF_BEST 9
.DEFINE CUF_KEYS 17
.DEFINE CUF_Y 29
CUF_Begin:
    tsc
    sec
    sbc #CUF_SIZE
    tcs
    lda CUF_SIZE+6,s
    pha
    lda CUF_SIZE+6,s
    pha
    jsl Camera_targetY
    pla
    pla
    lda.b tcc__r0
    sta CUF_Y,s
    jsl Camera_followAngleX
    pea 1536
    pei (tcc__r0)
    jsl Camera_pitch
    pla
    pla
    lda #8
    sta.w camera+20
    lda CUF_SIZE+4,s
    clc
    adc #10
    sta.b tcc__r0
    lda CUF_SIZE+6,s
    sta.b tcc__r0h
    lda [tcc__r0]
    sta.w camera+10
    lda CUF_SIZE+4,s
    clc
    adc #4
    sta.b tcc__r0
    lda [tcc__r0]
    sta.w camTarget
    lda CUF_SIZE+4,s
    clc
    adc #8
    sta.b tcc__r0
    lda [tcc__r0]
    sta.w camTarget+4
    lda CUF_SIZE+4,s
    inc a
    sta.b tcc__r0
    lda [tcc__r0]
    and #$00FF
    sta.w camTarget+6
    lda CUF_Y,s
    sec
    sbc.w camTarget+2
    cmp #$8000
    ror a
    cmp #$8000
    ror a
    clc
    adc.w camTarget+2
    sta.w camTarget+2
    inc.w fbTick
    tsc
    clc
    adc #CUF_WANT
    tay
    lda.w camera+10
    jsr CUF_Location
    lda.w camTarget
    jsr CUF_Sector
    sta CUF_KEYS,s
    lda.w camTarget+2
    jsr CUF_Height
    sta CUF_KEYS+2,s
    lda.w camTarget+4
    jsr CUF_Sector
    sta CUF_KEYS+4,s
    lda CUF_WANT,s
    jsr CUF_Sector
    sta CUF_KEYS+6,s
    lda CUF_WANT+2,s
    jsr CUF_Height
    sta CUF_KEYS+8,s
    lda CUF_WANT+4,s
    jsr CUF_Sector
    sta CUF_KEYS+10,s
    lda.w tcValid
    beq CUF_Miss
    lda.w tcMode
    bne CUF_Miss
    lda CUF_KEYS,s
    cmp.w tcSX
    bne CUF_Miss
    lda CUF_KEYS+2,s
    cmp.w tcSY
    bne CUF_Miss
    lda CUF_KEYS+4,s
    cmp.w tcSZ
    bne CUF_Miss
    lda CUF_KEYS+6,s
    cmp.w tcWX
    bne CUF_Miss
    lda CUF_KEYS+8,s
    cmp.w tcWY
    bne CUF_Miss
    lda CUF_KEYS+10,s
    cmp.w tcWZ
    beq CUF_Cached
CUF_Miss:
    stz.w tcMode
    pea $7E
    pea (tcTo & $FFFF)
    pea 0
    jsl Camera_getBestLocation
    pla
    pla
    pla
    lda.b tcc__r0
    sta.w tcClear
    bne +
    pea $7E
    pea (tcTo & $FFFF)
    jsl Camera_clip
    pla
    pla
+   lda #1
    sta.w tcValid
    lda CUF_KEYS,s
    sta.w tcSX
    lda CUF_KEYS+2,s
    sta.w tcSY
    lda CUF_KEYS+4,s
    sta.w tcSZ
    lda CUF_KEYS+6,s
    sta.w tcWX
    lda CUF_KEYS+8,s
    sta.w tcWY
    lda CUF_KEYS+10,s
    sta.w tcWZ
CUF_Cached:
    lda.w tcClear
    beq CUF_Blocked
    cmp #1
    bne CUF_Fallback
    lda CUF_WANT,s
    sta CUF_BEST,s
    lda CUF_WANT+2,s
    sta CUF_BEST+2,s
    lda CUF_WANT+4,s
    sta CUF_BEST+4,s
    bra CUF_Clip
CUF_Fallback:
    tsc
    clc
    adc #CUF_BEST
    tay
    lda.w fbDir
    lsr a
    ror a
    ror a
    and #$C000
    jsr CUF_Location
    lda.w fbRoom
    sta.w tcTo+6
CUF_Clip:
    lda.w tcTo+6
    sta CUF_BEST+6,s
    pea 0
    tsc
    clc
    adc #(CUF_BEST+2)
    pha
    jsl Camera_clip
    pla
    pla
    bra CUF_Move
CUF_Blocked:
    lda.w tcTo
    sta CUF_BEST,s
    lda.w tcTo+2
    sta CUF_BEST+2,s
    lda.w tcTo+4
    sta CUF_BEST+4,s
    lda.w tcTo+6
    sta CUF_BEST+6,s
CUF_Move:
    pea 0
    tsc
    clc
    adc #(CUF_BEST+2)
    pha
    jsl Camera_move
    pla
    pla
    stz.w camera+18
    jsl Camera_lookAtTarget
    tsc
    clc
    adc #CUF_SIZE
    tcs
    rtl

; Signed word >> 10 / >> 8 without a ten-iteration compiler loop.
CUF_Sector:
    xba
    and #$00FF
    lsr a
    lsr a
    cmp #$0020
    bcc +
    ora #$FFC0
+   rts
CUF_Height:
    xba
    and #$00FF
    cmp #$0080
    bcc +
    ora #$FF00
+   rts

; A angle, Y stack Location. The target fields are read after the X/Z
; helper just as in Camera_locationForAngle. Preserve destination on stack.
CUF_Location:
    sta.b tcc__r5
    phy
    pea 0
    tya
    clc
    adc #4
    pha
    pea 0
    phy
    pei (tcc__r5)
    jsl Camera_getLocationForAngle_xz
    pla
    pla
    pla
    pla
    pla
    ply
    lda.w camTarget+2
    clc
    adc.w camera+16
    sta.w 2,y
    lda.w camTarget+6
    sta.w 6,y
    rts

; Exact word floorShift used by the follow camera. Valid shifts 0..15
; are arithmetic; larger counts retain the C runtime's zero result.
Camera_floorShift:
    lda 6,s
    cmp #16
    bcs CFS_Zero
    tay
    lda 4,s
    cpy #0
    beq CFS_Done
CFS_Loop:
    cmp #$8000
    ror a
    dey
    bne CFS_Loop
CFS_Done:
    sta.b tcc__r0
    rtl
CFS_Zero:
    stz.b tcc__r0
    rtl

; CAMERA.BIN's interpolated offsets. ABI: item far pointer at 4,s,
; component (followY/middleY/middleX/middleZ) at 8,s. No persistent RAM.
Camera_boxOffset:
    lda 8,s
    cmp #4
    bcs CBO_Empty
    lda.w olLevel+12
    bne CBO_Header
CBO_Empty:
    stz.b tcc__r0
    rtl
CBO_Header:
    lda.w olLevel+8
    sta.b tcc__r1
    lda.w olLevel+10
    sta.b tcc__r1h
    lda [tcc__r1]
    asl a
    asl a
    asl a
    sta.b tcc__r2
    ldy #2
    lda [tcc__r1],y
    asl a
    asl a
    clc
    adc.b tcc__r2
    clc
    adc #4
    clc
    adc.b tcc__r1
    sta.b tcc__r1                ; animation directory
    lda 4,s
    clc
    adc #12
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    lda [tcc__r0]
    sta.b tcc__r2                ; animation index
    lda [tcc__r1]
    cmp.b tcc__r2
    beq CBO_Empty
    bcc CBO_Empty
    lda.b tcc__r2
    asl a
    asl a
    asl a
    sta.b tcc__r2
    lda 8,s
    asl a
    clc
    adc.b tcc__r2
    clc
    adc #2
    tay
    lda [tcc__r1],y
    bne CBO_Record
    stz.b tcc__r0
    rtl
CBO_Record:
    clc
    adc.w olLevel+8
    sta.b tcc__r1                ; scalar stream
    ldy #2
    lda [tcc__r1],y
    sta.b tcc__r2h               ; signed minimum
    lda [tcc__r1]
    xba
    and #$00FF
    sta.b tcc__r2                ; bits per sample
    bne CBO_Frame
    lda.b tcc__r2h
    sta.b tcc__r0
    rtl
CBO_Frame:
    inc.b tcc__r0
    inc.b tcc__r0
    lda [tcc__r0]
    and #$00FF
    sta.b tcc__r3                ; tick
    lda [tcc__r1]
    and #$00FF
    cmp.b tcc__r3
    beq CBO_First
    bcs CBO_Index
CBO_First:
    stz.b tcc__r3                ; invalid tick safely selects first sample
CBO_Index:
    ldx.b tcc__r2
    lda #0
CBO_Product:
    clc
    adc.b tcc__r3
    dex
    bne CBO_Product
    pha
    and #7
    tax
    pla
    lsr a
    lsr a
    lsr a
    clc
    adc #4
    tay
    lda [tcc__r1],y
    sta.b tcc__r3
    iny
    iny
    lda [tcc__r1],y
    and #$00FF
    cpx #0
    beq CBO_Mask
CBO_Shift:
    lsr a
    ror.b tcc__r3
    dex
    bne CBO_Shift
CBO_Mask:
    lda.b tcc__r2
    asl a
    tax
    lda.b tcc__r3
    and.l CBO_Masks,x
    clc
    adc.b tcc__r2h
    sta.b tcc__r0
    rtl
CBO_Masks:
    .dw $0000,$0001,$0003,$0007,$000F,$001F,$003F,$007F,$00FF
    .dw $01FF,$03FF,$07FF,$0FFF,$1FFF,$3FFF,$7FFF,$FFFF

Camera_targetY:
    pea 0
    lda 8,s                     ; original item bank
    pha
    lda 8,s                     ; original item low word
    pha
    jsl Camera_boxOffset
    pla
    pla
    pla
    lda.b tcc__r0
    sta.b tcc__r1
    lda 4,s
    clc
    adc #6
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    lda [tcc__r0]
    clc
    adc.b tcc__r1
    sta.b tcc__r0
    rtl

Camera_lookAtTarget:
    lda.w camTarget
    sec
    sbc.w camera
    sta.b tcc__r0
    lda.w camTarget+4
    sec
    sbc.w camera+4
    sta.b tcc__r1
    ora.b tcc__r0
    beq CLT_Return
    lda.b tcc__r0
    ldx #0
    cmp #$8000
    bcc +
    dex
+   phx
    pha
    lda.b tcc__r1
    ldx #0
    cmp #$8000
    bcc +
    dex
+   phx
    pha
    jsl phd_atan
    pla
    pla
    pla
    pla
    lda.b tcc__r0
    sta.w camera+8
CLT_Return:
    rtl
.ENDIF

.DEFINE CT_PX     1
.DEFINE CT_PY     3
.DEFINE CT_PZ     5
.DEFINE CT_ROOM   7
.DEFINE CT_TO     9
.DEFINE CT_END   13
.DEFINE CT_D     15
.DEFINE CT_SA    17
.DEFINE CT_SB    21
.DEFINE CT_OFF   25
.DEFINE CT_STEP  27
.DEFINE CT_EDGE  29
.DEFINE CT_Q     31           ; full 20-byte RoomQuery
.DEFINE CT_SIZE  50

; Rounded 1.14 trig lookup, then two signed products of distH (camera+14,
; targetDist split by the pitch). Keep each full product before taking bits
; 14..29. Both products come before either write, as camera.c's (which keeps
; them); the target's Z is read after the X write, so aliased outputs still
; see C's order.
Camera_getLocationForAngle_xz:
    lda 6,s
    sta.b tcc__r0
    lda 8,s
    sta.b tcc__r0h
    lda 10,s
    sta.b tcc__r1
    lda 12,s
    sta.b tcc__r1h
    lda 4,s
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
    sta.b tcc__r3
    txa
    clc
    adc #512
    and #$07FE
    tax
    lda.l olSinTable,x
    sta.b tcc__r3h
    lda.b tcc__r3
    jsr CT_CameraScale
    pha                         ; x offset: both products before any write
    lda.b tcc__r3h
    jsr CT_CameraScale
    sta.b tcc__r2
    lda.w camTarget
    sec
    sbc 1,s
    sta [tcc__r0]
    pla
    lda.w camTarget+4
    sec
    sbc.b tcc__r2
    sta [tcc__r1]
    rtl

CT_CameraScale:
.IFDEF FX3
    ; Exact cardinal products need no multiplier, including -32768 distH.
    cmp #0
    beq CTS_Zero
    cmp #$4000
    beq CTS_Unit
    cmp #$C000
    bne CTS_Product
    lda #0
    sec
    sbc.w camera+14
    rts
CTS_Unit:
    lda.w camera+14
CTS_Zero:
    rts
CTS_Product:
.ENDIF
    sta.b tcc__r9
    eor.w camera+14
    sta.b tcc__r4
    lda.b tcc__r9
    bpl +
    eor #$FFFF
    inc a
    sta.b tcc__r9
+   lda.w camera+14
    bpl +
    eor #$FFFF
    inc a
+   sta.b tcc__r10
    stz.b tcc__r9h
    stz.b tcc__r10h
    jsr.l tcc__mull
    lda.b tcc__r4
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
+   stx.b tcc__r4
    tya
    asl a
    rol.b tcc__r4
    asl a
    rol.b tcc__r4
    lda.b tcc__r4
    rts

; Same public C ABI: room/x/y/z, then floor/ceiling/room far pointers.
; Only the 20-byte RoomQuery is local. No output writes on failure; outputs
; may alias and are written floor, ceiling, room just as camera.c specifies.
camQuery:
    tsa
    sec
    sbc #20
    tas
    lda #0
    pha
    tsa
    clc
    adc #3
    pha
    .REPEAT 4
    lda 34,s
    pha
    .ENDR
    jsr.l Room_query
    tsa
    clc
    adc #12
    tas
    lda.b tcc__r0
    beq CT_QueryReturn
    lda 32,s
    sta.b tcc__r0
    lda 34,s
    sta.b tcc__r0h
    lda 3,s
    cmp #32767
    bne +
    lda #$8100
+   sta [tcc__r0]
    lda 36,s
    sta.b tcc__r0
    lda 38,s
    sta.b tcc__r0h
    lda 5,s
    sta [tcc__r0]
    lda 40,s
    sta.b tcc__r0
    lda 42,s
    sta.b tcc__r0h
    lda 1,s
    sta [tcc__r0]
    lda #1
    sta.b tcc__r0
CT_QueryReturn:
    tsa
    clc
    adc #20
    tas
    rtl

; The room resolver ignores heights, but still performs the same full
; query and preserves the caller's room on failure. Public C word ABI.
camRoomAt:
    tsa
    sec
    sbc #20
    tas
    lda #0
    pha
    tsa
    clc
    adc #3
    pha
    .REPEAT 4
    lda 34,s
    pha
    .ENDR
    jsr.l Room_query
    tsa
    clc
    adc #12
    tas
    lda.b tcc__r0
    beq +
    lda 1,s
    bra ++
+   lda 24,s
++  sta.b tcc__r0
    tsa
    clc
    adc #20
    tas
    rtl

.DEFINE CM_VIEW 1
.DEFINE CM_Q    9
.DEFINE CM_SIZE 28

; Camera_move's public C ABI takes one far Location pointer. Keep signed
; word subtraction/ASR/addition and x/y/z write order, even for overlapping
; inputs. All original queries still execute, including the second query
; of the newly resolved room and the post-trace room lookup.
Camera_move:
    tsa
    sec
    sbc #CM_SIZE
    tas
    lda CM_SIZE+4,s
    sta.b tcc__r0
    lda CM_SIZE+6,s
    sta.b tcc__r0h
    .REPEAT 3 INDEX axis
    ldy #axis*2
    lda [tcc__r0],y
    sec
    sbc.w camera+axis*2
    jsr CM_Step
    clc
    adc.w camera+axis*2
    sta.w camera+axis*2
    .ENDR
    ldy #6
    lda [tcc__r0],y
    jsr CM_Query
    sta.w camera+6
    jsr CM_Query
    lda.b tcc__r0
    beq CM_Return
    lda CM_Q+2,s
    cmp #32767
    bne +
    lda #$8100                ; camQuery's OL_NO_FLOOR -> WALL conversion
+   sec
    sbc #256                  ; original signed-word floor subtraction
    eor #$8000
    sta.b tcc__r4
    lda.w camera+2
    eor #$8000
    cmp.b tcc__r4
    bcc CM_Return
    lda CM_SIZE+4,s
    sta.b tcc__r0
    lda CM_SIZE+6,s
    sta.b tcc__r0h
    ldy #2
    lda [tcc__r0],y
    eor #$8000
    cmp.b tcc__r4
    bcc CM_Return
    .REPEAT 4 INDEX axis
    lda.w camera+axis*2
    sta CM_VIEW+axis*2,s
    .ENDR
    lda #0
    pha
    tsa
    clc
    adc #CM_VIEW+2
    pha
    lda #$7E                  ; globals are in WRAM; WLA label values are bank-local
    pha
    lda #(camTarget & $FFFF)
    pha
    jsr.l Room_trace
    tsa
    clc
    adc #8
    tas
    .REPEAT 3 INDEX axis
    lda CM_VIEW+axis*2,s
    sta.w camera+axis*2
    .ENDR
    lda CM_VIEW+6,s
    jsr CM_Query
    sta.w camera+6
CM_Return:
    tsa
    clc
    adc #CM_SIZE
    tas
    rtl

; Private JSR: A = a signed delta -> upstream move()'s step for camera.speed
; (camera+20): 8 is `>> 3`, which floors; 4 is a C divide, which truncates
; toward zero; 1 is the whole delta.
CM_Step:
    ldx.w camera+20
    cpx #8
    bne CM_StepNot8
    cmp #$8000
    ror a
    cmp #$8000
    ror a
    cmp #$8000
    ror a
    rts
CM_StepNot8:
    cpx #4
    bne CM_StepDone
    cmp #0
    bpl CM_StepPos
    eor #$FFFF
    inc a
    lsr a
    lsr a
    eor #$FFFF
    inc a
    rts
CM_StepPos:
    lsr a
    lsr a
CM_StepDone:
    rts

; Private JSR: A=room, xyz from camera. The parent's query record starts
; at CM_Q after our return/room pushes. Return resolved/fallback room in A
; and leave Room_query's success flag in r0 for the height-test caller.
CM_Query:
    pha
    lda #0
    pha
    tsa
    clc
    adc #CM_Q+2+4
    pha
    lda.w camera+4
    pha
    lda.w camera+2
    pha
    lda.w camera
    pha
    lda 11,s
    pha
    jsr.l Room_query
    tsa
    clc
    adc #12
    tas
    lda.b tcc__r0
    beq +
    lda CM_Q+4,s
    plx
    rts
+   pla
    rts

; primary coordinate offset, then the other two coordinate offsets.
; Expanding the same walk for X/Z preserves their original query order.
.MACRO CT_AXIS
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    lda 8,s
    sta.b tcc__r1
    lda 10,s
    sta.b tcc__r1h
    ldy #\1
    lda [tcc__r1],y
    sec
    sbc [tcc__r0],y
    sta.b tcc__r2
    bne _ct_nonzero\@
    lda #1                   ; zero delta does not assign to->room
    sta.b tcc__r0
    rtl
_ct_nonzero\@:
    lda [tcc__r1],y
    eor [tcc__r0],y
    and #$FC00
    bne _ct_walk\@
    ldy #6                   ; same sector: original no-iteration result
    lda [tcc__r0],y
    sta [tcc__r1],y
    lda #1
    sta.b tcc__r0
    rtl
_ct_walk\@:
    tsa
    sec
    sbc #CT_SIZE
    tas
    lda.b tcc__r1
    sta CT_TO,s
    lda.b tcc__r1h
    sta CT_TO+2,s
    lda.b tcc__r2
    sta CT_D,s
    lda [tcc__r1],y
    sta CT_END,s
    ldy #0
    lda [tcc__r0],y
    sta CT_PX,s
    ldy #2
    lda [tcc__r0],y
    sta CT_PY,s
    ldy #4
    lda [tcc__r0],y
    sta CT_PZ,s
    ldy #6
    lda [tcc__r0],y
    sta CT_ROOM,s
    ldy #\2
    lda [tcc__r1],y
    sec
    sbc [tcc__r0],y
    sta CT_SA,s
    ldy #\3
    lda [tcc__r1],y
    sec
    sbc [tcc__r0],y
    sta CT_SB,s

    lda CT_D,s
    tax
    lda CT_SA,s
    jsr CT_Slope
    lda.b tcc__r0
    sta CT_SA,s
    lda.b tcc__r1
    sta CT_SA+2,s
    lda CT_D,s
    tax
    lda CT_SB,s
    jsr CT_Slope
    lda.b tcc__r0
    sta CT_SB,s
    lda.b tcc__r1
    sta CT_SB+2,s

    lda CT_D,s
    bmi _ct_negative\@
    lda #1024
    sta CT_STEP,s
    lda #1
    sta CT_EDGE,s
    lda CT_PX+\1,s
    ora #1023
    bra _ct_boundary\@
_ct_negative\@:
    lda #$FC00
    sta CT_STEP,s
    lda #$FFFF
    sta CT_EDGE,s
    lda CT_PX+\1,s
    and #$FC00
_ct_boundary\@:
    tax
    sec
    sbc CT_PX+\1,s
    sta CT_OFF,s
    txa
    sta CT_PX+\1,s

    lda CT_OFF,s
    tay
    lda CT_SA+2,s
    tax
    lda CT_SA,s
    jsr CT_Scale
    clc
    adc CT_PX+\2,s
    sta CT_PX+\2,s
    lda CT_OFF,s
    tay
    lda CT_SB+2,s
    tax
    lda CT_SB,s
    jsr CT_Scale
    clc
    adc CT_PX+\3,s
    sta CT_PX+\3,s

    lda CT_D,s
    bpl _ct_loop\@
    lda CT_SA,s              ; after the first fractional step only the
    eor #$FFFF               ; wrapped low word is added/subtracted
    inc a
    sta CT_SA,s
    lda CT_SB,s
    eor #$FFFF
    inc a
    sta CT_SB,s
_ct_loop\@:
    lda CT_D,s
    bmi _ct_backwards\@
    lda CT_END,s
    sec
    sbc CT_PX+\1,s
    bra _ct_compare\@
_ct_backwards\@:
    lda CT_PX+\1,s
    sec
    sbc CT_END,s
_ct_compare\@:
    beq _ct_clear\@
    bmi _ct_clear\@          ; signed *wrapped* difference, not N xor V
    lda #0
    ldx #\1
    jsr.l traceCheck
    lda.b tcc__r0
    beq _ct_hit\@
    lda CT_EDGE,s
    ldx #\1
    jsr.l traceCheck
    lda.b tcc__r0
    beq _ct_hit\@
    lda CT_PX+\1,s
    clc
    adc CT_STEP,s
    sta CT_PX+\1,s
    lda CT_PX+\2,s
    clc
    adc CT_SA,s
    sta CT_PX+\2,s
    lda CT_PX+\3,s
    clc
    adc CT_SB,s
    sta CT_PX+\3,s
    bra _ct_loop\@
_ct_hit\@:
    lda CT_TO,s
    sta.b tcc__r0
    lda CT_TO+2,s
    sta.b tcc__r0h
    ldy #0
    lda CT_PX,s
    sta [tcc__r0],y
    ldy #2
    lda CT_PY,s
    sta [tcc__r0],y
    ldy #4
    lda CT_PZ,s
    sta [tcc__r0],y
    ldy #6
    lda CT_ROOM,s
    sta [tcc__r0],y
    stz.b tcc__r0
    bra _ct_return\@
_ct_clear\@:
    lda CT_TO,s
    sta.b tcc__r0
    lda CT_TO+2,s
    sta.b tcc__r0h
    ldy #6
    lda CT_ROOM,s
    sta [tcc__r0],y
    lda #1
    sta.b tcc__r0
_ct_return\@:
    tsa
    clc
    adc #CT_SIZE
    tas
    rtl
.ENDM

traceX:
    CT_AXIS 0 2 4
traceZ:
    CT_AXIS 4 0 2

; Signed word delta A << 10 / signed word divisor X, truncating toward zero.
; Preserve the complete 32-bit quotient in r0/r1, including steep rays.
CT_Slope:
    sta.b tcc__r9
    bne +
    stz.b tcc__r0
    stz.b tcc__r1
    rts
+   ldy #0
    bit #$8000
    beq +
    dey
+   sty.b tcc__r9h
    stx.b tcc__r10
    txa
    ldy #0
    bit #$8000
    beq +
    dey
+   sty.b tcc__r10h
    .REPEAT 10
    asl.b tcc__r9
    rol.b tcc__r9h
    .ENDR
    pei (tcc__r10h)
    pei (tcc__r10)
    pei (tcc__r9h)
    pei (tcc__r9)
    jsr.l tcc__divdi3
    tsa
    clc
    adc #8
    tas
    rts

; Bits 10..25 of signed 32-bit X:A * signed word Y, modulo 2^32.
; Full multiply is necessary before narrowing: slope can exceed 16 bits.
CT_Scale:
    sta.b tcc__r9
    stx.b tcc__r9h
    sty.b tcc__r10
    tya
    ldx #0
    bit #$8000
    beq +
    dex
+   stx.b tcc__r10h
    jsr.l tcc__mull
    stx.b tcc__r4
    tya
    .REPEAT 6
    asl a
    rol.b tcc__r4
    .ENDR
    lda.b tcc__r4
    rts

; Private JSL helper (not the C traceCheck ABI). A is the probe's +/-1
; displacement, X its axis (0 or 4); the parent's 50-byte frame follows the
; three-byte return. Room_query remains the authority for portals/slopes.
; Invalid queries pass without changing the room. A failed height check
; keeps the old room for the hit, including the first probe at a boundary.
traceCheck:
    sta.b tcc__r4
    lda #0                   ; output pointer into the mirrored WRAM stack
    pha
    tsa
    clc
    adc #CT_Q+3+2
    pha
    lda CT_PZ+3+4,s
    cpx #4
    bne +
    clc
    adc.b tcc__r4
+   pha
    lda CT_PY+3+6,s
    pha
    lda CT_PX+3+8,s
    cpx #0
    bne +
    clc
    adc.b tcc__r4
+   pha
    lda CT_ROOM+3+10,s
    pha
    jsr.l Room_query
    tsa
    clc
    adc #12
    tas
    lda.b tcc__r0
    beq CT_CheckInvalid
    lda CT_Q+3+2,s            ; floor, translating OL_NO_FLOOR to WALL
    cmp #32767
    bne +
    lda #$8100
+   eor #$8000
    sta.b tcc__r4
    lda CT_PY+3,s
    eor #$8000
    cmp.b tcc__r4
    beq +
    bcs CT_CheckHit
+   lda CT_Q+3+4,s
    eor #$8000
    sta.b tcc__r4
    lda CT_PY+3,s
    eor #$8000
    cmp.b tcc__r4
    bcc CT_CheckHit
    lda CT_Q+3,s
    sta CT_ROOM+3,s
CT_CheckInvalid:
    lda #1
    sta.b tcc__r0
    rtl
CT_CheckHit:
    stz.b tcc__r0
    rtl

.ENDS
.ENDIF

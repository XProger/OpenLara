; FX3 Nav hot paths. nav_fx3.c is retained in C_MOTION reference cartridges.
; 816-tcc ABI: A/X/Y 16, D=0, DB=$7E, JSL; all tcc pseudo-registers scratch.
; Nav is explicitly allocated in $7E; independent table pointers read ROM.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFDEF FX3
.IFNDEF C_MOTION
.BASE $80
.BANK 7 SLOT 0                  ; $87 (was $BD)
.SECTION "nav_fx3_native" SEMIFREE
.accu 16
.index 16
.16bit

.DEFINE NN_COUNT 8192
.DEFINE NN_ZONE  8194
.DEFINE NN_WEIGHT 8196
.DEFINE NN_END   8198
.DEFINE NN_NEXT  8200
.DEFINE NN_HEAD  8202
.DEFINE NN_TAIL  8204
.DEFINE NN_STEP  8206
.DEFINE NN_DROP  8208
.DEFINE NN_MASK  8212
.DEFINE NN_OWNER 8214
.DEFINE NN_SIZE  8240

; Reuse dominates admission: Enemy_update and Enemy_target both acquire.
; Only three fixed contexts exist. First-time admission into an empty slot
; stays native too; reclamation/nearest/forced eviction retain the complete
; C path and receive the caller's unchanged arguments/return through JML.
Enemy_navAcquire:
    lda 4,s
    sec
    sbc #(items & $FFFF)
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    cmp.w olNav+NN_OWNER
    beq NA_First
    cmp.w olNav+NN_SIZE+NN_OWNER
    beq NA_Second
    cmp.w olNav+NN_SIZE*2+NN_OWNER
    beq NA_Third
    bra NA_Admission
NA_First:
    lda #(olNav & $FFFF)
    bra NA_Return
NA_Second:
    lda #((olNav+NN_SIZE) & $FFFF)
    bra NA_Return
NA_Third:
    lda #((olNav+NN_SIZE*2) & $FFFF)
NA_Return:
    sta.b tcc__r0
    lda #$7E
    sta.b tcc__r0h
    rtl
NA_Admission:
    lda 6,s
    cmp #$7E                   ; the fixed item array's explicit WRAM bank
    beq +
    jml Enemy_navAcquireCold
+
.MACRO NA_EMPTY_SLOT
    ldx #(olNav+\1*NN_SIZE & $FFFF)
    lda.w NN_OWNER,x
    cmp #$FFFF
    bne +
    jmp NA_Empty
+
    cmp.w itemsCount
    bcs NA_Cold
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc #(items & $FFFF)
    tax
    lda.w 24,x
    and #3
    cmp #2                     ; an earlier inactive slot takes precedence
    beq NA_Cold
    lda.w 15,x
    and #$0040
    beq NA_Cold
.ENDM
    NA_EMPTY_SLOT 0
    NA_EMPTY_SLOT 1
    NA_EMPTY_SLOT 2
NA_Cold:
    jml Enemy_navAcquireCold

NA_Empty:
    phx                        ; slot pointer survives the point query
    lda 6,s
    tax
    lda.w 8,x
    pha
    lda.w 4,x
    pha
    lda.w 1,x
    and #$00FF
    pha
    jsr.l navBoxAt
    pla
    pla
    pla
    lda.b tcc__r0
    cmp.w olBoxCount
    bcc +
    plx
    stz.b tcc__r0
    stz.b tcc__r0h
    rtl
+   pha                        ; box, then slot, then unchanged caller frame
    lda 8,s                    ; item pointer
    sec
    sbc #(items & $FFFF)
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    tay                        ; owner index
    lda 3,s
    tax                        ; Nav pointer
    tya
    sta.w NN_OWNER,x
    sep #$20
    lda #0
    sta.w olEnemyTurn,y
    rep #$20
    lda #256
    sta.w NN_STEP,x
    lda #0
    sta.w 8210,x               ; vSpeed
    lda 8,s
    tay
    lda.w 0,y
    and #$00FF
    cmp #9
    beq NA_Fly
    cmp #7
    beq NA_Wolf
    cmp #12
    bcc +
    cmp #15
    bcc NA_Wolf                 ; Lion::initExtra: drop -1024
+
    lda #$FF00                 ; bear / other ground drop -256
    bra NA_Drop
NA_Wolf:
    lda #$FC00
    bra NA_Drop
NA_Fly:
    lda #20480
    sta.w NN_STEP,x
    lda #16
    sta.w 8210,x
    lda #$B000                 ; -20480
NA_Drop:
    sta.w NN_DROP,x
    lda 1,s                    ; Nav_init(slot, box)
    pha
    lda #$7E
    pha
    phx
    jsr.l Nav_init
    pla
    pla
    pla
    pla                        ; saved box
    pla                        ; saved slot is the return pointer
    sta.b tcc__r0
    lda #$7E
    sta.b tcc__r0h
    rtl

; Native shell around the unchanged C mood rules. Items/Nav are explicitly
; in $7E, the immutable box/zone tables in ROM. Keep world deltas 32-bit until
; BOTH axes fit steering's signed range; never independently clamp an axis.
Enemy_target:
    stz.w olEnemyInZone
    lda #0
    pha
    lda 8,s
    pha
    lda 8,s
    pha
    jsr.l Enemy_navAcquire
    pla
    pla
    pla
    lda.b tcc__r0h
    sta.w olEnemyNav+2
    lda.b tcc__r0
    sta.w olEnemyNav
    bne +
    jmp ET_False
+   lda 4,s
    sta.w olNavEnemy
    tax
    lda 6,s
    sta.w olNavEnemy+2
    lda 8,s
    sta.w olNavLara
    lda 10,s
    sta.w olNavLara+2
    jsr ET_Box
    lda.b tcc__r0
    sta.w olEnemyBox
    ldx.w olNavLara
    jsr ET_Box
    lda.b tcc__r0
    sta.w olTargetBox
    cmp.w olBoxCount
    bcc +
    jmp ET_False
+   lda.w olEnemyBox
    cmp.w olBoxCount
    bcc +
    jmp ET_False
+   ldx.w olNavEnemy
    ldy #(olEnemyPos & $FFFF)
    jsr ET_Point
    ldx.w olNavLara
    ldy #(olTargetPos & $FFFF)
    jsr ET_Point
    ldx.w olEnemyNav
    lda.w NN_ZONE,x
    HOST_ZONE_OFFSET
    clc
    HOST_ADD_BASE olZoneBase
    sta.b tcc__r1
    lda.w olEnemyBox
    asl a
    clc
    adc.b tcc__r1
    tax
    HOST_LDA olZoneData, 0
    sta.b tcc__r2
    lda.w olTargetBox
    asl a
    clc
    adc.b tcc__r1
    tax
    HOST_LDA olZoneData, 0
    cmp.b tcc__r2
    bne +
    lda #1
    sta.w olEnemyInZone
+   jsr.l Enemy_targetMood
    lda #$7E
    pha
    lda #(olEnemyPos & $FFFF)
    pha
    lda.w olEnemyBox
    pha
    lda.w olEnemyNav+2
    pha
    lda.w olEnemyNav
    pha
    jsr.l Nav_getWaypoint
    tsc
    clc
    adc #10
    tcs
    ldx.w olEnemyNav
    lda.w 8228,x
    sec
    sbc.w olEnemyPos
    sta.b tcc__r2
    lda.w 8230,x
    sbc.w olEnemyPos+2
    sta.b tcc__r2h
    lda.w 8236,x
    sec
    sbc.w olEnemyPos+8
    sta.b tcc__r3
    lda.w 8238,x
    sbc.w olEnemyPos+10
    sta.b tcc__r3h
ET_Fits:
    lda.b tcc__r2h
    beq ET_PosX
    cmp #$FFFF
    bne ET_Shift
    lda.b tcc__r2
    cmp #$8001
    bcc ET_Shift
    bra ET_Z
ET_PosX:
    lda.b tcc__r2
    bmi ET_Shift
ET_Z:
    lda.b tcc__r3h
    beq ET_PosZ
    cmp #$FFFF
    bne ET_Shift
    lda.b tcc__r3
    cmp #$8001
    bcc ET_Shift
    bra ET_Return
ET_PosZ:
    lda.b tcc__r3
    bmi ET_Shift
ET_Return:
    lda 12,s
    sta.b tcc__r0
    lda 14,s
    sta.b tcc__r0h
    lda.b tcc__r2
    sta [tcc__r0]
    lda 16,s
    sta.b tcc__r0
    lda 18,s
    sta.b tcc__r0h
    lda.b tcc__r3
    sta [tcc__r0]
    lda #1
    sta.b tcc__r0
    rtl
ET_False:
    stz.b tcc__r0
    rtl
ET_Shift:
    lda.b tcc__r2h
    cmp #$8000
    ror.b tcc__r2h
    ror.b tcc__r2
    lda.b tcc__r3h
    cmp #$8000
    ror.b tcc__r3h
    ror.b tcc__r3
    bra ET_Fits

; X -> item, Y -> destination point. All tcc scratch and X/Y may change.
ET_Point:
    lda.w 8,x
    pha
    lda.w 6,x
    pha
    lda.w 4,x
    pha
    lda.w 1,x
    and #$00FF
    pha
    lda #$7E
    pha
    phy
    jsr.l Nav_point
    tsc
    clc
    adc #12
    tcs
    rts
ET_Box:
    lda.w 8,x
    pha
    lda.w 4,x
    pha
    lda.w 1,x
    and #$00FF
    pha
    jsr.l navBoxAt
    pla
    pla
    pla
    rts

; Traverse the corridor's ordered clipping operations and queue/box links
; natively, without far-pointer spills
; or multiplying a cell index by eight through the compiler runtime.
; Locals: iterations 1,s / current box 3,s / last box 5,s (six bytes).
Nav_getWaypoint:
    tsc
    sec
    sbc #6
    tcs
    lda #0
    sta 1,s
    lda 14,s
    sta 3,s
    pha
    lda 14,s
    pha
    lda 14,s
    pha
    jsr.l Nav_expand
    pla
    pla
    pla
    lda 16,s
    sta.b tcc__r0
    lda 18,s
    sta.b tcc__r0h
    lda 10,s
    tax
    clc
    adc #8228
    sta.b tcc__r1
    ; Every Nav context is in DB=$7E; the source point remains a far pointer.
    ldy #10
-   lda [tcc__r0],y
    sta (tcc__r1),y
    dey
    dey
    bpl -
    lda 3,s
    cmp.w olBoxCount
    bcc +
    jmp NW_Return
+   cmp.w NN_END,x
    bne NW_Corridor
    ldy #10
-   lda.w 8216,x
    sta.w 8228,x
    inx
    inx
    dey
    dey
    bpl -
    jmp NW_Return
NW_Corridor:
    ldy #10
-   lda [tcc__r0],y
    sta.w olNavClipFrom,y
    dey
    dey
    bpl -
    lda.b tcc__r1
    sta.w olNavClipWP
    lda #$7E
    sta.w olNavClipWP+2
    lda 3,s
    pha
    jsr.l Nav_bounds
    pla
    jsr NW_CopyBounds
    lda #15
    sta.w olNavClip
    lda 3,s
    sta 5,s
    ; The first box is already in both bound sets. Test the source as usual,
    ; but do not copy those same sixteen bytes again when it is inside.
    jsr.l Nav_inside
    lda.b tcc__r0
    bne NW_Next
    bra NW_Clip
NW_Loop:
    lda 3,s
    sta 5,s
    pha
    jsr.l Nav_bounds
    pla
NW_Inside:
    jsr.l Nav_inside
    lda.b tcc__r0
    beq NW_Clip
    jsr NW_CopyBounds
    bra NW_Next
NW_Clip:
    jsr.l Nav_clipMinX
    lda.b tcc__r0
    beq +
    jmp NW_Return
+   jsr.l Nav_clipMaxX
    lda.b tcc__r0
    beq +
    jmp NW_Return
+   jsr.l Nav_clipMinZ
    lda.b tcc__r0
    beq +
    jmp NW_Return
+   jsr.l Nav_clipMaxZ
    lda.b tcc__r0
    bne NW_Return
NW_Next:
    lda 10,s
    tax
    lda 3,s
    cmp.w NN_END,x
    bne NW_Link
    lda 12,s
    pha
    lda 12,s
    pha
    jsr.l Nav_finish
    pla
    pla
    bra NW_Return
NW_Link:
    asl a
    asl a
    asl a
    clc
    adc 10,s
    tax
    lda.w 4,x
    sta 3,s
    cmp.w olBoxCount
    bcs NW_DeadEnd
    asl a
    asl a
    asl a
    clc
    HOST_ADD_BASE olBoxBase
    tax
    HOST_LDA olBoxData, 6
    sta.b tcc__r0
    lda 10,s
    tax
    lda.b tcc__r0
    and.w NN_MASK,x
    bne NW_DeadEnd
    lda 1,s
    inc a
    sta 1,s
    cmp.w olBoxCount
    bcs NW_DeadEnd
    jmp NW_Loop
NW_DeadEnd:
    lda 5,s
    pha
    lda 14,s
    pha
    lda 14,s
    pha
    jsr.l Nav_deadEnd
    pla
    pla
    pla
NW_Return:
    tsc
    clc
    adc #6
    tcs
    rtl
NW_CopyBounds:
    ; Fixed sixteen-byte workspace, with no index setup or loop branches.
    lda.w olNavBounds+14
    sta.w olNavClipBounds+14
    lda.w olNavBounds+12
    sta.w olNavClipBounds+12
    lda.w olNavBounds+10
    sta.w olNavClipBounds+10
    lda.w olNavBounds+8
    sta.w olNavClipBounds+8
    lda.w olNavBounds+6
    sta.w olNavClipBounds+6
    lda.w olNavBounds+4
    sta.w olNavClipBounds+4
    lda.w olNavBounds+2
    sta.w olNavClipBounds+2
    lda.w olNavBounds
    sta.w olNavClipBounds
    rts

navInvalidateSectors:
    ldx #504
    lda #$FFFF
-   sta.w olNavSectorCache,x
    dex
    dex
    dex
    dex
    dex
    dex
    dex
    dex
    bpl -
    rtl

; Immutable level map cache. Direct navSectorBox remains the codec oracle;
; this wrapper is used only by the room-aware gameplay point lookup.
navCachedSectorBox:
    lda 4,s
    cmp.w olSectorCount
    bcs NX_Invalid
    and #63
    asl a
    asl a
    asl a
    tax
    lda 4,s
    cmp.w olNavSectorCache,x
    bne NX_Miss
    lda 6,s
    cmp.w olNavSectorCache+2,x
    bne NX_Miss
    lda.w olSectorCount
    cmp.w olNavSectorCache+6,x
    bne NX_Miss
    lda.w olNavSectorCache+4,x
    sta.b tcc__r0
    rtl
NX_Invalid:
    lda #$FFFF
    sta.b tcc__r0
    rtl
NX_Miss:
    phx
    lda 8,s                     ; depth, then original sector
    pha
    lda 8,s
    pha
    jsr.l navSectorBox
    pla
    pla
    plx
    lda 4,s
    sta.w olNavSectorCache,x
    lda 6,s
    sta.w olNavSectorCache+2,x
    lda.b tcc__r0
    sta.w olNavSectorCache+4,x
    lda.w olSectorCount
    sta.w olNavSectorCache+6,x
    rtl

Nav_point:
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    lda 8,s                     ; room's ten-byte record
    asl a
    sta.b tcc__r1
    asl a
    asl a
    clc
    adc.b tcc__r1
    HOST_ADD_BASE olRoomBase
    tax
    lda 10,s
    sec
    HOST_SBC olRoomData, 2
    sta.b tcc__r1               ; signed wrapped room-relative X
    HOST_LDA olRoomData, 0
    and #$00FF
    jsr NP_Coord
    lda.b tcc__r2
    sta [tcc__r0]
    ldy #2
    lda.b tcc__r2h
    sta [tcc__r0],y
    lda 12,s
    ldy #4
    sta [tcc__r0],y
    cmp #$8000
    bcc +
    lda #$FFFF
    bra ++
+   lda #0
++  ldy #6
    sta [tcc__r0],y
    lda 14,s
    sec
    HOST_SBC olRoomData, 4
    sta.b tcc__r1
    HOST_LDA olRoomData, 1
    and #$00FF
    jsr NP_Coord
    ldy #8
    lda.b tcc__r2
    sta [tcc__r0],y
    ldy #10
    lda.b tcc__r2h
    sta [tcc__r0],y
    rtl
NP_Coord:
    phx                        ; the caller still needs its room-record index
    asl a
    asl a
    tax                        ; exact full-width sector*1024, shared with bounds
    lda.b tcc__r1
    bpl +
    lda #$FFFF
    bra ++
+   lda #0
++  sta.b tcc__r3
    lda.l NB_Units,x
    clc
    adc.b tcc__r1
    sta.b tcc__r2
    lda.l NB_Units+2,x
    adc.b tcc__r3
    sta.b tcc__r2h
    plx
    rts

; Mood helpers share this serial CPU workspace. Preserve full 32-bit world
; comparisons and the exact PRNG call count; C_MOTION keeps their C bodies.
; Both normal and wild ATTACK remain ATTACK while Lara lives, the enemy is
; uninjured and both occupy the same zone. A valid end box also rules out
; the final cold target seed. No other mood/transition takes this shortcut.
Enemy_targetMood:
    ldx.w olNavEnemy
    lda.w 23,x
    and #$00FF
    cmp #2
    bne EM_Cold
    lda.w 24,x
    and #8
    bne EM_Cold
    lda.w olEnemyInZone
    beq EM_Cold
    ldx.w olNavLara
    lda.w 16,x
    beq EM_Cold
    bmi EM_Cold
    ldx.w olEnemyNav
    lda.w NN_END,x
    cmp #$FFFF
    beq EM_Cold
    jml Enemy_moodChoose
EM_Cold:
    jml Enemy_targetMoodCold

Enemy_moodChoose:
    ldx.w olNavEnemy
    lda.w 23,x
    and #$00FF
    cmp #2
    beq +
    jml Enemy_moodChooseCold
+   jsr.l rand_logic
    ldx.w olNavEnemy
    lda.w 0,x
    and #$00FF
    ldy #$0400
    cmp #7
    beq EMC_Wolf
    cmp #12
    beq EMC_LionMale
    cmp #13
    beq EMC_Wolf
    cmp #14
    beq EMC_Wolf
    cmp #8
    beq EMC_Raptor
    cmp #19
    bne EMC_Roll
EMC_Raptor:
    ldy #$4000
    bra EMC_Roll
EMC_Wolf:
    ldy #$2000
    bra EMC_Roll
EMC_LionMale:
    ldy #$7FFF
EMC_Roll:
    cpy.b tcc__r0
    bcc EMC_Return
    beq EMC_Return
    ldx.w olEnemyNav
    lda.w olTargetPos
    sta.w 8216,x
    lda.w olTargetPos+2
    sta.w 8218,x
    lda.w olTargetPos+4
    sta.w 8220,x
    lda.w olTargetPos+6
    sta.w 8222,x
    lda.w olTargetPos+8
    sta.w 8224,x
    lda.w olTargetPos+10
    sta.w 8226,x
    lda.w olTargetBox
    sta.w NN_NEXT,x
    lda.w NN_ZONE,x
    cmp #2                     ; only fly targets use Lara's upper bound
    bne EMC_Return
    ldx.w olNavLara
    lda.w 12,x
    asl a
    asl a
    sta.b tcc__r0
    asl a
    clc
    adc.b tcc__r0
    HOST_ADD_BASE olAnimBoxBase
    tax
    HOST_LDA olAnimBoxData, 4
    sta.b tcc__r1
    ldy #0
    cmp #0
    bpl +
    dey
+   sty.b tcc__r2
    ldx.w olEnemyNav
    lda.w 8220,x
    clc
    adc.b tcc__r1
    sta.w 8220,x
    lda.w 8222,x
    adc.b tcc__r2
    sta.w 8222,x
EMC_Return:
    rtl

Enemy_randomBox:
    jsr.l rand_logic
    lda.b tcc__r0
    sta.b tcc__r9
    stz.b tcc__r9h
    ldx.w olEnemyNav
    lda.w NN_COUNT,x
    sta.b tcc__r10
    stz.b tcc__r10h
    jsr.l tcc__mull
    stx.b tcc__r1
    tya
    asl a
    rol.b tcc__r1               ; exact (rand * cellsCount) >> 15 low word
    lda.b tcc__r1
    asl a
    asl a
    asl a
    clc
    adc.w olEnemyNav
    tax
    lda.w 0,x
    sta.b tcc__r0
    rtl

Enemy_checkZone:
    lda 4,s
    cmp.w olBoxCount
    bcc +
    jmp EC_False
+   asl a
    sta.b tcc__r1
    ldx.w olEnemyNav
    lda.w NN_ZONE,x
    HOST_ZONE_OFFSET
    clc
    HOST_ADD_BASE olZoneBase
    sta.b tcc__r2
    clc
    adc.b tcc__r1
    tax
    HOST_LDA olZoneData, 0
    sta.b tcc__r3
    lda.w olEnemyBox
    asl a
    clc
    adc.b tcc__r2
    tax
    HOST_LDA olZoneData, 0
    cmp.b tcc__r3
    beq +
    jmp EC_False
+   lda 4,s
    asl a
    asl a
    asl a
    clc
    HOST_ADD_BASE olBoxBase
    tax
    HOST_LDA olBoxData, 2
    and #$00FF
    jsr NB_Scale
    ldy #0
    jsr EC_Compare
    bcs EC_True                 ; enemy <= minimum
    HOST_LDA olBoxData, 3
    and #$00FF
    jsr EC_Max
    ldy #0
    jsr EC_Compare
    bcc EC_True                 ; enemy >= maximum - 1
    beq EC_True
    HOST_LDA olBoxData, 0
    and #$00FF
    jsr NB_Scale
    ldy #8
    jsr EC_Compare
    bcs EC_True
    HOST_LDA olBoxData, 1
    and #$00FF
    jsr EC_Max
    ldy #8
    jsr EC_Compare
    bcc EC_True
    beq EC_True
EC_False:
    stz.b tcc__r0
    rtl
EC_True:
    lda #1
    sta.b tcc__r0
    rtl
EC_Max:
    jsr NB_Scale
    lda.b tcc__r0
    sec
    sbc #1
    sta.b tcc__r0
    lda.b tcc__r0h
    sbc #0
    sta.b tcc__r0h
    rts
EC_Compare:
    lda.w olEnemyPos+2,y
    eor #$8000
    sta.b tcc__r4
    lda.b tcc__r0h
    eor #$8000
    cmp.b tcc__r4
    bne +
    lda.b tcc__r0
    cmp.w olEnemyPos,y
+   rts

; Original stalking-box predicate. Both coordinate sums/subtractions stay
; 32-bit. No local frame, table copy, PRNG roll or mutable navigation cache.
; r2 retains a quadrant/x-sign, r3 Lara's quadrant; axis helpers use r0/r1.
Enemy_checkStalk:
    lda 4,s
    asl a
    asl a
    asl a
    clc
    HOST_ADD_BASE olBoxBase
    tax
    HOST_LDA olBoxData, 2
    ldy #0
    jsr NSK_BoxAxis
    bcs +
    jmp NSK_False
+   sta.b tcc__r2
    HOST_LDA olBoxData, 0
    ldy #8
    jsr NSK_BoxAxis
    bcs +
    jmp NSK_False
+   jsr NSK_Quadrant
    sta.b tcc__r2
    lda.w olNavLara
    sta.b tcc__r0
    lda.w olNavLara+2
    sta.b tcc__r0h
    ldy #11                    ; high angle byte through the full far pointer
    lda [tcc__r0],y
    and #$00C0
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    eor #2                     ; ((int16)angle >> 14) + 2
    sta.b tcc__r3
    cmp.b tcc__r2
    beq NSK_False
    eor.b tcc__r2
    cmp #2                     ; abs(qLara - qBox) == 2
    bne NSK_True
    ldy #0
    jsr NSK_EnemyAxis
    sta.b tcc__r2
    ldy #8
    jsr NSK_EnemyAxis
    jsr NSK_Quadrant
    cmp.b tcc__r3
    beq NSK_False
NSK_True:
    lda #1
    sta.b tcc__r0
    rtl
NSK_False:
    stz.b tcc__r0
    rtl

; A=packed min/max bytes, X=ROM record, Y=coordinate offset (0/8).
; Carry set iff abs((min+max)*512-target) <= 3072; A=delta strictly > 0.
; Bias before the wide subtraction permits one unsigned range comparison.
NSK_BoxAxis:
    sta.b tcc__r1
    and #$00FF
    sta.b tcc__r0
    lda.b tcc__r1
    xba
    and #$00FF
    clc
    adc.b tcc__r0              ; exact nine-bit sum, 0..510
    sta.b tcc__r1
    xba
    and #$FF00
    asl a
    sta.b tcc__r0
    lda.b tcc__r1
    xba
    and #1
    rol a                      ; high word of sum*512, including bit 7 carry
    sta.b tcc__r0h
    lda.b tcc__r0
    clc
    adc #3072
    sta.b tcc__r0
    lda.b tcc__r0h
    adc #0
    sta.b tcc__r0h
    lda.b tcc__r0
    sec
    sbc.w olTargetPos,y
    sta.b tcc__r0
    lda.b tcc__r0h
    sbc.w olTargetPos+2,y
    bne NSK_Outside
    lda.b tcc__r0
    cmp #6145
    bcs NSK_Outside
    cmp #3073
    lda #0
    adc #0                     ; > 0, not >= 0
    sec
    rts
NSK_Outside:
    clc
    rts

NSK_EnemyAxis:
    lda.w olEnemyPos,y
    sec
    sbc.w olTargetPos,y
    sta.b tcc__r0
    lda.w olEnemyPos+2,y
    sbc.w olTargetPos+2,y
    bmi NSK_NonPositive
    bne NSK_Positive
    lda.b tcc__r0
    beq NSK_NonPositive
NSK_Positive:
    lda #1
    rts
NSK_NonPositive:
    lda #0
    rts

; A=dz>0, r2=dx>0. Return original quadrants 0/3/1/2, preserving r3.
NSK_Quadrant:
    ldx.b tcc__r2
    beq +
    eor #1
    inc a
    inc a
+   rts

; Ten local bytes: full-width dx 1/3,s, dz 5/7,s, head offset 9,s.
Enemy_moodDistance:
    tsc
    sec
    sbc #10
    tcs
    lda.w olTargetPos
    sec
    sbc.w olEnemyPos
    sta 1,s
    lda.w olTargetPos+2
    sbc.w olEnemyPos+2
    sta 3,s
    lda.w olTargetPos+8
    sec
    sbc.w olEnemyPos+8
    sta 5,s
    lda.w olTargetPos+10
    sbc.w olEnemyPos+10
    sta 7,s
    ldx.w olNavEnemy
    lda.w 0,x
    and #$00FF
    cmp #7
    beq MD_Wolf
    cmp #8
    beq MD_Bear
    cmp #19
    beq MD_Lion
    cmp #12
    bcc MD_Abs
    cmp #15
    bcs MD_Abs
MD_Lion:
    lda #400
    bra +
MD_Bear:
    lda #500
    bra +
MD_Wolf:
    lda #375
+   sta 9,s
    lda.w 10,x
    jsr MD_Head
    sta.b tcc__r1
    ldx #0
    cmp #0
    bpl +
    dex
+   stx.b tcc__r2
    lda 1,s
    sec
    sbc.b tcc__r1
    sta 1,s
    lda 3,s
    sbc.b tcc__r2
    sta 3,s
    ldx.w olNavEnemy
    lda.w 10,x
    clc
    adc #$4000
    jsr MD_Head
    sta.b tcc__r1
    ldx #0
    cmp #0
    bpl +
    dex
+   stx.b tcc__r2
    lda 5,s
    sec
    sbc.b tcc__r1
    sta 5,s
    lda 7,s
    sbc.b tcc__r2
    sta 7,s
MD_Abs:
    lda 3,s
    bpl +
    lda 1,s
    eor #$FFFF
    clc
    adc #1
    sta 1,s
    lda 3,s
    eor #$FFFF
    adc #0
    sta 3,s
+   lda 7,s
    bpl +
    lda 5,s
    eor #$FFFF
    clc
    adc #1
    sta 5,s
    lda 7,s
    eor #$FFFF
    adc #0
    sta 7,s
+   lda 7,s
    eor #$8000
    sta.b tcc__r2
    lda 3,s
    eor #$8000
    cmp.b tcc__r2
    bne +
    lda 1,s
    cmp 5,s
+   bcc MD_ZGreater
    lda 7,s
    cmp #$8000
    ror a
    sta.b tcc__r2
    lda 5,s
    ror a
    clc
    adc 1,s
    sta.b tcc__r0
    lda.b tcc__r2
    adc 3,s
    bra MD_Saturate
MD_ZGreater:
    lda 3,s
    cmp #$8000
    ror a
    sta.b tcc__r2
    lda 1,s
    ror a
    clc
    adc 5,s
    sta.b tcc__r0
    lda.b tcc__r2
    adc 7,s
MD_Saturate:
    bmi MD_Return
    bne MD_Max
    lda.b tcc__r0
    cmp #$8000
    bcc MD_Return
MD_Max:
    lda #$7FFF
    sta.b tcc__r0
MD_Return:
    tsc
    clc
    adc #10
    tcs
    rtl

; A=angle. Exact signed (sin(angle) * offset) >> 14 in A. Product is
; bounded to +/-500, so only two low-product bits enter the final word.
MD_Head:
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
    php
    bpl +
    eor #$FFFF
    inc a
+   sta.b tcc__r9
    stz.b tcc__r9h
    lda 12,s                    ; caller's offset beyond JSR return + PHP
    sta.b tcc__r10
    stz.b tcc__r10h
    jsr.l tcc__mull
    ; Restore the full signed product before taking its arithmetic >> 14.
    ; Positive magnitudes allow the CPU multiplier for negative trig too.
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
+
    stx.b tcc__r1
    tya
    asl a
    rol.b tcc__r1
    asl a
    rol.b tcc__r1
    lda.b tcc__r1
    rts

Nav_init:
    lda 4,s
    sta.b tcc__r0
    tax
    lda.w NN_STEP,x
    cmp #256
    beq NI_Ground1
    cmp #512
    beq NI_Ground2
    lda #2                     ; ZONE_FLY
    bra NI_KindReady
NI_Ground2:
    lda #1
    bra NI_KindReady
NI_Ground1:
    lda #0
NI_KindReady:
    sta.w NN_ZONE,x
    stz.w NN_WEIGHT,x
    stz.w NN_COUNT,x
    lda #$4000
    sta.w NN_MASK,x
    lda #$FFFF
    sta.w NN_END,x
    sta.w NN_NEXT,x
    sta.w NN_HEAD,x
    sta.w NN_TAIL,x
    lda.w olBoxCount
    bne +
    jmp NI_Return
+   and #31
    sta.b tcc__r5
    lda.w olBoxCount
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sta.b tcc__r4               ; retain the 32-cell batch count for both passes
    tay
    lda #$FFFF
    cpy #0
    bne +
    jmp NI_ClearTail
+
    ; Clear 32 cells per stride. Leave EVERY boxIndex word untouched;
    ; only the subsequent compact zone list is allowed to replace those.
NI_ClearBatch:
    stz.w 2,x
    sta.w 4,x
    sta.w 6,x
    stz.w 10,x
    sta.w 12,x
    sta.w 14,x
    stz.w 18,x
    sta.w 20,x
    sta.w 22,x
    stz.w 26,x
    sta.w 28,x
    sta.w 30,x
    stz.w 34,x
    sta.w 36,x
    sta.w 38,x
    stz.w 42,x
    sta.w 44,x
    sta.w 46,x
    stz.w 50,x
    sta.w 52,x
    sta.w 54,x
    stz.w 58,x
    sta.w 60,x
    sta.w 62,x
    stz.w 66,x
    sta.w 68,x
    sta.w 70,x
    stz.w 74,x
    sta.w 76,x
    sta.w 78,x
    stz.w 82,x
    sta.w 84,x
    sta.w 86,x
    stz.w 90,x
    sta.w 92,x
    sta.w 94,x
    stz.w 98,x
    sta.w 100,x
    sta.w 102,x
    stz.w 106,x
    sta.w 108,x
    sta.w 110,x
    stz.w 114,x
    sta.w 116,x
    sta.w 118,x
    stz.w 122,x
    sta.w 124,x
    sta.w 126,x
    stz.w 130,x
    sta.w 132,x
    sta.w 134,x
    stz.w 138,x
    sta.w 140,x
    sta.w 142,x
    stz.w 146,x
    sta.w 148,x
    sta.w 150,x
    stz.w 154,x
    sta.w 156,x
    sta.w 158,x
    stz.w 162,x
    sta.w 164,x
    sta.w 166,x
    stz.w 170,x
    sta.w 172,x
    sta.w 174,x
    stz.w 178,x
    sta.w 180,x
    sta.w 182,x
    stz.w 186,x
    sta.w 188,x
    sta.w 190,x
    stz.w 194,x
    sta.w 196,x
    sta.w 198,x
    stz.w 202,x
    sta.w 204,x
    sta.w 206,x
    stz.w 210,x
    sta.w 212,x
    sta.w 214,x
    stz.w 218,x
    sta.w 220,x
    sta.w 222,x
    stz.w 226,x
    sta.w 228,x
    sta.w 230,x
    stz.w 234,x
    sta.w 236,x
    sta.w 238,x
    stz.w 242,x
    sta.w 244,x
    sta.w 246,x
    stz.w 250,x
    sta.w 252,x
    sta.w 254,x
    txa
    clc
    adc #256
    tax
    lda #$FFFF
    dey
    beq NI_ClearTail
    jmp NI_ClearBatch
NI_ClearTail:
    ldy.b tcc__r5
    beq NI_Cleared
NI_Clear:
    stz.w 2,x
    sta.w 4,x
    sta.w 6,x
    txa
    clc
    adc #8
    tax
    lda #$FFFF
    dey
    bne NI_Clear
NI_Cleared:
    lda 8,s
    cmp.w olBoxCount
    bcc +
    jmp NI_Return
+   lda 8,s
    asl a
    sta.b tcc__r1
    ldx.b tcc__r0
    lda.w NN_ZONE,x
    asl a                      ; default and alternate directory entries
    tax
    HOST_LDA olZoneData, 6
    sta.b tcc__r2h
    HOST_LDA olZoneData, 0
    sta.b tcc__r2
    clc
    adc.b tcc__r1
    tax
    HOST_LDA olZoneData, 0
    sta.b tcc__r3               ; default starting zone
    lda.b tcc__r2h
    clc
    adc.b tcc__r1
    tax
    HOST_LDA olZoneData, 0
    sta.b tcc__r3h              ; alternate starting zone
    lda.b tcc__r2h
    clc
    adc.b olZoneData
    sta.b tcc__r2h              ; absolute alternate array within DB
    stz.b tcc__r1               ; box index
    ; Bank-local ROM scanning can use shorter absolute indexed compares.
    ; Keep the WRAM destination a full pointer. NMI saves/restores DB and
    ; never touches the tcc registers; all exits below restore our caller.
    lda #$7E
    sta.b tcc__r0h
    lda.b tcc__r2
    clc
    adc.b olZoneData
    tax
    ldy #0                     ; compact-list byte offset
    phb
    sep #$20
    lda.b olZoneData+2
    pha
    plb
    rep #$20
    cpx.b tcc__r2h
    beq +
    jmp NI_UnionStart
+
    lda.b tcc__r4
    bne +
    jmp NI_ZoneTail
+   lda.b tcc__r3
    ; Keep the sought zone in A across misses. Unrolling removes the
    ; per-box index increment/count load/compare from cold admission;
    ; a hit still writes only the original compact-list boxIndex word.
.MACRO NN_ZONE_CELL
    cmp.w \1*2,x
    bne +
    lda.b tcc__r1
    .IF \1 != 0
    adc #\1-1                 ; matching CMP supplied carry=1; no index overflow
    .ENDIF
    sta [tcc__r0],y
    tya
    .IF \1 == 0
    adc #7                     ; carry still 1 from CMP
    .ELSE
    adc #8
    .ENDIF                     ; nonzero cell's index sum left carry=0
    tay
    lda.b tcc__r3
+
.ENDM
NI_ZoneBatch:
    NN_ZONE_CELL 0
    NN_ZONE_CELL 1
    NN_ZONE_CELL 2
    NN_ZONE_CELL 3
    NN_ZONE_CELL 4
    NN_ZONE_CELL 5
    NN_ZONE_CELL 6
    NN_ZONE_CELL 7
    NN_ZONE_CELL 8
    NN_ZONE_CELL 9
    NN_ZONE_CELL 10
    NN_ZONE_CELL 11
    NN_ZONE_CELL 12
    NN_ZONE_CELL 13
    NN_ZONE_CELL 14
    NN_ZONE_CELL 15
    NN_ZONE_CELL 16
    NN_ZONE_CELL 17
    NN_ZONE_CELL 18
    NN_ZONE_CELL 19
    NN_ZONE_CELL 20
    NN_ZONE_CELL 21
    NN_ZONE_CELL 22
    NN_ZONE_CELL 23
    NN_ZONE_CELL 24
    NN_ZONE_CELL 25
    NN_ZONE_CELL 26
    NN_ZONE_CELL 27
    NN_ZONE_CELL 28
    NN_ZONE_CELL 29
    NN_ZONE_CELL 30
    NN_ZONE_CELL 31
    txa
    clc
    adc #64
    tax
    lda.b tcc__r1
    clc
    adc #32
    sta.b tcc__r1
    lda.b tcc__r3
    dec.b tcc__r4
    beq NI_ZoneTail
    jmp NI_ZoneBatch
NI_ZoneTail:
    lda.b tcc__r5
    beq NI_ZoneDone
    lda.b tcc__r3
NI_Zone:
    NN_ZONE_CELL 0
NI_Next:
    inx
    inx
    inc.b tcc__r1
    dec.b tcc__r5
    bne NI_Zone
NI_ZoneDone:
    ; Y already counts every compact-list write in eight-byte strides.
    ; Recover cellsCount once instead of incrementing another word per hit.
    plb
    tya
    lsr a
    lsr a
    lsr a
    ldx.b tcc__r0
    sta.w NN_COUNT,x
NI_Return:
    rtl

; Distinct default/alternate maps contribute their union, in original box
; order, without duplicate entries. Both source arrays share the ZONE bank.
; X/Y are absolute source cursors; r0 advances through compact-list output.
NI_UnionStart:
    ldy.b tcc__r2h
    lda.b tcc__r4
    bne +
    jmp NI_UnionTail
+   lda.b tcc__r3
.MACRO NN_UNION_CELL
    cmp.w \1*2,x
    beq _nu_hit\@
    lda.w \1*2,y
    cmp.b tcc__r3h
    bne _nu_next\@
_nu_hit\@:
    lda.b tcc__r1
    .IF \1 != 0
    adc #\1-1                 ; either successful CMP supplied carry=1
    .ENDIF
    sta [tcc__r0]
    lda.b tcc__r0
    .IF \1 == 0
    adc #7
    .ELSE
    adc #8
    .ENDIF                     ; same carry contract as the single-map scan
    sta.b tcc__r0
_nu_next\@:
    lda.b tcc__r3
.ENDM
NI_UnionBatch:
    NN_UNION_CELL 0
    NN_UNION_CELL 1
    NN_UNION_CELL 2
    NN_UNION_CELL 3
    NN_UNION_CELL 4
    NN_UNION_CELL 5
    NN_UNION_CELL 6
    NN_UNION_CELL 7
    NN_UNION_CELL 8
    NN_UNION_CELL 9
    NN_UNION_CELL 10
    NN_UNION_CELL 11
    NN_UNION_CELL 12
    NN_UNION_CELL 13
    NN_UNION_CELL 14
    NN_UNION_CELL 15
    NN_UNION_CELL 16
    NN_UNION_CELL 17
    NN_UNION_CELL 18
    NN_UNION_CELL 19
    NN_UNION_CELL 20
    NN_UNION_CELL 21
    NN_UNION_CELL 22
    NN_UNION_CELL 23
    NN_UNION_CELL 24
    NN_UNION_CELL 25
    NN_UNION_CELL 26
    NN_UNION_CELL 27
    NN_UNION_CELL 28
    NN_UNION_CELL 29
    NN_UNION_CELL 30
    NN_UNION_CELL 31
    txa
    clc
    adc #64
    tax
    tya
    clc
    adc #64
    tay
    lda.b tcc__r1
    clc
    adc #32
    sta.b tcc__r1
    lda.b tcc__r3
    dec.b tcc__r4
    beq NI_UnionTail
    jmp NI_UnionBatch
NI_UnionTail:
    lda.b tcc__r5
    beq NI_UnionDone
    lda.b tcc__r3
NI_UnionCell:
    NN_UNION_CELL 0
    inx
    inx
    iny
    iny
    inc.b tcc__r1
    dec.b tcc__r5
    bne NI_UnionCell
NI_UnionDone:
    plb
    lda.b tcc__r0
    sec
    sbc 4,s
    lsr a
    lsr a
    lsr a
    pha
    lda 6,s
    tax
    pla
    sta.w NN_COUNT,x
    rtl

; Original queue admission/generation and five-step expansion, without a
; C temporary frame. Preserve the target across rebase and the initial zone
; across searches; all persistent state remains in the owning Nav context.
Nav_expand:
    tsc
    sec
    sbc #6
    tcs
    ; 1,s target across rebase; 3,s zone; 5,s steps left.
    ; Arguments: 10/12,s Nav pointer, 14,s current box.
    lda 10,s
    tax
    lda.w NN_NEXT,x
    cmp.w olBoxCount
    bcs NE_Queue
    cmp.w NN_END,x
    beq NE_Queue
    sta 1,s
    lda.w NN_WEIGHT,x
    cmp #$7FFF
    bne NE_Target
    lda 14,s
    pha
    lda 14,s
    pha
    lda 14,s
    pha
    jsr.l Nav_init
    tsc
    clc
    adc #6
    tcs
    lda 10,s
    tax
    lda 1,s
    sta.w NN_NEXT,x
NE_Target:
    lda.w NN_NEXT,x
    sta.w NN_END,x
    asl a
    asl a
    asl a
    clc
    adc 10,s
    tay
    lda.w 6,y
    cmp #$FFFF
    bne NE_Weight
    lda.w NN_TAIL,x
    cmp.w NN_END,x
    beq NE_Weight
    lda.w NN_HEAD,x
    sta.w 6,y
    cmp #$FFFF
    bne +
    lda.w NN_END,x
    sta.w NN_TAIL,x
+   lda.w NN_END,x
    sta.w NN_HEAD,x
NE_Weight:
    inc.w NN_WEIGHT,x
    lda.w NN_WEIGHT,x
    sta.w 2,y
    lda #$FFFF
    sta.w 4,y
NE_Queue:
    lda.w NN_HEAD,x
    cmp #$FFFF
    beq NE_Return
    asl a
    sta.b tcc__r0
    lda.w NN_ZONE,x
    HOST_ZONE_OFFSET
    clc
    adc.b tcc__r0
    HOST_ADD_BASE olZoneBase
    tax
    HOST_LDA olZoneData, 0
    sta 3,s
    lda #5
    sta 5,s
NE_Search:
    lda 3,s
    pha
    lda 14,s
    pha
    lda 14,s
    pha
    jsr.l Nav_search
    tsc
    clc
    adc #6
    tcs
NE_NextSearch:
    lda 10,s
    tax
    lda.w NN_HEAD,x
    cmp #$FFFF
    beq NE_Return
    lda 5,s
    dec a
    sta 5,s
    beq NE_Return
    ; Nav_search preserves the context, zone pointer/value and signed step
    ; limits in scratch. Reuse that setup only within this uninterrupted
    ; five-step batch. NMI never enters Nav or uses these registers.
    jsr.l Nav_searchNext
    bra NE_NextSearch
NE_Return:
    tsc
    clc
    adc #6
    tcs
    rtl

Nav_search:
    lda 4,s
    sta.b tcc__r0
    tax
    lda.w NN_STEP,x
    eor #$8000
    sta.b tcc__f2               ; signed-biased step; table reads use Y
    lda.w NN_DROP,x
    eor #$8000
    sta.b tcc__r0h             ; r0 is a bank-local pointer; high half is free
    lda 8,s
    sta.b tcc__r3               ; zone searched
    lda.w NN_ZONE,x
    HOST_ZONE_OFFSET
    clc
    HOST_ADD_BASE olZoneBase
    sta.b tcc__r2h
; Internal continuation: no stack arguments. Used only by Nav_expand while
; r0=context, r0h=drop, f2=step, r3=zone and r2h=zone offset remain valid.
Nav_searchNext:
    ldx.b tcc__r0
    lda.w NN_HEAD,x
    sta.b tcc__r1h              ; current box index
    asl a
    asl a
    asl a
    sta.b tcc__r1
    clc
    HOST_ADD_BASE olBoxBase
    tax
    HOST_LDA olBoxData, 4
    sta.b tcc__r3h              ; current floor
    HOST_LDA_NEXT olBoxData, 6, 2 ; Y still names this box's floor word
    and #$3FFF
    asl a
    clc
    HOST_ADD_BASE olOverlapBase
    sta.b tcc__r2               ; overlap byte cursor
    lda.b tcc__r1
    clc
    adc.b tcc__r0
    sta.b tcc__r1               ; current cell pointer
    tax
    lda.w 2,x
    sta.b tcc__r4               ; full current weight
    and #$7FFF
    sta.b tcc__r10              ; generation only
    ldy.b tcc__r2               ; keep the overlap cursor live across visited cells
NS_Loop:
    lda [olOverlapData],y       ; independent CPU FastROM pointer, including $F0-$FF
    sta.b tcc__r5h              ; sign marks last neighbour
    bpl +
    and #$3FFF
+   iny
    iny
    sta.b tcc__r4h
    cmp.w olBoxCount
    bcs NS_Next
    ; A visited/newer cell cannot change, whatever its zone/floor says.
    ; Reject it before immutable geometry reads. No queue mutation moves
    ; across those predicates, so traversal order and generation rules stay.
    asl a
    asl a
    asl a
    clc
    adc.b tcc__r0
    tax
    lda.w 2,x
    cmp.b tcc__r4               ; identical full weights can never change this cell
    beq NS_Next
NS_Different:
    sta.b tcc__r10h
    and #$7FFF
    cmp.b tcc__r10
    bcc NS_Geometry
    bne NS_Next
NS_Same:
    lda.b tcc__r4
    bmi NS_Next
    lda.b tcc__r10h
    bmi NS_Geometry
NS_Next:
    lda.b tcc__r5h
    bpl NS_Loop
    jmp NS_Done
NS_Geometry:
    stx.b tcc__r5               ; only a possibly changing neighbour needs its pointer
    ; Only geometry readers need Y as a table index. Save the next overlap
    ; cursor here, then restore it on EVERY geometry/append exit. NMI owns
    ; neither these scratch registers nor the search; the stack stays bounded.
    phy
    lda.b tcc__r4h
    asl a
    clc
    adc.b tcc__r2h
    tay
    lda [olZoneData],y
    cmp.b tcc__r3
    beq +
    jmp NS_GeometryNext
+   lda.b tcc__r5              ; reuse the already-scaled cell address
    sec
    sbc.b tcc__r0
    clc
    adc #4
    tay
    lda [olBoxData],y          ; keep the floor cursor through the limit tests
    sec
    sbc.b tcc__r3h
    bvc +                       ; a >16-bit floor delta exceeds every limit
    jmp NS_GeometryNext
+   eor #$8000                  ; signed order -> unsigned order
    sta.b tcc__r9h
    lda.b tcc__f2
    cmp.b tcc__r9h
    bcs +
    jmp NS_GeometryNext
+   lda.b tcc__r0h
    cmp.b tcc__r9h
    bcc +
    beq +
    jmp NS_GeometryNext
+   lda.b tcc__r4
    bpl NS_Open
    ldx.b tcc__r5
    sta.w 2,x
    bra NS_Append
NS_Open:
    HOST_LDA_NEXT olBoxData, 6, 2 ; no bound/predicate above changes Y
    ldx.b tcc__r0
    and.w NN_MASK,x
    beq NS_Unblocked
    lda.b tcc__r4
    ora #$8000
    ldx.b tcc__r5
    sta.w 2,x
    bra NS_Append
NS_Unblocked:
    ldx.b tcc__r5
    lda.b tcc__r4
    sta.w 2,x
    lda.b tcc__r1h
    sta.w 4,x
NS_Append:
    lda.w 6,x
    cmp #$FFFF
    bne NS_GeometryNext
    ldx.b tcc__r0
    lda.w NN_TAIL,x
    cmp.b tcc__r4h
    beq NS_GeometryNext
    asl a
    asl a
    asl a
    clc
    adc.b tcc__r0
    tax
    lda.b tcc__r4h
    sta.w 6,x
    ldx.b tcc__r0
    sta.w NN_TAIL,x
NS_GeometryNext:
    ply
    jmp NS_Next
NS_Done:
    ldx.b tcc__r1
    lda.w 6,x
    ldy #NN_HEAD
    sta (tcc__r0),y
    cmp #$FFFF
    bne +
    ldy #NN_TAIL
    sta (tcc__r0),y
+   lda #$FFFF
    sta.w 6,x
    rtl

; Compare a bound with the source point: signed high words, then unsigned
; low words. C/Z reflect the full 32-bit order; X/Y and globals are intact.
.MACRO NI_COMPARE
    lda.w olNavBounds+\2+2
    cmp.w olNavClipFrom+\1+2
    beq _ni_low\@
    lda.w olNavClipFrom+\1+2
    eor #$8000
    sta.b tcc__r0
    lda.w olNavBounds+\2+2
    eor #$8000
    cmp.b tcc__r0
    bra _ni_done\@
_ni_low\@:
    lda.w olNavBounds+\2
    cmp.w olNavClipFrom+\1
_ni_done\@:
.ENDM

Nav_inside:
    NI_COMPARE 0, 0
    bcc +
    bne NInside_False
+   NI_COMPARE 0, 4
    bcc NInside_False
    NI_COMPARE 8, 8
    bcc +
    bne NInside_False
+   NI_COMPARE 8, 12
    bcc NInside_False
    lda #1
    sta.b tcc__r0
    rtl
NInside_False:
    stz.b tcc__r0
    rtl

; Four symmetric sides of the original corridor algorithm. Arguments are
; flag, axis, other axis, side bound, opposite clipped bound, other min bound,
; and 1 for a maximum side. This emits straight-line code, not a new policy.
.MACRO NC_CLIP
    NI_COMPARE \2, \4
.IF \7
    bcc _nc_work\@
.ELSE
    bcc _nc_noWork\@
    beq _nc_noWork\@
    bra _nc_work\@
_nc_noWork\@:
.ENDIF
    jmp NC_False
_nc_work\@:
    lda.w olNavClip
    and #\1
    bne +
    jmp _nc_other\@
+   NI_COMPARE \3, \6
    bcc +
    beq +
    jmp _nc_other\@
+   NI_COMPARE \3, (\6+4)
    bcs +
    jmp _nc_other\@
+   lda.w olNavBounds+\4
.IF \7
    sec
    sbc #512
    sta.b tcc__r0
    lda.w olNavBounds+\4+2
    sbc #0
.ELSE
    clc
    adc #512
    sta.b tcc__r0
    lda.w olNavBounds+\4+2
    adc #0
.ENDIF
    sta.b tcc__r0h
    lda.w olNavClipWP
    clc
    adc #\2
    tax
.IF \7
    jsr NC_MinTo
.ELSE
    jsr NC_MaxTo
.ENDIF
    lda.w olNavClip
    and #16
    beq +
    jmp NC_True
+   lda.w olNavBounds+\6
    sta.b tcc__r0
    lda.w olNavBounds+\6+2
    sta.b tcc__r0h
    ldx #((olNavClipBounds+\6) & $FFFF)
    jsr NC_MaxTo
    lda.w olNavBounds+\6+4
    sta.b tcc__r0
    lda.w olNavBounds+\6+6
    sta.b tcc__r0h
    ldx #((olNavClipBounds+\6+4) & $FFFF)
    jsr NC_MinTo
    lda #\1
    sta.w olNavClip
    jmp NC_False
_nc_other\@:
    lda.w olNavClip
    cmp #\1
    bne +
    jmp NC_False
+   ldx.w olNavClipWP
    lda.w olNavClipBounds+\5
.IF \7
    clc
    adc #512
    sta.w \2,x
    lda.w olNavClipBounds+\5+2
    adc #0
.ELSE
    sec
    sbc #512
    sta.w \2,x
    lda.w olNavClipBounds+\5+2
    sbc #0
.ENDIF
    sta.w \2+2,x
    lda.w olNavClip
    cmp #15
    beq +
    jmp NC_True
+   lda #16
    sta.w olNavClip
    jmp NC_False
.ENDM

Nav_clipMinX:
    NC_CLIP 1, 0, 8, 0, 4, 8, 0
Nav_clipMaxX:
    NC_CLIP 2, 0, 8, 4, 0, 8, 1
Nav_clipMinZ:
    NC_CLIP 4, 8, 0, 8, 12, 0, 0
Nav_clipMaxZ:
    NC_CLIP 8, 8, 0, 12, 8, 0, 1
NC_True:
    lda #1
    sta.b tcc__r0
    rtl
NC_False:
    stz.b tcc__r0
    rtl

; Local helpers: clamp the signed 32-bit value at $7E:X toward r0:r0h.
; Preserve X/Y. Compare signed high words, then unsigned low words.
NC_MaxTo:
    jsr NC_Order
    bcc NC_NoStore
    bra NC_Store
NC_MinTo:
    jsr NC_Order
    bcs NC_NoStore
NC_Store:
    lda.b tcc__r0
    sta.w 0,x
    lda.b tcc__r0h
    sta.w 2,x
NC_NoStore:
    rts
NC_Order:
    lda.b tcc__r0h
    cmp.w 2,x
    beq NC_OrderLow
    lda.w 2,x
    eor #$8000
    sta.b tcc__r1
    lda.b tcc__r0h
    eor #$8000
    cmp.b tcc__r1
    rts
NC_OrderLow:
    lda.b tcc__r0
    cmp.w 0,x
    rts

Nav_finish:
    lda 4,s
    tay
    lda.w olNavClip
    and #3
    beq NF_ClampX
    ldx.w olNavClipWP
    lda.w 8216,y
    sta.w 0,x
    lda.w 8218,y
    sta.w 2,x
    bra NF_Z
NF_ClampX:
    lda.w olNavClip
    and #16
    bne NF_Z
    ldx.w olNavClipWP
    jsr NC_ClampX
NF_Z:
    lda.w olNavClip
    and #12
    beq NF_ClampZ
    ldx.w olNavClipWP
    lda.w 8224,y
    sta.w 8,x
    lda.w 8226,y
    sta.w 10,x
    bra NF_Y
NF_ClampZ:
    lda.w olNavClip
    and #16
    bne NF_Y
    lda.w olNavClipWP
    clc
    adc #8
    tax
    jsr NC_ClampZ
NF_Y:
    ldx.w olNavClipWP
    lda.w 8220,y
    sta.w 4,x
    lda.w 8222,y
    sta.w 6,x
    rtl

Nav_deadEnd:
    lda.w olNavClip
    and #16
    bne ND_Floor
    ldx.w olNavClipWP
    jsr NC_ClampX
    lda.w olNavClipWP
    clc
    adc #8
    tax
    jsr NC_ClampZ
ND_Floor:
    lda 8,s
    asl a
    asl a
    asl a
    clc
    HOST_ADD_BASE olBoxBase
    tax
    HOST_LDA olBoxData, 4
    sta.b tcc__r0
    ldx #0
    cmp #0
    bpl +
    dex
+   stx.b tcc__r0h
    lda 4,s
    tax
    lda.w NN_ZONE,x
    cmp #2
    bne ND_Store
    lda.b tcc__r0
    sec
    sbc #384
    sta.b tcc__r0
    lda.b tcc__r0h
    sbc #0
    sta.b tcc__r0h
ND_Store:
    ldx.w olNavClipWP
    lda.b tcc__r0
    sta.w 4,x
    lda.b tcc__r0h
    sta.w 6,x
    rtl

.MACRO NC_CLAMP_AXIS
    lda.w olNavBounds+\1
    clc
    adc #512
    sta.b tcc__r0
    lda.w olNavBounds+\1+2
    adc #0
    sta.b tcc__r0h
    jsr NC_MaxTo
    lda.w olNavBounds+\1+4
    sec
    sbc #512
    sta.b tcc__r0
    lda.w olNavBounds+\1+6
    sbc #0
    sta.b tcc__r0h
    jmp NC_MinTo
.ENDM
NC_ClampX:
    NC_CLAMP_AXIS 0
NC_ClampZ:
    NC_CLAMP_AXIS 8

; Corridor helpers keep full signed 32-bit coordinates. The C callers return
; int32 in r0(low):r1(high), NOT r0:r0h (the latter is a far pointer).
Nav_copy:
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r0h
    lda 8,s
    sta.b tcc__r1
    lda 10,s
    sta.b tcc__r1h
    ldy #0
NC_Loop:
    lda [tcc__r1],y
    sta [tcc__r0],y
    iny
    iny
    cpy #12
    bcc NC_Loop
    rtl

Nav_min:
    ldx #0
    bra NM_Compare
Nav_max:
    ldx #1
NM_Compare:
    lda 6,s
    eor #$8000
    sta.b tcc__r0
    lda 10,s
    eor #$8000
    cmp.b tcc__r0
    bne NM_Order
    lda 8,s
    cmp 4,s
NM_Order:                       ; C set iff b >= a, signed high/unsigned low
    txa
    beq NM_Min
    bcc NM_A
    bra NM_B
NM_Min:
    bcs NM_A
NM_B:
    lda 8,s
    sta.b tcc__r0
    lda 10,s
    sta.b tcc__r1
    rtl
NM_A:
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r1
    rtl

Nav_clamp:
    lda 4,s
    sta.b tcc__r0
    lda 6,s
    sta.b tcc__r1
    eor #$8000
    sta.b tcc__r4
    lda 10,s
    eor #$8000
    cmp.b tcc__r4
    bne NL_MinOrder
    lda 8,s
    cmp.b tcc__r0
NL_MinOrder:
    bcc NL_Max
    lda 8,s
    sta.b tcc__r0
    lda 10,s
    sta.b tcc__r1
NL_Max:
    lda.b tcc__r1
    eor #$8000
    sta.b tcc__r4
    lda 14,s
    eor #$8000
    cmp.b tcc__r4
    bne NL_MaxOrder
    lda 12,s
    cmp.b tcc__r0
NL_MaxOrder:
    bcs NL_Return
    lda 12,s
    sta.b tcc__r0
    lda 14,s
    sta.b tcc__r1
NL_Return:
    rtl

Nav_bounds:
    lda 4,s
    asl a
    asl a
    asl a
    tay
    lda [olBoxData],y           ; packed minZ/maxZ, then minX/maxX
    sta.b tcc__r0
    iny
    iny
    lda [olBoxData],y
    sta.b tcc__r0h
    ; Direct table indexing avoids four helper calls, index saves and
    ; result spills per corridor box. Maximum subtraction remains 32-bit:
    ; sector zero is -1 and every low-word zero borrows from the high word.
.MACRO NB_BOUND ARGS packed, dest, maximum
    lda.b packed
.IF maximum
    xba
.ENDIF
    and #$00FF
    asl a
    asl a
    tax
    lda.l NB_Units,x
.IF maximum
    sec
    sbc #1
.ENDIF
    sta.w olNavBounds+dest
    lda.l NB_Units+2,x
.IF maximum
    sbc #0
.ENDIF
    sta.w olNavBounds+dest+2
.ENDM
    NB_BOUND tcc__r0h, 0, 0
    NB_BOUND tcc__r0h, 4, 1
    NB_BOUND tcc__r0, 8, 0
    NB_BOUND tcc__r0, 12, 1
    rtl
NB_Scale:
    phx
    asl a
    asl a
    tax                        ; sector byte -> exact signed-32-bit units
    lda.l NB_Units,x
    sta.b tcc__r0
    lda.l NB_Units+2,x
    sta.b tcc__r0h
    plx
    rts
; Immutable CPU FastROM lookup, never staged in WRAM. The high word matters
; above world coordinate 65535; maximum-bound subtraction still borrows.
NB_Units:
    .dw $0000, $0000
    .dw $0400, $0000
    .dw $0800, $0000
    .dw $0C00, $0000
    .dw $1000, $0000
    .dw $1400, $0000
    .dw $1800, $0000
    .dw $1C00, $0000
    .dw $2000, $0000
    .dw $2400, $0000
    .dw $2800, $0000
    .dw $2C00, $0000
    .dw $3000, $0000
    .dw $3400, $0000
    .dw $3800, $0000
    .dw $3C00, $0000
    .dw $4000, $0000
    .dw $4400, $0000
    .dw $4800, $0000
    .dw $4C00, $0000
    .dw $5000, $0000
    .dw $5400, $0000
    .dw $5800, $0000
    .dw $5C00, $0000
    .dw $6000, $0000
    .dw $6400, $0000
    .dw $6800, $0000
    .dw $6C00, $0000
    .dw $7000, $0000
    .dw $7400, $0000
    .dw $7800, $0000
    .dw $7C00, $0000
    .dw $8000, $0000
    .dw $8400, $0000
    .dw $8800, $0000
    .dw $8C00, $0000
    .dw $9000, $0000
    .dw $9400, $0000
    .dw $9800, $0000
    .dw $9C00, $0000
    .dw $A000, $0000
    .dw $A400, $0000
    .dw $A800, $0000
    .dw $AC00, $0000
    .dw $B000, $0000
    .dw $B400, $0000
    .dw $B800, $0000
    .dw $BC00, $0000
    .dw $C000, $0000
    .dw $C400, $0000
    .dw $C800, $0000
    .dw $CC00, $0000
    .dw $D000, $0000
    .dw $D400, $0000
    .dw $D800, $0000
    .dw $DC00, $0000
    .dw $E000, $0000
    .dw $E400, $0000
    .dw $E800, $0000
    .dw $EC00, $0000
    .dw $F000, $0000
    .dw $F400, $0000
    .dw $F800, $0000
    .dw $FC00, $0000
    .dw $0000, $0001
    .dw $0400, $0001
    .dw $0800, $0001
    .dw $0C00, $0001
    .dw $1000, $0001
    .dw $1400, $0001
    .dw $1800, $0001
    .dw $1C00, $0001
    .dw $2000, $0001
    .dw $2400, $0001
    .dw $2800, $0001
    .dw $2C00, $0001
    .dw $3000, $0001
    .dw $3400, $0001
    .dw $3800, $0001
    .dw $3C00, $0001
    .dw $4000, $0001
    .dw $4400, $0001
    .dw $4800, $0001
    .dw $4C00, $0001
    .dw $5000, $0001
    .dw $5400, $0001
    .dw $5800, $0001
    .dw $5C00, $0001
    .dw $6000, $0001
    .dw $6400, $0001
    .dw $6800, $0001
    .dw $6C00, $0001
    .dw $7000, $0001
    .dw $7400, $0001
    .dw $7800, $0001
    .dw $7C00, $0001
    .dw $8000, $0001
    .dw $8400, $0001
    .dw $8800, $0001
    .dw $8C00, $0001
    .dw $9000, $0001
    .dw $9400, $0001
    .dw $9800, $0001
    .dw $9C00, $0001
    .dw $A000, $0001
    .dw $A400, $0001
    .dw $A800, $0001
    .dw $AC00, $0001
    .dw $B000, $0001
    .dw $B400, $0001
    .dw $B800, $0001
    .dw $BC00, $0001
    .dw $C000, $0001
    .dw $C400, $0001
    .dw $C800, $0001
    .dw $CC00, $0001
    .dw $D000, $0001
    .dw $D400, $0001
    .dw $D800, $0001
    .dw $DC00, $0001
    .dw $E000, $0001
    .dw $E400, $0001
    .dw $E800, $0001
    .dw $EC00, $0001
    .dw $F000, $0001
    .dw $F400, $0001
    .dw $F800, $0001
    .dw $FC00, $0001
    .dw $0000, $0002
    .dw $0400, $0002
    .dw $0800, $0002
    .dw $0C00, $0002
    .dw $1000, $0002
    .dw $1400, $0002
    .dw $1800, $0002
    .dw $1C00, $0002
    .dw $2000, $0002
    .dw $2400, $0002
    .dw $2800, $0002
    .dw $2C00, $0002
    .dw $3000, $0002
    .dw $3400, $0002
    .dw $3800, $0002
    .dw $3C00, $0002
    .dw $4000, $0002
    .dw $4400, $0002
    .dw $4800, $0002
    .dw $4C00, $0002
    .dw $5000, $0002
    .dw $5400, $0002
    .dw $5800, $0002
    .dw $5C00, $0002
    .dw $6000, $0002
    .dw $6400, $0002
    .dw $6800, $0002
    .dw $6C00, $0002
    .dw $7000, $0002
    .dw $7400, $0002
    .dw $7800, $0002
    .dw $7C00, $0002
    .dw $8000, $0002
    .dw $8400, $0002
    .dw $8800, $0002
    .dw $8C00, $0002
    .dw $9000, $0002
    .dw $9400, $0002
    .dw $9800, $0002
    .dw $9C00, $0002
    .dw $A000, $0002
    .dw $A400, $0002
    .dw $A800, $0002
    .dw $AC00, $0002
    .dw $B000, $0002
    .dw $B400, $0002
    .dw $B800, $0002
    .dw $BC00, $0002
    .dw $C000, $0002
    .dw $C400, $0002
    .dw $C800, $0002
    .dw $CC00, $0002
    .dw $D000, $0002
    .dw $D400, $0002
    .dw $D800, $0002
    .dw $DC00, $0002
    .dw $E000, $0002
    .dw $E400, $0002
    .dw $E800, $0002
    .dw $EC00, $0002
    .dw $F000, $0002
    .dw $F400, $0002
    .dw $F800, $0002
    .dw $FC00, $0002
    .dw $0000, $0003
    .dw $0400, $0003
    .dw $0800, $0003
    .dw $0C00, $0003
    .dw $1000, $0003
    .dw $1400, $0003
    .dw $1800, $0003
    .dw $1C00, $0003
    .dw $2000, $0003
    .dw $2400, $0003
    .dw $2800, $0003
    .dw $2C00, $0003
    .dw $3000, $0003
    .dw $3400, $0003
    .dw $3800, $0003
    .dw $3C00, $0003
    .dw $4000, $0003
    .dw $4400, $0003
    .dw $4800, $0003
    .dw $4C00, $0003
    .dw $5000, $0003
    .dw $5400, $0003
    .dw $5800, $0003
    .dw $5C00, $0003
    .dw $6000, $0003
    .dw $6400, $0003
    .dw $6800, $0003
    .dw $6C00, $0003
    .dw $7000, $0003
    .dw $7400, $0003
    .dw $7800, $0003
    .dw $7C00, $0003
    .dw $8000, $0003
    .dw $8400, $0003
    .dw $8800, $0003
    .dw $8C00, $0003
    .dw $9000, $0003
    .dw $9400, $0003
    .dw $9800, $0003
    .dw $9C00, $0003
    .dw $A000, $0003
    .dw $A400, $0003
    .dw $A800, $0003
    .dw $AC00, $0003
    .dw $B000, $0003
    .dw $B400, $0003
    .dw $B800, $0003
    .dw $BC00, $0003
    .dw $C000, $0003
    .dw $C400, $0003
    .dw $C800, $0003
    .dw $CC00, $0003
    .dw $D000, $0003
    .dw $D400, $0003
    .dw $D800, $0003
    .dw $DC00, $0003
    .dw $E000, $0003
    .dw $E400, $0003
    .dw $E800, $0003
    .dw $EC00, $0003
    .dw $F000, $0003
    .dw $F400, $0003
    .dw $F800, $0003
    .dw $FC00, $0003
.ENDS
.ENDIF
.ENDIF

; ItemObj_animChange/animProcess. item.c remains the executable C reference.
; 816-tcc ABI: native 16-bit A/X/Y, D=0, DB=$7E, far item pointer at 4,s.
; FX3 per-frame effects are native; end commands retain their C body/ABI.

.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF C_MOTION
.IFDEF FX3
.BASE $80
.BANK 4 SLOT 0
.ELSE
.BASE $7B
.BANK 4 SLOT 4
.DEFINE ANIM_BLOB_BANK $71
.ENDIF
.SECTION "item_anim_native" SEMIFREE
.accu 16
.index 16
.16bit

; Called directly from an exported entry point: skip this JSR's two-byte
; return as well as the caller's JSL return to find the original argument.
Anim_LoadItem:
    lda 6,s
    sta.b tcc__r0
    lda 8,s
    sta.b tcc__r0h
    rts

; A = animation index; r1 = its 14-byte host record. Preserves r0.
Anim_Record:
    asl a
    sta.b tcc__r2
    asl a
    asl a
    asl a
    sec
    sbc.b tcc__r2
    clc
    adc.w olAnimBase
    sta.b tcc__r1
.IFDEF FX3
    lda.b olAnimData+2
.ELSE
    lda #ANIM_BLOB_BANK
.ENDIF
    sta.b tcc__r1h
    rts

ItemObj_animChange:
    jsr Anim_LoadItem
    jsr Anim_Change
    rtl

Anim_Change:
    ldy #2
    lda [tcc__r0],y            ; state, goal
    xba
    eor [tcc__r0],y
    and #$00FF
    bne +
    rts
+   ldy #12
    lda [tcc__r0],y
    jsr Anim_Record
    ldy #8
    lda [tcc__r1],y
    and #$00FF
    bne +
    rts
+   sta.b tcc__r2             ; number of flattened transitions
    ldy #6
    lda [tcc__r1],y
    asl a
    sta.b tcc__r3
    asl a
    clc
    adc.b tcc__r3              ; transition index * 6
    clc
    adc.w olTransBase
    sta.b tcc__r1
.IFDEF FX3
    lda.b olTransData+2
    sta.b tcc__r1h
.ENDIF
    ldy #14
    lda [tcc__r0],y
    and #$00FF
    sta.b tcc__r3              ; local frame
    ldy #3
    lda [tcc__r0],y
    and #$00FF
    sta.b tcc__r3h             ; requested goal
Anim_Transition:
    lda [tcc__r1]
    and #$00FF
    cmp.b tcc__r3h
    bne Anim_NextTransition
    ldy #1
    lda [tcc__r1],y
    and #$00FF
    cmp.b tcc__r3
    bcc +
    bne Anim_NextTransition
+   ldy #2
    lda [tcc__r1],y
    and #$00FF
    cmp.b tcc__r3
    bcc Anim_NextTransition

    ldy #3
    lda [tcc__r1],y
    ldy #14
    sep #$20
    sta [tcc__r0],y
    rep #$20
    ldy #4
    lda [tcc__r1],y
    ldy #12
    sta [tcc__r0],y
    jsr Anim_Record
    ldy #9
    lda [tcc__r1],y
    ldy #2
    sep #$20
    sta [tcc__r0],y
    rep #$20
    rts

Anim_NextTransition:
    lda.b tcc__r1
    clc
    adc #6
    sta.b tcc__r1
    dec.b tcc__r2
    bne Anim_Transition
    rts

; Upstream clears a fulfilled pending state immediately after animChange
; and again after an end-of-animation link. Never clear Lara's slot.
Anim_ClearNext:
    lda [tcc__r0]
    and #$00FF
    beq +
    lda.b tcc__r0
    sec
    sbc #(items & $FFFF)
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    tax
    lda.l olEnemyNext,x
    ldy #2
    eor [tcc__r0],y
    and #$00FF
    bne +
    sep #$20
    lda #0
    sta.l olEnemyNext,x
    rep #$20
+   rts

; A = fx. The command handler may clobber all tcc scratch registers.
Anim_Command:
    pha
    lda.b tcc__r0h
    pha
    pei (tcc__r0)
    jsr.l ItemObj_animCmd
    pla
    pla
    pla
    rts

.IFDEF FX3
; ItemObj_animCmd(item, fx): the per-frame loop is visited far more often
; than the animation end. Preserve its captured command list/count even
; when FX_LARA_NORMAL changes the item's animation and current frame.
; No persistent scratch: only that rare call needs six saved stack bytes.
; All three immutable tables use independent CPU FastROM pointers.
ItemObj_animCmd:
    lda 8,s
    bne AC_Frame
    jmp.l ItemObj_animCmdEnd     ; same arguments and JSL return, no new C frame
AC_Frame:
    jsr Anim_LoadItem
    ldy #12
    lda [tcc__r0],y
    jsr Anim_Record
    ldy #12
    lda [tcc__r1],y
    and #$FF
    bne +
    rtl
+   sta.b tcc__r2              ; original count is not reloaded after effects
    ldy #10
    lda [tcc__r1],y
    asl a
    asl a
    asl a
    clc
    adc.b olCmdData
    sta.b tcc__r1
    lda.b olCmdData+2
    sta.b tcc__r1h
AC_Loop:
    lda [tcc__r1]
    and #$FF
    cmp #5                     ; ANIM_CMD_SOUND
    bne +
    jmp AC_Sound
+   cmp #6                     ; ANIM_CMD_EFFECT
    bne AC_Next
    ldy #14
    lda [tcc__r0],y
    and #$FF
    sta.b tcc__r3
    ldy #2
    lda [tcc__r1],y
    cmp.b tcc__r3               ; full word: a frame of 256 must not match 0
    bne AC_Next
    ldy #4
    lda [tcc__r1],y
    beq AC_Rotate
    cmp #2                     ; FX_LARA_NORMAL
    beq AC_Lara
    cmp #3                     ; FX_LARA_BUBBLES
    beq AC_Bubbles
    cmp #12                    ; FX_LARA_HANDSFREE
    bne AC_Next
AC_Lara:
    sta.b tcc__r3
    lda.b tcc__r0
    cmp #(items & $FFFF)
    bne AC_Next
    lda.b tcc__r0h
    cmp #$7E                   ; pointer identity, not item type or physical alias
    bne AC_Next
    lda.b tcc__r3
    cmp #2
    beq AC_Normal
    stz.w olLaraWeaponState
    bra AC_Next
AC_Rotate:
    ldy #10
    lda [tcc__r0],y
    eor #$8000
    sta [tcc__r0],y
AC_Next:
    lda.b tcc__r1
    clc
    adc #8
    sta.b tcc__r1
    dec.b tcc__r2
    bne AC_Loop
    rtl
AC_Normal:
    ldy #22
    lda [tcc__r0],y
    and #$FF
    asl a
    asl a
    asl a
    asl a
    tay
    lda [olModelData],y
    clc
    adc #11                    ; LARA_ANIM_STAND, as in the C effect switch
    pei (tcc__r1h)
    pei (tcc__r1)
    pei (tcc__r2)
    pha
    pei (tcc__r0h)
    pei (tcc__r0)
    jsr.l ItemObj_animSet
    pla
    pla
    pla
    pla
    sta.b tcc__r2
    pla
    sta.b tcc__r1
    pla
    sta.b tcc__r1h
    jsr Anim_LoadItem
    bra AC_Next
AC_Bubbles:
    ; fxBubbles(room, JOINT_HEAD, (0, 0, 50)), in fx.c
    pei (tcc__r1h)
    pei (tcc__r1)
    pei (tcc__r2)
    pei (tcc__r0h)
    pei (tcc__r0)
    jsr.l Fx_bubbles
    pla
    pla
    pla
    sta.b tcc__r2
    pla
    sta.b tcc__r1
    pla
    sta.b tcc__r1h
    jsr Anim_LoadItem
    jmp AC_Next
AC_Sound:
    ; The frame's sound, at the item -- the C case in item.c, natively.
    ldy #14
    lda [tcc__r0],y
    and #$FF
    sta.b tcc__r3
    ldy #2
    lda [tcc__r1],y
    cmp.b tcc__r3               ; full word: a frame of 256 must not match 0
    bne AC_Next
    ldy #4
    lda [tcc__r1],y
    and #$3FFF                  ; the id, without TR's environment bits
    sta.b tcc__r3
    pei (tcc__r1h)
    pei (tcc__r1)
    pei (tcc__r2)
    pei (tcc__r0h)
    pei (tcc__r0)
    pei (tcc__r3)
    jsr.l Sound_playItem
    pla
    pla
    pla
    pla
    sta.b tcc__r2
    pla
    sta.b tcc__r1
    pla
    sta.b tcc__r1h
    jsr Anim_LoadItem
    jmp AC_Next
.ENDIF

ItemObj_animProcess:
    jsr Anim_LoadItem
    ldy #14
    lda [tcc__r0],y
    inc a
    sep #$20
    sta [tcc__r0],y
    rep #$20
    jsr Anim_Change
    jsr Anim_ClearNext
    ldy #12
    lda [tcc__r0],y
    jsr Anim_Record
    ldy #14
    lda [tcc__r0],y
    and #$00FF
    sta.b tcc__r2
    lda [tcc__r1]
    and #$00FF
    cmp.b tcc__r2
    beq Anim_End
    bcs Anim_FrameCommand
Anim_End:
    lda #0
    jsr Anim_Command
    jsr Anim_LoadItem
    ldy #12
    lda [tcc__r0],y
    jsr Anim_Record
    ldy #4
    lda [tcc__r1],y
    ldy #14
    sep #$20
    sta [tcc__r0],y
    rep #$20
    ldy #2
    lda [tcc__r1],y
    ldy #12
    sta [tcc__r0],y
    jsr Anim_Record
    ldy #9
    lda [tcc__r1],y
    ldy #2
    sep #$20
    sta [tcc__r0],y
    rep #$20
    sta.b tcc__r2              ; low byte is the new state
    lda [tcc__r0]
    and #$00FF
    beq +
    lda.b tcc__r2
    ldy #3
    sep #$20
    sta [tcc__r0],y
    rep #$20
    jsr Anim_ClearNext
+   ldy #14
    lda [tcc__r0],y
    and #$00FF
    sta.b tcc__r2
    lda [tcc__r1]
    and #$00FF
    cmp.b tcc__r2
    beq Anim_ResetFrame
    bcs Anim_FrameCommand
Anim_ResetFrame:
    lda #0
    ldy #14
    sep #$20
    sta [tcc__r0],y
    rep #$20
Anim_FrameCommand:
    ldy #12
    lda [tcc__r1],y
    and #$00FF
    beq +
    lda #1
    jsr Anim_Command
+   rtl

.ENDS
.ENDIF

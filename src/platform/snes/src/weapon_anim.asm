; Native counterpart of weapon_anim.c, using the same 12-byte WeaponArm.
; 816-tcc ABI: word input at 4,s, DB=$7E, D=0, 16-bit A/X/Y.
; Arm index X survives local helpers; C/runtime calls save it on the stack.
.INCLUDE "hdr.asm"
.INCLUDE "texbank.inc"
.INCLUDE "config.inc"
.IFDEF FX3
.BASE $80
.BANK 7 SLOT 0                  ; $87 (was $BD)
.ELSE
.BASE $41
.BANK 61 SLOT 8
.ENDIF
.SECTION "weapon_animation_native" SEMIFREE
.accu 16
.index 16
.16bit

.DEFINE WA_ANIM  olWeaponArms
.DEFINE WA_FRAME olWeaponArms+2
.DEFINE WA_X     olWeaponArms+4
.DEFINE WA_Y     olWeaponArms+6
.DEFINE WA_BASIS olWeaponArms+8
.DEFINE WA_DRAWN olWeaponArms+10

Weapon_resetArms:
    ldx #12
-   lda #$FFFF
    sta.w WA_ANIM,x
    stz.w WA_FRAME,x
    stz.w WA_X,x
    stz.w WA_Y,x
    stz.w WA_BASIS,x
    txa
    beq +
    ldx #0
    bra -
+   rtl

Weapon_animate:
    lda 4,s
    pha                         ; input, retained across both arms and calls
    ldx #0
WA_Loop:
    lda.w olLaraWeaponState
    cmp #2                      ; DRAW
    bne +
    jmp WA_Draw
+   cmp #3                      ; HOLSTER
    bne +
    jmp WA_Holster
+   cmp #4                      ; READY
    beq WA_Ready
    pla
    jml Weapon_resetArms

WA_Ready:
    lda #1
    cpx #0
    beq +
    asl a
+   and.w olWeaponAim
    sta.b tcc__r4               ; this arm's aim bit
    beq +
    lda.w olWeaponAimX
+   ldy.w WA_X,x
    jsr WA_Lerp
    sta.w WA_X,x
    lda.b tcc__r4
    beq +
    lda.w olWeaponAimY
+   ldy.w WA_Y,x
    jsr WA_Lerp
    sta.w WA_Y,x

    lda.b tcc__r4
    bne WA_Raise
    lda 1,s
    and #$0020                  ; IN_ACTION = IK_B
    beq WA_Lower
    lda.w olWeaponTarget
    cmp #$00FF
    bne WA_Lower
WA_Raise:
    lda.w WA_ANIM,x
    bne WA_Reload               ; FIRE
    lda.w WA_FRAME,x
    cmp #PISTOL_ANIM_0_LAST
    beq +
    inc.w WA_FRAME,x
    bra WA_Basis
+   lda 1,s
    and #$0020
    beq WA_Basis
    phx
.IFDEF FX3
    stx.w olWeaponFireArm       ; which arm's aim the shot leaves along
.ENDIF
    lda.b tcc__r4
    pha
    jsl Weapon_fire
    pla
    plx
    lda #3
    sta.w WA_ANIM,x
    stz.w WA_FRAME,x
    bra WA_Basis
WA_Reload:
    inc.w WA_FRAME,x
    lda.w WA_FRAME,x
.IFDEF FX3
    cmp.w olWeaponReload        ; weaponParams reloadTimer: 9, the uzis' 3
.ELSE
    cmp #9                      ; weaponParams reloadTimer
.ENDIF
    bne WA_Basis
    bra WA_AimLast
WA_Lower:
    lda.w WA_ANIM,x
    cmp #3
    beq WA_AimLast
    lda.w WA_FRAME,x
    beq WA_Basis
    dec.w WA_FRAME,x
    bra WA_Basis
WA_AimLast:
    stz.w WA_ANIM,x
    lda #PISTOL_ANIM_0_LAST
    sta.w WA_FRAME,x
WA_Basis:
    lda.w WA_FRAME,x
    ora.w WA_ANIM,x             ; READY only has AIM and FIRE
    beq +
    lda #1
+   sta.w WA_BASIS,x
    jmp WA_Next

WA_Draw:
    inc.w WA_FRAME,x
    lda #PISTOL_ANIM_1_LAST
    ldy.w WA_ANIM,x
    cpy #1
    beq +
    lda #PISTOL_ANIM_2_LAST
+   cmp.w WA_FRAME,x
    bcs WA_DrawAngles
    stz.w WA_FRAME,x
    cpy #1
    bne +
    phx                         ; SND_DRAW as the pistols leave the thighs
    pea $007E
    pea items
    pea 6
    jsl Sound_playItem
    pla
    pla
    pla
    plx
    lda #1
    sta.w WA_DRAWN,x
.IFDEF FX3
    sta.w olWeaponKitDirty      ; her thigh and hand meshes change
.ENDIF
    inc a
    bra ++
+   lda #0
++  sta.w WA_ANIM,x
WA_DrawAngles:
    stz.w WA_X,x
    stz.w WA_Y,x
    stz.w WA_BASIS,x
    jmp WA_Next

WA_Holster:
    lda.w WA_ANIM,x
    cmp #$FFFF
    bne +
    jmp WA_Next
+   ldy.w WA_FRAME,x
    beq WA_HolsterChange
    cmp #0                      ; AIM: decay the angles over its last frames
    bne WA_HolsterFrame
    sty.b tcc__r4h
    phx
    lda.w WA_X,x
    tax
    lda.b tcc__r4h
    jsl tcc__div
    plx
    lda.w WA_X,x
    sec
    sbc.b tcc__r9
    sta.w WA_X,x
    phx
    lda.w WA_Y,x
    tax
    lda.b tcc__r4h
    jsl tcc__div
    plx
    lda.w WA_Y,x
    sec
    sbc.b tcc__r9
    sta.w WA_Y,x
WA_HolsterFrame:
    lda.w WA_ANIM,x
    cmp #3
    beq +
    dec.w WA_FRAME,x
    bra WA_Next
+   stz.w WA_FRAME,x
    bra WA_Next
WA_HolsterChange:
    cmp #0                      ; AIM -> DRAW
    bne +
    lda #2
    ldy #PISTOL_ANIM_2_LAST
    bra WA_HolsterStore
+   cmp #2                      ; DRAW -> PICK, gun goes onto the thigh
    bne +
    stz.w WA_DRAWN,x
.IFDEF FX3
    lda.w olWeaponKit           ; weaponHolsterPistols' meshSwapPistols: the
    sta.w olWeaponKit+4         ; thighs wear this gun's holsters from now
    lda #1
    sta.w olWeaponKitDirty
.ENDIF
    phx                         ; SND_HOLSTER
    pea $007E
    pea items
    pea 7
    jsl Sound_playItem
    pla
    pla
    pla
    plx
    lda #1
    ldy #PISTOL_ANIM_1_LAST
    bra WA_HolsterStore
+   cmp #3                      ; FIRE -> AIM
    bne +
    lda #0
    ldy #PISTOL_ANIM_0_LAST
    bra WA_HolsterStore
+   lda #$FFFF                  ; PICK -> finished
WA_HolsterStore:
    sta.w WA_ANIM,x
    tya
    sta.w WA_FRAME,x
WA_Next:
    cpx #0
    bne +
    ldx #12
    jmp WA_Loop
+   lda.w olLaraWeaponState
    cmp #2
    bne +
    lda.w WA_ANIM+12
    bne WA_Done
    lda #4
    sta.w olLaraWeaponState
    bra WA_Done
+   cmp #3
    bne WA_Done
    lda.w WA_ANIM
    and.w WA_ANIM+12
    cmp #$FFFF
    bne WA_Done
    stz.w olLaraWeaponState
    jsr.l Weapon_resetArms
WA_Done:
    pla
    rtl

; angleLerp: A is target, Y is current; wrap the signed angular delta and
; clamp it to ten degrees per game tick. X and r4 are preserved.
WA_Lerp:
    sty.b tcc__r3
    sec
    sbc.b tcc__r3
    bmi +
    cmp #1820
    bcc ++
    lda #1820
    bra ++
+   cmp #(-1820 & $FFFF)
    bcs ++
    lda #(-1820 & $FFFF)
++  clc
    adc.b tcc__r3
    rts

; Lara::updateLook, pistol/return-to-centre paths. It runs before movement
; and Weapon_update, using the preceding tick's arms. All fields are words;
; ItemObj.health is at byte 16. Keep the C reference in weapon_anim.c.
Lara_resetLook:
    stz.w olLaraLook
    stz.w olLaraLook+2
    stz.w olLaraLook+4
    stz.w olLaraLook+6
    rtl

Lara_updateLook:
    lda.w items+16
    beq Lara_resetLook
    bmi Lara_resetLook
.IFDEF FX3
    ; With no target, an item the camera looked at last tick (upstream's
    ; camera.lastItem) holds her head where lookAt turned it: no decay.
    lda.w olWeaponTarget
    cmp #$00FF
    bne +
    lda.w olCamLastItem
    cmp #$00FF
    beq +
    rtl
+
.ENDIF
    ldx #2                     ; yaw, then pitch
LK_Axis:
    lda.w olWeaponTarget
    cmp #$00FF
    beq LK_Decay
.IFDEF FX3
    lda.w olWeaponKit           ; the shotgun: the torso on the right arm's
    cmp #3                      ; aim, the head still (updateLook)
    bne +
    stz.w olLaraLook+4,x
    lda.w olWeaponArms+4,x
    sta.w olLaraLook,x
    bra LK_Next
+
.ENDIF
    ; Bias both signed words, average the full 17-bit sum, then unbias.
    ; This is floor((R+L)/2), even when their sum overflows a signed word.
    lda.w olWeaponArms+4,x
    eor #$8000
    sta.b tcc__r3
    lda.w olWeaponArms+16,x
    eor #$8000
    clc
    adc.b tcc__r3
    ror a
    eor #$8000
    ldy.w olWeaponAim
    cpy #3
    bne LK_Store
    cmp #$8000                 ; both arms aim: another arithmetic shift
    ror a                      ; floor((R+L)/4)
    bra LK_Store
LK_Decay:
    lda.w olLaraLook+4,x
    sta.b tcc__r3
    bpl +
    eor #$FFFF
    inc a
+   lsr a
    lsr a
    lsr a
    sta.b tcc__r4               ; abs(head angle) >> 3
    lda.b tcc__r3
    cmp #$8001                 ; short arc: -32768 turns the negative way
    bcs +
    sec
    sbc.b tcc__r4
    bra LK_Store
+   clc
    adc.b tcc__r4
LK_Store:
    sta.w olLaraLook,x
    sta.w olLaraLook+4,x
LK_Next:
    dex
    dex
    bpl LK_Axis
    rtl
.ENDS

; OpenLara - SNES / Super FX target
; Sound effects, natively: sound.c is the executable C reference, and the
; cartridge runs this. The compiled unit came out at 2,344 bytes -- three
; long multiplies, a long square root call and the port protocol through far
; pointers -- against 765 bytes free in the GSU-2's resident image; this is
; the same algorithm on the CPU's own 8x8 multiplier.
;
; 816-tcc ABI: native 16-bit A/X/Y, D=0, DB=$7E; arguments at 4,s upward
; after the JSL; an int returns in tcc__r0 (and A). Scratch is the .bss
; words sound.c declares (olSndWork), not the tcc registers: item_anim.asm
; and weapon_anim.asm call in with their own state in those.
;
; One deliberate difference from sound.c: rand_draw is a 16-bit xorshift,
; not upstream's 32-bit LCG. Nothing observable depends on the draw PRNG's
; sequence -- it decides a random pitch, gain and footstep variant -- and the
; LCG's 32-bit multiply would cost a third of this unit's budget.

.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF C_MOTION
.IFDEF FX3
.BASE $80
.BANK 8 SLOT 0                  ; $88 (was $BE)
.ELSE
.BASE $3F
.BANK 63 SLOT 6
.ENDIF
.SECTION "sound_native" SEMIFREE
.accu 16
.index 16
.16bit

.DEFINE APU_PORT0   $002140
.DEFINE APU_PORT1   $002141
.DEFINE APU_PORT2   $002142
.DEFINE APU_PORT3   $002143
.DEFINE MPY_A       $004202
.DEFINE MPY_B       $004203
.DEFINE MPY_RESULT  $004216

.DEFINE SND_MAX_DIST 8192

; the .bss words olSndWork[] in sound.c
.DEFINE snd_rec     olSndWork          ; the info record's address
.DEFINE snd_vol     olSndWork+2        ; upstream's 16-bit volume
.DEFINE snd_source  olSndWork+4        ; the command: source
.DEFINE snd_volume  olSndWork+6        ;              VOL
.DEFINE snd_pitch   olSndWork+8        ;              multiplier
.DEFINE snd_mode    olSndWork+10       ;              mode
.DEFINE snd_dot     olSndWork+12       ; dx*dx + dy*dy + dz*dz, 32 bits
.DEFINE snd_abs     olSndWork+16
.DEFINE snd_tmp     olSndWork+18
.DEFINE snd_flags   olSndWork+20
.DEFINE snd_count   olSndWork+22

; --- rand_draw -------------------------------------------------------------
Snd_Rand:
    lda.w olDrawSeed
    bne +
    lda #$ACE1                  ; a zero seed would stay zero
+   sta.w snd_tmp
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    eor.w snd_tmp               ; x ^= x << 7
    sta.w snd_tmp
    xba
    and #$00FF
    lsr a
    eor.w snd_tmp               ; x ^= x >> 9
    sta.w snd_tmp
    xba
    and #$FF00
    eor.w snd_tmp               ; x ^= x << 8
    sta.w olDrawSeed
    and #$7FFF
    rts

.IFDEF FX3
; int32 rand_draw(void) for C: the effects' draws (src/fx.c), on the same
; generator as the sound's, as upstream shares one.
rand_draw:
    jsr Snd_Rand
    sta.b tcc__r0
    stz.b tcc__r1
    rtl
.ENDIF

; --- the command -------------------------------------------------------------
; snd_source/volume/pitch/mode -> the four ports. Waits, briefly, for the
; driver's echo of the previous command; advances the sequence so this one
; reads differently; zero is never sent.
Snd_Send:
    ldx #250
    sep #$20
.accu 8
-   lda.l APU_PORT0
    cmp.w olSndSeq
    beq +
    dex
    bne -
+   lda.w snd_source
    sta.l APU_PORT1
    lda.w snd_volume
    sta.l APU_PORT2
    lda.w snd_pitch
    sta.l APU_PORT3
    rep #$20
.accu 16
    lda.w olSndSeq
    clc
    adc #4
    and #$00FC
    bne +
    lda #4
+   ora.w snd_mode
    sta.w olSndSeq
    sep #$20
.accu 8
    sta.l APU_PORT0
    rep #$20
.accu 16
    inc.w ol_sndCount
    rts

; --- the square, on the multiplier ---------------------------------------------
; A = |d| < 8192; snd_dot += d * d. Three 8x8 products: lo*lo, hi*hi << 16
; and 2 * hi*lo << 8 (hi is at most 31, so the doubled cross term keeps to
; sixteen bits). A sixteen-bit store to $4202 writes both factors, and the
; next factor is prepared during the eight cycles the multiplier wants;
; NMI does not touch $4202-$4217.
Snd_Square:
    sta.w snd_abs
    and #$00FF
    sta.w snd_tmp
    xba
    ora.w snd_tmp
    sta.l MPY_A                 ; lo * lo
    lda.w snd_abs
    xba
    and #$00FF
    sta.w snd_tmp
    xba
    ora.w snd_tmp
    tax                         ; hi * hi, ready
    lda.l MPY_RESULT
    clc
    adc.w snd_dot
    sta.w snd_dot
    lda.w snd_dot+2
    adc #0
    sta.w snd_dot+2
    txa
    sta.l MPY_A                 ; hi * hi
    ldx.w snd_abs               ; lo * hi is the word itself
    nop
    nop
    nop
    lda.l MPY_RESULT
    clc
    adc.w snd_dot+2
    sta.w snd_dot+2
    txa
    sta.l MPY_A                 ; lo * hi
    nop
    nop
    nop
    nop
    lda.l MPY_RESULT
    asl a                       ; * 2
    sta.w snd_tmp
    and #$00FF
    xba                         ; low byte << 8
    clc
    adc.w snd_dot
    sta.w snd_dot
    lda.w snd_tmp
    xba
    and #$00FF                  ; high byte, into the high word
    adc.w snd_dot+2
    sta.w snd_dot+2
    rts

; A = a signed delta. Aborts the play when |d| >= SND_MAX_DIST, as upstream
; does per axis; otherwise adds its square to snd_dot.
Snd_Axis:
    bpl +
    eor #$FFFF
    inc a
+   cmp #SND_MAX_DIST
    bcs Snd_AxisFar
    jmp Snd_Square
Snd_AxisFar:
    pla                         ; the JSR's return: the caller aborts
    jmp Sound_playNo

; --- the id's info -----------------------------------------------------------
; The caller's id is at 4,s, so 6,s here. X = the record; carry set: none,
; or no driver.
Snd_Lookup:
    lda.w ol_sndReady
    beq Snd_LookupNone
    lda 6,s
    cmp #256
    bcs Snd_LookupNone
    tax
    lda.w olSndTable,x
    and #$00FF
    cmp #$00FF
    beq Snd_LookupNone
    asl a
    asl a                       ; carry clear: at most 95 * 4
    adc #(olSndTable + 256)
    tax
    clc
    rts
Snd_LookupNone:
    sec
    rts

; --- Sound_play(id, x, y, z) ---------------------------------------------------
; 4,s id  6,s x  8,s y  10,s z. Returns 1 when a command went to the driver.
Sound_play:
.IFDEF FX3
    lda.l olSettings+2          ; settings.c's audio_sfx: off, nothing is sent
    and #$00FF
    beq Sound_playNo
.ENDIF
    jsr Snd_Lookup
    bcs Sound_playNo
    stx.w snd_rec

    ; the chance roll: chance >> 7 against rand >> 7
    lda.w 2,x
    and #$00FF
    beq +
    sta.w snd_tmp
    jsr Snd_Rand
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a                       ; >> 7: 0..255
    cmp.w snd_tmp
    beq +
    bcs Sound_playNo            ; chance < roll
+   bra SP_Volume
Sound_playNo:
    lda #0
    sta.b tcc__r0
    rtl
SP_Volume:
    lda.w 1,x                   ; X is still the record: Snd_Rand keeps it
    and #$00FF
    xba
    lsr a                       ; volume << 7
    sta.w snd_vol
    lda.w 3,x
    and #$00FF
    sta.w snd_flags

    ; the distance from the camera target
    stz.w snd_dot
    stz.w snd_dot+2
    lda 6,s
    sec
    sbc.w camTarget
    jsr Snd_Axis
    lda 8,s
    sec
    sbc.w camTarget+2
    jsr Snd_Axis
    lda 10,s
    sec
    sbc.w camTarget+4
    jsr Snd_Axis
    lda.w snd_dot+2
    pha
    lda.w snd_dot
    pha
    jsl phd_sqrt
    pla
    pla
    lda.b tcc__r0
    asl a
    asl a                       ; sqrt << 2: at most 56752
    cmp.w snd_vol
    bcs SP_No2                  ; out of earshot
    sta.w snd_tmp
    lda.w snd_vol
    sec
    sbc.w snd_tmp
    sta.w snd_vol

    ; random gain: volume -= rand >> 2
    lda.w snd_flags
    and #$0080
    beq +
    jsr Snd_Rand
    lsr a
    lsr a
    cmp.w snd_vol
    bcs SP_No2
    sta.w snd_tmp
    lda.w snd_vol
    sec
    sbc.w snd_tmp
    sta.w snd_vol
+
    ; >> 9, + 1, doubled for the driver's signed VOL
    lda.w snd_vol
    xba
    and #$00FF
    lsr a
    bne +
SP_No2:
    jmp Sound_playNo
+   inc a
    asl a
    cmp #128
    bcc +
    lda #127
+   sta.w snd_volume

    ; random pitch: 128 + ((rand * 13) >> 14) - 13, in sixteen bits
    lda #128
    sta.w snd_pitch
    lda.w snd_flags
    and #$0040
    beq +
    jsr Snd_Rand
    lsr a
    lsr a
    lsr a                       ; 0..4095
    sta.w snd_tmp
    asl a
    asl a
    sta.w snd_count             ; * 4
    asl a                       ; * 8
    clc
    adc.w snd_count
    clc
    adc.w snd_tmp               ; * 13, at most 53235
    xba
    and #$00FF
    lsr a
    lsr a
    lsr a                       ; >> 11: 0..25
    clc
    adc #(128 - 13)
    sta.w snd_pitch
+
    ; the variant: source + (rand * count) >> 15
    ldx.w snd_rec
    lda.w 0,x
    and #$00FF
    sta.w snd_source
    lda.w snd_flags
    lsr a
    lsr a
    and #$000F
    cmp #2
    bcc +
    sta.w snd_count
    jsr Snd_Rand
    lsr a
    lsr a
    lsr a                       ; 0..4095
    sta.w snd_tmp
    lda #0
    ldx.w snd_count
-   clc
    adc.w snd_tmp
    dex
    bne -                       ; * count, at most 61425
    xba
    and #$00FF
    lsr a
    lsr a
    lsr a
    lsr a                       ; >> 12: 0..count-1
    clc
    adc.w snd_source
    sta.w snd_source
+
    ; UNIQUE and REPLAY as they are; LOOP and the fourth take a new voice
    lda.w snd_flags
    and #$0003
    cmp #3
    bcc +
    lda #2
+   sta.w snd_mode

    lda 4,s
    xba
    and #$FF00
    ora.w snd_source
    sta.w ol_sndLast
    jsr Snd_Send
    lda #1
    sta.b tcc__r0
    rtl

; --- Sound_playItem(id, item) ----------------------------------------------------
; 4,s id  6,s item (far). The item's x, y, z, then Sound_play.
Sound_playItem:
    lda 6,s
    sta.b tcc__r0
    lda 8,s
    sta.b tcc__r0h
    ldy #8
    lda [tcc__r0],y             ; z
    pha
    ldy #6
    lda [tcc__r0],y             ; y
    pha
    ldy #4
    lda [tcc__r0],y             ; x
    pha
    lda 10,s                    ; id, past the three pushes
    pha
    jsl Sound_play
    pla
    pla
    pla
    pla
    rtl

; --- Sound_stop(id) ---------------------------------------------------------------
; Every variant of the sound, keyed off.
Sound_stop:
    jsr Snd_Lookup
    bcs Sound_stopDone
    lda.w 0,x
    and #$00FF
    sta.w snd_source
    lda.w 3,x
    lsr a
    lsr a
    and #$000F
    bne +
    inc a
+   sta.w snd_count
    stz.w snd_volume
    stz.w snd_pitch
    lda #3
    sta.w snd_mode
-   jsr Snd_Send
    inc.w snd_source
    dec.w snd_count
    bne -
Sound_stopDone:
    rtl

.ENDS
.ENDIF

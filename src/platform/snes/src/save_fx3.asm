; OpenLara - SNES / Super FX target
; The saved game's two block moves and its checksum (FX3; save.c is the rest).
;
; The save lives in battery-backed game pak RAM at $70:1000 (save.c says why
; there); everything it holds is C .bss in WRAM bank $7E. So a field is one
; MVN either way, 7 cycles a byte, rather than 816-tcc's byte loop through
; two far pointers.
;
; void olSramCopy(unsigned int sram, void* wram, unsigned int len,
;                 unsigned int toSram)
;   len bytes between $70:sram and $7E:wram (the pointer's low word; .bss is
;   all in $7E), into SRAM when toSram is not zero, out of it when it is.
;
; unsigned int olSramSum(unsigned int sram, unsigned int words)
;   Fletcher's sum over 16-bit words, both halves 16 bits: s1 += w, s2 += s1;
;   s1 ^ s2 rotated, so that a block of zeroes or of $FF is not a valid sum
;   of itself.
;
; 816-tcc ABI: A/X/Y16, D0, DB $7E; arguments at 4,s after the JSL; the
; result in tcc__r0. MVN leaves DB at its destination bank, so DB is kept
; around it. An NMI in the middle of a move is fine: MVN restarts with A, X
; and Y as the interrupt left them, and the NMI handler sets its own DB.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.INCLUDE "snes.inc"
.IFDEF FX3
.BASE $80
.BANK 9 SLOT 0                  ; $89 (was $BF)
.SECTION "save_native" SEMIFREE
.accu 16
.index 16
.16bit

olSramCopy:
    lda 10,s                    ; len
    beq @done
    dec a
    sta.b tcc__r0               ; MVN moves A + 1 bytes
    lda 12,s
    bne @toSram
    lda 6,s
    tay                         ; into WRAM
    lda 4,s
    tax                         ; from SRAM
    lda.b tcc__r0
    phb
    mvn $70, $7E
    plb
    rtl
@toSram:
    lda 4,s
    tay                         ; into SRAM
    lda 6,s
    tax                         ; from WRAM
    lda.b tcc__r0
    phb
    mvn $7E, $70
    plb
@done:
    rtl

olSramSum:
    stz.b tcc__r1               ; s1
    stz.b tcc__r1h              ; s2
    lda 6,s
    beq @out
    tay
    lda 4,s
    tax
@loop:
    lda.l $700000,x
    clc
    adc.b tcc__r1
    sta.b tcc__r1
    clc
    adc.b tcc__r1h
    sta.b tcc__r1h
    inx
    inx
    dey
    bne @loop
@out:
    lda.b tcc__r1h
    xba
    eor.b tcc__r1
    eor #$5A5A
    sta.b tcc__r0
    rtl
;
; The settings' two (settings.c), here in FastROM, not in the host's WRAM
; image, which has no room: main.asm's NMI and ItemsExchange call them.
;
; olCgramPiece -- the NMI's: the next 64 bytes of olCgram (palGamma's
; output) to CG-RAM, from the end, while olCgramLeft says there are some.
; A 8, X/Y 16, D 0, DB 0 (the I/O); returns A 8.
olCgramPiece:
    .accu 8
    lda.l olHudDirty
    bne @none
    rep #$20
    .accu 16
    lda.l olCgramLeft
    beq @none
    sec
    sbc #64
    sta.l olCgramLeft
    pha
    clc
    adc #(olCgram & $FFFF)
    sta A1T1L                   ; and A1T1H
    pla
    lsr a                       ; a byte offset, a colour index
    sep #$20
    .accu 8
    sta CGADD
    stz DMAP1                   ; A->B, increment, one register
    lda #$22                    ; CGDATA
    sta BBAD1
    lda #$7E
    sta A1B1
    ldx #64
    stx DAS1L
    lda #$02
    sta MDMAEN
    rtl
@none:
    sep #$20
    rtl

; olGammaApply -- settings.c's palGamma over CG-RAM's 256 colours, which
; 816-tcc took two fields over. Upstream's level per channel value c is
; min(31, c + (((c c >> 2) - c) (gamma << 4) >> 10)): here (d gamma) >> 6,
; d = (c c >> 2) - c on the CPU's multiplier, and d is -1 only for c 1..3,
; where the floor takes one off when there is any gamma -- as three word
; tables, each level already shifted into its channel. Then olCgram[i] =
; R[c & 31] | G[(c >> 5) & 31] | B[(c >> 10) & 31], c = olCgramBase[i].
; C ABI: A/X/Y 16, D 0, DB $7E (the tables and colours are .bss).
olGammaApply:
    .accu 16
    lda.l olSettings+4          ; video_gamma, 0..16
    and #$00FF
    sta.b tcc__r1
    xba
    sta.b tcc__r3               ; gamma << 8: WRMPYB's half
    ldx #$0000                  ; the level, times two
@level:
    txa
    lsr a
    sta.b tcc__r2               ; c
    xba
    ora.b tcc__r2
    sta.l WRMPYA                ; c x c (a 16-bit store writes both)
    nop
    nop
    nop
    nop                         ; the eight cycles the product takes
    lda.l RDMPYL
    lsr a
    lsr a
    sec
    sbc.b tcc__r2               ; d = (c c >> 2) - c
    bpl @up
    lda.b tcc__r2               ; d = -1: one less, if there is a gamma
    ldy.b tcc__r1
    beq @put
    dec a
    bra @put
@up:
    ora.b tcc__r3
    sta.l WRMPYA                ; d x gamma: d <= 209, gamma <= 16
    nop
    nop
    nop
    nop
    lda.l RDMPYL
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.b tcc__r2
    cmp #32
    bcc @put
    lda #31
@put:
    sta.w olGammaR,x
    asl a
    asl a
    asl a
    asl a
    asl a
    sta.w olGammaG,x
    asl a
    asl a
    asl a
    asl a
    asl a
    sta.w olGammaB,x
    inx
    inx
    cpx #64
    bne @level
    ldx #510
-   lda.w olCgramBase,x
    and #$001F
    asl a
    tay
    lda.w olGammaR,y
    sta.b tcc__r0
    lda.w olCgramBase,x
    lsr a
    lsr a
    lsr a
    lsr a
    and #$003E                  ; ((c >> 5) & 31) * 2
    tay
    lda.w olGammaG,y
    ora.b tcc__r0
    sta.b tcc__r0
    lda.w olCgramBase,x
    xba
    lsr a
    and #$003E                  ; ((c >> 10) & 31) * 2
    tay
    lda.w olGammaB,y
    ora.b tcc__r0
    sta.w olCgram,x
    dex
    dex
    bpl -
    rtl

; olTitlePin -- ItemsExchange's, with the ring's text: on a title boot the
; title page's picture is BG_BUF's for the whole boot (main.asm CopyTitle),
; never captured over (mesh.gsu IR_Grey). A 16.
olTitlePin:
    .accu 16
    lda.l olBootKind
    cmp #2                      ; save.h BOOT_TITLE
    bne +
    lda #$0001
    sta.l BG_STATE_LONG
+   rtl

; olGameText -- ItemsExchange's, for a frame without the ring: the game's
; own text in olInvText (settings.c Settings_text: drawFPS's counter, or
; none) to GS_TEXT when it has changed. A/X/Y 16, D 0; DB kept.
olGameText:
    .accu 16
    lda.l olGameTextDirty
    beq +
    lda #$0000
    sta.l olGameTextDirty
    lda.l olInvText
    asl a
    asl a
    asl a
    clc
    adc #7                      ; 8 + 8n bytes, less one for MVN
    ldx #(olInvText & $FFFF)
    ldy #(GS_TEXT_LONG & $FFFF)
    phb
    mvn $7E, $70
    plb
+   rtl
.ENDS
.ENDIF

; OpenLara - SNES / Super FX target
; The ring's sounds (FX3): upstream's SND_INV_SPIN .. SND_INV_PAGE in APU RAM
; while the ring is open.
;
; The level's APU image has no room for them, and nothing in the level can
; sound while the ring is open, so they take the room of the level's last
; samples, just under the gun region (packer sound_SNES.h): GUNSND.BIN
; carries them after the guns' records, with their own directory entries
; and pitch words for sources past the guns'. Opening, they go in; closing,
; SOUND.BIN's own bytes come back over them, and their directory entries go
; back to the silent block. The gun in her hands keeps its region.
;
; The upload is olSndGun's (main.asm): a STOP for source $FF sends the
; driver back to the IPL, blocks follow, and the driver starts again at
; $0200, idle. About 170 ms at the IPL's pace each way for LEVEL1's 8 KB,
; inside the tick that opens or closes the ring -- about one rendered frame.
;
; void olSndRing(unsigned int open) -- C ABI, open at 4,s. 816-tcc's
; scratch registers only: the caller is C.
;   tcc__r0  the block's source (far)     tcc__r1  the IPL's running byte
;   tcc__r2  GUNSND.BIN (far)             tcc__r3  count, first source
;   tcc__r4  APU address, bytes           tcc__r5  open
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.INCLUDE "snes.inc"
.INCLUDE "levels.inc"
.IFDEF FX3
.BASE $80
.BANK 6 SLOT 0                  ; $86
.SECTION "sound_ring" SEMIFREE
.ACCU 16
.INDEX 16
.16bit

olSndRing:
    php
    phb
    phk
    plb                         ; I/O, as olSndGun addresses it
    SETAXY16
    lda 6,s                     ; open, past P and B
    sta.b tcc__r5
    lda.l ol_sndReady
    beq @none
    lda.l olLevel+OLD_GUN_SIZE
    beq @none
    lda.l olLevel+OLD_GUN
    sta.b tcc__r2
    lda.l olLevel+OLD_GUN+2
    sta.b tcc__r2h
    ldy #74
    lda [tcc__r2],y             ; the ring's count
    bne +
@none:
    jmp @out
+   sta.b tcc__r3
    ldy #72
    lda [tcc__r2],y
    sta.b tcc__r3h              ; its first source
    ldy #76
    lda [tcc__r2],y
    sta.b tcc__r4               ; its APU address
    ldy #78
    lda [tcc__r2],y
    sta.b tcc__r4h              ; its bytes

    SETA8                       ; STOP, source $FF: back to the IPL
    ldx #$0000
-   lda $2140                   ; the last command acknowledged
    cmp.l olSndSeq
    beq +
    dex
    bne -
+   lda #$FF
    sta $2141
    lda.l olSndSeq
    clc
    adc #$04
    and #$FC
    bne +
    lda #$04
+   ora #$03                    ; STOP
    sta $2140
    ldx #$0000
-   SETA16
    lda $2140
    cmp #$BBAA
    SETA8
    beq +
    dex
    bne -
    jmp @lost                   ; the IPL never answered
+   lda #$CC
    sta.b tcc__r1

    SETA16
    lda.b tcc__r5
    beq @close
    ldy #80                     ; open: the directory entries
    lda [tcc__r2],y
    clc
    adc.b tcc__r2               ; GUNSND.BIN lies within one bank
    sta.b tcc__r0
    lda.b tcc__r2h
    sta.b tcc__r0h
    jsr RingEntries
    lda.b tcc__r3               ; their pitch words, after them
    asl a
    asl a
    clc
    adc.b tcc__r0
    sta.b tcc__r0
    lda.b tcc__r3h
    asl a
    clc
    adc #$0600
    tay
    lda.b tcc__r3
    asl a
    tax
    SETA8
    jsr RingBlock
    SETA16
    ldy #82                     ; and the samples
    lda [tcc__r2],y
    clc
    adc.b tcc__r2
    sta.b tcc__r0
    lda.b tcc__r2h
    sta.b tcc__r0h
    bra @samples

@close:
    lda.b tcc__r3h              ; close: the entries SOUND.BIN has there
    asl a
    asl a
    clc
    adc.l olLevel+OLD_SOUND     ; the image from $0400, within one bank
    sta.b tcc__r0
    lda.l olLevel+OLD_SOUND+2
    sta.b tcc__r0h
    jsr RingEntries
    lda.b tcc__r4               ; and its bytes under the ring
    sec
    sbc #$0400
    clc
    adc.l olLevel+OLD_SOUND
    sta.b tcc__r0
    lda.l olLevel+OLD_SOUND+2
    sta.b tcc__r0h
@samples:
    ldy.b tcc__r4
    ldx.b tcc__r4h
    SETA8
    jsr RingBlock
    ldy #$0200                  ; the end code: start the driver
    sty $2142
    stz $2141
    lda.b tcc__r1
    sta $2140
-   cmp $2140
    bne -
    stz $2140
    ldx #$0000
-   lda $2140
    beq +
    dex
    bne -
@lost:
    SETA16
    lda #$0000
    sta.l ol_sndReady           ; no driver: no sound from here
    bra @out
.ACCU 8
+   SETA16
    lda #$0000
    sta.l olSndSeq              ; idle, as after the boot's upload
@out:
    plb
    plp
    rtl

; The ring's directory entries from tcc__r0, to $0400 + first * 4. A16 in
; and out.
.ACCU 16
RingEntries:
    lda.b tcc__r3h
    asl a
    asl a
    clc
    adc #$0400
    tay
    lda.b tcc__r3
    asl a
    asl a
    tax
    SETA8
    jsr RingBlock
    SETA16
    rts

; main.asm's ApuBlock: X bytes (not 0) from [tcc__r0] to APU Y, through the
; IPL's protocol, its running byte in tcc__r1. A8.
.ACCU 8
RingBlock:
    sty $2142                   ; $2142 and $2143: the destination
    lda #$01
    sta $2141                   ; non-zero: a data block follows
    lda.b tcc__r1
    sta $2140
-   cmp $2140
    bne -
    ldy #$0000
@byte:
    lda [tcc__r0],y
    sta $2141
    tya
    sta $2140
-   cmp $2140
    bne -
    iny
    dex
    bne @byte
    clc
    adc #$02
    bne +
    inc a
+   sta.b tcc__r1
    rts

.ENDS
.ENDIF

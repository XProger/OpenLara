; OpenLara - SNES / Super FX target
; The sound driver: what runs on the SPC700.
;
; The S-DSP does everything a sample player needs in hardware -- BRR decode,
; pitch, per-voice volume, eight voices mixed -- so the sound CPU's whole job
; is to turn a four-byte command from the 65816 into register writes. It has
; no timer, no envelope, no music: the level's samples are uploaded once at
; boot (SOUND.BIN, from the packer) after this program, and from then on the
; sound CPU spins on port 0 waiting to be told to play one.
;
; The protocol, four ports, one command at a time:
;
;   $2141  source number         the sample, an index into the directory
;   $2142  volume 0..127         written to VOL(L) and VOL(R) alike -- mono,
;                                as the GBA mixes
;   $2143  pitch multiplier      128 is the sample's own rate; upstream's
;                                random pitch variation is +-10%
;   $2140  command               bits 7-2 a sequence number the host advances
;                                every command so two consecutive commands
;                                never read alike; bits 1-0 the mode:
;                                  0 UNIQUE  if the source is playing, retune
;                                            it; otherwise start it
;                                  1 REPLAY  if playing, restart it
;                                  2 NEW     start another voice regardless
;                                  3 STOP    key off every voice playing it;
;                                            source $FF: key off all and go
;                                            back to the IPL ROM, which then
;                                            says $BBAA -- the host's restart
;
; The host writes ports 1-3, then port 0; the driver acts and echoes the
; command byte on port 0 as its acknowledgement. Zero is never a command:
; the host writes it once, after the upload, to say the driver may start,
; and the driver answers with zero to say it has.
;
; Voices: one byte per voice holds the source it plays, $FF for none. A voice
; is free when that says so or when ENDX says its sample has run out -- the
; DSP sets that bit when the END block finishes and clears it at Key On.
; When all eight are busy the command is dropped, as upstream drops a sample
; past SND_CHANNELS.
;
; Memory: $0000-$001F this program's variables, $0100-$01EF stack, $0200 the
; program, $0400 the directory (DIR = $04), $0600 the pitch words, $0700
; the samples, $FF00 the four bytes the idle echo circuit still writes.

.MEMORYMAP
    DEFAULTSLOT 0
    SLOTSIZE $0200
    SLOT 0 $0200
.ENDME
.ROMBANKMAP
    BANKSTOTAL 1
    BANKSIZE $0200
    BANKS 1
.ENDRO

.DEFINE PORT0       $F4
.DEFINE PORT1       $F5
.DEFINE PORT2       $F6
.DEFINE PORT3       $F7
.DEFINE DSPADDR     $F2
.DEFINE DSPDATA     $F3

.DEFINE DSP_MVOLL   $0C
.DEFINE DSP_MVOLR   $1C
.DEFINE DSP_EVOLL   $2C
.DEFINE DSP_EVOLR   $3C
.DEFINE DSP_KON     $4C
.DEFINE DSP_KOF     $5C
.DEFINE DSP_FLG     $6C
.DEFINE DSP_ENDX    $7C
.DEFINE DSP_EFB     $0D
.DEFINE DSP_PMON    $2D
.DEFINE DSP_NON     $3D
.DEFINE DSP_EON     $4D
.DEFINE DSP_DIR     $5D
.DEFINE DSP_ESA     $6D
.DEFINE DSP_EDL     $7D

.DEFINE V_VOLL      $00
.DEFINE V_VOLR      $01
.DEFINE V_PITCHL    $02
.DEFINE V_PITCHH    $03
.DEFINE V_SRCN      $04
.DEFINE V_ADSR1     $05
.DEFINE V_GAIN      $07

.DEFINE PITCH_TABLE $0600

; --- variables, page zero ---------------------------------------------------
.DEFINE lastCmd     $00     ; the last command acted on
.DEFINE cmdSource   $01
.DEFINE cmdVolume   $02
.DEFINE cmdPitch    $03
.DEFINE cmdMode     $04
.DEFINE busy        $05     ; a bit per voice with a source assigned
.DEFINE pitchL      $06
.DEFINE pitchH      $07
.DEFINE tmpL        $08
.DEFINE tmpH        $09
.DEFINE voiceBit    $0A
.DEFINE voiceReg    $0B     ; voice * $10
.DEFINE voiceSrc    $10     ; $10-$17: the source each voice plays, $FF free

.BANK 0 SLOT 0
.ORG 0
.SECTION "driver" FORCE

Start:
    mov x,#$EF
    mov sp,x
    mov a,#0
    mov x,#0
-   mov $00+x,a
    inc x
    cmp x,#$20
    bne -
    mov a,#$FF
    mov x,#0
-   mov voiceSrc+x,a
    inc x
    cmp x,#8
    bne -

    ; The DSP: main volume up, echo and noise off, every voice keyed off,
    ; direct GAIN at full so the envelope is the sample's own.
    mov DSPADDR,#DSP_FLG
    mov DSPDATA,#$20            ; RES 0, MUTE 0, /ECEN 1, NCK 0
    mov DSPADDR,#DSP_KOF
    mov DSPDATA,#$FF
    mov DSPADDR,#DSP_MVOLL
    mov DSPDATA,#$7F
    mov DSPADDR,#DSP_MVOLR
    mov DSPDATA,#$7F
    mov DSPADDR,#DSP_EVOLL
    mov DSPDATA,#0
    mov DSPADDR,#DSP_EVOLR
    mov DSPDATA,#0
    mov DSPADDR,#DSP_EFB
    mov DSPDATA,#0
    mov DSPADDR,#DSP_PMON
    mov DSPDATA,#0
    mov DSPADDR,#DSP_NON
    mov DSPDATA,#0
    mov DSPADDR,#DSP_EON
    mov DSPDATA,#0
    mov DSPADDR,#DSP_DIR
    mov DSPDATA,#$04
    mov DSPADDR,#DSP_ESA
    mov DSPDATA,#$FF
    mov DSPADDR,#DSP_EDL
    mov DSPDATA,#0
    mov x,#0
-   mov a,x
    or a,#V_ADSR1
    mov DSPADDR,a
    mov DSPDATA,#0
    mov a,x
    or a,#V_GAIN
    mov DSPADDR,a
    mov DSPDATA,#$7F
    mov a,x
    mov DSPADDR,a               ; VOL(L)
    mov DSPDATA,#0
    inc a
    mov DSPADDR,a               ; VOL(R)
    mov DSPDATA,#0
    mov a,x
    clrc
    adc a,#$10
    mov x,a
    cmp x,#$80
    bne -
    ; Key Off has done its work by the time the host has noticed us; clear
    ; it so a later Key On is not fighting it.
    mov y,#0
-   dec y
    bne -
    mov DSPADDR,#DSP_KOF
    mov DSPDATA,#0

    ; The upload's last handshake value is still on port 0. Wait for the
    ; host to clear it, then answer in kind: from here on zero means idle.
-   mov a,PORT0
    bne -
    mov PORT0,#0

; --- the command loop --------------------------------------------------------
Loop:
    mov a,PORT0
    beq Loop
    cmp a,lastCmd
    beq Loop
    mov lastCmd,a
    mov cmdSource,PORT1
    mov cmdVolume,PORT2
    mov cmdPitch,PORT3
    and a,#3
    mov cmdMode,a

    ; Voices whose sample has ended are free again.
    mov DSPADDR,#DSP_ENDX
    mov a,DSPDATA
    and a,busy
    beq +
    eor a,#$FF
    and a,busy
    mov busy,a
    ; those with the bit now clear: forget their source
    mov x,#0
    mov a,#1
-   mov voiceBit,a
    and a,busy
    bne ++
    mov a,#$FF
    mov voiceSrc+x,a
++  mov a,voiceBit
    asl a
    inc x
    cmp x,#8
    bne -
+
    mov a,cmdMode
    cmp a,#3
    beq Stop
    cmp a,#2
    beq PlayNew
    ; UNIQUE and REPLAY look for a voice already playing the source.
    mov x,#0
    mov a,#1
-   mov voiceBit,a
    mov a,voiceSrc+x
    cmp a,cmdSource
    beq Found
    mov a,voiceBit
    asl a
    inc x
    cmp x,#8
    bne -
    bra PlayNew

Found:
    ; x = voice, voiceBit = its KON bit. Retune, and restart for REPLAY.
    call !VoiceRegs
    call !SetVolumePitch
    mov a,cmdMode
    cmp a,#1
    beq +
    jmp !Ack                    ; past a bne's reach since the $FF stop
+
    mov DSPADDR,#DSP_KON
    mov a,voiceBit
    mov DSPDATA,a
    bra Ack

PlayNew:
    ; the lowest free voice
    mov x,#0
    mov a,#1
-   mov voiceBit,a
    and a,busy
    beq Take
    mov a,voiceBit
    asl a
    inc x
    cmp x,#8
    bne -
    bra Ack                     ; all eight busy: dropped, as upstream drops

Take:
    call !VoiceRegs
    mov a,cmdSource
    mov voiceSrc+x,a
    mov a,voiceBit
    or a,busy
    mov busy,a
    mov a,voiceReg
    or a,#V_SRCN
    mov DSPADDR,a
    mov a,cmdSource
    mov DSPDATA,a
    call !SetVolumePitch
    mov DSPADDR,#DSP_KON
    mov a,voiceBit
    mov DSPDATA,a
    bra Ack

Stop:
    mov a,cmdSource
    cmp a,#$FF
    bne StopSource
    mov DSPADDR,#DSP_KOF        ; everything off, and the ramp under way
    mov DSPDATA,#$FF
    mov y,#0
-   dec y
    bne -
    mov $F1,#$B0                ; CONTROL: IPL ROM in, both port latches clear
    jmp !$FFC0                  ; the IPL: $BBAA on ports 0/1, waiting for $CC
StopSource:
    mov x,#0
    mov a,#1
-   mov voiceBit,a
    mov a,voiceSrc+x
    cmp a,cmdSource
    bne +
    mov DSPADDR,#DSP_KOF
    mov a,voiceBit
    mov DSPDATA,a
    eor a,#$FF
    and a,busy
    mov busy,a
    mov a,#$FF
    mov voiceSrc+x,a
+   mov a,voiceBit
    asl a
    inc x
    cmp x,#8
    bne -
    ; KOF bits are level, not edge: clear them once the ramp has started
    mov y,#0
-   dec y
    bne -
    mov DSPADDR,#DSP_KOF
    mov DSPDATA,#0

Ack:
    mov a,lastCmd
    mov PORT0,a
    jmp !Loop

; x = voice -> voiceReg = x * $10. Preserves x.
VoiceRegs:
    mov a,x
    asl a
    asl a
    asl a
    asl a
    mov voiceReg,a
    ret

; Volume to both channels; pitch = table[source] * multiplier / 128.
SetVolumePitch:
    mov a,voiceReg
    mov DSPADDR,a               ; VOL(L)
    mov a,cmdVolume
    mov DSPDATA,a
    inc DSPADDR                 ; VOL(R)
    mov DSPDATA,a

    mov a,cmdSource
    asl a
    mov y,a
    mov a,!PITCH_TABLE+y
    mov tmpL,a                  ; base pitch, low
    mov a,!PITCH_TABLE+1+y
    mov tmpH,a                  ; base pitch, high
    mov a,cmdPitch
    bne +
    mov a,#128                  ; zero means the sample's own rate
+   mov cmdPitch,a
    mov y,tmpH
    mul ya                      ; high byte * multiplier
    movw pitchL,ya
    movw ya,pitchL
    addw ya,pitchL              ; * 2: the /128 of a byte shifted up by 8
    movw pitchL,ya
    mov a,cmdPitch
    mov y,tmpL
    mul ya                      ; low byte * multiplier, then / 128
    movw tmpL,ya
    movw ya,tmpL
    addw ya,tmpL                ; * 2; carry = bit 15
    mov tmpL,y                  ; / 256
    mov a,#0
    rol a                       ; the bit that fell out
    mov tmpH,a
    movw ya,pitchL
    addw ya,tmpL
    movw pitchL,ya
    mov a,pitchH
    and a,#$3F                  ; fourteen bits
    mov pitchH,a

    mov a,voiceReg
    or a,#V_PITCHL
    mov DSPADDR,a
    mov a,pitchL
    mov DSPDATA,a
    inc DSPADDR
    mov a,pitchH
    mov DSPDATA,a
    ret

.ENDS

; Host direct-page ABI shared by the main loop and native RAM handoffs.
; Keep addresses identical on both sides of each ownership transition.
.IFNDEF OL_HOST_DP
.DEFINE OL_HOST_DP 1

.ENUM $00
    ; --- read by tools/typesnes_dump.ts at exactly these offsets ------------
    ; A .ENUM hands out addresses in declaration order and wlalink puts none
    ; of them in the symbol file, so the harness cannot look them up -- it
    ; reads $7E0000 and $7E0002. That makes their position part of the
    ; contract: add new variables *below* this pair, never above it. The last
    ; variable inserted above them moved both by two and the harness went on
    ; reading, quietly, from the wrong place.
    fps             dw      ; completed frames in the last 60 fields
    fbShow          db      ; buffer currently on screen (0/1)
    ; ------------------------------------------------------------------------

    keys            dw      ; current pad state in InputKey bits
    keysPrev        dw
    keysAccum       dw      ; every bit seen since the last game update
    frameIndex      dw
    fpsCounter      dw

    fbDraw          db      ; buffer the GSU is rendering into (0/1)
    gsuDone         db      ; native FX3: 0 unclaimed, 1 claimed, 3 prepared;
                           ; GSU-2: completed render, set by GSU IRQ
    dmaBusy         db      ; a frame buffer upload is in flight
    dmaBuffer       db      ; buffer being uploaded
    scmrShadow      db      ; SCMR with RON|RAN set

    dmaSrc          dw      ; game pak RAM offset of the next chunk
    dmaDst          dw      ; VRAM word address of the next chunk
    dmaLeft         dw      ; bytes still to move this frame

    tmapTmp         dw      ; boot scratch
    dmaTmp          dw      ; NMI scratch
    padRaw          dw      ; raw $4218/$4219 read
    cpuOwnsRam      db      ; desired game pak RAM-bus owner across NMI
.IFNDEF FX3
    romTaken        db      ; GSU-2: this tick holds the ROM bus (level load)
.ENDIF
    logicPhase      db      ; one 30 Hz tick for every two NTSC fields
    logicPending    db      ; ticks NMI has queued for the main loop

    ; The clock's gauges, published to C globals at the 60-field rollover so
    ; the runner can find them by name: ticks completed, and the field
    ; boundaries those ticks straddled -- which, over the tick count, is the
    ; fraction of a field a tick costs the renderer.
    tickCounter     dw
    tickFieldAcc    dw
    tickStart       dw      ; frameIndex when the running tick began
.IFDEF FX3
    renderHold      db      ; 0 idle; $90..$80 bounded publication countdown
.ENDIF
    hostDpEnd       db      ; not a variable: where the list ends, for below
.ENDE

; The .ENUM has no linker check, and on FX3 src/host_tables.asm keeps table
; pointers from $28 (olRoomBase). One byte added above once pushed
; renderHold onto olRoomBase's low byte: every NMI rewrote the room table
; pointer, and room probes read zeros. Stop at assembly instead.
.IFDEF FX3
.IF hostDpEnd > $28
.FAIL "host direct-page variables overrun $28, FX3's olRoomBase"
.ENDIF
.ELSE
.IF hostDpEnd > $30
.FAIL "host direct-page variables overrun $30, the tcc registers"
.ENDIF
.ENDIF

.ENDIF

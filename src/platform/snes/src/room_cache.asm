; Resolved room-query cache. Room_reset invalidates all entries when
; mutable sectors change. Keep the original direct-page cache as the fast
; first lookup, and reuse exact resolutions across interleaved actor probes.
.INCLUDE "snes.inc"
.INCLUDE "hdr.asm"
.INCLUDE "src/host_dp.inc"
.INCLUDE "config.inc"
.IFNDEF C_MOTION
.IFDEF FX3
.DEFINE RQC_COUNT 32
; FastROM leaves this WRAM region available beside the navigation data.
; Keep interleaved actor probes out of the smaller general BSS allocation.
.RAMSECTION "room_query_cache" BANK $7E SLOT 9
olRoomQueryCache dsb RQC_COUNT*32 ; direct-mapped, 16-word records
olRoomQueryCacheSlot dw
olRoomQueryCacheUnaligned dw   ; arbitrary grids resolve without caching
olRoomQueryCacheEnabled dw     ; two or more admitted creatures share probes
.ENDS
.ELSE
.DEFINE RQC_COUNT 32
.ENDIF

.IFDEF FX3
.BASE $80
.BANK 4 SLOT 0
.ELSE
.BASE $3F
.BANK 63 SLOT 6                 ; resident $7E:8000 code image
.ENDIF
.SECTION "room_query_cache_native" SEMIFREE
.ACCU 16
.INDEX 16
.16bit

.IFNDEF FX3
; Link writable records into the copied WRAM image. This spends resident
; image headroom, not the nearly full general BSS, and cannot overlap code.
olRoomQueryCache:
    .DSB RQC_COUNT*32, 0
olRoomQueryCacheSlot:
    .DW 0
olRoomQueryCacheUnaligned:
    .DW 0
olRoomQueryCacheEnabled:
    .DW 0
.ENDIF

.IFNDEF FX3
.IFNDEF C_MOTION
; Native CPU-only regions can let the GSU use cartridge RAM concurrently.
; A16 result: 1 if ownership must be reclaimed, otherwise 0. X/Y, DB and D
; are untouched. Update the desired owner before SCMR so NMI
; restores the right bus state at every possible interruption boundary.
olGsuYieldRam:
    SETA8
    lda.l cpuOwnsRam
    beq @none
    lda.l scmrShadow
    and #$08
    beq @none
    lda #0
    sta.l cpuOwnsRam
    lda.l scmrShadow
    sta.l GSU_SCMR
    SETA16
    lda #1
    rtl
@none:
    SETA16
    lda #0
    rtl

; A16 is olGsuYieldRam's result. Call before any cartridge access resumes.
olGsuReclaimRam:
    and #$00FF
    beq @done
    SETA8
    lda #1
    sta.l cpuOwnsRam
    lda.l scmrShadow
    and #$F7
    sta.l GSU_SCMR
    SETA16
@done:
    rtl
.ENDIF
.ENDIF

RQ_CacheReset:
.IFDEF FX3
    stz.w olRoomQueryCacheEnabled
.ELSE
    lda #1                     ; GSU-2 has a shared flood, no Nav owner pool
    sta.w olRoomQueryCacheEnabled
.ENDIF
    ldx #(RQC_COUNT-1)*32
-   lda #$FFFF                  ; impossible valid room hint
    sta.w olRoomQueryCache,x
    txa
    sec
    sbc #32
    tax
    bpl -
    rtl

.IFDEF FX3
RQ_CacheSelect:
    ldx #0
    lda.l olNav+8214
    cmp #$FFFF
    beq +
    inx
+   lda.l olNav+8240+8214
    cmp #$FFFF
    beq +
    inx
+   lda.l olNav+16480+8214
    cmp #$FFFF
    beq +
    inx
+   txa
    cmp #2
    bcc +
    lda #1
    sta.w olRoomQueryCacheEnabled
    rtl
+   stz.w olRoomQueryCacheEnabled
    rtl
.ENDIF

RQ_CacheLookup:
    lda.w olRoomQueryCacheUnaligned
    beq +
    lda #0
    rtl
+
    lda.b rq_x
.IFDEF FX3
    ; XOR commutes with the byte swap and logical shift. Combine the two
    ; coordinates first; the final mask retains the identical slot key.
    eor.b rq_z
    xba
    lsr a
    lsr a
    eor.b rq_hint
    sta.b tcc__r2
.ELSE
    xba
    lsr a
    lsr a
    eor.b rq_hint
    sta.b tcc__r2
    lda.b rq_z
    xba
    lsr a
    lsr a
    eor.b tcc__r2
    sta.b tcc__r2
.ENDIF
    lda.b rq_y
    xba
    and #$00FF
    eor.b tcc__r2
    and #(RQC_COUNT-1)
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    stx.w olRoomQueryCacheSlot
    lda.w olRoomQueryCache,x
    cmp.b rq_hint
    bne RQC_Miss
    lda.b rq_x
    and #$FC00
    cmp.w olRoomQueryCache+2,x
    bne RQC_Miss
    lda.b rq_z
    and #$FC00
    cmp.w olRoomQueryCache+4,x
    bne RQC_Miss
    lda.b rq_y
    and #$FF00
    cmp.w olRoomQueryCache+6,x
    bne RQC_Miss
    lda.w olRoomQueryCache,x
    sta.b rq_qcHint
    lda.w olRoomQueryCache+2,x
    sta.b rq_qcSX
    lda.w olRoomQueryCache+4,x
    sta.b rq_qcSZ
    lda.w olRoomQueryCache+6,x
    sta.b rq_qcY
    lda.w olRoomQueryCache+8,x
    sta.b rq_qcRoom
    lda.w olRoomQueryCache+10,x
    sta.b rq_qcTrigger
    lda.w olRoomQueryCache+12,x
    sta.b rq_qcRX
    lda.w olRoomQueryCache+14,x
    sta.b rq_qcRZ
    lda.w olRoomQueryCache+16,x
    sta.b rq_qcFSX
    lda.w olRoomQueryCache+18,x
    sta.b rq_qcFSZ
    lda.w olRoomQueryCache+20,x
    sta.b rq_qcFloor
    lda.w olRoomQueryCache+22,x
    sta.b rq_qcCeil
    lda.w olRoomQueryCache+24,x
    sta.b rq_qcFsx
    lda.w olRoomQueryCache+26,x
    sta.b rq_qcFsz
    lda.w olRoomQueryCache+28,x
    sta.b rq_qcCsx
    lda.w olRoomQueryCache+30,x
    sta.b rq_qcCsz
    lda #1
    rtl
RQC_Miss:
    lda #0
    rtl

RQ_CacheStore:
    lda.w olRoomQueryCacheUnaligned
    beq +
    lda #$FFFF
    sta.b rq_qcHint             ; preserve resolution, but do not reuse it
    rtl
+
    ldx.w olRoomQueryCacheSlot
    lda.b rq_qcHint
    sta.w olRoomQueryCache,x
    lda.b rq_qcSX
    sta.w olRoomQueryCache+2,x
    lda.b rq_qcSZ
    sta.w olRoomQueryCache+4,x
    lda.b rq_qcY
    sta.w olRoomQueryCache+6,x
    lda.b rq_qcRoom
    sta.w olRoomQueryCache+8,x
    lda.b rq_qcTrigger
    sta.w olRoomQueryCache+10,x
    lda.b rq_qcRX
    sta.w olRoomQueryCache+12,x
    lda.b rq_qcRZ
    sta.w olRoomQueryCache+14,x
    lda.b rq_qcFSX
    sta.w olRoomQueryCache+16,x
    lda.b rq_qcFSZ
    sta.w olRoomQueryCache+18,x
    lda.b rq_qcFloor
    sta.w olRoomQueryCache+20,x
    lda.b rq_qcCeil
    sta.w olRoomQueryCache+22,x
    lda.b rq_qcFsx
    sta.w olRoomQueryCache+24,x
    lda.b rq_qcFsz
    sta.w olRoomQueryCache+26,x
    lda.b rq_qcCsx
    sta.w olRoomQueryCache+28,x
    lda.b rq_qcCsz
    sta.w olRoomQueryCache+30,x
    rtl
.ENDS
.ENDIF

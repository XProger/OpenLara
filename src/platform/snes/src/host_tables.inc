; Full FX3 table pointers are in direct-page gaps, not one implicit bank.
; The low word aliases the legacy Base name for native pointer construction.
; Indexed readers use relative indices and HOST_LDA; it clobbers Y, keeps
; X/carry and sets N/Z just as LDA does. Callers must not keep live state in Y.
.MACRO HOST_ADD_BASE ARGS base
.IFNDEF FX3
    adc.w base
.ENDIF
.ENDM

; A = FX3 zone family (ground-1/ground-2/fly: 0/1/2). The selected flip's
; offsets are cached at level/flip changes. Clobbers Y and A, preserves X.
.MACRO HOST_ZONE_OFFSET
    asl a
    tay
    lda.w olZoneOffsets,y
.ENDM

; Adjacent fields of the same record, when Y still holds the preceding
; field's relative index. Preserves carry/X, exactly like HOST_LDA.
.MACRO HOST_LDA_NEXT ARGS ptr, field, delta
.IFDEF FX3
    .IF delta > 0
    .REPT delta
    iny
    .ENDR
    .ELSE
    .REPT -delta
    dey
    .ENDR
    .ENDIF
    lda [ptr],y
.ELSE
    lda.l $710000+field,x
.ENDIF
.ENDM

.MACRO HOST_SBC ARGS ptr, field
.IFDEF FX3
    txy
    .REPT field
    iny
    .ENDR
    sbc [ptr],y
.ELSE
    sbc.l $710000+field,x
.ENDIF
.ENDM

.MACRO HOST_LDA ARGS ptr, field
.IFDEF FX3
    txy
    .REPT field
    iny
    .ENDR
    lda [ptr],y
.ELSE
    lda.l $710000+field,x
.ENDIF
.ENDM

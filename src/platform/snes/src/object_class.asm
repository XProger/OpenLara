; Native object classification. The generated table executes the unchanged C
; ItemObj_class/Enemy_row definitions at build time; C_MOTION keeps that C.
; A/X/Y 16, D=0, DB=$7E, JSL; type is the unsigned word at 4,s, result r0.
.INCLUDE "hdr.asm"
.INCLUDE "config.inc"
.IFNDEF C_MOTION
.IFDEF FX3
.BASE $80
.BANK 6 SLOT 0
.ELSE
.BASE $3F
.BANK 63 SLOT 6                ; resident $7E:8000 image, including its table
.ENDIF
.SECTION "object_class_native" SEMIFREE
.accu 16
.index 16
.16bit

ItemObj_classTable:
.INCLUDE "object_class_table.inc"
ItemObj_classTableEnd:

ItemObj_class:
    lda 4,s
    cmp #256
    bcs OC_Unknown
    tax
    lda.l ItemObj_classTable,x
    and #$00FF
    sta.b tcc__r0
    rtl
OC_Unknown:
    lda #OBJECT_CLASS_NONE
    sta.b tcc__r0
    rtl

.ENDS
.ENDIF

        .macro COPY_ROW
        mov.l   @r0+, r1
        mov.l   @r0+, r2
        mov.l   @r0+, r3
        mov.l   @r0+, r4
        mov.l   r1, @(32, r0)
        mov.l   r2, @(36, r0)
        mov.l   r3, @(40, r0)
        mov.l   r4, @(44, r0)
        .endm

        .align 4

        .global _matrixPush_asm
_matrixPush_asm:
        mov.l   .var_matrixPtr, r5
        mov.l   @r5, r0

        COPY_ROW /* row[0] */
        COPY_ROW /* row[1] */
        COPY_ROW /* row[2] */

        rts
        mov.l   r0, @r5

.var_matrixPtr:
        .long	_gMatrixPtr

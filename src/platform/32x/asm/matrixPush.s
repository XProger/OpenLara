        .text

        .global _matrixPush_asm
_matrixPush_asm:
        mov.l   .var_matrixPtr, r5
        mov.l   @r5, r0

        /* row[0] */
        mov.l   @r0+, r1
        mov.l   @r0+, r2
        mov.l   @r0+, r3
        mov.l   @r0+, r4
        mov.l   r1, @(32, r0)
        mov.l   r2, @(36, r0)
        mov.l   r3, @(40, r0)
        mov.l   r4, @(44, r0)

        /* row[1] */
        mov.l   @r0+, r1
        mov.l   @r0+, r2
        mov.l   @r0+, r3
        mov.l   @r0+, r4
        mov.l   r1, @(32, r0)
        mov.l   r2, @(36, r0)
        mov.l   r3, @(40, r0)
        mov.l   r4, @(44, r0)

        /* row[2] */
        mov.l   @r0+, r1
        mov.l   @r0+, r2
        mov.l   @r0+, r3
        mov.l   @r0+, r4
        mov.l   r1, @(32, r0)
        mov.l   r2, @(36, r0)
        mov.l   r3, @(40, r0)
        mov.l   r4, @(44, r0)

        rts
        mov.l   r0, @r5

.var_matrixPtr:
        .long	_gMatrixPtr

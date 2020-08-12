@--------------------------------------------------------------------------------
@ udiv.s
@--------------------------------------------------------------------------------
@ Provides an implementation of signed division
@--------------------------------------------------------------------------------

@ refer to the unsigned division
    .extern __aeabi_uidivmod
    .extern __aeabi_uidiv

@ r0: the numerator / r1: the denominator
@ after it, r0 has the quotient and r1 has the modulo
    .section .iwram, "ax", %progbits
    .align 2
    .arm
    .global __aeabi_idivmod
    .type __aeabi_idivmod STT_FUNC
__aeabi_idivmod:

    .section .iwram, "ax", %progbits
    .align 2
    .arm
    .global __aeabi_idiv
    .type __aeabi_idiv STT_FUNC
__aeabi_idiv:

    @ Move the lr to r12 and make the numbers positive
    mov     r12, lr

    cmp     r0, #0
    rsblt   r0, #0
    orrlt   r12, #1 << 30

    cmp     r1, #0
    rsblt   r1, #0
    orrlt   r12, #1 << 31

    @ Call the unsigned division
    .extern udiv32pastzero
    bl      udiv32pastzero

    @ Test the old sign bits
    tst     r12, #1 << 30
    rsbne   r0, r0, #0
    rsbne   r1, r0, #0
    tst     r12, #1 << 31
    rsbne   r0, r0, #0

    @ Erase the sign bits from the return address, and return
    bic     r12, #3 << 30
    bx      r12

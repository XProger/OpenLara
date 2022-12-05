#include "common_asm.inc"

dst .req r0     // arg
end .req r14

.global clearFB_asm
clearFB_asm:
    stmfd sp!, {r4-r6}
    fiq_on

    // 12 words
    mov r1, #0
    mov r2, #0
    mov r3, #0
    mov r4, #0
    mov r5, #0
    mov r6, #0
    // FIQ regs
    mov r8, #0
    mov r9, #0
    mov r10, #0
    mov r11, #0
    mov r12, #0
    mov r13, #0
    add end, dst, #(FRAME_WIDTH * FRAME_HEIGHT)

.loop:
    // fill 12 * 4 * 8 bytes per iteration
    stmia dst!, {r1-r6, r8-r13}
    stmia dst!, {r1-r6, r8-r13}
    stmia dst!, {r1-r6, r8-r13}
    stmia dst!, {r1-r6, r8-r13}
    stmia dst!, {r1-r6, r8-r13}
    stmia dst!, {r1-r6, r8-r13}
    stmia dst!, {r1-r6, r8-r13}
    stmia dst!, {r1-r6, r8-r13}

    cmp dst, end
    blt .loop

    fiq_off
    ldmfd sp!, {r4-r6}
    bx lr

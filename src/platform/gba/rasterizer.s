.section .iwram
.arm

// Thanks to Gericom

.macro LIGHTMAP_INDEX res, g
    and \res, \g, #0x1F00           // res = g & 0x1F00
.endm

.macro LIGHTMAP_OFFSET res, index, lightmap
    add \res, \res, \lightmap       // res += lightmap
.endm

.macro FETCH_LIGHTMAP res, index, lightmap
    ldrb \res, [\lightmap, \index]  // res = lightmap[index]
.endm

.macro FETCH_TILE res, uv, tile
    and \res, \uv, #0xFF00
    orr \res, \res, \uv, lsr #24    // res = (uv & 0xFF00) | (uv >> 24)
    ldrb \res, [\tile, \res]        // res = tile[res]
.endm

.macro FETCH_PALETTE res, idx, pal
    mov \res, \idx, lsl #1
    ldrh \res, [\pal, \res]         // res = pal[index]
.endm

.macro PUT_PIXEL buf, col, off
    strh \col, [\buf, #\off]        // buf[off/2] = col
.endm

.macro INC_PIXEL buf, col, off
    ldrh \col, [\buf, #\off]
    add \col, \col, #4
    strh \col, [\buf, #\off]        // buf[off/2] = col
.endm

.macro PUT_PIXEL_A buf, col, off
    cmp \col, #0                    // if (col != 0)
    strneh \col, [\buf, #\off]      //     buf[off/2] = col
.endm

.macro CHECK_WIDTH width, stack
    subs \width, \width, #1
    ldmmifd sp!, \stack             // if (--width < 0) return
.endm

.macro PUT_PIXELS_F width, pix0, pix1, stack
    PUT_PIXEL r0, r2, \pix0

    CHECK_WIDTH \width, "\stack"

    PUT_PIXEL r0, r2, \pix1
.endm

.macro PUT_PIXELS_G width, pix0, pix1, stack
// r2 - g
// r3 - dgdx
// r11 - palette
// lr - lightmap
    LIGHTMAP_INDEX r6, r2
    FETCH_LIGHTMAP r6, r6, lr
    FETCH_PALETTE r6, r6, r11

    PUT_PIXEL r0, r6, \pix0

    CHECK_WIDTH \width, "\stack"

    PUT_PIXEL r0, r6, \pix1

    add r2, r2, r3      // g += dgdx
.endm

.macro PUT_PIXELS_FT width, pix0, pix1, stack
    FETCH_TILE r6, r2, r12
    FETCH_LIGHTMAP r6, r6, lr
    FETCH_PALETTE r6, r6, r11
    PUT_PIXEL r0, r6, \pix0

    CHECK_WIDTH \width, "\stack"

    add r2, r2, r3      // t += dtdx

    FETCH_TILE r6, r2, r12
    FETCH_LIGHTMAP r6, r6, lr
    FETCH_PALETTE r6, r6, r11
    PUT_PIXEL r0, r6, \pix1

    add r2, r2, r3      // t += dtdx
.endm

.macro PUT_PIXELS_GT width, pix0, pix1, stack
    LIGHTMAP_INDEX r7, r2
    LIGHTMAP_OFFSET r7, r7, lr

    FETCH_TILE r6, r3, r12
    FETCH_LIGHTMAP r6, r6, r7
    FETCH_PALETTE r6, r6, r11
    PUT_PIXEL r0, r6, \pix0

    CHECK_WIDTH \width, "\stack"

    add r3, r3, r5      // t += dtdx

    FETCH_TILE r6, r3, r12
    FETCH_LIGHTMAP r6, r6, r7
    FETCH_PALETTE r6, r6, r11
    PUT_PIXEL r0, r6, \pix1

    add r3, r3, r5      // t += dtdx
    add r2, r2, r4      // g += dgdx
.endm

.macro PUT_PIXELS_FTA width, pix0, pix1, stack
    FETCH_TILE r6, r2, r12
    FETCH_LIGHTMAP r6, r6, lr
    FETCH_PALETTE r6, r6, r11
    PUT_PIXEL_A r0, r6, \pix0

    CHECK_WIDTH \width, "\stack"

    add r2, r2, r3      // t += dtdx

    FETCH_TILE r6, r2, r12
    FETCH_LIGHTMAP r6, r6, lr
    FETCH_PALETTE r6, r6, r11
    PUT_PIXEL_A r0, r6, \pix1

    add r2, r2, r3      // t += dtdx
.endm

.macro PUT_PIXELS_GTA width, pix0, pix1, stack
    LIGHTMAP_INDEX r7, r2
    LIGHTMAP_OFFSET r7, r7, lr

    FETCH_TILE r6, r3, r12
    FETCH_LIGHTMAP r6, r6, r7
    FETCH_PALETTE r6, r6, r11
    PUT_PIXEL_A r0, r6, \pix0

    CHECK_WIDTH \width, "\stack"

    add r3, r3, r5      // t += dtdx

    FETCH_TILE r6, r3, r12
    FETCH_LIGHTMAP r6, r6, r7
    FETCH_PALETTE r6, r6, r11
    PUT_PIXEL_A r0, r6, \pix1

    add r3, r3, r5      // t += dtdx
    add r2, r2, r4      // g += dgdx
.endm

.macro SCANLINE buf, width, func, stack
    sub \width, \width, #1         //--width
1:
    \func \width, 0, 2, "\stack"    // 0, 1
    CHECK_WIDTH \width, "\stack"

    \func \width, 4, 6, "\stack"    // 2, 3
    CHECK_WIDTH \width, "\stack"

    \func \width, 8, 10, "\stack"   // 4, 5
    CHECK_WIDTH \width, "\stack"

    \func \width, 12, 14, "\stack"  // 6, 7

    add \buf, \buf, #16             // buf += 8px

    subs \width, \width, #1
    bpl 1b
    ldmfd sp!, \stack
.endm

.global scanlineF_asm
scanlineF_asm:
// r0 = pixel
// r1 = width
// r2 = color
    stmfd sp!, {lr}

    SCANLINE r0, r1, PUT_PIXELS_F, "{pc}"

.global scanlineG_asm
scanlineG_asm:
// r0 = pixel
// r1 = width
// r2 = g
// r3 = dgdx
    stmfd sp!, {r6,r11,lr}
    ldr lr,= ft_lightmap
    ldr r11,= palette
    ldr lr, [lr] // ft_lightmap = *ft_lightmap

    SCANLINE r0, r1, PUT_PIXELS_G, "{r6,r11,pc}"

.global scanlineFT_asm
scanlineFT_asm:
// r0 = pixel
// r1 = width
// r2 = t
// r3 = dtdx
    stmfd sp!, {r6,r7,r11,lr}
    ldr r11,= palette
    ldr r12,= tile
    ldr lr,= ft_lightmap
    ldr r12, [r12] // tile = *tile
    ldr lr, [lr] // ft_lightmap = *ft_lightmap

    SCANLINE r0, r1, PUT_PIXELS_FT, "{r6,r7,r11,pc}"

.global scanlineGT_asm
scanlineGT_asm:
// r0 = pixel
// r1 = width
// r2 = g
// r3 = t
    stmfd sp!, {r4,r5,r6,r7,r11,lr}
stack_offset = 6 * 4
dgdx = stack_offset + 0
dtdx = stack_offset + 4
    ldr r4, [sp, #dgdx]
    ldr r5, [sp, #dtdx]
    ldr r11,= palette
    ldr r12,= tile
    ldr lr,= lightmap
    ldr r12, [r12]

    SCANLINE r0, r1, PUT_PIXELS_GT, "{r4,r5,r6,r7,r11,pc}"

.global scanlineFTA_asm
scanlineFTA_asm:
// r0 = pixel
// r1 = width
// r2 = t
// r3 = dtdx
    stmfd sp!, {r6,r7,r11,lr}
    ldr r11,= palette
    ldr r12,= tile
    ldr lr,= ft_lightmap
    ldr r12, [r12] // tile = *tile
    ldr lr, [lr] // ft_lightmap = *ft_lightmap

    SCANLINE r0, r1, PUT_PIXELS_FTA, "{r6,r7,r11,pc}"

.global scanlineGTA_asm
scanlineGTA_asm:
// r0 = pixel
// r1 = width
// r2 = g
// r3 = t
    stmfd sp!, {r4,r5,r6,r7,r11,lr}
stack_offset = 6 * 4
dgdx = stack_offset + 0
dtdx = stack_offset + 4
    ldr r4, [sp, #dgdx]
    ldr r5, [sp, #dtdx]
    ldr r11,= palette
    ldr r12,= tile
    ldr lr,= lightmap
    ldr r12, [r12]

    SCANLINE r0, r1, PUT_PIXELS_GTA, "{r4,r5,r6,r7,r11,pc}"



//------------ WORK IN PROGRESS ---------------

.macro CHECK_WIDTH_INNER width
    subs \width, \width, #1
    bmi 2f                      // if (--width < 0) return
.endm

.macro PUT_PIXELS_F_INNER buf, width, color, pix0, pix1
    PUT_PIXEL \buf, \color, \pix0

    CHECK_WIDTH_INNER \width

    PUT_PIXEL \buf, \color, \pix1
.endm

.macro SCANLINE_INNER buf, width, color, func
    sub \width, \width, #1          //--width
3:
    \func \buf, \width, \color, 0, 2    // 0, 1
    CHECK_WIDTH_INNER \width

    \func \buf, \width, \color, 4, 6    // 2, 3
    CHECK_WIDTH_INNER \width

    \func \buf, \width, \color, 8, 10   // 4, 5
    CHECK_WIDTH_INNER \width

    \func \buf, \width, \color, 12, 14  // 6, 7

    add \buf, \buf, #16             // buf += 8px

    subs \width, \width, #1
      bpl 3b
.endm


.global rasterizeF_inner_asm
rasterizeF_inner_asm:
// r0 = pixel
// r1 = L edge
// r2 = R edge
// r3 = color
    stmfd sp!, {r4,r5,r6,r7,r8,lr}
// ip - h
    ldr ip, [r1]        // ip = L.h
    ldr lr, [r2]        // lr = R.h
    cmp lr, ip
      movlt r4, lr      // if (R.h < L.h) h = R.h
      movge r4, ip      // else h = L.h
    sub ip, ip, r4
    sub lr, lr, r4
    str ip, [r1]        // L.h -= h
    str lr, [r2]        // R.h -= h

    ldr ip, [r1, #4]    // ip = L.x
    ldr lr, [r2, #4]    // lr = R.x
    ldr r7, [r1, #16]   // r1 = L.dx
    ldr r8, [r2, #16]   // r2 = R.dx

    sub r4, r4, #1
1:
    lsr r6, ip, #16             // r6 = x1 = (L.x >> 16)
    rsbs r5, r6, lr, lsr #16    // r5 = width = (R.x >> 16) - r6
      ble 2f                    // if (width <= 0) go next scanline

    add r6, r0, r6, lsl #1      // r6 = pixel + x1

    SCANLINE_INNER r6, r5, r3, PUT_PIXELS_F_INNER

2:
    add ip, ip, r7          // L.x += L.dx
    add lr, lr, r8          // R.x += R.dx
    add r0, r0, #320        // pixel += FRAME_WIDTH (160)

    subs r4, r4, #1
      bpl 1b
    str ip, [r1, #4]
    str lr, [r2, #4]

    ldmfd sp!, {r4,r5,r6,r7,r8,pc}


/* TODO
.global rasterizeF_asm
rasterizeF_asm:
// r0 = pixel
// r1 = L edge
// r2 = R edge
// r3 = palIndex -> color

// r4 - Lh
// r5 - Lx
// r6 - Ldx
// r7 - Rh
// r8 - Rx
// r9 - Rdx
    // x1 - asr
    // r12 - width

    stmfd sp!, {r4,r5,r6,r7,r8,r9,lr}

    ldr lr,= lightmap
    ldr r12,= palette

    // uint16 color = palette[lightmap[(L.top->v.g << 8) | palIndex]];
    ldr r6, [r1, #28]        // r6 = L.top
    ldrb r6, [r6, #6]        // r6 = L.top->v.g
    orr r3, r3, r6, lsl #8   // r3 = (L.top->v.g << 8) | palIndex
    ldrb r3, [lr, r3]        // r3 = lightmap[r3]
    add r3, r12, r3, lsl #1  // r3 = palette + r6
    ldrh r3, [r3]            // r3 = palette[r3] // color

1:
    2:
        ldr r7, [r1]
        cmp r7, #0
        ldreq r7, [r1, #28]
        ldreq r7, [r7, #12]


        beq 2b




    SCANLINE r0, r1, PUT_PIXELS_F, "{r4,r5,r6,r7,r11,pc}"

    b 1b
*/

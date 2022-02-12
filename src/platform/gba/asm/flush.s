#include "common_asm.inc"

flags        .req r0    // flags is always in r0 for rasterize & draw* calls
vXY          .req r1
vZG          .req r2
tmp          .req r3

OT           .req r4
list         .req r5
face         .req r6
VERTICES     .req r7
TEXTURES     .req r8
SPRITES      .req r9
TILE         .req r10
MASK         .req r11

index01      .req r12
index23      .req lr

faces        .req vXY
uv01         .req index01
uv23         .req index23
uwvh         .req index01
verticesBase .req vZG
facesBase    .req vZG
vertex       .req vZG
texture      .req tmp
texAnim      .req vXY
texIndex     .req tmp
texTile      .req tmp
sprite       .req tmp
sprIndex     .req tmp
sprTile      .req tmp
type         .req tmp
zero         .req tmp
uv           .req tmp
vXY0         .req vXY
vZG0         .req vZG
vXY1         .req index01
vZG1         .req index23

SP_SIZE = (16 * VERTEX_SIZEOF)

.extern rasterize_c, drawTriangle, drawQuad, drawPoly

.global flush_asm
flush_asm:
    stmfd sp!, {r4-r11, lr}

    ldr verticesBase, =gVerticesBase
    ldr VERTICES, =gVertices
    str VERTICES, [verticesBase]

    ldr tmp, =gFacesBase
    ldr faces, =gFaces
    ldr facesBase, [tmp]

    cmp facesBase, faces
    ldmeqfd sp!, {r4-r11, lr}
    bxeq lr

    str faces, [tmp]

    ldr tmp, =level
    ldr TILE, =gTile
    ldr TEXTURES, [tmp, #LEVEL_TEXTURES]
    ldr SPRITES, [tmp, #LEVEL_SPRITES]
    ldr OT, =gOT
    add list, OT, #((OT_SIZE - 1) << 2)

    mov MASK, #0xFF00
    orr MASK, MASK, MASK, lsl #16

    sub sp, #SP_SIZE
.loop_ot:
    ldr face, [list], #-4   // read the first face from the list and decrement
    cmp face, #0
    beq .next_ot            // list is empty, go next

    mov zero, #0
    str zero, [list, #4]    // reset the list pointer in OT

.loop_list:
    ldmia face, {flags, face, index01, index23} // read face params and next face

    and type, flags, #FACE_TYPE_MASK
    
.draw_primitive: // shadows, triangles, quads and clipped polys
    cmp type, #FACE_TYPE_GTA
    bgt .draw_sprite

  .set_vertices:
    // 1st vertex
    mov vertex, index01, lsl #16
    add vertex, VERTICES, vertex, lsr #(16 - 3)
    ldmia vertex, {vXY, vZG}
    stmia sp, {vXY, vZG}

    // 2nd vertex
    add vertex, VERTICES, index01, lsr #(16 - 3) // assumption: vertex index will never exceed 8191
    ldmia vertex, {vXY, vZG}
    str vXY, [sp, #(VERTEX_X + VERTEX_SIZEOF * 1)]
    str vZG, [sp, #(VERTEX_Z + VERTEX_SIZEOF * 1)]
    
    // 3rd vertex
    mov vertex, index23, lsl #16
    add vertex, VERTICES, vertex, lsr #(16 - 3)
    ldmia vertex, {vXY, vZG}
    str vXY, [sp, #(VERTEX_X + VERTEX_SIZEOF * 2)]
    str vZG, [sp, #(VERTEX_Z + VERTEX_SIZEOF * 2)]

    // 4th vertex (quads only)
    tst flags, #FACE_TRIANGLE
    addeq vertex, VERTICES, index23, lsr #(16 - 3)
    ldmeqia vertex, {vXY, vZG}
    streq vXY, [sp, #(VERTEX_X + VERTEX_SIZEOF * 3)]
    streq vZG, [sp, #(VERTEX_Z + VERTEX_SIZEOF * 3)]

    // skip texturing for FACE_TYPE_SHADOW and FACE_TYPE_F
    cmp type, #FACE_TYPE_F
    ble .draw

  .set_texture:
    mov texIndex, flags, lsl #(32 - FACE_TEXTURE_BITS)
    //cmp texIndex, #(MAX_ANIM_TEX << (32 - FACE_TEXTURE_BITS)) // TODO split to animated and static textures arrays
    add texIndex, texIndex, texIndex, lsl #1
    add texture, TEXTURES, texIndex, lsr #(32 - FACE_TEXTURE_BITS - 2)
    //addge texture, TEXTURES, texIndex, lsr #(32 - FACE_TEXTURE_BITS - 2)
    //ldrlt texAnim, =gAnimTextures
    //addlt texture, texAnim, texIndex, lsr #(32 - FACE_TEXTURE_BITS - 2)

    ldmia texture, {texTile, uv01, uv23}
    str texTile, [TILE]

    and uv, MASK, uv01
    str uv, [sp, #(VERTEX_T + VERTEX_SIZEOF * 0)]
    and uv, MASK, uv01, lsl #8
    str uv, [sp, #(VERTEX_T + VERTEX_SIZEOF * 1)]
    and uv, MASK, uv23
    str uv, [sp, #(VERTEX_T + VERTEX_SIZEOF * 2)]
    and uv, MASK, uv23, lsl #8
    str uv, [sp, #(VERTEX_T + VERTEX_SIZEOF * 3)]

  .draw:
    // r0 = flags
    mov r1, sp
    adr lr, .next_face

    tst flags, #FACE_CLIPPED
    bne drawPoly
    tst flags, #FACE_TRIANGLE
    bne drawTriangle
    beq drawQuad

.draw_sprite: // sprites and gui elements
    mov vertex, index01, lsl #16
    add vertex, VERTICES, vertex, lsr #(16 - 3)
    ldmia vertex, {vXY0, vZG0, vXY1, vZG1}
    stmia sp, {vXY0, vZG0}
    str vXY1, [sp, #(VERTEX_X + VERTEX_SIZEOF * 1)]
    str vZG1, [sp, #(VERTEX_Z + VERTEX_SIZEOF * 1)]

    // r0 = flags
    mov r1, sp
    adr lr, .next_face

    // gui
    cmp type, #FACE_TYPE_SPRITE
    bne rasterize_asm

    // sprite
    and sprIndex, flags, #0xFF
    add sprite, SPRITES, sprIndex, lsl #4
    ldmia sprite, {sprTile, uwvh}
    str sprTile, [TILE]
    and uv, uwvh, MASK
    str uv, [sp, #(VERTEX_T + VERTEX_SIZEOF * 0)]
    bic uv, uwvh, MASK
    str uv, [sp, #(VERTEX_T + VERTEX_SIZEOF * 1)]
    b rasterize_asm

.next_face:
    cmp face, #0
    bne .loop_list

.next_ot:
    cmp list, OT
    bge .loop_ot

    add sp, #SP_SIZE
    ldmfd sp!, {r4-r11, lr}
    bx lr

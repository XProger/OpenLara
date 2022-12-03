#include "common_asm.inc"

flags        .req r0    // flags must be in r0 for rasterize & draw* calls
ptr          .req r1    // must be in r1
vXY          .req r2
vZG          .req r3
tmp          .req r4
list         .req r5
face         .req r6
VERTICES     .req r7
TEXTURES     .req r8
OT           .req r9
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

vA           .req vXY
vB           .req vZG

Qs           .req ptr
Qe           .req TILE
Ts           .req MASK
Te           .req index01
PN           .req index23
sprites      .req index01

SP_SIZE = (8 * VERTEX_SIZEOF) + 4
SP_SPRITES = SP_SIZE - 4

.extern drawPoly

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

    // fill VertexLink prev & next indices
    sub sp, #SP_SIZE
    add tmp, sp, #VERTEX_PREV
    mov Qs, #255
    add Qs, #4
    mvn Qe, #512
    sub Ts, Qs, #1
    mvn Te, #256
    mvn PN, #65024
    // quad 
    strh Qs, [tmp], #VERTEX_SIZEOF
    strh PN, [tmp], #VERTEX_SIZEOF
    strh PN, [tmp], #VERTEX_SIZEOF
    strh Qe, [tmp], #VERTEX_SIZEOF
    // triangle
    strh Ts, [tmp], #VERTEX_SIZEOF
    strh PN, [tmp], #VERTEX_SIZEOF
    strh Te, [tmp], #VERTEX_SIZEOF

    ldr tmp, =level
    ldr TILE, =gTile
    ldr TEXTURES, [tmp, #LEVEL_TEXTURES]
    ldr SPRITES, [tmp, #LEVEL_SPRITES]
    ldr OT, =gOT
    add list, OT, #((OT_SIZE - 1) << 2)

    mov MASK, #0xFF00
    orr MASK, MASK, MASK, lsl #16

    str SPRITES, [sp, #SP_SPRITES]
.loop_ot:
    ldr face, [list], #-4   // read the first face from the list and decrement
    cmp face, #0
    beq .next_ot            // list is empty, go next

.loop_list:
    ldmia face, {flags, face, index01, index23} // read face params and next face

    and type, flags, #FACE_TYPE_MASK

.draw_primitive: // shadows, triangles, quads and clipped polys
    cmp type, #FACE_TYPE_GTA
    bgt .draw_sprite

    tst flags, #FACE_TRIANGLE
    moveq ptr, sp                           // ptr to quad
    addne ptr, sp, #(VERTEX_SIZEOF * 4)     // ptr to triangle

  .set_vertices:
    // 1st vertex
    mov vertex, index01, lsl #16
    add vertex, VERTICES, vertex, lsr #(16 - 3)
    ldmia vertex, {vXY, vZG}
    stmia ptr, {vXY, vZG}

    // 2nd vertex
    add vertex, VERTICES, index01, lsr #(16 - 3) // assumption: vertex index will never exceed 8191
    ldmia vertex, {vXY, vZG}
    str vXY, [ptr, #(VERTEX_X + VERTEX_SIZEOF * 1)]
    str vZG, [ptr, #(VERTEX_Z + VERTEX_SIZEOF * 1)]

    // 3rd vertex
    mov vertex, index23, lsl #16
    add vertex, VERTICES, vertex, lsr #(16 - 3)
    ldmia vertex, {vXY, vZG}
    str vXY, [ptr, #(VERTEX_X + VERTEX_SIZEOF * 2)]
    str vZG, [ptr, #(VERTEX_Z + VERTEX_SIZEOF * 2)]

    // 4th vertex (quads only)
    addeq vertex, VERTICES, index23, lsr #(16 - 3)
    ldmeqia vertex, {vXY, vZG}
    streq vXY, [ptr, #(VERTEX_X + VERTEX_SIZEOF * 3)]
    streq vZG, [ptr, #(VERTEX_Z + VERTEX_SIZEOF * 3)]

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
    str uv, [ptr, #(VERTEX_T + VERTEX_SIZEOF * 0)]
    and uv, MASK, uv01, lsl #8
    str uv, [ptr, #(VERTEX_T + VERTEX_SIZEOF * 1)]
    and uv, MASK, uv23
    str uv, [ptr, #(VERTEX_T + VERTEX_SIZEOF * 2)]
    and uv, MASK, uv23, lsl #8
    str uv, [ptr, #(VERTEX_T + VERTEX_SIZEOF * 3)]

  .draw:
    // r0 = flags
    // r1 = ptr
    tst face, face
    adrne lr, .loop_list
    adreq lr, .next_ot_zero

    tst flags, #FACE_CLIPPED
    bne drawPoly

    // get top vertex for tri or quad rasterization
    mov tmp, ptr
    ldrsh vA, [tmp, #(VERTEX_Y + VERTEX_SIZEOF * 0)]
    ldrsh vB, [tmp, #(VERTEX_Y + VERTEX_SIZEOF * 1)]
    cmp vA, vB
    addgt ptr, tmp, #(VERTEX_SIZEOF * 1)
    movgt vA, vB
    ldrsh vB, [tmp, #(VERTEX_Y + VERTEX_SIZEOF * 2)]
    cmp vA, vB
    addgt ptr, tmp, #(VERTEX_SIZEOF * 2)
    movgt vA, vB
    lsls vB, flags, #(31 - FACE_TRIANGLE_BIT) // check #FACE_TRIANGLE as sign bit for both pl and gt w/o branch
    ldrplsh vB, [tmp, #(VERTEX_Y + VERTEX_SIZEOF * 3)]
    cmppl vA, vB
    addgt ptr, tmp, #(VERTEX_SIZEOF * 3)
    b rasterize_asm

.draw_sprite: // sprites and gui elements
    mov ptr, sp
    mov vertex, index01, lsl #16
    add vertex, VERTICES, vertex, lsr #(16 - 3)
    ldmia vertex, {vXY0, vZG0, vXY1, vZG1}
    stmia ptr, {vXY0, vZG0}
    str vXY1, [ptr, #(VERTEX_X + VERTEX_SIZEOF * 1)]
    str vZG1, [ptr, #(VERTEX_Z + VERTEX_SIZEOF * 1)]

    // r0 = flags
    // r1 = ptr
    tst face, face
    adrne lr, .loop_list
    adreq lr, .next_ot_zero

    // gui
    cmp type, #FACE_TYPE_SPRITE
    bne rasterize_asm

    // sprite
    and sprIndex, flags, #0xFF
    ldr sprites, [sp, #SP_SPRITES]
    add sprite, sprites, sprIndex, lsl #4
    ldmia sprite, {sprTile, uwvh}
    str sprTile, [TILE]
    and uv, uwvh, MASK
    str uv, [ptr, #(VERTEX_T + VERTEX_SIZEOF * 0)]
    bic uv, uwvh, MASK
    str uv, [ptr, #(VERTEX_T + VERTEX_SIZEOF * 1)]
    b rasterize_asm

.next_ot_zero:
    str face, [list, #4]    // reset the list pointer in OT

.next_ot:
    cmp list, OT
    bge .loop_ot

    add sp, #SP_SIZE
    ldmfd sp!, {r4-r11, lr}
    bx lr

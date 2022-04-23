#include "common_asm.inc"

this      .req r0       // arg
x         .req r1       // arg
z         .req r2       // arg
info      .req r3
roomX     .req r12
roomZ     .req roomX
sx        .req x
sz        .req z
sectors   .req this
sectorsX  .req roomX
sectorsZ  .req roomZ
offset    .req sectorsZ

// const Sector* Room::getSector(int32 x, int32 z) const
.global _ZNK4Room9getSectorEii
_ZNK4Room9getSectorEii:
    ldr info, [this, #4]

    // sx = X_CLAMP((x - (info->x << 8)) >> 10, 0, info->xSectors - 1);
    ldrsh roomX, [info]
    subs sx, x, roomX, lsl #8
    movlt sx, #0
    mov sx, sx, lsr #10
    ldrb sectorsX, [info, #20]
    cmp sx, sectorsX
    subge sx, sectorsX, #1

    // sz = X_CLAMP((z - (info->z << 8)) >> 10, 0, info->zSectors - 1);
    ldrsh roomZ, [info, #2]
    subs sz, z, roomZ, lsl #8
    movlt sz, #0
    mov sz, sz, lsr #10
    ldrb sectorsZ, [info, #21]
    cmp sz, sectorsZ
    subge sz, sectorsZ, #1

    // return sectors + sx * info->zSectors + sz;
    ldr sectors, [this, #8]
    mla offset, sx, sectorsZ, sz
    add sectors, offset, lsl #3 // sizeof(Sector) == (1 << 3)

    bx lr

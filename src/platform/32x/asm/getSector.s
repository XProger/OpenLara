#include "common.i"
SEG_PHYSICS

#define sectors         r0
#define info            r1
#define roomX           r2
#define roomZ           r3
#define this            r4      // arg
#define x               r5      // arg
#define z               r6      // arg
#define mask            r7
#define sectorsX        roomX
#define sectorsZ        roomZ
#define offset          sectorsZ

.text
.align 4
.global __ZNK4Room9getSectorEii
__ZNK4Room9getSectorEii:
        mov.l   @(4, this), info
        mov.w   @info+, roomX
        mov.w   @info+, roomZ

        add     #16, info       // offset to [sectorsX, sectorsZ]

.getX:  // x = MIN(MAX(x - (roomX << 8), 0) >> 10, sectorsX - 1)
        shll8   roomX
        sub     roomX, x
        mov.b   @info+, sectorsX
        cmp/pz  x
        subc    mask, mask
        and     mask, x
        shlr8   x
        shlr2   x
        cmp/ge  sectorsX, x
        bf/s    .getZ
        add     #-1, sectorsX
        mov     sectorsX, x

.getZ:  // z = MIN(MAX(z - (roomZ << 8), 0) >> 10, sectorsZ - 1)
        shll8   roomZ
        sub     roomZ, z
        mov.b   @info+, sectorsZ
        cmp/pz  z
        subc    mask, mask
        and     mask, z
        shlr8   z
        shlr2   z
        cmp/ge  sectorsZ, z
        bf/s    .getPtr
        mov.l   @(8, this), sectors
        mov     sectorsZ, z
        add     #-1, z

.getPtr:
        // offset = sectors + ((x * sectorsZ + z) << 3)
        mulu.w  sectorsZ, x
        sts     MACL, x
        add     x, z
        shll2   z
        shll    z
        rts
        add     z, sectors

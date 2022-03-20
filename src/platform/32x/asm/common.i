#define SEG_MATH    .text
#define SEG_TRANS   .data
#define SEG_FACE    .data
#define SEG_RASTER  .data
#define SEG_PHYSICS .data

// Matrix:
// int16 e00, e01, e02  // rotation
// int16 e10, e11, e12  // rotation
// int16 e20, e21, e22  // rotation
// int16 e03, e13, e23  // translation
//
// word [ N Z Z ]  // rot
//      [ Z N Z ]  // rot
//      [ Z Z N ]  // rot
//      [ Z Z Z ]  // trans
// long [ NZ ZZ NZ ZZ NZ ZZ ]

// row[0] rotation
#define M00     0
#define M01     2
#define M02     4
// row[1] rotation
#define M10     6
#define M11     8
#define M12     10
// row[2] rotation
#define M20     12
#define M21     14
#define M22     16
// row[3] translation
#define M03     18
#define M13     20
#define M23     22

#define MATRIX_SIZEOF   24

#define FIXED_SHIFT     14

#define FACE_TYPE_F     1

#define VERTEX_X        0
#define VERTEX_Y        2
#define VERTEX_Z        4
#define VERTEX_G        6
#define VERTEX_CLIP     7
#define VERTEX_T        8
#define VERTEX_PREV     12
#define VERTEX_NEXT     13

#define VIEW_DIST       (1024 * 10)   // max = DIV_TABLE_END << PROJ_SHIFT
#define FOG_SHIFT       1
#define FOG_MAX         VIEW_DIST
#define FOG_MIN         (FOG_MAX - (8192 >> FOG_SHIFT))
#define VIEW_MIN        (64)
#define VIEW_MAX        (VIEW_DIST)
#define VIEW_OFF        4096

#define CLIP_LEFT       (1 << 0)
#define CLIP_RIGHT      (1 << 1)
#define CLIP_TOP        (1 << 2)
#define CLIP_BOTTOM     (1 << 3)
#define CLIP_FAR        (1 << 4)
#define CLIP_NEAR       (1 << 5)

#define VP_MINX         0
#define VP_MINY         4
#define VP_MAXX         8
#define VP_MAXY         12

.macro shlr14 reg
        shll2   \reg
        shlr16  \reg
        //exts.w  reg, reg      // skip this because of mov.w
.endm
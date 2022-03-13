#define SEG_MATH    .text
#define SEG_TRANS   .data
#define SEG_FACE    .data
#define SEG_RASTER  .data

// row[0]
#define M03     0   // int32
#define M00     4   // int16
#define M01     6   // int16
#define M02     8   // int16
#define M0P     10  // int16 (padding)
// row[1]
#define M13     12  // int32
#define M10     16  // int16
#define M11     18  // int16
#define M12     20  // int16
#define M1P     22  // int16 (padding)
// row[2]
#define M23     24  // int32
#define M20     28  // int16
#define M21     30  // int16
#define M22     32  // int16
#define M2P     34  // int16 (padding)

#define FIXED_SHIFT 14

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

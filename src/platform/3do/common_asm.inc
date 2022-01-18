    IMPORT gMatrixPtr
    IMPORT viewportRel
    IMPORT gVertices
    IMPORT gFacesBase
    IMPORT gOT
    IMPORT gPalette
    IMPORT gShadowQuads
    IMPORT gCameraViewPos
    IMPORT shadeTable
    IMPORT divTable
    IMPORT gSinCosTable
    IMPORT level

CCB_NOBLK       EQU 0x00000010
CCB_BGND        EQU 0x00000020
CCB_ACE         EQU 0x00004000
CCB_ACCW        EQU 0x00020000
CCB_ACW         EQU 0x00040000
CCB_ALSC        EQU 0x00080000
CCB_ACSC        EQU 0x00100000
CCB_YOXY        EQU 0x00200000
CCB_CCBPRE      EQU 0x00400000
CCB_LDPLUT      EQU 0x00800000
CCB_LDPPMP      EQU 0x01000000
CCB_LDPRS       EQU 0x02000000
CCB_LDSIZE      EQU 0x04000000
CCB_PPABS       EQU 0x08000000
CCB_SPABS       EQU 0x10000000
CCB_NPABS       EQU 0x20000000
SIZE_OF_CCB     EQU 60

FRAME_WIDTH     EQU 320
FRAME_HEIGHT    EQU 240

FIXED_SHIFT     EQU 14
F16_SHIFT       EQU (16 - FIXED_SHIFT)
PROJ_SHIFT      EQU 4
LVL_TEX_OFFSET  EQU (23 * 4)

CLIP_SHIFT      EQU 8
CLIP_MASK       EQU ((1 << CLIP_SHIFT) - 1)
CLIP_LEFT       EQU (1 << 0)
CLIP_RIGHT      EQU (1 << 1)
CLIP_TOP        EQU (1 << 2)
CLIP_BOTTOM     EQU (1 << 3)
CLIP_FAR        EQU (1 << 4)
CLIP_NEAR       EQU (1 << 5)

FACE_MIP_SHIFT  EQU 11
MIP_DIST        EQU (1024 * 5)
VIEW_DIST       EQU (1024 * 10)     ; max = DIV_TABLE_END << PROJ_SHIFT
FOG_SHIFT       EQU 1
FOG_MAX         EQU VIEW_DIST
FOG_MIN         EQU (FOG_MAX - (8192 >> FOG_SHIFT))
VIEW_MIN_F      EQU (64 << FIXED_SHIFT)
VIEW_MAX_F      EQU (VIEW_DIST << FIXED_SHIFT)
VIEW_OFF_F      EQU (1024 << FIXED_SHIFT)
OT_SHIFT        EQU 4
OT_SIZE         EQU ((VIEW_MAX_F >> (FIXED_SHIFT + OT_SHIFT)) + 1)

DIV_TABLE_END   EQU (1025 - 1)
VIEW_MIN        EQU (256 << CLIP_SHIFT)
VIEW_MAX        EQU (VIEW_DIST << CLIP_SHIFT)

MIN_INT32       EQU 0x80000000
MAX_INT32       EQU 0x7FFFFFFF

MulManyVec3Mat33_F16 EQU (0x50000 + 2)


; max depth = max(z0, z1, z2, z3) >> (CLIP_SHIFT + OT_SHIFT)
    MACRO
        MAX_Z4 $depth, $z0, $z1, $z2, $z3
        mov $depth, $z0, asr #(CLIP_SHIFT + OT_SHIFT)
        cmp $depth, $z1, asr #(CLIP_SHIFT + OT_SHIFT)
        movlt $depth, $z1, asr #(CLIP_SHIFT + OT_SHIFT)
        cmp $depth, $z2, asr #(CLIP_SHIFT + OT_SHIFT)
        movlt $depth, $z2, asr #(CLIP_SHIFT + OT_SHIFT)
        cmp $depth, $z3, asr #(CLIP_SHIFT + OT_SHIFT)
        movlt $depth, $z3, asr #(CLIP_SHIFT + OT_SHIFT)
    MEND


; max depth = max(z0, z1, z2) >> (CLIP_SHIFT + OT_SHIFT)
    MACRO
        MAX_Z3 $depth, $z0, $z1, $z2, $z3
        mov $depth, $z0, asr #(CLIP_SHIFT + OT_SHIFT)
        cmp $depth, $z1, asr #(CLIP_SHIFT + OT_SHIFT)
        movlt $depth, $z1, asr #(CLIP_SHIFT + OT_SHIFT)
        cmp $depth, $z2, asr #(CLIP_SHIFT + OT_SHIFT)
        movlt $depth, $z2, asr #(CLIP_SHIFT + OT_SHIFT)
    MEND


; average depth = (z0 + z1 + z2 + z2) / 4 >> (CLIP_SHIFT + OT_SHIFT)
    MACRO
        AVG_Z4 $depth, $z0, $z1, $z2, $z3
        add $depth, $z0, $z1
        add $depth, $depth, $z2
        add $depth, $depth, $z3
        mov $depth, $depth, asr #(2 + CLIP_SHIFT + OT_SHIFT)
    MEND


; average depth = (z0 + z1 + z2 + z2) / 4 >> (CLIP_SHIFT + OT_SHIFT)
    MACRO
        AVG_Z3 $depth, $z0, $z1, $z2
        add $depth, $z0, $z1
        add $depth, $depth, $z2, lsl #1
        mov $depth, $depth, asr #(2 + CLIP_SHIFT + OT_SHIFT)
    MEND


; back facing check
    MACRO
        CCW $cross, $dx0, $dy0, $dx1, $dy1, $skip
        mul $cross, $dy0, $dx1
        rsb $cross, cross, #0
        mlas $cross, $dx0, $dy1, $cross
        ble skip
    MEND


; back/front facing check depending on sign
    MACRO
        CCW_SIGN $cross, $sign, $dx0, $dy0, $dx1, $dy1, $skip
        mul $cross, $dy0, $dx1
        rsb $cross, cross, #0
        mla $cross, $dx0, $dy1, $cross
        teq $cross, $sign
        bmi skip
    MEND

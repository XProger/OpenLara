/* OpenLara - SNES / Super FX target
 * The inventory's drawing side (FX3).
 *
 * The ring items' models come from the packer's INV.BIN in CPU ROM: per
 * upstream InvSlot (and one more, the closed passport) the model's stream
 * address and GSU bank, its first animation's frame rate and length.
 *
 * Inventory_publish is upstream's drawPage and drawSlot up to the draw
 * itself: the view (matrixSetView from anglesFromVector, then
 * matrixTranslateAbs), and per item its rotation onto the ring, the push
 * outwards, the quarter turn, and for the chosen one the lift, the pull
 * towards the view and its own spin -- every matrix call upstream makes, in
 * its 1.14 fixed point, on the PPU's multiplier. What goes to the GSU is the
 * finished model-view matrix per item in its own 1.15 format (M_00..M_TZ in
 * mesh.gsu), with the model and its pose; InvRender draws each under it.
 * ItemsExchange (main.asm) copies the list to game pak RAM at GS_INV.
 */

#include "inventory.h"
#include "level.h"
#include "olhost.h"

#ifdef OL_C_MOTION
#define mul(a, b)   ((int32)(a) * (int32)(b))
#else
int32 olMulS16(int a, int b);
#define mul(a, b)   olMulS16((a), (b))
#endif

#define SLOT_PASSPORT_CLOSED    SLOT_MAX    /* INV.BIN's 27th record */
#define INV_GSU_MAX             13
#define INV_GSU_WORDS           16

/* The list for the GSU: a count, then 32 bytes an item -- M_00, M_01, M_02,
   M_TX, M_10 .. M_TZ, the model's address, its bank (relative), its pose
   (animation 0, frame << 8), a spare word. */
uint16 olInvGsu[1 + INV_GSU_MAX * INV_GSU_WORDS];

int32 olInvM[12];           /* the current matrix: 1.14 rotation, translation */
#define M olInvM            /* (inventory_fx3.asm works on it) */
static int32 V[12];         /* the view, pushed */

static const unsigned char* invAt(unsigned int offset)
{
    union { const unsigned char* p; uint16 w[2]; } u;
    u.w[0] = (uint16)(olLevel.invAt + offset);   /* bank-contained */
    u.w[1] = olLevel.invBank;
    return u.p;
}

/* the level carries the slot's model: LEVEL1 has no Lara's Home, which
   upstream's title level does */
int Inventory_hasModel(unsigned int slot)
{
    return olLevel.invSize != 0 && slot <= SLOT_MAX && invAt(slot << 3)[2] != 0xFF;
}

/* getAnimLength: the model's first animation, frameEnd - frameBegin */
int Inventory_modelFrames(unsigned int slot)
{
    if (olLevel.invSize == 0 || slot > SLOT_MAX) return 0;
    return (int)olData16(invAt(slot << 3), 4);
}

/* X_ROTXY on columns a and b of every row: upstream's matrixRotate*. The
   row loops are native on the cartridge (inventory_fx3.asm: 816-tcc's
   int32 helpers made the ring's render sync five fields long); this C is
   the C_MOTION oracle's and the arithmetic's definition. */
#ifndef OL_C_MOTION
void Inv_rot(unsigned int a, unsigned int b, int s, int c);
void Inv_trans(int x, int y, int z);
void Inv_rotY90(void);
void Inv_pack(uint16* out);
#endif

static void rot(int angle, unsigned int a, unsigned int b)
{
    int s, c;
#ifdef OL_C_MOTION
    unsigned int r;
#endif
    if (!angle) return;
    s = (int)phd_sin((int32)angle);
    c = (int)phd_cos((int32)angle);
#ifdef OL_C_MOTION
    for (r = 0; r < 12; r += 4) {
        int x = (int)M[r + a], y = (int)M[r + b];
        M[r + a] = (mul(x, c) - mul(y, s)) >> 14;
        M[r + b] = (mul(y, c) + mul(x, s)) >> 14;
    }
#else
    Inv_rot(a, b, s, c);
#endif
}

#define rotY(a)     rot((a), 0, 2)
#define rotX(a)     rot((a), 2, 1)
#define rotZ(a)     rot((a), 1, 0)

/* matrixTranslateRel */
#ifdef OL_C_MOTION
static void trans(int x, int y, int z)
{
    unsigned int r;
    for (r = 0; r < 12; r += 4)
        M[r + 3] += mul((int)M[r], x) + mul((int)M[r + 1], y) + mul((int)M[r + 2], z);
}
#else
#define trans(x, y, z)  Inv_trans((x), (y), (z))
#endif

/* The quarter turn upstream makes with matrixRotateYXZ(0, ANGLE_90, 0):
   sin 1, cos 0 -- exactly a column swap, a' = -b, b' = a. */
#ifdef OL_C_MOTION
void Inv_rotY90(void)
{
    unsigned int r;
    for (r = 0; r < 12; r += 4) {
        int32 a = M[r];
        M[r] = -M[r + 2];
        M[r + 2] = a;
    }
}

static int toGsu(int32 v)
{
    v <<= 1;                            /* 1.14 -> 1.15 */
    if (v > 32767) v = 32767;
    if (v < -32767) v = -32767;
    return (int)v;
}

/* Executable definition of inventory_fx3.asm's final record packing.
   Keep the 32-bit shift/wrap, saturation, signed low-word translation and
   floor rounding of the first two rows in the same order as drawSlot. */
void Inv_pack(uint16* out)
{
    unsigned int r;
    for (r = 0; r < 12; r++) {
        int v = (r & 3) == 3 ? (int)(M[r] >> 14) : toGsu(M[r]);
        if (r < 8) v = (int)(mul(v, 23) >> 5);
        out[r] = (uint16)v;
    }
}
#endif

void Inventory_publish(void)
{
    int32 sx, cx, sy, cy;
    int ax, ay, h;
    unsigned int i, n, r, drawn = 0;
    uint16* out;

    if (olInvState == INV_STATE_NONE || olLevel.invSize == 0) {
        olInvGsu[0] = 0;
        return;
    }
    if (olInvPage == INV_PAGE_END) {
        olInvGsu[0] = INV_GSU_FRAME;        /* the background and the text, no ring */
        Inventory_text();
        return;
    }

    /* anglesFromVector(0, -(height + INV_CAMERA_Y), -INV_CAMERA_Z) */
    h = olInvHeight + INV_CAMERA_Y;
    ay = (int)(int16)phd_atan(-(int32)INV_CAMERA_Z, 0);
    ax = (int)(int16)phd_atan((int32)INV_CAMERA_Z, (int32)h);

    /* matrixSetView(pos, angleX + pitch, angleY) */
    sx = phd_sin((int32)(int16)(ax + olInvPitch));
    cx = phd_cos((int32)(int16)(ax + olInvPitch));
    sy = phd_sin((int32)ay);
    cy = phd_cos((int32)ay);
    M[0] = cy;                 M[1] = 0;    M[2] = -sy;
    M[4] = mul((int)sx, (int)sy) >> 14;  M[5] = cx;  M[6] = mul((int)sx, (int)cy) >> 14;
    M[8] = mul((int)cx, (int)sy) >> 14;  M[9] = -sx; M[10] = mul((int)cx, (int)cy) >> 14;
    /* matrixTranslateAbs(0, 0, 0): the view's own position, pos = (0,
       height, radius + INV_CAMERA_Z), taken away */
    for (r = 0; r < 12; r += 4)
        M[r + 3] = -(mul((int)M[r + 1], olInvHeight) + mul((int)M[r + 2], olInvRadius + INV_CAMERA_Z));
    for (r = 0; r < 12; r++) V[r] = M[r];

    n = olInvItemsCount;
    if (n > INV_GSU_MAX) n = INV_GSU_MAX;
    out = olInvGsu + 1;
    for (i = 0; i < n; i++)
    {
        unsigned int slot = olInvItemsList[i], model = slot;
        int selected = (i == olInvItemIndex) &&
                       (olInvState == INV_STATE_SHOW || olInvFrameTarget != olInvFrameIndex);
        int frame = 0;
        const unsigned char* rec;

        for (r = 0; r < 12; r++) M[r] = V[r];
        /* matrixRotateY(i * ANGLE_360 / itemsCount - rot - ANGLE_90) */
        rotY((int)(int16)((int32)i * 65536L / (int32)olInvItemsCount - olInvRot - 16384));
        trans(olInvRadius, 0, 0);
        Inv_rotY90();
        if (i == olInvItemIndex) {
            rotX(olInvSelRotPre);
            /* as close as upstream brings it, 256 from the view for the
               passport: FX3's near plane is 64 (mesh.gsu TM_Near/PC_Near,
               on the ring's doubled table) */
            trans(0, 0, olInvSelDist);
            rotY(-olInvSelRotY);            /* matrixRotateYXZ(-selRotX, -selRotY, 0) */
            rotX(-olInvSelRotX);
            rotY(olInvRotItem);
        }

        if (slot == SLOT_PASSPORT && !selected) model = SLOT_PASSPORT_CLOSED;
        rec = invAt(model << 3);
        if (rec[2] == 0xFF) continue;       /* not in this level */
        if (selected) {
            unsigned int rate = rec[3] ? rec[3] : 1;
            frame = olInvFrameIndex / (int)rate;
        }

        /* The GSU projects inventory frames at F 256 (FrameBody); across
           and up by 23/32 makes that upstream's framing, F 204.8 on 240
           columns, on these 216. */
        Inv_pack(out);
        out[12] = olData16(rec, 0);
        out[13] = rec[2];
        out[14] = (uint16)(frame << 8);
        out[15] = olInvVisMask;             /* upstream's item.visibleMask */
        out += INV_GSU_WORDS;
        drawn++;
    }
    olInvGsu[0] = (uint16)drawn;
    Inventory_text();
}

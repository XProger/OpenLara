/* GBA/32X FAST_HITMASK contact: current frame bounds, then an unexpanded
 * creature push. These immutable boxes and renderer portals stay in FX3
 * FastROM while FX runs; only the per-item contact latch is mutable. */
#include "enemy.h"
#include "nav_fx3.h"
#include "collision.h"
#include "lara.h"

uint8 olEnemyTouch[OL_ITEM_MAX];

#ifdef OL_C_MOTION
const AABBi* ItemObj_frameBox(ItemObj* item)
{
    unsigned int anim = item->animIndex, count = olHeader16(OL_HOST_ANIMS);
    unsigned int record = olData16(olAnimBoxData, olMul12(count) + (anim << 1));
    unsigned int offset = olMul12(anim);
    if (record) {
        if (item->frameIndex < olData16(olAnimBoxData, record))
            offset = olData16(olAnimBoxData, record + 2 + ((unsigned int)item->frameIndex << 1));
    }
    return (const AABBi*)(olAnimBoxData + offset);
}

int Room_isAdjacent(unsigned int room, unsigned int other)
{
    const unsigned char *entry, *portal;
    union { const unsigned char* pointer; uint16 words[2]; } address;
    unsigned int count;
    if (room >= olRoomCount || other >= olRoomCount) return 0;
    if (room == other) return 1;
    /* A runtime int32 -> far-pointer cast loses the bank in 816-tcc. The
       room table and each renderer portal list are bank-contained, so bind
       their two words. */
    address.words[0] = 4 + (room << 5);
    address.words[1] = olLevel.levelBank;
    entry = address.pointer;
    count = entry[5];
    address.words[0] = olData16(entry, 2);
    address.words[1] = olLevel.levelBank + entry[4];
    portal = address.pointer;
    while (count--) {
        if (olData16(portal, 0) == other) return 1;
        portal += 32;
    }
    return 0;
}

#endif

/* Room-adjacent deltas retain the port's wrapped world-coordinate seam;
 * products, rotated coordinates and expanded bounds stay full 32-bit as
 * upstream does. Do not narrow each product before subtracting/shifting. */
#ifdef OL_C_MOTION
int32 Frame_rotate(int x, int z, int s, int c)
{
    return ((int32)x * c - (int32)z * s) >> FIXED_SHIFT;
}
#else
int32 Frame_rotate(int x, int z, int s, int c);
#endif
#ifdef OL_C_MOTION
int Frame_inBounds(int32 p, int low, int high, int radius)
{
    if (p < (int32)low - radius) return 0;
    if (p > (int32)high + radius) return 0;
    return 1;
}
#else
int Frame_inBounds(int32 p, int low, int high, int radius);
#endif
/* Inputs fit signed words: world deltas wrap by contract; push-back local
   coordinates have already passed the signed-word box limits. The RESULT
   must stay wide (a diagonal rotation can exceed 32767). */
#define FRAME_RX(x,z,s,c) Frame_rotate((int)(x), (int)(z), (int)(s), (int)(c))
#define FRAME_RZ(x,z,s,c) Frame_rotate((int)(z), (int)(x), (int)-(s), (int)(c))

int ItemObj_collideFrameBounds(ItemObj* item, ItemObj* lara, int radius)
{
    const AABBi* a = ItemObj_frameBox(item);
    const AABBi* b = ItemObj_frameBox(lara);
    int dy = lara->y - item->y;
    int dx, dz;
    int s, c;
    int32 px, pz;
    if ((int32)a->maxY - b->minY <= dy) return 0;
    if ((int32)a->minY - b->maxY >= dy) return 0;
    dx = lara->x - item->x; dz = lara->z - item->z;
    s = (int)phd_sin((int32)item->angleY); c = (int)phd_cos((int32)item->angleY);
    px = FRAME_RX(dx, dz, s, c); pz = FRAME_RZ(dx, dz, s, c);
    if (!Frame_inBounds(px, a->minX, a->maxX, radius)) return 0;
    return Frame_inBounds(pz, a->minZ, a->maxZ, radius);
}

static int Frame_nearest(unsigned int ax, unsigned int bx, unsigned int az, unsigned int bz)
{
    int face = 0;
    /* Stable minimum is the source's tie order, with separate guards for
       816-tcc's miscompiled chains of wide short-circuit comparisons. */
    if (bx < ax) { ax = bx; face = 1; }
    if (az < ax) { ax = az; face = 2; }
    if (bz < ax) face = 3;
    return face;
}

int ItemObj_pushFrame(ItemObj* item, ItemObj* lara)
{
    const AABBi* a = ItemObj_frameBox(item);
    int dx = lara->x - item->x, dz = lara->z - item->z;
    int s = (int)phd_sin((int32)item->angleY), c = (int)phd_cos((int32)item->angleY);
    int32 px = FRAME_RX(dx, dz, s, c), pz = FRAME_RZ(dx, dz, s, c);
    int lx, lz;
    unsigned int ax, bx, az, bz;
    if (!Frame_inBounds(px, a->minX, a->maxX, 0)) return 0;
    if (!Frame_inBounds(pz, a->minZ, a->maxZ, 0)) return 0;
    /* Now inside signed-word limits: coordinates fit int16, and the four
       nonnegative edge distances fit uint16 even for a 65535-unit box. */
    lx = (int)px; lz = (int)pz;
    ax = (unsigned int)lx - a->minX; bx = (unsigned int)a->maxX - lx;
    az = (unsigned int)lz - a->minZ; bz = (unsigned int)a->maxZ - lz;
    switch (Frame_nearest(ax, bx, az, bz)) {
        case 0: lx = a->minX; break;
        case 1: lx = a->maxX; break;
        case 2: lz = a->minZ; break;
        default: lz = a->maxZ; break;
    }
    s = -s;
    lara->x = (int16)(item->x + FRAME_RX(lx, lz, s, c));
    lara->z = (int16)(item->z + FRAME_RZ(lx, lz, s, c));
    return 1;
}

#ifdef OL_C_MOTION
/* Deliberately unfused executable reference: the native path reuses the
   frame box and local coordinates, but must keep these two results. */
int ItemObj_frameContact(ItemObj* item, ItemObj* lara, int radius, int push)
{
    if (!ItemObj_collideFrameBounds(item, lara, radius)) return 0;
    if (push) {
        if (ItemObj_pushFrame(item, lara)) return 3;
    }
    return 1;
}

void Enemy_collideLara(ItemObj* lara)
{
    unsigned int n;
    for (n = 0; n < OL_NAV_SLOTS; n++) {
        unsigned int owner = olNav[n].owner;
        ItemObj* item;
        int dx, dy, dz, angle, contact, radius, x, z;
        if (owner >= itemsCount) continue;
        item = items + owner;
        /* Migrate species individually with contact/attack regressions. */
        if (item->type < 12 || item->type > 14) {
            if (item->type != 19) {
                if (item->type != 7) {
                    if (item->type != 8) {
                        if (item->type != 9) continue;
                    }
                }
            }
        }
        olEnemyTouch[owner] = 0;
        if (lara->health <= 0) continue;
        if (ItemObj_status(item) == ITEM_STATUS_INVISIBLE) continue;
        if (ItemObj_status(item) == ITEM_STATUS_INACTIVE) continue;
        dx = lara->x - item->x; dy = lara->y - item->y; dz = lara->z - item->z;
        if (dx <= -4096 || dx >= 4096) continue;
        if (dy <= -4096 || dy >= 4096) continue;
        if (dz <= -4096 || dz >= 4096) continue;
        if (!Room_isAdjacent(lara->room, item->room)) continue;
        x = lara->x; z = lara->z;
        contact = ItemObj_frameContact(item, lara, cinfo.radius, cinfo.enemyPush);
        if (!contact) continue;
        olEnemyTouch[owner] = 1; /* pre-push hitMask, even when the push rejects */
        if (!(contact & 2)) continue;
        Lara_enemyHit(item, x, z);
        /* Source collidePush's room guard: reject a push through a wall,
           otherwise retain the accepted position for the next collision.
           Enemy::collide keeps radius zero through ALL of collidePush,
           including these room probes, then restores the caller's radius. */
        radius = cinfo.radius;
        cinfo.radius = 0;
        angle = cinfo.angle;
        cinfo.gapPos = -WALL; cinfo.gapNeg = -LARA_STEP_HEIGHT; cinfo.gapCeiling = 0;
        dx = lara->x - cinfo.posX; dz = lara->z - cinfo.posZ;
        CollisionInfo_setAngle(&cinfo, (uint16)phd_atan((int32)dz, (int32)dx));
        ItemObj_collideRoom(item, &cinfo, LARA_HEIGHT, 0);
        CollisionInfo_setAngle(&cinfo, (uint16)angle);
        if (cinfo.type != CT_NONE) {
            lara->x = (int16)cinfo.posX; lara->z = (int16)cinfo.posZ;
        } else {
            cinfo.posX = lara->x; cinfo.posY = lara->y; cinfo.posZ = lara->z;
            ItemObj_updateRoom(lara, -10);
        }
        cinfo.radius = radius;
    }
}
#endif

/* ---------------------------------------------------------------------------
 * Lara's hit reactions: ItemObj::animHit, the tail of Lara::updateCollision
 * and the override in Lara::draw.
 *
 * collidePush(lara, cinfo, true) -- Enemy::collide's -- takes the offset she
 * had BEFORE the push, less the creature's box centre turned back into the
 * world, and names the quadrant she was struck from; a box 256 or less tall
 * (a bat) does not. Only this variant pushes her with creatures, so all of it
 * lives here. The quadrant is kept plus one, so the zeroed .bss is "none".
 * ------------------------------------------------------------------------ */
#include "sound.h"
#include "level.h"

uint8 olLaraHitQuadrant;    /* upstream's hitQuadrant + 1; 0 is -1 */
uint8 olLaraHitTimer;
uint8 olLaraHitFrame;
/* A push records what animHit needs -- her offset before it, the box centre,
   the creature's yaw and hers -- and marks the quadrant pending; it is worked
   out when her pose is published. The last push of a tick wins either way,
   and a push can come three times a tick where a publish comes five times a
   second: the rotation and phd_atan at every push cost the three-creature
   gate a tick. */
int16 olLaraHitIn[6];

#define LARA_ANIM_HIT_FRONT 125
#define LARA_ANIM_HIT_BACK  126
#define LARA_ANIM_HIT_LEFT  127
#define LARA_ANIM_HIT_RIGHT 128
#define LARA_HIT_FRAME_MAX  34

/* x/z is where she stood before the push -- inside the creature's box, so
   the offsets are box-sized and phd_atan's signed-word range holds them.
   Both are native in contact_fx3.asm; these are the C_MOTION references. */
#ifdef OL_C_MOTION
void Lara_enemyHit(ItemObj* item, int x, int z)
{
    const AABBi* a;

    if (!cinfo.enemyHit) return;
    a = ItemObj_frameBox(item);
    if ((int32)a->maxY - a->minY <= 256) return;

    olLaraHitIn[0] = (int16)(x - item->x);
    olLaraHitIn[1] = (int16)(z - item->z);
    olLaraHitIn[2] = (int16)(((int32)a->minX + a->maxX) >> 1);
    olLaraHitIn[3] = (int16)(((int32)a->minZ + a->maxZ) >> 1);
    olLaraHitIn[4] = (int16)item->angleY;
    olLaraHitIn[5] = (int16)items[0].angleY;
    olLaraHitQuadrant = OL_HIT_PENDING;
    olLaraHitTimer = 5;
}

void Lara_hitResolve(void)
{
    int s, c;
    int32 dx, dz;
    uint16 q;

    if (olLaraHitQuadrant != OL_HIT_PENDING) return;
    s = -(int)phd_sin((int32)(uint16)olLaraHitIn[4]);
    c = (int)phd_cos((int32)(uint16)olLaraHitIn[4]);
    dx = (int32)olLaraHitIn[0] - FRAME_RX(olLaraHitIn[2], olLaraHitIn[3], s, c);
    dz = (int32)olLaraHitIn[1] - FRAME_RZ(olLaraHitIn[2], olLaraHitIn[3], s, c);
    q = (uint16)((uint16)olLaraHitIn[5] - (uint16)phd_atan(dz, dx) + ANGLE_180 + ANGLE_45);
    olLaraHitQuadrant = (uint8)((q >> 14) + 1);
}
#endif

/* After her state's collision handler, on a tick with the timer running
   (lara.c tests it). Native in contact_fx3.asm. */
#ifdef OL_C_MOTION
void Lara_hitTick(void)
{
    if (items[0].health <= 0) olLaraHitQuadrant = 0;   /* updateObjectsCollision */

    if (olLaraHitTimer == 0) return;
    if (olLaraHitFrame == 0) Sound_playItem(SND_HIT, items);
    if (olLaraHitFrame < LARA_HIT_FRAME_MAX) olLaraHitFrame++;
    if (--olLaraHitTimer == 0) {
        olLaraHitQuadrant = 0;
        olLaraHitFrame = 0;
    }
}
#endif

/* Lara::draw: the renderer is told the hit animation at hitFrame instead of
   her own, after gameRenderSync has published her. Her own state is not
   touched, as upstream restores it after drawModel. */
extern unsigned char ol_itemState_gsu[];
extern unsigned int ol_animFreeze;

void olLaraHitPublish(void)
{
    unsigned int anim;

    if (olLaraHitQuadrant == 0 || itemsCount == 0 || ol_animFreeze) return;
    Lara_hitResolve();
    switch (olLaraHitQuadrant) {
        case 1:  anim = LARA_ANIM_HIT_FRONT; break;
        case 2:  anim = LARA_ANIM_HIT_LEFT;  break;
        case 3:  anim = LARA_ANIM_HIT_BACK;  break;
        default: anim = LARA_ANIM_HIT_RIGHT; break;
    }
    ol_itemState_gsu[0] = (unsigned char)anim;
    ol_itemState_gsu[1] = (unsigned char)(olLaraHitFrame /
        olAnimFrameRate(olModelFirstAnim(items[0].model) + anim));
}

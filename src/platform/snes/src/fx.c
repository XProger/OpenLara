/* OpenLara - SNES / Super FX target
 * Upstream's effects, FX3: src/fixed/item.h's fxBlood, fxRicochet, fxSmoke,
 * fxSplash and fxBubbles, and object.h's SpriteEffect and Bubble.
 *
 * Upstream takes an effect from ItemObj's free list (ItemObj::add) and puts
 * it back when its sprite sequence has played (remove). Every entity here
 * exists from the load -- the renderer's slot table and the host's list are
 * the packer's -- so the packer appends a pool of hidden ones (appendFxPool,
 * FX.BIN's poolFirst and poolCount) and Fx_add takes the first of them not
 * in use, retyped. All five types draw with the one effects model: its
 * animation per kind, its frame the sprite (packer buildFxModel, the GSU's
 * BillboardNode), so frameIndex is upstream's with a frame rate of one.
 *
 * getJoint, which the creatures' bite and her bubbles ask, is the packer's:
 * FX.BIN's points are upstream's int32 matrix chain from the identity on
 * the frame getFrame picks for each game tick, in sixteenths; Fx_joint turns
 * them by the item's angles on this side's sine table, which the packer
 * measured to within five units of upstream's answer.
 *
 * Kept on the record, where upstream has fields of its own: SpriteEffect's
 * tick in `state`, ANIMATED in flags2 (the creatures' WILD bit, which an
 * effect never is), and the bubble's angle.x and angle.z in `trigger` (read
 * by nothing after the load) and `timer`.
 */

#include "fx.h"
#include "room.h"
#include "lara.h"
#include "level.h"
#include "sound.h"

#define SND_RICOCHET    10
#define SND_BUBBLE      37

unsigned int olFxFirst, olFxCount;
unsigned int olFxLive, olFxPending;         /* render_sync.asm's */
unsigned int olSyncEnd;                     /* the records gameRenderSync wrote */
static const uint8 fxType[5] = { 153, 155, 158, 160, 164 };

static const unsigned char* fxAt(unsigned int offset)
{
    union { const unsigned char* p; uint16 w[2]; } u;
    u.w[0] = (uint16)(olLevel.fxAt + offset);      /* bank-contained */
    u.w[1] = olLevel.fxBank;
    return u.p;
}

void Fx_levelInit(void)
{
    unsigned int i;
    olFxFirst = OL_NO_ACTIVE;               /* past every slot: no pool */
    olFxCount = olFxLive = 0;
    olFxPending = 1;                        /* publish the pool once */
    if (olLevel.fxSize == 0) return;
    olFxFirst = olData16(fxAt(0), 0);
    olFxCount = olData16(fxAt(0), 2);
    for (i = 0; i < olFxCount; i++)         /* a loaded game's, in flight */
        if (ItemObj_status(items + olFxFirst + i) != ITEM_STATUS_INVISIBLE) olFxLive++;
}

/* ItemObj::remove */
void Fx_kill(ItemObj* fx);
static void Fx_remove(ItemObj* fx)
{
    ItemObj_deactivate(fx);
    ItemObj_setStatus(fx, ITEM_STATUS_INVISIBLE);
    ItemObj_touch(fx);
    if (olFxLive) olFxLive--;
    olFxPending = 1;                        /* its hidden record, once */
}
/* render_sync.asm's FX_Sprite: the removal, and a move past its range */
void Fx_kill(ItemObj* fx) { Fx_remove(fx); }
void Fx_spriteMove(ItemObj* fx);

ItemObj* Fx_add(unsigned int kind, unsigned int room, int x, int y, int z,
                unsigned int angleY)
{
    unsigned int i, n;
    if (olFxCount == 0) return 0;
    if (olData16(fxAt(4), kind << 1) == 0) return 0;   /* no such sprite here */
    for (i = olFxFirst, n = olFxCount; n; i++, n--) {
        ItemObj* fx = items + i;
        if (ItemObj_status(fx) != ITEM_STATUS_INVISIBLE) continue;
        fx->type   = fxType[kind];
        fx->room   = (uint8)room;
        fx->x      = (int16)x;
        fx->y      = (int16)y;
        fx->z      = (int16)z;
        fx->angleY = (uint16)angleY;
        fx->animIndex  = (uint16)(olModelFirstAnim(fx->model) + kind);
        fx->frameIndex = 0;
        fx->state  = 0;                     /* SpriteEffect's tick */
        fx->timer  = 0;
        fx->hSpeed = 0;
        fx->vSpeed = 0;
        fx->health = 0;                     /* never a weapon's target */
        fx->flags2 &= (uint8)~(ITEM_FLAG_ANIMATED | ITEM_FLAG_GRAVITY);
        ItemObj_setStatus(fx, ITEM_STATUS_ACTIVE);
        ItemObj_activate(fx);
        ItemObj_touch(fx);
        olFxLive++;
        return fx;
    }
    return 0;                               /* upstream's ASSERT and NULL */
}

/* SpriteEffect::update */
static void SpriteEffect_update(ItemObj* fx)
{
    fx->state++;
    if ((int)fx->state >= fx->timer) {
        fx->state = 0;
        if (!(fx->flags2 & ITEM_FLAG_ANIMATED)) {
            Fx_remove(fx);
            return;
        }
        fx->frameIndex++;
        if (fx->frameIndex >= olAnimFrames(fx->animIndex)) {
            Fx_remove(fx);
            return;
        }
    }
    if (fx->hSpeed) Fx_spriteMove(fx);
}

void Fx_spriteMove(ItemObj* fx)
{
    int32 s = phd_sin((int32)fx->angleY);
    int32 c = phd_cos((int32)fx->angleY);
    fx->x = (int16)(fx->x + (int)((s * fx->hSpeed) >> FIXED_SHIFT));
    fx->z = (int16)(fx->z + (int)((c * fx->hSpeed) >> FIXED_SHIFT));
}

/* Bubble::update. Upstream's has no way out but the surface; one born where
   no water is over it (WALL) would rise for ever and keep its pool slot, so
   it goes at once instead. */
static void Bubble_update(ItemObj* fx)
{
    RoomQuery q;
    fx->y = (int16)(fx->y + fx->vSpeed);
    if (fx->roomFloor == (int16)WALL || fx->roomFloor > fx->y) {
        Fx_remove(fx);
        return;
    }
    fx->trigger = (uint16)(fx->trigger + ANGLE_1 * 9);             /* angle.x */
    fx->timer = (int16)(uint16)((uint16)fx->timer + ANGLE_1 * 13);  /* angle.z */
    fx->x = (int16)(fx->x + (int)((phd_sin((int32)fx->trigger) * 11) >> FIXED_SHIFT));
    fx->z = (int16)(fx->z + (int)((phd_sin((int32)(uint16)fx->timer) * 8) >> FIXED_SHIFT));
    if (Room_query(fx->room, fx->x, fx->y, fx->z, &q)) fx->room = (uint8)q.room;
}

void Fx_update(ItemObj* item)
{
    if (item->type == 155) Bubble_update(item);
    else SpriteEffect_update(item);
}

/* FX.BIN's point for the item's animation and tick at this site: its offset
   in the file, or zero */
static unsigned int fxPoint(ItemObj* item, unsigned int site)
{
    const unsigned char* rec = fxAt(16 + site * 6);
    unsigned int a = item->animIndex - olData16(rec, 0);
    unsigned int map, tick;
    if (a >= olData16(rec, 2) || olData16(rec, 4) == 0) return 0;
    map = olData16(fxAt(olData16(rec, 4)), a << 1);
    if (map == 0) return 0;
    tick = item->frameIndex;
    if (tick >= olData16(fxAt(map), 0)) tick = 0;
    return olData16(fxAt(map), 2 + (tick << 1));
}

/* X_ROTXY(a, b): the pair turned as a matrix rotation turns a point */
static void fxTurn(int32* a, int32* b, unsigned int angle)
{
    int32 s = phd_sin((int32)angle), c = phd_cos((int32)angle);
    int32 t = (c * *a - s * *b) >> FIXED_SHIFT;
    *b = (s * *a + c * *b) >> FIXED_SHIFT;
    *a = t;
}

int Fx_joint(ItemObj* item, unsigned int site, int* x, int* y, int* z)
{
    unsigned int p;
    int32 px, py, pz;

    if (olFxCount == 0) return 0;
    p = fxPoint(item, site);
    if (p == 0) return 0;
    px = olDataS16(fxAt(p), 0);
    py = olDataS16(fxAt(p), 2);
    pz = olDataS16(fxAt(p), 4);
    /* matrixRotateYXZ(angle.x, angle.y, angle.z) on the point: Z, then X,
       then Y; only she is pitched or leans */
    if (item == items) {
        if (olLaraAngleZ) fxTurn(&px, &py, (unsigned int)olLaraAngleZ);
        if (olLaraAngleX) fxTurn(&py, &pz, (unsigned int)olLaraAngleX);
    }
    fxTurn(&pz, &px, item->angleY);
    *x = (int)(px >> 4);
    *y = (int)(py >> 4);
    *z = (int)(pz >> 4);
    return 1;
}

/* fxBlood(fxPos, fxAngleY, fxSpeed), in the room of whoever bleeds it */
void Fx_blood(unsigned int room, int x, int y, int z, unsigned int angleY, int speed)
{
    ItemObj* fx = Fx_add(FX_BLOOD, room, x, y, z, angleY);
    if (!fx) return;
    fx->hSpeed = (int16)speed;
    fx->timer = 4;
    fx->flags2 |= ITEM_FLAG_ANIMATED;
}

void Fx_ricochet(unsigned int room, int x, int y, int z, int sound)
{
    ItemObj* fx = Fx_add(FX_RICOCHET, room, x, y, z, 0);
    if (!fx) return;
    fx->timer = 4;
    fx->frameIndex = (uint8)((unsigned int)rand_draw() % olAnimFrames(fx->animIndex));
    if (sound) Sound_playItem(SND_RICOCHET, fx);
}

void Fx_smoke(unsigned int room, int x, int y, int z)
{
    ItemObj* fx = Fx_add(FX_SMOKE, room, x, y, z, 0);
    if (!fx) return;
    fx->timer = 3;
    fx->flags2 |= ITEM_FLAG_ANIMATED;
}

/* fxSplash: ten, from the surface over her, outward at random */
void Fx_splash(ItemObj* item)
{
    int level, depth;
    unsigned int i;
    Room_water(item->room, item->x, item->z, &level, &depth);
    for (i = 0; i < 10; i++) {
        ItemObj* fx = Fx_add(FX_SPLASH, item->room, item->x, level, item->z,
                             (unsigned int)(uint16)((rand_draw() - ANGLE_90) << 1));
        if (!fx) return;
        fx->hSpeed = (int16)(rand_draw() >> 8);
        fx->flags2 |= ITEM_FLAG_ANIMATED;
    }
}

/* fxBubbles(room, JOINT_HEAD, (0, 0, 50)): none, one or two, each with the
   Bubble constructor's sound, frame, rise and surface */
void Fx_bubbles(ItemObj* item)
{
    int x, y, z, level, depth;
    unsigned int count = (unsigned int)rand_draw() % 3;
    if (!count) return;
    if (!Fx_joint(item, FX_SITE_HEAD, &x, &y, &z)) return;
    x += item->x; y += item->y; z += item->z;
    while (count--) {
        ItemObj* fx = Fx_add(FX_BUBBLE, item->room, x, y, z, 0);
        if (!fx) return;
        Sound_playItem(SND_BUBBLE, fx);
        fx->frameIndex = (uint8)((unsigned int)rand_draw() % olAnimFrames(fx->animIndex));
        fx->vSpeed = (int16)-(10 + (int)((unsigned int)rand_draw() % 6));
        fx->trigger = 0;                    /* angle (0, 0, 90) */
        fx->timer = (int16)ANGLE_90;
        fx->angleY = 0;
        Room_water(fx->room, x, z, &level, &depth);
        fx->roomFloor = (int16)level;
    }
}

void Fx_bite(ItemObj* item, unsigned int site)
{
    int x, y, z;
    if (!Fx_joint(item, site, &x, &y, &z)) return;
    Fx_blood(item->room, item->x + x, item->y + y, item->z + z, 0, 0);
}

/* OpenLara - SNES / Super FX target
 * The port of src/fixed/item.h's ItemObj methods.
 *
 * Read this next to upstream. The control flow is theirs, function for
 * function, and where it differs the comment says why -- which is nearly
 * always that the level lives in game pak RAM here rather than behind a
 * pointer, or that a 32-bit operation is a library call on this compiler and
 * the value fits sixteen bits anyway.
 */

#include "item.h"
#include "sound.h"
#include "level.h"
#include "room.h"
#include "lara.h"
#ifdef OL_FX3
#include "enemy.h"
#ifdef OL_FX3
#include "fx.h"
#endif
#endif

ItemObj      items[OL_ITEM_MAX];
unsigned int itemsCount;
uint8 olEnemyNext[OL_ITEM_MAX], olEnemyTurn[OL_ITEM_MAX];

/* ---------------------------------------------------------------------------
 * const Anim* ItemObj::animChange(const Anim* anim)
 *
 * Upstream returns the animation it moved to and takes the current one as an
 * argument; here the item carries its own animation index and the function
 * changes it in place, because there is no Anim* to return -- an animation is
 * an index into a table in game pak RAM.
 *
 * The two nested loops are one here. TR keeps the states an animation can
 * reach and the frame ranges each is reachable from as separate tables; the
 * packer flattens them into one list per animation, in TR's order, so the
 * first match still wins.
 * ------------------------------------------------------------------------ */
#ifdef OL_C_MOTION
void ItemObj_animChange(ItemObj* item)
{
    unsigned int a = item->animIndex;
    unsigned int n = olAnimTransCount(a);
    unsigned int t;

    if (item->goalState == item->state || n == 0)
        return;

    t = olAnimTransStart(a);

    while (n--)
    {
        if (olTransGoal(t) == item->goalState &&
            item->frameIndex >= olTransBegin(t) &&
            item->frameIndex <= olTransEnd(t))
        {
            item->frameIndex = (uint8)olTransFrame(t);
            item->animIndex = olTransAnim(t);
            item->state = (uint8)olAnimState(item->animIndex);
            return;
        }
        t++;
    }
}
#endif

/* ---------------------------------------------------------------------------
 * void ItemObj::animCmd(bool fx, const Anim* anim)
 *
 * Upstream walks a variable-length stream and has to know each command's
 * argument count to step past it; the packer unpacks them into fixed records,
 * so this walks an array. The split is upstream's: the commands that change
 * where an item *is* run with fx false, at the end of an animation, and the
 * ones tied to a frame run with fx true.
 * ------------------------------------------------------------------------ */
#define ANIM_CMD_OFFSET     1
#define ANIM_CMD_JUMP       2
#define ANIM_CMD_EMPTY      3
#define ANIM_CMD_KILL       4
#define ANIM_CMD_SOUND      5
#define ANIM_CMD_EFFECT     6

#define FX_ROTATE_180       0
#define FX_LARA_NORMAL      2
#define FX_LARA_BUBBLES     3
#define FX_LARA_HANDSFREE   12

/* FX3 dispatches per-frame effects natively. The end-of-animation path
   keeps this complete body and ABI; C_MOTION also uses it for fx != 0. */
#if defined(OL_FX3) && !defined(OL_C_MOTION)
void ItemObj_animCmdEnd(ItemObj* item, int fx)
#else
void ItemObj_animCmd(ItemObj* item, int fx)
#endif
{
    unsigned int c = olAnimCmdStart(item->animIndex);
    unsigned int n = olAnimCmdCount(item->animIndex);

    while (n--)
    {
        switch (olCmdOp(c))
        {
            case ANIM_CMD_OFFSET:
            {
                if (!fx)
                {
                    /* X_ROTX and X_ROTY, which is upstream: the offset is in
                       the item's own frame, so a door slides the way it faces
                       rather than along +X. */
                    int32 s = phd_sin((int32)item->angleY);
                    int32 c2 = phd_cos((int32)item->angleY);
                    int x = olCmdA0(c), z = olCmdA2(c);

                    item->x += (int)(((int32)x * c2 + (int32)z * s)
                                     >> FIXED_SHIFT);
                    item->y += olCmdA1(c);
                    item->z += (int)(((int32)z * c2 - (int32)x * s)
                                     >> FIXED_SHIFT);
                }
                break;
            }

            /* Upstream's: the status goes INACTIVE and the class decides
               what that means -- a creature's death pose stops (FX3 keeps
               its corpse countdown active until GBA's hide deadline), a
               switch becomes usable, a falling floor lies still. (It used
               to clear ITEM_FLAG_ACTIVE here without unlinking, which left
               the entity on the list with the flag that guards a second
               activate() clear: a re-trigger would have made a cycle.) */
            case ANIM_CMD_KILL:
            {
                if (!fx) {
                    ItemObj_setStatus(item, ITEM_STATUS_INACTIVE);
                    ItemObj_touch(item);
                }
                break;
            }

            /* EMPTY is Lara's hands: the end of a pull, a pick-up, a key
               turned, and her arms are free again. */
            case ANIM_CMD_EMPTY:
            {
                if (!fx && item == items) {
                    olLaraWeaponState = WEAPON_STATE_FREE;
                }
                break;
            }

            /* Upstream's fx switch, as far as this side has effects: the
               half turn a roll ends with; her hands freed, which is how
               TR1 ends a pull, a pick-up and a key turned (the EMPTY
               command above is the other way); and back to standing. */
            case ANIM_CMD_EFFECT:
            {
                if (fx && (unsigned int)olCmdA0(c) == item->frameIndex)
                {
                    int e = olCmdA1(c);
                    if (e == FX_ROTATE_180) {
                        item->angleY += ANGLE_180;
                    } else if (e == FX_LARA_HANDSFREE && item == items) {
                        olLaraWeaponState = WEAPON_STATE_FREE;
                    } else if (e == FX_LARA_NORMAL && item == items) {
                        ItemObj_animSet(item, olModelFirstAnim(item->model)
                                              + LARA_ANIM_STAND);
                    }
#ifdef OL_FX3
                    else if (e == FX_LARA_BUBBLES) {
                        Fx_bubbles(item);
                    }
#endif
                }
                break;
            }

            /* This is how TR launches her. The push comes out of the
               animation, the same as her running speed does -- nothing in
               this port says how high she jumps. vSpeed is a speed and takes
               the frame conversion; hSpeed stays per tick, because move()
               scales it when it displaces. Upstream's `vSpeedHack` is the
               one exception: the speed c_checkClimbUp works out for a jump
               up at a ledge, spent here in place of the animation's. */
            case ANIM_CMD_JUMP:
            {
                if (!fx) {
                    if (item == items && olLaraVSpeedHack) {
                        item->vSpeed = (int16)(-olLaraVSpeedHack);
                        olLaraVSpeedHack = 0;
                    } else {
                        item->vSpeed = (int16)OL_SPEED(olCmdA0(c));
                    }
                    item->hSpeed = (int16)olCmdA1(c);
                    item->flags2 |= ITEM_FLAG_GRAVITY;
                }
                break;
            }

            /* The frame's sound, at the item: footsteps, the pull of a
               lever, a wolf's snarl. Bit 14 and up of the argument are
               TR's environment flags (land/water), which upstream masks off. */
            case ANIM_CMD_SOUND:
            {
                if (fx && (unsigned int)olCmdA0(c) == item->frameIndex) {
                    Sound_playItem((unsigned int)olCmdA1(c) & 0x3FFF, item);
                }
                break;
            }

            default:
                break;
        }
        c++;
    }
}

/* ---------------------------------------------------------------------------
 * void ItemObj::animSet(int32 animIndex, bool resetState)
 *
 * Trimmed to the one call upstream makes of it here: dying. There is no
 * transition from standing to dead, which is why Enemy::hit sets the animation
 * outright instead of asking for a state.
 * ------------------------------------------------------------------------ */
void ItemObj_animSet(ItemObj* item, unsigned int anim)
{
    ItemObj_animSetFrame(item, anim, 0);
}

void ItemObj_animSetFrame(ItemObj* item, unsigned int anim,
                          unsigned int frame)
{
    ItemObj_animSetState(item, anim, frame, 1);
}

void ItemObj_animSetState(ItemObj* item, unsigned int anim,
                          unsigned int frame, int resetState)
{
    unsigned int frames;

    if (anim >= OL_ANIM_MAX) return;
    frames = olAnimFrames(anim);
    if (frames == 0) return;

    item->animIndex = (uint16)anim;
    if (frame >= frames) frame = frames - 1;
    item->frameIndex = (uint8)frame;
    if (resetState) {
        item->state = (uint8)olAnimState(anim);
        item->goalState = item->state;
    }
}

/* ---------------------------------------------------------------------------
 * void ItemObj::animProcess(bool movement)
 *
 * Upstream's order exactly: advance the frame, then ask whether a transition
 * applies, then check for the end of the animation. The order is why TR's
 * transition ranges legally end one frame past their animation -- the index
 * being tested is one beyond the frame just drawn.
 * ------------------------------------------------------------------------ */
#ifdef OL_C_MOTION
void ItemObj_animProcess(ItemObj* item)
{
    item->frameIndex++;

    ItemObj_animChange(item);

    if (item->type != ITEM_LARA && olEnemyNext[item - items] == item->state)
        olEnemyNext[item - items] = 0;

    if (item->frameIndex >= olAnimFrames(item->animIndex))
    {
        ItemObj_animCmd(item, 0);

        item->frameIndex = (uint8)olAnimNextFrame(item->animIndex);
        item->animIndex = (uint16)olAnimNext(item->animIndex);
        item->state = (uint8)olAnimState(item->animIndex);

        /* Upstream: everything but Lara has its goal follow its state at the
           end of an animation. Lara must retain the requested state across
           animation links until a transition can grant it; resetting it here
           can swallow jumps when rendering makes logic updates sparse. */
        if (item->type != ITEM_LARA) {
            item->goalState = item->state;
            if (olEnemyNext[item - items] == item->state)
                olEnemyNext[item - items] = 0;
        }

        if (item->frameIndex >= olAnimFrames(item->animIndex))
            item->frameIndex = 0;
    }

    if (olAnimCmdCount(item->animIndex))
        ItemObj_animCmd(item, 1);
}
#endif

/* ---------------------------------------------------------------------------
 * bool Object::isActive()
 *
 * Upstream's, including the part that reads oddly: a timer of zero means "no
 * timer" and runs for ever, a positive one counts down, and -1 has expired.
 * The countdown is per fixed 30 Hz TR tick.
 * ------------------------------------------------------------------------ */

int ItemObj_isActive(ItemObj* item)
{
    /* Upstream's test is the *mask*, not ITEM_FLAG_ACTIVE: the flag means
       "on the update list" and the mask means "every trigger wired to this
       has fired". They coincide for a door on one pad and part company for a
       door on two, and for anything an antipad clears -- which upstream
       switches off by clearing mask bits and never touches the flag.

       REVERSE is what the level's own "active at start" becomes: the mask
       was full when the level loaded, ItemObj::init cleared it and set the
       bit, and from then on an empty mask means *on* and a full one off. */
    int rev = (item->flags2 & ITEM_FLAG_REVERSE) != 0;

    if ((item->flags & ITEM_FLAG_MASK) != ITEM_FLAG_MASK) return rev;
    if (item->timer == OL_TIMER_EXPIRED) return rev;
    if (item->timer == 0) return !rev;

    item->timer -= OL_FRAME_TICKS;
    if (item->timer <= 0) item->timer = OL_TIMER_EXPIRED;
    return !rev;
}

int ItemObj_animIsEnd(ItemObj* item, unsigned int offset)
{
    /* Upstream compares against the animation's last frame; this side's
       frame index is local to the animation, so the last is count - 1. */
    return (unsigned int)item->frameIndex + 1 + offset
           == olAnimFrames(item->animIndex);
}

/* ---------------------------------------------------------------------------
 * void ItemObj::animSkip(int32 stateBefore, int32 stateAfter, bool advance)
 *
 * Upstream's. The graph is walked to the state wanted -- frames go by, the
 * position does not -- then the state after it is asked for, so the
 * interaction plays out and returns to standing on its own. The walk is
 * bounded here: upstream trusts the graph, and a graph that cannot reach the
 * state would hang a machine with no debugger.
 * ------------------------------------------------------------------------ */
void ItemObj_animSkip(ItemObj* item, unsigned int before, unsigned int after,
                      int advance)
{
    int x = item->x, y = item->y, z = item->z;
    int guard = 64;

    item->goalState = (uint8)before;

    while (item->state != before) {
        if (--guard < 0) break;
        ItemObj_animProcess(item);
    }

    if (advance) ItemObj_animProcess(item);

    item->x = (int16)x;
    item->y = (int16)y;
    item->z = (int16)z;
    item->vSpeed = 0;
    item->hSpeed = 0;
    item->goalState = (uint8)after;
}

/* ---------------------------------------------------------------------------
 * bool ItemObj::moveTo(const vec3i &point, ItemObj* item, bool lerp)
 *
 * Where she has to stand to use something, in the thing's own frame: a
 * hundred units short of a pickup, 362 in front of a keyhole. Upstream's
 * getRelative is the item's rotation applied to the point, which for a
 * thing that only turns about Y is the same rotation the OFFSET command
 * uses above. The lerp arm is the underwater one -- she swims onto the
 * spot at sixteen units a tick along the longest axis rather than
 * upstream's normalised sixteen, which is one divide instead of a square
 * root and a reciprocal, and turns toward it two degrees a tick.
 * ------------------------------------------------------------------------ */
static int angleStep(int a, int b, int w)
{
    int d = (int)(int16)(uint16)(b - a);
    if (d > w)  return a + w;
    if (d < -w) return a - w;
    return b;
}

int ItemObj_moveTo(ItemObj* mover, int px, int py, int pz, ItemObj* item,
                   int lerp)
{
    int32 s = phd_sin((int32)item->angleY);
    int32 c = phd_cos((int32)item->angleY);
    int x = item->x + (int)(((int32)px * c + (int32)pz * s) >> FIXED_SHIFT);
    int y = item->y + py;
    int z = item->z + (int)(((int32)pz * c - (int32)px * s) >> FIXED_SHIFT);
    int dx, dy, dz, d, ax, ay, az;

    if (!lerp) {
        mover->x = (int16)x;
        mover->y = (int16)y;
        mover->z = (int16)z;
        mover->angleY = item->angleY;
        return 1;
    }

    dx = x - mover->x;
    dy = y - mover->y;
    dz = z - mover->z;
    ax = (dx < 0) ? -dx : dx;
    ay = (dy < 0) ? -dy : dy;
    az = (dz < 0) ? -dz : dz;
    d = ax;
    if (ay > d) d = ay;
    if (az > d) d = az;

    if (d > 16) {
        mover->x = (int16)(mover->x + (dx << 4) / d);
        mover->y = (int16)(mover->y + (dy << 4) / d);
        mover->z = (int16)(mover->z + (dz << 4) / d);
    } else {
        mover->x = (int16)x;
        mover->y = (int16)y;
        mover->z = (int16)z;
    }

    mover->angleY = (uint16)angleStep((int)mover->angleY, (int)item->angleY,
                                      ANGLE_1 * 2);

    return mover->x == x && mover->y == y && mover->z == z &&
           mover->angleY == item->angleY;
}

/* ---------------------------------------------------------------------------
 * void ItemObj::activate()
 * void ItemObj::deactivate()
 *
 * Upstream's, with an index where the pointer was. The list is singly linked
 * and deactivate walks it to find the entry before this one, exactly as
 * upstream does -- it is a few dozen entries and it happens when something
 * stops, not every frame.
 *
 * activate() refuses a second entry, which upstream's ASSERT wanted and its
 * code did not do: putting the same item on twice makes a cycle, and the
 * walk that follows never ends.
 * ------------------------------------------------------------------------ */
/* Explicitly initialised in gameInit/olLevelInit. A non-zero C initializer
   puts this in 816-tcc's .data pair, but this target has no crt0 to copy that
   pair into WRAM. Worse, the orphan RAM half used to link at $04:0000, which
   mirrors direct-page WRAM and aliases the host's fps counter. */
unsigned int olFirstActive;

void ItemObj_activate(ItemObj* item)
{
    unsigned int i = (unsigned int)(item - items);
#ifdef OL_FX3
    int enemy = Enemy_row(item->type) >= 0;
    /* Enemy::activate rejects corpses before list insertion or enable().
       A repeated trigger must not let a corpse evict a living Nav owner. */
    if (enemy) {
        if (item->health <= 0) return;
    }
#endif

#ifdef OL_FX3
    /* Enemy::activate calls enable even if ItemObj::activate did not need
       to insert it again. An evicted but still active creature may retry. */
    if (!(item->flags & ITEM_FLAG_ACTIVE)) {
#else
    if (item->flags & ITEM_FLAG_ACTIVE) return;
#endif
        item->flags |= ITEM_FLAG_ACTIVE;
        item->nextActive = (uint8)olFirstActive;
        olFirstActive = i;
#ifdef OL_FX3
    }
    if (enemy) {
        if (!Enemy_navAcquire(item, ItemObj_status(item) == ITEM_STATUS_NONE)) {
            ItemObj_setStatus(item, ITEM_STATUS_INVISIBLE);
        }
    }
#endif
}

void ItemObj_deactivate(ItemObj* item)
{
    unsigned int i = (unsigned int)(item - items);
    unsigned int prev = OL_NO_ACTIVE;
    unsigned int curr = olFirstActive;

    while (curr != OL_NO_ACTIVE)
    {
        unsigned int next = items[curr].nextActive;

        if (curr == i)
        {
            item->flags &= (uint8)~ITEM_FLAG_ACTIVE;
            item->nextActive = OL_NO_ACTIVE;
            ItemObj_touch(item);

            if (prev != OL_NO_ACTIVE) {
                items[prev].nextActive = (uint8)next;
            } else {
                olFirstActive = next;
            }
            return;
        }

        prev = curr;
        curr = next;
    }

    /* Not on the list, which is still a request to stop. */
    item->flags &= (uint8)~ITEM_FLAG_ACTIVE;
    ItemObj_touch(item);
}

/* ---------------------------------------------------------------------------
 * void ItemObj::move()
 *
 * Upstream's, both halves.
 *
 * The horizontal speed is the *animation's*: TR gives each one a speed and an
 * acceleration in 16.16, and `hSpeed` is that pair evaluated at the frame the
 * entity is on. That is the whole of why a walk and a run move differently --
 * nothing says how fast Lara runs, her run animation does.
 *
 * Both upstream and this host run at thirty ticks a second.
 *
 * Lara's `extraL->moveAngle` is the one exception to moving along an item's
 * facing. It lives as a Lara-only global rather than making every 32-byte
 * ItemObj larger.
 * ------------------------------------------------------------------------ */
#define GRAVITY         OL_ACCEL(6)     /* upstream's GRAVITY */
#define VSPEED_TERMINAL OL_SPEED(128)

/* Kept executable as a reference cartridge (`make motioncheck`). The normal
   ROM uses item_motion.asm with the same integer widths and rounding. */
#ifdef OL_C_MOTION
void ItemObj_move(ItemObj* item)
{
    int32 sp    = olAnimSpeed(item->animIndex);
    int32 accel = olAnimAccel(item->animIndex);
    int32 f     = (int32)item->frameIndex;

    /* Upstream's `frameIndex - anim->frameBegin`: this port's frame index is
       already local to the animation, which is what the packer resolved, so
       the subtraction is not there to be done. */
    if (item->flags2 & ITEM_FLAG_GRAVITY)
    {
        /* Upstream adjusts hSpeed by the ramp's *step* rather than setting it
           outright, so that a speed an animation command put there -- the
           push a jump leaves with -- survives being accelerated. */
        int32 s0 = sp + accel * (f - 1);
        int32 s1 = s0 + accel;

        item->hSpeed = (int16)((int)item->hSpeed
                               - (int)(s0 >> 16) + (int)(s1 >> 16));

        item->vSpeed = (int16)(item->vSpeed +
                               ((item->vSpeed < VSPEED_TERMINAL) ? GRAVITY : 1));
        item->y = (int16)(item->y + item->vSpeed);
    }
    else
    {
        item->hSpeed = (int16)(int)((sp + accel * f) >> 16);
    }

    if (item->hSpeed == 0) return;

    {
        int32 h = (int32)(long)item->hSpeed * OL_FRAME_TICKS;
        uint16 a = (item->type == 0) ? olLaraMoveAngle : item->angleY;
        item->x += (int)((phd_sin((int32)a) * h) >> FIXED_SHIFT);
        item->z += (int)((phd_cos((int32)a) * h) >> FIXED_SHIFT);
    }
}
#endif

void ItemObj_updateRoom(ItemObj* item, int offset)
{
    RoomQuery q;
    if (Room_query(item->room, item->x, item->y + offset, item->z, &q)) {
        item->room = (uint8)q.room;
        item->roomFloor = (int16)q.floor;
    } else {
        item->roomFloor = OL_NO_FLOOR;
    }
}

/* ---------------------------------------------------------------------------
 * bool ItemObj::collideBounds(Lara* lara, CollisionInfo* cinfo) const
 * void ItemObj::collidePush(Lara* lara, CollisionInfo* cinfo, bool) const
 *
 * The real collision, ported. Upstream rotates her offset *into the item's
 * frame* with a sine and a cosine rather than rounding the item to the nearest
 * quadrant, tests it against the box grown by her radius, pushes her out along
 * whichever of the four edges is nearest, and rotates back. That is exact at
 * any angle, and it tests Y, which the quadrant version this replaces did
 * not -- a door you could walk under was a door you could not walk past.
 *
 * `cinfo` is upstream's CollisionInfo and carries far more than the radius;
 * the radius is all this needs until Lara has a box of her own.
 *
 * X_ROTX and X_ROTY are common.h's, the same rotation the matrix code uses.
 * ------------------------------------------------------------------------ */
#define X_ROTX(x, y, s, c)  ((int)((((int32)(x) * (c)) - ((int32)(y) * (s))) >> FIXED_SHIFT))
#define X_ROTY(x, y, s, c)  ((int)((((int32)(y) * (c)) + ((int32)(x) * (s))) >> FIXED_SHIFT))

#ifdef OL_C_MOTION
void ItemObj_getBoundingBox(ItemObj* item, AABBi* out)
{
    unsigned int a = item->animIndex;

    out->minX = olAnimBox(a, 0);  out->maxX = olAnimBox(a, 1);
    out->minY = olAnimBox(a, 2);  out->maxY = olAnimBox(a, 3);
    out->minZ = olAnimBox(a, 4);  out->maxZ = olAnimBox(a, 5);
}
#endif

#ifdef OL_C_MOTION
int ItemObj_collideBounds(ItemObj* item, ItemObj* lara, int radius, int height)
{
    AABBi a;
    int minX, maxX, minY, maxY, minZ, maxZ;
    int dy = lara->y - item->y;
    int dx, dz, px, pz;
    int32 s, c;

    ItemObj_getBoundingBox(item, &a);
    minX = a.minX; maxX = a.maxX;
    minY = a.minY; maxY = a.maxY;
    minZ = a.minZ; maxZ = a.maxZ;

    /* Upstream's is `a.maxY - b.minY <= dy || a.minY - b.maxY >= dy`, where b
       is her own box. She has no frame here to take one from, so hers is a
       radius and a height: maxY at her feet, minY that far above them. TR's Y
       grows downward, which is why her top is the *smaller* number.

       Reducing her to a point instead reads as a door you can walk through
       when you stand exactly level with its base, because the test is `<=`. */
    if ((maxY - (-height) <= dy) || (minY - 0 >= dy))
        return 0;

    dx = lara->x - item->x;
    dz = lara->z - item->z;

    s = phd_sin((int32)item->angleY);
    c = phd_cos((int32)item->angleY);

    px = X_ROTX(dx, dz, s, c);
    pz = X_ROTY(dx, dz, s, c);

    return (px >= minX - radius) && (px <= maxX + radius) &&
           (pz >= minZ - radius) && (pz <= maxZ + radius);
}
#endif

void ItemObj_collidePush(ItemObj* item, ItemObj* lara, int radius)
{
    AABBi a;
    int dx = lara->x - item->x;
    int dz = lara->z - item->z;
    int minX, maxX, minZ, maxZ;
    int px, pz, ax, bx, az, bz;
    int32 s, c;

    ItemObj_getBoundingBox(item, &a);
    minX = a.minX - radius;  maxX = a.maxX + radius;
    minZ = a.minZ - radius;  maxZ = a.maxZ + radius;

    s = phd_sin((int32)item->angleY);
    c = phd_cos((int32)item->angleY);

    px = X_ROTX(dx, dz, s, c);
    pz = X_ROTY(dx, dz, s, c);

    if ((px < minX) || (px > maxX) || (pz < minZ) || (pz > maxZ))
        return;

    ax = px - minX;
    bx = maxX - px;
    az = pz - minZ;
    bz = maxZ - pz;

    if (ax <= bx && ax <= az && ax <= bz)        px -= ax;
    else if (bx <= ax && bx <= az && bx <= bz)   px += bx;
    else if (az <= ax && az <= bx && az <= bz)   pz -= az;
    else                                        pz += bz;

    s = -s;     /* back out of the item's frame */

    lara->x = item->x + X_ROTX(px, pz, s, c);
    lara->z = item->z + X_ROTY(px, pz, s, c);
}

/* ---------------------------------------------------------------------------
 * int32 ItemObj::getBridgeFloor(int32 x, int32 z) const
 * void ItemObj::getItemFloorCeiling(...) const
 *
 * Upstream's, with the ceiling half left out: nothing here asks about one yet.
 * ------------------------------------------------------------------------ */
#define ITEM_TRAP_DOOR_1    65
#define ITEM_TRAP_DOOR_2    66
#define ITEM_BRIDGE_FLAT    68
#define ITEM_BRIDGE_TILT_1  69
#define ITEM_BRIDGE_TILT_2  70
#define ITEM_DRAWBRIDGE    41

/* int32 ItemObj::getTrapDoorFloor(int32 x, int32 z) const: a trapdoor is two
   sectors long, its own and the one it opens toward. */
static int ItemObj_getTrapDoorFloor(ItemObj* item, int x, int z)
{
    int dx = (item->x >> 10) - (x >> 10);
    int dz = (item->z >> 10) - (z >> 10);

#ifdef OL_FX3
    dx = (int)(((unsigned int)(dx + 32) & 63) - 32);
    dz = (int)(((unsigned int)(dz + 32) & 63) - 32);
#endif

    if (dx == 0 && dz == 0) return item->y;
    if (dx == 0 && dz ==  1 && item->angleY == ANGLE_0)   return item->y;
    if (dx == 0 && dz == -1 && item->angleY == (uint16)ANGLE_180) return item->y;
    if (dx == 1 && dz ==  0 && item->angleY == ANGLE_90)  return item->y;
    if (dx == -1 && dz == 0 && item->angleY == (uint16)(0 - ANGLE_90)) return item->y;
    return WALL;
}

int ItemObj_getBridgeFloor(ItemObj* item, int x, int z)
{
    int h;

#ifdef OL_FX3
    if (item->type == ITEM_DRAWBRIDGE)
        return (item->state == 1) ? ItemObj_getDrawBridgeFloor(item, x, z) : WALL;
    if (item->type == ITEM_TRAP_DOOR_1 || item->type == ITEM_TRAP_DOOR_2)
        return (item->state == 0) ? ItemObj_getTrapDoorFloor(item, x, z) : WALL;
#endif
    if (item->type == ITEM_BRIDGE_FLAT)
        return item->y;

    if (item->angleY == ANGLE_0)             h = 1024 - x;
    else if (item->angleY == (uint16)ANGLE_180) h = x;
    else if (item->angleY == ANGLE_90)       h = z;
    else                                     h = 1024 - z;

    h &= 1023;

    return item->y + ((item->type == ITEM_BRIDGE_TILT_1) ? (h >> 2) : (h >> 1));
}

void ItemObj_getItemFloorCeiling(ItemObj* item, int x, int y, int z,
                                 int* floor)
{
    int h;

    if (item->type == ITEM_TRAP_DOOR_1 || item->type == ITEM_TRAP_DOOR_2)
    {
        /* Shut, it is a floor over the two sectors it covers; open, it is
           nothing and she falls through. Upstream applies it only where it
           lies above the floor the room offered. */
        if (item->state != 0) return;
        h = ItemObj_getTrapDoorFloor(item, x, z);
        if (h == WALL || h >= *floor) return;
    }
#ifdef OL_FX3
    else if (item->type == ITEM_DRAWBRIDGE)
    {
        h = ItemObj_getBridgeFloor(item, x, z);
        if (h == WALL) return;
    }
#endif
    else
    {
        if (item->type < ITEM_BRIDGE_FLAT || item->type > ITEM_BRIDGE_TILT_2)
            return;

        /* The slope is sampled at the querying point, not at the bridge's
           own origin. Using item->x/item->z happened to make the midpoint
           look right while turning every tilted bridge into a flat
           platform. */
        h = ItemObj_getBridgeFloor(item, x, z);
    }

    /* Upstream: `if (floor && (y <= h)) *floor = h;` -- a bridge only counts
       if you are above it. */
    if (y <= h)
        *floor = h;
}

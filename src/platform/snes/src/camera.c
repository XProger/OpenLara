/* OpenLara - SNES / Super FX target
 * The port of src/fixed/camera.h's follow mode.
 */

#include "camera.h"
#include "room.h"

#include "lara.h"
#include "weapon.h"

Camera camera;

/* A Location is upstream's: a point and the room it was last known to be
   in. camTarget is upstream's `target`: her position, at the height
   Camera_targetY works out, which every trace below starts from. */
Location camTarget; /* native camera math and sound.c use the same target */

/* Upstream's `>> 3` and `>> 2` on a signed delta floor it; a C divide
   truncates, and the difference is a view that arrives from one side and
   stops seven units short from the other. Spelt out, because what this
   compiler makes of a signed right shift is not something to rely on. */
#if defined(OL_FX3) && !defined(OL_C_MOTION)
int Camera_floorShift(int v, int shift);
#define floorShift Camera_floorShift
#else
static int floorShift(int v, int shift)
{
    if (v >= 0) return v >> shift;
    return -(int)(((unsigned int)(-v) + ((1u << shift) - 1)) >> shift);
}
#endif

/* The fallback's answer -- which of the four directions -- kept while the
   question is the same. The direction, not the place: upstream's candidate
   is 1536 from her in that direction, so it moves with her, and a cached
   place held the view still while she ran (the first half-second of LEVEL1). Eight traces
   is 100 K cycles on this processor -- two fields -- and against a wall
   the question is asked every tick with the same target sector, the same
   height and the same facing, to within a sixteenth of a turn. Upstream
   re-traces every frame because it can. Camera_init forgets it. */
#if defined(OL_FX3) && !defined(OL_C_MOTION)
#define CAM_CACHE_LOCAL /* shared with the native follow path */
#else
#define CAM_CACHE_LOCAL static
#endif
int             fbValid;      /* shared with camera_cold.c */
static int      fbSX, fbSY, fbSZ, fbAngle;
CAM_CACHE_LOCAL unsigned int fbDir;      /* the cardinal it chose: 0..3 quarter turns */
CAM_CACHE_LOCAL unsigned int fbRoom;     /* and the room that place resolved to */
/* And at most once in eight ticks: running past a corner fails the first
   trace on and off, and a fallback on each of those failures put the
   running tick over two fields. Between runs the clipped hit point stands
   in, which is what upstream itself settles for when the wall is far. */
unsigned int    fbTick, fbLastTick;
#define CAM_FALLBACK_EVERY  8

/* The trace's verdict, kept while its question is the same: the sector and
   height band she is in, and the sector and height band the view wants to
   be in. Standing still, or running straight down a corridor, that is every
   tick; a turn or a sector boundary re-traces. The trace is most of the 25 K
   cycles a tick an uncached camera cost, and 25 K was the walking tick over
   its budget.

   Only the verdict. When the line was clear the goal is upstream's own --
   the wanted location this tick, clipped this tick -- so in the open the
   view moves exactly as upstream's does. Keeping the clipped *position*
   instead (until 2026-09-23) froze the goal for the ten ticks she takes to
   cross a sector: a sawtooth of up to 60 units while running, and a view
   that settled wherever she had entered her last sector rather than behind
   her. The fallback (2) is kept as its direction and placed afresh the
   same way. A blocked line with no fallback keeps its traced hit point,
   which is the approximation left: it is not re-traced within the key.
   Camera_init forgets it. */
int             tcValid;      /* shared with camera_cold.c */
CAM_CACHE_LOCAL unsigned int tcMode;
CAM_CACHE_LOCAL int tcClear, tcSX, tcSY, tcSZ, tcWX, tcWY, tcWZ;
CAM_CACHE_LOCAL Location tcTo;



/* ---------------------------------------------------------------------------
 * Location Camera::getLocationForAngle(int32 angle, int32 distH, int32 distV)
 *
 * Upstream returns a Location by value and carries a room with it; here it
 * writes the two coordinates through pointers, because a struct return on this
 * compiler is a memcpy and there is none in the runtime. On the camera's own
 * target and distance; distV is 0, see Camera_targetY.
 * ------------------------------------------------------------------------ */
int offValid;                   /* the C scaler's cache; see below */
#ifndef OL_C_MOTION
void Camera_getLocationForAngle_xz(unsigned int angle, int* outX, int* outZ);
#else
/* Upstream's `distH * s >> FIXED_SHIFT`, two 32-bit library multiplies a
   call on this compiler -- 15 K cycles of the follow tick. A quarter turn
   needs none (sin is 0 or 1.0), and the wanted place asks the same angle
   every tick she runs straight, so its products are kept. Camera_init
   forgets them. */
static unsigned int offAngle;
static int offDist, offX, offZ;

static int camScale(int32 v)
{
    if (v == 0) return 0;
    if (v == (1 << FIXED_SHIFT)) return camera.distH;
    if (v == -(1 << FIXED_SHIFT)) return -camera.distH;
    return (int)(((int32)camera.distH * v) >> FIXED_SHIFT);
}

static void Camera_getLocationForAngle_xz(unsigned int angle, int* outX, int* outZ)
{
    int32 s = phd_sin((int32)angle);
    int32 c = phd_cos((int32)angle);

    int ox, oz;

    if ((angle & (ANGLE_90 - 1)) == 0) {        /* a quarter turn: exact, free */
        ox = camScale(s);                       /* both before either write */
        oz = camScale(c);
        *outX = camTarget.x - ox;
        *outZ = camTarget.z - oz;
        return;
    }
    if (!(offValid && angle == offAngle && camera.distH == offDist)) {
        offX = camScale(s);
        offZ = camScale(c);
        offAngle = angle;
        offDist = camera.distH;
        offValid = 1;
    }
    *outX = camTarget.x - offX;
    *outZ = camTarget.z - offZ;
}
#endif

/* ---------------------------------------------------------------------------
 * The clipper: upstream's trace, traceX, traceZ from room.h and checkWall,
 * checkHeight, clip and getBestLocation from camera.h. Every question is a
 * Room_query at a point, which is the reason the follow camera went through
 * walls until Room_query could answer one -- see "The camera follows her".
 *
 * A Location is upstream's: a point and the room it was last known to be
 * in. Floors come back as OL_NO_FLOOR from the table and as WALL here, so
 * the tests read as upstream's.
 * ------------------------------------------------------------------------ */
#define CAM_WALL        (-127 * 256)
#define TRACE_SHIFT     10
#define X_ABS(v)        ((v) < 0 ? -(v) : (v))

#ifndef OL_C_MOTION
int camQuery(unsigned int room, int x, int y, int z,
             int* floor, int* ceiling, unsigned int* outRoom);
#else
static int camQuery(unsigned int room, int x, int y, int z,
                    int* floor, int* ceiling, unsigned int* outRoom)
{
    RoomQuery q;
    if (!Room_query(room, x, y, z, &q)) return 0;
    *floor   = (q.floor == OL_NO_FLOOR) ? CAM_WALL : q.floor;
    *ceiling = q.ceiling;
    *outRoom = q.room;
    return 1;
}
#endif

/* Upstream's `Room::getRoom(x, y, z)`: the room a point resolves to. */
#ifndef OL_C_MOTION
unsigned int camRoomAt(unsigned int room, int x, int y, int z);
#else
static unsigned int camRoomAt(unsigned int room, int x, int y, int z)
{
    int floor, ceiling;
    unsigned int r;
    if (camQuery(room, x, y, z, &floor, &ceiling, &r)) return r;
    return room;
}
#endif

#ifndef OL_C_MOTION
/* Exact native sector walkers in FX3 FastROM or GSU-2 resident WRAM.
   The C path below remains executable in reference cartridges. */
int traceX(const Location* from, Location* to);
int traceZ(const Location* from, Location* to);
#else
/* TRACE_CHECK, as a function: the point is inside the world unless it is
   under the floor or over the ceiling of the sector it resolves to. The
   inaccurate form upstream uses on the view side reads the sector's base
   heights; here both forms read the point-exact ones the query gives. */
static int traceCheck(unsigned int* room, int x, int y, int z,
                      Location* to, unsigned int failRoom, int px, int py, int pz)
{
    int floor, ceiling;
    unsigned int r;

    if (!camQuery(*room, x, y, z, &floor, &ceiling, &r)) return 1;
    *room = r;
    if (y > floor || y < ceiling) {
        to->x = px; to->y = py; to->z = pz;
        to->room = failRoom;
        return 0;
    }
    return 1;
}

static int traceX(const Location* from, Location* to)
{
    int dx = to->x - from->x;
    int32 sy, sz;
    int px, py, pz;
    unsigned int room, next;

    if (dx == 0) return 1;
    /* No X boundary can be visited inside one sector. Avoid the slope
       divides/multiplies, but preserve the no-iteration room assignment. */
    if (((from->x ^ to->x) & ~1023) == 0) {
        to->room = from->room;
        return 1;
    }

    /* A 32-bit divide is 1,500 cycles here, and the follow camera's traces
       are level: no delta, no divide. */
    sy = (to->y == from->y) ? 0 : (((int32)(to->y - from->y) << TRACE_SHIFT) / dx);
    sz = (to->z == from->z) ? 0 : (((int32)(to->z - from->z) << TRACE_SHIFT) / dx);

    px = from->x; py = from->y; pz = from->z;
    room = from->room;

    if (dx < 0) {
        px &= ~1023;
        py = (int)(py + ((sy * (px - from->x)) >> TRACE_SHIFT));
        pz = (int)(pz + ((sz * (px - from->x)) >> TRACE_SHIFT));
        while ((int)(int16)(uint16)(px - to->x) > 0) {
            if (!traceCheck(&room, px, py, pz, to, room, px, py, pz)) return 0;
            next = room;
            if (!traceCheck(&next, px - 1, py, pz, to, room, px, py, pz)) return 0;
            room = next;
            px -= 1024;
            py = (int)(py - sy);
            pz = (int)(pz - sz);
        }
    } else {
        px |= 1023;
        py = (int)(py + ((sy * (px - from->x)) >> TRACE_SHIFT));
        pz = (int)(pz + ((sz * (px - from->x)) >> TRACE_SHIFT));
        while ((int)(int16)(uint16)(to->x - px) > 0) {
            if (!traceCheck(&room, px, py, pz, to, room, px, py, pz)) return 0;
            next = room;
            if (!traceCheck(&next, px + 1, py, pz, to, room, px, py, pz)) return 0;
            room = next;
            px += 1024;
            py = (int)(py + sy);
            pz = (int)(pz + sz);
        }
    }

    to->room = room;
    return 1;
}

static int traceZ(const Location* from, Location* to)
{
    int dz = to->z - from->z;
    int32 sx, sy;
    int px, py, pz;
    unsigned int room, next;

    if (dz == 0) return 1;
    if (((from->z ^ to->z) & ~1023) == 0) {
        to->room = from->room;
        return 1;
    }

    sx = (to->x == from->x) ? 0 : (((int32)(to->x - from->x) << TRACE_SHIFT) / dz);
    sy = (to->y == from->y) ? 0 : (((int32)(to->y - from->y) << TRACE_SHIFT) / dz);

    px = from->x; py = from->y; pz = from->z;
    room = from->room;

    if (dz < 0) {
        pz &= ~1023;
        px = (int)(px + ((sx * (pz - from->z)) >> TRACE_SHIFT));
        py = (int)(py + ((sy * (pz - from->z)) >> TRACE_SHIFT));
        while ((int)(int16)(uint16)(pz - to->z) > 0) {
            if (!traceCheck(&room, px, py, pz, to, room, px, py, pz)) return 0;
            next = room;
            if (!traceCheck(&next, px, py, pz - 1, to, room, px, py, pz)) return 0;
            room = next;
            pz -= 1024;
            px = (int)(px - sx);
            py = (int)(py - sy);
        }
    } else {
        pz |= 1023;
        px = (int)(px + ((sx * (pz - from->z)) >> TRACE_SHIFT));
        py = (int)(py + ((sy * (pz - from->z)) >> TRACE_SHIFT));
        while ((int)(int16)(uint16)(to->z - pz) > 0) {
            if (!traceCheck(&room, px, py, pz, to, room, px, py, pz)) return 0;
            next = room;
            if (!traceCheck(&next, px, py, pz + 1, to, room, px, py, pz)) return 0;
            room = next;
            pz += 1024;
            px = (int)(px + sx);
            py = (int)(py + sy);
        }
    }

    to->room = room;
    return 1;
}

#endif

/* The short signed differences in traceX/Z's loop conditions deliberately
   cross the world's wrapped X/Z seam. Comparing absolute coordinates skips
   the whole walk when a nearby endpoint changes sign at 32768. */

/* bool trace(const Location &from, Location &to, bool accurate): whether
   the line is clear; when it is not, `to` is moved to where it stopped. */
int Room_trace(const Location* from, Location* to)
{
    int dx = X_ABS(to->x - from->x);
    int dz = X_ABS(to->z - from->z);
    int dy, res;
    int floor, ceiling, h;
    unsigned int r;

    if (dz > dx) {
        res = traceX(from, to);
        if (!traceZ(from, to)) return 0;
    } else {
        res = traceZ(from, to);
        if (!traceX(from, to)) return 0;
    }

    dy = to->y - from->y;
    if (dy)
    {
        if (camQuery(to->room, to->x, to->y, to->z, &floor, &ceiling, &r))
        {
            h = floor;
            if (to->y <= h || from->y >= h) {
                h = ceiling;
                if (to->y >= h || from->y <= h) h = CAM_WALL;
            }
            if (h != CAM_WALL) {
                to->y = h;
                h -= from->y;
                to->x = from->x + (int)(((int32)(to->x - from->x) * h) / dy);
                to->z = from->z + (int)(((int32)(to->z - from->z) * h) / dy);
                return 0;
            }
        }
    }

    return res;
}

static int checkWall(unsigned int room, int x, int y, int z)
{
    int floor, ceiling;
    unsigned int r;
    if (!camQuery(room, x, y, z, &floor, &ceiling, &r)) return 1;
    return (floor == CAM_WALL || ceiling == CAM_WALL || ceiling >= floor ||
            y > floor || y < ceiling);
}

static int checkHeight(unsigned int room, int x, int y, int z)
{
    int floor, ceiling;
    unsigned int r;
    if (!camQuery(room, x, y, z, &floor, &ceiling, &r)) return y;

    if (floor != CAM_WALL && ceiling != CAM_WALL && ceiling < floor)
    {
        if (y - CAM_RADIUS < ceiling && y + CAM_RADIUS > floor)
            return (floor + ceiling) >> 1;
        if (y + CAM_RADIUS > floor)
            return floor - CAM_RADIUS;
        if (y - CAM_RADIUS < ceiling)
            return ceiling + CAM_RADIUS;
    }
    return y;
}

/* void Camera::clip(Location &loc): the view's radius off every wall.

   Upstream asks six questions: the height at the point, then a wall at each
   of four probes CAM_RADIUS away, then the room. With the view between its
   sector's floor and ceiling, one question answers most of them
   exactly: checkHeight is arithmetic on that floor and ceiling, a probe that
   stays in the same sector reads nearly the same two heights (see the slope
   margin below), and every snap stays
   in the same sector column, so the room is the one the question found. Only
   a probe that crosses into the next sector is asked -- at most one an axis,
   since CAM_RADIUS is under half a sector. Walls, a view outside its sector
   or within a slope's reach of its floor or ceiling take upstream's path
   whole. In a corridor that is two queries a
   tick instead of six. */
/* And the last answer, for the same question: standing still, or a view
   parked behind a wall, asks it every tick. Camera_init forgets it (a block
   moved under a probe is the one change it cannot see; the next step asks
   again). */
int clipValid;
static Location clipIn, clipOut;

void Camera_clip(Location* loc)
{
    RoomQuery q;
    int F, C, y, same, full;
    /* locals: every loc-> is a far pointer, which this compiler rebuilds */
    int x = loc->x, z = loc->z;
    unsigned int room = loc->room;

    y = loc->y;
    if (clipValid && x == clipIn.x && y == clipIn.y && z == clipIn.z &&
        room == clipIn.room) {
        loc->x = clipOut.x; loc->y = clipOut.y; loc->z = clipOut.z;
        loc->room = clipOut.room;
        return;
    }
    clipIn.x = x; clipIn.y = y; clipIn.z = z; clipIn.room = room;
    full = !Room_query(room, x, y, z, &q) ||
           q.floor == OL_NO_FLOOR || q.ceiling == CAM_WALL ||
           y < q.ceiling || y > q.floor;

    same = 0;
    if (!full) {
        /* checkHeight, on the one sector */
        F = q.floor; C = q.ceiling;
        if (C < F) {
            if (y - CAM_RADIUS < C && y + CAM_RADIUS > F) y = (F + C) >> 1;
            else if (y + CAM_RADIUS > F) y = F - CAM_RADIUS;
            else if (y - CAM_RADIUS < C) y = C + CAM_RADIUS;
        }
        /* checkWall of a probe that stays in this sector. A slope moves a
           height |s|/4 a unit, so less than 64|s| over the probe's 255: a
           view that far clear of both reads clear at every such probe.
           Nearer than that, ask them all. */
        if (y - C >= ((X_ABS(q.ceilingSlopeX) + X_ABS(q.ceilingSlopeZ)) << 6) &&
            F - y >= ((X_ABS(q.floorSlopeX) + X_ABS(q.floorSlopeZ)) << 6))
            same = (C >= F || y > F || y < C);
        else
            full = 1;
    }
    if (full) y = checkHeight(room, x, loc->y, z);

    if ((full || (x & 1023) < CAM_RADIUS)
            ? checkWall(room, x - CAM_RADIUS, y, z) : same)
        x = (x & ~1023) + CAM_RADIUS;
    if ((full || (x & 1023) > 1023 - CAM_RADIUS)
            ? checkWall(room, x + CAM_RADIUS, y, z) : same)
        x = (x | 1023) - CAM_RADIUS;
    if ((full || (z & 1023) < CAM_RADIUS)
            ? checkWall(room, x, y, z - CAM_RADIUS) : same)
        z = (z & ~1023) + CAM_RADIUS;
    if ((full || (z & 1023) > 1023 - CAM_RADIUS)
            ? checkWall(room, x, y, z + CAM_RADIUS) : same)
        z = (z | 1023) - CAM_RADIUS;

    room = full ? camRoomAt(room, x, y, z) : q.room;
    loc->x = clipOut.x = x; loc->y = clipOut.y = y; loc->z = clipOut.z = z;
    loc->room = clipOut.room = room;
    clipValid = 1;
}

static int fastLength(int dx, int dz)
{
    dx = X_ABS(dx);
    dz = X_ABS(dz);
    return (dx > dz) ? (dx + (dz >> 1)) : (dz + (dx >> 1));
}

#if defined(OL_FX3) && defined(OL_C_MOTION)
/* C oracle for the native packed-scalar lookup. CAMERA.BIN is bank-contained. */
int Camera_boxOffset(ItemObj* item, unsigned int component)
{
    union { const unsigned char* p; uint16 w[2]; } address;
    const unsigned char* base;
    const unsigned char* record;
    unsigned int directory, offset, tick = item->frameIndex, bits, pos, shift, value;
    if (component > CAM_BOX_MIDDLE_Z || olLevel.cameraSize == 0) return 0;
    address.w[0] = olLevel.cameraAt; address.w[1] = olLevel.cameraBank;
    base = address.p;
    directory = 4 + olMul8(olData16(base, 0)) + (olData16(base, 2) << 2);
    if (item->animIndex >= olData16(base, directory)) return 0;
    offset = olData16(base, directory + 2 + olMul8(item->animIndex) + (component << 1));
    if (!offset) return 0;
    record = base + offset;
    if (tick >= record[0]) tick = 0;
    bits = record[1];
    if (!bits) return (int)(int16)olData16(record, 2);
    pos = tick * bits;
    shift = pos & 7;
    offset = 4 + (pos >> 3);
    value = olData16(record, offset) >> shift;
    if (shift) value |= (unsigned int)record[offset + 2] << (16 - shift);
    value &= (unsigned int)(65535u >> (16 - bits));
    return (int)(int16)(olData16(record, 2) + value);
}
#endif

/* Upstream's target height, from Camera::update: three quarters of the way
   up her bounding box -- her chest standing, her hands hanging, lower in a
   crouch -- so the camera follows what she is doing, not only where she
   is. FX3 reads the exact GBA frame-interpolated offsets from CAMERA.BIN;
   GSU-2 retains its animation union. Contact's nearest-frame box is separate.
   The view sits distV (267) above it and the renderer shears its projection
   so the target is on the centre line, as upstream's pitch puts it. */
#if defined(OL_FX3) && !defined(OL_C_MOTION)
int Camera_targetY(ItemObj* item);
#else
int Camera_targetY(ItemObj* item)
{
#ifdef OL_FX3
    return item->y + Camera_boxOffset(item, CAM_BOX_FOLLOW_Y);
#else
    int minY = olAnimBox(item->animIndex, 2);
    int maxY = olAnimBox(item->animIndex, 3);
    return item->y + maxY + floorShift((minY - maxY) * 3, 2);
#endif
}
#endif

/* Upstream's update() ends by pointing the view at the target: the yaw is
   the vector's, not her facing, which is the difference between a view
   that swings round behind her when she turns and one that turns with her
   from wherever it happens to be -- and, when a wall has displaced it,
   between a view that has her in it and one that does not. The pitch
   upstream derives alongside is the GSU's: it projects camTarget.y (handed
   over as ol_camTgtY) and moves its centre line to it. On top of
   her the vector says nothing, and the yaw stays what it was. */
#if defined(OL_FX3) && !defined(OL_C_MOTION)
void Camera_lookAtTarget(void);
#else
void Camera_lookAtTarget(void)
{
    int dx = camTarget.x - camera.x;
    int dz = camTarget.z - camera.z;
    if (dx == 0 && dz == 0) return;
    camera.angleY = (unsigned int)(uint16)phd_atan((int32)dz, (int32)dx);
}
#endif

/* Location Camera::getLocationForAngle(angle, distH, distV), on the split
   Camera_pitch keeps in camera.distH/distV. */
static void Camera_locationForAngle(unsigned int angle, Location* out)
{
    Camera_getLocationForAngle_xz(angle, &out->x, &out->z);
    out->y = camTarget.y + camera.distV;
    out->room = camTarget.room;
}

/* getBestLocation's split of targetDist by targetAngle.x: distH across and
   distV up. Two 32-bit multiplies, so only when the angle or the distance
   changes; following her on land neither ever does. Camera_init forgets it. */
int pitchValid;
static int pitchKey, pitchDist;

void Camera_pitch(int ax, int dist)
{
    int32 s, c;
    camera.targetDist = dist;
    if (pitchValid && ax == pitchKey && dist == pitchDist) return;
    s = phd_sin((int32)ax);
    c = phd_cos((int32)ax);
    camera.distH = (int)(((int32)dist * c) >> FIXED_SHIFT);
    camera.distV = (int)(((int32)dist * s) >> FIXED_SHIFT);
    pitchKey = ax;
    pitchDist = dist;
    pitchValid = 1;
}

/* Upstream's updateFollow: the angle her state asked for, else
   CAM_ANGLE_FOLLOW, plus her own pitch (swimming), clamped. */
int Camera_followAngleX(void)
{
    int ax = (camera.askAngleX && CAM_ASK ? camera.askAngleX : CAM_ANGLE_FOLLOW) + (int)(int16)olLaraAngleX;
    if (ax < -CAM_ANGLE_MAX) ax = -CAM_ANGLE_MAX;
    if (ax >  CAM_ANGLE_MAX) ax =  CAM_ANGLE_MAX;
    return ax;
}

/* ---------------------------------------------------------------------------
 * Location Camera::getBestLocation(bool clip)
 *
 * The place the follow camera wants, traced from the target; when the line
 * is blocked and the wanted place is within 768 of her, the four cardinal
 * directions are tried and the one nearest the present view wins.
 * ------------------------------------------------------------------------ */
int Camera_getBestLocation(int clipFlag, Location* best)
{
    Location tmpDest, tmpView;
    int dist, minDist;
    int sx, sy, sz, a;
    unsigned int i;

    Camera_locationForAngle(camera.targetAngleY, best);

    if (Room_trace(&camTarget, best)) return 1;

    if (clipFlag) {
        if (best->x != camTarget.x || best->y != camTarget.y || best->z != camTarget.z) return 0;
    }

    dist = fastLength(camTarget.x - best->x, camTarget.z - best->z);
    if (dist > 768) return 0;

    sx = camTarget.x >> 10;
    sy = camTarget.y >> 8;
    sz = camTarget.z >> 10;
    a  = (int)(camera.targetAngleY >> 12);
    /* Within the key, or inside the once-in-eight limit, the last choice
       stands: its place is worked out afresh by the caller. */
    if (fbValid && ((sx == fbSX && sy == fbSY && sz == fbSZ && a == fbAngle) ||
                    (unsigned int)(fbTick - fbLastTick) < CAM_FALLBACK_EVERY))
        return 2;
    fbLastTick = fbTick;

    minDist = 32767;
    for (i = 0; i < 4; i++)
    {
        Camera_locationForAngle(i * ANGLE_90, &tmpDest);
        tmpView.x = camera.x; tmpView.y = camera.y; tmpView.z = camera.z;
        tmpView.room = camera.room;

        if (!Room_trace(&camTarget, &tmpDest)) continue;
        if (!Room_trace(&tmpDest, &tmpView)) continue;

        dist = fastLength(camera.x - tmpDest.x, camera.z - tmpDest.z);
        if (dist < minDist) {
            minDist = dist;
            fbDir = i;
            fbRoom = tmpDest.room;
            /* field by field: a struct copy is a memcpy the runtime lacks */
            best->x = tmpDest.x; best->y = tmpDest.y; best->z = tmpDest.z;
            best->room = tmpDest.room;
        }
    }

    /* None clear: upstream keeps the blocked trace's point, as here. */
    if (minDist == 32767) return 0;
    fbValid = 1;
    fbSX = sx; fbSY = sy; fbSZ = sz; fbAngle = a;
    return 2;
}

/* ---------------------------------------------------------------------------
 * void Camera::move(Location &to, int32 speed)
 *
 * Upstream clips the wanted location, lerps the view toward it by 1/speed --
 * which is what stops the camera snapping round every time Lara turns --
 * and, when the view has ended up near a floor, traces from the target to
 * it once more. The divide is by a constant power of two here, because a
 * general divide on this processor is a library call and `speed` is 8 in
 * every path that reaches it.
 * ------------------------------------------------------------------------ */
#ifndef OL_C_MOTION
void Camera_move(Location* to);
#else
/* upstream's move(): speed 8 is `>> 3`, which floors; any other speed above
   1 is a C divide, which truncates toward zero; 1 goes the whole way */
static int camStep(int d)
{
    if (camera.speed == 8) return floorShift(d, CAM_SPEED_SHIFT);
    if (camera.speed == 4) return d < 0 ? -(int)((unsigned int)(-d) >> 2) : (d >> 2);
    return d;
}

void Camera_move(Location* to)
{
    Location view;
    int floor, ceiling;
    unsigned int r;

    /* clip(to) is the caller's now: its answer is cached with the trace's */

    camera.x += camStep(to->x - camera.x);
    camera.y += camStep(to->y - camera.y);
    camera.z += camStep(to->z - camera.z);
    camera.room = camRoomAt(to->room, camera.x, camera.y, camera.z);

    if (camQuery(camera.room, camera.x, camera.y, camera.z, &floor, &ceiling, &r))
    {
        floor -= 256;
        if (camera.y >= floor && to->y >= floor)
        {
            view.x = camera.x; view.y = camera.y; view.z = camera.z;
            view.room = camera.room;
            Room_trace(&camTarget, &view);
            camera.x = view.x; camera.y = view.y; camera.z = view.z;
            camera.room = camRoomAt(view.room, camera.x, camera.y, camera.z);
        }
    }
}
#endif

/* ---------------------------------------------------------------------------
 * void Camera::updateFollow(ItemObj* item)
 *
 * Upstream accumulates targetAngle from the item's own angle and then asks
 * getBestLocation, which clips, and moves toward what it answers.
 * ------------------------------------------------------------------------ */
#ifdef OL_FX3
void Camera_lookAt(int offset);         /* camera_fix.c */
void Camera_updateFixed(void);
void Camera_tail(int isFixed);
#endif

#if defined(OL_FX3) && !defined(OL_C_MOTION)
void Camera_updateFollowCold(ItemObj* item)
#else
void Camera_updateFollow(ItemObj* item)
#endif
{
    Location want, best;
    int sx, sy, sz, wx, wy, wz, y, ax, ay, clipFlag;
    unsigned int mode = camera.mode;
#ifdef OL_FX3
    int isFixed = 0;

    /* Camera::update: a fixed camera with a target looks at the target's
       middle; without one, at her. Her own target with an item to look at
       turns her head to it first (lookAt), which can make this LOOK. All of
       it only while any of that state is live (olCamBusy): the three-
       creature clock gate has no room for it on every tick. */
    if (olCamBusy) {
        olCamFixedView = 0;
        if (olCamLookAt != CAM_NO_ITEM && (mode == CAM_MODE_FIXED || mode == CAM_MODE_OBJECT)) {
            isFixed = 1;
            item = items + olCamLookAt;
            y = item->y + Camera_boxOffset(item, CAM_BOX_MIDDLE_Y);
        } else {
            y = Camera_targetY(item);
            if (olCamLookAt != CAM_NO_ITEM) {
                Camera_lookAt(y);
                mode = camera.mode;
            }
        }
    } else
#endif
    y = Camera_targetY(item);

    /* Camera::update: three quarters up her box; looking and aiming, 256
       higher (y is down); eased by a quarter either way. Then the mode's
       own targetAngle and distance, and the speed move() divides by. */
    ay = (int)item->angleY;
    clipFlag = 0;
    if (mode == CAM_MODE_FOLLOW) {
        /* targetAngle.y is an offset from her facing there, and the
           distance whatever her state asked (the interactions' 1024) */
#ifdef OL_FX3
        if (camera.ask) {
            ay += camera.askAngleY;
            Camera_pitch(Camera_followAngleX(), camera.askDist ? camera.askDist : CAM_DIST_FOLLOW);
        } else {
            Camera_pitch(Camera_followAngleX(), CAM_DIST_FOLLOW);
        }
#else
        ay += camera.askAngleY;
        Camera_pitch(Camera_followAngleX(), camera.askDist ? camera.askDist : CAM_DIST_FOLLOW);
#endif
        camera.speed = 8;
    } else if (mode == CAM_MODE_COMBAT || mode == CAM_MODE_LOOK) {
        y -= 256;
        ax = (int)(int16)olLaraAngleX;
        if (mode == CAM_MODE_COMBAT) {
            /* updateCombat: both arms share one target and one angleAim */
            ax += CAM_ANGLE_COMBAT;
            if (olWeaponTarget != OL_NO_ACTIVE) {
                ax += olWeaponAimX;
                ay += olWeaponAimY;
            } else {
                ax += olLaraLook.headX + olLaraLook.torsoX;
                ay += olLaraLook.headY + olLaraLook.torsoY;
            }
            Camera_pitch(ax, CAM_DIST_COMBAT);
            camera.speed = 8;
        } else {
            /* updateLook: 768 away, or the follow distance while an item
               holds her look */
            ax += olLaraLook.headX + olLaraLook.torsoX;
            ay += olLaraLook.headY + olLaraLook.torsoY;
#ifdef OL_FX3
            Camera_pitch(ax, (olCamBusy && olCamLookAt != CAM_NO_ITEM) ? CAM_DIST_FOLLOW : CAM_DIST_LOOK);
#else
            Camera_pitch(ax, CAM_DIST_LOOK);
#endif
            camera.speed = 4;
        }
        clipFlag = 1;
    }
    camera.targetAngleY = (unsigned int)(uint16)ay;

    /* the target's height eases toward hers: upstream's `>> 2` -- except
       across a cut to or from a fixed camera (lastFixed), which snaps it
       and moves the view the whole way (speed 1) */
    camTarget.x    = item->x;
    camTarget.z    = item->z;
    camTarget.room = item->room;
#ifdef OL_FX3
    if (olCamBusy) {
        if (mode == CAM_MODE_LOOK || mode == CAM_MODE_COMBAT) {
            if (olCamLastFixed) {
                camTarget.y = 0;            /* upstream's, verbatim */
                camera.speed = 1;
            } else {
                camTarget.y += floorShift(y - camTarget.y, 2);
            }
        } else {
            olCamLastFixed ^= (unsigned int)isFixed;
            if (olCamLastFixed) {
                camTarget.y = y;
                camera.speed = 1;
            } else {
                camTarget.y += floorShift(y - camTarget.y, 2);
            }
        }
    } else
#endif
    camTarget.y += floorShift(y - camTarget.y, 2);

    /* center (block push/pull, climbing out of water): the box's middle,
       turned by the yaw -- X_ROTXY(dz, dx, s, c) -- off look and combat */
#ifdef OL_FX3
    if (CAM_ASK && camera.center && mode != CAM_MODE_LOOK && mode != CAM_MODE_COMBAT) {
        int32 s = phd_sin((int32)item->angleY), c = phd_cos((int32)item->angleY);
        int32 dx = Camera_boxOffset(item, CAM_BOX_MIDDLE_X);
        int32 dz = Camera_boxOffset(item, CAM_BOX_MIDDLE_Z);
        camTarget.x += (int)((dx * c + dz * s) >> FIXED_SHIFT);
        camTarget.z += (int)((dz * c - dx * s) >> FIXED_SHIFT);
    }
#endif

    fbTick++;

#ifdef OL_FX3
    if (olCamBusy && mode >= CAM_MODE_FIXED) {
        Camera_updateFixed();
        goto done;
    }
#endif

    Camera_locationForAngle(camera.targetAngleY, &want);
    sx = camTarget.x >> 10; sy = camTarget.y >> 8; sz = camTarget.z >> 10;
    wx = want.x >> 10;      wy = want.y >> 8;      wz = want.z >> 10;

    if (!(tcValid && sx == tcSX && sy == tcSY && sz == tcSZ &&
          wx == tcWX && wy == tcWY && wz == tcWZ && mode == tcMode))
    {
        tcMode = mode;
        tcClear = Camera_getBestLocation(clipFlag, &tcTo);
        if (!tcClear) Camera_clip(&tcTo);
        tcValid = 1;
        tcSX = sx; tcSY = sy; tcSZ = sz;
        tcWX = wx; tcWY = wy; tcWZ = wz;
    }
    if (tcClear) {
        /* upstream's goal, this tick: the wanted place itself (1) or the
           fallback's direction (2), 1536 from where she is now, clipped */
        if (tcClear == 1) {
            best.x = want.x; best.y = want.y; best.z = want.z;
        } else {
            Camera_locationForAngle(fbDir * ANGLE_90, &best);
            tcTo.room = fbRoom;         /* the hint for its clip */
        }
        best.room = tcTo.room;
        Camera_clip(&best);
    } else {
        best.x = tcTo.x; best.y = tcTo.y; best.z = tcTo.z;
        best.room = tcTo.room;
    }

    Camera_move(&best);

#ifdef OL_FX3
done:
    /* the tail of Camera::update: back to following unless OBJECT holds */
    if (olCamBusy) Camera_tail(isFixed);
    else
#endif
    camera.mode = CAM_MODE_FOLLOW;      /* as Camera::update leaves it */
#ifdef OL_FX3
    if (camera.ask) {                   /* and what her state asked, spent */
        camera.ask = 0;
#endif
        camera.askAngleX = 0;
        camera.askAngleY = 0;
        camera.askDist = 0;
        camera.center = 0;
#ifdef OL_FX3
    }
#endif
    Camera_lookAtTarget();
}

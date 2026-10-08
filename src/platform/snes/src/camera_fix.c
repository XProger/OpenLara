/* OpenLara - SNES / Super FX target
 * Fixed cameras and camera targets (FX3): the rest of src/fixed/camera.h's
 * modes and src/fixed/room.h's camera trigger actions. camera.c's update
 * calls in here for the fixed view and the look at an item.
 */

#include "camera.h"
#include "room.h"
#include "level.h"
#include "lara.h"
#include "weapon.h"

#define X_ABS(v)        ((v) < 0 ? -(v) : (v))

extern Location camTarget;
void Camera_clip(Location* loc);
void Camera_move(Location* to);

/* Upstream's `>> 1` on a signed value floors; see camera.c. */
static int floorShift(int v, int shift)
{
    if (v >= 0) return v >> shift;
    return -(int)(((unsigned int)(-v) + ((1u << shift) - 1)) >> shift);
}

/* ---------------------------------------------------------------------------
 * Fixed cameras and camera targets: upstream's checkCamera, checkTrigger's
 * ACTIVATE_CAMERA and CAMERA_TARGET arms, updateFixed and lookAt.
 *
 * CAMERA.BIN (packer/out_SNES.h) in FX3 CPU ROM: u16 cameras, u16
 * triggers; per camera int16 x, y, z, u8 room, u8 flags (TR's ONCE); per
 * trigger its camera ($FF none), the camera's timer, once | speed << 1 and
 * the target's slot ($FF none).
 *
 * Upstream runs the trigger's camera part inside checkTrigger, once from a
 * landing and once at the end of Lara::update -- the second after
 * updateWeapon's toCombat. The early and late trigger actions keep that
 * order here too; the camera sees them after the active-item update, so
 * Camera_noteTrigger and Camera_passTrigger retain their mode history for
 * Camera_prepare: s_checkLook's LOOK before both, toCombat's COMBAT before
 * the late one, then any landed block's OBJECT camera.
 * ------------------------------------------------------------------------ */
#include "olhost.h"

int olCamIndex, olCamLastIndex, olCamTimer;
unsigned int olCamLookAt, olCamLastItem, olCamLastFixed;
unsigned int olCamFixedView;                /* the view is a fixed camera's this tick */
unsigned int olCamBusy;                     /* any of this state is not at rest */
unsigned int olTrigLate;                    /* lara.c: the end-of-update checkTrigger */
uint8 olCamOnce[32];                         /* FIXED_CAMERA_FLAG_ONCE, a bit a camera; saved */
static unsigned int camEvents;
/* Lara can check twice; each other item can land once in the active-list
   update. Keep those events until the camera sees them in that order. */
#define CAM_EVENT_MAX   (OL_ITEM_MAX + 2)
static uint16 camEvent[CAM_EVENT_MAX];       /* trigger | passed | late | switch on | object */
#define CAM_EV_PASSED   0x0100
#define CAM_EV_LATE     0x0200
#define CAM_EV_SWITCH   0x0400
#define CAM_EV_OBJECT   0x0800

static const unsigned char* camAt(unsigned int offset)
{
    union { const unsigned char* p; uint16 w[2]; } u;
    u.w[0] = (uint16)(olLevel.cameraAt + offset);   /* bank-contained */
    u.w[1] = olLevel.cameraBank;
    return u.p;
}

/* a trigger's four bytes */
static const unsigned char* camTrig(unsigned int t)
{
    return camAt(4 + olMul8(olData16(camAt(0), 0)) + (t << 2));
}

void Camera_levelInit(void)
{
    unsigned int i, n;
    olCamIndex = olCamLastIndex = -1;
    olCamTimer = 0;
    olCamLookAt = olCamLastItem = CAM_NO_ITEM;
    olCamLastFixed = 0;
    olCamFixedView = 0;
    olCamBusy = 0;
    camEvents = 0;
    for (i = 0; i < 32; i++) olCamOnce[i] = 0;
    if (olLevel.cameraSize == 0) return;
    n = olData16(camAt(0), 0);
    for (i = 0; i < n; i++)
        if (camAt(4 + olMul8(i))[7] & 1) olCamOnce[i >> 3] |= (uint8)(1 << (i & 7));
}

unsigned int Camera_noteTrigger(unsigned int t)
{
    unsigned int e = camEvents;
    if (olLevel.cameraSize == 0 || e >= CAM_EVENT_MAX) return CAM_NO_ITEM;
    camEvent[e] = (uint16)(t | (olTrigLate ? CAM_EV_LATE : 0));
    camEvents = e + 1;
    olCamBusy = 1;
    return e;
}

unsigned int Camera_noteTriggerObject(unsigned int t)
{
    unsigned int e = Camera_noteTrigger(t);
    if (e != CAM_NO_ITEM) camEvent[e] |= CAM_EV_OBJECT;
    return e;
}

void Camera_passTrigger(unsigned int event, unsigned int switchOn)
{
    if (event >= camEvents) return;
    camEvent[event] |= CAM_EV_PASSED | (switchOn ? CAM_EV_SWITCH : 0);
}

/* checkCamera(fd, camera), before the trigger's type is asked */
static void Camera_checkCamera(const unsigned char* c)
{
    int lookAt = 0;
    if (c[0] != 0xFF) {
        if ((int)c[0] != olCamLastIndex) {
            olCamLookAt = CAM_NO_ITEM;
            return;
        }
        olCamIndex = c[0];
        if (olCamTimer < 0 || camera.mode == CAM_MODE_LOOK || camera.mode == CAM_MODE_COMBAT) {
            olCamTimer = -1;
            olCamLookAt = CAM_NO_ITEM;
            return;
        }
        camera.mode = CAM_MODE_FIXED;
        lookAt = 1;
    }
    if (c[3] != 0xFF && camera.mode != CAM_MODE_LOOK && camera.mode != CAM_MODE_COMBAT)
        olCamLookAt = c[3];
    if (!lookAt && olCamLookAt != CAM_NO_ITEM && olCamLookAt != olCamLastItem &&
        (items[olCamLookAt].flags2 & ITEM_FLAG_ANIMATED))
        olCamLookAt = CAM_NO_ITEM;
}

/* checkTrigger's arms, when its type let the actions run */
static void Camera_triggerArms(unsigned int t, const unsigned char* c, unsigned int ev)
{
    unsigned int cam = c[0];
    if (cam != 0xFF && !(olCamOnce[cam >> 3] & (1 << (cam & 7)))) {
        olCamIndex = (int)cam;
        if (camera.mode != CAM_MODE_LOOK && camera.mode != CAM_MODE_COMBAT &&
            olTrigType(t) != 7 &&                                    /* COMBAT */
            !((ev & CAM_EV_SWITCH) && olTrigTimer(t) != 0) &&
            (olTrigType(t) == 2 || olCamIndex != olCamLastIndex))     /* SWITCH */
        {
            olCamTimer = c[1];
            if (olCamTimer != 1) olCamTimer *= 30;
            if (c[2] & 1) olCamOnce[cam >> 3] |= (uint8)(1 << (cam & 7));
            camera.mode = (ev & CAM_EV_OBJECT) ? CAM_MODE_OBJECT : CAM_MODE_FIXED;
        }
    }
    if (c[3] != 0xFF && (camera.mode == CAM_MODE_FIXED || camera.mode == CAM_MODE_OBJECT))
        olCamLookAt = c[3];
}

static void Camera_replay(unsigned int phase)
{
    unsigned int i;
    for (i = 0; i < camEvents; i++) {
        unsigned int ev = camEvent[i], t = ev & 0xFF;
        const unsigned char* c;
        if (ev & CAM_EV_OBJECT) {
            if (phase != 2) continue;
        } else {
            if (phase == 2) continue;
            if (((ev & CAM_EV_LATE) != 0) != (phase != 0)) continue;
        }
        c = camTrig(t);
        if (camera.mode != CAM_MODE_OBJECT) Camera_checkCamera(c);
        if (ev & CAM_EV_PASSED) Camera_triggerArms(t, c, ev);
    }
}

/* The camera's mode as the update meets it: s_checkLook, the landing's
   trigger, toCombat (updateWeapon), the late trigger, then landed blocks. */
void Camera_prepare(void)
{
    if (olLaraLooking) camera.mode = CAM_MODE_LOOK;
    Camera_replay(0);
    if (items[0].health > 0 && camera.mode != CAM_MODE_LOOK &&
        (olLaraWeaponState == WEAPON_STATE_DRAW || olLaraWeaponState == WEAPON_STATE_READY))
        camera.mode = CAM_MODE_COMBAT;
    Camera_replay(1);
    Camera_replay(2);                 /* landed blocks, after Lara's update */
    camEvents = 0;
}

/* The tail of Camera::update: lastFixed and lastIndex, and back to FOLLOW
   (unless OBJECT holds), handing lookAtItem to lastItem. At rest again when
   nothing is left for the next tick to read. */
void Camera_tail(int isFixed)
{
    olCamLastFixed = (unsigned int)isFixed;
    olCamLastIndex = olCamIndex;
    if (camera.mode != CAM_MODE_OBJECT || olCamTimer == -1) {
        camera.mode = CAM_MODE_FOLLOW;
        olCamIndex = -1;
        olCamLastItem = olCamLookAt;
        olCamLookAt = CAM_NO_ITEM;
    }
    olCamBusy = olCamLookAt != CAM_NO_ITEM || olCamLastItem != CAM_NO_ITEM || olCamLastFixed ||
                olCamIndex >= 0 || olCamLastIndex >= 0 || camera.mode != CAM_MODE_FOLLOW;
}

/* updateFixed: the camera's own place, clipped, the whole way at once;
   its timer runs down to -1, which checkCamera reads as "over". */
void Camera_updateFixed(void)
{
    Location best;
    const unsigned char* c = camAt(4 + olMul8((unsigned int)olCamIndex));
    best.x = olDataS16(c, 0);
    best.y = olDataS16(c, 2);
    best.z = olDataS16(c, 4);
    best.room = c[6];
    camera.speed = 1;
    Camera_clip(&best);
    Camera_move(&best);
    olCamFixedView = 1;
    if (olCamTimer != 0) {
        olCamTimer--;
        if (olCamTimer == 0) olCamTimer = -1;
    }
}

/* upstream's angleLerp: toward b by at most w, the short way */
static int camAngleLerp(int a, int b, int w)
{
    int d = (int)(int16)(uint16)(b - a);
    if (d > w) return (int)(int16)(uint16)(a + w);
    if (d < -w) return (int)(int16)(uint16)(a - w);
    return b;
}

#define CAM_LOOK_ANGLE_MAX      (22 * ANGLE_1)
#define CAM_LOOK_ANGLE_MIN      (-42 * ANGLE_1)
#define CAM_LOOK_ANGLE_Y        (44 * ANGLE_1)
#define CAM_LOOK_TURN_SPEED     (2 * ANGLE_1)

/* lookAt(offset): her head turns to the item, if it is within her look's
   reach, and the camera looks with her; once per item (ANIMATED). */
void Camera_lookAt(int offset)
{
    ItemObj* it = items + olCamLookAt;
    /* in sixteen bits, as every difference of wrapped positions here is */
    int32 dx = (int32)(int)(it->x - items[0].x);
    int32 dz = (int32)(int)(it->z - items[0].z);
    int ay, ax;

    ay = floorShift((int)(int16)(uint16)((unsigned int)phd_atan(dz, dx) - items[0].angleY), 1);
    if (X_ABS(ay) >= CAM_LOOK_ANGLE_Y) {
        olCamLookAt = CAM_NO_ITEM;
        return;
    }
    offset -= it->y + Camera_boxOffset(it, CAM_BOX_MIDDLE_Y);
    ax = floorShift((int)(int16)phd_atan((int32)phd_sqrt((uint32)(dx * dx + dz * dz)), (int32)offset), 1);
    if (ax < CAM_LOOK_ANGLE_MIN || ax > CAM_LOOK_ANGLE_MAX) {
        olCamLookAt = CAM_NO_ITEM;
        return;
    }
    olLaraLook.headX = (int16)camAngleLerp(olLaraLook.headX, ax, CAM_LOOK_TURN_SPEED);
    olLaraLook.headY = (int16)camAngleLerp(olLaraLook.headY, ay, CAM_LOOK_TURN_SPEED);
    olLaraLook.torsoX = olLaraLook.headX;
    olLaraLook.torsoY = olLaraLook.headY;
    it->flags2 |= ITEM_FLAG_ANIMATED;
    camera.mode = CAM_MODE_LOOK;
}

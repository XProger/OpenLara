/* OpenLara - SNES / Super FX target
 * The port of src/fixed/camera.h, follow mode.
 *
 * Upstream has seven modes -- free, follow, combat, look, fixed, object and
 * cutscene -- and a clipper that walks rooms to keep the view out of walls.
 * What is here is `updateFollow` and the two things it stands on,
 * `getLocationForAngle` and `move`, which between them are the difference
 * between a floating eye and somebody driving Lara.
 *
 * `getBestLocation` and the clipper behind it are here too now, on
 * Room_query: the view is traced from the target through the sectors between
 * them and stopped at the first wall, floor or ceiling, and kept its radius
 * off every wall. The other modes -- look, combat, fixed, cutscene -- are
 * not.
 */

#ifndef H_OL_CAMERA
#define H_OL_CAMERA

#include "fixed.h"
#include "item.h"

/* Upstream's, from camera.h. CAM_ANGLE_FOLLOW is the ten degrees the follow
   view sits above her and looks down at the target: the view is placed there
   exactly (distH across, distV up), and the renderer turns the look-down into
   a shear of its projection centre -- see "The pitch" in the README. CAM_SPEED
   is 1 << 3 upstream and the shift is what the move applies. ANGLE(x) is
   upstream's x * ANGLE_1, 182 a degree. */
#define CAM_DIST_FOLLOW     1536
#define CAM_SPEED_SHIFT     3
#define CAM_RADIUS          255
#define CAM_ANGLE_FOLLOW    (-10 * 182)
#define CAM_ANGLE_COMBAT    (-10 * 182)
#define CAM_ANGLE_MAX       (85 * 182)
#define CAM_DIST_COMBAT     2048
#define CAM_DIST_LOOK       768

/* Upstream's CameraMode, the three that exist here. game.c sets camera.mode
   before each update, as upstream's toCombat and s_checkLook do before
   Camera::update, which then puts it back to FOLLOW. */
#define CAM_MODE_FOLLOW     0
#define CAM_MODE_COMBAT     1
#define CAM_MODE_LOOK       2
#define CAM_MODE_FIXED      3   /* FX3: a trigger's fixed camera */
#define CAM_MODE_OBJECT     4   /* upstream's, for a heavy trigger; unused */
#define CAM_NO_ITEM         0xFF

#ifdef OL_FX3
/* Offsets from GBA's interpolated frame bounds, in CAMERA.BIN. */
#define CAM_BOX_FOLLOW_Y 0
#define CAM_BOX_MIDDLE_Y 1
#define CAM_BOX_MIDDLE_X 2
#define CAM_BOX_MIDDLE_Z 3
int Camera_boxOffset(ItemObj* item, unsigned int component);
#endif

typedef struct Camera {
    int          x, y, z;       /* upstream's view.pos */
    unsigned int room;
    unsigned int angleY;        /* what the renderer is handed: the yaw from
                                   the view to the target, upstream's
                                   angle.y, not her facing */
    unsigned int targetAngleY;  /* her facing: where the view wants to be */
    int          targetDist;
    int          distH, distV;  /* targetDist split by the pitch upstream's
                                   getBestLocation uses: across and up. The
                                   native scaler reads distH at camera+14 */
    unsigned int mode;          /* CAM_MODE_*, set by the caller each tick */
    unsigned int speed;         /* upstream's move() divisor: 8, 4 or 1; the
                                   native Camera_move reads it at camera+20 */
    /* What her state asks of the follow view this tick -- upstream's
       targetAngle.x/.y, targetDist and center as its handlers set them --
       read by the next update and cleared by it, as Camera::update resets
       them. 0 is "not asked": CAM_ANGLE_FOLLOW, no offset, CAM_DIST_FOLLOW. */
    int          askAngleX;     /* camera+22 */
    int          askAngleY;
    int          askDist;
    unsigned int center;        /* aim at her box's middle, turned by her yaw */
    unsigned int ask;           /* any of the four asked this tick */
} Camera;

extern Camera camera;

/* The flag saves FX3's zero-headroom creature gate the per-tick reads and
   resets; GSU-2 has the time but not the bytes for the handlers' stores. */
#ifdef OL_FX3
#define CAM_ASKED()     (camera.ask = 1)
#define CAM_ASK         camera.ask
#else
#define CAM_ASKED()     ((void)0)
#define CAM_ASK         1
#endif

void Camera_init(void);
void Camera_snap(ItemObj* item);   /* upstream's `center` on a fresh view */
void Camera_updateFollow(ItemObj* item);
void Camera_pitch(int ax, int dist);   /* camera.distH/distV for this tick */

#ifdef OL_FX3
/* Fixed cameras and camera targets (camera.c): upstream's index, lastIndex,
   timer, lookAtItem, lastItem and lastFixed. */
extern int olCamIndex, olCamLastIndex, olCamTimer;
extern unsigned int olCamLookAt, olCamLastItem, olCamLastFixed, olCamFixedView, olCamBusy;
extern unsigned int olTrigLate;
unsigned int Camera_noteTrigger(unsigned int t);
unsigned int Camera_noteTriggerObject(unsigned int t);
void Camera_passTrigger(unsigned int event, unsigned int switchOn);
void Camera_prepare(void);
void Camera_levelInit(void);
#endif

#endif

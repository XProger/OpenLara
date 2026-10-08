/* OpenLara - SNES / Super FX target
 * The camera's load and boot paths: Camera_init at power-on, Camera_snap on
 * the tick that loads the level. Neither runs while the GSU owns the ROM
 * (UpdateDemo takes it for the load tick), so on GSU-2 they are kept in ROM
 * bank $3C with olLevelInit instead of in camera.c's resident bank.
 */

#include "camera.h"
#include "room.h"

extern int fbValid, tcValid, clipValid, offValid, pitchValid;
extern Location camTarget;
extern unsigned int fbTick, fbLastTick;
int  Camera_getBestLocation(int clipFlag, Location* best);
void Camera_clip(Location* loc);
int  Camera_targetY(ItemObj* item);
void Camera_lookAtTarget(void);
int  Camera_followAngleX(void);

void Camera_init(void)
{
    camera.x = 0;
    camera.y = 0;
    camera.z = 0;
    camera.room = 0;
    camera.angleY = 0;
    camera.targetAngleY = 0;
    camera.targetDist = CAM_DIST_FOLLOW;
    camTarget.x = camTarget.y = camTarget.z = 0;
    camTarget.room = 0;
    fbValid = 0;
    fbTick = 0;
    fbLastTick = 0;
    tcValid = 0;
    clipValid = 0;
    offValid = 0;
    pitchValid = 0;
    camera.distH = CAM_DIST_FOLLOW;
    camera.mode = CAM_MODE_FOLLOW;
    camera.speed = 8;
    camera.distV = 0;
    camera.askAngleX = 0;
    camera.askAngleY = 0;
    camera.askDist = 0;
    camera.center = 0;
    camera.ask = 0;
#ifdef OL_FX3
    Camera_levelInit();                 /* the fixed cameras' rest state */
#endif
}

/* Put the view where it wants to be rather than lerping toward it. Upstream
   does this through `center` and `lastFixed` when a view is fresh, at speed
   1, which arrives at the clipped location in one update; without it the
   first dozen frames of a level are the camera flying in from wherever it
   was left, which is also a frame the two renderers cannot agree on. Traced
   and clipped like any other tick's: the unclipped location at the LEVEL1
   start is a sector inside the entrance wall, and that was the first frame
   drawn. The caches start over with the level. */
void Camera_snap(ItemObj* item)
{
    Location best;

#ifdef OL_FX3
    Camera_levelInit();
#endif

    camera.targetAngleY = item->angleY;
    camera.targetDist   = CAM_DIST_FOLLOW;
    Camera_pitch(Camera_followAngleX(), CAM_DIST_FOLLOW);

    camTarget.x    = item->x;
    camTarget.y    = Camera_targetY(item);
    camTarget.z    = item->z;
    camTarget.room = item->room;

    /* the present view, for the fallback's "nearest" -- upstream's init
       puts it a hundred units behind the target */
    camera.x    = camTarget.x;
    camera.y    = camTarget.y;
    camera.z    = camTarget.z - 100;
    camera.room = camTarget.room;

    fbValid = 0;
    tcValid = 0;
    clipValid = 0;

    Camera_getBestLocation(0, &best);
    Camera_clip(&best);

    camera.x    = best.x;
    camera.y    = best.y;
    camera.z    = best.z;
    camera.room = best.room;
    Camera_lookAtTarget();
}

#ifndef H_CAMERA
#define H_CAMERA

#include "common.h"

#define CAM_SPEED               (1 << 3)
#define CAM_ROT_SPEED           (1 << 9)
#define CAM_DIST_FOLLOW         1536
#define CAM_DIST_LOOK           768
#define CAM_DIST_COMBAT         2048
#define CAMERA_ANGLE_FOLLOW     ANGLE(-10)
#define CAMERA_ANGLE_COMBAT     ANGLE(-10)
#define CAMERA_ANGLE_MAX        ANGLE(85)

enum CameraMode
{
    CAMERA_MODE_FREE,
    CAMERA_MODE_FOLLOW,
    CAMERA_MODE_COMBAT,
    CAMERA_MODE_LOOK,
    CAMERA_MODE_FIXED,
    CAMERA_MODE_OBJECT,
    CAMERA_MODE_CUTSCENE,
};

struct Camera
{
    Location view;
    Location target;

    int32 targetDist;
    int16 targetAngleX;
    int16 targetAngleY;

    int16 angleX;
    int16 angleY;

    AABB  frustumBase;

    Item* laraItem;
    Item* lastItem;
    Item* lookAtItem;

    int32 speed;
    int32 timer;
    int32 index;
    int32 lastIndex;

    CameraMode mode;

    bool modeSwitch;
    bool lastFixed;
    bool center;

    void init(Item* lara)
    {
        ASSERT(lara->extraL);

        target.pos = lara->pos;
        target.pos.y -= 1024;
        target.room = lara->room;

        view = target;
        view.pos.z -= 100;

        angleX = 0;
        angleY = 0;

        targetAngleX = 0;
        targetAngleY = 0;
        targetDist = CAM_DIST_FOLLOW;

        laraItem = lara;
        lastItem = NULL;
        lookAtItem = NULL;

        speed = 1;
        timer = 0;
        index = -1;
        lastIndex = -1;

        mode = CAMERA_MODE_FOLLOW;

        modeSwitch = false;
        lastFixed = false;
        center = false;
    }

    void updateFree()
    {
        matrixSetView(view.pos, angleX, angleY);

        Matrix &m = matrixGet();

        if (keys & IK_UP)    angleX -= CAM_ROT_SPEED;
        if (keys & IK_DOWN)  angleX += CAM_ROT_SPEED;
        if (keys & IK_LEFT)  angleY -= CAM_ROT_SPEED;
        if (keys & IK_RIGHT) angleY += CAM_ROT_SPEED;

        angleX = X_CLAMP(angleX, -CAMERA_ANGLE_MAX, CAMERA_ANGLE_MAX);

        if (keys & IK_A)
        {
            view.pos.x += m[2].x * CAM_SPEED >> 10;
            view.pos.y += m[2].y * CAM_SPEED >> 10;
            view.pos.z += m[2].z * CAM_SPEED >> 10;
        }

        if (keys & IK_B)
        {
            view.pos.x -= m[2].x * CAM_SPEED >> 10;
            view.pos.y -= m[2].y * CAM_SPEED >> 10;
            view.pos.z -= m[2].z * CAM_SPEED >> 10;
        }

        view.room = view.room->getRoom(view.pos.x, view.pos.y, view.pos.z);
    }

    Location getLocationForAngle(int32 angle, int32 distH, int32 distV)
    {
        Location res;
        res.pos.x = target.pos.x - (distH * phd_sin(angle) >> FIXED_SHIFT);
        res.pos.y = target.pos.y + (distV);
        res.pos.z = target.pos.z - (distH * phd_cos(angle) >> FIXED_SHIFT);
        res.room = target.room;
        return res;
    }

    void clip(Location &loc)
    {
        const Sector* sector = loc.room->getSector(loc.pos.x, loc.pos.z);
        
        int32 floor = sector->getFloor(loc.pos.x, loc.pos.y, loc.pos.z);
        if (floor != WALL && loc.pos.y > floor - 128) {
            loc.pos.y = floor - 128;
        }

        int32 ceiling = sector->getCeiling(loc.pos.x, loc.pos.y, loc.pos.z);
        if (ceiling != WALL && loc.pos.y < ceiling + 128) {
            loc.pos.y = ceiling + 128;
        }

        // TODO clip walls?
    }

    Location getBestLocation(bool clip)
    {
        int32 distH = targetDist * phd_cos(targetAngleX) >> FIXED_SHIFT;
        int32 distV = targetDist * phd_sin(targetAngleX) >> FIXED_SHIFT;

        Location best = getLocationForAngle(targetAngleY, distH, distV);

        if (trace(target, best, true))
            return best;

        if (clip && best.pos != target.pos)
            return best;

        int32 distQ = X_SQR(target.pos.x - best.pos.x) + X_SQR(target.pos.z - best.pos.z);

        if (distQ > X_SQR(768))
            return best;

        int32 minDistQ = INT_MAX;

        for (int32 i = 0; i < 4; i++)
        {
            Location tmpDest = getLocationForAngle(i * ANGLE_90, distH, distV);
            Location tmpView = view;

            if (!trace(target, tmpDest, true) || !trace(tmpDest, tmpView, false))
                continue;

            distQ = X_SQR(view.pos.x - tmpDest.pos.x) + X_SQR(view.pos.z - tmpDest.pos.z);

            if (distQ < minDistQ)
            {
                minDistQ = distQ;
                best = tmpDest;
            }
        }

        return best;
    }

    void move(Location &to, int32 speed)
    {
        clip(to);

        vec3i d = to.pos - view.pos;

        if (speed > 1) {
            d /= speed;
        }

        view.pos += d;
        view.room = to.room->getRoom(view.pos.x, view.pos.y, view.pos.z);
    }

    void updateFollow(Item* item)
    {
        if (targetAngleX == 0) {
            targetAngleX = CAMERA_ANGLE_FOLLOW;
        }

        targetAngleX = X_CLAMP(targetAngleX + item->angle.x, -CAMERA_ANGLE_MAX, CAMERA_ANGLE_MAX);
        targetAngleY += item->angle.y;

        Location best = getBestLocation(false);

        move(best, lastFixed ? speed : 12);
    }

    void updateCombat(Item* item)
    {
        ASSERT(item->type == ITEM_LARA);

        targetAngleX = item->angle.x + CAMERA_ANGLE_COMBAT;
        targetAngleY = item->angle.y;
        
        if (item->extraL->armR.target || item->extraL->armL.target)
        {
            int32 aX = item->extraL->armR.angleAim.x + item->extraL->armL.angleAim.x;
            int32 aY = item->extraL->armR.angleAim.y + item->extraL->armL.angleAim.y;

            if (item->extraL->armR.target && item->extraL->armL.target) {
                targetAngleX += aX >> 1;
                targetAngleY += aY >> 1;
            } else {
                targetAngleX += aX;
                targetAngleY += aY;
            }
        } else {
            targetAngleX += item->extraL->head.angle.x + item->extraL->torso.angle.x;
            targetAngleY += item->extraL->head.angle.y + item->extraL->torso.angle.y;
        }

        targetDist = CAM_DIST_COMBAT;

        Location best = getBestLocation(true);

        move(best, speed);
    }

    void updateLook(Item* item)
    {
        ASSERT(item->type == ITEM_LARA);

        targetAngleX = item->extraL->head.angle.x + item->extraL->torso.angle.x + item->angle.x;
        targetAngleY = item->extraL->head.angle.y + item->extraL->torso.angle.y + item->angle.y;
        targetDist = lookAtItem ? CAM_DIST_FOLLOW : CAM_DIST_LOOK;

        Location best = getBestLocation(true);

        move(best, speed);
    }

    void updateFixed()
    {
        const FixedCamera* cam = cameras + index;

        Location best;
        best.pos = cam->pos;
        best.room = rooms + cam->roomIndex;

        lastFixed = true;
        move(best, 1);

        if (timer != 0)
        {
            timer--;
            if (timer == 0) {
                timer = -1;
            }
        }
    }

    void lookAt(int32 offset)
    {
        int32 dx = lookAtItem->pos.x - laraItem->pos.x;
        int32 dz = lookAtItem->pos.z - laraItem->pos.z;

        int16 ay = int16(phd_atan(dz, dx) - laraItem->angle.y) >> 1;

        if (abs(ay) >= LARA_LOOK_ANGLE_Y)
        {
            lookAtItem = NULL;
            return;
        }

        const Bounds& box = lookAtItem->getBoundingBox(true);

        offset -= lookAtItem->pos.y + ((box.minY + box.maxY) >> 1);

        int16 ax = int16(phd_atan(phd_sqrt(X_SQR(dx) + X_SQR(dz)), offset)) >> 1;

        if (ax < LARA_LOOK_ANGLE_MIN || ax > LARA_LOOK_ANGLE_MAX)
        {
            lookAtItem = NULL;
            return;
        }

        laraItem->extraL->head.angle.x = angleLerp(laraItem->extraL->head.angle.x, ax, LARA_LOOK_TURN_SPEED);
        laraItem->extraL->head.angle.y = angleLerp(laraItem->extraL->head.angle.y, ay, LARA_LOOK_TURN_SPEED);

        laraItem->extraL->torso.angle = laraItem->extraL->head.angle;

        lookAtItem->flags.animated = true; // use as once flag
        mode = CAMERA_MODE_LOOK;
    }

    void update()
    {
        if (keys & IK_START)
        {
            if (!modeSwitch)
            {
                modeSwitch = true;
                if (mode != CAMERA_MODE_FREE) {
                    mode = CAMERA_MODE_FREE;
                } else {
                    mode = CAMERA_MODE_FOLLOW;
                }
            }
        } else {
            modeSwitch = false;
        }

        if (mode == CAMERA_MODE_FREE)
        {
            updateFree();
            prepareFrustum();
            matrixSetView(view.pos, angleX, angleY);
            return;
        }

        bool isFixed = false;
        Item* item = laraItem;

        if (lookAtItem && (mode == CAMERA_MODE_FIXED || mode == CAMERA_MODE_OBJECT))
        {
            isFixed = true;
            item = lookAtItem;
        }

        ASSERT(item);

        target.room = item->room;
        target.pos.x = item->pos.x;
        target.pos.z = item->pos.z;

        const Bounds &box = item->getBoundingBox(true);

        int32 y = item->pos.y;
        if (isFixed) {
            y += (box.minY + box.maxY) >> 1;
        } else {
            y += box.maxY + ((box.minY - box.maxY) * 3 >> 2);
        }

        if (!isFixed && lookAtItem) {
            lookAt(y);
        }

        if (mode == CAMERA_MODE_LOOK || mode == CAMERA_MODE_COMBAT)
        {
            y -= 256;

            if (lastFixed) {
                target.pos.y = 0;
                speed = 1;
            } else {
                target.pos.y += (y - target.pos.y) >> 2;
                speed = (mode == CAMERA_MODE_LOOK) ? 4 : 8;
            }

        } else {

            if (center)
            {
                int32 offset = (box.minZ + box.maxZ) >> 1;
                target.pos.x += (phd_sin(item->angle.y) * offset) >> FIXED_SHIFT;
                target.pos.z += (phd_cos(item->angle.y) * offset) >> FIXED_SHIFT;
            }

            lastFixed ^= isFixed;

            if (lastFixed) {
                target.pos.y = y;
                speed = 1;
            } else {
                target.pos.y += (y - target.pos.y) >> 2;
            }
        }

        switch (mode)
        {
            case CAMERA_MODE_FOLLOW : updateFollow(item); break;
            case CAMERA_MODE_COMBAT : updateCombat(item); break;
            case CAMERA_MODE_LOOK   : updateLook(item); break;
            default                 : updateFixed();
        }

        lastFixed = isFixed;
        lastIndex = index;

        if (mode != CAMERA_MODE_OBJECT || timer == -1)
        {
            mode = CAMERA_MODE_FOLLOW;
            index = -1;
            lastItem = lookAtItem;
            lookAtItem = NULL;
            targetAngleX = 0;
            targetAngleY = 0;
            targetDist = CAM_DIST_FOLLOW;
            center = false;
        }

        vec3i dir = target.pos - view.pos;
        anglesFromVector(dir.x, dir.y, dir.z, angleX, angleY);

        prepareFrustum();

        matrixSetView(view.pos, angleX, angleY);
    }

    void prepareFrustum()
    {
        matrixSetIdentity();
        matrixRotateYXZ(angleX, angleY, 0);

        static const vec3i v[5] = {
        // near plane
            vec3i( 0, 0, 0 ),
        // far plane
            vec3i( -FRUSTUM_FAR_X, -FRUSTUM_FAR_Y, FRUSTUM_FAR_Z ),
            vec3i(  FRUSTUM_FAR_X, -FRUSTUM_FAR_Y, FRUSTUM_FAR_Z ),
            vec3i( -FRUSTUM_FAR_X,  FRUSTUM_FAR_Y, FRUSTUM_FAR_Z ),
            vec3i(  FRUSTUM_FAR_X,  FRUSTUM_FAR_Y, FRUSTUM_FAR_Z )
        };

        const Matrix &m = matrixGet();

        frustumBase.minX =  0xFFFFFFF;
        frustumBase.maxX = -0xFFFFFFF;
        frustumBase.minY =  0xFFFFFFF;
        frustumBase.maxY = -0xFFFFFFF;
        frustumBase.minZ =  0xFFFFFFF;
        frustumBase.maxZ = -0xFFFFFFF;

        for (int32 i = 0; i < 5; i++)
        {
            int32 x = DP43(m[0], v[i]) >> FIXED_SHIFT;
            int32 y = DP43(m[1], v[i]) >> FIXED_SHIFT;
            int32 z = DP43(m[2], v[i]) >> FIXED_SHIFT;

            frustumBase.minX = X_MIN(frustumBase.minX, x);
            frustumBase.maxX = X_MAX(frustumBase.maxX, x);
            frustumBase.minY = X_MIN(frustumBase.minY, y);
            frustumBase.maxY = X_MAX(frustumBase.maxY, y);
            frustumBase.minZ = X_MIN(frustumBase.minZ, z);
            frustumBase.maxZ = X_MAX(frustumBase.maxZ, z);
        }

        frustumBase.minX += view.pos.x - 1024;
        frustumBase.maxX += view.pos.x + 1024;
        frustumBase.minY += view.pos.y - 1024;
        frustumBase.maxY += view.pos.y + 1024;
        frustumBase.minZ += view.pos.z - 1024;
        frustumBase.maxZ += view.pos.z + 1024;
    }

    void updateFrustum(int32 offsetX, int32 offsetY, int32 offsetZ)
    {
        frustumAABB.minX = frustumBase.minX - offsetX;
        frustumAABB.maxX = frustumBase.maxX - offsetX;
        frustumAABB.minY = frustumBase.minY - offsetY;
        frustumAABB.maxY = frustumBase.maxY - offsetY;
        frustumAABB.minZ = frustumBase.minZ - offsetZ;
        frustumAABB.maxZ = frustumBase.maxZ - offsetZ;
    }

    void toCombat()
    {
        if (mode == CAMERA_MODE_FREE)
            return;

        if (mode == CAMERA_MODE_CUTSCENE)
            return;

        if (mode == CAMERA_MODE_LOOK)
            return;

        mode = CAMERA_MODE_COMBAT;
    }
};

Camera* gCamera;
Camera viewCameras[MAX_PLAYERS];

#endif

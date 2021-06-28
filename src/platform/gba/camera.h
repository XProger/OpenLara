#ifndef H_CAMERA
#define H_CAMERA

#include "common.h"

#define CAM_SPEED               (1 << 3)
#define CAM_ROT_SPEED           (1 << 9)
#define CAM_DIST_FOLLOW         (1024 + 512)
#define CAMERA_ANGLE_FOLLOW    -10 * DEG2SHORT
#define CAMERA_ANGLE_MAX        85 * DEG2SHORT
#define CAMERA_TRACE_SHIFT      3
#define CAMERA_TRACE_STEPS      (1 << CAMERA_TRACE_SHIFT)

enum CameraMode
{
    CAMERA_MODE_FREE,
    CAMERA_MODE_FOLLOW,
    CAMERA_MODE_COMBAT,
    CAMERA_MODE_LOOK,
    CAMERA_MODE_FIXED,
    CAMERA_MODE_OBJECT,
};

struct Camera
{
    struct Location {
        Room* room;
        vec3i pos;
    };

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
    }

    void updateRoom()
    {
        view.room = view.room->getRoom(view.pos.x, view.pos.y, view.pos.z);

        const Sector* sector = view.room->getSector(view.pos.x, view.pos.z);
        int32 floor = sector->getFloor(view.pos.x, view.pos.y, view.pos.z);
        int32 ceiling = sector->getCeiling(view.pos.x, view.pos.y, view.pos.z) - 256;
        
        if (floor != WALL) {
            view.pos.y = X_MIN(view.pos.y, floor - 256);
        }

        if (ceiling != WALL)
        {
            view.pos.y = X_MAX(view.pos.y, ceiling + 256);
        }
    }

    int32 traceX()
    {
        return 1;
    }

    int32 traceZ()
    {
        return 1;
    }

    bool trace()
    {
        int32 dx = abs(view.pos.x - target.pos.x);
        int32 dz = abs(view.pos.z - target.pos.z);

        int32 tx, tz;

        if (dx < dz) {
            tx = traceX();
            tz = traceZ();
            if (tz == 0) return false;
        } else {
            tz = traceZ();
            tx = traceX();
            if (tx == 0) return false;
        }

        return true;
    }

    bool trace(const Location &from, Location &to, int32 radius)
    {
        vec3i d = to.pos - from.pos;
        d.x >>= CAMERA_TRACE_SHIFT;
        d.y >>= CAMERA_TRACE_SHIFT;
        d.z >>= CAMERA_TRACE_SHIFT;

        Room* room = from.room;
        vec3i pos = from.pos;
        int32 i;

        for (i = 0; i < CAMERA_TRACE_STEPS; i++)
        {
            if (radius)
            {
                to.pos = pos;
                to.room = room;
            }

            pos += d;
            room = room->getRoom(pos.x, pos.y, pos.z);

            const Sector* sector = room->getSector(pos.x, pos.z);
            int32 floor = sector->getFloor(pos.x, pos.y, pos.z);
            int32 ceiling = sector->getCeiling(pos.x, pos.y, pos.z);

            if (floor == WALL || ceiling == WALL || ceiling >= floor)
            {
                return false;
            }

            int32 h = pos.y - floor;
            if (h > 0)
            {
                if (h >= radius) {
                    return false;
                }
                pos.y = floor;
            }

            h = ceiling - pos.y;
            if (h > 0)
            {
                if (h >= radius) {
                    return false;
                }
                pos.y = ceiling;
            }
        }

        to.pos = pos;
        to.room = room;

        return true;
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

    Location getBestLocation(Item* item)
    {
        int32 distH = targetDist * phd_cos(targetAngleX) >> FIXED_SHIFT;
        int32 distV = targetDist * phd_sin(targetAngleX) >> FIXED_SHIFT;

        Location best = getLocationForAngle(targetAngleY + item->angle.y, distH, distV);

        if (trace(target, best, 200))
            return best;

        int32 distQ = X_SQR(target.pos.x - best.pos.x) + X_SQR(target.pos.z - best.pos.z);

        if (distQ > X_SQR(768))
            return best;

        int32 minDistQ = INT_MAX;

        for (int32 i = 0; i < 4; i++)
        {
            Location tmp = getLocationForAngle(i * ANGLE_90, distH, distV);

            if (!trace(target, tmp, 200) || !trace(tmp, view, 0)) {
                continue;
            }

            distQ = X_SQR(view.pos.x - tmp.pos.x) + X_SQR(view.pos.z - tmp.pos.z);

            if (distQ < minDistQ)
            {
                minDistQ = distQ;
                best = tmp;
            }
        }

        return best;
    }

    void move(const Location &to, int32 speed)
    {
        vec3i d = to.pos - view.pos;
        if (speed > 1)
        {
            d.x /= speed;
            d.y /= speed;
            d.z /= speed;
        }

        view.pos += d;
        view.room =  to.room->getRoom(view.pos.x, view.pos.y, view.pos.z);
    }

    void updateFollow(Item* item)
    {
        if (targetAngleX == 0) {
            targetAngleX = CAMERA_ANGLE_FOLLOW;
        }

        targetAngleX = X_CLAMP(targetAngleX + item->angle.x, -CAMERA_ANGLE_MAX, CAMERA_ANGLE_MAX);

        Location best = getBestLocation(item);

        move(best, lastFixed ? speed : 12);
    }

    void updateCombat()
    {
        //
    }

    void updateLook()
    {
        //
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
            updateRoom();
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

        const Bounds &box = item->getBoundingBox();

        int32 y = item->pos.y;
        if (isFixed) {
            y += (box.minY + box.maxY) >> 1;
        } else {
            y += box.maxY + ((box.minY - box.maxY) * 3 >> 2);
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
            target.pos.x = item->pos.x;
            target.pos.z = item->pos.z;

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
            case CAMERA_MODE_COMBAT : updateCombat(); break;
            case CAMERA_MODE_LOOK   : updateLook(); break;
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

        updateRoom();
    }

    void prepareFrustum()
    {
        matrixSetIdentity();
        matrixRotateY(angleY);
        matrixRotateX(angleX);

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
};

Camera camera;

#endif

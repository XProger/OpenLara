#ifndef H_CAMERA
#define H_CAMERA

#include "common.h"

#define CAM_SPEED           (1 << 3)
#define CAM_ROT_SPEED       (1 << 9)
#define CAM_ROT_X_MAX       int16(85 * 0x8000 / 180)
#define CAM_DIST_FOLLOW     (1024 + 512)

enum CameraMode
{
    CAMERA_MODE_FREE   = 0,
    CAMERA_MODE_FOLLOW = 1,
    CAMERA_MODE_COMBAT = 2,
    CAMERA_MODE_FIXED  = 3,
    CAMERA_MODE_OBJECT = 4,
};

struct Camera
{
    struct Location {
        Room* room;
        vec3i pos;
    };

    Location view;
    Location target;

    int16 targetAngleX;
    int16 targetAngleY;
    int16 targetDist;

    int16 angleX;
    int16 angleY;

    AABB  frustumBase;

    Item* item;

    CameraMode mode;
    bool modeSwitch;

    void init()
    {
        item = NULL;
        mode = CAMERA_MODE_FOLLOW;
        modeSwitch = false;

        angleX = 0;
        angleY = 0;

        view.pos = vec3i(0);
        target.pos = view.pos;
        targetDist = CAM_DIST_FOLLOW;
        targetAngleX = 0;
        targetAngleY = 0;
    }

    void freeControl()
    {
        matrixSetView(view.pos, angleX, angleY);

        Matrix &m = matrixGet();

        if (keys & IK_UP)    angleX -= CAM_ROT_SPEED;
        if (keys & IK_DOWN)  angleX += CAM_ROT_SPEED;
        if (keys & IK_LEFT)  angleY -= CAM_ROT_SPEED;
        if (keys & IK_RIGHT) angleY += CAM_ROT_SPEED;

        angleX = X_CLAMP(angleX, -CAM_ROT_X_MAX, CAM_ROT_X_MAX);

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

        const RoomInfo::Sector* sector = view.room->getSector(view.pos.x, view.pos.z);
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
            freeControl();
        }

        if (mode == CAMERA_MODE_FOLLOW && item)
        {
            int32 tx = item->pos.x;
            int32 ty = item->pos.y;
            int32 tz = item->pos.z;

            const Box &box = item->getBoundingBox();
            ty += box.maxY + ((box.minY - box.maxY) * 3 >> 2);

            target.pos.x = tx;
            target.pos.y += (ty - target.pos.y) >> 2;
            target.pos.z = tz;

            int16 angle = item->angleY + targetAngleY;

            int32 dy = targetDist * phd_sin(targetAngleX) >> FIXED_SHIFT;
            int32 dz = targetDist * phd_cos(targetAngleX) >> FIXED_SHIFT;

            int32 cx = target.pos.x - (phd_sin(angle) * dz >> FIXED_SHIFT);
            int32 cy = target.pos.y - 256 + dy;
            int32 cz = target.pos.z - (phd_cos(angle) * dz >> FIXED_SHIFT);

            view.pos.x += (cx - view.pos.x) >> 2;
            view.pos.y += (cy - view.pos.y) >> 2;
            view.pos.z += (cz - view.pos.z) >> 2;

            updateRoom();

            vec3i dir = target.pos - view.pos;
            anglesFromVector(dir.x, dir.y, dir.z, angleX, angleY);
        }

        prepareFrustum();

        matrixSetView(view.pos, angleX, angleY);

        updateRoom();

    // reset additional angles, Lara states can override it during the update proc
        targetAngleX = 0;
        targetAngleY = 0;
        targetDist = CAM_DIST_FOLLOW;
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

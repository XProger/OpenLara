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
};

struct Camera
{
    vec3i viewPos;
    vec3i targetPos;

    int16 targetAngleX;
    int16 targetAngleY;
    int16 targetDist;

    int16 angleX;
    int16 angleY;
    int32 room;

    Item* item;

    CameraMode mode;
    bool modeSwitch;

    void init()
    {
        item = NULL;
        mode = CAMERA_MODE_FOLLOW;
        modeSwitch = false;

        viewPos.x = 75162;
        viewPos.y = 2048;
        viewPos.z = 5000;

        targetPos = viewPos;
        targetAngleX = 0;
        targetAngleY = 0;
        targetDist = CAM_DIST_FOLLOW;

        angleX = 0;
        angleY = 16 << 8;

        //angleX = -0x1000;
        //angleY = int16(0x8000);
    }

    void freeControl()
    {
        matrixSetView(viewPos, angleX, angleY);

        Matrix &m = matrixGet();

        if (keys & IK_UP)    angleX -= CAM_ROT_SPEED;
        if (keys & IK_DOWN)  angleX += CAM_ROT_SPEED;
        if (keys & IK_LEFT)  angleY -= CAM_ROT_SPEED;
        if (keys & IK_RIGHT) angleY += CAM_ROT_SPEED;

        angleX = X_CLAMP(angleX, -CAM_ROT_X_MAX, CAM_ROT_X_MAX);

        if (keys & IK_A)
        {
            viewPos.x += m[2].x * CAM_SPEED >> 10;
            viewPos.y += m[2].y * CAM_SPEED >> 10;
            viewPos.z += m[2].z * CAM_SPEED >> 10;
        }

        if (keys & IK_B)
        {
            viewPos.x -= m[2].x * CAM_SPEED >> 10;
            viewPos.y -= m[2].y * CAM_SPEED >> 10;
            viewPos.z -= m[2].z * CAM_SPEED >> 10;
        }
    }

    void updateRoom()
    {
        room = getRoomIndex(room, viewPos.x, viewPos.y, viewPos.z);
        const RoomInfo::Sector* sector = getSector(room, viewPos.x, viewPos.z);
        int32 floor = getFloor(sector, viewPos.x, viewPos.y, viewPos.z);
        int32 ceiling = getCeiling(sector, viewPos.x, viewPos.y, viewPos.z) - 256;
        
        if (floor != WALL) {
            viewPos.y = X_MIN(viewPos.y, floor - 256);
        }

        if (ceiling != WALL)
        {
            viewPos.y = X_MAX(viewPos.y, ceiling + 256);
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
            freeControl();
        }

        if (mode == CAMERA_MODE_FOLLOW && item) {
            int32 tx = item->pos.x;
            int32 ty = item->pos.y;
            int32 tz = item->pos.z;

            const Box &box = getBoundingBox(item);
            ty += box.maxY + ((box.minY - box.maxY) * 3 >> 2);

            targetPos.x = tx;
            targetPos.y += (ty - targetPos.y) >> 2;
            targetPos.z = tz;

            int16 angle = item->angleY + targetAngleY;

            int32 dy = targetDist * phd_sin(targetAngleX) >> FIXED_SHIFT;
            int32 dz = targetDist * phd_cos(targetAngleX) >> FIXED_SHIFT;

            int32 cx = targetPos.x - (phd_sin(angle) * dz >> FIXED_SHIFT);
            int32 cy = targetPos.y - 256 + dy;
            int32 cz = targetPos.z - (phd_cos(angle) * dz >> FIXED_SHIFT);

            viewPos.x += (cx - viewPos.x) >> 2;
            viewPos.y += (cy - viewPos.y) >> 2;
            viewPos.z += (cz - viewPos.z) >> 2;

            updateRoom();

            anglesFromVector(targetPos.x - viewPos.x, targetPos.y - viewPos.y, targetPos.z - viewPos.z, angleX, angleY);
        }

        matrixSetView(viewPos, angleX, angleY);

        updateRoom();

        targetAngleX = 0;
        targetAngleY = 0;
        targetDist = CAM_DIST_FOLLOW;
    }
};

Camera camera;

#endif
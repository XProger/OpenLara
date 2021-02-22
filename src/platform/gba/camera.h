#ifndef H_CAMERA
#define H_CAMERA

#include "common.h"

#define CAM_SPEED     (1 << 3)
#define CAM_ROT_SPEED (1 << 9)
#define CAM_ROT_X_MAX int16(85 * 0x8000 / 180)

struct Camera {
    vec3i pos;
    int16 rotX, rotY;
    int32 room;

    void init() {
        pos.x = 75162;
        pos.y = 2048;
        pos.z = 5000;

        rotX = 0;
        rotY = 16 << 8;

        //rotX = -0x1000;
        //rotY = int16(0x8000);
    }

    void update() {
        if (keys[IK_UP])    rotX -= CAM_ROT_SPEED;
        if (keys[IK_DOWN])  rotX += CAM_ROT_SPEED;
        if (keys[IK_LEFT])  rotY -= CAM_ROT_SPEED;
        if (keys[IK_RIGHT]) rotY += CAM_ROT_SPEED;

        rotX = clamp(rotX, -CAM_ROT_X_MAX, CAM_ROT_X_MAX);

        matrixSetView(pos.x, pos.y, pos.z, rotX, rotY);

        Matrix &m = matrixGet();

        if (keys[IK_R]) {
            pos.x += m[0].x * CAM_SPEED >> 10;
            pos.y += m[0].y * CAM_SPEED >> 10;
            pos.z += m[0].z * CAM_SPEED >> 10;
        }

        if (keys[IK_L]) {
            pos.x -= m[0].x * CAM_SPEED >> 10;
            pos.y -= m[0].y * CAM_SPEED >> 10;
            pos.z -= m[0].z * CAM_SPEED >> 10;
        }

        if (keys[IK_A]) {
            pos.x += m[2].x * CAM_SPEED >> 10;
            pos.y += m[2].y * CAM_SPEED >> 10;
            pos.z += m[2].z * CAM_SPEED >> 10;
        }

        if (keys[IK_B]) {
            pos.x -= m[2].x * CAM_SPEED >> 10;
            pos.y -= m[2].y * CAM_SPEED >> 10;
            pos.z -= m[2].z * CAM_SPEED >> 10;
        }

        room = getRoomIndex(room, &pos);
    }
};

Camera camera;

#endif
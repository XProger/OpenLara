#ifndef H_CAMERA
#define H_CAMERA

#include "common.h"

uint16 camRotY = 16 << 8;

int32 camSinY;
int32 camCosY;

int32 camX = 75162;
int32 camY = 3072 - 1024;
int32 camZ = 5000;

Rect clip;

#ifdef _WIN32
    #define CAM_SPEED     (1 << 2)
    #define CAM_ROT_SPEED (1 << 2)
#else
    #define CAM_SPEED     (1 << 6)
    #define CAM_ROT_SPEED (1 << 8)
#endif

void updateCamera() {
    if (keys[IK_LEFT])  camRotY -= CAM_ROT_SPEED;
    if (keys[IK_RIGHT]) camRotY += CAM_ROT_SPEED;

    {
        ALIGN4 ObjAffineSource src;
        ALIGN4 ObjAffineDest   dst;

        src.sX    = 0x0100;
        src.sY    = 0x0100;
        src.theta = camRotY;

        ObjAffineSet(&src, &dst, 1, 2);

        camCosY = dst.pd << 8;
        camSinY = dst.pc << 8;
    }

    int32 dx = camSinY;
    int32 dz = camCosY;

    dx *= CAM_SPEED;
    dz *= CAM_SPEED;

    dx >>= 16;
    dz >>= 16;

    if (keys[IK_UP]) {
        camX += int32(dx);
        camZ += int32(dz);
    }

    if (keys[IK_DOWN]) {
        camX -= int32(dx);
        camZ -= int32(dz);
    }

    if (keys[IK_L]) {
        camX -= int32(dz);
        camZ += int32(dx);
    }

    if (keys[IK_R]) {
        camX += int32(dz);
        camZ -= int32(dx);
    }

    if (keys[IK_A]) camY -= CAM_SPEED;
    if (keys[IK_B]) camY += CAM_SPEED;

    clip = { 0, 0, FRAME_WIDTH, FRAME_HEIGHT };
}

#endif
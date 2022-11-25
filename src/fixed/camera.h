#ifndef H_CAMERA
#define H_CAMERA

#include "common.h"

#define CAM_SPEED               (1 << 3)
#define CAM_ROT_SPEED           (1 << 9)
#define CAM_DIST_FOLLOW         1536
#define CAM_DIST_LOOK           768
#define CAM_DIST_COMBAT         2048
#define CAM_RADIUS              255
#define CAM_ANGLE_FOLLOW        ANGLE(-10)
#define CAM_ANGLE_COMBAT        ANGLE(-10)
#define CAM_ANGLE_MAX           ANGLE(85)

void Camera::initCinematic()
{
    switch (gLevelID)
    {
        case LVL_TR1_CUT_1:
            gCinematicCamera.view.pos.x = 36668;
            gCinematicCamera.view.pos.z = 63180;
            gCinematicCamera.targetAngle.y = -ANGLE(128);
            break;
        case LVL_TR1_CUT_2:
            gCinematicCamera.view.pos.x = 51962;
            gCinematicCamera.view.pos.z = 53760;
            gCinematicCamera.targetAngle.y = ANGLE_90 - 4;
            break;
        case LVL_TR1_CUT_3:
            gCinematicCamera.targetAngle.y = ANGLE_90;
            //level.flip();
            break;
        case LVL_TR1_CUT_4:
            gCinematicCamera.targetAngle.y = ANGLE_90;
            break;
        default:
            ASSERT(false);
            break;
    }
}

void Camera::updateCinematic()
{
    const CameraFrame &frame = level.cameraFrames[timer++];

    int32 px = frame.pos.x;
    int32 py = frame.pos.y;
    int32 pz = frame.pos.z;

    int32 dx = frame.target.x - px;
    int32 dy = frame.target.y - py;
    int32 dz = frame.target.z - pz;

    anglesFromVector(dx, dy, dz, angle.x, angle.y);

    int32 s, c;
    sincos(targetAngle.y, s, c);

    X_ROTXY(pz, px, s, c);

    px += view.pos.x;
    py += view.pos.y;
    pz += view.pos.z;

    matrixSetView(_vec3i(px, py, pz), angle.x, angle.y + targetAngle.y);

    Room* nextRoom = view.room->getRoom(px, py, pz);
    if (nextRoom) {
        view.room = nextRoom;
    }

    if (timer >= level.cameraFramesCount) {
        timer = level.cameraFramesCount - 1;
        nextLevel(LevelID(gLevelID + 1));
    }
}

void Camera::init(ItemObj* lara)
{
    ASSERT(lara->extraL);

    target.pos = lara->pos;
    target.pos.y -= 1024;
    target.room = lara->room;

    view = target;
    view.pos.z -= 100;

    targetDist = CAM_DIST_FOLLOW;
    targetAngle = _vec3s(0, 0, 0);
    angle = _vec3s(0, 0, 0);

    laraItem = lara;
    lastItem = NULL;
    lookAtItem = NULL;

    speed = 1;
    timer = 0;
    index = -1;
    lastIndex = -1;

    mode = CAMERA_MODE_FOLLOW;

    lastFixed = false;
    center = false;
}

Location Camera::getLocationForAngle(int32 angle, int32 distH, int32 distV)
{
    int32 s, c;
    sincos(angle, s, c);

    Location res;
    res.pos.x = target.pos.x - (distH * s >> FIXED_SHIFT);
    res.pos.y = target.pos.y + (distV);
    res.pos.z = target.pos.z - (distH * c >> FIXED_SHIFT);
    res.room = target.room;
    return res;
}

bool checkWall(Room* room, int32 x, int32 y, int32 z)
{
    Room* nextRoom = room->getRoom(x, y, z);

    const Sector* sector = nextRoom->getSector(x, z);
    int32 floor = sector->getFloor(x, y, z);
    int32 ceiling = sector->getCeiling(x, y, z);
    return (floor == WALL || ceiling == WALL || ceiling >= floor || y > floor || y < ceiling);
}

int32 checkHeight(Room* room, int32 x, int32 y, int32 z)
{
    Room* nextRoom = room->getRoom(x, y, z);
    const Sector* sector = nextRoom->getSector(x, z);
    int32 floor = sector->getFloor(x, y, z);
    int32 ceiling = sector->getCeiling(x, y, z);

    if (floor != WALL && ceiling != WALL && ceiling < floor)
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

void Camera::clip(Location &loc)
{
    loc.pos.y = checkHeight(loc.room, loc.pos.x, loc.pos.y, loc.pos.z);

    if (checkWall(loc.room, loc.pos.x - CAM_RADIUS, loc.pos.y, loc.pos.z)) {
        loc.pos.x = (loc.pos.x & (~1023)) + CAM_RADIUS;
    }

    if (checkWall(loc.room, loc.pos.x + CAM_RADIUS, loc.pos.y, loc.pos.z)) {
        loc.pos.x = (loc.pos.x | 1023) - CAM_RADIUS;
    }

    if (checkWall(loc.room, loc.pos.x, loc.pos.y, loc.pos.z - CAM_RADIUS)) {
        loc.pos.z = (loc.pos.z & (~1023)) + CAM_RADIUS;
    }

    if (checkWall(loc.room, loc.pos.x, loc.pos.y, loc.pos.z + CAM_RADIUS)) {
        loc.pos.z = (loc.pos.z | 1023) - CAM_RADIUS;
    }

    loc.room = loc.room->getRoom(loc.pos.x, loc.pos.y, loc.pos.z);
}

Location Camera::getBestLocation(bool clip)
{
    int32 s, c;
    sincos(targetAngle.x, s, c);

    int32 distH = targetDist * c >> FIXED_SHIFT;
    int32 distV = targetDist * s >> FIXED_SHIFT;

    Location best = getLocationForAngle(targetAngle.y, distH, distV);

    if (trace(target, best, true))
        return best;

    if (clip && best.pos != target.pos)
        return best;

    int32 dist = fastLength(target.pos.x - best.pos.x, target.pos.z - best.pos.z);

    if (dist > 768)
        return best;

    int32 minDist = INT_MAX;

    for (int32 i = 0; i < 4; i++)
    {
        Location tmpDest = getLocationForAngle(i * ANGLE_90, distH, distV);
        Location tmpView = view;

        if (!trace(target, tmpDest, true) || !trace(tmpDest, tmpView, false))
            continue;

        dist = fastLength(view.pos.x - tmpDest.pos.x, view.pos.z - tmpDest.pos.z);

        if (dist < minDist)
        {
            minDist = dist;
            best = tmpDest;
        }
    }

    return best;
}

void Camera::move(Location &to, int32 speed)
{
    clip(to);

    vec3i d = to.pos - view.pos;

    if (speed > 1)
    {
        if (speed == 8)
        {
            d.x >>= 3;
            d.y >>= 3;
            d.z >>= 3;
        }
        else
        {
            d.x /= speed;
            d.y /= speed;
            d.z /= speed;
        }
    }

    view.pos += d;
    view.room = to.room->getRoom(view.pos.x, view.pos.y, view.pos.z);

    const Sector* sector = view.room->getSector(view.pos.x, view.pos.z);

    int32 floor = sector->getFloor(view.pos.x, view.pos.y, view.pos.z) - 256;
    if (view.pos.y >= floor && to.pos.y >= floor)
    {
        trace(target, view, true);
        view.room = view.room->getRoom(view.pos.x, view.pos.y, view.pos.z);
    }
}

void Camera::updateFree()
{
    matrixSetView(view.pos, angle.x, angle.y);

    Matrix &m = matrixGet();

    if (keys & IK_UP)    angle.x -= CAM_ROT_SPEED;
    if (keys & IK_DOWN)  angle.x += CAM_ROT_SPEED;
    if (keys & IK_LEFT)  angle.y -= CAM_ROT_SPEED;
    if (keys & IK_RIGHT) angle.y += CAM_ROT_SPEED;

    angle.x = X_CLAMP(angle.x, -CAM_ANGLE_MAX, CAM_ANGLE_MAX);

    if (keys & IK_A)
    {
        view.pos.x += m.e20 * CAM_SPEED >> 10;
        view.pos.y += m.e21 * CAM_SPEED >> 10;
        view.pos.z += m.e22 * CAM_SPEED >> 10;
    }

    if (keys & IK_B)
    {
        view.pos.x -= m.e20 * CAM_SPEED >> 10;
        view.pos.y -= m.e21 * CAM_SPEED >> 10;
        view.pos.z -= m.e22 * CAM_SPEED >> 10;
    }

    if (keys & IK_R)
    {
        view.pos.x += m.e00 * CAM_SPEED >> 10;
        view.pos.y += m.e01 * CAM_SPEED >> 10;
        view.pos.z += m.e02 * CAM_SPEED >> 10;
    }

    if (keys & IK_L)
    {
        view.pos.x -= m.e00 * CAM_SPEED >> 10;
        view.pos.y -= m.e01 * CAM_SPEED >> 10;
        view.pos.z -= m.e02 * CAM_SPEED >> 10;
    }

    view.room = view.room->getRoom(view.pos.x, view.pos.y, view.pos.z);
}

void Camera::updateFollow(ItemObj* item)
{
    if (targetAngle.x == 0) {
        targetAngle.x = CAM_ANGLE_FOLLOW;
    }

    targetAngle.x = X_CLAMP(targetAngle.x + item->angle.x, -CAM_ANGLE_MAX, CAM_ANGLE_MAX);
    targetAngle.y += item->angle.y;

    Location best = getBestLocation(false);

    move(best, lastFixed ? speed : 8);
}

void Camera::updateCombat(ItemObj* item)
{
    ASSERT(item->type == ITEM_LARA);

    targetAngle.x = item->angle.x + CAM_ANGLE_COMBAT;
    targetAngle.y = item->angle.y;
        
    if (item->extraL->armR.target || item->extraL->armL.target)
    {
        int32 aX = item->extraL->armR.angleAim.x + item->extraL->armL.angleAim.x;
        int32 aY = item->extraL->armR.angleAim.y + item->extraL->armL.angleAim.y;

        if (item->extraL->armR.target && item->extraL->armL.target) {
            targetAngle.x += aX >> 1;
            targetAngle.y += aY >> 1;
        } else {
            targetAngle.x += aX;
            targetAngle.y += aY;
        }
    } else {
        targetAngle.x += item->extraL->head.angle.x + item->extraL->torso.angle.x;
        targetAngle.y += item->extraL->head.angle.y + item->extraL->torso.angle.y;
    }

    targetDist = CAM_DIST_COMBAT;

    Location best = getBestLocation(true);

    move(best, speed);
}

void Camera::updateLook(ItemObj* item)
{
    ASSERT(item->type == ITEM_LARA);

    targetAngle.x = item->extraL->head.angle.x + item->extraL->torso.angle.x + item->angle.x;
    targetAngle.y = item->extraL->head.angle.y + item->extraL->torso.angle.y + item->angle.y;
    targetDist = lookAtItem ? CAM_DIST_FOLLOW : CAM_DIST_LOOK;

    Location best = getBestLocation(true);

    move(best, speed);
}

void Camera::updateFixed()
{
    const FixedCamera* cam = level.cameras + index;

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

void Camera::lookAt(int32 offset)
{
    int32 dx = lookAtItem->pos.x - laraItem->pos.x;
    int32 dz = lookAtItem->pos.z - laraItem->pos.z;

    int16 ay = int16(phd_atan(dz, dx) - laraItem->angle.y) >> 1;

    if (abs(ay) >= LARA_LOOK_ANGLE_Y)
    {
        lookAtItem = NULL;
        return;
    }

    const AABBs& box = lookAtItem->getBoundingBox(true);

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

    lookAtItem->flags |= ITEM_FLAG_ANIMATED; // use as once flag
    mode = CAMERA_MODE_LOOK;
}

void Camera::update()
{
    if (mode == CAMERA_MODE_FREE)
    {
        updateFree();
        matrixSetView(view.pos, angle.x, angle.y);
        return;
    }

    bool isFixed = false;
    ItemObj* item = laraItem;

    if (lookAtItem && (mode == CAMERA_MODE_FIXED || mode == CAMERA_MODE_OBJECT))
    {
        isFixed = true;
        item = lookAtItem;
    }

    ASSERT(item);

    target.room = item->room;
    target.pos.x = item->pos.x;
    target.pos.z = item->pos.z;

    const AABBs &box = item->getBoundingBox(true);

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
            int32 dx = (box.minX + box.maxX) >> 1;
            int32 dz = (box.minZ + box.maxZ) >> 1;
            int32 s, c;
            sincos(item->angle.y, s, c);
            X_ROTXY(dz, dx, s, c);

            target.pos.x += dx;
            target.pos.z += dz;
        }

        lastFixed = (int32(lastFixed) ^ int32(isFixed)) != 0; // armcpp 3DO compiler (lastFixed ^= isFixed)

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
        targetAngle.x = 0;
        targetAngle.y = 0;
        targetDist = CAM_DIST_FOLLOW;
        center = false;
    }

    vec3i dir = target.pos - view.pos;
    anglesFromVector(dir.x, dir.y, dir.z, angle.x, angle.y);

    matrixSetView(view.pos, angle.x, angle.y);
}

void Camera::toCombat()
{
    if (mode == CAMERA_MODE_FREE)
        return;

    if (mode == CAMERA_MODE_CUTSCENE)
        return;

    if (mode == CAMERA_MODE_LOOK)
        return;

    mode = CAMERA_MODE_COMBAT;
}

#endif

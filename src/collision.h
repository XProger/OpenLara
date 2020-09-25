#ifndef H_COLLISION
#define H_COLLISION

#include "core.h"
#include "utils.h"
#include "format.h"
#include "controller.h"

struct Collision {
    enum Side { NONE, LEFT, RIGHT, FRONT, BACK, TOP, BOTTOM } side;

    struct Info {
        int room, roomAbove, roomBelow;
        int climb;
        float floor, ceiling;
    } info[4];

    Collision() : side(NONE) {}

    Collision(Controller *controller, int roomIndex, vec3 &pos, const vec3 &offset, const vec3 &velocity, float radius, float angle, int minHeight, int maxHeight, int maxAscent, int maxDescent) {
        if (velocity.x > 0.0f || velocity.z > 0.0f)
            angle = normalizeAngle(PI2 + vec2(velocity.z, velocity.x).angle());
        pos += velocity;

        int q = angleQuadrant(angle, 0.25f);

        const vec2 v[] = {
            vec2( -radius,  radius ),
            vec2(  radius,  radius ),
            vec2(  radius, -radius ),
            vec2( -radius, -radius ),
        };

        const vec2 &l = v[q], &r = v[(q + 1) % 4];

        vec2 f = (q %= 2) ? vec2(l.x, radius * cosf(angle)) : vec2(radius * sinf(angle), l.y),
                p(pos.x, pos.z),
                d(0.0F);

        vec3 hpos = pos + offset;

        int height = maxHeight - minHeight;

        getFloor(controller->level, roomIndex, vec3(pos.x, hpos.y, pos.z));

        if (checkHeight(controller, roomIndex, hpos, vec2(0.0f), height, 0xFFFFFF, 0xFFFFFF, side = NONE)) {
            pos.x -= velocity.x;
            pos.z -= velocity.z;
            side = FRONT;
            return;
        }

        float hCell = info[NONE].ceiling - (hpos.y - maxHeight);
        if (hCell > 0) {
            if (hCell > 128) {
                pos.x -= velocity.x;
                pos.z -= velocity.z;
                side = FRONT;
            } else {
                pos.y = info[NONE].ceiling + maxHeight - offset.y;
                side  = TOP;
            }
        }

        float hFloor = info[NONE].floor - (hpos.y + minHeight);
        if (hFloor < 0 && hFloor > -256) {
            pos.y = info[NONE].floor - minHeight - offset.y;
            side  = BOTTOM;
        }

        if (checkHeight(controller, roomIndex, hpos, f, height, maxAscent, maxDescent, FRONT)) {
            d = vec2(-velocity.x, -velocity.z);
            q ^= 1;
            d[q] = getOffset(p[q] + f[q], p[q]);
        } else if (checkHeight(controller, roomIndex, hpos, l, height, maxAscent, maxDescent, LEFT)) {
            d[q] = getOffset(p[q] + l[q], p[q] + f[q]);
        } else if (checkHeight(controller, roomIndex, hpos, r, height, maxAscent, maxDescent, RIGHT)) {
            d[q] = getOffset(p[q] + r[q], p[q] + f[q]);
        } else
            return;

        pos += vec3(d.x, 0.0f, d.y);
    }

    inline bool checkHeight(Controller *controller, int roomIndex, const vec3 &pos, const vec2 &offset, int height, int maxAscent, int maxDescent, Side side) {
        TR::Level::FloorInfo info;
        controller->getFloorInfo(roomIndex, pos + vec3(offset.x, 0.0f, offset.y), info);

        Info &inf = this->info[side];
        inf.room      = info.roomNext != TR::NO_ROOM ? info.roomNext : roomIndex;
        inf.roomAbove = info.roomAbove;
        inf.roomBelow = info.roomBelow;
        inf.floor     = info.floor;
        inf.ceiling   = info.ceiling;
        inf.climb     = info.climb;

        if ((info.ceiling == info.floor) ||  (info.floor - info.ceiling < height) || (pos.y - info.floor > maxAscent) || (info.floor - pos.y > maxDescent) || (info.ceiling > pos.y) ||
            (maxAscent == maxDescent && (maxAscent <= 256 + 128) && (abs(info.slantX) > 2 || abs(info.slantZ) > 2))) {
            this->side = side;
            return true;
        }
        return false;
    }

    inline float getOffset(float from, float to) {
        int a = int(from) / 1024;
        int b = int(to)   / 1024;

        from -= float(a * 1024.0f);

        if (b == a)
            return 0.0f;
        else if (b > a)
            return -from + 1025.0f;
        return -from - 1.0f;
    }

    static int getFloor(TR::Level *level, int &roomIndex, const vec3 &pos) {
        int dx, dz, x = int(pos.x), z = int(pos.z);

        TR::Room::Sector *s = &level->getSector(roomIndex, x, z, dx, dz);
        while (s->ceiling * 256 > pos.y && s->roomAbove != TR::NO_ROOM) {
            roomIndex = s->roomAbove;
            s = &level->getSector(roomIndex, x, z, dx, dz);
        }

        return s->floor * 256;
    }
};

#endif
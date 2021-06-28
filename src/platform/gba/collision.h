#ifndef H_COLLISION
#define H_COLLISION

#include "common.h"

enum CollisionType {
    CT_NONE          = 0,
    CT_FRONT         = (1 << 0),
    CT_LEFT          = (1 << 1),
    CT_RIGHT         = (1 << 2),
    CT_CEILING       = (1 << 3),
    CT_FRONT_CEILING = (1 << 4),
    CT_FLOOR_CEILING = (1 << 5),
};

struct CollisionInfo
{
    enum SideType {
        ST_MIDDLE,
        ST_FRONT,
        ST_LEFT,
        ST_RIGHT,
        ST_MAX
    };

    struct Side
    {
        int32     floor;
        int32     ceiling;
        SlantType slantType;
    };

    const FloorData* trigger;

    Side m;
    Side f;
    Side l;
    Side r;

    int32 radius;

    int32 gapPos;
    int32 gapNeg;
    int32 gapCeiling;

    vec3i offset;
    vec3i pos;

    int16 angle;
    uint16 quadrant;

    CollisionType type;

    int8 slantX;
    int8 slantZ;

    bool enemyPush;
    bool enemyHit;
    bool staticHit;
    bool stopOnSlant;
    bool stopOnLava;

    void setSide(SideType st, int32 floor, int32 ceiling)
    {
        SlantType slantType;

        if (gLastFloorSlant.slantX == 0 && gLastFloorSlant.slantZ == 0) {
            slantType = SLANT_NONE;
        } else if (abs(gLastFloorSlant.slantX) < 3 && abs(gLastFloorSlant.slantZ) < 3) {
            slantType = SLANT_LOW;
        } else {
            slantType = SLANT_HIGH;
        }

        if (st != ST_MIDDLE) {
            if (stopOnSlant && floor < 0 && slantType == SLANT_HIGH) {
                floor -= 0x7FFF;
            } else if (stopOnSlant && floor > 0 && slantType == SLANT_HIGH) {
                floor = 512;
            }/* TODO lava else if (stopOnLava && floor > 0 && trigger && FloorData(*(uint16*)trigger).cmd.func == FloorData::LAVA) {
                floor = 512;
            }*/
        }

        Side *s = &m + st;
        s->slantType = slantType;
        s->floor     = floor;
        s->ceiling   = ceiling;
    }
};

CollisionInfo cinfo;

int32 alignOffset(int32 a, int32 b)
{
    int32 ca = a / 1024;
    int32 cb = b / 1024;

    if (ca == cb) {
        return 0;
    }

    a &= 1023;

    if (ca < cb) {
        return 1025 - a;
    }

    return -(a + 1);
}

bool collideStatic(Room* room, CollisionInfo &cinfo, const vec3i &p, int32 height)
{
    cinfo.staticHit = false;
    cinfo.offset    = vec3i(0);

    Bounds objBox;
    objBox.minX = -cinfo.radius;
    objBox.maxX =  cinfo.radius;
    objBox.minZ = -cinfo.radius;
    objBox.maxZ =  cinfo.radius;
    objBox.minY = -height;
    objBox.maxY = 0;

    Room** nearRoom = room->getNearRooms(p, cinfo.radius, height);

    while (*nearRoom)
    {
        const Room* room = *nearRoom++;

        for (int i = 0; i < room->info->meshesCount; i++)
        {
            const RoomMesh* mesh = room->data.meshes + i;

        #ifdef NO_STATIC_MESH_PLANTS
            if (mesh->id < 10) continue;
        #endif

            const StaticMesh* staticMesh = staticMeshes + mesh->id;

            if (staticMesh->flags & STATIC_MESH_FLAG_NO_COLLISION) continue;

            Bounds meshBox = boxRotate(staticMesh->cbox, (mesh->rot - 2) * ANGLE_90);

        // TODO align RoomInfo::Mesh (room relative int16?)
            vec3i pos;
            pos.x = mesh->pos.x + (room->info->x << 8);
            pos.y = mesh->pos.y;
            pos.z = mesh->pos.z + (room->info->z << 8);

            pos -= p;

            boxTranslate(meshBox, pos);

            if (!boxIntersect(meshBox, objBox)) continue;

            cinfo.offset = boxPushOut(meshBox, objBox);

            bool flip = (cinfo.quadrant > 1);

            if (cinfo.quadrant & 1) {
                if (abs(cinfo.offset.z) > cinfo.radius) {
                    cinfo.offset.z = cinfo.pos.z - p.z;
                    cinfo.type = CT_FRONT;
                } else if (cinfo.offset.z != 0) {
                    cinfo.offset.x = 0;
                    cinfo.type = ((cinfo.offset.z > 0) ^ flip) ? CT_RIGHT : CT_LEFT;
                } else {
                    cinfo.offset = vec3i(0);
                }
            } else {
                if (abs(cinfo.offset.x) > cinfo.radius) {
                    cinfo.offset.x = cinfo.pos.x - p.x;
                    cinfo.type = CT_FRONT;
                } else if (cinfo.offset.x != 0) {
                    cinfo.offset.z = 0;
                    cinfo.type = ((cinfo.offset.x > 0) ^ flip) ? CT_LEFT : CT_RIGHT;
                } else {
                    cinfo.offset = vec3i(0);
                }
            }

            cinfo.staticHit = true;

            return true;
        }
    }

    return false;
}

void collideRoom(Item* item, int32 height, int32 yOffset = 0)
{
    cinfo.type = CT_NONE;
    cinfo.offset = vec3i(0, 0, 0);

    vec3i p = item->pos;
    p.y += yOffset;

    int32 y = p.y - height;

    int32 cy = y - 160;

    int32 floor, ceiling;

    Room* room = item->room;

    #define CHECK_HEIGHT(v) {\
        room = room->getRoom(v.x, cy, v.z);\
        const Sector* sector = room->getSector(v.x, v.z);\
        floor = sector->getFloor(v.x, cy, v.z);\
        if (floor != WALL) floor -= p.y;\
        ceiling = sector->getCeiling(v.x, cy, v.z);\
        if (ceiling != WALL) ceiling -= y;\
    }

// middle
    CHECK_HEIGHT(p);

    cinfo.trigger = gLastFloorData;
    cinfo.slantX = gLastFloorSlant.slantX;
    cinfo.slantZ = gLastFloorSlant.slantZ;

    cinfo.setSide(CollisionInfo::ST_MIDDLE, floor, ceiling);

    vec3i f, l, r;
    int32 R = cinfo.radius;

    switch (cinfo.quadrant) {
        case 0 : {
            f = vec3i((R * phd_sin(cinfo.angle)) >> FIXED_SHIFT, 0, R);
            l = vec3i(-R, 0,  R);
            r = vec3i( R, 0,  R);
            break;
        }
        case 1 : {
            f = vec3i( R, 0, (R * phd_cos(cinfo.angle)) >> FIXED_SHIFT);
            l = vec3i( R, 0,  R);
            r = vec3i( R, 0, -R);
            break;
        }
        case 2 : {
            f = vec3i((R * phd_sin(cinfo.angle)) >> FIXED_SHIFT, 0, -R);
            l = vec3i( R, 0, -R);
            r = vec3i(-R, 0, -R);
            break;
        }
        case 3 : {
            f = vec3i(-R, 0, (R * phd_cos(cinfo.angle)) >> FIXED_SHIFT);
            l = vec3i(-R, 0, -R);
            r = vec3i(-R, 0,  R);
            break;
        }
        default : {
            f.x = f.y = f.z = 0;
            l.x = l.y = l.z = 0;
            r.x = r.y = r.z = 0;
            ASSERT(false);
        }
    }

    f += p;
    l += p;
    r += p;

    vec3i delta;
    delta.x = cinfo.pos.x - p.x;
    delta.y = cinfo.pos.y - p.y;
    delta.z = cinfo.pos.z - p.z;

// front
    CHECK_HEIGHT(f);
    cinfo.setSide(CollisionInfo::ST_FRONT, floor, ceiling);

// left
    CHECK_HEIGHT(l);
    cinfo.setSide(CollisionInfo::ST_LEFT, floor, ceiling);

// right
    CHECK_HEIGHT(r);
    cinfo.setSide(CollisionInfo::ST_RIGHT, floor, ceiling);

// static objects
    collideStatic(item->room, cinfo, p, height);

// check middle
    if (cinfo.m.floor == WALL)
    {
        cinfo.offset = delta;
        cinfo.type   = CT_FRONT;
        return;
    }

    if (cinfo.m.floor <= cinfo.m.ceiling)
    {
        cinfo.offset = delta;
        cinfo.type   = CT_FLOOR_CEILING;
        return;
    }

    if (cinfo.m.ceiling >= 0)
    {
        cinfo.offset.y = cinfo.m.ceiling;
        cinfo.type     = CT_CEILING;
    }

// front
    if (cinfo.f.floor > cinfo.gapPos || 
        cinfo.f.floor < cinfo.gapNeg ||
        cinfo.f.ceiling > cinfo.gapCeiling)
    {
        if (cinfo.quadrant & 1)
        {
            cinfo.offset.x = alignOffset(f.x, p.x);
            cinfo.offset.z = delta.z;
        } else {
            cinfo.offset.x = delta.x;
            cinfo.offset.z = alignOffset(f.z, p.z);
        }

        cinfo.type = CT_FRONT;
        return;
    }

// front ceiling
    if (cinfo.f.ceiling >= cinfo.gapCeiling)
    {
        cinfo.offset = delta;
        cinfo.type   = CT_FRONT_CEILING;
        return;
    }

// left
    if (cinfo.l.floor > cinfo.gapPos || cinfo.l.floor < cinfo.gapNeg)
    {
        if (cinfo.quadrant & 1) {
            cinfo.offset.z = alignOffset(l.z, f.z);
        } else {
            cinfo.offset.x = alignOffset(l.x, f.x);
        }
        cinfo.type = CT_LEFT;
        return;
    }

// right
    if (cinfo.r.floor > cinfo.gapPos || cinfo.r.floor < cinfo.gapNeg)
    {
        if (cinfo.quadrant & 1) {
            cinfo.offset.z = alignOffset(r.z, f.z);
        } else {
            cinfo.offset.x = alignOffset(r.x, f.x);
        }
        cinfo.type = CT_RIGHT;
        return;
    }
}

#endif

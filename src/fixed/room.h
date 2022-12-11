#ifndef H_ROOM
#define H_ROOM

#include "common.h"

EWRAM_DATA Room* roomsList[MAX_ROOM_LIST];

//#ifdef ROM_READ
EWRAM_DATA int32 dynSectorsCount;
EWRAM_DATA Sector dynSectors[MAX_DYN_SECTORS];   // EWRAM 8k
//#endif

const Sector* Sector::getSectorBelow(int32 posX, int32 posZ) const
{
    if (roomBelow == NO_ROOM)
        return this;
    return rooms[roomBelow].getSector(posX, posZ)->getSectorBelow(posX, posZ);
}

const Sector* Sector::getSectorAbove(int32 posX, int32 posZ) const
{
    if (roomAbove == NO_ROOM)
        return this;
    return rooms[roomAbove].getSector(posX, posZ)->getSectorAbove(posX, posZ);
}

int32 Sector::getFloor(int32 x, int32 y, int32 z) const
{
    gLastFloorData = NULL;

    const Sector* lowerSector = getSectorBelow(x, z);

    int32 floor = lowerSector->floor << 8;

    gLastFloorSlant = 0;

    if (lowerSector->floorIndex)
    {
        const FloorData* fd = level.floors + lowerSector->floorIndex;
        uint16 cmd = *fd++;

        if (FD_FLOOR_TYPE(cmd) == FLOOR_TYPE_FLOOR) // found floor
        {
            gLastFloorSlant = *fd;
            int32 sx = FD_SLANT_X(gLastFloorSlant);
            int32 sz = FD_SLANT_Z(gLastFloorSlant);
            int32 dx = x & 1023;
            int32 dz = z & 1023;
            floor -= sx * (sx < 0 ? dx : (dx - 1023)) >> 2;
            floor -= sz * (sz < 0 ? dz : (dz - 1023)) >> 2;
        }

        lowerSector->getTriggerFloorCeiling(x, y, z, &floor, NULL);
    }

    return floor;
}

int32 Sector::getCeiling(int32 x, int32 y, int32 z) const
{
    const Sector* upperSector = getSectorAbove(x, z);

    int32 ceiling = upperSector->ceiling << 8;

    if (upperSector->floorIndex)
    {
        const FloorData* fd = level.floors + upperSector->floorIndex;
        uint16 cmd = *fd++;

        if (FD_FLOOR_TYPE(cmd) == FLOOR_TYPE_FLOOR) // skip floor
        {
            fd++;
            cmd = *fd++;
        }

        if (FD_FLOOR_TYPE(cmd) == FLOOR_TYPE_CEILING) // found ceiling
        {
            int32 sx = FD_SLANT_X(*fd);
            int32 sz = FD_SLANT_Z(*fd);
            int32 dx = x & 1023;
            int32 dz = z & 1023;
            ceiling -= sx * (sx < 0 ? (dx - 1023) : dx) >> 2;
            ceiling += sz * (sz < 0 ? dz : (dz - 1023)) >> 2;
        }
    }

    const Sector* lowerSector = getSectorBelow(x, z);

    if (lowerSector->floorIndex)
    {
        lowerSector->getTriggerFloorCeiling(x, y, z, NULL, &ceiling);
    }

    return ceiling;
}

Room* Sector::getNextRoom() const
{
    if (!floorIndex)
        return NULL;

    // always in this order
    // - floor
    // - ceiling
    // - portal
    // - other

    const FloorData* fd = level.floors + floorIndex;
    uint16 cmd = *fd++;

    if (FD_FLOOR_TYPE(cmd) == FLOOR_TYPE_FLOOR)  // skip floor
    {
        if (FD_END(cmd)) return NULL;
        fd++;
        cmd = *fd++;
    }

    if (FD_FLOOR_TYPE(cmd) == FLOOR_TYPE_CEILING) // skip ceiling
    {
        if (FD_END(cmd)) return NULL;
        fd++;
        cmd = *fd++;
    }

    if (FD_FLOOR_TYPE(cmd) != FLOOR_TYPE_PORTAL) // no portal
        return NULL;

    ASSERT(*fd != NO_ROOM);

    return rooms + *fd;
}

void Sector::getTriggerFloorCeiling(int32 x, int32 y, int32 z, int32* floor, int32* ceiling) const
{
    if (!floorIndex)
        return;

    uint16 cmd;
    const FloorData* fd = level.floors + floorIndex;

    do {
        cmd = *fd++;
            
        switch (FD_FLOOR_TYPE(cmd))
        {
            case FLOOR_TYPE_PORTAL:
            case FLOOR_TYPE_FLOOR:
            case FLOOR_TYPE_CEILING:
            {
                fd++;
                break;
            }
            
            case FLOOR_TYPE_TRIGGER:
            {
                if (floor && !gLastFloorData) {
                    gLastFloorData = fd - 1;
                }

                fd++;
                uint16 trigger;

                do {
                    trigger = *fd++;

                    if (FD_ACTION(trigger) == TRIGGER_ACTION_ACTIVATE_OBJECT)
                    {
                        items[FD_ARGS(trigger)].getItemFloorCeiling(x, y, z, floor, ceiling);
                    }

                    if (FD_ACTION(trigger) == TRIGGER_ACTION_ACTIVATE_CAMERA)
                    {
                        trigger = *fd++; // skip camera index
                    }

                } while (!FD_END(trigger));

                break;
            }

            case FLOOR_TYPE_LAVA:
                if (floor) {
                    gLastFloorData = fd - 1;
                }
                break;
        }

    } while (!FD_END(cmd));
}

#ifndef __32X__
#if !defined(USE_ASM) || defined(__3DO__) // TODO for 3DO
const Sector* Room::getSector(int32 x, int32 z) const
{
    // TODO remove clamp?
    int32 sx = X_CLAMP((x - (info->x << 8)) >> 10, 0, info->xSectors - 1);
    int32 sz = X_CLAMP((z - (info->z << 8)) >> 10, 0, info->zSectors - 1);

    return sectors + sx * info->zSectors + sz;
}
#endif
#endif

const Sector* Room::getWaterSector(int32 x, int32 z) const
{
    const Room* room = this;
    const Sector* sector = room->getSector(x, z);

    // go up to the air
    if (ROOM_FLAG_WATER(room->info->flags))
    {
        while (sector->roomAbove != NO_ROOM)
        {
            room = rooms + sector->roomAbove;

            if (!ROOM_FLAG_WATER(room->info->flags)) {
                return sector;
            }

            sector = room->getSector(x, z);
        }
        return sector;
    }
    
    // go down to the water
    while (sector->roomBelow != NO_ROOM)
    {
        room = rooms + sector->roomBelow;
        sector = room->getSector(x, z);

        if (ROOM_FLAG_WATER(room->info->flags)) {
            return sector;
        }
    }

    return NULL;
}

Room* Room::getRoom(int32 x, int32 y, int32 z)
{
    const Sector* sector = getSector(x, z);

    Room* room = this;

    while (1)
    {
        Room* nextRoom = sector->getNextRoom();
        if (!nextRoom)
            break;
        room = nextRoom;
        sector = room->getSector(x, z);
    };

    while (sector->roomAbove != NO_ROOM && y < (sector->ceiling << 8))
    {
        room = rooms + sector->roomAbove;
        sector = room->getSector(x, z);
    }

    while (sector->roomBelow != NO_ROOM && y >= (sector->floor << 8))
    {
        room = rooms + sector->roomBelow;
        sector = room->getSector(x, z);
    }

    return room;
}

bool Room::collideStatic(CollisionInfo &cinfo, const vec3i &p, int32 height)
{
    cinfo.staticHit = false;
    cinfo.offset = _vec3i(0, 0, 0);

    AABBi objBox;
    objBox.minX = -cinfo.radius;
    objBox.maxX =  cinfo.radius;
    objBox.minZ = -cinfo.radius;
    objBox.maxZ =  cinfo.radius;
    objBox.minY = -height;
    objBox.maxY = 0;

    Room** nearRoom = getNearRooms(p, cinfo.radius, height);

    while (*nearRoom)
    {
        const Room* room = *nearRoom++;

        int32 rx = p.x - (room->info->x << 8);
        int32 ry = p.y;
        int32 rz = p.z - (room->info->z << 8);

        for (int i = 0; i < room->info->meshesCount; i++)
        {
            const RoomMesh* mesh = room->data.meshes + i;

        #ifdef NO_STATIC_MESH_PLANTS
            if (STATIC_MESH_ID(mesh->zf) < 10)
                continue;
        #endif

            const StaticMesh* staticMesh = level.staticMeshes + STATIC_MESH_ID(mesh->zf);

            if (staticMesh->flags & STATIC_MESH_FLAG_NO_COLLISION)
                continue;

            int32 x = (int32(mesh->xy) >> 16) - rx;
            int32 y = (int32(mesh->xy) << 16 >> 16) - ry;
            int32 z = (int32(mesh->zf) >> 16) - rz;

            if (abs(x) > MAX_STATIC_MESH_RADIUS || abs(z) > MAX_STATIC_MESH_RADIUS || abs(y) > MAX_STATIC_MESH_RADIUS)
                continue;

            AABBi meshBox(staticMesh->cbox);
            boxRotateYQ(meshBox, STATIC_MESH_QUADRANT(mesh->zf));
            boxTranslate(meshBox, x, y, z);

            if (!boxIntersect(meshBox, objBox))
                continue;

            cinfo.offset = boxPushOut(meshBox, objBox);

            bool flip = (cinfo.quadrant > 1);

            if (cinfo.quadrant & 1) {
                if (abs(cinfo.offset.z) > cinfo.radius) {
                    cinfo.offset.z = cinfo.pos.z - p.z;
                    if ((cinfo.offset.x < 0 && cinfo.quadrant == 1) || (cinfo.offset.x > 0 && cinfo.quadrant == 3)) {
                        cinfo.type = CT_FRONT;
                    }
                } else if (cinfo.offset.z != 0) {
                    cinfo.offset.x = 0;
                    cinfo.type = ((cinfo.offset.z > 0) ^ flip) ? CT_RIGHT : CT_LEFT;
                } else {
                    cinfo.offset = _vec3i(0, 0, 0);
                }
            } else {
                if (abs(cinfo.offset.x) > cinfo.radius) {
                    cinfo.offset.x = cinfo.pos.x - p.x;
                    if ((cinfo.offset.z < 0 && cinfo.quadrant == 0) || (cinfo.offset.z > 0 && cinfo.quadrant == 2)) {
                        cinfo.type = CT_FRONT;
                    }
                } else if (cinfo.offset.x != 0) {
                    cinfo.offset.z = 0;
                    cinfo.type = ((cinfo.offset.x > 0) ^ flip) ? CT_LEFT : CT_RIGHT;
                } else {
                    cinfo.offset = _vec3i(0, 0, 0);
                }
            }

            cinfo.staticHit = (cinfo.offset.x != 0 || cinfo.offset.z != 0);

            return true;
        }
    }

    return false;
}

bool Room::checkPortal(const Portal* portal)
{
    vec3i d;
    d.x = portal->v[0].x - gCameraViewPos.x + (info->x << 8);
    d.y = portal->v[0].y - gCameraViewPos.y;
    d.z = portal->v[0].z - gCameraViewPos.z + (info->z << 8);

    Matrix &m = matrixGet();

    vec3i pv[4];
/*
#ifdef __3DO__
    int32 axis = 0;
    if (d.x >= 0) axis |= (2 << 0);
    if (d.x <  0) axis |= (1 << 0);
    if (d.y >= 0) axis |= (2 << 2);
    if (d.y <  0) axis |= (1 << 2);
    if (d.z >= 0) axis |= (2 << 4);
    if (d.z <  0) axis |= (1 << 4);

    if (!(portal->normalMask & axis))
        return false;
#else*/
    if (DP33(portal->n.x, portal->n.y, portal->n.z, d.x, d.y, d.z) >= 0)
        return false;
//#endif

    int32 x0 = clip.x1 - (FRAME_WIDTH >> 1);
    int32 y0 = clip.y1 - (FRAME_HEIGHT >> 1);
    int32 x1 = clip.x0 - (FRAME_WIDTH >> 1);
    int32 y1 = clip.y0 - (FRAME_HEIGHT >> 1);

    int32 znear = 0, zfar = 0;

    for (int32 i = 0; i < 4; i++)
    {
        int32 px = portal->v[i].x;
        int32 py = portal->v[i].y;
        int32 pz = portal->v[i].z;

        int32 x = DP43(m.e00, m.e01, m.e02, m.e03, px, py, pz);
        int32 y = DP43(m.e10, m.e11, m.e12, m.e13, px, py, pz);
        int32 z = DP43(m.e20, m.e21, m.e22, m.e23, px, py, pz);

        pv[i].x = x;
        pv[i].y = y;
        pv[i].z = z;

        if (z <= 0) {
            znear++;
            continue;
        }

        if (z > VIEW_MAX_F)
        {
            z = VIEW_MAX_F;
            zfar++;
        }

        x >>= FIXED_SHIFT;
        y >>= FIXED_SHIFT;
        z >>= FIXED_SHIFT;

        PERSPECTIVE(x, y, z);

        if (x < x0) x0 = x;
        if (x > x1) x1 = x;
        if (y < y0) y0 = y;
        if (y > y1) y1 = y;
    }

    if (znear == 4 || zfar == 4)
        return false;

    x0 += (FRAME_WIDTH >> 1);
    y0 += (FRAME_HEIGHT >> 1);
    x1 += (FRAME_WIDTH >> 1);
    y1 += (FRAME_HEIGHT >> 1);

    if (znear)
    {
        vec3i *a = pv;
        vec3i *b = pv + 3;
        for (int32 i = 0; i < 4; i++)
        {
            if ((a->z < 0) ^ (b->z < 0))
            {
                if (a->x < 0 && b->x < 0) {
                    x0 = 0;
                } else if (a->x > 0 && b->x > 0) {
                    x1 = FRAME_WIDTH;
                } else {
                    x0 = 0;
                    x1 = FRAME_WIDTH;
                }

                if (a->y < 0 && b->y < 0) {
                    y0 = 0;
                } else if (a->y > 0 && b->y > 0) {
                    y1 = FRAME_HEIGHT;
                } else {
                    y0 = 0;
                    y1 = FRAME_HEIGHT;
                }
            }
            b = a;
            a++;
        }
    }

    if (x0 < clip.x0) x0 = clip.x0;
    if (x1 > clip.x1) x1 = clip.x1;
    if (y0 < clip.y0) y0 = clip.y0;
    if (y1 > clip.y1) y1 = clip.y1;

    if (x0 >= x1 || y0 >= y1)
        return false;

    Room* nextRoom = rooms + portal->roomIndex;

    if (x0 < nextRoom->clip.x0) nextRoom->clip.x0 = x0;
    if (x1 > nextRoom->clip.x1) nextRoom->clip.x1 = x1;
    if (y0 < nextRoom->clip.y0) nextRoom->clip.y0 = y0;
    if (y1 > nextRoom->clip.y1) nextRoom->clip.y1 = y1;

    return true;
}

Room** Room::addVisibleRoom(Room** list)
{
    matrixPush();
    matrixTranslateAbs(info->x << 8, 0, info->z << 8);

    for (int32 i = 0; i < info->portalsCount; i++)
    {
        const Portal* portal = data.portals + i;

        if (checkPortal(portal))
        {
            Room* nextRoom = rooms + portal->roomIndex;

            list = nextRoom->addVisibleRoom(list);

            if (!nextRoom->visible)
            {
                nextRoom->visible = true;
                *list++ = nextRoom;
            }
        }
    }

    matrixPop();

    return list;
}

Room** Room::getVisibleRooms()
{
    Room** list = addVisibleRoom(roomsList);
    *list++ = this;
    *list++ = NULL;

    ASSERT(list - roomsList <= MAX_ROOM_LIST);

    return roomsList;
}

void Room::reset()
{
    visible = false;
    clip = RectMinMax( FRAME_WIDTH, FRAME_HEIGHT, 0, 0 );
}

Room** Room::addNearRoom(Room** list, int32 x, int32 y, int32 z)
{
    Room* nearRoom = getRoom(x, y, z);

    int32 count = list - roomsList;
    for (int32 i = 0; i < count; i++)
    {
        if (roomsList[i] == nearRoom)
            return list;
    }

    *list++ = nearRoom;
    return list;
}

Room** Room::getNearRooms(const vec3i &pos, int32 radius, int32 height)
{
    Room** list = roomsList;

    *list++ = this;
    
    list = addNearRoom(list, pos.x - radius, pos.y, pos.z - radius);
    list = addNearRoom(list, pos.x + radius, pos.y, pos.z - radius);
    list = addNearRoom(list, pos.x + radius, pos.y, pos.z + radius);
    list = addNearRoom(list, pos.x - radius, pos.y, pos.z + radius);

    list = addNearRoom(list, pos.x - radius, pos.y - height, pos.z - radius);
    list = addNearRoom(list, pos.x + radius, pos.y - height, pos.z - radius);
    list = addNearRoom(list, pos.x + radius, pos.y - height, pos.z + radius);
    list = addNearRoom(list, pos.x - radius, pos.y - height, pos.z + radius);

    *list++ = NULL;

    return roomsList;
}

Room** Room::getAdjRooms()
{
    Room** list = roomsList;

    *list++ = this;
    for (int32 i = 0; i < info->portalsCount; i++)
    {
        *list++ = rooms + data.portals[i].roomIndex;
    }
    *list++ = NULL;

    return roomsList;
}

void Room::modify()
{
//#ifdef ROM_READ
    if (sectors == data.sectors)
    {
        // convert room->sectors to mutable (non-ROM) data
        sectors = dynSectors + dynSectorsCount;
        memcpy((Sector*)sectors, data.sectors, info->xSectors * info->zSectors * sizeof(Sector));

        dynSectorsCount += info->xSectors * info->zSectors;
        //printf("dynSectors: %d\n", dynSectorsCount);
        ASSERT(dynSectorsCount <= MAX_DYN_SECTORS);
    }
//#endif
}

void Room::add(ItemObj* item)
{
    ASSERT(item && item->nextItem == NULL);

    item->room = this;
    item->nextItem = firstItem;
    firstItem = item;
}

void Room::remove(ItemObj* item)
{
    ASSERT(item && item->room == this);

    ItemObj* prev = NULL;
    ItemObj* curr = firstItem;

    while (curr)
    {
        ItemObj* next = curr->nextItem;

        if (curr == item)
        {
            item->nextItem = NULL;

            if (prev) {
                prev->nextItem = next;
            } else {
                firstItem = next;
            }

            break;
        }

        prev = curr;
        curr = next;
    }
}

#define TRACE_SHIFT 10 // trace precision

#define TRACE_CHECK(r, x, y, z) \
{ \
    const Sector* sector = r->getSector(x, z); \
    if (accurate) { \
        if (y > sector->getFloor(x, y, z) || y < sector->getCeiling(x, y, z)) \
        { \
            to.pos = p; \
            to.room = room; \
            return false; \
        } \
    } else { \
        if (y > (sector->floor << 8) || y < (sector->ceiling << 8)) \
        { \
            to.pos = p; \
            to.room = room; \
            return false; \
        } \
    } \
}

bool traceX(const Location &from, Location &to, bool accurate)
{
    vec3i d = to.pos - from.pos;

    if (!d.x)
        return true;

    d.y = (d.y << TRACE_SHIFT) / d.x;
    d.z = (d.z << TRACE_SHIFT) / d.x;

    vec3i p = from.pos;

    Room* room = from.room;

    if (d.x < 0)
    {
        d.x = 1024;
        p.x &= ~1023;
        p.y += d.y * (p.x - from.pos.x) >> TRACE_SHIFT;
        p.z += d.z * (p.x - from.pos.x) >> TRACE_SHIFT;

        while (p.x > to.pos.x)
        {
            room = room->getRoom(p.x, p.y, p.z);
            TRACE_CHECK(room, p.x, p.y, p.z);

            Room* nextRoom = room->getRoom(p.x - 1, p.y, p.z);
            TRACE_CHECK(nextRoom, p.x - 1, p.y, p.z);

            room = nextRoom;
            p -= d;
        }
    }
    else
    {
        d.x = 1024;
        p.x |= 1023;
        p.y += d.y * (p.x - from.pos.x) >> TRACE_SHIFT;
        p.z += d.z * (p.x - from.pos.x) >> TRACE_SHIFT;

        while (p.x < to.pos.x)
        {
            room = room->getRoom(p.x, p.y, p.z);
            TRACE_CHECK(room, p.x, p.y, p.z);

            Room* nextRoom = room->getRoom(p.x + 1, p.y, p.z);
            TRACE_CHECK(nextRoom, p.x + 1, p.y, p.z);

            room = nextRoom;
            p += d;
        }
    }

    to.room = room;

    return true;
}

bool traceZ(const Location &from, Location &to, bool accurate)
{
    vec3i d = to.pos - from.pos;

    if (!d.z)
        return true;

    d.x = (d.x << TRACE_SHIFT) / d.z;
    d.y = (d.y << TRACE_SHIFT) / d.z;

    vec3i p = from.pos;

    Room* room = from.room;

    if (d.z < 0)
    {
        d.z = 1024;
        p.z &= ~1023;
        p.x += d.x * (p.z - from.pos.z) >> TRACE_SHIFT;
        p.y += d.y * (p.z - from.pos.z) >> TRACE_SHIFT;

        while (p.z > to.pos.z)
        {
            room = room->getRoom(p.x, p.y, p.z);
            TRACE_CHECK(room, p.x, p.y, p.z);

            Room* nextRoom = room->getRoom(p.x, p.y, p.z - 1);
            TRACE_CHECK(nextRoom, p.x, p.y, p.z - 1);

            room = nextRoom;
            p -= d;
        }
    }
    else
    {
        d.z = 1024;
        p.z |= 1023;
        p.x += d.x * (p.z - from.pos.z) >> TRACE_SHIFT;
        p.y += d.y * (p.z - from.pos.z) >> TRACE_SHIFT;

        while (p.z < to.pos.z)
        {
            room = room->getRoom(p.x, p.y, p.z);
            TRACE_CHECK(room, p.x, p.y, p.z);

            Room* nextRoom = room->getRoom(p.x, p.y, p.z + 1);
            TRACE_CHECK(nextRoom, p.x, p.y, p.z + 1);

            room = nextRoom;
            p += d;
        }
    }

    to.room = room;

    return true;
}

#undef TRACE_CHECK

bool trace(const Location &from, Location &to, bool accurate)
{
    int32 dx = abs(to.pos.x - from.pos.x);
    int32 dz = abs(to.pos.z - from.pos.z);
    int32 dy;

    bool res;

    if (dz > dx) {
        res = traceX(from, to, accurate);
        if (!traceZ(from, to, accurate))
            return false;
    } else {
        res = traceZ(from, to, accurate);
        if (!traceX(from, to, accurate))
            return false;
    }

    dy = to.pos.y - from.pos.y;

    if (dy)
    {
        const Sector* sector = to.room->getSector(to.pos.x, to.pos.z);

        int32 h = sector->getFloor(to.pos.x, to.pos.y, to.pos.z);
        if (to.pos.y <= h || from.pos.y >= h)
        {
            h = sector->getCeiling(to.pos.x, to.pos.y, to.pos.z);
            if (to.pos.y >= h || from.pos.y <= h)
            {
                h = WALL;
            }
        }

        if (h != WALL)
        {
            to.pos.y = h;
            h -= from.pos.y;
            to.pos.x = from.pos.x + (to.pos.x - from.pos.x) * h / dy; // @DIV
            to.pos.z = from.pos.z + (to.pos.z - from.pos.z) * h / dy; // @DIV
            return false;
        }
    }

    return res;
}

void checkCamera(const FloorData* fd, Camera* camera)
{
    bool lookAt = false;

    while (1)
    {
        uint16 triggerCmd = *fd++;

        switch (FD_ACTION(triggerCmd))
        {
            case TRIGGER_ACTION_ACTIVATE_CAMERA:
            {
                FD_SET_END(triggerCmd, FD_END(*fd++));

                if (FD_ARGS(triggerCmd) != camera->lastIndex)
                {
                    camera->lookAtItem = NULL;
                    return;
                }

                camera->index = FD_ARGS(triggerCmd);

                if (camera->timer < 0 || camera->mode == CAMERA_MODE_LOOK || camera->mode == CAMERA_MODE_COMBAT)
                {
                    camera->timer = -1;
                    camera->lookAtItem = NULL;
                    return;
                }

                camera->mode = CAMERA_MODE_FIXED;
                lookAt = true;
                break;
            }

            case TRIGGER_ACTION_CAMERA_TARGET:
            {
                if (camera->mode == CAMERA_MODE_LOOK || camera->mode == CAMERA_MODE_COMBAT)
                    break;

                ASSERT(FD_ARGS(triggerCmd) < level.itemsCount);
                camera->lookAtItem = items + FD_ARGS(triggerCmd);
                break;
            }

            case TRIGGER_ACTION_FLYBY:
            {
                FD_SET_END(triggerCmd, FD_END(*fd++));
                break;
            }
        }

        if (FD_END(triggerCmd))
            break;
    };

    if (!lookAt && camera->lookAtItem && camera->lookAtItem != camera->lastItem && (camera->lookAtItem->flags & ITEM_FLAG_ANIMATED)) {
        camera->lookAtItem = NULL;
    }
}

void checkTrigger(const FloorData* fd, ItemObj* lara)
{
    if (!fd)
        return;

    if (FD_FLOOR_TYPE(*fd) == FLOOR_TYPE_LAVA)
    {
        // TODO lava

        if (FD_END(*fd))
            return;

        fd++;
    }

    uint16 cmd = *fd++;
    uint16 data = *fd++;

    ItemObj* switchItem = NULL;
    ItemObj* cameraItem = NULL;

    Camera* camera = lara ? &lara->extraL->camera : &playersExtra[0].camera;

    if (camera->mode != CAMERA_MODE_OBJECT) {
        checkCamera(fd, camera);
    }

    if (!lara && FD_TRIGGER_TYPE(cmd) != TRIGGER_TYPE_OBJECT)
        return;

    if (lara)
    {
        switch (FD_TRIGGER_TYPE(cmd))
        {
            case TRIGGER_TYPE_ACTIVATE:
                break;

            case TRIGGER_TYPE_PAD:
            case TRIGGER_TYPE_ANTIPAD:
            {
                if (lara->pos.y != lara->roomFloor)
                    return;
                break;
            }

            case TRIGGER_TYPE_SWITCH:
            {
                switchItem = items + FD_ARGS(*fd);
                if (!useSwitch(switchItem, FD_TIMER(data)))
                    return;
                fd++;
                break;
            }

            case TRIGGER_TYPE_KEY:
            {
                ItemObj* keyItem = items + FD_ARGS(*fd);
                if (!useKey(keyItem, lara))
                    return;
                fd++;
                break;
            }

            case TRIGGER_TYPE_PICKUP:
            {
                ItemObj* pickupItem = items + FD_ARGS(*fd);
                if (!usePickup(pickupItem))
                    return;
                fd++;
                break;
            }
                
            case TRIGGER_TYPE_OBJECT:
                return;

            case TRIGGER_TYPE_COMBAT:
            {
                if (lara->extraL->weaponState != WEAPON_STATE_READY)
                    return;
                break;
            }

            case TRIGGER_TYPE_DUMMY:
                return;
        }
    }

    while (1)
    {
        uint16 triggerCmd = *fd++;

        switch (FD_ACTION(triggerCmd))
        {
            case TRIGGER_ACTION_ACTIVATE_OBJECT:
            {
                ASSERT(FD_ARGS(triggerCmd) < level.itemsCount);
                ItemObj* item = items + FD_ARGS(triggerCmd);
                
                if (item->flags & ITEM_FLAG_ONCE)
                    break;

                item->timer = FD_TIMER(data);
                if (item->timer != 1) {
                    item->timer *= 30;
                }

                if (FD_TRIGGER_TYPE(cmd) == TRIGGER_TYPE_SWITCH) {
                    item->flags ^= (FD_MASK(data) << ITEM_FLAGS_MASK_SHIFT);
                } else if (FD_TRIGGER_TYPE(cmd) == TRIGGER_TYPE_ANTIPAD) {
                    item->flags &= ~(FD_MASK(data) << ITEM_FLAGS_MASK_SHIFT);
                } else {
                    item->flags |= (FD_MASK(data) << ITEM_FLAGS_MASK_SHIFT);
                }

                if ((item->flags & ITEM_FLAG_MASK) != ITEM_FLAG_MASK)
                    break;

                if (FD_ONCE(data)) {
                    item->flags |= ITEM_FLAG_ONCE;
                }

                if (item->flags & ITEM_FLAG_ACTIVE)
                    break;

                item->activate();

                if (!(item->flags & ITEM_FLAG_STATUS) && (item->flags & ITEM_FLAG_ACTIVE)) {
                    item->flags |= ITEM_FLAG_STATUS_ACTIVE;
                }

                break;
            }

            case TRIGGER_ACTION_ACTIVATE_CAMERA:
            {
                uint16 cam = *fd++;
                FD_SET_END(triggerCmd, FD_END(cam));

                if (level.cameras[FD_ARGS(triggerCmd)].flags & FIXED_CAMERA_FLAG_ONCE)
                    break;

                camera->index = FD_ARGS(triggerCmd);

                if (camera->mode == CAMERA_MODE_LOOK || camera->mode == CAMERA_MODE_COMBAT)
                    break;

                if (FD_TRIGGER_TYPE(cmd) == TRIGGER_TYPE_COMBAT)
                    break;

                if (FD_TRIGGER_TYPE(cmd) == TRIGGER_TYPE_SWITCH && (switchItem->state == 1) && (FD_TIMER(data) != 0))
                    break;

                if (FD_TRIGGER_TYPE(cmd) == TRIGGER_TYPE_SWITCH || camera->index != camera->lastIndex)
                {
                    camera->timer = FD_TIMER(cam);
                    if (camera->timer != 1) {
                        camera->timer *= 30;
                    }

                    if (FD_ONCE(cam)) {
                        level.cameras[camera->index].flags |= FIXED_CAMERA_FLAG_ONCE;
                    }
                
                    camera->speed = (FD_SPEED(cam) << 3) + 1;
                    camera->mode = lara ? CAMERA_MODE_FIXED : CAMERA_MODE_OBJECT;
                }
                break;
            }

            case TRIGGER_ACTION_FLOW:
                // TODO flow
                break;

            case TRIGGER_ACTION_FLIP:
                // TODO flipmap
                break;

            case TRIGGER_ACTION_FLIP_ON:
                // TODO flipmap
                break;

            case TRIGGER_ACTION_FLIP_OFF:
                // TODO flipmap
                break;

            case TRIGGER_ACTION_CAMERA_TARGET:
            {
                cameraItem = items + FD_ARGS(triggerCmd);
                break;
            }

            case TRIGGER_ACTION_END:
                nextLevel(LevelID(gLevelID + 1));
                break;

            case TRIGGER_ACTION_SOUNDTRACK:
            {
                int32 track = doTutorial(lara, FD_ARGS(triggerCmd));

                if (track == 0) break;

                uint8 &flags = gSaveGame.tracks[track];

                if (flags & TRACK_FLAG_ONCE)
                    break;

                if (FD_TRIGGER_TYPE(cmd) == TRIGGER_TYPE_SWITCH)
                    flags ^= FD_MASK(data);
                else if (FD_TRIGGER_TYPE(cmd) == TRIGGER_TYPE_ANTIPAD)
                    flags &= ~FD_MASK(data);
                else
                    flags |= FD_MASK(data);

                if ((flags & TRACK_FLAG_MASK) == TRACK_FLAG_MASK)
                {
                    if (FD_ONCE(data)) {
                        flags |= TRACK_FLAG_ONCE;
                    }
                    sndPlayTrack(track);
                } else {
                    sndStopTrack();
                }
                break;
            }

            case TRIGGER_ACTION_EFFECT:
                // TODO effect
                break;

            case TRIGGER_ACTION_SECRET:
            {
                if (gSaveGame.secrets & (1 << FD_ARGS(triggerCmd)))
                    break;
                gSaveGame.secrets |= (1 << FD_ARGS(triggerCmd));
                sndPlayTrack(13);
                break;
            }

            case TRIGGER_ACTION_CLEAR_BODIES:
                break;

            case TRIGGER_ACTION_FLYBY:
                FD_SET_END(triggerCmd, FD_END(*fd++));
                break;

            case TRIGGER_ACTION_CUTSCENE:
                break;
        }

        if (FD_END(triggerCmd))
            break;
    };

    if (cameraItem && (camera->mode == CAMERA_MODE_FIXED || camera->mode == CAMERA_MODE_OBJECT))
    {
        camera->lookAtItem = cameraItem;
    }
}

#endif

#ifndef H_ROOM
#define H_ROOM

#include "common.h"
#include "camera.h"

void animTexturesShift()
{
    const int16* data = level.animTexData;

    int32 texRangesCount = *data++;

    for (int32 i = 0; i < texRangesCount; i++)
    {
        int32 count = *data++;

        Texture tmp = textures[*data];
        while (count > 0)
        {
            textures[data[0]] = textures[data[1]];
            data++;
            count--;
        }
        textures[*data++] = tmp;
    }
}

int32 getBridgeFloor(const Item* item, int32 x, int32 z)
{
    if (item->type == ITEM_BRIDGE_FLAT) {
        return item->pos.y;
    }

    int32 h;
    if (item->angle.y == ANGLE_0) {
        h = 1024 - x;
    } else if (item->angle.y == ANGLE_180) {
        h = x;
    } else if (item->angle.y == ANGLE_90) {
        h = z;
    } else {
        h = 1024 - z;
    }

    h &= 1023;

    return item->pos.y + ((item->type == ITEM_BRIDGE_TILT_1) ? (h >> 2) : (h >> 1));
}

int32 getTrapDoorFloor(const Item* item, int32 x, int32 z)
{
    int32 dx = (item->pos.x >> 10) - (x >> 10);
    int32 dz = (item->pos.z >> 10) - (z >> 10);

    if (((dx ==  0) && (dz ==  0)) ||
        ((dx ==  0) && (dz ==  1) && (item->angle.y ==  ANGLE_0))   ||
        ((dx ==  0) && (dz == -1) && (item->angle.y ==  ANGLE_180)) ||
        ((dx ==  1) && (dz ==  0) && (item->angle.y ==  ANGLE_90))  ||
        ((dx == -1) && (dz ==  0) && (item->angle.y == -ANGLE_90)))
    {
        return item->pos.y;
    }

    return WALL;
}

int32 getDrawBridgeFloor(const Item* item, int32 x, int32 z)
{
    int32 dx = (item->pos.x >> 10) - (x >> 10);
    int32 dz = (item->pos.z >> 10) - (z >> 10);

    if (((dx == 0) && ((dz == -1) || (dz == -2)) && (item->angle.y ==  ANGLE_0))   ||
        ((dx == 0) && ((dz ==  1) || (dz ==  2)) && (item->angle.y ==  ANGLE_180)) ||
        ((dz == 0) && ((dx == -1) || (dz == -2)) && (item->angle.y ==  ANGLE_90))  ||
        ((dz == 0) && ((dx ==  1) || (dz ==  2)) && (item->angle.y == -ANGLE_90)))
    {
        return item->pos.y;
    }

    return WALL;
}

void getItemFloorCeiling(const Item* item, int32 x, int32 y, int32 z, int32* floor, int32* ceiling)
{
    int32 h = WALL;

    switch (item->type)
    {
        case ITEM_TRAP_FLOOR:
        {
            if (item->state == 0 || item->state == 1) {
                h = item->pos.y - 512;
            }
            break;
        }
        case ITEM_DRAWBRIDGE:
        {
            if (item->state == 1) {
                h = getDrawBridgeFloor(item, x, z);
            }
            break;
        }
        case ITEM_BRIDGE_FLAT:
        case ITEM_BRIDGE_TILT_1:
        case ITEM_BRIDGE_TILT_2:
        {
            h = getBridgeFloor(item, x, z);
            break;
        }
        case ITEM_TRAP_DOOR_1:
        case ITEM_TRAP_DOOR_2:
        {
            if (item->state != 0)
                return;

            h = getTrapDoorFloor(item, x, z);

            if ((floor && (h >= *floor)) || (ceiling && (h <= *ceiling)))
            {
                return;
            }
        }
    }

    if (h == WALL)
        return;

    if (floor && (y <= h))
    {
        *floor = h;
    }

    if (ceiling && (y > h))
    {
        *ceiling = h + 256;
    }
}


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

    gLastFloorSlant.value = 0;

    if (lowerSector->floorIndex)
    {
        const FloorData* fd = level.floors + lowerSector->floorIndex;
        FloorData::Command cmd = (fd++)->cmd;

        if (cmd.func == FLOOR_TYPE_FLOOR) // found floor
        {
            gLastFloorSlant = *fd;
            int32 sx = fd->slantX;
            int32 sz = fd->slantZ;
            int32 dx = x & 1023;
            int32 dz = z & 1023;
            floor -= sx * (sx < 0 ? dx : (dx - 1023)) >> 2;
            floor -= sz * (sz < 0 ? dz : (dz - 1023)) >> 2;
        }
    }

    lowerSector->getTriggerFloorCeiling(x, y, z, &floor, NULL);

    return floor;
}

int32 Sector::getCeiling(int32 x, int32 y, int32 z) const
{
    const Sector* upperSector = getSectorAbove(x, z);

    int32 ceiling = upperSector->ceiling << 8;

    if (upperSector->floorIndex)
    {
        const FloorData* fd = level.floors + upperSector->floorIndex;
        FloorData::Command cmd = (fd++)->cmd;

        if (cmd.func == FLOOR_TYPE_FLOOR) // skip floor
        {
            fd++;
            cmd = (fd++)->cmd;
        }

        if (cmd.func == FLOOR_TYPE_CEILING) // found ceiling
        {
            int32 sx = fd->slantX;
            int32 sz = fd->slantZ;
            int32 dx = x & 1023;
            int32 dz = z & 1023;
            ceiling -= sx * (sx < 0 ? (dx - 1023) : dx) >> 2;
            ceiling += sz * (sz < 0 ? dz : (dz - 1023)) >> 2;
        }
    }

    const Sector* lowerSector = getSectorBelow(x, z);

    lowerSector->getTriggerFloorCeiling(x, y, z, NULL, &ceiling);

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
    FloorData::Command cmd = (fd++)->cmd;

    if (cmd.func == FLOOR_TYPE_FLOOR)  // skip floor
    {
        if (cmd.end) return NULL;
        fd++;
        cmd = (fd++)->cmd;
    }

    if (cmd.func == FLOOR_TYPE_CEILING) // skip ceiling
    {
        if (cmd.end) return NULL;
        fd++;
        cmd = (fd++)->cmd;
    }

    if (cmd.func != FLOOR_TYPE_PORTAL) // no portal
        return NULL;

    ASSERT(fd->value != NO_ROOM);

    return rooms + fd->value;
}

void Sector::getTriggerFloorCeiling(int32 x, int32 y, int32 z, int32* floor, int32* ceiling) const
{
    if (!floorIndex)
        return;

    FloorData::Command cmd;
    const FloorData* fd = level.floors + floorIndex;

    do {
        cmd = (fd++)->cmd;
            
        switch (cmd.func)
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
                FloorData::TriggerCommand trigger;

                do {
                    trigger = (fd++)->triggerCmd;

                    if (trigger.action == TRIGGER_ACTION_ACTIVATE_OBJECT)
                    {
                        getItemFloorCeiling(items + trigger.args, x, y, z, floor, ceiling);
                    }

                    if (trigger.action == TRIGGER_ACTION_ACTIVATE_CAMERA)
                    {
                        trigger = (fd++)->triggerCmd; // skip camera index
                    }

                } while (!trigger.end);

                break;
            }

            case FLOOR_TYPE_LAVA:
                if (floor) {
                    gLastFloorData = fd - 1;
                }
                break;
        }

    } while (!cmd.end);
}


const Sector* Room::getSector(int32 x, int32 z) const
{
    int32 sx = X_CLAMP((x - (info->x << 8)) >> 10, 0, info->xSectors - 1);
    int32 sz = X_CLAMP((z - (info->z << 8)) >> 10, 0, info->zSectors - 1);

    return sectors + sx * info->zSectors + sz;
}

const Sector* Room::getWaterSector(int32 x, int32 z) const
{
    const Room* room = this;
    const Sector* sector = room->getSector(x, z);

    // go up to the air
    if (room->info->flags.water)
    {
        while (sector->roomAbove != NO_ROOM)
        {
            room = rooms + sector->roomAbove;

            if (!room->info->flags.water) {
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

        if (room->info->flags.water) {
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

bool Room::checkPortal(const Portal* portal)
{
    vec3i d;
    d.x = portal->v[0].x - cameraViewPos.x + (info->x << 8);
    d.y = portal->v[0].y - cameraViewPos.y;
    d.z = portal->v[0].z - cameraViewPos.z + (info->z << 8);

    if (DP33(portal->n, d) >= 0) {
        return false;
    }

    int32 x0 = clip.x1;
    int32 y0 = clip.y1;
    int32 x1 = clip.x0;
    int32 y1 = clip.y0;

    int32 znear = 0, zfar = 0;

    Matrix &m = matrixGet();

    vec3i pv[4];

    for (int32 i = 0; i < 4; i++)
    {
        const vec3s &v = portal->v[i];

        int32 x = DP43(m[0], v);
        int32 y = DP43(m[1], v);
        int32 z = DP43(m[2], v);

        pv[i].x = x;
        pv[i].y = y;
        pv[i].z = z;

        if (z <= VIEW_MIN_F) {
            znear++;
            continue;
        }

        if (z >= VIEW_MAX_F) {
            z = VIEW_MAX_F;
            zfar++;
        }

        if (z != 0) {
            PERSPECTIVE(x, y, z);

            x += FRAME_WIDTH  >> 1;
            y += FRAME_HEIGHT >> 1;
        } else {
            x = (x > 0) ? viewport.x1 : viewport.x0;
            y = (y > 0) ? viewport.y1 : viewport.y0;
        }

        if (x < x0) x0 = x;
        if (x > x1) x1 = x;
        if (y < y0) y0 = y;
        if (y > y1) y1 = y;
    }

    if (znear == 4 || zfar == 4) return false;

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

    if (x0 >= x1 || y0 >= y1) return false;

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
    matrixTranslateAbs(vec3i(info->x << 8, 0, info->z << 8));

    for (int32 i = 0; i < info->portalsCount; i++)
    {
        const Portal* portal = data.portals + i;

        if (checkPortal(portal))
        {
            Room* nextRoom = rooms + portal->roomIndex;

            list = nextRoom->addVisibleRoom(list);

            if (!nextRoom->visible) {
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
    Room** list = roomsList;

    list = addVisibleRoom(list);
    *list++ = this;
    *list++ = NULL;

    ASSERT(list - roomsList <= MAX_ROOM_LIST);

    return roomsList;
}

void Room::reset()
{
    visible = false;
    clip = Rect( FRAME_WIDTH, FRAME_HEIGHT, 0, 0 );
}

Room** Room::addNearRoom(Room** list, int32 x, int32 y, int32 z)
{
    Room* nearRoom = getRoom(x, y, z);

    int32 count = list - roomsList;
    for (int32 i = 0; i < count; i++)
    {
        if (roomsList[i] == nearRoom) return list;
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
    if (sectors == data.sectors)
    {
        // convert room->sectors to mutable (non-ROM) data
        sectors = dynSectors + dynSectorsCount;
        memcpy((Sector*)sectors, data.sectors, info->xSectors * info->zSectors * sizeof(Sector));

        dynSectorsCount += info->xSectors * info->zSectors;
        //printf("dynSectors: %d\n", dynSectorsCount);
        ASSERT(dynSectorsCount <= MAX_DYN_SECTORS);
    }
}

void Room::add(Item* item)
{
    ASSERT(item && item->nextItem == NULL);

    item->room = this;
    item->nextItem = firstItem;
    firstItem = item;
}

void Room::remove(Item* item)
{
    ASSERT(item && item->room == this);

    item->room = NULL;

    Item* prev = NULL;
    Item* curr = firstItem;

    while (curr)
    {
        Item* next = curr->nextItem;

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

void checkCamera(const FloorData* fd)
{
    if (camera.mode == CAMERA_MODE_OBJECT)
        return;

    while (1)
    {
        FloorData::TriggerCommand triggerCmd = (fd++)->triggerCmd;

        switch (triggerCmd.action)
        {
            case TRIGGER_ACTION_ACTIVATE_CAMERA:
            {
                triggerCmd.end = (fd++)->triggerCmd.end;

                if (triggerCmd.args != camera.lastIndex)
                    break;

                camera.index = triggerCmd.args;

                if (camera.timer < 0 || camera.mode == CAMERA_MODE_LOOK || camera.mode == CAMERA_MODE_COMBAT)
                {
                    camera.timer = -1;
                    break;
                }

                camera.mode = CAMERA_MODE_FIXED;
                break;
            }

            case TRIGGER_ACTION_CAMERA_TARGET:
            {
                if (camera.mode == CAMERA_MODE_LOOK || camera.mode == CAMERA_MODE_COMBAT)
                    break;

                ASSERT(triggerCmd.args < level.itemsCount);
                camera.lookAtItem = items + triggerCmd.args;
                break;
            }

            case TRIGGER_ACTION_FLYBY:
            {
                triggerCmd.end = (fd++)->triggerCmd.end;
                break;
            }
        }

        if (triggerCmd.end) break;
    };
}

void checkTrigger(const FloorData* fd, Item* lara)
{
    if (!fd)
        return;

    if (fd->cmd.func == FLOOR_TYPE_LAVA)
    {
        // TODO lava

        if (fd->cmd.end)
            return;

        fd++;
    }

    FloorData::Command cmd = (fd++)->cmd;
    FloorData::TriggerInfo info = (fd++)->triggerInfo;

    Item* switchItem = NULL;
    Item* cameraItem = NULL;

    checkCamera(fd);

    if (!lara && cmd.type != TRIGGER_TYPE_OBJECT)
        return;

    if (lara)
    {
        switch (cmd.type)
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
                switchItem = items + fd->triggerCmd.args;
                if (!useSwitch(switchItem, info.timer))
                    return;
                fd++;
                break;
            }

            case TRIGGER_TYPE_KEY:
            {
                Item* keyItem = items + fd->triggerCmd.args;
                if (!useKey(keyItem, lara))
                    return;
                fd++;
                break;
            }

            case TRIGGER_TYPE_PICKUP:
            {
                Item* pickupItem = items + fd->triggerCmd.args;
                if (!usePickup(pickupItem))
                    return;
                fd++;
                break;
            }
                
            case TRIGGER_TYPE_OBJECT:
                return;

            case TRIGGER_TYPE_COMBAT:
            {
                if (lara->weaponState != WEAPON_STATE_READY)
                    return;
                break;
            }

            case TRIGGER_TYPE_DUMMY:
                return;
        }
    }

    while (1)
    {
        FloorData::TriggerCommand triggerCmd = (fd++)->triggerCmd;

        switch (triggerCmd.action)
        {
            case TRIGGER_ACTION_ACTIVATE_OBJECT:
            {
                ASSERT(triggerCmd.args < level.itemsCount);
                Item* item = items + triggerCmd.args;
                
                if (item->flags.once)
                    break;

                item->timer = info.timer;
                if (item->timer != 1) {
                    item->timer *= 30;
                }

                if (cmd.type == TRIGGER_TYPE_SWITCH) {
                    item->flags.mask ^= info.mask;
                } else if (cmd.type == TRIGGER_TYPE_ANTIPAD) {
                    item->flags.mask &= ~info.mask;
                } else {
                    item->flags.mask |= info.mask;
                }

                if (item->flags.mask != ITEM_FLAGS_MASK_ALL)
                    break;

                item->flags.once |= info.once;

                if (item->flags.active)
                    break;

                item->flags.status = ITEM_FLAGS_STATUS_ACTIVE;

                item->activate();
                break;
            }

            case TRIGGER_ACTION_ACTIVATE_CAMERA:
            {
                FloorData::TriggerCommand cam = (fd++)->triggerCmd;
                triggerCmd.end = cam.end;

                if (cameras[triggerCmd.args].flags.once)
                    break;

                camera.index = triggerCmd.args;

                if (camera.mode == CAMERA_MODE_LOOK || camera.mode == CAMERA_MODE_COMBAT)
                    break;

                if (cmd.type == TRIGGER_TYPE_COMBAT)
                    break;

                if (cmd.type == TRIGGER_TYPE_SWITCH && (switchItem->state == 1) && (info.timer != 0))
                    break;

                if (cmd.type == TRIGGER_TYPE_SWITCH || camera.index != camera.lastIndex)
                {
                    camera.timer = cam.timer;
                    if (camera.timer != 1) {
                        camera.timer *= 30;
                    }

                    if (cam.once) {
                        cameras[camera.index].flags.once = true;
                    }
                
                    camera.speed = (cam.speed << 3) + 1;
                    camera.mode = lara ? CAMERA_MODE_FIXED : CAMERA_MODE_OBJECT;
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
                cameraItem = items + triggerCmd.args;
                break;
            }

            case TRIGGER_ACTION_END:
                // TODO go to the next level
                break;

            case TRIGGER_ACTION_SOUNDTRACK:
            {
                int32 track = doTutorial(lara, triggerCmd.args);

                if (track == 0) break;

                SaveGame::TrackFlags &flags = gSaveGame.tracks[track];

                if (flags.once)
                    break;

                if (cmd.type == TRIGGER_TYPE_SWITCH)
                    flags.mask ^= info.mask;
                else if (cmd.type == TRIGGER_TYPE_ANTIPAD)
                    flags.mask &= ~info.mask;
                else
                    flags.mask |= info.mask;

                if (flags.mask == ITEM_FLAGS_MASK_ALL) {
                    flags.once |= info.once;
                    musicPlay(track);
                } else {
                    musicStop();
                }
                break;
            }

            case TRIGGER_ACTION_EFFECT:
                // TODO effect
                break;

            case TRIGGER_ACTION_SECRET:
            {
                if ((gSaveGame.secrets >> triggerCmd.args) & 1)
                    break;

                gSaveGame.secrets |= (1 << triggerCmd.args);
                mixer.playMusic(TRACK_13_WAV); // TODO play sample?
                break;
            }

            case TRIGGER_ACTION_CLEAR_BODIES:
                break;

            case TRIGGER_ACTION_FLYBY:
                triggerCmd.end = (fd++)->triggerCmd.end;
                break;

            case TRIGGER_ACTION_CUTSCENE:
                break;
        }

        if (triggerCmd.end) break;
    };

    if (cameraItem && (camera.mode == CAMERA_MODE_FIXED || camera.mode == CAMERA_MODE_OBJECT))
    {
        camera.lookAtItem = cameraItem;
    }
}

#endif

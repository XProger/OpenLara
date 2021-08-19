#ifndef H_LEVEL
#define H_LEVEL

#include "common.h"
#include "camera.h"

Level level;

const Sprite* sprites;
const uint8* tiles;

#ifndef MODE_PAL
extern uint16 palette[256];
#endif

IWRAM_DATA uint8 lightmap[256 * 32]; // IWRAM 8k

EWRAM_DATA Item items[MAX_ITEMS];

#define MAX_DYN_SECTORS     (1024*3)
int32                       dynSectorsCount;
EWRAM_DATA Sector dynSectors[MAX_DYN_SECTORS];   // EWRAM 8k
EWRAM_DATA Texture textures[MAX_TEXTURES];
EWRAM_DATA Room rooms[MAX_ROOMS];
EWRAM_DATA Model models[MAX_MODELS];
EWRAM_DATA const Mesh* meshes[MAX_MESHES];
EWRAM_DATA StaticMesh staticMeshes[MAX_STATIC_MESHES];
EWRAM_DATA FixedCamera cameras[MAX_CAMERAS];

Item* Item::sFirstActive;
Item* Item::sFirstFree;

Room* roomsList[MAX_ROOM_LIST];

void readLevel(const uint8* data)
{
    memcpy(&level, data, sizeof(level));
    
    { // fix level data offsets
        uint32* ptr = (uint32*)&level.palette;
        while (ptr <= (uint32*)&level.soundOffsets)
        {
            *ptr++ += (uint32)data;
        }
    }

    { // prepare rooms
        for (int32 i = 0; i < level.roomsCount; i++)
        {
            Room* room = rooms + i;
            room->info = level.roomsInfo + i;
            room->data = room->info->data;

            for (uint32 j = 0; j < sizeof(room->data) / 4; j++)
            {
                int32* x = (int32*)&room->data + j;
                *x += (int32)data;
            }

            room->sectors = room->data.sectors;
            room->firstItem = NULL;
        }
    }

    // initialize global pointers
#ifdef MODE_PAL
    paletteSet(level.palette);
#else
    memcpy(palette, level.palette, sizeof(palette));
#endif

    memcpy(lightmap, level.lightmap, sizeof(lightmap));

    tiles = level.tiles;

    // prepare textures (required by anim tex logic)
    memcpy(textures, level.textures, level.texturesCount * sizeof(Texture));

    sprites = level.sprites;

    // prepare models // TODO prerocess
    memset(models, 0, sizeof(models));
    for (int32 i = 0; i < level.modelsCount; i++)
    {
        const Model* model = level.models + i;
        models[model->type] = *model;
    }

    // prepare meshes
    for (int32 i = 0; i < level.meshesCount; i++)
    {
        meshes[i] = (Mesh*)(level.meshData + level.meshOffsets[i]);
    }

    //  prepare static meshes // TODO preprocess
    memset(staticMeshes, 0, sizeof(staticMeshes));
    for (int32 i = 0; i < level.staticMeshesCount; i++)
    {
        const StaticMesh* staticMesh = level.staticMeshes + i;

        ASSERT(staticMesh->id < MAX_STATIC_MESHES);
        staticMeshes[staticMesh->id] = *staticMesh;
    }

    // prepare sprites // TODO preprocess
    for (int32 i = 0; i < level.spriteSequencesCount; i++)
    {
        const SpriteSeq* spriteSeq = level.spriteSequences + i;

        if (spriteSeq->type >= TR1_ITEM_MAX) // WTF?
            continue;

        Model* m = models + spriteSeq->type;
        m->count = int8(spriteSeq->count);
        m->start = spriteSeq->start;
    }

    // prepare fixed cameras
    memcpy(cameras, level.cameras, level.camerasCount * sizeof(FixedCamera));
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
            to.pos.x = from.pos.x + (to.pos.x - from.pos.x) * h / dy;
            to.pos.z = from.pos.z + (to.pos.z - from.pos.z) * h / dy;
            return false;
        }
    }

    return res;
}

void checkCamera(const FloorData* fd)
{
    if (gCamera->mode == CAMERA_MODE_OBJECT)
        return;

    bool checkItem = true;

    while (1)
    {
        FloorData::TriggerCommand triggerCmd = (fd++)->triggerCmd;

        switch (triggerCmd.action)
        {
            case TRIGGER_ACTION_ACTIVATE_CAMERA:
            {
                triggerCmd.end = (fd++)->triggerCmd.end;

                if (triggerCmd.args != gCamera->lastIndex)
                {
                    gCamera->lookAtItem = NULL;
                    break;
                }

                gCamera->index = triggerCmd.args;

                if (gCamera->timer < 0 || gCamera->mode == CAMERA_MODE_LOOK || gCamera->mode == CAMERA_MODE_COMBAT)
                {
                    gCamera->timer = -1;
                    gCamera->lookAtItem = NULL;
                    break;
                }

                gCamera->mode = CAMERA_MODE_FIXED;
                checkItem = false;
                break;
            }

            case TRIGGER_ACTION_CAMERA_TARGET:
            {
                if (gCamera->mode == CAMERA_MODE_LOOK || gCamera->mode == CAMERA_MODE_COMBAT)
                    break;

                ASSERT(triggerCmd.args < level.itemsCount);
                gCamera->lookAtItem = items + triggerCmd.args;
                break;
            }

            case TRIGGER_ACTION_FLYBY:
            {
                triggerCmd.end = (fd++)->triggerCmd.end;
                break;
            }
        }

        if (triggerCmd.end)
            break;
    };

    if (checkItem && gCamera->lookAtItem && gCamera->lookAtItem != gCamera->lastItem && gCamera->lookAtItem->flags.animated) {
        gCamera->lookAtItem = NULL;
    }
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

                item->activate();

                if (item->flags.status == ITEM_FLAGS_STATUS_NONE) {
                    item->flags.status = ITEM_FLAGS_STATUS_ACTIVE;
                }

                break;
            }

            case TRIGGER_ACTION_ACTIVATE_CAMERA:
            {
                FloorData::TriggerCommand cam = (fd++)->triggerCmd;
                triggerCmd.end = cam.end;

                if (cameras[triggerCmd.args].flags.once)
                    break;

                gCamera->index = triggerCmd.args;

                if (gCamera->mode == CAMERA_MODE_LOOK || gCamera->mode == CAMERA_MODE_COMBAT)
                    break;

                if (cmd.type == TRIGGER_TYPE_COMBAT)
                    break;

                if (cmd.type == TRIGGER_TYPE_SWITCH && (switchItem->state == 1) && (info.timer != 0))
                    break;

                if (cmd.type == TRIGGER_TYPE_SWITCH || gCamera->index != gCamera->lastIndex)
                {
                    gCamera->timer = cam.timer;
                    if (gCamera->timer != 1) {
                        gCamera->timer *= 30;
                    }

                    if (cam.once) {
                        cameras[gCamera->index].flags.once = true;
                    }
                
                    gCamera->speed = (cam.speed << 3) + 1;
                    gCamera->mode = lara ? CAMERA_MODE_FIXED : CAMERA_MODE_OBJECT;
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
                if (gSaveGame.secrets & (1 << triggerCmd.args))
                    break;
                gSaveGame.secrets |= (1 << triggerCmd.args);
                musicPlay(13);
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

        if (triggerCmd.end)
            break;
    };

    if (cameraItem && (gCamera->mode == CAMERA_MODE_FIXED || gCamera->mode == CAMERA_MODE_OBJECT))
    {
        gCamera->lookAtItem = cameraItem;
    }
}

#endif

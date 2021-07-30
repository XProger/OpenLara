#ifndef H_GAME
#define H_GAME

#include "common.h"
#include "sound.h"
#include "level.h"
#include "room.h"
#include "camera.h"
#include "item.h"
#include "draw.h"

struct Game
{
    int32 animTexFrame;

    void init()
    {
        gSettings.controls.retarget = true;

        set_seed_logic(osGetSystemTimeMS() * 3);
        set_seed_draw(osGetSystemTimeMS() * 7);

        animTexFrame = 0;

        loadLevel(levelData);
    }

    void loadLevel(const void* data)
    {
        drawFree();

        memset(&gSaveGame, 0, sizeof(gSaveGame));

        Item::sFirstActive = NULL;
        Item::sFirstFree = NULL;

        gCurTrack = -1;

        dynSectorsCount = 0;

        readLevel((uint8*)data);

        // prepare rooms
        for (int32 i = 0; i < level.roomsCount; i++)
        {
            rooms[i].reset();
        }

        // prepare items free list
        for (int32 i = MAX_ITEMS - 1; i >= level.itemsCount; i--)
        {
            items[i].nextItem = items + i + 1;
        }
        Item::sFirstFree = items + level.itemsCount;

        // init items
        for (int32 i = 0; i < level.itemsCount; i++)
        {
            const ItemInfo* info = level.itemsInfo + i;
            Item* item = items + i;

            item->type = info->type;
            item->intensity = info->intensity;

            item->pos.x = info->pos.x + (rooms[info->roomIndex].info->x << 8);
            item->pos.y = info->pos.y;
            item->pos.z = info->pos.z + (rooms[info->roomIndex].info->z << 8);

            item->angle.y = (info->flags.angle - 2) * ANGLE_90;
            item->flags.value = info->flags.value;

            if (item->type == ITEM_LARA) {
                players[0] = (Lara*)item;
            }

            item->init(rooms + info->roomIndex);

            if (item->type == ITEM_LARA || item->type == ITEM_CUT_1)
            {
                ASSERT(item->extraL);

                players[0] = (Lara*)item;
                players[0]->extraL->camera = &viewCameras[0];
                players[0]->extraL->camera->init(item);

            // gym
                //resetLara(item, 13, vec3i(38953, 3328, 63961), ANGLE_90 + ANGLE_45); // pool
            // level 1
                //resetLara(item, 0, vec3i(74588, 3072, 19673), ANGLE_0); // first darts
                //resetLara(item, 9, vec3i(49669, 7680, 57891), ANGLE_0); // first door
                //resetLara(item, 10, vec3i(43063, 7168, 61198), ANGLE_0); // transp
                //resetLara(item, 14, vec3i(20215, 6656, 52942), ANGLE_90 + ANGLE_45); // bridge
                //resetLara(item, 17, vec3i(16475, 6656, 59845), ANGLE_90); // bear
                //resetLara(item, 26, vec3i(24475, 6912, 83505), ANGLE_90); // switch timer 1
                //resetLara(item, 35, vec3i(35149, 2048, 74189), ANGLE_90); // switch timer 2
            // level 2
                //resetLara(item, 15, vec3i(66179, 0, 25920), -ANGLE_90 - ANGLE_45); // sprites
                //resetLara(item, 19, vec3i(61018, 1024, 31214), ANGLE_180); // block
                //resetLara(item, 14, vec3i(64026, 512, 20806), ANGLE_0); // key and puzzle
                //resetLara(item, 5, vec3i(55644, 0, 29155), -ANGLE_90); // keyhole
                //resetLara(item, 71, vec3i(12705, -768, 30195), -ANGLE_90); // puzzle
                //resetLara(item, 63, vec3i(31055, -2048, 33406), ANGLE_0); // right room
                //resetLara(item, 44, vec3i(27868, -1024, 29191), -ANGLE_90); // swing blades
            // level 3a
                //resetLara(item, 44, vec3i(73798, 2304, 9819), ANGLE_90); // uw gears
            }
        }

        drawInit();
    }

    void resetLara(Item* lara, int32 roomIndex, const vec3i &pos, int32 angleY)
    {
        lara->room->remove(lara);

        lara->pos = pos;
        lara->angle.y = angleY;
        lara->health = LARA_MAX_HEALTH;

        lara->extraL->camera->target.pos = lara->pos;
        lara->extraL->camera->target.room = lara->room;
        lara->extraL->camera->view = lara->extraL->camera->target;

        rooms[roomIndex].add(lara);
    }

    void updateItems()
    {
        Item* item = Item::sFirstActive;
        while (item)
        {
            Item* next = item->nextActive;
            item->update();
            item = next;
        }

        for (int32 i = 0; i < MAX_PLAYERS; i++)
        {
            if (players[i]) {
                players[i]->update();
            }
        }
    }

    void nextFrame(int32 frames)
    {
        causticsFrame += frames;

        animTexFrame += frames;
        while (animTexFrame > 5)
        {
            animTexturesShift();
            animTexFrame -= 5;
        }
    }

    void update(int32 frames)
    {
        PROFILE(CNT_UPDATE);

        if (frames > MAX_UPDATE_FRAMES) {
            frames = MAX_UPDATE_FRAMES;
        }

        for (int32 i = 0; i < frames; i++)
        {
            updateItems();
        }

        nextFrame(frames);

        if (keys & IK_SELECT) {
            musicPlay(4);
        }
    }

    void render()
    {
        #define TEXT_POSX   FRAME_WIDTH

        {
            PROFILE(CNT_RENDER);

            clear();

            for (int32 i = 0; i < MAX_PLAYERS; i++)
            {
                gCamera = players[0]->extraL->camera;
                ASSERT(gCamera);
                // TODO set viewports for coop
            #ifndef PROFILE_SOUNDTIME
                drawRooms();
            #endif
            }

            drawNumber(fps, TEXT_POSX, 16);
        }

        #ifdef PROFILING
            for (int32 i = 0; i < CNT_MAX; i++)
            {
                drawNumber(gCounters[i], TEXT_POSX, 32 + i * 16);
            }
        #endif

    #ifndef PROFILE_SOUNDTIME
        PROFILE_CLEAR();
    #endif
    }
};

#endif

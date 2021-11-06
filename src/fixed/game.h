#ifndef H_GAME
#define H_GAME

#include "common.h"
#include "room.h"
#include "camera.h"
#include "item.h"
#include "draw.h"
#include "nav.h"
#include "level.h"

struct Game
{
    int32 animTexFrame;

    void init(const char* name)
    {
        renderInit();

        gSettings.controls.retarget = true;

        startLevel(name);
    }

    void startLevel(const char* name)
    {
        set_seed_logic(osGetSystemTimeMS() * 3);
        set_seed_draw(osGetSystemTimeMS() * 7);

        animTexFrame = 0;

        void* data = osLoadLevel(name);
        loadLevel(data);
    }

    void loadLevel(const void* data)
    {
        drawFree();

        memset(&gSaveGame, 0, sizeof(gSaveGame));

        ItemObj::sFirstActive = NULL;
        ItemObj::sFirstFree = NULL;

        gCurTrack = -1;

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
        ItemObj::sFirstFree = items + level.itemsCount;

        // init items
        for (int32 i = 0; i < level.itemsCount; i++)
        {
            const ItemObjInfo* info = level.itemsInfo + i;
            ItemObj* item = items + i;

            item->type = info->type;
            item->intensity = uint8(info->intensity);

            item->pos.x = info->pos.x + (rooms[info->roomIndex].info->x << 8);
            item->pos.y = info->pos.y;
            item->pos.z = info->pos.z + (rooms[info->roomIndex].info->z << 8);

            item->angle.y = ((info->params.value >> 14) - 2) * ANGLE_90;
            item->flags = info->params.flags;
            item->flags.reverse = 0;
            item->flags.shadow = 0;

            if (item->type == ITEM_LARA || item->type == ITEM_CUT_1) {
                players[0] = (Lara*)item;
            }

            item->init(rooms + info->roomIndex);
        }

    // gym
        //resetLara(0, 8, _vec3i(55994, 0, 52603), ANGLE_90); // piano
        //resetLara(0, 13, _vec3i(38953, 3328, 63961), ANGLE_90 + ANGLE_45); // pool
    // level 1
        //resetLara(0, 0, _vec3i(74588, 3072, 19673), ANGLE_0); // first darts
        //resetLara(0, 9, _vec3i(49669, 7680, 57891), ANGLE_0); // first door
        //resetLara(0, 10, _vec3i(43063, 7168, 61198), ANGLE_0); // transp
        //resetLara(0, 14, _vec3i(20215, 6656, 52942), ANGLE_90 + ANGLE_45); // bridge
        //resetLara(0, 17, _vec3i(16475, 6656, 59845), ANGLE_90); // bear
        //resetLara(0, 26, _vec3i(24475, 6912, 83505), ANGLE_90); // switch timer 1
        //resetLara(0, 35, _vec3i(35149, 2048, 74189), ANGLE_90); // switch timer 2
    // level 2
        //resetLara(0, 15, _vec3i(66179, 0, 25920), -ANGLE_90 - ANGLE_45); // sprites
        //resetLara(0, 19, _vec3i(61018, 1024, 31214), ANGLE_180); // block
        //resetLara(0, 14, _vec3i(64026, 512, 20806), ANGLE_0); // key and puzzle
        //resetLara(0, 5, _vec3i(55644, 0, 29155), -ANGLE_90); // keyhole
        //resetLara(0, 71, _vec3i(12705, -768, 30195), -ANGLE_90); // puzzle
        //resetLara(0, 63, _vec3i(31055, -2048, 33406), ANGLE_0); // right room
        //resetLara(0, 44, _vec3i(27868, -1024, 29191), -ANGLE_90); // swing blades
    // level 3a
        //resetLara(0, 44, _vec3i(73798, 2304, 9819), ANGLE_90); // uw gears

        drawInit();

    #ifdef __3DO__
        //players[0]->angle.y += ANGLE_180;
        //players[0]->pos.x += 1024;
    #endif
    }

    void resetLara(int32 index, int32 roomIndex, const vec3i &pos, int32 angleY)
    {
        Lara* lara = players[index];

        lara->room->remove(lara);

        lara->pos = pos;
        lara->angle.y = angleY;
        lara->health = LARA_MAX_HEALTH;

        lara->extraL->camera.target.pos = lara->pos;
        lara->extraL->camera.target.room = lara->room;
        lara->extraL->camera.view = lara->extraL->camera.target;

        rooms[roomIndex].add(lara);
    }

    void updateItems()
    {
        ItemObj* item = ItemObj::sFirstActive;
        while (item)
        {
            ItemObj* next = item->nextActive;
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
    }

    void render()
    {
        #define TEXT_POSX   FRAME_WIDTH

        {
            PROFILE(CNT_RENDER);

            setViewport(RectMinMax(0, 0, FRAME_WIDTH, FRAME_HEIGHT));

            clear();

            for (int32 i = 0; i < MAX_PLAYERS; i++)
            {
                // TODO set viewports for coop
            #ifndef PROFILE_SOUNDTIME
                drawRooms(&players[i]->extraL->camera);
            #endif
            }

            drawText(0, FRAME_HEIGHT - 8, "! early alpha version !", TEXT_ALIGN_CENTER);
            flush();

            drawNumber(fps, TEXT_POSX, 16);
        }

        #ifdef PROFILING
            for (int32 i = 0; i < CNT_MAX; i++)
            {
            #ifdef __3DO__
                extern void drawInt(int32 x, int32 y, int32 c);
                drawInt(FRAME_WIDTH - 8, 4 + 24 + 8 + 8 * i, gCounters[i]);
            #else
                drawNumber(gCounters[i], TEXT_POSX, 32 + i * 16);
            #endif
            }
        #endif

    #ifndef PROFILE_SOUNDTIME
        PROFILE_CLEAR();
    #endif
    }
};

#endif

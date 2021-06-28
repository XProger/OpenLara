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
        set_seed_ctrl(osGetSystemTimeMS() * 3);
        set_seed_draw(osGetSystemTimeMS() * 7);

        animTexFrame = 0;

        loadLevel(levelData);
    }

    void loadLevel(const void* data)
    {
        drawFree();
        readLevel((uint8*)data);

    // prepare rooms
        for (int32 i = 0; i < level.roomsCount; i++)
        {
            rooms[i].reset();
        }

    // prepare items
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

            item->init(rooms + info->roomIndex);

            if (item->type == ITEM_LARA)
            {
                camera.init(item);
                camera.laraItem = item;

            //#ifdef PROFILE
            // gym
                //resetItem(item, 13, vec3i(38953, 3328, 63961), ANGLE_90 + ANGLE_45); // pool
            // level 1
                //resetItem(item, 0, vec3i(74588, 3072, 19673), ANGLE_0); // first darts
                //resetItem(item, 9, vec3i(49669, 7680, 57891), ANGLE_0); // first door
                //resetItem(item, 10, vec3i(43063, 7168, 61198), ANGLE_0); // transp
                //resetItem(item, 14, vec3i(20215, 6656, 52942), ANGLE_90 + ANGLE_45); // bridge
                //resetItem(item, 17, vec3i(16475, 6656, 59845), ANGLE_90); // bear
                //resetItem(item, 26, vec3i(24475, 6912, 83505), ANGLE_90); // switch timer 1
                //resetItem(item, 35, vec3i(35149, 2048, 74189), ANGLE_90); // switch timer 2
            // level 2
                //resetItem(item, 15, vec3i(66179, 0, 25920), -ANGLE_90 - ANGLE_45); // sprites
                //resetItem(item, 19, vec3i(61018, 1024, 31214), ANGLE_180); // block
                //resetItem(item, 14, vec3i(64026, 512, 20806), ANGLE_0); // key and puzzle
                //resetItem(item, 5, vec3i(55644, 0, 29155), -ANGLE_90); // keyhole
                //resetItem(item, 71, vec3i(12705, -768, 30195), -ANGLE_90); // puzzle
                //resetItem(item, 63, vec3i(31055, -2048, 33406), ANGLE_0); // right room
                //resetItem(item, 44, vec3i(27868, -1024, 29191), -ANGLE_90); // swing blades
            // level 3a
                //resetItem(item, 44, vec3i(73798, 2304, 9819), ANGLE_90); // uw gears
            //#endif

                camera.view.pos = camera.target.pos = item->pos;
                camera.view.room = item->room;

                players[0] = (Lara*)item;
            }
        }

        drawInit();
    }

    void resetItem(Item* item, int32 roomIndex, const vec3i &pos, int32 angleY)
    {
        item->room->remove(item);

        item->pos = pos;
        item->angle.y = angleY;
        item->health = LARA_MAX_HEALTH;

        rooms[roomIndex].add(item);
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
        if (frames > MAX_UPDATE_FRAMES) {
            frames = MAX_UPDATE_FRAMES;
        }

        for (int32 i = 0; i < frames; i++)
        {
            updateItems();
            camera.update();
        }

        nextFrame(frames);

        if (keys & IK_SELECT) {
            mixer.playMusic(TRACK_13_WAV);
        }
    }

    void render()
    {
    #ifdef ROTATE90_MODE
        #define TEXT_POSX   FRAME_HEIGHT
    #else
        #define TEXT_POSX   FRAME_WIDTH
    #endif

        clear();

        #ifdef PROFILE
            dbg_transform = 0;
            dbg_poly = 0;
            dbg_flush = 0;
            dbg_vert_count = 0;
            dbg_poly_count = 0;
        #endif

        drawRooms();

        #ifdef PROFILE
            drawNumber(dbg_transform, TEXT_POSX, 32);
            drawNumber(dbg_poly, TEXT_POSX, 48);
            drawNumber(dbg_flush, TEXT_POSX, 64);
            drawNumber(dbg_vert_count, TEXT_POSX, 84);
            drawNumber(dbg_poly_count, TEXT_POSX, 100);
        #endif

        drawNumber(fps, TEXT_POSX, 16);
    }
};

#endif

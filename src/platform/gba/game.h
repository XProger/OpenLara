#ifndef H_GAME
#define H_GAME

#include "common.h"
#include "sound.h"
#include "level.h"
#include "room.h"
#include "camera.h"
#include "item.h"
#include "draw.h"

Lara* players[2];

Lara* getLara(const vec3i &pos)
{
    UNUSED(pos); // TODO two players
    return players[0]; // TODO find nearest player
}

struct Game
{
    void init()
    {
        loadLevel(LEVEL1_PHD);
    }

    void loadLevel(const void* data)
    {
        camera.init();

        readLevel((uint8*)data);

    // prepare rooms
        for (int32 i = 0; i < roomsCount; i++)
        {
            rooms[i].reset();
        }

    // prepare items
        for (int32 i = 0; i < itemsCount; i++)
        {
            Item* item = items + i;

            item->init(rooms + item->startRoomIndex);

            if (item->type == ITEM_LARA)
            {
                camera.item = item;

            //#ifdef PROFILE
            // debug
                //resetItem(item, 0, vec3i(74588, 3072, 19673), 0); // level 1 (first darts)
                //resetItem(item, 9, vec3i(49669, 7680, 57891), 0); // level 1 (first door)
                //resetItem(item, 10, vec3i(43063, 7168, 61198), 0); // level 1 (transp)
                //resetItem(item, 14, vec3i(20215, 6656, 52942), ANGLE_90 + ANGLE_45); // level 1 (bridge)
                //resetItem(item, 17, vec3i(16475, 6656, 59845), ANGLE_90); // level 1 (bear)
                //resetItem(item, 26, vec3i(24475, 6912, 83505), ANGLE_90); // level 1 (switch timer)
                //resetItem(item, 35, vec3i(35149, 2048, 74189), ANGLE_90); // level 1 (switch timer)
            //#endif

                camera.view.pos = camera.target.pos = item->pos;
                camera.view.room = item->room;

                players[0] = (Lara*)item;
            }
        }

    // prepare glyphs
        for (int32 i = 0; i < spritesSeqCount; i++)
        {
            if (spritesSeq[i].type == ITEM_GLYPHS)
            {
                seqGlyphs = i;
                break;
            }
        }
    }

    void resetItem(Item* item, int32 roomIndex, const vec3i &pos, int32 angleY)
    {
        item->room->remove(item);

        item->pos = pos;
        item->angleY = angleY;
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

    void update(int32 frames)
    {
    #ifdef TEST
        return;
    #endif
        if (frames > MAX_UPDATE_FRAMES) {
            frames = MAX_UPDATE_FRAMES;
        }

        for (int32 i = 0; i < frames; i++)
        {
            updateItems();
            camera.update();
        }

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

        #ifdef TEST
            #ifdef __GBA__
                VBlankIntrWait();
            #endif

            int32 cycles = 0;
            PROFILE_START();
            drawTest();
            PROFILE_STOP(cycles);

            drawNumber(cycles, TEXT_POSX, 32);
        #else
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

        #endif

        drawNumber(fps, TEXT_POSX, 16);
    }
};

#endif

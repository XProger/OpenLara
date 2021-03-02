#ifndef H_GAME
#define H_GAME

#include "common.h"
#include "sound.h"
#include "level.h"
#include "camera.h"
#include "item.h"
#include "lara.h"
#include "draw.h"

struct Game
{
    void init()
    {
        loadLevel(LEVEL1_PHD);
    }

    void loadLevel(const void* data)
    {
        camera.init();

        readLevel((uint8*)LEVEL1_PHD);

    // prepare rooms
        for (int32 i = 0; i < roomsCount; i++)
        {
            roomReset(rooms + i);
        }

    // prepare items
        for (int32 i = 0; i < itemsCount; i++)
        {
            Item* item = items + i;

            itemInit(item);

            if (item->room > -1) {
                roomItemAdd(item->room, i);
            }

            if (item->type == ITEM_LARA) {
                camera.item = item;

            // debug
                //resetItem(i, 14, vec3i(20215, 6656, 52942), 0); // level 1 (bridge)
                //resetItem(i, 26, vec3i(24475, 6912, 83505), 90 * DEG2SHORT); // level 1 (switch timer)

                camera.room = item->room;
            }

            // TODO remove
            if (item->type == ITEM_LARA ||
                item->type == ITEM_WOLF ||
                item->type == ITEM_BEAR ||
                item->type == ITEM_BAT  ||
                item->type == ITEM_CRYSTAL)
            {
                activateItem(i);
            }
        }

    // prepare glyphs
        for (int32 i = 0; i < spritesSeqCount; i++) {
            if (spritesSeq[i].type == ITEM_GLYPHS) {
                seqGlyphs = i;
                break;
            }
        }
    }

    void resetItem(int32 itemIndex, int32 roomIndex, const vec3i &pos, int32 angleY)
    {
        roomItemRemove(itemIndex);

        Item* item = items + itemIndex;
        item->pos = vec3i(20215, 6656, 52942);
        item->angleY = angleY;
        item->health = LARA_MAX_HEALTH;

        roomItemAdd(roomIndex, itemIndex);
    }

    void updateItems()
    {
        curItemIndex = firstActive;
        while (curItemIndex != NO_ITEM)
        {
            Item* item = items + curItemIndex;

            if (item->type == ITEM_LARA) {
                Lara* lara = (Lara*)item;
                lara->update();
            } else {
                itemControl(item);
            }

            curItemIndex = item->nextActive;
        }
    }

    void update(int32 frames)
    {
        if (frames > MAX_UPDATE_FRAMES) {
            frames = MAX_UPDATE_FRAMES;
        }

        for (int32 i = 0; i < frames; i++) {
            updateItems();
            camera.update();
        }
    }

#ifdef ROTATE90_MODE
    int32 TEXT_POSX = FRAME_HEIGHT;
#else
    int32 TEXT_POSX = FRAME_WIDTH;
#endif

    void render()
    {
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

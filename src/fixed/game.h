#ifndef H_GAME
#define H_GAME

#include "common.h"
#include "room.h"
#include "camera.h"
#include "item.h"
#include "draw.h"
#include "nav.h"
#include "level.h"
#include "inventory.h"

EWRAM_DATA LevelID gNextLevel = LVL_MAX;

void nextLevel(LevelID next)
{
    if ((next == LVL_TR1_3A) && (inventory.state == INV_STATE_NONE)) // alpha version
    {
        inventory.open(players[0], INV_PAGE_END);
        return;
    }
    gNextLevel = next;
}

bool gameSave()
{
    gSaveGame.version = SAVEGAME_VER;
    gSaveGame.level = gLevelID;
    gSaveGame.track = gCurTrack;
    gSaveGame.randSeedLogic = gRandSeedLogic;
    gSaveGame.randSeedDraw = gRandSeedDraw;

    memset(gSaveGame.invSlots, 0, sizeof(gSaveGame.invSlots));
    memcpy(gSaveGame.invSlots, inventory.counts, sizeof(inventory.counts));

    uint8* ptr = gSaveData;
    ItemObj* item = items;
    for (int32 i = 0; i < level.itemsCount; i++, item++)
    {
        ptr = item->save(ptr);
    }
    gSaveGame.dataSize = ptr - gSaveData;

    return osSaveGame();
}

bool gameLoad()
{
    if (!osLoadGame())
    {
        if (gSaveGame.dataSize == 0)
            return false;
    }

    SaveGame tmp = gSaveGame;
    gLevelID = (LevelID)gSaveGame.level;
    startLevel(gLevelID);
    gSaveGame = tmp;

    inventory.setSlots(gSaveGame.invSlots);

    ItemObj::sFirstActive = NULL;
    ItemObj::sFirstFree = items + level.itemsCount;

    uint8* ptr = gSaveData;
    ItemObj* item = items;
    for (int32 i = 0; i < level.itemsCount; i++, item++)
    {
        ptr = item->load(ptr);

        if (item->flags & ITEM_FLAG_ACTIVE) {
            item->activate();
        }
    }

    if (gSaveGame.track != -1) {
        sndPlayTrack(gSaveGame.track);
    }

    gRandSeedLogic = gSaveGame.randSeedLogic;
    gRandSeedDraw = gSaveGame.randSeedDraw;

    return true;
}

void gameInit()
{
    drawInit();

    gSaveGame.dataSize = 0;

    gSettings.version = SETTINGS_VER;
    gSettings.controls_vibration = 1;
    gSettings.controls_swap = 0;
    gSettings.audio_sfx = 1;
    gSettings.audio_music = 1;
    gSettings.video_gamma = 0;
    gSettings.video_fps = 1;
    gSettings.video_vsync = 0;
    osLoadSettings();

    inventory.init();

    startLevel(gLevelID);
}

void gameFree()
{
    drawLevelFree();
    drawFree();
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

void gameLoadLevel(const void* data)
{
    drawLevelFree();

    memset(&gSaveGame, 0, sizeof(gSaveGame));
    memset(enemiesExtra, 0, sizeof(enemiesExtra));

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
    items[MAX_ITEMS - 1].nextItem = NULL;
    for (int32 i = MAX_ITEMS - 2; i >= level.itemsCount; i--)
    {
        items[i].nextItem = items + i + 1;
    }
    ItemObj::sFirstFree = items + level.itemsCount;

    for (int32 i = 0; i < MAX_PLAYERS; i++)
    {
        players[i] = NULL;
    }

    if (gLevelID == LVL_TR1_TITLE) {
        // init dummy Lara for updateInput()
        items->extraL = playersExtra;
        items->extraL->camera.mode = CAMERA_MODE_FOLLOW;
        inventory.open(items, INV_PAGE_TITLE);
    } else {
        inventory.page = INV_PAGE_MAIN;

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

            item->angle.y = ((info->flags >> 14) - 2) * ANGLE_90;
            item->flags = info->flags;

            if (item->type == ITEM_LARA) {
                players[0] = (Lara*)item;
            }

            item->init(rooms + info->roomIndex);
        }

        if (isCutsceneLevel())
        {
            gCinematicCamera.initCinematic();
        }


    // gym
        //resetLara(0, 7, _vec3i(39038, -1280, 51712), ANGLE_90); // start
        //resetLara(0, 8, _vec3i(55994, 0, 52603), ANGLE_90); // piano
        //resetLara(0, 9, _vec3i(47672, 256, 40875), ANGLE_90); // hall
        //resetLara(0, 13, _vec3i(38953, 3328, 63961), ANGLE_90 + ANGLE_45); // pool
    // level 1
        //resetLara(0, 0, _vec3i(74588, 3072, 19673), ANGLE_0); // first darts
        //resetLara(0, 9, _vec3i(49669, 7680, 57891), ANGLE_0); // first door
        //resetLara(0, 10, _vec3i(43063, 7168, 61198), ANGLE_0); // transp
        //resetLara(0, 14, _vec3i(20215, 6656, 52942), ANGLE_90 + ANGLE_45); // bridge
        //resetLara(0, 25, _vec3i(8789, 5632, 80173), 0); // portal
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
        //resetLara(0, 51, _vec3i(41015, 3584, 34494), ANGLE_180); // valley
    }

    drawLevelInit();
}

void startLevel(LevelID id)
{
    gRandSeedLogic = osGetSystemTimeMS() * 3;
    gRandSeedDraw = osGetSystemTimeMS() * 7;

    sndStop();
    sndFreeSamples();

    const void* data = osLoadLevel(id);
    gameLoadLevel(data);

    sndInitSamples();
    sndPlayTrack(getAmbientTrack());
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

    if (isCutsceneLevel())
    {
        gCinematicCamera.updateCinematic();
    }
    else
    {
        for (int32 i = 0; i < MAX_PLAYERS; i++)
        {
            if (players[i]) {
                players[i]->update();
            }
        }
    }
}

void gameUpdate(int32 frames)
{
    PROFILE(CNT_UPDATE);

    if (frames > MAX_UPDATE_FRAMES) {
        frames = MAX_UPDATE_FRAMES;
    }

    if (!sndTrackIsPlaying()) {
        gCurTrack = -1;
        sndPlayTrack(getAmbientTrack());
    }

    if (inventory.state != INV_STATE_NONE)
    {
        Lara* lara = (Lara*)inventory.lara;
        ASSERT(lara);
        lara->updateInput();
        inventory.update(frames);

        if ((inventory.page != INV_PAGE_TITLE) && (inventory.state == INV_STATE_NONE))
        {
            if (lara->useItem(inventory.useSlot)) {
                inventory.useSlot = SLOT_MAX;
            }
        }
    }

    if ((inventory.page != INV_PAGE_TITLE) && (inventory.state == INV_STATE_NONE) && (gNextLevel == LVL_MAX))
    {
        for (int32 i = 0; i < frames; i++)
        {
            updateItems();
        }
        updateLevel(frames);
    }

    if ((gNextLevel != LVL_MAX) && (inventory.state == INV_STATE_NONE))
    {
        gLevelID = gNextLevel;
        gNextLevel = LVL_MAX;
        if (gLevelID == LVL_LOAD) {
            gameLoad();
        } else {
            startLevel(gLevelID);
        }
        gameUpdate(1);
    }
}

void gameRender()
{
    {
        PROFILE(CNT_RENDER);

        setViewport(RectMinMax(0, 0, FRAME_WIDTH, FRAME_HEIGHT));

        if (inventory.state == INV_STATE_NONE)
        {
            clear();

            for (int32 i = 0; i < MAX_PLAYERS; i++)
            {
                // TODO set viewports for coop
            #ifndef PROFILE_SOUNDTIME
                if (isCutsceneLevel()) {
                    drawCinematicRooms();
                } else {
                    drawRooms(&players[i]->extraL->camera);
                }
            #endif

                if (players[i])
                {
                    drawHUD(players[i]);
                }
            }
        } else {
            inventory.draw();
        }

        //if (inventory.state == INV_STATE_NONE)
        {
            drawFPS();
        }

        flush();
    }

#ifdef PROFILING
    drawProfiling();
    #ifndef PROFILE_SOUNDTIME
        PROFILE_CLEAR();
    #endif
#endif
}

#endif

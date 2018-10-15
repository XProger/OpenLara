#ifndef H_SAVEGAME
#define H_SAVEGAME

#include "utils.h"

#define MAX_FLIPMAP_COUNT     32
#define MAX_TRACKS_COUNT      256

#define LVL_FLAG_CHECKPOINT (1 << 31)
#define SAVE_FILENAME       "savegame.dat"
#define SAVE_MAGIC          FOURCC("OLS1")

enum SaveResult {
    SAVE_RESULT_SUCCESS,
    SAVE_RESULT_ERROR,
    SAVE_RESULT_WAIT,
};

struct SaveItem {
    uint16 type;
    uint16 count;
};

struct SaveProgress {
    uint32 time;
    uint32 distance;
    uint32 mediUsed;
    uint32 secrets;
    uint32 pickups;
    uint32 ammoUsed;
    uint32 kills;
};

struct SaveEntity {
// base
    int32  x, y, z;
    uint16 rotation;
    uint16 type;
    uint16 flags;
    int16  timer;
// animation
    uint16 animIndex;
    uint16 animFrame;
// common
    uint16 room;
    uint16 extraSize;
    union Extra {
        struct {
            float  velX, velY, velZ;
            float  angleX;
            float  health;
            float  oxygen;
            float  stamina;
            float  poison;
            float  freeze;
            uint16 reserved;
            uint16 itemWeapon;
            uint16 itemHands;
            uint16 itemBack;
            uint16 itemHolster;
            union {
                struct { uint16 wet:1, burn:1; };
                uint16 value;
            } spec;
        } lara;
        struct {
            float  health;
            uint16 targetBox;
            union {
                struct { uint16 mood:3; };
                uint16 value;
            } spec;
        } enemy;
    } extra;
};

struct SaveState {
    struct ByteFlags {
        uint8 once:1, active:5, :2;
    };

    ByteFlags flipmaps[MAX_FLIPMAP_COUNT];
    ByteFlags tracks[MAX_TRACKS_COUNT];

    union {
        struct { uint32 track:8, flipped:1; };
        uint32 value;
    } flags;
};

struct SaveSlot {
    uint32 level;
    uint32 size;
    uint8  *data;

    static int cmp(const SaveSlot &a, const SaveSlot &b) {
        if (a.level < b.level)
            return -1;
        if (a.level > b.level)
            return +1;
        return 0;
    }
};

Array<SaveSlot> saveSlots;
SaveResult      saveResult;
int             loadSlot;

void freeSaveSlots() {
    for (int i = 0; i < saveSlots.length; i++)
        delete[] saveSlots[i].data;
    saveSlots.clear();
}

void readSaveSlots(Stream *stream) {
    uint32 magic;
    if (stream->size < 4 || stream->read(magic) != SAVE_MAGIC)
        return;

    freeSaveSlots();

    SaveSlot slot;
    while (stream->pos < stream->size) {
        stream->read(slot.level);
        stream->read(slot.size);
        stream->read(slot.data, slot.size);
        saveSlots.push(slot);
    }
}

uint8* writeSaveSlots(int &size) {
    size = 4;
    for (int i = 0; i < saveSlots.length; i++)
        size += 4 + 4 + saveSlots[i].size;

    uint8 *data = new uint8[size];
    uint8 *ptr  = data;

    uint32 *magic = (uint32*)ptr;
    ptr += sizeof(*magic);
    *magic = SAVE_MAGIC;

    for (int i = 0; i < saveSlots.length; i++) {
        SaveSlot &s = saveSlots[i];
        memcpy(ptr + 0, &s.level, 4);
        memcpy(ptr + 4, &s.size,  4);
        memcpy(ptr + 8, s.data,   s.size);
        ptr += 4 + 4 + s.size;
    }

    return data;
}

void removeSaveSlot(TR::LevelID levelID, bool checkpoint) {
    TR::Version version = TR::getGameVersionByLevel(levelID);

    for (int i = 0; i < saveSlots.length; i++) {
        SaveSlot &slot = saveSlots[i];

        bool isCheckpointSlot = (slot.level & LVL_FLAG_CHECKPOINT) != 0;
        TR::LevelID id = TR::LevelID(slot.level & ~LVL_FLAG_CHECKPOINT);

        if (TR::getGameVersionByLevel(id) != version)
            continue;

        if (isCheckpointSlot || (!checkpoint && levelID == id)) {
            delete[] slot.data;
            saveSlots.remove(i);
            i--;
            continue;
        }
    }
}

int getSaveSlot(TR::LevelID levelID, bool checkpoint) {
    TR::Version version = TR::getGameVersionByLevel(levelID);

    for (int i = 0; i < saveSlots.length; i++) {
        SaveSlot &slot = saveSlots[i];

        bool isCheckpointSlot = (slot.level & LVL_FLAG_CHECKPOINT) != 0;
        TR::LevelID id = TR::LevelID(slot.level & ~LVL_FLAG_CHECKPOINT);

        if (TR::getGameVersionByLevel(id) != version)
            continue;

        if ((checkpoint && isCheckpointSlot) || (!checkpoint && levelID == id))
            return i;
    }

    return -1;
}


#endif

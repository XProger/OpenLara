#ifndef H_INVENTORY
#define H_INVENTORY

#include "common.h"

enum StringID {
      STR_PASSPORT
    , STR_COMPASS
    , STR_HOME
    , STR_MAP
    , STR_DETAIL
    , STR_SOUND
    , STR_CONTROLS
    , STR_GAMMA
    , STR_PISTOLS
    , STR_SHOTGUN
    , STR_MAGNUMS
    , STR_UZIS
    , STR_AMMO_PISTOLS
    , STR_AMMO_SHOTGUN
    , STR_AMMO_MAGNUMS
    , STR_AMMO_UZIS
    , STR_EXPLOSIVE
    , STR_MEDIKIT_SMALL
    , STR_MEDIKIT_BIG
    , STR_PUZZLE
    , STR_PUZZLE_GOLD_IDOL
    , STR_PUZZLE_GOLD_BAR
    , STR_PUZZLE_COG
    , STR_PUZZLE_FUSE
    , STR_PUZZLE_ANKH
    , STR_PUZZLE_HORUS
    , STR_PUZZLE_ANUBIS
    , STR_PUZZLE_SCARAB
    , STR_PUZZLE_PYRAMID
    , STR_LEADBAR
    , STR_KEY
    , STR_KEY_SILVER
    , STR_KEY_RUSTY
    , STR_KEY_GOLD
    , STR_KEY_SAPPHIRE
    , STR_KEY_NEPTUNE
    , STR_KEY_ATLAS
    , STR_KEY_DAMOCLES
    , STR_KEY_THOR
    , STR_KEY_ORNATE
    , STR_SCION
    , STR_MAX
};

const char* InvNames[STR_MAX] = {
      "Game"
    , "Compass"
    , "Lara's Home"
    , "Map"
    , "Detail Levels"
    , "Sound"
    , "Controls"
    , "Gamma"
    , "Pistols"
    , "Shotgun"
    , "Magnums"
    , "Uzis"
    , "Pistol Clips"
    , "Shotgun Shells"
    , "Magnum Clips"
    , "Uzi Clips"
    , "Explosive"
    , "Small Medi Pack"
    , "Large Medi Pack"
    , "Puzzle"
    , "Gold Idol"
    , "Gold Bar"
    , "Machine Cog"
    , "Fuse"
    , "Ankh"
    , "Eye of Horus"
    , "Seal of Anubis"
    , "Scarab"
    , "Pyramid Key"
    , "Lead Bar"
    , "Key"
    , "Silver Key"
    , "Rusty Key"
    , "Gold Key"
    , "Sapphire Key"
    , "Neptune Key"
    , "Atlas Key"
    , "Damocles Key"
    , "Thor Key"
    , "Ornate Key"
    , "Scion"
};

enum InvSlot {
// Items
      SLOT_LEADBAR
    , SLOT_KEY_ITEM_1
    , SLOT_KEY_ITEM_2
    , SLOT_KEY_ITEM_3
    , SLOT_KEY_ITEM_4
    , SLOT_PUZZLE_4
    , SLOT_PUZZLE_3
    , SLOT_PUZZLE_2
    , SLOT_PUZZLE_1
    , SLOT_SCION
// Inventory
    , SLOT_COMPASS
    , SLOT_MAP
    , SLOT_PISTOLS
    , SLOT_AMMO_PISTOLS
    , SLOT_SHOTGUN
    , SLOT_AMMO_SHOTGUN
    , SLOT_MAGNUMS
    , SLOT_AMMO_MAGNUMS
    , SLOT_UZIS
    , SLOT_AMMO_UZIS
    , SLOT_EXPLOSIVE
    , SLOT_MEDIKIT_BIG
    , SLOT_MEDIKIT_SMALL
// Options
    , SLOT_PASSPORT
    , SLOT_DETAIL
    , SLOT_SOUND
    , SLOT_CONTROLS
    , SLOT_GAMMA
    , SLOT_HOME
    , SLOT_MAX
};


struct InvItem
{
    uint8 type;
    uint8 sid;
    // TODO params
};

const InvItem INV_SLOTS[SLOT_MAX] = {
      { ITEM_INV_LEADBAR        , STR_LEADBAR       }
    , { ITEM_INV_KEY_ITEM_1     , STR_KEY           }
    , { ITEM_INV_KEY_ITEM_2     , STR_KEY           }
    , { ITEM_INV_KEY_ITEM_3     , STR_KEY           }
    , { ITEM_INV_KEY_ITEM_4     , STR_KEY           }
    , { ITEM_INV_PUZZLE_4       , STR_PUZZLE        }
    , { ITEM_INV_PUZZLE_3       , STR_PUZZLE        }
    , { ITEM_INV_PUZZLE_2       , STR_PUZZLE        }
    , { ITEM_INV_PUZZLE_1       , STR_PUZZLE        }
    , { ITEM_INV_SCION          , STR_SCION         }
    , { ITEM_INV_COMPASS        , STR_COMPASS       }
    , { ITEM_INV_MAP            , STR_MAP           }
    , { ITEM_INV_PISTOLS        , STR_PISTOLS       }
    , { ITEM_INV_AMMO_PISTOLS   , STR_AMMO_PISTOLS  }
    , { ITEM_INV_SHOTGUN        , STR_SHOTGUN       }
    , { ITEM_INV_AMMO_SHOTGUN   , STR_AMMO_SHOTGUN  }
    , { ITEM_INV_MAGNUMS        , STR_MAGNUMS       }
    , { ITEM_INV_AMMO_MAGNUMS   , STR_AMMO_MAGNUMS  }
    , { ITEM_INV_UZIS           , STR_UZIS          }
    , { ITEM_INV_AMMO_UZIS      , STR_AMMO_UZIS     }
    , { ITEM_INV_EXPLOSIVE      , STR_EXPLOSIVE     }
    , { ITEM_INV_MEDIKIT_BIG    , STR_MEDIKIT_BIG   }
    , { ITEM_INV_MEDIKIT_SMALL  , STR_MEDIKIT_SMALL }
    , { ITEM_INV_PASSPORT       , STR_PASSPORT      }
    , { ITEM_INV_DETAIL         , STR_DETAIL        }
    , { ITEM_INV_SOUND          , STR_SOUND         }
    , { ITEM_INV_CONTROLS       , STR_CONTROLS      }
    , { ITEM_INV_GAMMA          , STR_GAMMA         }
    , { ITEM_INV_HOME           , STR_HOME          }
};


struct Inventory
{
    InvSlot useSlot;

    int32 numKeys;

    int32 counts[X_COUNT(INV_SLOTS)];

    Inventory()
    {
        memset(counts, 0, sizeof(counts));

        useSlot = SLOT_MAX;
        numKeys = 0;

        add(ITEM_INV_PASSPORT);
        add(ITEM_INV_DETAIL);
        add(ITEM_INV_SOUND);
        add(ITEM_INV_CONTROLS);
        //add(ITEM_INV_GAMMA);
        //add(ITEM_INV_HOME);

        add(ITEM_INV_COMPASS);
        add(ITEM_INV_PISTOLS);

        //add(ITEM_INV_KEY_ITEM_1);
        //add(ITEM_INV_PUZZLE_1);
    }

    ItemType remapToInv(ItemType type)
    {
        switch (type)
        {
            case ITEM_PISTOLS               : return ITEM_INV_PISTOLS;
            case ITEM_SHOTGUN               : return ITEM_INV_SHOTGUN;
            case ITEM_MAGNUMS               : return ITEM_INV_MAGNUMS;
            case ITEM_UZIS                  : return ITEM_INV_UZIS;
            case ITEM_AMMO_PISTOLS          : return ITEM_INV_AMMO_PISTOLS;
            case ITEM_AMMO_SHOTGUN          : return ITEM_INV_AMMO_SHOTGUN;
            case ITEM_AMMO_MAGNUMS          : return ITEM_INV_AMMO_MAGNUMS;
            case ITEM_AMMO_UZIS             : return ITEM_INV_AMMO_UZIS;
            case ITEM_MEDIKIT_SMALL         : return ITEM_INV_MEDIKIT_SMALL;
            case ITEM_MEDIKIT_BIG           : return ITEM_INV_MEDIKIT_BIG;
            case ITEM_PUZZLE_1              : return ITEM_INV_PUZZLE_1;
            case ITEM_PUZZLE_2              : return ITEM_INV_PUZZLE_2;
            case ITEM_PUZZLE_3              : return ITEM_INV_PUZZLE_3;
            case ITEM_PUZZLE_4              : return ITEM_INV_PUZZLE_4;
            case ITEM_LEADBAR               : return ITEM_INV_LEADBAR;
            case ITEM_KEY_ITEM_1            : return ITEM_INV_KEY_ITEM_1;
            case ITEM_KEY_ITEM_2            : return ITEM_INV_KEY_ITEM_2;
            case ITEM_KEY_ITEM_3            : return ITEM_INV_KEY_ITEM_3;
            case ITEM_KEY_ITEM_4            : return ITEM_INV_KEY_ITEM_4;
            case ITEM_SCION_PICKUP_QUALOPEC :
            case ITEM_SCION_PICKUP_DROP     :
            case ITEM_SCION_PICKUP_HOLDER   : return ITEM_INV_SCION;
        }
        return type;
    }

    InvSlot remapToSlot(ItemType type)
    {
        type = remapToInv(type);

        for (int32 i = 0; i < X_COUNT(INV_SLOTS); i++)
        {
            if (INV_SLOTS[i].type == type)
                return (InvSlot)i;
        }

        ASSERT(false);
        return SLOT_COMPASS;
    }

    InvSlot remapHoleToSlot(ItemType type)
    {
        switch (type)
        {
            case ITEM_PUZZLEHOLE_1 : return SLOT_PUZZLE_1;
            case ITEM_PUZZLEHOLE_2 : return SLOT_PUZZLE_2;
            case ITEM_PUZZLEHOLE_3 : return SLOT_PUZZLE_3;
            case ITEM_PUZZLEHOLE_4 : return SLOT_PUZZLE_4;
            case ITEM_KEYHOLE_1    : return SLOT_KEY_ITEM_1;
            case ITEM_KEYHOLE_2    : return SLOT_KEY_ITEM_2;
            case ITEM_KEYHOLE_3    : return SLOT_KEY_ITEM_3;
            case ITEM_KEYHOLE_4    : return SLOT_KEY_ITEM_4;
        }
        return SLOT_MAX;
    }

    void add(ItemType type, int32 count = 1)
    {
        InvSlot slot = remapToSlot(type);
        counts[slot] += count;
        // TODO check max

        if (slot < SLOT_COMPASS) {
            numKeys += count;
        }
    }

    void remove(InvSlot slot, int32 count)
    {
        counts[slot] -= count;

        if (slot < SLOT_COMPASS) {
            numKeys -= count;
        }
    }

    void show(ItemObj* lara, ItemObj* hole)
    {
        if (hole) {
            useSlot = remapHoleToSlot((ItemType)hole->type);
        }
    }

    bool applyItem(ItemObj* hole)
    {
        #define CHECK_CASE(A, B) case A: { if (useSlot != B) return false; break; }

        switch (hole->type)
        {
            CHECK_CASE(ITEM_PUZZLEHOLE_1, SLOT_PUZZLE_1);
            CHECK_CASE(ITEM_PUZZLEHOLE_2, SLOT_PUZZLE_2);
            CHECK_CASE(ITEM_PUZZLEHOLE_3, SLOT_PUZZLE_3);
            CHECK_CASE(ITEM_PUZZLEHOLE_4, SLOT_PUZZLE_4);
            CHECK_CASE(ITEM_KEYHOLE_1,    SLOT_KEY_ITEM_1);
            CHECK_CASE(ITEM_KEYHOLE_2,    SLOT_KEY_ITEM_2);
            CHECK_CASE(ITEM_KEYHOLE_3,    SLOT_KEY_ITEM_3);
            CHECK_CASE(ITEM_KEYHOLE_4,    SLOT_KEY_ITEM_4);
            default: return false;
        }

        remove(useSlot, 1);
        useSlot = SLOT_MAX;

        return true;
    }

    void draw()
    {
        //
    }
};

Inventory inventory;

#endif

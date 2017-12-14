#ifndef H_GAMEFLOW
#define H_GAMEFLOW

#include "utils.h"

namespace TR {

    bool useEasyStart;

    enum {
        NO_TRACK = 0xFF,
    };

    enum Version : uint32 {
        VER_UNKNOWN  = 0,

        VER_PC       = 256,
        VER_PSX      = 512,
        VER_SEGA     = 1024,

        VER_TR1      = 1,
        VER_TR2      = 2, 
        VER_TR3      = 4,
        VER_TR4      = 8,
        VER_TR5      = 16,

        VER_VERSION  = VER_TR1 | VER_TR2 | VER_TR3 | VER_TR4 | VER_TR5,
        VER_PLATFORM = VER_PC  | VER_PSX | VER_SEGA,

        VER_TR1_PC   = VER_TR1 | VER_PC,
        VER_TR1_PSX  = VER_TR1 | VER_PSX,
        VER_TR1_SEGA = VER_TR1 | VER_SEGA,

        VER_TR2_PC   = VER_TR2 | VER_PC,
        VER_TR2_PSX  = VER_TR2 | VER_PSX,

        VER_TR3_PC   = VER_TR3 | VER_PC,
        VER_TR3_PSX  = VER_TR3 | VER_PSX,
    };

    enum LevelID : uint32 {
        LVL_CUSTOM,
    // TR1
        LVL_TR1_TITLE,
        LVL_TR1_GYM,
        LVL_TR1_1,
        LVL_TR1_2,
        LVL_TR1_3A,
        LVL_TR1_3B,
        LVL_TR1_CUT_1,
        LVL_TR1_4,
        LVL_TR1_5,
        LVL_TR1_6,
        LVL_TR1_7A,
        LVL_TR1_7B,
        LVL_TR1_CUT_2,
        LVL_TR1_8A,
        LVL_TR1_8B,
        LVL_TR1_8C,
        LVL_TR1_10A,
        LVL_TR1_CUT_3,
        LVL_TR1_10B,
        LVL_TR1_CUT_4,
        LVL_TR1_10C,
        LVL_TR1_EGYPT,
        LVL_TR1_CAT,
        LVL_TR1_END,
        LVL_TR1_END2,
    // TR2
        LVL_TR2_TITLE,
        LVL_TR2_ASSAULT,
        LVL_TR2_WALL,
        LVL_TR2_CUT_1,
        LVL_TR2_BOAT,
        LVL_TR2_VENICE,
        LVL_TR2_OPERA,
        LVL_TR2_CUT_2,
        LVL_TR2_RIG,
        LVL_TR2_PLATFORM,
        LVL_TR2_CUT_3,
        LVL_TR2_UNWATER,
        LVL_TR2_KEEL,
        LVL_TR2_LIVING,
        LVL_TR2_DECK,
        LVL_TR2_SKIDOO,
        LVL_TR2_MONASTRY,
        LVL_TR2_CATACOMB,
        LVL_TR2_ICECAVE,
        LVL_TR2_EMPRTOMB,
        LVL_TR2_FLOATING,
        LVL_TR2_CUT_4,
        LVL_TR2_XIAN,
        LVL_TR2_HOUSE,
    // TR3
        LVL_TR3_TITLE,
        LVL_TR3_HOUSE,
        LVL_TR3_JUNGLE,
        LVL_TR3_CUT_6,
        LVL_TR3_TEMPLE,
        LVL_TR3_CUT_9,
        LVL_TR3_QUADCHAS,
        LVL_TR3_TONYBOSS,
        LVL_TR3_SHORE,
        LVL_TR3_CUT_1,
        LVL_TR3_CRASH,
        LVL_TR3_CUT_4,
        LVL_TR3_RAPIDS,
        LVL_TR3_TRIBOSS,
        LVL_TR3_ROOFS,
        LVL_TR3_CUT_2,
        LVL_TR3_SEWER,
        LVL_TR3_CUT_5,
        LVL_TR3_TOWER,
        LVL_TR3_CUT_11,
        LVL_TR3_OFFICE,
        LVL_TR3_NEVADA,
        LVL_TR3_CUT_7,
        LVL_TR3_COMPOUND,
        LVL_TR3_CUT_8,
        LVL_TR3_AREA51,
        LVL_TR3_ANTARC,
        LVL_TR3_CUT_3,
        LVL_TR3_MINES,
        LVL_TR3_CITY,
        LVL_TR3_CUT_12,
        LVL_TR3_CHAMBER,
        LVL_TR3_STPAUL,

        LVL_MAX,
    };

    enum {
    // TR1
        TRACK_TR1_TITLE     = 2,
        TRACK_TR1_CAVES     = 5,
        TRACK_TR1_SECRET    = 13,
        TRACK_TR1_CISTERN   = 57,
        TRACK_TR1_EGYPT     = 58,
        TRACK_TR1_MINE      = 59,
        TRACK_TR1_CUT_1     = 23,
        TRACK_TR1_CUT_2     = 25,
        TRACK_TR1_CUT_3     = 24,
        TRACK_TR1_CUT_4     = 22,
    // TR2
        TRACK_TR2_TITLE     = 64,
        TRACK_TR2_CHINA_1   = 33,
        TRACK_TR2_CHINA_2   = 59,
        TRACK_TR2_ITALY     = 31,
        TRACK_TR2_RIG       = 58,
        TRACK_TR2_UNWATER_1 = 34,
        TRACK_TR2_UNWATER_2 = 31,
        TRACK_TR2_TIBET_1   = 33,
        TRACK_TR2_TIBET_2   = 31,
        TRACK_TR2_CUT_1     = 3,
        TRACK_TR2_CUT_2     = 4,
        TRACK_TR2_CUT_3     = 5,
        TRACK_TR2_CUT_4     = 30,
    // TR3
        TRACK_TR3_TITLE     = 5,
        TRACK_TR3_HOUSE     = 2,
        TRACK_TR3_CAVES     = 30,
        TRACK_TR3_INDIA_1   = 34,
        TRACK_TR3_INDIA_2   = 30,
        TRACK_TR3_SOUTH_1   = 32,
        TRACK_TR3_SOUTH_2   = 33,
        TRACK_TR3_SOUTH_3   = 36,
        TRACK_TR3_LONDON_1  = 73,
        TRACK_TR3_LONDON_2  = 74,
        TRACK_TR3_LONDON_3  = 31,
        TRACK_TR3_LONDON_4  = 78,
        TRACK_TR3_NEVADA_1  = 33,
        TRACK_TR3_NEVADA_2  = 27,
        TRACK_TR3_ANTARC_1  = 28,
        TRACK_TR3_ANTARC_2  = 30,
        TRACK_TR3_ANTARC_3  = 26,
        TRACK_TR3_CUT_1     = 68,
        TRACK_TR3_CUT_2     = 67,
        TRACK_TR3_CUT_3     = 62,
        TRACK_TR3_CUT_4     = 65,
        TRACK_TR3_CUT_5     = 63,
        TRACK_TR3_CUT_6     = 64,
        TRACK_TR3_CUT_7     = 72,
        TRACK_TR3_CUT_8     = 70,
        TRACK_TR3_CUT_9     = 69,
        TRACK_TR3_CUT_11    = 71,
        TRACK_TR3_CUT_12    = 66,
    };

    struct {
        const char *name;
        const char *title;
        int        ambientTrack;
    } LEVEL_INFO[LVL_MAX] = {
    // TR1
        { ""          , "Custom Level",             TRACK_TR1_CAVES     },
        { "TITLE"     , "",                         TRACK_TR1_TITLE     },
        { "GYM"       , "Lara's Home",              NO_TRACK            },
        { "LEVEL1"    , "Caves",                    TRACK_TR1_CAVES     },
        { "LEVEL2"    , "City of Vilcabamba",       TRACK_TR1_CAVES     },
        { "LEVEL3A"   , "Lost Valley",              TRACK_TR1_CAVES     },
        { "LEVEL3B"   , "Tomb of Qualopec",         TRACK_TR1_CAVES     },
        { "CUT1"      , "",                         TRACK_TR1_CUT_1     },
        { "LEVEL4"    , "St. Francis' Folly",       TRACK_TR1_CAVES     },
        { "LEVEL5"    , "Colosseum",                TRACK_TR1_CAVES     },
        { "LEVEL6"    , "Palace Midas",             TRACK_TR1_CAVES     },
        { "LEVEL7A"   , "The Cistern",              TRACK_TR1_CISTERN   },
        { "LEVEL7B"   , "Tomb of Tihocan",          TRACK_TR1_CISTERN   },
        { "CUT2"      , "",                         TRACK_TR1_CUT_2     },
        { "LEVEL8A"   , "City of Khamoon",          TRACK_TR1_EGYPT     },
        { "LEVEL8B"   , "Obelisk of Khamoon",       TRACK_TR1_EGYPT     },
        { "LEVEL8C"   , "Sanctuary of the Scion",   TRACK_TR1_EGYPT     },
        { "LEVEL10A"  , "Natla's Mines",            TRACK_TR1_MINE      },
        { "CUT3"      , "",                         TRACK_TR1_CUT_3     },
        { "LEVEL10B"  , "Atlantis",                 TRACK_TR1_MINE      },
        { "CUT4"      , "",                         TRACK_TR1_CUT_4     },
        { "LEVEL10C"  , "The Great Pyramid",        TRACK_TR1_MINE      },
        { "EGYPT"     , "Return to Egypt",          TRACK_TR1_EGYPT     },
        { "CAT"       , "Temple of the Cat",        TRACK_TR1_EGYPT     },
        { "END"       , "Atlantean Stronghold",     TRACK_TR1_EGYPT     },
        { "END2"      , "The Hive",                 TRACK_TR1_EGYPT     },
    // TR2
        { "TITLE"     , "",                         TRACK_TR2_TITLE     },
        { "ASSAULT"   , "Lara's Home",              NO_TRACK            },
        { "WALL"      , "The Great Wall",           TRACK_TR2_CHINA_1   },
        { "CUT1"      , "",                         TRACK_TR2_CUT_1     },
        { "BOAT"      , "Venice",                   NO_TRACK            },
        { "VENICE"    , "Bartoli's Hideout",        NO_TRACK            },
        { "OPERA"     , "Opera House",              TRACK_TR2_ITALY     },
        { "CUT2"      , "",                         TRACK_TR2_CUT_2     },
        { "RIG"       , "Offshore Rig",             TRACK_TR2_RIG       },
        { "PLATFORM"  , "Diving Area",              TRACK_TR2_RIG       },
        { "CUT3"      , "",                         TRACK_TR2_CUT_3     },
        { "UNWATER"   , "40 Fathoms",               TRACK_TR2_UNWATER_1 },
        { "KEEL"      , "Wreck of the Maria Doria", TRACK_TR2_UNWATER_2 },
        { "LIVING"    , "Living Quarters",          TRACK_TR2_UNWATER_1 },
        { "DECK"      , "The Deck",                 TRACK_TR2_UNWATER_2 },
        { "SKIDOO"    , "Tibetan Foothills",        TRACK_TR2_TIBET_1   },
        { "MONASTRY"  , "Barkhang Monastery",       NO_TRACK            },
        { "CATACOMB"  , "Catacombs of the Talion",  TRACK_TR2_TIBET_2   },
        { "ICECAVE"   , "Ice Palace",               TRACK_TR2_TIBET_2   },
        { "EMPRTOMB"  , "Temple of Xian",           TRACK_TR2_CHINA_2   },
        { "FLOATING"  , "Floating Islands",         TRACK_TR2_CHINA_2   },
        { "CUT4"      , "",                         TRACK_TR2_CUT_4     },
        { "XIAN"      , "The Dragon's Lair",        TRACK_TR2_CHINA_2   },
        { "HOUSE"     , "Home Sweet Home",          NO_TRACK            },
    // TR3
        { "TITLE",      "",                         TRACK_TR3_TITLE     },
        { "HOUSE",      "Lara's House",             NO_TRACK            },
        { "JUNGLE",     "Jungle",                   TRACK_TR3_INDIA_1   },
        { "CUT6",       "",                         TRACK_TR3_CUT_6     },
        { "TEMPLE",     "Temple Ruins",             TRACK_TR3_INDIA_1   },
        { "CUT9",       "",                         TRACK_TR3_CUT_9     },
        { "QUADCHAS",   "The River Ganges",         TRACK_TR3_INDIA_1   },
        { "TONYBOSS",   "Caves Of Kaliya",          TRACK_TR3_INDIA_2   },
        { "SHORE",      "Coastal Village",          TRACK_TR3_SOUTH_1   },
        { "CUT1",       "",                         TRACK_TR3_CUT_1     },
        { "CRASH",      "Crash Site",               TRACK_TR3_SOUTH_2   },
        { "CUT4",       "",                         TRACK_TR3_CUT_4     },
        { "RAPIDS",     "Madubu Gorge",             TRACK_TR3_SOUTH_3   },
        { "TRIBOSS",    "Temple Of Puna",           TRACK_TR3_CAVES     },
        { "ROOFS",      "Thames Wharf",             TRACK_TR3_LONDON_1  },
        { "CUT2",       "",                         TRACK_TR3_CUT_2     },
        { "SEWER",      "Aldwych",                  TRACK_TR3_LONDON_2  },
        { "CUT5",       "",                         TRACK_TR3_CUT_5     },
        { "TOWER",      "Lud's Gate",               TRACK_TR3_LONDON_3  },
        { "CUT11",      "",                         TRACK_TR3_CUT_11    },
        { "OFFICE",     "City",                     TRACK_TR3_LONDON_4  },
        { "NEVADA",     "Nevada Desert",            TRACK_TR3_NEVADA_1  },
        { "CUT7",       "",                         TRACK_TR3_CUT_7     },
        { "COMPOUND",   "High Security Compound",   TRACK_TR3_NEVADA_2  },
        { "CUT8",       "",                         TRACK_TR3_CUT_8     },
        { "AREA51",     "Area 51",                  TRACK_TR3_NEVADA_2  },
        { "ANTARC",     "Antarctica",               TRACK_TR3_ANTARC_1  },
        { "CUT3",       "",                         TRACK_TR3_CUT_3     },
        { "MINES",      "RX-Tech Mines",            TRACK_TR3_ANTARC_2  },
        { "CITY",       "Lost City Of Tinnos",      TRACK_TR3_ANTARC_3  },
        { "CUT12",      "",                         TRACK_TR3_CUT_12    },
        { "CHAMBER",    "Meteorite Cavern",         TRACK_TR3_ANTARC_3  },
        { "STPAUL",     "All Hallows",              TRACK_TR3_CAVES     },
    };

    LevelID getLevelID(int size, Version &version, bool &isDemoLevel) {
        isDemoLevel = false;
        switch (size) {
        // TR1
            case 508614  : version = VER_TR1_PSX;
            case 334874  :
            case 316138  :
            case 316460  : return LVL_TR1_TITLE;
            case 1074234 : version = VER_TR1_PSX;
            case 3236806 :
            case 3237128 : return LVL_TR1_GYM;
            case 1448896 : version = VER_TR1_PSX;
            case 2533312 :
            case 2533634 : return LVL_TR1_1;
            case 2873406 : isDemoLevel = true; return LVL_TR1_2;
            case 1535734 : version = VER_TR1_PSX;
            case 2873128 :
            case 2873450 : return LVL_TR1_2;
            case 1630560 : version = VER_TR1_PSX;
            case 2934408 :
            case 2934730 : return LVL_TR1_3A;
            case 1506614 : version = VER_TR1_PSX;
            case 2737936 :
            case 2738258 : return LVL_TR1_3B;
            case 722402  : version = VER_TR1_PSX;
            case 599840  : return LVL_TR1_CUT_1;
            case 1621970 : version = VER_TR1_PSX;
            case 3030550 :
            case 3030872 : return LVL_TR1_4;
            case 1585942 : version = VER_TR1_PSX;
            case 2718218 :
            case 2718540 : return LVL_TR1_5;
            case 1708464 : version = VER_TR1_PSX;
            case 3139590 :
            case 3074376 : return LVL_TR1_6;
            case 1696664 : version = VER_TR1_PSX;
            case 2817290 :
            case 2817612 : return LVL_TR1_7A;
            case 1733274 : version = VER_TR1_PSX;
            case 3388774 :
            case 3389096 : return LVL_TR1_7B;
            case 542960  : version = VER_TR1_PSX;
            case 354320  : return LVL_TR1_CUT_2;
            case 1563356 : version = VER_TR1_PSX;
            case 2880242 :
            case 2880564 : return LVL_TR1_8A;
            case 1565630 : version = VER_TR1_PSX;
            case 2886434 :
            case 2886756 : return LVL_TR1_8B;
            case 1619360 : version = VER_TR1_PSX;
            case 3105128 :
            case 3105450 : return LVL_TR1_8C;
            case 1678018 : version = VER_TR1_PSX;
            case 3223816 :
            case 3224138 : return LVL_TR1_10A;
            case 636660  : version = VER_TR1_PSX;
            case 512104  : return LVL_TR1_CUT_3;
            case 1686748 : version = VER_TR1_PSX;
            case 3094020 : return LVL_TR1_10B;
            case 940398  : version = VER_TR1_PSX;
            case 879582  : return LVL_TR1_CUT_4;
            case 1814278 : version = VER_TR1_PSX;
            case 3531702 :
            case 3532024 : return LVL_TR1_10C;
            case 3278614 : version = VER_TR1_PSX;
            case 3279242 : return LVL_TR1_EGYPT;
            case 3270370 : version = VER_TR1_PSX;
            case 3270998 : return LVL_TR1_CAT;
            case 3208018 : return LVL_TR1_END;
            case 3153300 : return LVL_TR1_END2;
        // TR2
            case 148744  : version = VER_TR2_PSX;
            case 611892  : return LVL_TR2_TITLE;
            case 1565974 : version = VER_TR2_PSX;
            case 3471450 : return LVL_TR2_ASSAULT;
            case 1661622 : version = VER_TR2_PSX;
            case 2986356 : return LVL_TR2_WALL;
            case 2014880 : version = VER_TR2_PSX;
            case 2010678 : version = VER_TR2_PSX; // PLAYABLE.PSX
            case 3945738 : return LVL_TR2_BOAT;
            case 2081402 : version = VER_TR2_PSX;
            case 4213070 : return LVL_TR2_VENICE;
            case 2060882 : version = VER_TR2_PSX;
            case 3996500 : return LVL_TR2_OPERA;
            case 1950216 : version = VER_TR2_PSX;
            case 3944472 : return LVL_TR2_RIG;
            case 1993070 : version = VER_TR2_PSX;
            case 4101590 : return LVL_TR2_PLATFORM;
            case 1784200 : version = VER_TR2_PSX;
            case 3131814 : return LVL_TR2_UNWATER;
            case 2014110 : version = VER_TR2_PSX;
            case 3995356 : return LVL_TR2_KEEL;
            case 1808444 : version = VER_TR2_PSX;
            case 3501472 : return LVL_TR2_LIVING;
            case 1968084 : version = VER_TR2_PSX;
            case 3890042 : return LVL_TR2_DECK;
            case 1981902 : version = VER_TR2_PSX;
            case 3521106 : return LVL_TR2_SKIDOO;
            case 2120772 : version = VER_TR2_PSX;
            case 4325562 : return LVL_TR2_MONASTRY;
            case 1877970 : version = VER_TR2_PSX;
            case 3605052 : return LVL_TR2_CATACOMB;
            case 1987854 : version = VER_TR2_PSX;
            case 3803398 : return LVL_TR2_ICECAVE;
            case 2101594 : version = VER_TR2_PSX;
            case 4291468 : return LVL_TR2_EMPRTOMB;
            case 2078690 : version = VER_TR2_PSX;
            case 4007486 : return LVL_TR2_FLOATING;
            case 1748838 : version = VER_TR2_PSX;
            case 3173840 : return LVL_TR2_XIAN;
            case 1886728 : version = VER_TR2_PSX;
            case 3693108 : return LVL_TR2_HOUSE;
            case 952798  : version = VER_TR2_PSX;
            case 1767052 : return LVL_TR2_CUT_1;
            case 1098876 : version = VER_TR2_PSX;
            case 2037544 : return LVL_TR2_CUT_2;
            case 1288922 : version = VER_TR2_PSX;
            case 2558286 : return LVL_TR2_CUT_3;
            case 1116966 : version = VER_TR2_PSX;
            case 1900230 : return LVL_TR2_CUT_4;
        // TR3 (TODO PSX)
            case 1275266 : return LVL_TR3_TITLE;
            case 3437855 : return LVL_TR3_HOUSE;
            case 3360001 : return LVL_TR3_JUNGLE;
            case 3180412 : return LVL_TR3_TEMPLE;
            case 3131453 : return LVL_TR3_QUADCHAS;
            case 2618437 : return LVL_TR3_TONYBOSS;
            case 3365562 : return LVL_TR3_SHORE;
            case 3331132 : return LVL_TR3_CRASH;
            case 3148248 : return LVL_TR3_RAPIDS;
            case 2276838 : return LVL_TR3_TRIBOSS;
            case 3579476 : return LVL_TR3_ROOFS;
            case 3670149 : return LVL_TR3_SEWER;
            case 3523586 : return LVL_TR3_TOWER;
            case 2504910 : return LVL_TR3_OFFICE;
            case 3140417 : return LVL_TR3_NEVADA;
            case 3554880 : return LVL_TR3_COMPOUND;
            case 3530130 : return LVL_TR3_AREA51;
            case 3515272 : return LVL_TR3_ANTARC;
            case 3331087 : return LVL_TR3_MINES;
            case 3575873 : return LVL_TR3_CITY;
            case 3190736 : return LVL_TR3_CHAMBER;
            case 3140028 : return LVL_TR3_STPAUL;
            case 1547866 : return LVL_TR3_CUT_1;
            case 1957327 : return LVL_TR3_CUT_2;
            case 2020225 : return LVL_TR3_CUT_3;
            case 1048466 : return LVL_TR3_CUT_4;
            case 1580868 : return LVL_TR3_CUT_5;
            case 2430940 : return LVL_TR3_CUT_6;
            case 1253227 : return LVL_TR3_CUT_7;
            case  944962 : return LVL_TR3_CUT_8;
            case 1788075 : return LVL_TR3_CUT_9;
            case 1402003 : return LVL_TR3_CUT_11;
            case 2321393 : return LVL_TR3_CUT_12;
        }
        return LVL_CUSTOM;
    }

    LevelID getTitleId(Version version) {
        switch (version & VER_VERSION) {
            case VER_TR1 : return LVL_TR1_TITLE;
            case VER_TR2 : return LVL_TR2_TITLE;
            case VER_TR3 : return LVL_TR3_TITLE;
        }
        return LVL_TR1_TITLE;
        ASSERT(false);
    }

    LevelID getHomeId(Version version) {
        switch (version & VER_VERSION) {
            case VER_TR1 : return LVL_TR1_GYM;
            case VER_TR2 : return LVL_TR2_ASSAULT;
            case VER_TR3 : return LVL_TR3_HOUSE;
        }
        return LVL_TR1_TITLE;
        ASSERT(false);
    }

    LevelID getStartId(Version version) {
        switch (version & VER_VERSION) {
            case VER_TR1 : return LVL_TR1_1;
            case VER_TR2 : return LVL_TR2_WALL;
            case VER_TR3 : return LVL_TR3_JUNGLE;
        }
        return LVL_MAX;
        ASSERT(false);
    }

    LevelID getEndId(Version version) {
        switch (version & VER_VERSION) {
            case VER_TR1 : return LVL_TR1_10C;
            case VER_TR2 : return LVL_TR2_HOUSE;
            case VER_TR3 : return LVL_TR3_CHAMBER;
        }
        return LVL_MAX;
        ASSERT(false);
    }

    bool isCutsceneLevel(LevelID id) {
        return id == LVL_TR1_CUT_1 || id == LVL_TR1_CUT_2 || id == LVL_TR1_CUT_3 || id == LVL_TR1_CUT_4 ||
               id == LVL_TR2_CUT_1 || id == LVL_TR2_CUT_2 || id == LVL_TR2_CUT_3 || id == LVL_TR2_CUT_4 || 
               id == LVL_TR3_CUT_1 || id == LVL_TR3_CUT_2  || id == LVL_TR3_CUT_3  || id == LVL_TR3_CUT_4 ||
               id == LVL_TR3_CUT_5 || id == LVL_TR3_CUT_6  || id == LVL_TR3_CUT_7  || id == LVL_TR3_CUT_8 ||
               id == LVL_TR3_CUT_9 || id == LVL_TR3_CUT_11 || id == LVL_TR3_CUT_12;
    }

    Version getGameVersion() {
        useEasyStart = true;
        if (Stream::existsContent("DATA/GYM.PHD") || Stream::existsContent("GYM.PHD"))
            return VER_TR1_PC;
        if (Stream::existsContent("PSXDATA/GYM.PSX"))
            return VER_TR1_PSX;
        if (Stream::existsContent("DATA/GYM.SAT"))
            return VER_TR1_SEGA;

        if (Stream::existsContent("data/ASSAULT.TR2") || Stream::existsContent("assault.TR2"))
            return VER_TR2_PC;
        if (Stream::existsContent("DATA/ASSAULT.PSX"))
            return VER_TR2_PSX;

        if (Stream::existsContent("data/JUNGLE.TR2"))
            return VER_TR3_PC;

        useEasyStart = false;
        return VER_UNKNOWN;
    }

    void getGameLevelFile(char *dst, Version version, LevelID id) {
        if (useEasyStart) {
            switch (version) {
                case VER_TR1_PC   :
                    sprintf(dst, "DATA/%s.PHD", LEVEL_INFO[id].name);
                    if (Stream::existsContent(dst)) break;
                    sprintf(dst, "%s.PHD", LEVEL_INFO[id].name);
                    break;
                case VER_TR1_PSX  : sprintf(dst, "PSXDATA/%s.PSX", LEVEL_INFO[id].name); break;
                case VER_TR1_SEGA : sprintf(dst, "DATA/%s.SAT",    LEVEL_INFO[id].name); break;
                case VER_TR2_PC   : { // oh FFFFUUUUUUCKing CaTaComB.Tr2!
                    if (id == LVL_TR2_VENICE || id == LVL_TR2_CUT_2 || id == LVL_TR2_PLATFORM || id == LVL_TR2_CUT_3 || id == LVL_TR2_UNWATER || 
                        id == LVL_TR2_KEEL || id == LVL_TR2_LIVING || id == LVL_TR2_DECK || id == LVL_TR2_CATACOMB || id == LVL_TR2_ICECAVE ||
                        id == LVL_TR2_CUT_4 || id == LVL_TR2_XIAN || id == LVL_TR2_HOUSE) {
                        strcpy(dst, LEVEL_INFO[id].name);
                        String::toLower(dst);
                        sprintf(dst, "DATA/%s.TR2", dst);
                    } else if (id == LVL_TR2_TITLE) {
                        sprintf(dst, "DATA/%s.tr2", LEVEL_INFO[id].name);
                    } else if (id == LVL_TR2_EMPRTOMB) {
                        strcpy(dst, "DATA/Emprtomb.tr2");
                    } else {
                        sprintf(dst, "DATA/%s.TR2", LEVEL_INFO[id].name);
                    }
                    if (Stream::existsContent(dst)) break;
                    strcpy(dst, LEVEL_INFO[id].name);
                    String::toLower(dst);
                    strcat(dst, ".TR2");
                    break;
                }
                case VER_TR2_PSX : sprintf(dst, "DATA/%s.PSX", LEVEL_INFO[id].name); break;
                case VER_TR3_PC  : sprintf(dst, isCutsceneLevel(id) ? "cuts/%s.TR2" : "data/%s.TR2", LEVEL_INFO[id].name); break;
                default          : ASSERT(false);
            }
        } else {
            strcpy(dst, "level/");
            if (version & VER_TR1) strcat(dst, "1/");
            if (version & VER_TR2) strcat(dst, "2/");
            if (version & VER_TR3) strcat(dst, "3/");
            strcat(dst, LEVEL_INFO[id].name);

            #ifdef __EMSCRIPTEN__
                 strcat(dst, ".PSX");
            #else
                switch (version) {
                    case VER_TR1_PC  : strcat(dst, ".PHD"); break;
                    case VER_TR2_PC  : 
                    case VER_TR3_PC  : strcat(dst, ".TR2"); break;
                    case VER_TR1_PSX :
                    case VER_TR2_PSX : 
                    case VER_TR3_PSX : strcat(dst, ".PSX"); break;
                    default : ASSERT(false);
                }
            #endif
        }
    }

    const char* getGameSoundsFile(Version version) {
        if (!useEasyStart) {
            if (version == VER_TR2_PC)
                return "audio/2/MAIN.SFX";
            if (version == VER_TR3_PC)
                return "audio/3/MAIN.SFX";
        } else
            if (version == VER_TR2_PC || version == VER_TR3_PC)
                return "data/MAIN.SFX";
        ASSERT(false);
        return NULL;
    }

    uint8 remapTrack(Version version, uint8 track) {
        static const uint8 TR2_TRACK_MAPPING[] = { 
            2, 2, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 18, 18, 19, 20, 
            21, 22, 23, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61
        };

        if (version & VER_TR2) {
            ASSERT(track < COUNT(TR2_TRACK_MAPPING));
            return TR2_TRACK_MAPPING[track];
        }
        return track;
    }

    bool checkTrack(const char *pre, char *name) {
        static const char *fmt[] = { ".ogg", ".mp3", ".wav" };
        static const char *lng[] = { "", "_EN", "_DE", "_FR", "_IT", "_JA", "_RU" };

        char buf[32];
        for (int f = 0; f < COUNT(fmt); f++)
            for (int l = 0; l < COUNT(lng); l++) {
                strcpy(buf, pre);
                strcat(buf, name);
                strcat(buf, lng[l]);
                strcat(buf, fmt[f]);
                if (Stream::existsContent(buf)) {
                    strcpy(name, buf);
                    return true;
                }
            }

        return false;
    }

    void getGameTrack(Version version, int track, Stream::Callback callback, void *userData) {
        track = remapTrack(version, track);

        char title[32];
        if (useEasyStart) {
            switch (version) {
                case VER_TR1_PC  :
                case VER_TR1_PSX :
                    sprintf(title, "track_%02d", track);
                    if (!checkTrack("", title) && !checkTrack("audio/1/", title) && !checkTrack("audio/", title)) {
                        callback(NULL, userData);
                        return;
                    }
                    break;
                case VER_TR2_PC  :
                case VER_TR2_PSX :
                    if (Stream::existsContent("audio/cdaudio.mp3")) {
                        callback(Sound::openCDAudioMP3("audio/cdaudio.dat", "audio/cdaudio.mp3", track), userData);
                        return;
                    }
                    sprintf(title, "track_%02d", track);
                    if (!checkTrack("", title) && !checkTrack("audio/2/", title) && !checkTrack("audio/", title)) {
                        callback(NULL, userData);
                        return;
                    }
                    break;
                case VER_TR3_PC  :
                case VER_TR3_PSX :
                    callback(Sound::openCDAudioWAD("audio/cdaudio.wad", track), userData);
                    return;
                default : return;
            }
        } else {
            switch (version) {
                case VER_TR1_PC  :
                case VER_TR1_PSX :
                    sprintf(title, "audio/1/track_%02d.ogg", track);
                    break;
                case VER_TR2_PC  :
                case VER_TR2_PSX :
                    sprintf(title, "audio/2/track_%02d.ogg", track);
                    break;
                case VER_TR3_PC  :
                case VER_TR3_PSX :
                    #ifndef __EMSCRIPTEN__
                        callback(Sound::openCDAudioWAD("audio/3/cdaudio.wad", track), userData);
                        return;
                    #else
                        sprintf(title, "audio/3/track_%02d.wav", track);
                    #endif
                    break;
                default : return;
            }
        }

        new Stream(title, callback, userData);
    }

    const char* getGameScreen(Version version, LevelID id) {
        if (useEasyStart) {

            #define CHECK_FILE(name) if (Stream::existsContent(name)) return name

            switch (id) {
                case LVL_TR1_TITLE :
                    CHECK_FILE("TITLEH.png");
                    CHECK_FILE("DATA/TITLEH.PCX");
                    CHECK_FILE("DELDATA/AMERTIT.RAW");
                    break;

                case LVL_TR2_TITLE :
                    CHECK_FILE("TITLE.png");
                    CHECK_FILE("data/TITLE.PCX");
                    CHECK_FILE("pix/title.pcx");
                    CHECK_FILE("PIXUS/TITLEUS.RAW"); // TODO: add other languages
                    break;

                case LVL_TR3_TITLE :
                    CHECK_FILE("pix/TITLEUK.BMP"); // TODO: add other languages
                    break;

                default : ;
            }

           #undef CHECK_FILE

        } else {
            switch (id) {
                case LVL_TR1_TITLE : return "level/1/TITLEH.PCX";

                case LVL_TR2_TITLE : return "level/2/TITLE.PCX";

                case LVL_TR3_TITLE : return "level/3/TITLEUK.BMP";

                default            : ;
            }
        }

        return NULL;
    }
}

#endif
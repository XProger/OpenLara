#ifndef H_GAMEFLOW
#define H_GAMEFLOW

#include "utils.h"

#define CHECK_FILE(name) if (Stream::existsContent(name)) return name

namespace TR {

    bool useEasyStart;

    enum {
        NO_TRACK = 0xFF,
    };

    enum Version {
        VER_UNKNOWN  = 0,

        VER_PC       = 256,
        VER_PSX      = 512,
        VER_SAT      = 1024,

        VER_TR1      = 1,
        VER_TR2      = 2, 
        VER_TR3      = 4,
        VER_TR4      = 8,
        VER_TR5      = 16,

        VER_VERSION  = VER_TR1 | VER_TR2 | VER_TR3 | VER_TR4 | VER_TR5,
        VER_PLATFORM = VER_PC  | VER_PSX | VER_SAT,

        VER_TR1_PC   = VER_TR1 | VER_PC,
        VER_TR1_PSX  = VER_TR1 | VER_PSX,
        VER_TR1_SAT  = VER_TR1 | VER_SAT,

        VER_TR2_PC   = VER_TR2 | VER_PC,
        VER_TR2_PSX  = VER_TR2 | VER_PSX,

        VER_TR3_PC   = VER_TR3 | VER_PC,
        VER_TR3_PSX  = VER_TR3 | VER_PSX,

        VER_MAX      = 0xFFFFFFFF,
    };

    enum LevelID {
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
        LVL_TR2_CUT_4,
        LVL_TR2_FLOATING,
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

    struct LevelInfo {
        const char *name;
        const char *title;
        int        track;
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
        { "CUT4"      , "",                         TRACK_TR2_CUT_4     },
        { "FLOATING"  , "Floating Islands",         TRACK_TR2_CHINA_2   },
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

    LevelID getLevelID(int size, const char *name, Version &version, bool &isDemoLevel) {
        isDemoLevel = false;
        switch (size) {
        // TR1
            // TITLE
            case 585648  : // PSX JAP
            case 508614  : version = VER_TR1_PSX;
            //case 320412  : // PC JAP
            case 5148    : // SAT
            case 334874  :
            case 316138  :
            case 316460  : return LVL_TR1_TITLE;
            // GYM
            case 1234800 : // PSX JAP
            case 1074234 : version = VER_TR1_PSX;
            case 343854  : // SAT
            case 3377974 : // PC JAP
            case 3236806 :
            case 3237128 : return LVL_TR1_GYM;
            // LEVEL1
            case 1667568 : // PSX JAP
            case 1448896 : version = VER_TR1_PSX;
            case 497656  :
            case 2540906 : // PC JAP
            case 2533312 :
            case 2533634 : return LVL_TR1_1;
            // LEVEL2
            case 2873406 : isDemoLevel = true; return LVL_TR1_2;
            case 1766352 : // PSX JAP
            case 1535734 : version = VER_TR1_PSX;
            case 2880722 : // PC JAP
            case 2873128 :
            case 2873450 : return LVL_TR1_2;
            // LEVEL3A
            case 1876896 : // PSX JAP
            case 1630560 : version = VER_TR1_PSX;
            case 2942002 : // PC JAP
            case 2934408 :
            case 2934730 : return LVL_TR1_3A;
            // LEVEL3B
            case 1510414 : // PSX JAP
            case 1506614 : version = VER_TR1_PSX;
            case 2745530 : // PC JAP
            case 2737936 :
            case 2738258 : return LVL_TR1_3B;
            // CUT1
            case 722402  : version = VER_TR1_PSX;
            case 599840  : return LVL_TR1_CUT_1;
            // LEVEL4
            case 1624130 : // PSX JAP
            case 1621970 : version = VER_TR1_PSX;
            case 3038144 : // PC JAP
            case 3030550 :
            case 3030872 : return LVL_TR1_4;
            // LEVEL5
            case 1588102 : // PSX JAP
            case 1585942 : version = VER_TR1_PSX;
            case 2725812 : // PC JAP
            case 2718218 :
            case 2718540 : return LVL_TR1_5;
            // LEVEL6
            case 1710624 : // PSX JAP
            case 1708464 : version = VER_TR1_PSX;
            case 3147184 : // PC JAP
            case 3139590 :
            case 3074376 : return LVL_TR1_6;
            // LEVEL7A
            case 1698824 : // PSX JAP
            case 1696664 : version = VER_TR1_PSX;
            case 2824884 : // PC JAP
            case 2817290 :
            case 2817612 : return LVL_TR1_7A;
            // LEVEL7B
            case 1735434 : // PSX JAP
            case 1733274 : version = VER_TR1_PSX;
            case 3603912 : // PC JAP
            case 3388774 :
            case 3389096 : return LVL_TR1_7B;
            // CUT2
            case 542960  : version = VER_TR1_PSX;
            case 354320  : return LVL_TR1_CUT_2;
            // LEVEL8A
            case 1565494 : // PSX JAP
            case 1563356 : version = VER_TR1_PSX;
            case 2887836 : // PC JAP
            case 2880242 :
            case 2880564 : return LVL_TR1_8A;
            // LEVEL8B
            case 1567790 : // PSX JAP
            case 1565630 : version = VER_TR1_PSX;
            case 2894028 : // PC JAP
            case 2886434 :
            case 2886756 : return LVL_TR1_8B;
            // LEVEL8C
            case 1621520 : // PSX JAP
            case 1619360 : version = VER_TR1_PSX;
            case 3072066 : // PC JAP
            case 3105128 :
            case 3105450 : return LVL_TR1_8C;
            // LEVEL10A
            case 1680146 : // PSX JAP
            case 1678018 : version = VER_TR1_PSX;
            case 3270372 : // PC JAP
            case 3223816 :
            case 3224138 : return LVL_TR1_10A;
            // CUT3
            case 636660  : version = VER_TR1_PSX;
            case 512104  : return LVL_TR1_CUT_3;
            // LEVEL10B
            case 1688908 : // PSX JAP
            case 1686748 : version = VER_TR1_PSX;
            case 3101614 : // PC JAP
            case 3094342 :
            case 3094020 : return LVL_TR1_10B;
            // CUT4
            case 940398  : version = VER_TR1_PSX;
            case 879582  : return LVL_TR1_CUT_4;
            // LEVEL10C
            case 1816438 : // PSX JAP
            case 1814278 : version = VER_TR1_PSX;
            case 3533814 : // PC JAP
            case 3531702 :
            case 3532024 : return LVL_TR1_10C;
            // EGYPT
            case 3278614 : version = VER_TR1_PSX;
            case 3279242 : return LVL_TR1_EGYPT;
            // CAT
            case 3270370 : version = VER_TR1_PSX;
            case 3270998 : return LVL_TR1_CAT;
            // END
            case 3208018 : return LVL_TR1_END;
            // END2
            case 3153300 : return LVL_TR1_END2;
        // TR2
            // TITLE
            case 148698  : // PSX JAP
            case 148744  : version = VER_TR2_PSX;
            case 611892  : return LVL_TR2_TITLE;
            // ASSAULT
            case 1568632 : // PSX JAP
            case 1565974 : version = VER_TR2_PSX;
            case 3471450 : return LVL_TR2_ASSAULT;
            // WALL
            case 1664198 : // PSX JAP
            case 1661622 : version = VER_TR2_PSX;
            case 2986356 : return LVL_TR2_WALL;
            // BOAT
            case 2017538 : // PSX JAP
            case 2014880 :
            case 2010678 : version = VER_TR2_PSX; // PLAYABLE.PSX
            case 3945738 : return LVL_TR2_BOAT;
            // VENICE
            case 2084060 : // PSX JAP
            case 2081402 : version = VER_TR2_PSX;
            case 4213070 : return LVL_TR2_VENICE;
            // OPERA
            case 2063540 : // PSX JAP
            case 2060882 : version = VER_TR2_PSX;
            case 3996500 : return LVL_TR2_OPERA;
            // RIG
            case 1952874 : // PSX JAP
            case 1950216 : version = VER_TR2_PSX;
            case 3944472 : return LVL_TR2_RIG;
            // PLATFORM
            case 1995728 : // PSX JAP
            case 1993070 : version = VER_TR2_PSX;
            case 4101590 : return LVL_TR2_PLATFORM;
            // UNWATER
            case 1786858 : // PSX JAP
            case 1784200 : version = VER_TR2_PSX;
            case 3131814 : return LVL_TR2_UNWATER;
            // KEEL
            case 2016768 : // PSX JAP
            case 2014110 : version = VER_TR2_PSX;
            case 3995356 : return LVL_TR2_KEEL;
            // LIVING
            case 1811102 : // PSX JAP
            case 1808444 : version = VER_TR2_PSX;
            case 3501472 : return LVL_TR2_LIVING;
            // DECK
            case 1970742 : // PSX JAP
            case 1968084 : version = VER_TR2_PSX;
            case 3890042 : return LVL_TR2_DECK;
            // SKIDOO
            case 1984560 : // PSX JAP
            case 1981902 : version = VER_TR2_PSX;
            case 3521106 : return LVL_TR2_SKIDOO;
            // MONASTRY
            case 2123446 : // PSX JAP
            case 2120772 : version = VER_TR2_PSX;
            case 4325562 : return LVL_TR2_MONASTRY;
            // CATACOMB
            case 1880628 : // PSX JAP
            case 1877970 : version = VER_TR2_PSX;
            case 3605052 : return LVL_TR2_CATACOMB;
            // ICECAVE
            case 1990512 : // PSX JAP
            case 1987854 : version = VER_TR2_PSX;
            case 3803398 : return LVL_TR2_ICECAVE;
            // EMPRTOMB
            case 2104252 : // PSX JAP
            case 2101594 : version = VER_TR2_PSX;
            case 4291468 : return LVL_TR2_EMPRTOMB;
            // FLOATING
            case 2081348 : // PSX JAP
            case 2078690 : version = VER_TR2_PSX;
            case 4007486 : return LVL_TR2_FLOATING;
            // XIAN
            case 1751496 : // PSX JAP
            case 1748838 : version = VER_TR2_PSX;
            case 3173840 : return LVL_TR2_XIAN;
            // HOUSE
            case 1889386 : // PSX JAP
            case 1886728 : version = VER_TR2_PSX;
            case 3693108 : return LVL_TR2_HOUSE;
            // CUT1
            case 952800  : // PSX JAP
            case 952798  : version = VER_TR2_PSX;
            case 1767052 : return LVL_TR2_CUT_1;
            // CUT2
            case 1098878 : // PSX JAP
            case 1098876 : version = VER_TR2_PSX;
            case 2037544 : return LVL_TR2_CUT_2;
            // CUT3
            case 1288924 : // PSX JAP
            case 1288922 : version = VER_TR2_PSX;
            case 2558286 : return LVL_TR2_CUT_3;
            // CUT4
            case 1116968 : // PSX JAP
            case 1116966 : version = VER_TR2_PSX;
            case 1900230 : return LVL_TR2_CUT_4;
        // TR3
            // TITLE
            case 653485  : // PSX JAP
            case 653403  :
            case 1275266 : return LVL_TR3_TITLE;
            // HOUSE
            case 1891244 : // PSX JAP
            case 1889506 :
            case 3437855 : return LVL_TR3_HOUSE;
            // JUNGLE
            case 1933419 : // PSX JAP
            case 1930923 :
            case 3360001 : return LVL_TR3_JUNGLE;
            // TEMPLE
            case 1945795 : // PSX JAP
            case 1943847 :
            case 3180412 : return LVL_TR3_TEMPLE;
            // QUADCHAS
            case 1881562 : // PSX JAP
            case 1880004 :
            case 3131453 : return LVL_TR3_QUADCHAS;
            // TONYBOSS
            case 1495530 : // PSX JAP
            case 1493796 :
            case 2618437 : return LVL_TR3_TONYBOSS;
            // SHORE
            case 1981723 : // PSX JAP
            case 1979611 :
            case 3365562 : return LVL_TR3_SHORE;
            // CRASH
            case 2018552 : // PSX JAP
            case 2016876 :
            case 3331132 : return LVL_TR3_CRASH;
            // RAPIDS
            case 1942136 : // PSX JAP
            case 1940346 :
            case 3148248 : return LVL_TR3_RAPIDS;
            // TRIBOSS
            case 1523788 : // PSX JAP
            case 1521998 :
            case 2276838 : return LVL_TR3_TRIBOSS;
            // ROOFS
            case 2098953 : // PSX JAP
            case 2097077 :
            case 3579476 : return LVL_TR3_ROOFS;
            // SEWER
            case 2035235 : // PSX JAP
            case 2033505 :
            case 3670149 : return LVL_TR3_SEWER;
            // TOWER
            case 2080921 : // PSX JAP
            case 2079183 :
            case 3523586 : return LVL_TR3_TOWER;
            // OFFICE
            case 1550728 : // PSX JAP
            case 1549006 :
            case 2504910 : return LVL_TR3_OFFICE;
            // NEVADA
            case 1940862 : // PSX JAP
            case 1938738 :
            case 3140417 : return LVL_TR3_NEVADA;
            // COMPOUND
            case 2050018 : // PSX JAP
            case 2048288 :
            case 3554880 : return LVL_TR3_COMPOUND;
            // AREA51
            case 2056073 : // PSX JAP
            case 2054325 :
            case 3530130 : return LVL_TR3_AREA51;
            // ANTARC
            case 2000298 : // PSX JAP
            case 1998398 :
            case 3515272 : return LVL_TR3_ANTARC;
            // MINES
            case 2025641 : // PSX JAP
            case 2023895 :
            case 3331087 : return LVL_TR3_MINES;
            // CITY
            case 2058380 : // PSX JAP
            case 2056388 :
            case 3575873 : return LVL_TR3_CITY;
            // CHAMBER
            case 1782615 : // PSX JAP
            case 1780885 :
            case 3190736 : return LVL_TR3_CHAMBER;
            // STPAUL
            case 1711157 : // PSX JAP
            case 1709427 :
            case 3140028 : return LVL_TR3_STPAUL;
            // CUT1
            case 757909  : // PSX JAP
            case 757827  :
            case 1547866 : return LVL_TR3_CUT_1;
            // CUT2
            case 982847  : // PSX JAP
            case 982765  :
            case 1957327 : return LVL_TR3_CUT_2;
            // CUT3
            case 1243054 : // PSX JAP
            case 1242972 :
            case 2020225 : return LVL_TR3_CUT_3;
            // CUT4
            case 430212  : // PSX JAP
            case 430130  :
            case 1048466 : return LVL_TR3_CUT_4;
            // CUT5
            case 971367  : // PSX JAP
            case 971285  :
            case 1580868 : return LVL_TR3_CUT_5;
            // CUT6
            case 1325570 : // PSX JAP
            case 1325488 :
            case 2430940 : return LVL_TR3_CUT_6;
            // CUT7
            case 467203  : // PSX JAP
            case 467121  :
            case 1253227 : return LVL_TR3_CUT_7;
            // CUT8
            case 494396  : // PSX JAP
            case 494314  :
            case 944962  : return LVL_TR3_CUT_8;
            // CUT9
            case 1002318 : // PSX JAP
            case 1002236 :
            case 1788075 : return LVL_TR3_CUT_9;
            // CUT11
            case 590267  : // PSX JAP
            case 590185  :
            case 1402003 : return LVL_TR3_CUT_11;
            // CUT12
            case 1080128 : // PSX JAP
            case 1080046 :
            case 2321393 : return LVL_TR3_CUT_12;
        }

        if (name) {
            // skip directory path
            int start = 0;
            for (int i = strlen(name) - 1; i >= 0; i--)
                if (name[i] == '/' || name[i] == '\\') {
                    start = i + 1;
                    break;
                }
            // skip file extension
            char buf[255];
            strcpy(buf, name + start);
            for (int i = 0; i < int(strlen(buf)); i++)
                if (buf[i] == '.') {
                    buf[i] = 0;
                    break;
                }
            // compare with standard levels
            // TODO: fix TITLE (2-3), HOUSE (3), CUTx (2-3)
            for (int i = 0; i < LVL_MAX; i++)
                if (!strcmp(buf, LEVEL_INFO[i].name))
                    return LevelID(i);
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
        ASSERT(false);
        return LVL_MAX;
    }

    LevelID getStartId(Version version) {
        switch (version & VER_VERSION) {
            case VER_TR1 : return LVL_TR1_1;
            case VER_TR2 : return LVL_TR2_WALL;
            case VER_TR3 : return LVL_TR3_JUNGLE;
        }
        ASSERT(false);
        return LVL_MAX;
    }

    LevelID getEndId(Version version) {
        switch (version & VER_VERSION) {
            case VER_TR1 : return LVL_TR1_END2;
            case VER_TR2 : return LVL_TR2_HOUSE;
            case VER_TR3 : return LVL_TR3_CHAMBER;
        }
        ASSERT(false);
        return LVL_MAX;
    }

    bool isCutsceneLevel(LevelID id) {
        return id == LVL_TR1_CUT_1 || id == LVL_TR1_CUT_2 || id == LVL_TR1_CUT_3 || id == LVL_TR1_CUT_4 ||
               id == LVL_TR2_CUT_1 || id == LVL_TR2_CUT_2 || id == LVL_TR2_CUT_3 || id == LVL_TR2_CUT_4 || 
               id == LVL_TR3_CUT_1 || id == LVL_TR3_CUT_2  || id == LVL_TR3_CUT_3  || id == LVL_TR3_CUT_4 ||
               id == LVL_TR3_CUT_5 || id == LVL_TR3_CUT_6  || id == LVL_TR3_CUT_7  || id == LVL_TR3_CUT_8 ||
               id == LVL_TR3_CUT_9 || id == LVL_TR3_CUT_11 || id == LVL_TR3_CUT_12;
    }

    bool isTitleLevel(LevelID id) {
        return id == LVL_TR1_TITLE || id == LVL_TR2_TITLE || id == LVL_TR3_TITLE;
    }

    bool isEmptyLevel(LevelID id) {
        return id == TR::LVL_TR1_10A || id == TR::LVL_TR2_RIG || id == TR::LVL_TR3_COMPOUND;
    }

    Version getGameVersion() {
        useEasyStart = true;
        if (Stream::existsContent("DATA/GYM.PHD") || Stream::existsContent("GYM.PHD"))
            return VER_TR1_PC;
        if (Stream::existsContent("PSXDATA/GYM.PSX"))
            return VER_TR1_PSX;
        if (Stream::existsContent("DATA/GYM.SAT"))
            return VER_TR1_SAT;

        if (Stream::existsContent("data/ASSAULT.TR2") || Stream::existsContent("assault.TR2"))
            return VER_TR2_PC;
        if (Stream::existsContent("DATA/ASSAULT.PSX"))
            return VER_TR2_PSX;

        if (Stream::existsContent("data/JUNGLE.TR2"))
            return VER_TR3_PC;

        if (Stream::existsContent("DATA/JUNGLE.PSX"))
            return VER_TR3_PSX;

        useEasyStart = false;
        return VER_UNKNOWN;
    }

    Version getGameVersionByLevel(LevelID id) {
        if (id >= LVL_TR1_TITLE && id <= LVL_TR1_END2)
            return VER_TR1;
        if (id >= LVL_TR2_TITLE && id <= LVL_TR2_HOUSE)
            return VER_TR2;
        if (id >= LVL_TR3_TITLE && id <= LVL_TR3_STPAUL)
            return VER_TR3;
        return VER_UNKNOWN;
    }

    void getGameLevelFile(char *dst, Version version, LevelID id) {
        if (useEasyStart) {
            switch (version) {
                case VER_TR1_PC  :
                    sprintf(dst, "DATA/%s.PHD", LEVEL_INFO[id].name);
                    if (Stream::existsContent(dst)) break;
                    sprintf(dst, "%s.PHD", LEVEL_INFO[id].name);
                    break;
                case VER_TR1_PSX : sprintf(dst, "PSXDATA/%s.PSX", LEVEL_INFO[id].name); break;
                case VER_TR1_SAT : sprintf(dst, "DATA/%s.SAT",    LEVEL_INFO[id].name); break;
                case VER_TR2_PC  : { // oh FFFFUUUUUUCKing CaTaComB.Tr2!
                    if (id == LVL_TR2_VENICE || id == LVL_TR2_CUT_2 || id == LVL_TR2_PLATFORM || id == LVL_TR2_CUT_3 || id == LVL_TR2_UNWATER || 
                        id == LVL_TR2_KEEL || id == LVL_TR2_LIVING || id == LVL_TR2_DECK || id == LVL_TR2_CATACOMB || id == LVL_TR2_ICECAVE ||
                        id == LVL_TR2_CUT_4 || id == LVL_TR2_XIAN || id == LVL_TR2_HOUSE) {
                        char buf[64];
                        strcpy(buf, LEVEL_INFO[id].name);
                        String::toLower(buf);
                        sprintf(dst, "DATA/%s.TR2", buf);
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
                case VER_TR3_PSX : sprintf(dst, isCutsceneLevel(id) ? "CUTS/%s.PSX" : "DATA/%s.PSX", LEVEL_INFO[id].name); break;
                default          : ASSERT(false);
            }
        } else {
            strcpy(dst, "level/");
            if ((version & VER_TR1) || version == VER_UNKNOWN) strcat(dst, "1/");
            if (version & VER_TR2) strcat(dst, "2/");
            if (version & VER_TR3) strcat(dst, "3/");
            strcat(dst, LEVEL_INFO[id].name);

            #ifdef _OS_WEB
                 strcat(dst, ".PSX");
            #else
                switch (version) {
                    case VER_TR1_PC  : strcat(dst, ".PHD"); break;
                    case VER_TR2_PC  : 
                    case VER_TR3_PC  : strcat(dst, ".TR2"); break;
                    case VER_TR1_PSX :
                    case VER_TR2_PSX : 
                    case VER_TR3_PSX : strcat(dst, ".PSX"); break;
                    case VER_UNKNOWN : 
                        if (Stream::existsContent("level/1/TITLE.PSX")) {
                            strcpy(dst, "level/1/TITLE.PSX");
                            return;
                        }
                        if (Stream::existsContent("level/1/TITLE.PHD")) {
                            strcpy(dst, "level/1/TITLE.PHD");
                            return;
                        }
                        if (Stream::existsContent("level/2/TITLE.TR2")) {
                            strcpy(dst, "level/2/TITLE.TR2");
                            return;
                        }
                        if (Stream::existsContent("level/2/TITLE.PSX")) {
                            strcpy(dst, "level/2/TITLE.PSX");
                            return;
                        }
                        if (Stream::existsContent("level/3/TITLE.TR2")) {
                            strcpy(dst, "level/3/TITLE.TR2");
                            return;
                        }
                        if (Stream::existsContent("level/3/TITLE.PSX")) {
                            strcpy(dst, "level/3/TITLE.PSX");
                            return;
                        }
                        ASSERT(false); //
                        break;
                    default : ASSERT(false);
                }
            #endif
        }
    }

    const char* getGameSoundsFile(Version version) {
        if (version == VER_TR2_PC) {
            CHECK_FILE("data/MAIN.SFX");    // PC
            CHECK_FILE("MAIN.SFX");         // Android
            return "audio/2/MAIN.SFX";      // Web
        }

        if (version == VER_TR3_PC) {
            CHECK_FILE("data/MAIN.SFX");    // PC
            return "audio/3/MAIN.SFX";      // Web
        }

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
                case VER_TR1_SAT :
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
                    //if (Stream::existsContent("audio/cdaudio.mp3")) {
                    //    callback(Sound::openCDAudioMP3("audio/cdaudio.dat", "audio/cdaudio.mp3", track), userData);
                    //    return;
                    //}
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
                case VER_TR1_SAT :
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
                    #ifndef _OS_WEB
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

    const char* getGameScreen(LevelID id) {
        switch (id) {
        // TR1
            case LVL_TR1_TITLE :
                CHECK_FILE("TITLEH.png");           // Android
                CHECK_FILE("DATA/TITLEH.PCX");      // PC
                CHECK_FILE("DELDATA/AMERTIT.RAW");  // PSX
                CHECK_FILE("DELDATA/JAPTIT.RAW");   // PSX JAP
                CHECK_FILE("BINDATA/USATIT.BIN");   // SEGA
                return "level/1/AMERTIT.PNG";       // WEB
            case LVL_TR1_GYM :
                CHECK_FILE("DELDATA/GYMLOAD.RAW");
                CHECK_FILE("BINDATA/GYM224.BIN");
                return "level/1/GYMLOAD.PNG";
            case LVL_TR1_1  :
            case LVL_TR1_2  :
            case LVL_TR1_3A :
            case LVL_TR1_3B :
                CHECK_FILE("DELDATA/AZTECLOA.RAW");
                CHECK_FILE("BINDATA/AZTEC224.BIN");
                return "level/1/AZTECLOA.PNG";
            case LVL_TR1_4  :
            case LVL_TR1_5  :
            case LVL_TR1_6  :
            case LVL_TR1_7A :
            case LVL_TR1_7B :
                CHECK_FILE("DELDATA/GREEKLOA.RAW");
                CHECK_FILE("BINDATA/GREEK224.BIN");
                return "level/1/GREEKLOA.PNG";
            case LVL_TR1_8A :
            case LVL_TR1_8B :
            case LVL_TR1_8C :
                CHECK_FILE("DELDATA/EGYPTLOA.RAW");
                CHECK_FILE("BINDATA/EGYPT224.BIN");
                return "level/1/EGYPTLOA.PNG";
            case LVL_TR1_10A :
            case LVL_TR1_10B :
            case LVL_TR1_10C :
                CHECK_FILE("DELDATA/ATLANLOA.RAW");
                CHECK_FILE("BINDATA/ATLAN224.BIN");
                return "level/1/ATLANLOA.PNG";
        // TR2
            case LVL_TR2_TITLE :
                CHECK_FILE("TITLE.png");            // Android
                CHECK_FILE("data/TITLE.PCX");       // PC
                CHECK_FILE("pix/title.pcx");        // PC
                CHECK_FILE("PIXUS/TITLEUS.RAW");    // PSX US
                CHECK_FILE("PIXJAP/TITLEJAP.RAW");  // PSX US
                return "level/2/TITLEUS.PNG";       // WEB
            case LVL_TR2_ASSAULT :
            case LVL_TR2_HOUSE   :
                CHECK_FILE("PIX/MANSION.RAW"); 
                return "level/2/MANSION.PNG";
            case LVL_TR2_WALL     :
            case LVL_TR2_EMPRTOMB :
            case LVL_TR2_FLOATING :
            case LVL_TR2_XIAN     :
                CHECK_FILE("PIX/CHINA.RAW"); 
                return "level/2/CHINA.PNG";
            case LVL_TR2_BOAT   :
            case LVL_TR2_VENICE :
            case LVL_TR2_OPERA  :
                CHECK_FILE("PIX/VENICE.RAW"); 
                return "level/2/VENICE.PNG";
            case LVL_TR2_RIG      :
            case LVL_TR2_PLATFORM :
                CHECK_FILE("PIX/RIG.RAW"); 
                return "level/2/RIG.PNG";
            case LVL_TR2_UNWATER :
            case LVL_TR2_KEEL    :
            case LVL_TR2_LIVING  :
            case LVL_TR2_DECK    :
                CHECK_FILE("PIX/TITAN.RAW"); 
                return "level/2/TITAN.PNG";
            case LVL_TR2_SKIDOO   :
            case LVL_TR2_MONASTRY :
            case LVL_TR2_CATACOMB :
            case LVL_TR2_ICECAVE  :
                CHECK_FILE("PIX/TIBET.RAW");
                return "level/2/TIBET.PNG";
        // TR3
            case LVL_TR3_TITLE :
                CHECK_FILE("pix/TITLEUK.BMP");      // PC
                CHECK_FILE("PIXUS/TITLEUS.RAW");    // PSX
                CHECK_FILE("PIXJAP/TITLEJAP.RAW");  // PSX
                return "level/3/TITLEUK.PNG";       // WEB
            case LVL_TR3_HOUSE  :
                CHECK_FILE("pix/HOUSE.BMP"); 
                CHECK_FILE("PIX/HOUSE.RAW"); 
                return "level/3/HOUSE.PNG";
            case LVL_TR3_JUNGLE   :
            case LVL_TR3_TEMPLE   :
            case LVL_TR3_QUADCHAS :
            case LVL_TR3_TONYBOSS :
                CHECK_FILE("pix/INDIA.BMP"); 
                CHECK_FILE("PIX/INDIA.RAW"); 
                return "level/3/INDIA.PNG";
            case LVL_TR3_SHORE   :
            case LVL_TR3_CRASH   :
            case LVL_TR3_RAPIDS  :
            case LVL_TR3_TRIBOSS :
                CHECK_FILE("pix/SOUTHPAC.BMP"); 
                CHECK_FILE("PIX/SOUTHPAC.RAW"); 
                return "level/3/SOUTHPAC.PNG";
            case LVL_TR3_ROOFS  :
            case LVL_TR3_SEWER  :
            case LVL_TR3_TOWER  :
            case LVL_TR3_OFFICE :
            case LVL_TR3_STPAUL :
                CHECK_FILE("pix/LONDON.BMP"); 
                CHECK_FILE("PIX/LONDON.RAW"); 
                return "level/3/LONDON.PNG";
            case LVL_TR3_NEVADA   :
            case LVL_TR3_COMPOUND :
            case LVL_TR3_AREA51   :
                CHECK_FILE("pix/NEVADA.BMP"); 
                CHECK_FILE("PIX/NEVADA.RAW"); 
                return "level/3/NEVADA.PNG";
            case LVL_TR3_ANTARC  :
            case LVL_TR3_MINES   :
            case LVL_TR3_CITY    :
            case LVL_TR3_CHAMBER :
                CHECK_FILE("pix/ANTARC.BMP"); 
                CHECK_FILE("PIX/ANTARC.RAW"); 
                return "level/3/ANTARC.PNG";

            default : return NULL;
        }
    }

    const char* getGameLogo(Version version) {
        if (version & VER_TR1) {
            CHECK_FILE("FMV/CORELOGO.FMV");
            CHECK_FILE("FMV/CORE.RPL");
            CHECK_FILE("video/1/CORELOGO.FMV");
            CHECK_FILE("video/1/CORE.RPL");
        }

        if (version & VER_TR2) {
            CHECK_FILE("FMV/LOGO.FMV");
            CHECK_FILE("FMV/LOGO.RPL");
            CHECK_FILE("video/2/LOGO.FMV");
            CHECK_FILE("video/2/LOGO.RPL");
        }

        if (version & VER_TR3) {
            CHECK_FILE("FMV/LOGO.FMV");
            CHECK_FILE("fmv/logo.rpl");
            CHECK_FILE("video/3/LOGO.FMV");
            CHECK_FILE("video/3/logo.rpl");
        }

        return NULL;
    }

    const char* getGameVideo(LevelID id) {
        switch (id) {
        // TR1
            case LVL_TR1_TITLE    :
                CHECK_FILE("FMV/CAFE.FMV");
                CHECK_FILE("FMV/CAFE.RPL");
                CHECK_FILE("video/1/CAFE.FMV");
                CHECK_FILE("video/1/CAFE.RPL");
                break;
            case LVL_TR1_GYM      :
                CHECK_FILE("FMV/MANSION.FMV");
                CHECK_FILE("FMV/MANSION.RPL");
                CHECK_FILE("video/1/MANSION.FMV");
                CHECK_FILE("video/1/MANSION.RPL");
                break;
            case LVL_TR1_1        :
                CHECK_FILE("FMV/SNOW.FMV");
                CHECK_FILE("FMV/SNOW.RPL");
                CHECK_FILE("video/1/SNOW.FMV");
                CHECK_FILE("video/1/SNOW.RPL");
                break;
            case LVL_TR1_4        :
                CHECK_FILE("FMV/LIFT.FMV");
                CHECK_FILE("FMV/LIFT.RPL");
                CHECK_FILE("video/1/LIFT.FMV");
                CHECK_FILE("video/1/LIFT.RPL");
                break;
            case LVL_TR1_8A       :
                CHECK_FILE("FMV/VISION.FMV");
                CHECK_FILE("FMV/VISION.RPL");
                CHECK_FILE("video/1/VISION.FMV");
                CHECK_FILE("video/1/VISION.RPL");
                break;
            case LVL_TR1_10A      :
                CHECK_FILE("FMV/CANYON.FMV");
                CHECK_FILE("FMV/CANYON.RPL");
                CHECK_FILE("video/1/CANYON.FMV");
                CHECK_FILE("video/1/CANYON.RPL");
                break;
            case LVL_TR1_10B      :
                CHECK_FILE("FMV/PYRAMID.FMV");
                CHECK_FILE("FMV/PYRAMID.RPL");
                CHECK_FILE("video/1/PYRAMID.FMV");
                CHECK_FILE("video/1/PYRAMID.RPL");
                break;
            case LVL_TR1_CUT_4    :
                CHECK_FILE("FMV/PRISON.FMV");
                CHECK_FILE("FMV/PRISON.RPL");
                CHECK_FILE("video/1/PRISON.FMV");
                CHECK_FILE("video/1/PRISON.RPL");
                break;
            case LVL_TR1_EGYPT    :
                CHECK_FILE("FMV/END.FMV");
                CHECK_FILE("FMV/END.RPL");
                CHECK_FILE("video/1/END.FMV");
                CHECK_FILE("video/1/END.RPL");
                break;
        // TR2
            case LVL_TR2_TITLE    :
                CHECK_FILE("FMV/ANCIENT.FMV");
                CHECK_FILE("fmv/ANCIENT.RPL");
                CHECK_FILE("video/2/ANCIENT.FMV");
                CHECK_FILE("video/2/ANCIENT.RPL");
                break;
            case LVL_TR2_WALL     :
                CHECK_FILE("FMV/MODERN.FMV");
                CHECK_FILE("fmv/MODERN.RPL");
                CHECK_FILE("video/2/MODERN.FMV");
                CHECK_FILE("video/2/MODERN.RPL");
                break;
            case LVL_TR2_RIG      :
                CHECK_FILE("FMV/LANDING.FMV");
                CHECK_FILE("fmv/LANDING.RPL");
                CHECK_FILE("video/2/LANDING.FMV");
                CHECK_FILE("video/2/LANDING.RPL");
                break;
            case LVL_TR2_UNWATER  :
                CHECK_FILE("FMV/MS.FMV");
                CHECK_FILE("fmv/MS.RPL");
                CHECK_FILE("video/2/MS.FMV");
                CHECK_FILE("video/2/MS.RPL");
                break;
            case LVL_TR2_SKIDOO   :
                CHECK_FILE("FMV/CRASH.FMV");
                CHECK_FILE("fmv/CRASH.RPL");
                CHECK_FILE("video/2/CRASH.FMV");
                CHECK_FILE("video/2/CRASH.RPL");
                break;
            case LVL_TR2_EMPRTOMB :
                CHECK_FILE("FMV/JEEP.FMV");
                CHECK_FILE("fmv/JEEP.RPL");
                CHECK_FILE("video/2/JEEP.FMV");
                CHECK_FILE("video/2/JEEP.RPL");
                break;
            case LVL_TR2_HOUSE    :
                CHECK_FILE("FMV/END.FMV");
                CHECK_FILE("fmv/END.RPL");
                CHECK_FILE("video/2/END.FMV");
                CHECK_FILE("video/2/END.RPL");
                break;
        // TR3
            case LVL_TR3_TITLE :
                CHECK_FILE("FMV/INTRO.FMV");
                CHECK_FILE("fmv/Intr_Eng.rpl");
                CHECK_FILE("video/3/INTRO.FMV");
                CHECK_FILE("video/3/Intr_Eng.rpl");
                break;
            case LVL_TR3_SHORE :
                CHECK_FILE("FMV/LAGOON.FMV");
                CHECK_FILE("fmv/Sail_Eng.rpl");
                CHECK_FILE("video/3/LAGOON.FMV");
                CHECK_FILE("video/3/Sail_Eng.rpl");
                break;
            case LVL_TR3_ANTARC :
                CHECK_FILE("FMV/HUEY.FMV");
                CHECK_FILE("fmv/Crsh_Eng.rpl");
                CHECK_FILE("video/3/HUEY.FMV");
                CHECK_FILE("video/3/Crsh_Eng.rpl");
                break;
            case LVL_TR3_STPAUL :
                CHECK_FILE("FMV/END.FMV");
                CHECK_FILE("fmv/Endgame.rpl");
                CHECK_FILE("video/3/END.FMV");
                CHECK_FILE("video/3/Endgame.rpl");
                break;
            default : ;
        }
        return NULL;
    }

    #define FOG_DIST    (1.0f / (18 * 1024))
    #define FOG_BLACK   vec4(0.0f, 0.0f, 0.0f, FOG_DIST)
    #define FOG_SANDY   vec4(0.2f, 0.1f, 0.0f, FOG_DIST)
    #define FOG_GREEN   vec4(0.0f, 0.1f, 0.0f, FOG_DIST)
    #define FOG_RED     vec4(0.2f, 0.0f, 0.0f, FOG_DIST)

    vec4 getFogParams(LevelID id) {
        switch (id) {
            case LVL_TR1_1     :
            case LVL_TR1_2     :
            case LVL_TR1_3A    :
            case LVL_TR1_3B    :
            case LVL_TR1_CUT_1 : return FOG_BLACK;
            case LVL_TR1_4     :
            case LVL_TR1_5     :
            case LVL_TR1_6     : return FOG_SANDY;
            case LVL_TR1_7A    :
            case LVL_TR1_7B    :
            case LVL_TR1_CUT_2 : return FOG_GREEN;
            case LVL_TR1_8A    :
            case LVL_TR1_8B    :
            case LVL_TR1_8C    : return FOG_SANDY;
            case LVL_TR1_10A   : return FOG_BLACK;
            case LVL_TR1_CUT_3 :
            case LVL_TR1_10B   :
            case LVL_TR1_CUT_4 :
            case LVL_TR1_10C   : return FOG_RED;
            case LVL_TR1_EGYPT :
            case LVL_TR1_CAT   :
            case LVL_TR1_END   :
            case LVL_TR1_END2  : return FOG_SANDY;
            default            : return FOG_BLACK;
        }
    }
}

#undef CHECK_FILE

#endif
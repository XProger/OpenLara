#ifndef H_GAMEFLOW
#define H_GAMEFLOW

#include "utils.h"
#include "lang.h"

#define CHECK_FILE(name) if (Stream::existsContent(name)) return name

namespace TR {

    bool useEasyStart;
    bool isGameEnded;

    enum {
        NO_TRACK = 0xFF,
    };

    enum Version {
        VER_UNKNOWN  = 0,

        VER_PC       = 0x1000,
        VER_PSX      = 0x2000,
        VER_SAT      = 0x4000,
        VER_SDC      = 0x8000,

        VER_TR1      = 0x01,
        VER_TR2      = 0x02,
        VER_TR3      = 0x04,
        VER_TR4      = 0x08,
        VER_TR5      = 0x10,

        VER_VERSION  = VER_TR1 | VER_TR2 | VER_TR3 | VER_TR4 | VER_TR5,
        VER_PLATFORM = VER_PC  | VER_PSX | VER_SAT | VER_SDC,

        VER_TR1_PC   = VER_TR1 | VER_PC,
        VER_TR1_PSX  = VER_TR1 | VER_PSX,
        VER_TR1_SAT  = VER_TR1 | VER_SAT,

        VER_TR2_PC   = VER_TR2 | VER_PC,
        VER_TR2_PSX  = VER_TR2 | VER_PSX,

        VER_TR3_PC   = VER_TR3 | VER_PC,
        VER_TR3_PSX  = VER_TR3 | VER_PSX,

        VER_TR4_PC   = VER_TR4 | VER_PC,
        VER_TR4_PSX  = VER_TR4 | VER_PSX,
        VER_TR4_SDC  = VER_TR4 | VER_SDC,

        VER_TR5_PC   = VER_TR5 | VER_PC,
        VER_TR5_PSX  = VER_TR5 | VER_PSX,
        VER_TR5_SDC  = VER_TR5 | VER_SDC,

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
    // TR4
        LVL_TR4_TITLE,
        LVL_TR4_ANGKOR1,
        LVL_TR4_ANG_RACE,
        LVL_TR4_SETTOMB1,
        LVL_TR4_SETTOMB2,
        LVL_TR4_JEEPCHAS,
        LVL_TR4_JEEPCHS2,
        LVL_TR4_KARNAK1,
        LVL_TR4_HALL,
        LVL_TR4_LAKE,
        LVL_TR4_SEMER,
        LVL_TR4_SEMER2,
        LVL_TR4_TRAIN,
        LVL_TR4_ALEXHUB,
        LVL_TR4_ALEXHUB2,
        LVL_TR4_PALACES,
        LVL_TR4_PALACES2,
        LVL_TR4_CSPLIT1,
        LVL_TR4_CSPLIT2,
        LVL_TR4_LIBRARY,
        LVL_TR4_LIBEND,
        LVL_TR4_BIKEBIT,
        LVL_TR4_NUTRENCH,
        LVL_TR4_CORTYARD,
        LVL_TR4_LOWSTRT,
        LVL_TR4_HIGHSTRT,
        LVL_TR4_CITNEW,
        LVL_TR4_JOBY1A,
        LVL_TR4_JOBY1B,
        LVL_TR4_JOBY2,
        LVL_TR4_JOBY3A,
        LVL_TR4_JOBY3B,
        LVL_TR4_JOBY4A,
        LVL_TR4_JOBY4B,
        LVL_TR4_JOBY4C,
        LVL_TR4_JOBY5A,
        LVL_TR4_JOBY5B,
        LVL_TR4_JOBY5C,

        LVL_MAX,
    };

    enum {
    // TR1
        TRACK_TR1_TITLE     = 2,
        TRACK_TR1_CAVES     = 5,
        TRACK_TR1_SECRET    = 13,
        TRACK_TR1_CISTERN   = 57,
        TRACK_TR1_WIND      = 58,
        TRACK_TR1_PYRAMID   = 59,
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
    // TR4
        TRACK_TR4_BOSS      = 97,
        TRACK_TR4_JEEP      = 98,
        TRACK_TR4_BATTLE    = 102,
        TRACK_TR4_TITLE     = 104,
        TRACK_TR4_COASTAL   = 105,
        TRACK_TR4_TRAIN     = 106,
        TRACK_TR4_IN_DARK   = 107,
        TRACK_TR4_IN_DRIPS  = 108,
        TRACK_TR4_WEIRD1    = 109,
        TRACK_TR4_OUT_DAY   = 110,
        TRACK_TR4_OUT_NIGHT = 111,
    };

    struct LevelInfo {
        const char *name;
        StringID   title;
        uint8      track;
        uint8      secrets;
    } LEVEL_INFO[LVL_MAX] = {
        { ""          , STR_EMPTY         , TRACK_TR1_CAVES     , 0 },
    // TR1
        { "TITLE"     , STR_EMPTY         , TRACK_TR1_TITLE     , 0 },
        { "GYM"       , STR_TR1_GYM       , NO_TRACK            , 0 },
        { "LEVEL1"    , STR_TR1_LEVEL1    , TRACK_TR1_CAVES     , 3 },
        { "LEVEL2"    , STR_TR1_LEVEL2    , TRACK_TR1_CAVES     , 3 },
        { "LEVEL3A"   , STR_TR1_LEVEL3A   , TRACK_TR1_CAVES     , 5 },
        { "LEVEL3B"   , STR_TR1_LEVEL3B   , TRACK_TR1_CAVES     , 3 },
        { "CUT1"      , STR_EMPTY         , TRACK_TR1_CUT_1     , 0 },
        { "LEVEL4"    , STR_TR1_LEVEL4    , TRACK_TR1_WIND      , 4 },
        { "LEVEL5"    , STR_TR1_LEVEL5    , TRACK_TR1_WIND      , 3 },
        { "LEVEL6"    , STR_TR1_LEVEL6    , TRACK_TR1_WIND      , 3 },
        { "LEVEL7A"   , STR_TR1_LEVEL7A   , TRACK_TR1_CISTERN   , 3 },
        { "LEVEL7B"   , STR_TR1_LEVEL7B   , TRACK_TR1_CISTERN   , 2 },
        { "CUT2"      , STR_EMPTY         , TRACK_TR1_CUT_2     , 0 },
        { "LEVEL8A"   , STR_TR1_LEVEL8A   , TRACK_TR1_WIND      , 3 },
        { "LEVEL8B"   , STR_TR1_LEVEL8B   , TRACK_TR1_WIND      , 3 },
        { "LEVEL8C"   , STR_TR1_LEVEL8C   , TRACK_TR1_WIND      , 1 },
        { "LEVEL10A"  , STR_TR1_LEVEL10A  , TRACK_TR1_CISTERN   , 3 },
        { "CUT3"      , STR_EMPTY         , TRACK_TR1_CUT_3     , 0 },
        { "LEVEL10B"  , STR_TR1_LEVEL10B  , TRACK_TR1_PYRAMID   , 3 },
        { "CUT4"      , STR_EMPTY         , TRACK_TR1_CUT_4     , 0 },
        { "LEVEL10C"  , STR_TR1_LEVEL10C  , TRACK_TR1_PYRAMID   , 3 },
        { "EGYPT"     , STR_TR1_EGYPT     , TRACK_TR1_WIND      , 3 },
        { "CAT"       , STR_TR1_CAT       , TRACK_TR1_WIND      , 4 },
        { "END"       , STR_TR1_END       , TRACK_TR1_WIND      , 2 },
        { "END2"      , STR_TR1_END2      , TRACK_TR1_WIND      , 1 },
    // TR2
        { "TITLE"     , STR_EMPTY         , TRACK_TR2_TITLE     , 0 },
        { "ASSAULT"   , STR_TR2_ASSAULT   , NO_TRACK            , 0 },
        { "WALL"      , STR_TR2_WALL      , TRACK_TR2_CHINA_1   , 3 },
        { "CUT1"      , STR_EMPTY         , TRACK_TR2_CUT_1     , 0 },
        { "BOAT"      , STR_TR2_BOAT      , NO_TRACK            , 3 },
        { "VENICE"    , STR_TR2_VENICE    , NO_TRACK            , 3 },
        { "OPERA"     , STR_TR2_OPERA     , TRACK_TR2_ITALY     , 3 },
        { "CUT2"      , STR_EMPTY         , TRACK_TR2_CUT_2     , 0 },
        { "RIG"       , STR_TR2_RIG       , TRACK_TR2_RIG       , 3 },
        { "PLATFORM"  , STR_TR2_PLATFORM  , TRACK_TR2_RIG       , 3 },
        { "CUT3"      , STR_EMPTY         , TRACK_TR2_CUT_3     , 0 },
        { "UNWATER"   , STR_TR2_UNWATER   , TRACK_TR2_UNWATER_1 , 3 },
        { "KEEL"      , STR_TR2_KEEL      , TRACK_TR2_UNWATER_2 , 3 },
        { "LIVING"    , STR_TR2_LIVING    , TRACK_TR2_UNWATER_1 , 3 },
        { "DECK"      , STR_TR2_DECK      , TRACK_TR2_UNWATER_2 , 3 },
        { "SKIDOO"    , STR_TR2_SKIDOO    , TRACK_TR2_TIBET_1   , 3 },
        { "MONASTRY"  , STR_TR2_MONASTRY  , NO_TRACK            , 3 },
        { "CATACOMB"  , STR_TR2_CATACOMB  , TRACK_TR2_TIBET_2   , 3 },
        { "ICECAVE"   , STR_TR2_ICECAVE   , TRACK_TR2_TIBET_2   , 3 },
        { "EMPRTOMB"  , STR_TR2_EMPRTOMB  , TRACK_TR2_CHINA_2   , 3 },
        { "CUT4"      , STR_EMPTY         , TRACK_TR2_CUT_4     , 0 },
        { "FLOATING"  , STR_TR2_FLOATING  , TRACK_TR2_CHINA_2   , 3 },
        { "XIAN"      , STR_TR2_XIAN      , TRACK_TR2_CHINA_2   , 3 },
        { "HOUSE"     , STR_TR2_HOUSE     , NO_TRACK            , 3 },
    // TR3
        { "TITLE"     , STR_EMPTY         , TRACK_TR3_TITLE     , 0 },
        { "HOUSE"     , STR_TR3_HOUSE     , NO_TRACK            , 3 },
        { "JUNGLE"    , STR_TR3_JUNGLE    , TRACK_TR3_INDIA_1   , 3 },
        { "CUT6"      , STR_EMPTY         , TRACK_TR3_CUT_6     , 0 },
        { "TEMPLE"    , STR_TR3_TEMPLE    , TRACK_TR3_INDIA_1   , 3 },
        { "CUT9"      , STR_EMPTY         , TRACK_TR3_CUT_9     , 0 },
        { "QUADCHAS"  , STR_TR3_QUADCHAS  , TRACK_TR3_INDIA_1   , 3 },
        { "TONYBOSS"  , STR_TR3_TONYBOSS  , TRACK_TR3_INDIA_2   , 3 },
        { "SHORE"     , STR_TR3_SHORE     , TRACK_TR3_SOUTH_1   , 3 },
        { "CUT1"      , STR_EMPTY         , TRACK_TR3_CUT_1     , 0 },
        { "CRASH"     , STR_TR3_CRASH     , TRACK_TR3_SOUTH_2   , 3 },
        { "CUT4"      , STR_EMPTY         , TRACK_TR3_CUT_4     , 0 },
        { "RAPIDS"    , STR_TR3_RAPIDS    , TRACK_TR3_SOUTH_3   , 3 },
        { "TRIBOSS"   , STR_TR3_TRIBOSS   , TRACK_TR3_CAVES     , 3 },
        { "ROOFS"     , STR_TR3_ROOFS     , TRACK_TR3_LONDON_1  , 3 },
        { "CUT2"      , STR_EMPTY         , TRACK_TR3_CUT_2     , 0 },
        { "SEWER"     , STR_TR3_SEWER     , TRACK_TR3_LONDON_2  , 3 },
        { "CUT5"      , STR_EMPTY         , TRACK_TR3_CUT_5     , 0 },
        { "TOWER"     , STR_TR3_TOWER     , TRACK_TR3_LONDON_3  , 3 },
        { "CUT11"     , STR_EMPTY         , TRACK_TR3_CUT_11    , 0 },
        { "OFFICE"    , STR_TR3_OFFICE    , TRACK_TR3_LONDON_4  , 3 },
        { "NEVADA"    , STR_TR3_NEVADA    , TRACK_TR3_NEVADA_1  , 3 },
        { "CUT7"      , STR_EMPTY         , TRACK_TR3_CUT_7     , 0 },
        { "COMPOUND"  , STR_TR3_COMPOUND  , TRACK_TR3_NEVADA_2  , 3 },
        { "CUT8"      , STR_EMPTY         , TRACK_TR3_CUT_8     , 0 },
        { "AREA51"    , STR_TR3_AREA51    , TRACK_TR3_NEVADA_2  , 3 },
        { "ANTARC"    , STR_TR3_ANTARC    , TRACK_TR3_ANTARC_1  , 3 },
        { "CUT3"      , STR_EMPTY         , TRACK_TR3_CUT_3     , 0 },
        { "MINES"     , STR_TR3_MINES     , TRACK_TR3_ANTARC_2  , 3 },
        { "CITY"      , STR_TR3_CITY      , TRACK_TR3_ANTARC_3  , 3 },
        { "CUT12"     , STR_EMPTY         , TRACK_TR3_CUT_12    , 0 },
        { "CHAMBER"   , STR_TR3_CHAMBER   , TRACK_TR3_ANTARC_3  , 3 },
        { "STPAUL"    , STR_TR3_STPAUL    , TRACK_TR3_CAVES     , 3 },
    // TR4
        { "title"     , STR_UNKNOWN       , TRACK_TR4_TITLE     , 0 },
        { "angkor1"   , STR_UNKNOWN       , TRACK_TR4_OUT_DAY   , 0 },
        { "ang_race"  , STR_UNKNOWN       , TRACK_TR4_OUT_DAY   , 0 },
        { "settomb1"  , STR_UNKNOWN       , TRACK_TR4_IN_DARK   , 0 },
        { "settomb2"  , STR_UNKNOWN       , TRACK_TR4_IN_DARK   , 0 },
        { "jeepchas"  , STR_UNKNOWN       , TRACK_TR4_OUT_DAY   , 0 },
        { "jeepchs2"  , STR_UNKNOWN       , TRACK_TR4_JEEP      , 0 },
        { "karnak1"   , STR_UNKNOWN       , TRACK_TR4_OUT_DAY   , 0 },
        { "hall"      , STR_UNKNOWN       , TRACK_TR4_OUT_DAY   , 0 },
        { "lake"      , STR_UNKNOWN       , TRACK_TR4_OUT_DAY   , 0 },
        { "semer"     , STR_UNKNOWN       , TRACK_TR4_IN_DARK   , 0 },
        { "semer2"    , STR_UNKNOWN       , TRACK_TR4_IN_DARK   , 0 },
        { "train"     , STR_UNKNOWN       , TRACK_TR4_TRAIN     , 0 },
        { "alexhub"   , STR_UNKNOWN       , TRACK_TR4_OUT_DAY   , 0 },
        { "alexhub2"  , STR_UNKNOWN       , TRACK_TR4_COASTAL   , 0 },
        { "palaces"   , STR_UNKNOWN       , TRACK_TR4_IN_DARK   , 0 },
        { "palaces2"  , STR_UNKNOWN       , TRACK_TR4_IN_DARK   , 0 },
        { "csplit1"   , STR_UNKNOWN       , TRACK_TR4_IN_DRIPS  , 0 },
        { "csplit2"   , STR_UNKNOWN       , TRACK_TR4_IN_DRIPS  , 0 },
        { "library"   , STR_UNKNOWN       , TRACK_TR4_IN_DRIPS  , 0 },
        { "libend"    , STR_UNKNOWN       , TRACK_TR4_WEIRD1    , 0 },
        { "bikebit"   , STR_UNKNOWN       , TRACK_TR4_BATTLE    , 0 },
        { "nutrench"  , STR_UNKNOWN       , TRACK_TR4_BATTLE    , 0 },
        { "cortyard"  , STR_UNKNOWN       , TRACK_TR4_BATTLE    , 0 },
        { "lowstrt"   , STR_UNKNOWN       , TRACK_TR4_BATTLE    , 0 },
        { "highstrt"  , STR_UNKNOWN       , TRACK_TR4_BATTLE    , 0 },
        { "citnew"    , STR_UNKNOWN       , TRACK_TR4_BATTLE    , 0 },
        { "joby1a"    , STR_UNKNOWN       , TRACK_TR4_OUT_NIGHT , 0 },
        { "joby1b"    , STR_UNKNOWN       , TRACK_TR4_IN_DARK   , 0 },
        { "joby2"     , STR_UNKNOWN       , TRACK_TR4_IN_DARK   , 0 },
        { "joby3a"    , STR_UNKNOWN       , TRACK_TR4_OUT_NIGHT , 0 },
        { "joby3b"    , STR_UNKNOWN       , TRACK_TR4_IN_DARK   , 0 },
        { "joby4a"    , STR_UNKNOWN       , TRACK_TR4_OUT_NIGHT , 0 },
        { "joby4b"    , STR_UNKNOWN       , TRACK_TR4_IN_DARK   , 0 },
        { "joby4c"    , STR_UNKNOWN       , TRACK_TR4_OUT_NIGHT , 0 },
        { "joby5a"    , STR_UNKNOWN       , TRACK_TR4_IN_DARK   , 0 },
        { "joby5b"    , STR_UNKNOWN       , TRACK_TR4_BOSS      , 0 },
        { "joby5c"    , STR_UNKNOWN       , TRACK_TR4_IN_DRIPS  , 0 },
    };

    static const char* TRACK_LIST_TR4[] = {
          "044_Attack_part_i"
        , "008_VonCroy9a"
        , "100_Attack_part_ii"
        , "010_VonCroy10"
        , "015_VonCroy14"
        , "073_Secret"
        , "109_Lyre_01"
        , "042_Action_Part_iv"
        , "043_Action_Part_v"
        , "030_VonCroy30"
        , "012_VonCroy11b"
        , "011_VonCroy11a"
        , "063_Misc_Inc_01"
        , "014_VonCroy13b"
        , "111_charmer"
        , "025_VonCroy24b"
        , "023_VonCroy23"
        , "006_VonCroy7"
        , "024_VonCroy24a"
        , "110_Lyre_02"
        , "020_VonCroy19"
        , "034_VonCroy34"
        , "054_General_Part_ii"
        , "036_VonCroy36"
        , "004_VonCroy5"
        , "035_VonCroy35"
        , "027_VonCroy27"
        , "053_General_Part_i"
        , "022_VonCroy22b"
        , "028_VonCroy28_L11"
        , "003_VonCroy4"
        , "001_VonCroy2"
        , "041_Action_Part_iii"
        , "057_General_Part_v"
        , "018_VonCroy17"
        , "064_Misc_Inc_02"
        , "033_VonCroy33"
        , "031_VonCroy31_L12"
        , "032_VonCroy32_L13"
        , "016_VonCroy15"
        , "065_Misc_Inc_03"
        , "040_Action_Part_ii"
        , "112_Gods_part_iv"
        , "029_VonCroy29"
        , "007_VonCroy8"
        , "013_VonCroy12_13a_Lara4"
        , "009_VonCroy9b_Lara3"
        , "081_dig"
        , "085_intro"
        , "071_Ominous_Part_i"
        , "095_phildoor"
        , "061_In_The_Pyramid_Part_i"
        , "050_Underwater_Find_part_i"
        , "058_Gods_Part_i"
        , "005_VonCroy6_Lara2"
        , "045_Authentic_TR"
        , "060_Gods_Part_iii"
        , "055_General_Part_iii"
        , "059_Gods_Part_ii"
        , "068_Mystery_Part_ii"
        , "076_captain2"
        , "019_Lara6_VonCroy18"
        , "002_VonCroy3"
        , "066_Misc_Inc_04"
        , "067_Mystery_Part_i"
        , "038_A_Short_01"
        , "088_key"
        , "017_VonCroy16_lara5"
        , "026_VC25_L9_VC26_L10"
        , "056_General_Part_iv"
        , "021_VC20_L7_VC21_L8_VC22a"
        , "096_sarcoph"
        , "087_jeepB"
        , "091_minilib1"
        , "086_jeepA"
        , "051_Egyptian_Mood_Part_i"
        , "078_croywon"
        , "092_minilib2"
        , "083_horus"
        , "049_Close_to_the_End_part_ii"
        , "037_VC37_L15_VC38"
        , "097_scorpion"
        , "089_larawon"
        , "094_minilib4"
        , "098_throne"
        , "048_Close_to_the_End"
        , "070_Mystery_Part_iv"
        , "093_minilib3"
        , "072_Puzzle_part_i"
        , "074_backpack"
        , "069_Mystery_Part_iii"
        , "052_Egyptian_Mood_Part_ii"
        , "084_inscrip"
        , "099_whouse"
        , "047_Boss_02"
        , "080_crypt2"
        , "090_libend"
        , "046_Boss_01"
        , "062_Jeep_Thrills_max"
        , "079_crypt1"
        , "082_finale"
        , "075_captain1"
        , "105_A5_Battle"
        , "077_crocgod"
        , "039_TR4_Title_Q10"
        , "108_A8_Coastal"
        , "107_A7_Train+"
        , "101_A1_In_Dark"
        , "102_A2_In_Drips"
        , "104_A4_Weird1"
        , "106_A6_Out_Day"
        , "103_A3_Out_Night"
    };

    Version getGameVersionByLevel(LevelID id) {
        if (id >= LVL_TR1_TITLE && id <= LVL_TR1_END2)
            return VER_TR1;
        if (id >= LVL_TR2_TITLE && id <= LVL_TR2_HOUSE)
            return VER_TR2;
        if (id >= LVL_TR3_TITLE && id <= LVL_TR3_STPAUL)
            return VER_TR3;
        if (id >= LVL_TR4_TITLE && id <= LVL_TR4_JOBY5C)
            return VER_TR4;
        return VER_UNKNOWN;
    }

    LevelID getLevelID(int size, const char *name, Version &version, bool &isDemoLevel) {
        isDemoLevel = false;
        switch (size) {
        // TR1
            // TITLE
            case 585648  : // PSX JAP
            case 508614  : version = VER_TR1_PSX;
            case 5148    : // SAT
            case 320412  : // PC JAP
            case 334874  :
            case 316138  :
            case 316518  : // PC G
            case 316460  : return LVL_TR1_TITLE;
            // GYM
            case 1234800 : // PSX JAP
            case 1074234 : version = VER_TR1_PSX;
            case 343854  : // SAT
            case 3377974 : // PC JAP
            case 3236806 :
            case 3282970 : // PC G
            case 3237128 : return LVL_TR1_GYM;
            // LEVEL1
            case 1667568 : // PSX JAP
            case 1448896 : version = VER_TR1_PSX;
            case 497656  : // SAT
            case 2540906 : // PC JAP
            case 2533312 :
            case 2533634 : return LVL_TR1_1;
            // LEVEL2
            case 2873406 : isDemoLevel = true; return LVL_TR1_2;
            case 1766352 : // PSX JAP
            case 1535734 : version = VER_TR1_PSX;
            case 532250  : // SAT
            case 2880722 : // PC JAP
            case 2873128 :
            case 2873450 : return LVL_TR1_2;
            // LEVEL3A
            case 1876896 : // PSX JAP
            case 1630560 : version = VER_TR1_PSX;
            case 547782  : // SAT
            case 2942002 : // PC JAP
            case 2934408 :
            case 2934730 : return LVL_TR1_3A;
            // LEVEL3B
            case 1510414 : // PSX JAP
            case 1506614 : version = VER_TR1_PSX;
            case 310960  : // SAT
            case 2745530 : // PC JAP
            case 2737936 :
            case 2738258 : return LVL_TR1_3B;
            // CUT1
            case 722402  : version = VER_TR1_PSX;
            case 142116  : // SAT
            case 599840  : return LVL_TR1_CUT_1;
            // LEVEL4
            case 1624130 : // PSX JAP
            case 1621970 : version = VER_TR1_PSX;
            case 440612  : // SAT
            case 3038144 : // PC JAP
            case 3030550 :
            case 3030872 : return LVL_TR1_4;
            // LEVEL5
            case 1588102 : // PSX JAP
            case 1585942 : version = VER_TR1_PSX;
            case 389996  : // SAT
            case 2725812 : // PC JAP
            case 2718218 :
            case 2718540 : return LVL_TR1_5;
            // LEVEL6
            case 1710624 : // PSX JAP
            case 1708464 : version = VER_TR1_PSX;
            case 573506  : // SAT
            case 3147184 : // PC JAP
            case 3139590 :
            case 3074376 : return LVL_TR1_6;
            // LEVEL7A
            case 1698824 : // PSX JAP
            case 1696664 : version = VER_TR1_PSX;
            case 581416  : // SAT
            case 2824884 : // PC JAP
            case 2817290 :
            case 2817612 : return LVL_TR1_7A;
            // LEVEL7B
            case 1735434 : // PSX JAP
            case 1733274 : version = VER_TR1_PSX;
            case 596416  : // SAT
            case 3603912 : // PC JAP
            case 3388774 :
            case 3395618 : // PC G
            case 3389096 : return LVL_TR1_7B;
            // CUT2
            case 542960  : version = VER_TR1_PSX;
            case 70860   : // SAT
            case 354320  : return LVL_TR1_CUT_2;
            // LEVEL8A
            case 1565494 : // PSX JAP
            case 1563356 : version = VER_TR1_PSX;
            case 592188  : // SAT
            case 2887836 : // PC JAP
            case 2880242 :
            case 2880564 : return LVL_TR1_8A;
            // LEVEL8B
            case 1567790 : // PSX JAP
            case 1565630 : version = VER_TR1_PSX;
            case 599928  : // SAT
            case 2894028 : // PC JAP
            case 2886434 :
            case 2886756 : return LVL_TR1_8B;
            // LEVEL8C
            case 1621520 : // PSX JAP
            case 1619360 : version = VER_TR1_PSX;
            case 536950  : // SAT
            case 3072066 : // PC JAP
            case 3105128 :
            case 3025380 : // PC G
            case 3105450 : return LVL_TR1_8C;
            // LEVEL10A
            case 1680146 : // PSX JAP
            case 1678018 : version = VER_TR1_PSX;
            case 569856  : // SAT
            case 3270372 : // PC JAP
            case 3223816 :
            case 3154346 : // PC G
            case 3224138 : return LVL_TR1_10A;
            // CUT3
            case 636660  : version = VER_TR1_PSX;
            case 210134  : // SAT
            case 512104  : return LVL_TR1_CUT_3;
            // LEVEL10B
            case 1688908 : // PSX JAP
            case 1686748 : version = VER_TR1_PSX;
            case 525646  : // SAT
            case 3101614 : // PC JAP
            case 3094342 :
            case 3094020 : return LVL_TR1_10B;
            // CUT4
            case 940398  : version = VER_TR1_PSX;
            case 167188  : // SAT
            case 879582  : return LVL_TR1_CUT_4;
            // LEVEL10C
            case 1816438 : // PSX JAP
            case 1814278 : version = VER_TR1_PSX;
            case 418170  : // SAT
            case 3533814 : // PC JAP
            case 3531702 :
            case 3496692 : // PC G
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
        // TR4
            case 3007155 : return LVL_TR4_TITLE;
            case 4034313 : return LVL_TR4_ANGKOR1;
            case 4343019 : return LVL_TR4_ANG_RACE;
            case 3715110 : return LVL_TR4_SETTOMB1;
            case 3868566 : return LVL_TR4_SETTOMB2;
            case 3600478 : return LVL_TR4_JEEPCHAS;
            case 4826055 : return LVL_TR4_JEEPCHS2;
            case 4773596 : return LVL_TR4_KARNAK1;
            case 4882065 : return LVL_TR4_HALL;
            case 5021843 : return LVL_TR4_LAKE;
            case 4409367 : return LVL_TR4_SEMER;
            case 4294398 : return LVL_TR4_SEMER2;
            case 3246177 : return LVL_TR4_TRAIN;
            case 4007946 : return LVL_TR4_ALEXHUB;
            case 4735043 : return LVL_TR4_ALEXHUB2;
            case 4549992 : return LVL_TR4_PALACES;
            case 4779709 : return LVL_TR4_PALACES2;
            case 4570232 : return LVL_TR4_CSPLIT1;
            case 4838007 : return LVL_TR4_CSPLIT2;
            case 4606099 : return LVL_TR4_LIBRARY;
            case 3240517 : return LVL_TR4_LIBEND;
            case 5013974 : return LVL_TR4_BIKEBIT;
            case 4260336 : return LVL_TR4_NUTRENCH;
            case 4989001 : return LVL_TR4_CORTYARD;
            case 3970465 : return LVL_TR4_LOWSTRT;
            case 4725022 : return LVL_TR4_HIGHSTRT;
            case 4776907 : return LVL_TR4_CITNEW;
            case 5011064 : return LVL_TR4_JOBY1A;
            case 4544163 : return LVL_TR4_JOBY1B;
            case 4839409 : return LVL_TR4_JOBY2;
            case 4433722 : return LVL_TR4_JOBY3A;
            case 5141026 : return LVL_TR4_JOBY3B;
            case 4786641 : return LVL_TR4_JOBY4A;
            case 4401690 : return LVL_TR4_JOBY4B;
            case 4999677 : return LVL_TR4_JOBY4C;
            case 3741579 : return LVL_TR4_JOBY5A;
            case 4623726 : return LVL_TR4_JOBY5B;
            case 4398142 : return LVL_TR4_JOBY5C;
        }

        if (name) {
            // skip directory path
            int start = 0;
            for (int i = int(strlen(name)) - 1; i >= 0; i--)
                if (name[i] == '/' || name[i] == '\\') {
                    start = i + 1;
                    break;
                }
            // skip file extension
            char buf[255];
            strcpy(buf, name + start);
            char *ext = NULL;
            for (int i = 0; i < int(strlen(buf)); i++)
                if (buf[i] == '.') {
                    buf[i] = 0;
                    ext = buf + i + 1;
                    break;
                }
            // compare with standard levels
            // TODO: fix TITLE (2-3), HOUSE (3), CUTx (2-3)
            for (int i = 0; i < LVL_MAX; i++)
                if (!strcmp(buf, LEVEL_INFO[i].name)) {
                    LevelID id = LevelID(i);
                    if (ext) {
                        version = getGameVersionByLevel(id);
                        if (!strcmp("PSX", ext)) {
                            version = Version(version | VER_PSX);
                        } else if (!strcmp("SAT", ext)) {
                            version = Version(version | VER_SAT);
                        } else {
                            version = Version(version | VER_PC);
                        }
                    }
                    return id;
                }
        }

        return LVL_CUSTOM;
    }

    LevelID getTitleId(Version version) {
        switch (version & VER_VERSION) {
            case VER_TR1 : return LVL_TR1_TITLE;
            case VER_TR2 : return LVL_TR2_TITLE;
            case VER_TR3 : return LVL_TR3_TITLE;
            case VER_TR4 : return LVL_TR4_TITLE;
        }
        return LVL_TR1_TITLE;
        ASSERT(false);
    }

    LevelID getHomeId(Version version) {
        switch (version & VER_VERSION) {
            case VER_TR1 : return LVL_TR1_GYM;
            case VER_TR2 : return LVL_TR2_ASSAULT;
            case VER_TR3 : return LVL_TR3_HOUSE;
            case VER_TR4 : return LVL_MAX;
        }
        ASSERT(false);
        return LVL_MAX;
    }

    LevelID getStartId(Version version) {
        switch (version & VER_VERSION) {
            case VER_TR1 : return LVL_TR1_1;
            case VER_TR2 : return LVL_TR2_WALL;
            case VER_TR3 : return LVL_TR3_JUNGLE;
            case VER_TR4 : return LVL_TR4_ANGKOR1;
        }
        ASSERT(false);
        return LVL_MAX;
    }

    LevelID getEndId(Version version) {
        switch (version & VER_VERSION) {
            case VER_TR1 : return LVL_TR1_10C;
            case VER_TR2 : return LVL_TR2_HOUSE;
            case VER_TR3 : return LVL_TR3_CHAMBER;
            case VER_TR4 : return LVL_TR4_JOBY5C;
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
        return id == LVL_TR1_TITLE ||
               id == LVL_TR2_TITLE ||
               id == LVL_TR3_TITLE ||
               id == LVL_TR4_TITLE;
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

        if (Stream::existsContent("data/angkor1.tr4"))
            return VER_TR4_PC;

        useEasyStart = false;
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
                case VER_TR2_PC  : {
                    sprintf(dst, "DATA/%s.TR2", LEVEL_INFO[id].name);
                    if (Stream::existsContent(dst)) break;
                    strcpy(dst, LEVEL_INFO[id].name);
                    StrUtils::toLower(dst);
                    strcat(dst, ".TR2");
                    break;
                }
                case VER_TR2_PSX : sprintf(dst, "DATA/%s.PSX", LEVEL_INFO[id].name); break;
                case VER_TR3_PC  : sprintf(dst, isCutsceneLevel(id) ? "cuts/%s.TR2" : "data/%s.TR2", LEVEL_INFO[id].name); break;
                case VER_TR3_PSX : sprintf(dst, isCutsceneLevel(id) ? "CUTS/%s.PSX" : "DATA/%s.PSX", LEVEL_INFO[id].name); break;
                case VER_TR4_PC  : sprintf(dst, "DATA/%s.tr4", LEVEL_INFO[id].name); break;
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
                    case VER_TR1_SAT : strcat(dst, ".SAT"); break;
                    case VER_UNKNOWN :
                        if (Stream::existsContent("level/1/TITLE.PSX")) {
                            strcpy(dst, "level/1/TITLE.PSX");
                            return;
                        }
                        if (Stream::existsContent("level/1/TITLE.PHD")) {
                            strcpy(dst, "level/1/TITLE.PHD");
                            return;
                        }
                        if (Stream::existsContent("level/1/TITLE.SAT")) {
                            strcpy(dst, "level/1/TITLE.SAT");
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
        static const uint8 TR1_TRACK_MAPPING[] = {
            2, 2, 2, 11, 12, 3, 13, 14, 15, 16, 17, 18, 19, 60, 20, 21, 22, 23, 24, 25, 26, 27,
            7, 8, 9, 10, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
            47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 28, 4, 5, 6
        };

        static const uint8 TR2_TRACK_MAPPING[] = {
            2, 2, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 18, 18, 19, 20,
            21, 22, 23, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61
        };

        if (version & VER_TR1) {
            ASSERT(track < COUNT(TR1_TRACK_MAPPING));
            return TR1_TRACK_MAPPING[track];
        }

        if (version & VER_TR2) {
            ASSERT(track < COUNT(TR2_TRACK_MAPPING));
            return TR2_TRACK_MAPPING[track];
        }

        return track;
    }

    bool checkTrack(const char *pre, char *name) {
        static const char *fmt[] = { ".ogg", ".mp3", ".wav" };
        const char *lng[] = { "", "", LANG_PREFIXES };

        int start = 1;
        if (Core::settings.audio.language != 0) {
            start = 0;
            lng[start] = lng[Core::settings.audio.language + 2];
        }

        char buf[64];
        for (int f = 0; f < COUNT(fmt); f++)
            for (int l = start; l < COUNT(lng); l++) {
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

    StringID getVideoSubs(LevelID id) {
        switch (id) {
        // TR1
            case LVL_TR1_TITLE : return isGameEnded ? STR_EMPTY : STR_TR1_SUB_CAFE;
            case LVL_TR1_4     : return STR_TR1_SUB_LIFT;
            case LVL_TR1_10A   : return STR_TR1_SUB_CANYON;
            case LVL_TR1_CUT_4 : return STR_TR1_SUB_PRISON;
            default            : return STR_EMPTY;
        }
    }

    bool getVideoTrack(LevelID id, Stream::Callback callback, void *userData) {
        char title[32];

        const char *str = NULL;
        switch (id) {
        // TR1
            case LVL_TR1_TITLE : str = isGameEnded ? NULL : "CAFE"; break;
            case LVL_TR1_4     : str = "LIFT";   break;
            case LVL_TR1_10A   : str = "CANYON"; break;
            case LVL_TR1_CUT_4 : str = "PRISON"; break;
        // TR2 TODO
        // TR3 TODO
            default            : return false;
        }

        sprintf(title, "track_%s", str);
        if (!checkTrack("", title) && !checkTrack("audio/1/", title) && !checkTrack("audio/", title)) {
            return false;
        }

        new Stream(title, callback, userData);
        return true;
    }

    StringID getSubs(Version version, int track) {
        if ((version & VER_TR1) && (track >= 22 && track <= 56 && track != 24)) {
            return StringID(STR_TR1_SUB_22 + (track - 22));
        }
        // TR2, TR3 TODO
        return STR_EMPTY;
    }

    bool checkWebDub(Version version, int track) {
        if (getSubs(version, track) != STR_EMPTY) {
            int lang = Core::settings.audio.language + STR_LANG_EN;
            return lang == STR_LANG_EN || lang == STR_LANG_DE || lang == STR_LANG_FR || lang == STR_LANG_RU || lang == STR_LANG_JA;
        }
        return false;
    }

    void getGameTrack(Version version, int track, Stream::Callback callback, void *userData) {
        char title[64];
        if (useEasyStart) {
            switch (version) {
                case VER_TR1_SAT :
                case VER_TR1_PC  :
                case VER_TR1_PSX :
                    sprintf(title, "track_%02d", track);
                    if (!checkTrack("", title) && !checkTrack("audio/1/", title) && !checkTrack("audio/", title)) {
                        track = remapTrack(version, track);
                        sprintf(title, "%03d", track);
                        if (!checkTrack("", title) && !checkTrack("audio/1/", title) && !checkTrack("audio/", title)) {
                            callback(NULL, userData);
                            return;
                        }
                    }
                    break;
                case VER_TR2_PC  :
                case VER_TR2_PSX :
                    //if (Stream::existsContent("audio/cdaudio.mp3")) {
                    //    callback(Sound::openCDAudioMP3("audio/cdaudio.dat", "audio/cdaudio.mp3", track), userData);
                    //    return;
                    //}
                    track = remapTrack(version, track);
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
                case VER_TR4_PC  :
                    strcpy(title, TRACK_LIST_TR4[track]);
                    if (!checkTrack("audio/", title)) {
                        callback(NULL, userData);
                    }
                    break;
                default : return;
            }
        } else {
            switch (version) {
                case VER_TR1_SAT :
                case VER_TR1_PC  :
                case VER_TR1_PSX : {
                    if (TR::checkWebDub(version, track)) {
                        const char *lng[] = { LANG_PREFIXES };
                        sprintf(title, "audio/1/track_%02d%s.ogg", track, lng[Core::settings.audio.language]);
                    } else {
                        if (TR::getSubs(version, track) != STR_EMPTY) {
                            sprintf(title, "audio/1/track_%02d_EN.ogg", track);
                        } else {
                            sprintf(title, "audio/1/track_%02d.ogg", track);
                        }
                    }
                #ifndef _OS_WEB
                    if (Stream::existsContent(title))
                        break;
                    track = remapTrack(version, track);
                    sprintf(title, "audio/1/%03d.ogg", track);
                #endif
                    break;
                }
                case VER_TR2_PC  :
                case VER_TR2_PSX :
                    track = remapTrack(version, track);
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
                CHECK_FILE("BINDATA/TITLE1.BIN");   // SEGA
                return "level/1/AMERTIT.PNG";       // WEB
            case LVL_TR1_GYM :
                CHECK_FILE("DELDATA/GYMLOAD.RAW");
                CHECK_FILE("BINDATA/GYM224.BIN");
                CHECK_FILE("BINDATA/GYM.BIN");
                return "level/1/GYMLOAD.PNG";
            case LVL_TR1_1  :
            case LVL_TR1_2  :
            case LVL_TR1_3A :
            case LVL_TR1_3B :
                CHECK_FILE("DELDATA/AZTECLOA.RAW");
                CHECK_FILE("BINDATA/AZTEC224.BIN");
                CHECK_FILE("BINDATA/AZTEC.BIN");
                return "level/1/AZTECLOA.PNG";
            case LVL_TR1_4  :
            case LVL_TR1_5  :
            case LVL_TR1_6  :
            case LVL_TR1_7A :
            case LVL_TR1_7B :
                CHECK_FILE("DELDATA/GREEKLOA.RAW");
                CHECK_FILE("BINDATA/GREEK224.BIN");
                CHECK_FILE("BINDATA/GREEK.BIN");
                return "level/1/GREEKLOA.PNG";
            case LVL_TR1_8A :
            case LVL_TR1_8B :
            case LVL_TR1_8C :
                CHECK_FILE("DELDATA/EGYPTLOA.RAW");
                CHECK_FILE("BINDATA/EGYPT224.BIN");
                CHECK_FILE("BINDATA/EGYPT.BIN");
                return "level/1/EGYPTLOA.PNG";
            case LVL_TR1_10A :
            case LVL_TR1_10B :
            case LVL_TR1_10C :
                CHECK_FILE("DELDATA/ATLANLOA.RAW");
                CHECK_FILE("BINDATA/ATLAN224.BIN");
                CHECK_FILE("BINDATA/ATLAN.BIN");
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
            CHECK_FILE("FMV/CORELOGO.CPK");
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
            case LVL_TR1_TITLE :
                if (isGameEnded) {
                    CHECK_FILE("FMV/END.FMV");
                    CHECK_FILE("FMV/END.RPL");
                    CHECK_FILE("FMV/END.CPK");
                    CHECK_FILE("video/1/END.FMV");
                    CHECK_FILE("video/1/END.RPL");
                    CHECK_FILE("video/1/END.CPK");
                } else {
                    CHECK_FILE("FMV/CAFE.FMV");
                    CHECK_FILE("FMV/CAFE.RPL");
                    CHECK_FILE("FMV/CAFE.CPK");
                    CHECK_FILE("video/1/CAFE.FMV");
                    CHECK_FILE("video/1/CAFE.RPL");
                    CHECK_FILE("video/1/CAFE.CPK");
                }
                break;
            case LVL_TR1_GYM :
                CHECK_FILE("FMV/MANSION.FMV");
                CHECK_FILE("FMV/MANSION.RPL");
                CHECK_FILE("FMV/MANSION.CPK");
                CHECK_FILE("video/1/MANSION.FMV");
                CHECK_FILE("video/1/MANSION.RPL");
                CHECK_FILE("video/1/MANSION.CPK");
                break;
            case LVL_TR1_1 :
                CHECK_FILE("FMV/SNOW.FMV");
                CHECK_FILE("FMV/SNOW.RPL");
                CHECK_FILE("FMV/SNOW.CPK");
                CHECK_FILE("video/1/SNOW.FMV");
                CHECK_FILE("video/1/SNOW.RPL");
                CHECK_FILE("video/1/SNOW.CPK");
                break;
            case LVL_TR1_4 :
                CHECK_FILE("FMV/LIFT.FMV");
                CHECK_FILE("FMV/LIFT.RPL");
                CHECK_FILE("FMV/LIFT.CPK");
                CHECK_FILE("video/1/LIFT.FMV");
                CHECK_FILE("video/1/LIFT.RPL");
                CHECK_FILE("video/1/LIFT.CPK");
                break;
            case LVL_TR1_8A :
                CHECK_FILE("FMV/VISION.FMV");
                CHECK_FILE("FMV/VISION.RPL");
                CHECK_FILE("FMV/VISION.CPK");
                CHECK_FILE("video/1/VISION.FMV");
                CHECK_FILE("video/1/VISION.RPL");
                CHECK_FILE("video/1/VISION.CPK");
                break;
            case LVL_TR1_10A :
                CHECK_FILE("FMV/CANYON.FMV");
                CHECK_FILE("FMV/CANYON.RPL");
                CHECK_FILE("FMV/CANYON.CPK");
                CHECK_FILE("video/1/CANYON.FMV");
                CHECK_FILE("video/1/CANYON.RPL");
                CHECK_FILE("video/1/CANYON.CPK");
                break;
            case LVL_TR1_10B :
                CHECK_FILE("FMV/PYRAMID.FMV");
                CHECK_FILE("FMV/PYRAMID.RPL");
                CHECK_FILE("FMV/PYRAMID.CPK");
                CHECK_FILE("video/1/PYRAMID.FMV");
                CHECK_FILE("video/1/PYRAMID.RPL");
                CHECK_FILE("video/1/PYRAMID.CPK");
                break;
            case LVL_TR1_CUT_4 :
                CHECK_FILE("FMV/PRISON.FMV");
                CHECK_FILE("FMV/PRISON.RPL");
                CHECK_FILE("FMV/PRISON.CPK");
                CHECK_FILE("video/1/PRISON.FMV");
                CHECK_FILE("video/1/PRISON.RPL");
                CHECK_FILE("video/1/PRISON.CPK");
                break;
            case LVL_TR1_EGYPT :
                CHECK_FILE("FMV/END.FMV");
                CHECK_FILE("FMV/END.RPL");
                CHECK_FILE("FMV/END.CPK");
                CHECK_FILE("video/1/END.FMV");
                CHECK_FILE("video/1/END.RPL");
                CHECK_FILE("video/1/END.CPK");
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
                if (isGameEnded) {
                    CHECK_FILE("FMV/END.FMV");
                    CHECK_FILE("fmv/Endgame.rpl");
                    CHECK_FILE("video/3/END.FMV");
                    CHECK_FILE("video/3/Endgame.rpl");
                } else {
                    CHECK_FILE("FMV/INTRO.FMV");
                    CHECK_FILE("fmv/Intr_Eng.rpl");
                    CHECK_FILE("video/3/INTRO.FMV");
                    CHECK_FILE("video/3/Intr_Eng.rpl");
                }
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
    #define FOG_NONE    vec4(0.0f,  0.0f, 0.0f,  0.0f)
    #define FOG_BLACK   vec4(0.0f,  0.0f, 0.0f,  FOG_DIST)
    #define FOG_SANDY   vec4(0.2f,  0.1f, 0.0f,  FOG_DIST)
    #define FOG_GREEN   vec4(0.0f,  0.1f, 0.0f,  FOG_DIST)
    #define FOG_RED     vec4(0.2f,  0.0f, 0.0f,  FOG_DIST)
    #define FOG_MIST    vec4(0.25f, 0.2f, 0.15f, FOG_DIST)

    vec4 getFogParams(LevelID id) {
        switch (id) {
            case LVL_TR1_1     :
            case LVL_TR1_2     : return FOG_BLACK;
            case LVL_TR1_3A    : return FOG_MIST;
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

    struct SkyParams {
        vec4 skyDownColor;
        vec4 skyUpColor;
        vec4 sunDirSize;
        vec4 sunColorGlare;
        vec4 cloudDownColor;
        vec4 cloudUpColor;
        vec3 wind;
    };

    #define CLOUD_UP    vec3()
    #define CLOUD_DOWN  vec3()

    bool getSkyParams(LevelID id, SkyParams &params) {
        switch (id) {
            case LVL_TR1_3A :
                params.skyDownColor    = vec4(0.8f, 0.8f, 0.7f, 1.0f);
                params.skyUpColor      = vec4(0.3f, 0.4f, 0.5f, 1.0f);
                params.sunDirSize      = vec4(vec3(1.0f, 0.75f, -1.0f).normal(), 0.0025f);
                params.sunColorGlare   = vec4(0.8f, 0.4f, 0.1f, 4.0f);
                params.cloudDownColor  = vec4(0.35f, 0.4f, 0.45f, 1.0f);
                params.cloudUpColor    = vec4(1.1f, 1.045f, 0.88f, 1.0f);
                params.wind            = vec3(0.01f, -0.005f, 0.005f);
                break;
            case LVL_TR1_5 :
                params.skyDownColor    = vec4(0.15f, 0.05f, 0.0f, 1.0f);
                params.skyUpColor      = vec4(0.3f, 0.2f, 0.1f, 1.0f);
                params.sunDirSize      = vec4(vec3(-1.0f, 0.8f, -1.0f).normal(), 0.0015f);
                params.sunColorGlare   = vec4(0.7f, 0.7f, 0.6f, 256.0f);
                params.cloudDownColor  = vec4(0.2f, 0.1f, 0.0f, 1.0f);
                params.cloudUpColor    = vec4(0.5f, 0.5f, 0.4f, 1.0f);
                params.wind            = vec3(0.01f, -0.005f, 0.005f);
                break;
            case LVL_TR3_HOUSE : // test
                params.skyDownColor    = vec4(0.8f, 0.8f, 0.7f, 1.0f);
                params.skyUpColor      = vec4(0.3f, 0.4f, 0.5f, 1.0f);
                params.sunDirSize      = vec4(vec3(1.0f, 0.75f, -1.0f).normal(), 0.0025f);
                params.sunColorGlare   = vec4(0.8f, 0.4f, 0.1f, 4.0f);
                params.cloudDownColor  = vec4(0.35f, 0.4f, 0.45f, 1.0f);
                params.cloudUpColor    = vec4(1.1f, 1.045f, 0.88f, 1.0f);
                params.wind            = vec3(0.01f, -0.005f, 0.005f);
                break;
            default : return false;
        }
        return true;
    }
}

#undef CHECK_FILE

#endif

#ifndef H_FORMAT
#define H_FORMAT

#include "utils.h"
#include "gameflow.h"
#include "savegame.h"

#define MAX_RESERVED_ENTITIES 128
#define MAX_TRIGGER_COMMANDS  32
#define MAX_MESHES            512
#define MAX_WEAPONS           4

// Lara's height in units / height in meters
#define ONE_METER             (768.0f / 1.8f)

#define TR1_TYPES_START       0
#define TR2_TYPES_START       1000
#define TR3_TYPES_START       2000

#define TR_TYPES(E) \
    E( LARA                  = TR1_TYPES_START) \
    E( LARA_PISTOLS          ) \
    E( LARA_SHOTGUN          ) \
    E( LARA_MAGNUMS          ) \
    E( LARA_UZIS             ) \
    E( LARA_SPEC             ) \
    E( ENEMY_DOPPELGANGER    ) \
    E( ENEMY_WOLF            ) \
    E( ENEMY_BEAR            ) \
    E( ENEMY_BAT             ) \
    E( ENEMY_CROCODILE_LAND  ) \
    E( ENEMY_CROCODILE_WATER ) \
    E( ENEMY_LION_MALE       ) \
    E( ENEMY_LION_FEMALE     ) \
    E( ENEMY_PUMA            ) \
    E( ENEMY_GORILLA         ) \
    E( ENEMY_RAT_LAND        ) \
    E( ENEMY_RAT_WATER       ) \
    E( ENEMY_REX             ) \
    E( ENEMY_RAPTOR          ) \
    E( ENEMY_MUTANT_1        ) \
    E( ENEMY_MUTANT_2        ) \
    E( ENEMY_MUTANT_3        ) \
    E( ENEMY_CENTAUR         ) \
    E( ENEMY_MUMMY           ) \
    E( UNUSED_1              ) \
    E( UNUSED_2              ) \
    E( ENEMY_LARSON          ) \
    E( ENEMY_PIERRE          ) \
    E( ENEMY_SKATEBOARD      ) \
    E( ENEMY_SKATEBOY        ) \
    E( ENEMY_COWBOY          ) \
    E( ENEMY_MR_T            ) \
    E( ENEMY_NATLA           ) \
    E( ENEMY_GIANT_MUTANT    ) \
    E( TRAP_FLOOR            ) \
    E( TRAP_SWING_BLADE      ) \
    E( TRAP_SPIKES           ) \
    E( TRAP_BOULDER          ) \
    E( DART                  ) \
    E( TRAP_DART_EMITTER     ) \
    E( DRAWBRIDGE            ) \
    E( TRAP_SLAM             ) \
    E( TRAP_SWORD            ) \
    E( HAMMER_HANDLE         ) \
    E( HAMMER_BLOCK          ) \
    E( LIGHTNING             ) \
    E( MOVING_OBJECT         ) \
    E( BLOCK_1               ) \
    E( BLOCK_2               ) \
    E( BLOCK_3               ) \
    E( BLOCK_4               ) \
    E( MOVING_BLOCK          ) \
    E( TRAP_CEILING_1        ) \
    E( TRAP_CEILING_2        ) \
    E( SWITCH                ) \
    E( SWITCH_WATER          ) \
    E( DOOR_1                ) \
    E( DOOR_2                ) \
    E( DOOR_3                ) \
    E( DOOR_4                ) \
    E( DOOR_5                ) \
    E( DOOR_6                ) \
    E( DOOR_7                ) \
    E( DOOR_8                ) \
    E( TRAP_DOOR_1           ) \
    E( TRAP_DOOR_2           ) \
    E( UNUSED_3              ) \
    E( BRIDGE_1              ) \
    E( BRIDGE_2              ) \
    E( BRIDGE_3              ) \
    E( INV_PASSPORT          ) \
    E( INV_COMPASS           ) \
    E( INV_HOME              ) \
    E( GEARS_1               ) \
    E( GEARS_2               ) \
    E( GEARS_3               ) \
    E( CUT_1                 ) \
    E( CUT_2                 ) \
    E( CUT_3                 ) \
    E( CUT_4                 ) \
    E( INV_PASSPORT_CLOSED   ) \
    E( INV_MAP               ) \
    E( CRYSTAL               ) \
    E( PISTOLS               ) \
    E( SHOTGUN               ) \
    E( MAGNUMS               ) \
    E( UZIS                  ) \
    E( AMMO_PISTOLS          ) \
    E( AMMO_SHOTGUN          ) \
    E( AMMO_MAGNUMS          ) \
    E( AMMO_UZIS             ) \
    E( EXPLOSIVE             ) \
    E( MEDIKIT_SMALL         ) \
    E( MEDIKIT_BIG           ) \
    E( INV_DETAIL            ) \
    E( INV_SOUND             ) \
    E( INV_CONTROLS          ) \
    E( INV_GAMMA             ) \
    E( INV_PISTOLS           ) \
    E( INV_SHOTGUN           ) \
    E( INV_MAGNUMS           ) \
    E( INV_UZIS              ) \
    E( INV_AMMO_PISTOLS      ) \
    E( INV_AMMO_SHOTGUN      ) \
    E( INV_AMMO_MAGNUMS      ) \
    E( INV_AMMO_UZIS         ) \
    E( INV_EXPLOSIVE         ) \
    E( INV_MEDIKIT_SMALL     ) \
    E( INV_MEDIKIT_BIG       ) \
    E( PUZZLE_1              ) \
    E( PUZZLE_2              ) \
    E( PUZZLE_3              ) \
    E( PUZZLE_4              ) \
    E( INV_PUZZLE_1          ) \
    E( INV_PUZZLE_2          ) \
    E( INV_PUZZLE_3          ) \
    E( INV_PUZZLE_4          ) \
    E( PUZZLE_HOLE_1         ) \
    E( PUZZLE_HOLE_2         ) \
    E( PUZZLE_HOLE_3         ) \
    E( PUZZLE_HOLE_4         ) \
    E( PUZZLE_DONE_1         ) \
    E( PUZZLE_DONE_2         ) \
    E( PUZZLE_DONE_3         ) \
    E( PUZZLE_DONE_4         ) \
    E( LEADBAR               ) \
    E( INV_LEADBAR           ) \
    E( MIDAS_HAND            ) \
    E( KEY_ITEM_1            ) \
    E( KEY_ITEM_2            ) \
    E( KEY_ITEM_3            ) \
    E( KEY_ITEM_4            ) \
    E( INV_KEY_ITEM_1        ) \
    E( INV_KEY_ITEM_2        ) \
    E( INV_KEY_ITEM_3        ) \
    E( INV_KEY_ITEM_4        ) \
    E( KEY_HOLE_1            ) \
    E( KEY_HOLE_2            ) \
    E( KEY_HOLE_3            ) \
    E( KEY_HOLE_4            ) \
    E( UNUSED_4              ) \
    E( UNUSED_5              ) \
    E( SCION_PICKUP_QUALOPEC ) \
    E( SCION_PICKUP_DROP     ) \
    E( SCION_TARGET          ) \
    E( SCION_PICKUP_HOLDER   ) \
    E( SCION_HOLDER          ) \
    E( UNUSED_6              ) \
    E( UNUSED_7              ) \
    E( INV_SCION             ) \
    E( EXPLOSION             ) \
    E( UNUSED_8              ) \
    E( WATER_SPLASH          ) \
    E( UNUSED_9              ) \
    E( BUBBLE                ) \
    E( UNUSED_10             ) \
    E( UNUSED_11             ) \
    E( BLOOD                 ) \
    E( UNUSED_12             ) \
    E( SMOKE                 ) \
    E( CENTAUR_STATUE        ) \
    E( CABIN                 ) \
    E( MUTANT_EGG_SMALL      ) \
    E( RICOCHET              ) \
    E( SPARKLES              ) \
    E( MUZZLE_FLASH          ) \
    E( UNUSED_13             ) \
    E( UNUSED_14             ) \
    E( VIEW_TARGET           ) \
    E( WATERFALL             ) \
    E( NATLA_BULLET          ) \
    E( MUTANT_BULLET         ) \
    E( CENTAUR_BULLET        ) \
    E( UNUSED_16             ) \
    E( UNUSED_17             ) \
    E( LAVA_PARTICLE         ) \
    E( TRAP_LAVA_EMITTER     ) \
    E( FLAME                 ) \
    E( TRAP_FLAME_EMITTER    ) \
    E( TRAP_LAVA             ) \
    E( MUTANT_EGG_BIG        ) \
    E( BOAT                  ) \
    E( EARTHQUAKE            ) \
    E( UNUSED_18             ) \
    E( UNUSED_19             ) \
    E( UNUSED_20             ) \
    E( UNUSED_21             ) \
    E( UNUSED_22             ) \
    E( LARA_BRAID            ) \
    E( GLYPHS                ) \
    E( TR1_TYPE_MAX          ) \
    E( _LARA                 = TR2_TYPES_START ) \
    E( _LARA_PISTOLS         ) \
    E( _LARA_BRAID           ) \
    E( _LARA_SHOTGUN         ) \
    E( LARA_AUTOPISTOLS      ) \
    E( _LARA_UZIS            ) \
    E( LARA_M16              ) \
    E( LARA_GRENADE          ) \
    E( LARA_HARPOON          ) \
    E( LARA_FLARE            ) \
    E( LARA_SNOWMOBILE       ) \
    E( LARA_BOAT             ) \
    E( _LARA_SPEC            ) \
    E( VEHICLE_SNOWMOBILE_RED ) \
    E( VEHICLE_BOAT          ) \
    E( ENEMY_DOG             ) \
    E( ENEMY_GOON_MASK_1     ) \
    E( ENEMY_GOON_MASK_2     ) \
    E( ENEMY_GOON_MASK_3     ) \
    E( ENEMY_GOON_KNIFE      ) \
    E( ENEMY_GOON_SHOTGUN    ) \
    E( ENEMY_RAT             ) \
    E( ENEMY_DRAGON_FRONT    ) \
    E( ENEMY_DRAGON_BACK     ) \
    E( GONDOLA               ) \
    E( ENEMY_SHARK           ) \
    E( ENEMY_MORAY_1         ) \
    E( ENEMY_MORAY_2         ) \
    E( ENEMY_BARACUDA        ) \
    E( ENEMY_DIVER           ) \
    E( ENEMY_GUNMAN_1        ) \
    E( ENEMY_GUNMAN_2        ) \
    E( ENEMY_GOON_STICK_1    ) \
    E( ENEMY_GOON_STICK_2    ) \
    E( ENEMY_GOON_FLAME      ) \
    E( UNUSED_23             ) \
    E( ENEMY_SPIDER          ) \
    E( ENEMY_SPIDER_GIANT    ) \
    E( ENEMY_CROW            ) \
    E( ENEMY_TIGER           ) \
    E( ENEMY_MARCO           ) \
    E( ENEMY_GUARD_SPEAR     ) \
    E( ENEMY_GUARD_SPEAR_STATUE )\
    E( ENEMY_GUARD_SWORD     ) \
    E( ENEMY_GUARD_SWORD_STATUE ) \
    E( ENEMY_YETI            ) \
    E( ENEMY_BIRD_MONSTER    ) \
    E( ENEMY_EAGLE           ) \
    E( ENEMY_MERCENARY_1     ) \
    E( ENEMY_MERCENARY_2     ) \
    E( ENEMY_MERCENARY_3     ) \
    E( VEHICLE_SNOWMOBILE_BLACK ) \
    E( ENEMY_MERCENARY_SNOWMOBILE ) \
    E( ENEMY_MONK_1          ) \
    E( ENEMY_MONK_2          ) \
    E( _TRAP_FLOOR           ) \
    E( UNUSED_24             ) \
    E( TRAP_BOARDS           ) \
    E( TRAP_SWING_BAG        ) \
    E( _TRAP_SPIKES          ) \
    E( _TRAP_BOULDER         ) \
    E( _DART                 ) \
    E( _TRAP_DART_EMITTER    ) \
    E( _DRAWBRIDGE           ) \
    E( _TRAP_SLAM            ) \
    E( ELEVATOR              ) \
    E( MINISUB               ) \
    E( _BLOCK_1              ) \
    E( _BLOCK_2              ) \
    E( _BLOCK_3              ) \
    E( _BLOCK_4              ) \
    E( LAVA_BOWL             ) \
    E( WINDOW_1              ) \
    E( WINDOW_2              ) \
    E( UNUSED_25             ) \
    E( UNUSED_26             ) \
    E( PROPELLER_PLANE       ) \
    E( SAW                   ) \
    E( HOOK                  ) \
    E( _TRAP_CEILING_1       ) \
    E( TRAP_SPINDLE          ) \
    E( TRAP_BLADE_WALL       ) \
    E( TRAP_BLADE_STATUE     ) \
    E( TRAP_BOULDERS         ) \
    E( TRAP_ICICLES          ) \
    E( TRAP_SPIKES_WALL      ) \
    E( JUMPPAD               ) \
    E( TRAP_SPIKES_CEILING   ) \
    E( BELL                  ) \
    E( WAKE_BOAT             ) \
    E( WAKE_SNOWMOBILE       ) \
    E( SNOWMOBILE_BELT       ) \
    E( WHEEL_KNOB            ) \
    E( SWITCH_BIG            ) \
    E( PROPELLER_WATER       ) \
    E( PROPELLER_AIR         ) \
    E( TRAP_SWING_BOX        ) \
    E( _CUT_1                ) \
    E( _CUT_2                ) \
    E( _CUT_3                ) \
    E( UI_FRAME              ) \
    E( ROLLING_DRUMS         ) \
    E( ZIPLINE_HANDLE        ) \
    E( SWITCH_BUTTON         ) \
    E( _SWITCH               ) \
    E( _SWITCH_WATER         ) \
    E( _DOOR_1               ) \
    E( _DOOR_2               ) \
    E( _DOOR_3               ) \
    E( _DOOR_4               ) \
    E( _DOOR_5               ) \
    E( _DOOR_6               ) \
    E( _DOOR_7               ) \
    E( _DOOR_8               ) \
    E( _TRAP_DOOR_1          ) \
    E( _TRAP_DOOR_2          ) \
    E( TRAP_DOOR_3           ) \
    E( _BRIDGE_1             ) \
    E( _BRIDGE_2             ) \
    E( _BRIDGE_3             ) \
    E( _INV_PASSPORT         ) \
    E( INV_STOPWATCH         ) \
    E( _INV_HOME             ) \
    E( _CUT_4                ) \
    E( CUT_5                 ) \
    E( CUT_6                 ) \
    E( CUT_7                 ) \
    E( CUT_8                 ) \
    E( CUT_9                 ) \
    E( CUT_10                ) \
    E( CUT_11                ) \
    E( UNUSED_27             ) \
    E( UNUSED_28             ) \
    E( _INV_PASSPORT_CLOSED  ) \
    E( _INV_MAP              ) \
    E( _PISTOLS              ) \
    E( _SHOTGUN              ) \
    E( AUTOPISTOLS           ) \
    E( _UZIS                 ) \
    E( HARPOON               ) \
    E( M16                   ) \
    E( GRENADE               ) \
    E( _AMMO_PISTOLS         ) \
    E( _AMMO_SHOTGUN         ) \
    E( AMMO_AUTOPISTOLS      ) \
    E( _AMMO_UZIS            ) \
    E( AMMO_HARPOON          ) \
    E( AMMO_M16              ) \
    E( AMMO_GRENADE          ) \
    E( _MEDIKIT_SMALL        ) \
    E( _MEDIKIT_BIG          ) \
    E( FLARES                ) \
    E( FLARE                 ) \
    E( _INV_DETAIL           ) \
    E( _INV_SOUND            ) \
    E( _INV_CONTROLS         ) \
    E( UNUSED_29             ) \
    E( _INV_PISTOLS          ) \
    E( _INV_SHOTGUN          ) \
    E( INV_AUTOPISTOLS       ) \
    E( _INV_UZIS             ) \
    E( INV_HARPOON           ) \
    E( INV_M16               ) \
    E( INV_GRENADE           ) \
    E( _INV_AMMO_PISTOLS     ) \
    E( _INV_AMMO_SHOTGUN     ) \
    E( INV_AMMO_AUTOPISTOLS  ) \
    E( _INV_AMMO_UZIS        ) \
    E( INV_AMMO_HARPOON      ) \
    E( INV_AMMO_M16          ) \
    E( INV_AMMO_GRENADE      ) \
    E( _INV_MEDIKIT_SMALL    ) \
    E( _INV_MEDIKIT_BIG      ) \
    E( INV_FLARES            ) \
    E( _PUZZLE_1             ) \
    E( _PUZZLE_2             ) \
    E( _PUZZLE_3             ) \
    E( _PUZZLE_4             ) \
    E( _INV_PUZZLE_1         ) \
    E( _INV_PUZZLE_2         ) \
    E( _INV_PUZZLE_3         ) \
    E( _INV_PUZZLE_4         ) \
    E( _PUZZLE_HOLE_1        ) \
    E( _PUZZLE_HOLE_2        ) \
    E( _PUZZLE_HOLE_3        ) \
    E( _PUZZLE_HOLE_4        ) \
    E( _PUZZLE_DONE_1        ) \
    E( _PUZZLE_DONE_2        ) \
    E( _PUZZLE_DONE_3        ) \
    E( _PUZZLE_DONE_4        ) \
    E( SECRET_1              ) \
    E( SECRET_2              ) \
    E( SECRET_3              ) \
    E( _KEY_ITEM_1           ) \
    E( _KEY_ITEM_2           ) \
    E( _KEY_ITEM_3           ) \
    E( _KEY_ITEM_4           ) \
    E( _INV_KEY_ITEM_1       ) \
    E( _INV_KEY_ITEM_2       ) \
    E( _INV_KEY_ITEM_3       ) \
    E( _INV_KEY_ITEM_4       ) \
    E( _KEY_HOLE_1           ) \
    E( _KEY_HOLE_2           ) \
    E( _KEY_HOLE_3           ) \
    E( _KEY_HOLE_4           ) \
    E( QUEST_ITEM_1          ) \
    E( QUEST_ITEM_2          ) \
    E( INV_QUEST_ITEM_1      ) \
    E( INV_QUEST_ITEM_2      ) \
    E( DRAGON_EXPLOSION_1    ) \
    E( DRAGON_EXPLOSION_2    ) \
    E( DRAGON_EXPLOSION_3    ) \
    E( SOUND_ALARM           ) \
    E( SOUND_WATER           ) \
    E( _ENEMY_REX            ) \
    E( SOUND_BIRD            ) \
    E( SOUND_CLOCK           ) \
    E( SOUND_PLACEHOLDER     ) \
    E( DRAGON_BONES_FRONT    ) \
    E( DRAGON_BONES_BACK     ) \
    E( TRAP_EXTRA_FIRE       ) \
    E( TRAP_MINE             ) \
    E( UNUSED_30             ) \
    E( INV_BACKGROUND        ) \
    E( GRAY_DISK             ) \
    E( GONG_STICK            ) \
    E( GONG                  ) \
    E( DETONATOR             ) \
    E( HELICOPTER            ) \
    E( _EXPLOSION            ) \
    E( _WATER_SPLASH         ) \
    E( _BUBBLE               ) \
    E( UNUSED_31             ) \
    E( _BLOOD                ) \
    E( UNUSED_32             ) \
    E( FLARE_SPARKLES        ) \
    E( MUZZLE_GLOW           ) \
    E( UNUSED_33             ) \
    E( _RICOCHET             ) \
    E( UNUSED_34             ) \
    E( _MUZZLE_FLASH         ) \
    E( MUZZLE_FLASH_M16      ) \
    E( UNUSED_35             ) \
    E( _VIEW_TARGET          ) \
    E( _WATERFALL            ) \
    E( HARPOON_WTF           ) \
    E( UNUSED_36             ) \
    E( UNUSED_37             ) \
    E( GRENADE_BULLET        ) \
    E( HARPOON_BULLET        ) \
    E( _LAVA_PARTICLE        ) \
    E( _TRAP_LAVA_EMITTER    ) \
    E( _FLAME                ) \
    E( _TRAP_FLAME_EMITTER   ) \
    E( SKY                   ) \
    E( _GLYPHS               ) \
    E( ENEMY_MONK_3          ) \
    E( SOUND_DOOR_BELL       ) \
    E( SOUND_ALARM_BELL      ) \
    E( HELICOPTER_FLYING     ) \
    E( ENEMY_WINSTON         ) \
    E( UNUSED_38             ) \
    E( LARA_END_PLACE        ) \
    E( LARA_END_SHOTGUN      ) \
    E( DRAGON_EXPLSION_EMITTER ) \
    E( TR2_TYPE_MAX          ) \
    E( __LARA                = TR3_TYPES_START ) \
    E( __LARA_PISTOLS        ) \
    E( __LARA_BRAID          ) \
    E( __LARA_SHOTGUN        ) \
    E( LARA_DESERT_EAGLE     ) \
    E( __LARA_UZIS           ) \
    E( LARA_MP5              ) \
    E( LARA_ROCKET           ) \
    E( __LARA_GRENADE        ) \
    E( __LARA_HARPOON        ) \
    E( __LARA_FLARE          ) \
    E( LARA_UPV              ) \
    E( VEHICLE_UPV           ) \
    E( UNUSED_TR3_13         ) \
    E( VEHICLE_KAYAK         ) \
    E( __VEHICLE_BOAT        ) \
    E( VEHICLE_QUADBIKE      ) \
    E( VEHICLE_MINECART      ) \
    E( BIG_GUN               ) \
    E( HYDRO_PROPELLER       ) \
    E( ENEMY_TRIBESMAN_AXE   ) \
    E( ENEMY_TRIBESMAN_DART  ) \
    E( __ENEMY_DOG           ) \
    E( __ENEMY_RAT           ) \
    E( KILL_ALL_TRIGGERS     ) \
    E( ENEMY_WHALE           ) \
    E( __ENEMY_DIVER         ) \
    E( __ENEMY_CROW          ) \
    E( __ENEMY_TIGER         ) \
    E( ENEMY_VULTURE         ) \
    E( ASSAULT_TARGET        ) \
    E( ENEMY_CRAWLER_MUTANT_1  ) \
    E( ENEMY_ALLIGATOR       ) \
    E( UNUSED_TR3_33         ) \
    E( ENEMY_COMPSOGNATHUS   ) \
    E( ENEMY_LIZARD_MAN      ) \
    E( ENEMY_PUNA            ) \
    E( ENEMY_MERCENARY       ) \
    E( ENEMY_RAPTOR_HUNG     ) \
    E( ENEMY_RX_TECH_GUY_1   ) \
    E( ENEMY_RX_TECH_GUY_2   ) \
    E( ENEMY_ANTARC_DOG      ) \
    E( ENEMY_CRAWLER_MUTANT_2  ) \
    E( UNUSED_TR3_43         ) \
    E( ENEMY_TINNOS_WASP     ) \
    E( ENEMY_TINNOS_MONSTER  ) \
    E( ENEMY_BRUTE_MUTANT    ) \
    E( RESPAWN_TINNOS_WASP   ) \
    E( RESPAWN_RAPTOR        ) \
    E( ENEMY_WILLARD_SPIDER  ) \
    E( ENEMY_RX_TECH_FLAME_GUY ) \
    E( ENEMY_LONDON_MERCENARY  ) \
    E( UNUSED_TR3_52         ) \
    E( ENEMY_PUNK            ) \
    E( UNUSED_TR3_54         ) \
    E( UNUSED_TR3_55         ) \
    E( ENEMY_LONDON_GUARD    ) \
    E( ENEMY_SOPHIA          ) \
    E( CLEANER_ROBOT         ) \
    E( UNUSED_TR3_59         ) \
    E( ENEMY_MILITARY_1      ) \
    E( ENEMY_MILITARY_2      ) \
    E( PRISONER              ) \
    E( ENEMY_MILITARY_3      ) \
    E( GUN_TURRET            ) \
    E( ENEMY_DAM_GUARD       ) \
    E( TRIPWIRE              ) \
    E( ELECTRIC_WIRE         ) \
    E( KILLER_TRIPWIRE       ) \
    E( ENEMY_COBRA           ) \
    E( ENEMY_SHIVA           ) \
    E( ENEMY_MONKEY          ) \
    E( UNUSED_TR3_72         ) \
    E( ENEMY_TONY            ) \
    E( AI_GUARD              ) \
    E( AI_AMBUSH             ) \
    E( AI_PATROL_1           ) \
    E( AI_MODIFY             ) \
    E( AI_FOLLOW             ) \
    E( AI_PATROL_2           ) \
    E( AI_PATH               ) \
    E( AI_CHECK              ) \
    E( UNUSED_TR3_82         ) \
    E( __TRAP_FLOOR          ) \
    E( UNUSED_TR3_84         ) \
    E( UNUSED_TR3_85         ) \
    E( TRAP_SWING_THING      ) \
    E( __TRAP_SPIKES         ) \
    E( __TRAP_BOULDER        ) \
    E( TRAP_BOULDER_GIANT    ) \
    E( __DART                ) \
    E( __TRAP_DART_EMITTER   ) \
    E( UNUSED_TR3_92         ) \
    E( UNUSED_TR3_93         ) \
    E( TRAP_SKELETON         ) \
    E( __BLOCK_1             ) \
    E( __BLOCK_2             ) \
    E( __BLOCK_3             ) \
    E( __BLOCK_4             ) \
    E( UNUSED_TR3_99         ) \
    E( UNUSED_TR3_100        ) \
    E( __WINDOW_1            ) \
    E( __WINDOW_2            ) \
    E( UNUSED_TR3_103        ) \
    E( UNUSED_TR3_104        ) \
    E( UNUSED_TR3_105        ) \
    E( AREA51_SWINGER        ) \
    E( __TRAP_CEILING_1      ) \
    E( __TRAP_SPINDLE        ) \
    E( UNUSED_TR3_109        ) \
    E( SUBWAY_TRAIN          ) \
    E( TRAP_WALL_KNIFE_BLADE ) \
    E( UNUSED_TR3_112        ) \
    E( __TRAP_ICICLES        ) \
    E( __TRAP_SPIKES_WALL    ) \
    E( UNUSED_TR3_115        ) \
    E( TRAP_SPIKES_VERT      ) \
    E( __WHEEL_KNOB          ) \
    E( __SWITCH_BIG          ) \
    E( __CUT_1               ) \
    E( __CUT_2               ) \
    E( __CUT_3               ) \
    E( SHIVA_STATUE          ) \
    E( MONKEY_MEDIPACK       ) \
    E( MONKEY_KEY            ) \
    E( __UI_FRAME            ) \
    E( UNUSED_TR3_126        ) \
    E( __ZIPLINE_HANDLE      ) \
    E( __SWITCH_BUTTON       ) \
    E( __SWITCH              ) \
    E( __SWITCH_WATER        ) \
    E( __DOOR_1              ) \
    E( __DOOR_2              ) \
    E( __DOOR_3              ) \
    E( __DOOR_4              ) \
    E( __DOOR_5              ) \
    E( __DOOR_6              ) \
    E( __DOOR_7              ) \
    E( __DOOR_8              ) \
    E( __TRAP_DOOR_1         ) \
    E( __TRAP_DOOR_2         ) \
    E( __TRAP_DOOR_3         ) \
    E( __BRIDGE_1            ) \
    E( __BRIDGE_2            ) \
    E( __BRIDGE_3            ) \
    E( __INV_PASSPORT        ) \
    E( __INV_STOPWATCH       ) \
    E( __INV_HOME            ) \
    E( __CUT_4               ) \
    E( __CUT_5               ) \
    E( __CUT_6               ) \
    E( __CUT_7               ) \
    E( __CUT_8               ) \
    E( __CUT_9               ) \
    E( __CUT_10              ) \
    E( __CUT_11              ) \
    E( CUT_12                ) \
    E( UNUSED_TR3_157        ) \
    E( __INV_PASSPORT_CLOSED ) \
    E( __INV_MAP             ) \
    E( __PISTOLS             ) \
    E( __SHOTGUN             ) \
    E( DESERT_EAGLE          ) \
    E( __UZIS                ) \
    E( __HARPOON             ) \
    E( MP5                   ) \
    E( ROCKET                ) \
    E( __GRENADE             ) \
    E( __AMMO_PISTOLS        ) \
    E( __AMMO_SHOTGUN        ) \
    E( AMMO_DESERT_EAGLE     ) \
    E( __AMMO_UZIS           ) \
    E( __AMMO_HARPOON        ) \
    E( AMMO_MP5              ) \
    E( AMMO_ROCKET           ) \
    E( __AMMO_GRENADE        ) \
    E( __MEDIKIT_SMALL       ) \
    E( __MEDIKIT_BIG         ) \
    E( __FLARES              ) \
    E( __FLARE               ) \
    E( CRYSTAL_PICKUP        ) \
    E( __INV_DETAIL          ) \
    E( __INV_SOUND           ) \
    E( __INV_CONTROLS        ) \
    E( INV_GLOBE             ) \
    E( __INV_PISTOLS         ) \
    E( __INV_SHOTGUN         ) \
    E( INV_DESERT_EAGLE      ) \
    E( __INV_UZIS            ) \
    E( __INV_HARPOON         ) \
    E( INV_MP5               ) \
    E( INV_ROCKET            ) \
    E( __INV_GRENADE         ) \
    E( __INV_AMMO_PISTOLS    ) \
    E( __INV_AMMO_SHOTGUN    ) \
    E( INV_AMMO_DESERT_EAGLE ) \
    E( __INV_AMMO_UZIS       ) \
    E( __INV_AMMO_HARPOON    ) \
    E( INV_AMMO_MP5          ) \
    E( INV_AMMO_ROCKET       ) \
    E( __INV_AMMO_GRENADE    ) \
    E( __INV_MEDIKIT_SMALL   ) \
    E( __INV_MEDIKIT_BIG     ) \
    E( __INV_FLARES          ) \
    E( INV_CRYSTAL           ) \
    E( __PUZZLE_1            ) \
    E( __PUZZLE_2            ) \
    E( __PUZZLE_3            ) \
    E( __PUZZLE_4            ) \
    E( __INV_PUZZLE_1        ) \
    E( __INV_PUZZLE_2        ) \
    E( __INV_PUZZLE_3        ) \
    E( __INV_PUZZLE_4        ) \
    E( __PUZZLE_HOLE_1       ) \
    E( __PUZZLE_HOLE_2       ) \
    E( __PUZZLE_HOLE_3       ) \
    E( __PUZZLE_HOLE_4       ) \
    E( __PUZZLE_DONE_1       ) \
    E( __PUZZLE_DONE_2       ) \
    E( __PUZZLE_DONE_3       ) \
    E( __PUZZLE_DONE_4       ) \
    E( UNUSER_TR3_121        ) \
    E( UNUSER_TR3_122        ) \
    E( UNUSER_TR3_123        ) \
    E( __KEY_ITEM_1          ) \
    E( __KEY_ITEM_2          ) \
    E( __KEY_ITEM_3          ) \
    E( __KEY_ITEM_4          ) \
    E( __INV_KEY_ITEM_1      ) \
    E( __INV_KEY_ITEM_2      ) \
    E( __INV_KEY_ITEM_3      ) \
    E( __INV_KEY_ITEM_4      ) \
    E( __KEY_HOLE_1          ) \
    E( __KEY_HOLE_2          ) \
    E( __KEY_HOLE_3          ) \
    E( __KEY_HOLE_4          ) \
    E( __QUEST_ITEM_1        ) \
    E( __QUEST_ITEM_2        ) \
    E( __INV_QUEST_ITEM_1    ) \
    E( __INV_QUEST_ITEM_2    ) \
    E( STONE_ITEM_1          ) \
    E( STONE_ITEM_2          ) \
    E( STONE_ITEM_3          ) \
    E( STONE_ITEM_4          ) \
    E( INV_STONE_ITEM_1      ) \
    E( INV_STONE_ITEM_2      ) \
    E( INV_STONE_ITEM_3      ) \
    E( INV_STONE_ITEM_4      ) \
    E( UNUSED_TR3_248        ) \
    E( UNUSED_TR3_249        ) \
    E( UNUSED_TR3_250        ) \
    E( UNUSED_TR3_251        ) \
    E( UNUSED_TR3_252        ) \
    E( UNUSED_TR3_253        ) \
    E( UNUSED_TR3_254        ) \
    E( UNUSED_TR3_255        ) \
    E( UNUSED_TR3_256        ) \
    E( UNUSED_TR3_257        ) \
    E( UNUSED_TR3_258        ) \
    E( UNUSED_TR3_259        ) \
    E( UNUSED_TR3_260        ) \
    E( UNUSED_TR3_261        ) \
    E( UNUSED_TR3_262        ) \
    E( UNUSED_TR3_263        ) \
    E( UNUSED_TR3_264        ) \
    E( UNUSED_TR3_265        ) \
    E( UNUSED_TR3_266        ) \
    E( UNUSED_TR3_267        ) \
    E( UNUSED_TR3_268        ) \
    E( UNUSED_TR3_269        ) \
    E( UNUSED_TR3_270        ) \
    E( UNUSED_TR3_271        ) \
    E( KEYS_SPRITE_1         ) \
    E( KEYS_SPRITE_2         ) \
    E( UNUSED_TR3_274        ) \
    E( UNUSED_TR3_275        ) \
    E( STONE_ITEM_PLACE_1    ) \
    E( STONE_ITEM_PLACE_2    ) \
    E( STONE_ITEM_PLACE_3    ) \
    E( STONE_ITEM_PLACE_4    ) \
    E( UNUSED_TR3_280        ) \
    E( UNUSED_TR3_281        ) \
    E( DRAGON_STATUE         ) \
    E( UNUSED_TR3_283        ) \
    E( UNUSED_TR3_284        ) \
    E( UNKNOWN_VISIBLE       ) \
    E( UNUSED_TR3_286        ) \
    E( __ENEMY_REX           ) \
    E( __ENEMY_RAPTOR        ) \
    E( UNUSED_TR3_289        ) \
    E( UNUSED_TR3_290        ) \
    E( LASER_SWEEPER         ) \
    E( ELECTRIC_FILER        ) \
    E( UNUSED_TR3_293        ) \
    E( SHADOW_SPRITE         ) \
    E( __DETONATOR           ) \
    E( MISC_SPRITES          ) \
    E( __BUBBLE              ) \
    E( UNUSED_TR3_298        ) \
    E( __MUZZLE_GLOW         ) \
    E( __MUZZLE_FLASH        ) \
    E( MUZZLE_FLASH_MP5      ) \
    E( UNUSED_TR3_302        ) \
    E( UNUSED_TR3_303        ) \
    E( __VIEW_TARGET         ) \
    E( __WATERFALL           ) \
    E( __HARPOON_WTF         ) \
    E( UNUSED_TR3_307        ) \
    E( UNUSED_TR3_308        ) \
    E( BULLET_ROCKET         ) \
    E( BULLET_HARPOON        ) \
    E( BULLET_GRENADE        ) \
    E( BIG_MISSILE           ) \
    E( __SMOKE               ) \
    E( MOVABLE_BOOM          ) \
    E( LARA_SKIN             ) \
    E( GLOW_2                ) \
    E( UNKNONW_TR3_VISIBLE   ) \
    E( LIGHT_1               ) \
    E( LIGHT_2               ) \
    E( LIGHT_3               ) \
    E( LIGHT_4               ) \
    E( LIGHT_5               ) \
    E( LIGHT_6               ) \
    E( LIGHT_7               ) \
    E( LIGHT_8               ) \
    E( LIGHT_9               ) \
    E( LIGHT_10              ) \
    E( LIGHT_11              ) \
    E( UNUSED_TR3_329        ) \
    E( PARTICLE_FIRE_1       ) \
    E( PARTICLE_FIRE_2       ) \
    E( PARTICLE_FIRE_3       ) \
    E( PARTICLE_FIRE_4       ) \
    E( PARTICLE_SMOKE_1      ) \
    E( PARTICLE_SMOKE_2      ) \
    E( PARTICLE_SMOKE_3      ) \
    E( PARTICLE_SMOKE_4      ) \
    E( ENEMY_PIRANHAS        ) \
    E( ENEMY_FISH            ) \
    E( UNUSED_TR3_340        ) \
    E( UNUSED_TR3_341        ) \
    E( UNUSED_TR3_342        ) \
    E( UNUSED_TR3_343        ) \
    E( UNUSED_TR3_344        ) \
    E( UNUSED_TR3_345        ) \
    E( UNUSED_TR3_346        ) \
    E( ENEMY_BAT_SWARM       ) \
    E( UNUSED_TR3_348        ) \
    E( ANIMATING_1           ) \
    E( ANIMATING_2           ) \
    E( ANIMATING_3           ) \
    E( ANIMATING_4           ) \
    E( ANIMATING_5           ) \
    E( ANIMATING_6           ) \
    E( __SKY                 ) \
    E( __GLYPHS              ) \
    E( __SOUND_DOOR_BELL     ) \
    E( UNUSED_TR3_358        ) \
    E( UNUSED_TR3_359        ) \
    E( __ENEMY_WINSTON       ) \
    E( ENEMY_WINSTON_CAMO    ) \
    E( GLYPHS_TIMER          ) \
    E( UNUSED_TR3_363        ) \
    E( UNUSED_TR3_364        ) \
    E( __EARTHQUAKE          ) \
    E( GUN_SHELL_1           ) \
    E( GUN_SHELL_2           ) \
    E( UNUSED_TR3_368        ) \
    E( UNUSED_TR3_369        ) \
    E( TINNOS_LIGHT_SHAFT    ) \
    E( UNUSED_TR3_371        ) \
    E( UNUSED_TR3_372        ) \
    E( ELECTRIC_SWITCH       ) \
    E( TR3_TYPE_MAX          )

namespace TR {

    struct Level;

    enum {
        NO_FLOOR = -127,
        NO_ROOM  = 0xFF,
        NO_BOX   = 0xFFFF,
        NO_WATER = 0x7FFFFFFF,
        ACTIVE   = 0x1F,
    };

    enum {
        ANIM_CMD_NONE       ,
        ANIM_CMD_OFFSET     ,
        ANIM_CMD_JUMP       ,
        ANIM_CMD_EMPTY      ,
        ANIM_CMD_KILL       ,
        ANIM_CMD_SOUND      ,
        ANIM_CMD_EFFECT     ,
    };

    struct Effect {
        enum Type {
            NONE           = -1,
            ROTATE_180     ,
            FLOOR_SHAKE    ,
            LARA_NORMAL    ,
            LARA_BUBBLES   ,
            FINISH_LEVEL   ,
            EARTHQUAKE     ,
            FLOOD          ,
            UNK1           ,
            STAIRS2SLOPE   ,
            UNK3           ,
            UNK4           ,
            EXPLOSION      ,
            LARA_HANDSFREE ,
            FLIP_MAP       ,
            DRAW_RIGHTGUN  ,
            DRAW_LEFTGUN   ,
            SHOT_RIGHTGUN  ,
            SHOT_LEFTGUN   ,
            MESH_SWAP_1    ,
            MESH_SWAP_2    ,
            MESH_SWAP_3    ,
            INV_ON         ,
            INV_OFF        ,
            DYN_ON         ,
            DYN_OFF        ,
            STATUE_FX      ,
            RESET_HAIR     ,
            BOILER_FX      ,
            ASSAULT_RESET  ,
            ASSAULT_STOP   ,
            ASSAULT_START  ,
            ASSAULT_FINISH ,
            FOOTPRINT      ,
        // specific
            TR1_FLICKER    = 16,
        };
    };

    enum {
        SND_NO              = 2,

        SND_LANDING         = 4,
        
        SND_UNHOLSTER       = 6,
        SND_HOLSTER         = 7,
        SND_PISTOLS_SHOT    = 8,
        SND_SHOTGUN_RELOAD  = 9,
        SND_RICOCHET        = 10,
        
        SND_HIT_BEAR        = 16,
        SND_HIT_WOLF        = 20,

        SND_SCREAM          = 30,
        SND_HIT             = 31,

        SND_WATER_SPLASH    = 33,
        
        SND_BUBBLE          = 37,
         
        SND_UZIS_SHOT       = 43,
        SND_MAGNUMS_SHOT    = 44,
        SND_SHOTGUN_SHOT    = 45,
        SND_EMPTY           = 48,
        SND_HIT_UNDERWATER  = 50,
        
        SND_UNDERWATER      = 60,

        SND_BOULDER         = 70,

        SND_FLOOD           = 81,

        SND_HIT_LION        = 85,

        SND_HIT_RAT         = 95,
        
        SND_LIGHTNING       = 98,
        SND_ROCK            = 99,

        SND_SWORD           = 103,
        SND_EXPLOSION       = 104,

        SND_INV_SPIN        = 108,
        SND_INV_HOME        = 109,
        SND_INV_CONTROLS    = 110,
        SND_INV_SHOW        = 111,
        SND_INV_HIDE        = 112,
        SND_INV_COMPASS     = 113,
        SND_INV_WEAPON      = 114,
        SND_INV_PAGE        = 115,
        SND_HEALTH          = 116,
        
        SND_STAIRS2SLOPE    = 119,

        SND_NATLA_SHOT      = 123,

        SND_HIT_SKATEBOY    = 132,

        SND_HIT_MUTANT      = 142,
        SND_STOMP           = 147,
        
        SND_LAVA            = 149,
        SND_FLAME           = 150,
        SND_DART            = 151,
        
        SND_TNT             = 170,
        SND_MUTANT_DEATH    = 171,
        SND_SECRET          = 173,

        SND_HELICOPTER      = 297,

        SND_WINSTON_SCARED  = 344,
        SND_WINSTON_WALK    = 345,
        SND_WINSTON_PUSH    = 346,
        SND_WINSTON_TRAY    = 347,
    };

    enum {
        MODEL_LARA          = 0,
        MODEL_PISTOLS       = 1,
        MODEL_SHOTGUN       = 2,
        MODEL_MAGNUMS       = 3,
        MODEL_UZIS          = 4,
        MODEL_LARA_SPEC     = 5,
    };

    enum HitType {
        HIT_DEFAULT,
        HIT_FALL,
        HIT_DART,
        HIT_BLADE,
        HIT_BOULDER,
        HIT_SPIKES,
        HIT_SWORD,
        HIT_LAVA,
        HIT_SLAM,
        HIT_REX,
        HIT_LIGHTNING,
        HIT_MIDAS,
        HIT_GIANT_MUTANT,
    };

    struct Action {
        enum Type {
            ACTIVATE        ,   // activate item
            CAMERA_SWITCH   ,   // switch to camera
            FLOW            ,   // underwater flow
            FLIP            ,   // flip map
            FLIP_ON         ,   // flip on
            FLIP_OFF        ,   // flip off
            CAMERA_TARGET   ,   // look at item
            END             ,   // end level
            SOUNDTRACK      ,   // play soundtrack
            EFFECT          ,   // special effect trigger
            SECRET          ,   // secret found
        };
    };

    namespace Limits {

        struct Limit {
            float  dy, dz, ay;
            ::Box  box;
            bool   alignAngle;
            bool   alignHoriz;
        };

        Limit SWITCH = {
            0, 376, 30,     ::Box(vec3(-200, 0, 312), vec3(200, 0, 512)), true, false
        };

        Limit SWITCH_UNDERWATER = {
            0, 100, 80,     ::Box(vec3(-1024, -1024, -1024), vec3(1024, 1024, 512)), true, true
        };

        Limit SWITCH_BUTTON = {
            0, 292, 30,     ::Box(vec3(-200, 0, 250), vec3(200, 0, 512)), true, true
        };

        Limit PICKUP = {
            0, -100, 180,   ::Box(vec3(-256, -100, -256), vec3(256, 100, 100)), false, true
        };

        Limit PICKUP_UNDERWATER = {
            -200, -350, 45, ::Box(vec3(-512, -512, -512), vec3(512, 512, 512)), false, true
        };

        Limit KEY_HOLE = { 
            0, 362, 30,     ::Box(vec3(-200, 0, 312), vec3(200, 0, 512)), true, true
        };

        Limit PUZZLE_HOLE = { 
            0, 327, 30,     ::Box(vec3(-200, 0, 312), vec3(200, 0, 512)), true, true
        };

        Limit BLOCK = { 
            0, -612, 30,    ::Box(vec3(-300, 0, -692), vec3(300, 0, -512)), true, false
        };

        Limit MIDAS = { 
            512, -612, 30,  ::Box(vec3(-700, 284, -700), vec3(700, 996, 700)), true, false
        };

        Limit SCION = { 
            640, -202, 30,  ::Box(vec3(-256, 540, -350), vec3(256, 740, -200)), false, false
        };

        Limit SCION_HOLDER = { 
            640, -202, 10,  ::Box(vec3(-256, 206, -862), vec3(256, 306, -200)), true, false
        };
    }

    struct fixed {
        uint16  L;
        int16   H;
        operator float() const {
            return H + L / 65535.0f;
        }
    };

    struct angle {
        uint16 value;

        angle() {}
        angle(uint16 value) : value(value) {}
        angle(float value) : value(uint16(value / (PI * 0.5f) * 16384.0f)) {}
        operator float() const { return value / 16384.0f * PI * 0.5f; };
    };

    enum TextureType {
        TEX_TYPE_ROOM,
        TEX_TYPE_ITEM,
        TEX_TYPE_OBJECT,
        TEX_TYPE_SPRITE,
    };

    struct TextureInfo {
        TextureType  type;     // determines in which atlas this texture will be stored
        TextureType  dataType; // original texture type from file
        uint16       index;
        uint16       clut;
        uint16       tile;
        uint32       attribute:15, animated:1;    // 0 - opaque, 1 - transparent, 2 - blend additive, animated, triangle
        short2       texCoord[4];
        short2       texCoordAtlas[4];
        int16        l, t, r, b;

        uint16       sub[4], i5;

        TextureInfo() {}

        TextureInfo(TextureType type, int16 l, int16 t, int16 r, int16 b, uint8 tx, uint8 ty, uint8 tw, uint8 th) : type(type), dataType(type), attribute(1), l(l), t(t), r(r), b(b) {
            texCoord[0] = texCoordAtlas[0] = short2( tx,          ty          );
            texCoord[1] = texCoordAtlas[1] = short2( tx + tw - 1, ty + th - 1 );
        }

        short4 getMinMax() const {
            if (dataType == TEX_TYPE_SPRITE)
                return short4( texCoord[0].x, texCoord[0].y, texCoord[1].x, texCoord[1].y );

            return short4(
                min(min(texCoord[0].x, texCoord[1].x), texCoord[2].x),
                min(min(texCoord[0].y, texCoord[1].y), texCoord[2].y),
                max(max(texCoord[0].x, texCoord[1].x), texCoord[2].x),
                max(max(texCoord[0].y, texCoord[1].y), texCoord[2].y)
            );
        }

        short4 getMinMaxAtlas() const {
            if (dataType == TEX_TYPE_SPRITE)
                return short4( texCoordAtlas[0].x, texCoordAtlas[0].y, texCoordAtlas[1].x, texCoordAtlas[1].y );

            return short4(
                min(min(texCoordAtlas[0].x, texCoordAtlas[1].x), texCoordAtlas[2].x),
                min(min(texCoordAtlas[0].y, texCoordAtlas[1].y), texCoordAtlas[2].y),
                max(max(texCoordAtlas[0].x, texCoordAtlas[1].x), texCoordAtlas[2].x),
                max(max(texCoordAtlas[0].y, texCoordAtlas[1].y), texCoordAtlas[2].y)
            );
        }
    };

    // used for access from ::cmp func
    static TextureInfo   *gObjectTextures = NULL;
    static TextureInfo   *gSpriteTextures = NULL;
    static int            gObjectTexturesCount;
    static int            gSpriteTexturesCount;

    struct Face {
        union {
            struct { uint16 texture:15, doubleSided:1; };
            uint16 value;
        } flags;

        short3 normal;
        uint16 vertices[4];
        uint8  triangle:1, colored:1, water:1, flip:5;

        static int cmp(const Face &a, const Face &b) {
            int aIndex = a.flags.texture;
            int bIndex = b.flags.texture;
            if (aIndex >= gObjectTexturesCount)
                return 1;
            if (bIndex >= gObjectTexturesCount)
                return -1;

            ASSERT(aIndex < gObjectTexturesCount);
            ASSERT(bIndex < gObjectTexturesCount);

            TextureInfo &ta = gObjectTextures[aIndex];
            TextureInfo &tb = gObjectTextures[bIndex];

            if (ta.tile < tb.tile)
                return -1;
            if (ta.tile > tb.tile)
                return  1;

            #ifdef SPLIT_BY_CLUT
                if (ta.clut < tb.clut)
                    return -1;
                if (ta.clut > tb.clut)
                    return  1;
            #endif

            if (aIndex < bIndex)
                return -1;
            if (aIndex > bIndex)
                return  1;

            return 0;
        }
    };

    #define FACE4_SIZE (sizeof(uint16) + sizeof(uint16) * 4) // flags + vertices[4]
    #define FACE3_SIZE (FACE4_SIZE - sizeof(uint16))

    struct ColorIndex4 {
        uint8 a:4, b:4;
    };

    struct Tile4 {
        ColorIndex4 index[256 * 256 / 2];
    };

    struct Tile8 {
        uint8 index[256 * 256];
    };

    struct Tile16 {
        Color16 color[256 * 256];
    };

    struct Tile32 {
        Color32 color[256 * 256]; // + 128 for mips data
    };

    struct CLUT {
        Color16 color[16];
    };

    struct Room {

        struct Info {
            int32 x, z;
            int32 yBottom, yTop;
        } info;

        struct Data {
            uint32      size;       // Number of data words (uint16_t's)

            int16       vCount;
            int16       tCount;
            int16       rCount;
            int16       fCount;
            int16       sCount;

            Face        *faces;

            struct Vertex {
                short3  pos;
                int16   unused_lighting;   // 0 (bright) .. 0x1FFF (dark)
                uint16  attributes;
                Color32 color;
            } *vertices;

            struct Sprite {
                int16 vertexIndex;
                int16 texture;

                static int cmp(const Sprite &a, const Sprite &b) {
                    ASSERT(a.texture < gSpriteTexturesCount);
                    ASSERT(b.texture < gSpriteTexturesCount);

                    TextureInfo &ta = gSpriteTextures[a.texture];
                    TextureInfo &tb = gSpriteTextures[b.texture];

                    if (ta.tile < tb.tile)
                        return -1;
                    if (ta.tile > tb.tile)
                        return  1;

                    #ifdef SPLIT_BY_CLUT
                        if (ta.clut < tb.clut)
                            return -1;
                        if (ta.clut > tb.clut)
                            return  1;
                    #endif

                    if (a.texture < b.texture)
                        return -1;
                    if (a.texture > b.texture)
                        return  1;

                    return 0;
                }

            } *sprites;
        } data;

        uint16  portalsCount;
        uint16  zSectors;
        uint16  xSectors;
        uint16  ambient;    // 0 (bright) .. 0x1FFF (dark)
        uint16  ambient2;
        uint16  lightMode;
        uint16  lightsCount;
        uint16  meshesCount;
        int16   alternateRoom;
        union {
            struct {
                uint16 water:1, :2, sky:1, :1, wind:1, unused:9, visible:1;
            };
            uint16 value;
        } flags;
        uint16  reverbType;
        uint8   waterScheme;
        uint8   filter;
        uint8   align;
        int32   waterLevel[2]; // water level for normal and flipped level state
        int32   waterLevelSurface;

        struct DynLight {
            int32 id;
            vec4  pos;
            vec4  color;
        } dynLights[2]; // 1 is reserved for main light
        
        int32 dynLightsCount;

        struct Portal {
            uint16 roomIndex;
            short3 normal;
            short3 vertices[4];

            vec3 getCenter() const {
                return vec3(float( (int(vertices[0].x) + int(vertices[1].x) + int(vertices[2].x) + int(vertices[3].x)) / 4 ),
                            float( (int(vertices[0].y) + int(vertices[1].y) + int(vertices[2].y) + int(vertices[3].y)) / 4 ),
                            float( (int(vertices[0].z) + int(vertices[1].z) + int(vertices[2].z) + int(vertices[3].z)) / 4 ));
            }

            vec3 getSize() const {
                return vec3(float( abs(int(vertices[0].x) - int(vertices[2].x)) / 2 ),
                            float( abs(int(vertices[0].y) - int(vertices[2].y)) / 2 ),
                            float( abs(int(vertices[0].z) - int(vertices[2].z)) / 2 ));
            }
        } *portals;

        struct Sector {
            uint32  material;
            uint16  floorIndex; // Index into FloorData[]
            uint16  boxIndex;   // Index into Boxes[] (-1 if none)
            uint8   roomBelow;  // 255 is none
            int8    floor;      // Absolute height of floor * 256
            uint8   roomAbove;  // 255 if none
            int8    ceiling;    // Absolute height of ceiling * 256
        } *sectors;

        struct Light {
            int32   x, y, z;
            uint32  radius;
            int32   intensity;
            Color32 color;
        } *lights;

        struct Mesh {
            int32   x, y, z;
            angle   rotation;
            uint16  meshID;
            Color32 color;
            uint32  meshIndex; // index into static meshes array
        } *meshes;

        vec3 getOffset() const {
            return vec3(float(info.x), 0.0f, float(info.z));
        }

        vec3 getCenter() const {
            return vec3(info.x + xSectors * 512.0f, (info.yBottom + info.yTop) * 0.5f, info.z + zSectors * 512.0f);
        }

        Sector* getSector(int sx, int sz) {
            if (sz <= 0 || sz >= zSectors - 1) {
                sz = clamp(sz, 0, zSectors - 1);
                sx = clamp(sx, 1, xSectors - 2);
            } else
                sx = clamp(sx, 0, xSectors - 1);

            ASSERT(sx >= 0 && sx < xSectors && sz >= 0 && sz < zSectors);
            return sectors + sx * zSectors + sz;
        }

        void addDynLight(int32 id, const vec4 &pos, const vec4 &color, bool priority = false) {
            DynLight *light = NULL;
            for (int i = 0; i < dynLightsCount; i++)
                if (dynLights[i].id == id) {
                    float maxRadius = min(dynLights[i].color.w, color.w); // radius is invSqrt
                    light = &dynLights[i];
                    light->color.w = maxRadius;
                    break;
                }

            // 1 is additional second light, can be overridden
            if (!light) {
                if (dynLightsCount < COUNT(dynLights))
                    light = &dynLights[dynLightsCount++];
                else
                    light = &dynLights[1];
            }

            light->id    = id;
            light->pos   = pos;
            light->color = color;

            if (priority && dynLights[0].id != id)
                swap(dynLights[0], dynLights[1]);
        }

        void removeDynLight(int32 id) {
            for (int i = 0; i < dynLightsCount; i++)
                if (dynLights[i].id == id) {
                    if (i == 0) dynLights[0] = dynLights[1];
                    dynLightsCount--;
                    break;
                }
        }

        int getAmbient(int x, int y, int z, Light **nearLight = NULL) const {
            if (!lightsCount) {
                return ambient;
            }

            int ambientInv = 0x1FFF - ambient;
            int maxValue   = 0;

            for (int i = 0; i < lightsCount; i++) {
                Light &light = lights[i];
                if (light.intensity > 8192)
                    continue;

                int dx = x - light.x;
                int dy = y - light.y;
                int dz = z - light.z;

                int D = (SQR(dx) + SQR(dy) + SQR(dz)) >> 12;
                int R = SQR(light.radius >> 1) >> 12;

                int value = min(0x1FFF, (light.intensity * R) / (D + R) + ambientInv);

                if (maxValue < value) {
                    if (nearLight) {
                        *nearLight = &light;
                    }
                    maxValue = value;
                }
            }

            return 0x1FFF - (maxValue + ambientInv) / 2;
        }
    };

    union FloorData {
        uint16 value;
        union Command {
            struct {
                uint16 func:5, tri:3, sub:7, end:1;
            };
            struct {
                int16 :5, a:5, b:5, :1;
            } triangle;
        } cmd;
        struct {
            int16 slantX:8, slantZ:8;
        };
        struct {
            uint16 a:4, b:4, c:4, d:4;
        };
        struct TriggerInfo {
            uint16  timer:8, once:1, mask:5, :2;
        } triggerInfo;
        union TriggerCommand {
            struct {
                uint16 args:10, action:5, end:1;
            };
            struct {
                uint16 timer:8, once:1, speed:5, :2;
            };
        } triggerCmd;

        enum {
              NONE
            , PORTAL
            , FLOOR
            , CEILING
            , TRIGGER
            , LAVA
            , CLIMB
            , FLOOR_NW_SE_SOLID
            , FLOOR_NE_SW_SOLID
            , CEILING_NW_SE_SOLID
            , CEILING_NE_SW_SOLID
            , FLOOR_NW_SE_PORTAL_SE
            , FLOOR_NW_SE_PORTAL_NW
            , FLOOR_NE_SW_PORTAL_SW
            , FLOOR_NE_SW_PORTAL_NE
            , CEILING_NW_SE_PORTAL_SE
            , CEILING_NW_SE_PORTAL_NW
            , CEILING_NE_SW_PORTAL_SW
            , CEILING_NE_SW_PORTAL_NE
            , MONKEY
            , MINECART_LEFT
            , MINECART_RIGHT
        };
    };

    union Overlap {
        struct { uint16 boxIndex:15, end:1; };
        uint16 value;
    };

    struct Flags {
        uint16 :8, once:1, active:5, :2;
    };

    // internal mesh structure
    struct Mesh {

        struct Vertex {
            short4 coord;
            short4 normal;
        };

        short3      center;
        int16       radius;
        union {
            struct {
                uint16 isStatic:1, reserved:15;
            };
            uint16 value;
        }           flags;
        int16       vCount;
        int16       tCount;
        int16       rCount;
        int16       fCount;

        int32       offset;
        Vertex      *vertices;
        Face        *faces;

        Mesh() : vertices(0), faces(0) {}
        ~Mesh() {
            delete[] vertices;
            delete[] faces;
        }
    };

    struct Entity {
        enum ActiveState { asNone, asActive, asInactive };
        enum Type        { NONE = 0, TR_TYPES(DECL_ENUM) TYPE_MAX = 0xFFFF };

        Type    type;
        int16   room;
        int32   x, y, z;
        angle   rotation;
        int16   intensity;
        int16   intensity2;
        union Flags {
            struct { 
                uint16 state:2, unused:3, smooth:1, :1, invisible:1, once:1, active:5, reverse:1, rendered:1;
            };
            uint16 value;
        } flags;
    // not exists in file
        int32   modelIndex;     // index of representation in models (index + 1) or spriteSequences (-(index + 1)) arrays
        void    *controller;    // Controller implementation or NULL

        static Type remap(Version version, Type type) {
            if (version & VER_TR1)
                return type;

            if (version & VER_TR2)
                type = Type(type + TR2_TYPES_START);

            if (version & VER_TR3)
                type = Type(type + TR3_TYPES_START);

            #define REMAP_2(TYPE) case _##TYPE  : return TYPE
            #define REMAP_3(TYPE) case __##TYPE : return TYPE

            switch (type) {
            // TR2
                REMAP_2( LARA                 );
                REMAP_2( LARA_PISTOLS         );
                REMAP_2( LARA_BRAID           );
                REMAP_2( LARA_SHOTGUN         );
                REMAP_2( LARA_UZIS            );
                REMAP_2( LARA_SPEC            );
                REMAP_2( TRAP_FLOOR           );
                REMAP_2( TRAP_SPIKES          );
                REMAP_2( TRAP_BOULDER         );
                REMAP_2( DART                 );
                REMAP_2( TRAP_DART_EMITTER    );
                REMAP_2( DRAWBRIDGE           );
                REMAP_2( TRAP_SLAM            );
                REMAP_2( BLOCK_1              );
                REMAP_2( BLOCK_2              );
                REMAP_2( BLOCK_3              );
                REMAP_2( BLOCK_4              );
                REMAP_2( TRAP_CEILING_1       );
                REMAP_2( CUT_1                );
                REMAP_2( CUT_2                );
                REMAP_2( CUT_3                );
                REMAP_2( SWITCH               );
                REMAP_2( SWITCH_WATER         );
                REMAP_2( DOOR_1               );
                REMAP_2( DOOR_2               );
                REMAP_2( DOOR_3               );
                REMAP_2( DOOR_4               );
                REMAP_2( DOOR_5               );
                REMAP_2( DOOR_6               );
                REMAP_2( DOOR_7               );
                REMAP_2( DOOR_8               );
                REMAP_2( TRAP_DOOR_1          );
                REMAP_2( TRAP_DOOR_2          );
                REMAP_2( BRIDGE_1             );
                REMAP_2( BRIDGE_2             );
                REMAP_2( BRIDGE_3             );
                REMAP_2( INV_PASSPORT         );
                REMAP_2( INV_HOME             );
                REMAP_2( CUT_4                );
                REMAP_2( INV_PASSPORT_CLOSED  );
                REMAP_2( INV_MAP              );
                REMAP_2( PISTOLS              );
                REMAP_2( SHOTGUN              );
                REMAP_2( UZIS                 );
                REMAP_2( AMMO_PISTOLS         );
                REMAP_2( AMMO_SHOTGUN         );
                REMAP_2( AMMO_UZIS            );
                REMAP_2( MEDIKIT_SMALL        );
                REMAP_2( MEDIKIT_BIG          );
                REMAP_2( INV_DETAIL           );
                REMAP_2( INV_SOUND            );
                REMAP_2( INV_CONTROLS         );
                REMAP_2( INV_PISTOLS          );
                REMAP_2( INV_SHOTGUN          );
                REMAP_2( INV_UZIS             );
                REMAP_2( INV_AMMO_PISTOLS     );
                REMAP_2( INV_AMMO_SHOTGUN     );
                REMAP_2( INV_AMMO_UZIS        );
                REMAP_2( INV_MEDIKIT_SMALL    );
                REMAP_2( INV_MEDIKIT_BIG      );
                REMAP_2( PUZZLE_1             );
                REMAP_2( PUZZLE_2             );
                REMAP_2( PUZZLE_3             );
                REMAP_2( PUZZLE_4             );
                REMAP_2( INV_PUZZLE_1         );
                REMAP_2( INV_PUZZLE_2         );
                REMAP_2( INV_PUZZLE_3         );
                REMAP_2( INV_PUZZLE_4         );
                REMAP_2( PUZZLE_HOLE_1        );
                REMAP_2( PUZZLE_HOLE_2        );
                REMAP_2( PUZZLE_HOLE_3        );
                REMAP_2( PUZZLE_HOLE_4        );
                REMAP_2( PUZZLE_DONE_1        );
                REMAP_2( PUZZLE_DONE_2        );
                REMAP_2( PUZZLE_DONE_3        );
                REMAP_2( PUZZLE_DONE_4        );
                REMAP_2( KEY_ITEM_1           );
                REMAP_2( KEY_ITEM_2           );
                REMAP_2( KEY_ITEM_3           );
                REMAP_2( KEY_ITEM_4           );
                REMAP_2( INV_KEY_ITEM_1       );
                REMAP_2( INV_KEY_ITEM_2       );
                REMAP_2( INV_KEY_ITEM_3       );
                REMAP_2( INV_KEY_ITEM_4       );
                REMAP_2( KEY_HOLE_1           );
                REMAP_2( KEY_HOLE_2           );
                REMAP_2( KEY_HOLE_3           );
                REMAP_2( KEY_HOLE_4           );
                REMAP_2( ENEMY_REX            );
                REMAP_2( EXPLOSION            );
                REMAP_2( WATER_SPLASH         );
                REMAP_2( BUBBLE               );
                REMAP_2( BLOOD                );
                REMAP_2( RICOCHET             );
                REMAP_2( MUZZLE_FLASH         );
                REMAP_2( VIEW_TARGET          );
                REMAP_2( WATERFALL            );
                REMAP_2( LAVA_PARTICLE        );
                REMAP_2( TRAP_LAVA_EMITTER    );
                REMAP_2( FLAME                );
                REMAP_2( TRAP_FLAME_EMITTER   );
                REMAP_2( GLYPHS               );
            // TR3
                REMAP_3( LARA                 );
                REMAP_3( LARA_PISTOLS         );
                REMAP_3( LARA_BRAID           );
                REMAP_3( LARA_SHOTGUN         );
                REMAP_3( LARA_UZIS            );
                REMAP_3( LARA_GRENADE         );
                REMAP_3( LARA_HARPOON         );
                REMAP_3( LARA_FLARE           );
                REMAP_3( VEHICLE_BOAT         );
                REMAP_3( ENEMY_DOG            );
                REMAP_3( ENEMY_RAT            );
                REMAP_3( ENEMY_DIVER          );
                REMAP_3( ENEMY_CROW           );
                REMAP_3( ENEMY_TIGER          );
                REMAP_3( TRAP_FLOOR           );
                REMAP_3( TRAP_SPIKES          );
                REMAP_3( TRAP_BOULDER         );
                REMAP_3( DART                 );
                REMAP_3( TRAP_DART_EMITTER    );
                REMAP_3( BLOCK_1              );
                REMAP_3( BLOCK_2              );
                REMAP_3( BLOCK_3              );
                REMAP_3( BLOCK_4              );
                REMAP_3( WINDOW_1             );
                REMAP_3( WINDOW_2             );
                REMAP_3( TRAP_CEILING_1       );
                REMAP_3( TRAP_SPINDLE         );
                REMAP_3( TRAP_ICICLES         );
                REMAP_3( TRAP_SPIKES_WALL     );
                REMAP_3( WHEEL_KNOB           );
                REMAP_3( SWITCH_BIG           );
                REMAP_3( CUT_1                );
                REMAP_3( CUT_2                );
                REMAP_3( CUT_3                );
                REMAP_3( UI_FRAME             );
                REMAP_3( ZIPLINE_HANDLE       );
                REMAP_3( SWITCH_BUTTON        );
                REMAP_3( SWITCH               );
                REMAP_3( SWITCH_WATER         );
                REMAP_3( DOOR_1               );
                REMAP_3( DOOR_2               );
                REMAP_3( DOOR_3               );
                REMAP_3( DOOR_4               );
                REMAP_3( DOOR_5               );
                REMAP_3( DOOR_6               );
                REMAP_3( DOOR_7               );
                REMAP_3( DOOR_8               );
                REMAP_3( TRAP_DOOR_1          );
                REMAP_3( TRAP_DOOR_2          );
                REMAP_3( TRAP_DOOR_3          );
                REMAP_3( BRIDGE_1             );
                REMAP_3( BRIDGE_2             );
                REMAP_3( BRIDGE_3             );
                REMAP_3( INV_PASSPORT         );
                REMAP_3( INV_STOPWATCH        );
                REMAP_3( INV_HOME             );
                REMAP_3( CUT_4                );
                REMAP_3( CUT_5                );
                REMAP_3( CUT_6                );
                REMAP_3( CUT_7                );
                REMAP_3( CUT_8                );
                REMAP_3( CUT_9                );
                REMAP_3( CUT_10               );
                REMAP_3( CUT_11               );
                REMAP_3( INV_PASSPORT_CLOSED  );
                REMAP_3( INV_MAP              );
                REMAP_3( PISTOLS              );
                REMAP_3( SHOTGUN              );
                REMAP_3( UZIS                 );
                REMAP_3( HARPOON              );
                REMAP_3( GRENADE              );
                REMAP_3( AMMO_PISTOLS         );
                REMAP_3( AMMO_SHOTGUN         );
                REMAP_3( AMMO_UZIS            );
                REMAP_3( AMMO_HARPOON         );
                REMAP_3( AMMO_GRENADE         );
                REMAP_3( MEDIKIT_SMALL        );
                REMAP_3( MEDIKIT_BIG          );
                REMAP_3( FLARES               );
                REMAP_3( FLARE                );
                REMAP_3( INV_DETAIL           );
                REMAP_3( INV_SOUND            );
                REMAP_3( INV_CONTROLS         );
                REMAP_3( INV_PISTOLS          );
                REMAP_3( INV_SHOTGUN          );
                REMAP_3( INV_UZIS             );
                REMAP_3( INV_HARPOON          );
                REMAP_3( INV_GRENADE          );
                REMAP_3( INV_AMMO_PISTOLS     );
                REMAP_3( INV_AMMO_SHOTGUN     );
                REMAP_3( INV_AMMO_UZIS        );
                REMAP_3( INV_AMMO_HARPOON     );
                REMAP_3( INV_AMMO_GRENADE     );
                REMAP_3( INV_MEDIKIT_SMALL    );
                REMAP_3( INV_MEDIKIT_BIG      );
                REMAP_3( INV_FLARES           );
                REMAP_3( PUZZLE_1             );
                REMAP_3( PUZZLE_2             );
                REMAP_3( PUZZLE_3             );
                REMAP_3( PUZZLE_4             );
                REMAP_3( INV_PUZZLE_1         );
                REMAP_3( INV_PUZZLE_2         );
                REMAP_3( INV_PUZZLE_3         );
                REMAP_3( INV_PUZZLE_4         );
                REMAP_3( PUZZLE_HOLE_1        );
                REMAP_3( PUZZLE_HOLE_2        );
                REMAP_3( PUZZLE_HOLE_3        );
                REMAP_3( PUZZLE_HOLE_4        );
                REMAP_3( PUZZLE_DONE_1        );
                REMAP_3( PUZZLE_DONE_2        );
                REMAP_3( PUZZLE_DONE_3        );
                REMAP_3( PUZZLE_DONE_4        );
                REMAP_3( KEY_ITEM_1           );
                REMAP_3( KEY_ITEM_2           );
                REMAP_3( KEY_ITEM_3           );
                REMAP_3( KEY_ITEM_4           );
                REMAP_3( INV_KEY_ITEM_1       );
                REMAP_3( INV_KEY_ITEM_2       );
                REMAP_3( INV_KEY_ITEM_3       );
                REMAP_3( INV_KEY_ITEM_4       );
                REMAP_3( KEY_HOLE_1           );
                REMAP_3( KEY_HOLE_2           );
                REMAP_3( KEY_HOLE_3           );
                REMAP_3( KEY_HOLE_4           );
                REMAP_3( QUEST_ITEM_1         );
                REMAP_3( QUEST_ITEM_2         );
                REMAP_3( INV_QUEST_ITEM_1     );
                REMAP_3( INV_QUEST_ITEM_2     );
                REMAP_3( ENEMY_REX            );
                REMAP_3( ENEMY_RAPTOR         );
                REMAP_3( DETONATOR            );
                REMAP_3( BUBBLE               );
                REMAP_3( MUZZLE_GLOW          );
                REMAP_3( MUZZLE_FLASH         );
                REMAP_3( VIEW_TARGET          );
                REMAP_3( WATERFALL            );
                REMAP_3( HARPOON_WTF          );
                REMAP_3( SMOKE                );
                REMAP_3( SKY                  );
                REMAP_3( GLYPHS               );
                REMAP_3( SOUND_DOOR_BELL      );
                REMAP_3( ENEMY_WINSTON        );
                REMAP_3( EARTHQUAKE           );

                default : return type;
            }

            #undef REMAP
        }


        bool isEnemy() const {
            return (type >= ENEMY_DOPPELGANGER && type <= ENEMY_GIANT_MUTANT) || type == SCION_TARGET ||
                   (type >= ENEMY_DOG && type <= ENEMY_DRAGON_BACK) ||
                   (type >= ENEMY_SHARK && type <= ENEMY_MONK_2);
        }

        bool isBigEnemy() const {
            return type == ENEMY_REX || type == ENEMY_MUTANT_1 || type == ENEMY_CENTAUR;
        }

        bool isVehicle() const {
            return type == VEHICLE_BOAT || type == VEHICLE_SNOWMOBILE_RED || type == VEHICLE_SNOWMOBILE_BLACK;
        }

        bool isDoor() const {
            return type >= DOOR_1 && type <= DOOR_8;
        }

        static bool isPuzzleItem(Type type) {
            return (type >= PUZZLE_1 && type <= PUZZLE_4) || (type >= INV_PUZZLE_1 && type <= INV_PUZZLE_4);
        }
        
        static bool isKeyItem(Type type) {
            return (type >= KEY_ITEM_1 && type <= KEY_ITEM_4) || (type >= INV_KEY_ITEM_1 && type <= INV_KEY_ITEM_4);
        }

        static bool isWeapon(Type type) {
            return (type >= PISTOLS && type <= AMMO_UZIS) ||
                   (type == M16 || type == AMMO_M16) ||
                   (type == MP5 || type == AMMO_MP5) ||
                   (type == AUTOPISTOLS || type == AMMO_AUTOPISTOLS) ||
                   (type == DESERT_EAGLE || type == AMMO_DESERT_EAGLE) || 
                   (type == GRENADE || type == AMMO_GRENADE) || 
                   (type == ROCKET || type == AMMO_ROCKET) ||
                   (type == HARPOON || type == AMMO_HARPOON);
        }

        static bool isPickup(Type type) {
            return isPuzzleItem(type) ||
                   isKeyItem(type) ||
                   isWeapon(type) ||
                   (type == MEDIKIT_SMALL || type == MEDIKIT_BIG) || 
                   (type == SCION_PICKUP_QUALOPEC || type == SCION_PICKUP_DROP || type == SCION_PICKUP_HOLDER || type == LEADBAR) ||
                   (type == CRYSTAL) ||
                   (type >= SECRET_1 && type <= SECRET_3) ||
                   (type == FLARES || type == FLARE) || 
                   (type >= STONE_ITEM_1 && type <= STONE_ITEM_4);
        }

        static bool isInventoryItem(Type type) {
            return type == INV_PASSPORT
                || type == INV_PASSPORT_CLOSED
                || type == INV_MAP
                || type == INV_COMPASS
                || type == INV_STOPWATCH
                || type == INV_HOME
                || type == INV_DETAIL
                || type == INV_SOUND
                || type == INV_CONTROLS
                || type == INV_GAMMA
                || type == INV_PISTOLS
                || type == INV_SHOTGUN
                || type == INV_MAGNUMS
                || type == INV_UZIS
                || type == INV_AMMO_PISTOLS
                || type == INV_AMMO_SHOTGUN
                || type == INV_AMMO_MAGNUMS
                || type == INV_AMMO_UZIS
                || type == INV_EXPLOSIVE
                || type == INV_MEDIKIT_SMALL
                || type == INV_MEDIKIT_BIG
                || type == INV_PUZZLE_1
                || type == INV_PUZZLE_2
                || type == INV_PUZZLE_3
                || type == INV_PUZZLE_4
                || type == INV_KEY_ITEM_1
                || type == INV_KEY_ITEM_2
                || type == INV_KEY_ITEM_3
                || type == INV_KEY_ITEM_4
                || type == INV_LEADBAR
                || type == INV_SCION;
        }

        static bool isCrossLevelItem(Type type) {
            return isPickup(type) && !isPuzzleItem(type) && !isKeyItem(type) && (type != LEADBAR) && !(type >= SECRET_1 && type <= SECRET_3);
        }

        bool isPickup() const {
            return isPickup(type);
        }

        bool isActor() const {
            return (type >= CUT_1 && type <= CUT_4) || (type >= CUT_5 && type <= CUT_11) || (type == CUT_12);
        }

        bool isPuzzleHole() const {
            return type >= PUZZLE_HOLE_1 && type <= PUZZLE_HOLE_4;
        }

        bool isBlock() const {
            return type >= BLOCK_1 && type <= BLOCK_4;
        }

        bool isLara() const {
            return type == LARA;
        }

        bool isSprite() const {
            return type == EXPLOSION || type == WATER_SPLASH || type == BUBBLE || 
                   type == BLOOD || type == SMOKE || type == FLAME || 
                   type == RICOCHET || type == SPARKLES || type == LAVA_PARTICLE;
        }

        bool castShadow() const {
            return isLara() || isEnemy() || isVehicle() || isActor() || type == DART || type == TRAP_SWORD || type == ENEMY_WINSTON || type == ENEMY_WINSTON_CAMO;
        }

        void getAxis(int &dx, int &dz) {
            switch (rotation.value / 0x4000) {
                case 0  : dx =  0; dz =  1; break;
                case 1  : dx =  1; dz =  0; break;
                case 2  : dx =  0; dz = -1; break;
                case 3  : dx = -1; dz =  0; break;
                default : dx =  0; dz =  0; break;
            }
        }

        static Type getItemForHole(Type hole) {
            switch (hole) {
                case PUZZLE_HOLE_1 : return PUZZLE_1;
                case PUZZLE_HOLE_2 : return PUZZLE_2;
                case PUZZLE_HOLE_3 : return PUZZLE_3;
                case PUZZLE_HOLE_4 : return PUZZLE_4;
                case KEY_HOLE_1    : return KEY_ITEM_1;
                case KEY_HOLE_2    : return KEY_ITEM_2;
                case KEY_HOLE_3    : return KEY_ITEM_3;
                case KEY_HOLE_4    : return KEY_ITEM_4;
                case MIDAS_HAND    : return LEADBAR;
                default            : return LARA;
            }
        }

        static void fixOpaque(Type type, bool &opaque) { // to boost performance on mobile devices
            if (type >= LARA && type <= ENEMY_GIANT_MUTANT
                && type != ENEMY_REX
                && type != ENEMY_RAPTOR
                && type != ENEMY_MUTANT_1
                && type != ENEMY_MUTANT_2
                && type != ENEMY_MUTANT_3
                && type != ENEMY_CENTAUR
                && type != ENEMY_GIANT_MUTANT
                && type != ENEMY_MUMMY
                && type != ENEMY_NATLA)
                opaque = true;
            if (type == SWITCH || type == SWITCH_WATER || type == CUT_1)
                opaque = true;
            if ((type >= PUZZLE_HOLE_1 && type <= PUZZLE_HOLE_4) || type == LIGHTNING)
                opaque = false;
        }
    };

    #define INV_ITEM_PAIR(TYPE) { Entity::TYPE, Entity::INV_##TYPE }

    static struct {
        Entity::Type src, dst;
    } ITEM_TO_INV[] = {
    // weapon
        INV_ITEM_PAIR(PISTOLS),
        INV_ITEM_PAIR(SHOTGUN),
        INV_ITEM_PAIR(MAGNUMS),
        INV_ITEM_PAIR(UZIS),
        INV_ITEM_PAIR(AUTOPISTOLS),
        INV_ITEM_PAIR(HARPOON),
        INV_ITEM_PAIR(M16),
        INV_ITEM_PAIR(GRENADE),
    // ammo
        INV_ITEM_PAIR(AMMO_PISTOLS),
        INV_ITEM_PAIR(AMMO_SHOTGUN),
        INV_ITEM_PAIR(AMMO_MAGNUMS),
        INV_ITEM_PAIR(AMMO_UZIS),
        INV_ITEM_PAIR(AMMO_AUTOPISTOLS),
        INV_ITEM_PAIR(AMMO_HARPOON),
        INV_ITEM_PAIR(AMMO_M16),
        INV_ITEM_PAIR(AMMO_GRENADE),
    // items
        INV_ITEM_PAIR(MEDIKIT_BIG),
        INV_ITEM_PAIR(MEDIKIT_SMALL),
        INV_ITEM_PAIR(FLARES),
    // key items
        INV_ITEM_PAIR(KEY_ITEM_1),
        INV_ITEM_PAIR(KEY_ITEM_2),
        INV_ITEM_PAIR(KEY_ITEM_3),
        INV_ITEM_PAIR(KEY_ITEM_4),
    // puzzle items
        INV_ITEM_PAIR(PUZZLE_1),
        INV_ITEM_PAIR(PUZZLE_2),
        INV_ITEM_PAIR(PUZZLE_3),
        INV_ITEM_PAIR(PUZZLE_4),
    // other items
        INV_ITEM_PAIR(LEADBAR),
        INV_ITEM_PAIR(QUEST_ITEM_1),
        INV_ITEM_PAIR(QUEST_ITEM_2),
        { Entity::SCION_PICKUP_DROP,     Entity::INV_SCION },
        { Entity::SCION_PICKUP_QUALOPEC, Entity::INV_SCION },
    };

    #undef INV_ITEM_PAIR

    struct Animation {
        uint32  frameOffset;
        uint8   frameRate;
        uint8   frameSize;

        uint16  state;

        fixed   speed;
        fixed   accel;

        uint16  frameStart;
        uint16  frameEnd;
        uint16  nextAnimation;
        uint16  nextFrame;

        uint16  scCount;
        uint16  scOffset;

        uint16  acCount;
        uint16  animCommand;
    };

    struct AnimState {
        uint16  state;
        uint16  rangesCount;
        uint16  rangesOffset;
    };

    struct AnimRange {
        int16   low;            // Lowest frame that uses this range
        int16   high;           // Highest frame that uses this range
        int16   nextAnimation;  // Animation to dispatch to
        int16   nextFrame;      // Frame offset to dispatch to
    };

    struct MinMax {
        int16 minX, maxX, minY, maxY, minZ, maxZ;

        vec3 min() const { return vec3((float)minX, (float)minY, (float)minZ); }
        vec3 max() const { return vec3((float)maxX, (float)maxY, (float)maxZ); }
    };

    struct AnimFrame {
        MinMax  box;
        short3  pos;
        uint16  angles[1];  // angle frames in YXZ order, first angle must be skipped in TR1

        #define ANGLE_SCALE (2.0f * PI / 1024.0f)

        vec3 unpack(uint16 a, uint16 b) {
            return vec3(float((a & 0x3FF0) >> 4), float( ((a & 0x000F) << 6) | ((b & 0xFC00) >> 10)), float(b & 0x03FF)) * ANGLE_SCALE;
        }

        vec3 getAngle(Version version, int joint) {
            int index = 0;

            if (version & VER_TR1) {
                index = joint * 2 + 1;
                uint16 b = angles[index++];
                uint16 a = angles[index++];
                if (version & VER_SAT)
                    swap(a, b);
                return unpack(a, b);
            }

            if (version & (VER_TR2 | VER_TR3)) {

                // TODO: remove this !!!
                for (int i = 0; i < joint; i++)
                    if (!(angles[index++] & 0xC000))
                        index++;

                uint16 a = angles[index++];

                float rot = float(a & 0x03FF) * ANGLE_SCALE;

                switch (a & 0xC000) {
                    case 0x4000 : return vec3(rot, 0, 0);
                    case 0x8000 : return vec3(0, rot, 0);
                    case 0xC000 : return vec3(0, 0, rot);
                    default     : return unpack(a, angles[index++]);
                }
            }

            return vec3(0);
        }

        #undef ANGLE_SCALE
    };

    struct AnimTexture {
        uint16 count;
        uint16 *textures;
    };

    struct Node {
        uint32  flags;
        int32   x, y, z;
    };

    struct Model {
        Entity::Type type;
        uint16       index;
        uint16       mCount;
        uint16       mStart;
        uint32       node;
        uint32       frame;
        uint16       animation;
        uint16       align;
    };

    struct StaticMesh {
        uint32  id;
        uint16  mesh;
        MinMax  vbox;
        MinMax  cbox;
        uint16  flags;

        void getBox(bool collision, angle rotation, ::Box &box) {
            int k = rotation.value / 0x4000;

            MinMax &m = collision ? cbox : vbox;

            ASSERT(m.minX <= m.maxX && m.minY <= m.maxY && m.minZ <= m.maxZ);

            box = ::Box(m.min(), m.max());
            box.rotate90(k);

            ASSERT(box.min.x <= box.max.x && box.min.y <= box.max.y && box.min.z <= box.max.z);
        }
    };

    struct SpriteSequence {
        Entity::Type type;
        uint16       unused;
        int16        sCount;
        int16        sStart;
        uint8        transp;
    };

    struct Camera {
        int32   x, y, z;
        union {
            int16   room;   // for camera
            int16   speed;  // for sink (underwater current)
        };
        union {
            struct { uint16 :8, once:1, :5, :2; };
            uint16 boxIndex;
        } flags;
    };

    struct CameraFrame {
        short3  target;
        short3  pos;
        int16   fov;
        int16   roll;
    };

    struct SoundSource {
        int32   x, y, z;    // absolute position of sound source (world coordinates)
        uint16  id;         // internal sample index
        uint16  flags;      // 0x40, 0x80, or 0xC0
    };

    struct Box {
        union {
            struct {
                int32 minZ, maxZ; // Horizontal dimensions in global units
                int32 minX, maxX;
            };
            int32 sides[4];
        };

        int16   floor; // Height value in global units
        union {
            struct {
                uint16 index:14, block:1, blockable:1;    // Index into Overlaps[].
            };
            uint16 value;
        } overlap;

        bool contains(int32 x, int32 z) const {
            return z >= minZ && z <= maxZ && x >= minX && x <= maxX;
        }

        void expand(int32 value) {
            minZ -= value;
            minX -= value;
            maxZ += value;
            maxX += value;
        }

        void clip(const Box &box) {
            minZ = max(minZ, box.minZ);
            minX = max(minX, box.minX);
            maxZ = min(maxZ, box.maxZ);
            maxX = min(maxX, box.maxX);
        }
    };

    struct Zone {
        uint16 *ground1;
        uint16 *ground2;
        uint16 *ground3;
        uint16 *ground4;
        uint16 *fly;
    };

    struct SoundInfo {
        float  volume;
        float  chance;
        float  range;
        float  pitch;
        uint16 index;
        union {
            struct { uint16 mode:2, count:4, unused:6, camera:1, pitch:1, gain:1, :1; };
            uint16 value;
        } flags;
    };

    struct Location {
        int16  room;
        uint16 box;
        vec3   pos;
    };

    struct Level {
        Version         version;
        LevelID         id;

        int32           tilesCount;

        uint32          unused;

        uint16          roomsCount;
        Room            *rooms;

        int32           floorsCount;
        FloorData       *floors;

        int16           meshesCount;
        Mesh            meshes[MAX_MESHES];

        int32           meshDataSize;
        uint16          *meshData;

        int32           meshOffsetsCount;
        int32           *meshOffsets;

        int32           animsCount;
        Animation       *anims;

        int32           statesCount;
        AnimState       *states;

        int32           rangesCount;
        AnimRange       *ranges;

        int32           commandsCount;
        int16           *commands;

        int32           nodesDataSize;
        uint32          *nodesData;

        int32           frameDataSize;
        uint16          *frameData;

        int32           modelsCount;
        Model           *models;

        int32           staticMeshesCount;
        StaticMesh      *staticMeshes;

        int32           objectTexturesBaseCount;
        int32           objectTexturesCount;
        TextureInfo     *objectTextures;

        int32           objectTexturesDataSize;
        uint8           *objectTexturesData;

        int32           roomTexturesCount;
        TextureInfo     *roomTextures;

        int32           roomTexturesDataSize;
        uint8           *roomTexturesData;

        int32           itemTexturesCount;
        TextureInfo     *itemTextures;

        int32           itemTexturesBaseCount;
        int32           itemTexturesDataSize;
        uint8           *itemTexturesData;

        int32           spriteTexturesCount;
        TextureInfo     *spriteTextures;

        int32           spriteSequencesCount;
        SpriteSequence  *spriteSequences;

        int32           spriteTexturesDataSize;
        uint8           *spriteTexturesData;

        int32           camerasCount;
        Camera          *cameras;

        int32           soundSourcesCount;
        SoundSource     *soundSources;

        int32           boxesCount;
        Box             *boxes;
        int32           overlapsCount;
        Overlap         *overlaps;
        Zone            zones[2];   // default and alternative

        int16           animTexturesCount;
        AnimTexture     *animTextures;

        int32           entitiesBaseCount;
        int32           entitiesCount;
        Entity          *entities;

        Color24         *palette;
        Color32         *palette32;

        uint16          kanjiSprite;
        int32           clutsCount;
        CLUT            *cluts;

        Tile4           *tiles4;
        Tile8           *tiles8;
        Tile16          *tiles16;

        uint16          cameraFramesCount;
        CameraFrame     *cameraFrames;

        uint16          demoDataSize;
        uint8           *demoData;

        int16           *soundsMap;

        int32           soundsInfoCount;
        SoundInfo       *soundsInfo;

        int32           soundDataSize;
        uint8           *soundData;

        int32           soundOffsetsCount;
        uint32          *soundOffsets;
        uint32          *soundSize;

        Color32         skyColor;

   // common
        struct Trigger {
            enum Type {
                ACTIVATE    ,
                PAD         ,
                SWITCH      ,
                KEY         ,
                PICKUP      ,
                HEAVY       ,
                ANTIPAD     ,
                COMBAT      ,
                DUMMY       ,
            };
        };
    
        struct FloorInfo {
        // TODO reduse type sizes
            float roomFloor, roomCeiling;
            int roomNext, roomBelow, roomAbove;
            float floor, ceiling;
            int slantX, slantZ;
            int floorIndex;
            int boxIndex;
            int lava;
            int climb;
            int trigCmdCount;
            Trigger::Type trigger;
            FloorData::TriggerInfo trigInfo;
            FloorData::TriggerCommand trigCmd[MAX_TRIGGER_COMMANDS];

            vec3 getNormal() {
                return vec3((float)-slantX, -4.0f, (float)-slantZ).normal();
            }

            vec3 getSlant(const vec3 &dir) {
                // project floor normal into plane(dir, up) 
                vec3 r = vec3(dir.z, 0.0f, -dir.x); // up(0, 1, 0).cross(dir)
                vec3 n = getNormal();
                n = n - r * r.dot(n);
                // project dir into plane(dir, n)
                return n.cross(dir.cross(n)).normal();
            }
        };

        SaveState    state;

        int     cutEntity;
        mat4    cutMatrix;
        bool    isDemoLevel;
        bool    simpleItems;

        struct Extra {

            static int getWeaponIndex(Entity::Type type) {
                #if MAX_WEAPONS != 4
                    #error update this func first
                #endif

                switch (type) {
                // pistols
                    case Entity::LARA_PISTOLS      :
                    case Entity::INV_PISTOLS       : 
                    case Entity::INV_AMMO_PISTOLS  : 
                    case Entity::AMMO_PISTOLS      : 
                    case Entity::PISTOLS           : return  0;
                // shotgun
                    case Entity::LARA_SHOTGUN      :
                    case Entity::INV_SHOTGUN       :
                    case Entity::INV_AMMO_SHOTGUN  : 
                    case Entity::AMMO_SHOTGUN      : 
                    case Entity::SHOTGUN           : return  1;
                // magnums
                    case Entity::LARA_MAGNUMS      :
                    case Entity::INV_MAGNUMS       :
                    case Entity::INV_AMMO_MAGNUMS  : 
                    case Entity::AMMO_MAGNUMS      : 
                    case Entity::MAGNUMS           : return  2;
                // uzis
                    case Entity::LARA_UZIS         :
                    case Entity::INV_UZIS          :
                    case Entity::INV_AMMO_UZIS     : 
                    case Entity::AMMO_UZIS         : 
                    case Entity::UZIS              : return  3;

                    default : ASSERT(false);
                }
                return 0;
            }

            int16 muzzleFlash;
            int16 puzzleDone[4];
            int16 braid;
            int16 laraSpec;
            int16 laraSkin;
            int16 meshSwap[3];
            int16 sky;
            int16 smoke;
            int16 waterSplash;
            int16 glyphs;

            struct {
                int16 items[MAX_WEAPONS];
                int16& operator[] (Entity::Type type) {
                    return items[getWeaponIndex(type)];
                };
            } weapons;

            struct {
                int16 passport;
                int16 passport_closed;
                int16 map;
                int16 compass;
                int16 stopwatch;
                int16 home;
                int16 detail;
                int16 sound;
                int16 controls;
                int16 gamma;
                int16 explosive;

                struct {
                    int16 items[MAX_WEAPONS];
                    int16& operator[] (Entity::Type type) {
                        return items[getWeaponIndex(type)];
                    };
                } weapons;

                struct {
                    int16 items[MAX_WEAPONS];
                    int16& operator[] (Entity::Type type) {
                        return items[getWeaponIndex(type)];
                    };
                } ammo;

                int16 medikit[2];
                int16 puzzle[4];
                int16 key[4];

                int16 leadbar;
                int16 scion;
            } inv;
        } extra;

        uint32 tsubCount;
        uint8 *tsub;

        Level(Stream &stream) {
            memset(this, 0, sizeof(*this));
            version  = VER_UNKNOWN;
            cutEntity = -1;

            int startPos = stream.pos;
            uint32 magic;

            #define MAGIC_TR1_PC  0x00000020
            #define MAGIC_TR1_PSX 0x56414270
            #define MAGIC_TR1_SAT 0x4D4F4F52
            #define MAGIC_TR2_PC  0x0000002D
            #define MAGIC_TR3_PC1 0xFF080038
            #define MAGIC_TR3_PC2 0xFF180038
            #define MAGIC_TR3_PC3 0xFF180034
            #define MAGIC_TR3_PSX 0xFFFFFFC8

            id = TR::getLevelID(stream.size, stream.name, version, isDemoLevel);

            if (version == VER_UNKNOWN || version == VER_TR1_PSX) {
                stream.read(magic);
                if (magic == MAGIC_TR1_SAT) {
                    version = VER_TR1_SAT;

                    stream.seek(-4);
                // get file name without extension
                    size_t len = strlen(stream.name);
                    char *name = new char[len + 1];
                    memcpy(name, stream.name, len);
                    for (int i = int(len) - 1; i >= 0; i--) {
                        if (name[i] == '/' || name[i] == '\\')
                            break;
                        if (name[i] == '.') {
                            len = i;
                            break;
                        }
                    }
                    name[len] = 0;
                    LOG("load Sega Saturn level: %s\n", name);

                    strcat(name, ".SAD");
                    Stream sad(name);
                    name[len] = '\0';
                    strcat(name, ".SPR");
                    Stream spr(name);
                    name[len] = '\0';
                    strcat(name, ".SND");
                    Stream snd(name);
                    name[len] = '\0';
                    strcat(name, ".CIN");

                    Stream *cin = NULL;
                    if (Stream::existsContent(name)) {
                        cin = new Stream(name);
                    } else {
                        len = strlen(name);
                        for (int i = int(len) - 1; i >= 0; i--)
                            if (name[i] == '/' || name[i] == '\\') {
                                char *newName = new char[len + 11 + 1];
                                name[i] = 0;
                                strcpy(newName, name);
                                strcat(newName, "/../CINDATA/");
                                strcat(newName, name + i + 1);
                                delete[] name;
                                name = newName;
                                break;
                            }

                        if (Stream::existsContent(name))
                            cin = new Stream(name);
                    }

                    delete[] name;

                    readSAT(sad);
                    readSAT(spr);
                    readSAT(snd);
                    if (cin) {
                        readCIN(*cin);
                        delete cin;
                    }
                    readSAT(stream); // sat
                    return;
                }

                if (magic != MAGIC_TR1_PC && magic != MAGIC_TR2_PC && magic != MAGIC_TR3_PC1 && magic != MAGIC_TR3_PC2 && magic != MAGIC_TR3_PC3 && magic != MAGIC_TR3_PSX) {
                    stream.read(magic);
                }

                switch (magic) {
                    case MAGIC_TR1_PC  : version = VER_TR1_PC;  break;
                    case MAGIC_TR1_PSX : version = VER_TR1_PSX; break;
                    case MAGIC_TR2_PC  : version = VER_TR2_PC;  break;
                    case MAGIC_TR3_PC1 :
                    case MAGIC_TR3_PC2 : 
                    case MAGIC_TR3_PC3 : version = VER_TR3_PC;  break;
                    case MAGIC_TR3_PSX : version = VER_TR3_PSX; break;
                    default            : ;
                }
            }

            if (version == VER_UNKNOWN) {
                LOG("unsupported level format\n"); 
                ASSERT(false);
                return;
            }

            if (version == VER_TR2_PSX) {
                stream.read(soundOffsets, stream.read(soundOffsetsCount) + 1);
                soundSize = new uint32[soundOffsetsCount];
                soundDataSize = 0;
                for (int i = 0; i < soundOffsetsCount; i++) {
                    ASSERT(soundOffsets[i] < soundOffsets[i + 1]);
                    soundSize[i]    = soundOffsets[i + 1] - soundOffsets[i];
                    soundOffsets[i] = soundDataSize;
                    soundDataSize  += soundSize[i];
                }
                stream.read(soundData, soundDataSize);
            }

            if (version == VER_TR3_PSX) {
                stream.read(soundOffsets, stream.read(soundOffsetsCount));
                if (soundOffsetsCount) {
                    stream.read(soundDataSize);
                    stream.read(soundData, soundDataSize);
                    soundSize = new uint32[soundOffsetsCount];
                    int size = 0;
                    for (int i = 0; i < soundOffsetsCount - 1; i++) {
                        ASSERT(soundOffsets[i] < soundOffsets[i + 1]);
                        size += soundSize[i] = soundOffsets[i + 1] - soundOffsets[i];
                    }

                    if (soundOffsetsCount) {
                        soundSize[soundOffsetsCount - 1] = soundDataSize;
                        if (soundOffsetsCount > 1)
                            soundSize[soundOffsetsCount - 1] -= soundSize[soundOffsetsCount - 2];
                    }
                }

            // skip code modules
                int size;
                for (int i = 0; i < 13; i++) {
                    stream.read(size);
                    if (size) {
                        stream.seek(size);
                        stream.read(size);
                        stream.seek(size);
                    }
                }
            }

            if (version == VER_TR2_PC || version == VER_TR3_PC) {
                stream.read(palette,   256);
                stream.read(palette32, 256);
            }

            if (version == VER_TR1_PSX && !isCutsceneLevel()) {
                uint32 offsetTexTiles;
                stream.seek(8);
                stream.read(offsetTexTiles);
            // sound offsets
                uint16 numSounds;
                stream.setPos(startPos + 22);
                stream.read(numSounds);
                stream.setPos(startPos + 2086 + numSounds * 512);
                soundOffsetsCount = numSounds;
                soundOffsets = new uint32[soundOffsetsCount];
                soundSize    = new uint32[soundOffsetsCount];
                soundDataSize = 0;
                for (int i = 0; i < soundOffsetsCount; i++) {
                    soundOffsets[i] = soundDataSize;
                    uint16 size;
                    stream.read(size);
                    soundDataSize += soundSize[i] = size * 8;
                }           
            // sound data
                stream.setPos(startPos + 2600 + numSounds * 512);
                stream.read(soundData, soundDataSize);
                stream.setPos(startPos + offsetTexTiles + 8);
            }

            // tiles
            if (version & VER_PC) {
                stream.read(tiles8, stream.read(tilesCount));
            }

            if (version == VER_TR2_PC || version == VER_TR3_PC) 
                stream.read(tiles16, tilesCount);

            if (version == VER_TR1_PSX) {
                stream.read(tiles4, tilesCount = 13);
                stream.read(cluts,  clutsCount = 1024);
            }

            if (version != VER_TR3_PSX)
                stream.read(unused);

        // rooms
            rooms = stream.read(roomsCount) ? new Room[roomsCount] : NULL;
            for (int i = 0; i < roomsCount; i++) 
                readRoom(stream, i);

        // floors
            stream.read(floors, stream.read(floorsCount));

            if (version == VER_TR3_PSX) {
                // outside room offsets
                stream.seek(27 * 27 * 2);
                // outside rooms table
                int size;
                stream.read(size);
                stream.seek(size);
                // room mesh bbox
                stream.read(size);
                stream.seek(8 * size);
            }

        // meshes
            meshesCount = 0;
            stream.read(meshData,    stream.read(meshDataSize));
            stream.read(meshOffsets, stream.read(meshOffsetsCount));
        // animations

            stream.read(anims,     stream.read(animsCount));
            stream.read(states,    stream.read(statesCount));
            stream.read(ranges,    stream.read(rangesCount));
            stream.read(commands,  stream.read(commandsCount));
            stream.read(nodesData, stream.read(nodesDataSize));
            stream.read(frameData, stream.read(frameDataSize));
        // models
            models = stream.read(modelsCount) ? new Model[modelsCount] : NULL;
            for (int i = 0; i < modelsCount; i++) {
                Model &m = models[i];
                uint16 type;
                m.type = Entity::Type(stream.read(type));
                stream.seek(sizeof(m.index));
                m.index = i;
                stream.read(m.mCount);
                stream.read(m.mStart);
                stream.read(m.node);
                stream.read(m.frame);
                stream.read(m.animation);
                if (version & VER_PSX)
                    stream.seek(2);
            }
            stream.read(staticMeshes, stream.read(staticMeshesCount));

            if (version == VER_TR2_PSX || version == VER_TR3_PSX) {
                stream.read(tiles4, stream.read(tilesCount));
                stream.read(clutsCount);
                if (clutsCount > 1024) { // check for japanese version (read kanji CLUT index)
                    kanjiSprite = clutsCount & 0xFFFF;
                    stream.seek(-2);
                    stream.read(clutsCount);
                }
                if (version == VER_TR3_PSX)
                    clutsCount *= 2; // read underwater cluts too
                stream.read(cluts, clutsCount);
                if (version != VER_TR3_PSX)
                    stream.seek(4);
            }

        // textures & UV
            if (version != VER_TR3_PC)
                readObjectTex(stream);
            readSpriteTex(stream);
        // palette for demo levels
            if (version == VER_TR1_PC && isDemoLevel) stream.read(palette, 256);
        // cameras
            stream.read(cameras,        stream.read(camerasCount));
        // sound sources
            stream.read(soundSources,   stream.read(soundSourcesCount));
        // AI
            boxes = stream.read(boxesCount) ? new Box[boxesCount] : NULL;
            for (int i = 0; i < boxesCount; i++) {
                Box &b = boxes[i];
                if (version & VER_TR1) {
                    stream.read(b.minZ);
                    stream.read(b.maxZ);
                    stream.read(b.minX);
                    stream.read(b.maxX);
                }
                
                if (version & (VER_TR2 | VER_TR3)) {
                    uint8 value;
                    b.minZ = stream.read(value) * 1024;
                    b.maxZ = stream.read(value) * 1024;
                    b.minX = stream.read(value) * 1024;
                    b.maxX = stream.read(value) * 1024;
                }

                stream.read(b.floor);
                stream.read(b.overlap.value);
            }

            stream.read(overlaps, stream.read(overlapsCount));
            for (int i = 0; i < 2; i++) {
                stream.read(zones[i].ground1, boxesCount);
                stream.read(zones[i].ground2, boxesCount);
                if (!(version & VER_TR1)) {
                    stream.read(zones[i].ground3, boxesCount);
                    stream.read(zones[i].ground4, boxesCount);
                } else {
                    zones[i].ground3 = NULL;
                    zones[i].ground4 = NULL;
                }
                stream.read(zones[i].fly, boxesCount);
            }

        // animated textures
            readAnimTex(stream);

            if (version == VER_TR3_PC)
                readObjectTex(stream);

        // entities (enemies, items, lara etc.)
            readEntities(stream);

            if (version & VER_PC) {
                stream.seek(32 * 256);
            // palette for release levels
                if ((version == VER_TR1_PC) && !isDemoLevel) 
                    stream.read(palette, 256);
            // cinematic frames for cameras (PC)
                stream.read(cameraFrames,   stream.read(cameraFramesCount));
            // demo data
                stream.read(demoData,       stream.read(demoDataSize));
            }

            if (version == VER_TR2_PSX)
                stream.seek(4);

            if (version == VER_TR3_PSX) {
                stream.read(skyColor);

                roomTexturesCount = stream.readLE32();

                if (roomTexturesCount) {
                    roomTextures = new TextureInfo[roomTexturesCount];

                // load room textures
                    for (int i = 0; i < roomTexturesCount; i++) {
                        readObjectTex(stream, roomTextures[i], TEX_TYPE_ROOM);
                        stream.seek(2 * 16); // skip 2 mipmap levels
                    }
                }
            }

        // sounds
            stream.read(soundsMap, (version & VER_TR1) ? 256 : 370);
            soundsInfo = stream.read(soundsInfoCount) ? new SoundInfo[soundsInfoCount] : NULL;
            for (int i = 0; i < soundsInfoCount; i++) {
                SoundInfo &s = soundsInfo[i];

                stream.read(s.index);
                if (version & (VER_TR1 | VER_TR2)) {
                    uint16 v;
                    stream.read(v); s.volume = float(v) / 0x7FFF;
                    stream.read(v); s.chance = float(v) / 0xFFFF;
                    s.range = 8 * 1024;
                    s.pitch = 0.2f;
                } else {
                    uint8 v;
                    stream.read(v); s.volume = float(v) / 0xFF;
                    stream.read(v); s.range  = float(v) * 1024;
                    stream.read(v); s.chance = float(v) / 0xFF;
                    stream.read(v); s.pitch  = float(v) / 0xFF;
                }

                stream.read(s.flags.value);

                ASSERT(s.volume <= 1.0f);
            }

            if (version == VER_TR3_PSX)
                stream.seek(4);

            if (version == VER_TR1_PC) {
                stream.read(soundData,    stream.read(soundDataSize));
                stream.read(soundOffsets, stream.read(soundOffsetsCount));
            }

            if (version == VER_TR2_PC || version == VER_TR3_PC) {
                stream.read(soundOffsets, stream.read(soundOffsetsCount));
                new Stream(getGameSoundsFile(version), sfxLoadAsync, this);
            }

        // cinematic frames for cameras (PSX)
            if (version & VER_PSX) {
                if (version != VER_TR3_PSX)
                    stream.seek(4);
                stream.read(cameraFrames, stream.read(cameraFramesCount));
            }

            prepare();
        }

        ~Level() {
        // rooms
            for (int i = 0; i < roomsCount; i++) {
                Room &r = rooms[i];
                delete[] r.data.vertices;
                delete[] r.data.faces;
                delete[] r.data.sprites;
                delete[] r.portals;
                delete[] r.sectors;
                delete[] r.lights;
                delete[] r.meshes;
            }
            delete[] rooms;
            delete[] floors;
            delete[] meshOffsets;
            delete[] anims;
            delete[] states;
            delete[] ranges;
            delete[] commands;
            delete[] nodesData;
            delete[] frameData;
            delete[] models;
            delete[] staticMeshes;
            delete[] objectTextures;
            delete[] objectTexturesData;
            delete[] roomTextures;
            delete[] roomTexturesData;
            delete[] itemTextures;
            delete[] itemTexturesData;
            delete[] spriteTextures;
            delete[] spriteSequences;
            delete[] spriteTexturesData;
            delete[] cameras;
            delete[] soundSources;
            delete[] boxes;
            delete[] overlaps;
            for (int i = 0; i < 2; i++) {
                delete[] zones[i].ground1;
                delete[] zones[i].ground2;
                delete[] zones[i].ground3;
                delete[] zones[i].ground4;
                delete[] zones[i].fly;
            }
            for (int i = 0; i < animTexturesCount; i++)
                delete[] animTextures[i].textures;
            delete[] animTextures;
            delete[] entities;
            delete[] palette;
            delete[] palette32;
            delete[] cluts;
            delete[] tiles4;
            delete[] tiles8;
            delete[] tiles16;
            delete[] cameraFrames;
            delete[] demoData;
            delete[] soundsMap;
            delete[] soundsInfo;
            delete[] soundData;
            delete[] soundOffsets;
            delete[] soundSize;

            delete[] tsub;
        }

        #define CHUNK(str) ((uint64)((const char*)(str))[0]        | ((uint64)((const char*)(str))[1] << 8)  | ((uint64)((const char*)(str))[2] << 16) | ((uint64)((const char*)(str))[3] << 24) | \
                           ((uint64)((const char*)(str))[4] << 32) | ((uint64)((const char*)(str))[5] << 40) | ((uint64)((const char*)(str))[6] << 48) | ((uint64)((const char*)(str))[7] << 56))

        void readSAT(Stream &stream) {
        #if !defined(_OS_PSP) && !defined(_OS_3DS)
            Room *room = NULL;

            while (stream.pos < stream.size) {
                uint64 chunkType = stream.read64();
                /*
                {
                        char buf[9];
                        memcpy(buf, &chunkType, 8);
                        buf[8] = 0;
                        LOG("chunk: \"%s\"\n", buf);
                }
                */

                switch (chunkType) {
                // SAT
                    case CHUNK("ROOMFILE") : 
                        ASSERTV(stream.readBE32() == 0x00000000);
                        ASSERTV(stream.readBE32() == 0x00000020);
                        break;
                    case CHUNK("ROOMTINF") :
                        ASSERTV(stream.readBE32() == 0x00000010);
                        roomTexturesCount = stream.readBE32();
                        roomTextures = roomTexturesCount ? new TextureInfo[roomTexturesCount] : NULL;
                        for (int i = 0; i < roomTexturesCount; i++)
                            readObjectTex(stream, roomTextures[i], TEX_TYPE_ROOM);
                        break;
                    case CHUNK("ROOMTQTR") : {
                        ASSERTV(stream.readBE32() == 0x00000001);
                        roomTexturesDataSize = stream.readBE32();
                        roomTexturesData = roomTexturesDataSize ? new uint8[roomTexturesDataSize] : NULL;
                        stream.raw(roomTexturesData, roomTexturesDataSize);
/*
                        int32 count = stream.readBE32();
                        stream.seek(count);
                    */
                        break;
                    }
                    case CHUNK("ROOMTSUB") :
                        ASSERTV(stream.readBE32() == 0x00000001);
                        /*
                        roomTexturesDataSize = stream.readBE32();
                        roomTexturesData = roomTexturesDataSize ? new uint8[roomTexturesDataSize] : NULL;
                        stream.raw(roomTexturesData, roomTexturesDataSize);
                        */

                        tsubCount = stream.readBE32();
                        tsub = new uint8[tsubCount];
                        stream.raw(tsub, sizeof(uint8) * tsubCount);

                        break;
                    case CHUNK("ROOMTPAL") : {
                        ASSERTV(stream.readBE32() == 0x00000003);
                        stream.seek(stream.readBE32() * 3);
                        break;
                    }
                    case CHUNK("ROOMSPAL") : {
                        ASSERTV(stream.readLE32() == 0x02000000);
                        stream.seek(stream.readBE32() * 2);
                        break;
                    }
                    case CHUNK("ROOMDATA") :
                        ASSERTV(stream.readBE32() == 0x00000044);
                        roomsCount = stream.readBE32();
                        rooms = new Room[roomsCount];
                        memset(rooms, 0, sizeof(Room) * roomsCount);
                        break;
                    case CHUNK("ROOMNUMB") :
                        ASSERTV(stream.readBE32() == 0x00000000);
                        room = &rooms[stream.readBE32()];
                        break;
                    case CHUNK("MESHPOS ") :
                        ASSERT(room);
                        room->info.x       = stream.readBE32();
                        room->info.z       = stream.readBE32();
                        room->info.yBottom = stream.readBE32();
                        room->info.yTop    = stream.readBE32();
                        break;
                    case CHUNK("MESHSIZE") : {
                        ASSERT(room);
                        uint32 flag = stream.readBE32();
                        if (flag == 0x00000014) {
                            room->meshesCount = stream.readBE32();

                            room->meshes = room->meshesCount ? new Room::Mesh[room->meshesCount] : NULL;
                            for (int i = 0; i < room->meshesCount; i++) {
                                Room::Mesh &m = room->meshes[i];
                                m.x = stream.readBE32();
                                m.y = stream.readBE32();
                                m.z = stream.readBE32();
                                m.rotation.value = stream.readBE16();
                                uint16 intensity = stream.readBE16();
                                stream.readBE16();
                                int value = clamp((intensity > 0x1FFF) ? 255 : (255 - (intensity >> 5)), 0, 255);
                                m.color.r = m.color.g = m.color.b = value;
                                m.color.a = 0;
                                m.meshID = stream.readBE16();
                            }

                            //stream.seek(20 * count); // unknown
                            break;
                        }
                        ASSERT(flag == 0x00000002);

                        Room::Data &data = room->data;

                        data.size = stream.readBE32();
                        data.vCount = stream.readBE16();
                        data.vertices = new Room::Data::Vertex[data.vCount];
                        for (int j = 0; j < data.vCount; j++) {
                            Room::Data::Vertex &v = data.vertices[j];
                            v.pos.x      = stream.readBE16();
                            v.pos.y      = stream.readBE16();
                            v.pos.z      = stream.readBE16();
                            v.color      = Color16(stream.readBE16());
                            v.attributes = 0;
                        }

                        data.fCount  = stream.readBE16();
                        data.faces   = new Face[data.fCount];
                        data.sprites = new Room::Data::Sprite[data.fCount];

                        enum {
                            TYPE_R_TRANSP    = 33,
                            TYPE_T_INVISIBLE = 34,
                            TYPE_R_INVISIBLE = 35,
                            TYPE_T_SOLID     = 36,
                            TYPE_R_SOLID     = 37,
                            TYPE_SPRITE      = 39,
                        };

                        int fIndex = 0;

                        uint16 typesCount = stream.readBE16();
                        for (int j = 0; j < typesCount; j++) {
                            uint16 type  = stream.readBE16();
                            uint16 count = stream.readBE16();
                            //LOG(" type:%d count:%d\n", (int)type, (int)count);
                            for (int k = 0; k < count; k++) {
                                ASSERT(fIndex < data.fCount);

                                Face &f = data.faces[fIndex++];
                                f.flags.value = 0;
                                switch (type) {
                                    case TYPE_SPRITE       : {
                                        Room::Data::Sprite &sprite = data.sprites[data.sCount++];
                                        sprite.vertexIndex = (stream.readBE16() >> 4);
                                        sprite.texture     = (stream.readBE16() >> 4);
                                        fIndex--;
                                        break;
                                    }
                                    case TYPE_T_INVISIBLE  :
                                    case TYPE_T_SOLID      :
                                        f.triangle    = true;
                                        f.vertices[0] = (stream.readBE16() >> 4);
                                        f.vertices[1] = (stream.readBE16() >> 4);
                                        f.vertices[2] = (stream.readBE16() >> 4);
                                        f.vertices[3] = 0;
                                        ASSERT(f.vertices[0] < data.vCount && f.vertices[1] < data.vCount && f.vertices[2] < data.vCount);
                                        f.flags.value = stream.readBE16();
                                        if (type == TYPE_T_INVISIBLE) {
                                            fIndex--;
                                            continue;
                                        }
                                        data.tCount++;
                                        break;
                                    case TYPE_R_INVISIBLE  :
                                    case TYPE_R_TRANSP     :
                                    case TYPE_R_SOLID      :
                                        f.triangle    = false;
                                        f.vertices[0] = (stream.readBE16() >> 4);
                                        f.vertices[1] = (stream.readBE16() >> 4);
                                        f.vertices[2] = (stream.readBE16() >> 4);
                                        f.vertices[3] = (stream.readBE16() >> 4);
                                        ASSERT(f.vertices[0] < data.vCount && f.vertices[1] < data.vCount && f.vertices[2] < data.vCount && f.vertices[3] < data.vCount);
                                        f.flags.value = stream.readBE16();
                                        if (type == TYPE_R_INVISIBLE) {
                                            fIndex--;
                                            continue;
                                        }
                                        data.rCount++;
                                        break;
                                    default :
                                        LOG("! unknown face type: %d\n", type);
                                        ASSERT(false);
                                }
                                ASSERT(f.flags.value % 16 == 0);
                                ASSERT(f.flags.value / 16 < roomTexturesCount);
                                f.flags.value /= 16;
                                f.water       = false;
                                f.colored     = false;
                                f.flip        = false;

                                if (type == TYPE_R_TRANSP)
                                    roomTextures[f.flags.texture].attribute = 1;
                            }
                        }
                        data.fCount = fIndex;
                        break;
                    }
                    case CHUNK("DOORDATA") : {
                        int32 roomIndex = stream.readBE32();
                        ASSERT(roomIndex < roomsCount);
                        Room *room = &rooms[roomIndex];
                        ASSERT(room && room->sectors == NULL);

                        room->portalsCount = stream.readBE32();
                        room->portals = new Room::Portal[room->portalsCount];

                        for (int j = 0; j < room->portalsCount; j++) {
                            Room::Portal &p = room->portals[j];
                            p.roomIndex = stream.readBE16();
                            p.normal.x  = stream.readBE16();
                            p.normal.y  = stream.readBE16();
                            p.normal.z  = stream.readBE16();
                            for (int k = 0; k < 4; k++) {
                                p.vertices[k].x = stream.readBE16();
                                p.vertices[k].y = stream.readBE16();
                                p.vertices[k].z = stream.readBE16();
                            }
                        }
                        break;
                    }
                    case CHUNK("FLOORDAT") :
                        ASSERT(room);
                        room->zSectors = stream.readBE32();
                        room->xSectors = stream.readBE32();
                        break;
                    case CHUNK("FLOORSIZ") : {
                        ASSERTV(stream.readBE32() == 0x00000008);
                        ASSERT(room && room->sectors == NULL);

                        int32 count = stream.readBE32();
                        ASSERT(count == room->xSectors * room->zSectors);

                        room->sectors = count ? new Room::Sector[count] : NULL;

                        for (int i = 0; i < count; i++) {
                            Room::Sector &s = room->sectors[i];
                            s.material   = 0;
                            s.floorIndex = stream.readBE16();
                            s.boxIndex   = stream.readBE16();
                            stream.read(s.roomBelow);
                            stream.read(s.floor);
                            stream.read(s.roomAbove);
                            stream.read(s.ceiling);
                        }
                        break;
                    }
                    case CHUNK("FLORDATA") :
                        ASSERTV(stream.readBE32() == 0x00000002);
                        ASSERT(floors == NULL);
                        floorsCount = stream.readBE32();
                        floors = new FloorData[floorsCount];
                        for (int i = 0; i < floorsCount; i++)
                            floors[i].value = stream.readBE16();
                        break;
                    case CHUNK("LIGHTAMB") :
                        ASSERT(room);
                        room->ambient  = stream.readBE32();
                        room->ambient2 = stream.readBE32();
                        break;
                    case CHUNK("RM_FLIP ") : {
                        ASSERTV(stream.readBE32() == 0x00000002);
                        uint32 value = stream.readBE32();
                        room->alternateRoom = value == 0xFFFFFFFF ? -1 : value;
                        break;
                    }
                    case CHUNK("RM_FLAGS") : {
                        ASSERT(room);
                        ASSERTV(stream.readBE32() == 0x00000002);
                        uint32 value = stream.readBE32();
                        room->flags.water = (value & 0x01) != 0;
                        break;
                    }
                    case CHUNK("LIGHTSIZ") : {
                        ASSERTV(stream.readBE32() == 0x00000014);
                        ASSERT(room && room->lights == NULL);
                        room->lightsCount = stream.readBE32();
                        room->lights = room->lightsCount ? new Room::Light[room->lightsCount] : NULL;
                        for (int i = 0; i < room->lightsCount; i++) {
                            Room::Light &light = room->lights[i];
                            light.x = stream.readBE32();
                            light.y = stream.readBE32();
                            light.z = stream.readBE32();

                            int16 intensity  = stream.readBE16();
                            int16 intensity2 = stream.readBE16();
                            ASSERTV(intensity == intensity2);
                            int value = clamp((intensity > 0x1FFF) ? 0 : (intensity >> 5), 0, 255);
                            light.color.r = light.color.g = light.color.b = value;
                            light.color.a = 0;
                            light.intensity = intensity;

                            light.radius = stream.readBE32() * 2;
                        }
                        break;
                    }
                    case CHUNK("CAMERAS ") :
                        ASSERTV(stream.readBE32() == 0x00000010);
                        ASSERT(cameras == NULL);
                        camerasCount = stream.readBE32();
                        cameras = camerasCount ? new Camera[camerasCount] : NULL;
                        for (int i = 0; i < camerasCount; i++) {
                            Camera &cam = cameras[i];
                            cam.x = stream.readBE32();
                            cam.y = stream.readBE32();
                            cam.z = stream.readBE32();
                            cam.room = stream.readBE16();
                            cam.flags.boxIndex = stream.readBE16();
                        }
                        break;
                    case CHUNK("SOUNDFX ") : {
                        uint32 flag = stream.readBE32();
                        if (flag == 0x00000000) { // SND
                            ASSERTV(stream.readBE32() == 0x00000000);

                            int pos = stream.pos;
                            stream.setPos(0);
                            soundDataSize = stream.size;
                            soundData = new uint8[soundDataSize];
                            stream.raw(soundData, soundDataSize);
                            soundOffsetsCount = 256;
                            soundOffsets = new uint32[soundOffsetsCount];
                            soundSize    = new uint32[soundOffsetsCount];
                            memset(soundOffsets, 0, soundOffsetsCount * sizeof(uint32));
                            memset(soundSize,    0, soundOffsetsCount * sizeof(uint32));
                            stream.setPos(pos);

                            break;
                        }
                        ASSERT(flag == 0x00000010); // SAT

                        ASSERT(soundSources == NULL);
                        soundSourcesCount = stream.readBE32();
                        soundSources = soundSourcesCount ? new SoundSource[soundSourcesCount] : NULL;
                        for (int i = 0; i < soundSourcesCount; i++) {
                            SoundSource &s = soundSources[i];
                            s.x     = stream.readBE32();
                            s.y     = stream.readBE32();
                            s.z     = stream.readBE32();
                            s.id    = stream.readBE16();
                            s.flags = stream.readBE16();
                        }
                        break;
                    }
                    case CHUNK("BOXES   ") :
                        ASSERTV(stream.readBE32() == 0x00000014);
                        ASSERT(boxes == NULL);
                        boxesCount = stream.readBE32();
                        boxes = boxesCount ? new Box[boxesCount] : NULL;
                        for (int i = 0; i < boxesCount; i++) {
                            Box &b = boxes[i];
                            b.minZ          = stream.readBE32();
                            b.maxZ          = stream.readBE32();
                            b.minX          = stream.readBE32();
                            b.maxX          = stream.readBE32();
                            b.floor         = stream.readBE16();
                            b.overlap.value = stream.readBE16();
                        }
                        break;
                    case CHUNK("OVERLAPS") :
                        ASSERTV(stream.readBE32() == 0x00000002);
                        ASSERT(overlaps == NULL);
                        overlapsCount = stream.readBE32();
                        overlaps = overlapsCount ? new Overlap[overlapsCount] : NULL;
                        for (int i = 0; i < overlapsCount; i++)
                            overlaps[i].value = stream.readBE16();
                        break;
                    case CHUNK("GND_ZONE") :
                    case CHUNK("GND_ZON2") :
                    case CHUNK("FLY_ZONE") : {
                        ASSERTV(stream.readBE32() == 0x00000002);
                        uint16 **ptr;

                        switch (chunkType) {
                            case CHUNK("GND_ZONE") : ptr = zones[0].ground1 ? &zones[1].ground1 : &zones[0].ground1; break;
                            case CHUNK("GND_ZON2") : ptr = zones[0].ground2 ? &zones[1].ground2 : &zones[0].ground2; break;
                            case CHUNK("FLY_ZONE") : ptr = zones[0].fly     ? &zones[1].fly     : &zones[0].fly;     break;
                            default                : ptr = NULL;
                        }
                        
                        ASSERT(ptr != NULL);
                        ASSERT(*ptr == NULL);

                        int32 count = stream.readBE32();
                        *ptr = count ? new uint16[count] : NULL;
                        for (int i = 0; i < count; i++)
                            (*ptr)[i] = stream.readBE16();
                        break;
                    }
                    case CHUNK("ARANGES ") : {
                        ASSERTV(stream.readBE32() == 0x00000008);
                        animTexturesCount = stream.readBE32();
                        animTextures = new AnimTexture[animTexturesCount];
                        for (int i = 0; i < animTexturesCount; i++) {
                            AnimTexture &animTex = animTextures[i];
                            uint32 first = stream.readBE32();
                            uint32 last  = stream.readBE32();
                            animTex.count    = last - first + 1;
                            animTex.textures = new uint16[animTex.count];
                            for (int j = 0; j < animTex.count; j++)
                                animTex.textures[j] = first + j;
                        }
                        break;
                    }
                    case CHUNK("ITEMDATA") : {
                        ASSERTV(stream.readBE32() == 0x00000014);
                        entitiesBaseCount = stream.readBE32();
                        entitiesCount = entitiesBaseCount + MAX_RESERVED_ENTITIES;
                        entities = new Entity[entitiesCount];
                        for (int i = 0; i < entitiesBaseCount; i++) {
                            Entity &e = entities[i];
                            e.type = Entity::Type(stream.readBE16());
                            e.room = stream.readBE16();
                            e.x    = stream.readBE32();
                            e.y    = stream.readBE32();
                            e.z    = stream.readBE32();
                            e.rotation.value = stream.readBE16();
                            e.intensity      = stream.readBE16();
                            e.intensity2     = stream.readBE16();
                            e.flags.value    = stream.readBE16();
                        }
                        break;
                    }
                    case CHUNK("ROOMEND ") :
                        ASSERTV(stream.readBE32() == 0x00000000);
                        ASSERTV(stream.readBE32() == 0x00000000);
                        prepare();
                        break;
                // SAD
                    case CHUNK("OBJFILE ") :
                        ASSERTV(stream.readBE32() == 0x00000000);
                        ASSERTV(stream.readBE32() == 0x00000020);
                        break;
                    case CHUNK("ANIMS   ") :
                        ASSERTV(stream.readBE32() == 0x00000022);
                        ASSERT(anims == NULL);
                        animsCount = stream.readBE32();
                        anims = animsCount ? new Animation[animsCount] : NULL;
                        for (int i = 0; i < animsCount; i++) {
                            Animation &anim = anims[i];
                            anim.frameOffset   = stream.readBE32();
                            anim.frameSize     = stream.read();
                            anim.frameRate     = stream.read();
                            anim.state         = stream.readBE16();
                            anim.speed.H       = stream.readBE16();
                            anim.speed.L       = stream.readBE16();
                            anim.accel.H       = stream.readBE16();
                            anim.accel.L       = stream.readBE16();
                            anim.frameStart    = stream.readBE16();
                            anim.frameEnd      = stream.readBE16();
                            anim.nextAnimation = stream.readBE16();
                            anim.nextFrame     = stream.readBE16();
                            anim.scCount       = stream.readBE16();
                            anim.scOffset      = stream.readBE16();
                            anim.acCount       = stream.readBE16();
                            anim.animCommand   = stream.readBE16();
                        }
                        break;
                    case CHUNK("CHANGES ") :
                        ASSERTV(stream.readBE32() == 0x00000008);
                        ASSERT(states == NULL);
                        statesCount = stream.readBE32();
                        states = statesCount ? new AnimState[statesCount] : NULL;
                        for (int i = 0; i < statesCount; i++) {
                            AnimState &state = states[i];
                            state.state         = stream.readBE16();
                            state.rangesCount   = stream.readBE16();
                            state.rangesOffset  = stream.readBE16();
                            ASSERTV(stream.readBE16() == state.rangesOffset); // dummy
                        }
                        break;
                    case CHUNK("RANGES \0") :
                        ASSERTV(stream.readBE32() == 0x00000008);
                        ASSERT(ranges == NULL);
                        rangesCount = stream.readBE32();
                        ranges = rangesCount ? new AnimRange[rangesCount] : NULL;
                        for (int i = 0; i < rangesCount; i++) {
                            AnimRange &range = ranges[i];
                            range.low           = stream.readBE16();
                            range.high          = stream.readBE16();
                            range.nextAnimation = stream.readBE16();
                            range.nextFrame     = stream.readBE16();
                        }
                        break;
                    case CHUNK("COMMANDS") :
                        ASSERTV(stream.readBE32() == 0x00000002);
                        ASSERT(commands == NULL);
                        commandsCount = stream.readBE32();
                        commands = commandsCount ? new int16[commandsCount] : NULL;
                        for (int i = 0; i < commandsCount; i++)
                            commands[i] = stream.readBE16();
                        break;
                    case CHUNK("ANIBONES") :
                        ASSERTV(stream.readBE32() == 0x00000004);
                        ASSERT(nodesData == NULL);
                        nodesDataSize = stream.readBE32();
                        nodesData = nodesDataSize ? new uint32[nodesDataSize] : NULL;
                        for (int i = 0; i < nodesDataSize; i++)
                            nodesData[i] = stream.readBE32();
                        break;
                    case CHUNK("ANIMOBJ ") :
                        ASSERTV(stream.readBE32() == 0x00000038);
                        ASSERT(models == NULL);
                        modelsCount = stream.readBE32();
                        models = modelsCount ? new Model[modelsCount] : NULL;
                        for (int i = 0; i < modelsCount; i++) {
                            Model &model = models[i];
                            ASSERTV(stream.readBE16() == 0x0000);
                            model.index     = i;
                            model.type      = Entity::Type(stream.readBE16());
                            model.mCount    = stream.readBE16();
                            model.mStart    = stream.readBE16();
                            model.node      = stream.readBE32();
                            model.frame     = stream.readBE32();
                            model.animation = stream.readBE16();
                            ASSERTV(stream.readBE16() == model.animation);
                        }
                        break;
                    case CHUNK("STATOBJ ") :
                        ASSERTV(stream.readBE32() == 0x00000020);
                        ASSERT(staticMeshes == NULL);
                        staticMeshesCount = stream.readBE32();
                        staticMeshes = staticMeshesCount ? new StaticMesh[staticMeshesCount] : NULL;
                        for (int i = 0; i < staticMeshesCount; i++) {
                            StaticMesh &mesh = staticMeshes[i];
                            mesh.id        = stream.readBE32();
                            mesh.mesh      = stream.readBE16();
                            mesh.vbox.minX = stream.readBE16();
                            mesh.vbox.maxX = stream.readBE16();
                            mesh.vbox.minY = stream.readBE16();
                            mesh.vbox.maxY = stream.readBE16();
                            mesh.vbox.minZ = stream.readBE16();
                            mesh.vbox.maxZ = stream.readBE16();
                            mesh.cbox.minX = stream.readBE16();
                            mesh.cbox.maxX = stream.readBE16();
                            mesh.cbox.minY = stream.readBE16();
                            mesh.cbox.maxY = stream.readBE16();
                            mesh.cbox.minZ = stream.readBE16();
                            mesh.cbox.maxZ = stream.readBE16();
                            mesh.flags     = stream.readBE16();
                        }
                        break;
                    case CHUNK("FRAMES  ") :
                        ASSERTV(stream.readBE32() == 0x00000002);
                        ASSERT(frameData == NULL);
                        frameDataSize = stream.readBE32();
                        frameData = frameDataSize ? new uint16[frameDataSize] : NULL;
                        for (int i = 0; i < frameDataSize; i++) 
                            frameData[i] = stream.readBE16();
                        break;
                    case CHUNK("MESHPTRS") :
                        ASSERTV(stream.readBE32() == 0x00000004);
                        ASSERT(meshOffsets == NULL);
                        meshOffsetsCount = stream.readBE32();
                        meshOffsets = meshOffsetsCount ? new int32[meshOffsetsCount] : NULL;
                        for (int i = 0; i < meshOffsetsCount; i++) 
                            meshOffsets[i] = stream.readBE32();
                        break;
                    case CHUNK("MESHDATA") :
                        ASSERTV(stream.readBE32() == 0x00000002);
                        ASSERT(meshData == NULL);
                        meshDataSize = stream.readBE32();
                        meshData = meshDataSize ? new uint16[meshDataSize] : NULL;
                        stream.raw(meshData, sizeof(uint16) * meshDataSize);
                        break;
                    case CHUNK("OTEXTINF") :
                        ASSERTV(stream.readBE32() == 0x00000010);
                        ASSERT(objectTextures == NULL);
                        objectTexturesCount = stream.readBE32();
                        objectTextures = objectTexturesCount ? new TextureInfo[objectTexturesCount * 5] : NULL;
                        for (int i = 0; i < objectTexturesCount; i++)
                            readObjectTex(stream, objectTextures[i], TEX_TYPE_OBJECT);
                        objectTexturesBaseCount = objectTexturesCount;
                        expandObjectTex(objectTextures, objectTexturesCount);
                        break;
                    case CHUNK("OTEXTDAT") : {
                        ASSERTV(stream.readBE32() == 0x00000001);
                        objectTexturesDataSize = stream.readBE32();
                        objectTexturesData = objectTexturesDataSize ? new uint8[objectTexturesDataSize] : NULL;
                        stream.raw(objectTexturesData, objectTexturesDataSize);
                        break;
                    }
                    case CHUNK("ITEXTINF") : {
                        ASSERTV(stream.readBE32() == 0x00000014);
                        itemTexturesCount = stream.readBE32();
                        itemTextures = itemTexturesCount ? new TextureInfo[itemTexturesCount * 5] : NULL;
                        for (int i = 0; i < itemTexturesCount; i++)
                            readObjectTex(stream, itemTextures[i], TEX_TYPE_ITEM);
                        itemTexturesBaseCount = itemTexturesCount;
                        expandObjectTex(itemTextures, itemTexturesCount);
                        break;
                    }
                    case CHUNK("ITEXTDAT") : {
                        ASSERTV(stream.readBE32() == 0x00000001);
                        itemTexturesDataSize = stream.readBE32();
                        itemTexturesData = itemTexturesDataSize ? new uint8[itemTexturesDataSize] : NULL;
                        stream.raw(itemTexturesData, itemTexturesDataSize);
                        break;
                    }
                    case CHUNK("OBJEND  ") :
                        ASSERTV(stream.readBE32() == 0x00000000);
                        ASSERTV(stream.readBE32() == 0x00000000);
                        break;
                // SPR
                    case CHUNK("SPRFILE ") :
                        ASSERTV(stream.readBE32() == 0x00000000);
                        ASSERTV(stream.readBE32() == 0x00000020);
                        break;
                    case CHUNK("SPRITINF") : {
                        ASSERTV(stream.readBE32() == 0x00000010);
                        spriteTexturesCount = stream.readBE32();
                        spriteTextures = spriteTexturesCount ? new TextureInfo[spriteTexturesCount] : NULL;
                        for (int i = 0; i < spriteTexturesCount; i++)
                            readSpriteTex(stream, spriteTextures[i]);
                        break;
                    }
                    case CHUNK("SPRITDAT") : {
                        ASSERTV(stream.readBE32() == 0x00000001);
                        spriteTexturesDataSize = stream.readBE32();
                        spriteTexturesData = spriteTexturesDataSize ? new uint8[spriteTexturesDataSize] : NULL;
                        stream.raw(spriteTexturesData, spriteTexturesDataSize);
                        break;
                    }
                    case CHUNK("OBJECTS ") : {
                        ASSERTV(stream.readBE32() == 0x00000000);
                        spriteSequencesCount = stream.readBE32();
                        spriteSequences = spriteSequencesCount ? new SpriteSequence[spriteSequencesCount] : NULL;
                        for (int i = 0; i < spriteSequencesCount; i++) {
                            SpriteSequence &s = spriteSequences[i];
                            ASSERTV(stream.readBE16() == 0);
                            s.type   = Entity::remap(version, Entity::Type(stream.readBE16()));
                            s.sCount = (s.type >= Entity::TR1_TYPE_MAX) ? 1 : -stream.readBE16();
                            s.sStart = stream.readBE16();
                            s.transp = 1;
                        }
                        break;
                    }
                    case CHUNK("SPRITEND") :
                        ASSERTV(stream.readBE32() == 0x00000000);
                        ASSERTV(stream.readBE32() == 0x00000000);
                        break;
                // SND
                    case CHUNK("SAMPLUT ") : {
                        ASSERTV(stream.readBE32() == 0x00000002);
                        int count = stream.readBE32();
                        soundsMap = new int16[count];
                        for (int i = 0; i < count; i++)
                            soundsMap[i] = stream.readBE16();
                        break;
                    }
                    case CHUNK("SAMPINFS") : {
                        ASSERTV(stream.readBE32() == 0x00000008);
                        soundsInfoCount = stream.readBE32();
                        soundsInfo = soundsInfoCount ? new SoundInfo[soundsInfoCount] : NULL;
                        for (int i = 0; i < soundsInfoCount; i++) {
                            SoundInfo &s = soundsInfo[i];
                            s.index       = stream.readBE16();
                            s.volume      = float(stream.readBE16()) / 0x7FFF;
                            s.chance      = float(stream.readBE16()) / 0xFFFF;
                            s.range       = 8 * 1024;
                            s.pitch       = 0.2f;
                            s.flags.value = stream.readBE16();
                        }
                        break;
                    }
                    case CHUNK("SAMPLE  ") : {
                        int32 index = stream.readBE32();
                        int32 size  = stream.readBE32();
                        ASSERT(index < soundOffsetsCount);
                        soundOffsets[index] = stream.pos - 4;
                        soundSize[index]    = size - 4; // trim last samples (clicks)
                        *((uint32*)(&soundData[soundOffsets[index]])) = FOURCC("SEGA"); // mark sample as SEGA PCM
                        stream.seek(size);
                        break;
                    }
                    case CHUNK("ENDFILE\0") :
                        ASSERTV(stream.readBE32() == 0x00000000);
                        ASSERTV(stream.readBE32() == 0x00000000);
                        break;
                    default : {
                        char buf[9];
                        memcpy(buf, &chunkType, 8);
                        buf[8] = 0;
                        LOG("! unknown chunk type: \"%s\"\n", buf);
                        ASSERT(false);
                        break;
                    }
                }
            }
        #endif
        }

        void readCIN(Stream &stream) {
            stream.seek(2); // skip unknown word
            cameraFramesCount = (stream.size - 2) / 16;
            cameraFrames = cameraFramesCount ? new CameraFrame[cameraFramesCount] : NULL;
            stream.raw(cameraFrames, cameraFramesCount * 16);
        }

        void appendObjectTex(TextureInfo *&objTex, int32 &count) {
            if (!objTex) return;
            TextureInfo *newObjectTextures = new TextureInfo[objectTexturesCount + count];
            memcpy(newObjectTextures,                       objectTextures,   sizeof(TextureInfo) * objectTexturesCount);
            memcpy(newObjectTextures + objectTexturesCount, objTex,           sizeof(TextureInfo) * count);
            delete[] objectTextures;
            objectTextures       = newObjectTextures;
            objectTexturesCount += count;

            delete[] objTex;
            objTex = NULL;
            count  = 0;
        }

        void expandObjectTex(TextureInfo *textures, int32 &count) {
            for (int i = 0; i < count; i++)
                for (int j = 0; j < 4; j++) {
                    int idx = count * (1 + j) + i;
                    textures[idx] = textures[i];
                    textures[idx].tile = textures[idx].sub[j];
                }
            count *= 5;
        }

        int getItemTexureByIndex(uint16 index) {
            for (int i = 0; i < itemTexturesCount; i++)
                if (itemTextures[i].index == index)
                    return i;
            ASSERT(false);
            return 0;
        }

        void prepare() {
            if (version == VER_TR1_PC) {
            // Amiga -> PC color palette for TR1 PC
                ASSERT(palette);
                Color24 *c = palette;
                for (int i = 0; i < 256; i++) {
                    c->r <<= 2;
                    c->g <<= 2;
                    c->b <<= 2;
                    c++;
                }
            }

            for (int i = 0; i < modelsCount; i++) {
                Model &model = models[i];
                model.type = Entity::remap(version, model.type);

                for (int j = 0; j < model.mCount; j++)
                    initMesh(model.mStart + j, model.type);
            }

            for (int i = 0; i < staticMeshesCount; i++)
                initMesh(staticMeshes[i].mesh);

            remapMeshOffsetsToIndices();

            delete[] meshData;
            meshData = NULL;

            LOG("meshes: %d\n", meshesCount);

            for (int i = 0; i < entitiesBaseCount; i++) {
                Entity &e = entities[i];
                e.type = Entity::remap(version, e.type);

                e.controller = NULL;
                e.modelIndex = 0;

            // turn off interpolation for some entities
                e.flags.smooth = !((id == LVL_TR2_CUT_1 && (e.type == Entity::CUT_6 || e.type == Entity::CUT_8 || e.type == Entity::CUT_9))
                                    || e.type == Entity::SWITCH_BUTTON);
            }

            for (int i = entitiesBaseCount; i < entitiesCount; i++)
                entities[i].controller = NULL;

            if (isCutsceneLevel()) {
                for (int i = 0; i < entitiesBaseCount; i++) {
                    Entity &e = entities[i];
                    if ((((version & VER_TR1)) && e.type == Entity::CUT_1) || 
                        (((version & (VER_TR2 | VER_TR3))) && e.isLara())) {
                        cutEntity = i;
                        break;
                    }
                }
            }

            if (roomTextures) {
                // remap texture indices for room faces
                for (int i = 0; i < roomsCount; i++) {
                    Room::Data &data = rooms[i].data;
                    for (int j = 0; j < data.fCount; j++)
                        data.faces[j].flags.texture += objectTexturesCount;
                }

                for (int i = 0; i < animTexturesCount; i++)
                    for (int j = 0; j < animTextures[i].count; j++)
                        animTextures[i].textures[j] += objectTexturesCount;

                appendObjectTex(roomTextures, roomTexturesCount);
            }

            if (itemTextures) {
                // remap texture indices for inventory items
                for (int i = 0; i < modelsCount; i++) {
                    Model &model = models[i];
                    if (Entity::isInventoryItem(model.type)) {
                        for (int j = model.mStart; j < model.mStart + model.mCount; j++) {
                            Mesh &mesh = meshes[meshOffsets[j]];
                            for (int k = 0; k < mesh.fCount; k++)
                                if (!mesh.faces[k].colored) {
                                    mesh.faces[k].flags.texture += objectTexturesCount;
                                }
                        }
                    }
                }

                appendObjectTex(itemTextures, itemTexturesCount);
            }

            if (version == VER_TR1_SAT) {
                for (int i = 0; i < objectTexturesCount; i++) {
                    if (objectTextures[i].tile == 1)
                        objectTextures[i].tile = 0xFFFF;    // mark as unused
                }
            }

            initRoomMeshes();
            initAnimTex();
            initExtra();
            initCutscene();
            initTextureTypes();

            gObjectTextures      = objectTextures;
            gSpriteTextures      = spriteTextures;
            gObjectTexturesCount = objectTexturesCount;
            gSpriteTexturesCount = spriteTexturesCount;
        }

        void readSamples(Stream &stream) {
            stream.read(soundData, soundDataSize = stream.size);

            int32 dataOffsets[512];
            int32 dataOffsetsCount = 0;
            int32 dataOffset = 0;

            while (dataOffset < soundDataSize) {
                ASSERT(FOURCC(soundData + dataOffset) == FOURCC("RIFF"));
                dataOffsets[dataOffsetsCount++] = dataOffset;
                dataOffset += FOURCC(soundData + dataOffset + 4) + 8; // add chunk size
            }

            for (int i = 0; i < soundOffsetsCount; i++)
                soundOffsets[i] = dataOffsets[soundOffsets[i]];
        }

        static void sfxLoadAsync(Stream *stream, void *userData) {
            if (!stream) {
                LOG("! can't load MAIN.SFX\n");
                return;
            }
            ((Level*)userData)->readSamples(*stream);
            delete stream;
        }


        void initExtra() {
        // get special models indices
            memset(&extra, 0xFF, sizeof(extra));

            for (int i = 0; i < modelsCount; i++) {
                Entity::Type type = models[i].type;

                switch (type) {
                    case Entity::MUZZLE_FLASH        : extra.muzzleFlash     = i; break;
                    case Entity::PUZZLE_DONE_1       : extra.puzzleDone[0]   = i; break;
                    case Entity::PUZZLE_DONE_2       : extra.puzzleDone[1]   = i; break;
                    case Entity::PUZZLE_DONE_3       : extra.puzzleDone[2]   = i; break;
                    case Entity::PUZZLE_DONE_4       : extra.puzzleDone[3]   = i; break;
                    case Entity::LARA_BRAID          : extra.braid           = i; break;
                    case Entity::LARA_SPEC           : extra.laraSpec        = i; break;
                    case Entity::LARA_SKIN           : extra.laraSkin        = i; break;
                    case Entity::CUT_1               : extra.meshSwap[0]     = i; break;
                    case Entity::CUT_2               : extra.meshSwap[1]     = i; break;
                    case Entity::CUT_3               : extra.meshSwap[2]     = i; break;

                    case Entity::INV_PASSPORT        : extra.inv.passport    = i; break;
                    case Entity::INV_PASSPORT_CLOSED : extra.inv.passport_closed = i; break;
                    case Entity::INV_MAP             : extra.inv.map         = i; break;
                    case Entity::INV_COMPASS         : extra.inv.compass     = i; break;
                    case Entity::INV_STOPWATCH       : extra.inv.stopwatch   = i; break;
                    case Entity::INV_HOME            : extra.inv.home        = i; break;
                    case Entity::INV_DETAIL          : extra.inv.detail      = i; break;
                    case Entity::INV_SOUND           : extra.inv.sound       = i; break;
                    case Entity::INV_CONTROLS        : extra.inv.controls    = i; break;
                    case Entity::INV_GAMMA           : extra.inv.gamma       = i; break;
                    case Entity::INV_EXPLOSIVE       : extra.inv.explosive   = i; break;

                    case Entity::LARA_PISTOLS        :
                    case Entity::LARA_SHOTGUN        :
                    case Entity::LARA_MAGNUMS        :
                    case Entity::LARA_UZIS           : extra.weapons[type]     = i; break;

                    case Entity::INV_PISTOLS         :
                    case Entity::INV_SHOTGUN         :
                    case Entity::INV_MAGNUMS         :
                    case Entity::INV_UZIS            : extra.inv.weapons[type] = i; break;

                    case Entity::INV_AMMO_PISTOLS    :
                    case Entity::INV_AMMO_SHOTGUN    :
                    case Entity::INV_AMMO_MAGNUMS    :
                    case Entity::INV_AMMO_UZIS       : extra.inv.ammo[type]  = i; break;

                    case Entity::INV_MEDIKIT_SMALL   : extra.inv.medikit[0]  = i; break;
                    case Entity::INV_MEDIKIT_BIG     : extra.inv.medikit[1]  = i; break;

                    case Entity::INV_PUZZLE_1        : extra.inv.puzzle[0]   = i; break;
                    case Entity::INV_PUZZLE_2        : extra.inv.puzzle[1]   = i; break;
                    case Entity::INV_PUZZLE_3        : extra.inv.puzzle[2]   = i; break;
                    case Entity::INV_PUZZLE_4        : extra.inv.puzzle[3]   = i; break;

                    case Entity::INV_KEY_ITEM_1      : extra.inv.key[0]      = i; break;
                    case Entity::INV_KEY_ITEM_2      : extra.inv.key[1]      = i; break;
                    case Entity::INV_KEY_ITEM_3      : extra.inv.key[2]      = i; break;
                    case Entity::INV_KEY_ITEM_4      : extra.inv.key[3]      = i; break;
                                                                         
                    case Entity::INV_LEADBAR         : extra.inv.leadbar     = i; break;
                    case Entity::INV_SCION           : extra.inv.scion       = i; break;

                    case Entity::SKY                 : extra.sky = i; break;

                    default : ;
                }
            }
                
            for (int i = 0; i < spriteSequencesCount; i++)
                switch (spriteSequences[i].type) {
                    case Entity::SMOKE        : extra.smoke        = i; break;
                    case Entity::WATER_SPLASH : extra.waterSplash  = i; break;
                    case Entity::GLYPHS       : extra.glyphs       = i; break;
                    default : ;
                }

            ASSERT(extra.glyphs != -1);
        }

        void initCutscene() {
            if (id == LVL_TR3_CUT_2 || id == LVL_TR3_CUT_6)
                cutEntity = 1; // TODO TR3
        // init cutscene transform
            cutMatrix.identity();
            if (cutEntity > -1) {
                Entity &e = entities[cutEntity];
                switch (id) { // HACK to detect cutscene level number
                    case LVL_TR1_CUT_1 :
                        cutMatrix.translate(vec3(36668, float(e.y), 63180));
                        cutMatrix.rotateY(-23312.0f / float(0x4000) * PI * 0.5f);
                        break;
                    case LVL_TR1_CUT_2 :
                        cutMatrix.translate(vec3(51962, float(e.y), 53760));
                        cutMatrix.rotateY(16380.0f / float(0x4000) * PI * 0.5f);
                        break;
                    case LVL_TR1_CUT_3 :
                        flipMap();
                    case LVL_TR1_CUT_4 :
                        cutMatrix.translate(vec3(float(e.x), float(e.y), float(e.z)));
                        cutMatrix.rotateY(PI * 0.5f);
                        break;
                    default :
                        cutMatrix.translate(vec3(float(e.x), float(e.y), float(e.z)));
                        cutMatrix.rotateY(e.rotation);
                }
            }
        }

        void initTextureTypes() {
            // rooms geometry
            for (int roomIndex = 0; roomIndex < roomsCount; roomIndex++) {
                Room       &room = rooms[roomIndex];
                Room::Data &data = room.data;
                for (int i = 0; i < data.fCount; i++) {
                    Face &f = data.faces[i];
                    ASSERT(!f.colored);
                    ASSERT(f.flags.texture < objectTexturesCount);
                    TextureInfo &t = objectTextures[f.flags.texture];
                    t.type = TEX_TYPE_ROOM;
                }
            }

            // rooms static meshes
            for (int staticMeshIndex = 0; staticMeshIndex < staticMeshesCount; staticMeshIndex++) {
                StaticMesh *staticMesh = &staticMeshes[staticMeshIndex];
                if (!meshOffsets[staticMesh->mesh]) continue;
                Mesh &mesh = meshes[meshOffsets[staticMesh->mesh]];

                for (int i = 0; i < mesh.fCount; i++) {
                    Face &f = mesh.faces[i];
                    ASSERT(f.colored || f.flags.texture < objectTexturesCount);
                    if (f.colored) continue;
                    TextureInfo &t = objectTextures[f.flags.texture];
                    t.type = TEX_TYPE_ROOM;
                }
            }

            // animated textures
            for (int animTextureIndex = 0; animTextureIndex < animTexturesCount; animTextureIndex++) {
                AnimTexture &animTex = animTextures[animTextureIndex];

                for (int j = 0; j < animTex.count; j++) {
                    TextureInfo &t = objectTextures[animTex.textures[j]];
                    t.type = TEX_TYPE_ROOM;
                }
            }

            // objects (duplicate all textures that are simultaneously used for static meshes)
            TextureInfo *dupObjTex = NULL;
            int         dupObjTexCount = 0;

            for (int modelIndex = 0; modelIndex < modelsCount; modelIndex++) {
                Model &model = models[modelIndex];
                for (int meshIndex = model.mStart; meshIndex < model.mStart + model.mCount; meshIndex++) {
                    Mesh &mesh = meshes[meshOffsets[meshIndex]];

                    for (int i = 0; i < mesh.fCount; i++) {
                        Face &f = mesh.faces[i];
                        ASSERT(f.colored || f.flags.texture < objectTexturesCount);
                        if (f.colored) continue;
                        TextureInfo &t = objectTextures[f.flags.texture];
                        if (t.type != TEX_TYPE_OBJECT) {
                            if (!dupObjTex) {
                                dupObjTex = new TextureInfo[128];
                            }

                            int index = 0;
                            while (index < dupObjTexCount) {
                                TextureInfo &ot = dupObjTex[index];
                                if (ot.tile == t.tile && ot.clut == t.clut && ot.texCoord[0] == t.texCoord[0] && ot.texCoord[1] == t.texCoord[1])
                                    break;
                                index++;
                            }

                            if (index == dupObjTexCount) {
                                ASSERT(index <= 128);
                                dupObjTex[dupObjTexCount] = t;
                                dupObjTex[dupObjTexCount].type = TEX_TYPE_OBJECT;
                                dupObjTexCount++;
                            }

                            f.flags.texture = objectTexturesCount + index;
                        }
                    }
                }
            }

            appendObjectTex(dupObjTex, dupObjTexCount);
        }

        static Entity::Type convToInv(Entity::Type type) {
            for (int j = 0; j < COUNT(ITEM_TO_INV); j++)
                if (type == ITEM_TO_INV[j].src) {
                    return ITEM_TO_INV[j].dst;
                }
            return type;
        }

        static Entity::Type convFromInv(Entity::Type type) {
            for (int j = 0; j < COUNT(ITEM_TO_INV); j++)
                if (type == ITEM_TO_INV[j].dst) {
                    return ITEM_TO_INV[j].src;
                }
            return type;
        }

        void initModelIndices() {
            for (int i = 0; i < entitiesCount; i++) {
                TR::Entity &e = entities[i];
                e.modelIndex = getModelIndex(e.type);
            }
        }

        LevelID getTitleId() const {
            return TR::getTitleId(version);
        }

        LevelID getHomeId() const {
            return TR::getHomeId(version);
        }

        LevelID getStartId() const {
            return TR::getStartId(version);
        }

        LevelID getEndId() const {
            return TR::getEndId(version);
        }

        bool isTitle() const {
            return id == getTitleId();
        }

        bool isHome() const {
            return id == getHomeId();
        }

        bool isEnd() const {
            return id == getEndId();
        }

        bool isCutsceneLevel() const {
            return TR::isCutsceneLevel(id);
        }

        void readFace(Stream &stream, Face &f, bool colored, bool triangle) {
            f.triangle = triangle;

            for (int i = 0; i < (triangle ? 3 : 4); i++)
                stream.read(f.vertices[i]);

            if (triangle)
                f.vertices[3] = 0;

            stream.read(f.flags.value);

            f.colored = colored;
            f.water = false;
            f.flip  = false;
        }

        void readRoom(Stream &stream, int roomIndex) {
            Room &r = rooms[roomIndex];
            Room::Data &d = r.data;
        // room info
            stream.read(r.info);
        // room data
            stream.read(d.size);
            int startOffset = stream.pos;
            if (version == VER_TR1_PSX) stream.seek(2);

            // only for TR3 PSX
            int32 partsCount;
            int32 parts[16];

            if (version == VER_TR3_PSX) {
                d.vCount = d.tCount = d.rCount = d.fCount = 0;
            // pre-calculate number of vertices, triangles and rectangles for TR3 PSX format
                stream.read(partsCount);
                stream.raw(parts, sizeof(parts));

                for (int pIndex = 0; pIndex < partsCount; pIndex++) {
                    stream.setPos(startOffset + parts[pIndex] + 6);

                    uint8 tmpV, tmpT;
                    d.vCount += stream.read(tmpV);
                    d.tCount += stream.read(tmpT);

                    stream.seek(tmpV * 4 + tmpT * 4); // skip vertices and triangles

                    int q = 0;
                    uint32 info;
                    while (1) {
                        if (!q) {
                            stream.read(info);
                            q = 3;
                        }
                        uint32 texture = info & 0x03FF;
                        if (texture == 0x03FF) // ending flag
                            break;
                        info >>= 10;
                        stream.seek(4); // skip rectangle indices
                        d.rCount++;
                        q--;
                    }
                }

                ASSERT(int(d.size * 2) >= stream.pos - startOffset);
                stream.setPos(startOffset + d.size * 2);

                d.fCount   = d.rCount + d.tCount;
                d.faces    = d.fCount ? new Face[d.fCount] : NULL;
                d.vertices = d.vCount ? new Room::Data::Vertex[d.vCount] : NULL;

                d.vCount = d.fCount = 0;
            } else
                d.vertices = stream.read(d.vCount) ? new Room::Data::Vertex[d.vCount] : NULL;

            if (version == VER_TR3_PSX) {
                for (int pIndex = 0; pIndex < partsCount; pIndex++) {
                    stream.setPos(startOffset + parts[pIndex] + 4);

                    uint8 rIndex, rFlags;
                    stream.read(rIndex); // room index
                    stream.read(rFlags); // room flags

                    ASSERT(rIndex == roomIndex);
                    r.flags.value = rFlags;

                    uint8 vCount, tCount;
                    stream.read(vCount);
                    stream.read(tCount);

                    int32 vStart = d.vCount;
                // read vertices
                    for (uint8 i = 0; i < vCount; i++) {
                        Room::Data::Vertex &v = d.vertices[d.vCount++];

                        struct {
                            uint32 z:5, y:5, x:5, color:15, flags:2;
                        } cv;

                        stream.raw(&cv, sizeof(cv));
                        //ASSERT(cv.attribute == 0);
                        v.pos.x      = (cv.x << 10);
                        v.pos.y      = (cv.y << 8) + r.info.yTop;
                        v.pos.z      = (cv.z << 10);
                        v.attributes = 0;
                        v.color      = Color16(cv.color);
                    }

                // read triangles
                    for (uint8 i = 0; i < tCount; i++) {
                        struct {
                            uint32 i0:7, i1:7, i2:7, texture:10, unknown:1;
                        } t;

                        stream.raw(&t, sizeof(t));

                        ASSERT(t.unknown == 0);

                        Face &f = d.faces[d.fCount++];
                        f.flags.texture     = t.texture;
                        f.flags.doubleSided = false;
                        f.triangle    = true;
                        f.colored     = false;
                        f.water       = false;
                        f.flip        = false;
                        f.vertices[0] = vStart + t.i0;
                        f.vertices[1] = vStart + t.i1;
                        f.vertices[2] = vStart + t.i2;
                        f.vertices[3] = 0;
                        ASSERT(f.vertices[0] < d.vCount);
                        ASSERT(f.vertices[1] < d.vCount);
                        ASSERT(f.vertices[2] < d.vCount);
                    }
                // read rectangles
                    int q = 0;
                    uint32 info;
                    while (1) {
                        if (!q) {
                            stream.read(info);
                            q = 3;
                        }

                        uint32 texture = info & 0x03FF;
                        if (texture == 0x3FF)
                            break;
                        info >>= 10;

                        struct {
                            uint32 i0:7, i1:7, i2:7, i3:7, unknown:4;
                        } r;
                        stream.raw(&r, sizeof(r));

                        ASSERT(r.unknown == 0);

                        Face &f = d.faces[d.fCount++];
                        f.flags.texture     = texture;
                        f.flags.doubleSided = false;
                        f.triangle    = false;
                        f.colored     = false;
                        f.water       = false;
                        f.flip        = false;
                        f.vertices[0] = vStart + r.i0;
                        f.vertices[1] = vStart + r.i1;
                        f.vertices[2] = vStart + r.i2;
                        f.vertices[3] = vStart + r.i3;

                        ASSERT(f.vertices[0] < d.vCount);
                        ASSERT(f.vertices[1] < d.vCount);
                        ASSERT(f.vertices[2] < d.vCount);
                        ASSERT(f.vertices[3] < d.vCount);

                        q--;
                    }
                }
            } else {
                for (int i = 0; i < d.vCount; i++) {
                    Room::Data::Vertex &v = d.vertices[i];

                    uint16 lighting;

                    if (version == VER_TR2_PSX) {
                        struct {
                            uint32 lighting:8, attributes:8, z:5, y:5, x:5, w:1;
                        } cv;

                        stream.raw(&cv, sizeof(cv));

                        v.pos.x       = (cv.x << 10);
                        v.pos.y       = (cv.y << 8) + r.info.yTop;
                        v.pos.z       = (cv.z << 10);
                        lighting      = cv.lighting;
                        v.attributes  = cv.attributes;

                        lighting = 0x1FFF - (lighting << 5); // TODO: calc lighting by lighting = [0..255] and mode = [0..31] values
                    } else {
                        stream.read(v.pos.x);
                        stream.read(v.pos.y);
                        stream.read(v.pos.z);
                        stream.read(lighting);

                        if (version == VER_TR2_PC || version == VER_TR3_PC)
                            stream.read(v.attributes);
                   
                        if (version == VER_TR2_PC)
                            stream.read(lighting); // real lighting value

                        if (version == VER_TR3_PC) {
                            Color16 color;
                            stream.read(color.value);
                            v.color = color.getBGR();
                        }
                    }

                    if (version == VER_TR1_PSX || version == VER_TR3_PSX)
                        lighting = 0x1FFF - (lighting << 5); // convert vertex luminance from PSX to PC format

                    if ((version & VER_VERSION) < VER_TR3) { // lighting to color conversion
                        int value = clamp((lighting > 0x1FFF) ? 255 : (255 - (lighting >> 5)), 0, 255);
                        v.color.r = v.color.g = v.color.b = value;
                        v.color.a = 255;
                    }
                }

                if (version == VER_TR2_PSX)
                    stream.seek(2);

                int tmp = stream.pos;
                if (version == VER_TR2_PSX) {
                    stream.read(d.rCount);
                    stream.seek(sizeof(uint16) * d.rCount);
                    if ((stream.pos - startOffset) % 4) stream.seek(2);
                    stream.seek(sizeof(uint16) * 4 * d.rCount);
                } else
                    stream.seek(stream.read(d.rCount) * FACE4_SIZE); // uint32 colored (not existing in file)
                stream.read(d.tCount);
                stream.setPos(tmp);

                d.fCount = d.rCount + d.tCount;
                d.faces  = d.fCount ? new Face[d.fCount] : NULL;

                int idx = 0;

                int16 tmpCount;
                stream.read(tmpCount);
                ASSERT(tmpCount == d.rCount);

                if (version == VER_TR2_PSX) {
                    for (int i = 0; i < d.rCount; i++)
                        stream.raw(&d.faces[i].flags.value, sizeof(uint16));
                    if ((stream.pos - startOffset) % 4) stream.seek(2);
                    for (int i = 0; i < d.rCount; i++) {
                        Face &f = d.faces[i];
                        stream.raw(f.vertices, sizeof(uint16) * 4);
                        f.vertices[0] >>= 2;
                        f.vertices[1] >>= 2;
                        f.vertices[2] >>= 2;
                        f.vertices[3] >>= 2;
                        f.triangle = false;
                        f.colored  = false;
                        f.water    = false;
                        f.flip     = false;
                    }
                } else
                    for (int i = 0; i < d.rCount; i++) readFace(stream, d.faces[idx++], false, false);

                stream.read(tmpCount);
                ASSERT(tmpCount == d.tCount);

                if (version == VER_TR2_PSX) {
                    stream.seek(2);
                    for (int i = 0; i < d.tCount; i++) {
                        Face &f = d.faces[d.rCount + i];
                        stream.raw(&f.flags.value, sizeof(uint16));
                        stream.raw(f.vertices, sizeof(uint16) * 3);
                        f.vertices[0] >>= 2;
                        f.vertices[1] >>= 2;
                        f.vertices[2] >>= 2;
                        f.vertices[3] = 0;
                        f.triangle = true;
                        f.colored  = false;
                        f.water    = false;
                        f.flip     = false;
                    }
                } else {
                    for (int i = 0; i < d.tCount; i++)
                        readFace(stream, d.faces[idx++], false, true);
                }
            }

            if (version & VER_PSX) { // swap indices (quad strip -> quad list) only for PSX version
                for (int j = 0; j < d.fCount; j++)
                    if (!d.faces[j].triangle)
                        swap(d.faces[j].vertices[2], d.faces[j].vertices[3]);
            }

        // room sprites
            if (version == VER_TR2_PSX || version == VER_TR3_PSX) { // there is no room sprites
                d.sprites = NULL;
                d.sCount  = 0;
            } else
                stream.read(d.sprites, stream.read(d.sCount));

            if (version == VER_TR3_PSX)
                if (partsCount != 0)
                    stream.seek(4); // skip unknown shit

            ASSERT(int(d.size * 2) >= stream.pos - startOffset);
            stream.setPos(startOffset + d.size * 2);

        // portals
            stream.read(r.portals, stream.read(r.portalsCount));

            if (version == VER_TR2_PSX || version == VER_TR3_PSX) {
                for (int i = 0; i < r.portalsCount; i++) {
                    r.portals[i].vertices[0].y += r.info.yTop;
                    r.portals[i].vertices[1].y += r.info.yTop;
                    r.portals[i].vertices[2].y += r.info.yTop;
                    r.portals[i].vertices[3].y += r.info.yTop;
                }
            }

        // sectors
            stream.read(r.zSectors);
            stream.read(r.xSectors);
            r.sectors = (r.zSectors * r.xSectors > 0) ? new Room::Sector[r.zSectors * r.xSectors] : NULL;

            for (int i = 0; i < r.zSectors * r.xSectors; i++) {
                Room::Sector &s = r.sectors[i];

                stream.read(s.floorIndex);
                stream.read(s.boxIndex);
                stream.read(s.roomBelow);
                stream.read(s.floor);
                stream.read(s.roomAbove);
                stream.read(s.ceiling);

                if (version & (VER_TR1 | VER_TR2)) {
                    s.material = 0;
                } else {
                    s.material = s.boxIndex & 0x0F; 
                    s.boxIndex = s.boxIndex >> 4;
                    if (s.boxIndex == 2047) 
                        s.boxIndex = 0; // TODO TR3 slide box indices
                }
            }
        // ambient light luminance
            stream.read(r.ambient);

            if (version != VER_TR3_PSX) {
                if (version & (VER_TR2 | VER_TR3))
                    stream.read(r.ambient2);

                if (version & VER_TR2)
                    stream.read(r.lightMode);
            } else {
                r.ambient = 0x1FFF - r.ambient;
                stream.read(r.ambient2);
            }

        // lights
            r.lights = stream.read(r.lightsCount) ? new Room::Light[r.lightsCount] : NULL;
            for (int i = 0; i < r.lightsCount; i++) {
                Room::Light &light = r.lights[i];
                stream.read(light.x);
                stream.read(light.y);
                stream.read(light.z);

                uint16 intensity;

                if (version & VER_TR3)
                    stream.read(light.color);

                stream.read(intensity);

                if (version == VER_TR1_PSX)
                    stream.seek(2);

                if (version & (VER_TR2 | VER_TR3))
                    stream.seek(2); // intensity2

                stream.read(light.radius);

                if (version & VER_TR2)
                    stream.seek(4); // radius2

                if ((version & VER_VERSION) < VER_TR3) {
                    int value = clamp((intensity > 0x1FFF) ? 0 : (intensity >> 5), 0, 255);
                    light.color.r = light.color.g = light.color.b = value;
                    light.color.a = 0;
                }

                light.intensity = intensity;

                if (version == VER_TR3_PSX)
                    light.radius >>= 2;

                light.radius *= 2;
            }
        // meshes
            stream.read(r.meshesCount);
            r.meshes = r.meshesCount ? new Room::Mesh[r.meshesCount] : NULL;
            for (int i = 0; i < r.meshesCount; i++) {
                Room::Mesh &m = r.meshes[i];
                stream.read(m.x);
                stream.read(m.y);
                stream.read(m.z);
                stream.read(m.rotation);
                if (version & VER_TR3) {
                    Color16 color;
                    stream.read(color.value);
                    m.color = color;
                    stream.seek(2);
                } else {
                    if (version & VER_TR2)
                        stream.seek(2);

                    uint16 intensity;
                    stream.read(intensity);
                    if ((version & VER_VERSION) < VER_TR3) {
                        int value = clamp((intensity > 0x1FFF) ? 255 : (255 - (intensity >> 5)), 0, 255);
                        m.color.r = m.color.g = m.color.b = value;
                        m.color.a = 0;
                    }
                }

                stream.read(m.meshID);
                if (version == VER_TR1_PSX)
                    stream.seek(2); // skip padding
            }

        // misc flags
            stream.read(r.alternateRoom);
            stream.read(r.flags.value);
            if (version & VER_TR3) {
                stream.read(r.waterScheme);
                stream.read(r.reverbType);
            }

            r.dynLightsCount = 0;
        }

        void initMesh(int mIndex, Entity::Type type = Entity::NONE) {
            int offset = meshOffsets[mIndex];
            for (int i = 0; i < meshesCount; i++)
                if (meshes[i].offset == offset)
                    return;

            Stream stream(NULL, &meshData[offset / 2], 1024 * 1024);

            Mesh &mesh = meshes[meshesCount++];
            mesh.offset = offset;

            uint32 fOffset = 0xFFFFFFFF;

            if (type == Entity::SKY && version == VER_TR3_PSX) {
                mesh.center.x    = 
                mesh.center.y    =
                mesh.center.z    =
                mesh.radius      =
                mesh.tCount      = 0;
                mesh.flags.value = 0x80;
                stream.read(mesh.vCount);
                stream.read(mesh.rCount);
            } else {
                stream.read(mesh.center);
                stream.read(mesh.radius);

                if (version == VER_TR3_PSX) {
                    uint8  tmp;
                    uint16 tmpOffset;
                    mesh.vCount      = stream.read(tmp);
                    mesh.flags.value = stream.read(tmp);
                    stream.read(tmpOffset);
                    fOffset          = stream.pos + tmpOffset;
                } else {
                    stream.read(mesh.flags.value);
                    stream.read(mesh.vCount);
                }

                if (version == VER_TR1_SAT) {
                    mesh.center.x    = swap16(mesh.center.x);
                    mesh.center.y    = swap16(mesh.center.y);
                    mesh.center.z    = swap16(mesh.center.z);
                    mesh.radius      = swap16(mesh.radius);
                    mesh.flags.value = swap16(mesh.flags.value);
                    mesh.vCount      = swap16(mesh.vCount);
                }
            }

            switch (version) {
                case VER_TR1_SAT : {
                    mesh.vertices = new Mesh::Vertex[mesh.vCount];
                    for (int i = 0; i < mesh.vCount; i++) {
                        short4 &c = mesh.vertices[i].coord;
                        c.x = stream.readBE16();
                        c.y = stream.readBE16();
                        c.z = stream.readBE16();
                    }
                    int16 nCount = stream.readBE16();
                    ASSERT(mesh.vCount == abs(nCount));
                    for (int i = 0; i < mesh.vCount; i++) {
                        short4 &c = mesh.vertices[i].coord;
                        short4 &n = mesh.vertices[i].normal;
                        if (nCount > 0) { // normal
                            n.x = stream.readBE16();
                            n.y = stream.readBE16();
                            n.z = stream.readBE16();
                            n.w = 1;
                            c.w = 0x1FFF;
                        } else { // intensity
                            c.w = stream.readBE16();
                            n = short4( 0, 0, 0, 0 );
                        }
                    }

                    mesh.fCount = stream.readBE16();
                    mesh.faces = new Face[mesh.fCount];
                    mesh.tCount = mesh.rCount = 0;

                    enum {
                        TYPE_T_TEX_UNK        = 2,
                        TYPE_T_COLOR          = 4,
                        TYPE_T_TEX            = 8,
                        TYPE_T_TEX_TRANSP     = 16,
                        TYPE_R_COLOR          = 5,
                        TYPE_R_TEX            = 9,
                        TYPE_R_TRANSP         = 17,
                        TYPE_R_FLIP_TEX       = 49,
                        TYPE_R_FLIP_TRANSP    = 57,
                    };

                    TextureInfo   *textures;
                    int           texturesCount;
                    int           texturesBaseCount;

                    if (itemTexturesCount && Entity::isInventoryItem(type)) {
                        textures          = itemTextures;
                        texturesCount     = itemTexturesCount;
                        texturesBaseCount = itemTexturesBaseCount;
                    } else {
                        textures          = objectTextures;
                        texturesCount     = objectTexturesCount;
                        texturesBaseCount = objectTexturesBaseCount;
                    }

                    int fIndex = 0;
                    uint16 typesCount = stream.readBE16();
                    for (int j = 0; j < typesCount; j++) {
                        uint16 type  = stream.readBE16();
                        uint16 count = stream.readBE16();
                        for (int k = 0; k < count; k++) {
                            ASSERT(fIndex < mesh.fCount);
                            Face &f = mesh.faces[fIndex++];
                            f.water   = false;
                            f.colored = false;
                            f.flip    = 0;
                            switch (type) {
                                case TYPE_T_COLOR      : f.colored = true;
                                case TYPE_T_TEX_UNK    : 
                                case TYPE_T_TEX        :
                                case TYPE_T_TEX_TRANSP :
                                    f.triangle    = true;
                                    f.vertices[0] = (stream.readBE16() >> 5);
                                    f.vertices[1] = (stream.readBE16() >> 5);
                                    f.vertices[2] = (stream.readBE16() >> 5);
                                    f.vertices[3] = 0;
                                    f.flags.value = stream.readBE16();
                                    ASSERT(f.vertices[0] < mesh.vCount && f.vertices[1] < mesh.vCount && f.vertices[2] < mesh.vCount);
                                    mesh.tCount++;

                                    if (!f.colored) {
                                        union {
                                            struct { uint16 texture:12, flip:4; };
                                            uint16 value;
                                        } tri;

                                        tri.value = f.flags.value;
                                        f.flags.value = tri.texture;
                                        f.flip        = tri.flip;

                                        if (textures == itemTextures)
                                            f.flags.value = getItemTexureByIndex(f.flags.value);

                                        f.flags.value += texturesBaseCount;

                                        if (f.flip != 0 && f.flip != 2 && f.flip != 4 && f.flip != 6 && f.flip != 8 && f.flip != 10 && f.flip != 12 && f.flip != 14) {
                                            // TODO puma flip 14 bug
                                            ASSERT(false);
                                            f.colored = true;
                                            f.flags.value = 0x00FF;
                                        }

                                        if (f.flip == 6 || f.flip == 12) { // 3 bit
                                            f.flags.value += texturesBaseCount + texturesBaseCount + texturesBaseCount;
                                        }

                                        if (f.flip == 4 || f.flip == 14) {
                                            f.flags.value += texturesBaseCount + texturesBaseCount;
                                        }

                                        if (f.flip == 2 || f.flip == 8) {
                                            f.flags.value += texturesBaseCount;
                                        }
                                    }

                                    break;
                                case TYPE_R_COLOR       : f.colored = true;
                                case TYPE_R_FLIP_TEX    :
                                case TYPE_R_FLIP_TRANSP : f.flip = 1;
                                case TYPE_R_TEX         :
                                case TYPE_R_TRANSP      :
                                    f.triangle    = false;
                                    f.vertices[0] = (stream.readBE16() >> 5);
                                    f.vertices[1] = (stream.readBE16() >> 5);
                                    f.vertices[2] = (stream.readBE16() >> 5);
                                    f.vertices[3] = (stream.readBE16() >> 5);
                                    f.flags.value = stream.readBE16();
                                    ASSERT(f.vertices[0] < mesh.vCount && f.vertices[1] < mesh.vCount && f.vertices[2] < mesh.vCount && f.vertices[3] < mesh.vCount);
                                    mesh.rCount++;

                                    f.flip = (type == TYPE_R_FLIP_TEX || type == TYPE_R_FLIP_TRANSP);

                                    if (!f.colored) {
                                        ASSERT(f.flags.value % 16 == 0);
                                        f.flags.value /= 16;

                                        if (textures == itemTextures)
                                            f.flags.value = getItemTexureByIndex(f.flags.value);
                                    }

                                    break;
                                default :
                                    LOG("! unknown face type: %d\n", type);
                                    ASSERT(false);
                            }

                            ASSERT(f.colored || f.flags.value < texturesCount);

                            if (type == TYPE_R_TRANSP || type == TYPE_R_FLIP_TRANSP || type == TYPE_T_TEX_TRANSP)
                                textures[f.flags.texture].attribute = 1;

                        }
                    }
                    ASSERT(fIndex == mesh.fCount);
                    break;
                }
                case VER_TR1_PC :
                case VER_TR2_PC :
                case VER_TR3_PC : {
                /*  struct {
                        short3      center;
                        short2      collider;
                        short       vCount;
                        short3      vertices[vCount];
                        short       nCount;
                        short3      normals[max(0, nCount)];
                        ushort      luminance[-min(0, nCount)];
                        short       rCount;
                        Rectangle   rectangles[rCount];
                        short       tCount;
                        Triangle    triangles[tCount];
                        short       crCount;
                        Rectangle   crectangles[crCount];
                        short       ctCount;
                        Triangle    ctriangles[ctCount];
                    }; */
                    mesh.vertices = new Mesh::Vertex[mesh.vCount];
                    for (int i = 0; i < mesh.vCount; i++) {
                        short4 &c = mesh.vertices[i].coord;
                        stream.read(c.x);
                        stream.read(c.y);
                        stream.read(c.z);
                    }
                    int16 nCount;
                    stream.read(nCount);
                    ASSERT(mesh.vCount == abs(nCount));
                    for (int i = 0; i < mesh.vCount; i++) {
                        short4 &c = mesh.vertices[i].coord;
                        short4 &n = mesh.vertices[i].normal;
                        if (nCount > 0) { // normal
                            stream.read(n.x);
                            stream.read(n.y);
                            stream.read(n.z);
                            n.w = 1;
                            c.w = 0x1FFF;
                        } else { // intensity
                            stream.read(c.w);
                            n = short4( 0, 0, 0, 0 );
                        }
                    }

                    uint16 rCount, tCount, crCount, ctCount;

                    int tmp = stream.pos;
                    stream.seek(stream.read(rCount)  * FACE4_SIZE); // uint32 colored (not existing in file)
                    stream.seek(stream.read(tCount)  * FACE3_SIZE);
                    stream.seek(stream.read(crCount) * FACE4_SIZE);
                    stream.seek(stream.read(ctCount) * FACE3_SIZE);
                    stream.setPos(tmp);

                    mesh.rCount = rCount + crCount;
                    mesh.tCount = tCount + ctCount;
                    mesh.fCount = mesh.rCount + mesh.tCount;
                    mesh.faces  = mesh.fCount ? new Face[mesh.fCount] : NULL;

                    int idx = 0;
                    stream.seek(sizeof(rCount));  for (int i = 0; i < rCount; i++)  readFace(stream, mesh.faces[idx++], false, false);
                    stream.seek(sizeof(tCount));  for (int i = 0; i < tCount; i++)  readFace(stream, mesh.faces[idx++], false,  true);
                    stream.seek(sizeof(crCount)); for (int i = 0; i < crCount; i++) readFace(stream, mesh.faces[idx++],  true, false);
                    stream.seek(sizeof(ctCount)); for (int i = 0; i < ctCount; i++) readFace(stream, mesh.faces[idx++],  true,  true);
                    break;
                }
                case VER_TR1_PSX :
                case VER_TR2_PSX : {
                /*  struct {
                        short3      center;
                        short2      collider;
                        short       vCount;
                        short4      vertices[abs(vCount)];
                        short4      normals[max(0, vCount)];
                        ushort      luminance[-min(0, vCount)];
                        short       rCount;
                        Rectangle   rectangles[rCount];
                        short       tCount;
                        Triangle    triangles[tCount];
                    }; */
                    int nCount = mesh.vCount;
                    mesh.vCount = abs(mesh.vCount);
                    mesh.vertices = new Mesh::Vertex[mesh.vCount];

                    for (int i = 0; i < mesh.vCount; i++)
                        stream.read(mesh.vertices[i].coord);

                    for (int i = 0; i < mesh.vCount; i++) {
                        short4 &c = mesh.vertices[i].coord;
                        short4 &n = mesh.vertices[i].normal;
                        if (nCount > 0) { // normal
                            stream.read(n);
                            n.w = 1;
                            c.w = 0x1FFF;
                        } else { // intensity
                            stream.read(c.w);
                            n = short4( 0, 0, 0, 0 );
                        }
                    }

                    if ((version & VER_TR2) && nCount > 0) { // TODO probably for unused meshes only but need to check
                        uint16 crCount = 0, ctCount = 0;
                        stream.read(crCount);
                        stream.seek((FACE4_SIZE + 2) * crCount);
                        stream.read(ctCount);
                        stream.seek((FACE3_SIZE + 2) * ctCount);
                    }

                    int tmp = stream.pos;
                    stream.seek(stream.read(mesh.rCount) * FACE4_SIZE); // uint32 colored (not existing in file)
                    stream.seek(stream.read(mesh.tCount) * FACE3_SIZE);
                    stream.setPos(tmp);

                    mesh.fCount = mesh.rCount + mesh.tCount;
                    mesh.faces  = mesh.fCount ? new Face[mesh.fCount] : NULL;

                    int idx = 0;
                    stream.seek(sizeof(mesh.rCount)); for (int i = 0; i < mesh.rCount; i++) readFace(stream, mesh.faces[idx++], false, false);
                    stream.seek(sizeof(mesh.tCount)); for (int i = 0; i < mesh.tCount; i++) readFace(stream, mesh.faces[idx++], false,  true);

                    if (!mesh.fCount)
                        LOG("! warning: mesh %d has no geometry with %d vertices\n", meshesCount - 1, mesh.vCount);
                    //ASSERT(mesh.rCount != 0 || mesh.tCount != 0);
                        
                    for (int i = 0; i < mesh.fCount; i++) {
                        Face &f = mesh.faces[i];
                        #ifndef SPLIT_BY_TILE
                            f.colored = (f.flags.texture < 256) ? true : false;
                        #endif

                        if (version == VER_TR2_PSX) {
                            f.vertices[0] >>= 3;
                            f.vertices[1] >>= 3;
                            f.vertices[2] >>= 3;
                            f.vertices[3] >>= 3;
                        }
                    }

                    break;
                }
                case VER_TR3_PSX : {
                    if (!mesh.vCount) {
                        mesh.vertices = NULL;
                        mesh.faces    = NULL;
                        mesh.tCount   = mesh.rCount = mesh.fCount = 0;
                        break;
                    }

                    mesh.vertices = new Mesh::Vertex[mesh.vCount];

                    for (int i = 0; i < mesh.vCount; i++)
                        stream.read(mesh.vertices[i].coord);

                    if (mesh.flags.value & 0x80) { // static mesh without normals
                        for (int i = 0; i < mesh.vCount; i++)
                            mesh.vertices[i].normal = short4( 0, 0, 0, 0 );
                    } else {
                        for (int i = 0; i < mesh.vCount; i++)
                            stream.read(mesh.vertices[i].normal);
                    }

                    if (type != Entity::SKY) {
                        ASSERT(stream.pos == fOffset);

                        stream.setPos(fOffset);
                        stream.read(mesh.tCount);
                        stream.read(mesh.rCount);
                    }
                    mesh.fCount = mesh.rCount + mesh.tCount;
                    mesh.faces  = mesh.fCount ? new Face[mesh.fCount] : NULL;

                // read triangles
                    int idx = 0;
                    uint32 info;

                    for (int i = 0; i < mesh.tCount; i++) {
                        if (!(i % 4))
                            stream.read(info);

                        struct {
                            uint32 i0:8, i1:8, i2:8, tex:8;
                        } r;
                        stream.raw(&r, sizeof(r));

                        Face &f = mesh.faces[idx++];
                        f.flags.doubleSided = false;
                        f.flags.texture     = (info & 0xFF) | (r.tex << 8);
                        f.triangle = true;
                        f.colored  = false;
                        f.water    = false;
                        f.flip     = false;

                        f.vertices[0] = r.i0;
                        f.vertices[1] = r.i1;
                        f.vertices[2] = r.i2;
                        f.vertices[3] = 0;

                        info >>= 8;
                    }

                // read rectangles
                    for (int i = 0; i < mesh.rCount; i++) {
                        if (!(i % 2))
                            stream.read(info);
                            
                        Face &f = mesh.faces[idx++];
                        f.flags.doubleSided = false;
                        f.flags.texture     = info & 0xFFFF;
                        f.triangle = false;
                        f.colored  = false;
                        f.water    = false;
                        f.flip     = false;

                        struct {
                            uint32 i0:8, i1:8, i2:8, i3:8;
                        } r;
                        stream.raw(&r, sizeof(r));

                        f.vertices[0] = r.i0;
                        f.vertices[1] = r.i1;
                        f.vertices[2] = r.i3;
                        f.vertices[3] = r.i2;

                        info >>= 16;
                    }

                    break;
                }
                default : ASSERT(false);
            }

            #define RECALC_ZERO_NORMALS(mesh, face)\
                int fn = -1;\
                for (int j = 0; j < (face.triangle ? 3 : 4); j++) {\
                    Mesh::Vertex &v = mesh.vertices[face.vertices[j]];\
                    short4 &n = v.normal;\
                    if (!(n.x | n.y | n.z)) {\
                        if (fn > -1) {\
                            n = mesh.vertices[face.vertices[fn]].normal;\
                            continue;\
                        }\
                        vec3 o = mesh.vertices[face.vertices[0]].coord;\
                        vec3 a = o - mesh.vertices[face.vertices[1]].coord;\
                        vec3 b = o - mesh.vertices[face.vertices[2]].coord;\
                        o = b.cross(a).normal() * 16300.0f;\
                        n.x = (int)o.x;\
                        n.y = (int)o.y;\
                        n.z = (int)o.z;\
                        fn = j;\
                    }\
                }

        // recalc zero normals
            for (int i = 0; i < mesh.fCount; i++) {
                Face &f = mesh.faces[i];
                RECALC_ZERO_NORMALS(mesh, f);
            }

            #undef RECALC_ZERO_NORMALS
        }

        void remapMeshOffsetsToIndices() {
            for (int i = 0; i < meshOffsetsCount; i++) {
                int index = -1;
                for (int j = 0; j < meshesCount; j++)
                    if (meshes[j].offset == meshOffsets[i]) {
                        index = j;
                        break;
                    }
                meshOffsets[i] = index;
            }
        }

        void readObjectTex(Stream &stream, TextureInfo &t, TextureType type = TEX_TYPE_OBJECT) {
            #define SET_PARAMS(t, d, c) {\
                    t.clut        = c;\
                    t.tile        = d.tile;\
                    t.attribute   = d.attribute;\
                    t.animated    = false;\
                    t.texCoord[0] = t.texCoordAtlas[0] = short2( d.x0, d.y0 );\
                    t.texCoord[1] = t.texCoordAtlas[1] = short2( d.x1, d.y1 );\
                    t.texCoord[2] = t.texCoordAtlas[2] = short2( d.x2, d.y2 );\
                    t.texCoord[3] = t.texCoordAtlas[3] = short2( d.x3, d.y3 );\
                }

            t.type = t.dataType = type;

            switch (version) {
                case VER_TR1_SAT : {
                    struct {
                        uint16  attribute;
                        uint16  tile;
                        uint16  clut;
                        uint8   w, h;
                        uint8   x0, y0;
                        uint8   x1, y1;
                        uint8   x2, y2;
                        uint8   x3, y3;
                    } d;

                    t.index = 0;
                    if (type == TEX_TYPE_ITEM)
                        t.index = stream.readBE16();

                    d.attribute = 0;
                    d.tile   = stream.readBE16(); // offset to 4-bit indices
                    t.sub[0] = stream.readBE16();
                    t.sub[1] = stream.readBE16();
                    t.sub[2] = stream.readBE16();
                    t.sub[3] = stream.readBE16();
                    d.clut   = stream.readBE16(); // offset to color palette
                    d.w      = stream.read() * 8;
                    d.h      = stream.read();
                    t.i5     = stream.readBE16();

                    if (type == TEX_TYPE_ROOM) { // for room tiles we will use fullres TSUB textures instead of mips TQTR
                        d.w *= 2;
                        d.h *= 2;
                    }

                    if (type == TEX_TYPE_ITEM)
                        ASSERTV(stream.readBE16() == t.i5);

                    d.x0 = d.y0 = d.x3 = d.y1 = 0;
                    d.x1 = d.x2 = max(0, d.w - 1);
                    d.y2 = d.y3 = max(0, d.h - 1);

                    SET_PARAMS(t, d, d.clut);

                    //if (type == TEX_TYPE_ITEM) {
                    //    LOG("%d %d | %d %d %d %d | %d %d %dx%d | %d\n", t.index, t.tile, t.sub[0], t.sub[1], t.sub[2], t.sub[3], t.clut, t.i5, d.w, d.h, bi);
                    //}

                    break;
                }
                case VER_TR1_PC :
                case VER_TR2_PC :
                case VER_TR3_PC : {
                    struct {
                        uint16  attribute;
                        uint16  tile:14, :2;
                        uint8   xh0, x0, yh0, y0;
                        uint8   xh1, x1, yh1, y1;
                        uint8   xh2, x2, yh2, y2;
                        uint8   xh3, x3, yh3, y3;
                    } d;
                    stream.raw(&d, sizeof(d));
                    SET_PARAMS(t, d, 0);
                    break;
                }
                case VER_TR1_PSX : 
                case VER_TR2_PSX :
                case VER_TR3_PSX : {
                    struct {
                        uint8   x0, y0;
                        uint16  clut;
                        uint8   x1, y1;
                        uint16  tile:14, :2;
                        uint8   x2, y2;
                        uint16  unknown2;
                        uint8   x3, y3;
                        uint16  attribute; 
                    } d;
                    stream.raw(&d, sizeof(d));
                    if (version == VER_TR3_PSX) {
                        if (d.attribute == 0)
                            d.attribute = 0;
                        else if (d.attribute == 2)
                            d.attribute = 2;
                        else if (d.attribute == 0xFFFF)
                            d.attribute = 1;
                        else {
                            //ASSERT(false);
                            d.attribute = 0;
                        }
                    }
                    SET_PARAMS(t, d, d.clut);
                    break;
                }
                default : ASSERT(false);
            }

            #undef SET_PARAMS
        }

        void readObjectTex(Stream &stream) {
            objectTextures = stream.read(objectTexturesCount) ? new TextureInfo[objectTexturesCount] : NULL;
            for (int i = 0; i < objectTexturesCount; i++)
                readObjectTex(stream, objectTextures[i]);
        }

        void readSpriteTex(Stream &stream, TextureInfo &t) {
            #define SET_PARAMS(t, d, c) {\
                    t.clut = c;\
                    t.tile = d.tile;\
                    t.l    = d.l;\
                    t.t    = d.t;\
                    t.r    = d.r;\
                    t.b    = d.b;\
                }

            t.type      = TEX_TYPE_SPRITE;
            t.dataType  = TEX_TYPE_SPRITE;
            t.attribute = 1;

            switch (version) {
                case VER_TR1_SAT : {
                    struct {
                        uint16 tile;
                        uint16 clut;
                        uint8  w, h;
                        int16  l, t, r, b;
                    } d;
                    d.tile = stream.readBE16(); // offset to 4-bit indices
                    d.clut = stream.readBE16() - d.tile; // offset to 16-bit color table (16 colors)
                    d.w    = stream.read();     // texture width div 8
                    d.h    = stream.read();     // texture height
                    d.l    = stream.readBE16();
                    d.t    = stream.readBE16();
                    d.r    = stream.readBE16();
                    d.b    = stream.readBE16();
                    ASSERTV((int16)stream.readBE16() == d.b);
                    SET_PARAMS(t, d, d.clut);
                    t.texCoord[0] = t.texCoordAtlas[0] = short2( 0,                     0                );
                    t.texCoord[1] = t.texCoordAtlas[1] = short2( ((int16)d.w << 3) - 1, ((int16)d.h) - 1 );
                    break;
                }
                case VER_TR1_PC :
                case VER_TR2_PC :
                case VER_TR3_PC : {
                    struct {
                        uint16  tile;
                        uint8   u, v;
                        uint16  w, h;
                        int16   l, t, r, b;
                    } d;
                    stream.raw(&d, sizeof(d));
                    SET_PARAMS(t, d, 0);
                    t.texCoord[0] = t.texCoordAtlas[0] = short2( d.u,                       d.v                       );
                    t.texCoord[1] = t.texCoordAtlas[1] = short2( (uint8)(d.u + (d.w >> 8)), (uint8)(d.v + (d.h >> 8)) );
                    break;
                }
                case VER_TR1_PSX : 
                case VER_TR2_PSX :
                case VER_TR3_PSX : {
                    struct {
                        int16   l, t, r, b;
                        uint16  clut;
                        uint16  tile;
                        uint8   u0, v0;
                        uint8   u1, v1;
                    } d;
                    stream.raw(&d, sizeof(d));
                    SET_PARAMS(t, d, d.clut);
                    t.texCoord[0] = t.texCoordAtlas[0] = short2( d.u0, d.v0 );
                    t.texCoord[1] = t.texCoordAtlas[1] = short2( d.u1, d.v1 );
                    break;
                }
                default : ASSERT(false);
            }

            #undef SET_PARAMS
        }

        void readSpriteTex(Stream &stream) {
            spriteTextures = stream.read(spriteTexturesCount) ? new TextureInfo[spriteTexturesCount] : NULL;
            for (int i = 0; i < spriteTexturesCount; i++)
                readSpriteTex(stream, spriteTextures[i]);

            spriteSequences = stream.read(spriteSequencesCount) ? new SpriteSequence[spriteSequencesCount] : NULL;
            for (int i = 0; i < spriteSequencesCount; i++) {
                SpriteSequence &s = spriteSequences[i];
                uint16 type;
                stream.read(type);
                s.type = Entity::remap(version, Entity::Type(type));
                stream.read(s.unused);
                stream.read(s.sCount);
                stream.read(s.sStart);
                s.sCount = -s.sCount;
                s.transp = 1;
            }

        // remove unavailable sprites (check EGYPT.PHD)
            for (int roomIndex = 0; roomIndex < roomsCount; roomIndex++) {
                Room::Data &data = rooms[roomIndex].data;
                
                int i = 0;
                while (i < data.sCount)
                    if (data.sprites[i].vertexIndex >= data.vCount || data.sprites[i].texture >= spriteTexturesCount) {
                        LOG("! room %d has wrong sprite %d (v:%d/%d t:%d/%d)\n", roomIndex, i, data.sprites[i].vertexIndex, data.vCount, data.sprites[i].texture, spriteTexturesCount);
                        ASSERT(false);
                        data.sprites[i] = data.sprites[--data.sCount];
                    } else
                        i++;

                if (!data.sCount && data.sprites) {
                    delete[] data.sprites;
                    data.sprites = NULL;
                }
            }
        }

        void readAnimTex(Stream &stream) {
            uint32 size;
            stream.read(size);

            if (!size) return;

            stream.read(animTexturesCount);
            animTextures = animTexturesCount ? new AnimTexture[animTexturesCount] : NULL;

            for (int i = 0; i < animTexturesCount; i++) {
                AnimTexture &animTex = animTextures[i];
                animTex.count    = stream.readLE16() + 1;
                animTex.textures = new uint16[animTex.count];
                for (int j = 0; j < animTex.count; j++)
                    animTex.textures[j] = stream.readLE16();
            }
        }

        void readEntities(Stream &stream) {
            entitiesCount = stream.read(entitiesBaseCount) + MAX_RESERVED_ENTITIES;
            entities = new Entity[entitiesCount];
            for (int i = 0; i < entitiesBaseCount; i++) {
                Entity &e = entities[i];
                uint16 type;
                e.type = Entity::Type(stream.read(type));
                stream.read(e.room);
                stream.read(e.x);
                stream.read(e.y);
                stream.read(e.z);
                stream.read(e.rotation);
                stream.read(e.intensity);
                if (version & (VER_TR2 | VER_TR3))
                    stream.read(e.intensity2);
                stream.read(e.flags.value);
            }
        }

        void initRoomMeshes() {
            for (int i = 0; i < roomsCount; i++) {
                Room &room = rooms[i];
                for (int j = 0; j < room.meshesCount; j++)
                    room.meshes[j].meshIndex = getMeshByID(room.meshes[j].meshID);
            }
        }

        void initAnimTex() {
            for (int i = 0; i < animTexturesCount; i++) {
                uint8 transp = 0;
                for (int j = 0; j < animTextures[i].count; j++) {
                    TextureInfo &t = objectTextures[animTextures[i].textures[j]];
                    t.animated = true;
                    if (t.attribute != 0)
                        transp = t.attribute;
                }

                if (transp) {
                    for (int j = 0; j < animTextures[i].count; j++) {
                        TextureInfo &t = objectTextures[animTextures[i].textures[j]];
                        t.attribute = transp;
                    }
                }
            }
        }

        void shiftAnimTex() {
            for (int i = 0; i < animTexturesCount; i++) {
                AnimTexture &animTex = animTextures[i];
                TextureInfo tmp = objectTextures[animTex.textures[0]];
                for (int j = 0; j < animTex.count - 1; j++)
                    objectTextures[animTex.textures[j]] = objectTextures[animTex.textures[j + 1]];
                objectTextures[animTex.textures[animTex.count - 1]] = tmp;
            }
        }

        void fillObjectTexture32(Tile32 *dst, const Color32 *data, const short4 &uv, TextureInfo *t) {
            Color32 *ptr = &dst->color[uv.y * 256];
            for (int y = uv.y; y < uv.w; y++) {
                for (int x = uv.x; x < uv.z; x++) {
                    const Color32 &p = data[y * 256 + x];
                    ptr[x].r = p.r;
                    ptr[x].g = p.g;
                    ptr[x].b = p.b;
                    ptr[x].a = p.a;
                }
                ptr += 256;
            }
            premultiplyAlpha(dst->color, uv);
        }

        void fillObjectTexture(Tile32 *dst, const short4 &uv, TextureInfo *t) {
        // convert to RGBA
            switch (version) {
                case VER_TR1_SAT : {
                    uint32 iOffset = uint32(uint16(t->tile)) << 3;
                    uint32 cOffset = uint32(uint16(t->clut + t->tile)) << 3;

                    uint8 *data = NULL;
                    switch (t->dataType) {
                        case TEX_TYPE_ROOM   : data = roomTexturesData;   break;
                        case TEX_TYPE_ITEM   : data = itemTexturesData;   break;
                        case TEX_TYPE_OBJECT : data = objectTexturesData; break;
                        case TEX_TYPE_SPRITE : data = spriteTexturesData; break;
                    }

                    ASSERT(data != NULL);

                    ColorIndex4 *indices = (ColorIndex4*) (data + iOffset);
                    CLUT        *clut    = (CLUT*)        (data + cOffset);

                    int h = uv.w - uv.y;
                    int w = uv.z - uv.x;
                    ASSERT(w <= 256 && h <= 256);

                    for (int y = 0; y < h; y++)
                        for (int x = 0; x < w; x++) {
                            ColorIndex4 *index;
                            
                            if (t->dataType == TEX_TYPE_ROOM) {
                                int iw = w / 2;
                                int ih = h / 2;
                                int ix = x % iw;
                                int iy = y % ih;

                                int offset = uint32(uint16(t->sub[y >= ih ? (x >= iw ? 2 : 3) : (x >= iw ? 1 : 0)])) << 3;

                                offset += (iy * iw + ix) / 2;
                                index = (ColorIndex4*) (tsub + offset);
                            } else
                                index = indices + (y * w + x) / 2;

                            int idx = (x % 2) ? index->a : index->b;
                            Color16 &c = clut->color[idx];

                            if (t->attribute == 1 && idx == 0)
                                dst->color[y * 256 + x] = Color32(0, 0, 0, 0);
                            else
                                dst->color[y * 256 + x] = Color16(swap16(c.value));
                        }

                    break;
                }
                case VER_TR1_PC : {
                    ASSERT(tiles8);
                    ASSERT(palette);

                    Color32 *ptr = &dst->color[uv.y * 256];
                    for (int y = uv.y; y < uv.w; y++) {
                        for (int x = uv.x; x < uv.z; x++) {
                            ASSERT(x >= 0 && y >= 0 && x < 256 && y < 256);
                            uint8 index = tiles8[t->tile].index[y * 256 + x];
                            Color24 &p = palette[index];
                            if (index != 0) {
                                ptr[x].r = p.r;
                                ptr[x].g = p.g;
                                ptr[x].b = p.b;
                                ptr[x].a = 255;
                            } else
                                ptr[x].r = ptr[x].g = ptr[x].b = ptr[x].a = 0;
                        }
                        ptr += 256;
                    }
                    break;
                }
                case VER_TR2_PC :
                case VER_TR3_PC : {
                    ASSERT(tiles16);

                    Color32 *ptr = &dst->color[uv.y * 256];
                    for (int y = uv.y; y < uv.w; y++) {
                        for (int x = uv.x; x < uv.z; x++) {
                            Color32 c = tiles16[t->tile].color[y * 256 + x];
                            ptr[x].r = c.b;
                            ptr[x].g = c.g;
                            ptr[x].b = c.r;
                            ptr[x].a = c.a;
                        }
                        ptr += 256;
                    }
                    break;
                }
                case VER_TR1_PSX :
                case VER_TR2_PSX : 
                case VER_TR3_PSX : {
                    ASSERT(tiles4);
                    ASSERT(cluts);

                    CLUT   &clut = cluts[t->clut];
                    Tile4  &src  = tiles4[t->tile];

                    for (int y = uv.y; y < uv.w; y++)
                        for (int x = uv.x; x < uv.z; x++)
                            dst->color[y * 256 + x] = clut.color[(x % 2) ? src.index[(y * 256 + x) / 2].b : src.index[(y * 256 + x) / 2].a];

                    break;
                }
                default : ASSERT(false);
            }

            premultiplyAlpha(dst->color, uv);
        }

        void premultiplyAlpha(Color32 *data, const short4 &uv) {
        // pre-multiple alpha
            for (int y = uv.y; y < uv.w; y++)
                for (int x = uv.x; x < uv.z; x++) {
                    Color32 &c = data[y * 256 + x]; 
                    c.r = uint8((uint16(c.r) * c.a) / 255);
                    c.g = uint8((uint16(c.g) * c.a) / 255);
                    c.b = uint8((uint16(c.b) * c.a) / 255);
                }
        }

    // common methods
        Color32 getColor(int texture) const {
            switch (version) {
                case VER_TR1_SAT : return Color16((uint16)texture);
                case VER_TR1_PC  : return palette[texture & 0xFF];
                case VER_TR2_PC  :
                case VER_TR3_PC  : return palette32[(texture >> 8) & 0xFF];
                case VER_TR1_PSX : 
                case VER_TR2_PSX : 
                case VER_TR3_PSX : {
                    ASSERT((texture & 0x7FFF) < 256);
                    TextureInfo &t = objectTextures[texture & 0x7FFF];
                    int idx  = (t.texCoord[0].y * 256 + t.texCoord[0].x) / 2;
                    int part = t.texCoord[0].x % 2;
                    Tile4 &tile = tiles4[t.tile];
                    CLUT  &clut = cluts[t.clut];
                    return clut.color[part ? tile.index[idx].b : tile.index[idx].a];
                }
                default : ASSERT(false);
            }
            return Color32(255, 0, 255, 255);
        }

        Stream* getSampleStream(int index) const {
            if (!soundOffsets || !soundData) return NULL;
            uint8 *data = &soundData[soundOffsets[index]];
            uint32 size = 0;
            switch (version) {
                case VER_TR1_SAT : size = soundSize[index]; break;
                case VER_TR1_PC  : 
                case VER_TR2_PC  :
                case VER_TR3_PC  : size = FOURCC(data + 4) + 8; break; // read size from wave header
                case VER_TR1_PSX :
                case VER_TR2_PSX :
                case VER_TR3_PSX : size = soundSize[index]; break;
                default          : ASSERT(false);
            }
            return new Stream(NULL, data, size);
        }

        int getMeshByID(uint32 id) const {
            for (int i = 0; i < staticMeshesCount; i++)
                if (staticMeshes[i].id == id)
                    return i;
            ASSERT(false);
            return 0;
        }

        int16 getModelIndex(Entity::Type type) const {
            if (!simpleItems)
                type = convToInv(type);

        //#ifndef _DEBUG
            if ((type >= Entity::AI_GUARD && type <= Entity::AI_CHECK) || 
                (type >= Entity::GLOW_2 && type <= Entity::ENEMY_BAT_SWARM) ||
                type == Entity::WATERFALL || type == Entity::KILL_ALL_TRIGGERS || type == Entity::VIEW_TARGET || type == Entity::SOUND_DOOR_BELL || type == Entity::SOUND_ALARM_BELL || type == Entity::TRIPWIRE)
                return 0;
        //#endif

            if (type == Entity::ENEMY_MUTANT_2 || type == Entity::ENEMY_MUTANT_3)
                type = Entity::ENEMY_MUTANT_1; // hardcoded mutant models remapping
            
            if (((version & VER_TR3)) && (type == Entity::RICOCHET || type == Entity::WATER_SPLASH || type == Entity::BLOOD || type == Entity::FLAME)) {
                type = Entity::MISC_SPRITES; // TODO TR3
            }

            for (int i = 0; i < modelsCount; i++)
                if (type == models[i].type)
                    return i + 1;

            for (int i = 0; i < spriteSequencesCount; i++)
                if (type == spriteSequences[i].type)
                    return -(i + 1);

            return 0;
        }

        void flipMap() {
            for (int i = 0; i < roomsCount; i++)
                if (rooms[i].alternateRoom > -1) {
                    Room &src = rooms[i];
                    Room &dst = rooms[src.alternateRoom];

                    swap(src, dst);
                    swap(src.alternateRoom, dst.alternateRoom);
                }
            state.flags.flipped = !state.flags.flipped;
        }

        void floorSkipCommand(FloorData* &fd, int func) {
            switch (func) {
                case FloorData::PORTAL  :
                case FloorData::FLOOR   :
                case FloorData::CEILING : 
                    fd++;
                    break;

                case FloorData::FLOOR_NW_SE_SOLID       :
                case FloorData::FLOOR_NE_SW_SOLID       :
                case FloorData::CEILING_NW_SE_SOLID     :
                case FloorData::CEILING_NE_SW_SOLID     :
                case FloorData::FLOOR_NW_SE_PORTAL_NW   :
                case FloorData::FLOOR_NW_SE_PORTAL_SE   :
                case FloorData::FLOOR_NE_SW_PORTAL_NE   :
                case FloorData::FLOOR_NE_SW_PORTAL_SW   :
                case FloorData::CEILING_NW_SE_PORTAL_NW :
                case FloorData::CEILING_NW_SE_PORTAL_SE :
                case FloorData::CEILING_NE_SW_PORTAL_NE :
                case FloorData::CEILING_NE_SW_PORTAL_SW :
                    fd++;
                    break;

                case FloorData::TRIGGER :
                    fd++;
                    do {} while (!(*fd++).triggerCmd.end);
                    break;

                case FloorData::LAVA :
                case FloorData::CLIMB :
                case FloorData::MONKEY         :
                case FloorData::MINECART_LEFT  :
                case FloorData::MINECART_RIGHT :
                    break;

                default : LOG("unknown func to skip: %d\n", func);
            }
        }

        int getNextRoom(const Room::Sector *sector) const {
            ASSERT(sector);
            if (!sector->floorIndex) return NO_ROOM;
            FloorData *fd = &floors[sector->floorIndex];
        // floor data always in this order
            if (   fd->cmd.func == FloorData::FLOOR
                || fd->cmd.func == FloorData::FLOOR_NW_SE_SOLID
                || fd->cmd.func == FloorData::FLOOR_NE_SW_SOLID
                || fd->cmd.func == FloorData::FLOOR_NW_SE_PORTAL_SE
                || fd->cmd.func == FloorData::FLOOR_NW_SE_PORTAL_NW
                || fd->cmd.func == FloorData::FLOOR_NE_SW_PORTAL_SW
                || fd->cmd.func == FloorData::FLOOR_NE_SW_PORTAL_NE) {

                if (fd->cmd.end) return NO_ROOM;
                fd += 2;
            }

            if (   fd->cmd.func == FloorData::CEILING
                || fd->cmd.func == FloorData::CEILING_NE_SW_SOLID
                || fd->cmd.func == FloorData::CEILING_NW_SE_SOLID
                || fd->cmd.func == FloorData::CEILING_NE_SW_PORTAL_SW
                || fd->cmd.func == FloorData::CEILING_NE_SW_PORTAL_NE
                || fd->cmd.func == FloorData::CEILING_NW_SE_PORTAL_SE
                || fd->cmd.func == FloorData::CEILING_NW_SE_PORTAL_NW) {

                if (fd->cmd.end) return NO_ROOM;
                fd += 2;
            }

            if (fd->cmd.func == FloorData::PORTAL)
                return (++fd)->value;

            return NO_ROOM;
        }

        Room::Sector& getSector(int roomIndex, int x, int z, int &dx, int &dz) const {
            ASSERT(roomIndex >= 0 && roomIndex < roomsCount);

            Room &room = rooms[roomIndex];

            int sx = x - room.info.x;
            int sz = z - room.info.z;

            sx = clamp(sx, 0, room.xSectors * 1024 - 1);
            sz = clamp(sz, 0, room.zSectors * 1024 - 1);

            dx = sx % 1024;
            dz = sz % 1024;
            sx /= 1024;
            sz /= 1024;

            return room.sectors[sx * room.zSectors + sz];
        }
        
        Room::Sector& getSector(int roomIndex, int x, int z, int &sectorIndex) {
            ASSERT(roomIndex >= 0 && roomIndex < roomsCount);
            Room &room = rooms[roomIndex];
            x -= room.info.x;
            z -= room.info.z;
            x /= 1024;
            z /= 1024;
            return room.sectors[sectorIndex = (x * room.zSectors + z)];
        }
        
        Room::Sector* getSector(int16 &roomIndex, const vec3 &pos) {
            ASSERT(roomIndex >= 0 && roomIndex <= roomsCount);

            Room::Sector *sector = NULL;

            int x = int(pos.x);
            int y = int(pos.y);
            int z = int(pos.z);

        // check horizontal
            int16 prevRoom = roomIndex;

            while (1) { // Let's Rock!
                Room &room = rooms[roomIndex];

                int sx = (x - room.info.x) / 1024;
                int sz = (z - room.info.z) / 1024;
                sector = room.getSector(sx, sz);

                int nextRoom = getNextRoom(sector);
                if (nextRoom == NO_ROOM || nextRoom == prevRoom)
                    break;

                prevRoom  = roomIndex;
                roomIndex = nextRoom;
            };

        // check vertical
            while (sector->roomAbove != NO_ROOM && y < sector->ceiling * 256) {
                Room &room = rooms[roomIndex = sector->roomAbove];
                sector = room.getSector((x - room.info.x) / 1024, (z - room.info.z) / 1024);
            }

            while (sector->roomBelow != NO_ROOM && y >= sector->floor * 256) {
                Room &room = rooms[roomIndex = sector->roomBelow];
                sector = room.getSector((x - room.info.x) / 1024, (z - room.info.z) / 1024);
            }

            return sector;
        }

        float getFloor(const Room::Sector *sector, const vec3 &pos, int16 *roomIndex = NULL) {
            int x = int(pos.x);
            int z = int(pos.z);
            int dx = x & 1023;
            int dz = z & 1023;

            while (sector->roomBelow != NO_ROOM) {
                Room &room = rooms[sector->roomBelow];
                if (roomIndex)
                    *roomIndex = sector->roomBelow;
                sector = room.getSector((x - room.info.x) / 1024, (z - room.info.z) / 1024);
            }

            int floor = sector->floor * 256;

            if (!sector->floorIndex)
                return float(floor);

            FloorData *fd = &floors[sector->floorIndex];
            FloorData::Command cmd;

            do {
                cmd = (*fd++).cmd;
                
                switch (cmd.func) {
                    case TR::FloorData::FLOOR                 :
                    case TR::FloorData::FLOOR_NW_SE_SOLID     : 
                    case TR::FloorData::FLOOR_NE_SW_SOLID     : 
                    case TR::FloorData::FLOOR_NW_SE_PORTAL_SE :
                    case TR::FloorData::FLOOR_NW_SE_PORTAL_NW :
                    case TR::FloorData::FLOOR_NE_SW_PORTAL_SW :
                    case TR::FloorData::FLOOR_NE_SW_PORTAL_NE : {
                        int sx, sz;

                        if (cmd.func == TR::FloorData::FLOOR) {
                            sx = fd->slantX;
                            sz = fd->slantZ;
                        } else {
                            if (cmd.func == TR::FloorData::FLOOR_NW_SE_SOLID     || 
                                cmd.func == TR::FloorData::FLOOR_NW_SE_PORTAL_SE ||
                                cmd.func == TR::FloorData::FLOOR_NW_SE_PORTAL_NW) {
                                if (dx <= 1024 - dz) {
                                    floor += cmd.triangle.b * 256;
                                    sx = fd->a - fd->b;
                                    sz = fd->c - fd->b;
                                } else {
                                    floor += cmd.triangle.a * 256;
                                    sx = fd->d - fd->c;
                                    sz = fd->d - fd->a;
                                }
                            } else {
                                if (dx <= dz) {
                                    floor += cmd.triangle.b * 256;
                                    sx = fd->d - fd->c;
                                    sz = fd->c - fd->b;
                                } else {
                                    floor += cmd.triangle.a * 256;
                                    sx = fd->a - fd->b;
                                    sz = fd->d - fd->a;
                                }
                            }
                        }
                        fd++;

                        floor -= sx * (sx > 0 ? (dx - 1023) : dx) >> 2;
                        floor -= sz * (sz > 0 ? (dz - 1023) : dz) >> 2;
                        break;
                    }

                    case FloorData::TRIGGER :  {
                        fd++;
                        FloorData::TriggerCommand trigCmd;
                        do {
                            trigCmd = (*fd++).triggerCmd;
                            if (trigCmd.action != Action::ACTIVATE)
                                continue;
                            // TODO controller[trigCmd.args]->getFloor(&floor, x, y, z);
                        } while (!trigCmd.end);
                        break;
                    }

                    default : floorSkipCommand(fd, cmd.func);
                }
            } while (!cmd.end);

            return float(floor);
        }

        float getCeiling(const Room::Sector *sector, const vec3 &pos) {
            int x = int(pos.x);
            int z = int(pos.z);
            int dx = x & 1023;
            int dz = z & 1023;

            ASSERT(sector);
            while (sector->roomAbove != NO_ROOM) {
                Room &room = rooms[sector->roomAbove];
                sector = room.getSector((x - room.info.x) / 1024, (z - room.info.z) / 1024);
            }

            int ceiling = sector->ceiling * 256;

            if (!sector->floorIndex)
                return float(ceiling);

            FloorData *fd = &floors[sector->floorIndex];
            FloorData::Command cmd;

            do {
                cmd = (*fd++).cmd;
                
                switch (cmd.func) {
                    case TR::FloorData::CEILING                 :
                    case TR::FloorData::CEILING_NE_SW_SOLID     :
                    case TR::FloorData::CEILING_NW_SE_SOLID     :
                    case TR::FloorData::CEILING_NE_SW_PORTAL_SW :
                    case TR::FloorData::CEILING_NE_SW_PORTAL_NE :
                    case TR::FloorData::CEILING_NW_SE_PORTAL_SE :
                    case TR::FloorData::CEILING_NW_SE_PORTAL_NW : { 
                        int sx, sz;

                        if (cmd.func == TR::FloorData::CEILING) {
                            sx = fd->slantX;
                            sz = fd->slantZ;
                        } else {
                            if (cmd.func == TR::FloorData::CEILING_NW_SE_SOLID     || 
                                cmd.func == TR::FloorData::CEILING_NW_SE_PORTAL_SE ||
                                cmd.func == TR::FloorData::CEILING_NW_SE_PORTAL_NW) {
                                if (dx <= 1024 - dz) {
                                    ceiling += cmd.triangle.b * 256;
                                    sx = fd->c - fd->d;
                                    sz = fd->b - fd->c;
                                } else {
                                    ceiling += cmd.triangle.a * 256;
                                    sx = fd->b - fd->a;
                                    sz = fd->a - fd->d;
                                }
                            } else {
                                if (dx <= dz) {
                                    ceiling += cmd.triangle.b * 256;
                                    sx = fd->b - fd->a;
                                    sz = fd->b - fd->c;
                                } else {
                                    ceiling += cmd.triangle.a * 256;
                                    sx = fd->c - fd->d;
                                    sz = fd->a - fd->d;
                                }
                            }
                        }
                        fd++;

                        ceiling -= sx * (sx < 0 ? (dx - 1023) : dx) >> 2; 
                        ceiling += sz * (sz > 0 ? (dz - 1023) : dz) >> 2; 
                        break;
                    }

                    case FloorData::TRIGGER :  {
                        fd++;
                        FloorData::TriggerCommand trigCmd;
                        do {
                            trigCmd = (*fd++).triggerCmd;
                            if (trigCmd.action != Action::ACTIVATE)
                                continue;
                            // TODO controller[trigCmd.args]->getCeiling(&ceiling, x, y, z);
                        } while (!trigCmd.end);
                        break;
                    }

                    default : floorSkipCommand(fd, cmd.func);
                }
            } while (!cmd.end);

            return float(ceiling);
        }

        Room::Sector* getWaterLevelSector(int16 &roomIndex, const vec3 &pos) {
            int x = int(pos.x);
            int z = int(pos.z);

            Room *room = &rooms[roomIndex];
            Room::Sector *sector = room->getSector((x - room->info.x) / 1024, (z - room->info.z) / 1024);

            if (room->flags.water) { // go up to the air
                while (sector->roomAbove != NO_ROOM) {
                    room = &rooms[sector->roomAbove];
                    if (!room->flags.water)
                        return sector;
                    roomIndex = sector->roomAbove;
                    sector = room->getSector((x - room->info.x) / 1024, (z - room->info.z) / 1024);
                }
            } else { // go down to the water
                while (sector->roomBelow != NO_ROOM) {
                    room   = &rooms[roomIndex = sector->roomBelow];
                    sector = room->getSector((x - room->info.x) / 1024, (z - room->info.z) / 1024);
                    if (room->flags.water)
                        return sector;
                }
            }
            return NULL;
        }

        void getWaterInfo(int16 roomIndex, const vec3 &pos, float &level, float &depth) {
            depth = 0.0f;
            level = 0.0f;

            Room::Sector *sector = getWaterLevelSector(roomIndex, pos);
            if (!sector)
                return;

            level = sector->ceiling * 256.0f;

            if (pos.y < level)
                depth = pos.y - level;
            else
                depth = getFloor(sector, pos) - level;
        }

        bool isBlocked(int16 &roomIndex, const vec3 &pos) {
            Room::Sector *sector = getSector(roomIndex, pos);
            return pos.y >= getFloor(sector, pos) || pos.y <= getCeiling(sector, pos);
        }

    }; // struct Level
}

#endif

#ifndef H_FORMAT
#define H_FORMAT

#include "utils.h"
#include "gameflow.h"

#define MAX_RESERVED_ENTITIES 128
#define MAX_FLIPMAP_COUNT     32
#define MAX_TRACKS_COUNT      256
#define MAX_TRIGGER_COMMANDS  32
#define MAX_MESHES            512

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
    E( AMMO_EXPLOSIVE        ) \
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
    E( INV_AMMO_EXPLOSIVE    ) \
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
    E( INV_KEY_1             ) \
    E( INV_KEY_2             ) \
    E( INV_KEY_3             ) \
    E( INV_KEY_4             ) \
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
    E( MUTANT_GRENADE        ) \
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
    E( _INV_KEY_1            ) \
    E( _INV_KEY_2            ) \
    E( _INV_KEY_3            ) \
    E( _INV_KEY_4            ) \
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
    E( __INV_KEY_1           ) \
    E( __INV_KEY_2           ) \
    E( __INV_KEY_3           ) \
    E( __INV_KEY_4           ) \
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
            FLICKER        = 16,
            UNKNOWN        ,
            MESH_SWAP_1    ,
            MESH_SWAP_2    ,
            MESH_SWAP_3    ,
            INV_ON         ,
            INV_OFF        ,
            DYN_ON         ,
            DYN_OFF        ,
            FOOTPRINT      = 32,
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

        SND_HIT_SKATEBOY    = 132,

        SND_HIT_MUTANT      = 142,
        SND_STOMP           = 147,
        
        SND_LAVA            = 149,
        SND_FLAME           = 150,
        SND_DART            = 151,
        
        SND_TNT             = 170,
        SND_MUTANT_DEATH    = 171,
        SND_SECRET          = 173,
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

    struct Color32 {
        uint8 r, g, b, a;

        Color32() {}
        Color32(uint8 r, uint8 g, uint8 b, uint8 a) : r(r), g(g), b(b), a(a) {}
    };

    struct Color24 {
        uint8 r, g, b;

        Color24() {}
        Color24(uint8 r, uint8 g, uint8 b) : r(r), g(g), b(b) {}

        operator Color32() const { return Color32(r, g, b, 255); }
    };

    union Color16 {
        struct { uint16 r:5, g:5, b:5, a:1; };
        uint16 value;

        Color16() {}
        Color16(uint16 value) : value(value) {}

        Color32  getBGR()  const { return Color32((b << 3) | (b >> 2), (g << 3) | (g >> 2), (r << 3) | (r >> 2), 255); }
        operator Color24() const { return Color24((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2)); }
        operator Color32() const { return Color32((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2), -a); }
    };

    struct Vertex {
        int16 x, y, z;

        operator vec3()   const { return vec3((float)x, (float)y, (float)z); }
        operator short3() const { return *((short3*)this); }
    };

    struct Tile {
        uint16 index:14, undefined:1, triangle:1;
    };

    struct ObjectTexture {
        uint16  clut;
        Tile    tile;                        // tile or palette index
        uint16  attribute:15, animated:1;    // 0 - opaque, 1 - transparent, 2 - blend additive, animated
        short2  texCoord[4];
        short2  texCoordAtlas[4];

        short4 getMinMax() const {
            return short4(
                min(min(texCoord[0].x, texCoord[1].x), texCoord[2].x),
                min(min(texCoord[0].y, texCoord[1].y), texCoord[2].y),
                max(max(texCoord[0].x, texCoord[1].x), texCoord[2].x),
                max(max(texCoord[0].y, texCoord[1].y), texCoord[2].y)
            );
        }

        short4 getMinMaxAtlas() const {
            return short4(
                min(min(texCoordAtlas[0].x, texCoordAtlas[1].x), texCoordAtlas[2].x),
                min(min(texCoordAtlas[0].y, texCoordAtlas[1].y), texCoordAtlas[2].y),
                max(max(texCoordAtlas[0].x, texCoordAtlas[1].x), texCoordAtlas[2].x),
                max(max(texCoordAtlas[0].y, texCoordAtlas[1].y), texCoordAtlas[2].y)
            );
        }
    };

    struct SpriteTexture {
        uint16  clut;
        uint16  tile;
        int16   l, t, r, b;
        short2  texCoord[2];
        short2  texCoordAtlas[2];

        short4 getMinMax() const {
            return short4( texCoord[0].x, texCoord[0].y, texCoord[1].x, texCoord[1].y );
        }

        short4 getMinMaxAtlas() const {
            return short4( texCoordAtlas[0].x, texCoordAtlas[0].y, texCoordAtlas[1].x, texCoordAtlas[1].y );
        }
    };

    // used for access from ::cmp func
    static ObjectTexture *gObjectTextures = NULL;
    static SpriteTexture *gSpriteTextures = NULL;
    static int            gObjectTexturesCount;
    static int            gSpriteTexturesCount;

    struct Face {
        union {
            struct { uint16 texture:15, doubleSided:1; };
            uint16 value;
        } flags;
        uint8  colored; // !!! not existing in file
        uint8  vCount;  // !!! not existing in file
        short3 normal;  // !!! not existing in file
        uint16 vertices[4];

        static int cmp(const Face &a, const Face &b) {
            int aIndex = a.flags.texture;
            int bIndex = b.flags.texture;
            if (aIndex >= gObjectTexturesCount)
                return 1;
            if (bIndex >= gObjectTexturesCount)
                return -1;

            ASSERT(aIndex < gObjectTexturesCount);
            ASSERT(bIndex < gObjectTexturesCount);

            ObjectTexture &ta = gObjectTextures[aIndex];
            ObjectTexture &tb = gObjectTextures[bIndex];

            if (ta.tile.index < tb.tile.index)
                return -1;
            if (ta.tile.index > tb.tile.index)
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

    struct Tile4 {
        struct {
            uint8 a:4, b:4;
        } index[256 * 256 / 2];
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
                TR::Vertex  vertex;
                int16       unused_lighting;   // 0 (bright) .. 0x1FFF (dark)
                uint16      attributes;
                Color32     color;
            } *vertices;

            struct Sprite {
                int16       vertex;
                int16       texture;

                static int cmp(const Sprite &a, const Sprite &b) {
                    ASSERT(a.texture < gSpriteTexturesCount);
                    ASSERT(b.texture < gSpriteTexturesCount);

                    SpriteTexture &ta = gSpriteTextures[a.texture];
                    SpriteTexture &tb = gSpriteTextures[b.texture];

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
        int32   waterLevel;

        struct DynLight {
            int32 id;
            vec4  pos;
            vec4  color;
        } dynLights[2]; // 1 is reserved for main light
        
        int32 dynLightsCount;

        struct Portal {
            uint16  roomIndex;
            Vertex  normal;
            Vertex  vertices[4];

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

        Sector* getSector(int sx, int sz) {
            ASSERT(sx >= 0 && sx < xSectors && sz >= 0 && sz < zSectors);
            return sectors + sx * zSectors + sz;
        }

        void addDynLight(int32 id, const vec4 &pos, const vec4 &color) {
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
        }

        void removeDynLight(int32 id) {
            for (int i = 0; i < dynLightsCount; i++)
                if (dynLights[i].id == id) {
                    if (i == 0) dynLights[0] = dynLights[1];
                    dynLightsCount--;
                    break;
                }
        }
    };

    union FloorData {
        uint16 data;
        struct Command {
            uint16 func:5, tri:3, sub:7, end:1;
        } cmd;
        struct Slant {
            int8 x:8, z:8;
        } slant;
        struct TriggerInfo {
            uint16  timer:8, once:1, mask:5, :2;
        } triggerInfo;
        union TriggerCommand {
            uint16 value;
            struct {
                uint16 args:10, action:5, end:1;
            };
            struct {
                uint16 timer:8, once:1, speed:5, :2;
            };
        } triggerCmd;

        enum {
            NONE    ,
            PORTAL  ,
            FLOOR   ,
            CEILING ,
            TRIGGER ,
            LAVA    ,
            CLIMB   ,
        };
    };

    struct Overlap {
        uint16 boxIndex:15, end:1;
    };

    struct Flags {
        uint16 :8, once:1, active:5, :2;
    };

    struct ByteFlags {
        uint16 once:1, active:5, :2;
    };

    // internal mesh structure
    struct Mesh {

        struct Vertex {
            short4 coord;
            short4 normal;
        };

        TR::Vertex  center;
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
        enum Type        { TR_TYPES(DECL_ENUM) TYPE_MAX = 0xFFFF };

        Type    type;
        int16   room;
        int32   x, y, z;
        angle   rotation;
        int16   intensity;
        int16   intensity2;
        union Flags {
            struct { 
                uint16 state:2, unused:3, smooth:1, collision:1, invisible:1, once:1, active:5, reverse:1, rendered:1;
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
                REMAP_2( INV_KEY_1            );
                REMAP_2( INV_KEY_2            );
                REMAP_2( INV_KEY_3            );
                REMAP_2( INV_KEY_4            );
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
                REMAP_3( INV_KEY_1            );
                REMAP_3( INV_KEY_2            );
                REMAP_3( INV_KEY_3            );
                REMAP_3( INV_KEY_4            );
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
            return type >= DOOR_1 && type <= DOOR_6;
        }

        bool isCollider() const {
            return isEnemy() ||
                   isVehicle() ||
                   isDoor() ||
                   (type == DRAWBRIDGE && flags.active != ACTIVE) ||
                   ((type == HAMMER_HANDLE || type == HAMMER_BLOCK) && flags.collision) ||
                   type == CRYSTAL || type == MOVING_OBJECT || type == SCION_HOLDER;
        }

        static bool isPickup(Type type) {
            return (type >= PISTOLS && type <= AMMO_UZIS) ||
                   (type >= PUZZLE_1 && type <= PUZZLE_4) ||
                   (type >= KEY_ITEM_1 && type <= KEY_ITEM_4) ||
                   (type == MEDIKIT_SMALL || type == MEDIKIT_BIG) || 
                   (type == SCION_PICKUP_QUALOPEC || type == SCION_PICKUP_DROP || type == SCION_PICKUP_HOLDER || type == LEADBAR) ||
                   (type >= SECRET_1 && type <= SECRET_3) ||
                   (type == M16 || type == AMMO_M16) ||
                   (type == MP5 || type == AMMO_MP5) ||
                   (type == AUTOPISTOLS || type == AMMO_AUTOPISTOLS) ||
                   (type == DESERT_EAGLE || type == AMMO_DESERT_EAGLE) || 
                   (type == GRENADE || type == AMMO_GRENADE) || 
                   (type == ROCKET || type == AMMO_ROCKET) ||
                   (type == HARPOON || type == AMMO_HARPOON) ||
                   (type == FLARES || type == FLARE) || 
                   (type >= STONE_ITEM_1 && type <= STONE_ITEM_4);
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
            return isLara() || isEnemy() || isVehicle() || isActor() || type == DART || type == TRAP_SWORD;
        }

        void getAxis(int &dx, int &dz) {
            switch (rotation.value / 0x4000) {
                case 0  : dx =  0; dz =  1; break;
                case 1  : dx =  1; dz =  0; break;
                case 2  : dx =  0, dz = -1; break;
                case 3  : dx = -1, dz =  0; break;
                default : dx =  0; dz =  0; break;
            }
        }

        static Type convToInv(Type type) {
            switch (type) {
                case PISTOLS       : return INV_PISTOLS;
                case SHOTGUN       : return INV_SHOTGUN;
                case MAGNUMS       : return INV_MAGNUMS;
                case UZIS          : return INV_UZIS;
 
                case AMMO_PISTOLS  : return INV_AMMO_PISTOLS;
                case AMMO_SHOTGUN  : return INV_AMMO_SHOTGUN;
                case AMMO_MAGNUMS  : return INV_AMMO_MAGNUMS;
                case AMMO_UZIS     : return INV_AMMO_UZIS;

                case MEDIKIT_SMALL : return INV_MEDIKIT_SMALL;
                case MEDIKIT_BIG   : return INV_MEDIKIT_BIG;

                case PUZZLE_1      : return INV_PUZZLE_1;
                case PUZZLE_2      : return INV_PUZZLE_2;
                case PUZZLE_3      : return INV_PUZZLE_3;
                case PUZZLE_4      : return INV_PUZZLE_4;

                case KEY_ITEM_1    : return INV_KEY_1;
                case KEY_ITEM_2    : return INV_KEY_2;
                case KEY_ITEM_3    : return INV_KEY_3;
                case KEY_ITEM_4    : return INV_KEY_4;

                case LEADBAR       : return INV_LEADBAR;
                //case TR::Entity::SCION         : return TR::Entity::INV_SCION;
                default            : return type;
            }
        }

        static Type convFromInv(Type type) {
            switch (type) {
                case INV_PISTOLS       : return PISTOLS;
                case INV_SHOTGUN       : return SHOTGUN;
                case INV_MAGNUMS       : return MAGNUMS;
                case INV_UZIS          : return UZIS;
 
                case INV_AMMO_PISTOLS  : return AMMO_PISTOLS;
                case INV_AMMO_SHOTGUN  : return AMMO_SHOTGUN;
                case INV_AMMO_MAGNUMS  : return AMMO_MAGNUMS;
                case INV_AMMO_UZIS     : return AMMO_UZIS;

                case INV_MEDIKIT_SMALL : return MEDIKIT_SMALL;
                case INV_MEDIKIT_BIG   : return MEDIKIT_BIG;

                case INV_PUZZLE_1      : return PUZZLE_1;
                case INV_PUZZLE_2      : return PUZZLE_2;
                case INV_PUZZLE_3      : return PUZZLE_3;
                case INV_PUZZLE_4      : return PUZZLE_4;

                case INV_KEY_1         : return KEY_ITEM_1;
                case INV_KEY_2         : return KEY_ITEM_2;
                case INV_KEY_3         : return KEY_ITEM_3;
                case INV_KEY_4         : return KEY_ITEM_4;

                case INV_LEADBAR       : return LEADBAR;
                //case TR::Entity::SCION         : return TR::Entity::INV_SCION;
                default            : return type;
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
            if (type == SWITCH || type == SWITCH_WATER)
                opaque = true;
            if ((type >= PUZZLE_HOLE_1 && type <= PUZZLE_HOLE_4) || type == LIGHTNING)
                opaque = false;
        }
    };

    struct Animation {
        uint32  frameOffset;    // Byte offset into Frames[] (divide by 2 for Frames[i])
        uint8   frameRate;      // Engine ticks per frame
        uint8   frameSize;      // Number of int16_t's in Frames[] used by this animation

        uint16  state;

        fixed   speed;
        fixed   accel;

        uint16  frameStart;     // First frame in this animation
        uint16  frameEnd;       // Last frame in this animation
        uint16  nextAnimation;
        uint16  nextFrame;

        uint16  scCount;
        uint16  scOffset;       // Offset into StateChanges[]

        uint16  acCount;        // How many of them to use.
        uint16  animCommand;    // Offset into AnimCommand[]
    };

    struct AnimState {
        uint16  state;
        uint16  rangesCount;    // number of ranges
        uint16  rangesOffset;   // Offset into animRanges[]
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
        Vertex  pos;
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
        int16   count;        // number of texture offsets - 1 in group
        int16   textures[1];  // offsets into objectTextures[]
    };

    struct Node {
        uint32  flags;
        int32   x, y, z;
    };

    struct Model {
        Entity::Type type;
        uint16       unused;
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
        Vertex  target;
        Vertex  pos;
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

    struct SaveGame {

        struct Item {
            uint16 type;
            uint16 count;
        };

        struct Progress {
            uint32 size;
            uint32 time;
            uint32 distance;
            uint8  levelID;
            uint8  mediUsed;
            uint8  secrets;
            uint8  pickups;
            uint16 ammoUsed;
            uint8  kills;
            uint8  itemsCount;
        // Item items[itemsCount]
        };

        struct Entity {
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
                    uint16 itemHands;
                    uint16 itemBack;
                    uint16 itemHolster;
                    union {
                        struct { uint16 wet:1, burn:1; };
                        uint16 value;
                    } flags;
                } lara;
                struct {
                    float  health;
                    uint16 mood;
                    uint16 targetBox;
                } enemy;

                struct {
                    int32 jointIndex;
                    float sleep;
                } flame;
            } extra;
        };

        struct CurrentState {
            ByteFlags flipmaps[MAX_FLIPMAP_COUNT];
            ByteFlags tracks[MAX_TRACKS_COUNT];

            uint16   fogColor;
            union {
                struct { uint16 track:8, flipped:1; };
                uint16 value;
            } flags;
            uint16   entitiesCount;
            Progress progress;
            //Entity   entities[entitiesCount];
        };

        int32  size;
        uint16 version;
        uint16 progressCount;
        // Progress     progress[levelsCount];
        // CurrentState currentState;
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

        int32           meshDataCount;
        uint16          *meshData;

        int32           meshOffsetsCount;
        uint32          *meshOffsets;

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

        int32           objectTexturesCount;
        ObjectTexture   *objectTextures;

        int32           spriteTexturesCount;
        SpriteTexture   *spriteTextures;

        int32           spriteSequencesCount;
        SpriteSequence  *spriteSequences;

        int32           camerasCount;
        Camera          *cameras;

        int32           soundSourcesCount;
        SoundSource     *soundSources;

        int32           boxesCount;
        Box             *boxes;
        int32           overlapsCount;
        Overlap         *overlaps;
        Zone            zones[2];   // default and alternative

        int32           animTexturesDataSize;
        uint16          *animTexturesData;
        int16           animTexturesCount;
        AnimTexture     **animTextures;

        int32           entitiesBaseCount;
        int32           entitiesCount;
        Entity          *entities;

        int32           paletteSize;
        Color24         *palette;
        Color32         *palette32;

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

        SaveGame                save;
        SaveGame::CurrentState  state;

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
            int trigCmdCount;
            int climb;
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

        int     cutEntity;
        mat4    cutMatrix;
        bool    isDemoLevel;

        struct {
            int16 muzzleFlash;
            int16 puzzleDone[4];
            int16 weapons[4];
            int16 braid;
            int16 laraSpec;
            int16 laraSkin;
            int16 meshSwap[3];
            int16 sky;
            int16 smoke;
            int16 waterSplash;
            int16 glyphs;

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

                int16 weapon[4];
                int16 ammo[4];
                int16 medikit[2];
                int16 puzzle[4];
                int16 key[4];

                int16 leadbar;
                int16 scion;
            } inv;
        } extra;

        Level(Stream &stream) : version(VER_UNKNOWN), soundData(NULL), soundOffsets(NULL), soundSize(NULL) {
            int startPos = stream.pos;
            memset(this, 0, sizeof(*this));
            cutEntity = -1;

            tiles4    = NULL;
            tiles8    = NULL;
            tiles16   = NULL;
            palette   = NULL;
            palette32 = NULL;

            int soundOffset = 0;

            uint32 magic;

            #define MAGIC_TR1_PC  0x00000020
            #define MAGIC_TR1_PSX 0x56414270
            #define MAGIC_TR2_PC  0x0000002D
            #define MAGIC_TR3_PC1 0xFF080038
            #define MAGIC_TR3_PC2 0xFF180038
            #define MAGIC_TR3_PC3 0xFF180034
            #define MAGIC_TR3_PSX 0xFFFFFFC8

            id = TR::getLevelID(stream.size, version, isDemoLevel);

            if (version == VER_UNKNOWN || version == VER_TR1_PSX) {
                stream.read(magic);
                if (magic != MAGIC_TR1_PC && magic != MAGIC_TR2_PC && magic != MAGIC_TR3_PC1 && magic != MAGIC_TR3_PC2 && magic != MAGIC_TR3_PC3 && magic != MAGIC_TR3_PSX) {
                    soundOffset = magic;
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
                memset(this, 0, sizeof(*this)); 
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
            }

            if (version == VER_TR3_PSX) {
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
            stream.read(meshData,    stream.read(meshDataCount));
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
                stream.read(m.unused);
                stream.read(m.mCount);
                stream.read(m.mStart);
                stream.read(m.node);
                stream.read(m.frame);
                stream.read(m.animation);
                if (version & VER_PSX)
                    stream.seek(2);
                m.type = Entity::remap(version, m.type);

                for (int j = 0; j < m.mCount; j++)
                    initMesh(m.mStart + j, m.type);
            }
            stream.read(staticMeshes, stream.read(staticMeshesCount));

            for (int i = 0; i < staticMeshesCount; i++)
                initMesh(staticMeshes[i].mesh);

            remapMeshOffsetsToIndices();

            delete[] meshData;
            meshData = NULL;

            LOG("meshes: %d\n", meshesCount);

            if (version == VER_TR2_PSX || version == VER_TR3_PSX) {
                stream.read(tiles4, stream.read(tilesCount));
                stream.read(clutsCount);
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

                int32 roomTexCount;
                stream.read(roomTexCount);

                if (roomTexCount) {
                    ObjectTexture *oldTex = objectTextures;
                // reallocate textures info to add room textures
                    objectTextures = new ObjectTexture[objectTexturesCount + roomTexCount];
                    if (oldTex) {
                        memcpy(objectTextures, oldTex, sizeof(ObjectTexture) * objectTexturesCount);
                        delete[] oldTex;
                    }
                // load room textures
                    for (int i = objectTexturesCount; i < objectTexturesCount + roomTexCount; i++) {
                        ObjectTexture &t = objectTextures[i];
                        readObjectTex(stream, t, true);
                        stream.seek(2 * 16); // skip 2 mipmap levels
                    }

                // remap room texture indices
                    for (int i = 0; i < roomsCount; i++) {
                        Room::Data &d = rooms[i].data;
                        for (int j = 0; j < d.fCount; j++) {
                            Face &f = d.faces[j];

                            ASSERT(f.flags.texture < roomTexCount);

                            f.flags.texture += objectTexturesCount;
                        }
                    }

                // remap animated textures
                    for (int i = 0; i < animTexturesCount; i++)
                        for (int j = 0; j < animTextures[i]->count; j++)
                            animTextures[i]->textures[j] += objectTexturesCount;

                    LOG("objTex:%d + roomTex:%d = %d\n", objectTexturesCount, roomTexCount, objectTexturesCount + roomTexCount);
                    objectTexturesCount += roomTexCount;
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


        // Amiga -> PC color palette for TR1 PC
            if (version == VER_TR1_PC) {
                ASSERT(palette);
                Color24 *c = palette;
                for (int i = 0; i < 256; i++) {
                    c->r <<= 2;
                    c->g <<= 2;
                    c->b <<= 2;
                    c++;
                }
            }

            initRoomMeshes();
            initAnimTex();

            memset(&state, 0, sizeof(state));

            initExtra();
            initCutscene();

            gObjectTextures = objectTextures;
            gSpriteTextures = spriteTextures;
            gObjectTexturesCount = objectTexturesCount;
            gSpriteTexturesCount = spriteTexturesCount;
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
            delete[] spriteTextures;
            delete[] spriteSequences;
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
            delete[] animTexturesData;
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

            for (int i = 0; i < modelsCount; i++)
                switch (models[i].type) {
                    case Entity::MUZZLE_FLASH        : extra.muzzleFlash     = i; break;
                    case Entity::PUZZLE_DONE_1       : extra.puzzleDone[0]   = i; break;
                    case Entity::PUZZLE_DONE_2       : extra.puzzleDone[1]   = i; break;
                    case Entity::PUZZLE_DONE_3       : extra.puzzleDone[2]   = i; break;
                    case Entity::PUZZLE_DONE_4       : extra.puzzleDone[3]   = i; break;
                    case Entity::LARA_PISTOLS        : extra.weapons[0]      = i; break;
                    case Entity::LARA_SHOTGUN        : extra.weapons[1]      = i; break;
                    case Entity::LARA_MAGNUMS        : extra.weapons[2]      = i; break;
                    case Entity::LARA_UZIS           : extra.weapons[3]      = i; break;
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

                    case Entity::INV_PISTOLS         : extra.inv.weapon[0]   = i; break;
                    case Entity::INV_SHOTGUN         : extra.inv.weapon[1]   = i; break;
                    case Entity::INV_MAGNUMS         : extra.inv.weapon[2]   = i; break;
                    case Entity::INV_UZIS            : extra.inv.weapon[3]   = i; break;

                    case Entity::INV_AMMO_PISTOLS    : extra.inv.ammo[0]     = i; break;
                    case Entity::INV_AMMO_SHOTGUN    : extra.inv.ammo[1]     = i; break;
                    case Entity::INV_AMMO_MAGNUMS    : extra.inv.ammo[2]     = i; break;
                    case Entity::INV_AMMO_UZIS       : extra.inv.ammo[3]     = i; break;

                    case Entity::INV_MEDIKIT_SMALL   : extra.inv.medikit[0]  = i; break;
                    case Entity::INV_MEDIKIT_BIG     : extra.inv.medikit[1]  = i; break;

                    case Entity::INV_PUZZLE_1        : extra.inv.puzzle[0]   = i; break;
                    case Entity::INV_PUZZLE_2        : extra.inv.puzzle[1]   = i; break;
                    case Entity::INV_PUZZLE_3        : extra.inv.puzzle[2]   = i; break;
                    case Entity::INV_PUZZLE_4        : extra.inv.puzzle[3]   = i; break;

                    case Entity::INV_KEY_1           : extra.inv.key[0]      = i; break;
                    case Entity::INV_KEY_2           : extra.inv.key[1]      = i; break;
                    case Entity::INV_KEY_3           : extra.inv.key[2]      = i; break;
                    case Entity::INV_KEY_4           : extra.inv.key[3]      = i; break;
                                                                         
                    case Entity::INV_LEADBAR         : extra.inv.leadbar     = i; break;
                    case Entity::INV_SCION           : extra.inv.scion       = i; break;

                    case Entity::SKY                 : extra.sky = i; break;

                    default : ;
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

        // turn off interpolation for some entities
            if (id == LVL_TR2_CUT_1)
                for (int i = 0; i < entitiesBaseCount; i++) {
                    Entity &e = entities[i];
                    if (e.type == Entity::CUT_6 || e.type == Entity::CUT_8 || e.type == Entity::CUT_9) {
                        e.flags.smooth = false;
                    }
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
            f.vCount = triangle ? 3 : 4;

            for (int i = 0; i < f.vCount; i++)
                stream.read(f.vertices[i]);

            if (triangle)
                f.vertices[3] = 0;

            stream.read(f.flags.value);

        #ifndef SPLIT_BY_TILE
            f.colored = colored;
        #else
            f.colored = false;
        #endif
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
                            uint32 z:5, y:5, x:5, color:16, unknown:1;
                        } cv;

                        stream.raw(&cv, sizeof(cv));
                        //ASSERT(cv.attribute == 0);
                        v.vertex.x = (cv.x << 10);
                        v.vertex.y = (cv.y << 8) + r.info.yTop;
                        v.vertex.z = (cv.z << 10);

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
                        f.vCount      = 3;
                        f.colored     = false;
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
                        f.vCount      = 4;
                        f.colored     = false;
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
                            uint32 lighting:8, attributes:8, z:5, y:5, x:5, :1;
                        } cv;

                        stream.raw(&cv, sizeof(cv));

                        v.vertex.x    = (cv.x << 10);
                        v.vertex.y    = (cv.y << 8) + r.info.yTop;
                        v.vertex.z    = (cv.z << 10);
                        lighting      = cv.lighting;
                        v.attributes  = 0;//cv.attributes;
                    } else {
                        stream.read(v.vertex.x);
                        stream.read(v.vertex.y);
                        stream.read(v.vertex.z);
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

                    if (version & VER_PSX)
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
                        f.vCount = 4;
                        stream.raw(f.vertices, sizeof(uint16) * 4);
                        f.vertices[0] >>= 2;
                        f.vertices[1] >>= 2;
                        f.vertices[2] >>= 2;
                        f.vertices[3] >>= 2;
                        f.colored = false;
                    }
                } else
                    for (int i = 0; i < d.rCount; i++) readFace(stream, d.faces[idx++], false, false);

                stream.read(tmpCount);
                ASSERT(tmpCount == d.tCount);

                if (version == VER_TR2_PSX) {
                    stream.seek(2);
                    for (int i = 0; i < d.tCount; i++) {
                        Face &f = d.faces[d.rCount + i];
                        f.vCount = 3;
                        stream.raw(&f.flags.value, sizeof(uint16));
                        stream.raw(f.vertices, sizeof(uint16) * 3);
                        f.vertices[0] >>= 2;
                        f.vertices[1] >>= 2;
                        f.vertices[2] >>= 2;
                        f.vertices[3] = 0;
                        f.colored = false;
                    }
                } else {
                    for (int i = 0; i < d.tCount; i++)
                        readFace(stream, d.faces[idx++], false, true);
                }
            }

            if (version & VER_PSX) { // swap indices (quad strip -> quad list) only for PSX version
                for (int j = 0; j < d.fCount; j++)
                    if (d.faces[j].vCount == 4)
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
            r.sectors = (r.zSectors * r.xSectors) ? new Room::Sector[r.zSectors * r.xSectors] : NULL;

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
                    int value = 255 - clamp((intensity > 0x1FFF) ? 0 : (intensity >> 5), 0, 255);
                    light.color.r = light.color.g = light.color.b = value;
                    light.color.a = 0;
                }

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

        void initMesh(int mIndex, Entity::Type type = Entity::LARA) {
            int offset = meshOffsets[mIndex];
            for (int i = 0; i < meshesCount; i++)
                if (meshes[i].offset == offset)
                    return;

            Stream stream(NULL, &meshData[offset / 2], 1024 * 1024);

            Mesh &mesh = meshes[meshesCount++];
            mesh.offset = offset;

            uint32 fOffset;

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
            }

            switch (version) {
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
                        f.colored = false;
                        f.vCount  = 3;

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
                        f.colored = false;
                        f.vCount  = 4;

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
                for (int j = 0; j < face.vCount; j++) {\
                    Mesh::Vertex &v = mesh.vertices[face.vertices[j]];\
                    short4 &n = v.normal;\
                    if (!(n.x | n.y | n.z)) {\
                        if (fn > -1) {\
                            n = mesh.vertices[face.vertices[fn]].normal;\
                            continue;\
                        }\
                        vec3 o(mesh.vertices[face.vertices[0]].coord);\
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

        void readObjectTex(Stream &stream, ObjectTexture &t, bool isRoom = false) {
            #define SET_PARAMS(t, d, c) {\
                    t.clut        = c;\
                    t.tile        = d.tile;\
                    t.attribute   = d.attribute;\
                    t.animated    = false;\
                    t.texCoord[0] = t.texCoordAtlas[0] = short2( d.x0, d.y0 );\
                    t.texCoord[1] = t.texCoordAtlas[1] = short2( d.x1, d.y1 );\
                    t.texCoord[2] = t.texCoordAtlas[2] = short2( d.x2, d.y2 );\
                    t.texCoord[3] = t.texCoordAtlas[3] = short2( d.x3, d.y3 );\
                    ASSERT(d.x0 < 256 && d.x1 < 256 && d.x2 < 256 && d.x3 < 256 && d.y0 < 256 && d.y1 < 256 && d.y2 < 256 && d.y3 < 256);\
                }

            switch (version) {
                case VER_TR1_PC :
                case VER_TR2_PC :
                case VER_TR3_PC : {
                    struct {
                        uint16  attribute;
                        Tile    tile;
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
                        Tile    tile;
                        uint8   x2, y2;
                        uint16  unknown;
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
            objectTextures = stream.read(objectTexturesCount) ? new ObjectTexture[objectTexturesCount] : NULL;
            for (int i = 0; i < objectTexturesCount; i++)
                readObjectTex(stream, objectTextures[i]);
        }

        void readSpriteTex(Stream &stream) {
            #define SET_PARAMS(t, d, c) {\
                    t.clut = c;\
                    t.tile = d.tile;\
                    t.l    = d.l;\
                    t.t    = d.t;\
                    t.r    = d.r;\
                    t.b    = d.b;\
                }

            spriteTextures = stream.read(spriteTexturesCount) ? new SpriteTexture[spriteTexturesCount] : NULL;
            for (int i = 0; i < spriteTexturesCount; i++) {
                SpriteTexture &t = spriteTextures[i];
                switch (version) {
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
            }

            #undef SET_PARAMS

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
            }
        }

        void readAnimTex(Stream &stream) {
            stream.read(animTexturesData, stream.read(animTexturesDataSize));
            animTextures = NULL;

            if (!animTexturesDataSize)
                return;

            uint16 *ptr = animTexturesData;
            animTexturesCount = *ptr++;
            if (!animTexturesCount)
                return;

            animTextures = new AnimTexture*[animTexturesCount];
            for (int i = 0; i < animTexturesCount; i++) {
                animTextures[i] = (AnimTexture*)ptr;
                animTextures[i]->count++; // count + 1
                ptr += 1 + animTextures[i]->count; // skip count and texture indices
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
                e.flags.smooth = true;

                e.type = Entity::remap(version, e.type);

                e.controller = NULL;
                e.modelIndex = getModelIndex(e.type);
            }
            for (int i = entitiesBaseCount; i < entitiesCount; i++)
                entities[i].controller = NULL;

            if (isCutsceneLevel()) {
                for (int i = 0; i < entitiesBaseCount; i++) {
                    TR::Entity &e = entities[i];
                    if ((((version & VER_TR1)) && e.isActor()) || 
                        (((version & (VER_TR2 | VER_TR3))) && e.isLara())) {
                        cutEntity = i;
                        break;
                    }
                }
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
            for (int i = 0; i < animTexturesCount; i++)
                for (int j = 0; j < animTextures[i]->count; j++)
                    objectTextures[animTextures[i]->textures[j]].animated = true;
        }

        void shiftAnimTex() {
            for (int i = 0; i < animTexturesCount; i++) {
                AnimTexture *at = animTextures[i];
                ObjectTexture tmp = objectTextures[at->textures[0]];
                for (int j = 0; j < at->count - 1; j++)
                    objectTextures[at->textures[j]] = objectTextures[at->textures[j + 1]];
                objectTextures[at->textures[at->count - 1]] = tmp;
            }
        }

        void fillObjectTexture(Tile32 *dst, const short4 &uv, int16 tileIndex, int16 clutIndex) {
        // convert to RGBA
            switch (version) {
                case VER_TR1_PC : {
                    ASSERT(tiles8);
                    ASSERT(palette);

                    Color32 *ptr = &dst->color[uv.y * 256];
                    for (int y = uv.y; y < uv.w; y++) {
                        for (int x = uv.x; x < uv.z; x++) {
                            ASSERT(x >= 0 && y >= 0 && x < 256 && y < 256);
                            uint8 index = tiles8[tileIndex].index[y * 256 + x];
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
                            Color32 c = tiles16[tileIndex].color[y * 256 + x];
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

                    CLUT   &clut = cluts[clutIndex];
                    Tile4  &src  = tiles4[tileIndex];

                    for (int y = uv.y; y < uv.w; y++)
                        for (int x = uv.x; x < uv.z; x++)
                            dst->color[y * 256 + x] = clut.color[(x % 2) ? src.index[(y * 256 + x) / 2].b : src.index[(y * 256 + x) / 2].a];

                    break;
                }
                default : ASSERT(false);
            }
        }

    // common methods
        Color32 getColor(int texture) const {
            switch (version) {
                case VER_TR1_PC  : return palette[texture & 0xFF];
                case VER_TR2_PC  :
                case VER_TR3_PC  : return palette32[(texture >> 8) & 0xFF];
                case VER_TR1_PSX : 
                case VER_TR2_PSX : 
                case VER_TR3_PSX : {
                    ASSERT((texture & 0x7FFF) < 256);
                    ObjectTexture &t = objectTextures[texture & 0x7FFF];
                    int idx  = (t.texCoord[0].y * 256 + t.texCoord[0].x) / 2;
                    int part = t.texCoord[0].x % 2;
                    Tile4 &tile = tiles4[t.tile.index];
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

        int getMeshByID(int id) const {
            for (int i = 0; i < staticMeshesCount; i++)
                if (staticMeshes[i].id == id)
                    return i;
            ASSERT(false);
            return 0;
        }

        int16 getModelIndex(Entity::Type type) const {
        //#ifndef _DEBUG
            if ((type >= Entity::AI_GUARD && type <= Entity::AI_CHECK) || 
                (type >= Entity::GLOW_2 && type <= Entity::ENEMY_BAT_SWARM) || 
                (type == Entity::TRAP_FLAME_EMITTER || type == Entity::MIDAS_HAND) ||
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
                    TR::Room &src = rooms[i];
                    TR::Room &dst = rooms[src.alternateRoom];

                    swap(src, dst);
                    swap(src.alternateRoom, dst.alternateRoom);
                }
            state.flags.flipped = !state.flags.flipped;
        }

        void floorSkipCommand(FloorData* &fd, int func) {
            switch (func) {
                case FloorData::PORTAL      :
                case FloorData::FLOOR   :
                case FloorData::CEILING : 
                    fd++;
                    break;

                case FloorData::TRIGGER :
                    fd++;
                    do {} while (!(*fd++).triggerCmd.end);
                    break;

                case FloorData::LAVA :
                case FloorData::CLIMB :
                    break;

                case 0x07 :
                case 0x08 :
                case 0x09 :
                case 0x0A :
                case 0x0B :
                case 0x0C :
                case 0x0D :
                case 0x0E :
                case 0x0F :
                case 0x10 :
                case 0x11 :
                case 0x12 : fd++; break; // TODO TR3 triangulation

                case 0x13 : break; // TODO TR3 monkeyswing

                case 0x14 : 
                case 0x15 : break; // TODO TR3 minecart

                default : LOG("unknown func to skip: %d\n", func);
            }
        }

        int getNextRoom(const Room::Sector *sector) const {
            ASSERT(sector);
            if (!sector->floorIndex) return NO_ROOM;
            FloorData *fd = &floors[sector->floorIndex];
        // floor data always in this order
            if (!fd->cmd.end && fd->cmd.func == FloorData::FLOOR)   fd += 2; // skip floor slant info
            if (!fd->cmd.end && fd->cmd.func == FloorData::CEILING) fd += 2; // skip ceiling slant info
            if (fd->cmd.func == FloorData::PORTAL)  return (++fd)->data;
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
            while (1) { // Let's Rock!
                Room &room = rooms[roomIndex];

                int sx = (x - room.info.x) / 1024;
                int sz = (z - room.info.z) / 1024;

                if (sz <= 0 || sz >= room.zSectors - 1) {
                    sz = clamp(sz, 0, room.zSectors - 1);
                    sx = clamp(sx, 1, room.xSectors - 2);
                } else
                    sx = clamp(sx, 0, room.xSectors - 1);

                sector = room.getSector(sx, sz);

                int nextRoom = getNextRoom(sector);
                if (nextRoom == NO_ROOM)
                    break;

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
                    case FloorData::FLOOR   : {
                        FloorData::Slant slant = (*fd++).slant;
                        int sx = (int)slant.x;
                        int sz = (int)slant.z;
                        int dx = x % 1024;
                        int dz = z % 1024;
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

            ASSERT(sector);
            while (sector->roomAbove != NO_ROOM) {
                Room &room = rooms[sector->roomAbove];
                sector = room.getSector((x - room.info.x) / 1024, (z - room.info.z) / 1024);
            }

            int ceiling = sector->ceiling * 256;

            if (!sector->floorIndex)
                return float(ceiling);

            FloorData *fd = &floors[sector->floorIndex];
            
            if (fd->cmd.func == FloorData::FLOOR) {
                if (fd->cmd.end) return float(ceiling);
                fd += 2; // skip floor slant
            }

            if (fd->cmd.func == FloorData::CEILING) {
                FloorData::Slant slant = (++fd)->slant;
                int sx = (int)slant.x;
                int sz = (int)slant.z;
                int dx = x % 1024;
                int dz = z % 1024;
                ceiling -= sx * (sx < 0 ? (dx - 1023) : dx) >> 2;
                ceiling += sz * (sz > 0 ? (dz - 1023) : dz) >> 2;
            }

            // TODO parse triggers to collide with objects (bridges, trap doors/floors etc)

            return float(ceiling);
        }

        TR::Room::Sector* getWaterLevelSector(int16 &roomIndex, const vec3 &pos) {
            int x = int(pos.x);
            int z = int(pos.z);

            TR::Room *room = &rooms[roomIndex];
            TR::Room::Sector *sector = room->getSector((x - room->info.x) / 1024, (z - room->info.z) / 1024);

            if (room->flags.water) { // go up to the air
                while (sector->roomAbove != NO_ROOM) {
                    room = &rooms[sector->roomAbove];
                    if (!room->flags.water)
                        break;
                    roomIndex = sector->roomAbove;
                    sector = room->getSector((x - room->info.x) / 1024, (z - room->info.z) / 1024);
                }
                return sector;
            } else { // go down to the water
                while (sector->roomBelow != NO_ROOM) {
                    room   = &rooms[roomIndex = sector->roomBelow];
                    sector = room->getSector((x - room->info.x) / 1024, (z - room->info.z) / 1024);
                    if (room->flags.water)
                        break;
                }
                return sector;
            }
            return NULL;
        }

        bool isBlocked(int16 &roomIndex, const vec3 &pos) {
            Room::Sector *sector = getSector(roomIndex, pos);
            return pos.y >= getFloor(sector, pos) || pos.y <= getCeiling(sector, pos);
        }

    }; // struct Level
}

#endif

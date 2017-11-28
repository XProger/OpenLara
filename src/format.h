#ifndef H_FORMAT
#define H_FORMAT

#include "utils.h"

#define MAX_RESERVED_ENTITIES 128
#define MAX_FLIPMAP_COUNT     32
#define MAX_TRACKS_COUNT      256
#define MAX_TRIGGER_COMMANDS  32
#define MAX_MESHES            512

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

    enum {
        NO_FLOOR = -127,
        NO_ROOM  = 0xFF,
        NO_BOX   = 0xFFFF,
        NO_TRACK = 0xFF,
        ACTIVE   = 0x1F,
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

    enum {
        ANIM_CMD_NONE       ,
        ANIM_CMD_OFFSET     ,
        ANIM_CMD_JUMP       ,
        ANIM_CMD_EMPTY      ,
        ANIM_CMD_KILL       ,
        ANIM_CMD_SOUND      ,
        ANIM_CMD_EFFECT     ,
    };

    enum Effect : int32 {
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

    enum Action : uint16 {
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

    namespace Limits {

        struct Limit {
            float  dy, dz, ay;
            ::Box  box;
            bool   alignAngle;
            bool   alignHoriz;
        };

        Limit SWITCH = {
            0, 376, 30,     {{-200, 0, 312}, {200, 0, 512}}, true, false
        };

        Limit SWITCH_UNDERWATER = {
            0, 100, 80,     {{-1024, -1024, -1024}, {1024, 1024, 512}}, true, true
        };

        Limit PICKUP = {
            0, -100, 180,   {{-256, -100, -256}, {256, 100, 100}}, false, true
        };

        Limit PICKUP_UNDERWATER = {
            -200, -350, 45, {{-512, -512, -512}, {512, 512, 512}}, false, true
        };

        Limit KEY_HOLE = { 
            0, 362, 30,     {{-200, 0, 312}, {200, 0, 512}}, true, true
        };

        Limit PUZZLE_HOLE = { 
            0, 327, 30,     {{-200, 0, 312}, {200, 0, 512}}, true, true
        };

        Limit BLOCK = { 
            0, -612, 30,    {{-300, 0, -692}, {300, 0, -512}}, true, false
        };

        Limit MIDAS = { 
            512, -612, 30,  {{-700, 284, -700}, {700, 996, 700}}, true, false
        };

        Limit SCION = { 
            640, -202, 30,  {{-256, 540, -350}, {256, 740, -200}}, false, false
        };

        Limit SCION_HOLDER = { 
            640, -202, 10,  {{-256, 206, -862}, {256, 306, -200}}, true, false
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

        Color32  getBGR()  const { return Color32((b << 3) | (b >> 2), (g << 3) | (g >> 2), (r << 3) | (r >> 2), 255); }
        operator Color24() const { return Color24((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2)); }
        operator Color32() const { return Color32((r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2), -a); }
    };

    struct Vertex {
        int16 x, y, z;

        operator vec3()   const { return vec3((float)x, (float)y, (float)z); }
        operator short3() const { return *((short3*)this); }
    };

    struct Rectangle {
        uint16 vertices[4];
        union {
            struct { uint16 texture:15, doubleSided:1; };
            uint16 value;
        } flags;
        uint16 colored; // !!! not existing in file
    };

    struct Triangle {
        uint16 vertices[3];
        union {
            struct { uint16 texture:15, doubleSided:1; };
            uint16 value;
        } flags;
        uint32 colored; // !!! not existing in file
    };

    #define FACE4_SIZE (sizeof(Rectangle) - sizeof(uint16))
    #define FACE3_SIZE (sizeof(Triangle)  - sizeof(uint32))

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
            int16       rCount;
            int16       tCount;
            int16       sCount;

            struct Vertex {
                TR::Vertex  vertex;
                int16       unused_lighting;   // 0 (bright) .. 0x1FFF (dark)
                uint16      attributes;
                Color32     color;
            } *vertices;

            Rectangle   *rectangles;
            Triangle    *triangles;

            struct Sprite {
                int16       vertex;
                int16       texture;
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
        struct {
            uint16 water:1, :2, sky:1, :1, wind:1, unused:9, visible:1;
        } flags;
        uint8   waterScheme;
        uint8   reverbType;
        uint8   filter;
        uint8   align;
        uint32  waterLevel;

        struct  DynLight {
            int32 id;
            vec4  pos;
            vec4  color;
        } dynLights[2];
        
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

        void addDynLight(int32 id, const vec4 &pos, const vec4 &color) {
            DynLight *light = NULL;
            for (int i = 0; i < dynLightsCount; i++)
                if (dynLights[i].id == id) {
                    light = &dynLights[i];
                    break;
                }
             // 1 is additional second light, can be overridden
            if (!light) {
                if (dynLightsCount < 2) {
                    light = &dynLights[dynLightsCount];
                    dynLightsCount = min(2, dynLightsCount + 1);
                } else
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
                uint16 args:10;
                Action action:5;
                uint16 end:1;
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
                uint16 transparent:1, reserved:15;
            };
            uint16 value;
        }           flags;
        int16       vCount;
        int16       rCount;
        int16       tCount;

        int32       offset;
        Vertex      *vertices;
        Rectangle   *rectangles;
        Triangle    *triangles;

        Mesh() : vertices(0), rectangles(0), triangles(0) {}
        ~Mesh() {
            delete[] vertices;
            delete[] rectangles;
            delete[] triangles;
        }
    };

    struct Entity {
        enum ActiveState : uint16 { asNone, asActive, asInactive };
        enum Type : uint16 { TR_TYPES(DECL_ENUM) };

        Type    type;
        int16   room;
        int32   x, y, z;
        angle   rotation;
        int16   intensity;
        int16   intensity2;
        union Flags {
            struct { ActiveState state:2; uint16 unused:4, collision:1, invisible:1, once:1, active:5, reverse:1, rendered:1; };
            uint16 value;
        } flags;
    // not exists in file
        int32   modelIndex;     // index of representation in models (index + 1) or spriteSequences (-(index + 1)) arrays
        void    *controller;    // Controller implementation or NULL

        static Type remap(Version version, Type type) {
            if (version & VER_TR1)
                return type;

            if (version & VER_TR2)
                type = TR::Entity::Type(type + TR2_TYPES_START);

            if (version & VER_TR3)
                type = TR::Entity::Type(type + TR3_TYPES_START);

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

    #pragma warning( push )
    #pragma warning( disable : 4200 ) // zero-sized array warning
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
        int16   textures[0];  // offsets into objectTextures[]
    };
    #pragma warning( push )

    struct Node {
        uint32  flags;
        int32   x, y, z;
    };

    struct Model {
        Entity::Type    type;
        uint16          unused;
        uint16          mCount;    
        uint16          mStart;    
        uint32          node;      
        uint32          frame;     
        uint16          animation; 
        uint16          align;     
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

    struct Tile {
        uint16 index:14, undefined:1, triangle:1;
    };

    struct ObjectTexture {
        uint16  clut;
        Tile    tile;                        // tile or palette index
        uint16  attribute:15, repeat:1;      // 0 - opaque, 1 - transparent, 2 - blend additive, 
        short2  texCoord[4];

        short4 getMinMax() const {
            return {
                min(min(texCoord[0].x, texCoord[1].x), texCoord[2].x),
                min(min(texCoord[0].y, texCoord[1].y), texCoord[2].y),
                max(max(texCoord[0].x, texCoord[1].x), texCoord[2].x),
                max(max(texCoord[0].y, texCoord[1].y), texCoord[2].y),
            };
        }
    };

    struct SpriteTexture {
        uint16  clut;
        uint16  tile;
        int16   l, t, r, b;
        short2  texCoord[2];

        short4 getMinMax() const {
            return { texCoord[0].x, texCoord[0].y, texCoord[1].x, texCoord[1].y };
        }
    };

    struct SpriteSequence {
        Entity::Type    type;
        uint16          unused;
        int16           sCount;
        int16           sStart;
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
        uint32  minZ, maxZ; // Horizontal dimensions in global units
        uint32  minX, maxX;
        int16   floor;      // Height value in global units
        union {
            struct {
                uint16 index:14, block:1, blockable:1;    // Index into Overlaps[].
            };
            uint16 value;
        } overlap;

        bool contains(uint32 x, uint32 z) {
            return x >= minX && x <= maxX && z >= minZ && z <= maxZ;
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
            uint8  room;
            uint8  extraSize;
            union Extra {
                struct {
                    float  velX, velY, velZ;
                    uint16 angleX;
                    uint16 health;
                    uint16 oxygen;
                    uint16 stamina;
                    uint8  itemHands;
                    uint8  itemBack;
                    uint8  itemHolster;
                    union {
                        struct { uint8 wet:1, burn:1; };
                        uint8 value;
                    } flags;
                } lara;
                struct {
                    uint16 health;
                    uint16 mood;
                } enemy;
            } extra;
        };

        struct Item {
            uint16 type;
            uint16 count;
        };

        struct CurrentState {
            // EntityState entities[entitiesCount];

            uint32 fogColor;
            uint16 secrets;
            union {
                struct { uint16 track:8, flipped:1; };
                uint16 value;
            } flags;

            ByteFlags flipmaps[MAX_FLIPMAP_COUNT];
            ByteFlags tracks[MAX_TRACKS_COUNT];
        };

        uint32 size;
        uint16 levelID;
    // full game stats
        uint16 mediUsed;
        uint16 pickups;
        uint16 secrets;
        uint32 kills;
        uint32 ammoUsed;
        uint32 distance;
        uint32 time;

        uint16 itemsCount;    // 0 -> skip inventory items array
        uint16 entitiesCount; // 0 -> skip current state

        // Item items[itemsCount]
        // CurrentState currentState;
    };


    struct Level {
        Version         version;
        LevelID         id;

        int32           tilesCount;
        Tile32          *tiles;

        uint32          unused;

        uint16          roomsCount;
        Room            *rooms;

        int32           floorsCount;
        FloorData       *floors;

        int16           meshesCount;
        Mesh            meshes[MAX_MESHES];

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

        int32           entitiesBaseCount;
        int32           entitiesCount;
        Entity          *entities;

        int32           paletteSize;
        Color24         *palette;
        Color32         *palette32;
        int32           clutsCount;
        CLUT            *cluts;
        Tile4           *tiles4;

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

        SaveGame                save;
        SaveGame::CurrentState  state;

   // common
        enum Trigger : uint32 {
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
            Trigger trigger;
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

        void    *cameraController;
        void    *laraController;

        int     cutEntity;
        mat4    cutMatrix;
        bool    isDemoLevel;
        bool    isHomeLevel;

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
            Tile8 *tiles8 = NULL;
            Tile16 *tiles16 = NULL;

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

            id = getLevelID(stream.size);

            if (version == VER_UNKNOWN || version == VER_TR1_PSX) {
                stream.read(magic);
                if (magic != MAGIC_TR1_PC && magic != MAGIC_TR2_PC && magic != MAGIC_TR3_PC1 && magic != MAGIC_TR3_PC2 && magic != MAGIC_TR3_PC3) {
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
                    soundSize[i]    = soundOffsets[i + 1] - soundOffsets[i];
                    soundOffsets[i] = soundDataSize;
                    soundDataSize  += soundSize[i];
                }
                stream.read(soundData, soundDataSize);
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

            if (version & VER_PC) {
            // tiles
                stream.read(tiles8, stream.read(tilesCount));
            }

            if (version == VER_TR2_PC || version == VER_TR3_PC) 
                stream.read(tiles16, tilesCount);

            if (version == VER_TR1_PSX) {
            // tiles
                stream.read(tiles4, tilesCount = 13);
                stream.read(cluts,  clutsCount = 512);                
                stream.seek(0x4000);
            }
            stream.read(unused);

        // rooms
            rooms = stream.read(roomsCount) ? new Room[roomsCount] : NULL;
            for (int i = 0; i < roomsCount; i++) 
                readRoom(stream, rooms[i]);

        // floors
            stream.read(floors,    stream.read(floorsCount));
        // meshes
            readMeshes(stream);
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
                stream.read(m.type);
                stream.read(m.unused);
                stream.read(m.mCount);
                stream.read(m.mStart);
                stream.read(m.node);
                stream.read(m.frame);
                stream.read(m.animation);
                if (version & VER_PSX)
                    stream.seek(2);
                m.type = Entity::remap(version, m.type);
            }
            stream.read(staticMeshes, stream.read(staticMeshesCount));

            if (version == VER_TR2_PSX) {
                stream.read(tiles4, stream.read(tilesCount));
                stream.read(cluts, stream.read(clutsCount));
                //stream.read(palette32, stream.read(paletteSize));
                stream.seek(4);
            }

        // textures & UV
            if (version & (VER_TR1 | VER_TR2))
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
            stream.read(animTexturesData,   stream.read(animTexturesDataSize));
            if (version & VER_TR3)
                readObjectTex(stream);
        // entities (enemies, items, lara etc.)
            entitiesCount = stream.read(entitiesBaseCount) + MAX_RESERVED_ENTITIES;
            entities = new Entity[entitiesCount];
            for (int i = 0; i < entitiesBaseCount; i++) {
                Entity &e = entities[i];
                stream.read(e.type);
                stream.read(e.room);
                stream.read(e.x);
                stream.read(e.y);
                stream.read(e.z);
                stream.read(e.rotation);
                stream.read(e.intensity);
                if (version & (VER_TR2 | VER_TR3))
                    stream.read(e.intensity2);
                stream.read(e.flags.value);

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

        // sounds
            stream.read(soundsMap,  (version & VER_TR1) ? 256 : 370);
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

            if (version == VER_TR1_PC) {
                stream.read(soundData,    stream.read(soundDataSize));
                stream.read(soundOffsets, stream.read(soundOffsetsCount));
            }

            if (version == VER_TR2_PC || version == VER_TR3_PC) {
                stream.read(soundOffsets, stream.read(soundOffsetsCount));
                new Stream(version == VER_TR2_PC ? "audio/2/MAIN.SFX" : "audio/3/MAIN.SFX", sfxLoadAsync, this);
            }

        // cinematic frames for cameras (PSX)
            if (version & VER_PSX) {
                stream.seek(4);
                stream.read(cameraFrames, stream.read(cameraFramesCount));
            }

            initRoomMeshes();
            initTiles(tiles4, tiles8, tiles16, palette, palette32, cluts);

            //delete[] tiles4;
            //tiles4 = NULL;
            delete[] tiles8;
            delete[] tiles16;

            memset(&state, 0, sizeof(state));

            initExtra();
            initCutscene();
        }

        ~Level() {
            delete[] tiles;
        // rooms
            for (int i = 0; i < roomsCount; i++) {
                Room &r = rooms[i];
                delete[] r.data.vertices;
                delete[] r.data.rectangles;
                delete[] r.data.triangles;
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
            delete[] entities;
            delete[] palette;
            delete[] palette32;
            delete[] cluts;
            delete[] tiles4;
            delete[] cameraFrames;
            delete[] demoData;
            delete[] soundsMap;
            delete[] soundsInfo;
            delete[] soundData;
            delete[] soundOffsets;
            delete[] soundSize;
        }

        LevelID getLevelID(int size) {
            isDemoLevel = false;
            isHomeLevel = false;
            switch (size) {
            // TR1
                case 508614  : version = VER_TR1_PSX;
                case 316138  :
                case 316460  : return LVL_TR1_TITLE;
                case 1074234 : version = VER_TR1_PSX;
                case 3236806 :
                case 3237128 : isHomeLevel = true; return LVL_TR1_GYM;
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
                case 3471450 : isHomeLevel = true; return LVL_TR2_ASSAULT;
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
            // TR3
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

        uint8 remapTrack(uint8 track) {
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
                        state.flags.flipped = true;
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

        LevelID getTitleId() const {
            switch (version & VER_VERSION) {
                case VER_TR1 : return LVL_TR1_TITLE;
                case VER_TR2 : return LVL_TR2_TITLE;
                case VER_TR3 : return LVL_TR3_TITLE;
            }
            return LVL_TR1_TITLE;
            ASSERT(false);
        }

        LevelID getHomeId() const {
            switch (version & VER_VERSION) {
                case VER_TR1 : return LVL_TR1_GYM;
                case VER_TR2 : return LVL_TR2_ASSAULT;
                case VER_TR3 : return LVL_TR3_HOUSE;
            }
            return LVL_TR1_TITLE;
            ASSERT(false);
        }

        LevelID getStartId() const {
            switch (version & VER_VERSION) {
                case VER_TR1 : return LVL_TR1_1;
                case VER_TR2 : return LVL_TR2_WALL;
                case VER_TR3 : return LVL_TR3_JUNGLE;
            }
            return LVL_MAX;
            ASSERT(false);
        }

        LevelID getEndId() const {
            switch (version & VER_VERSION) {
                case VER_TR1 : return LVL_TR1_10C;
                case VER_TR2 : return LVL_TR2_HOUSE;
                case VER_TR3 : return LVL_TR3_CHAMBER;
            }
            return LVL_MAX;
            ASSERT(false);
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
            return id == LVL_TR1_CUT_1 || id == LVL_TR1_CUT_2 || id == LVL_TR1_CUT_3 || id == LVL_TR1_CUT_4 ||
                   id == LVL_TR2_CUT_1 || id == LVL_TR2_CUT_2 || id == LVL_TR2_CUT_3 || id == LVL_TR2_CUT_4 || 
                   id == LVL_TR3_CUT_1 || id == LVL_TR3_CUT_2  || id == LVL_TR3_CUT_3  || id == LVL_TR3_CUT_4 ||
                   id == LVL_TR3_CUT_5 || id == LVL_TR3_CUT_6  || id == LVL_TR3_CUT_7  || id == LVL_TR3_CUT_8 ||
                   id == LVL_TR3_CUT_9 || id == LVL_TR3_CUT_11 || id == LVL_TR3_CUT_12;
        }

        void readFace(Stream &stream, Rectangle &v, bool colored = false) {
            for (int i = 0; i < COUNT(v.vertices); i++)
                stream.read(v.vertices[i]);
            stream.read(v.flags.value);
            v.colored = colored;
        }

        void readFace(Stream &stream, Triangle &v, bool colored = false) {
            for (int i = 0; i < COUNT(v.vertices); i++)
                stream.read(v.vertices[i]);
            stream.read(v.flags.value);
            v.colored = colored;
        }

        void readRoom(Stream &stream, Room &r) {
            Room::Data &d = r.data;
        // room info
            stream.read(r.info);
        // room data
            stream.read(d.size);
            int startOffset = stream.pos;
            if (version == VER_TR1_PSX) stream.seek(2);
            d.vertices = stream.read(d.vCount) ? new Room::Data::Vertex[d.vCount] : NULL;
            for (int i = 0; i < d.vCount; i++) {
                Room::Data::Vertex &v = d.vertices[i];

                uint16 lighting;

                if (version == VER_TR2_PSX) {
                    union Compressed {
                        struct { uint32 lighting:8, attributes:8, z:5, y:5, x:5, w:1; };
                        uint32 value;
                    } comp;
                    stream.read(comp.value);
                    v.vertex.x    = (comp.x << 10);
                    v.vertex.y    = (comp.y << 8) + r.info.yTop;
                    v.vertex.z    = (comp.z << 10);
                    lighting      = comp.lighting;
                    v.attributes  = comp.attributes;
                    ASSERT(comp.w == 0);
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

            d.rectangles = stream.read(d.rCount) ? new Rectangle[d.rCount] : NULL;
            if (version == VER_TR2_PSX) {
                for (int i = 0; i < d.rCount; i++)
                    stream.raw(&d.rectangles[i].flags.value, sizeof(uint16));
                if ((stream.pos - startOffset) % 4) stream.seek(2);
                for (int i = 0; i < d.rCount; i++) {
                    Rectangle &v = d.rectangles[i];
                    stream.raw(v.vertices, sizeof(v.vertices));
                    v.vertices[0] >>= 2;
                    v.vertices[1] >>= 2;
                    v.vertices[2] >>= 2;
                    v.vertices[3] >>= 2;
                    v.colored = false;
                }
            } else {
                for (int i = 0; i < d.rCount; i++)
                    readFace(stream, d.rectangles[i]);
            }

            if (version & VER_PSX) { // swap indices (quad strip -> quad list) only for PSX version
                for (int j = 0; j < d.rCount; j++)
                    swap(d.rectangles[j].vertices[2], d.rectangles[j].vertices[3]);
            }

            d.triangles = stream.read(d.tCount) ? new Triangle[d.tCount] : NULL;
            if (version == VER_TR2_PSX) {
                stream.seek(2);
                for (int i = 0; i < d.tCount; i++) {
                    Triangle &v = d.triangles[i];
                    stream.raw(&v.flags.value, sizeof(uint16));
                    stream.raw(v.vertices, sizeof(v.vertices));
                    v.vertices[0] >>= 2;
                    v.vertices[1] >>= 2;
                    v.vertices[2] >>= 2;
                    v.colored = false;
                }
            } else {
                for (int i = 0; i < d.tCount; i++)
                    readFace(stream, d.triangles[i]);
            }

        // room sprites
            if (version == VER_TR2_PSX) { // there is no room sprites
                d.sprites = NULL;
                d.sCount  = NULL;
            } else
                stream.read(d.sprites, stream.read(d.sCount));

        // portals
            stream.read(r.portals, stream.read(r.portalsCount));

            if (version == VER_TR2_PSX) {
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
            r.sectors = (r.zSectors * r.xSectors) ? new TR::Room::Sector[r.zSectors * r.xSectors] : NULL;

            for (int i = 0; i < r.zSectors * r.xSectors; i++) {
                TR::Room::Sector &s = r.sectors[i];

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
            if (version & (VER_TR2 | VER_TR3))
                stream.read(r.ambient2);

            if (version & VER_TR2)
                stream.read(r.lightMode);

        // lights
            r.lights = stream.read(r.lightsCount) ? new Room::Light[r.lightsCount] : NULL;
            for (int i = 0; i < r.lightsCount; i++) {
                Room::Light &light = r.lights[i];
                stream.read(light.x);
                stream.read(light.y);
                stream.read(light.z);

                uint16 intensity;

                if (version == VER_TR3_PC)
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
                    int value = 2555 - clamp((intensity > 0x1FFF) ? 0 : (intensity >> 5), 0, 255);
                    light.color.r = light.color.g = light.color.b = value;
                    light.color.a = 0;
                }

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
                    m.color = color.getBGR();
                }

                if (version & VER_TR2)
                    stream.seek(2);

                uint16 intensity;
                stream.read(intensity);
                if ((version & VER_VERSION) < VER_TR3) {
                    int value = clamp((intensity > 0x1FFF) ? 255 : (255 - (intensity >> 5)), 0, 255);
                    m.color.r = m.color.g = m.color.b = value;
                    m.color.a = 0;
                }

                stream.read(m.meshID);
                if (version == VER_TR1_PSX)
                    stream.seek(2); // just an align for PSX version
            }

        // misc flags
            stream.read(r.alternateRoom);
            stream.read(r.flags);
            if (version & VER_TR3) {
                stream.read(r.waterScheme);
                stream.read(r.reverbType);
                stream.read(r.filter); // unused
            }

            r.dynLightsCount = 0;
        }


        void readMeshes(Stream &stream) {
            uint32 meshDataSize;
            stream.read(meshDataSize);
            int32 start = stream.pos;
            int32 end   = stream.pos + meshDataSize * 2;
            meshesCount = 0;
            while (stream.pos < end) {
                Mesh &mesh = meshes[meshesCount++];
                mesh.offset = stream.pos - start;

                stream.read(mesh.center);
                stream.read(mesh.radius);
                stream.read(mesh.flags);
                stream.read(mesh.vCount);

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
                                n = { 0, 0, 0, 0 };
                            }
                        }

                        uint16 rCount, crCount, tCount, ctCount;

                        int tmp = stream.pos;
                        stream.seek(stream.read(rCount)  * FACE4_SIZE); // uint32 colored (not existing in file)
                        stream.seek(stream.read(tCount)  * FACE3_SIZE);
                        stream.seek(stream.read(crCount) * FACE4_SIZE);
                        stream.seek(stream.read(ctCount) * FACE3_SIZE);
                        stream.setPos(tmp);

                        mesh.rCount = rCount + crCount;
                        mesh.tCount = tCount + ctCount;

                        mesh.rectangles = mesh.rCount ? new Rectangle[mesh.rCount] : NULL;
                        mesh.triangles  = mesh.tCount ? new Triangle[mesh.tCount]  : NULL;

                        stream.seek(sizeof(uint16)); for (int i = 0; i < rCount; i++)  readFace(stream, mesh.rectangles[i]);
                        stream.seek(sizeof(uint16)); for (int i = 0; i < tCount; i++)  readFace(stream, mesh.triangles[i]);
                        stream.seek(sizeof(uint16)); for (int i = 0; i < crCount; i++) readFace(stream, mesh.rectangles[rCount + i], true);
                        stream.seek(sizeof(uint16)); for (int i = 0; i < ctCount; i++) readFace(stream, mesh.triangles[tCount + i], true);
                        break;
                    }
                    case VER_TR1_PSX : 
                    case VER_TR2_PSX :{
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
                                n = { 0, 0, 0, 0 };
                            }
                        }

                        if (version == VER_TR2_PSX && nCount > 0) { // TODO probably for unused meshes only but need to check
                            uint16 crCount = 0, ctCount = 0;
                            stream.read(crCount);
                            stream.seek((FACE4_SIZE + 2) * crCount);
                            stream.read(ctCount);
                            stream.seek((FACE3_SIZE + 2) * ctCount);
                        }

                        mesh.rectangles = stream.read(mesh.rCount) ? new Rectangle[mesh.rCount] : NULL;
                        for (int i = 0; i < mesh.rCount; i++) readFace(stream, mesh.rectangles[i]);

                        mesh.triangles  = stream.read(mesh.tCount) ? new Triangle[mesh.tCount]  : NULL;
                        for (int i = 0; i < mesh.tCount; i++) readFace(stream, mesh.triangles[i]);

                        if (mesh.rCount == 0 && mesh.tCount == 0)
                            LOG("! warning: mesh %d has no geometry with %d vertices\n", meshesCount - 1, mesh.vCount);
                        //ASSERT(mesh.rCount != 0 || mesh.tCount != 0);
                        
                        for (int i = 0; i < mesh.rCount; i++) {
                            Rectangle &f = mesh.rectangles[i];
                            f.colored = (f.flags.texture < 256) ? true : false;

                            if (version == VER_TR2_PSX) {
                                f.vertices[0] >>= 3;
                                f.vertices[1] >>= 3;
                                f.vertices[2] >>= 3;
                                f.vertices[3] >>= 3;
                            }
                        }

                        for (int i = 0; i < mesh.tCount; i++) {
                            Triangle &f = mesh.triangles[i];
                            f.colored = (f.flags.texture < 256) ? true : false;

                            if (version == VER_TR2_PSX) {
                                f.vertices[0] >>= 3;
                                f.vertices[1] >>= 3;
                                f.vertices[2] >>= 3;
                            }
                        }
                        
                        break;
                    }
                    default : ASSERT(false);
                }

                #define RECALC_ZERO_NORMALS(mesh, face, count)\
                    int fn = -1;\
                    for (int j = 0; j < count; j++) {\
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
                for (int i = 0; i < mesh.rCount; i++) {
                    Rectangle &f = mesh.rectangles[i];
                    ASSERT(f.vertices[0] < mesh.vCount);
                    ASSERT(f.vertices[1] < mesh.vCount);
                    ASSERT(f.vertices[2] < mesh.vCount);
                    ASSERT(f.vertices[3] < mesh.vCount);
                    RECALC_ZERO_NORMALS(mesh, f, 4);
                }

                for (int i = 0; i < mesh.tCount; i++) {
                    Triangle &f = mesh.triangles[i];
                    ASSERT(f.vertices[0] < mesh.vCount);
                    ASSERT(f.vertices[1] < mesh.vCount);
                    ASSERT(f.vertices[2] < mesh.vCount);
                    RECALC_ZERO_NORMALS(mesh, f, 3);
                }

                #undef RECALC_ZERO_NORMALS

                int32 align = (stream.pos - start - mesh.offset) % 4;
                if (align) stream.seek(4 - align);
            }
            ASSERT(stream.pos - start == meshDataSize * 2);

            stream.read(meshOffsets, stream.read(meshOffsetsCount));

        // remap offsets to mesh index
            for (int i = 0; i < meshOffsetsCount; i++) {
                int index = -1;
//                if (!meshOffsets[i] && i)
//                    index = -1;
//                else
                for (int j = 0; j < meshesCount; j++)
                    if (meshes[j].offset == meshOffsets[i]) {
                        index = j;
                        break;
                    }
                meshOffsets[i] = index;
            }
        }

        void readObjectTex(Stream &stream) {
            #define SET_PARAMS(t, d, c) {\
                    t.clut        = c;\
                    t.tile        = d.tile;\
                    t.attribute   = d.attribute;\
                    t.texCoord[0] = { d.x0, d.y0 };\
                    t.texCoord[1] = { d.x1, d.y1 };\
                    t.texCoord[2] = { d.x2, d.y2 };\
                    t.texCoord[3] = { d.x3, d.y3 };\
                    ASSERT(d.x0 < 256 && d.x1 < 256 && d.x2 < 256 && d.x3 < 256 && d.y0 < 256 && d.y1 < 256 && d.y2 < 256 && d.y3 < 256);\
                }

            objectTextures = stream.read(objectTexturesCount) ? new ObjectTexture[objectTexturesCount] : NULL;
            for (int i = 0; i < objectTexturesCount; i++) {
                ObjectTexture &t = objectTextures[i];
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
                        stream.read(d);
                        SET_PARAMS(t, d, 0);
                        break;
                    }
                    case VER_TR1_PSX : 
                    case VER_TR2_PSX : {
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
                        stream.read(d);
                        SET_PARAMS(t, d, d.clut);
                        break;
                    }
                    default : ASSERT(false);
                }
            }

            #undef SET_PARAMS
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
                        stream.read(d);
                        SET_PARAMS(t, d, 0);
                        t.texCoord[0] = { d.u,                       d.v                       };
                        t.texCoord[1] = { (uint8)(d.u + (d.w >> 8)), (uint8)(d.v + (d.h >> 8)) };
                        break;
                    }
                    case VER_TR1_PSX : 
                    case VER_TR2_PSX : {
                        struct {
                            int16   l, t, r, b;
                            uint16  clut;
                            uint16  tile;
                            uint8   u0, v0;
                            uint8   u1, v1;
                        } d;
                        stream.read(d);
                        SET_PARAMS(t, d, d.clut);
                        t.texCoord[0] = { d.u0, d.v0 };
                        t.texCoord[1] = { d.u1, d.v1 };
                        break;
                    }
                    default : ASSERT(false);
                }
            }

            #undef SET_PARAMS

            spriteSequences = stream.read(spriteSequencesCount) ? new SpriteSequence[spriteSequencesCount] : NULL;
            for (int i = 0; i < spriteSequencesCount; i++) {
                TR::SpriteSequence &s = spriteSequences[i];
                stream.read(s.type);
                stream.read(s.unused);
                stream.read(s.sCount);
                stream.read(s.sStart);
                s.type = Entity::remap(version, s.type);
                s.sCount = -s.sCount;
            }
        }

        void initRoomMeshes() {
            for (int i = 0; i < roomsCount; i++) {
                Room &room = rooms[i];
                for (int j = 0; j < room.meshesCount; j++)
                    room.meshes[j].meshIndex = getMeshByID(room.meshes[j].meshID);
            }
        }

        void initTiles(Tile4 *tiles4, Tile8 *tiles8, Tile16 *tiles16, Color24 *palette, Color32 *palette32, CLUT *cluts) {
            tiles = new Tile32[tilesCount];
        // convert to RGBA
            switch (version) {
                case VER_TR1_PC : {
                    ASSERT(tiles8);
                    ASSERT(palette);

                    for (int i = 0; i < 256; i++) { // Amiga -> PC color palette
                        Color24 &c = palette[i];
                        c.r <<= 2;
                        c.g <<= 2;
                        c.b <<= 2;
                    }

                    for (int i = 0; i < tilesCount; i++) {
                        Color32 *ptr = &tiles[i].color[0];
                        for (int y = 0; y < 256; y++) {
                            for (int x = 0; x < 256; x++) {
                                uint8 index = tiles8[i].index[y * 256 + x];
                                Color24 &p = palette[index];
                                ptr[x].r = p.r;
                                ptr[x].g = p.g;
                                ptr[x].b = p.b;
                                ptr[x].a = index == 0 ? 0 : 255;
                            }
                            ptr += 256;
                        }
                    }
                    break;
                }
                case VER_TR1_PSX :
                case VER_TR2_PSX : {
                    ASSERT(tiles4);
                    ASSERT(cluts);

                    for (int i = 0; i < objectTexturesCount; i++) {
                        ObjectTexture &t = objectTextures[i];
                        CLUT   &clut = cluts[t.clut];
                        Tile32 &dst  = tiles[t.tile.index];
                        Tile4  &src  = tiles4[t.tile.index];

                        int minX = min(min(t.texCoord[0].x, t.texCoord[1].x), t.texCoord[2].x);
                        int maxX = max(max(t.texCoord[0].x, t.texCoord[1].x), t.texCoord[2].x);
                        int minY = min(min(t.texCoord[0].y, t.texCoord[1].y), t.texCoord[2].y);
                        int maxY = max(max(t.texCoord[0].y, t.texCoord[1].y), t.texCoord[2].y);

                        for (int y = minY; y <= maxY; y++)
                            for (int x = minX; x <= maxX; x++)
                                dst.color[y * 256 + x] = clut.color[(x % 2) ? src.index[(y * 256 + x) / 2].b : src.index[(y * 256 + x) / 2].a];
                    }

                    for (int i = 0; i < spriteTexturesCount; i++) {
                        SpriteTexture &t = spriteTextures[i];
                        CLUT   &clut = cluts[t.clut];
                        Tile32 &dst  = tiles[t.tile];
                        Tile4  &src  = tiles4[t.tile];
                        
                        for (int y = t.texCoord[0].y; y <= t.texCoord[1].y; y++)
                            for (int x = t.texCoord[0].x; x <= t.texCoord[1].x; x += 2) {                           
                                dst.color[y * 256 + x + 0] = clut.color[src.index[(y * 256 + x) / 2].a];
                                dst.color[y * 256 + x + 1] = clut.color[src.index[(y * 256 + x) / 2].b];
                            }
                    }

                    break;
                }
                case VER_TR2_PC :
                case VER_TR3_PC : {
                    ASSERT(tiles16);

                    for (int i = 0; i < tilesCount; i++) {
                        Color32 *ptr = &tiles[i].color[0];
                        for (int y = 0; y < 256; y++) {
                            for (int x = 0; x < 256; x++) {
                                TR::Color32 c = tiles16[i].color[y * 256 + x];
                                ptr[x].r = c.b;
                                ptr[x].g = c.g;
                                ptr[x].b = c.r;
                                ptr[x].a = c.a;
                            }
                            ptr += 256;
                        }
                    }
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
                case VER_TR2_PSX : {
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
                case VER_TR2_PSX : size = soundSize[index]; break;
                default          : ASSERT(false);
            }
            return new Stream(data, size);
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
            if ((type >= Entity::AI_GUARD && type <= Entity::AI_CHECK) || (type >= Entity::GLOW_2 && type <= Entity::ENEMY_BAT_SWARM) || type == Entity::WATERFALL || type == Entity::KILL_ALL_TRIGGERS || type == Entity::VIEW_TARGET || type == Entity::SOUND_DOOR_BELL || type == Entity::SOUND_ALARM_BELL)
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

            ASSERT(false);
            return 0;
        }

        int getNextRoom(int floorIndex) const {
            if (!floorIndex) return NO_ROOM;
            FloorData *fd = &floors[floorIndex];
        // floor data always in this order and can't be less than uint16 x 3
            if (fd->cmd.func == FloorData::FLOOR)   fd += 2; // skip floor slant info
            if (fd->cmd.func == FloorData::CEILING) fd += 2; // skip ceiling slant info
            if (fd->cmd.func == FloorData::PORTAL)  return (++fd)->data;
            return NO_ROOM;
        }

        Room::Sector& getSector(int roomIndex, int x, int z, int &dx, int &dz) const {
            ASSERT(roomIndex >= 0 && roomIndex < roomsCount);

            if (state.flags.flipped && rooms[roomIndex].alternateRoom > -1)
                roomIndex = rooms[roomIndex].alternateRoom;

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

        Room::Sector* getSector(int16 &roomIndex, int x, int y, int z) const {
            ASSERT(roomIndex >= 0 && roomIndex <= roomsCount);

            Room::Sector *sector = NULL;

        // check horizontal
            while (1) { // Let's Rock!
                TR::Room &room = rooms[roomIndex];

                int sx = (x - room.info.x) >> 10;
                int sz = (z - room.info.z) >> 10;

                if (sz <= 0 || sz >= room.xSectors - 1) {
                    sx = clamp(sx, 0, room.xSectors - 1);
                    sz = clamp(sz, 1, room.zSectors - 2);
                } else
                    sx = clamp(sx, 0, room.xSectors - 1);

                sector = room.sectors + sx * room.zSectors + sz;

                int nextRoom = getNextRoom(sector->floorIndex);
                if (nextRoom == NO_ROOM)
                    break;

                roomIndex = nextRoom;
            };

        // check vertical
            while (sector->roomAbove != NO_ROOM && y < sector->ceiling * 256) {
                TR::Room &room = rooms[roomIndex = sector->roomAbove];
                sector = room.sectors + (x - room.info.x) / 1024 * room.zSectors + (z - room.info.z) / 1024;
            }

            while (sector->roomBelow != NO_ROOM && y >= sector->floor * 256) {
                TR::Room &room = rooms[roomIndex = sector->roomBelow];
                sector = room.sectors + (x - room.info.x) / 1024 * room.zSectors + (z - room.info.z) / 1024;
            }

            return sector;
        }
    }; // struct Level
}

#endif

/* OpenLara - SNES / Super FX target
 * The inventory (FX3): src/fixed/inventory.h's slots, pages and states.
 */

#ifndef H_OL_INVENTORY
#define H_OL_INVENTORY

#include "item.h"

/* upstream's InvSlot */
#define SLOT_LEADBAR        0
#define SLOT_KEY_ITEM_1     1
#define SLOT_KEY_ITEM_2     2
#define SLOT_KEY_ITEM_3     3
#define SLOT_KEY_ITEM_4     4
#define SLOT_PUZZLE_4       5
#define SLOT_PUZZLE_3       6
#define SLOT_PUZZLE_2       7
#define SLOT_PUZZLE_1       8
#define SLOT_SCION          9
#define SLOT_COMPASS        10
#define SLOT_PISTOLS        11
#define SLOT_AMMO_PISTOLS   12
#define SLOT_SHOTGUN        13
#define SLOT_AMMO_SHOTGUN   14
#define SLOT_MAGNUMS        15
#define SLOT_AMMO_MAGNUMS   16
#define SLOT_UZIS           17
#define SLOT_AMMO_UZIS      18
#define SLOT_MEDIKIT_BIG    19
#define SLOT_MEDIKIT_SMALL  20
#define SLOT_PASSPORT       21
#define SLOT_DETAIL         22
#define SLOT_SOUND          23
#define SLOT_CONTROLS       24
#define SLOT_HOME           25
#define SLOT_MAX            26

/* upstream's InvState */
#define INV_STATE_NONE          0
#define INV_STATE_OPENING       1
#define INV_STATE_CLOSING       2
#define INV_STATE_CLOSE         3
#define INV_STATE_READY         4
#define INV_STATE_DEATH         5
#define INV_STATE_SPIN          6
#define INV_STATE_SELECT        7
#define INV_STATE_DESELECT      8
#define INV_STATE_SHOW          9
#define INV_STATE_PAGE_MAIN     10
#define INV_STATE_PAGE_KEYS     11
#define INV_STATE_PAGE_OPTIONS  12

/* upstream's InvPage */
#define INV_PAGE_TITLE      0
#define INV_PAGE_DEATH      1
#define INV_PAGE_END        2
#define INV_PAGE_USE        3
#define INV_PAGE_MAIN       4
#define INV_PAGE_KEYS       5
#define INV_PAGE_OPTIONS    6

/* upstream's PassportPage */
#define PASSPORT_LOAD_GAME  0
#define PASSPORT_SAVE_GAME  1
#define PASSPORT_EXIT       2

/* upstream's OptionType, the ones this cartridge's options use */
#define OPT_SPACE           0
#define OPT_BUTTON          1
#define OPT_BAR             2
#define OPT_SWITCH          3
#define OPT_TEXT            5
#define OPT_MAX             8

/* the options' strings (inventory_text.c) */
#define OSTR_SAVE_1         1       /* upstream's STR_GBA_SAVE_WARNING_1..3 */
#define OSTR_SAVE_2         2
#define OSTR_SAVE_3         3
#define OSTR_OK             4
#define OSTR_CURRENT        5       /* upstream's STR_CURRENT_POSITION */
#define OSTR_GAMMA          6       /* the settings pages' (settings.c) */
#define OSTR_FPS            7
#define OSTR_VSYNC          8
#define OSTR_SFX            9
#define OSTR_MUSIC          10
#define OSTR_RUMBLE         11
#define OSTR_SWAP           12
#define OSTR_LEVEL          16      /* + n: the cartridge's level n, by its LevelID */

/* GS_INV's first word: the ring's item count, and this bit for a frame of
   the inventory's with none (upstream's END page): mesh.gsu FrameBody. */
#define INV_GSU_FRAME       0x8000

#define INV_CAMERA_HEIGHT   (-1536)
#define INV_CAMERA_Y        96
#define INV_CAMERA_Z        768
#define INV_RING_RADIUS     688

extern int16 olInvCounts[SLOT_MAX];
extern unsigned int olInvState, olInvPage, olInvUseSlot;
extern unsigned int olInvItemsCount, olInvItemIndex;
extern uint8 olInvItemsList[SLOT_MAX];
extern int olInvHeight, olInvRadius, olInvPitch, olInvRot, olInvRotItem;
extern int olInvSelDist, olInvSelRotPre, olInvSelRotX, olInvSelRotY;
extern int olInvFrameIndex, olInvFrameTarget;
extern uint16 olInvVisMask;                 /* upstream's itemVisMask */
extern unsigned int olInvNumKeys;
extern unsigned int olInvPassportPage, olInvOptionIndex, olInvOptionsCount, olInvOptionsHeight;
extern uint8 olInvOptType[OPT_MAX], olInvOptStr[OPT_MAX];
extern uint16 olInvOptValue[OPT_MAX];               /* a bar's 0..256, a switch's 0/1 */
extern unsigned int olInvRestart;
extern uint16 olInvText[];                          /* inventory_text.c */
extern uint8 olInvRects[];
extern uint16 olInvGsu[];                           /* inventory_draw.c */

void Inventory_init(void);
void Inventory_addType(unsigned int type);
unsigned int Inventory_count(unsigned int type);
int  Inventory_applyItem(unsigned int holeType);
void Inventory_remove(unsigned int slot, int count);
void Inventory_open(unsigned int page, unsigned int arg, unsigned int keys, unsigned int keysPrev);
void Inventory_update(unsigned int input, int frames);
int  Inventory_use(ItemObj* lara, unsigned int slot);
int  Inventory_tick(ItemObj* lara, unsigned int keys);
void Inventory_levelEnd(unsigned int keys);         /* a level's END trigger */
int  Inventory_modelFrames(unsigned int slot);      /* inventory_draw.c */
int  Inventory_hasModel(unsigned int slot);         /* inventory_draw.c */
void Inventory_publish(void);                       /* inventory_draw.c */
void Inventory_text(void);                          /* inventory_text.c */
void Shadow_publish(void);                          /* inventory_text.c */
extern uint16 olShadow[];
void olRestartLevel(void);                          /* main.asm: the boot again */

#endif

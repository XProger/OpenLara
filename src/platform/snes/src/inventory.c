/* OpenLara - SNES / Super FX target
 * The inventory: src/fixed/inventory.h, FX3 only (GSU-2 has no room).
 *
 * Upstream's ring of item models, opened with Select: its slots, pages
 * (main, keys, options), the open/close/spin/select/show state machine with
 * its timed interpolations, and what choosing an item does -- a medipack
 * heals when the ring closes (Lara::useItem), the pistols are drawn
 * (changeWeapon from FREE), a key or piece is handed to the hole that asked.
 * While it is open the game does not update, as upstream's gameUpdate skips
 * the items; Lara's own tick continues the tick the ring closes.
 *
 * The drawing is the GSU's (Inventory_publish, InvRender in mesh.gsu): the
 * host works out each ring item's whole model-view matrix, as drawPage's
 * matrix calls would, and the GSU draws the model under it.
 *
 * Counts start as upstream's inventory.init() leaves them: the passport,
 * the options, the compass, all four guns, ten small and five large
 * medipacks. Choosing a gun is Lara::changeWeapon (weapon.c Weapon_change).
 *
 * The title page is upstream's too (INV_PAGE_TITLE): a power-on opens it
 * over the first level, frozen at its load (game.c), on the title screen
 * (main.asm CopyTitle); its passport starts a level through the boot.
 */

#include "inventory.h"
#include "lara.h"
#include "level.h"
#include "sound.h"
#include "weapon.h"
#include "camera.h"
#include "save.h"
#include "room.h"
#include "settings.h"

extern Location camTarget;

#define IN_SELECT           IK_SELECT

#define SND_INV_SPIN        108
#define SND_INV_HOME        109
#define SND_INV_CONTROLS    110
#define SND_INV_SHOW        111
#define SND_INV_HIDE        112
#define SND_INV_COMPASS     113
#define SND_INV_WEAPON      114
#define SND_INV_PAGE        115
#define SND_HEALTH          116

#define ANG(x)              ((x) * ANGLE_1)
#define ANGLE_180_32        32768L
#define ANGLE_360_32        65536L

/* upstream's TR1 item types for the pickups and the holes */
#define TYPE_PISTOLS        84
#define TYPE_AMMO_PISTOLS   88
#define TYPE_MEDIKIT_SMALL  93
#define TYPE_MEDIKIT_BIG    94
#define TYPE_PUZZLE_1       110
#define TYPE_PUZZLEHOLE_1   118
#define TYPE_LEADBAR        126
#define TYPE_KEY_ITEM_1     129
#define TYPE_KEYHOLE_1      137
#define TYPE_SCION_Q        143
#define TYPE_SCION_HOLDER   146

typedef struct {
    uint8 type;             /* the model's TR type */
    uint8 snd;
    int16 selDist;
    int16 selRotPre;
    int16 selRotX;
    int16 selRotY;
} InvItem;

/* upstream's INV_SLOTS, in its slot order */
static const InvItem INV_SLOTS[SLOT_MAX] = {
    { 127, SND_INV_SHOW,     256, ANG(20), ANG(44), ANG(67) },    /* LEADBAR */
    { 133, SND_INV_SHOW,     296, ANG(20), ANG(15), ANG(0)  },    /* KEY_ITEM_1 */
    { 134, SND_INV_SHOW,     296, ANG(20), ANG(15), ANG(0)  },
    { 135, SND_INV_SHOW,     296, ANG(20), ANG(15), ANG(0)  },
    { 136, SND_INV_SHOW,     296, ANG(20), ANG(15), ANG(0)  },
    { 117, SND_INV_SHOW,     296, ANG(20), ANG(15), ANG(0)  },    /* PUZZLE_4 */
    { 116, SND_INV_SHOW,     296, ANG(20), ANG(15), ANG(0)  },
    { 115, SND_INV_SHOW,     296, ANG(20), ANG(15), ANG(0)  },
    { 114, SND_INV_SHOW,     296, ANG(20), ANG(15), ANG(0)  },    /* PUZZLE_1 */
    { 150, SND_INV_SHOW,     296, ANG(20), ANG(15), ANG(45) },    /* SCION */
    {  72, SND_INV_COMPASS,  512, ANG(20), ANG(70), ANG(0)  },    /* COMPASS */
    {  99, SND_INV_WEAPON,   296, ANG(17), ANG(20), ANG(0)  },    /* PISTOLS */
    { 103, SND_INV_SHOW,     296, ANG(17), ANG(20), ANG(0)  },
    { 100, SND_INV_WEAPON,   296, ANG(17), ANG(20), ANG(0)  },    /* SHOTGUN */
    { 104, SND_INV_SHOW,     296, ANG(17), ANG(20), ANG(0)  },
    { 101, SND_INV_WEAPON,   296, ANG(17), ANG(20), ANG(0)  },    /* MAGNUMS */
    { 105, SND_INV_SHOW,     296, ANG(17), ANG(20), ANG(0)  },
    { 102, SND_INV_WEAPON,   296, ANG(17), ANG(20), ANG(0)  },    /* UZIS */
    { 106, SND_INV_SHOW,     296, ANG(17), ANG(20), ANG(0)  },
    { 109, SND_INV_SHOW,     352, ANG(20), ANG(44), ANG(22) },    /* MEDIKIT_BIG */
    { 108, SND_INV_SHOW,     216, ANG(22), ANG(40), ANG(22) },    /* MEDIKIT_SMALL */
    {  71, SND_INV_SHOW,     384, ANG(25), ANG(23), ANG(0)  },    /* PASSPORT */
    {  95, SND_INV_SHOW,     424, ANG(22), ANG(36), ANG(0)  },    /* DETAIL */
    {  96, SND_INV_SHOW,     368, ANG(26), ANG(12), ANG(0)  },    /* SOUND */
    {  97, SND_INV_CONTROLS, 352, ANG(30), ANG(67), ANG(0)  },    /* CONTROLS */
    {  73, SND_INV_HOME,     384, ANG(25), ANG(23), ANG(0)  },    /* HOME */
};

#define FRAME_PASSPORT      14
#define FRAME_COMPASS       10

int16 olInvCounts[SLOT_MAX];
unsigned int olInvState, olInvPage, olInvUseSlot;
unsigned int olInvItemsCount, olInvItemIndex;
uint8 olInvItemsList[SLOT_MAX];
int olInvHeight, olInvRadius, olInvPitch, olInvRot, olInvRotItem;
int olInvSelDist, olInvSelRotPre, olInvSelRotX, olInvSelRotY;
int olInvFrameIndex, olInvFrameTarget;

unsigned int olInvNumKeys;                  /* inventory_text.c: the page arrows */
unsigned int olInvPassportPage, olInvOptionIndex, olInvOptionsCount, olInvOptionsHeight;
uint8 olInvOptType[OPT_MAX], olInvOptStr[OPT_MAX];
uint16 olInvOptValue[OPT_MAX];
unsigned int olInvRestart;                  /* Load Game or Exit: the level again */
static unsigned int restartLevel;           /* and which one, */
static unsigned int restartKind;            /* or the title (BOOT_TITLE) */
static unsigned int invNextState, invNextIndex;
static int invTimer;
static int heightTarget, heightInc, radiusTarget, radiusInc;
static int pitchTarget, pitchInc, rotTarget, rotInc, rotItemInc;
static int selDistTarget, selDistInc, selRotPreTarget, selRotPreInc;
static int selRotXTarget, selRotXInc, selRotYTarget, selRotYInc;
static unsigned int invInput, invInputPrev;

static int keyHit(unsigned int key)
{
    return (invInput & key) && !(invInputPrev & key);
}

static void invSound(unsigned int id)
{
    /* upstream's soundPlay(id, NULL): unattenuated, where the view is */
    Sound_play(id, camTarget.x, camTarget.y, camTarget.z);
}

/* The ring's own sounds (SND_INV_*) are in APU RAM only while it is open:
   sound_ring.asm swaps them in over the level's last samples and puts those
   back. A boot clears this, and uploads the level's image whole. */
void olSndRing(unsigned int open);
static unsigned int ringSounds;

static void invRingSounds(unsigned int open)
{
    if (ringSounds == open) return;
    olSndRing(open);
    ringSounds = open;
}

static void invAdd(unsigned int slot, int count)
{
    olInvCounts[slot] += (int16)count;
    if (slot < SLOT_COMPASS) olInvNumKeys += (unsigned int)count;
}

void Inventory_remove(unsigned int slot, int count)
{
    olInvCounts[slot] -= (int16)count;
    if (slot < SLOT_COMPASS) olInvNumKeys -= (unsigned int)count;
}

void Inventory_init(void)
{
    unsigned int i;
    for (i = 0; i < SLOT_MAX; i++) olInvCounts[i] = 0;
    olInvNumKeys = 0;
    olInvState = INV_STATE_NONE;
    olInvUseSlot = SLOT_MAX;
    invAdd(SLOT_PASSPORT, 1);
    invAdd(SLOT_DETAIL, 1);
    invAdd(SLOT_SOUND, 1);
    invAdd(SLOT_CONTROLS, 1);
    invAdd(SLOT_HOME, 1);
    invAdd(SLOT_COMPASS, 1);
    invAdd(SLOT_PISTOLS, 1);
    invAdd(SLOT_SHOTGUN, 1);
    invAdd(SLOT_MAGNUMS, 1);
    invAdd(SLOT_UZIS, 1);
    invAdd(SLOT_MEDIKIT_SMALL, 10);
    invAdd(SLOT_MEDIKIT_BIG, 5);
}

/* remapToSlot, for what she picks up; SLOT_MAX for anything that is not
   carried (upstream asserts) */
static unsigned int invSlotOfType(unsigned int type)
{
    if (type >= TYPE_KEY_ITEM_1 && type < TYPE_KEY_ITEM_1 + 4) return SLOT_KEY_ITEM_1 + (type - TYPE_KEY_ITEM_1);
    if (type >= TYPE_PUZZLE_1 && type < TYPE_PUZZLE_1 + 4) return SLOT_PUZZLE_1 - (type - TYPE_PUZZLE_1);
    if (type == TYPE_LEADBAR) return SLOT_LEADBAR;
    if (type == TYPE_MEDIKIT_SMALL) return SLOT_MEDIKIT_SMALL;
    if (type == TYPE_MEDIKIT_BIG) return SLOT_MEDIKIT_BIG;
    if (type == TYPE_PISTOLS) return SLOT_PISTOLS;
    if (type >= TYPE_PISTOLS + 1 && type <= TYPE_PISTOLS + 3) return SLOT_SHOTGUN + 2 * (type - TYPE_PISTOLS - 1);
    if (type >= TYPE_AMMO_PISTOLS && type <= TYPE_AMMO_PISTOLS + 3) return SLOT_AMMO_PISTOLS + 2 * (type - TYPE_AMMO_PISTOLS);
    if (type == TYPE_SCION_Q || type == TYPE_SCION_Q + 1 || type == TYPE_SCION_HOLDER) return SLOT_SCION;
    return SLOT_MAX;
}

void Inventory_addType(unsigned int type)
{
    unsigned int slot = invSlotOfType(type);
    if (slot < SLOT_MAX) invAdd(slot, 1);
}

unsigned int Inventory_count(unsigned int type)
{
    unsigned int slot = invSlotOfType(type);
    return slot < SLOT_MAX ? (unsigned int)olInvCounts[slot] : 0;
}

/* remapHoleToSlot */
static unsigned int holeSlot(unsigned int holeType)
{
    if (holeType >= TYPE_KEYHOLE_1 && holeType < TYPE_KEYHOLE_1 + 4)
        return SLOT_KEY_ITEM_1 + (holeType - TYPE_KEYHOLE_1);
    if (holeType >= TYPE_PUZZLEHOLE_1 && holeType < TYPE_PUZZLEHOLE_1 + 4)
        return SLOT_PUZZLE_1 - (holeType - TYPE_PUZZLEHOLE_1);
    return SLOT_MAX;
}

/* applyItem: the one chosen on the ring, if it is the one the hole takes,
   spent */
int Inventory_applyItem(unsigned int holeType)
{
    if (olInvUseSlot >= SLOT_MAX || olInvUseSlot != holeSlot(holeType)) return 0;
    Inventory_remove(olInvUseSlot, 1);
    olInvUseSlot = SLOT_MAX;
    return 1;
}

static void setState(unsigned int state, unsigned int next, int timer)
{
    olInvState = state;
    invNextState = next;
    invTimer = timer;
    rotItemInc = -olInvRotItem / timer;
}

static void setHeight(int target)
{
    heightTarget = target;
    heightInc = (target - olInvHeight) / invTimer;
}

static void setRadius(int target)
{
    radiusTarget = target;
    radiusInc = (target - olInvRadius) / invTimer;
}

static void setPitch(int target)
{
    pitchTarget = target;
    pitchInc = (target - olInvPitch) / invTimer;
}

/* delta and target are upstream's int32 angles: ANGLE_180 and 360/n */
static void setRot(int32 delta, int32 target)
{
    rotTarget = (int)target;
    rotInc = (int)(delta / invTimer);
}

static int animLength(void)
{
    unsigned int slot = olInvItemsList[olInvItemIndex];
    if (slot == SLOT_PASSPORT) return FRAME_PASSPORT;
    if (slot == SLOT_COMPASS) return FRAME_COMPASS;
    return Inventory_modelFrames(slot);
}

static void setSelection(int selected)
{
    unsigned int slot = olInvItemsList[olInvItemIndex];
    if (selected) {
        selDistTarget = INV_SLOTS[slot].selDist;
        selRotPreTarget = INV_SLOTS[slot].selRotPre;
        selRotXTarget = INV_SLOTS[slot].selRotX;
        selRotYTarget = INV_SLOTS[slot].selRotY;
    } else {
        selDistTarget = selRotPreTarget = selRotXTarget = selRotYTarget = 0;
    }
    selDistInc = (selDistTarget - olInvSelDist) / invTimer;
    selRotPreInc = (selRotPreTarget - olInvSelRotPre) / invTimer;
    selRotXInc = (selRotXTarget - olInvSelRotX) / invTimer;
    selRotYInc = (selRotYTarget - olInvSelRotY) / invTimer;
    olInvFrameTarget = selected ? animLength() : 0;
    olInvVisMask = 0xFFFF;
    if (selected && slot == SLOT_PASSPORT) {
        olInvPassportPage = olInvPage == INV_PAGE_OPTIONS ? PASSPORT_SAVE_GAME : PASSPORT_LOAD_GAME;
        olInvFrameTarget += (int)olInvPassportPage * 5;
    }
}

static void option(unsigned int type, unsigned int str, unsigned int height, unsigned int value)
{
    if (olInvOptionsCount >= OPT_MAX) return;
    olInvOptType[olInvOptionsCount] = (uint8)type;
    olInvOptStr[olInvOptionsCount] = (uint8)str;
    olInvOptValue[olInvOptionsCount] = (uint16)value;
    olInvOptionsCount++;
    olInvOptionsHeight += height;
}
#define OPTION_SPACE()      option(OPT_SPACE, 0, 8, 0)
#define OPTION_BTN(s)       option(OPT_BUTTON, (s), 18, 0)
#define OPTION_TEXT(s)      option(OPT_TEXT, (s), 18, 0)
#define OPTION_BAR(s, v)    option(OPT_BAR, (s), 18, (v))
#define OPTION_SWITCH(s, v) option(OPT_SWITCH, (s), 18, (v))

/* initOptions: the passport's Load Game lists the levels -- the ones this
   cartridge carries -- and, when the battery RAM holds a save (osCheckSave),
   Current Position, chosen first; the settings pages are upstream's
   (settings.c: what each does here). */
static void initOptions(void)
{
    unsigned int i, slot = olInvItemsList[olInvItemIndex];
    olInvOptionsCount = 0;
    olInvOptionIndex = 0;
    olInvOptionsHeight = 4;
    if (slot == SLOT_PASSPORT && olInvPassportPage == PASSPORT_LOAD_GAME) {
        for (i = 0; i < olLevelCount; i++) OPTION_BTN(OSTR_LEVEL + i);
        if (olSaveValid) {
            OPTION_SPACE();
            OPTION_BTN(OSTR_CURRENT);
            olInvOptionIndex = olInvOptionsCount - 1;
        }
    } else if (slot == SLOT_DETAIL) {
        OPTION_BAR(OSTR_GAMMA, (unsigned int)olSettings.gamma << 4);
        OPTION_SWITCH(OSTR_FPS, olSettings.fps);
        OPTION_SWITCH(OSTR_VSYNC, olSettings.vsync);
    } else if (slot == SLOT_SOUND) {
        OPTION_SWITCH(OSTR_SFX, olSettings.sfx);
        OPTION_SWITCH(OSTR_MUSIC, olSettings.music);
    } else if (slot == SLOT_CONTROLS) {
        OPTION_SWITCH(OSTR_RUMBLE, olSettings.vibration);
        OPTION_SWITCH(OSTR_SWAP, olSettings.swap);
    }
}

#define ADD_SLOT(s) if (olInvCounts[s]) olInvItemsList[olInvItemsCount++] = (uint8)(s)

static void setPage(unsigned int page)
{
    olInvPage = page;
    olInvItemsCount = 0;
    switch (page)
    {
        case INV_PAGE_TITLE:
        case INV_PAGE_DEATH:
        case INV_PAGE_OPTIONS:
            ADD_SLOT(SLOT_PASSPORT);
            ADD_SLOT(SLOT_CONTROLS);
            ADD_SLOT(SLOT_DETAIL);
            ADD_SLOT(SLOT_SOUND);
            /* Lara's Home, if the level has its model: upstream's title
               level does, this cartridge's levels do not (nor the gym) */
            if (page == INV_PAGE_TITLE && Inventory_hasModel(SLOT_HOME)) ADD_SLOT(SLOT_HOME);
            break;
        case INV_PAGE_MAIN:
            ADD_SLOT(SLOT_COMPASS);
            ADD_SLOT(SLOT_PISTOLS);
            ADD_SLOT(SLOT_SHOTGUN);
            ADD_SLOT(SLOT_MAGNUMS);
            ADD_SLOT(SLOT_UZIS);
            ADD_SLOT(SLOT_MEDIKIT_BIG);
            ADD_SLOT(SLOT_MEDIKIT_SMALL);
            break;
        case INV_PAGE_USE:
        case INV_PAGE_KEYS:
            ADD_SLOT(SLOT_LEADBAR);
            ADD_SLOT(SLOT_KEY_ITEM_1);
            ADD_SLOT(SLOT_KEY_ITEM_2);
            ADD_SLOT(SLOT_KEY_ITEM_3);
            ADD_SLOT(SLOT_KEY_ITEM_4);
            ADD_SLOT(SLOT_PUZZLE_4);
            ADD_SLOT(SLOT_PUZZLE_3);
            ADD_SLOT(SLOT_PUZZLE_2);
            ADD_SLOT(SLOT_PUZZLE_1);
            ADD_SLOT(SLOT_SCION);
            break;
    }
    olInvItemIndex = invNextIndex = 0;
    if (olInvItemsCount > 0)
        setRot(ANGLE_180_32, (int32)olInvItemIndex * ANGLE_360_32 / (int32)olInvItemsCount);
    olInvRot = (int)((unsigned int)rotTarget + 0x8000u);     /* rotTarget - ANGLE_180 */
}

void Inventory_open(unsigned int page, unsigned int arg, unsigned int keys, unsigned int keysPrev)
{
    invInput = keysPrev;             /* upstream's update(1) sees lastInput, input */
    restartKind = BOOT_START;
    if (page == INV_PAGE_END) {
        /* upstream's alpha end: no ring, straight to READY on the page */
        olInvRestart = 0;
        olInvPage = INV_PAGE_END;
        olInvState = INV_STATE_READY;
        olInvItemsCount = 0;
        invSound(SND_HEALTH);
        return;
    }
    olInvHeight = -1536;                /* INV_CAMERA_HEIGHT */
    /* the title's tilt, which upstream's opening never sets a target for:
       pitchTarget's at the end of it (0 at a power-on) */
    olInvPitch = page == INV_PAGE_TITLE ? 1024 : 0;
    olInvRadius = 0;
    olInvFrameIndex = 0;
    olInvVisMask = 0xFFFF;
    olInvUseSlot = SLOT_MAX;
    olInvRestart = 0;
    olInvOptionsCount = 0;
    olInvRotItem = 0;
    olInvSelDist = olInvSelRotPre = olInvSelRotX = olInvSelRotY = 0;
    setState(INV_STATE_OPENING, page == INV_PAGE_DEATH ? INV_STATE_DEATH : INV_STATE_READY, 16);
    setHeight(-256);
    setRadius(INV_RING_RADIUS);
    setPage(page);
    if (page == INV_PAGE_USE) {
        /* the hole's own key or piece first: getItemIndexForSlot */
        unsigned int i, slot = holeSlot(arg);
        olInvItemIndex = invNextIndex = 0;
        for (i = 0; i < olInvItemsCount; i++)
            if (olInvItemsList[i] == slot) { olInvItemIndex = invNextIndex = i; break; }
        setRot(ANGLE_180_32, (int32)olInvItemIndex * ANGLE_360_32 / (int32)olInvItemsCount);
        olInvRot = (int)((unsigned int)rotTarget + 0x8000u);
    }
    invRingSounds(1);
    invSound(SND_INV_SHOW);
    Inventory_update(keys, 1);
}

static void invClose(void)
{
    setState(INV_STATE_CLOSING, INV_STATE_NONE, 16);
    setHeight(-1536);
    setRadius(0);
    setRot(ANGLE_180_32, (int32)olInvRot - ANGLE_180_32);
}

static int animate(int frames)
{
    int moving = 1;
    if (olInvFrameIndex == olInvFrameTarget) return 0;
    if (olInvFrameIndex < olInvFrameTarget) {
        olInvFrameIndex += frames;
        if (olInvFrameIndex >= olInvFrameTarget) { olInvFrameIndex = olInvFrameTarget; moving = 0; }
    } else {
        olInvFrameIndex -= frames;
        if (olInvFrameIndex <= olInvFrameTarget) { olInvFrameIndex = olInvFrameTarget; moving = 0; }
    }
    Settings_visMask();                 /* updateVisMask, each way out */
    return moving;
}

/* onOption: Up/Down through the options, past spaces and text; Left and
   Right move a bar by 16 of 256 and turn a switch over */
static void onOption(void)
{
    unsigned int n, v;
    if (!olInvOptionsCount) return;
    if (keyHit(IN_DOWN)) {
        for (n = 0; n < olInvOptionsCount; n++) {
            if (++olInvOptionIndex >= olInvOptionsCount) olInvOptionIndex = 0;
            if (olInvOptType[olInvOptionIndex] != OPT_SPACE && olInvOptType[olInvOptionIndex] != OPT_TEXT) break;
        }
    }
    if (keyHit(IN_UP)) {
        for (n = 0; n < olInvOptionsCount; n++) {
            olInvOptionIndex = olInvOptionIndex ? olInvOptionIndex - 1 : olInvOptionsCount - 1;
            if (olInvOptType[olInvOptionIndex] != OPT_SPACE && olInvOptType[olInvOptionIndex] != OPT_TEXT) break;
        }
    }
    v = olInvOptValue[olInvOptionIndex];
    if (olInvOptType[olInvOptionIndex] == OPT_BAR) {
        if (keyHit(IN_LEFT)) v = v > 16 ? v - 16 : 0;
        if (keyHit(IN_RIGHT)) v = v < 240 ? v + 16 : 256;
    } else if (olInvOptType[olInvOptionIndex] == OPT_SWITCH) {
        if (keyHit(IN_LEFT)) v = !v;    /* no call right of ||: README */
        else if (keyHit(IN_RIGHT)) v = !v;
    }
    olInvOptValue[olInvOptionIndex] = (uint16)v;
}

static void passportTurn(unsigned int page, int frames)
{
    olInvPassportPage = page;
    olInvFrameTarget += frames;
    initOptions();
    invSound(SND_INV_PAGE);
}

/* onPassport. Load Game's levels start the one chosen and Exit to Title the
   title page, through the boot (nextLevel); Current Position starts the
   save's level and loads the save over it (upstream's LVL_LOAD); Save Game
   is gameSave into the battery RAM (save.c), and the warning and OK if it
   does not read back. The title's passport has Load Game alone. */
static void onPassport(void)
{
    if (olInvPassportPage == PASSPORT_SAVE_GAME && olInvOptionsCount > 0) {   /* the warning */
        if (keyHit(IN_ACTION) || keyHit(IN_JUMP)) {
            olInvFrameTarget = 0;
            invNextState = INV_STATE_CLOSE;
        }
        return;
    }
    if (olInvPage == INV_PAGE_OPTIONS || olInvPage == INV_PAGE_DEATH) {
        if (olInvPassportPage == PASSPORT_LOAD_GAME && keyHit(IN_RIGHT))      passportTurn(PASSPORT_SAVE_GAME, 5);
        else if (olInvPassportPage == PASSPORT_SAVE_GAME && keyHit(IN_RIGHT)) passportTurn(PASSPORT_EXIT, 5);
        else if (olInvPassportPage == PASSPORT_EXIT && keyHit(IN_LEFT))       passportTurn(PASSPORT_SAVE_GAME, -5);
        else if (olInvPassportPage == PASSPORT_SAVE_GAME && keyHit(IN_LEFT))  passportTurn(PASSPORT_LOAD_GAME, -5);
    }
    if (keyHit(IN_ACTION)) {
        if (olInvPassportPage == PASSPORT_SAVE_GAME && Save_write()) {
            olInvFrameTarget = 0;
            invNextState = INV_STATE_CLOSE;
        } else if (olInvPassportPage == PASSPORT_SAVE_GAME) {
            olInvOptionsHeight = 4;
            olInvOptionsCount = 0;
            OPTION_SPACE();
            OPTION_TEXT(OSTR_SAVE_1);
            OPTION_TEXT(OSTR_SAVE_2);
            OPTION_TEXT(OSTR_SAVE_3);
            OPTION_SPACE();
            OPTION_BTN(OSTR_OK);
            olInvOptionIndex = 5;               /* OPT_ID_OK */
        } else {
            /* nextLevel: the chosen level, LVL_LOAD, or the title */
            unsigned int str = olInvOptStr[olInvOptionIndex];
            olInvRestart = 1;
            restartLevel = 0;
            if (olInvPassportPage == PASSPORT_EXIT) restartKind = BOOT_TITLE;
            else if (str == OSTR_CURRENT) olInvRestart = 2;
            else if (str >= OSTR_LEVEL) restartLevel = str - OSTR_LEVEL;
            olInvFrameTarget = 0;
            invNextState = INV_STATE_CLOSE;
        }
    }
}

/* onItem: what Action on the shown item does; a passport, compass or
   option is put back with Jump or Select. */
static int onItem(void)
{
    unsigned int slot;
    onOption();
    if (invInput & (IN_JUMP | IN_SELECT)) {
        olInvFrameTarget = 0;
        invNextState = INV_STATE_READY;
    }
    slot = olInvItemsList[olInvItemIndex];
    switch (slot)
    {
        case SLOT_COMPASS:
            if (invInput & IN_ACTION) {
                olInvFrameTarget = 0;
                invNextState = INV_STATE_CLOSE;
            }
            break;
        case SLOT_PASSPORT:
            onPassport();
            break;
        case SLOT_DETAIL:
        case SLOT_SOUND:
        case SLOT_CONTROLS:
            Settings_option();          /* onDetail, onSound, onControls */
            break;
        case SLOT_HOME:
            /* onHome is nextLevel(LVL_TR1_GYM), and the gym is not on
               this cartridge: Lara's Home is shown, and stays shut */
            break;
        default:
            if (slot < SLOT_PASSPORT) {     /* keys, pieces, weapons, medipacks */
                invNextState = INV_STATE_CLOSE;
                olInvUseSlot = slot;
            }
            break;
    }
    return invNextState == INV_STATE_NONE;
}

static void toPage(unsigned int next, int pitch)
{
    setState(INV_STATE_CLOSING, next, 12);
    setRadius(0);
    setRot(ANGLE_180_32, (int32)olInvRot - ANGLE_180_32);
    setPitch(pitch);
}

static void fromPage(unsigned int page)
{
    olInvPitch = -olInvPitch;
    setState(INV_STATE_OPENING, INV_STATE_READY, 12);
    setRadius(INV_RING_RADIUS);
    setPitch(0);
    setPage(page);
}

void Inventory_update(unsigned int input, int frames)
{
    invInputPrev = invInput;
    invInput = input;

    /* onEnd: Action, Jump or Select, and nextLevel(LVL_TR1_TITLE) */
    if (olInvPage == INV_PAGE_END) {
        if (keyHit(IN_ACTION) || keyHit(IN_JUMP) || keyHit(IN_SELECT)) {
            olInvRestart = 1;
            restartLevel = 0;
            restartKind = BOOT_TITLE;
            olInvState = INV_STATE_NONE;
        }
        return;
    }

    if (olInvState != INV_STATE_SHOW) olInvRotItem += rotItemInc * frames;

    if (invTimer > 0)
    {
        invTimer -= frames;
        olInvHeight += heightInc * frames;
        olInvRadius += radiusInc * frames;
        olInvPitch += pitchInc * frames;
        olInvRot += rotInc * frames;
        olInvSelDist += selDistInc * frames;
        olInvSelRotPre += selRotPreInc * frames;
        olInvSelRotX += selRotXInc * frames;
        olInvSelRotY += selRotYInc * frames;
        if (invTimer <= 0)
        {
            invTimer = 0;
            olInvState = invNextState;
            invNextState = INV_STATE_NONE;
            olInvHeight = heightTarget;
            olInvRadius = radiusTarget;
            olInvPitch = pitchTarget;
            olInvRot = rotTarget;
            olInvSelDist = selDistTarget;
            olInvSelRotPre = selRotPreTarget;
            olInvSelRotX = selRotXTarget;
            olInvSelRotY = selRotYTarget;
            heightInc = radiusInc = pitchInc = rotInc = 0;
            olInvRotItem = 0;
            rotItemInc = 512;
            selDistInc = selRotPreInc = selRotXInc = selRotYInc = 0;
            olInvItemIndex = invNextIndex;
        }
    }

    switch (olInvState)
    {
        case INV_STATE_CLOSE:
            invClose();
            break;

        case INV_STATE_READY:
            if ((invInput & IN_LEFT) && olInvItemsCount > 1) {
                invSound(SND_INV_SPIN);
                invNextIndex = olInvItemIndex + 1;
                if (invNextIndex >= olInvItemsCount) invNextIndex -= olInvItemsCount;
                setState(INV_STATE_SPIN, INV_STATE_READY, 12);
                setRot(ANGLE_360_32 / (int32)olInvItemsCount,
                       (int32)invNextIndex * ANGLE_360_32 / (int32)olInvItemsCount);
            } else if ((invInput & IN_RIGHT) && olInvItemsCount > 1) {
                invSound(SND_INV_SPIN);
                invNextIndex = olInvItemIndex ? olInvItemIndex - 1 : olInvItemsCount - 1;
                setState(INV_STATE_SPIN, INV_STATE_READY, 12);
                setRot(-ANGLE_360_32 / (int32)olInvItemsCount,
                       (int32)invNextIndex * ANGLE_360_32 / (int32)olInvItemsCount);
            } else if (invInput & IN_UP) {
                if (olInvPage == INV_PAGE_OPTIONS) toPage(INV_STATE_PAGE_MAIN, ANG(45));
                else if (olInvPage == INV_PAGE_MAIN && olInvNumKeys > 0) toPage(INV_STATE_PAGE_KEYS, ANG(45));
            } else if (invInput & IN_DOWN) {
                if (olInvPage == INV_PAGE_KEYS) toPage(INV_STATE_PAGE_MAIN, -ANG(45));
                else if (olInvPage == INV_PAGE_MAIN) toPage(INV_STATE_PAGE_OPTIONS, -ANG(45));
            } else if ((keyHit(IN_SELECT) || keyHit(IN_JUMP)) && olInvPage != INV_PAGE_TITLE) {
                invSound(SND_INV_HIDE);
                olInvUseSlot = SLOT_MAX;
                invClose();
            } else if (invInput & IN_ACTION) {
                invSound(INV_SLOTS[olInvItemsList[olInvItemIndex]].snd);
                setState(INV_STATE_SELECT, INV_STATE_SHOW, 8);
                setSelection(1);
                initOptions();
            }
            break;

        case INV_STATE_DEATH:
            invSound(INV_SLOTS[olInvItemsList[olInvItemIndex]].snd);
            setState(INV_STATE_SELECT, INV_STATE_SHOW, 8);
            setSelection(1);
            initOptions();
            break;

        case INV_STATE_SHOW:
            olInvRotItem = 0;
            if (animate(frames)) break;
            if (!onItem() && olInvFrameIndex == olInvFrameTarget) {
                setState(INV_STATE_DESELECT, invNextState, 8);
                setSelection(0);
            }
            break;

        case INV_STATE_PAGE_MAIN:    fromPage(INV_PAGE_MAIN); break;
        case INV_STATE_PAGE_KEYS:    fromPage(INV_PAGE_KEYS); break;
        case INV_STATE_PAGE_OPTIONS: fromPage(INV_PAGE_OPTIONS); break;
    }
}

/* nextLevel(gLevelID + 1), for a level's END trigger: the cartridge's next
   level through the boot (startLevel); past its last, the alpha's end page,
   which upstream opens for LVL_TR1_3A -- the level after LEVEL2 -- and
   whose Action goes to the title. */
void Inventory_levelEnd(unsigned int keys)
{
    olLevelEnd = 0;
    if (olLevelIndex + 1 < olLevelCount) {
        Save_boot(olLevelIndex + 1, BOOT_START);
        olRestartLevel();
    }
    Inventory_open(INV_PAGE_END, 0, keys, keys);
}

/* Lara::useItem, the tick the ring has closed. */
int Inventory_use(ItemObj* lara, unsigned int slot)
{
    switch (slot)
    {
        case SLOT_PISTOLS:
            Weapon_change(WEAPON_PISTOLS);
            return 1;
        case SLOT_SHOTGUN:
            Weapon_change(WEAPON_SHOTGUN);
            return 1;
        case SLOT_MAGNUMS:
            Weapon_change(WEAPON_MAGNUMS);
            return 1;
        case SLOT_UZIS:
            Weapon_change(WEAPON_UZIS);
            return 1;
        case SLOT_MEDIKIT_BIG:
        case SLOT_MEDIKIT_SMALL:
            if (lara->health < LARA_MAX_HEALTH) {
                lara->health = (int16)(lara->health + (slot == SLOT_MEDIKIT_BIG ? LARA_MAX_HEALTH
                                                                               : (LARA_MAX_HEALTH >> 1)));
                if (lara->health > LARA_MAX_HEALTH) lara->health = LARA_MAX_HEALTH;
                Inventory_remove(slot, 1);
                Sound_playItem(SND_HEALTH, lara);
            }
            return 1;
    }
    return 0;
}

/* The start of each game tick: the ring's update while it is open, and
   upstream's gameUpdate hand-over when it closes. 1: the game is paused. */
int Inventory_tick(ItemObj* lara, unsigned int keys)
{
    if (olInvState == INV_STATE_NONE) return 0;
    Inventory_update(keys, 1);
    if (olInvState != INV_STATE_NONE) return 1;
    if (olInvRestart) {                         /* main.asm: the boot again */
        if (olInvRestart == 2) Save_requestLoad();
        else Save_boot(restartLevel, restartKind);
        olRestartLevel();
    }
    invRingSounds(0);                           /* the level's sounds again */
    if (Inventory_use(lara, olInvUseSlot)) olInvUseSlot = SLOT_MAX;
    return 0;
}

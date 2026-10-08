/* OpenLara - SNES / Super FX target
 * The inventory's text (FX3): upstream's drawText, and the strings its
 * Inventory::draw puts up -- the page title and arrows, the chosen item's
 * name and its count -- laid out here in the glyphs' own pixels and handed
 * to the GSU (gsu/mesh.gsu TextRender), which plots them over the ring.
 *
 * FONT.BIN (packer buildFont), in CPU ROM: the glyphs' palette and the
 * background's greys, the level's upstream LevelID, the GSU bank the glyphs
 * and the grey remap are in, the remap's address, and per glyph its trimmed
 * block -- the address, w, h, and the block's corner from the pen. The pen
 * advances by upstream's char_width, as upstream's does, not by the blocks.
 */

#include "inventory.h"
#include "level.h"
#include "olhost.h"
#include "settings.h"

#define OL_FPS          (*(uint16 *)0x7E0000LL) /* main.asm's direct page: fps */

extern int ol_floorEff;
extern unsigned int ol_animFreeze;

#define FRAME_W         216
#define FRAME_H         144
#define INV_TEXT_MAX    64

#define FONT_LEVEL      56
#define FONT_COUNT      57
#define FONT_BANK       58
#define FONT_REMAP      60
#define FONT_DARK       62
#define FONT_GLYPHS     64

#define INV_RECT_MAX    24      /* the box, its border, the chosen one's, a bar */
#define OPTIONS_WIDTH   192     /* upstream's 216 of 240: 12 either side */

/* renderLine/renderFill colours: TR palette 14, 10 and 15 at CG-RAM
   184-186 (FONT.BIN); 0 is renderFill's darkening */
#define COL_DARKEN      0
#define COL_BORDER_1    0xB8
#define COL_BORDER_2    0xB9
#define COL_SELECT      0xBA
/* renderBar's BAR_OPTION: TR 43 44 43 42 41 (FONT.BIN's tail, CG-RAM
   188-191), the border TR 19 and 17 (the HUD's palette 0, 135-136) */
#define COL_BAR_41      0xBC
#define COL_BAR_19      0x87
#define COL_BAR_17      0x88

#define ALIGN_LEFT      0
#define ALIGN_CENTER    1
#define ALIGN_RIGHT     2

/* upstream's LevelID */
#define LVL_TR1_2       3
#define LVL_TR1_3A      4
#define LVL_TR1_4       7
#define LVL_TR1_5       8
#define LVL_TR1_6       9
#define LVL_TR1_7A      10
#define LVL_TR1_7B      11
#define LVL_TR1_8A      13
#define LVL_TR1_8B      14
#define LVL_TR1_8C      15
#define LVL_TR1_10A     16
#define LVL_TR1_EGYPT   21
#define LVL_TR1_CAT     22

/* For the GSU: a count, the font's bank relative to the level's, the grey
   and the darkening remaps' addresses there (mesh.gsu BgCapture,
   RectRender), then per glyph x, y, its address, w | h << 8. ItemsExchange
   copies it to GS_TEXT; and the rectangles, a count and per rectangle x, y,
   w, h, colour, 0, to GS_RECT. */
uint16 olInvText[4 + INV_TEXT_MAX * 4];
uint8 olInvRects[2 + INV_RECT_MAX * 6];

#ifndef OL_C_MOTION
/* The native helper compares every input exactly. Its snapshot holds no
   output: olInvText/olInvRects remain the renderer's authoritative records.
   C_MOTION deliberately composes every time as the executable oracle. */
unsigned int olInvTextCacheValid;
uint16 olInvTextCache[66];
int Inventory_textCached(void);       /* inventory_text_cache.asm */
#endif

static uint16* textOut;
static unsigned int textCount, fontCount, rectCount;

static const uint8 charWidth[110] = {
    14, 11, 11, 11, 11, 11, 11, 13, 8, 11, 12, 11, 13, 13, 12, 11, 12, 12, 11, 12, 13, 13, 13, 12, 12, 11, /* A-Z */
    9, 9, 9, 9, 9, 9, 9, 9, 5, 9, 9, 5, 12, 10, 9, 9, 9, 8, 9, 8, 9, 9, 11, 9, 9, 9,                    /* a-z */
    12, 8, 10, 10, 10, 10, 10, 9, 10, 10,                                                               /* 0-9 */
    5, 5, 5, 11, 9, 7, 8, 6, 0, 7, 7, 3, 8, 8, 13, 7, 9, 4, 12, 12,
    7, 5, 7, 7, 7, 7, 7, 7, 7, 7, 16, 14, 14, 14, 16, 16, 16, 16, 16, 12, 14, 8, 8, 8, 8, 8, 8, 8
};

static const uint8 charMap[102] = {
    0, 64, 66, 78, 77, 74, 78, 79, 69, 70, 92, 72, 63, 71, 62, 68, 52, 53, 54, 55, 56, 57, 58, 59,
    60, 61, 73, 73, 66, 74, 75, 65, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 80, 76, 81, 97, 98, 77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 100, 101, 102, 67, 0, 0, 0, 0, 0, 0, 0
};

/* INV_SLOTS' str */
static const char* const slotName[SLOT_MAX] = {
    "Lead Bar", "Key", "Key", "Key", "Key", "Puzzle", "Puzzle", "Puzzle", "Puzzle", "Scion",
    "Compass", "Pistols", "Pistol Clips", "Shotgun", "Shotgun Shells", "Magnums", "Magnum Clips",
    "Uzis", "Uzi Clips", "Large Medi Pack", "Small Medi Pack", "Game", "Graphics", "Sound",
    "Controls", "Lara's Home"
};

/* getItemStr's LVLCHECKs: level, slot, name */
typedef struct { uint8 level, slot; const char* name; } LevelName;
static const LevelName levelName[] = {
    { LVL_TR1_2,     SLOT_KEY_ITEM_1, "Silver Key" },
    { LVL_TR1_2,     SLOT_PUZZLE_1,   "Gold Idol" },
    { LVL_TR1_3A,    SLOT_PUZZLE_1,   "Machine Cog" },
    { LVL_TR1_4,     SLOT_KEY_ITEM_1, "Neptune Key" },
    { LVL_TR1_4,     SLOT_KEY_ITEM_2, "Atlas Key" },
    { LVL_TR1_4,     SLOT_KEY_ITEM_3, "Damocles Key" },
    { LVL_TR1_4,     SLOT_KEY_ITEM_4, "Thor Key" },
    { LVL_TR1_5,     SLOT_KEY_ITEM_1, "Rusty Key" },
    { LVL_TR1_6,     SLOT_PUZZLE_1,   "Gold Bar" },
    { LVL_TR1_7A,    SLOT_KEY_ITEM_1, "Gold Key" },
    { LVL_TR1_7A,    SLOT_KEY_ITEM_2, "Silver Key" },
    { LVL_TR1_7A,    SLOT_KEY_ITEM_3, "Rusty Key" },
    { LVL_TR1_7B,    SLOT_KEY_ITEM_1, "Gold Key" },
    { LVL_TR1_7B,    SLOT_KEY_ITEM_2, "Rusty Key" },
    { LVL_TR1_7B,    SLOT_KEY_ITEM_3, "Rusty Key" },
    { LVL_TR1_8A,    SLOT_KEY_ITEM_1, "Sapphire Key" },
    { LVL_TR1_8B,    SLOT_KEY_ITEM_1, "Sapphire Key" },
    { LVL_TR1_8B,    SLOT_PUZZLE_2,   "Scarab" },
    { LVL_TR1_8B,    SLOT_PUZZLE_3,   "Eye of Horus" },
    { LVL_TR1_8B,    SLOT_PUZZLE_4,   "Ankh" },
    { LVL_TR1_8B,    SLOT_PUZZLE_1,   "Eye of Horus" },
    { LVL_TR1_8C,    SLOT_KEY_ITEM_1, "Gold Key" },
    { LVL_TR1_8C,    SLOT_PUZZLE_1,   "Ankh" },
    { LVL_TR1_8C,    SLOT_PUZZLE_2,   "Scarab" },
    { LVL_TR1_10A,   SLOT_PUZZLE_1,   "Fuse" },
    { LVL_TR1_10A,   SLOT_PUZZLE_2,   "Pyramid Key" },
    { LVL_TR1_EGYPT, SLOT_KEY_ITEM_1, "Gold Key" },
    { LVL_TR1_CAT,   SLOT_KEY_ITEM_1, "Ornate Key" },
};

/* upstream's STR_TR1_*, by LevelID */
static const char* const levelTitle[] = {
    "", "Lara's Home", "Caves", "City of Vilcabamba", "Lost Valley", "Tomb of Qualopec",
    "", "St. Francis' Folly", "Colosseum", "Palace Midas", "The Cistern", "Tomb of Tihocan",
    "", "City of Khamoon", "Obelisk of Khamoon", "Sanctuary of the Scion", "Natla's Mines",
    "", "Atlantis", "", "The Great Pyramid", "Return to Egypt", "Temple of the Cat"
};

/* STR_GBA_SAVE_WARNING_1..3's case on this cartridge (the save did not
   read back), STR_OK, STR_CURRENT_POSITION, the settings pages' labels;
   OSTR_LEVEL entries */
static const char* const optionText[OSTR_LEVEL] = {
    0, "No back-up RAM:", "a saved game", "cannot be kept.", "OK", "Current Position",
    "Gamma", "Show FPS", "VSync", "SFX", "Music", "Rumble", "Swap A B", "", "", ""
};

static const unsigned char* fontAt(unsigned int offset)
{
    union { const unsigned char* p; uint16 w[2]; } u;
    u.w[0] = (uint16)(olLevel.fontAt + offset);   /* bank-contained */
    u.w[1] = olLevel.fontBank;
    return u.p;
}

static unsigned int charRemap(unsigned int c)
{
    if (c < 11) return c + 81;
    if (c < 16) return c + 91;
    if (c < 32 || c >= 32 + 102) return 0;
    return charMap[c - 32];
}

static int textWidth(const char* s)
{
    int w = 0;
    unsigned int c;
    while ((c = (unsigned char)*s++) != 0) {
        if (c == ' ') { w += 6; continue; }
        w += charWidth[charRemap(c)] + 1;
    }
    return w;
}

/* renderGlyph: upstream's rasterizer clips a glyph; this frame's text never
   reaches an edge, so one that would is left out whole */
static void glyph(int x, int y, unsigned int index)
{
    const unsigned char* r;
    unsigned int w, h;
    if (textCount >= INV_TEXT_MAX || index >= fontCount) return;
    r = fontAt(FONT_GLYPHS + index * 6);
    w = r[2];
    h = r[3];
    if (!w) return;                     /* two tests, not || !: tools/tcclint.py */
    if (!h) return;
    x += (int)(signed char)r[4];
    y += (int)(signed char)r[5];
    if (x < 0 || y < 0 || x + (int)w > FRAME_W || y + (int)h > FRAME_H) return;
    textOut[0] = (uint16)x;
    textOut[1] = (uint16)y;
    textOut[2] = olData16(r, 0);
    textOut[3] = (uint16)(w | (h << 8));
    textOut += 4;
    textCount++;
}

static void drawText(int x, int y, const char* s, unsigned int align)
{
    unsigned int c, index;
    if (!s) return;                     /* two tests, not || !: tools/tcclint.py */
    if (!*s) return;
    if (align == ALIGN_CENTER) x += (FRAME_W - textWidth(s)) >> 1;
    if (align == ALIGN_RIGHT)  x += FRAME_W - textWidth(s);
    while ((c = (unsigned char)*s++) != 0) {
        if (c == ' ') { x += 6; continue; }
        if (c == '$') index = (unsigned char)*s++;     /* special char */
        else index = charRemap(c);
        glyph(x, c == 'p' ? y - 1 : y, index);          /* upstream's 'p' */
        x += charWidth[index] + 1;
    }
}

/* renderFill / renderLine: a rectangle for the GSU */
static void rect(int x, int y, int w, int h, unsigned int colour)
{
    uint8* r;
    if (rectCount >= INV_RECT_MAX || w <= 0 || h <= 0) return;
    if (x < 0 || y < 0 || x + w > FRAME_W || y + h > FRAME_H) return;
    r = olInvRects + 2 + rectCount * 6;
    r[0] = (uint8)x; r[1] = (uint8)y; r[2] = (uint8)w; r[3] = (uint8)h;
    r[4] = (uint8)colour; r[5] = 0;
    rectCount++;
}

static void border(int x, int y, int w, int h, unsigned int c1, unsigned int c2)
{
    rect(x + 1, y, w - 2, 1, c1);
    rect(x + 1, y + h - 1, w - 2, 1, c2);
    rect(x, y, 1, h, c1);
    rect(x + w - 1, y, 1, h, c2);
}

static const char* optionName(unsigned int str)
{
    unsigned int level;
    if (str < OSTR_LEVEL) return optionText[str];
    level = olLevelIds[str - OSTR_LEVEL];
    return level < sizeof(levelTitle) / sizeof(levelTitle[0]) ? levelTitle[level] : "";
}

/* renderBar(x, y, width, value, BAR_OPTION): five lines of the bar's
   colours as far as value / 256 goes, renderFill past it (the one
   darkening there is, shade 25, for upstream's 27), renderBorder round */
static const uint8 barRows[5] = { 2, 3, 2, 1, 0 };     /* TR 43 44 43 42 41 */

static void bar(int x, int y, int width, int value)
{
    unsigned int i;
    int w = (value * width) >> 8;
    if (w > 0)
        for (i = 0; i < 5; i++) rect(x + 1, y + 1 + (int)i, w, 1, COL_BAR_41 + barRows[i]);
    if (w < width) rect(x + 1 + w, y + 1, width - w, 5, COL_DARKEN);
    border(x, y, width + 2, 7, COL_BAR_19, COL_BAR_17);
}

/* drawOptions: the box, its border, the chosen one's frame, the labels */
static void drawOptions(void)
{
    int w = OPTIONS_WIDTH, h = (int)olInvOptionsHeight;
    int y = (FRAME_H - h) / 2 - 12;
    unsigned int i;
    if (olInvState != INV_STATE_SHOW) return;   /* two tests, not || !: tools/tcclint.py */
    if (!olInvOptionsCount) return;
    rect((FRAME_W - w) / 2 + 1, y + 1, w - 2, h - 2, COL_DARKEN);
    border((FRAME_W - w) / 2, y, w, h, COL_BORDER_1, COL_BORDER_2);
    w -= 4;
    h = 18;
    y += 2;
    for (i = 0; i < olInvOptionsCount; i++) {
        if (olInvOptionIndex == i) border((FRAME_W - w) / 2, y, w, h, COL_SELECT, COL_SELECT);
        if (olInvOptType[i] == OPT_SPACE) y -= 10;
        else if (olInvOptType[i] == OPT_BAR) {
            bar(FRAME_W / 2, y + 6, 80, (int)olInvOptValue[i]);
            drawText(-FRAME_W / 2 - 8, y + 16, optionName(olInvOptStr[i]), ALIGN_RIGHT);
        } else if (olInvOptType[i] == OPT_SWITCH) {
            drawText(-FRAME_W / 2 - 8, y + 16, optionName(olInvOptStr[i]), ALIGN_RIGHT);
            if (olInvOptValue[i]) drawText(44, y + 16, "On", ALIGN_CENTER);  /* not ?: (816-tcc) */
            else drawText(44, y + 16, "Off", ALIGN_CENTER);
        }
        else drawText(0, y + 16, optionName(olInvOptStr[i]), ALIGN_CENTER);
        y += 18;
    }
}

static const char* itemName(unsigned int slot)
{
    unsigned int i, level = fontAt(FONT_LEVEL)[0];
    if (olInvState == INV_STATE_SHOW && slot == SLOT_PASSPORT) {
        if (olInvPassportPage == PASSPORT_LOAD_GAME) return "Load Game";
        if (olInvPassportPage == PASSPORT_SAVE_GAME) return "Save Game";
        return "Exit to Title";
    }
    for (i = 0; i < sizeof(levelName) / sizeof(levelName[0]); i++)
        if (levelName[i].level == level && levelName[i].slot == slot)
            return levelName[i].name;
    return slotName[slot];
}

/* The shadows: upstream's drawShadow(item, 160) for hers and every
   creature's (Enemy sets ITEM_FLAG_SHADOW). Per shadow the entity's slot
   in GS_ITEM, the floor's depth below it, its frame box's middle and
   160/1024 of its size (5/32 here, in sixteen bits); the GSU's ItemTilt
   finds its entity's record as it draws it and ShadowRender builds
   renderShadow's octagon from it, on the floor, darkened through the
   font's second remap (renderFill's) -- before the entity itself. Hers,
   then the seven nearest creatures still to be seen; published with the
   frame's entities, from the same tick. None under the ring, without the
   font's tables, while the poses are frozen, or over no floor (upstream's
   WALL). */
#define SHADOW_MAX      8
uint16 olShadow[3 + SHADOW_MAX * 6];

#ifndef OL_C_MOTION
/* Publication scratch for shadow_fx3.asm: counters, selected slots and
   exact sixteen-bit distances. The output itself is never cached. */
uint16 olShadowWork[23];
#else
static void shadowOf(uint16* out, unsigned int i, int floor)
{
    const AABBi* b = ItemObj_frameBox(items + i);
    out[0] = (uint16)(0xF800 + i * 12);         /* GS_ITEM + slot */
    out[1] = (uint16)(floor - items[i].y);
    out[2] = (uint16)((b->maxX + b->minX) >> 1);
    out[3] = (uint16)((b->maxZ + b->minZ) >> 1);
    out[4] = (uint16)(((b->maxX - b->minX) * 5) >> 5);
    out[5] = (uint16)(((b->maxZ - b->minZ) * 5) >> 5);
}

void Shadow_publish(void)
{
    unsigned int n = 0, k, j, m = 0;
    uint8 pick[SHADOW_MAX - 1];
    unsigned int dist[SHADOW_MAX - 1];
    olShadow[0] = 0;
    /* ol_animFreeze holds the drawn poses where the host's boxes are not,
       for the renderers' byte-for-byte comparison: none then, as
       olLaraHitPublish stands down */
    if (!itemsCount || olInvState != INV_STATE_NONE || olLevel.fontSize == 0 || ol_animFreeze) return;
    olShadow[1] = fontAt(FONT_BANK)[0];
    olShadow[2] = olData16(fontAt(FONT_DARK), 0);
    if (ol_floorEff != OL_NO_FLOOR) {
        shadowOf(olShadow + 3, 0, ol_floorEff);
        n = 1;
    }

    /* the nearest creatures to her, by |dx| + |dz| (sixteen-bit differences:
       positions wrap), kept sorted */
    for (k = 0; k < olCreatureCount; k++) {
        unsigned int i = olCreatures[k], d;
        int dx, dz;
        ItemObj* it = items + i;
        if (ItemObj_status(it) == ITEM_STATUS_INVISIBLE || it->roomFloor == OL_NO_FLOOR) continue;
        dx = (int)(it->x - items[0].x); if (dx < 0) dx = -dx;
        dz = (int)(it->z - items[0].z); if (dz < 0) dz = -dz;
        d = (unsigned int)dx + (unsigned int)dz;
        if (m == SHADOW_MAX - 1 && d >= dist[m - 1]) continue;
        if (m < SHADOW_MAX - 1) m++;
        for (j = m - 1; j > 0 && dist[j - 1] > d; j--) { dist[j] = dist[j - 1]; pick[j] = pick[j - 1]; }
        dist[j] = d;
        pick[j] = (uint8)i;
    }
    for (k = 0; k < m; k++, n++) shadowOf(olShadow + 3 + n * 6, pick[k], items[pick[k]].roomFloor);
    olShadow[0] = (uint16)n;
}
#endif

/* olInvText's header, and no glyphs yet; 0 without the font */
static int textBegin(void)
{
    textOut = olInvText + 4;
    textCount = rectCount = 0;
    olInvText[0] = 0;
    olInvText[2] = 0;                   /* no remap: a black background */
    olInvRects[0] = 0;
    if (olLevel.fontSize == 0) return 0;
    fontCount = fontAt(FONT_COUNT)[0];
    olInvText[1] = fontAt(FONT_BANK)[0];
    olInvText[2] = olData16(fontAt(FONT_REMAP), 0);
    olInvText[3] = olData16(fontAt(FONT_DARK), 0);
    return 1;
}

/* drawFPS: upstream's int2str(fps) at (2, 16) in the font */
static void drawFps(unsigned int v)
{
    char buf[8], *p = buf + 7;
    *p = 0;
    do { *--p = (char)('0' + v % 10); v /= 10; } while (v);
    drawText(2, 16, p, ALIGN_LEFT);
}

/* drawFPS, for the game's frames (settings.c Settings_text), or nothing
   for a key of 0 */
void Inventory_fpsText(unsigned int key)
{
#ifndef OL_C_MOTION
    /* The game's counter shares the ring's output buffers. Reopening the
       same page must restore its text even when every ring input matches. */
    olInvTextCacheValid = 0;
#endif
    if (!textBegin()) return;
    if (key) drawFps(key & 0x7FFF);
    olInvText[0] = (uint16)textCount;
}

void Inventory_text(void)
{
    unsigned int slot;
    const char* title;

#ifndef OL_C_MOTION
    if (Inventory_textCached()) return;
#endif
    if (!textBegin()) return;
    slot = olInvItemsList[olInvItemIndex];

    /* drawEndPage: upstream's STR_ALPHA_END_1..6, a line every 16 from 48 */
    if (olInvPage == INV_PAGE_END) {
        drawText(0, 48, "Thanks for playing the", ALIGN_CENTER);
        drawText(0, 64, "OpenLara alpha demo!", ALIGN_CENTER);
        drawText(0, 96, "follow the project news:", ALIGN_CENTER);
        drawText(0, 112, "discord.gg/EF8JaQB", ALIGN_CENTER);
        drawText(0, 128, "t.me/openlara", ALIGN_CENTER);
        if (olSettings.fps) drawFps(OL_FPS);
        olInvText[0] = (uint16)textCount;
        return;
    }

    if (olInvState != INV_STATE_READY && olInvState != INV_STATE_SPIN &&
        olInvState != INV_STATE_SELECT && olInvState != INV_STATE_DESELECT &&
        olInvState != INV_STATE_SHOW) {
        if (olSettings.fps) drawFps(OL_FPS);
        olInvText[0] = (uint16)textCount;
        return;
    }

    /* drawSlot: the chosen item's count, in the small digits, while the
       ring is still */
    if (olInvState == INV_STATE_READY && olInvCounts[slot] > 1) {
        char buf[8], *p = buf + 7;
        unsigned int v = (unsigned int)olInvCounts[slot];
        *p = 0;
        do { *--p = (char)(1 + v % 10); v /= 10; } while (v);    /* '0'..'9' - 47 */
        drawText(FRAME_W / 2 + 32, FRAME_H - 32, p, ALIGN_LEFT);
    }

    if (olInvState != INV_STATE_SHOW && olInvState != INV_STATE_SELECT &&
        olInvState != INV_STATE_DESELECT)
    {
        /* getTitleStr (816-tcc will not take it as a ternary: ICE 4) */
        title = 0;
        if (olInvPage == INV_PAGE_MAIN) title = "INVENTORY";
        else if (olInvPage == INV_PAGE_OPTIONS) title = "OPTIONS";
        else if (olInvPage == INV_PAGE_KEYS || olInvPage == INV_PAGE_USE) title = "ITEMS";
        drawText(0, 20, title, ALIGN_CENTER);

        if (olInvPage == INV_PAGE_OPTIONS || (olInvPage == INV_PAGE_MAIN && olInvNumKeys)) {
            drawText(4, 4 + 16, "[", ALIGN_LEFT);
            drawText(-6, 4 + 16, "[", ALIGN_RIGHT);
        }
        if (olInvPage == INV_PAGE_MAIN || olInvPage == INV_PAGE_KEYS) {
            drawText(4, FRAME_H - 5, "]", ALIGN_LEFT);
            drawText(-6, FRAME_H - 5, "]", ALIGN_RIGHT);
        }
    }

    /* drawPage's drawOptions, once the item has come to rest */
    if (olInvFrameIndex == olInvFrameTarget) drawOptions();

    if (olInvFrameIndex == olInvFrameTarget && olInvState != INV_STATE_SPIN) {
        const char* str = itemName(slot);
        drawText(0, FRAME_H - 8, str, ALIGN_CENTER);
        /* the passport's page arrows, where its pages turn */
        if (olInvState == INV_STATE_SHOW && slot == SLOT_PASSPORT &&
            (olInvPage == INV_PAGE_OPTIONS || olInvPage == INV_PAGE_DEATH)) {
            int len = textWidth(str);
            if (olInvPassportPage != PASSPORT_EXIT)
                drawText((FRAME_W + len) / 2 + 4, FRAME_H - 8, "$\x6D", ALIGN_LEFT);
            if (olInvPassportPage != PASSPORT_LOAD_GAME)
                drawText((FRAME_W - len) / 2 - 18, FRAME_H - 8, "$\x6C", ALIGN_LEFT);
        }
    }

    if (olSettings.fps) drawFps(OL_FPS);     /* gameRender's drawFPS, over the ring too */
    olInvText[0] = (uint16)textCount;
    olInvRects[0] = (uint8)rectCount;
}

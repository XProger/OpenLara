/* OpenLara - SNES / Super FX target
 * The host loop: what this target has instead of src/fixed/game.h.
 *
 * Everything that is a port of upstream has moved out of here into a file
 * named for the one it came from -- item.c, room.c, nav.c, enemy.c, object.c,
 * level.c. What is left is the part that exists only on this machine: the
 * camera, the pad, the floor probes the GSU stages, and packing entity state
 * into the twelve bytes the renderer reads.
 *
 * Type model of this compiler, which the port has to be written around:
 *   char 1, short 2, int 2, long 2, long long 4, pointer 4 (far).
 * There is no 32-bit `int`. OpenLara's int32 maps to `long long` here, and
 * every 32-bit multiply becomes a jsr.l tcc__mull, so fixed-point maths wants
 * to stay on the GSU wherever it can.
 */

#include "fixed.h"
#include "level.h"
#include "item.h"
#include "room.h"
#include "nav.h"
#include "enemy.h"
#include "object.h"
#include "common.h"
#include "camera.h"
#ifdef OL_FX3
#include "inventory.h"
#include "save.h"
#include "fx.h"
#ifdef OL_FX3
#include "settings.h"
#endif
unsigned int olInvKeysPrev;
uint8 olFadeFields;                     /* the fade-in's fields left (main.asm NMI) */
#endif
#include "lara.h"
#include "collision.h"
#include "weapon.h"
#include "sound.h"

/* Eye height above the floor. TR puts Lara's at 762; this is a camera, and a
   little lower reads better in a corridor two metres high. */
#define EYE_HEIGHT      512

/* Collision. The world is a 1024-unit grid, so a wall is a neighbouring sector
   you are not allowed into. TR's Y grows downward, so a higher floor is a
   *smaller* number. */
#define SECTOR_SIZE     1024

/* Gravity is ItemObj::move's, with upstream's own constant, and how high she
   jumps comes out of the animation command that launches her. */


/* Uninitialised globals land in .bss, which is already WRAM. Nothing here may
   be assumed zero: there is no crt0, so a global that is not written before it
   is read is whatever the machine was last thinking. */
unsigned int ol_keys;
unsigned int ol_ticks;

/* The clock's gauges, written by the host assembly at every 60-field
   rollover (and on every dropped tick), read by the runner by name. `tps` is
   ticks completed in the last 60 fields -- 30 when the clock keeps up;
   `tickFields` is how many field boundaries those ticks straddled, which
   over `tps` is the fraction of a field a tick costs the renderer; and
   `ticksDropped` counts every tick the NMI's backlog cap threw away. */
unsigned int ol_tps;
unsigned int ol_tickFields;
unsigned int ol_ticksDropped;
unsigned int ol_angle;
int          ol_camX;
int          ol_camY;
int          ol_camZ;
/* The height the view looks at, camTarget.y: the GSU projects it and puts
   its centre line there, which is upstream's pitch (README "The pitch"). */
int          ol_camTgtY;
extern Location camTarget;      /* camera.c */

/* Current CPU-side room query. FX3 uses ROM tables and writable WRAM sectors;
   GSU-2 uses staged SRAM. Neither depends on a completed rendered frame. */
unsigned int ol_gsuVersion;
unsigned int ol_room;
unsigned int ol_camRoom;
int          ol_floor;
int          ol_ceiling;
int          ol_sectorX;
int          ol_sectorZ;
unsigned int ol_trigger;

/* The floor after the entities standing on it have had their say. */
int          ol_floorEff;

/* Exactly the twelve bytes an entity that GS_ITEM wants, in that order, so the
   host asm copies it without looking inside. See gsu/gsu.inc. */
unsigned char ol_itemState_gsu[OL_ITEM_MAX * 12];
unsigned char olItemDirty[OL_ITEM_MAX];

/* Where the host wants the floor looked at: Lara's feet, which stopped being
   the camera the moment the camera followed her. Six bytes, staged beside the
   entity state -- see GS_PROBE in gsu/gsu.inc. */
int ol_probe[3];

/* Hold every entity on one frame. The renderer's two implementations have to
   agree byte for byte and cannot while entity state advances on one side and
   stands still on the other. Not only a test hook -- the inventory screen will
   want exactly this. */
unsigned int ol_animFreeze;

/* One bit per case that came out wrong when the ported maths ran with the
   compiler's own type widths. tools/fixedcheck builds src/math.c for the
   desktop, where int32 is a real 32-bit int; on the cartridge it is
   `long long` and int is sixteen bits, and that difference is invisible to a
   differential test that compiles the same source. */
unsigned int ol_mathFail;


/* ---------------------------------------------------------------------------
 * Lara.
 *
 * Item 0 of every TR level, and the camera follows her -- which is what
 * makes "is Lara near this switch" a question with an answer. She is drawn
 * like any other entity; the opening view alone has her inside the near
 * plane, the entrance wall keeping the camera 264 behind her.
 *
 * lara.h is ported as far as lara.c and collision.c say; what is here is
 * the camera, the pad, and what the runner reads.
 * ------------------------------------------------------------------------ */

/* The camera is derived from her now, rather than being her. What the GSU is
   handed is two points: where to look from, and where to probe. See
   Camera_updateFollow, and GS_PROBE in gsu/gsu.inc. */
/* No initialisers: an initialised static lands in 816-tcc's orphan .data
   half at $04:0000, which mirrors the direct page -- these two aliased the
   host's fps counter. gameInit sets them. */
/* Not static: gameInit, in game_init.c, seeds them. */
int olCamSX, olCamSY, olCamSZ;
unsigned int olCamHint;

#if defined(OL_FX3) && !defined(OL_C_MOTION)
void Lara_toCamera(void);
void Lara_toCameraCold(void)
#else
static void Lara_toCamera(void)
#endif
{
    RoomQuery q;
    int sx, sy, sz;
    unsigned int hint;

    if (itemsCount == 0) return;

    /* upstream's toCombat, from updateWeapon while the guns are drawing or
       ready -- which returns before it for a dead Lara */
#ifdef OL_FX3
    if (olCamBusy)
        Camera_prepare();                /* with this tick's camera triggers */
    else
#endif
    if (olLaraLooking)                   /* s_checkLook; toCombat defers to it */
        camera.mode = CAM_MODE_LOOK;
    else if (items[0].health > 0 && (olLaraWeaponState == WEAPON_STATE_DRAW ||
                                     olLaraWeaponState == WEAPON_STATE_READY))
        camera.mode = CAM_MODE_COMBAT;
    Camera_updateFollow(items);

    ol_camX  = camera.x;
    ol_camY  = camera.y;
    ol_camZ  = camera.z;
    ol_camTgtY = camTarget.y;
    ol_angle = camera.angleY;

    /* Its room can only change when it leaves the sector it is in, or she
       leaves hers, and a settled follow camera does neither for seconds at
       a time. One query per sector rather than one per tick. */
    sx = camera.x >> 10;
    sy = camera.y >> 8;
    sz = camera.z >> 10;
#ifdef OL_FX3
    /* a fixed camera can be rooms away from her: its own room is the hint */
    hint = (olCamBusy && olCamFixedView) ? camera.room : items[0].room;
#else
    hint = items[0].room;
#endif
    if (sx == olCamSX && sy == olCamSY && sz == olCamSZ && hint == olCamHint)
        return;
    olCamSX = sx; olCamSY = sy; olCamSZ = sz;
    olCamHint = hint;

    if (Room_query(hint, camera.x, camera.y, camera.z, &q)) {
        camera.room = q.room;
        ol_camRoom = q.room;
    } else {
        ol_camRoom = hint;
    }
}

/* The HUD: upstream's drawHUD. The air bar while she is in the water, and
   under it -- or alone -- the health bar, while she holds her guns ready or
   for forty ticks after her health last changed (upstream sets healthTimer
   to 40 in hit() and when a medipack is used, to 100 at the level start,
   and stops counting while she is dead). Watching the value catches every
   damage site, the native ones included, without one line at each.

   Thirteen 8x8 sprites a bar, whose characters HUD.BIN's comment in
   packer/out_SNES.h lays out; the NMI copies olHudOam to OAM when olHudDirty
   says there is something new. Sprites 26-127 stay hidden from the boot. */
#ifdef OL_FX3
#define HUD_X       128         /* 20 + 216 - 108, as upstream's FRAME_WIDTH - 108 */
#define HUD_Y       44          /* 40 + 4: OAM Y n lines up with BG row n */
#define HUD_TILE    154         /* $79A0 words, in the second name table */
#define HUD_ATTR    0x31        /* priority 3, palette 0, name table 1 */
#else
#define HUD_X       148
#define HUD_Y       4
#define HUD_TILE    64          /* $6400 words, above the BG1 tile map */
#define HUD_ATTR    0x30
#endif
#define HUD_HIDDEN  0xF0
#ifdef OL_FX3
#define INV_BAR_X   76          /* 20 + (216 - 104) / 2: upstream's renderBar */
#define INV_BAR_Y   64          /* 40 + 24 */
#endif

unsigned char olHudOam[26 * 4];
unsigned char olHudDirty;
static int           olHudHealth;
static unsigned int  olHudTimer;
static unsigned int  olHudKey;     /* 0 at power-on: nothing shown, as OAM */

#if defined(OL_FX3) && !defined(OL_C_MOTION)
static int olHudWidthHealth;
static unsigned int olHudWidth;

/* Health usually stays unchanged for many ticks. Keep the exact two
   upstream floors, but only divide when the value changes. This key is
   separate from healthTimer's: medipacks change health while it is paused,
   and a level load seeds healthTimer before drawing the first bar. */
static unsigned int olHudHealthWidth(int h)
{
    if (h <= 0) return 1;
    if (h != olHudWidthHealth) {
        olHudWidthHealth = h;
        olHudWidth = ((unsigned int)h * 32u / 125u * 25u >> 6) + 1;
    }
    return olHudWidth;
}
#else
#define olHudHealthWidth(h) (((unsigned int)(h) * 32u / 125u * 25u >> 6) + 1)
#endif

static void olHudBar(unsigned char* p, unsigned int x, unsigned int y,
                     unsigned int w, unsigned char attr)
{
    unsigned int i, t;
    int n;
    for (i = 0; i < 13; i++) {
        if (i == 0) {
            t = (w > 7) ? 7 : w;
        } else {
            n = (int)w - (int)(i * 8 - 1);
            if (n < 0) n = 0;
            if (i < 12) {
                if (n > 8) n = 8;
                t = 8 + n;
            } else {
                t = 17 + n;             /* w <= 100, so n <= 5 */
            }
        }
        p[0] = (unsigned char)(x + i * 8);
        p[1] = (unsigned char)y;
        p[2] = (unsigned char)(HUD_TILE + t);
        p[3] = attr;
        p += 4;
    }
}

/* Rebuilt only when a bar appears, goes or changes width: the widths, plus
   one so that zero is "not shown", are the key. Every tick it was 3.8% of
   FX3's whole CPU profile, for sprites that change a few times a minute. */
static void olHudUpdate(void)
{
    unsigned char* p = olHudOam;
    unsigned int x = HUD_X, y = HUD_Y, i, air = 0, hp = 0, key;
    int h = items[0].health;

#ifdef OL_FX3
    /* The inventory draws no HUD, and her healthTimer stands still while
       it is open (Lara::update counts it). drawSlot's one bar instead: her
       health under the title, while a medi pack is chosen and the ring is
       still. */
    if (olInvState != INV_STATE_NONE) {
        unsigned int slot = olInvItemsList[olInvItemIndex];
        if (olInvState == INV_STATE_READY && h > 0 &&
            (slot == SLOT_MEDIKIT_SMALL || slot == SLOT_MEDIKIT_BIG))
            hp = olHudHealthWidth(h);
        key = 0x8000u | hp;
        if (key == olHudKey) return;
        olHudKey = key;
        for (i = 1; i < sizeof(olHudOam); i += 4) olHudOam[i] = HUD_HIDDEN;
        if (hp) olHudBar(p, INV_BAR_X, INV_BAR_Y, hp - 1, HUD_ATTR);
        olHudDirty = 1;
        return;
    }
#endif

    if (h != olHudHealth) {
        olHudHealth = h;
        olHudTimer = 40;
    } else if (h > 0 && olHudTimer) {
        olHudTimer--;
    }

    /* Upstream's value is v = x * 256 / max and the width v * 100 >> 8;
       the same floors, reduced so they stay in sixteen bits. */
    if (olLaraWaterState == WATER_STATE_SURFACE ||
        olLaraWaterState == WATER_STATE_UNDER)
        air = ((unsigned int)olLaraOxygen * 32u / 225u * 25u >> 6) + 1;  /* / 1800 */
    if (olHudTimer || olLaraWeaponState == WEAPON_STATE_READY)
        hp = (h > 0) ? olHudHealthWidth(h) : 1; /* / 1000 */

    key = (air << 8) | hp;
    if (key == olHudKey) return;
    olHudKey = key;

    for (i = 1; i < sizeof(olHudOam); i += 4) olHudOam[i] = HUD_HIDDEN;
    if (air) {
        olHudBar(p, x, y, air - 1, HUD_ATTR | 0x02);        /* palette 1 */
        p += 13 * 4;
        y += 10;
    }
    if (hp) olHudBar(p, x, y, hp - 1, HUD_ATTR);
    olHudDirty = 1;
}

/* The floor the GSU staged is the room's; an entity in the same sector can
   raise it, which is upstream's getItemFloorCeiling. */
static void olItemFloor(void)
{
    int eff = ol_floor;                 /* one read, then it is ours */

    if (itemsCount && eff != OL_NO_FLOOR) {
        Lara_itemFloorAt(items[0].x, items[0].y, items[0].z, &eff);
    }
    /* Published once, whole: the runner samples this at field boundaries. */
    ol_floorEff = eff;
}

/* What the renderer is told: twelve bytes an entity, in the order GS_ITEM
   wants them. The GSU indexes the model's own animation table, so the
   animation has to be local to the model rather than to the level; the
   position is the world's, and the renderer takes the room's origin off it. */
#if !defined(OL_FX3) || defined(OL_C_MOTION)
static void olItemToRenderer(unsigned int i, unsigned char anim,
                             unsigned char frame)
{
    unsigned char* p = ol_itemState_gsu + i * 12;
    p[0] = anim;
    p[1] = frame;
    p[2] = (unsigned char)items[i].x;
    p[3] = (unsigned char)(items[i].x >> 8);
    p[4] = (unsigned char)items[i].y;
    p[5] = (unsigned char)(items[i].y >> 8);
    p[6] = (unsigned char)items[i].z;
    p[7] = (unsigned char)(items[i].z >> 8);
    p[8] = (unsigned char)items[i].angleY;
    p[9] = (unsigned char)(items[i].angleY >> 8);
    p[10] = items[i].room;      /* DrawRoomItems draws it in this room */
}
#endif

#if defined(OL_FX3) && !defined(OL_C_MOTION)
void olUpdateItems(void);               /* render_sync.asm */
#else
static void olUpdateItems(void)
{
    if (ol_animFreeze) return;

    /* Upstream's updateItems: the active list, and only the active list.
       An entity that has taken itself off it -- a floor that has finished
       falling -- stops costing anything. `next` is read before the update
       because the update is allowed to remove the entity it is given. */
    {
        unsigned int a = olFirstActive;
        while (a != OL_NO_ACTIVE)
        {
            unsigned int next = items[a].nextActive;
#ifdef OL_FX3
            /* the effects pool is the list's tail: SpriteEffect/Bubble
               without ItemObj_class's walk through every class */
            if (a >= olFxFirst) Fx_update(items + a);
            else
#endif
            ItemObj_update(items + a, items);
            a = next;
        }
    }

}
#endif

/* Called by the host only between rendered frames. Lara, the active list
   and whatever ItemObj_touch marked since the last frame: an entity off the
   list has not moved, and its record is still what it was. This used to
   serialize every entity, about 130 K cycles a frame, which the clock could
   absorb at one frame a second and slipped under at two. */
#if !defined(OL_FX3) || defined(OL_C_MOTION)
void gameRenderSync(void)
{
    unsigned int i, end = itemsCount;

#ifdef OL_FX3
    /* render_sync.asm's: short of an idle effects pool (fx.c) */
    olSyncEnd = itemsCount;
    if (!itemsCount) return;
    if (!(olFxLive | olFxPending) && olFxCount && olFxFirst < itemsCount) olSyncEnd = olFxFirst;
    olFxPending = 0;
    end = olSyncEnd;
#endif
    for (i = 0; i < end; i++)
    {
        if (i != 0 && !ol_animFreeze && !olItemDirty[i]
            && !(items[i].flags & ITEM_FLAG_ACTIVE))
            continue;
        olItemDirty[i] = 0;

        /* Upstream's ITEM_FLAG_STATUS_INVISIBLE, which is what a pickup
           becomes once it has been taken. The renderer skips an entity whose
           animation is $FF -- see DrawRoomItems. */
        if (ItemObj_status(items + i) == ITEM_STATUS_INVISIBLE) {
            olItemToRenderer(i, OL_ITEM_HIDDEN, 0);
            continue;
        }

        if (ol_animFreeze) {
            olItemToRenderer(i, 0, (unsigned char)(ol_animFreeze - 1));
        } else {
            olItemToRenderer(i,
                (unsigned char)(items[i].animIndex - olModelFirstAnim(items[i].model)),
                (unsigned char)(items[i].frameIndex /
                                olAnimFrameRate(items[i].animIndex)));
        }
    }
}
#endif

/* Where she stands, for the camera, the probe and the runner: the room she
   is in, the floor and ceiling under and over her, her sector's origin and
   its trigger. Collision asks its own questions -- see collision.c. */
static int olQueryLara(int fresh)
{
    ItemObj* lara;

    if (itemsCount == 0) return 0;
    lara = items;

    /* Lara_update ended with this exact query; ask again only when it has
       not run this tick. */
    if (fresh) {
        olLaraQueryOk = Room_query(lara->room, lara->x, lara->y, lara->z,
                                   &olLaraQuery);
    }
    if (!olLaraQueryOk) {
        ol_floor = ol_ceiling = OL_NO_FLOOR;
        ol_trigger = 0xFFFF;
        return 0;
    }

    lara->room = (uint8)olLaraQuery.room;
    ol_room    = olLaraQuery.room;
    ol_floor   = olLaraQuery.floor;
    ol_ceiling = olLaraQuery.ceiling;
    ol_sectorX = olLaraQuery.sectorX;
    ol_sectorZ = olLaraQuery.sectorZ;
    ol_trigger = (olLaraQuery.trigger == 0xFF) ? 0xFFFF : olLaraQuery.trigger;
    return 1;
}

void gameUpdate(void)
{
    unsigned int keys;
#ifndef OL_FX3
    unsigned int i;
#endif
    int loaded = 0;
    /* Published as the last thing this tick does, on every exit, so that
       anyone sampling ol_ticks from outside -- the test runner -- sees a
       whole tick's results or none of them. */
    unsigned int tick = ol_ticks + 1;

    /* GSU-2 stages the entity list during its first render; FX3's CPU tables
       are already bound at boot. Both enter through the same ready check. */
    if (!itemsCount) {
        if (olLevelReady())
        {
            olLevelInit();
#ifndef OL_FX3
            for (i = 0; i < itemsCount; i++) {
                items[i].health = (int16)Enemy_health(items[i].type);
            }
            items[0].health = LARA_MAX_HEALTH;
#endif
            /* TR starts her swimming when the level starts her in water. */
            if (olRoomWater(items[0].room)) {
                olLaraWaterState = WATER_STATE_UNDER;
                ItemObj_animSet(items, olModelFirstAnim(items[0].model)
                                       + LARA_ANIM_UNDERWATER);
            }
#ifdef OL_FX3
            /* Load Game's Current Position: gameLoad after startLevel */
            if (olSaveLoad) {
                Save_apply();
                Fx_levelInit();         /* its effects in flight, counted */
            }
#endif
            Camera_snap(items);     /* rather than flying in from the origin */
#ifdef OL_FX3
            /* upstream's gBrightness = -128 at the load, 16 up a tick:
               28 off every channel at the first frame, 0 at the eighth
               (main.asm's NMI, COLDATA, counts the fields) */
            olFadeFields = 15;
#endif
            olHudHealth = items[0].health;
            olHudTimer  = 100;      /* Lara::init's healthTimer */
            olHudKey    = 0xFFFF;   /* rebuild on the first tick */
            loaded = 1;
#ifdef OL_FX3
            /* upstream's startLevel(LVL_TR1_TITLE): the title page, over
               this level held at its load -- the ring pauses the game, and
               draws no room -- until its passport boots a level */
            if (olBootKind == BOOT_TITLE)
                Inventory_open(INV_PAGE_TITLE, 0, ol_keys, ol_keys);
            /* level.h's palSet at the load: the settings' gamma, if any */
            Settings_level();
#endif
        }
    }

    /* On GSU-2 fixed ticks can begin before the first render has staged the
       level table. Neither variant has a Lara to update until its own table
       readiness check succeeds. */
    if (!itemsCount) {
        ol_ticks = tick;
        return;
    }

    /* The previous tick ended with a query at this exact position, so its
       floor and wall neighbourhood are the right inputs to the state machine.
       A fresh level is the one exception. */
    if (loaded) {
        olQueryLara(1);
        olItemFloor();
        /* And that is the whole of this tick. olLevelInit is already more
           than a tick's worth, and the camera's first clip on top of it --
           eight traces with every cache cold -- took the load tick past
           eight fields and dropped one. The snapped view is published as it
           is; the next tick is the first real one. A tick the NMI dropped
           behind the load is not game time dilating -- LEVEL2's load, with
           95 entities and 599 sector links, drops one every boot -- so the
           gauge starts counting from here. */
        ol_ticksDropped = 0;
        ol_camX    = camera.x;
        ol_camY    = camera.y;
        ol_camZ    = camera.z;
        ol_camTgtY = camTarget.y;
        ol_angle   = camera.angleY;
        ol_camRoom = camera.room;
        ol_ticks = tick;
        return;
    }

    keys = ol_keys;

#ifdef OL_FX3
    /* upstream's controls_swap: the two face buttons trade (settings.c) */
    if (olSettings.swap)
        keys = (keys & ~(IK_A | IK_B)) | ((keys & IK_A) ? IK_B : 0) | ((keys & IK_B) ? IK_A : 0);
#endif

#ifdef OL_FX3
    /* The inventory: while it is open the game does not move (upstream's
       gameUpdate skips the items); the tick it closes, what was chosen is
       used and the game goes on in the same tick. The state test is here,
       not only inside: the far call alone, every tick in play, costs the
       three-creature gate its tick. */
    if (olInvState != INV_STATE_NONE && Inventory_tick(items, keys)) {
        olInvKeysPrev = keys;
        olHudUpdate();
        ol_ticks = tick;
        return;
    }
#endif

    /* Look uses the previous tick's arm angles, before movement and weapon
       animation, as Lara::update does upstream. It also clears on death.
       Look held with nothing targeted is the head's, and eats the d-pad. */
    if (itemsCount && items[0].health > 0 && olWeaponTarget == OL_NO_ACTIVE &&
        (keys & IN_LOOK))
        keys = Lara_manualLook(keys);
    else
        Lara_updateLook();
    olLaraLooking = 0;

    /* Dead is dead: nothing she does moves her. TR plays an animation and
       fades to the menu, neither of which exists here. */
    if (itemsCount && items[0].health <= 0) {
#ifdef OL_FX3
        /* Lara::update: Select while she is dead opens the death page --
           the passport, its Load Game (this level again) */
        if ((keys & IK_SELECT) && !(olInvKeysPrev & IK_SELECT))
            Inventory_open(INV_PAGE_DEATH, 0, keys, olInvKeysPrev);
        olInvKeysPrev = keys;
#endif
        Weapon_update(items, 0);
        olUpdateItems();
        Lara_toCamera();
        olHudUpdate();
        ol_ticks = tick;
        return;
    }

    /* B is the action button. It is a held input to her state machine and
       to the things she can use -- see use.c -- as IN_ACTION. */

    /* She is item 0 and she is driven by `lara.h`, not by this file: the state
       machine picks a goal, the animation graph grants it, the animation's
       own speed moves her, and collision.c puts her where the world allows.
       What is left here is the camera and what the runner reads. */
#ifdef OL_FX3
    /* Lara::update opens it on a Select pressed while she is alive (dead,
       Select still restarts: main.asm's CheckRestart), and her tick goes on */
    if ((keys & IK_SELECT) && !(olInvKeysPrev & IK_SELECT) && items[0].health > 0)
        Inventory_open(INV_PAGE_MAIN, 0, keys, olInvKeysPrev);
    olInvKeysPrev = keys;
#endif
    Lara_update(items, keys);
    /* After Lara's water/interaction state has changed, before creatures
       act on damage. R draws/holsters; B/Action fires when ready. */
    Weapon_update(items, keys);
#ifdef OL_FX3
    olTrigLate = 1;
    checkTrigger(cinfo.trigger);
    olTrigLate = 0;
#endif

    olQueryLara(0);

    /* Creatures wake as TR wakes them: a trigger names them, or the level
       loaded them active. checkTrigger does both; nothing here does. */
    olUpdateItems();

    /* The floor after the entities standing on it have had their say, for
       the runner and the probe. Collision has already stood her on it. */
    olItemFloor();

    Lara_toCamera();

    ol_probe[0] = items[0].x;
    ol_probe[1] = items[0].y - EYE_HEIGHT;   /* her eye, which is what a
                                                ceiling is measured against */
    ol_probe[2] = items[0].z;

    olHudUpdate();
#ifdef OL_FX3
    /* TRIGGER_ACTION_END is upstream's nextLevel(gLevelID + 1), taken once
       the tick is over: inventory.c */
    if (olLevelEnd) Inventory_levelEnd(keys);
#endif
    ol_ticks = tick;
}

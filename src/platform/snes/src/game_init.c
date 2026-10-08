/* OpenLara - SNES / Super FX target
 * gameInit: the power-on state, once, before the first StartGSU.
 *
 * It runs while this CPU still owns the cartridge ROM, so on GSU-2 it is
 * kept in ROM bank $3C beside olMathCheck and olLevelInit rather than in
 * resident WRAM (Makefile C_BANK_game_init). Everything it sets belongs
 * to game.c and the units it names.
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
#include "settings.h"
#endif
#include "lara.h"
#include "collision.h"
#include "weapon.h"
#include "sound.h"

#define EYE_HEIGHT      512     /* game.c's */

/* game.c's globals, the ones the runner reads by name. */
extern unsigned int ol_keys, ol_ticks, ol_tps, ol_tickFields, ol_ticksDropped;
extern unsigned int ol_angle, ol_room, ol_camRoom, ol_trigger, ol_animFreeze;
extern int ol_camX, ol_camY, ol_camZ, ol_camTgtY, ol_floor, ol_ceiling, ol_floorEff;
extern int ol_probe[3];
extern unsigned char ol_itemState_gsu[OL_ITEM_MAX * 12];
extern int olCamSX, olCamSY, olCamSZ;
extern unsigned int olCamHint;

void gameInit(void)
{
    unsigned int i;

    /* WRAM comes up undefined and there is no crt0 to clear .bss. Seed the
       PRNG before olMathCheck asserts its sequence, and zero Lara's turn
       state so she does not spin off garbage for the first second on real
       hardware. */
    rand_reset();
    /* The sound state -- the command sequence, the draw seed, the counters
       -- starts at zero from the reset's WRAM clear; there is no Sound_init
       on the cartridge (sound.c keeps one for the C reference). */
    olLaraTurn      = 0;
    olLaraMoveAngle = 0;
    olLaraVSpeedHack = 0;
    olLaraWaterState = WATER_STATE_ABOVE;
    olLaraAngleX     = 0;
    olLaraSwimTimer  = 0;
    olLaraOxygen     = LARA_MAX_OXYGEN;
    olLaraWeaponState = WEAPON_STATE_FREE;
    Weapon_init();

    /* Animation $FF is the renderer's hidden marker: until the first
       gameRenderSync the staged entity state draws nothing rather than
       whatever the .bss happened to hold. */
    for (i = 0; i < OL_ITEM_MAX * 12; i++) ol_itemState_gsu[i] = 0xFF;
    /* ... but no pitch: the renderer turns every entity by byte 11, and
       only hers (ItemsExchange) is ever anything but zero */
    for (i = 11; i < OL_ITEM_MAX * 12; i += 12) ol_itemState_gsu[i] = 0;
    for (i = 0; i < OL_ITEM_MAX; i++) olItemDirty[i] = 1;

    olMathCheck();

    ol_keys     = 0;
    ol_ticks    = 0;
    ol_tps      = 0;
    ol_tickFields   = 0;
    ol_ticksDropped = 0;
    ol_angle    = ANGLE_180;
    ol_room     = 0;
    ol_camRoom  = 0;

    /* Room 0 of LEVEL1, the entrance corridor, facing back down it at the
       doorway frame -- which is a static mesh, so the oracles cover that path
       from the first frame. The floor here is 3072 and the eye rides 512 above
       it, so the floor-follow has nothing to do and the two oracles agree. */
    ol_camX = 9216;
    ol_camY = 3072 - EYE_HEIGHT;
    ol_camZ = 6500;
    ol_camTgtY = ol_camY;       /* level: no shear before the level */

    Camera_init();
    camera.x = ol_camX;
    camera.y = ol_camY;
    camera.z = ol_camZ;

    ol_probe[0] = ol_camX;
    ol_probe[1] = ol_camY;
    ol_probe[2] = ol_camZ;

    itemsCount    = 0;
    olFirstActive = OL_NO_ACTIVE;
    olPickups     = 0;
    olLevelEnd    = 0;
    olSecrets     = 0;
    for (i = 0; i < 11; i++) olInventory[i] = 0;
#ifdef OL_FX3
    Settings_load();            /* gSettings' defaults, then osLoadSettings */
    Inventory_init();
    Save_check();               /* osCheckSave, and whether this boot loads it */
#endif
#ifndef OL_FX3
    navFrom[0]    = NO_BOX;
    navFrom[1]    = NO_BOX;
#endif
    navResetCache();
    ol_animFreeze = 0;
    ol_floor       = OL_NO_FLOOR;
    ol_ceiling     = OL_NO_FLOOR;
    ol_trigger     = 0xFFFF;
    ol_floorEff   = OL_NO_FLOOR;
    olCamSX          = 0x7FFF;
    olCamHint        = 0xFFFF;
    Room_reset();
}

/* OpenLara - SNES / Super FX target: pistol targeting and firing cadence. */
#ifndef H_OL_WEAPON
#define H_OL_WEAPON

#include "item.h"
#include "room.h"

/* weaponParams[WEAPON_PISTOLS], src/fixed/lara.h, in 30 Hz game ticks. */
#define PISTOL_RANGE     8192
#define PISTOL_HEIGHT    650
#define PISTOL_AIM       (ANGLE_1 * 60)
#define PISTOL_ARM_X     (ANGLE_1 * 80)
#define PISTOL_ARM_MIN_Y (-ANGLE_1 * 60)
#define PISTOL_ARM_MAX_Y (ANGLE_1 * 170)
#define PISTOL_DAMAGE    1
#define PISTOL_RELOAD    9

#define WEAPON_AIM_RIGHT 1
#define WEAPON_AIM_LEFT  2
#define WEAPON_AIM_BOTH  3

#define PISTOL_ANIM_AIM  0
#define PISTOL_ANIM_PICK 1
#define PISTOL_ANIM_DRAW 2
#define PISTOL_ANIM_FIRE 3
#define PISTOL_ANIM_NONE 0xFFFF

/* Words deliberately match the channel copied to the GSU at frame handoff. */
typedef struct {
    uint16 anim, frame;
    int16 angleX, angleY;
    uint16 useBasis, drawn;
} WeaponArm;
extern WeaponArm olWeaponArms[2];
extern unsigned int olWeaponTarget;   /* held target, OL_NO_ACTIVE for none */
extern unsigned int olWeaponAim;      /* independently retained arm aim bits */
extern int olWeaponAimX, olWeaponAimY; /* target angles relative to Lara */
extern unsigned int olWeaponShots;    /* includes misses; wraps at 65536 */

void Weapon_init(void);
void Weapon_getAimPoint(ItemObj* target, Location* point);
void Weapon_updateTargets(ItemObj* lara);
void Weapon_animate(unsigned int input);
void Weapon_fire(unsigned int aiming);
#ifdef OL_FX3
extern unsigned int olWeaponFireArm;  /* the firing arm's offset in olWeaponArms */
extern uint16 olWeaponFlash[4];       /* per arm: the flash's end tick, its roll */

/* All four of upstream's guns (FX3): extraL->weapon and goalWeapon, and the
   meshes Lara::meshSwap* leave her wearing. The pistols, magnums and uzis
   share the pistols' four arm animations (weaponParams' animType); the
   shotgun's own three follow them in WeaponArm.anim, both arms on one. */
#define WEAPON_PISTOLS      0
#define WEAPON_MAGNUMS      1
#define WEAPON_UZIS         2
#define WEAPON_SHOTGUN      3
#define WEAPON_NONE         4
#define SHOTGUN_ANIM_AIM    4
#define SHOTGUN_ANIM_DRAW   5
#define SHOTGUN_ANIM_FIRE   6

typedef struct {
    uint16 weapon, goal;    /* extraL->weapon, goalWeapon */
    uint16 holster;         /* the pistol-class gun her thigh holsters show */
    uint16 armed;           /* meshSwapShotgun(true): the shotgun in her hands */
    uint16 head;            /* the uzis' head: Action held, ready */
} WeaponKit;
extern WeaponKit olWeaponKit;
/* What the GSU reads (GS_KIT, mesh.gsu WeaponNodeFx3/WeaponLookFx3): a mesh
   byte per node, a pad, the shotgun's torso X/Y/Z, the flash's distance down
   the hand. Rebuilt by Weapon_publish at the render sync, when dirty. */
extern uint8 olWeaponKitDraw[24];
extern unsigned int olWeaponKitDirty;
extern unsigned int olWeaponReload;   /* weaponParams' reloadTimer: weapon_anim.asm */
void Weapon_change(unsigned int weapon);
void Weapon_restore(void);
void Weapon_publish(void);
#endif
void Weapon_resetArms(void);
void Weapon_update(ItemObj* lara, unsigned int input);

#endif

/* OpenLara - SNES / Super FX target
 * All four of upstream's guns (FX3): src/fixed/lara.h's weaponParams for the
 * pistols, magnums, uzis and shotgun, weaponUpdateState's change of gun,
 * weaponFire with the shotgun's six pellets, the shotgun's own controller
 * (weaponDrawShotgun, weaponHolsterShotgun, weaponUpdateShotgun), and the
 * meshes meshSwapPistols/meshSwapShotgun and weaponUpdateTargets' head leave
 * her wearing, published for the GSU (Weapon_publish). weapon.c keeps the
 * targeting and the per-tick update; weapon_anim.asm the pistol-class arms.
 */

#include "weapon.h"
#include "weapon_kit.h"
#include "lara.h"
#include "room.h"
#include "olweapon.h"
#include "enemy.h"
#include "fx.h"
#include "sound.h"

/* weaponParams (src/fixed/lara.h), the four guns', in 30 Hz game ticks. */
static const WeaponParam weaponParams[4] = {
    { 1, ANGLE_1 * 8,  650, SND_PISTOLS_SHOT, 9,  3, 155,
      ANGLE_1 * 60, ANGLE_1 * 60, ANGLE_1 * 80, -ANGLE_1 * 60, ANGLE_1 * 170 },
    { 2, ANGLE_1 * 8,  650, SND_MAGNUMS_SHOT, 9,  3, 155,
      ANGLE_1 * 60, ANGLE_1 * 60, ANGLE_1 * 80, -ANGLE_1 * 60, ANGLE_1 * 170 },
    { 1, ANGLE_1 * 8,  650, SND_UZIS_SHOT,    3,  2, 180,
      ANGLE_1 * 60, ANGLE_1 * 60, ANGLE_1 * 80, -ANGLE_1 * 60, ANGLE_1 * 170 },
    { 4, ANGLE_1 * 20, 500, SND_SHOTGUN_SHOT, 26, 0, 0,
      ANGLE_1 * 55, ANGLE_1 * 60, ANGLE_1 * 65, -ANGLE_1 * 80, ANGLE_1 * 80 },
};
const WeaponParam* olWeaponParam;             /* the gun in her hands' */
WeaponKit olWeaponKit;
uint8 olWeaponKitDraw[24];
unsigned int olWeaponKitDirty;
unsigned int olWeaponReload;

#define W_HEIGHT    (olWeaponParam->height)

/* Whose sounds the APU's gun region holds: the level's image starts with
   the pistols' (packer sound_SNES.h); main.asm olSndGun swaps them. */
unsigned int olSndGunLoaded;
void olSndGun(unsigned int gun);

/* weaponParams[extraL->weapon], once a gun is in her hands, and its sounds */
void Weapon_params(void)
{
    unsigned int w = olWeaponKit.weapon;
    if (w > WEAPON_SHOTGUN) w = WEAPON_PISTOLS;
    if (w != olSndGunLoaded) {
        olSndGun(w);
        olSndGunLoaded = w;
    }
    olWeaponParam = weaponParams;
    while (w--) olWeaponParam++;
    olWeaponReload = (unsigned int)olWeaponParam->reload;
    olWeaponKitDirty = 1;
}

/* After Weapon_init or a load has set olWeaponKit: its parameters, its
   meshes for the GSU. */
void Weapon_restore(void)
{
    Weapon_params();
    Weapon_publish();
}

/* Lara::changeWeapon, from the ring: the goal, and choosing the gun already
   holstered draws it (weaponUpdateState sees weapon != goalWeapon). */
void Weapon_change(unsigned int weapon)
{
    olWeaponKit.goal = (uint16)weapon;
    if (olLaraWeaponState == WEAPON_STATE_FREE && olWeaponKit.goal == olWeaponKit.weapon)
        olWeaponKit.weapon = WEAPON_NONE;
}

/* GS_KIT for the GSU: upstream's meshSwap results as a mesh per node (the
   packer's writeWeaponKit numbers them), so the renderer needs no rules of
   its own. Her thighs wear the holsters of the last pistol-class gun put
   away (weaponHolsterPistols' swap) unless that arm's gun is out; her hands
   the gun of the moment, or the shotgun's; her torso the shotgun on her back
   unless it is in her hands; her head the uzis' while she fires. And while
   the shotgun's animation runs, drawLaraNodes' extra torso turn. */
static const int16 shotgunTorso[] = SHOTGUN_TORSO_FRAMES;

void Weapon_publish(void)
{
    uint8* d = olWeaponKitDraw;
    unsigned int holster = 5 + 4 * olWeaponKit.holster;
    unsigned int gun = 5 + 4 * (olWeaponKit.weapon < WEAPON_SHOTGUN ? olWeaponKit.weapon : 0);
    const WeaponArm* R = olWeaponArms;
    const WeaponArm* L = olWeaponArms + 1;
    int16* torso = (int16*)(d + 16);
    olWeaponKitDirty = 0;
    d[1] = (uint8)(L->drawn ? 1 : holster);         /* thigh L */
    d[4] = (uint8)(R->drawn ? 2 : holster + 1);     /* thigh R */
    d[7] = (uint8)(olWeaponKit.armed ? 17 : 18);    /* torso */
    if (olWeaponKit.armed) {
        d[10] = 19;
        d[13] = 20;
    } else {
        d[10] = (uint8)(R->drawn ? gun + 2 : 3);    /* hand R */
        d[13] = (uint8)(L->drawn ? gun + 3 : 4);    /* hand L */
    }
    d[14] = (uint8)(olWeaponKit.head ? 22 : 21);
    torso[0] = torso[1] = torso[2] = 0;
    if (olWeaponKit.weapon == WEAPON_SHOTGUN && R->anim >= SHOTGUN_ANIM_AIM &&
        R->anim <= SHOTGUN_ANIM_FIRE) {
        const int16* t = shotgunTorso;
        unsigned int frame = R->frame;
        if (R->anim == SHOTGUN_ANIM_DRAW) {
            t += 3 * SHOTGUN_DRAW_TORSO;
            if (frame > SHOTGUN_DRAW_LAST) frame = 0;
        } else if (R->anim == SHOTGUN_ANIM_FIRE) {
            t += 3 * SHOTGUN_FIRE_TORSO;
            if (frame > SHOTGUN_FIRE_LAST) frame = 0;
        } else if (frame > SHOTGUN_AIM_LAST) frame = 0;
        t += 3 * frame;
        torso[0] = t[0];
        torso[1] = t[1];
        torso[2] = t[2];
    }
    torso[3] = olWeaponParam->flashOffset;
}

/* weaponFire's shot, for the arm weapon_anim.asm (or the shotgun's
   controller below) is firing -- its byte offset in olWeaponArms: the
   spread -- two rand_logic draws a pellet, from the generator the creatures
   share -- and, at no living target, the line 16384 units along the arm's
   aim, traced to the wall it ricochets off. A pistol-class shot at a target
   in the arm's aim hits it, as before (the joint spheres are not tested);
   the shotgun's six pellets are each tested (Weapon_pelletHits). A hit
   bleeds at the aim point (Enemy::hit's fxBlood). */
unsigned int olWeaponFireArm;
/* Each arm's flash: weaponParams' flashTimer (3, the uzis' 2) from a shot,
   which upstream counts down at the top of every updateWeapon. Here the
   shot's tick plus that is kept instead -- the tick the flash is gone by --
   and ItemsExchange (main.asm) works the timer out once a frame, for the GSU
   (GS_FLASH, DrawMeshFlash), retiring a spent one: no work on the ticks
   between. With it, the roll rand_draw gives the flash, top byte. Per arm:
   end, roll. */
uint16 olWeaponFlash[4];
extern unsigned int ol_ticks;               /* game.c */

static void Weapon_miss(int aimX, int aimY)
{
    const WeaponArm* arm = (const WeaponArm*)((const char*)olWeaponArms + olWeaponFireArm);
    ItemObj* lara = items;
    Location from, to;
    int32 sx, cx, sy, cy;
    aimX += arm->angleX;
    aimY += arm->angleY + (int)lara->angleY;
    sx = phd_sin((int32)(uint16)aimX); cx = phd_cos((int32)(uint16)aimX);
    sy = phd_sin((int32)(uint16)aimY); cy = phd_cos((int32)(uint16)aimY);
    from.x = lara->x;
    from.y = lara->y - W_HEIGHT;
    from.z = lara->z;
    from.room = lara->room;
    to.x = from.x + (int)((cx * sy) >> FIXED_SHIFT);    /* matrixGetDir: e2x */
    to.y = from.y - (int)sx;
    to.z = from.z + (int)((cx * cy) >> FIXED_SHIFT);
    to.room = from.room;
    Room_trace(&from, &to);
    Fx_ricochet(to.room, to.x, to.y, to.z, 1);
}

/* A shotgun pellet at the target: upstream tests the pellet's line against
   the target's joint spheres, which this port does not have for creatures.
   The stand-in is the target's current-frame box: the pellet hits when its
   offset from the aim, carried out to the target's distance, passes within
   R, the average of half the box's height and half its narrower horizontal
   extent: d * off <= R * 10430, the offset in 16-bit angle units (10430 a
   radian). Lengths are octagonal (max + 3/8 min, within 7%), and the product
   is taken in units of 32 and 16 to stay in sixteen bits. */
static unsigned int Weapon_length(int a, int b)
{
    unsigned int x = (unsigned int)(a < 0 ? -a : a);
    unsigned int y = (unsigned int)(b < 0 ? -b : b);
    if (x < y) {
        unsigned int t = x;
        x = y;
        y = t;
    }
    return x + ((y * 3) >> 3);
}

static int Weapon_pelletHits(ItemObj* target, const Location* point, int aimX, int aimY)
{
    const AABBi* box = ItemObj_frameBox(target);
    ItemObj* lara = items;
    int32 dy = (int32)point->y - ((int32)lara->y - W_HEIGHT);
    int w = box->maxX - box->minX;
    int l = box->maxZ - box->minZ;
    unsigned int d;
    if (dy < -8192) dy = -8192;
    if (dy > 8192) dy = 8192;
    d = Weapon_length((int16)(uint16)(point->x - lara->x), (int16)(uint16)(point->z - lara->z));
    d = Weapon_length((int)d, (int)dy);
    if (l < w) w = l;
    return (d >> 5) * (Weapon_length(aimX, aimY) >> 4) <=
           (unsigned int)((w + (box->maxY - box->minY)) >> 2) * 20;
}

static void Weapon_shot(unsigned int aiming, int aimX, int aimY)
{
    ItemObj* target;
    Location point;
    int damage = olWeaponParam->damage;
    if (!aiming) goto miss;
    if (olWeaponTarget == OL_NO_ACTIVE) goto miss;
    target = items + olWeaponTarget;
    if (target->health <= 0) goto miss;
    Weapon_getAimPoint(target, &point);
    if (olWeaponKit.weapon == WEAPON_SHOTGUN && !Weapon_pelletHits(target, &point, aimX, aimY))
        goto miss;
    Fx_blood(target->room, point.x, point.y, point.z, 0, 0);
    if (target->type == 19) {
        Raptor_hit(target, damage);
        return;
    }
    target->health -= damage;
    if (target->health < 0) target->health = 0;
    target->flags2 |= ITEM_FLAG_INJURED;
    /* Enemy::hit: the flinch, while it still lives; the wolf and the bear
       have one, the bat and the raptor none. */
    if (target->health > 0) {
        if (target->type == 7) Sound_playItem(SND_HIT_WOLF, target);
        else if (target->type == 8) Sound_playItem(SND_HIT_BEAR, target);
        else if (target->type == 12 || target->type == 13) Sound_playItem(SND_HIT_LION, target);
    } else {
        target->timer = OL_CORPSE_TICKS;
    }
    return;
miss:
    Weapon_miss(aimX, aimY);
}

/* weaponFire: one shot, or the shotgun's six pellets; then the gun's sound
   and, for a pistol-class gun, weaponUpdatePistols' flash. Ammunition is
   upstream's -1 for every gun: endless. */
void Weapon_fire(unsigned int aiming)
{
    const WeaponParam* p = olWeaponParam;
    unsigned int count = olWeaponKit.weapon == WEAPON_SHOTGUN ? 6 : 1;
    olWeaponShots++;
    do {
        int aimX = (int)(((rand_logic() - 0x4000) * p->spread) >> 16);
        int aimY = (int)(((rand_logic() - 0x4000) * p->spread) >> 16);
        Weapon_shot(aiming, aimX, aimY);
    } while (--count);
    Sound_playItem(p->sound, items);
    if (p->flashTimer) {
        /* ol_ticks is this tick's until gameUpdate ends: + 1 is this one. */
        olWeaponFlash[olWeaponFireArm ? 2 : 0] = (uint16)(ol_ticks + 1 + p->flashTimer);
        olWeaponFlash[olWeaponFireArm ? 3 : 1] = (uint16)((rand_draw() >> 7) & 0xFF);
    }
}

/* weaponUpdateState: R draws and holsters the gun in her hands; a new goal
   (the ring's) holsters the old one, and from FREE becomes the gun drawn. */
void Weapon_updateState(unsigned int input)
{
    unsigned int change = 0;
    if (olLaraWaterState <= WATER_STATE_WADE) {
        if (olWeaponKit.weapon != olWeaponKit.goal) {
            if (olLaraWeaponState == WEAPON_STATE_FREE) {
                olWeaponKit.weapon = olWeaponKit.goal;
                Weapon_params();
                change = 1;
            } else if (olLaraWeaponState == WEAPON_STATE_READY) {
                change = 1;
            }
        } else if (input & IN_WEAPON) {
            change = 1;
        }
    } else if (olLaraWeaponState == WEAPON_STATE_READY) {
        change = 1;
    }
    if (!change) return;
    if (olLaraWeaponState == WEAPON_STATE_FREE) {
        /* setWeaponState(DRAW): ANIM_PISTOLS_PICK, or ANIM_SHOTGUN_DRAW */
        unsigned int anim = PISTOL_ANIM_PICK;
        if (olWeaponKit.weapon == WEAPON_SHOTGUN) anim = SHOTGUN_ANIM_DRAW;
        olLaraWeaponState = WEAPON_STATE_DRAW;
        olWeaponArms[0].anim = olWeaponArms[1].anim = anim;
        olWeaponArms[0].frame = olWeaponArms[1].frame = 0;
    } else if (olLaraWeaponState == WEAPON_STATE_READY) {
        olLaraWeaponState = WEAPON_STATE_HOLSTER;
    }
}

static int Weapon_lerpAngle(int angle, int target)
{
    int delta = (int16)(uint16)(target - angle);
    if (delta > ANGLE_1 * 10) delta = ANGLE_1 * 10;
    if (delta < -ANGLE_1 * 10) delta = -ANGLE_1 * 10;
    return angle + delta;
}

/* weaponDrawShotgun, weaponHolsterShotgun and weaponUpdateShotgun: one
   animation on both arms, the right arm's aim, the hands' swap at frame 10
   of the draw. Rare enough to stay C: no timing gate draws a shotgun. */
void Weapon_animateShotgun(unsigned int input)
{
    WeaponArm* R = olWeaponArms;
    WeaponArm* L = olWeaponArms + 1;
    unsigned int anim = R->anim, frame = R->frame;
    if (olLaraWeaponState == WEAPON_STATE_DRAW) {
        frame++;
        if (frame == 10) {
            olWeaponKit.armed = 1;
            Sound_playItem(SND_DRAW, items);
        }
        if (frame >= SHOTGUN_DRAW_LAST) {
            frame = SHOTGUN_DRAW_LAST;
            olLaraWeaponState = WEAPON_STATE_READY;
        }
        R->angleX = R->angleY = 0;
    } else if (olLaraWeaponState == WEAPON_STATE_HOLSTER) {
        if (anim == SHOTGUN_ANIM_AIM) {
            if (frame == 0) {
                anim = SHOTGUN_ANIM_DRAW;
                frame = SHOTGUN_DRAW_LAST;
            } else frame--;
        } else if (anim == SHOTGUN_ANIM_FIRE) {
            frame++;
            if (frame > 12) {
                anim = SHOTGUN_ANIM_DRAW;
                frame = SHOTGUN_DRAW_LAST;
            }
        } else if (anim == SHOTGUN_ANIM_DRAW) {
            if (frame == 0) {
                olLaraWeaponState = WEAPON_STATE_FREE;  /* setWeaponState(FREE) */
                Weapon_resetArms();
                olWeaponKitDirty = 1;
                return;
            }
            if (frame == 10) {
                olWeaponKit.armed = 0;
                Sound_playItem(SND_HOLSTER, items);
            }
            frame--;
        } else {
            olLaraWeaponState = WEAPON_STATE_FREE;
            Weapon_resetArms();
            olWeaponKitDirty = 1;
            return;
        }
        R->angleX = R->angleY = 0;
    } else if (olLaraWeaponState == WEAPON_STATE_READY) {
        unsigned int aiming = olWeaponAim & WEAPON_AIM_RIGHT;
        unsigned int aim = aiming;
        /* weaponAim(R) */
        R->angleX = (int16)Weapon_lerpAngle(R->angleX, aiming ? olWeaponAimX : 0);
        R->angleY = (int16)Weapon_lerpAngle(R->angleY, aiming ? olWeaponAimY : 0);
        if ((input & IN_ACTION) && olWeaponTarget == OL_NO_ACTIVE) aim = 1;
        if (anim == SHOTGUN_ANIM_FIRE) {
            frame++;
            if (frame == 10) {
                Sound_playItem(SND_SHOTGUN_RELOAD, items);
            } else if (frame == (unsigned int)olWeaponReload) {
                anim = SHOTGUN_ANIM_AIM;
                frame = SHOTGUN_AIM_LAST;
            } else if (SHOTGUN_FIRE_LAST - frame < 10 && !aim) {
                /* how many frames the fire had left, off the aim's end */
                frame = SHOTGUN_AIM_LAST - (SHOTGUN_FIRE_LAST - frame);
                anim = SHOTGUN_ANIM_AIM;
            }
        } else if (anim == SHOTGUN_ANIM_DRAW) {
            if (aim) {
                anim = SHOTGUN_ANIM_AIM;
                frame = 1;
            }
        } else if (anim == SHOTGUN_ANIM_AIM) {
            if (aim) {
                if (frame == SHOTGUN_AIM_LAST) {
                    if (input & IN_ACTION) {
                        olWeaponFireArm = 0;
                        Weapon_fire(aiming);
                        frame = 1;
                        anim = SHOTGUN_ANIM_FIRE;
                    }
                } else frame++;
            } else if (frame == 0) {
                anim = SHOTGUN_ANIM_DRAW;
                frame = SHOTGUN_DRAW_LAST;
            } else frame--;
        }
    } else {
        Weapon_resetArms();
        return;
    }
    R->anim = L->anim = anim;
    R->frame = L->frame = frame;
    R->useBasis = L->useBasis = 0;
    olWeaponKitDirty = 1;                       /* the torso follows its frames */
}

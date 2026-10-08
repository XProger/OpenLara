/* OpenLara - SNES / Super FX target
 * The next part of src/fixed/lara.h's weapons: range, acquisition angles,
 * room visibility, damage and reload timing, using the pistol parameters.
 *
 * Acquisition follows weaponFindTargets, then weaponTrackTargets holds that
 * target and updates both arm aim flags each tick. weapon_anim.c carries
 * the pistol animation controller. Spread and joint-sphere hit tests remain;
 * creature aim points now follow getBoundingBox(false)'s current frame.
 * On FX3 the parameters are the gun in her hands' and weapon_kit.c has the
 * rest of the four guns: the change of gun, the shot, the shotgun.
 */

#include "weapon.h"
#include "lara.h"
#include "room.h"
#include "olweapon.h"
#ifdef OL_FX3
#include "weapon_kit.h"
#endif
#include "sound.h"

unsigned int olWeaponTarget;
unsigned int olWeaponAim;
int olWeaponAimX, olWeaponAimY;
unsigned int olWeaponShots;

#ifdef OL_FX3
/* the gun in her hands' weaponParams: weapon_kit.c */
#define W_HEIGHT    (olWeaponParam->height)
#define W_AIM_X     (olWeaponParam->aimX)
#define W_AIM_Y     (olWeaponParam->aimY)
#define W_ARM_X     (olWeaponParam->armX)
#define W_ARM_MIN_Y (olWeaponParam->armMinY)
#define W_ARM_MAX_Y (olWeaponParam->armMaxY)
#else
#define W_HEIGHT    PISTOL_HEIGHT
#define W_AIM_X     PISTOL_AIM
#define W_AIM_Y     PISTOL_AIM
#define W_ARM_X     PISTOL_ARM_X
#define W_ARM_MIN_Y PISTOL_ARM_MIN_Y
#define W_ARM_MAX_Y PISTOL_ARM_MAX_Y
#endif

void Weapon_init(void)
{
    olWeaponTarget = OL_NO_ACTIVE;
    olWeaponAim = 0;
    olWeaponAimX = olWeaponAimY = 0;
    olWeaponShots = 0;
    olLaraWeaponState = WEAPON_STATE_FREE;
    Lara_resetLook();
    Weapon_resetArms();
    olWeaponArms[0].drawn = olWeaponArms[1].drawn = 0;
#ifdef OL_FX3
    /* Lara::init: the pistols, holstered, and -- "TODO check if shotgun on
       back" -- the shotgun on her back; InitGSU hands the GSU this record */
    olWeaponKit.weapon = olWeaponKit.goal = WEAPON_PISTOLS;
    olWeaponKit.holster = WEAPON_PISTOLS;
    olWeaponKit.armed = olWeaponKit.head = 0;
    olSndGunLoaded = WEAPON_PISTOLS;    /* the boot's APU image holds hers */
    Weapon_restore();
#endif
}


/* Lara::weaponGetAimPoint: upper third of the current box, with its horizontal
   centre rotated by the target's yaw. Creature animations carry precomputed
   local points for getFrame's nearest-pose choice, indexed by game tick.
   Other models retain the animation union. The cartridge uses weapon_aim.asm;
   keep the C expression as the reference. */
#ifdef OL_HOST_BUILD
void Weapon_getAimPoint(ItemObj* target, Location* point)
{
    int32 x, y, z, s, c;
    unsigned int base = olAnimAimBase;
    unsigned int record = 0;
    if (base) {
        if (target->animIndex >= olBlob16(base)) {
            x = y = z = 0;
            goto rotate;
        }
        record = olBlob16(base + 2 + (target->animIndex << 1));
    }
    if (record) {
        unsigned int tick = target->frameIndex;
        record += base;
        if (tick >= olBlob16(record)) tick = 0;
        record = base + olBlob16(record + 2 + (tick << 1));
        x = olBlobS16(record);
        y = olBlobS16(record + 2);
        z = olBlobS16(record + 4);
    } else {
        int16* box = (int16*)(olBlob + olAnimBoxBase + olMul12(target->animIndex));
        x = ((int32)box[0] + box[1]) >> 1;
        z = ((int32)box[4] + box[5]) >> 1;
        y = box[2] + ((int32)box[3] - box[2]) / 3;
    }
rotate:
    s = phd_sin((int32)target->angleY);
    c = phd_cos((int32)target->angleY);

    point->x = target->x + (int)((x * c + z * s) >> FIXED_SHIFT);
    point->z = target->z + (int)((z * c - x * s) >> FIXED_SHIFT);
    point->y = target->y + (int)y;
    point->room = target->room;
}
#endif

/* weaponUpdateTargets, combining the shared geometry in weaponFindTargets
   and weaponTrackTargets.
   A held target survives occlusion or lost aim; release permits a new one. */
void Weapon_updateTargets(ItemObj* lara)
{
    Location from, to;
    unsigned int i;
    unsigned int tracking = (olWeaponTarget != OL_NO_ACTIVE);
    unsigned int wasAiming = olWeaponAim;
    int sectorX, sectorZ;
    int sectorLimit = tracking ? 31 : 8;
    int minAimY = W_AIM_Y;

    olWeaponAim = 0;
    from.x = lara->x;
    from.y = lara->y - W_HEIGHT;
    from.z = lara->z;
    from.room = lara->room;
    if (!Room_pointOpen(&from)) return;
    sectorX = olSectorX(lara->room, lara->x);
    sectorZ = olSectorZ(lara->room, lara->z);

    /* Upstream ranks the active list by absolute yaw, not distance. Equal
       angles replace the previous candidate, preserving that list's order. */
    for (i = tracking ? olWeaponTarget : olFirstActive;
         i != OL_NO_ACTIVE; i = tracking ? OL_NO_ACTIVE : items[i].nextActive)
    {
        ItemObj* target = items + i;
        int32 dx, dy, dz, d;
        int off, aimX, aimY;
        unsigned int targetRoom;

        /* Keep these guards separate: 816-tcc miscompiled the mixed ||/!
           expression and let a zero-health ACTIVE target keep its lock. */
        if (target->health <= 0) goto invalidTarget;
        if (!(target->flags & ITEM_FLAG_ACTIVE)) goto invalidTarget;
        if (ItemObj_status(target) != ITEM_STATUS_ACTIVE) goto invalidTarget;
        if (target->room >= olRoomCount) goto invalidTarget;

        /* X/Z intentionally wrap at 65536 in this port. Absolute sectors
           disambiguate distant rooms before using their short delta, and
           keep a nearby target across the signed-coordinate seam in range.
           Tracking does not reapply acquisition range, as upstream. It does
           stop aiming beyond the short trace's representable sector delta. */
        off = olSectorX(target->room, target->x) - sectorX;
        if (off < -sectorLimit || off > sectorLimit) continue;
        off = olSectorZ(target->room, target->z) - sectorZ;
        if (off < -sectorLimit || off > sectorLimit) continue;
        if (!tracking) {
            dx = (int32)(int16)(uint16)(target->x - lara->x);
            dy = (int32)target->y - lara->y;
            dz = (int32)(int16)(uint16)(target->z - lara->z);
            if (dx < -PISTOL_RANGE || dx > PISTOL_RANGE) continue;
            if (dy < -PISTOL_RANGE || dy > PISTOL_RANGE) continue;
            if (dz < -PISTOL_RANGE || dz > PISTOL_RANGE) continue;
            d = dx * dx + dy * dy + dz * dz;
            if (d > (int32)PISTOL_RANGE * PISTOL_RANGE) continue;
        }

        Weapon_getAimPoint(target, &to);
        dy = (int32)to.y - from.y;
        if (dy < -32767 || dy > 32767) continue;
        anglesFromVector((int)(int16)(uint16)(to.x - from.x),
                         (int)(int16)(uint16)(to.y - from.y),
                         (int)(int16)(uint16)(to.z - from.z), &aimX, &aimY);
        aimX = (int)(int16)(uint16)(aimX - olLaraAngleX);
        aimY = (int)(int16)(uint16)(aimY - lara->angleY);
        if (tracking) {
            olWeaponAimX = aimX;
            olWeaponAimY = aimY;
        } else {
            /* Signed bounds also reject -32768, whose abs overflows. */
            off = (int)(int16)(uint16)(aimX - olLaraLook.torsoX);
            if (off < -W_AIM_X || off > W_AIM_X) continue;
            off = (int)(int16)(uint16)(aimY - olLaraLook.torsoY);
            if (off < -minAimY || off > minAimY) continue;
        }
        if (!Room_pointOpen(&to)) continue;
        targetRoom = to.room;
        if (!Room_trace(&from, &to)) continue;
        if (!Room_pointOpen(&to)) continue;
        /* Overlapping rooms without a connecting portal are not visible
           merely because their coordinates coincide. */
        if (to.room != targetRoom) continue;

        if (!tracking) {
            minAimY = (off < 0) ? -off : off;
            olWeaponTarget = i;
            olWeaponAimX = aimX;
            olWeaponAimY = aimY;
        }
        /* Acquisition ranks relative to the torso; tracking always aims
           relative to Lara's object basis. A newly selected target can be
           outside the body's acquisition cone, so apply retention to it
           too, using the previous tick's flags for each candidate. */
        olWeaponAim = 0;
        if (aimX >= -W_AIM_X && aimX <= W_AIM_X &&
            aimY >= -W_AIM_Y && aimY <= W_AIM_Y) {
            olWeaponAim = WEAPON_AIM_BOTH;
        } else {
            if (aimX < -W_ARM_X || aimX > W_ARM_X) continue;
            olWeaponAim = wasAiming;
            if (aimY < W_ARM_MIN_Y || aimY > W_ARM_MAX_Y)
                olWeaponAim &= ~WEAPON_AIM_RIGHT;
            if (aimY < -W_ARM_MAX_Y || aimY > -W_ARM_MIN_Y)
                olWeaponAim &= ~WEAPON_AIM_LEFT;
        }
        continue;
invalidTarget:
        if (tracking) olWeaponTarget = OL_NO_ACTIVE;
    }
}

#ifndef OL_FX3
void Weapon_fire(unsigned int aiming)
{
    olWeaponShots++;
    if (aiming && olWeaponTarget != OL_NO_ACTIVE) {
        ItemObj* target = items + olWeaponTarget;
        target->health -= PISTOL_DAMAGE;
        if (target->health < 0) target->health = 0;
        target->flags2 |= ITEM_FLAG_INJURED;
        /* Enemy::hit: the flinch, while it still lives; the wolf and the
           bear have one, the bat and the raptor none. */
        if (target->health > 0) {
            if (target->type == 7) Sound_playItem(SND_HIT_WOLF, target);
            else if (target->type == 8) Sound_playItem(SND_HIT_BEAR, target);
        }
    }
    /* weaponFire: the shot itself, after the hit */
    Sound_playItem(SND_PISTOLS_SHOT, items);
}
#endif

void Weapon_update(ItemObj* lara, unsigned int input)
{

    if (lara->health <= 0) {
        Weapon_resetArms();
        goto disabled;
    }
#ifdef OL_FX3
    Weapon_updateState(input);
#else
    if (PISTOL_AVAILABLE) {
        if (olLaraWaterState <= WATER_STATE_WADE) {
            if (input & IN_WEAPON) {
                if (olLaraWeaponState == WEAPON_STATE_FREE) {
                    olLaraWeaponState = WEAPON_STATE_DRAW;
                    olWeaponArms[0].anim = olWeaponArms[1].anim = PISTOL_ANIM_PICK;
                    olWeaponArms[0].frame = olWeaponArms[1].frame = 0;
                } else if (olLaraWeaponState == WEAPON_STATE_READY) {
                    olLaraWeaponState = WEAPON_STATE_HOLSTER;
                }
            }
        } else if (olLaraWeaponState == WEAPON_STATE_READY) {
            olLaraWeaponState = WEAPON_STATE_HOLSTER;
        }
    }
#endif
    if (olLaraWeaponState == WEAPON_STATE_READY) {
#ifdef OL_FX3
        /* weaponUpdateTargets: the uzis' gritted head while Action is held */
        unsigned int head = (input & IN_ACTION) ? 1 : 0;
        if (head != olWeaponKit.head) {
            olWeaponKit.head = (uint16)head;
            olWeaponKitDirty = 1;
        }
#endif
        /* Releasing Action allows acquisition again even while merely
           aiming, as weaponUpdateTargets does upstream. */
        if (!(input & IN_ACTION)) olWeaponTarget = OL_NO_ACTIVE;
        Weapon_updateTargets(lara);
    } else {
        olWeaponTarget = OL_NO_ACTIVE;
        olWeaponAim = 0;
#ifdef OL_FX3
        /* weaponHolster: meshSwap(ITEM_LARA, JOINT_MASK_HEAD) */
        if (olLaraWeaponState == WEAPON_STATE_HOLSTER && olWeaponKit.head) {
            olWeaponKit.head = 0;
            olWeaponKitDirty = 1;
        }
#endif
    }
#ifdef OL_FX3
    if (olWeaponKit.weapon == WEAPON_SHOTGUN) {
        Weapon_animateShotgun(input);
        return;
    }
#endif
    Weapon_animate(input);
    return;
disabled:
    olWeaponTarget = OL_NO_ACTIVE;
    olWeaponAim = 0;
}

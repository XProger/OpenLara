/* OpenLara - SNES / Super FX target: FX3's four guns (weapon_kit.c). */
#ifndef H_OL_WEAPON_KIT
#define H_OL_WEAPON_KIT

#include "weapon.h"

/* weaponParams (src/fixed/lara.h), the four guns', in 30 Hz game ticks. */
typedef struct {
    int16 damage, spread, height, sound, reload, flashTimer, flashOffset;
    int16 aimX, aimY, armX, armMinY, armMaxY;
} WeaponParam;
extern const WeaponParam* olWeaponParam;     /* the gun in her hands' */

extern unsigned int olSndGunLoaded;          /* the gun whose sounds the APU holds */

void Weapon_params(void);
void Weapon_updateState(unsigned int input);
void Weapon_animateShotgun(unsigned int input);

#endif

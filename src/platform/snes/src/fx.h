/* OpenLara - SNES / Super FX target
 * Upstream's effects (src/fixed/item.h fxBlood, fxRicochet, fxSmoke,
 * fxSplash, fxBubbles; object.h SpriteEffect and Bubble), FX3 only: src/fx.c.
 */
#ifndef H_OL_FX
#define H_OL_FX

#include "item.h"

/* The effects model's animations, in the packer's order (buildFxModel). */
#define FX_SPLASH       0
#define FX_BUBBLE       1
#define FX_BLOOD        2
#define FX_SMOKE        3
#define FX_RICOCHET     4

/* FX.BIN's joint sites (packer buildFxData): getJoint(joint, offset) as the
   creatures' bite and her breath ask it. */
#define FX_SITE_WOLF    0
#define FX_SITE_BEAR    1
#define FX_SITE_BAT     2
#define FX_SITE_RAPTOR  3
#define FX_SITE_HEAD    4

/* The level's pool, from FX.BIN, at the load tick (and again after a load). */
void Fx_levelInit(void);

/* ItemObj::add(type, room, pos, angleY) and the effect's constructor, from
   the pool: zero when it is empty or the level has no such sprite. */
ItemObj* Fx_add(unsigned int kind, unsigned int room, int x, int y, int z,
                unsigned int angleY);

/* SpriteEffect::update or Bubble::update, by type. */
void Fx_update(ItemObj* item);

/* getJoint(site's joint, site's offset) relative to the item's position,
   turned by its angles: zero when FX.BIN has no point for its frame. */
int Fx_joint(ItemObj* item, unsigned int site, int* x, int* y, int* z);

void Fx_blood(unsigned int room, int x, int y, int z, unsigned int angleY, int speed);
void Fx_ricochet(unsigned int room, int x, int y, int z, int sound);
void Fx_smoke(unsigned int room, int x, int y, int z);
void Fx_splash(ItemObj* item);
void Fx_bubbles(ItemObj* item);
/* Enemy::bite's blood, at the site's joint on its current frame. */
void Fx_bite(ItemObj* item, unsigned int site);

/* The pool, effects in flight, and one gone since the last frame. */
extern unsigned int olFxFirst, olFxCount, olFxLive, olFxPending;
extern unsigned int olSyncEnd;

#endif

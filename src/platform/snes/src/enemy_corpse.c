/* GBA Enemy::update under HIDE_CORPSES. The item timer survives save/load;
   fireTrigger preserves it after death instead of refreshing an activation
   timer. No per-enemy allocation is needed. Called only for a dead FX3 actor. */
#include "enemy.h"

int Enemy_corpseTick(ItemObj* item)
{
    /* Upstream admits an invisible enemy before spending its corpse timer.
       Keep that order when a dying creature has lost its Nav context. */
    if (ItemObj_status(item) == ITEM_STATUS_INVISIBLE) {
        if (!Enemy_navAcquire(item, 0)) return 1;
        ItemObj_setStatus(item, ITEM_STATUS_ACTIVE);
        ItemObj_touch(item);
    }

    /* Original post-decrement: 300 updates remain visible, the next hides
       the body. Deactivation alone would leave its last GSU pose visible. */
    if (item->timer == 0) {
        item->timer = -1;
        Enemy_navRelease(item);
        ItemObj_deactivate(item);
        ItemObj_setStatus(item, ITEM_STATUS_INVISIBLE);
        ItemObj_touch(item);
        return 1;
    }
    item->timer--;

    if (ItemObj_status(item) == ITEM_STATUS_INACTIVE) {
        /* The death animation has ended. Release navigation once, keep the
           frozen final pose, and remain on the update list until expiry. */
        if (item->health != OL_NOT_ENEMY) {
            item->health = OL_NOT_ENEMY;
            Enemy_navRelease(item);
        }
        return 1;
    }
    return 0;
}

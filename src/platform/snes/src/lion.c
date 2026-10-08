/* FX3: src/fixed/enemy.h's Lion inherits Enemy::updateState unchanged.
 * It keeps the animation's goal, never selects a bite or a death animation,
 * and never changes the zero maxTurn installed by setExtra. The original
 * model's commands still drive animation, movement, sounds and deactivation.
 * Species health, aggression and navigation limits belong to the shared
 * enemy table and navigation code; weapon hits supply Lion::hit's sound.
 */
#include "enemy.h"

void Lion_update(ItemObj* item, ItemObj* lara)
{
    unsigned int slot = (unsigned int)(item - items);
    int dx, dz;
    if (item->health > 0) Enemy_target(item, lara, &dx, &dz);
    olEnemyTouch[slot] = 0;
    Enemy_moveGround(item, 0);
}

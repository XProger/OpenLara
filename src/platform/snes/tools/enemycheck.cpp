/* Execute Bear's ORIGINAL enums/updateState, extracted unchanged from the
 * shared engine. Only platform fields and Lara::hit/bite effects are adapted.
 * stdin: state goal mood info distance next health target-health flags turn seed
 * stdout: eight little-endian words: goal next turn target-health flags seed
 * low/high and number of hit calls. No copied state-machine implementation. */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iostream>

using int32 = int32_t;
using uint32 = uint32_t;
struct vec3i { int32 x, y, z; };
static vec3i _vec3i(int32 x, int32 y, int32 z) { return {x, y, z}; }
static uint32 logicSeed;
static int32 rand_logic()
{
    logicSeed = logicSeed * 0x41C64E6Du + 0x3039;
    return (logicSeed >> 10) & 32767;
}
enum { MOOD_SLEEP, MOOD_STALK, MOOD_ATTACK, MOOD_ESCAPE };
// Adapter stores maxTurn in degrees, matching the port's byte representation.
enum { ENEMY_TURN_2 = 2, ENEMY_TURN_5 = 5 };
enum { ITEM_FLAG_INJURED = 8, ITEM_FLAG_REVERSE = 16 };
struct Target {
    int32 health;
    unsigned hits = 0;
    void hit(int32 damage, const vec3i&, int32)
    {
        hits++;
        if (health > 0 && damage > 0) health = std::max(0, health - damage);
    }
};
struct Bear {
    int32 state, goalState, mood, nextState, health, flags;
    uint32 hitMask;
    vec3i pos = {};
    struct Extra { int32 maxTurn; } *extraE;
    struct Info { Target* target; bool front, canAttack; int32 dist; } tinfo;
    void bite(int32 damage, const vec3i& offset, int32) { tinfo.target->hit(damage, offset, 0); }
#include "bearstate.inc"
};
static void word(uint32 v) { std::putchar(v & 255); std::putchar(v >> 8 & 255); }
int main()
{
    Bear b;
    Bear::Extra extra;
    Target target;
    unsigned info;
    while (std::cin >> b.state) {
        if (!(std::cin >> b.goalState >> b.mood >> info >> b.tinfo.dist >> b.nextState
              >> b.health >> target.health >> b.flags >> extra.maxTurn >> logicSeed)) return 1;
        b.extraE = &extra; b.tinfo.target = &target; target.hits = 0;
        b.tinfo.front = (info & 1) != 0; b.tinfo.canAttack = (info & 4) != 0;
        b.hitMask = info & 16 ? Bear::HIT_MASK : 1; // bit 0 is outside the original mask
        word(b.updateState()); word(b.nextState); word(extra.maxTurn);
        word(target.health); word(b.flags); word(logicSeed); word(logicSeed >> 16); word(target.hits);
    }
    return std::cin.eof() ? 0 : 2;
}

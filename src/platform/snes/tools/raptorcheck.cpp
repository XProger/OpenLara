/* Original Raptor enums/updateState, extracted verbatim from fixed/enemy.h.
 * Only platform fields and Lara hit effects are adapted. Input/output match
 * enemycheck.cpp; visual tilt is evaluated but is not yet rendered on SNES. */
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
enum { ENEMY_TURN_1 = 1, ENEMY_TURN_4 = 4 };
struct Target {
    int32 health;
    unsigned hits = 0;
    void hit(int32 damage, const vec3i&, int32)
    {
        hits++;
        if (health > 0 && damage > 0) health = std::max(0, health - damage);
    }
};
struct Raptor {
    int32 state, goalState, mood, nextState, health, flags;
    uint32 hitMask;
    struct Extra { int32 maxTurn; } *extraE;
    struct Info { Target* target; bool front, canAttack; int32 dist, turn, tilt; } tinfo;
    void bite(int32 damage, const vec3i& offset, int32) { tinfo.target->hit(damage, offset, 0); }
#include "raptorstate.inc"
};
static void word(uint32 v) { std::putchar(v & 255); std::putchar(v >> 8 & 255); }
int main()
{
    Raptor r;
    Raptor::Extra extra;
    Target target;
    unsigned info;
    while (std::cin >> r.state) {
        if (!(std::cin >> r.goalState >> r.mood >> info >> r.tinfo.dist >> r.nextState
              >> r.health >> target.health >> r.flags >> extra.maxTurn >> logicSeed)) return 1;
        r.extraE = &extra; r.tinfo.target = &target; target.hits = 0;
        r.tinfo.front = (info & 1) != 0; r.tinfo.canAttack = (info & 4) != 0;
        r.tinfo.turn = r.tinfo.tilt = 0;
        r.hitMask = info & 16 ? Raptor::HIT_MASK : 1;
        word(r.updateState()); word(r.nextState); word(extra.maxTurn);
        word(target.health); word(r.flags); word(logicSeed); word(logicSeed >> 16); word(target.hits);
    }
    return std::cin.eof() ? 0 : 2;
}

#ifndef H_ENEMY
#define H_ENEMY

#include "item.h"

struct Enemy : Item
{
    Enemy(Room* room, int32 hp) : Item(room)
    {
        flags.shadow = true;
        health = hp;
    }

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        //
        UNUSED(lara);
        UNUSED(cinfo);
    }

    virtual void update()
    {
        updateAnim();
    }
};


struct Wolf : Enemy
{
    Wolf(Room* room) : Enemy(room, 6) {}
};


struct Bear : Enemy
{
    Bear(Room* room) : Enemy(room, 20) {}
};


struct Bat : Enemy
{
    Bat(Room* room) : Enemy(room, 1) {}
};

/*
    case ITEM_LARA            : item->health = 1000; break;
    case ITEM_DOPPELGANGER    : item->health = 1000; break;
    case ITEM_WOLF            : item->health = 6;    break;
    case ITEM_BEAR            : item->health = 20;   break;
    case ITEM_BAT             : item->health = 1;    break;
    case ITEM_CROCODILE_LAND  : item->health = 20;   break;
    case ITEM_CROCODILE_WATER : item->health = 20;   break;
    case ITEM_LION_MALE       : item->health = 30;   break;
    case ITEM_LION_FEMALE     : item->health = 25;   break;
    case ITEM_PUMA            : item->health = 45;   break;
    case ITEM_GORILLA         : item->health = 22;   break;
    case ITEM_RAT_LAND        : item->health = 5;    break;
    case ITEM_RAT_WATER       : item->health = 5;    break;
    case ITEM_REX             : item->health = 100;  break;
    case ITEM_RAPTOR          : item->health = 20;   break;
    case ITEM_MUTANT_1        : item->health = 50;   break;
    case ITEM_MUTANT_2        : item->health = 50;   break;
    case ITEM_MUTANT_3        : item->health = 50;   break;
    case ITEM_CENTAUR         : item->health = 120;  break;
    case ITEM_MUMMY           : item->health = 18;   break;
    case ITEM_LARSON          : item->health = 50;   break;
    case ITEM_PIERRE          : item->health = 70;   break;
    case ITEM_SKATER          : item->health = 125;  break;
    case ITEM_COWBOY          : item->health = 150;  break;
    case ITEM_MR_T            : item->health = 200;  break;
    case ITEM_NATLA           : item->health = 400;  break;
    case ITEM_GIANT_MUTANT    : item->health = 500;  break;
*/

#endif

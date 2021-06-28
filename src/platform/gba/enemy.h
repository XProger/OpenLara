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
    }

    virtual void update()
    {
        animProcess();
    }
};


struct Doppelganger : Enemy
{
    Doppelganger(Room* room) : Enemy(room, LARA_MAX_HEALTH) {}
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


struct Crocodile : Enemy
{
    Crocodile(Room* room) : Enemy(room, 20) {}
};


struct Lion : Enemy
{
    Lion(Room* room) : Enemy(room, 1)
    {
        switch (type)
        {
            case ITEM_LION_MALE   : health = 30; break;
            case ITEM_LION_FEMALE : health = 25; break;
            case ITEM_PUMA        : health = 40; break;
        }
    }
};


struct Gorilla : Enemy
{
    Gorilla(Room* room) : Enemy(room, 22) {}
};


struct Rat : Enemy
{
    Rat(Room* room) : Enemy(room, 5) {}
};


struct Rex : Enemy
{
    Rex(Room* room) : Enemy(room, 100) {}
};


struct Raptor : Enemy
{
    Raptor(Room* room) : Enemy(room, 20) {}
};


struct Mutant : Enemy
{
    Mutant(Room* room) : Enemy(room, 50) {}
};


struct Centaur : Enemy
{
    Centaur(Room* room) : Enemy(room, 120) {}
};


struct Mummy : Enemy
{
    Mummy(Room* room) : Enemy(room, 18) {}
};


struct Larson : Enemy
{
    Larson(Room* room) : Enemy(room, 50) {}
};


struct Pierre : Enemy
{
    Pierre(Room* room) : Enemy(room, 70) {}
};


struct Skater : Enemy
{
    Skater(Room* room) : Enemy(room, 125) {}
};


struct Cowboy : Enemy
{
    Cowboy(Room* room) : Enemy(room, 150) {}
};


struct MrT : Enemy
{
    MrT(Room* room) : Enemy(room, 200) {}
};


struct Natla : Enemy
{
    Natla(Room* room) : Enemy(room, 400) {}
};


struct Adam : Enemy
{
    Adam(Room* room) : Enemy(room, 500) {}
};

#endif

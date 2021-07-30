#ifndef H_ENEMY
#define H_ENEMY

#include "item.h"

EWRAM_DATA ExtraInfoEnemy enemiesExtra[MAX_ENEMIES];

struct Enemy : Item
{
    Enemy(Room* room, int32 hp) : Item(room)
    {
        flags.shadow = true;

    #ifndef STATIC_ITEMS
        angle.y += (rand_logic() - 0x4000) >> 1;
    #endif

        health = hp;
    }

    void setExtra(ExtraInfoEnemy* extra)
    {
        ASSERT(!extraE);

        extraE = extra;

        if (!extraE)
            return;

        if (extra->enemy)
        {
            extra->enemy->flags.status = ITEM_FLAGS_STATUS_INVISIBLE;
            extra->enemy->disable();
        }

        extra->enemy = this;
        extra->rotHead = extra->rotNeck = 0;

        // TODO initialize enemy and navigation
    }

    bool enable(bool forced)
    {
        //return false;

        if (extraE)
            return true;

        for (int32 i = 0; i < MAX_ENEMIES; i++)
        {
            if (!enemiesExtra[i].enemy)
            {
                setExtra(enemiesExtra + i);
                return true;
            }
        }

        for (int32 i = 0; i < MAX_PLAYERS; i++)
        {
            if (!players[i])
                continue;

            vec3i &viewPos = players[i]->extraL->camera->view.pos;

            int32 index = -1;
            int32 maxDistQ = 0;

            if (!forced)
            {
                vec3i d = pos - viewPos;
                maxDistQ = X_SQR(d.x >> 8) + X_SQR(d.y >> 8) + X_SQR(d.z >> 8);
            }

            for (int32 j = 0; j < MAX_ENEMIES; j++)
            {
                vec3i d = enemiesExtra[j].enemy->pos - viewPos;
                int32 distQ = X_SQR(d.x >> 8) + X_SQR(d.y >> 8) + X_SQR(d.z >> 8);
                if (distQ > maxDistQ)
                {
                    maxDistQ = distQ;
                    index = j;
                }
            }

            if (index != -1)
            {
                setExtra(enemiesExtra + index);
                return true;
            }
        }

        return false;
    }

    void disable()
    {
        ASSERT(extraE);
        extraE->enemy = NULL;
        extraE = NULL;
    }

    virtual void activate()
    {
        Item::activate();

        if (!enable(flags.status == ITEM_FLAGS_STATUS_NONE)) {
            flags.status = ITEM_FLAGS_STATUS_INVISIBLE;
        }
    }

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        if (health > 0) {
            health -= damage;

            if (health <= 0) {
                gSaveGame.kills++;
            }
        }

        if (health > 0)
        {
            if (type != ITEM_MUMMY) {
                fxBlood(point, 0, 0);
            }

            if (soundId) {
                soundPlay(soundId, pos);
            }
        }
    }

    virtual void collide(Lara* lara, CollisionInfo* cinfo)
    {
        if (health <= 0) // TODO T-Rex still collide after death
            return;

        if (!extraE) // disabled
            return;

        if (!updateHitMask(lara, cinfo))
            return;

        if (!cinfo->enemyPush)
            return;
        
        int32 r = cinfo->radius;
        cinfo->radius = 0;
        collidePush(lara, cinfo, true);
        cinfo->radius = r;
    }

    virtual void update()
    {
        if (flags.status == ITEM_FLAGS_STATUS_INVISIBLE)
        {
            if (!enable(false))
                return;
            flags.status = ITEM_FLAGS_STATUS_ACTIVE;
        }

        if (extraE) {
            logic();
        }

        animProcess();
    }

    virtual void logic() {}
};


struct Doppelganger : Enemy
{
    Doppelganger(Room* room) : Enemy(room, LARA_MAX_HEALTH) {}
};


struct Wolf : Enemy
{
    Wolf(Room* room) : Enemy(room, 6) {}

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_WOLF);
    }
};


struct Bear : Enemy
{
    Bear(Room* room) : Enemy(room, 20) {}

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_BEAR);
    }
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

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, (type == ITEM_PUMA) ? 0 : SND_HIT_LION);
    }
};


struct Gorilla : Enemy
{
    Gorilla(Room* room) : Enemy(room, 22) {}
};


struct Rat : Enemy
{
    Rat(Room* room) : Enemy(room, 5) {}

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_RAT);
    }
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

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_SKATER);
    }
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

    virtual void hit(int32 damage, const vec3i &point, int32 soundId)
    {
        Enemy::hit(damage, point, SND_HIT_ADAM);
    }
};

#endif

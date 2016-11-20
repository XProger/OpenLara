#ifndef H_SPRITE
#define H_SPRITE

#include "controller.h"

struct Sprite : Controller {

    enum {
        FRAME_ANIMATED = -1,
        FRAME_RANDOM   = -2,
    };

    bool  instant;
    int   frame, flag;
    float time;

    Sprite(TR::Level *level, int entity, bool instant = true, int frame = FRAME_ANIMATED) : Controller(level, entity), instant(instant), flag(frame), time(0.0f) {
        if (frame >= 0) { // specific frame
            this->frame = frame;
        } else if (frame == FRAME_RANDOM) { // random frame
            this->frame = rand() % getSequence().sCount;
        } else if (frame == FRAME_ANIMATED) { // animated
            this->frame = 0;
        }
    }

    static void add(TR::Level *level, TR::Entity::Type type, int room, int x, int y, int z, int frame = -1) {
        int index = level->entityAdd(type, room, x, y, z, 0, -1);
        if (index > -1) {
            level->entities[index].intensity  = 0x1FFF - level->rooms[room].ambient;
            level->entities[index].controller = new Sprite(level, index, true, frame);
        }
    }

    TR::SpriteSequence& getSequence() {
        return level->spriteSequences[-(getEntity().modelIndex + 1)];
    }

    void update() {
        if (flag >= 0) return;

        bool remove = false;
        time += Core::deltaTime;

        if (flag == FRAME_ANIMATED) {
            frame = int(time * SPRITE_FPS);
            TR::SpriteSequence &seq = getSequence();
            if (instant && frame >= seq.sCount)
                remove = true;
            else
                frame %= seq.sCount;
        } else
            if (instant && time >= (1.0f / SPRITE_FPS))
                remove = true;

        if (remove) {
            level->entityRemove(entity);
            delete this;
        }
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh) {
        mat4 m(Core::mModel);
        m.translate(pos);
        Core::active.shader->setParam(uModel, m);
        mesh->renderSprite(-(getEntity().modelIndex + 1), frame);
    }
};

#endif
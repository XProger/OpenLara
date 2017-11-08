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
    vec3  velocity;

    BlendMode blendMode;

    Sprite(IGame *game, int entity, bool instant = true, int frame = FRAME_ANIMATED) : Controller(game, entity), instant(instant), flag(frame), time(0), velocity(0), blendMode(bmAlpha) {
        if (frame >= 0) { // specific frame
            this->frame = frame;
        } else if (frame == FRAME_RANDOM) { // random frame
            this->frame = rand() % getSequence().sCount;
        } else if (frame == FRAME_ANIMATED) { // animated
            this->frame = 0;
        }
    }

    TR::SpriteSequence& getSequence() {
        return level->spriteSequences[-(getEntity().modelIndex + 1)];
    }

    virtual void update() {
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

        pos += velocity * (30.0f * Core::deltaTime);

        if (remove)
            game->removeEntity(this);
    }

    virtual void render(Frustum *frustum, MeshBuilder *mesh, Shader::Type type, bool caustics) {
        Core::setBlending(blendMode);
        Core::active.shader->setParam(uBasis, Basis(Core::mViewInv.getRot(), pos));
        mesh->renderSprite(-(getEntity().modelIndex + 1), frame);
        Core::setBlending(bmNone);
    }
};

#endif
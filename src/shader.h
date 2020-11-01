#ifndef H_SHADER
#define H_SHADER

#include "core.h"

struct Shader : GAPI::Shader {

    enum Type { 
        DEFAULT = 0,
        SPRITE = 0, FLASH, ROOM, ENTITY, MIRROR,
        FILTER_UPSCALE = 0, FILTER_DOWNSAMPLE, FILTER_DOWNSAMPLE_DEPTH, FILTER_GRAYSCALE, FILTER_BLUR, FILTER_ANAGLYPH, FILTER_EQUIRECTANGULAR,
        WATER_DROP = 0, WATER_SIMULATE, WATER_CAUSTICS, WATER_RAYS, WATER_MASK, WATER_COMPOSE,
        SKY_TEXTURE = 0, SKY_CLOUDS, SKY_AZURE,
        MAX = 6
    };

    Shader(Core::Pass pass, Type type, int *def, int defCount) : GAPI::Shader() {
        init(pass, type, def, defCount);
    }

    virtual ~Shader() {
        deinit();
    }

    void setup() {
        bind();
        setParam(uViewProj,  Core::mViewProj);
        setParam(uLightProj, Core::mLightProj);
        setParam(uViewPos,   Core::viewPos);
        setParam(uParam,     Core::params);
    }
};

#endif

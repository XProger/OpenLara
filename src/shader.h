#ifndef H_SHADER
#define H_SHADER

#include "core.h"

struct Shader : GAPI::Shader {

    enum Type { 
        DEFAULT = 0,
        /* shader */ SPRITE = 0, FLASH = 1, ROOM = 2, ENTITY = 3, MIRROR = 4, 
        /* filter */ FILTER_DOWNSAMPLE = 1, FILTER_GRAYSCALE = 2, FILTER_BLUR = 3, FILTER_EQUIRECTANGULAR = 4,
        /* water  */ WATER_DROP = 0, WATER_STEP = 1, WATER_CAUSTICS = 2, WATER_MASK = 3, WATER_COMPOSE = 4,
        MAX = 6
    };

    Shader(Core::Pass pass, int *def, int defCount) : GAPI::Shader() {
        init(pass, def, defCount);
    }

    virtual ~Shader() {
        deinit();
    }

    void setup() {
        bind();
        setParam(uViewProj,  Core::mViewProj);
        setParam(uLightProj, Core::mLightProj[0], Core::settings.detail.shadows > Core::Settings::Quality::MEDIUM ? SHADOW_OBJ_MAX : 1);
        setParam(uViewPos,   Core::viewPos);
        setParam(uParam,     Core::params);
        setParam(uFogParams, Core::fogParams);
    }
};

#endif

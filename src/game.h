#ifndef H_GAME_TR
#define H_GAME_TR

#include "core.h"
#include "format.h"
#include "level.h"

namespace Game {
	Level		*level;

	void init() {
		Core::init();
		level = new Level("data\\LEVEL2_DEMO.PHD");
	}

	void free() {
		delete level;

		Core::free();
	}

	void update() {
		level->update();
	}

	void render() {
		Core::clear(vec4(0.0f, 0.0f, 0.0f, 0.0));
		Core::setViewport(0, 0, Core::width, Core::height);
		Core::setBlending(bmAlpha);

		level->render();
	}
}

#endif
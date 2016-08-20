#ifndef H_GAME_TR
#define H_GAME_TR

#include "core.h"
#include "format.h"
#include "level.h"

struct Camera {
	float fov, znear, zfar;
	vec3 pos, angle;

	void update() {
		vec3 dir = vec3(sinf(angle.y - PI) * cosf(-angle.x), -sinf(-angle.x), cosf(angle.y - PI) * cosf(-angle.x));
		vec3 v = vec3(0);

		if (Input::down[ikW]) v = v + dir;
		if (Input::down[ikS]) v = v - dir;
		if (Input::down[ikD]) v = v + dir.cross(vec3(0, 1, 0));
		if (Input::down[ikA]) v = v - dir.cross(vec3(0, 1, 0));

		pos = pos + v.normal() * (Core::deltaTime * 2048.0f);

		if (Input::down[ikMouseL]) {
			vec2 delta = Input::mouse.pos - Input::mouse.start.L;
			angle.x -= delta.y * 0.01f;
			angle.y -= delta.x * 0.01f;
			angle.x = min(max(angle.x, -PI * 0.5f + EPS), PI * 0.5f - EPS);
			Input::mouse.start.L = Input::mouse.pos;
		}

	}

	void setup() {
		Core::mView.identity();
		Core::mView.rotateZ(-angle.z);
		Core::mView.rotateX(-angle.x);
		Core::mView.rotateY(-angle.y);
		Core::mView.translate(vec3(-pos.x, -pos.y, -pos.z));

		Core::mProj = mat4(fov, (float)Core::width / (float)Core::height, znear, zfar);
	
		Core::mViewProj = Core::mProj * Core::mView;
	}
};

namespace Game {
	Level		*level;
	Camera		camera;

	void init() {
		Core::init();

	//	Stream stream("GYM.PHD");
		level = new Level("data\\LEVEL3A.PHD");

		camera.fov		= 90.0f;
		camera.znear	= 0.1f * 2048.0f;
		camera.zfar		= 1000.0f * 2048.0f;
//		camera.pos		= vec3(-10, -2, 26);
		camera.pos		= vec3(-13.25f, 0.42f, 38.06f) * 2048.0f;
//		camera.pos		= vec3(-36, -1, 2);
		camera.angle	= vec3(0);
	}

	void free() {
		delete level;

		Core::free();
	}

	void update() {
		camera.update();

		level->update();
	}

	void render() {
		Core::clear(vec4(0.0f, 0.0f, 0.0f, 0.0));
		Core::setViewport(0, 0, Core::width, Core::height);
		Core::setBlending(bmAlpha);

		camera.setup();		
		level->render();
	}
	
	/*
	void input(InputKey key, InputState state) {
		static vec2 mLast;
		if (state == isDown && key == ikMouseL) {
			mLast = Input::mouse.pos;
			return;
		}

		if (state == isMove && key == ikMouse && (Input::mouse.L.down || Input::mouse.R.down)) {
			vec2 delta = Input::mouse.pos - mLast;
			camera.angle.x -= delta.y * 0.01f;
			camera.angle.y -= delta.x * 0.01f;
			camera.angle.x = _min(_max(camera.angle.x, -PI * 0.5f + EPS), PI * 0.5f - EPS);
			mLast = Input::mouse.pos;
		}
	}
	*/
}

#endif
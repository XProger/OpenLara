#ifndef H_CONTROLLER
#define H_CONTROLLER

#include "format.h"

struct Controller {
	TR::Level *level;
	TR::Animation *anim;
	float fTime;

	vec3	pos;
	float	angle;

	int state;	// LaraState
	int lastFrame;

	Controller(TR::Level *level) : level(level), pos(0.0f), angle(0.0f), fTime(0.0f) {
		anim = &level->anims[0];
		lastFrame = 0;
	}

	void update() {
		float rot = 0.0f;
		state = TR::STATE_STOP;
		if (Input::down[ikShift]) {
			if (Input::down[ikUp])		{ state = TR::STATE_WALK; };
			if (Input::down[ikDown])	{ state = TR::STATE_BACK; };
			if (Input::down[ikLeft])	{ if (!Input::down[ikUp] && !Input::down[ikDown]) { state = TR::STATE_STEP_LEFT; }	else rot = -Core::deltaTime * PI; };
			if (Input::down[ikRight])	{ if (!Input::down[ikUp] && !Input::down[ikDown]) { state = TR::STATE_STEP_RIGHT; }	else rot =  Core::deltaTime * PI; };
		} else
			if (Input::down[ikSpace]) {
				if (anim->state == TR::STATE_RUN)
					state = TR::STATE_FORWARD_JUMP;
				else
					if (Input::down[ikUp]) 
						state = anim->state != TR::STATE_COMPRESS ? TR::STATE_COMPRESS : TR::STATE_FORWARD_JUMP;
					else
						if (Input::down[ikDown]) 
							state = anim->state != TR::STATE_COMPRESS ? TR::STATE_COMPRESS : TR::STATE_BACK_JUMP;
						else 
							if (Input::down[ikLeft])
								state = anim->state != TR::STATE_COMPRESS ? TR::STATE_COMPRESS : TR::STATE_LEFT_JUMP;
							else
								if (Input::down[ikRight])
									state = anim->state != TR::STATE_COMPRESS ? TR::STATE_COMPRESS : TR::STATE_RIGHT_JUMP;
								else
									state = TR::STATE_UP_JUMP;
			} else {
				if (Input::down[ikUp])		{ state = TR::STATE_RUN; };
				if (Input::down[ikDown])	{ state = TR::STATE_FAST_BACK; };
				if (Input::down[ikLeft])	{ if (!Input::down[ikUp] && !Input::down[ikDown]) state = TR::STATE_TURN_LEFT;	rot = -Core::deltaTime * PI; };
				if (Input::down[ikRight])	{ if (!Input::down[ikUp] && !Input::down[ikDown]) state = TR::STATE_TURN_RIGHT;	rot =  Core::deltaTime * PI; };				
			}

		if (Input::down[ikEnter])
			state = TR::STATE_COMPRESS;


		fTime += Core::deltaTime;
		int fCount = anim->frameEnd - anim->frameStart + 1;
		int fIndex = int(fTime * 30.0f);

//		LOG("%d / %d\n", fIndex, fCount);

//		fIndex = anim->frameStart + (fIndex % fCount);
		//LOG("%d\n", fIndex);

		/*
		if (anim->state == state) {
			for (int i = 0; i < anim->scCount; i++) {
				auto &sc = level->stateChanges[anim->scOffset + i];
				LOG("%d ", sc.state);
			}
			LOG("\n");
		}
		*/
		if (anim->state != state) {
			for (int i = 0; i < anim->scCount; i++) {
				auto &sc = level->states[anim->scOffset + i];
				if (sc.state == state) {
					for (int j = 0; j < sc.rCount; j++) {
						auto &range = level->ranges[sc.rangeOffset + j];
						if ( anim->frameStart + fIndex >= range.low && anim->frameStart + fIndex <= range.high) {
							int st = anim->state;
							anim  = &level->anims[range.nextAnimation];
							fTime = 0.0f;//(ad.nextFrame - anim->frameStart) / (30.0f / anim->frameRate);
							fIndex = range.nextFrame - anim->frameStart;
							fCount = anim->frameEnd - anim->frameStart + 1;
					//		LOG("set anim %d %f %f %d -> %d -> %d\n", range.nextAnimation, anim->accel.toFloat(), anim->speed.toFloat(), st, state, anim->state);
								
							//LOG("from %f to %f\n", s, s + a * c);
				//			LOG("frame: %d\n", fIndex);
							break;
						}
					}
					break;
				}
			}
		};

		if (fIndex >= fCount) {
			fIndex = anim->nextFrame;
			int id = anim->nextAnimation;
			anim = &level->anims[anim->nextAnimation];
//				LOG("nxt anim %d %f %f %d %d\n", id, anim->accel.toFloat(), anim->speed.toFloat(), anim->frameRate, anim->frameEnd - anim->frameStart + 1);

			//	LOG("from %f to %f\n", s, s + a * c / 30.0f);

			fIndex -= anim->frameStart;
//				LOG("frame: %d\n", fIndex);
			fTime = (fIndex) / 30.0f;
			//fCount = anim->frameEnd - anim->frameStart + 1;
			//LOG("reset\n");
		}
			
		if (anim->state == state) {
			angle += rot;
		}

		int16 *ptr = &level->commands[anim->animCommand];

		for (int i = 0; i < anim->acCount; i++) {
			switch (*ptr++) {
				case 0x01 : { // cmd position
						int16 sx = *ptr++;
						int16 sy = *ptr++;
						int16 sz = *ptr++;
						LOG("move: %d %d\n", (int)sx, (int)sy, (int)sz);
						break;
					}
				case 0x02 : { // cmd jump speed
						int16 sx = *ptr++;
						int16 sz = *ptr++;
						LOG("jump: %d %d\n", (int)sx, (int)sz);
						break;
					}
				case 0x03 : // empty hands
					break;
				case 0x04 : // kill
					break;
				case 0x05 : { // play sound
					int frame = (*ptr++);
					int id    = (*ptr++) & 0x3FFF;
					if (fIndex == frame - anim->frameStart && fIndex != lastFrame) {
						auto a = level->soundsMap[id];
						auto b = level->soundsInfo[a].index;
						auto c = level->soundOffsets[b];

						void *p = &level->soundData[c];

						PlaySound((LPSTR)p, NULL, SND_ASYNC | SND_MEMORY);
					}
					break;
				}
				case 0x06 :
					ptr += 2;
					break;
			}
		}


		float d = 0.0f;

		switch (anim->state) {
			case TR::STATE_BACK :
			case TR::STATE_BACK_JUMP :
			case TR::STATE_FAST_BACK :
				d = PI;
				break;
			case TR::STATE_STEP_LEFT :
			case TR::STATE_LEFT_JUMP :
				d = -PI * 0.5f;
				break;
			case TR::STATE_STEP_RIGHT :
			case TR::STATE_RIGHT_JUMP :
				d =  PI * 0.5f;
				break;
		}
		d += angle;

		float speed = anim->speed.toFloat() + anim->accel.toFloat() * (fTime * 30.0f);			
		pos = pos + vec3(sinf(d), 0, cosf(d)) * (speed * Core::deltaTime * 30.0f);


		lastFrame = fIndex;
	}

};

#endif
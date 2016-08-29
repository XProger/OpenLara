#ifndef H_CONTROLLER
#define H_CONTROLLER

#include "format.h"

#define GRAVITY 7.0f

struct Controller {
	TR::Level		*level;
	int				entity;

	TR::Animation	*anim;
	float fTime;

	vec3	pos, velocity;
	float	angle;

	int state;	// target state
	int lastFrame;

	int sc;
	bool lState;
	bool onGround;

	Controller(TR::Level *level, int entity) : level(level), entity(entity), pos(0.0f), velocity(0.0f), angle(0.0f), fTime(0.0f) {
		anim = &level->anims[getModel().animation];
		lastFrame = 0;

		TR::Entity &e = level->entities[entity];
		pos = vec3(e.x, e.y, e.z);
		angle = e.rotation / 16384.0f * PI * 0.5f;

		sc = 0;
		lState = false;

		state = TR::STATE_STOP;
	}

	void update() {
		float rot = 0.0f;
		
		enum { LEFT = 1, RIGHT = 2, FORTH = 4, BACK = 8, JUMP = 16, WALK = 32, ACTION = 64, WEAPON = 128, ROLL = 256, GROUND = 512, WATER = 1024, DEATH = 2048,
				PULL = 4096, PICKUP = 8192, SWITCH_ON = 16 * 1024, SWITCH_OFF = 32 * 1024, KEY = 64 * 1024, PUZZLE = 128 * 1024, HANG = 256 * 1024, FALL = 512 * 1024, COMPRESS = 1024 * 1024};
		int mask = 0;

		if (Input::down[ikW] || Input::joy.L.y < 0)								mask |= FORTH;
		if (Input::down[ikS] || Input::joy.L.y > 0)								mask |= BACK;
		if (Input::down[ikA] || Input::joy.L.x < 0)								mask |= LEFT;
		if (Input::down[ikD] || Input::joy.L.x > 0)								mask |= RIGHT;
		if (Input::down[ikSpace] || Input::down[ikJoyX])						mask |= JUMP;
		if (Input::down[ikShift] || Input::down[ikJoyLT])						mask |= WALK;
		if (Input::down[ikE] || /*Input::down[ikMouseL] ||*/ Input::down[ikJoyA])	mask |= ACTION;
		if (Input::down[ikQ] || Input::down[ikMouseR] || Input::down[ikJoyY])	mask |= WEAPON;
		if (onGround)															mask |= GROUND;
		if (getRoom().flags & 1)												mask |= WATER;
		if (velocity.y > 2048)													mask |= FALL;
		if (anim->state == TR::STATE_COMPRESS)									mask |= COMPRESS;


		int origMask = mask;
		if (origMask & (FORTH | BACK)) 
			mask &= ~(LEFT | RIGHT);


		int stateMask[TR::STATE_MAX];
		for (int i = 0; i < TR::STATE_MAX; i++)
			stateMask[i] = -1;

		stateMask[TR::STATE_WALK]				= GROUND | FORTH | WALK;
		stateMask[TR::STATE_RUN]				= GROUND | FORTH;
		stateMask[TR::STATE_STOP]				= GROUND;
		stateMask[TR::STATE_FORWARD_JUMP]		= GROUND | JUMP | FORTH;
//		stateMask[TR::STATE_FAST_TURN]			= 0;
		stateMask[TR::STATE_FAST_BACK]			= GROUND | BACK;
		stateMask[TR::STATE_TURN_RIGHT]			= GROUND | RIGHT;
		stateMask[TR::STATE_TURN_LEFT]			= GROUND | LEFT;
		stateMask[TR::STATE_DEATH]				= DEATH;
		stateMask[TR::STATE_FAST_FALL]			= FALL;
		stateMask[TR::STATE_HANG]				= HANG | ACTION;
		stateMask[TR::STATE_REACH]				= ACTION;
//		stateMask[TR::STATE_SPLAT]
//		stateMask[TR::STATE_TREAD]
//		stateMask[TR::STATE_FAST_TURN_14]
		stateMask[TR::STATE_COMPRESS]			= GROUND | JUMP;
		stateMask[TR::STATE_BACK]				= GROUND | WALK | BACK;
		stateMask[TR::STATE_SWIM]				= WATER | FORTH;
//		stateMask[TR::STATE_GLIDE]
//		stateMask[TR::STATE_NULL_19]
//		stateMask[TR::STATE_FAST_TURN_20]
		stateMask[TR::STATE_FAST_TURN_20]		= GROUND | LEFT | RIGHT;
		stateMask[TR::STATE_STEP_RIGHT]			= GROUND | WALK | RIGHT;
		stateMask[TR::STATE_STEP_LEFT]			= GROUND | WALK | LEFT;
		stateMask[TR::STATE_ROLL]				= GROUND | ROLL;
//		stateMask[TR::STATE_SLIDE]
		stateMask[TR::STATE_BACK_JUMP]			= GROUND | COMPRESS | BACK;
		stateMask[TR::STATE_RIGHT_JUMP]			= GROUND | COMPRESS | RIGHT;
		stateMask[TR::STATE_LEFT_JUMP]			= GROUND | COMPRESS | LEFT;
		stateMask[TR::STATE_UP_JUMP]			= GROUND | COMPRESS;

		stateMask[TR::STATE_DIVE]				= WATER;

		stateMask[TR::STATE_PUSH_PULL_READY]	= GROUND | ACTION | PULL;
		stateMask[TR::STATE_PICK_UP]			= GROUND | ACTION | PICKUP;
		stateMask[TR::STATE_SWITCH_ON]			= GROUND | ACTION | SWITCH_ON;
		stateMask[TR::STATE_SWITCH_OFF]			= GROUND | ACTION | SWITCH_OFF;
		stateMask[TR::STATE_USE_KEY]			= GROUND | ACTION | KEY;
		stateMask[TR::STATE_USE_PUZZLE]			= GROUND | ACTION | PUZZLE;

		stateMask[TR::STATE_SWAN_DIVE]			= JUMP | WALK | FORTH;
		

		fTime += Core::deltaTime;
		int fCount = anim->frameEnd - anim->frameStart + 1;
		int fIndex = int(fTime * 30.0f);

		state = -1;
		int maxMask = 0;
		if (stateMask[anim->state] != mask)
			for (int i = 0; i < anim->scCount; i++) {
				TR::AnimState &sc = level->states[anim->scOffset + i];
				if (sc.state >= TR::STATE_MAX || stateMask[sc.state] == -1)
					LOG("unknown state %d\n", sc.state);
				else
					if (stateMask[sc.state] > maxMask && ((stateMask[sc.state] & mask) == stateMask[sc.state])) {
						maxMask = stateMask[sc.state];
						state = anim->scOffset + i;
					}
			}

		if (state > -1 && anim->state != level->states[state].state) {
			TR::AnimState &sc = level->states[state];
			for (int j = 0; j < sc.rangesCount; j++) {
				TR::AnimRange &range = level->ranges[sc.rangesOffset + j];
				if ( anim->frameStart + fIndex >= range.low && anim->frameStart + fIndex <= range.high) {
					int st = anim->state;
					anim  = &level->anims[range.nextAnimation];
					fIndex = range.nextFrame - anim->frameStart;
					fCount = anim->frameEnd - anim->frameStart + 1;
					fTime = fIndex / 30.0f;
					break;
				}
			}
		}

#ifdef _DEBUG
		if (Input::down[ikEnter]) {
			if (!lState) {
				lState = true;
			//	state = TR::STATE_ROLL;
			//	fTime = 0;

			//	sc = (sc + 1) % level->statesCount;
			//	anim = &level->anims[146];//level->ranges[ level->states[sc].rangesOffset ].nextAnimation ];
			//	fTime = 0;
			//	state = level->states[sc].state;
				
				LOG("state: %d\n", anim->state);
				for (int i = 0; i < anim->scCount; i++) {
					auto &sc = level->states[anim->scOffset + i];
					LOG("-> %d : ", (int)sc.state);
					for (int j = 0; j < sc.rangesCount; j++) {
						TR::AnimRange &range = level->ranges[sc.rangesOffset + j];
						LOG("%d ", range.nextAnimation);
						//range.
					}
					LOG("\n");
				}
				
			}
		} else
			lState = false;
#endif
		if (anim->state == TR::STATE_RUN		|| 
			anim->state == TR::STATE_FAST_BACK	||
			anim->state == TR::STATE_WALK		||
			anim->state == TR::STATE_BACK		||
			anim->state == TR::STATE_TURN_LEFT	|| 
			anim->state == TR::STATE_TURN_RIGHT) {

			if (origMask & LEFT)  angle -= Core::deltaTime * PI;
			if (origMask & RIGHT) angle += Core::deltaTime * PI;
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

		bool endFrame = fIndex >= fCount;

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
					int16 sy = *ptr++;
					int16 sz = *ptr++;
					if (endFrame) {
						LOG("jump: %d %d\n", (int)sy, (int)sz);
						velocity.x = sinf(d) * sz;
						velocity.y = sy;
						velocity.z = cosf(d) * sz;
						onGround = false;
					}
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

					//	PlaySound((LPSTR)p, NULL, SND_ASYNC | SND_MEMORY);
					}
					break;
				}
				case 0x06 :
					if (fIndex != lastFrame && fIndex + anim->frameStart == ptr[0]) {
						if (ptr[1] == 0) {
							angle = angle + PI;
						}
					}
					ptr += 2;
					break;
			}
		}



		float dt = Core::deltaTime * 30.0f;

		if (onGround) {
			float speed = anim->speed.toFloat() + anim->accel.toFloat() * (fTime * 30.0f);
			velocity.x = sinf(d) * speed;
			velocity.z = cosf(d) * speed;
		}
		velocity.y += GRAVITY * dt;

		if (endFrame) {
			fIndex = anim->nextFrame;
			int id = anim->nextAnimation;
			anim = &level->anims[anim->nextAnimation];
			fIndex -= anim->frameStart;
			fTime = fIndex / 30.0f;
			fCount = anim->frameEnd - anim->frameStart + 1;
		}
		

		move(velocity * dt);
		collide();

		lastFrame = fIndex;
	}

	void move(const vec3 &offset) {
		vec3 p = pos;
		pos = pos + offset;

		updateEntity();

		TR::Room &room = getRoom();
		TR::Entity &entity = getEntity();

		int dx, dz;
		TR::Room::Sector &s = getSector(dx, dz);

		int d = entity.y - s.floor * 256;
		if (d >= 256 * 4) {
			pos.x = p.x;//vec3(entity.x, entity.y, entity.z);
			pos.z = p.z;
			updateEntity();
			if (d >= 256 * 4) 
				anim = &level->anims[53];	// forward smash
			else
				anim = &level->anims[11];	// instant stand
			state = anim->state;
			fTime = 0;
		}
	}

	void updateEntity() {
		TR::Entity &e = getEntity();
		e.x = int(pos.x);
		e.y = int(pos.y);
		e.z = int(pos.z);
		e.rotation = int(angle / (PI * 0.5f) * 16384.0f);
	}

	bool insideRoom(const vec3 &pos, int room) {
		TR::Room &r = level->rooms[room];
		vec3 min = vec3(r.info.x, r.info.yTop, r.info.z);
		vec3 max = min + vec3(r.xSectors * 1024, r.info.yBottom - r.info.yTop, r.zSectors * 1024);

		return	pos.x >= min.x && pos.x <= max.x && 
				pos.y >= min.y && pos.y <= max.y && 
				pos.z >= min.z && pos.z <= max.z;
	}

	TR::Entity& getEntity() {
		return level->entities[entity];
	}

	TR::Model& getModel() {
		TR::Entity &entity = getEntity();
		for (int i = 0; i < level->modelsCount; i++)
			if (entity.id == level->models[i].id)
				return level->models[i];
	}

	TR::Room& getRoom() {
		return level->rooms[getEntity().room];
	}

	TR::Room::Sector& getSector(int &dx, int &dz) {
		TR::Room &room = getRoom();
		TR::Entity &entity = getEntity();

		dx = entity.x - room.info.x;
		dz = entity.z - room.info.z;
		int sx = dx / 1024;
		int sz = dz / 1024;
		dx -= sx * 1024;
		dz -= sz * 1024;

		return room.sectors[sx * room.zSectors + sz];
	}

	void collide() {
		int dx, dz;
		TR::Room::Sector &s = getSector(dx, dz);
		TR::Entity &entity = getEntity();
	
		float bottom = s.floor * 256;

		float fx = dx / 1024.0f, fz = dz / 1024.0f;

	//	dx -= 512;
	//	dz -= 512;

		uint16 cmd, *d = &level->floors[s.floorIndex];

		if (s.floorIndex)
			do {
				cmd = *d++;
				int func = cmd & 0x00FF;		// function
				int sub  = (cmd & 0x7F00) >> 8;	// sub function

				switch (func) {
					case 1 :
						entity.room = *d++;
						break;
					case 2   :
					case 3 : {
						int8 sx = (int8)(*d & 0x00FF);
						int8 sz = (int8)((*d & 0xFF00) >> 8);

						if (func == 2) {
							if (sx > 0)
								bottom += (int)sx * (1024 - dx) >> 2;
							else
								bottom -= (int)sx * dx >> 2;
					
							if (sz > 0)
								bottom += (int)sz * (1024 - dz) >> 2;
							else
								bottom -= (int)sz * dz >> 2;	
						} else {
							/*
							if (sx < 0) {
								p[0].y += sx;
								p[3].y += sx;
							} else {
								p[1].y -= sx;
								p[2].y -= sx;
							}

							if (sz > 0) {
								p[0].y -= sz;
								p[1].y -= sz;
							} else {
								p[3].y += sz;
								p[2].y += sz;
							}
							*/
						}
						d++;
						break;
					}
					case 4 : {
						/*
						//*d++; // trigger setup
						if (sub == 0x00) LOG("trigger\n");
						if (sub == 0x01) LOG("pad\n");
						if (sub == 0x02) LOG("switch\n");
						if (sub == 0x03) LOG("key\n");
						if (sub == 0x04) LOG("pickup\n");
						if (sub == 0x05) LOG("heavy-trigger\n");
						if (sub == 0x06) LOG("anti-pad\n");
						if (sub == 0x07) LOG("combat\n");
						if (sub == 0x08) LOG("dummy\n");
						if (sub == 0x09) LOG("anti-trigger\n");
						*/
						uint16 act;
						do {
							act = *d++; // trigger action 
						} while (!(act & 0x8000));
				
						break;
					}
					default :
						LOG("unknown func: %d\n", func);
				}

			} while (!(cmd & 0x8000));


		onGround = pos.y > bottom;
		if (onGround) {
			onGround = true;
			if (s.roomBelow != 255) {
				entity.room = s.roomBelow;
				onGround = false;
				return;
			}
			pos.y = bottom;
			velocity.y = 0.0f;
		}

		entity.y = (int)pos.y;
	}

};

#endif
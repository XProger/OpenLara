#ifndef H_CONTROLLER
#define H_CONTROLLER

#include "format.h"

struct Controller {
	TR::Level		*level;
	int				entity;

	TR::Animation	*anim;
	float fTime;

	vec3	pos;
	float	angle;

	int state;	// LaraState
	int lastFrame;

	int sc;
	bool lState;

	Controller(TR::Level *level, int entity) : level(level), entity(entity), pos(0.0f), angle(0.0f), fTime(0.0f) {
		anim = &level->anims[0];
		lastFrame = 0;

		TR::Entity &e = level->entities[entity];
		pos = vec3(e.x, e.y, e.z);
		angle = e.rotation / 16384.0f * PI * 0.5f;

		sc = 0;
		lState = false;
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
			

		if (Input::down[ikEnter]) {
			if (!lState) {
				lState = true;
			//	state = TR::STATE_ROLL;
			//	fTime = 0;

			//	sc = (sc + 1) % level->statesCount;
			//	anim = &level->anims[146];//level->ranges[ level->states[sc].rangesOffset ].nextAnimation ];
			//	fTime = 0;
			//	state = level->states[sc].state;
				/*
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
				*/
			}
		} else
			lState = false;

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
					for (int j = 0; j < sc.rangesCount; j++) {
						auto &range = level->ranges[sc.rangesOffset + j];
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
			fIndex -= anim->frameStart;
			fTime = (fIndex) / 30.0f;
		}
			
		if (anim->state == state) {
			angle += rot;
		}

		float d = 0.0f;

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

						//PlaySound((LPSTR)p, NULL, SND_ASYNC | SND_MEMORY);
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

		move(vec3(sinf(d), 0, cosf(d)) * (speed * Core::deltaTime * 30.0f));
		collide();

		lastFrame = fIndex;
		
		updateEntity();

	/*
		TR::Room &room = level->rooms[level->entities[entity].room];
		for (int i = 0; i < room.portalsCount; i++) {
			if (insideRoom(pos, room.portals[i].roomIndex)) {
				level->entities[entity].room = room.portals[i].roomIndex;
				LOG("set room: %d\n", i);
				break;
			}
		}
		
		
		for (int i = 0; i < level->roomsCount; i++)
			if (insideRoom(pos, i) && i != level->entities[entity].room) {
				level->entities[entity].room = i;
				LOG("set room: %d\n", i);
				break;
			}
		*/
	}

	void checkPortals(const vec3 &oldPos, const vec3 &newPos) {
		TR::Room &room = getRoom();

		TR::Vertex a = { (int)oldPos.x - room.info.x, (int)oldPos.y, (int)oldPos.z - room.info.z };
		TR::Vertex b = { (int)newPos.x - room.info.x, (int)newPos.y, (int)newPos.z - room.info.z };

		for (int i = 0; i < room.portalsCount; i++) {
			TR::Vertex &n = room.portals[i].normal;
			TR::Vertex &v = room.portals[i].vertices[0];
			int d = -(v.x * n.x + v.y * n.y + v.z * n.z);
			int oldSign = sign(a.x * n.x + a.y * n.y + a.z * n.z + d);
			int newSign = sign(b.x * n.x + b.y * n.y + b.z * n.z + d);
			if (oldSign != newSign) {
				getEntity().room = room.portals[i].roomIndex;
				break;
			}
		}

	}

	void move(const vec3 &speed) {
		vec3 p = pos;
		pos = pos + speed;
		//pos.y += 1.0f; TODO: floor portal

		checkPortals(p, pos);

		updateEntity();

		TR::Room &room = getRoom();
		TR::Entity &entity = getEntity();

		int dx, dz;
		TR::Room::Sector &s = getSector(dx, dz);

		int d = entity.y - s.floor * 256;
		if (d >= 256 * 4) {
			pos = p;//vec3(entity.x, entity.y, entity.z);
			updateEntity();
			state = TR::STATE_STOP;
			if (d >= 256 * 4) 
				anim = &level->anims[53];	// forward smash
			else
				anim = &level->anims[11];	// instant stand
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

		uint16 *d = &level->floors[s.floorIndex];
		auto cmd = *d;

		do {
			cmd = *d;
			int func = cmd & 0x001F;		// function
			int sub  = (cmd & 0x7F00) >> 8;	// sub function
			d++;


			if (func == 0x00) {	// portal
				LOG("portal\n");
	//			d++;
			}

			if ((func == 0x02 || func == 0x03) && sub == 0x00) { // floor & ceiling heights
				int sx = (int)(int8)(*d & 0x00FF);
				int sz = (int)(int8)((*d & 0xFF00) >> 8);

				if (func == 0x02) {
					if (sx > 0)
						bottom += sx * (1024 - dx) >> 2;
					else
						bottom -= sx * dx >> 2;
					
					if (sz > 0)
						bottom += sz * (1024 - dz) >> 2;
					else
						bottom -= sz * dz >> 2;	
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

	//			d++;
			}

			d++;


	//		LOG("%d %d\n", func, sub);
		} while ((cmd & 0x8000) == 0);			// end

		pos.y = bottom;

		entity.y = (int)pos.y;
	}

};

#endif
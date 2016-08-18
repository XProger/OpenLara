#ifndef H_LEVEL
#define H_LEVEL

#include "utils.h"
#include "format.h"
#include "controller.h"

struct Level {
	TR::Level level;
	Texture **textures;
	float time;
	Controller *lara;

	Level(const char *name) : level(Stream(name)), time(0.0f) {
		if (level.tilesCount) {
			textures = new Texture*[level.tilesCount];

			for (int i = 0; i < level.tilesCount; i++) {
			//	sprintf(buf, "LEVEL1_%d.PVR", i);
			//	textures[i] = Core::load<Texture>(buf);
				textures[i] = getTexture(i);
			}
		} else 
			textures = NULL;

		lara = new Controller(&level);
	}

	~Level() {
		for (int i = 0; i < level.tilesCount; i++)
			delete textures[i];
		delete[] textures;
	}

	Texture *getTexture(int tile) {
		TR::RGBA data[256 * 256];
		for (int i = 0; i < 256 * 256; i++) {
			int index = level.tiles[tile].index[i];
			auto p = level.palette[index];
			data[i].r = p.r;
			data[i].g = p.g;
			data[i].b = p.b;
			data[i].a = index == 0 ? 0 : 255;
		}
		return new Texture(256, 256, 0, data);
	}

	void setTexture(int objTexture) {
		auto &t = level.objectTextures[objTexture];
		Core::setBlending(t.attribute == 2 ? bmAdd : bmAlpha);
		textures[t.tileAndFlag & 0x7FFF]->bind(0);
	}

	TR::StaticMesh* getMeshByID(int id) {
		for (int i = 0; i < level.staticMeshesCount; i++)
			if (level.staticMeshes[i].id == id)
				return &level.staticMeshes[i];
		return NULL;
	}


	#define SCALE (1.0f / 1024.0f / 2.0f)

	void renderRoom(const TR::Room &room) {
		glPushMatrix();
		glTranslatef(room.info.x, 0.0f, room.info.z);

		// rectangles
		for (int j = 0; j < room.data.rCount; j++) {
			auto &f = room.data.rectangles[j];
			auto &t = level.objectTextures[f.texture];
			setTexture(f.texture);

			glBegin(GL_QUADS);
			for (int k = 0; k < 4; k++) {
				auto &v = room.data.vertices[f.vertices[k]];
				float a = 1.0f - v.lighting / 8191.0f;
				glColor3f(a, a, a);
				glTexCoord2f(t.vertices[k].Xpixel / 256.0f, t.vertices[k].Ypixel / 256.0f);
				glVertex3f(v.vertex.x, v.vertex.y, v.vertex.z);
			}
			glEnd();
		}
		
		// triangles
		for (int j = 0; j < room.data.tCount; j++) {
			auto &f = room.data.triangles[j];
			auto &t = level.objectTextures[f.texture];
			setTexture(f.texture);

			glBegin(GL_TRIANGLES);	
			for (int k = 0; k < 3; k++) {
				auto &v = room.data.vertices[f.vertices[k]];
				float a = 1.0f - v.lighting / 8191.0f;
				glColor3f(a, a, a);
				glTexCoord2f(t.vertices[k].Xpixel / 256.0f, t.vertices[k].Ypixel / 256.0f);
				glVertex3f(v.vertex.x, v.vertex.y, v.vertex.z);
			}
			glEnd();
		}
		glPopMatrix();
	
		// meshes
		float a = 1.0f - room.ambient / 8191.0f;

		for (int j = 0; j < room.meshesCount; j++) {
			auto rMesh = room.meshes[j];
			auto sMesh = getMeshByID(rMesh.meshID);
			ASSERT(sMesh != NULL);

			glPushMatrix();
			glTranslatef(rMesh.x, rMesh.y, rMesh.z);
			glRotatef((rMesh.rotation >> 14) * 90.0f, 0, 1, 0);
			
			renderMesh(sMesh->mesh, vec3(a));

			glPopMatrix();
		}
	
		// sprites
		Core::setBlending(bmAlpha);
		for (int j = 0; j < room.data.sCount; j++)
			renderSprite(room, room.data.sprites[j]);
	}

	
	void renderMesh(uint32 meshOffset, const vec3 &color) {
	// remap mesh
		#define OFFSET(bytes) (ptr = (TR::Mesh*)((char*)ptr + bytes - sizeof(char*)))

		TR::Mesh *ptr = (TR::Mesh*)((char*)level.meshData + level.meshOffsets[meshOffset]);
		TR::Mesh mesh;
		mesh.center = ptr->center;
		mesh.radius = ptr->radius;
		mesh.vCount = ptr->vCount;
		mesh.vertices = (TR::Vertex*)&ptr->vertices;
		OFFSET(mesh.vCount * sizeof(TR::Vertex));
		mesh.nCount = ptr->nCount;
		mesh.normals = (TR::Vertex*)&ptr->normals;
		if (mesh.nCount > 0)
			OFFSET(mesh.nCount * sizeof(TR::Vertex));
		else
			OFFSET(-mesh.nCount * sizeof(int16));

		mesh.rCount = ptr->rCount;
		mesh.rectangles = (TR::Rectangle*)&ptr->rectangles;
		OFFSET(mesh.rCount * sizeof(TR::Rectangle));

		mesh.tCount = ptr->tCount;
		mesh.triangles = (TR::Triangle*)&ptr->triangles;
		OFFSET(mesh.tCount * sizeof(TR::Triangle));

		mesh.crCount = ptr->crCount;
		mesh.crectangles = (TR::Rectangle*)&ptr->crectangles;
		OFFSET(mesh.crCount * sizeof(TR::Rectangle));

		mesh.ctCount = ptr->ctCount;
		mesh.ctriangles = (TR::Triangle*)&ptr->ctriangles;
		OFFSET(mesh.ctCount * sizeof(TR::Triangle));

		if (mesh.nCount > 0)
			glEnable(GL_LIGHTING);
		glColor3f(color.x, color.y, color.z);

		// triangles
		for (int j = 0; j < mesh.tCount; j++) {
			auto &f = mesh.triangles[j];
			auto &t = level.objectTextures[f.texture];
			setTexture(f.texture);
				
			glBegin(GL_TRIANGLES);	
			for (int k = 0; k < 3; k++) {
				auto &v = mesh.vertices[f.vertices[k]];

				if (mesh.nCount > 0) {
					auto vn = mesh.normals[f.vertices[k]];
					vec3 n = vec3(vn.x, vn.y, vn.z).normal();
					glNormal3f(n.x, n.y, n.z);
				} else {
					auto l = mesh.lights[f.vertices[k]];
					float a = 1.0f - l / 8191.0f;
					glColor3f(a, a, a);
				}

				glTexCoord2f(t.vertices[k].Xpixel / 256.0f, t.vertices[k].Ypixel / 256.0f);
				glVertex3f(v.x, v.y, v.z);
			}
			glEnd();
		}

		// rectangles
		for (int j = 0; j < mesh.rCount; j++) {
			auto &f = mesh.rectangles[j];
			auto &t = level.objectTextures[f.texture];
			setTexture(f.texture);
				
			glBegin(GL_QUADS);	
			for (int k = 0; k < 4; k++) {
				auto &v = mesh.vertices[f.vertices[k]];

				if (mesh.nCount > 0) {
					auto vn = mesh.normals[f.vertices[k]];
					vec3 n = vec3(vn.x, vn.y, vn.z).normal();
					glNormal3f(n.x, n.y, n.z);
				} else {
					auto l = mesh.lights[f.vertices[k]];
					float a = 1.0f - l / 8191.0f;
					glColor3f(a, a, a);
				}
				glTexCoord2f(t.vertices[k].Xpixel / 256.0f, t.vertices[k].Ypixel / 256.0f);
				glVertex3f(v.x, v.y, v.z);
			}
			glEnd();
		}
					
		glDisable(GL_TEXTURE_2D);
		// debug normals

		// triangles (colored)
		for (int j = 0; j < mesh.ctCount; j++) {
			auto &f = mesh.ctriangles[j];
			auto &c = level.palette[f.texture & 0xFF];
				
			glBegin(GL_TRIANGLES);	
			for (int k = 0; k < 3; k++) {
				auto &v = mesh.vertices[f.vertices[k]];

				if (mesh.nCount > 0) {
					auto vn = mesh.normals[f.vertices[k]];
					vec3 n = vec3(vn.x, vn.y, vn.z).normal();
					glColor3f(c.r / 255.0f * color.x, c.g / 255.0f * color.y, c.b / 255.0f * color.z);
					glNormal3f(n.x, n.y, n.z);
				} else {
					auto l = mesh.lights[f.vertices[k]];
					float a = (1.0f - l / 8191.0f) / 255.0f;
					glColor3f(c.r * a, c.g * a, c.b * a);
				}
				glVertex3f(v.x, v.y, v.z);
			}
			glEnd();
		}
		// rectangles (colored)
		for (int j = 0; j < mesh.crCount; j++) {
			auto &f = mesh.crectangles[j];
			auto &c = level.palette[f.texture & 0xFF];
				
			glBegin(GL_QUADS);	
			for (int k = 0; k < 4; k++) {
				auto &v = mesh.vertices[f.vertices[k]];

				if (mesh.nCount > 0) {
					auto vn = mesh.normals[f.vertices[k]];
					vec3 n = vec3(vn.x, vn.y, vn.z).normal();
					glColor3f(c.r / 255.0f * color.x, c.g / 255.0f * color.y, c.b / 255.0f * color.z);
					glNormal3f(n.x, n.y, n.z);
				} else {
					auto l = mesh.lights[f.vertices[k]];
					float a = (1.0f - l / 8191.0f) / 255.0f;
					glColor3f(c.r * a, c.g * a, c.b * a);
				}
				glVertex3f(v.x, v.y, v.z);
			}
			glEnd();
		}
		glEnable(GL_TEXTURE_2D);

		if (mesh.nCount > 0)
			glDisable(GL_LIGHTING);
	}
	
	void renderSprite(const TR::SpriteTexture &sprite) {
		float u0 = sprite.u / 256.0f;
		float v0 = sprite.v / 255.0f;
		float u1 = u0 + sprite.w / (256.0f * 256.0f);
		float v1 = v0 + sprite.h / (256.0f * 256.0f);	

		mat4 m = Core::mView.inverse();
		vec3 up = m.up.xyz * vec3(-1, -1, 1) * (-1);
		vec3 right = m.right.xyz * vec3(-1, -1, 1);

		vec3 p[4];
		p[0] = right * sprite.r + up * sprite.b;
		p[1] = right * sprite.l + up * sprite.b;
		p[2] = right * sprite.l + up * sprite.t;
		p[3] = right * sprite.r + up * sprite.t;

		textures[sprite.tile]->bind(0);
		glBegin(GL_QUADS);
			glTexCoord2f(u0, v1);
			glVertex3fv((GLfloat*)&p[0]);
			glTexCoord2f(u1, v1);		   
			glVertex3fv((GLfloat*)&p[1]);
			glTexCoord2f(u1, v0);		   
			glVertex3fv((GLfloat*)&p[2]);
			glTexCoord2f(u0, v0);		   
			glVertex3fv((GLfloat*)&p[3]);
		glEnd();
	}

	void renderSprite(const TR::Room &room, const TR::Room::Data::Sprite &sprite) {
		auto &v = room.data.vertices[sprite.vertex];
		float a = 1.0f - v.lighting / (float)0x1FFF;
		glColor3f(a, a, a);

		glPushMatrix();
		glTranslatef(v.vertex.x + room.info.x, v.vertex.y, v.vertex.z + room.info.z);

		renderSprite(level.spriteTextures[sprite.texture]);

		glPopMatrix();
	}
	
	vec3 getAngle(TR::AnimFrame *frame, int index) {
		#define ANGLE_SCALE (2.0f * PI / 1024.0f)

		uint16 b = frame->angles[index * 2 + 0];
		uint16 a = frame->angles[index * 2 + 1];

		return vec3((a & 0x3FF0) >> 4, ( ((a & 0x000F) << 6) | ((b & 0xFC00) >> 10)), b & 0x03FF) * ANGLE_SCALE;
	}

	float lerpAngle(float a, float b, float t) {
		float d = b - a;
		if (d >= PI) 
			a += PI * 2.0f;
		else
			if (d <= -PI)
				a -= PI * 2.0f;
		return a + (b - a) * t;
	}

	quat lerpAngle(const vec3 &a, const vec3 &b, float t) {
	//	return vec3(lerpAngle(a.x, b.x, t),
	//				lerpAngle(a.y, b.y, t),
	//				lerpAngle(a.z, b.z, t));
	
		mat4 ma, mb;
		ma.identity();
		mb.identity();

		ma.rotateY(a.y);
		ma.rotateX(a.x);
		ma.rotateZ(a.z);

		mb.rotateY(b.y);
		mb.rotateX(b.x);
		mb.rotateZ(b.z);

		return ma.getRot().slerp(mb.getRot(), t).normal();
	}

	float debugTime = 0.0f;

	void renderModel(const TR::Model &model) {
		mat4 m;
		m.identity();
		
		TR::Animation *anim = &level.anims[model.animation];

		float fTime = time;

		if (model.id == ENTITY_LARA) {
			anim = lara->anim;
			fTime = lara->fTime;
			m.translate(lara->pos);
			m.rotateY(lara->angle);
		}

		float k = fTime * 30.0f / anim->frameRate;
		int fIndex = (int)k; 
		int fCount = (anim->frameEnd - anim->frameStart) / anim->frameRate + 1;

		int fSize = sizeof(TR::AnimFrame) + model.mCount * sizeof(uint16) * 2;
		k = k - fIndex;


		int fIndexA = fIndex % fCount, fIndexB = (fIndex + 1) % fCount;
		TR::AnimFrame *frameA = (TR::AnimFrame*)&level.frameData[(anim->frameOffset + fIndexA * fSize) >> 1];
			
		TR::Animation *nextAnim = NULL;
	
	   	if (fIndexB == 0) {
			nextAnim = &level.anims[anim->nextAnimation];
			fIndexB = (anim->nextFrame - nextAnim->frameStart) / anim->frameRate;
		} else
			nextAnim = anim;

//		LOG("%d %f\n", fIndexA, fTime);


		TR::AnimFrame *frameB = (TR::AnimFrame*)&level.frameData[(nextAnim->frameOffset + fIndexB * fSize) >> 1];



//		ASSERT(fpSize == fSize);
//		fSize = fpSize;

	//	LOG("%d\n", fIndex % fCount);
		//if (fCount > 1) LOG("%d %d\n", model->id, fCount);
	//	LOG("%d\n", fIndex % fCount);


//		Debug::Draw::box(Box(vec3(frameA->minX, frameA->minY, frameA->minZ), vec3(frameA->maxX, frameA->maxY, frameA->maxZ)));

		TR::Node *node = (int)model.node < level.nodesDataSize ? (TR::Node*)&level.nodesData[model.node] : NULL;

		int sIndex = 0;
		mat4 stack[20];

		m.translate(vec3(frameA->x, frameA->y, frameA->z).lerp(vec3(frameB->x, frameB->y, frameB->z), k));

		for (int i = 0; i < model.mCount; i++) {

			if (i > 0 && node) {
				TR::Node &t = node[i - 1];

				if (t.flags & 0x01) m = stack[--sIndex];
				if (t.flags & 0x02) stack[sIndex++] = m;

				ASSERT(sIndex >= 0);
				ASSERT(sIndex < 20);

				m.translate(vec3(t.x, t.y, t.z));
			}
			
			quat q = lerpAngle(getAngle(frameA, i), getAngle(frameB, i), k);
			m = m * mat4(q, vec3(0.0f));
			
		
		//	vec3 angle = lerpAngle(getAngle(frameA, i), getAngle(frameB, i), k);
		//	m.rotateY(angle.y);
		//	m.rotateX(angle.x);
		//	m.rotateZ(angle.z);
			

			glPushMatrix();
			glMultMatrixf((GLfloat*)&m);
			renderMesh(model.mStart + i, vec3(1.0f));
			glPopMatrix();
		}
	}

	void renderEntity(const TR::Entity &entity) {
		glPushMatrix();
		glTranslatef(entity.x, entity.y, entity.z);

		if (entity.intensity > -1) {
			float a = 1.0f - entity.intensity / (float)0x1FFF;
			glColor3f(a, a, a);
		} else
			glColor3f(1, 1, 1);
	

		for (int i = 0; i < level.modelsCount; i++)
			if (entity.id == level.models[i].id) {
				glRotatef((entity.rotation >> 14) * 90.0f, 0, 1, 0);
				renderModel(level.models[i]);
				break;
			}
		
		for (int i = 0; i < level.spriteSequencesCount; i++)
			if (entity.id == level.spriteSequences[i].id) {
				renderSprite(level.spriteTextures[level.spriteSequences[i].sStart]);
				break;
			}

		glPopMatrix();
	}


	/*
	void debugPortals() {
		glDisable(GL_TEXTURE_2D);
		Core::setBlending(bmAdd);
		glColor3f(0, 0.25f, 0);
		glDepthMask(GL_FALSE);

		glPushMatrix();
		glScalef(-SCALE, -SCALE, SCALE);

		glBegin(GL_QUADS);
		for (int i = 0; i < rooms.count; i++) {
			int x = rooms[i]->info.x;
			int z = rooms[i]->info.z;
			for (int j = 0; j < rooms[i]->portals.count; j++) {
				auto &p = rooms[i]->portals[j];
				for (int k = 0; k < 4; k++) {
					auto &v = p.vertices[k];
					glVertex3f(v.x + x, v.y, v.z + z);
				}
			}
		}
		glEnd();

		glPopMatrix();

		glDepthMask(GL_TRUE);
		glEnable(GL_TEXTURE_2D);
		Core::setBlending(bmAlpha);
	}

	void debugFloor(const vec3 &f, const vec3 &c, int floorIndex) {
		vec3 vf[4] = { f, f + vec3(1024, 0, 0), f + vec3(1024, 0, 1024), f + vec3(0, 0, 1024) };
		vec3 vc[4] = { c, c + vec3(1024, 0, 0), c + vec3(1024, 0, 1024), c + vec3(0, 0, 1024) };

		auto *d = &floors[floorIndex];
		auto cmd = *d;

		do {
			cmd = *d;
			int func = cmd & 0x001F;			// function
			int sub  = (cmd & 0x7F00) >> 8;	// sub function
			d++;


			if (func == 0x00) {	// portal
	//			d++;
			}

			if ((func == 0x02 || func == 0x03) && sub == 0x00) { // floor & ceiling corners
				int sx = 256 * (int)(int8)(*d & 0x00FF);
				int sz = 256 * (int)(int8)((*d & 0xFF00) >> 8);

				auto &p = func == 0x02 ? vf : vc;

				if (func == 0x02) {

					if (sx > 0) {
						p[0].y += sx;
						p[3].y += sx;
					} else {
						p[1].y -= sx;
						p[2].y -= sx;
					}

					if (sz > 0) {
						p[0].y += sz;
						p[1].y += sz;
					} else {
						p[3].y -= sz;
						p[2].y -= sz;
					}

				} else {

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

				}

	//			d++;
			}

			d++;


	//		LOG("%d %d\n", func, sub);
		} while ((cmd & 0x8000) == 0);			// end

		glColor3f(0, 1, 0);
		glBegin(GL_LINE_STRIP);
			for (int i = 0; i < 5; i++)
				glVertex3fv((GLfloat*)&vf[i % 4]);
		glEnd();

		glColor3f(1, 0, 0);
		glBegin(GL_LINE_STRIP);
			for (int i = 0; i < 5; i++)
				glVertex3fv((GLfloat*)&vc[i % 4]);
		glEnd();
	}

	void debugSectors(tr_room *room) {
		vec3 p = (Core::viewPos / SCALE - vec3(-room->info.x, 0, room->info.z)) / vec3(1024, 1, 1024);
		int px = (int)-p.x;
		int pz = (int)p.z;

		for (int z = 0; z < room->zSectors; z++)
			for (int x = 0; x < room->xSectors; x++) {
				auto &s = room->sectors[x * room->zSectors + z];
				vec3 f(x * 1024 + room->info.x, s.floor * 256, z * 1024 + room->info.z);
				vec3 c(x * 1024 + room->info.x, s.ceiling * 256, z * 1024 + room->info.z);

				debugFloor(f, c, s.floorIndex);
			}
	}

	void debugRooms() {
		glDisable(GL_TEXTURE_2D);
		Core::setBlending(bmAdd);
		glColor3f(0, 0.25f, 0);
		glDepthMask(GL_FALSE);

		glPushMatrix();
		glScalef(-SCALE, -SCALE, SCALE);

		for (int i = 0; i < rooms.count; i++) {
			auto &r = *rooms[i];
			vec3 p = vec3(r.info.x, r.info.yTop, r.info.z);
			if (isInsideRoom(Core::viewPos, rooms[i])) {
				debugSectors(rooms[i]);
				glColor3f(0, 1, 0);
			} else
				glColor3f(1, 1, 1);

			Debug::Draw::box(Box(p, p + vec3(r.xSectors * 1024, r.info.yBottom - r.info.yTop, r.zSectors * 1024)));
		}

		glPopMatrix();

		glDepthMask(GL_TRUE);
		glEnable(GL_TEXTURE_2D);
		Core::setBlending(bmAlpha);
	}

	void debugMeshes() {
		glPushMatrix();
		glScalef(-SCALE, -SCALE, SCALE);
		for (int i = 0; i < meshes.count; i++) {
			renderMesh(meshes[i], vec3(1.0f));
			glTranslatef(-128, 0, 0);
		}
		glPopMatrix();
	}

	void debugLights() {
		glDisable(GL_TEXTURE_2D);
		glPointSize(8);
		glBegin(GL_POINTS);
		for (int i = 0; i < rooms.count; i++)
			for (int j = 0; j < rooms[i]->lights.count; j++) {
				auto &l = rooms[i]->lights[j];
				float a = l.Intensity1 / 8191.0f;
				glColor3f(a, a, a);
				glVertex3f(-l.x * SCALE, -l.y * SCALE, l.z * SCALE);
			}
		glEnd();
		glEnable(GL_TEXTURE_2D);
	}

	void debugEntity() {
		Core::setCulling(cfNone);
		Core::active.shader = NULL;
		glUseProgram(0);

		mat4 mProj;
		glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat*)&mProj);

		glPushMatrix();
		glScalef(-SCALE, -SCALE, SCALE);

		for (int i = 0; i < entities.count; i++) {
			tr_entity *entity = &entities[i];

			glPushMatrix();
			glTranslatef(entity->x, entity->y, entity->z);

			for (int i = 0; i < models.count; i++)
				if (entity->id == models[i].id) {
					glRotatef((entity->rotation >> 14) * 90.0f, 0, 1, 0);
					tr_anim_frame *frame = (tr_anim_frame*)&frames[models[i].frame >> 1];
					glTranslatef(frame->x, frame->y, frame->z);
					break;
				}
		
			mat4 mView, mViewProj;
			glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)&mView);
			mViewProj = mProj * mView;
			vec4 p = mViewProj * vec4(0, 0, 0, 1);
			if (p.w > 0) {
				p.xyz /= p.w;
				p.y = -p.y;
	
				p.xy = (p.xy * 0.5f + 0.5f) * vec2(Core::width, Core::height);
				char buf[16];
				sprintf(buf, "%d", entity->id);
	
				UI::begin();
 				font->print(p.xy, vec4(1, 0, 0, 1), buf);
				UI::end();
			}
		
			glPopMatrix();
		}
		glPopMatrix();

		Core::setCulling(cfFront);
		Core::active.shader = NULL;
		glUseProgram(0);
	}

	bool isInsideRoom(const vec3 pos, tr_room *room) {
		vec3 min = vec3(room->info.x, room->info.yTop, room->info.z);
		Box box(min, min + vec3(room->xSectors * 1024, room->info.yBottom - room->info.yTop, room->zSectors * 1024));
		return box.intersect(vec3(-pos.x, -pos.y, pos.z) / SCALE);
	}
	*/
	float tickTextureAnimation = 0.0f;

	void update() {
		time += Core::deltaTime;
		lara->update();

		if (tickTextureAnimation > 0.25f) {
			tickTextureAnimation = 0.0f;

			if (level.animTexturesDataSize) {
				uint16 *ptr = &level.animTexturesData[0];
				int count = *ptr++;
				for (int i = 0; i < count; i++) {
					auto animTex = (TR::AnimTexture*)ptr;
					auto id = level.objectTextures[animTex->textures[0]];
					for (int j = 0; j < animTex->tCount; j++) // tCount = count of textures in animation group - 1 (!!!)
						level.objectTextures[animTex->textures[j]] = level.objectTextures[animTex->textures[j + 1]];
					level.objectTextures[animTex->textures[animTex->tCount]] = id;
					ptr += (sizeof(TR::AnimTexture) + sizeof(animTex->textures[0]) * (animTex->tCount + 1)) / sizeof(uint16);
				}
			}
		} else
			tickTextureAnimation += Core::deltaTime;
	}

	void render() {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.9f);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_NORMALIZE);
		glEnable(GL_COLOR_MATERIAL);
		glEnable(GL_LIGHT0);

		Core::setCulling(cfFront);
		glColor3f(1, 1, 1);

		glScalef(-SCALE, -SCALE, SCALE);

		for (int i = 0; i < level.roomsCount; i++)
			renderRoom(level.rooms[i]);

		for (int i = 0; i < level.entitiesCount; i++)
			renderEntity(level.entities[i]);

	//	debugRooms();
	//	debugMeshes();
	//	debugLights();
	//	debugPortals();
	//	debugEntity();
	}

};

#endif
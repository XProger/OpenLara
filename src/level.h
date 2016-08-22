#ifndef H_LEVEL
#define H_LEVEL

#include "core.h"
#include "utils.h"
#include "format.h"
#include "controller.h"
#include "camera.h"
#include "debug.h"

const char SHADER[] = 
	#include "shader.glsl"
;

struct Level {
	TR::Level	level;
	Shader		*shader;
	Texture		*atlas;
	Mesh		*mesh;

	Controller	*lara;

	float		time;

	MeshRange	*rangeRooms;

	Camera		camera;


	int mCount;
	struct MeshInfo : MeshRange {
		int			offset;
		TR::Vertex	center;
		int32		radius;
	} *meshInfo;

	Level(const char *name) : level(Stream(name)), time(0.0f) {
		shader = new Shader(SHADER);
		initAtlas();
		initMesh();

		int entity = 0;
		for (int i = 0; i < level.entitiesCount; i++)
			if (level.entities[i].id == ENTITY_LARA) {
				entity = i;
				break;
			}

		lara = new Controller(&level, entity);

		camera.fov		= 90.0f;
		camera.znear	= 0.1f * 2048.0f;
		camera.zfar		= 1000.0f * 2048.0f;
		camera.pos		= vec3(-lara->pos.x, -lara->pos.y, lara->pos.z) + vec3(0, 1024, -512);
//		camera.pos		= vec3(-10, -2, 26);
//		camera.pos		= vec3(-13.25f, 0.42f, 38.06f) * 2048.0f;
//		camera.pos		= vec3(-36, -1, 2);
		camera.angle	= vec3(0, PI, 0);
	}

	~Level() {
		delete shader;
		delete atlas;
		delete mesh;
		delete[] rangeRooms;
		delete[] meshInfo;

		delete lara;
	}

	void initAtlas() {
		if (!level.tilesCount) {
			atlas = NULL;
			return;
		}

		TR::RGBA *data = new TR::RGBA[1024 * 1024];
		for (int i = 0; i < level.tilesCount; i++) {
			int tx = (i % 4) * 256;
			int ty = (i / 4) * 256;

			TR::RGBA *ptr = &data[ty * 1024 + tx];
			for (int y = 0; y < 256; y++) {
				for (int x = 0; x < 256; x++) {
					int index = level.tiles[i].index[y * 256 + x];
					auto p = level.palette[index];
					ptr[x].r = p.r;
					ptr[x].g = p.g;
					ptr[x].b = p.b;
					ptr[x].a = index == 0 ? 0 : 255;
				}
				ptr += 1024;
			}
		}

		for (int y = 1020; y < 1024; y++)
			for (int x = 1020; x < 1024; x++) {
				int i = y * 1024 + x;
				data[i].r = data[i].g = data[i].b = data[i].a = 255;	// white texel for colored triangles
			}

		atlas = new Texture(1024, 1024, 0, data);
		delete[] data;
	}

	void initMesh() {
		// TODO: sort by texture attribute (t.attribute == 2 ? bmAdd : bmAlpha)

		rangeRooms = new MeshRange[level.roomsCount];

		int iCount = 0, vCount = 0;

	// get rooms mesh info
		for (int i = 0; i < level.roomsCount; i++) {
			rangeRooms[i].vStart = vCount;
			rangeRooms[i].iStart = iCount;
			TR::Room::Data &d = level.rooms[i].data;
			iCount += d.rCount * 6 + d.tCount * 3;
			vCount += d.rCount * 4 + d.tCount * 3;
			rangeRooms[i].iCount = iCount - rangeRooms[i].iStart;
		}
	// get objects mesh info
		#define OFFSET(bytes) (ptr = (TR::Mesh*)((char*)ptr + (bytes) - sizeof(char*)))

		mCount = 0;
		TR::Mesh *ptr = (TR::Mesh*)level.meshData;
		while ( ((int)ptr - (int)level.meshData) < level.meshDataSize * 2 ) {
			mCount++;

			OFFSET(ptr->vCount * sizeof(TR::Vertex));
			if (ptr->nCount > 0)
				OFFSET(ptr->nCount * sizeof(TR::Vertex));
			else
				OFFSET(-ptr->nCount * sizeof(int16));

			iCount += ptr->rCount * 6;
			vCount += ptr->rCount * 4;
			OFFSET(ptr->rCount * sizeof(TR::Rectangle));

			iCount += ptr->tCount * 3;
			vCount += ptr->tCount * 3;
			OFFSET(ptr->tCount * sizeof(TR::Triangle));

			iCount += ptr->crCount * 6;
			vCount += ptr->crCount * 4;
			OFFSET(ptr->crCount * sizeof(TR::Rectangle));

			iCount += ptr->ctCount * 3;
			vCount += ptr->ctCount * 3;
			OFFSET(ptr->ctCount * sizeof(TR::Triangle) + sizeof(TR::Mesh));
			ptr = (TR::Mesh*)(((int)ptr + 3) & ~3);
		}
		meshInfo = new MeshInfo[mCount];

		Index  *indices  = new Index[iCount];
		Vertex *vertices = new Vertex[vCount];
		iCount = vCount = 0;

	// rooms geometry
		for (int i = 0; i < level.roomsCount; i++) {
			TR::Room::Data &d = level.rooms[i].data;

			int vStart = vCount;

			for (int j = 0; j < d.rCount; j++) {
				auto &f = d.rectangles[j];
				auto &t = level.objectTextures[f.texture];

				int  tile = t.tileAndFlag & 0x7FFF;
				int  tx = (tile % 4) * 256;
				int  ty = (tile / 4) * 256;

				int  vIndex = vCount - vStart;

				indices[iCount + 0] = vIndex + 0;
				indices[iCount + 1] = vIndex + 1;
				indices[iCount + 2] = vIndex + 2;

				indices[iCount + 3] = vIndex + 0;
				indices[iCount + 4] = vIndex + 2;
				indices[iCount + 5] = vIndex + 3;

				iCount += 6;

				for (int k = 0; k < 4; k++) {
					auto &v = d.vertices[f.vertices[k]];
					uint8 a = 255 - (v.lighting >> 5);
					
					vertices[vCount].coord		= { v.vertex.x, v.vertex.y, v.vertex.z };
					vertices[vCount].color		= { a, a, a, 255 };
					vertices[vCount].normal		= { 0, 0, 0, 0x7FFF };
					vertices[vCount].texCoord	= { ((tx + t.vertices[k].Xpixel) << 5) + 16, ((ty + t.vertices[k].Ypixel) << 5) + 16};
					vCount++;
				}
			}

			for (int j = 0; j < d.tCount; j++) {
				auto &f = d.triangles[j];
				auto &t = level.objectTextures[f.texture];

				int  tile = t.tileAndFlag & 0x7FFF;
				int  tx = (tile % 4) * 256;
				int  ty = (tile / 4) * 256;

				int  vIndex = vCount - vStart;

				indices[iCount + 0] = vIndex + 0;
				indices[iCount + 1] = vIndex + 1;
				indices[iCount + 2] = vIndex + 2;

				iCount += 3;

				for (int k = 0; k < 3; k++) {
					auto &v = d.vertices[f.vertices[k]];
					uint8 a = 255 - (v.lighting >> 5);
					
					vertices[vCount].coord		= { v.vertex.x, v.vertex.y, v.vertex.z };
					vertices[vCount].color		= { a, a, a, 255 };
					vertices[vCount].normal		= { 0, 0, 0, 0x7FFF };
					vertices[vCount].texCoord	= { ((tx + t.vertices[k].Xpixel) << 5) + 16, ((ty + t.vertices[k].Ypixel) << 5) + 16};
					vCount++;
				}
			}
		}

	// objects geometry
		mCount = 0;
		ptr = (TR::Mesh*)level.meshData;
		while ( ((int)ptr - (int)level.meshData) < level.meshDataSize * sizeof(uint16) ) {
			MeshInfo &info = meshInfo[mCount++];
			info.offset = (int)ptr - (int)level.meshData;
			info.vStart = vCount;
			info.iStart = iCount;	
			info.center = ptr->center;
			info.radius = ptr->radius;

			TR::Vertex *mVertices = (TR::Vertex*)&ptr->vertices;

			OFFSET(ptr->vCount * sizeof(TR::Vertex));

			TR::Vertex	*normals = NULL;
			int16		*lights  = NULL;
			int			nCount   = ptr->nCount;

			if (ptr->nCount > 0) {
				normals = (TR::Vertex*)&ptr->normals;
				OFFSET(ptr->nCount * sizeof(TR::Vertex));
			} else {
				lights = (int16*)&ptr->lights;
				OFFSET(-ptr->nCount * sizeof(int16));
			}

			int vStart = vCount;
		// rectangles
			for (int j = 0; j < ptr->rCount; j++) {
				auto &f = ((TR::Rectangle*)&ptr->rectangles)[j];
				auto &t = level.objectTextures[f.texture];

				int  tile = t.tileAndFlag & 0x7FFF;
				int  tx = (tile % 4) * 256;
				int  ty = (tile / 4) * 256;

				int  vIndex = vCount - vStart;

				indices[iCount + 0] = vIndex + 0;
				indices[iCount + 1] = vIndex + 1;
				indices[iCount + 2] = vIndex + 2;

				indices[iCount + 3] = vIndex + 0;
				indices[iCount + 4] = vIndex + 2;
				indices[iCount + 5] = vIndex + 3;

				iCount += 6;

				for (int k = 0; k < 4; k++) {
					auto &v = mVertices[f.vertices[k]];
					
					vertices[vCount].coord		= { v.x, v.y, v.z };

					if (nCount > 0) {
						TR::Vertex &n = normals[f.vertices[k]];
						vertices[vCount].normal	= { n.x, n.y, n.z, 0 };
						vertices[vCount].color	= { 255, 255, 255, 255 };
					} else {
						uint8 a = 255 - (lights[f.vertices[k]] >> 5);
						vertices[vCount].normal	= { 0, 0, 0, 255 };
						vertices[vCount].color	= { a, a, a, 255 };
					}
					vertices[vCount].texCoord	= { ((tx + t.vertices[k].Xpixel) << 5) + 16, ((ty + t.vertices[k].Ypixel) << 5) + 16};
					vCount++;
				}
			}
			OFFSET(ptr->rCount * sizeof(TR::Rectangle));

		// triangles 
			for (int j = 0; j < ptr->tCount; j++) {
				auto &f = ((TR::Triangle*)&ptr->triangles)[j];
				auto &t = level.objectTextures[f.texture];

				int  tile = t.tileAndFlag & 0x7FFF;
				int  tx = (tile % 4) * 256;
				int  ty = (tile / 4) * 256;

				int  vIndex = vCount - vStart;

				indices[iCount + 0] = vIndex + 0;
				indices[iCount + 1] = vIndex + 1;
				indices[iCount + 2] = vIndex + 2;

				iCount += 3;

				for (int k = 0; k < 3; k++) {
					auto &v = mVertices[f.vertices[k]];
					vertices[vCount].coord		= { v.x, v.y, v.z };

					if (nCount > 0) {
						TR::Vertex &n = normals[f.vertices[k]];
						vertices[vCount].normal	= { n.x, n.y, n.z, 0 };
						vertices[vCount].color	= { 255, 255, 255, 255 };
					} else {
						uint8 a = 255 - (lights[f.vertices[k]] >> 5);
						vertices[vCount].normal	= { 0, 0, 0, 255 };
						vertices[vCount].color	= { a, a, a, 255 };
					}
					vertices[vCount].texCoord	= { ((tx + t.vertices[k].Xpixel) << 5) + 16, ((ty + t.vertices[k].Ypixel) << 5) + 16};
					vCount++;
				}
			}
			OFFSET(ptr->tCount * sizeof(TR::Triangle));

		// color rectangles
			for (int j = 0; j < ptr->crCount; j++) {
				auto &f = ((TR::Rectangle*)&ptr->crectangles)[j];
				auto &c = level.palette[f.texture & 0xFF];

				int  vIndex = vCount - vStart;

				indices[iCount + 0] = vIndex + 0;
				indices[iCount + 1] = vIndex + 1;
				indices[iCount + 2] = vIndex + 2;

				indices[iCount + 3] = vIndex + 0;
				indices[iCount + 4] = vIndex + 2;
				indices[iCount + 5] = vIndex + 3;

				iCount += 6;

				for (int k = 0; k < 4; k++) {
					auto &v = mVertices[f.vertices[k]];
					
					vertices[vCount].coord		= { v.x, v.y, v.z };

					if (nCount > 0) {
						TR::Vertex &n = normals[f.vertices[k]];
						vertices[vCount].normal	= { n.x, n.y, n.z, 0 };
						vertices[vCount].color	= { c.r, c.g, c.b, 255 };
					} else {
						uint8 a = 255 - (lights[f.vertices[k]] >> 5);
						vertices[vCount].normal	= { 0, 0, 0, 255 };
						vertices[vCount].color	= { a, a, a, 255 };	// TODO: apply color
					}
					vertices[vCount].texCoord	= { 1022 << 5, 1022 << 5 };
					vCount++;
				}
			}
			OFFSET(ptr->crCount * sizeof(TR::Rectangle));

		// color triangles
			for (int j = 0; j < ptr->ctCount; j++) {
				auto &f = ((TR::Triangle*)&ptr->ctriangles)[j];
				auto &c = level.palette[f.texture & 0xFF];

				int  vIndex = vCount - vStart;

				indices[iCount + 0] = vIndex + 0;
				indices[iCount + 1] = vIndex + 1;
				indices[iCount + 2] = vIndex + 2;

				iCount += 3;

				for (int k = 0; k < 3; k++) {
					auto &v = mVertices[f.vertices[k]];
					
					vertices[vCount].coord		= { v.x, v.y, v.z };

					if (nCount > 0) {
						TR::Vertex &n = normals[f.vertices[k]];
						vertices[vCount].normal	= { n.x, n.y, n.z, 0 };
						vertices[vCount].color	= { c.r, c.g, c.b, 255 };
					} else {
						uint8 a = 255 - (lights[f.vertices[k]] >> 5);
						vertices[vCount].normal	= { 0, 0, 0, 255 };
						vertices[vCount].color	= { a, a, a, 255 };	// TODO: apply color
					}
					vertices[vCount].texCoord	= { 1022 << 5, 1022 << 5 };
					vCount++;
				}
			}
			OFFSET(ptr->ctCount * sizeof(TR::Triangle) + sizeof(TR::Mesh));

			ptr = (TR::Mesh*)(((int)ptr + 3) & ~3);

			info.iCount = iCount - info.iStart;
		}

		mesh = new Mesh(indices, iCount, vertices, vCount);
		delete[] indices;
		delete[] vertices;
	}

	TR::StaticMesh* getMeshByID(int id) {
		for (int i = 0; i < level.staticMeshesCount; i++)
			if (level.staticMeshes[i].id == id)
				return &level.staticMeshes[i];
		return NULL;
	}

	//#define SCALE (1.0f / 1024.0f / 2.0f)

	void renderRoom(int index) {
		TR::Room &room = level.rooms[index];

		mat4 m = Core::mModel;
		Core::mModel.translate(vec3(room.info.x, 0.0f, room.info.z));
		shader->setParam(uModel, Core::mModel);
		mesh->render(rangeRooms[index]);
		Core::mModel = m;
	
		Core::color = vec4(1.0f);
		shader->setParam(uColor, Core::color);	
		
		// meshes
		for (int j = 0; j < room.meshesCount; j++) {
			auto rMesh = room.meshes[j];
			auto sMesh = getMeshByID(rMesh.meshID);
			ASSERT(sMesh != NULL);

			mat4 m = Core::mModel;
			Core::mModel.translate(vec3(rMesh.x, rMesh.y, rMesh.z));
//			Core::mModel.rotateY((rMesh.rotation >> 14) * 90.0f * DEG2RAD);
			Core::mModel.rotateY(rMesh.rotation / 16384.0f * PI * 0.5f);

			getLight(vec3(rMesh.x, rMesh.y, rMesh.z), index);

			renderMesh(sMesh->mesh);

			Core::mModel = m;
		}
	/*
		// sprites
		Core::setBlending(bmAlpha);
		for (int j = 0; j < room.data.sCount; j++)
			renderSprite(room, room.data.sprites[j]);
			*/
	}

	
	void renderMesh(uint32 meshOffset) {
		if (!level.meshOffsets[meshOffset] && meshOffset)
			return;

		for (int i = 0; i < mCount; i++)
			if (meshInfo[i].offset == level.meshOffsets[meshOffset]) {
				shader->setParam(uModel, Core::mModel);
				mesh->render(meshInfo[i]);
				break;
			}
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

//		bindTexture(sprite.tile);
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
	
	void renderModel(const TR::Model &model) {		
		TR::Animation *anim = &level.anims[model.animation];

		float fTime = time;

		if (model.id == ENTITY_LARA) {
			anim = lara->anim;
			fTime = lara->fTime;
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

		mat4 m;
		m.identity();
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
			
			mat4 tmp = Core::mModel;
			Core::mModel = Core::mModel * m;
			renderMesh(model.mStart + i);
			Core::mModel = tmp;
		}
	}

	int getLightIndex(const vec3 &pos, int &room) {
		int idx = -1;
		float dist;
		for (int j = 0; j < level.roomsCount; j++)
			for (int i = 0; i < level.rooms[j].lightsCount; i++) {
				TR::Room::Light &light = level.rooms[j].lights[i];
				float d = (pos - vec3(light.x, light.y, light.z)).length();
				if (idx == -1 || d < dist) {
					idx = i;
					dist = d;
					room = j;
				}
			}
		return idx;
	}

	void getLight(const vec3 &pos, int roomIndex) {
		int room;
		int idx = getLightIndex(pos, room);
		if (idx > -1) {
			TR::Room::Light &light = level.rooms[room].lights[idx];
			float c = level.rooms[room].lights[idx].Intensity / 8191.0f;
			Core::lightPos   = vec3(light.x, light.y, light.z);
			Core::lightColor = vec4(c, c, c, light.fade * light.fade);
		} else {
			Core::lightPos   = vec3(0.0f);
			Core::lightColor = vec4(0.0f);
		}
		Core::ambient = vec3(1.0f - level.rooms[roomIndex].ambient / 8191.0f);
		shader->setParam(uAmbient, Core::ambient);	
		shader->setParam(uLightPos, Core::lightPos);	
		shader->setParam(uLightColor, Core::lightColor);
	}

	void renderEntity(const TR::Entity &entity) {
	//	if (!(entity.flags & ENTITY_FLAG_VISIBLE))
	//		return;
			
		mat4 m = Core::mModel;
		Core::mModel.translate(vec3(entity.x, entity.y, entity.z));

		float c = (entity.intensity > -1) ? (1.0f - entity.intensity / (float)0x1FFF) : 1.0f;
		float l = 1.0f;

		Core::color	= vec4(c, c, c, 1.0);
		shader->setParam(uColor, Core::color);
		
		getLight(vec3(entity.x, entity.y, entity.z), entity.room);
				
		for (int i = 0; i < level.modelsCount; i++)
			if (entity.id == level.models[i].id) {
				Core::mModel.rotateY(entity.rotation / 16384.0f * PI * 0.5f);
				renderModel(level.models[i]);
				break;
			}
	/*	
		for (int i = 0; i < level.spriteSequencesCount; i++)
			if (entity.id == level.spriteSequences[i].id) {
				renderSprite(level.spriteTextures[level.spriteSequences[i].sStart]);
				break;
			}
	*/
		Core::mModel = m;
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
	*/
	void debugFloor(const vec3 &f, const vec3 &c, int floorIndex, bool current) {
		vec3 vf[4] = { f, f + vec3(1024, 0, 0), f + vec3(1024, 0, 1024), f + vec3(0, 0, 1024) };
		vec3 vc[4] = { c, c + vec3(1024, 0, 0), c + vec3(1024, 0, 1024), c + vec3(0, 0, 1024) };

		uint16 *d = &level.floors[floorIndex];
		auto cmd = *d;

		do {
			cmd = *d;
			int func = cmd & 0x001F;		// function
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

		if (current)
			glColor3f(1, 1, 1);
		else
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
	
	void debugSectors(int index) {
		TR::Room &room = level.rooms[index];

		vec3 p = (lara->pos - vec3(room.info.x, 0, room.info.z)) / vec3(1024, 1, 1024);

		for (int z = 0; z < room.zSectors; z++)
			for (int x = 0; x < room.xSectors; x++) {
				auto &s = room.sectors[x * room.zSectors + z];
				vec3 f(x * 1024 + room.info.x, s.floor * 256, z * 1024 + room.info.z);
				vec3 c(x * 1024 + room.info.x, s.ceiling * 256, z * 1024 + room.info.z);

				debugFloor(f, c, s.floorIndex, (int)p.x == x && (int)p.z == z);
			}
	}
	
	void debugRooms() {
		Core::setBlending(bmAdd);
		glColor3f(0, 0.25f, 0);
		glDepthMask(GL_FALSE);

		for (int i = 0; i < level.roomsCount; i++) {
			TR::Room &r = level.rooms[i];
			vec3 p = vec3(r.info.x, r.info.yTop, r.info.z);

			if (i == level.entities[lara->entity].room) {//  isInsideRoom(Core::viewPos, rooms[i])) {
				debugSectors(i);
				glColor3f(0, 1, 0);
			} else
				glColor3f(1, 1, 1);
			
			Debug::Draw::box(p, p + vec3(r.xSectors * 1024, r.info.yBottom - r.info.yTop, r.zSectors * 1024));
		}

		glDepthMask(GL_TRUE);
		Core::setBlending(bmAlpha);
	}
	/*
	void debugMeshes() {
		glPushMatrix();
		glScalef(-SCALE, -SCALE, SCALE);
		for (int i = 0; i < meshes.count; i++) {
			renderMesh(meshes[i], vec3(1.0f));
			glTranslatef(-128, 0, 0);
		}
		glPopMatrix();
	}
	*/
	void debugLights() {
		int roomIndex = level.entities[lara->entity].room;
		int lightIndex = getLightIndex(lara->pos, roomIndex);

		glPointSize(8);
		glBegin(GL_POINTS);
		for (int i = 0; i < level.roomsCount; i++)
			for (int j = 0; j < level.rooms[i].lightsCount; j++) {
				TR::Room::Light &l = level.rooms[i].lights[j];
				float a = l.Intensity / 8191.0f;
				vec3 p = vec3(l.x, l.y, l.z);
				vec4 color = vec4(a, a, a, 1);
				Debug::Draw::point(p, color);
				if (i == roomIndex && j == lightIndex)
					color = vec4(0, 1, 0, 1);
				Debug::Draw::sphere(p, l.fade, color);
			}
		glEnd();
	}

	/*
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
		camera.update();

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
		camera.pos = vec3(-lara->pos.x, -lara->pos.y, lara->pos.z) + vec3(0, 1024, -1024);
		camera.setup();;

		shader->bind();
		atlas->bind(0);
		mesh->bind();

		shader->setParam(uViewProj, Core::mViewProj);
		shader->setParam(uModel, Core::mModel);

		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.9f);
		glEnable(GL_DEPTH_TEST);

		Core::setCulling(cfFront);

		Core::mModel.identity();

		Core::color   = vec4(1.0f);
		Core::ambient = vec3(0.0f);

		shader->setParam(uColor, Core::color);	
		shader->setParam(uAmbient, Core::ambient);	

		for (int i = 0; i < level.roomsCount; i++)
			renderRoom(i);

		for (int i = 0; i < level.entitiesCount; i++)
			renderEntity(level.entities[i]);

		Debug::Draw::begin();
		debugRooms();
	//	debugMeshes();
		debugLights();
	//	debugPortals();
	//	debugEntity();
		Debug::Draw::end();
	}

};

#endif
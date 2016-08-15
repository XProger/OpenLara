#ifndef H_GAME_TR
#define H_GAME_TR

#include "core.h"
	
	#define ENTITY_LARA						0
	
	#define ENTITY_ENEMY_TWIN				6
	#define ENTITY_ENEMY_WOLF				7
	#define ENTITY_ENEMY_BEAR				8
	#define ENTITY_ENEMY_BAT				9
	#define ENTITY_ENEMY_CROCODILE_LAND		10
	#define ENTITY_ENEMY_CROCODILE_WATER	11
	#define ENTITY_ENEMY_LION_MALE			12
	#define ENTITY_ENEMY_LION_FEMALE		13
	#define ENTITY_ENEMY_PUMA				14
	#define ENTITY_ENEMY_GORILLA			15
	#define ENTITY_ENEMY_RAT_LAND			16
	#define ENTITY_ENEMY_RAT_WATER			17
	#define ENTITY_ENEMY_REX				18
	#define ENTITY_ENEMY_RAPTOR				19
	#define ENTITY_ENEMY_MUTANT				20

	#define ENTITY_ENEMY_CENTAUR	23
	#define ENTITY_ENEMY_MUMMY		24
	#define ENTITY_ENEMY_LARSON		27

	#define ENTITY_CRYSTAL			83

	#define ENTITY_MEDIKIT_SMALL	93
	#define ENTITY_MEDIKIT_BIG		94

	#define ENTITY_VIEW_TARGET		169

	#define ENTITY_TRAP_FLOOR		35
	#define ENTITY_TRAP_SPIKES		37
	#define	ENTITY_TRAP_STONE		38
	#define ENTITY_TRAP_DART		40

	#define ENTITY_SWITCH			55

	#define ENTITY_GUN_SHOTGUN		85

	#define ENTITY_AMMO_UZI			91
	#define ENTITY_AMMO_SHOTGUN		89
	#define ENTITY_AMMO_MAGNUM		90

	enum LaraState {
		STATE_WALK,
		STATE_RUN,
		STATE_STOP,
		STATE_FORWARD_JUMP,
		STATE_FAST_TURN,
		STATE_FAST_BACK,
		STATE_TURN_RIGHT,
		STATE_TURN_LEFT,
		STATE_DEATH,
		STATE_FAST_FALL,
		STATE_HANG,
		STATE_REACH,
		STATE_SPLAT,
		STATE_TREAD,
		STATE_FAST_TURN_14,
		STATE_COMPRESS,
		STATE_BACK,
		STATE_SWIM,
		STATE_GLIDE,
		STATE_NULL_19,
		STATE_FAST_TURN_20,
		STATE_STEP_RIGHT,
		STATE_STEP_LEFT,
		STATE_ROLL_23,
		STATE_SLIDE,
		STATE_BACK_JUMP,
		STATE_RIGHT_JUMP,
		STATE_LEFT_JUMP,
		STATE_UP_JUMP,
		STATE_FALL_BACK,
		STATE_HANG_LEFT,
		STATE_HANG_RIGHT,
		STATE_SLIDE_BACK,
		STATE_SURF_TREAD,
		STATE_SURF_SWIM,
		STATE_DIVE,
		STATE_PUSH_BLOCK,
		STATE_PULL_BLOCK,
		STATE_PUSH_PULL_READY,
		STATE_PICK_UP,
		STATE_SWITCH_ON,
		STATE_SWITCH_OFF,
		STATE_USE_KEY,
		STATE_USE_PUZZLE,
		STATE_UNDERWATER_DEATH,
		STATE_ROLL_45,
		STATE_SPECIAL,
		STATE_SURF_BACK,
		STATE_SURF_LEFT,
		STATE_SURF_RIGHT,
		STATE_NULL_50,
		STATE_NULL_51,
		STATE_SWAN_DIVE,
		STATE_FAST_DIVE,
		STATE_NULL_54,
		STATE_WATER_OUT,
		STATE_CLIMB_START_AND_STANDING,
		STATE_CLIMB_UP,
		STATE_CLIMB_LEFT,
		STATE_CLIMB_END,
		STATE_CLIMB_RIGHT,
		STATE_CLIMB_DOWN,
		STATE_NULL_62,
		STATE_NULL_63,
		STATE_NULL_64,
		STATE_WADE,
		STATE_WATER_ROLL,
		STATE_PICK_UP_FLARE,
		STATE_NULL_68,
		STATE_NULL_69,
		STATE_DEATH_SLIDE,
		STATE_DUCK,
		STATE_DUCK_72,
		STATE_DASH,
		STATE_DASH_DIVE };

float time;
vec3 laraPos(0.0f);

typedef char			int8;
typedef	short			int16;
typedef int				int32;
typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;

#pragma pack(push, 1)

struct tr_colour {
	uint8 r, g, b;
};

struct tr_colour4 {
	uint8 r, g, b, a;
};

struct tr_vertex {
	int16 x, y, z;
};

struct tr_face4 {
	uint16 vertices[4];
	uint16 texture;		// 15 bit - double-sided
};

struct tr_face3 {
	uint16 vertices[3];
	uint16 texture;
};

struct tr_textile8 {
	uint8 index[256 * 256];
};

struct tr_room_info {
	int32 x, z;
	int32 yBottom, yTop;
};

struct tr_room_portal {
	uint16		roomIndex;
	tr_vertex	normal;
	tr_vertex	vertices[4];
};

struct tr_room_sector {
	uint16	floorIndex;	// Index into FloorData[]
	uint16	boxIndex;	// Index into Boxes[] (-1 if none)
	uint8	roomBelow;	// 255 is none
	int8	floor;		// Absolute height of floor * 256
	uint8	roomAbove;	// 255 if none
	int8	ceiling;	// Absolute height of ceiling * 256
};

struct tr_room_light {
	int32	x, y, z;	// Position of light, in world coordinates
	uint16	Intensity1;	// Light intensity
	uint32	fade;		// Falloff value
};

struct tr_room_vertex {
	tr_vertex	vertex;
	int16		lighting;	// 0 (bright) .. 0x1FFF (dark)
};

struct tr_room_sprite {
	int16 vertex;
	int16 texture;
};

struct tr_room_staticmesh {
	int32 x, y, z;
	uint16 rotation;	// (rotation >> 14) * 90
	uint16 intensity;	// 0 (bright) .. 0x1FFF (dark)
	uint16 meshID;
};

enum tr_romm_flag { FLAG_WATER = 1 };

struct tr_room_data {
	uint32							size;		// Number of data words (uint16_t's)
	int								align;
	Vector<tr_room_vertex, int16>	vertices;
	Vector<tr_face4, int16>			rectangles;
	Vector<tr_face3, int16>			triangles;
	Vector<tr_room_sprite, int16>	sprites;

	tr_room_data(Stream *stream) :
		size(stream->read(size)), 
		align(stream->pos),
		vertices(stream),
		rectangles(stream),
		triangles(stream),
		sprites(stream) { stream->seek((align + size * 2) - stream->pos); }
};

struct tr_room {
	tr_room_info						info;		// Where the room exists, in world coordinates
	tr_room_data						data;		// The room mesh

	Vector<tr_room_portal, uint16>		portals;	// List of visibility portals

	uint16								zSectors;	// ``Width'' of sector list
	uint16								xSectors;	// ``Height'' of sector list
	Vector<tr_room_sector>				sectors;	// List of sectors in this room

	int16								ambient;	// 0 (bright) .. 0x1FFF (dark)

	Vector<tr_room_light, uint16>		lights;	// List of lights

	Vector<tr_room_staticmesh, uint16>	meshes;	// List of static meshes

	int16								alternateRoom;
	int16								flags;		// RoomFlag

	bool								visible;

	tr_room(Stream *stream) : 
		info(stream->read(info)), 
		data(stream),
		portals(stream),
		zSectors(stream->read(zSectors)),
		xSectors(stream->read(xSectors)),
		sectors(stream, (int)zSectors * (int)xSectors),
		ambient(stream->read(ambient)),
		lights(stream),
		meshes(stream),
		alternateRoom(stream->read(alternateRoom)),
		flags(stream->read(flags)) {};
};

struct tr_mesh {
	tr_vertex	center;
	int32		radius;

	Vector<tr_vertex, int16>	vertices;		// List of vertices (relative coordinates)

	int16				nCount;
	Vector<tr_vertex>	normals;
	Vector<int16>		lights;	// nCount < 0 (abs(nCount))

	Vector<tr_face4, int16>	rectangles;		// list of textured rectangles
	Vector<tr_face3, int16>	triangles;		// list of textured triangles
	Vector<tr_face4, int16>	crectangles;	// list of coloured rectangles
	Vector<tr_face3, int16>	ctriangles;		// list of coloured triangles

	tr_mesh(Stream *stream) :
		center(stream->read(center)),
		radius(stream->read(radius)),
		vertices(stream),
		nCount(stream->read(nCount)),
		normals(stream, nCount < 0 ? 0 : nCount),
		lights(stream, nCount > 0 ? 0 : abs(nCount)),
		rectangles(stream),
		triangles(stream),
		crectangles(stream),
		ctriangles(stream) {}
};

struct tr_staticmesh {
	uint32		ID;				// Static Mesh Identifier
	uint16		mesh;			// Mesh (offset into MeshPointers[])
	tr_vertex	vBox[2];
	tr_vertex	cBox[2];
	uint16		Flags;
};

struct tr_entity {
	int16	id;				// Object Identifier (matched in Models[], or SpriteSequences[], as appropriate)
	int16	room;			// which room contains this item
	int32	x, y, z;		// world coords
	int16	rotation;		// ((0xc000 >> 14) * 90) degrees
	int16	intensity;		// (constant lighting; -1 means use mesh lighting)
	uint16	flags;			// 0x0100 indicates "initially invisible", 0x3e00 is Activation Mask
							// 0x3e00 indicates "open" or "activated";  these can be XORed with
							// related FloorData::FDlist fields (e.g. for switches)
};

struct tr_sprite_texture {
	uint16	tile;
	uint8	u, v;
	uint16	w, h;	// (ActualValue  * 256) + 255
	int16	l, t, r, b;
};

struct tr_sprite_sequence {
	int32	id;			// Sprite identifier
	int16	sCount;		// Negative of ``how many sprites are in this sequence''
	int16	sStart;		// Where (in sprite texture list) this sequence starts
};

struct tr_meshtree {
	uint32	flags;
	int32	x, y, z;
};

struct fixed {
	uint16	L;
	int16	H;
	float toFloat() {
		return H + L / 65535.0f;
	}
};

struct tr_animation {
	uint32	frameOffset;	// Byte offset into Frames[] (divide by 2 for Frames[i])
	uint8	frameRate;		// Engine ticks per frame
	uint8	frameSize;		// Number of int16_t's in Frames[] used by this animation

	uint16	state;

	fixed	speed;
	fixed	accel;

	uint16	frameStart;		// First frame in this animation
	uint16	frameEnd;		// Last frame in this animation
	uint16	nextAnimation;
	uint16	nextFrame;

	uint16	scCount;
	uint16	scOffset;		// Offset into StateChanges[]

	uint16	acCount;		// How many of them to use.
	uint16	animCommand;	// Offset into AnimCommand[]
};

struct tr_state_change {
	uint16	state;
	uint16	rCount;			// number of ranges
	uint16	rangeOffset;	// Offset into animRanges[]
};

struct tr_anim_range {
	int16	low;			// Lowest frame that uses this range
	int16	high;			// Highest frame that uses this range
	int16	nextAnimation;	// Animation to dispatch to
	int16	nextFrame;		// Frame offset to dispatch to
};

struct tr_anim_frame {
	int16	minX, minY, minZ;	// Bounding box (low)
	int16	maxX, maxY, maxZ;	// Bounding box (high)
	int16	x, y, z;			// Starting offset for this model
	int16	aCount;
	uint16	angles[0];			// angle frames in YXZ order
};

struct tr_anim_texture {
	int16	tCount;				// Actually, this is the number of texture ID's - 1
	int16	textures[0];		 // offsets into ObjectTextures[], in animation order
};

struct tr_box {
	int32	minZ, maxZ;	// Horizontal dimensions in global units
	int32	minX, maxX;
	int16	floor;		// Height value in global units
	int16	overlap;	// Index into Overlaps[].
};

struct tr_zone   // 12 bytes
{
    uint16 GroundZone1_Normal;
    uint16 GroundZone2_Normal;
    uint16 FlyZone_Normal;
    uint16 GroundZone1_Alternate;
    uint16 GroundZone2_Alternate;
    uint16 FlyZone_Alternate;
};

struct tr_sample_source {
	int32	x, y, z;	// absolute position of sound source (world coordinates)
	uint16	id;			// internal sample index
	uint16	flags;		// 0x40, 0x80, or 0xC0
};

struct tr_sample_info {
   uint16 sample;	// (index into SampleIndices) -- NOT USED IN TR4-5!!!
   uint16 volume;
   uint16 chance;	// If !=0 and ((rand()&0x7fff) > Chance), this sound is not played
   uint16 flags;	// Bits 0-1: Looped flag, bits 2-5: num samples, bits 6-7: UNUSED
};

struct tr_object_texture_vert {
	uint8	Xcoordinate; // 1 if Xpixel is the low value, 255 if Xpixel is the high value in the object texture
	uint8	Xpixel;
	uint8	Ycoordinate; // 1 if Ypixel is the low value, 255 if Ypixel is the high value in the object texture
	uint8	Ypixel;
};

struct tr_object_texture  {
	uint16	attribute;	// 0 - opaque, 1 - transparent, 2 - blend additive
	uint16	tileAndFlag;	// 0..14 - tile, 15 - is triangle
	tr_object_texture_vert	vertices[4]; // The four corners of the texture
};

struct tr_camera {
	int32	x, y, z;
	int16	room;
	uint16	flags;
};

struct tr_cinematic_frame
{
    int16 rotY;    // rotation about Y axis, +/- 32767 == +/- 180 degrees
    int16 rotZ;    // rotation about Z axis, +/- 32767 == +/- 180 degrees
    int16 rotZ2;   // seems to work a lot like rotZ;  I haven't yet been able to
                     // differentiate them
    int16 posZ;    // camera position relative to something (target? Lara? room
                     // origin?).  pos* are _not_ in world coordinates.
    int16 posY;    // camera position relative to something (see posZ)
    int16 posX;    // camera position relative to something (see posZ)
    int16 unknown; // changing this can cause a runtime error
    int16 rotX;    // rotation about X axis, +/- 32767 == +/- 180 degrees
};

struct tr_model {
	uint32 id;			// Item Identifier (matched in Entities[])
	uint16 mCount;		// number of meshes in this object
	uint16 mStart;		// stating mesh (offset into MeshPointers[])
	uint32 mTree;		// offset into MeshTree[]
	uint32 frame;		// byte offset into Frames[] (divide by 2 for Frames[i])
	uint16 animation;	// offset into Animations[]
};

#include <Windows.h>

struct tr_level {
	Texture		**textures;

	uint32						version; // version (4 bytes)
	
	Vector<tr_textile8>			tiles;		// 8-bit (palettized) textiles (NumTextiles * 65536 bytes)

	uint32						unused;	// 32-bit unused value (4 bytes)

	Array<tr_room, uint16>		rooms;
	Vector<uint16>				floors;	// floor data (NumFloorData * 2 bytes)
	int							meshesData;
	Vector<uint16>				meshBuffer;
	Array<tr_mesh>				meshes; // note that NumMeshPointers comes AFTER Meshes[]
	Vector<uint32>				meshPointers; // mesh pointer list (NumMeshPointers * 4 bytes)
	Vector<tr_animation>		animations; // animation list (NumAnimations * 32 bytes)
	Vector<tr_state_change>		stateChanges; // state-change list (NumStructures * 6 bytes)
	Vector<tr_anim_range>		animRanges; // animation-dispatch list list (NumAnimDispatches * 8 bytes)
	Vector<int16>				animCommands; // animation-command list (NumAnimCommands * 2 bytes)
	Vector<uint32>				meshTrees; // MeshTree list (NumMeshTrees * 4*4 bytes)
	Vector<uint16>				frames; // frame data (NumFrames * 2 bytes)
	Vector<tr_model>			models;	// model list (NumModels * 18 bytes)
	Vector<tr_staticmesh>		staticMeshes; // StaticMesh data (NumStaticMesh * 32 bytes)
	Vector<tr_object_texture>	objectTextures; // object texture list (NumObjectTextures * 20 bytes) (after AnimatedTextures in TR3)
	Vector<tr_sprite_texture>	spriteTextures; // sprite texture list (NumSpriteTextures * 16 bytes)
	Vector<tr_sprite_sequence>	spriteSequences; // sprite sequence data (NumSpriteSequences * 8 bytes)
	Vector<tr_camera>			cameras; // camera data (NumCameras * 16 bytes)
	Vector<tr_sample_source>	sampleSources; // sound source data (NumSoundSources * 16 bytes)
	Vector<tr_box>				boxes; // box data (NumBoxes * 20 bytes [TR1 version])
	Vector<uint16>				overlaps; // overlap data (NumOverlaps * 2 bytes)

	Vector<uint16>				groundZone1; // ground zone data [2*NumBoxes]
	Vector<uint16>				groundZone2; // ground zone 2 data [2*NumBoxes]
	Vector<uint16>				flyZone; // fly zone data [2*NumBoxes]

	Vector<uint16>						animatedTextures; // animated texture data (NumAnimatedTextures * 2 bytes)
	Vector<tr_entity>					entities; // item list (NumEntities * 22 bytes [TR1 version])
	Vector<uint8>						lightMap; // light map (8192 bytes)	
	Vector<tr_colour>					palette; // 8-bit palette (768 bytes)
	Vector<tr_cinematic_frame, uint16>	cinematicFrames; // (NumCinematicFrames * 16 bytes)
	Vector<uint8, uint16>				demoData; // demo data (NumDemoData bytes)
	Vector<int16>						samplesMap; // sound map (512 bytes)	
	Vector<tr_sample_info>				samplesInfo; // sound-detail list (NumSoundDetails * 8 bytes)
	Vector<uint8>						samples; // (array of uint8_t's -- embedded sound samples in Microsoft WAVE format)
	Vector<uint32>						sampleIndices; // sample indices (NumSampleIndices * 4 bytes)

	struct Controller {
		tr_level *level;
		tr_animation *anim;
		float fTime;

		vec3	pos;
		float	angle;

		int state;	// LaraState
		int lastFrame;

		Controller(tr_level *level) : level(level), pos(0.0f), angle(0.0f), fTime(0.0f) {
			anim = &level->animations[0];
			lastFrame = 0;
		}

		void update() {
			float rot = 0.0f;
			state = STATE_STOP;
			if (Input::down[ikShift]) {
				if (Input::down[ikUp])		{ state = STATE_WALK; };
				if (Input::down[ikDown])	{ state = STATE_BACK; };
				if (Input::down[ikLeft])	{ if (!Input::down[ikUp] && !Input::down[ikDown]) { state = STATE_STEP_LEFT; }	else rot = -Core::deltaTime * PI; };
				if (Input::down[ikRight])	{ if (!Input::down[ikUp] && !Input::down[ikDown]) { state = STATE_STEP_RIGHT; }	else rot =  Core::deltaTime * PI; };
			} else
				if (Input::down[ikSpace]) {
					if (anim->state == STATE_RUN)
						state = STATE_FORWARD_JUMP;
					else
						if (Input::down[ikUp]) 
							state = anim->state != STATE_COMPRESS ? STATE_COMPRESS : STATE_FORWARD_JUMP;
						else
							if (Input::down[ikDown]) 
								state = anim->state != STATE_COMPRESS ? STATE_COMPRESS : STATE_BACK_JUMP;
							else 
								if (Input::down[ikLeft])
									state = anim->state != STATE_COMPRESS ? STATE_COMPRESS : STATE_LEFT_JUMP;
								else
									if (Input::down[ikRight])
										state = anim->state != STATE_COMPRESS ? STATE_COMPRESS : STATE_RIGHT_JUMP;
									else
										state = STATE_UP_JUMP;
				} else {
					if (Input::down[ikUp])		{ state = STATE_RUN; };
					if (Input::down[ikDown])	{ state = STATE_FAST_BACK; };
					if (Input::down[ikLeft])	{ if (!Input::down[ikUp] && !Input::down[ikDown]) state = STATE_TURN_LEFT;	rot = -Core::deltaTime * PI; };
					if (Input::down[ikRight])	{ if (!Input::down[ikUp] && !Input::down[ikDown]) state = STATE_TURN_RIGHT;	rot =  Core::deltaTime * PI; };				
				}

			if (Input::down[ikEnter])
				state = STATE_COMPRESS;


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
					auto &sc = level->stateChanges[anim->scOffset + i];
					if (sc.state == state) {
						for (int j = 0; j < sc.rCount; j++) {
							auto &range = level->animRanges[sc.rangeOffset + j];
							if ( anim->frameStart + fIndex >= range.low && anim->frameStart + fIndex <= range.high) {
								int st = anim->state;
								anim  = &level->animations[range.nextAnimation];
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
				anim = &level->animations[anim->nextAnimation];
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

			int16 *ptr = &level->animCommands[anim->animCommand];

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
							auto a = level->samplesMap[id];
							auto b = level->samplesInfo[a].sample;
							auto c = level->sampleIndices[b];

							void *p = &level->samples[c];

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
				case STATE_BACK :
				case STATE_BACK_JUMP :
				case STATE_FAST_BACK :
					d = PI;
					break;
				case STATE_STEP_LEFT :
				case STATE_LEFT_JUMP :
					d = -PI * 0.5f;
					break;
				case STATE_STEP_RIGHT :
				case STATE_RIGHT_JUMP :
					d =  PI * 0.5f;
					break;
			}
			d += angle;

			float speed = anim->speed.toFloat() + anim->accel.toFloat() * (fTime * 30.0f);			
			pos = pos + vec3(sinf(d), 0, cosf(d)) * (speed * Core::deltaTime * 30.0f);


			lastFrame = fIndex;
		}

	} *lara;

	tr_level(Stream *stream) :
				version(stream->read(version)),
				tiles(stream),
				unused(stream->read(unused)),
				rooms(stream),
				floors(stream),
				meshesData(stream->pos + 4),
				meshBuffer(stream),
				meshPointers(stream),
				animations(stream),
				stateChanges(stream),
				animRanges(stream),
				animCommands(stream),
				meshTrees(stream),
				frames(stream),
				models(stream),
				staticMeshes(stream),
				objectTextures(stream),
				spriteTextures(stream),
				spriteSequences(stream),
				cameras(stream),
				sampleSources(stream),
				boxes(stream),
				overlaps(stream),
				groundZone1(stream, boxes.count * 2),
				groundZone2(stream, boxes.count * 2),
				flyZone(stream, boxes.count * 2),
				animatedTextures(stream),
				entities(stream),
				lightMap(stream, 32 * 256),
				palette(stream, 256),
				cinematicFrames(stream),
				demoData(stream),
				samplesMap(stream, 256),
				samplesInfo(stream),
				samples(stream),
				sampleIndices(stream) {
	
		int pos = stream->pos;

		meshes = Array<tr_mesh>();
		meshes.count = meshPointers.count;
		meshes.items = new tr_mesh*[meshes.count];

		for (int i = 0; i < meshPointers.count; i++) {
			stream->setPos(meshesData + meshPointers[i]);
			meshes.items[i] = new tr_mesh(stream);
		}

		// God bless Amiga
		int m = 0;
		for (int i = 0; i < palette.count; i++) {
			tr_colour &c = palette[i];
			c.r <<= 2;
			c.g <<= 2;
			c.b <<= 2;
		}

		if (tiles.count) {
			textures = new Texture*[tiles.count];

			for (int i = 0; i < tiles.count; i++) {
			//	sprintf(buf, "LEVEL1_%d.PVR", i);
			//	textures[i] = Core::load<Texture>(buf);
				textures[i] = getTexture(i);
			}
		} else 
			textures = NULL;

		lara = new Controller(this);
	}
	
	~tr_level() {
		delete lara;

		for (int i = 0; i < tiles.count; i++)
			delete textures[i];
		delete[] textures;
	}

	Texture *getTexture(int tile) {
		tr_colour4 data[256 * 256];
		for (int i = 0; i < 256 * 256; i++) {
			int index = tiles[tile].index[i];
			auto p = palette[index];
			data[i].r = p.r;
			data[i].g = p.g;
			data[i].b = p.b;
			data[i].a = index == 0 ? 0 : 255;
		}
		return new Texture(256, 256, 0, data);
	}

	#define SCALE (1.0f / 1024.0f / 2.0f)

	void renderRoom(tr_room *room) {
		glPushMatrix();
		glScalef(-SCALE, -SCALE, SCALE);
		glTranslatef(room->info.x, 0.0f, room->info.z);

		// rectangles
		for (int j = 0; j < room->data.rectangles.count; j++) {
			auto &f = room->data.rectangles[j];
			auto &t = objectTextures[f.texture];
			setTexture(f.texture);

			glBegin(GL_QUADS);
			for (int k = 0; k < 4; k++) {
				auto &v = room->data.vertices[f.vertices[k]];
				float a = 1.0f - v.lighting / 8191.0f;
				glColor3f(a, a, a);
				glTexCoord2f(t.vertices[k].Xpixel / 256.0f, t.vertices[k].Ypixel / 256.0f);
				glVertex3f(v.vertex.x, v.vertex.y, v.vertex.z);
			}
			glEnd();
		}
		
		// triangles
		for (int j = 0; j < room->data.triangles.count; j++) {
			auto &f = room->data.triangles[j];
			auto &t = objectTextures[f.texture];
			setTexture(f.texture);

			glBegin(GL_TRIANGLES);	
			for (int k = 0; k < 3; k++) {
				auto &v = room->data.vertices[f.vertices[k]];
				float a = 1.0f - v.lighting / 8191.0f;
				glColor3f(a, a, a);
				glTexCoord2f(t.vertices[k].Xpixel / 256.0f, t.vertices[k].Ypixel / 256.0f);
				glVertex3f(v.vertex.x, v.vertex.y, v.vertex.z);
			}
			glEnd();
		}
		glPopMatrix();

		// meshes
		float a = 1.0f - room->ambient / 8191.0f;

		for (int j = 0; j < room-> meshes.count; j++) {
			auto rMesh = room->meshes[j];
			auto sMesh = getMeshByID(rMesh.meshID);
			ASSERT(sMesh != NULL);

			glPushMatrix();
			glScalef(-SCALE, -SCALE, SCALE);
			glTranslatef(rMesh.x, rMesh.y, rMesh.z);
			glRotatef((rMesh.rotation >> 14) * 90.0f, 0, 1, 0);
			
			renderMesh(meshes[sMesh->mesh], vec3(a));

			glPopMatrix();
		}

		// sprites
		Core::setBlending(bmAlpha);
		for (int j = 0; j < room->data.sprites.count; j++)
			renderSprite(room, &room->data.sprites[j]);
	}

	void renderMesh(tr_mesh *mesh, const vec3 &color) {
		if (mesh->nCount > 0)
			glEnable(GL_LIGHTING);
		glColor3f(color.x, color.y, color.z);

		// triangles
		for (int j = 0; j < mesh->triangles.count; j++) {
			auto &f = mesh->triangles[j];
			auto &t = objectTextures[f.texture];
			setTexture(f.texture);
				
			glBegin(GL_TRIANGLES);	
			for (int k = 0; k < 3; k++) {
				auto &v = mesh->vertices[f.vertices[k]];

				if (mesh->nCount > 0) {
					auto vn = mesh->normals[f.vertices[k]];
					vec3 n = vec3(vn.x, vn.y, vn.z).normal();
					glNormal3f(n.x, n.y, n.z);
				} else {
					auto l = mesh->lights[f.vertices[k]];
					float a = 1.0f - l / 8191.0f;
					glColor3f(a, a, a);
				}

				glTexCoord2f(t.vertices[k].Xpixel / 256.0f, t.vertices[k].Ypixel / 256.0f);
				glVertex3f(v.x, v.y, v.z);
			}
			glEnd();
		}

		// rectangles
		for (int j = 0; j < mesh->rectangles.count; j++) {
			auto &f = mesh->rectangles[j];
			auto &t = objectTextures[f.texture];
			setTexture(f.texture);
				
			glBegin(GL_QUADS);	
			for (int k = 0; k < 4; k++) {
				auto &v = mesh->vertices[f.vertices[k]];

				if (mesh->nCount > 0) {
					auto vn = mesh->normals[f.vertices[k]];
					vec3 n = vec3(vn.x, vn.y, vn.z).normal();
					glNormal3f(n.x, n.y, n.z);
				} else {
					auto l = mesh->lights[f.vertices[k]];
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
		/*
		if (mesh->nCount > 0) {
			glDisable(GL_LIGHTING);
			glColor3f(0, 0, 1);
			glBegin(GL_LINES);
			for (int j = 0; j < mesh->triangles.count; j++) {
				auto &f = mesh->triangles[j];
				for (int k = 0; k < 3; k++) {
					auto &v = mesh->vertices[f.vertices[k]];
					auto vn = mesh->normals[f.vertices[k]];
					vec3 n = vec3(vn.x, vn.y, vn.z).normal() * 100.0f;
					glVertex3f(v.x, v.y, v.z);
					glVertex3f(v.x + n.x, v.y + n.y, v.z + n.z);
	
				}
			}
			glEnd();
			glEnable(GL_LIGHTING);
		}
		*/

		// triangles (colored)
		for (int j = 0; j < mesh->ctriangles.count; j++) {
			auto &f = mesh->ctriangles[j];
			auto &c = palette[f.texture & 0xFF];
				
			glBegin(GL_TRIANGLES);	
			for (int k = 0; k < 3; k++) {
				auto &v = mesh->vertices[f.vertices[k]];

				if (mesh->nCount > 0) {
					auto vn = mesh->normals[f.vertices[k]];
					vec3 n = vec3(vn.x, vn.y, vn.z).normal();
					glColor3f(c.r / 255.0f * color.x, c.g / 255.0f * color.y, c.b / 255.0f * color.z);
					glNormal3f(n.x, n.y, n.z);
				} else {
					auto l = mesh->lights[f.vertices[k]];
					float a = (1.0f - l / 8191.0f) / 255.0f;
					glColor3f(c.r * a, c.g * a, c.b * a);
				}
				glVertex3f(v.x, v.y, v.z);
			}
			glEnd();
		}
		// rectangles (colored)
		for (int j = 0; j < mesh->crectangles.count; j++) {
			auto &f = mesh->crectangles[j];
			auto &c = palette[f.texture & 0xFF];
				
			glBegin(GL_QUADS);	
			for (int k = 0; k < 4; k++) {
				auto &v = mesh->vertices[f.vertices[k]];

				if (mesh->nCount > 0) {
					auto vn = mesh->normals[f.vertices[k]];
					vec3 n = vec3(vn.x, vn.y, vn.z).normal();
					glColor3f(c.r / 255.0f * color.x, c.g / 255.0f * color.y, c.b / 255.0f * color.z);
					glNormal3f(n.x, n.y, n.z);
				} else {
					auto l = mesh->lights[f.vertices[k]];
					float a = (1.0f - l / 8191.0f) / 255.0f;
					glColor3f(c.r * a, c.g * a, c.b * a);
				}
				glVertex3f(v.x, v.y, v.z);
			}
			glEnd();
		}
		glEnable(GL_TEXTURE_2D);

		if (mesh->nCount > 0)
			glDisable(GL_LIGHTING);
	}

	void renderSprite(tr_sprite_texture *sprite) {
		float u0 = sprite->u / 256.0f;
		float v0 = sprite->v / 255.0f;
		float u1 = u0 + sprite->w / (256.0f * 256.0f);
		float v1 = v0 + sprite->h / (256.0f * 256.0f);	

		mat4 m = Core::mView.inverse();
		vec3 up = m.up.xyz * vec3(-1, -1, 1) * (-1);
		vec3 right = m.right.xyz * vec3(-1, -1, 1);

		vec3 p[4];
		p[0] = right * sprite->r + up * sprite->b;
		p[1] = right * sprite->l + up * sprite->b;
		p[2] = right * sprite->l + up * sprite->t;
		p[3] = right * sprite->r + up * sprite->t;

		textures[sprite->tile]->bind(0);
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

	void renderSprite(tr_room *room, tr_room_sprite *sprite) {
		auto &v = room->data.vertices[sprite->vertex];
		float a = 1.0f - v.lighting / (float)0x1FFF;
		glColor3f(a, a, a);

		glPushMatrix();
		glScalef(-SCALE, -SCALE, SCALE);
		glTranslatef(v.vertex.x + room->info.x, v.vertex.y, v.vertex.z + room->info.z);

		renderSprite(&spriteTextures[sprite->texture]);

		glPopMatrix();
	}

	vec3 getAngle(tr_anim_frame *frame, int index) {
		#define ANGLE_SCALE (2.0f * PI / 1024.0f)

		uint16 b = frame->angles[index * 2 + 0];
		uint16 a = frame->angles[index * 2 + 1];

//		vec3 k(1.0f);
		/*
		if (a & 0xC000)
			k = vec3::Z;
		else
			if (a & 0x8000)
				k = vec3::Y;
			else
				if (a & 0x4000)
					k = vec3::X;
		*/
//		k = k * ANGLE_SCALE;

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
	/*
		return vec3(lerpAngle(a.x, b.x, t),
					lerpAngle(a.y, b.y, t),
					lerpAngle(a.z, b.z, t));
	*/
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

	void renderModel(tr_model *model) {
		mat4 m;
		m.identity();
		
		tr_animation *anim = &animations[model->animation];

		float fTime = time;

		if (model->id == ENTITY_LARA) {
			anim = lara->anim;
			fTime = lara->fTime;
			m.translate(lara->pos);
			m.rotateY(lara->angle);
		}

		float k = fTime * 30.0f / anim->frameRate;
		int fIndex = (int)k; 
		int fCount = (anim->frameEnd - anim->frameStart) / anim->frameRate + 1;

		int fSize = sizeof(tr_anim_frame) + model->mCount * sizeof(uint16) * 2;
		k = k - fIndex;


		int fIndexA = fIndex % fCount, fIndexB = (fIndex + 1) % fCount;
		tr_anim_frame *frameA = (tr_anim_frame*)&frames[(anim->frameOffset + fIndexA * fSize) >> 1];
			
		tr_animation *nextAnim = NULL;
	
	   	if (fIndexB == 0) {
			nextAnim = &animations[anim->nextAnimation];
			fIndexB = (anim->nextFrame - nextAnim->frameStart) / anim->frameRate;
		} else
			nextAnim = anim;

//		LOG("%d %f\n", fIndexA, fTime);


		tr_anim_frame *frameB = (tr_anim_frame*)&frames[(nextAnim->frameOffset + fIndexB * fSize) >> 1];
/*
		anim = &animations[0];

		if (Input::down[ikO])
			debugTime += 2.0f * Core::deltaTime;
		fCount = (anim->frameEnd - anim->frameStart) / anim->frameRate + 1;

		fIndex = ((int)debugTime) % fCount;


		LOG("%f %d / %d (%d, %d)\n", debugTime, fIndex, fCount, int(anim->frameEnd - anim->frameStart), int(anim->frameRate) );


		frameA = frameB = (tr_anim_frame*)&frames[(anim->frameOffset + fIndex * fSize) >> 1];
		k = 0.0f;
*/

		/*

		float k = fTime * 30.0f / anim->frameRate;
		
		int fCount = (anim->frameEnd - anim->frameStart + 1);// / anim->frameRate;	
		ASSERT(fCount % anim->frameRate == 0);
		fCount /= anim->frameRate;
		
		int fIndex = int(k);
		k = k - (int)k;
*/
//		ASSERT(fpSize == fSize);
//		fSize = fpSize;

	//	LOG("%d\n", fIndex % fCount);
		//if (fCount > 1) LOG("%d %d\n", model->id, fCount);
	//	LOG("%d\n", fIndex % fCount);


//		Debug::Draw::box(Box(vec3(frameA->minX, frameA->minY, frameA->minZ), vec3(frameA->maxX, frameA->maxY, frameA->maxZ)));

		tr_meshtree *tree = (int)model->mTree < meshTrees.count ? (tr_meshtree*)&meshTrees[model->mTree] : NULL;

		int sIndex = 0;
		mat4 stack[20];

		m.translate(vec3(frameA->x, frameA->y, frameA->z).lerp(vec3(frameB->x, frameB->y, frameB->z), k));

		for (int i = 0; i < model->mCount; i++) {

			if (i > 0 && tree) {
				tr_meshtree &t = tree[i - 1];

				if (t.flags & 0x01) m = stack[--sIndex];
				if (t.flags & 0x02) stack[sIndex++] = m;

				ASSERT(sIndex >= 0);
				ASSERT(sIndex < 20);

				m.translate(vec3(t.x, t.y, t.z));
			}
			
			quat q = lerpAngle(getAngle(frameA, i), getAngle(frameB, i), k);
			m = m * mat4(q, vec3(0.0f));
			
			/*	
			vec3 angle = lerpAngle(getAngle(frameA, i), getAngle(frameB, i), k);
			m.rotateY(angle.y);
			m.rotateX(angle.x);
			m.rotateZ(angle.z);
			*/

			glPushMatrix();
			glMultMatrixf((GLfloat*)&m);
			renderMesh(meshes[model->mStart + i], vec3(1.0f));
			glPopMatrix();
		}
	}

	void renderEntity(tr_entity *entity) {
		glPushMatrix();
		glScalef(-SCALE, -SCALE, SCALE);
		glTranslatef(entity->x, entity->y, entity->z);

		if (entity->intensity > -1) {
			float a = 1.0f - entity->intensity / (float)0x1FFF;
			glColor3f(a, a, a);
		} else
			glColor3f(1, 1, 1);
	

		for (int i = 0; i < models.count; i++)
			if (entity->id == models[i].id) {
				glRotatef((entity->rotation >> 14) * 90.0f, 0, 1, 0);
				renderModel(&models[i]);
				break;
			}
		
		for (int i = 0; i < spriteSequences.count; i++)
			if (entity->id == spriteSequences[i].id) {
				renderSprite(&spriteTextures[spriteSequences[i].sStart]);
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

	tr_staticmesh* getMeshByID(int ID) {
		for (int i = 0; i < staticMeshes.count; i++)
			if (staticMeshes[i].ID == ID)
				return &staticMeshes[i];
		return NULL;
	}

	void setTexture(int objTexture) {
		auto &t = objectTextures[objTexture];
		Core::setBlending(t.attribute == 2 ? bmAdd : bmAlpha);
		textures[t.tileAndFlag & 0x7FFF]->bind(0);
	}

	float tickTextureAnimation = 0.0f;

	void update() {
		lara->update();

		if (tickTextureAnimation > 0.25f) {
			tickTextureAnimation = 0.0f;

			if (animatedTextures.count) {
				uint16 *ptr = &animatedTextures[0];
				int count = *ptr++;
				for (int i = 0; i < count; i++) {
					auto animTex = (tr_anim_texture*)ptr;
					auto id = objectTextures[animTex->textures[0]];
					for (int j = 0; j < animTex->tCount; j++) // tCount = count of textures in animation group - 1 (!!!)
						objectTextures[animTex->textures[j]] = objectTextures[animTex->textures[j + 1]];
					objectTextures[animTex->textures[animTex->tCount]] = id;
					ptr += (sizeof(tr_anim_texture) + sizeof(animTex->textures[0]) * (animTex->tCount + 1)) / sizeof(uint16);
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

		for (int i = 0; i < rooms.count; i++)
			renderRoom(rooms[i]);

		for (int i = 0; i < entities.count; i++)
			renderEntity(&entities[i]);

	//	debugRooms();
	//	debugMeshes();
	//	debugLights();
	//	debugPortals();
	//	debugEntity();
	}
};

#pragma pack(pop)

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

		pos = pos + v.normal() * (Core::deltaTime);

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

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf((GLfloat*)&Core::mProj);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((GLfloat*)&Core::mView);
	}
};

namespace Game {
	tr_level	*level;
	Camera		camera;

	void init() {
		Core::init();

		Stream stream("data\\LEVEL3A.PHD");
	//	Stream stream("GYM.PHD");
		level = new tr_level(&stream);

		camera.fov		= 90.0f;
		camera.znear	= 0.1f;
		camera.zfar		= 1000.0f;
//		camera.pos		= vec3(-10, -2, 26);
		camera.pos		= vec3(-13.25, 0.42, 38.06);
//		camera.pos		= vec3(-36, -1, 2);
		camera.angle	= vec3(0);
	}

	void free() {
		delete level;

		Core::free();
	}

	void update() {
		camera.update();
		time += Core::deltaTime;

		level->update();
	}

	void render() {
//		Core::clear(vec4(0.1f, 0.4f, 0.4f, 0.0));
		Core::clear(vec4(0.0f, 0.0f, 0.0f, 0.0));
		Core::setViewport(0, 0, Core::width, Core::height);
		Core::setBlending(bmAlpha);

		camera.setup();		
		/*
		Debug::Draw::begin();
		glDisable(GL_TEXTURE_2D);
		Debug::Draw::axes(10);
		glEnable(GL_TEXTURE_2D);
		*/
		vec3 pos = Core::mView.inverse().getPos();
		GLfloat p[4] = { pos.x, pos.y, pos.z, 1.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, p);
		glEnable(GL_DEPTH_TEST);

		level->render();

		//Debug::Draw::end();
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
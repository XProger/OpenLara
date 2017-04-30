#ifndef H_DEBUG
#define H_DEBUG

#include "core.h"
#include "format.h"
#include "controller.h"
#include "mesh.h"

namespace Debug {

    static GLuint font;

    void init() {
        font = glGenLists(256);
        HDC hdc = GetDC(0); 
        HFONT hfont = CreateFontA(-MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0,
                                 0, 0, FW_BOLD, 0, 0, 0,
                                 ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 ANTIALIASED_QUALITY, DEFAULT_PITCH, "Courier New");
        SelectObject(hdc, hfont);
        wglUseFontBitmaps(hdc, 0, 256, font);
        DeleteObject(hfont);
    }

    void free() {
        glDeleteLists(font, 256);
    }

    void begin() {
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf((GLfloat*)&Core::mProj);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glLoadMatrixf((GLfloat*)&Core::mView);

        glLineWidth(3);
        glPointSize(32);

        glUseProgram(0);
        Core::active.shader = NULL;
        Core::active.textures[0] = NULL;
    }

    void end() {
        //
    }

    namespace Draw {

        void box(const vec3 &min, const vec3 &max, const vec4 &color) {
            glColor4fv((GLfloat*)&color);
            glBegin(GL_LINES);
                glVertex3f(min.x, min.y, min.z);
                glVertex3f(max.x, min.y, min.z);
                glVertex3f(min.x, max.y, min.z);
                glVertex3f(max.x, max.y, min.z);

                glVertex3f(min.x, min.y, max.z);
                glVertex3f(max.x, min.y, max.z);
                glVertex3f(min.x, max.y, max.z);
                glVertex3f(max.x, max.y, max.z);

                glVertex3f(min.x, min.y, min.z);
                glVertex3f(min.x, min.y, max.z);
                glVertex3f(min.x, max.y, min.z);
                glVertex3f(min.x, max.y, max.z);

                glVertex3f(max.x, min.y, min.z);
                glVertex3f(max.x, min.y, max.z);
                glVertex3f(max.x, max.y, min.z);
                glVertex3f(max.x, max.y, max.z);

                glVertex3f(min.x, min.y, min.z);
                glVertex3f(min.x, max.y, min.z);

                glVertex3f(max.x, min.y, min.z);
                glVertex3f(max.x, max.y, min.z);
                glVertex3f(min.x, min.y, min.z);
                glVertex3f(min.x, max.y, min.z);

                glVertex3f(max.x, min.y, max.z);
                glVertex3f(max.x, max.y, max.z);
                glVertex3f(min.x, min.y, max.z);
                glVertex3f(min.x, max.y, max.z);
            glEnd();
        }

        void box(const mat4 &m, const vec3 &min, const vec3 &max, const vec4 &color) {
            glPushMatrix();
            glMultMatrixf((GLfloat*)&m);
            box(min, max, color);
            glPopMatrix();
        }

        void sphere(const vec3 &center, const float radius, const vec4 &color) {
            const float k = PI * 2.0f / 18.0f;

            glColor4fv((GLfloat*)&color);
            for (int j = 0; j < 3; j++) {
                glBegin(GL_LINE_STRIP);
                for (int i = 0; i < 19; i++) {
                    vec3 p = vec3(sinf(i * k), cosf(i * k), 0.0f) * radius;
                    glVertex3f(p[j] + center.x, p[(j + 1) % 3] + center.y, p[(j + 2) % 3] + center.z);
                }
                glEnd();
            }
        }

        void mesh(vec3 *vertices, Index *indices, int iCount) {
            glBegin(GL_LINES);
            for (int i = 0; i < iCount; i += 3) {
                vec3 &a = vertices[indices[i + 0]];
                vec3 &b = vertices[indices[i + 1]];
                vec3 &c = vertices[indices[i + 2]];
                glVertex3fv((GLfloat*)&a);
                glVertex3fv((GLfloat*)&b);

                glVertex3fv((GLfloat*)&b);
                glVertex3fv((GLfloat*)&c);

                glVertex3fv((GLfloat*)&c);
                glVertex3fv((GLfloat*)&a);
            }
            glEnd();
        }

        void axes(float size) {
            glBegin(GL_LINES);
                glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(size,    0,    0);
                glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(   0, size,    0);
                glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(   0,    0, size);
            glEnd();
        }

        void point(const vec3 &p, const vec4 &color) {
            glColor4fv((GLfloat*)&color);
            glBegin(GL_POINTS);
                glVertex3fv((GLfloat*)&p);
            glEnd();
        }

        void line(const vec3 &a, const vec3 &b, const vec4 &color) {
            glBegin(GL_LINES);                
                glColor4fv((GLfloat*)&color);
                glVertex3fv((GLfloat*)&a);
                glVertex3fv((GLfloat*)&b);
            glEnd();
        }

        void text(const vec2 &pos, const vec4 &color, const char *str) {
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();

            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            glOrtho(0, Core::width, Core::height, 0, 0, 1);

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glDisable(GL_TEXTURE_2D);
            glColor4fv((GLfloat*)&color);
            glRasterPos2f(pos.x, pos.y);
            glListBase(font);
            glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);

            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        }

        void text(const vec3 &pos, const vec4 &color, const char *str) {
			vec4 p = Core::mViewProj * vec4(pos, 1);
			if (p.w > 0) {
				p.xyz = p.xyz * (1.0f / p.w);
				p.y = -p.y;	
				p.xyz = (p.xyz * 0.5f + vec3(0.5f)) * vec3(Core::width, Core::height, 1.0f);	
                text(vec2(p.x, p.y), color, str);
			}
        }
    }

    namespace Level {

        void debugFloor(const TR::Level &level, int roomIndex, int x, int y, int z, int zone = -1) {            
            if (zone != -1) {
                int dx, dz;
                TR::Room::Sector &s = level.getSector(roomIndex, x, z, dx, dz);
                if (zone != level.zones[0].ground1[s.boxIndex])
                    return;                
            }
            
            TR::Level::FloorInfo info;

            vec3 rf[4], rc[4], f[4], c[4];

            int offsets[4][2] = { { 1, 1 }, { 1023, 1 }, { 1023, 1023 }, { 1, 1023 } };

            for (int i = 0; i < 4; i++) {
                level.getFloorInfo(roomIndex, x + offsets[i][0], y, z + offsets[i][1], info);
                rf[i] = vec3( x + offsets[i][0], info.roomFloor - 4,   z + offsets[i][1] );
                rc[i] = vec3( x + offsets[i][0], info.roomCeiling + 4, z + offsets[i][1] );
                f[i]  = vec3( x + offsets[i][0], info.floor - 4,       z + offsets[i][1] );
                c[i]  = vec3( x + offsets[i][0], info.ceiling + 4,     z + offsets[i][1] );
                if (info.roomBelow == 0xFF) rf[i].y = f[i].y;
                if (info.roomAbove == 0xFF) rc[i].y = c[i].y;
            }

            if (info.roomNext != 0xFF) {
                glColor4f(0.0f, 0.0f, 1.0f, 0.1f);
                glBegin(GL_QUADS);
                    glVertex3fv((GLfloat*)&f[3]);
                    glVertex3fv((GLfloat*)&f[2]);
                    glVertex3fv((GLfloat*)&f[1]);
                    glVertex3fv((GLfloat*)&f[0]);
                glEnd();
            } else {

                glColor4f(0.0f, 1.0f, 0.0f, 0.1f);
                glBegin(GL_QUADS);
                    glVertex3fv((GLfloat*)&f[3]);
                    glVertex3fv((GLfloat*)&f[2]);
                    glVertex3fv((GLfloat*)&f[1]);
                    glVertex3fv((GLfloat*)&f[0]);
                glEnd();

                if (info.trigCmdCount > 0)
                    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
                else
                    glColor4f(0.0f, 1.0f, 0.0f, 0.25f);
                glBegin(GL_LINE_STRIP);
                    for (int i = 0; i < 5; i++)
                        glVertex3fv((GLfloat*)&rf[i % 4]);
                glEnd();
            }
                
            glColor4f(1.0f, 0.0f, 0.0f, 0.1f);
            glBegin(GL_QUADS);
                glVertex3fv((GLfloat*)&c[0]);
                glVertex3fv((GLfloat*)&c[1]);
                glVertex3fv((GLfloat*)&c[2]);
                glVertex3fv((GLfloat*)&c[3]);
            glEnd();

            if (info.trigCmdCount > 0)
                glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
            else
                glColor4f(1.0f, 0.0f, 0.0f, 0.25f);
            glBegin(GL_LINE_STRIP);
                for (int i = 0; i < 5; i++)
                    glVertex3fv((GLfloat*)&rc[i % 4]);  
            glEnd();


            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

            /*
            if (boxIndex == 0xFFFF) {
                glBegin(GL_LINES);
                    float x = f.x + 512.0f, z = f.z + 512.0f;
                    glVertex3f(x, f.y, z);
                    glVertex3f(x, c.y, z);
                glEnd();                
            }*/
        }

        void debugBox(const TR::Box &b) {
            glBegin(GL_QUADS);
                float y = b.floor - 16.0f;
                glVertex3f(b.minX, y, b.maxZ);
                glVertex3f(b.maxX, y, b.maxZ);
                glVertex3f(b.maxX, y, b.minZ);
                glVertex3f(b.minX, y, b.minZ);
            glEnd();
        }

        void blocks(const TR::Level &level) {
            Core::setDepthTest(false);
            char buf[64];

            for (int j = 0; j < level.roomsCount; j++) {
                TR::Room &r = level.rooms[j];

                for (int z = 0; z < r.zSectors; z++)
                    for (int x = 0; x < r.xSectors; x++) {
                        TR::Room::Sector &s = r.sectors[x * r.zSectors + z];
                        if (s.boxIndex != 0xFFFF) {
                            bool blockable = level.boxes[s.boxIndex].overlap.value & 0x8000;
                            bool block     = level.boxes[s.boxIndex].overlap.value & 0x4000;
                            int  floor     = level.boxes[s.boxIndex].floor;

                            if (blockable || block) {                                
                                sprintf(buf, "blocked: %s", block ? "true" : "false");
                                Debug::Draw::text(vec3(r.info.x + x * 1024 + 512, floor, r.info.z + z * 1024 + 512), vec4(1, 1, 0, 1), buf);
                            }
                        }
                    }
            }

            Core::setDepthTest(true);
        }

        void debugOverlaps(const TR::Level &level, int boxIndex) {
            glColor4f(1.0f, 1.0f, 0.0f, 0.25f);
            TR::Overlap *o = &level.overlaps[level.boxes[boxIndex].overlap.index];
            do {
                TR::Box &b = level.boxes[o->boxIndex];
                debugBox(b);
            } while (!(o++)->end);
        }

        void sectors(const TR::Level &level, int roomIndex, int y, int zone = -1) {
            TR::Room &room = level.rooms[roomIndex];

            for (int z = 0; z < room.zSectors; z++)
                for (int x = 0; x < room.xSectors; x++)
                    debugFloor(level, roomIndex, room.info.x + x * 1024, y, room.info.z + z * 1024, zone);
        }

        void rooms(const TR::Level &level, const vec3 &pos, int roomIndex) {
            glDepthMask(GL_FALSE);

            for (int i = 0; i < level.roomsCount; i++) {
                TR::Room &r = level.rooms[i];
                vec3 p = vec3(r.info.x, r.info.yTop, r.info.z);

                if (i == roomIndex) {
                //if (lara->insideRoom(Core::viewPos, i)) {
                   // sectors(level, i);
                    glColor3f(0, 1, 0);
                } else
                    glColor3f(1, 1, 1);

            //    Debug::Draw::box(p, p + vec3(r.xSectors * 1024, r.info.yBottom - r.info.yTop, r.zSectors * 1024));
            }

            glDepthMask(GL_TRUE);
        }

        void portals(const TR::Level &level) {
            Core::setBlending(bmAdd);
            glColor3f(0, 0.25f, 0.25f);
            glDepthMask(GL_FALSE);

            glBegin(GL_QUADS);
            for (int i = 0; i < level.roomsCount; i++) {
            //    if (level.entities[91].room != i) continue;
                TR::Room &r = level.rooms[i];
                for (int j = 0; j < r.portalsCount; j++) {
                    TR::Room::Portal &p = r.portals[j];
                    for (int k = 0; k < 4; k++) {
                        TR::Vertex &v = p.vertices[k];
                        glVertex3f(v.x + r.info.x, v.y, v.z + r.info.z);
                    }
                }
            }
            glEnd();

            glDepthMask(GL_TRUE);
            Core::setBlending(bmNone);
        }

        void entities(const TR::Level &level) {
            char buf[255];

            for (int i = 0; i < level.entitiesCount; i++) {
                TR::Entity &e = level.entities[i];
          
                sprintf(buf, "%d", (int)e.type);
                Debug::Draw::text(vec3(e.x, e.y, e.z), e.flags.active ? vec4(0, 0, 0.8, 1) : vec4(0.8, 0, 0, 1), buf);
            }

            for (int i = 0; i < level.camerasCount; i++) {
                TR::Camera &c = level.cameras[i];
          
                sprintf(buf, "%d (%d)", i, c.room);
                Debug::Draw::text(vec3(c.x, c.y, c.z), vec4(0, 0.8, 0, 1), buf);
            }
        }

        void zones(const TR::Level &level, Lara *lara) {
            Core::setDepthTest(false);
            for (int i = 0; i < level.roomsCount; i++)
                sectors(level, i, int(lara->pos.y), lara->zone);
            Core::setDepthTest(true);

            char buf[64];
            for (int i = 0; i < level.entitiesCount; i++) {
                TR::Entity &e = level.entities[i];
          
                if (e.type < TR::Entity::LARA || e.type > TR::Entity::ENEMY_GIANT_MUTANT)
                    continue;

                sprintf(buf, "zone: %d", ((Character*)e.controller)->zone );
                Debug::Draw::text(vec3(e.x, e.y - 128, e.z), vec4(0, 1.0, 0.8, 1), buf);
            }
        }

        void lights(const TR::Level &level, int room, Controller *lara) {
        //    int roomIndex = level.entities[lara->entity].room;
        //    int lightIndex = getLightIndex(lara->pos, roomIndex);
            glPointSize(8);
            for (int i = 0; i < level.roomsCount; i++)
                for (int j = 0; j < level.rooms[i].lightsCount; j++) {
                    TR::Room::Light &l = level.rooms[i].lights[j];
                    float a = 1.0f - intensityf(l.intensity);
                    vec3 p = vec3(l.x, l.y, l.z);
                    vec4 color = vec4(a, a, a, 1);

//                    if (i == room) color.x = color.z = 0;
                    Debug::Draw::point(p, color);
                    //if (i == roomIndex && j == lightIndex)
                    //    color = vec4(0, 1, 0, 1);
                    Debug::Draw::sphere(p, l.radius, color);
                }

            vec4 color = vec4(lara->mainLightColor.x, 0.0f, 0.0f, 1.0f);
            Debug::Draw::point(lara->mainLightPos, color);
            Debug::Draw::sphere(lara->mainLightPos, lara->mainLightColor.w, color);
        }

        void meshes(const TR::Level &level) {
        // static objects
            for (int i = 0; i < level.roomsCount; i++) {
                TR::Room &r = level.rooms[i];
 
                for (int j = 0; j < r.meshesCount; j++) {
                    TR::Room::Mesh &m = r.meshes[j];
                                        
                    TR::StaticMesh *sm = level.getMeshByID(m.meshID);
                    ASSERT(sm != NULL);

                    Box box;
                    vec3 offset = vec3(m.x, m.y, m.z);
                    sm->getBox(false, m.rotation, box); // visible box

                    Debug::Draw::box(offset + box.min, offset + box.max, vec4(1, 1, 0, 0.25));

                    if (sm->flags == 2) { // collision box
                        sm->getBox(true, m.rotation, box);
                        Debug::Draw::box(offset + box.min - vec3(10.0f), offset + box.max + vec3(10.0f), vec4(1, 0, 0, 0.50));
                    }
                    
                    if (!level.meshOffsets[sm->mesh]) continue;
                    const TR::Mesh &mesh = level.meshes[level.meshOffsets[sm->mesh]];

                    {
                        char buf[255];
                        sprintf(buf, "flags %d", (int)mesh.flags.value);
                        Debug::Draw::text(offset + (box.min + box.max) * 0.5f, vec4(0.5, 0.5, 1.0, 1), buf);
                    }
                    
                }
            }
        // dynamic objects            
            for (int i = 0; i < level.entitiesCount; i++) {
                TR::Entity &e = level.entities[i];
                Controller *controller = (Controller*)e.controller;
                if (!controller) continue;

                mat4 matrix = controller->getMatrix();
                Box box = controller->animation.getBoundingBox(vec3(0.0f), 0);
                Debug::Draw::box(matrix, box.min, box.max, vec4(1.0));


                for (int j = 0; j < level.modelsCount; j++) {
                    TR::Model &m = level.models[j];
                    TR::Node *node = m.node < level.nodesDataSize ? (TR::Node*)&level.nodesData[m.node] : NULL;

                    if (!node) continue; // ???
                    if (e.type == m.type) {
                        ASSERT(m.animation < 0xFFFF);

                        int fSize = sizeof(TR::AnimFrame) + m.mCount * sizeof(uint16) * 2;

                        TR::Animation *anim  = controller->animation;
                        TR::AnimFrame *frame = (TR::AnimFrame*)&level.frameData[(anim->frameOffset + (controller ? int((controller->animation.time * 30.0f / anim->frameRate)) * fSize : 0) >> 1)];

                        //mat4 m;
                        //m.identity();
                        //m.translate(vec3(frame->x, frame->y, frame->z).lerp(vec3(frameB->x, frameB->y, frameB->z), k));

                        int  sIndex = 0;
                        mat4 stack[20];
                        mat4 joint;

                        joint.identity();
                        if (frame) joint.translate(frame->pos);

                        for (int k = 0; k < m.mCount; k++) {

                            if (k > 0 && node) {
                                TR::Node &t = node[k - 1];

                                if (t.flags & 0x01) joint = stack[--sIndex];
                                if (t.flags & 0x02) stack[sIndex++] = joint;

                                ASSERT(sIndex >= 0 && sIndex < 20);

                                joint.translate(vec3(t.x, t.y, t.z));
                            }

                            vec3 a = frame ? frame->getAngle(k) : vec3(0.0f);

                            mat4 rot;
                            rot.identity();
                            rot.rotateY(a.y);
                            rot.rotateX(a.x);
                            rot.rotateZ(a.z);

                            joint = joint * rot;

                            int offset = level.meshOffsets[m.mStart + k];
                            TR::Mesh *mesh = (TR::Mesh*)&level.meshes[offset];
                            //if (!mesh->flags) continue;
                            Debug::Draw::sphere(matrix * joint * mesh->center, mesh->radius, vec4(0, 1, 1, 0.5f));
                            
                            { //if (e.id != 0) {
                                char buf[255];
                                sprintf(buf, "(%d) radius %d flags %d", (int)e.type, (int)mesh->radius, (int)mesh->flags.value);
                                Debug::Draw::text(matrix * joint * mesh->center, vec4(0.5, 1, 0.5, 1), buf);
                            }
                            
                        }
                        Debug::Draw::box(matrix, frame->box.min(), frame->box.max(), vec4(1.0));

                        break;
                    }
                
                }

            }
        }

        void dumpSample(TR::Level *level, int index) {
            char buf[255];
            sprintf(buf, "samples_PSX/%03d.wav", index);
            FILE *f = fopen(buf, "wb");

            if (level->version == TR::Level::VER_TR1_PSX) {
                int dataSize = level->soundSize[index] / 16 * 28 * 2 * 4;

                struct Header {
                    uint32 RIFF;
                    uint32 rSize;
                    uint32 WAVE;
                    uint32 fmt;
                    uint32 wSize;
                    struct {
                        uint16  format;
                        uint16  channels;
                        uint32  samplesPerSec;
                        uint32  bytesPerSec;
                        uint16  block;
                        uint16  sampleBits;
                    } fmtHeader;
                    uint32 data;
                    uint32 dataSize;
                } header = {
                        FOURCC("RIFF"), sizeof(Header) - 8 + dataSize,
                        FOURCC("WAVE"), FOURCC("fmt "), 16,
                        { 1, 1, 44100, 44100 * 16 / 8, 0, 16 },
                        FOURCC("data"), dataSize
                    };

                fwrite(&header, sizeof(header), 1, f);

                Sound::VAG vag(new Stream(&level->soundData[level->soundOffsets[index]], dataSize));        
                Sound::Frame frames[4 * 28];
                while (int count = vag.decode(frames, 4 * 28))
                    for (int i = 0; i < count; i++)
                        fwrite(&frames[i].L, 2, 1, f);                
            }

            if (level->version == TR::Level::VER_TR1_PC) {
                uint32 *data = (uint32*)&level->soundData[level->soundOffsets[index]];
                fwrite(data, data[1] + 8, 1, f);
            }

            fclose(f);
        }

        #define case_name(a,b) case a::b : return #b

        const char *getTriggerType(const TR::Level &level, const TR::Level::Trigger &trigger) {
            switch (trigger) {
                case_name(TR::Level::Trigger, ACTIVATE );
                case_name(TR::Level::Trigger, PAD      );
                case_name(TR::Level::Trigger, SWITCH   );
                case_name(TR::Level::Trigger, KEY      );
                case_name(TR::Level::Trigger, PICKUP   );
                case_name(TR::Level::Trigger, HEAVY    );
                case_name(TR::Level::Trigger, ANTIPAD  );
                case_name(TR::Level::Trigger, COMBAT   );
                case_name(TR::Level::Trigger, DUMMY    );
                case_name(TR::Level::Trigger, ANTI     );
            }
            return "UNKNOWN";
        }

        const char *getTriggerAction(const TR::Level &level, const TR::Action &action) {
            switch (action) {
                case_name(TR::Action, ACTIVATE      );
                case_name(TR::Action, CAMERA_SWITCH );
                case_name(TR::Action, FLOW          );
                case_name(TR::Action, FLIP_MAP      );
                case_name(TR::Action, FLIP_ON       );
                case_name(TR::Action, FLIP_OFF      );
                case_name(TR::Action, CAMERA_TARGET );
                case_name(TR::Action, END           );
                case_name(TR::Action, SOUNDTRACK    );
                case_name(TR::Action, HARDCODE      );
                case_name(TR::Action, SECRET        );
                case_name(TR::Action, CLEAR         );
                case_name(TR::Action, CAMERA_FLYBY  );
                case_name(TR::Action, CUTSCENE      );
            }
            return "UNKNOWN";
        }

        const char *getEntityName(const TR::Level &level, const TR::Entity &entity) {
            switch (entity.type) {
                case_name(TR::Entity, LARA                 ); 
                case_name(TR::Entity, ENEMY_TWIN           ); 
                case_name(TR::Entity, ENEMY_WOLF           ); 
                case_name(TR::Entity, ENEMY_BEAR           ); 
                case_name(TR::Entity, ENEMY_BAT            ); 
                case_name(TR::Entity, ENEMY_CROCODILE_LAND ); 
                case_name(TR::Entity, ENEMY_CROCODILE_WATER); 
                case_name(TR::Entity, ENEMY_LION_MALE      ); 
                case_name(TR::Entity, ENEMY_LION_FEMALE    ); 
                case_name(TR::Entity, ENEMY_PUMA           ); 
                case_name(TR::Entity, ENEMY_GORILLA        ); 
                case_name(TR::Entity, ENEMY_RAT_LAND       ); 
                case_name(TR::Entity, ENEMY_RAT_WATER      ); 
                case_name(TR::Entity, ENEMY_REX            ); 
                case_name(TR::Entity, ENEMY_RAPTOR         ); 
                case_name(TR::Entity, ENEMY_MUTANT_1       ); 
                case_name(TR::Entity, ENEMY_CENTAUR        ); 
                case_name(TR::Entity, ENEMY_MUMMY          ); 
                case_name(TR::Entity, ENEMY_LARSON         ); 
                case_name(TR::Entity, TRAP_FLOOR           ); 
                case_name(TR::Entity, TRAP_BLADE           ); 
                case_name(TR::Entity, TRAP_SPIKES          ); 
                case_name(TR::Entity, TRAP_BOULDER         ); 
                case_name(TR::Entity, TRAP_DART            ); 
                case_name(TR::Entity, TRAP_DARTGUN         ); 
                case_name(TR::Entity, BLOCK_1              ); 
                case_name(TR::Entity, BLOCK_2              ); 
                case_name(TR::Entity, SWITCH               ); 
                case_name(TR::Entity, SWITCH_WATER         ); 
                case_name(TR::Entity, DOOR_1               ); 
                case_name(TR::Entity, DOOR_2               ); 
                case_name(TR::Entity, DOOR_3               ); 
                case_name(TR::Entity, DOOR_4               ); 
                case_name(TR::Entity, DOOR_BIG_1           ); 
                case_name(TR::Entity, DOOR_BIG_2           ); 
                case_name(TR::Entity, DOOR_5               ); 
                case_name(TR::Entity, DOOR_6               ); 
                case_name(TR::Entity, TRAP_DOOR_1          ); 
                case_name(TR::Entity, TRAP_DOOR_2          );
                case_name(TR::Entity, BRIDGE_0             );
                case_name(TR::Entity, BRIDGE_1             );
                case_name(TR::Entity, BRIDGE_2             );
                case_name(TR::Entity, GEARS_1              );
                case_name(TR::Entity, GEARS_2              );
                case_name(TR::Entity, GEARS_3              );
                case_name(TR::Entity, PUZZLE_1             ); 
                case_name(TR::Entity, PUZZLE_2             ); 
                case_name(TR::Entity, PUZZLE_3             ); 
                case_name(TR::Entity, PUZZLE_4             ); 
                case_name(TR::Entity, HOLE_PUZZLE          ); 
                case_name(TR::Entity, HOLE_PUZZLE_SET      ); 
                case_name(TR::Entity, PICKUP               ); 
                case_name(TR::Entity, KEY_1                ); 
                case_name(TR::Entity, KEY_2                ); 
                case_name(TR::Entity, KEY_3                ); 
                case_name(TR::Entity, KEY_4                ); 
                case_name(TR::Entity, HOLE_KEY             ); 
                case_name(TR::Entity, VIEW_TARGET          );
                case_name(TR::Entity, WATERFALL            ); 
            }
            return "UNKNOWN";
        }

        void info(const TR::Level &level, const TR::Entity &entity, Animation &anim) {
            float y = 0.0f;

            char buf[255];
            sprintf(buf, "DIP = %d, TRI = %d, SND = %d", Core::stats.dips, Core::stats.tris, Sound::channelsCount);
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);
            vec3 angle = ((Controller*)entity.controller)->angle * RAD2DEG;
            sprintf(buf, "pos = (%d, %d, %d), angle = (%d, %d), room = %d", entity.x, entity.y, entity.z, (int)angle.x, (int)angle.y, entity.room);
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);
            int rate = anim.anims[anim.index].frameRate;
            sprintf(buf, "state = %d, anim = %d, next = %d, rate = %d, frame = %.2f / %d (%f)", anim.state, anim.index, anim.next, rate, anim.time * 30.0f, anim.framesCount, anim.delta);
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);
            
            TR::Level::FloorInfo info;
            level.getFloorInfo(entity.room, entity.x, entity.y, entity.z, info);
            sprintf(buf, "floor = %d, roomBelow = %d, roomAbove = %d, height = %d", info.floorIndex, info.roomBelow, info.roomAbove, info.floor - info.ceiling);
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);
          
            if (info.trigCmdCount > 0) {
                y += 16;
                sprintf(buf, "trigger: %s%s mask: %d", getTriggerType(level, info.trigger), info.trigInfo.once ? " (once)" : "", info.trigInfo.mask);
                Debug::Draw::text(vec2(16, y += 16), vec4(0.5f, 0.8f, 0.5f, 1.0f), buf);

                for (int i = 0; i < info.trigCmdCount; i++) {
                    TR::FloorData::TriggerCommand &cmd = info.trigCmd[i];
                    
                    const char *ent = (cmd.action == TR::Action::ACTIVATE || cmd.action == TR::Action::CAMERA_TARGET) ? getEntityName(level, level.entities[cmd.args]) : "";
                    sprintf(buf, "%s -> %s (%d)", getTriggerAction(level, cmd.action), ent, cmd.args);
                    Debug::Draw::text(vec2(16, y += 16), vec4(0.1f, 0.6f, 0.1f, 1.0f), buf);
                }
            }
        }
    }
}

#endif
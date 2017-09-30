#ifndef H_DEBUG
#define H_DEBUG

#include "core.h"
#include "format.h"
#include "controller.h"
#include "mesh.h"

namespace Debug {

    static GLuint font;

    void init() {
        #ifdef WIN32
            font = glGenLists(256);
            HDC hdc = GetDC(0);
            HFONT hfont = CreateFontA(-MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0,
                                     0, 0, FW_BOLD, 0, 0, 0,
                                     ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     ANTIALIASED_QUALITY, DEFAULT_PITCH, "Courier New");
            SelectObject(hdc, hfont);
            wglUseFontBitmaps(hdc, 0, 256, font);
            DeleteObject(hfont);
        #elif LINUX
            XFontStruct *fontInfo;
            Font id;
            unsigned int first, last;
            fontInfo = XLoadQueryFont(glXGetCurrentDisplay(), "-adobe-times-medium-r-normal--17-120-100-100-p-88-iso8859-1");

            if (fontInfo == NULL) {
                LOG("no font found\n");
            }

            id = fontInfo->fid;
            first = fontInfo->min_char_or_byte2;
            last = fontInfo->max_char_or_byte2;

            font = glGenLists(last + 1);
            if (font == 0) {
                LOG("out of display lists\n");
            }
            glXUseXFont(id, first, last - first + 1, font + first);
        #endif
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
        Core::validateRenderState();
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

            Core::setDepthTest(false);
            Core::setCulling(cfNone);
            Core::validateRenderState();

            glColor4fv((GLfloat*)&color);
            glRasterPos2f(pos.x, pos.y);
            glListBase(font);
            glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
            Core::setDepthTest(true);
            Core::setCulling(cfFront);

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
                            bool blockable = level.boxes[s.boxIndex].overlap.blockable;
                            bool block     = level.boxes[s.boxIndex].overlap.block;
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
            char str[64];

            TR::Box &b = level.boxes[boxIndex];
            sprintf(str, "%d", boxIndex);
            Draw::text(vec3((b.maxX + b.minX) * 0.5, b.floor, (b.maxZ + b.minZ) * 0.5), vec4(0, 1, 0, 1), str);
            glColor4f(0.0f, 1.0f, 0.0f, 0.25f);
            Core::setBlending(bmAlpha);
            debugBox(b);

            TR::Overlap *o = &level.overlaps[level.boxes[boxIndex].overlap.index];
            do {
                TR::Box &b = level.boxes[o->boxIndex];
                sprintf(str, "%d", o->boxIndex);
                Draw::text(vec3((b.maxX + b.minX) * 0.5, b.floor, (b.maxZ + b.minZ) * 0.5), vec4(0, 0, 1, 1), str);
                glColor4f(0.0f, 0.0f, 1.0f, 0.25f);
                Core::setBlending(bmAlpha);
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
          
                sprintf(buf, "%d (%d)", (int)e.type, i);
                Debug::Draw::text(vec3(e.x, e.y, e.z), e.flags.active ? vec4(0, 0, 0.8, 1) : vec4(0.8, 0, 0, 1), buf);
            }

            for (int i = 0; i < level.camerasCount; i++) {
                TR::Camera &c = level.cameras[i];
          
                sprintf(buf, "%d (%d)", i, c.room);
                Debug::Draw::text(vec3(c.x, c.y, c.z), vec4(0, 0.8, 0, 1), buf);
            }
        }

        void path(TR::Level &level, Enemy *enemy) {
            Enemy::Path *path = enemy->path;

            if (!path || !enemy->target) return;
            for (int i = 0; i < path->count; i++) {
                TR::Box &b = level.boxes[path->boxes[i]];
                if (i == path->index)
                    glColor4f(0.5, 0.5, 0.0, 0.5);
                else
                    glColor4f(0.0, 0.5, 0.0, 0.5);
                debugBox(b);
            }

            Core::setDepthTest(false);
            Draw::point(enemy->waypoint, vec4(1.0));
            Core::setDepthTest(true);
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
                    TR::Room::Mesh &m  = r.meshes[j];
                    TR::StaticMesh *sm = &level.staticMeshes[m.meshIndex];

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
                    /*
                    {
                        char buf[255];
                        sprintf(buf, "flags %d", (int)mesh.flags.value);
                        Debug::Draw::text(offset + (box.min + box.max) * 0.5f, vec4(0.5, 0.5, 1.0, 1), buf);
                    }
                    */
                    
                }
            }
        // dynamic objects            
            for (int i = 0; i < level.entitiesCount; i++) {
                TR::Entity &e = level.entities[i];
                Controller *controller = (Controller*)e.controller;
                if (!controller) continue;

                mat4 matrix = controller->getMatrix();
                Basis basis(matrix);

                TR::Model *m = controller->getModel();
                if (!m) continue;

                bool bboxIntersect = false;

                ASSERT(m->mCount <= 34);

                int mask = 0;
                for (int j = 0; j < level.entitiesCount; j++) {
                    TR::Entity &t = level.entities[j];
                    if (j == i || ((!t.isEnemy() || !t.flags.active) && t.type != TR::Entity::LARA)) continue;
                    Controller *enemy = (Controller*)t.controller;
                    if (!controller->getBoundingBox().intersect(enemy->getBoundingBox()))
                        continue;
                    bboxIntersect = true;
                    mask |= controller->collide(enemy);
                }

                Box box = controller->getBoundingBoxLocal();
                Debug::Draw::box(matrix, box.min, box.max, bboxIntersect ? vec4(1, 0, 0, 1): vec4(1));

                Sphere spheres[34];
                int count;
                controller->getSpheres(spheres, count);

                for (int joint = 0; joint < count; joint++) {
                    Sphere &sphere = spheres[joint];
                    Debug::Draw::sphere(sphere.center, sphere.radius, (mask & (1 << joint)) ? vec4(1, 0, 0, 0.5f) : vec4(0, 1, 1, 0.5f));
                    /*
                    { //if (e.id != 0) {
                        char buf[255];
                        sprintf(buf, "(%d) radius %d flags %d", (int)e.type, (int)mesh->radius, (int)mesh->flags.value);
                        Debug::Draw::text(sphere.center, vec4(0.5, 1, 0.5, 1), buf);
                    }
                    */
                }
            }
        }

        void dumpSample(TR::Level *level, int index) {
            char buf[255];
            sprintf(buf, "samples_PSX/%03d.wav", index);
            FILE *f = fopen(buf, "wb");

            if (level->version == TR::VER_TR1_PSX) {
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
                        FOURCC("RIFF"), (uint32) sizeof(Header) - 8 + dataSize,
                        FOURCC("WAVE"), FOURCC("fmt "), 16,
                        { 1, 1, 44100, 44100 * 16 / 8, 0, 16 },
                        FOURCC("data"), (uint32) dataSize
                    };

                fwrite(&header, sizeof(header), 1, f);

                Sound::VAG vag(new Stream(&level->soundData[level->soundOffsets[index]], dataSize));        
                Sound::Frame frames[4 * 28];
                while (int count = vag.decode(frames, 4 * 28))
                    for (int i = 0; i < count; i++)
                        fwrite(&frames[i].L, 2, 1, f);                
            }

            if (level->version == TR::VER_TR1_PC) {
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
            }
            return "UNKNOWN";
        }

        const char *getTriggerAction(const TR::Level &level, const TR::Action &action) {
            switch (action) {
                case_name(TR::Action, ACTIVATE      );
                case_name(TR::Action, CAMERA_SWITCH );
                case_name(TR::Action, FLOW          );
                case_name(TR::Action, FLIP          );
                case_name(TR::Action, FLIP_ON       );
                case_name(TR::Action, FLIP_OFF      );
                case_name(TR::Action, CAMERA_TARGET );
                case_name(TR::Action, END           );
                case_name(TR::Action, SOUNDTRACK    );
                case_name(TR::Action, EFFECT        );
                case_name(TR::Action, SECRET        );
            }
            return "UNKNOWN";
        }

        const char *TR1_TYPE_NAMES[] = { TR1_TYPES(DECL_STR) };

        const char *getEntityName(const TR::Level &level, const TR::Entity &entity) {
            if (entity.type == TR::Entity::NONE)
                return "NONE";
            if (entity.type < 0 || entity.type >= COUNT(TR1_TYPE_NAMES))
                return "UNKNOWN";
            return TR1_TYPE_NAMES[entity.type];
        }

        void info(const TR::Level &level, const TR::Entity &entity, Animation &anim) {
            float y = 0.0f;

            int activeCount = 0;
            Controller *c = Controller::first;
            while (c) {
                activeCount++;
                c = c->next;
            }

            char buf[255];
            sprintf(buf, "DIP = %d, TRI = %d, SND = %d, active = %d", Core::stats.dips, Core::stats.tris, Sound::channelsCount, activeCount);
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);
            vec3 angle = ((Controller*)entity.controller)->angle * RAD2DEG;
            sprintf(buf, "pos = (%d, %d, %d), angle = (%d, %d), room = %d (camera: %d)", entity.x, entity.y, entity.z, (int)angle.x, (int)angle.y, ((Controller*)entity.controller)->getRoomIndex(), ((ICamera*)level.cameraController)->getRoomIndex());
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);
            int rate = anim.anims[anim.index].frameRate;
            sprintf(buf, "state = %d, anim = %d, next = %d, rate = %d, frame = %.2f / %d (%f)", anim.state, anim.index, anim.next, rate, anim.time * 30.0f, anim.framesCount, anim.delta);
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);
            
            TR::Level::FloorInfo info;
            level.getFloorInfo(((Controller*)entity.controller)->getRoomIndex(), entity.x, entity.y, entity.z, info);
            sprintf(buf, "floor = %d, roomBelow = %d, roomAbove = %d, height = %d", info.floorIndex, info.roomBelow, info.roomAbove, info.floor - info.ceiling);
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);

            y += 16;
            if (info.lava)
                Debug::Draw::text(vec2(16, y += 16), vec4(1.0f, 0.5f, 0.3f, 1.0f), "LAVA");

            if (info.trigCmdCount > 0) {
                sprintf(buf, "trigger: %s%s mask: %d timer: %d", getTriggerType(level, info.trigger), info.trigInfo.once ? " (once)" : "", info.trigInfo.mask, info.trigInfo.timer);
                Debug::Draw::text(vec2(16, y += 16), vec4(0.5f, 0.8f, 0.5f, 1.0f), buf);

                for (int i = 0; i < info.trigCmdCount; i++) {
                    TR::FloorData::TriggerCommand &cmd = info.trigCmd[i];
                    
                    const char *ent = (cmd.action == TR::Action::ACTIVATE || cmd.action == TR::Action::CAMERA_TARGET) ? getEntityName(level, level.entities[cmd.args]) : "";
                    sprintf(buf, "%s -> %s (%d)", getTriggerAction(level, cmd.action), ent, cmd.args);
                    if (cmd.action == TR::Action::CAMERA_SWITCH) {
                        i++;
                        sprintf(buf, "%s delay: %d speed: %d", buf, int(info.trigCmd[i].timer), int(info.trigCmd[i].speed) * 8 + 1);
                    }

                    Debug::Draw::text(vec2(16, y += 16), vec4(0.1f, 0.6f, 0.1f, 1.0f), buf);
                }
            }
        }
    }
}

#endif
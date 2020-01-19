#ifndef H_DEBUG
#define H_DEBUG

#include "core.h"
#include "format.h"
#include "controller.h"
#include "mesh.h"

namespace Debug {

    static GLuint font;

    void init() {
        #ifdef _OS_WIN
            font = glGenLists(256);
            HDC hdc = GetDC(0);
            HFONT hfont = CreateFontA(-MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0,
                                     0, 0, FW_BOLD, 0, 0, 0,
                                     ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     ANTIALIASED_QUALITY, DEFAULT_PITCH, "Courier New");
            SelectObject(hdc, hfont);
            wglUseFontBitmaps(hdc, 0, 256, font);
            DeleteObject(hfont);
        #elif _OS_LINUX
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

    void deinit() {
        glDeleteLists(font, 256);
    }

    void begin() {
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_TEXTURE_2D);
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
        glEnable(GL_TEXTURE_2D);
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

        void textColor(const vec2 &pos, const vec4 &color, const char *str) {
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();

            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            glOrtho(0, Core::width, Core::height, 0, 0, 1);

            Core::setDepthTest(false);
            Core::setCullMode(cmNone);
            Core::validateRenderState();

            glColor4fv((GLfloat*)&color);
            glRasterPos2f(pos.x, pos.y);
            glListBase(font);
            glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
            Core::setDepthTest(true);
            Core::setCullMode(cmFront);

            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glPopMatrix();
        }

        void text(const vec2 &pos, const vec4 &color, const char *str) {
            textColor(pos + vec2(1.0f), vec4(0.0f, 0.0f, 0.0f, 1.0f), str);
            textColor(pos, color, str);
        }

        void text(const vec3 &pos, const vec4 &color, const char *str) {
            vec4 p = Core::mViewProj * vec4(pos, 1);
            if (p.w > 0) {
                p.xyz() = p.xyz() * (1.0f / p.w);
                p.y = -p.y;	
                p.xyz() = (p.xyz() * 0.5f + vec3(0.5f)) * vec3(float(Core::width), float(Core::height), 1.0f);	
                text(vec2(p.x, p.y), color, str);
            }
        }
    }

    namespace Level {

        #define case_name(a,b) case a::b : return #b

        const char *getTriggerType(const TR::Level &level, const TR::Level::Trigger::Type &trigger) {
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

        const char *getTriggerAction(const TR::Level &level, uint16 action) {
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

        const char *TR_TYPE_NAMES[] = { TR_TYPES(DECL_STR) };

        const char *getEntityName(const TR::Level &level, const TR::Entity &entity) {
            if (entity.type < TR::Entity::TR1_TYPE_MAX)
                return TR_TYPE_NAMES[entity.type - TR1_TYPES_START];
            if (entity.type < TR::Entity::TR2_TYPE_MAX)
                return TR_TYPE_NAMES[entity.type - TR2_TYPES_START + (TR::Entity::TR1_TYPE_MAX - TR1_TYPES_START) + 1];
            if (entity.type < TR::Entity::TR3_TYPE_MAX)
                return TR_TYPE_NAMES[entity.type - TR3_TYPES_START + (TR::Entity::TR1_TYPE_MAX - TR1_TYPES_START) + (TR::Entity::TR2_TYPE_MAX - TR2_TYPES_START) + 2];
            if (entity.type < TR::Entity::TR4_TYPE_MAX)
                return TR_TYPE_NAMES[entity.type - TR4_TYPES_START + (TR::Entity::TR1_TYPE_MAX - TR1_TYPES_START) + (TR::Entity::TR2_TYPE_MAX - TR2_TYPES_START) + (TR::Entity::TR3_TYPE_MAX - TR3_TYPES_START) + 3];

            return "UNKNOWN";
        }

        void debugFloor(IGame *game, int roomIndex, int x, int y, int z, int zone = -1) {
            TR::Level *level = game->getLevel();

            if (zone != -1) {
                int dx, dz;
                TR::Room::Sector &s = level->getSector(roomIndex, x, z, dx, dz);
                if (zone != level->zones[0].ground1[s.boxIndex])
                    return;                
            }
            
            TR::Level::FloorInfo info;

            vec3 rf[4], rc[4], f[4], c[4];

            int offsets[4][2] = { { 1, 1 }, { 1023, 1 }, { 1023, 1023 }, { 1, 1023 } };

            for (int i = 0; i < 4; i++) {
                game->getLara()->getFloorInfo(roomIndex, vec3(float(x + offsets[i][0]), float(y), float(z + offsets[i][1])), info);
                rf[i] = vec3( float(x + offsets[i][0]), info.roomFloor - 4,   float(z + offsets[i][1]) );
                rc[i] = vec3( float(x + offsets[i][0]), info.roomCeiling + 4, float(z + offsets[i][1]) );
                f[i]  = vec3( float(x + offsets[i][0]), info.floor - 4,       float(z + offsets[i][1]) );
                c[i]  = vec3( float(x + offsets[i][0]), info.ceiling + 4,     float(z + offsets[i][1]) );

                /*
                int px = x + offsets[i][0];
                int py = y;
                int pz = z + offsets[i][1];

                int dx, dz;

                int16 ridx = roomIndex;
                TR::Room::Sector *sector = game->getLevel()->getSectorNext(ridx, px, py, pz);
                int floor = game->getLevel()->getFloor(sector, px, py, pz);
                int ceiling = game->getLevel()->getCeiling(sector, px, py, pz);

                f[i]  = vec3( px, floor - 4,   pz );
                c[i]  = vec3( px, ceiling + 4, pz );
                */
                if (info.roomBelow == TR::NO_ROOM) rf[i].y = f[i].y;
                if (info.roomAbove == TR::NO_ROOM) rc[i].y = c[i].y;
            }

            if (info.roomNext != 0xFF) {
                glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
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
                glVertex3f(float(b.minX), y, float(b.maxZ));
                glVertex3f(float(b.maxX), y, float(b.maxZ));
                glVertex3f(float(b.maxX), y, float(b.minZ));
                glVertex3f(float(b.minX), y, float(b.minZ));
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
                                Debug::Draw::text(vec3(float(r.info.x + x * 1024 + 512), float(floor), float(r.info.z + z * 1024 + 512)), vec4(1, 1, 0, 1), buf);
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
            Draw::text(vec3((b.maxX + b.minX) * 0.5f, b.floor, (b.maxZ + b.minZ) * 0.5f), vec4(0, 1, 0, 1), str);
            glColor4f(0.0f, 1.0f, 0.0f, 0.25f);
            Core::setBlendMode(bmAlpha);
            debugBox(b);

            TR::Overlap *o = &level.overlaps[level.boxes[boxIndex].overlap.index];
            do {
                TR::Box &b = level.boxes[o->boxIndex];
                sprintf(str, "%d", o->boxIndex);
                Draw::text(vec3((b.maxX + b.minX) * 0.5f, b.floor, (b.maxZ + b.minZ) * 0.5f), vec4(0, 0, 1, 1), str);
                glColor4f(0.0f, 0.0f, 1.0f, 0.25f);
                Core::setBlendMode(bmAlpha);
                debugBox(b);
            } while (!(o++)->end);
        }

        void debugBoxes(const TR::Level &level, uint16 *boxes, int count) {
            if (!boxes) return;

            glColor4f(0.0f, 1.0f, 0.0f, 0.25f);
            Core::setBlendMode(bmAlpha);
            Core::setDepthTest(false);
            Core::validateRenderState();
            for (int i = 0; i < count; i++)
                debugBox(level.boxes[boxes[i]]);
            Core::setDepthTest(true);
        }

        void sectors(IGame *game, int roomIndex, int y, int zone = -1) {
            TR::Room &room = game->getLevel()->rooms[roomIndex];

            for (int z = 0; z < room.zSectors; z++)
                for (int x = 0; x < room.xSectors; x++)
                    debugFloor(game, roomIndex, room.info.x + x * 1024, y, room.info.z + z * 1024, zone);
        }

        void rooms(const TR::Level &level, const vec3 &pos, int roomIndex) {
            glDepthMask(GL_FALSE);

            for (int i = 0; i < level.roomsCount; i++) {
                TR::Room &r = level.rooms[i];
                vec3 p = vec3(float(r.info.x), float(r.info.yTop), float(r.info.z));

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
            Core::setBlendMode(bmAdd);
            glColor3f(0, 0.25f, 0.25f);
            glDepthMask(GL_FALSE);

            glBegin(GL_QUADS);
            for (int i = 0; i < level.roomsCount; i++) {
                TR::Room &r = level.rooms[i];
                for (int j = 0; j < r.portalsCount; j++) {
                    TR::Room::Portal &p = r.portals[j];
                    for (int k = 0; k < 4; k++) {
                        short3 &v = p.vertices[k];
                        glVertex3f(float(v.x + r.info.x), float(v.y), float(v.z + r.info.z));
                    }
                }
            }
            glEnd();

            glDepthMask(GL_TRUE);
            Core::setBlendMode(bmNone);
        }

        void entities(const TR::Level &level) {
            char buf[255];

            for (int i = 0; i < level.entitiesCount; i++) {
                TR::Entity &e = level.entities[i];
                Controller *controller = (Controller*)e.controller;
                if (!controller) continue;
          
                sprintf(buf, "%s (%d) %s", getEntityName(level, e), i, controller->flags.invisible ? "INVISIBLE" : "");
                Debug::Draw::text(controller->getPos() + randf() * 64.0f, controller->flags.active ? vec4(0, 0, 0.8f, 1) : vec4(0.8f, 0, 0, 1), buf);
            }

            for (int i = 0; i < level.camerasCount; i++) {
                TR::Camera &c = level.cameras[i];
          
                sprintf(buf, "%d (%d)", i, c.room);
                Debug::Draw::text(vec3(float(c.x), float(c.y), float(c.z)), vec4(0, 0.8f, 0, 1), buf);
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

        void zones(IGame *game, Lara *lara) {
            TR::Level *level = game->getLevel();

            Core::setDepthTest(false);
            for (int i = 0; i < level->roomsCount; i++)
                sectors(game, i, int(lara->pos.y), lara->zone);
            Core::setDepthTest(true);

            char buf[64];
            for (int i = 0; i < level->entitiesCount; i++) {
                TR::Entity &e = level->entities[i];
          
                if (!e.controller || !e.isEnemy()) continue;

                Character *controller = (Character*)e.controller;
                sprintf(buf, "zone: %d", controller->zone);
                Debug::Draw::text(controller->pos - vec3(0, 128, 0), vec4(0, 1, 0.8f, 1), buf);
            }
        }

        void lights(const TR::Level &level, int room, Controller *lara) {
            glPointSize(8);
            for (int i = 0; i < level.roomsCount; i++)
                for (int j = 0; j < level.rooms[i].lightsCount; j++) {
                    TR::Room::Light &l = level.rooms[i].lights[j];
                    vec3 p = vec3(float(l.x), float(l.y), float(l.z));
                    vec4 color = vec4(l.color.r, l.color.g, l.color.b, 255) * (1.0f / 255.0f);

//                    if (i == room) color.x = color.z = 0;
                    Debug::Draw::point(p, color);
                    //if (i == roomIndex && j == lightIndex)
                    //    color = vec4(0, 1, 0, 1);
                    Debug::Draw::sphere(p, float(l.radius), color);
                }

            vec4 color = vec4(lara->mainLightColor.xyz(), 1.0f);
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
                    vec3 offset = vec3(float(m.x), float(m.y), float(m.z));
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

                const TR::Model *m = controller->getModel();
                if (!m) continue;

                bool bboxIntersect = false;

                ASSERT(m->mCount <= 34);

                int mask = 0;
                for (int j = 0; j < level.entitiesCount; j++) {
                    TR::Entity &t = level.entities[j];
                    Controller *enemy = (Controller*)t.controller;
                    if (!enemy) continue;
                    if (j == i || ((!t.isEnemy() || !enemy->flags.active) && !t.isLara())) continue;
                    if (!enemy || !controller->getBoundingBox().intersect(enemy->getBoundingBox()))
                        continue;
                    bboxIntersect = true;
                    mask |= controller->collide(enemy);
                }

                Box box = controller->getBoundingBoxLocal();
                Debug::Draw::box(matrix, box.min, box.max, bboxIntersect ? vec4(1, 0, 0, 1): vec4(1));

                Sphere spheres[MAX_JOINTS];
                int count = controller->getSpheres(spheres);

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
                uint32 dataSize = level->soundSize[index] / 16 * 28 * 2 * 4;

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

                Sound::VAG vag(new Stream(NULL, &level->soundData[level->soundOffsets[index]], dataSize));        
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

        void info(IGame *game, Controller *controller, Animation &anim) {
            TR::Level &level = *game->getLevel();

            if (level.isCutsceneLevel()) return;

            float y = 0.0f;

            int activeCount = 0;
            Controller *c = Controller::first;
            while (c) {
                activeCount++;
                c = c->next;
            }

            vec3 viewPos = ((Lara*)controller)->camera->frustum->pos;

            char buf[255];
            sprintf(buf, "DIP = %d, TRI = %d, SND = %d, active = %d", Core::stats.dips, Core::stats.tris, Sound::channelsCount, activeCount);
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);
            vec3 angle = controller->angle * RAD2DEG;
            sprintf(buf, "pos = (%d, %d, %d), angle = (%d, %d), room = %d (camera: %d [%d, %d, %d])", int(controller->pos.x), int(controller->pos.y), int(controller->pos.z), (int)angle.x, (int)angle.y, controller->getRoomIndex(), game->getCamera()->getRoomIndex(), int(viewPos.x), int(viewPos.y), int(viewPos.z));
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);
            int rate = anim.anims[anim.index].frameRate;
            sprintf(buf, "state = %d, anim = %d, next = %d, rate = %d, frame = %.2f / %d (%f)", anim.state, anim.index, anim.next, rate, anim.time * 30.0f, anim.framesCount, anim.delta);
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);
            
            TR::Level::FloorInfo info;
            controller->getFloorInfo(controller->getRoomIndex(), controller->pos, info);
            sprintf(buf, "floor = %d, roomBelow = %d, roomAbove = %d, roomNext = %d, height = %d", info.floorIndex, info.roomBelow, info.roomAbove, info.roomNext, int(info.floor - info.ceiling));
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);

            sprintf(buf, "stats: time = %d, distance = %d, secrets = %c%c%c, pickups = %d, mediUsed = %d, ammoUsed = %d, kills = %d", saveStats.time, saveStats.distance, 
                (saveStats.secrets & 4) ? '1' : '0', 
                (saveStats.secrets & 2) ? '1' : '0',
                (saveStats.secrets & 1) ? '1' : '0', saveStats.pickups, saveStats.mediUsed, saveStats.ammoUsed, saveStats.kills);
            Debug::Draw::text(vec2(16, y += 16), vec4(1.0f), buf);

            y += 16;
            if (info.lava)
                Debug::Draw::text(vec2(16, y += 16), vec4(1.0f, 0.5f, 0.3f, 1.0f), "LAVA");

            if (info.climb) {
                sprintf(buf, "climb mask:");
                if (info.climb & 0x01) strcat(buf, " +Z");
                if (info.climb & 0x02) strcat(buf, " +X");
                if (info.climb & 0x04) strcat(buf, " -Z");
                if (info.climb & 0x08) strcat(buf, " -X");
                Debug::Draw::text(vec2(16, y += 16), vec4(0.5f, 0.8f, 0.5f, 1.0f), buf);
            }

            if (info.trigCmdCount > 0) {
                sprintf(buf, "trigger: %s%s mask: %d timer: %d", getTriggerType(level, info.trigger), info.trigInfo.once ? " (once)" : "", info.trigInfo.mask, info.trigInfo.timer);
                Debug::Draw::text(vec2(16, y += 16), vec4(0.5f, 0.8f, 0.5f, 1.0f), buf);

                for (int i = 0; i < info.trigCmdCount; i++) {
                    TR::FloorData::TriggerCommand &cmd = info.trigCmd[i];
                    
                    const char *ent = (cmd.action == TR::Action::ACTIVATE || cmd.action == TR::Action::CAMERA_TARGET) ? (cmd.args < level.entitiesBaseCount ? getEntityName(level, level.entities[cmd.args]) : "BAD_ENTITY_INDEX") : "";
                    sprintf(buf, "%s -> %s (%d)", getTriggerAction(level, cmd.action), ent, cmd.args);
                    if (cmd.action == TR::Action::CAMERA_SWITCH || cmd.action == TR::Action::FLYBY || cmd.action == TR::Action::CUTSCENE) {
                        i++;
                        sprintf(buf, "%s delay: %d speed: %d", buf, int(info.trigCmd[i].timer), int(info.trigCmd[i].speed));
                    }

                    Debug::Draw::text(vec2(16, y += 16), vec4(0.1f, 0.6f, 0.1f, 1.0f), buf);
                }
            }
        }
    }
}

#endif
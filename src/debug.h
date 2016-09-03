#ifndef H_DEBUG
#define H_DEBUG

#include "core.h"
#include "format.h"

namespace Debug {

    void begin() {
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf((GLfloat*)&Core::mProj);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glLoadMatrixf((GLfloat*)&Core::mView);

        glLineWidth(3);
        glPointSize(32);

        glUseProgram(0);
    }

    void end() {
        //
    }

    namespace Draw {

        void box(const vec3 &min, const vec3 &max) {
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
    }

    namespace Level {

        void debugFloor(const TR::Level &level, const vec3 &f, const vec3 &c, int floorIndex, bool current) {
            vec3 vf[4] = { f, f + vec3(1024, 0, 0), f + vec3(1024, 0, 1024), f + vec3(0, 0, 1024) };
            vec3 vc[4] = { c, c + vec3(1024, 0, 0), c + vec3(1024, 0, 1024), c + vec3(0, 0, 1024) };

            uint16 cmd, *d = &level.floors[floorIndex];

            if (floorIndex)
                do {
                    cmd = *d++;
                    int func = cmd & 0x00FF;        // function
                    int sub  = (cmd & 0x7F00) >> 8; // sub function

                    if (func == 0x01) { // portal
                        d++;
                    //  d += 2;

                    }

                    if ((func == 0x02 || func == 0x03) && sub == 0x00) { // floor & ceiling corners
                        int sx = 256 * int((int8)(*d & 0x00FF));
                        int sz = 256 * int((int8)((*d & 0xFF00) >> 8));

                        auto &p = func == 0x02 ? vf : vc;

                        if (func == 0x02) {

                        //  if (current)
                        //      LOG("%d\n", sx);

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
                        d++;
                    }


                    if (func == 0x04) {
                        //*d++; // trigger setup
                        /*
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

                } while (!(cmd & 0x8000));

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

        void debugSectors(const TR::Level &level, const vec3 &pos, int roomIndex) {
            TR::Room &room = level.rooms[roomIndex];

            vec3 p = (pos - vec3(room.info.x, 0, room.info.z)) / vec3(1024, 1, 1024);

            for (int z = 0; z < room.zSectors; z++)
                for (int x = 0; x < room.xSectors; x++) {
                    auto &s = room.sectors[x * room.zSectors + z];
                    vec3 f(x * 1024 + room.info.x, s.floor * 256, z * 1024 + room.info.z);
                    vec3 c(x * 1024 + room.info.x, s.ceiling * 256, z * 1024 + room.info.z);

                    debugFloor(level, f, c, s.floorIndex, (int)p.x == x && (int)p.z == z);
                }
        }

        void rooms(const TR::Level &level, const vec3 &pos, int roomIndex) {
            Core::setBlending(bmAdd);
            glDepthMask(GL_FALSE);

            for (int i = 0; i < level.roomsCount; i++) {
                TR::Room &r = level.rooms[i];
                vec3 p = vec3(r.info.x, r.info.yTop, r.info.z);

                if (i == roomIndex) {
                //if (lara->insideRoom(Core::viewPos, i)) {
                    debugSectors(level, pos, i);
                    glColor3f(0, 1, 0);
                } else
                    glColor3f(1, 1, 1);

                Debug::Draw::box(p, p + vec3(r.xSectors * 1024, r.info.yBottom - r.info.yTop, r.zSectors * 1024));
            }

            glDepthMask(GL_TRUE);
            Core::setBlending(bmAlpha);
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
            Core::setBlending(bmAlpha);
        }

        void lights(const TR::Level &level) {
        //    int roomIndex = level.entities[lara->entity].room;
        //    int lightIndex = getLightIndex(lara->pos, roomIndex);

            glPointSize(8);
            glBegin(GL_POINTS);
            for (int i = 0; i < level.roomsCount; i++)
                for (int j = 0; j < level.rooms[i].lightsCount; j++) {
                    TR::Room::Light &l = level.rooms[i].lights[j];
                    float a = l.intensity / 8191.0f;
                    vec3 p = vec3(l.x, l.y, l.z);
                    vec4 color = vec4(a, a, a, 1);
                    Debug::Draw::point(p, color);
                    //if (i == roomIndex && j == lightIndex)
                    //    color = vec4(0, 1, 0, 1);
                    Debug::Draw::sphere(p, l.attenuation, color);
                }
            glEnd();
        }

    }
}

#endif
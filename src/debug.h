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

        void debugFloor(const TR::Level &level, const vec3 &f, const vec3 &c, int floorIndex, int boxIndex, bool current) {
            if (floorIndex == 0) return;

            vec3 vf[4] = { f, f + vec3(1024, 0, 0), f + vec3(1024, 0, 1024), f + vec3(0, 0, 1024) };
            vec3 vc[4] = { c, c + vec3(1024, 0, 0), c + vec3(1024, 0, 1024), c + vec3(0, 0, 1024) };

            if (current)
                glColor3f(1, 1, 1);
            else
                glColor3f(0, 1, 0);

            bool isPortal = false;

            TR::FloorData *fd = &level.floors[floorIndex];
            TR::FloorData::Command cmd;
            do {
                cmd = (*fd++).cmd;
                
                switch (cmd.func) {
                    case TR::FD_PORTAL  :
                        isPortal = true;
                        fd++;
                        break; // portal
                    case TR::FD_FLOOR   : // floor & ceiling
                    case TR::FD_CEILING : { 
                        TR::FloorData::Slant slant = (*fd++).slant;
                        int sx = 256 * (int)slant.x;
                        int sz = 256 * (int)slant.z;

                        auto &p = cmd.func == 0x02 ? vf : vc;
                        
                        if (cmd.func == 0x02) { // floor
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
                        } else { // ceiling
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
                        break;
                    }
                    case TR::FD_TRIGGER :  {
                        TR::FloorData::TriggerInfo info = (*fd++).triggerInfo;
                        TR::FloorData::TriggerCommand trigCmd;
                        glColor3f(1, 0, 1);
                        do {
                            trigCmd = (*fd++).triggerCmd; // trigger action
                            switch (trigCmd.func) {
                                case  0 : break; // activate item
                                case  1 : break; // switch to camera
                                case  2 : break; // camera delay
                                case  3 : break; // flip map
                                case  4 : break; // flip on
                                case  5 : break; // flip off
                                case  6 : break; // look at item
                                case  7 : break; // end level
                                case  8 : break; // play soundtrack
                                case  9 : break; // special hadrdcode trigger
                                case 10 : break; // secret found
                                case 11 : break; // clear bodies
                                case 12 : break; // flyby camera sequence
                                case 13 : break; // play cutscene
                            }
                        } while (!trigCmd.end);                       
                        break;
                    }
                    default : 
                        if (!cmd.end)
                            LOG("unknown func %d : %d\n", cmd.func, cmd.sub);
                }
            } while (!cmd.end);

            glBegin(GL_LINE_STRIP);
                for (int i = 0; i < 5; i++)
                    glVertex3fv((GLfloat*)&vf[i % 4]);
            glEnd();

            glColor3f(1, 0, 0);
            glBegin(GL_LINE_STRIP);
                for (int i = 0; i < 5; i++)
                    glVertex3fv((GLfloat*)&vc[i % 4]);  
            glEnd();

            if (isPortal) {
                glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
                glBegin(GL_QUADS);
                for (int i = 3; i >= 0; i--)
                    glVertex3fv((GLfloat*)&vf[i]);
                glEnd();
            }

            if (boxIndex == 0xFFFF) {
                glBegin(GL_LINES);
                    float x = f.x + 512.0f, z = f.z + 512.0f;
                    glVertex3f(x, f.y, z);
                    glVertex3f(x, c.y, z);
                glEnd();                
            }
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

        void debugOverlaps(const TR::Level &level, int boxIndex) {
            glColor4f(1.0f, 1.0f, 0.0f, 0.25f);
            TR::Overlap *o = &level.overlaps[level.boxes[boxIndex].overlap & 0x7FFF];
            do {
                TR::Box &b = level.boxes[o->boxIndex];
                debugBox(b);
            } while (!(o++)->end);
        }

        void debugSectors(const TR::Level &level, const vec3 &pos, int roomIndex) {
            TR::Room &room = level.rooms[roomIndex];

            vec3 p = (pos - vec3(room.info.x, 0, room.info.z)) / vec3(1024, 1, 1024);

            for (int z = 0; z < room.zSectors; z++)
                for (int x = 0; x < room.xSectors; x++) {
                    auto &s = room.sectors[x * room.zSectors + z];
                    float floor = s.floor * 256;
                    /*
                    if (s.boxIndex < 0xFFFF) {
                        auto &b = level.boxes[s.boxIndex];
                    //    floor = b.floor;
                    }
                    */
                    vec3 f(x * 1024 + room.info.x, floor - 1, z * 1024 + room.info.z);
                    vec3 c(x * 1024 + room.info.x, s.ceiling * 256 + 1, z * 1024 + room.info.z);

                    bool current = (int)p.x == x && (int)p.z == z;
                    debugFloor(level, f, c, s.floorIndex, s.boxIndex, current);

                    if (current && s.boxIndex != 0xFFFF && level.boxes[s.boxIndex].overlap != 0xFFFF) {
                        glDisable(GL_DEPTH_TEST);
                        glColor4f(0.0f, 1.0f, 0.0f, 0.25f);
                        debugBox(level.boxes[s.boxIndex]);
                        glColor4f(1.0f, 1.0f, 0.0f, 0.25f);
                        debugOverlaps(level, s.boxIndex);
                        glEnable(GL_DEPTH_TEST);
                    }
                }
        }

        void rooms(const TR::Level &level, const vec3 &pos, int roomIndex) {
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
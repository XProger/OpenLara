#ifndef H_INPUT
#define H_INPUT

#include "utils.h"

enum InputKey { ikNone,
    // keyboard
        ikLeft, ikRight, ikUp, ikDown, ikSpace, ikTab, ikEnter, ikEscape, ikShift, ikCtrl, ikAlt,
        ik0, ik1, ik2, ik3, ik4, ik5, ik6, ik7, ik8, ik9,
        ikA, ikB, ikC, ikD, ikE, ikF, ikG, ikH, ikI, ikJ, ikK, ikL, ikM,
        ikN, ikO, ikP, ikQ, ikR, ikS, ikT, ikU, ikV, ikW, ikX, ikY, ikZ,
    // mouse
        ikMouseL, ikMouseR, ikMouseM,
    // touch
        ikTouchA, ikTouchB, ikTouchC, ikTouchD, ikTouchE, ikTouchF,
    // gamepad
        ikJoyA, ikJoyB, ikJoyX, ikJoyY, ikJoyLB, ikJoyRB, ikJoySelect, ikJoyStart, ikJoyL, ikJoyR, ikJoyLT, ikJoyRT, ikJoyPOV,
        ikJoyLeft, ikJoyRight, ikJoyUp, ikJoyDown,
        ikMAX };

enum ControlKey { cLeft, cRight, cUp, cDown, cJump, cWalk, cAction, cWeapon, cLook, cStepLeft, cStepRight, cRoll, cInventory, cMAX };

namespace Input {

    bool down[ikMAX];

    struct KeySet {
        InputKey key, joy;
    };

    static const KeySet presets[1][cMAX] = {
        {   { ikLeft,   ikJoyLeft   },
            { ikRight,  ikJoyRight  },
            { ikUp,     ikJoyUp     },
            { ikDown,   ikJoyDown   },
            { ikAlt,    ikJoyX      },
            { ikShift,  ikJoyRB     },
            { ikCtrl,   ikJoyA      },
            { ikSpace,  ikJoyY      },
            { ikC,      ikJoyLB     },
            { ikZ,      ikJoyLT     },
            { ikX,      ikJoyRT     },
            { ikA,      ikJoyB      },
            { ikTab,    ikJoySelect },
        },
        /*
        {   { ikA,      ikJoyLeft   },
            { ikD,      ikJoyRight  },
            { ikW,      ikJoyUp     },
            { ikS,      ikJoyDown   },
            { ikSpace,  ikJoyX      },
            { ikShift,  ikJoyRB     },
            { ikP,      ikJoyA      },
            { ikMouseM, ikJoyY      },
            { ikMouseR, ikJoyLB     },
            { ikLeft,   ikJoyLT     },
            { ikRight,  ikJoyRT     },
            { ikUp,     ikJoyB      },
            { ikTab,    ikJoySelect },
        },
        */
    };

    KeySet  controls[cMAX];
    bool    state[cMAX];

    struct {
        vec2 pos;
        struct {
            vec2 L, R, M;
        } start;
    } mouse;

    struct {
        vec2  L, R;
        float LT, RT;
        int   POV;
    } joy;

    struct Touch {
        int  id;
        vec2 start;
        vec2 pos;
    } touch[6];

    struct {
        Basis pivot;
        Basis basis;
        bool  ready;

        void set() {
            if (!ready) {
                pivot = basis;
                ready = true;
            }
        }

        void reset() {
            pivot.identity();
            basis.identity();
            ready = false;
        }

        mat4 getMatrix() {
            Basis b = pivot.inverse() * basis;
            mat4 m;
            m.identity();
            m.setRot(b.rot);
            m.setPos(b.pos);
            return m;
        }
    } head;

    enum TouchButton { bNone, bWeapon, bWalk, bAction, bJump, bMAX };
    enum TouchZone   { zMove, zLook, zButton, zMAX };

    float       touchTimerVis, touchTimerTap;
    InputKey    touchKey[zMAX];
    TouchButton btn;
    vec2        btnPos[bMAX];
    float       btnRadius;
    bool        doubleTap;

    void setDown(InputKey key, bool value) {
        if (down[key] == value)
            return;

        if (value)
            switch (key) {
                case ikMouseL : mouse.start.L = mouse.pos;  break;
                case ikMouseR : mouse.start.R = mouse.pos;  break;
                case ikMouseM : mouse.start.M = mouse.pos;  break;
                case ikTouchA :
                case ikTouchB :
                case ikTouchC :
                case ikTouchD :
                case ikTouchE :
                case ikTouchF : touch[key - ikTouchA].start = touch[key - ikTouchA].pos; break;
                default       : ;
            }
        down[key] = value;
    }

    void setPos(InputKey key, const vec2 &pos) {
        switch (key) {
            case ikMouseL :
            case ikMouseR :
            case ikMouseM : mouse.pos = pos;         return;
            case ikJoyL   : joy.L     = pos;         return;
            case ikJoyR   : joy.R     = pos;         return;
            case ikJoyLT  : joy.LT    = pos.x;       break;
            case ikJoyRT  : joy.RT    = pos.x;       break;
            case ikJoyPOV : joy.POV   = (int)pos.x;  break;
            case ikTouchA :
            case ikTouchB :
            case ikTouchC :
            case ikTouchD :
            case ikTouchE :
            case ikTouchF : touch[key - ikTouchA].pos = pos; return;
            default       : return;
        }
        setDown(key, pos.x > 0.0f); // gamepad LT, RT, POV auto-down state
    }

    InputKey getTouch(int id) {
        for (int i = 0; i < COUNT(touch); i++)
            if (down[ikTouchA + i] && touch[i].id == id)
                return InputKey(ikTouchA + i);

        for (int i = 0; i < COUNT(touch); i++)
            if (!down[ikTouchA + i]) {
                touch[i].id = id;
                return InputKey(ikTouchA + i);
            }

        return ikNone;
    }

    void reset() {
        memset(down,    0, sizeof(down));
        memset(&mouse,  0, sizeof(mouse));
        memset(&joy,    0, sizeof(joy));
        memset(&touch,  0, sizeof(touch));
        head.reset();
    }

    void init() {
        reset();
        for (int i = 0; i < cMAX; i++)
            controls[i] = presets[0][i];

        touchTimerVis    = 0.0f;
        touchTimerTap    = 0.0f;
        doubleTap        = false;
        touchKey[zMove]  = touchKey[zLook] = touchKey[zButton] = ikNone;
    }

    bool checkTouchZone(TouchZone zone) {
        InputKey &t = touchKey[zone];
        if (t != ikNone && !down[t]) {
            t = ikNone;
            return true;
        }
        return false;
    }

    void getTouchDir(InputKey key, vec2 &dir) {
        vec2 delta = vec2(0.0f);
        if (key == ikNone)
            return;

        Touch &t = touch[key - ikTouchA];
        vec2 d = t.pos - t.start;                
        float len = d.length();
        if (len > EPS)
            delta = d * (min(len / 100.0f, 1.0f) / len);

        dir = delta;
    }

    void update() {
        int p = joy.POV;
        setDown(ikJoyUp,    p == 8 || p == 1 || p == 2);
        setDown(ikJoyRight, p == 2 || p == 3 || p == 4);
        setDown(ikJoyDown,  p == 4 || p == 5 || p == 6);
        setDown(ikJoyLeft,  p == 6 || p == 7 || p == 8);

        for (int i = 0; i < cMAX; i++) {
            KeySet &c = controls[i];
            state[i] = (c.key != ikNone && down[c.key]) || (c.joy != ikNone && down[c.joy]);
        }

    // update touch controls
        if (touchTimerTap > 0.0f)
            touchTimerTap = max(0.0f, touchTimerTap - Core::deltaTime);

        if (touchKey[zMove] != ikNone || touchKey[zLook] != ikNone || touchKey[zButton] != ikNone)
            touchTimerVis = 30.0f;
        else
            if (touchTimerVis > 0.0f)
                touchTimerVis = max(0.0f, touchTimerVis - Core::deltaTime);

    // update buttons
        float offset = Core::height * 0.25f;
        float radius = offset; 
        vec2  center = vec2(Core::width - offset * 0.7f, Core::height - offset * 0.7f);

        btnPos[bWeapon] = center;
        btnPos[bJump]   = center + vec2(cosf(-PI * 0.5f), sinf(-PI * 0.5f)) * radius;
        btnPos[bAction] = center + vec2(cosf(-PI * 3.0f / 4.0f), sinf(-PI * 3.0f / 4.0f)) * radius;
        btnPos[bWalk]   = center + vec2(cosf(-PI), sinf(-PI)) * radius;
        btnRadius       = Core::height * (25.0f / 1080.0f);

    // touch update
        if (checkTouchZone(zMove))
            joy.L = vec2(0.0f);

        if (checkTouchZone(zLook))
            joy.R = vec2(0.0f);

        if (checkTouchZone(zButton))
            btn = bNone;

        if (doubleTap)
            doubleTap = false;

        float zoneSize = Core::width / 3.0f;

        for (int i = 0; i < COUNT(touch); i++) {
            InputKey key = InputKey(i + ikTouchA);

            if (!down[key]) continue;
            if (key == touchKey[zMove] || key == touchKey[zLook] || key == touchKey[zButton]) continue;

            int zone = clamp(int(touch[i].pos.x / zoneSize), 0, 2);
            InputKey &t = touchKey[zone];

            if (t == ikNone) {
                t = key;
                if (zone == zMove) {
                    if (touchTimerTap > 0.0f && touchTimerTap < 0.3f) { // 100 ms gap to reduce false positives (bad touch sensors)
                        doubleTap = true;
                        touchTimerTap = 0.0f;
                    } else
                        touchTimerTap = 0.3f;
                }
            }
        }

    // set active touches as gamepad controls
        getTouchDir(touchKey[zMove], joy.L);
        getTouchDir(touchKey[zLook], joy.R);

        if (touchKey[zButton] != ikNone) {
            vec2 pos = touch[touchKey[zButton] - ikTouchA].pos;

            btn = bMAX;
            float minDist = btnRadius * 8.0f;
            for (int i = 0; i < bMAX; i++) {
                float d = (pos - btnPos[i]).length();
                if (d < minDist) {
                    minDist = d;
                    btn = TouchButton(i);
                }
            }

            switch (btn) {
                case bWeapon : state[cWeapon] = true; break;
                case bWalk   : state[cWalk]   = true; break;
                case bAction : state[cAction] = true; break;
                case bJump   : state[cJump]   = true; break;
                default      : ;
            }
        }

        if (doubleTap)
            state[cRoll] = true;
    }
}

#endif
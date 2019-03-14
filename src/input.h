#ifndef H_INPUT
#define H_INPUT

#include "core.h"
#include "utils.h"

#define INPUT_JOY_COUNT 4
#define MAX_PLAYERS     COUNT(Core::settings.controls)

namespace Input {
    InputKey lastKey;
    bool down[ikMAX];
    bool state[MAX_PLAYERS][cMAX];
    ControlKey lastState[MAX_PLAYERS];

    struct Mouse {
        vec2 pos;
        struct {
            vec2 L, R, M;
        } start;
    } mouse;

    struct Joystick {
        vec2   L, R;
        float  LT, RT;
        JoyKey lastKey;
        bool   down[jkMAX];
    } joy[INPUT_JOY_COUNT];

    struct Touch {
        int  id;
        vec2 start;
        vec2 pos;
    } touch[6];

    struct HMD {
        mat4 head;
        mat4 eye[2];
        mat4 proj[2];
        mat4 controllers[2];
        vec3 zero;
        bool ready;
        bool state[cMAX];

        void setView(const mat4 &pL, const mat4 &pR, const mat4 &vL, const mat4 &vR) {
            proj[0] = pL;
            proj[1] = pR;
            eye[0]  = vL;
            eye[1]  = vR;
        }

        void reset() {
            eye[0].identity();
            eye[1].identity();
            proj[0].identity();
            proj[1].identity();
        }
    } hmd;

    enum TouchButton { bMove, bWeapon, bWalk, bAction, bJump, bInventory, bMAX };
    enum TouchZone   { zMove, zLook, zButton, zMAX };

    float       touchTimerVis, touchTimerTap;
    InputKey    touchKey[zMAX];

    TouchButton btn;
    vec2        btnPos[bMAX];
    bool        btnEnable[bMAX];
    float       btnRadius;
    bool        doubleTap;

    void setDown(InputKey key, bool value, int index = 0) {
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

        if (value && key <= ikBack) {
            lastKey = key;
            touchTimerVis = 0.0f;
        }
    }

    void setPos(InputKey key, const vec2 &pos) {
        switch (key) {
            case ikMouseL :
            case ikMouseR :
            case ikMouseM : mouse.pos      = pos;         return;
            case ikTouchA :
            case ikTouchB :
            case ikTouchC :
            case ikTouchD :
            case ikTouchE :
            case ikTouchF : touch[key - ikTouchA].pos = pos; return;
            default       : return;
        }
    }

    void setJoyDown(int index, JoyKey key, bool value) {
        if (joy[index].down[key] == value)
            return;

        joy[index].down[key] = value;

        if (value) {
            joy[index].lastKey = key;
            touchTimerVis = 0.0f;
        }
    }

    void setJoyPos(int index, JoyKey key, const vec2 &pos) {
        switch (key) {
            case jkL   : joy[index].L   = pos;         return;
            case jkR   : joy[index].R   = pos;         return;
            case jkLT  : joy[index].LT  = pos.x;       break;
            case jkRT  : joy[index].RT  = pos.x;       break;
            default    : return;
        }
        setJoyDown(index, key, pos.x > EPS); // gamepad LT, RT auto-down state
    }

    void setJoyVibration(int playerIndex, float L, float R) {
        if (!Core::settings.controls[playerIndex].vibration)
            return;
        osJoyVibrate(Core::settings.controls[playerIndex].joyIndex, L, R);
    }
    
    void stopJoyVibration() {
        osJoyVibrate(Core::settings.controls[0].joyIndex, 0.0f, 0.0f);
        osJoyVibrate(Core::settings.controls[1].joyIndex, 0.0f, 0.0f);
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
        hmd.reset();
    }

    void init() {
        reset();
        hmd.ready        = false;
        hmd.zero         = vec3(INF, INF, INF);
        touchTimerVis    = 0.0f;
        touchTimerTap    = 0.0f;
        doubleTap        = false;
        touchKey[zMove]  = touchKey[zLook] = touchKey[zButton] = ikNone;
        memset(btnEnable, 1, sizeof(btnEnable));
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

    void setState(int playerIndex, ControlKey key, bool down) {
        if (down && !state[playerIndex][key])
            lastState[playerIndex] = key;
        state[playerIndex][key] = down;
    }

    void update() {
        bool newState[MAX_PLAYERS][cMAX];

        for (int j = 0; j < COUNT(Core::settings.controls); j++) {
            lastState[j] = cMAX;
            Core::Settings::Controls &ctrl = Core::settings.controls[j];
            for (int i = 0; i < cMAX; i++) {
                KeySet &c = ctrl.keys[i];
                newState[j][i] = (c.key != ikNone && down[c.key]) || (c.joy != jkNone && joy[ctrl.joyIndex].down[c.joy]) || hmd.state[i];
            }
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

        btnRadius          = Core::height * (25.0f / 1080.0f);
        btnPos[bWeapon]    = center;
        btnPos[bJump]      = center + vec2(cosf(-PI * 0.5f), sinf(-PI * 0.5f)) * radius;
        btnPos[bAction]    = center + vec2(cosf(-PI * 3.0f / 4.0f), sinf(-PI * 3.0f / 4.0f)) * radius;
        btnPos[bWalk]      = center + vec2(cosf(-PI), sinf(-PI)) * radius;
        btnPos[bInventory] = vec2(Core::width - btnRadius * 8.0f, btnRadius * 4.0f);

    // touch update
        Joystick &joy = Input::joy[Core::settings.controls[0].joyIndex];

        if (checkTouchZone(zMove))
            joy.L = vec2(0.0f);

        if (checkTouchZone(zLook))
            joy.L = vec2(0.0f);

        if (checkTouchZone(zButton))
            btn = bMove; // no active buttons == bNone

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

        newState[0][cLook] |= touchKey[zLook] != ikNone;

    // set active touches as gamepad controls
        getTouchDir(touchKey[zMove], joy.L);
        getTouchDir(touchKey[zLook], joy.R);

        if (touchKey[zButton] != ikNone) {
            vec2 pos = touch[touchKey[zButton] - ikTouchA].pos;

            btn = bMAX;
            float minDist = btnRadius * 8.0f;
            for (int i = 0; i < bMAX; i++)
                if (btnEnable[i]) {
                    float d = (pos - btnPos[i]).length();
                    if (d < minDist) {
                        minDist = d;
                        btn = TouchButton(i);
                    }
                }

            switch (btn) {
                case bWeapon    : newState[0][cWeapon   ] = true; break;
                case bWalk      : newState[0][cWalk     ] = true; break;
                case bAction    : newState[0][cAction   ] = true; break;
                case bJump      : newState[0][cJump     ] = true; break;
                case bInventory : newState[0][cInventory] = true; break;
                default         : ;
            }
        }

        if (doubleTap)
            newState[0][cRoll] = true;

        for (int j = 0; j < COUNT(Core::settings.controls); j++)
            for (int i = 0; i < cMAX; i++)
                setState(j, ControlKey(i), newState[j][i]);
    }
}

#endif

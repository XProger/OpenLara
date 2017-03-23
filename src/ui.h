#ifndef H_UI
#define H_UI

#include "core.h"
#include "controller.h"

struct UI {
    enum TouchButton { bWeapon, bWalk, bAction, bJump, bMAX };
    enum TouchZone   { zMove, zLook, zButton, zMAX };

    IGame       *game;
    float       touchTimerVis, touchTimerTap;
    InputKey    touch[zMAX];
    TouchButton btn;
    vec2        btnPos[bMAX];
    float       btnRadius;
    bool        doubleTap;

    UI(IGame *game) : game(game), touchTimerVis(0.0f), touchTimerTap(0.0f), doubleTap(false) {
        touch[zMove] = touch[zLook] = touch[zButton] = ikNone;
    }

    bool checkTouchZone(TouchZone zone) {
        InputKey &t = touch[zone];
        if (t != ikNone && !Input::down[t]) {
            t = ikNone;
            return true;
        }
        return false;
    }

    void getTouchDir(InputKey touch, vec2 &dir) {
        vec2 delta = vec2(0.0f);
        if (touch == ikNone)
            return;

        Input::Touch &t = Input::touch[touch - ikTouchA];
        vec2 d = t.pos - t.start;                
        float len = d.length();
        if (len > EPS)
            delta = d * (min(len / 100.0f, 1.0f) / len);

        dir = delta;
    }

    void getTouchButton(const vec2 &pos) {
        btn = bMAX;
        float minDist = 1000.0f;
        for (int i = 0; i < bMAX; i++) {
            float d = (pos - btnPos[i]).length();
            if (d < minDist) {
                minDist = d;
                btn = TouchButton(i);
            }
        }
    }

    void touchSetDown(bool down) {
        switch (btn) {
            case bWeapon : Input::setDown(ikJoyY,  down); break;
            case bWalk   : Input::setDown(ikJoyLB, down); break;
            case bAction : Input::setDown(ikJoyA,  down); break;
            case bJump   : Input::setDown(ikJoyX,  down); break;
            default      : ;
        }        
    }

    void update() {
        if (touch[zMove] != ikNone || touch[zLook] != ikNone || touch[zButton] != ikNone) {
            touchTimerVis = 30.0f;

            if (touchTimerTap > 0.0f)
                touchTimerTap = max(0.0f, touchTimerTap - Core::deltaTime);

        } else
            if (touchTimerVis > 0.0f)
                touchTimerVis = max(0.0f, touchTimerVis - Core::deltaTime);

    // update buttons
        float offset = Core::height * 0.25f;
        float radius = offset; 
        vec2  center = vec2(Core::width - offset * 0.7f, Core::height - offset * 0.7f);

        btnPos[bWeapon] = center;
        btnPos[bJump]   = center + vec2(cos(-PI * 0.5f), sin(-PI * 0.5f)) * radius;
        btnPos[bAction] = center + vec2(cos(-PI * 3.0f / 4.0f), sin(-PI * 3.0f / 4.0f)) * radius;
        btnPos[bWalk]   = center + vec2(cos(-PI), sin(-PI)) * radius;
        btnRadius       = Core::height * (25.0f / 1080.0f);

    // touch update
        if (checkTouchZone(zMove))
            Input::joy.L = vec2(0.0f);

        if (checkTouchZone(zLook))
            Input::joy.R = vec2(0.0f);

        if (checkTouchZone(zButton))
            touchSetDown(false);

        if (doubleTap) {
            doubleTap = false;
            Input::setDown(ikJoyB, false);
        }

        float zoneSize = Core::width / 3.0f;

        for (int i = 0; i < COUNT(Input::touch); i++) {
            InputKey key = InputKey(i + ikTouchA);

            if (!Input::down[key]) continue;
            if (key == touch[zMove] || key == touch[zLook] || key == touch[zButton]) continue;

            int zone = clamp(int(Input::touch[i].pos.x / zoneSize), 0, 2);
            InputKey &t = touch[zone];

            if (t == ikNone) {
                t = key;
                if (zone == zMove) {
                    if (touchTimerTap > 0.0f && touchTimerTap < 0.4f) { // 100 ms gap to reduce false positives (bad touch sensors)
                        doubleTap = true;
                        touchTimerTap = 0.0f;
                    } else
                        touchTimerTap = 0.5f;
                }
            }
        }

    // set active touches as gamepad controls
        getTouchDir(touch[zMove], Input::joy.L);
        getTouchDir(touch[zLook], Input::joy.R);
        if (touch[zButton] != ikNone) {
            getTouchButton(Input::touch[touch[zButton] - ikTouchA].pos);
            touchSetDown(true);
        }
        if (doubleTap)
            Input::setDown(ikJoyB, true);
    }

    void renderControl(const vec2 &pos, const vec2 &size, const vec4 &color) {
        Core::active.shader->setParam(uPosScale, vec4(pos, size * vec2(1.0f / 32767.0f)));
        Core::active.shader->setParam(uMaterial, color);
        game->getMesh()->renderCircle();
    }

    void renderTouch() {
        if (touchTimerVis <= 0.0f) return;

        Core::setDepthTest(false);
        Core::setBlending(bmAlpha);
        Core::setCulling(cfNone);

        Core::mViewProj = mat4(0.0f, float(Core::width), float(Core::height), 0.0f, 0.0f, 1.0f);
        
        game->setShader(Core::passGUI, Shader::NONE);

        float offset = Core::height * 0.25f;

        vec2 pos = vec2(offset, Core::height - offset);
        if (Input::down[touch[zMove]]) {
            Input::Touch &t = Input::touch[touch[zMove] - ikTouchA];
            renderControl(t.pos, vec2(btnRadius), vec4(0.5f));
            pos = t.start;
        }
        renderControl(pos, vec2(btnRadius), vec4(0.5f));

        for (int i = 0; i < bMAX; i++)
            renderControl(btnPos[i], vec2(btnRadius), vec4(0.5f));

        Core::setCulling(cfFront);
        Core::setBlending(bmNone);
        Core::setDepthTest(true);
    }

    void renderBar(const vec2 &pos, const vec2 &size, float value) {
        //
    }
};

#endif
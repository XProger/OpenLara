#ifndef H_INPUT
#define H_INPUT

#include "utils.h"

enum InputKey { ikNone,
    // keyboard
        ikLeft, ikRight, ikUp, ikDown, ikSpace, ikEnter, ikEscape, ikShift, ikCtrl, ikAlt,
        ik0, ik1, ik2, ik3, ik4, ik5, ik6, ik7, ik8, ik9,
        ikA, ikB, ikC, ikD, ikE, ikF, ikG, ikH, ikI, ikJ, ikK, ikL, ikM,
        ikN, ikO, ikP, ikQ, ikR, ikS, ikT, ikU, ikV, ikW, ikX, ikY, ikZ,
    // mouse
        ikMouseL, ikMouseR, ikMouseM,
    // touch
        ikTouchA, ikTouchB,
    // gamepad
        ikJoyA, ikJoyB, ikJoyX, ikJoyY, ikJoyLB, ikJoyRB, ikJoyL, ikJoyR, ikJoySelect, ikJoyStart, ikJoyLT, ikJoyRT, ikJoyDP,
        ikMAX };

namespace Input {

    bool down[ikMAX];

    struct {
        vec2 pos;
        struct {
            vec2 L, R, M;
        } start;
    } mouse;

    struct {
        vec2  L, R;
        float LT, RT, DP;
    } joy;

    struct {
        vec2 A, B;

        struct {
            vec2 A, B;
        } start;
    } touch;

    void reset() {
        memset(down,    0, sizeof(down));
        memset(&mouse,  0, sizeof(mouse));
        memset(&joy,    0, sizeof(joy));
        memset(&touch,  0, sizeof(touch));
    }

    void setDown(InputKey key, bool value) {
        if (down[key] == value)
            return;

        if (value)
            switch (key) {
                case ikMouseL : mouse.start.L = mouse.pos;  break;
                case ikMouseR : mouse.start.R = mouse.pos;  break;
                case ikMouseM : mouse.start.M = mouse.pos;  break;
                case ikTouchA : touch.start.A = touch.A;    break;
                case ikTouchB : touch.start.B = touch.B;    break;
            }

        down[key] = value;
    }

    void setPos(InputKey key, const vec2 &pos) {
        switch (key) {
            case ikMouseL :
            case ikMouseR :
            case ikMouseM : mouse.pos = pos;    break;
            case ikJoyL   : joy.L     = pos;    break;
            case ikJoyR   : joy.R     = pos;    break;
            case ikJoyLT  : joy.LT    = pos.x;  break;
            case ikJoyRT  : joy.RT    = pos.x;  break;
            case ikJoyDP  : joy.DP    = pos.x;  break;
            case ikTouchA : touch.A   = pos;    break;
            case ikTouchB : touch.B   = pos;    break;
        }
    }
}

#endif
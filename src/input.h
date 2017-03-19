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
        ikTouchA, ikTouchB, ikTouchC, ikTouchD, ikTouchE, ikTouchF,
    // gamepad
        ikJoyA, ikJoyB, ikJoyX, ikJoyY, ikJoyLB, ikJoyRB, ikJoySelect, ikJoyStart, ikJoyL, ikJoyR, ikJoyLT, ikJoyRT, ikJoyPOV,
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
        float LT, RT;
		int   POV;
    } joy;

    struct Touch {
        int  id;
        vec2 start;
        vec2 pos;
    } touch[6];

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
}

#endif
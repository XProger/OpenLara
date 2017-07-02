#ifndef H_UI
#define H_UI

#include "core.h"
#include "controller.h"

namespace UI {
    IGame *game;
    float width;
    float helpTipTime;
    bool  showHelp;

    const static uint8 char_width[110] = {
        14, 11, 11, 11, 11, 11, 11, 13, 8, 11, 12, 11, 13, 13, 12, 11, 12, 12, 11, 12, 13, 13, 13, 12, 
        12, 11, 9, 9, 9, 9, 9, 9, 9, 9, 5, 9, 9, 5, 12, 10, 9, 9, 9, 8, 9, 8, 9, 9, 11, 9, 9, 9, 12, 8,
        10, 10, 10, 10, 10, 9, 10, 10, 5, 5, 5, 11, 9, 10, 8, 6, 6, 7, 7, 3, 11, 8, 13, 16, 9, 4, 12, 12, 
        7, 5, 7, 7, 7, 7, 7, 7, 7, 7, 16, 14, 14, 14, 16, 16, 16, 16, 16, 12, 14, 8, 8, 8, 8, 8, 8, 8 }; 
        
    static const uint8 char_map[102] = {
            0, 64, 66, 78, 77, 74, 78, 79, 69, 70, 92, 72, 63, 71, 62, 68, 52, 53, 54, 55, 56, 57, 58, 59, 
        60, 61, 73, 73, 66, 74, 75, 65, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 
        18, 19, 20, 21, 22, 23, 24, 25, 80, 76, 81, 97, 98, 77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 
        37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 100, 101, 102, 67, 0, 0, 0, 0, 0, 0, 0 };

    enum Align  { aLeft, aRight, aCenter };

    inline int charRemap(char c) {
        return c > 10 ? (c > 15 ? char_map[c - 32] : c + 91) : c + 81; 
    }

    vec2 getTextSize(const char *text) {
        int x = 0, w = 0, h = 16;

        while (char c = *text++) {
            if (c == ' ' || c == '_') {
                x += 6;
            } else if (c == '@') {
                w = max(w, x);
                h += 16;
                x = 0;
            } else 
                x += char_width[charRemap(c)] + 1;
        }
        w = max(w, x);

        return vec2(float(w), float(h));
    }

    #define MAX_CHARS DYN_MESH_QUADS

    struct {
        Vertex  vertices[MAX_CHARS * 4];
        Index   indices[MAX_CHARS * 6];
        int     iCount;
        int     vCount;
    } buffer;

    void begin() {
        Core::setDepthTest(false);
        Core::setBlending(bmAlpha);
        Core::setCulling(cfNone);
        game->setupBinding();

        float aspect = float(Core::width) / float(Core::height);
        width = 480 * aspect;
        Core::mViewProj = mat4(0.0f, width, 480, 0.0f, 0.0f, 1.0f);

        game->setShader(Core::passGUI, Shader::DEFAULT);
        Core::active.shader->setParam(uMaterial, vec4(1));
        Core::active.shader->setParam(uPosScale, vec4(0, 0, 1, 1));

        buffer.iCount = buffer.vCount = 0;
    }

    void flush() {
        if (buffer.iCount > 0) {
            game->getMesh()->renderBuffer(buffer.indices, buffer.iCount, buffer.vertices, buffer.vCount);
            buffer.iCount = buffer.vCount = 0;
        }
    }

    void end() {
        flush();
        Core::setCulling(cfFront);
        Core::setBlending(bmNone);
        Core::setDepthTest(true);
    }

    void textOut(const vec2 &pos, const char *text, Align align = aLeft, float width = 0) {        
        if (!text) return;
       
        TR::Level *level = game->getLevel();
        MeshBuilder *mesh = game->getMesh();

        int seq = level->extra.glyphSeq;

        int x = int(pos.x);
        int y = int(pos.y);

        if (align == aCenter)
            x += int((width - getTextSize(text).x) / 2);

        if (align == aRight)
            x += int(width - getTextSize(text).x);

        int left = x;

        while (char c = *text++) {
            if (c == ' ' || c == '_') {
                x += 6;
                continue;
            }

            if (c == '@') {
                x = left;
                y += 16;
                continue;
            }

            int frame = charRemap(c);

            if (buffer.iCount == MAX_CHARS * 6)
                flush();

            TR::SpriteTexture &sprite = level->spriteTextures[level->spriteSequences[seq].sStart + frame];
            mesh->addSprite(buffer.indices, buffer.vertices, buffer.iCount, buffer.vCount, 0, x, y, 0, sprite, 255, true);

            x += char_width[frame] + 1;
        }
    }

    #undef MAX_CHARS


    void init(IGame *game) {
        UI::game = game;
        showHelp = false;
        helpTipTime = 5.0f;
    }

    void update() {
        if (Input::down[ikH]) {
            Input::down[ikH] = false;
            showHelp = !showHelp;
            helpTipTime = 0.0f;
        }
        if (helpTipTime > 0.0f)
            helpTipTime -= Core::deltaTime;
    }

    void renderControl(const vec2 &pos, float size, bool active) {
        Core::active.shader->setParam(uPosScale, vec4(pos, vec2(size * (active ? 2.0f : 1.0f) / 32767.0f)));
        Core::active.shader->setParam(uMaterial, vec4(active ? 0.7f : 0.5f));
        game->getMesh()->renderCircle();
    }

    void renderTouch() {
        game->setupBinding();

        if (Input::touchTimerVis <= 0.0f) return;

        Core::setDepthTest(false);
        Core::setBlending(bmAlpha);
        Core::setCulling(cfNone);

        Core::mViewProj = mat4(0.0f, float(Core::width), float(Core::height), 0.0f, 0.0f, 1.0f);
        
        game->setShader(Core::passGUI, Shader::DEFAULT);

        float offset = Core::height * 0.25f;

        vec2 pos = vec2(offset, Core::height - offset);
        if (Input::down[Input::touchKey[Input::zMove]]) {
            Input::Touch &t = Input::touch[Input::touchKey[Input::zMove] - ikTouchA];
            renderControl(t.pos, Input::btnRadius, true);
            pos = t.start;
        }
        renderControl(pos, Input::btnRadius, false);

        for (int i = Input::bWeapon; i < Input::bMAX; i++)
            renderControl(Input::btnPos[i], Input::btnRadius, Input::btn == i);

        Core::setCulling(cfFront);
        Core::setBlending(bmNone);
        Core::setDepthTest(true);
    }

    void renderBar(int type, const vec2 &pos, const vec2 &size, float value) {
        MeshBuilder *mesh = game->getMesh();

        mesh->addFrame(buffer.indices, buffer.vertices, buffer.iCount, buffer.vCount, pos - 2.0f, size + 4.0f, 0xFF4C504C, 0xFF748474);
        mesh->addBar(buffer.indices, buffer.vertices, buffer.iCount, buffer.vCount, type, pos - 1.0f, size + 2.0f, 0x80000000);
        if (value > 0.0f)
            mesh->addBar(buffer.indices, buffer.vertices, buffer.iCount, buffer.vCount, type, pos, vec2(size.x * value, size.y), 0xFFFFFFFF);
    }

const char *helpText = \
"Controls gamepad, touch and keyboard:@"\
" H - Show or hide this help@"\
" TAB - Inventory@"\
" LEFT - Left@"\
" RIGHT - Right@"\
" UP - Run@"\
" DOWN - Back@"\
" SHIFT - Walk@"\
" SPACE - Draw Weapon@"\
" CTRL - Action@"\
" ALT - Jump@"\
" Z - Step Left@"\
" X - Step Right@"\
" A - Roll@"\
" C - Look # not implemented #@"\
" V - First Person View@@"
"Actions:@"\
" Out of water - Run + Action@"\
" Handstand - Run + Walk@"\
" Swan dive - Run + Walk + jump@"\
" DOZY on - Look + Step Right + Action + Jump@"\
" DOZY off - Walk@";

    void renderHelp() {
        if (showHelp)
            textOut(vec2(0, 64), helpText, aRight, width - 32);
        else
            if (helpTipTime > 0.0f)
                textOut(vec2(0, 480 - 32), "Press H for help", aCenter, width);
    }
};

#endif
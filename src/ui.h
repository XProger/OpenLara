#ifndef H_UI
#define H_UI

#include "core.h"
#include "mesh.h"
#include "controller.h"

#define PICKUP_SHOW_TIME 5.0f
#define SUBTITLES_SPEED  0.1f

#ifdef _OS_PSV
    #define UI_SHOW_FPS
#endif

#include "lang.h"

#ifdef _NAPI_SOCKET
extern char command[256];
#endif

namespace UI {
    IGame    *game;
    float    width, height;
    float    helpTipTime;
    float    hintTime;
    float    subsTime;
    int      subsPartTime;
    int      subsPartLength;
    int      subsPos;
    int      subsLength;

    StringID hintStr;
    StringID subsStr;

    bool     showHelp;

    struct PickupItem {
        float      time;
        vec2       pos;
        int        playerIndex;
        int        modelIndex;
        Animation *animation;
    };

    Array<PickupItem> pickups;

    int advGlyphsStart;

    #define CYR_MAP           "ÁÃÄÆÇÈËÏÓÔÖ×ØÙÚÛÜÝÞßáâãäæçêëìíïòôö÷øùúûüýþÿ"
    #define CYR_MAP_COUNT     COUNT(CYR_MAP)
    #define CYR_MAP_START     102
    #define CYR_MAP_UPPERCASE 20

    const static uint8 char_width[110 + CYR_MAP_COUNT] = {
        14, 11, 11, 11, 11, 11, 11, 13, 8, 11, 12, 11, 13, 13, 12, 11, 12, 12, 11, 12, 13, 13, 13, 12, 12, 11, // A-Z
        9, 9, 9, 9, 9, 9, 9, 9, 5, 9, 9, 5, 12, 10, 9, 9, 9, 8, 9, 8, 9, 9, 11, 9, 9, 9, // a-z
        12, 8, 10, 10, 10, 10, 10, 9, 10, 10, // 0-9
        5, 5, 5, 11, 9, 7, 8, 6, 0, 7, 7, 3, 8, 8, 13, 7, 9, 4, 12, 12, 
        7, 5, 7, 7, 7, 7, 7, 7, 7, 7, 16, 14, 14, 14, 16, 16, 16, 16, 16, 12, 14, 8, 8, 8, 8, 8, 8, 8,
    // cyrillic
        11, 11, 11, 13, 10, 13, 11, 11, 12, 12, 11,  9, 13, 13, 10, 13, // ÁÃÄÆÇÈËÏÓÔÖ×ØÙÚÛ
         9, 11, 12, 11, 10,  9,  8, 10, 11,  9, 10, 10, 11,  9, 10, 12, // ÜÝÞßáâãäæçêëìíïò
        10, 10,  9, 11, 12,  9, 11,  8,  9, 13,  9                      // ôö÷øùúûüýþÿ
    }; 
        
    static const uint8 char_map[102 + 33*2] = {
            0, 64, 66, 78, 77, 74, 78, 79, 69, 70, 92, 72, 63, 71, 62, 68, 52, 53, 54, 55, 56, 57, 58, 59, 
        60, 61, 73, 73, 66, 74, 75, 65, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 
        18, 19, 20, 21, 22, 23, 24, 25, 80, 76, 81, 97, 98, 77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 
        37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 100, 101, 102, 67, 0, 0, 0, 0, 0, 0, 0,
    // cyrillic
        0, 110, 0, 111, 112, 0, 113, 114, 115, 0, 0, 116, 0, 0, 0, 117, 0, 0, 0, 118, 119, 0, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
        0, 130, 131, 132, 133, 0, 134, 135, 0, 0, 136, 137, 138, 139, 0, 140, 0, 0, 141, 0, 142, 0, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152,
    };

    enum Align  { aLeft, aRight, aCenter, aCenterV };

    inline char remapCyrillic(char c) {
        if ((c >= 'À' && c <= 'ß') || (c >= 'à' && c <= 'ÿ')) {
            switch (c) {
                case 'à' : return 'a';
                case 'å' : return 'e';
                case '¸' : return 'e';
                case 'è' : return 'u';
                case 'é' : return 'u';
                case 'î' : return 'o';
                case 'ð' : return 'p';
                case 'ñ' : return 'c';
                case 'ó' : return 'y';
                case 'õ' : return 'x';
                case 'À' : return 'A';
                case 'Â' : return 'B';
                case 'Å' : return 'E';
                case '¨' : return 'E';
                case 'Ê' : return 'K';
                case 'Ì' : return 'M';
                case 'Í' : return 'H';
                case 'Î' : return 'O';
                case 'Ð' : return 'P';
                case 'Ñ' : return 'C';
                case 'Ò' : return 'T';
                case 'Õ' : return 'X';
            }
            return c;
        }
        return c;
    }

    inline int charRemap(char c) {
        if ((c >= 'À' && c <= 'ß') || (c >= 'à' && c <= 'ÿ')) {
            return char_map[CYR_MAP_START + (c - 'À')];
        }

        if (c < 11)
            return c + 81;
        if (c < 16)
            return c + 91;
        ASSERT(c >= 32)
        return char_map[c - 32];
    }

    inline bool skipChar(char c) {
        return c == '~' || c == '$' || c == '(' || c == ')' || c == '|' || c == '/' || c == '*' || c == '{';
    }

    inline bool upperCase(int index) {
        return index < 26 || (index >= 110 && (index < 110 + CYR_MAP_UPPERCASE));
    }

    void patchGlyphs(TR::Level &level) {
        UI::advGlyphsStart = level.spriteTexturesCount;

        TR::TextureInfo cyrSprites[CYR_MAP_COUNT];
        for (int i = 0; i < COUNT(cyrSprites); i++) {
            int idx = 110 + i; // mapped index
            int w = char_width[idx];
            int h = upperCase(idx) ? 13 : 9;
            int o = 0;
            char c = CYR_MAP[i];

            if (c == 'á' || c == 'ä') h = 14;
            if (c == 'Ö' || c == 'Ù' || c == 'ö' || c == 'ù') { o = 1; h++; }
            if (c == 'ô') { o = 2; h += 2; }

            cyrSprites[i] = TR::TextureInfo(TR::TEX_TYPE_SPRITE, 0, -h + o, w, o, (i % 16) * 16, (i / 16) * 16 + (16 - h), w, h);
        }

        // add additional sprites for Cyrillyc glyphs
        int              newSpritesCount = level.spriteTexturesCount + COUNT(cyrSprites);
        TR::TextureInfo *newSprites      = new TR::TextureInfo[newSpritesCount];

        memcpy(newSprites,                             level.spriteTextures, sizeof(TR::TextureInfo) * level.spriteTexturesCount);
        memcpy(newSprites + level.spriteTexturesCount, cyrSprites,           sizeof(TR::TextureInfo) * COUNT(cyrSprites));

        delete[] level.spriteTextures;

        level.spriteTexturesCount = newSpritesCount;
        level.spriteTextures      = newSprites;
    }

    short2 getLineSize(const char *text) {
        int  x = 0;

        while (char c = *text++) {
            if (c == '[') break;
            c = remapCyrillic(c);
            if (c == '\xBF') c = '?';
            if (c == '\xA1') c = '!';

            if (skipChar(c)) {
                //
            } else if (c == ' ' || c == '_') {
                x += 6;
            } else if (c == '@') {
                break;
            } else {
                x += char_width[charRemap(c)] + 1;
            }
        }
        return short2(x, 16);
    }

    short2 getTextSize(const char *text) {
        int x = 0, w = 0, h = 16;

        while (char c = *text++) {
            if (c == '[') break;
            c = remapCyrillic(c);
            if (c == '\xBF') c = '?';
            if (c == '\xA1') c = '!';

            if (skipChar(c)) {
                //
            } else if (c == ' ' || c == '_') {
                x += 6;
            } else if (c == '@') {
                w = max(w, x);
                h += 16;
                x = 0;
            } else
                x += char_width[charRemap(c)] + 1;
        }
        w = max(w, x);

        return short2(w, h);
    }

    enum BarType {
        BAR_FLASH,
        BAR_HEALTH,
        BAR_OXYGEN,
        BAR_OPTION,
        BAR_WHITE,
        BAR_MAX,
    };

    #ifdef SPLIT_BY_TILE
        uint16 curTile, curClut;
    #endif

    void updateAspect(float aspect) {
        height = 480.0f;
        width  = height * aspect;
        Core::mProj = GAPI::ortho(0.0f, width, height, 0.0f, -128.0f, 127.0f);
        Core::setViewProj(Core::mView, Core::mProj);
        Core::active.shader->setParam(uViewProj, Core::mViewProj);
    }

    void begin() {
        ensureLanguage(Core::settings.audio.language);

        Core::setDepthTest(false);
        Core::setBlendMode(bmPremult);
        Core::setCullMode(cmNone);
        game->setupBinding();

        Core::mView.identity();
        Core::mModel.identity();

        game->setShader(Core::passGUI, Shader::DEFAULT);
        Core::setMaterial(1, 1, 1, 1);

        game->getMesh()->dynBegin();

        #ifdef SPLIT_BY_TILE
            curTile = curClut = 0xFFFF;
        #endif
    }

    void end() {
        game->getMesh()->dynEnd();
        Core::setCullMode(cmFront);
        Core::setBlendMode(bmNone);
        Core::setDepthTest(true);
    }

    enum ShadeType {
        SHADE_NONE   = 0,
        SHADE_ORANGE = 1,
        SHADE_GRAY   = 2,
    };

    int getLeftOffset(const char *text, Align align, int width) {
        if (align != aLeft) {
            int lineWidth = getLineSize(text).x;

            if (align == aCenter || align == aCenterV)
                return (width - lineWidth) / 2;

            if (align == aRight)
                return width - lineWidth;
        }
        return 0;
    }

    void textOut(const vec2 &pos, const char *text, Align align = aLeft, float width = 0, uint8 alpha = 255, ShadeType shade = SHADE_ORANGE, bool isShadow = false) {
        if (!text) return;
       
        TR::Level *level = game->getLevel();

        if (shade && !isShadow && ((level->version & TR::VER_TR3)))
            textOut(pos + vec2(1, 1), text, align, width, alpha, shade, true);

        MeshBuilder *mesh = game->getMesh();
        int seq = level->extra.glyphs;

        int x = int(pos.x) + getLeftOffset(text, align, int(width));
        int y = int(pos.y);
        if (align == aCenterV) {
            y -= getTextSize(text).y / 2;
        }

        while (char c = *text++) {
            if (c == '[') break; // subs part end (timing tags)

            bool invertX = false, invertY = false;
            int dx = 0, dy = 0;

            c = remapCyrillic(c);
            if (c == '\xBF') { c = '?'; invertX = invertY = true; }
            if (c == '\xA1') { c = '!'; invertX = invertY = true; }

            if (c == '@') {
                x = int(pos.x) + getLeftOffset(text, align, int(width));
                y += 16;
                continue;
            }

            if (c == ' ' || c == '_') {
                x += 6;
                continue;
            }

            char charFrame = c;
            if (charFrame == '\xBF') charFrame = '?';
            if (charFrame == '\xA1') charFrame = '!';
            if (charFrame == '|')    charFrame = ',';
            if (charFrame == '*')    charFrame = '.';
            if (charFrame == '{')    charFrame = '(';

            int frame = charRemap(charFrame);

            Color32 tColor, bColor;
            if (isShadow) {
                tColor = bColor = Color32(0, 0, 0, alpha);
            } else {
                tColor = bColor = Color32(255, 255, 255, alpha);

                if (shade && ((level->version & TR::VER_TR3))) {
                    if (shade == SHADE_ORANGE) {
                        tColor = Color32(255, 190, 90, alpha);
                        bColor = Color32(140, 50, 10, alpha);
                    }
                    if (shade == SHADE_GRAY) {
                        tColor = Color32(255, 255, 255, alpha);
                        bColor = Color32(128, 128, 128, alpha);
                    }
                }
            }

            bool isSkipChar = skipChar(c);

            if (isSkipChar) {
                int idx = charRemap(remapCyrillic(*text));
                bool isUppderCase = upperCase(idx);
                
                if (c == '{') {
                    invertY = true;
                    dx = isUppderCase ? 2 : 0;
                    dy = isUppderCase ? -17 : -13;
                } else if (c == '*') {
                    dx = (char_width[idx] - char_width[frame]) / 2;
                    dy = isUppderCase ? -13 : -9;
                } else if (c == '/') {
                    frame = idx;
                    text++;
                    isSkipChar = false;
                } else if (c == '|') {
                    dy = 2;
                    invertX = true;
                    if (isUppderCase) {
                        dx = (char_width[idx] - char_width[frame]);
                    } else {
                        dx = (char_width[idx] - char_width[frame]) / 2;
                    }
                } else {
                    dx = (char_width[idx] - char_width[frame]) / 2 - 1;
                    if (isUppderCase) { // if next char is uppercase
                        dy -= 4;
                    }
                }
            }

            if (invertX) dx += char_width[frame];
            if (invertY) dy -= 10;
            int ax = 1;

            if (c == '/') {
                ax += 2;
                x += 2;
                int ox = frame < 26 ? 1 : 0;
                int line = charRemap(')');
                mesh->addDynSprite(level->spriteSequences[seq].sStart + line, short3(x + ox + 1, y + 4, 0), false, false, tColor, bColor, true);
                mesh->addDynSprite(level->spriteSequences[seq].sStart + line, short3(x + ox - 3, y + 7, 0), false, false, tColor, bColor, true);
            }

            int spriteIndex = frame;
            if (frame < level->spriteSequences[seq].sCount) {
                spriteIndex += level->spriteSequences[seq].sStart;
            } else {
                spriteIndex += advGlyphsStart - 110;
            }

            if (spriteIndex >= level->spriteTexturesCount)
                continue;

            mesh->addDynSprite(spriteIndex, short3(x + dx, y + dy, 0), invertX, invertY, tColor, bColor, true);

            if (!isSkipChar) {
                x += char_width[frame] + ax;
            }
        }
    }

    void textOut(const vec2 &pos, StringID str, Align align = aLeft, float width = 0, uint8 alpha = 255, ShadeType shade = SHADE_ORANGE) {
        textOut(pos, STR[str], align, width, alpha, shade);
    }

    void specOut(const vec2 &pos, char specChar) {
        TR::Level *level = game->getLevel();
        MeshBuilder *mesh = game->getMesh();

        int seq = level->extra.glyphs;

        if (specChar >= level->spriteSequences[seq].sCount)
            return;

        mesh->addDynSprite(level->spriteSequences[seq].sStart + specChar, short3(int16(pos.x), int16(pos.y), 0), false, false, COLOR_WHITE, COLOR_WHITE, true);
    }

    #undef MAX_CHARS

    void init(IGame *game) {
        ensureLanguage(Core::settings.audio.language);
        UI::game = game;
        showHelp = false;
        helpTipTime = 5.0f;
        hintTime = subsTime = 0.0f;
    }

    void deinit() {
        for (int i = 0; i < pickups.length; i++) {
            delete pickups[i].animation;
        }
        pickups.clear();
    }

    void showHint(StringID str, float time) {
        hintStr  = str;
        hintTime = time;
    }

    void subsGetNextPart() {
        const char *subs = STR[subsStr];

        subsPos += subsPartLength;

        if (subsPos >= subsLength) {
            subsTime = 0.0f;
            subsStr  = STR_EMPTY;
        }

        for (int i = subsPos; i < subsLength; i++) {
            if (subs[i] == '[') {
                for (int j = i; j < subsLength; j++) {
                    if (subs[j] == ']') {
                        char buf[32];
                        memcpy(buf, subs + i + 1, j - i - 1);
                        buf[j - i - 1] = 0;

                        int time = atoi(buf);

                        subsTime += (time - subsPartTime) / 1000.0f;
                        subsPartTime = time;
                        subsPartLength = j - subsPos + 1;
                        return;
                    }
                }
            }
        }

        subsPartLength = subsLength - subsPos;
        subsTime = subsPartLength * SUBTITLES_SPEED;
    }

    void showSubs(StringID str) {
        if (str == STR_EMPTY || !Core::settings.audio.subtitles)
            return;
        subsStr        = str;
        subsLength     = strlen(STR[str]);
        subsPos        = 0;
        subsTime       = 0.0f;
        subsPartTime   = 0;
        subsPartLength = 0;

        subsGetNextPart();
    }

    void update() {
        if (hintTime > 0.0f) {
            hintTime = max(0.0f, hintTime - Core::deltaTime);
        }

        if (subsTime > 0.0f) {
            subsTime -= Core::deltaTime;
            if (subsTime <= 0.0f) {
                subsGetNextPart();
            }
        }

        if (Input::down[ikH]) {
            Input::down[ikH] = false;
            showHelp = !showHelp;
            helpTipTime = 0.0f;
        }
        if (helpTipTime > 0.0f)
            helpTipTime -= Core::deltaTime;

        float w = UI::width;
        if (game->getLara(1)) {
            w *= 0.5f;
        }

        int i = 0;
        while (i < pickups.length) {
            PickupItem &item = pickups[i];
            item.time -= Core::deltaTime;
            if (item.time <= 0.0f) {
                delete item.animation;
                pickups.remove(i);
            } else {
                vec2 target = vec2(w - 48.0f - Core::eye * 16.0f - (i % 4) * 96.0f, UI::height - 48.0f - (i / 4) * 96.0f);
                item.pos = item.pos.lerp(target, Core::deltaTime * 5.0f);
                i++;
            }
        }
    }

    void renderControl(const vec2 &pos, float size, bool active) {
        vec2 scale = vec2(size * (active ? 2.0f : 1.0f) / 32767.0f);
        mat4 m = Core::mViewProj;
        m.translate(vec3(pos.x, pos.y, 0.0));
        m.scale(vec3(scale.x, scale.y, 1.0));
        Core::active.shader->setParam(uViewProj, m);
        float a = active ? 0.7f : 0.5f;
        Core::setMaterial(a, a, a, a);
        game->getMesh()->renderCircle();
    }

    void renderTouch() {
        game->setupBinding();

        if (Input::touchTimerVis <= 0.0f) return;

        Core::setDepthTest(false);
        Core::setBlendMode(bmPremult);
        Core::setCullMode(cmNone);

        Core::mViewProj = GAPI::ortho(0.0f, float(Core::width), float(Core::height), 0.0f, 0.0f, 1.0f);
        
        game->setShader(Core::passGUI, Shader::DEFAULT);

        float offset = Core::height * 0.25f;

        if (Input::btnEnable[Input::bMove]) {
            vec2 pos = vec2(offset * 0.7f, Core::height - offset * 0.7f) + vec2(-cosf(-PI * 3.0f / 4.0f), sinf(-PI * 3.0f / 4.0f)) * offset;
            if (Input::down[Input::touchKey[Input::zMove]]) {
                Input::Touch &t = Input::touch[Input::touchKey[Input::zMove] - ikTouchA];
                renderControl(t.pos, Input::btnRadius, true);
                pos = t.start;
            }
            renderControl(pos, Input::btnRadius, false);
        }

        for (int i = Input::bWeapon; i < Input::bMAX; i++)
            if (Input::btnEnable[i])
                renderControl(Input::btnPos[i], Input::btnRadius, Input::btn == i);

        Core::setCullMode(cmFront);
        Core::setBlendMode(bmNone);
        Core::setDepthTest(true);
    }

    void renderBar(BarType type, const vec2 &pos, const vec2 &size, float value, uint32 fgColor = 0xFFFFFFFF, uint32 bgColor = 0x80000000, uint32 brColor1 = 0xFF4C504C, uint32 brColor2 = 0xFF748474, uint32 fgColor2 = 0) {
        MeshBuilder *mesh = game->getMesh();

        if (brColor1 != 0 || brColor2 != 0)
            mesh->addDynFrame(pos - 2.0f, size + 4.0f, brColor1, brColor2);
        if (bgColor != 0)
            mesh->addDynBar(whiteTile, pos - 1.0f, size + 2.0f, bgColor);
        if ((fgColor != 0 || fgColor2 != 0) && value > 0.0f)
            mesh->addDynBar(barTile[type], pos, vec2(size.x * value, size.y), fgColor, fgColor2);
    }

    void renderHelp() {
    #ifdef _NAPI_SOCKET
        textOut(vec2(16, height - 32), command, aLeft, width - 32, 255, UI::SHADE_GRAY);
    #endif
        float eye = UI::width * Core::eye * 0.02f;

        if (hintTime > 0.0f) {
            textOut(vec2(16 - eye, 32), hintStr, aLeft, width - 32, 255, UI::SHADE_GRAY);
        }

    #if defined(_OS_WEB) || defined(_OS_WIN) || defined(_OS_LINUX) || defined(_OS_MAC) || defined(_OS_RPI)
        if (showHelp) {
            textOut(vec2(32 - eye, 32), STR_HELP_TEXT, aLeft, width - 32, 255, UI::SHADE_GRAY);
        } else {
            if (helpTipTime > 0.0f) {
                textOut(vec2(0 - eye, height - 16), STR_HELP_PRESS, aCenter, width, 255, UI::SHADE_ORANGE);
            }
        }
    #endif

    #ifdef UI_SHOW_FPS
        char buf[256];
        sprintf(buf, "%d", Core::stats.fps);
        textOut(vec2(0, 16), buf, aLeft, width, 255, UI::SHADE_ORANGE);
    #endif
    }

    void renderSubs() {
        if (!Core::settings.audio.subtitles) return;

        float eye = UI::width * Core::eye * 0.02f;

        if (subsTime > 0.0f) {
            const char *subs = STR[subsStr] + subsPos;
            textOut(vec2(16 - eye, height - 48) + vec2(1, 1), subs, aCenterV, width - 32, 255, UI::SHADE_GRAY, true);
            textOut(vec2(16 - eye, height - 48), subs, aCenterV, width - 32, 255, UI::SHADE_GRAY);
        }
    }

    void addPickup(TR::Entity::Type type, int playerIndex, const vec2 &pos) {
        TR::Level *level = game->getLevel();

        PickupItem item;
        item.time        = PICKUP_SHOW_TIME;
        item.pos         = pos;
        item.playerIndex = playerIndex;
        item.modelIndex  = level->getModelIndex(TR::Level::convToInv(type));
        if (item.modelIndex <= 0)
            return;
        item.animation   = new Animation(level, &level->models[item.modelIndex - 1]);

        pickups.push(item);
    }

    void setupInventoryShading(vec3 offset) {
        Core::mView.identity();
        Core::mProj = GAPI::perspective(1.0f, 1.0f, 1.0f, 2.0f);
        Core::mLightProj = Core::mProj * Core::mView;

        game->setShader(Core::passCompose, Shader::ENTITY, false, false);
        Core::setMaterial(1.0f, 0.0f, 0.0f, 1.0f);

        vec4 o = vec4(offset, 0.0f);

        // center
        Core::lightPos[0]   = vec4(0.0f, 0.0f, 0.0f, 0.0f) + o;
        Core::lightColor[0] = vec4(0.4f, 0.4f, 0.4f, 1.0f / 2048.0f);
        // camera view
        Core::lightPos[1]   = vec4(0.0f, 0.0f, -2048.0f, 0.0f) + o;
        Core::lightColor[1] = vec4(0.9f, 0.9f, 0.9f, 1.0f / 2048.0f);
        // left
        Core::lightPos[2]   = vec4(-1536.0f,  256.0f, 0.0f, 0.0f) + o;
        Core::lightColor[2] = vec4(0.8f, 0.8f, 0.5f, 1.0f / 4096.0f);
        // right
        Core::lightPos[3]   = vec4( 1536.0f, -256.0f, 0.0f, 0.0f) + o;
        Core::lightColor[3] = vec4(0.8f, 0.6f, 0.8f, 1.0f / 4096.0f);

        Core::updateLights();

        vec4 ambient[6] = { vec4(0), vec4(0), vec4(0), vec4(0), vec4(0), vec4(0) };
        Core::active.shader->setParam(uAmbient, ambient[0], 6);
    }

    void renderPickups() {
        if (!pickups.length)
            return;

        Core::viewPos = vec3(0.0);

        mat4 mView = Core::mView;
        Core::mView.scale(vec3(0.5f));
        Core::setViewProj(Core::mView, Core::mProj);

        vec3 lightOffset = vec3(UI::width - 64.0f, UI::height - 64.0f, 2048.0f);
        setupInventoryShading(lightOffset);

        Basis joints[MAX_SPHERES];

        Core::setDepthTest(true);

        MeshBuilder *mesh = game->getMesh();
        for (int i = 0; i < pickups.length; i++) {
            const PickupItem &item = pickups[i];

            if (item.playerIndex != game->getCamera()->cameraIndex)
                continue;

            float offset = 0.0f;
            if (item.time < 1.0f) {
                offset = 1.0f - item.time;
                offset *= offset;
                offset *= offset;
                offset *= 512.0f;
            }

            mat4 matrix;
            matrix.identity();
            matrix.translate(vec3(item.pos.x + offset, item.pos.y, 0.0f) * 2.0f);
            matrix.rotateX(-15 * DEG2RAD);
            matrix.rotateY(item.time * PI * 0.5f);

            item.animation->getJoints(matrix, -1, true, joints);
            Core::setBasis(joints, item.animation->model->mCount);

            float alpha = 1.0f - min(PICKUP_SHOW_TIME - item.time, 1.0f);
            alpha *= alpha;
            alpha *= alpha;
            alpha = 1.0f - alpha;

            Core::setMaterial(1.0f, 0.0f, 0.0f, alpha);

            mesh->renderModelFull(item.modelIndex - 1);
        }

        Core::setDepthTest(false);

        Core::setViewProj(mView, Core::mProj);
        game->setShader(Core::passGUI, Shader::DEFAULT);
        Core::active.shader->setParam(uViewProj, Core::mViewProj);
        Core::setBlendMode(bmPremult);
        Core::setCullMode(cmNone);
        Core::setMaterial(1, 1, 1, 1);
    }
};

#endif

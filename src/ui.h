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

enum StringID {
      STR_NOT_IMPLEMENTED
// common
    , STR_LOADING
    , STR_HELP_PRESS
    , STR_HELP_TEXT
    , STR_LEVEL_STATS
    , STR_HINT_SAVING
    , STR_HINT_SAVING_DONE
    , STR_HINT_SAVING_ERROR
    , STR_YES
    , STR_NO
    , STR_OFF
    , STR_ON
    , STR_SPLIT
    , STR_VR
    , STR_QUALITY_LOW
    , STR_QUALITY_MEDIUM
    , STR_QUALITY_HIGH
    , STR_APPLY
    , STR_GAMEPAD_1
    , STR_GAMEPAD_2
    , STR_GAMEPAD_3
    , STR_GAMEPAD_4
    , STR_NOT_READY
    , STR_PLAYER_1
    , STR_PLAYER_2
    , STR_PRESS_ANY_KEY
    , STR_HELP_SELECT
    , STR_HELP_BACK
// inventory pages
    , STR_OPTION
    , STR_INVENTORY
    , STR_ITEMS
// save game page
    , STR_SAVEGAME
    , STR_CURRENT_POSITION
// inventory option
    , STR_GAME
    , STR_MAP
    , STR_COMPASS
    , STR_STOPWATCH
    , STR_HOME
    , STR_DETAIL
    , STR_SOUND
    , STR_CONTROLS
    , STR_GAMMA
// passport menu
    , STR_AUTOSAVE
    , STR_LOAD_GAME
    , STR_START_GAME
    , STR_RESTART_LEVEL
    , STR_EXIT_TO_TITLE
    , STR_EXIT_GAME
    , STR_SELECT_LEVEL
// detail options
    , STR_SELECT_DETAIL
    , STR_OPT_DETAIL_FILTER
    , STR_OPT_DETAIL_LIGHTING
    , STR_OPT_DETAIL_SHADOWS
    , STR_OPT_DETAIL_WATER
    , STR_OPT_DETAIL_VSYNC
    , STR_OPT_DETAIL_STEREO
    , STR_OPT_SIMPLE_ITEMS
    , STR_OPT_SUBTITLES
// sound options
    , STR_SET_VOLUMES
    , STR_REVERBERATION
// controls options
    , STR_SET_CONTROLS
    , STR_OPT_CONTROLS_KEYBOARD
    , STR_OPT_CONTROLS_GAMEPAD
    , STR_OPT_CONTROLS_VIBRATION
    , STR_OPT_CONTROLS_RETARGET
    , STR_OPT_CONTROLS_MULTIAIM
    // controls
    , STR_CTRL_FIRST
    , STR_CTRL_LAST = STR_CTRL_FIRST + cMAX - 1
    // keys
    , STR_KEY_FIRST
    , STR_KEY_LAST  = STR_KEY_FIRST + ikZ
    // gamepad
    , STR_JOY_FIRST
    , STR_JOY_LAST  = STR_JOY_FIRST + jkMAX - 1
// inventory items
    , STR_UNKNOWN
    , STR_EXPLOSIVE
    , STR_PISTOLS
    , STR_SHOTGUN
    , STR_MAGNUMS
    , STR_UZIS
    , STR_AMMO_PISTOLS
    , STR_AMMO_SHOTGUN
    , STR_AMMO_MAGNUMS
    , STR_AMMO_UZIS
    , STR_MEDI_SMALL
    , STR_MEDI_BIG
    , STR_LEAD_BAR
    , STR_SCION
// keys
    , STR_KEY
    , STR_KEY_SILVER
    , STR_KEY_RUSTY
    , STR_KEY_GOLD
    , STR_KEY_SAPPHIRE
    , STR_KEY_NEPTUNE
    , STR_KEY_ATLAS
    , STR_KEY_DAMOCLES
    , STR_KEY_THOR
    , STR_KEY_ORNATE
// puzzles
    , STR_PUZZLE
    , STR_PUZZLE_GOLD_IDOL
    , STR_PUZZLE_GOLD_BAR
    , STR_PUZZLE_COG
    , STR_PUZZLE_FUSE
    , STR_PUZZLE_ANKH
    , STR_PUZZLE_HORUS
    , STR_PUZZLE_ANUBIS
    , STR_PUZZLE_SCARAB
    , STR_PUZZLE_PYRAMID
// TR1 subtitles
    , STR_TR1_SUB_26
    , STR_TR1_SUB_27
    , STR_TR1_SUB_28
    , STR_TR1_SUB_29
    , STR_TR1_SUB_30
    , STR_TR1_SUB_31
    , STR_TR1_SUB_32
    , STR_TR1_SUB_33
    , STR_TR1_SUB_34
    , STR_TR1_SUB_35
    , STR_TR1_SUB_36
    , STR_TR1_SUB_37
    , STR_TR1_SUB_38
    , STR_TR1_SUB_39
    , STR_TR1_SUB_40
    , STR_TR1_SUB_41
    , STR_TR1_SUB_42
    , STR_TR1_SUB_43
    , STR_TR1_SUB_44
    , STR_TR1_SUB_45
    , STR_TR1_SUB_46
    , STR_TR1_SUB_47
    , STR_TR1_SUB_48
    , STR_TR1_SUB_49
    , STR_TR1_SUB_50
    , STR_TR1_SUB_51
    , STR_TR1_SUB_52
    , STR_TR1_SUB_53
    , STR_TR1_SUB_54
    , STR_TR1_SUB_55
    , STR_TR1_SUB_56

    , STR_MAX
};

const char *helpText = 
    "Start - add second player or restore Lara@"
    "H - Show or hide this help@"
    "ALT + ENTER - Fullscreen@"
    "5 - Save Game@"
    "9 - Load Game@"
    "C - Look@"
    "R - Slow motion@"
    "T - Fast motion@"
    "Roll - Up + Down@"
    "Step Left - Walk + Left@"
    "Step Right - Walk + Right@"
    "Out of water - Up + Action@"
    "Handstand - Up + Walk@"
    "Swan dive - Up + Walk + Jump@"
    "First Person View - Look + Action@"
    "DOZY on - Look + Duck + Action + Jump@"
    "DOZY off - Walk";

const char *levelStats = 
    "%s@@@"
    "KILLS %d@@"
    "PICKUPS %d@@"
    "SECRETS %d of %d@@"
    "TIME TAKEN %s";

const char *STR[STR_MAX] = {
      "Not implemented yet!"
// help
    , "Loading..."
    , "Press H for help"
    , helpText
    , levelStats
    , "Saving game..."
    , "Saving done!"
    , "SAVING ERROR!"
    , "YES"
    , "NO"
    , "Off"
    , "On"
    , "Split Screen"
    , "VR"
    , "Low"
    , "Medium"
    , "High"
    , "Apply"
    , "Gamepad 1"
    , "Gamepad 2"
    , "Gamepad 3"
    , "Gamepad 4"
    , "Not Ready"
    , "Player 1"
    , "Player 2"
    , "Press Any Key"
    , "%s - Select"
    , "%s - Go Back"
// inventory pages
    , "OPTION"
    , "INVENTORY"
    , "ITEMS"
// save game page
    , "Save Game?"
    , "Current Position"
// inventory option
    , "Game"
    , "Map"
    , "Compass"
    , "Statistics"
    , "Lara's Home"
    , "Detail Levels"
    , "Sound"
    , "Controls"
    , "Gamma"
// passport menu
    , "Autosave"
    , "Load Game"
    , "Start Game"
    , "Restart Level"
    , "Exit to Title"
    , "Exit Game"
    , "Load Game"
// detail options
    , "Select Detail"
    , "Filtering"
    , "Lighting"
    , "Shadows"
    , "Water"
    , "VSync"
    , "Stereo"
    , "Simple Items"
    , "Subtitles"
// sound options
    , "Set Volumes"
    , "Reverberation"
// controls options
    , "Set Controls"
    , "Keyboard"
    , "Gamepad"
    , "Vibration"
    , "Retargeting"
    , "Multi-aiming"
    // controls
    , "Left", "Right", "Up", "Down", "Jump", "Walk", "Action", "Draw Weapon", "Look", "Duck", "Dash", "Roll", "Inventory", "Start"
    // keys
    , "NONE", "LEFT", "RIGHT", "UP", "DOWN", "SPACE", "TAB", "ENTER", "ESCAPE", "SHIFT", "CTRL", "ALT"
    , "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
    , "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M"
    , "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
    // gamepad
    , "NONE", "A", "B", "X", "Y", "L BUMPER", "R BUMPER", "SELECT", "START", "L STICK", "R STICK", "L TRIGGER", "R TRIGGER", "D-LEFT", "D-RIGHT", "D-UP", "D-DOWN"
// inventory items
    , "Unknown"
    , "Explosive"
    , "Pistols"
    , "Shotgun"
    , "Magnums"
    , "Uzis"
    , "Pistol Clips"
    , "Shotgun Shells"
    , "Magnum Clips"
    , "Uzi Clips"
    , "Small Medi Pack"
    , "Large Medi Pack"
    , "Lead Bar"
    , "Scion"
// keys
    , "Key"
    , "Silver Key"
    , "Rusty Key"
    , "Gold Key"
    , "Sapphire Key"
    , "Neptune Key"
    , "Atlas Key"
    , "Damocles Key"
    , "Thor Key"
    , "Ornate Key"
// puzzles
    , "Puzzle"
    , "Gold Idol"
    , "Gold Bar"
    , "Machine Cog"
    , "Fuse"
    , "Ankh"
    , "Eye of Horus"
    , "Seal of Anubis"
    , "Scarab"
    , "Pyramid Key"
// TR1 subtitles
/* 26 */ , "Welcome to my home.@I'll take you on a guided tour."
/* 27 */ , "Use the directional buttons to go into the music room."
/* 28 */ , "OK. Let's do some tumbling.@Press the jump button."
/* 29 */ , "Now press it again and press one of@the directions and I'll jump that way."
/* 30 */ , "Ah, the main hall.@Sorry about the crates, I'm having some things put@ into storage and the delivery people haven't been yet."
/* 31 */ , "Run up to a crate, and while still pressing forwards@press action, and I'll vault up onto it."
/* 32 */ , "This used to be the ballroom, but I've converted it into my own personal gym.@What do you think?@Well, let's do some exercises."
/* 33 */ , "I don't actually run everywhere.@When I want to be careful, I walk.@Hold down the walk button, and walk to the white line."
/* 34 */ , "With the walk button down, I won't fall off even if you try to make me.@Go on, try it."
/* 35 */ , "If you want look around, press and hold the look button.@Then press in the direction you want to look."
/* 36 */ , "If a jump is too far for me, I can grab the ledge and save myself from a nasty fall.@Walk to the edge with the white line until I won't go any further.@Then press jump immediately followed by forwards then while @I'm in the air press and hold the action button."
/* 37 */ , "Press forward and I'll climb up."
/* 38 */ , "If I do a running jump, I can make a jump like that, no problem."
/* 39 */ , "Walk to the edge with the white line until I stop.@Then let go of walk and tap backwards to give me a run up.@Press forward, and almost immediately press and hold the jump button.@I won't actually jump until the last minute."
/* 40 */ , "Right. This is a really big one.@So do a running jump exactly as before except while I'm in the air@press and hold the action button to make me grab the ledge."
/* 41 */ , "Nice."
/* 42 */ , "Try to vault up here.@Press forwards and hold action."
/* 43 */ , "I can't climb up because the gap is too small.@But press right and I'll shimmy sideways@until there is room, then press forward."
/* 44 */ , "Great!@If there is a long drop and I don't want to@hurt myself jumping off I can let myself down carefully."
/* 45 */ , "Tap backwards, and I'll jump off backwards.@Immediately press and hold the action button,@and I'll grab the ledge on the way down."
/* 46 */ , "Then let go."
/* 47 */ , "Let's go for a swim."
/* 48 */ , "The jump button and the directions@move me around underwater."
/* 49 */ , "Ah! Air!@Just use forward and left and right@to manoeuvre around on the surface.@Press jump to dive down for another swim about.@Or go to the edge and press action to climb out."
/* 50 */ , "Right. Now I'd better take off these wet clothes."
/* 51 */ , "Say cheese!"
/* 52 */ , "Ain't nothin' personal."
/* 53 */ , "I still git a pain in my brain from ye.@An' it's tellin' me funny ideas now.@Like to shoot you to hell!"
/* 54 */ , "You can't bump off me and my brood so easy, Lara."
/* 55 */ , "A leetle late for the prize giving - non?@Still, it is the taking-part wheech counts."
/* 56 */ , "You firin' at me?@You firin' at me, huh?@Ain't nobody else, so you must be firin' at me!"
};

#ifdef _NAPI_SOCKET
extern char command[256];
#endif

namespace UI {
    IGame    *game;
    float    width, height;
    float    helpTipTime;
    float    hintTime;
    float    subsTime;

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

    const static uint8 char_width[110] = {
        14, 11, 11, 11, 11, 11, 11, 13, 8, 11, 12, 11, 13, 13, 12, 11, 12, 12, 11, 12, 13, 13, 13, 12, 12, 11, // A-Z
        9, 9, 9, 9, 9, 9, 9, 9, 5, 9, 9, 5, 12, 10, 9, 9, 9, 8, 9, 8, 9, 9, 11, 9, 9, 9, // a-z
        12, 8, 10, 10, 10, 10, 10, 9, 10, 10, // 0-9
        5, 5, 5, 11, 9, 10, 8, 6, 6, 7, 7, 3, 11, 8, 13, 16, 9, 4, 12, 12, 
        7, 5, 7, 7, 7, 7, 7, 7, 7, 7, 16, 14, 14, 14, 16, 16, 16, 16, 16, 12, 14, 8, 8, 8, 8, 8, 8, 8 }; 
        
    static const uint8 char_map[102] = {
            0, 64, 66, 78, 77, 74, 78, 79, 69, 70, 92, 72, 63, 71, 62, 68, 52, 53, 54, 55, 56, 57, 58, 59, 
        60, 61, 73, 73, 66, 74, 75, 65, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 
        18, 19, 20, 21, 22, 23, 24, 25, 80, 76, 81, 97, 98, 77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 
        37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 100, 101, 102, 67, 0, 0, 0, 0, 0, 0, 0 };

    enum Align  { aLeft, aRight, aCenter, aCenterV };

    inline int charRemap(char c) {
        ASSERT(c <= 126);
        if (c < 11)
            return c + 81;
        if (c < 16)
            return c + 91;
        ASSERT(c >= 32)
        return char_map[c - 32];
    }

    short2 getLineSize(const char *text) {
        int x = 0;

        while (char c = *text++) {
            if (c == ' ' || c == '_') {
                x += 6;
            } else if (c == '@') {
                break;
            } else 
                x += char_width[charRemap(c)] + 1;
        }
        return short2(x, 16);
    }

    short2 getTextSize(const char *text) {
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

            if (c == '@') {
                x = int(pos.x) + getLeftOffset(text, align, int(width));
                y += 16;
                continue;
            }

            if (c == ' ' || c == '_') {
                x += 6;
                continue;
            }

            int frame = charRemap(c);

            if (frame >= level->spriteSequences[seq].sCount)
                continue;

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

            mesh->addDynSprite(level->spriteSequences[seq].sStart + frame, short3(x, y, 0), tColor, bColor, true);

            x += char_width[frame] + 1;
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

        mesh->addDynSprite(level->spriteSequences[seq].sStart + specChar, short3(int16(pos.x), int16(pos.y), 0), COLOR_WHITE, COLOR_WHITE, true);
    }

    #undef MAX_CHARS

    void init(IGame *game) {
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

    void update() {
        if (hintTime > 0.0f) {
            hintTime = max(0.0f, hintTime - Core::deltaTime);
        }

        if (subsTime > 0.0f) {
            subsTime = max(0.0f, subsTime - Core::deltaTime);
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
        Core::setMaterial(1.0f, 1.0f, 1.0f, active ? 0.7f : 0.5f);
        game->getMesh()->renderCircle();
    }

    void renderTouch() {
        game->setupBinding();

        if (Input::touchTimerVis <= 0.0f) return;

        Core::setDepthTest(false);
        Core::setBlendMode(bmAlpha);
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

    void showHint(StringID str, float time) {
        hintStr  = str;
        hintTime = time;
    }

    void showSubs(StringID str) {
        if (str == STR_NOT_IMPLEMENTED || !Core::settings.detail.subtitles)
            return;
        subsStr  = str;
        subsTime = strlen(STR[str]) * SUBTITLES_SPEED;
    }

    StringID getSubs(int track) {
        if (game && (game->getLevel()->version & TR::VER_TR1) && track >= 26 && track <= 56)
            return StringID(STR_TR1_SUB_26 + (track - 26));
        return STR_NOT_IMPLEMENTED;
    }

    void renderHelp() {
    #ifdef _NAPI_SOCKET
        textOut(vec2(16, height - 32), command, aLeft, width - 32, 255, UI::SHADE_GRAY);
    #endif
        float eye = UI::width * Core::eye * 0.02f;

        if (hintTime > 0.0f) {
            textOut(vec2(16 - eye, 32), hintStr, aLeft, width - 32, 255, UI::SHADE_GRAY);
        }

        if (subsTime > 0.0f) {
            textOut(vec2(16 - eye, height - 48) + vec2(1, 1), STR[subsStr], aCenterV, width - 32, 255, UI::SHADE_GRAY, true);
            textOut(vec2(16 - eye, height - 48), STR[subsStr], aCenterV, width - 32, 255, UI::SHADE_GRAY);
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
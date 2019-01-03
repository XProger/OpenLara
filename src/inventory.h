#ifndef H_INVENTORY
#define H_INVENTORY

#include "format.h"
#include "controller.h"
#include "ui.h"
#include "savegame.h"

#define INVENTORY_MAX_ITEMS  32
#define INVENTORY_MAX_RADIUS 688.0f
#ifdef _OS_PSP
    #define INVENTORY_BG_SIZE    256
#else
    #define INVENTORY_BG_SIZE    512
#endif

#define INVENTORY_HEIGHT     2048.0f
#define TITLE_LOADING        64.0f
#define LINE_HEIGHT          20.0f

static const struct OptionItem *waitForKey = NULL;

struct OptionItem {
    enum Type {
        TYPE_TITLE,
        TYPE_EMPTY,
        TYPE_BUTTON,
        TYPE_PARAM,
        TYPE_KEY,
    } type;
    StringID title;
    intptr_t offset;
    uint32   color;
    uint32   icon;
    uint8    maxValue;
    bool     bar;

    OptionItem(Type type = TYPE_EMPTY, int title = STR_NOT_IMPLEMENTED, intptr_t offset = 0, uint32 color = 0xFFFFFFFF, int icon = 0, uint8 maxValue = 0, bool bar = false) : type(type), title(StringID(title)), offset(offset), color(color), icon(icon), maxValue(maxValue), bar(bar) {}

    void setValue(uint8 value, Core::Settings *settings) const {
        *(uint8*)(intptr_t(settings) + offset) = value;
    }

    bool checkValue(uint8 value) const {
        if (value > maxValue) return false;
        Core::Settings stg;
        switch (title) {
            case STR_OPT_DETAIL_FILTER   : stg.detail.setFilter((Core::Settings::Quality)value);   return stg.detail.filter   == value;
            case STR_OPT_DETAIL_LIGHTING : stg.detail.setLighting((Core::Settings::Quality)value); return stg.detail.lighting == value;
            case STR_OPT_DETAIL_SHADOWS  : stg.detail.setShadows((Core::Settings::Quality)value);  return stg.detail.shadows  == value;
            case STR_OPT_DETAIL_WATER    : stg.detail.setWater((Core::Settings::Quality)value);    return stg.detail.water    == value;
            default : return true;
        }
    }

    float drawParam(float x, float y, float w, StringID oStr, bool active, uint8 value) const {
        if (oStr != STR_NOT_IMPLEMENTED) {
            UI::textOut(vec2(x + 32.0f, y), oStr);
            x = x + w * 0.5f;
            w = w * 0.5f - 32.0f;
        }

        StringID vStr = StringID(color + int(value));

        uint8 alpha = 255;
        if (type == TYPE_KEY && waitForKey == this) {
            vStr = STR_PRESS_ANY_KEY;
            float t = (Core::getTime() % 1000) / 1000.0f;
            t = 0.2f + (sinf(t * PI * 2) * 0.5f + 0.5f) * 0.8f;
            alpha = uint8(t * 255.0f);
        }

        UI::textOut(vec2(x, y), vStr, UI::aCenter, w, alpha, UI::SHADE_GRAY); // color as StringID

        if (type == TYPE_PARAM && active) {
            int maxWidth = UI::getTextSize(STR[color + value]).x;
            maxWidth = maxWidth / 2 + 8;
            x += w * 0.5f;
            if (checkValue(value - 1)) UI::specOut(vec2(x - maxWidth - 16.0f, y), 108);
            if (checkValue(value + 1)) UI::specOut(vec2(x + maxWidth, y), 109);
        }
        return y + LINE_HEIGHT;
    }

    float drawBar(float x, float y, float w, bool active, uint8 value) const {
        UI::renderBar(UI::BAR_WHITE, vec2(x + (32.0f + 2.0f), y - LINE_HEIGHT + 6 + 2), vec2(w - (64.0f + 4.0f), LINE_HEIGHT - 6 - 4), value / float(maxValue), color, 0xFF000000, 0xFFA0A0A0, 0xFFA0A0A0, 0xFF000000);
        UI::specOut(vec2(x + 16.0f, y), icon);
        if (active) {
            if (value >        0) UI::specOut(vec2(x, y), 108);
            if (value < maxValue) UI::specOut(vec2(x + w - 12.0f, y), 109);
        }
        return y + LINE_HEIGHT;
    }

    float render(float x, float y, float w, bool active, Core::Settings *settings) const {
        if (active)
            UI::renderBar(UI::BAR_OPTION, vec2(x, y - LINE_HEIGHT + 6), vec2(w, LINE_HEIGHT - 6), 1.0f, 0xFFD8377C, 0);

        const uint8 &value = *(uint8*)(intptr_t(settings) + offset);

        switch (type) {
            case TYPE_TITLE   : 
                UI::renderBar(UI::BAR_OPTION, vec2(x, y - LINE_HEIGHT + 6), vec2(w, LINE_HEIGHT - 6), 1.0f, 0x802288FF, 0, 0, 0);
                UI::textOut(vec2(x, y), title, UI::aCenter, w, 255, UI::SHADE_GRAY); 
            case TYPE_EMPTY   : break;
            case TYPE_BUTTON  : {
                const char *caption = offset ? (char*)offset : STR[title];
                UI::textOut(vec2(x, y), caption, UI::aCenter, w);
                break;
            }
            case TYPE_PARAM : 
            case TYPE_KEY :
                return bar ? drawBar(x, y, w, active, value) : drawParam(x, y, w, title, active, value);
        }

        return y + LINE_HEIGHT;
    }
};

#define SETTINGS(x) OFFSETOF(Core::Settings, x)

static const OptionItem optDetail[] = {
    OptionItem( OptionItem::TYPE_TITLE,  STR_SELECT_DETAIL ),
    OptionItem( ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_FILTER,   SETTINGS( detail.filter   ), STR_QUALITY_LOW, 0, 2 ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_LIGHTING, SETTINGS( detail.lighting ), STR_QUALITY_LOW, 0, 2 ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_SHADOWS,  SETTINGS( detail.shadows  ), STR_QUALITY_LOW, 0, 2 ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_WATER,    SETTINGS( detail.water    ), STR_QUALITY_LOW, 0, 2 ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_SIMPLE_ITEMS,    SETTINGS( detail.simple   ), STR_OFF, 0, 1 ),
#if defined(_OS_WIN) || defined(_OS_LINUX) || defined(_OS_PSP) || defined(_OS_RPI) || defined(_OS_PSV)
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_VSYNC,    SETTINGS( detail.vsync    ), STR_OFF, 0, 1 ),
#endif
#ifndef _OS_PSP
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_STEREO,   SETTINGS( detail.stereo   ), STR_OFF, 0, 
#if /*defined(_OS_WIN) ||*/ defined(_OS_ANDROID)
    3 /* with VR option */
#else
    2 /* without VR support */
#endif
    ),
#endif
    OptionItem( ),
    OptionItem( OptionItem::TYPE_BUTTON, STR_APPLY ),
};

static const OptionItem optSound[] = {
    OptionItem( OptionItem::TYPE_TITLE,  STR_SET_VOLUMES ),
    OptionItem( ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_NOT_IMPLEMENTED,     SETTINGS( audio.music  ), 0xFF0080FF, 101, SND_MAX_VOLUME, true ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_NOT_IMPLEMENTED,     SETTINGS( audio.sound  ), 0xFFFF8000, 102, SND_MAX_VOLUME, true ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_REVERBERATION,       SETTINGS( audio.reverb ), STR_OFF, 0, 1 ),
};

static const OptionItem optControls[] = {
    OptionItem( OptionItem::TYPE_TITLE,  STR_SET_CONTROLS ),
    OptionItem( ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_NOT_IMPLEMENTED         , SETTINGS( playerIndex                    ), STR_PLAYER_1,  0, 1 ),
#ifndef _OS_CLOVER
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_CONTROLS_GAMEPAD    , SETTINGS( controls[0].joyIndex           ), STR_GAMEPAD_1, 0, 3 ),
#endif
#if defined(_OS_WIN) || defined(_OS_LINUX) || defined(_OS_RPI)
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_CONTROLS_VIBRATION  , SETTINGS( controls[0].vibration          ), STR_OFF,       0, 1 ),
#endif
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_CONTROLS_RETARGET   , SETTINGS( controls[0].retarget           ), STR_OFF,       0, 1 ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_CONTROLS_MULTIAIM   , SETTINGS( controls[0].multiaim           ), STR_OFF,       0, 1 ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_NOT_IMPLEMENTED         , SETTINGS( ctrlIndex                      ), STR_OPT_CONTROLS_KEYBOARD, 0, 1 ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cUp        , SETTINGS( controls[0].keys[ cUp        ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cDown      , SETTINGS( controls[0].keys[ cDown      ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cRight     , SETTINGS( controls[0].keys[ cRight     ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cLeft      , SETTINGS( controls[0].keys[ cLeft      ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cWalk      , SETTINGS( controls[0].keys[ cWalk      ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cJump      , SETTINGS( controls[0].keys[ cJump      ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cAction    , SETTINGS( controls[0].keys[ cAction    ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cWeapon    , SETTINGS( controls[0].keys[ cWeapon    ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cLook      , SETTINGS( controls[0].keys[ cLook      ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cDuck      , SETTINGS( controls[0].keys[ cDuck      ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cDash      , SETTINGS( controls[0].keys[ cDash      ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cRoll      , SETTINGS( controls[0].keys[ cRoll      ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cInventory , SETTINGS( controls[0].keys[ cInventory ] ), STR_KEY_FIRST ),
    OptionItem( OptionItem::TYPE_KEY,    STR_CTRL_FIRST + cStart     , SETTINGS( controls[0].keys[ cStart     ] ), STR_KEY_FIRST ),
};

static OptionItem optControlsPlayer[COUNT(optControls)];

struct Inventory {

    enum Page {
        PAGE_OPTION,
        PAGE_INVENTORY,
        PAGE_ITEMS,
        PAGE_SAVEGAME,
        PAGE_LEVEL_STATS,
        PAGE_MAX
    };

    IGame   *game;
    Texture *background[2];
    Video   *video;

    bool    playLogo;
    bool    playVideo;

    bool    active;
    bool    chosen;
    float   phaseRing, phasePage, phaseChoose, phaseSelect;
    int     index, targetIndex, pageItemIndex[PAGE_MAX];
    Page    page, targetPage;
    int     itemsCount;
    int     playerIndex;

    float       titleTimer;
    float       changeTimer;
    TR::LevelID nextLevel; // toggle result
    ControlKey  lastKey;

    mat4        head;
    
    int slot;
    Core::Settings settings;

    struct Item {
        TR::Entity::Type    type;
        int                 count;
        float               angle;
        Animation           *anim;

        int                 value;
        vec4                params;

        Array<OptionItem>   optLoadSlots;

        struct Desc {
            StringID    str;
            Page        page;
            int         model;
            
            Desc() {}
            Desc(StringID str, Page page, int model) : str(str), page(page), model(model) {}
        } desc;

        Item() : anim(NULL) {}

        Item(TR::Level *level, TR::Entity::Type type, int count = 1) : type(type), count(count), angle(0.0f), value(0), params(0.0f) {
            switch (type) {
                case TR::Entity::INV_PASSPORT        : desc = Desc( STR_GAME,            PAGE_OPTION,    level->extra.inv.passport        ); break;
                case TR::Entity::INV_PASSPORT_CLOSED : desc = Desc( STR_GAME,            PAGE_OPTION,    level->extra.inv.passport_closed ); break;
                case TR::Entity::INV_MAP             : desc = Desc( STR_MAP,             PAGE_INVENTORY, level->extra.inv.map             ); break;
                case TR::Entity::INV_COMPASS         : desc = Desc( STR_COMPASS,         PAGE_INVENTORY, level->extra.inv.compass         ); break;
                case TR::Entity::INV_STOPWATCH       : desc = Desc( STR_STOPWATCH,       PAGE_INVENTORY, level->extra.inv.stopwatch       ); break;
                case TR::Entity::INV_EXPLOSIVE       : desc = Desc( STR_EXPLOSIVE,       PAGE_INVENTORY, level->extra.inv.explosive       ); break;
                case TR::Entity::INV_HOME            : desc = Desc( STR_HOME,            PAGE_OPTION,    level->extra.inv.home            ); break;
                case TR::Entity::INV_DETAIL          : desc = Desc( STR_DETAIL,          PAGE_OPTION,    level->extra.inv.detail          ); break;
                case TR::Entity::INV_SOUND           : desc = Desc( STR_SOUND,           PAGE_OPTION,    level->extra.inv.sound           ); break;
                case TR::Entity::INV_CONTROLS        : desc = Desc( STR_CONTROLS,        PAGE_OPTION,    level->extra.inv.controls        ); break;
                case TR::Entity::INV_GAMMA           : desc = Desc( STR_GAMMA,           PAGE_OPTION,    level->extra.inv.gamma           ); break;
                                                                                   
                case TR::Entity::INV_PISTOLS         : desc = Desc( STR_PISTOLS,         PAGE_INVENTORY, level->extra.inv.weapons[type]   ); break;
                case TR::Entity::INV_SHOTGUN         : desc = Desc( STR_SHOTGUN,         PAGE_INVENTORY, level->extra.inv.weapons[type]   ); break;
                case TR::Entity::INV_MAGNUMS         : desc = Desc( STR_MAGNUMS,         PAGE_INVENTORY, level->extra.inv.weapons[type]   ); break;
                case TR::Entity::INV_UZIS            : desc = Desc( STR_UZIS,            PAGE_INVENTORY, level->extra.inv.weapons[type]   ); break;
                                                                                   
                case TR::Entity::INV_AMMO_PISTOLS    : desc = Desc( STR_AMMO_PISTOLS,    PAGE_INVENTORY, level->extra.inv.ammo[type]      ); break;
                case TR::Entity::INV_AMMO_SHOTGUN    : desc = Desc( STR_AMMO_SHOTGUN,    PAGE_INVENTORY, level->extra.inv.ammo[type]      ); break;
                case TR::Entity::INV_AMMO_MAGNUMS    : desc = Desc( STR_AMMO_MAGNUMS,    PAGE_INVENTORY, level->extra.inv.ammo[type]      ); break;
                case TR::Entity::INV_AMMO_UZIS       : desc = Desc( STR_AMMO_UZIS,       PAGE_INVENTORY, level->extra.inv.ammo[type]      ); break;

                case TR::Entity::INV_MEDIKIT_SMALL   : desc = Desc( STR_MEDI_SMALL,      PAGE_INVENTORY, level->extra.inv.medikit[0]      ); break;
                case TR::Entity::INV_MEDIKIT_BIG     : desc = Desc( STR_MEDI_BIG,        PAGE_INVENTORY, level->extra.inv.medikit[1]      ); break;

                case TR::Entity::INV_PUZZLE_1        : desc = Desc( STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[0]       ); break;
                case TR::Entity::INV_PUZZLE_2        : desc = Desc( STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[1]       ); break;
                case TR::Entity::INV_PUZZLE_3        : desc = Desc( STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[2]       ); break;
                case TR::Entity::INV_PUZZLE_4        : desc = Desc( STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[3]       ); break;
                                                                                                   
                case TR::Entity::INV_KEY_ITEM_1      : desc = Desc( STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[0]          ); break;
                case TR::Entity::INV_KEY_ITEM_2      : desc = Desc( STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[1]          ); break;
                case TR::Entity::INV_KEY_ITEM_3      : desc = Desc( STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[2]          ); break;
                case TR::Entity::INV_KEY_ITEM_4      : desc = Desc( STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[3]          ); break;
                                                                                                   
                case TR::Entity::INV_LEADBAR         : desc = Desc( STR_LEAD_BAR,        PAGE_ITEMS,     level->extra.inv.leadbar         ); break;
                case TR::Entity::INV_SCION           : desc = Desc( STR_SCION,           PAGE_ITEMS,     level->extra.inv.scion           ); break;
                default                              : desc = Desc( STR_UNKNOWN,         PAGE_ITEMS,     -1                               ); break;
            }

            if (desc.model > -1 && level->models[desc.model].animation != 0xFFFF) {
                anim = new Animation(level, &level->models[desc.model]);
                anim->isEnded = true;
            } else
                anim = NULL;
        }

        ~Item() {
            delete anim;
        }

        Item& operator = (Item &item) {
            memcpy(this, &item, sizeof(item));
            item.anim = NULL;
            return *this;
        }

        void reset() {
            if (anim) {
                anim->setAnim(0, 0, false);
                anim->isEnded = true;
            }
        }

        void initLoadSlots(TR::Level *level) {
            optLoadSlots.clear();
            optLoadSlots.push(OptionItem( OptionItem::TYPE_TITLE,  STR_SELECT_LEVEL ));
            optLoadSlots.push(OptionItem( ));

            for (int i = 0; i < saveSlots.length; i++) {
                const SaveSlot &slot = saveSlots[i];

                TR::LevelID id = slot.getLevelID();

                if (TR::getGameVersionByLevel(id) != (level->version & TR::VER_VERSION))
                    continue;

                OptionItem item;
                item.type   = OptionItem::TYPE_BUTTON;
                item.offset = slot.isCheckpoint() ? intptr_t(STR[STR_CURRENT_POSITION]) : intptr_t(TR::LEVEL_INFO[id].title); // offset as int pointer to level title string
                item.color  = i; // color as slot index
                optLoadSlots.push(item);
            }
/*
            #ifdef _DEBUG
                int passportSlotCount = 0;
                TR::LevelID passportSlots[32];

                switch (level->version & TR::VER_VERSION) {
                    case TR::VER_TR1 : 
                    #ifdef _OS_WEB
                        passportSlotCount = 2;
                        passportSlots[0]  = TR::LVL_TR1_1;
                        passportSlots[1]  = TR::LVL_TR1_2;
                    #else
                        passportSlotCount = 0;
                        for (int i = TR::LVL_TR1_1; i <= TR::LVL_TR1_10C; i++)
                            if (!TR::isCutsceneLevel(TR::LevelID(i))) {
                                passportSlots[passportSlotCount++] = TR::LevelID(i);
                            }
                    #endif
                        break;
                    case TR::VER_TR2 :
                    #ifdef _OS_WEB
                        passportSlotCount = 2;
                        passportSlots[0]  = TR::LVL_TR2_WALL;
                        passportSlots[1]  = TR::LVL_TR2_BOAT;
                    #else
                        passportSlotCount = 0;
                        for (int i = TR::LVL_TR2_WALL; i <= TR::LVL_TR2_HOUSE; i++)
                            if (!TR::isCutsceneLevel(TR::LevelID(i))) {
                                passportSlots[passportSlotCount++] = TR::LevelID(i);
                            }
                    #endif
                        break;
                    case TR::VER_TR3 :
                    #ifdef _OS_WEB
                        passportSlotCount = 1;
                        passportSlots[0]  = TR::LVL_TR3_JUNGLE;
                    #else
                        passportSlotCount = 0;
                        for (int i = TR::LVL_TR3_JUNGLE; i <= TR::LVL_TR3_STPAUL; i++)
                            if (!TR::isCutsceneLevel(TR::LevelID(i))) {
                                passportSlots[passportSlotCount++] = TR::LevelID(i);
                            }
                    #endif
                        break;
                    default : ASSERT(false);
                }

                for (int i = 0; i < passportSlotCount; i++) {
                    OptionItem item;
                    item.type   = OptionItem::TYPE_BUTTON;
                    item.offset = intptr_t(TR::LEVEL_INFO[passportSlots[i]].title); // offset as int pointer to level title string
                    item.color  = -passportSlots[i]; // color as level ID
                    optLoadSlots.push(item);
                }
            #endif
*/
        }

        const OptionItem* getOptions(int &optCount) const {
            switch (type) {
                case TR::Entity::INV_PASSPORT :
                    if (value != 0) return NULL;
                    optCount = optLoadSlots.length;
                    return optLoadSlots;
                case TR::Entity::INV_DETAIL :
                    optCount = COUNT(optDetail);
                    return optDetail;
                case TR::Entity::INV_SOUND :
                    optCount = COUNT(optSound);
                    return optSound;
                case TR::Entity::INV_CONTROLS :
                    ASSERT(optControls[2].offset == SETTINGS( playerIndex) );
                    for (int i = 0; i < COUNT(optControls); i++) {
                        OptionItem &opt = optControlsPlayer[i];
                        opt = optControls[i];

                        if (i > 2 && opt.offset != SETTINGS( playerIndex ) && opt.offset != SETTINGS( ctrlIndex ) )
                            opt.offset += sizeof(Core::Settings::Controls) * Core::settings.playerIndex;

                        if (opt.type == OptionItem::TYPE_KEY) {
                            if (Core::settings.ctrlIndex == 1) {
                                opt.offset++; // add offset to joy
                                opt.color = STR_JOY_FIRST;
                            } else
                                opt.color = STR_KEY_FIRST;
                        }
                    }
                    optCount = COUNT(optControlsPlayer);
                    return optControlsPlayer;
                default :
                    optCount = 0;
                    return NULL;
            }
        }

        Core::Settings& getSettings(Inventory *inv) const {
            return (type == TR::Entity::INV_SOUND || type == TR::Entity::INV_CONTROLS) ? Core::settings : inv->settings;
        }

        void nextSlot(int &slot, int dir) {
            int optCount;
            const OptionItem *options = getOptions(optCount);
            if (!options) return;

            int rep = 0;
            do {
                slot = (slot + dir + optCount) % optCount;
            // check for looping (no available slot)
                if (slot == 0) {
                    rep++;
                    if (rep > 1) {
                        slot = -1;
                        return;
                    }
                }
            } while (options[slot].type == OptionItem::TYPE_TITLE || options[slot].type == OptionItem::TYPE_EMPTY);
        }

        const OptionItem* control(int &slot, ControlKey key, float &timer, Core::Settings *settings) {
            int optCount;
            const OptionItem *opt = getOptions(optCount);
            if (!opt) return NULL;

            opt = opt + slot;

            uint8 &value = *(uint8*)(intptr_t(settings) + opt->offset);

            switch (key) {
                case cAction : return (opt->type == OptionItem::TYPE_BUTTON || opt->type == OptionItem::TYPE_KEY) ? opt : NULL;
                case cUp     : nextSlot(slot, -1); break;
                case cDown   : nextSlot(slot, +1); break;
                case cLeft   :
                    if (opt->type == OptionItem::TYPE_PARAM && opt->checkValue(value - 1)) {
                        value--;
                        timer = 0.2f;
                        return opt;
                    }    
                    break;
                case cRight  :
                    if (opt->type == OptionItem::TYPE_PARAM && opt->checkValue(value + 1)) {
                        value++;
                        timer = 0.2f;
                        return opt;
                    }    
                    break;
                default      : ;
            }

            return NULL;
        }

        void update(bool chosen, float phaseChoose) {
            if (!anim) return;
            anim->update();

            if (type == TR::Entity::INV_PASSPORT && chosen) {
                float t = (14 + value * 5) / 30.0f;

                if ( (anim->dir > 0.0f && anim->time > t) ||
                     (anim->dir < 0.0f && anim->time < t)) {
                    anim->dir = 0.0f;
                    anim->time = t;
                    anim->updateInfo();
                } 
            }
 
            if (type == TR::Entity::INV_COMPASS) {
                params.z = params.z + (params.y - params.x) * Core::deltaTime * 8.0f; // acceleration
                params.z -= params.z * Core::deltaTime; // damping
                params.x += params.z * Core::deltaTime; // apply speed
                if (chosen && anim->dir > 0.0f) {
                    if (phaseChoose >= 1.0f) {
                        float t = 7.0f / 30.0f;
                        if (anim->time > t) {
                            anim->dir = 0.0f;
                            anim->time = t;
                            anim->updateInfo();
                        }
                    } else {
                        anim->time = 0.0f;
                        anim->updateInfo();
                    }
                }
            }
        }

        void render(IGame *game, const Basis &basis) {
            if (!anim) return;

            MeshBuilder *mesh = game->getMesh();

            TR::Level *level = game->getLevel();
            TR::Model &m     = level->models[desc.model];
            Basis joints[MAX_SPHERES];

            mat4 matrix;
            matrix.identity();
            matrix.setRot(basis.rot);
            matrix.setPos(basis.pos);

            anim->getJoints(matrix, -1, true, joints);

            if (m.type == TR::Entity::INV_COMPASS) { // override needle animation
                joints[1].rotate(quat(vec3(0.0f, 1.0f, 0.0f), -params.x));
            }

            Core::setBasis(joints, m.mCount);

            mesh->renderModelFull(desc.model);
        }

        void choose() {
            if (!anim) return;            
            anim->setAnim(0, 0, false);
        }

    } *items[INVENTORY_MAX_ITEMS];

    static void loadTitleBG(Stream *stream, void *userData) {
        Inventory *inv = (Inventory*)userData;

        if (!inv->video)
            inv->skipVideo(); // play background track etc.

        if (!stream) {
            inv->titleTimer = 0.0f;
            return;
        }
        inv->titleTimer = inv->game->getLevel()->isTitle() ? 0.0f : 3.0f;

        inv->background[0] = Texture::Load(*stream);
        delete stream;
    }

    static void loadVideo(Stream *stream, void *userData) {
        Inventory *inv = (Inventory*)userData;
        if (stream)
            inv->video = new Video(stream);
        new Stream(TR::getGameScreen(inv->game->getLevel()->id), loadTitleBG, inv);
    }

    static void loadLogo(Stream *stream, void *userData) {
        Inventory *inv = (Inventory*)userData;
        if (stream)
            inv->video = new Video(stream);
        else
            inv->skipVideo();
    }

    Inventory() : game(NULL), itemsCount(0) {
        memset(background, 0, sizeof(background));
        reset();
    }

    ~Inventory() {
        delete video;
        clear();
    }

    void clear() {
        for (int i = 0; i < itemsCount; i++)
            delete items[i];
        itemsCount = 0;

        for (int i = 0; i < COUNT(background); i++) {
            delete background[i];
            background[i] = NULL;
        }
    }

    void reset() {
        clear();
        active      = false;
        chosen      = false;
        index       = targetIndex = 0;
        page        = targetPage = PAGE_OPTION;

        playerIndex = 0;
        changeTimer = 0.0f;
        nextLevel   = TR::LVL_MAX;
        lastKey     = cMAX;

        phaseRing = phasePage = phaseChoose = phaseSelect = 0.0f;
        memset(pageItemIndex, 0, sizeof(pageItemIndex));

        waitForKey = NULL;
        video      = NULL;

        titleTimer = TITLE_LOADING;

        if (!game) return;

        TR::Level *level = game->getLevel();

        add(TR::Entity::INV_PASSPORT);
        add(TR::Entity::INV_DETAIL);
        add(TR::Entity::INV_SOUND);
        add(TR::Entity::INV_CONTROLS);

        if (!level->isTitle() && !level->isCutsceneLevel() && !level->isHome()) {
            if (!TR::isEmptyLevel(level->id)) {
                add(TR::Entity::INV_PISTOLS, UNLIMITED_AMMO);
            }

            if (level->id == TR::LVL_TR2_HOUSE) {
                add(TR::Entity::INV_KEY_ITEM_1);
                add(TR::Entity::INV_PUZZLE_1);
            }
        #ifdef _DEBUG
            addWeapons();
            add(TR::Entity::MEDIKIT_BIG);
            add(TR::Entity::MEDIKIT_SMALL, 2);
            add(TR::Entity::INV_KEY_ITEM_1, 3);
            add(TR::Entity::INV_KEY_ITEM_2, 3);
            add(TR::Entity::INV_KEY_ITEM_3, 3);
            add(TR::Entity::INV_KEY_ITEM_4, 3);

            add(TR::Entity::INV_PUZZLE_1, 3);
            add(TR::Entity::INV_PUZZLE_2, 3);
            add(TR::Entity::INV_PUZZLE_3, 3);
            add(TR::Entity::INV_PUZZLE_4, 3);

            add(TR::Entity::INV_EXPLOSIVE);
            add(TR::Entity::INV_LEADBAR, 3);
        #endif
        } 

        if (level->isTitle()) {
            add(TR::Entity::INV_HOME);
        } else {
            add(TR::Entity::INV_COMPASS);
            add(TR::Entity::INV_STOPWATCH);
        }
    }

    void addWeapons() {
        TR::Level *level = game->getLevel();
        if (level->isTitle() || level->isCutsceneLevel() || level->isHome())
            return;

        if (level->version & TR::VER_TR1) {
            add(TR::Entity::INV_PISTOLS, UNLIMITED_AMMO);
            add(TR::Entity::INV_SHOTGUN, 250);
            add(TR::Entity::INV_MAGNUMS, 20);
            add(TR::Entity::INV_UZIS,    100);
        }
    }

    void startVideo() {
        applySounds(true);
        new Stream(playVideo ? TR::getGameVideo(game->getLevel()->id) : NULL, loadVideo, this);
    }

    void init(bool playLogo, bool playVideo) {
        this->playLogo  = playLogo;
        this->playVideo = playVideo;

        if (playLogo) {
            new Stream(TR::getGameLogo(game->getLevel()->version), loadLogo, this);
            return;
        }

        if (playVideo)
            startVideo();
        else 
            new Stream(TR::getGameScreen(game->getLevel()->id), loadTitleBG, this);
    }

    bool isActive() {
        return active || phaseRing > 0.0f;
    }

    int contains(TR::Entity::Type type) {
        type = TR::Level::convToInv(type);
        for (int i = 0; i < itemsCount; i++)
            if (items[i]->type == type)
                return i;
        return -1;
    }

    void addAmmo(TR::Entity::Type &type, int &count, int clip, TR::Entity::Type wpnType, TR::Entity::Type ammoType) {
        if (type == wpnType) {
            count *= clip;
            int index = contains(ammoType);
            if (index > -1) {
                count += items[index]->count * clip;
                remove(index);
            }
        } else {
            if (contains(wpnType) > -1) {
                type = wpnType;
                count *= clip;
            }
        }
    }

    void add(TR::Entity::Type type, int count = 1, bool smart = true) {
        type = TR::Level::convToInv(type);

        if (smart) {
            switch (type) {
                case TR::Entity::INV_PISTOLS      :
                case TR::Entity::INV_AMMO_PISTOLS :
                    count = UNLIMITED_AMMO; // pistols always has unlimited ammo
                    addAmmo(type, count, 10, TR::Entity::INV_PISTOLS, TR::Entity::INV_AMMO_PISTOLS);
                    break;
                case TR::Entity::INV_SHOTGUN      :
                case TR::Entity::INV_AMMO_SHOTGUN :
                    addAmmo(type, count,  2, TR::Entity::INV_SHOTGUN, TR::Entity::INV_AMMO_SHOTGUN);
                    break;
                case TR::Entity::INV_MAGNUMS      :
                case TR::Entity::INV_AMMO_MAGNUMS :
                    addAmmo(type, count, 25, TR::Entity::INV_MAGNUMS, TR::Entity::INV_AMMO_MAGNUMS);
                    break;
                case TR::Entity::INV_UZIS         :
                case TR::Entity::INV_AMMO_UZIS    :
                    addAmmo(type, count, 50, TR::Entity::INV_UZIS, TR::Entity::INV_AMMO_UZIS);
                    break;
                default : ;
            }
        }

        int i = contains(type);
        if (i > -1) {
            items[i]->count += count;
            items[i]->count = min(UNLIMITED_AMMO, items[i]->count);
            return;
        }

        ASSERT(itemsCount < INVENTORY_MAX_ITEMS);

        count = min(UNLIMITED_AMMO, count);

        Item *newItem = new Item(game->getLevel(), type, count);
        if (newItem->desc.model == -1) {
            delete newItem;
            return;
        }

        int pos = 0;
        for (int pos = 0; pos < itemsCount; pos++)
            if (items[pos]->type > type)
                break;

        if (pos - itemsCount) {
            for (int i = itemsCount; i > pos; i--)
                items[i] = items[i - 1];
        }

        items[pos] = newItem;
        itemsCount++;
    }

    int* getCountPtr(TR::Entity::Type type) {
        int i = contains(type);
        if (i < 0) return NULL;
        return &items[i]->count;
    }

    void remove(TR::Entity::Type type, int count = 1) {
        int index = contains(type);
        if (index == -1) return;

        items[index]->count -= count;
        if (items[index]->count <= 0)
            remove(index);
    }

    void remove(int index) {
        delete items[index];
        for (int i = index; i < itemsCount - 1; i++)
            items[i] = items[i + 1];
        itemsCount--;
    }
    
    bool chooseKey(int playerIndex, TR::Entity::Type hole) {
        TR::Entity::Type type = TR::Entity::getItemForHole(hole);
        if (type == TR::Entity::NONE)
            return false;
        int index = contains(type);
        if (index < 0)
            return false;
        toggle(playerIndex, items[index]->desc.page, type);
        return true;
    }

    bool use(TR::Entity::Type type) {
        if (contains(type) > -1) {
            remove(type);
            return true;
        }
        return false;
    }

    void applySounds(bool pause) {
        for (int i = 0; i < Sound::channelsCount; i++)
            if (Sound::channels[i]->flags & Sound::PAN) {
                if (pause)
                    Sound::channels[i]->pause();
                else
                    Sound::channels[i]->resume();
            }
    }

    void toggle(int playerIndex = 0, Page curPage = PAGE_INVENTORY, TR::Entity::Type type = TR::Entity::NONE) {
        if (titleTimer != 0.0f || (isActive() != active))
            return;

        Input::stopJoyVibration();
        
        this->playerIndex = playerIndex;
        titleTimer = 0.0f;

        if (phaseRing == 0.0f || phaseRing == 1.0f) {
            active = !active;
            vec3 p;

            applySounds(active);

            if (curPage == PAGE_SAVEGAME) {
                phaseRing = active ? 1.0f : 0.0f;
                slot = 1;
            } else {
                if (curPage != PAGE_LEVEL_STATS)
                    game->playSound(active ? TR::SND_INV_SHOW : TR::SND_INV_HIDE, p);
            }

            chosen = false;

            if (active) {
                if (curPage != PAGE_LEVEL_STATS) {
                    for (int i = 0; i < itemsCount; i++)
                        items[i]->reset();
                }

                nextLevel   = TR::LVL_MAX;
                phasePage   = 1.0f;
                phaseSelect = 1.0f;
                page        = targetPage  = curPage;

                if (type != TR::Entity::NONE) {
                    int i = contains(type);
                    if (i >= 0)
                        pageItemIndex[page] = getLocalIndex(i);
                }

                index = targetIndex = pageItemIndex[page];

                head.e00 = INF; // mark head matrix as unset

                //if (type == TR::Entity::INV_PASSPORT) // toggle after death
                //    chooseItem();
            }
        }
    }

    void doPhase(bool increase, float speed, float &value) {
        if (increase) {
            if (value < 1.0f) {
                value += Core::deltaTime * speed;
                if (value > 1.0f)
                    value = 1.0f;
            }
        } else {
            if (value > 0.0f) {
                value -= Core::deltaTime * speed;
                if (value < 0.0f)
                    value = 0.0f;
            }
        }
    }

    int getGlobalIndex(Page page, int localIndex) {
        for (int i = 0; i < itemsCount; i++)
            if (items[i]->desc.page == page) {
                if (!localIndex)
                    return i;
                localIndex--;
            }
        return 0;
    }

    int getLocalIndex(int globalIndex) {
        Page page = items[globalIndex]->desc.page;

        int idx = 0;
        for (int i = 0; i < globalIndex; i++)
            if (items[i]->desc.page == page)
                idx++;

        return idx;
    }

    float getAngle(int index, int count) {
        return PI * 2.0f / float(count) * index;
    }

    int getItemsCount(int page) {
        int count = 0;

        for (int i = 0; i < itemsCount; i++)
            if (items[i]->desc.page == page)
                count++;

        return count;
    }

    bool showHealthBar() {
        if (!itemsCount) return false;
        int idx = getGlobalIndex(page, index);
        TR::Entity::Type type = items[idx]->type;
        return active && phaseRing == 1.0f && index == targetIndex && phasePage == 1.0f && (type == TR::Entity::INV_MEDIKIT_SMALL || type == TR::Entity::INV_MEDIKIT_BIG);
    }

    void onChoose(Item *item) {
        TR::Level *level = game->getLevel();

        switch (item->type) {
            case TR::Entity::INV_PASSPORT : {
                game->playSound(TR::SND_INV_PAGE);
                item->value = 1;
                item->initLoadSlots(level);
                break;
            }
            case TR::Entity::INV_COMPASS : {
                float angle = normalizeAngle(game->getLara(playerIndex)->angle.y);
                item->params = vec4(angle + (randf() - 0.5f) * PI * 2.0f, angle, 0.0f, 0.0f); // initial angle, target angle, initial speed, unused
                break;
            }
            case TR::Entity::INV_CONTROLS :
                Core::settings.playerIndex = 0;
                Core::settings.ctrlIndex   = 0;
                break;
            case TR::Entity::INV_DETAIL :
                settings = Core::settings;
                break;
            default : ;
        }

        slot = -1;
        item->nextSlot(slot, 1);
    }

    void controlItem(Item *item, ControlKey key) {
        TR::Level *level = game->getLevel();

        if (item->type == TR::Entity::INV_PASSPORT) {
        // passport pages
            int oldValue = item->value;
            if (key == cLeft  && item->value > 0) { item->value--; item->anim->dir = -1.0f; game->playSound(TR::SND_INV_PAGE); }
            if (key == cRight && item->value < 2) { item->value++; item->anim->dir =  1.0f; game->playSound(TR::SND_INV_PAGE); }

            if (item->value != oldValue) {
                slot = 0;
                item->nextSlot(slot, -1);
            }

            if (key == cAction && phaseChoose == 1.0f && item->value != 0) {
                TR::LevelID id = level->id;
                switch (item->value) {
                    case 1 : {
                        if (level->isTitle()) { // start new game
                            nextLevel = level->getStartId();
                        } else { // restart level
                            int slot = getSaveSlot(id, false);
                            if (slot > -1)
                                game->loadGame(slot);
                            else
                                nextLevel = id; 
                            toggle();
                        }
                        break;
                    }
                    case 2 : 
                        if (!level->isTitle())
                            nextLevel = level->getTitleId();
                        else
                            Core::quit(); // exit game
                        break;
                }

                if (nextLevel != TR::LVL_MAX) {
                    item->anim->dir = -1.0f;
                    item->value = -100;
                    toggle();
                }
            }
        }

        Core::Settings &stg = item->getSettings(this);

        const OptionItem *opt = item->control(slot, key, changeTimer, &stg);
        if (opt)
            optionChanged(item, opt, stg);

        if (item->type == TR::Entity::INV_HOME && phaseChoose == 1.0f && key == cAction) {
            nextLevel = level->getHomeId();
            toggle();
        }
    }

    void optionChanged(Item *item, const OptionItem *opt, Core::Settings &settings) {
        if (item->type == TR::Entity::INV_PASSPORT) {
        #ifdef _DEBUG
            if (int(opt->color) < 0)
                nextLevel = TR::LevelID(-int(opt->color));
            else
        #endif
                game->loadGame(opt->color);
            item->anim->dir = -1.0f;
            item->value = -100;
            toggle();
        }

        if (item->type == TR::Entity::INV_SOUND) {
            game->applySettings(settings);
            if (opt->offset == SETTINGS( audio.sound ) )
                game->playSound(TR::SND_PISTOLS_SHOT);
        }

        if (item->type == TR::Entity::INV_CONTROLS && opt->type == OptionItem::TYPE_KEY) {
            waitForKey = opt;
            Input::lastKey = ikNone;
            Input::joy[Core::settings.controls[Core::settings.playerIndex].joyIndex].lastKey = jkNone;
        }

        if (item->type == TR::Entity::INV_SOUND || item->type == TR::Entity::INV_CONTROLS) {
            game->applySettings(settings);
        }

        if (item->type == TR::Entity::INV_DETAIL && opt->title == STR_APPLY) {
            game->applySettings(settings);
            chosen = false;
        }
    };

    void skipVideo() {
        delete video;
        video = NULL;

        if (playLogo) {
            playLogo = false;
            if (playVideo) {
                startVideo();
                return;
            }
        }
        playVideo = false;

        game->playTrack(0);
        if (game->getLevel()->isTitle()) {
            titleTimer = 0.0f;
            toggle(0, Inventory::PAGE_OPTION);
        }
        Input::reset();
        applySounds(false);
    }

    void update() {
        if (Input::lastState[0] == cInventory || Input::lastState[0] == cAction ||
            Input::lastState[1] == cInventory || Input::lastState[1] == cAction)
        {
            if (video) {
                skipVideo();
            } else if (titleTimer > 1.0f && titleTimer < 2.5f) {
                titleTimer = 1.0f;
            }
        }

        if (video) {
            video->update();
            if (video->isPlaying)
                return;
            skipVideo();
        }

        if (video || titleTimer == TITLE_LOADING) return;

        if (titleTimer != TITLE_LOADING && titleTimer > 0.0f) {
            titleTimer -= Core::deltaTime;
            if (titleTimer < 0.0f)
                titleTimer = 0.0f;
        }

        if (!isActive()) {
            lastKey = cMAX;
            return;
        }

        float lastChoose = phaseChoose;

        if (phaseChoose == 0.0f)
            doPhase(active, 2.0f, phaseRing);
        doPhase(true,   1.6f, phasePage);
        doPhase(chosen, 4.0f, phaseChoose);
        doPhase(true,   2.5f, phaseSelect);

        if (phaseChoose == 1.0f && lastChoose != 1.0f)
            onChoose(items[getGlobalIndex(page, index)]);

        if (page != targetPage && phasePage == 1.0f) {
            page  = targetPage;
            index = targetIndex = pageItemIndex[page];
        }

        if (index != targetIndex && phaseSelect == 1.0f)
            index = pageItemIndex[page] = targetIndex;

        int count = getItemsCount(page);

        bool ready = active && phaseRing == 1.0f && phasePage == 1.0f;

        Input::Joystick &joy     = Input::joy[Core::settings.controls[playerIndex].joyIndex];
        Input::Joystick &joyMain = Input::joy[0];

        ControlKey key = cMAX;
        if (Input::down[ikCtrl] || Input::down[ikEnter] || Input::lastState[playerIndex] == cAction || joy.down[jkA])
            key = cAction;
        else if (Input::down[ikAlt]   || joy.down[jkB]     || Input::lastState[playerIndex] == cInventory)
            key = cInventory;
        else if (Input::down[ikLeft]  || joy.down[jkLeft]  || joy.L.x < -0.5f || joyMain.down[jkLeft]  || joyMain.L.x < -0.5f)
            key = cLeft;
        else if (Input::down[ikRight] || joy.down[jkRight] || joy.L.x >  0.5f || joyMain.down[jkRight] || joyMain.L.x > 0.5f)
            key = cRight;
        else if (Input::down[ikUp]    || joy.down[jkUp]    || joy.L.y < -0.5f)
            key = cUp;
        else if (Input::down[ikDown]  || joy.down[jkDown]  || joy.L.y >  0.5f)
            key = cDown;

        Item *item = items[getGlobalIndex(page, index)];

        if (page == PAGE_LEVEL_STATS) {
            if (Input::lastState[playerIndex] != cMAX) {
                toggle(playerIndex, targetPage);
            }
        } else if (page == PAGE_SAVEGAME) {
            if (Input::lastState[playerIndex] == cLeft || Input::lastState[playerIndex] == cRight)
                slot ^= 1;

            if (Input::lastState[playerIndex] == cAction) {
                if (slot == 1) {
                    if (index > -1) {
                        TR::Entity &e = game->getLevel()->entities[index];
                        Controller *controller = (Controller*)e.controller;
                        controller->deactivate(true);
                    }
                    game->saveGame(game->getLevel()->id, index > -1, false);
                }
                toggle(playerIndex, targetPage);
            }

        } else if (index == targetIndex && targetPage == page && ready) {
            if (!chosen) {
                if ((key == cUp && !canFlipPage(-1)) || (key == cDown && !canFlipPage( 1)))
                    key = cMAX;

                switch (key) {
                    case cLeft  : { phaseSelect = 0.0f; targetIndex = (targetIndex - 1 + count) % count; } break;
                    case cRight : { phaseSelect = 0.0f; targetIndex = (targetIndex + 1) % count;         } break;
                    case cUp    : { phasePage = 0.0f; targetPage = Page(page + 1); } break;
                    case cDown  : { phasePage = 0.0f; targetPage = Page(page - 1); } break;
                    default : ;
                }

                if (index != targetIndex)
                    game->playSound(TR::SND_INV_SPIN);

                if (lastKey != key && key == cAction && phaseChoose == 0.0f)
                    chooseItem();
            } else {
                if (changeTimer > 0.0f) {
                    changeTimer -= Core::deltaTime;
                    if (changeTimer <= 0.0f) {
                        changeTimer = 0.0f;
                        lastKey = cMAX;
                    }
                }

                if (waitForKey) {
                    int newKey = -1;
                    if (Core::settings.ctrlIndex == 0 && Input::lastKey != ikNone) {
                        newKey = Input::lastKey;
                    } else {
                        JoyKey jk = Input::joy[Core::settings.controls[Core::settings.playerIndex].joyIndex].lastKey;
                        if (Core::settings.ctrlIndex == 1 && jk != jkNone)
                            newKey = jk;
                    }

                    if (newKey != -1) {
                        waitForKey->setValue(newKey, &Core::settings);
                        waitForKey = NULL;
                        lastKey = key;
                        game->applySettings(Core::settings);
                    }
                }

                if (key != cMAX && lastKey != key && changeTimer == 0.0f && phaseChoose == 1.0f) {
                    controlItem(item, key);
                }
            }

            if ((key == cInventory || key == cJump) && lastKey != key) {
                lastKey = key;
                if (chosen) {
                    if (phaseChoose == 1.0f) {
                        chosen  = false;
                        item->anim->dir = 1.0f;
                        item->angle     = 0.0f;
                    }
                } else
                    if (!game->getLevel()->isTitle())
                        toggle(playerIndex, targetPage);
            }
        }
        lastKey = key;

        if (page == PAGE_SAVEGAME || page == PAGE_LEVEL_STATS)
            return;

        ready = active && phaseRing == 1.0f && phasePage == 1.0f;

        float w = 90.0f * DEG2RAD * Core::deltaTime;

        int itemIndex = index == targetIndex ? getGlobalIndex(page, index) : -1;

        for (int i = 0; i < itemsCount; i++) {
            items[i]->update(chosen && itemIndex == i, phaseChoose);
            float &angle = items[i]->angle;

            if (itemIndex != i || chosen) {
                if (angle == 0.0f) {
                    continue;
                } else if (angle < 0.0f) {
                    angle += w;
                    if (angle > 0.0f)
                        angle = 0.0f;
                } else if (angle > 0.0f) {
                    angle -= w;
                    if (angle < 0.0f)
                        angle = 0.0f;
                }
            } else
                angle += w;

            angle = clampAngle(angle);
        }

        if (ready && chosen && phaseChoose == 1.0f && item->anim->isEnded) {
            TR::Entity::Type type = item->type;
            
            switch (type) {
                case TR::Entity::INV_PASSPORT        :
                case TR::Entity::INV_PASSPORT_CLOSED :
                case TR::Entity::INV_MAP             :
                case TR::Entity::INV_COMPASS         :
                case TR::Entity::INV_STOPWATCH       :
                case TR::Entity::INV_HOME            :
                case TR::Entity::INV_DETAIL          :
                case TR::Entity::INV_SOUND           :
                case TR::Entity::INV_CONTROLS        :
                case TR::Entity::INV_GAMMA           :
                case TR::Entity::INV_AMMO_PISTOLS    :
                case TR::Entity::INV_AMMO_SHOTGUN    :
                case TR::Entity::INV_AMMO_MAGNUMS    :
                case TR::Entity::INV_AMMO_UZIS       : break;
                default :
                    game->invUse(playerIndex, type);
                    toggle();
            }
        }

        if (!isActive() && nextLevel != TR::LVL_MAX)
            game->loadLevel(nextLevel);
    }

    void chooseItem() {
        Item *item = items[getGlobalIndex(page, index)];

        vec3 p;
        chosen = true;
        switch (item->type) {
            case TR::Entity::INV_COMPASS  : game->playSound(TR::SND_INV_COMPASS);   break;
            case TR::Entity::INV_HOME     : game->playSound(TR::SND_INV_HOME);      break;
            case TR::Entity::INV_CONTROLS : game->playSound(TR::SND_INV_CONTROLS);  break;
            case TR::Entity::INV_PISTOLS  :
            case TR::Entity::INV_SHOTGUN  :
            case TR::Entity::INV_MAGNUMS  :
            case TR::Entity::INV_UZIS     : game->playSound(TR::SND_INV_WEAPON);    break;
            default                       : game->playSound(TR::SND_INV_SHOW);      break;
        }
        item->choose();
    }

    bool canFlipPage(int dir) {
        if (game->getLevel()->isTitle() || (game->getLara(playerIndex) && ((Character*)game->getLara(playerIndex))->health <= 0.0f))
            return false;
        if (dir == -1) return page < PAGE_ITEMS  && getItemsCount(page + 1);
        if (dir ==  1) return page > PAGE_OPTION && getItemsCount(page - 1);
        return false;
    }

    Texture* getBackgroundTarget() {
        if (background[0] && (background[0]->origWidth != INVENTORY_BG_SIZE || background[0]->origHeight != INVENTORY_BG_SIZE)) {
            delete background[0];
            background[0] = NULL;
        }

        for (int i = 0; i < COUNT(background); i++)
            if (!background[i])
                background[i] = new Texture(INVENTORY_BG_SIZE, INVENTORY_BG_SIZE, FMT_RGBA, OPT_TARGET);

        return background[0];
    }

    void prepareBackground() {
        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR)
            return;

        #ifdef _OS_PSP
            return;
        #endif
        Core::defaultTarget = getBackgroundTarget();
        game->renderGame(false);
        Core::defaultTarget = NULL;

        Core::setDepthTest(false);
        Core::setBlendMode(bmNone);

    #ifdef FFP
        mat4 m;
        m.identity();
        Core::setViewProj(m, m);
        Core::mModel.identity();
    #endif

    #ifdef _OS_PSP
        //
    #else
        // vertical blur
        Core::setTarget(background[1], RT_STORE_COLOR);
        game->setShader(Core::passFilter, Shader::FILTER_BLUR, false, false);
        Core::active.shader->setParam(uParam, vec4(0, 1.0f / INVENTORY_BG_SIZE, 0, 0));
        background[0]->bind(sDiffuse);
        game->getMesh()->renderQuad();

        // horizontal blur
        Core::setTarget(background[0], RT_STORE_COLOR);
        game->setShader(Core::passFilter, Shader::FILTER_BLUR, false, false);
        Core::active.shader->setParam(uParam, vec4(1.0f / INVENTORY_BG_SIZE, 0, 0, 0));
        background[1]->bind(sDiffuse);
        game->getMesh()->renderQuad();

        // grayscale
        Core::setTarget(background[1], RT_STORE_COLOR);
        game->setShader(Core::passFilter, Shader::FILTER_GRAYSCALE, false, false);
        Core::active.shader->setParam(uParam, vec4(0.75f, 0.75f, 1.0f, 1.0f));
        background[0]->bind(sDiffuse);
        game->getMesh()->renderQuad();

        swap(background[0], background[1]);
    #endif

        Core::setDepthTest(true);
    }

    void renderItemCount(const Item *item, const vec2 &pos, float width) {
        char spec;
        switch (item->type) {
            case TR::Entity::INV_SHOTGUN : spec = 12; break;
            case TR::Entity::INV_MAGNUMS : spec = 13; break;
            case TR::Entity::INV_UZIS    : spec = 14; break;
            default                      : spec = 0;
        }

        if ((item->count > 1 || spec) && item->count < UNLIMITED_AMMO) {
            char buf[16];
            sprintf(buf, "%d %c", item->count, spec);
            for (int i = 0; buf[i] != ' '; i++)
                buf[i] -= 47;
            UI::textOut(pos, buf, UI::aRight, width, 255, UI::SHADE_NONE);
        }
    }

    void renderPassport(Item *item) {
        if (item->anim->dir != 0.0f) return; // check for "Load Game" page

        StringID str = STR_LOAD_GAME;

        if (game->getLevel()->isTitle()) {
            if (item->value == 1) str = STR_START_GAME;
            if (item->value == 2) str = STR_EXIT_GAME;
        } else {
            if (item->value == 1) str = STR_RESTART_LEVEL;
            if (item->value == 2) str = STR_EXIT_TO_TITLE;
        }

        UI::textOut(vec2(0, 480 - 32), str, UI::aCenter, UI::width);
        int tw = UI::getTextSize(STR[str]).x;

        if (item->value > 0) UI::specOut(vec2((UI::width - tw) * 0.5f - 32.0f, 480 - 32), 108);
        if (item->value < 2) UI::specOut(vec2((UI::width + tw) * 0.5f + 16.0f, 480 - 32), 109);

        if (item->value != 0) return;

        if (slot == -1) {
            //
        } else
            renderOptions(item);
    }

    void renderOptions(Item *item) {
        int optionsCount;
        const OptionItem *options = item->getOptions(optionsCount);

        if (!options)
            return;

        float width  = 320.0f;
        float height = optionsCount * LINE_HEIGHT + 8;

        float eye = UI::width * Core::eye * 0.02f;
        float x = ( UI::width  - width  ) * 0.5f - eye;
        float y = ( UI::height - height ) * 0.5f + LINE_HEIGHT;

    // background
        UI::renderBar(UI::BAR_OPTION, vec2(x, y - 16.0f), vec2(width, height), 0.0f, 0, 0xC0000000);

        x     += 8.0f;
        width -= 16.0f;

        Core::Settings &stg = item->getSettings(this);
        for (int i = 0; i < optionsCount; i++)
            y = options[i].render(x, y, width, slot == i, &stg);
    }


    StringID getItemName(StringID def, TR::LevelID id, TR::Entity::Type type) {
        if (!TR::Entity::isPuzzleItem(type) && !TR::Entity::isKeyItem(type))
            return def;

        #define LVLCHECK(L, T, S) if (id == TR::L && type == TR::Entity::INV_##T) return S;

        LVLCHECK(LVL_TR1_2,     KEY_ITEM_1, STR_KEY_SILVER);
        LVLCHECK(LVL_TR1_2,     PUZZLE_1,   STR_PUZZLE_GOLD_IDOL);

        LVLCHECK(LVL_TR1_3A,    PUZZLE_1,   STR_PUZZLE_COG);

        LVLCHECK(LVL_TR1_4,     KEY_ITEM_1, STR_KEY_NEPTUNE);
        LVLCHECK(LVL_TR1_4,     KEY_ITEM_2, STR_KEY_ATLAS);
        LVLCHECK(LVL_TR1_4,     KEY_ITEM_3, STR_KEY_DAMOCLES);
        LVLCHECK(LVL_TR1_4,     KEY_ITEM_4, STR_KEY_THOR);

        LVLCHECK(LVL_TR1_5,     KEY_ITEM_1, STR_KEY_RUSTY);

        LVLCHECK(LVL_TR1_6,     PUZZLE_1,   STR_PUZZLE_GOLD_BAR);

        LVLCHECK(LVL_TR1_7A,    KEY_ITEM_1, STR_KEY_GOLD);
        LVLCHECK(LVL_TR1_7A,    KEY_ITEM_2, STR_KEY_SILVER);
        LVLCHECK(LVL_TR1_7A,    KEY_ITEM_3, STR_KEY_RUSTY);

        LVLCHECK(LVL_TR1_7B,    KEY_ITEM_1, STR_KEY_GOLD);
        LVLCHECK(LVL_TR1_7B,    KEY_ITEM_2, STR_KEY_RUSTY);
        LVLCHECK(LVL_TR1_7B,    KEY_ITEM_3, STR_KEY_RUSTY);

        LVLCHECK(LVL_TR1_8A,    KEY_ITEM_1, STR_KEY_SAPPHIRE);

        LVLCHECK(LVL_TR1_8B,    KEY_ITEM_1, STR_KEY_SAPPHIRE);
        LVLCHECK(LVL_TR1_8B,    PUZZLE_2,   STR_PUZZLE_SCARAB);
        LVLCHECK(LVL_TR1_8B,    PUZZLE_3,   STR_PUZZLE_HORUS);
        LVLCHECK(LVL_TR1_8B,    PUZZLE_4,   STR_PUZZLE_ANKH);
        LVLCHECK(LVL_TR1_8B,    PUZZLE_1,   STR_PUZZLE_HORUS);

        LVLCHECK(LVL_TR1_8C,    KEY_ITEM_1, STR_KEY_GOLD);
        LVLCHECK(LVL_TR1_8C,    PUZZLE_1,   STR_PUZZLE_ANKH);
        LVLCHECK(LVL_TR1_8C,    PUZZLE_2,   STR_PUZZLE_SCARAB);

        LVLCHECK(LVL_TR1_10A,   PUZZLE_1,   STR_PUZZLE_FUSE);
        LVLCHECK(LVL_TR1_10A,   PUZZLE_2,   STR_PUZZLE_PYRAMID);

        LVLCHECK(LVL_TR1_EGYPT, KEY_ITEM_1, STR_KEY_GOLD);
        LVLCHECK(LVL_TR1_CAT,   KEY_ITEM_1, STR_KEY_ORNATE);

        #undef LVLCHECK

        return def;
    }

    void renderItemText(float eye, Item *item) {
        if (item->type == TR::Entity::INV_PASSPORT && phaseChoose == 1.0f) {
            //
        } else {
            StringID str = getItemName(item->desc.str, game->getLevel()->id, item->type);
            UI::textOut(vec2(-eye, 480 - 32), str, UI::aCenter, UI::width);
        }

        renderItemCount(item, vec2(UI::width / 2 - 160 - eye, 480 - 96), 320);

    // show health bar in inventory when selector is over medikit
        if (item->type == TR::Entity::INV_MEDIKIT_BIG || item->type == TR::Entity::INV_MEDIKIT_SMALL) {
            Character *lara = (Character*)game->getLara(playerIndex);
            if (lara) {
                float health = lara->health / 1000.0f; // LARA_MAX_HEALTH

                vec2 size = vec2(180, 10);
                vec2 pos;
                if (Core::settings.detail.stereo == Core::Settings::STEREO_VR)
                    pos = vec2((UI::width - size.x) * 0.5f - eye * 4.0f, 96);
                else
                    pos = vec2(UI::width - 32 - size.x - eye, 32);

                UI::renderBar(UI::BAR_HEALTH, pos, size, health);
            }
        }

        if (phaseChoose == 1.0f) {
            switch (item->type) {
                case TR::Entity::INV_PASSPORT :
                    renderPassport(item);
                    break;
                case TR::Entity::INV_DETAIL    :
                case TR::Entity::INV_SOUND     :
                case TR::Entity::INV_CONTROLS  :
                    renderOptions(item);
                    break;
                case TR::Entity::INV_GAMMA     :
                case TR::Entity::INV_STOPWATCH :
                case TR::Entity::INV_MAP       :
                    UI::textOut(vec2(-eye, 240), STR_NOT_IMPLEMENTED, UI::aCenter, UI::width);
                    break;
                default : ;
            }
        }
    }

    void renderPage(int page) {
        float phase = page == targetPage ? phasePage : (1.0f - phasePage);

        float alpha = 1.0f - phaseRing * phase;
        alpha *= alpha;
        alpha = 1.0f - alpha;
        Core::setMaterial(1.0f, 0.0f, 0.0f, alpha);

        int count = getItemsCount(page);

        vec2 cpos(1286, 256 + 1280 * (1.0f - phaseRing));
        float ringTilt      = cpos.angle();
        float radius        = phaseRing * INVENTORY_MAX_RADIUS * phase;
        float collapseAngle = phaseRing * phase * PI - PI;
        float ringHeight    = lerp(float(this->page), float(targetPage), hermite(phasePage)) * INVENTORY_HEIGHT;
        float angle         = getAngle(pageItemIndex[page], count);

        if (phaseSelect < 1.0f)
            angle = lerpAngle(angle, getAngle(targetIndex, count), hermite(phaseSelect));
        
        Basis basis = Basis(quat(vec3(1, 0, 0), ringTilt), vec3(0));

        int itemIndex = 0;
        for (int i = 0; i < itemsCount; i++) {
            Item *item = items[i];

            if (item->desc.page != page)
                continue;

            float a  = getAngle(itemIndex, count) - angle - collapseAngle;
            float ia = item->angle;
            float rd = radius;
            float rh = ringHeight;

            if (itemIndex == pageItemIndex[page] && (chosen || phaseChoose > 0.0f)) {
                ia *= 1.0f - phaseChoose;
                rh -=  128 * phaseChoose;
                rd +=  296 * phaseChoose;
            }

            Basis b = basis * Basis(quat(vec3(0, 1, 0), PI + ia - a), vec3(sinf(a), 0, -cosf(a)) * rd - vec3(0, item->desc.page * INVENTORY_HEIGHT - rh, 0));

            if (item->type == TR::Entity::INV_COMPASS) {
                b.rotate(quat(vec3(1.0f, 0.0f, 0.0f), -phaseChoose * PI * 0.1f));
            }

            item->render(game, b);

            itemIndex++;
        }
    }

    void renderTitleBG(float sx = 1.0f, float sy = 1.0f, uint8 alpha = 255) {
        float aspectSrc, aspectDst, aspectImg, ax, ay;

        if (background[0]) {
            float ox = sx * background[0]->origWidth;
            float oy = sy * background[0]->origHeight;
            aspectSrc = ox / oy;
            aspectDst = float(Core::width) / float(Core::height);
            ax = background[0]->origWidth  / float(background[0]->width);
            ay = background[0]->origHeight / float(background[0]->height);
        } else {
            aspectSrc = ax = ay = 1.0f;
            aspectDst = float(Core::width) / float(Core::height);
        }
        aspectImg = aspectSrc / aspectDst;

        #ifdef FFP
            mat4 m;
            m.identity();
            Core::setViewProj(m, m);
            Core::mModel.identity();
            Core::mModel.scale(vec3(1.0f / 32767.0f));
        #endif

        Core::setBlendMode(alpha < 255 ? bmAlpha : bmNone);

        Index  indices[6 * 3] = { 0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11 };
        Vertex vertices[4 * 3];

        short2 size;
        if (aspectImg < 1.0f) {
            size.x = short(32767 * aspectImg);
            size.y = 32767;

            vertices[ 4].coord = short4( -32767,  size.y, 0, 0);
            vertices[ 5].coord = short4(-size.x,  size.y, 0, 0);
            vertices[ 6].coord = short4(-size.x, -size.y, 0, 0);
            vertices[ 7].coord = short4( -32767, -size.y, 0, 0);

            vertices[ 8].coord = short4( size.x,  size.y, 0, 0);
            vertices[ 9].coord = short4(  32767,  size.y, 0, 0);
            vertices[10].coord = short4(  32767, -size.y, 0, 0);
            vertices[11].coord = short4( size.x, -size.y, 0, 0);
        } else {
            size.x = 32767;
            size.y = short(32767 / aspectImg);

            vertices[ 4].coord = short4(-size.x,  32767, 0, 0);
            vertices[ 5].coord = short4( size.x,  32767, 0, 0);
            vertices[ 6].coord = short4( size.x, size.y, 0, 0);
            vertices[ 7].coord = short4(-size.x, size.y, 0, 0);

            vertices[ 8].coord = short4(-size.x, -size.y, 0, 0);
            vertices[ 9].coord = short4( size.x, -size.y, 0, 0);
            vertices[10].coord = short4( size.x,  -32767, 0, 0);
            vertices[11].coord = short4(-size.x,  -32767, 0, 0);
        }

        short tw = short(ax * 32767);
        short th = short(ay * 32767);

        vertices[ 0].coord = short4(-size.x,  size.y, 0, 0);
        vertices[ 1].coord = short4( size.x,  size.y, 0, 0);
        vertices[ 2].coord = short4( size.x, -size.y, 0, 0);
        vertices[ 3].coord = short4(-size.x, -size.y, 0, 0);

        vertices[ 0].light =
        vertices[ 1].light =
        vertices[ 2].light =
        vertices[ 3].light = ubyte4(255, 255, 255, alpha);
        vertices[ 4].light = 
        vertices[ 5].light = 
        vertices[ 6].light = 
        vertices[ 7].light = 
        vertices[ 8].light = 
        vertices[ 9].light = 
        vertices[10].light = 
        vertices[11].light = ubyte4(0, 0, 0, alpha);

        vertices[ 0].texCoord = short4( 0,  0, 0, 0);
        vertices[ 1].texCoord = short4(tw,  0, 0, 0);
        vertices[ 2].texCoord = short4(tw, th, 0, 0);
        vertices[ 3].texCoord = short4( 0, th, 0, 0);
        vertices[ 4].texCoord =
        vertices[ 5].texCoord =
        vertices[ 6].texCoord =
        vertices[ 7].texCoord =
        vertices[ 8].texCoord =
        vertices[ 9].texCoord =
        vertices[10].texCoord =
        vertices[11].texCoord = short4(0, 0, 0, 0);

        if ((Core::settings.detail.stereo == Core::Settings::STEREO_VR && !video) || !background[0])
            Core::blackTex->bind(sDiffuse); // black background
        else
            background[0]->bind(sDiffuse);

        game->setShader(Core::passFilter, Shader::FILTER_UPSCALE, false, false);
        Core::active.shader->setParam(uParam, vec4(float(Core::active.textures[sDiffuse]->width), float(Core::active.textures[sDiffuse]->height), Core::getTime() * 0.001f, 0.0f));
        game->getMesh()->renderBuffer(indices, COUNT(indices), vertices, COUNT(vertices));
    }

    void renderGameBG() {
        Index  indices[6] = { 0, 1, 2, 0, 2, 3 };
        Vertex vertices[4];
        vertices[0].coord = short4(-32767,  32767, 0, 0);
        vertices[1].coord = short4( 32767,  32767, 0, 0);
        vertices[2].coord = short4( 32767, -32767, 0, 0);
        vertices[3].coord = short4(-32767, -32767, 0, 0);
        vertices[0].light =
        vertices[1].light =
        vertices[2].light =
        vertices[3].light = ubyte4(255, 255, 255, uint8(phaseRing * 255));
        vertices[0].texCoord = short4(    0, 32767, 0, 0);
        vertices[1].texCoord = short4(32767, 32767, 0, 0);
        vertices[2].texCoord = short4(32767,     0, 0, 0);
        vertices[3].texCoord = short4(    0,     0, 0, 0);

        Texture *backTex = NULL;
    #ifdef FFP
        backTex = Core::blackTex;

        mat4 m;
        m.identity();
        Core::setViewProj(m, m);
        Core::mModel.identity();
        Core::mModel.scale(vec3(1.0f / 32767.0f));
    #else
        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR || !background[0]) {
            backTex = Core::blackTex; // black background 
        } else
            backTex = background[0]; // blured grayscale image
    #endif
        backTex->bind(sDiffuse);

        game->setShader(Core::passFilter, Shader::FILTER_UPSCALE, false, false);
        Core::active.shader->setParam(uParam, vec4(float(Core::active.textures[sDiffuse]->width), float(Core::active.textures[sDiffuse]->height), 0.0f, 0.0f));

        Core::setBlendMode(phaseRing < 1.0f ? bmAlpha : bmNone);
        game->getMesh()->renderBuffer(indices, COUNT(indices), vertices, COUNT(vertices));
    }

    void renderBackground() {
        if (!isActive() && titleTimer == 0.0f)
            return;

        Core::setDepthTest(false);

        uint8 alpha;
        if (!isActive() && titleTimer > 0.0f && titleTimer < 1.0f)
            alpha = uint8(titleTimer * 255);
        else
            alpha = 255;

        float sy = 1.0f;

        if (background[0] && background[0]->origWidth / background[0]->origHeight == 2) // PSX images aspect correction
            sy = (480.0f / 640.0f) * ((float)background[0]->origWidth / (float)background[0]->origHeight);

        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR) {
            if (game->getLevel()->isTitle())
                renderTitleBG(1.0f, sy, alpha);
            else
                renderGameBG();
        } else {
            if (background[1])
                renderGameBG();
            else
                renderTitleBG(1.0f, sy, alpha);
        }

        Core::setBlendMode(bmPremult);
        Core::setDepthTest(true);
    }

    void setupCamera(float aspect, bool ui = false) {
        vec3 pos = vec3(0, 0, -1286);

        if (ui) {
            pos.x += UI::width * 0.5f;
            pos.y += UI::height * 0.5f;
            pos.z += 1024.0f;
        }

        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR)
            pos.z -= 256.0f;

        if (Core::settings.detail.stereo == Core::Settings::STEREO_ON)
            pos.x += Core::eye * 8.0f;

        Core::mViewInv = mat4(pos, pos + vec3(0, 0, 1), vec3(0, -1, 0));

        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR) {
            if (head.e00 == INF)
                head = Input::hmd.head.inverseOrtho();
            Core::mViewInv = Core::mViewInv * head * Input::hmd.eye[Core::eye == -1.0f ? 0 : 1];
        } else
            head.e00 = INF;

        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR)
            Core::mProj = Input::hmd.proj[Core::eye == -1.0f ? 0 : 1];
        else
            Core::mProj = GAPI::perspective(70.0f, aspect, 32.0f, 2048.0f);

        Core::mView   = Core::mViewInv.inverseOrtho();
        Core::viewPos = Core::mViewInv.getPos();

        Core::setViewProj(Core::mView, Core::mProj);
    }

    void render(float aspect) {
        if (video) {
            Core::setDepthTest(false);
            video->render();

            Texture *tmp = background[0];

            float sy = 1.0f;
            if ((game->getLevel()->version & TR::VER_TR1) && !playLogo)
                sy = 1.2f;

            Core::resetLights();

            background[0] = video->frameTex[0];
            renderTitleBG(1.0f, sy, 255);

            background[0] = video->frameTex[1];
            renderTitleBG(1.0f, sy, clamp(int((video->stepTimer / video->step) * 255), 0, 255));

            background[0] = tmp;

            Core::setDepthTest(true);
            return;
        }

        if (!isActive() && titleTimer == 0.0f)
            return;

        if (game->getLevel()->isCutsceneLevel() || !isActive())
            return;

    // items
        game->setupBinding();

        setupCamera(aspect);

        UI::setupInventoryShading();

        renderPage(page);
        if (page != targetPage)
            renderPage(targetPage);
    }

    void showLevelStats(const vec2 &pos) {
        char buf[256];
        char time[16];

        int secretsMax = 3;
        int secrets = ((saveStats.secrets & 1) != 0) +
                      ((saveStats.secrets & 2) != 0) +
                      ((saveStats.secrets & 4) != 0);

        int s = saveStats.time % 60;
        int m = saveStats.time / 60 % 60;
        int h = saveStats.time / 3600;

        if (h)
            sprintf(time, "%d:%02d:%02d", h, m, s);
        else
            sprintf(time, "%d:%02d", m, s);

        sprintf(buf, STR[STR_LEVEL_STATS], 
                TR::LEVEL_INFO[saveStats.level].title,
                saveStats.kills,
                saveStats.pickups,
                secrets, secretsMax, time);

        UI::textOut(pos, buf, UI::aCenter, UI::width);
    }

    void renderUI() {
        if (!active || phaseRing < 1.0f) return;

        Core::resetLights();

        static const StringID pageTitle[PAGE_MAX] = { STR_OPTION, STR_INVENTORY, STR_ITEMS, STR_SAVEGAME, STR_LEVEL_STATS };

        float eye = UI::width * Core::eye * 0.01f;

        if (Core::settings.detail.stereo == Core::Settings::STEREO_VR) {
            setupCamera(1.0f, true);
            Core::active.shader->setParam(uViewProj, Core::mViewProj);
            eye = 0.0f;
        }

        if (page == PAGE_SAVEGAME) {
            UI::renderBar(UI::BAR_OPTION, vec2(-eye + UI::width / 2 - 120, 240 - 14), vec2(240, LINE_HEIGHT - 6), 1.0f, 0x802288FF, 0, 0, 0);
            UI::textOut(vec2(-eye, 240), pageTitle[page], UI::aCenter, UI::width);
            UI::renderBar(UI::BAR_OPTION, vec2(-eye - 48 * slot + UI::width / 2, 240 + 24 - 16), vec2(48, 18), 1.0f, 0xFFD8377C, 0);
            UI::textOut(vec2(-eye - 48 + UI::width / 2, 240 + 24), STR_YES, UI::aCenter, 48);
            UI::textOut(vec2(-eye + UI::width / 2, 240 + 24), STR_NO, UI::aCenter, 48);
            return;
        }

        if (page == PAGE_LEVEL_STATS) {
            showLevelStats(vec2(-eye, 180));
            return;
        }

        if (!game->getLevel()->isTitle()) 
            UI::textOut(vec2(-eye, 32), pageTitle[page], UI::aCenter, UI::width);

        if (canFlipPage(-1)) {
            UI::textOut(vec2(16 - eye, 32), "[", UI::aLeft, UI::width);
            UI::textOut(vec2(-eye, 32), "[", UI::aRight, UI::width - 20);
        }

        if (canFlipPage(1)) {
            UI::textOut(vec2(16 - eye, 480 - 16), "]", UI::aLeft, UI::width);
            UI::textOut(vec2(-eye,  480 - 16), "]", UI::aRight, UI::width - 20);
        }

        if (index == targetIndex && page == targetPage)
            renderItemText(eye, items[getGlobalIndex(page, index)]);

    // inventory controls help
        if (page == targetPage && Input::touchTimerVis <= 0.0f) {
            float dx = 32.0f - eye;
            char buf[64];
            sprintf(buf, STR[STR_HELP_SELECT], STR[STR_KEY_FIRST + ikEnter] );
            UI::textOut(vec2(dx, 480 - 64), buf, UI::aLeft, UI::width);
            if (chosen) {
                sprintf(buf, STR[STR_HELP_BACK], STR[STR_KEY_FIRST + Core::settings.controls[playerIndex].keys[ cInventory ].key] );
                UI::textOut(vec2(0, 480 - 64), buf, UI::aRight, UI::width - dx);
            }
        }
    }
};

Inventory *inventory;

#undef SETTINGS
#undef LINE_HEIGHT

#endif

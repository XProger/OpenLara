#ifndef H_INVENTORY
#define H_INVENTORY

#include "format.h"
#include "controller.h"
#include "ui.h"

#define INVENTORY_MAX_ITEMS  32
#define INVENTORY_MAX_RADIUS 688.0f
#ifdef _PSP
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
            float maxWidth = UI::getTextSize(STR[color + value]).x;
            maxWidth = maxWidth * 0.5f + 8.0f;
            x += w * 0.5f;
            if (value > 0)        UI::specOut(vec2(x - maxWidth - 16.0f, y), 108);
            if (value < maxValue) UI::specOut(vec2(x + maxWidth, y), 109);
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

#define SETTINGS(x) intptr_t( &(((Core::Settings*)NULL)->x) )

static const OptionItem optDetail[] = {
    OptionItem( OptionItem::TYPE_TITLE,  STR_SELECT_DETAIL ),
    OptionItem( ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_FILTER,   SETTINGS( detail.filter   ), STR_QUALITY_LOW, 0, 2 ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_LIGHTING, SETTINGS( detail.lighting ), STR_QUALITY_LOW, 0, 2 ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_SHADOWS,  SETTINGS( detail.shadows  ), STR_QUALITY_LOW, 0, 2 ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_WATER,    SETTINGS( detail.water    ), STR_QUALITY_LOW, 0, 2 ),
#if defined(WIN32) || defined(LINUX) || defined(_PSP)
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_VSYNC,    SETTINGS( detail.vsync    ), STR_OFF, 0, 1 ),
#endif
#ifndef _PSP
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_DETAIL_STEREO,   SETTINGS( detail.stereo   ), STR_OFF, 0, 2 ),
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
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_CONTROLS_GAMEPAD    , SETTINGS( controls[0].joyIndex           ), STR_GAMEPAD_1, 0, 3 ),
    OptionItem( OptionItem::TYPE_PARAM,  STR_OPT_CONTROLS_VIBRATION  , SETTINGS( controls[0].vibration          ), STR_OFF,       0, 1 ),
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

static OptionItem optPassport[] = {
    OptionItem( OptionItem::TYPE_TITLE,  STR_SELECT_LEVEL ),
    OptionItem( ),
    OptionItem( OptionItem::TYPE_BUTTON ), // dummy
    OptionItem( ),
    OptionItem( ),
    OptionItem( ),
    OptionItem( ),
    OptionItem( ),
    OptionItem( ),
    OptionItem( ),
    OptionItem( ),
    OptionItem( ),
    OptionItem( ),
    OptionItem( ),
};

struct Inventory {

    enum Page {
        PAGE_OPTION,
        PAGE_INVENTORY,
        PAGE_ITEMS,
        PAGE_MAX
    };

    IGame   *game;
    Texture *background[3];

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

    int slot;
    Core::Settings settings;

    struct Item {
        TR::Entity::Type    type;
        int                 count;
        float               angle;
        Animation           *anim;

        int                 value;

        struct Desc {
            StringID    str;
            Page        page;
            int         model;
            
            Desc() {}
            Desc(StringID str, Page page, int model) : str(str), page(page), model(model) {}
        } desc;

        Item() : anim(NULL) {}

        Item(TR::Level *level, TR::Entity::Type type, int count = 1) : type(type), count(count), angle(0.0f), value(0) {
            switch (type) {
                case TR::Entity::INV_PASSPORT        : desc = Desc( STR_GAME,            PAGE_OPTION,    level->extra.inv.passport        ); break;
                case TR::Entity::INV_PASSPORT_CLOSED : desc = Desc( STR_GAME,            PAGE_OPTION,    level->extra.inv.passport_closed ); break;
                case TR::Entity::INV_MAP             : desc = Desc( STR_MAP,             PAGE_INVENTORY, level->extra.inv.map             ); break;
                case TR::Entity::INV_COMPASS         : desc = Desc( STR_COMPASS,         PAGE_INVENTORY, level->extra.inv.compass         ); break;
                case TR::Entity::INV_STOPWATCH       : desc = Desc( STR_STOPWATCH,       PAGE_INVENTORY, level->extra.inv.stopwatch       ); break;
                case TR::Entity::INV_HOME            : desc = Desc( STR_HOME,            PAGE_OPTION,    level->extra.inv.home            ); break;
                case TR::Entity::INV_DETAIL          : desc = Desc( STR_DETAIL,          PAGE_OPTION,    level->extra.inv.detail          ); break;
                case TR::Entity::INV_SOUND           : desc = Desc( STR_SOUND,           PAGE_OPTION,    level->extra.inv.sound           ); break;
                case TR::Entity::INV_CONTROLS        : desc = Desc( STR_CONTROLS,        PAGE_OPTION,    level->extra.inv.controls        ); break;
                case TR::Entity::INV_GAMMA           : desc = Desc( STR_GAMMA,           PAGE_OPTION,    level->extra.inv.gamma           ); break;
                                                                                   
                case TR::Entity::INV_PISTOLS         : desc = Desc( STR_PISTOLS,         PAGE_INVENTORY, level->extra.inv.weapon[0]       ); break;
                case TR::Entity::INV_SHOTGUN         : desc = Desc( STR_SHOTGUN,         PAGE_INVENTORY, level->extra.inv.weapon[1]       ); break;
                case TR::Entity::INV_MAGNUMS         : desc = Desc( STR_MAGNUMS,         PAGE_INVENTORY, level->extra.inv.weapon[2]       ); break;
                case TR::Entity::INV_UZIS            : desc = Desc( STR_UZIS,            PAGE_INVENTORY, level->extra.inv.weapon[3]       ); break;
                                                                                   
                case TR::Entity::INV_AMMO_PISTOLS    : desc = Desc( STR_AMMO_PISTOLS,    PAGE_INVENTORY, level->extra.inv.ammo[0]         ); break;
                case TR::Entity::INV_AMMO_SHOTGUN    : desc = Desc( STR_AMMO_SHOTGUN,    PAGE_INVENTORY, level->extra.inv.ammo[1]         ); break;
                case TR::Entity::INV_AMMO_MAGNUMS    : desc = Desc( STR_AMMO_MAGNUMS,    PAGE_INVENTORY, level->extra.inv.ammo[2]         ); break;
                case TR::Entity::INV_AMMO_UZIS       : desc = Desc( STR_AMMO_UZIS,       PAGE_INVENTORY, level->extra.inv.ammo[3]         ); break;

                case TR::Entity::INV_MEDIKIT_SMALL   : desc = Desc( STR_MEDI_SMALL,      PAGE_INVENTORY, level->extra.inv.medikit[0]      ); break;
                case TR::Entity::INV_MEDIKIT_BIG     : desc = Desc( STR_MEDI_BIG,        PAGE_INVENTORY, level->extra.inv.medikit[1]      ); break;

                case TR::Entity::INV_PUZZLE_1        : desc = Desc( STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[0]       ); break;
                case TR::Entity::INV_PUZZLE_2        : desc = Desc( STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[1]       ); break;
                case TR::Entity::INV_PUZZLE_3        : desc = Desc( STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[2]       ); break;
                case TR::Entity::INV_PUZZLE_4        : desc = Desc( STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[3]       ); break;
                                                                                                   
                case TR::Entity::INV_KEY_1           : desc = Desc( STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[0]          ); break;
                case TR::Entity::INV_KEY_2           : desc = Desc( STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[1]          ); break;
                case TR::Entity::INV_KEY_3           : desc = Desc( STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[2]          ); break;
                case TR::Entity::INV_KEY_4           : desc = Desc( STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[3]          ); break;
                                                                                                   
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

        const OptionItem* getOptions(int &optCount) const {
            switch (type) {
                case TR::Entity::INV_PASSPORT :
                    if (value != 0) return NULL;
                    optCount = COUNT(optPassport);
                    return optPassport;
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

                        if (i > 2 && i != 2 && i != 7)
                            opt.offset += sizeof(Core::Settings::Controls) * Core::settings.playerIndex;

                        if (i > 7) {
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

            do {
                slot = (slot + dir + optCount) % optCount;
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
                    if (opt->type == OptionItem::TYPE_PARAM && value > 0) {
                        value--;
                        timer = 0.2f;
                        return opt;
                    }    
                    break;
                case cRight  :
                    if (opt->type == OptionItem::TYPE_PARAM && value < opt->maxValue) {
                        value++;
                        timer = 0.2f;
                        return opt;
                    }    
                    break;
                default      : ;
            }

            return NULL;
        }

        void update() {
            if (!anim) return;
            anim->update();

            if (type == TR::Entity::INV_PASSPORT) {
                float t = (14 + value * 5) / 30.0f;
                
                if ( (anim->dir > 0.0f && anim->time > t) ||
                     (anim->dir < 0.0f && anim->time < t)) {
                    anim->dir = 0.0f;
                    anim->time = t;
                    anim->updateInfo();
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

            Core::setBasis(joints, m.mCount);

            Core::setBlending(bmNone);
            mesh->transparent = 0;
            mesh->renderModel(desc.model);
            
            Core::setBlending(bmAlpha);
            mesh->transparent = 1;
            mesh->renderModel(desc.model);

            Core::setBlending(bmAdd);
            Core::setDepthWrite(false);
            mesh->transparent = 2;
            mesh->renderModel(desc.model);
            Core::setDepthWrite(true);

            Core::setBlending(bmNone);
        }

        void choose() {
            if (!anim) return;            
            anim->setAnim(0, 0, false);
        }

    } *items[INVENTORY_MAX_ITEMS];

    static void loadTitleBG(Stream *stream, void *userData) {
        Inventory *inv = (Inventory*)userData;
        if (!stream) {
            inv->titleTimer = 0.0f;
            return;
        }
        inv->titleTimer = 3.0f;

        inv->background[0] = Texture::Load(*stream);
        delete stream;
    }

    Inventory(IGame *game) : game(game), active(false), chosen(false), index(0), targetIndex(0), page(PAGE_OPTION), targetPage(PAGE_OPTION), itemsCount(0), playerIndex(0), changeTimer(0.0f), nextLevel(TR::LVL_MAX), lastKey(cMAX) {
        TR::LevelID id = game->getLevel()->id;

        add(TR::Entity::INV_PASSPORT);
        add(TR::Entity::INV_DETAIL);
        add(TR::Entity::INV_SOUND);
        add(TR::Entity::INV_CONTROLS);

        if (!game->getLevel()->isTitle() && id != TR::LVL_TR1_GYM && id != TR::LVL_TR2_ASSAULT) {
/*
            if (level->extra.inv.map != -1)
                add(TR::Entity::INV_MAP);
            if (level->extra.inv.gamma != -1)
                add(TR::Entity::INV_GAMMA);
*/
            add(TR::Entity::INV_PISTOLS, UNLIMITED_AMMO);
            add(TR::Entity::INV_SHOTGUN, 10);
            add(TR::Entity::INV_MAGNUMS, 10);
            add(TR::Entity::INV_UZIS, 50);
//              add(TR::Entity::INV_MEDIKIT_SMALL, 999);
//              add(TR::Entity::INV_MEDIKIT_BIG, 999);
//              add(TR::Entity::INV_SCION, 1);
        #ifdef _DEBUG
            add(TR::Entity::INV_KEY_1, 3);
            add(TR::Entity::INV_KEY_2, 3);
            add(TR::Entity::INV_KEY_3, 3);
            add(TR::Entity::INV_KEY_4, 3);

            add(TR::Entity::INV_PUZZLE_1, 3);
            add(TR::Entity::INV_PUZZLE_2, 3);
            add(TR::Entity::INV_PUZZLE_3, 3);
            add(TR::Entity::INV_PUZZLE_4, 3);

            add(TR::Entity::INV_LEADBAR, 3);
        #endif
        } 

        TR::Level *level = game->getLevel();

        memset(background, 0, sizeof(background));

        const char *titleBG = TR::getGameScreen(level->version, level->id);
        if (titleBG) {
            titleTimer = TITLE_LOADING;
            new Stream(titleBG, loadTitleBG, this);
        } else
            titleTimer = 0.0f;

        if (level->isTitle()) {
            add(TR::Entity::INV_HOME);
        } else {
            add(TR::Entity::INV_COMPASS);
            add(TR::Entity::INV_STOPWATCH);
        }

        phaseRing = phasePage = phaseChoose = phaseSelect = 0.0f;
        memset(pageItemIndex, 0, sizeof(pageItemIndex));

        waitForKey = NULL;
    }

    ~Inventory() {
        for (int i = 0; i < itemsCount; i++)
            delete items[i];

        for (int i = 0; i < COUNT(background); i++)
            delete background[i];
    }

    bool isActive() {
        return active || phaseRing > 0.0f;
    }

    int contains(TR::Entity::Type type) {
        type = TR::Entity::convToInv(type);
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
        type = TR::Entity::convToInv(type);

        if (smart) {
            switch (type) {
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
            return;
        }

        ASSERT(itemsCount < INVENTORY_MAX_ITEMS);

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
        if (type == TR::Entity::LARA)
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

    bool toggle(int playerIndex = 0, Page curPage = PAGE_INVENTORY, TR::Entity::Type type = TR::Entity::LARA) {
        if (!game->getLara(playerIndex))
            return false;

        this->playerIndex = playerIndex;
        titleTimer = 0.0f;

        if (phaseRing == 0.0f || phaseRing == 1.0f) {
            active = !active;
            vec3 p;
            game->playSound(active ? TR::SND_INV_SHOW : TR::SND_INV_HIDE, p);
            chosen = false;

            if (active) {
                for (int i = 0; i < itemsCount; i++)
                    items[i]->reset();

                nextLevel   = TR::LVL_MAX;
                phasePage   = 1.0f;
                phaseSelect = 1.0f;
                page        = targetPage  = curPage;

                if (type != TR::Entity::LARA) {
                    int i = contains(type);
                    if (i >= 0)
                        pageItemIndex[page] = getLocalIndex(i);
                }

                index = targetIndex = pageItemIndex[page];

                //if (type == TR::Entity::INV_PASSPORT) // toggle after death
                //    chooseItem();
            }
        }
        return active;
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

                int passportSlotCount = 0;
                TR::LevelID passportSlots[32];

                switch (level->version & TR::VER_VERSION) {
                    case TR::VER_TR1 : 
                        passportSlotCount = 2;
                        passportSlots[0]  = TR::LVL_TR1_1;
                        passportSlots[1]  = TR::LVL_TR1_2;
                        break;
                    case TR::VER_TR2 :
                        passportSlotCount = 2;
                        passportSlots[0]  = TR::LVL_TR2_WALL;
                        passportSlots[1]  = TR::LVL_TR2_BOAT;
                        break;
                    case TR::VER_TR3 :
                        passportSlotCount = 1;
                        passportSlots[0]  = TR::LVL_TR3_JUNGLE;
                        break;
                    default : ASSERT(false);
                }

                for (int i = 0; i < passportSlotCount; i++) {
                    optPassport[2 + i].type   = OptionItem::TYPE_BUTTON;
                    optPassport[2 + i].offset = intptr_t(TR::LEVEL_INFO[passportSlots[i]].title); // offset as int pointer to level title string
                    optPassport[2 + i].color  = passportSlots[i]; // color as level ID
                }

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
                slot = -1;
                item->nextSlot(slot, 1);
            }

            if (key == cAction && phaseChoose == 1.0f && item->value != 0) {
                TR::LevelID id = level->id;
                switch (item->value) {
                    case 1 : nextLevel = level->isTitle() ? level->getStartId() : id; break;
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
            nextLevel = (TR::LevelID)opt->color;
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

    void update() {
        if (titleTimer != TITLE_LOADING && titleTimer > 0.0f) {
            titleTimer -= Core::deltaTime;
            if (titleTimer < 0.0f)
                titleTimer = 0.0f;
        }

        if (!isActive())
            return;

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

        Input::Joystick &joy = Input::joy[Core::settings.controls[playerIndex].joyIndex];

        ControlKey key = cMAX;
        if (Input::down[ikCtrl] || Input::down[ikEnter] || joy.down[jkA])
            key = cAction;
        else if (Input::down[ikAlt]   || joy.down[jkB]     || Input::state[playerIndex][cInventory])
            key = cInventory;
        else if (Input::down[ikLeft]  || joy.down[jkLeft]  || joy.L.x < -0.5f)
            key = cLeft;
        else if (Input::down[ikRight] || joy.down[jkRight] || joy.L.x >  0.5f)
            key = cRight;
        else if (Input::down[ikUp]    || joy.down[jkUp]    || joy.L.y < -0.5f)
            key = cUp;
        else if (Input::down[ikDown]  || joy.down[jkDown]  || joy.L.y >  0.5f)
            key = cDown;

        Item *item = items[getGlobalIndex(page, index)];

        if (index == targetIndex && targetPage == page && ready) {
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
                        item->value     = 1000;
                        item->angle     = 0.0f;
                    }
                } else
                    toggle();
            }
        }
        lastKey = key;

        ready = active && phaseRing == 1.0f && phasePage == 1.0f;

        float w = 90.0f * DEG2RAD * Core::deltaTime;

        int itemIndex = index == targetIndex ? getGlobalIndex(page, index) : -1;

        for (int i = 0; i < itemsCount; i++) {
            items[i]->update();
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
        if (game->getLevel()->isTitle() || ((Character*)game->getLara(playerIndex))->health <= 0.0f)
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
                background[i] = new Texture(INVENTORY_BG_SIZE, INVENTORY_BG_SIZE, Texture::RGBA, false);

        return background[0];
    }

    void prepareBackground() {
        Core::defaultTarget = getBackgroundTarget();
        game->renderGame(false);
        Core::defaultTarget = NULL;

        Core::setDepthTest(false);
        Core::setBlending(bmNone);

    #ifdef FFP
        mat4 m;
        m.identity();
        Core::setViewProj(m, m);
        Core::mModel.identity();
    #endif

    #ifdef _PSP
        //
    #else
        // vertical blur
        Core::setTarget(background[1], true);
        game->setShader(Core::passFilter, Shader::FILTER_BLUR, false, false);
        Core::active.shader->setParam(uParam, vec4(0, 1, 1.0f / INVENTORY_BG_SIZE, 0));;
        background[0]->bind(sDiffuse);
        game->getMesh()->renderQuad();

        // horizontal blur
        Core::setTarget(background[2], true);
        game->setShader(Core::passFilter, Shader::FILTER_BLUR, false, false);
        Core::active.shader->setParam(uParam, vec4(1, 0, 1.0f / INVENTORY_BG_SIZE, 0));;
        background[1]->bind(sDiffuse);
        game->getMesh()->renderQuad();

        // grayscale
        Core::setTarget(background[1], true);
        game->setShader(Core::passFilter, Shader::FILTER_GRAYSCALE, false, false);
        Core::active.shader->setParam(uParam, vec4(1, 0, 0, 0));
        background[2]->bind(sDiffuse);
        game->getMesh()->renderQuad();
    #endif

        Core::setTarget(NULL, true);

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
        float tw = UI::getTextSize(STR[str]).x;

        if (item->value > 0) UI::specOut(vec2((UI::width - tw) * 0.5f - 32.0f, 480 - 32), 108);
        if (item->value < 2) UI::specOut(vec2((UI::width + tw) * 0.5f + 16.0f, 480 - 32), 109);

        if (item->value != 0) return;

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

    void renderItemText(Item *item) {
        float eye = UI::width * Core::eye * 0.01f;

        if (item->type == TR::Entity::INV_PASSPORT && phaseChoose == 1.0f) {
            //
        } else
            UI::textOut(vec2(-eye, 480 - 32), item->desc.str, UI::aCenter, UI::width);

        renderItemCount(item, vec2(UI::width / 2 - 160 - eye, 480 - 96), 320);

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
                case TR::Entity::INV_COMPASS   :
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
        Core::active.shader->setParam(uMaterial, vec4(1.0f, 0.4f, 0.0f, alpha));

        int count = getItemsCount(page);

        vec2 cpos(1286, 256 + 1280 * (1.0f - phaseRing));
        float ringTilt      = cpos.angle();
        float radius        = phaseRing * INVENTORY_MAX_RADIUS * phase;
        float collapseAngle = phaseRing * phase * PI - PI;
        float ringHeight    = lerp(float(this->page), float(targetPage), hermite(phasePage)) * INVENTORY_HEIGHT;
        float angle         = getAngle(pageItemIndex[page], count);

        if (phaseSelect < 1.0f)
            angle = lerpAngle(angle, getAngle(targetIndex, count), hermite(phaseSelect));
        
        int itemIndex = 0;
        for (int i = 0; i < itemsCount; i++) {
            Item *item = items[i];

            if (item->desc.page != page)
                continue;

            float a  = getAngle(itemIndex, count) - angle - collapseAngle;
            float ia = item->angle;
            float ra = ringTilt;
            float rd = radius;
            float rh = ringHeight;

            if (itemIndex == pageItemIndex[page] && (chosen || phaseChoose > 0.0f)) {
                ia *= 1.0f - phaseChoose;
                rh -=  128 * phaseChoose;
                rd +=  296 * phaseChoose;
            }

            Basis basis = Basis(quat(vec3(1, 0, 0), ra), vec3(0.0f));
            basis = basis * Basis(quat(vec3(0, 1, 0), PI + ia - a), vec3(sinf(a), 0, -cosf(a)) * rd - vec3(0, item->desc.page * INVENTORY_HEIGHT - rh, 0));

            item->render(game, basis);

            itemIndex++;
        }
    }

    void renderBackground() {
        if (!isActive() && titleTimer == 0.0f)
            return;

        Core::setDepthTest(false);

        if (background[0]) {
            #ifdef FFP
                mat4 m;
                m.identity();
                Core::setViewProj(m, m);
                Core::mModel.identity();
            //    Core::mModel.scale(vec3(0.9f));
            #endif

            background[0]->bind(sDiffuse);  // orignal image
            if (background[1]) {
                game->setShader(Core::passFilter, Shader::FILTER_MIXER, false, false);
                Core::active.shader->setParam(uParam, vec4(phaseRing, 1.0f - phaseRing * 0.4f, 0, 0));;
                background[1]->bind(sNormal);   // blured grayscale image
                Core::setBlending(bmNone);
            } else {
                game->setShader(Core::passFilter, Shader::DEFAULT, false, false);
                
                float aspectSrc = float(background[0]->origWidth) / float(background[0]->origHeight);
                float aspectDst = float(Core::width) / float(Core::height);
                float aspectImg = aspectDst / aspectSrc;
                float ax = background[0]->origWidth  / float(background[0]->width);
                float ay = background[0]->origHeight / float(background[0]->height);
                Core::active.shader->setParam(uParam, vec4(ax * aspectImg, -ay, (0.5f - aspectImg * 0.5f) * ax, ay));

                if (!isActive() && titleTimer > 0.0f && titleTimer < 1.0f) {
                    Core::setBlending(bmAlpha);
                    Core::active.shader->setParam(uMaterial, vec4(1, 1, 1, titleTimer));
                } else {
                    Core::setBlending(bmNone);
                    Core::active.shader->setParam(uMaterial, vec4(1));
                }
            }
            game->getMesh()->renderQuad();
        }

        Core::setDepthTest(true);
        Core::setBlending(bmAlpha);
    }

    void render(float aspect) {
        if (!isActive() && titleTimer == 0.0f)
            return;

        if (game->getLevel()->isCutsceneLevel() || !isActive())
            return;

    // items
        game->setupBinding();

        Core::mLightProj.identity();

        Core::mView.identity();
        Core::mView.translate(vec3(-Core::eye * 8.0f, 0, -1286));   // y = -96 in title 

        Core::mView.up()  *= -1.0f;
        Core::mView.dir() *= -1.0f;
        Core::mViewInv = Core::mView.inverse();

        Core::mProj     = mat4(70.0f, aspect, 32.0f, 2048.0f);
        Core::setViewProj(Core::mView, Core::mProj);

        Core::viewPos   = Core::mViewInv.getPos();

        Core::whiteTex->bind(sShadow);
        game->setShader(Core::passCompose, Shader::ENTITY, false, false);

        vec3 ambient[6] = {
            vec3(0.4f), vec3(0.2f), vec3(0.4f), vec3(0.5f), vec3(0.4f), vec3(0.6f)
        };

        for (int i = 0; i < MAX_LIGHTS; i++) {
            Core::lightPos[i]   = vec3(0, 0, 0);
            Core::lightColor[i] = vec4(0, 0, 0, 1);
        }
        
        Core::active.shader->setParam(uLightPos,   Core::lightPos[0],   MAX_LIGHTS);
        Core::active.shader->setParam(uLightColor, Core::lightColor[0], MAX_LIGHTS);
        Core::active.shader->setParam(uAmbient,    ambient[0], 6);

        renderPage(page);
        if (page != targetPage)
            renderPage(targetPage);
    }

    void renderUI() {
        if (!active || phaseRing < 1.0f) return;

        static const StringID pageTitle[PAGE_MAX] = { STR_OPTION, STR_INVENTORY, STR_ITEMS };

        float eye = UI::width * Core::eye * 0.01f;

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

        if (index == targetIndex)
            renderItemText(items[getGlobalIndex(page, index)]);

    // inventory controls help
        float dx = 32.0f - eye;
        char buf[64];
        sprintf(buf, STR[STR_HELP_SELECT], STR[STR_KEY_FIRST + ikEnter] );
        UI::textOut(vec2(dx, 480 - 64), buf, UI::aLeft, UI::width);
        if (chosen) {
            sprintf(buf, STR[STR_HELP_BACK], STR[STR_KEY_FIRST + Core::settings.controls[playerIndex].keys[ cInventory ].key] );
            UI::textOut(vec2(0, 480 - 64), buf, UI::aRight, UI::width - dx);
        }
    }
};

#undef SETTINGS
#undef LINE_HEIGHT

#endif
#ifndef H_INVENTORY
#define H_INVENTORY

#include "format.h"
#include "controller.h"
#include "ui.h"

#define INVENTORY_MAX_ITEMS  32
#define INVENTORY_MAX_RADIUS 688.0f
#define INVENTORY_BG_SIZE    512
#define INVENTORY_HEIGHT     2048.0f

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

    float       changeTimer;
    TR::LevelID nextLevel; // toggle result
    ControlKey  lastKey;

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
        } desc;

        Item() : anim(NULL) {}

        Item(TR::Level *level, TR::Entity::Type type, int count = 1) : type(type), count(count), angle(0.0f), value(0) {
            switch (type) {
                case TR::Entity::INV_PASSPORT        : desc = { STR_GAME,            PAGE_OPTION,    level->extra.inv.passport        }; break;
                case TR::Entity::INV_PASSPORT_CLOSED : desc = { STR_GAME,            PAGE_OPTION,    level->extra.inv.passport_closed }; break;
                case TR::Entity::INV_MAP             : desc = { STR_MAP,             PAGE_INVENTORY, level->extra.inv.map             }; break;
                case TR::Entity::INV_COMPASS         : desc = { STR_COMPASS,         PAGE_INVENTORY, level->extra.inv.compass         }; break;
                case TR::Entity::INV_STOPWATCH       : desc = { STR_STOPWATCH,       PAGE_INVENTORY, level->extra.inv.stopwatch       }; break;
                case TR::Entity::INV_HOME            : desc = { STR_HOME,            PAGE_OPTION,    level->extra.inv.home            }; break;
                case TR::Entity::INV_DETAIL          : desc = { STR_DETAIL,          PAGE_OPTION,    level->extra.inv.detail          }; break;
                case TR::Entity::INV_SOUND           : desc = { STR_SOUND,           PAGE_OPTION,    level->extra.inv.sound           }; break;
                case TR::Entity::INV_CONTROLS        : desc = { STR_CONTROLS,        PAGE_OPTION,    level->extra.inv.controls        }; break;
                case TR::Entity::INV_GAMMA           : desc = { STR_GAMMA,           PAGE_OPTION,    level->extra.inv.gamma           }; break;
                                                                                   
                case TR::Entity::INV_PISTOLS         : desc = { STR_PISTOLS,         PAGE_INVENTORY, level->extra.inv.weapon[0]       }; break;
                case TR::Entity::INV_SHOTGUN         : desc = { STR_SHOTGUN,         PAGE_INVENTORY, level->extra.inv.weapon[1]       }; break;
                case TR::Entity::INV_MAGNUMS         : desc = { STR_MAGNUMS,         PAGE_INVENTORY, level->extra.inv.weapon[2]       }; break;
                case TR::Entity::INV_UZIS            : desc = { STR_UZIS,            PAGE_INVENTORY, level->extra.inv.weapon[3]       }; break;
                                                                                   
                case TR::Entity::INV_AMMO_PISTOLS    : desc = { STR_AMMO_PISTOLS,    PAGE_INVENTORY, level->extra.inv.ammo[0]         }; break;
                case TR::Entity::INV_AMMO_SHOTGUN    : desc = { STR_AMMO_SHOTGUN,    PAGE_INVENTORY, level->extra.inv.ammo[1]         }; break;
                case TR::Entity::INV_AMMO_MAGNUMS    : desc = { STR_AMMO_MAGNUMS,    PAGE_INVENTORY, level->extra.inv.ammo[2]         }; break;
                case TR::Entity::INV_AMMO_UZIS       : desc = { STR_AMMO_UZIS,       PAGE_INVENTORY, level->extra.inv.ammo[3]         }; break;

                case TR::Entity::INV_MEDIKIT_SMALL   : desc = { STR_MEDI_SMALL,      PAGE_INVENTORY, level->extra.inv.medikit[0]      }; break;
                case TR::Entity::INV_MEDIKIT_BIG     : desc = { STR_MEDI_BIG,        PAGE_INVENTORY, level->extra.inv.medikit[1]      }; break;

                case TR::Entity::INV_PUZZLE_1        : desc = { STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[0]       }; break;
                case TR::Entity::INV_PUZZLE_2        : desc = { STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[1]       }; break;
                case TR::Entity::INV_PUZZLE_3        : desc = { STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[2]       }; break;
                case TR::Entity::INV_PUZZLE_4        : desc = { STR_PUZZLE,          PAGE_ITEMS,     level->extra.inv.puzzle[3]       }; break;
                                                                                                   
                case TR::Entity::INV_KEY_1           : desc = { STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[0]          }; break;
                case TR::Entity::INV_KEY_2           : desc = { STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[1]          }; break;
                case TR::Entity::INV_KEY_3           : desc = { STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[2]          }; break;
                case TR::Entity::INV_KEY_4           : desc = { STR_KEY,             PAGE_ITEMS,     level->extra.inv.key[3]          }; break;
                                                                                                   
                case TR::Entity::INV_LEADBAR         : desc = { STR_LEAD_BAR,        PAGE_ITEMS,     level->extra.inv.leadbar         }; break;
                case TR::Entity::INV_SCION           : desc = { STR_SCION,           PAGE_ITEMS,     level->extra.inv.scion           }; break;
                default                              : desc = { STR_UNKNOWN,         PAGE_ITEMS,     -1                               }; break;
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

            anim->getJoints(basis, -1, true, joints);

            Core::active.shader->setParam(uBasis, joints[0], m.mCount);

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
        if (!stream) return;
        Inventory *inv = (Inventory*)userData;

        inv->background[0] = Texture::Load(*stream);
        delete stream;
    }

    Inventory(IGame *game) : game(game), active(false), chosen(false), index(0), targetIndex(0), page(PAGE_OPTION), targetPage(PAGE_OPTION), itemsCount(0), changeTimer(0.0f), nextLevel(TR::LVL_MAX), lastKey(cMAX) {
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

        if (level->isTitle()) {
            add(TR::Entity::INV_HOME);

            memset(background, 0, sizeof(background));

            const char *titleBG = TR::getGameScreen(level->version, level->id);
            if (titleBG)
                new Stream(titleBG, loadTitleBG, this);

        } else {
            add(TR::Entity::INV_COMPASS);
            add(TR::Entity::INV_STOPWATCH);

            for (int i = 0; i < COUNT(background); i++)
                background[i] = new Texture(INVENTORY_BG_SIZE, INVENTORY_BG_SIZE, Texture::RGBA, false);
        }

        phaseRing = phasePage = phaseChoose = phaseSelect = 0.0f;
        memset(pageItemIndex, 0, sizeof(pageItemIndex));
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
    
    bool chooseKey(TR::Entity::Type hole) {
        TR::Entity::Type type = TR::Entity::getItemForHole(hole);
        if (type == TR::Entity::LARA)
            return false;
        int index = contains(type);
        if (index < 0)
            return false;
        toggle(items[index]->desc.page, type);
        return true;
    }

    bool use(TR::Entity::Type type) {
        if (contains(type) > -1) {
            remove(type);
            return true;
        }
        return false;
    }

    bool toggle(Page curPage = PAGE_INVENTORY, TR::Entity::Type type = TR::Entity::LARA) {
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

        slot = 0;

        switch (item->type) {
            case TR::Entity::INV_PASSPORT : {
                game->playSound(TR::SND_INV_PAGE);
                item->value = 1;

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

                break;
            }
            case TR::Entity::INV_DETAIL : {
                settings = Core::settings;
                break;
            }
            default : ;
        }
    }

    void controlItem(Item *item, ControlKey key) {
        TR::Level *level = game->getLevel();

        if (item->type == TR::Entity::INV_PASSPORT && passportSlotCount) {
        // passport slots
            if (item->value == 0 && item->anim->dir == 0.0f) { // slot select
                if (key == cUp   ) { slot = (slot - 1 + passportSlotCount) % passportSlotCount; };
                if (key == cDown ) { slot = (slot + 1) % passportSlotCount; };
            }
        // passport pages
            if (key == cLeft  && item->value > 0) { item->value--; item->anim->dir = -1.0f; game->playSound(TR::SND_INV_PAGE); }
            if (key == cRight && item->value < 2) { item->value++; item->anim->dir =  1.0f; game->playSound(TR::SND_INV_PAGE); }

            if (key == cAction && phaseChoose == 1.0f) {
                TR::LevelID id = level->id;
                switch (item->value) {
                    case 0 : nextLevel = passportSlots[slot]; break;
                    case 1 : nextLevel = level->isTitle() ? level->getStartId() : id; break;
                    case 2 : nextLevel = level->isTitle() ? TR::LVL_MAX : level->getTitleId(); break;
                }

                if (nextLevel != TR::LVL_MAX) {
                    item->anim->dir = -1.0f;
                    item->value = -100;
                    toggle();
                }
            }
        }

        if (item->type == TR::Entity::INV_DETAIL) {
            int count = 6;
            if (key == cUp   ) { slot = (slot - 1 + count) % count; };
            if (key == cDown ) { slot = (slot + 1) % count;         };
            if (slot < count - 2) {
                Core::Settings::Quality q = settings.detail.quality[slot];
                if (key == cLeft  && q > Core::Settings::LOW  ) { q = Core::Settings::Quality(q - 1); }
                if (key == cRight && q < Core::Settings::HIGH ) { q = Core::Settings::Quality(q + 1); }
                if (q != settings.detail.quality[slot]) {
                    switch (slot) {
                        case 0 : settings.detail.setFilter(q);   break;
                        case 1 : settings.detail.setLighting(q); break;
                        case 2 : settings.detail.setShadows(q);  break;
                        case 3 : settings.detail.setWater(q);    break;
                    }
                    if (q == settings.detail.quality[slot])
                        game->playSound(TR::SND_INV_PAGE);
                }
            }

            if (slot == count - 2 && (key == cLeft || key == cRight)) // stereo
                settings.detail.stereo = !settings.detail.stereo;

            if (slot == count - 1 && key == cAction) { // apply
                game->applySettings(settings);
                chosen = false;
            }
        }

        if (item->type == TR::Entity::INV_SOUND) {
            int count = 3;
            if (key == cUp   ) { slot = (slot - 1 + count) % count; };
            if (key == cDown ) { slot = (slot + 1) % count;         };

            if (slot == 0 || slot == 1) { // music
                float &v = slot == 0 ? Core::settings.audio.music : Core::settings.audio.sound;
                if ((key == cLeft  && v > 0.0f) || (key == cRight && v < 1.0f)) {
                    v = key == cLeft ? max(0.0f, v - 0.05f) : min(1.0f, v + 0.05f);
                    changeTimer = 0.2f;
                    if (slot == 1)
                        game->playSound(TR::SND_PISTOLS_SHOT);
                    game->applySettings(Core::settings);
                }
            }

            if (slot == 2 && (key == cLeft || key == cRight)) {
                Core::settings.audio.reverb = !Core::settings.audio.reverb;
                game->applySettings(Core::settings);
            }
        }

        if (item->type == TR::Entity::INV_HOME && phaseChoose == 1.0f && key == cAction) {
            nextLevel = level->getHomeId();
            toggle();
        }

        if ((key == cInventory || key == cJump) && phaseChoose == 1.0f) {
            chosen = false;
            item->anim->dir = 1.0f;
            item->value     = 1000;
            item->angle     = 0.0f;
        }
    }

    void update() {
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

        ControlKey key = cMAX;
        if (Input::state[cAction])
            key = cAction;
        else if (Input::state[cInventory] || Input::state[cJump])
            key = cInventory;
        else if (Input::state[cLeft]  || Input::joy.L.x < -0.5f || Input::joy.R.x >  0.5f)
            key = cLeft;
        else if (Input::state[cRight] || Input::joy.L.x >  0.5f || Input::joy.R.x < -0.5f)
            key = cRight;
        else if (Input::state[cUp]   || Input::joy.L.y < -0.5f || Input::joy.R.y >  0.5f)
            key = cUp;
        else if (Input::state[cDown] || Input::joy.L.y >  0.5f || Input::joy.R.y < -0.5f)
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

                if (key != cMAX && lastKey != key && changeTimer == 0.0f)
                    controlItem(item, key);
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
                    game->invUse(type);
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
        if (game->getLevel()->isTitle() || ((Character*)game->getLara())->health <= 0.0f)
            return false;
        if (dir == -1) return page < PAGE_ITEMS  && getItemsCount(page + 1);
        if (dir ==  1) return page > PAGE_OPTION && getItemsCount(page - 1);
        return false;
    }

    void prepareBackground() {
        Core::setDepthTest(false);
        Core::setBlending(bmNone);

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
        Core::active.shader->setParam(uParam, vec4(1, 0, 0, 0));;
        background[2]->bind(sDiffuse);
        game->getMesh()->renderQuad();

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
            UI::textOut(pos, buf, UI::aRight, width, UI::SHADE_NONE);
        }
    }

    int slot, passportSlotCount;
    TR::LevelID passportSlots[32];
    Core::Settings settings;

    void renderPassport(Item *item) {
        if (item->anim->dir != 0.0f) return; // check for "Load Game" page

        float eye = UI::width * Core::eye * 0.02f;
        float h = 20.0f;
        float w = 320.0f;

        float x = (UI::width - w) * 0.5f - eye;
        float y = 120.0f;

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

    // background
        UI::renderBar(UI::BAR_OPTION, vec2(x - 8.0f, y - 16.0f), vec2(w + 16.0f, h * 16.0f), 0.0f, 0, 0xC0000000);
    // title
        UI::renderBar(UI::BAR_OPTION, vec2(x, y - h + 6), vec2(w, h - 6), 1.0f, 0x802288FF, 0, 0, 0);
        UI::textOut(vec2(x, y), STR_SELECT_LEVEL, UI::aCenter, w, UI::SHADE_GRAY);

        y += h * 2;
        UI::renderBar(UI::BAR_OPTION, vec2(x, y + slot * h + 6 - h), vec2(w, h - 6), 1.0f, 0xFFD8377C, 0);

        for (int i = 0; i < passportSlotCount; i++)
            if (passportSlots[i] == TR::LVL_MAX)
                UI::textOut(vec2(x, y + i * h), STR_AUTOSAVE, UI::aCenter, w);
            else
                UI::textOut(vec2(x, y + i * h), TR::LEVEL_INFO[passportSlots[i]].title, UI::aCenter, w);
    }

    float printBool(float x, float y, float w, StringID oStr, bool active, bool value) {
        StringID vStr = StringID(STR_OFF + int(value));

        UI::textOut(vec2(x, y), oStr);
        UI::textOut(vec2(x + w - 96.0f, y), vStr, UI::aCenter, 96.0f);
        if (active) {
            UI::specOut(vec2(x + w - 96.0f, y), 108);
            UI::specOut(vec2(x + w - 12.0f, y), 109);
        }
        return y + 20.0f;
    }

    float printQuality(float x, float y, float w, StringID oStr, bool active, Core::Settings::Quality value) {
        StringID vStr = StringID(STR_QUALITY_LOW + int(value));

        float d = x + w * 0.5f;
        UI::textOut(vec2(x + 32.0f, y), oStr);
        UI::textOut(vec2(d, y), vStr, UI::aCenter, w * 0.5f - 32.0f);
        if (active) {
            // d += 8.0f; TODO TR3
            if (value > Core::Settings::LOW)  UI::specOut(vec2(d, y), 108);
            if (value < Core::Settings::HIGH) UI::specOut(vec2(d + w * 0.5f - 32.0f - 16.0f, y), 109);
        }
        return y + 20.0f;
    }

    float printBar(float x, float y, float w, uint32 color, char icon, bool active, float value) {
        float h = 20.0f;
        UI::renderBar(UI::BAR_WHITE, vec2(x + (32.0f + 2.0f), y - h + 6 + 2), vec2(w - (64.0f + 4.0f), h - 6 - 4), value, color, 0xFF000000, 0xFFA0A0A0, 0xFFA0A0A0, 0xFF000000);
        UI::specOut(vec2(x + 16.0f, y), icon);
        if (active) {
            if (value > 0.0f) UI::specOut(vec2(x, y), 108);
            if (value < 1.0f) UI::specOut(vec2(x + w - 12.0f, y), 109);
        }
        return y + 20.0f;
    }

    void renderDetail(Item *item) {
        float w = 320.0f;
        float h = 20.0f;

        float eye = UI::width * Core::eye * 0.02f;
        float x = (UI::width - w) * 0.5f - eye;
        float y = 192.0f - h;
        
    // background
        UI::renderBar(UI::BAR_OPTION, vec2(x, y - 16.0f), vec2(w, h * 9.0f + 8.0f), 0.0f, 0, 0xC0000000);
    // title
        UI::renderBar(UI::BAR_OPTION, vec2(x, y - h + 6), vec2(w, h - 6), 1.0f, 0x802288FF, 0, 0, 0);
        UI::textOut(vec2(x, y), STR_SELECT_DETAIL, UI::aCenter, w, UI::SHADE_GRAY);

        y += h * 2;
        x += 8.0f;
        w -= 16.0f;
        float aw = slot == 5 ? (w - 128.0f) : w;
        
        UI::renderBar(UI::BAR_OPTION, vec2((UI::width - aw) * 0.5f - eye, y + (slot > 4 ? 6 : slot) * h + 6 - h), vec2(aw, h - 6), 1.0f, 0xFFD8377C, 0);
        y = printQuality(x, y, w, STR_OPT_DETAIL_FILTER,   slot == 0, settings.detail.filter);
        y = printQuality(x, y, w, STR_OPT_DETAIL_LIGHTING, slot == 1, settings.detail.lighting);
        y = printQuality(x, y, w, STR_OPT_DETAIL_SHADOWS,  slot == 2, settings.detail.shadows);
        y = printQuality(x, y, w, STR_OPT_DETAIL_WATER,    slot == 3, settings.detail.water);
        y = printBool(x + 32.0f, y, w - 64.0f - 16.0f, STR_OPT_DETAIL_STEREO,  slot == 4, settings.detail.stereo);
        y += h;
        UI::textOut(vec2(x + 64.0f, y), STR_APPLY, UI::aCenter, w - 128.0f);
    }

    void renderSound(Item *item) {
        float w = 320.0f;
        float h = 20.0f;

        float eye = UI::width * Core::eye * 0.02f;
        float x = (UI::width - w) * 0.5f - eye;
        float y = 192.0f;
        
    // background
        UI::renderBar(UI::BAR_OPTION, vec2(x, y - 16.0f), vec2(w, h * 5.0f + 8.0f), 0.0f, 0, 0xC0000000);
    // title
        UI::renderBar(UI::BAR_OPTION, vec2(x, y - h + 6), vec2(w, h - 6), 1.0f, 0x802288FF, 0, 0, 0);
        UI::textOut(vec2(x, y), STR_SET_VOLUMES, UI::aCenter, w, UI::SHADE_GRAY);

        y += h * 2;
        x += 8.0f;
        w -= 16.0f;

        UI::renderBar(UI::BAR_OPTION, vec2((UI::width - w) * 0.5f - eye, y + slot * h + 6 - h), vec2(w, h - 6), 1.0f, 0xFFD8377C, 0);

        float aw = w - 64.0f;
        aw -= 4.0f;
 
        y = printBar((UI::width - w) * 0.5f - eye, y, w, 0xFF0080FF, 101, slot == 0, Core::settings.audio.music);
        y = printBar((UI::width - w) * 0.5f - eye, y, w, 0xFFFF8000, 102, slot == 1, Core::settings.audio.sound);
        y = printBool(x + 32.0f, y, w - 64.0f, STR_REVERBERATION, slot == 2, Core::settings.audio.reverb);
    }

    void renderItemText(Item *item) {
        if (item->type == TR::Entity::INV_PASSPORT && phaseChoose == 1.0f) {
            //
        } else
            UI::textOut(vec2(0, 480 - 32), item->desc.str, UI::aCenter, UI::width);

        renderItemCount(item, vec2(UI::width / 2 - 160, 480 - 96), 320);

        if (phaseChoose == 1.0f) {
            switch (item->type) {
                case TR::Entity::INV_PASSPORT :
                    renderPassport(item);
                    break;
                case TR::Entity::INV_HOME :
                case TR::Entity::INV_COMPASS   :
                case TR::Entity::INV_STOPWATCH :
                case TR::Entity::INV_MAP :
                    break;
                case TR::Entity::INV_DETAIL :
                    renderDetail(item);
                    break;
                case TR::Entity::INV_SOUND :
                    renderSound(item);
                    break;
                case TR::Entity::INV_CONTROLS :
                case TR::Entity::INV_GAMMA :
                    UI::textOut(vec2(0, 240), STR_NOT_IMPLEMENTED, UI::aCenter, UI::width);
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

    void render() {
    // background
        Core::setDepthTest(false);
        Core::setBlending(bmNone);

        if (background[0]) {
            background[0]->bind(sDiffuse);  // orignal image
            if (background[1]) {
                game->setShader(Core::passFilter, Shader::FILTER_MIXER, false, false);
                Core::active.shader->setParam(uParam, vec4(phaseRing, 1.0f - phaseRing * 0.4f, 0, 0));;
                background[1]->bind(sNormal);   // blured grayscale image
            } else {
                game->setShader(Core::passFilter, Shader::DEFAULT, false, false);
                
                float aspectSrc = float(background[0]->origWidth) / float(background[0]->origHeight);
                float aspectDst = float(Core::width) / float(Core::height);
                float aspectImg = aspectDst / aspectSrc;
                float ax = background[0]->origWidth  / float(background[0]->width);
                float ay = background[0]->origHeight / float(background[0]->height);

                Core::active.shader->setParam(uParam, vec4(ax * aspectImg, -ay, (0.5f - aspectImg * 0.5f) * ax, ay));
            }
            Core::setBlending(bmNone);
            game->getMesh()->renderQuad();
        }

        Core::setDepthTest(true);
        Core::setBlending(bmAlpha);

        if (game->getLevel()->isCutsceneLevel())
            return;

    // items
        game->setupBinding();

        Core::mLightProj.identity();

        Core::mView.identity();
        Core::mView.translate(vec3(-Core::eye * 8.0f, 0, -1286));   // y = -96 in title 

        Core::mView.up  *= -1.0f;
        Core::mView.dir *= -1.0f;
        Core::mViewInv = Core::mView.inverse();

        float aspect = float(Core::width) / float(Core::height);

        Core::mProj     = mat4(70.0f, aspect, 32.0f, 2048.0f);
        Core::mViewProj = Core::mProj * Core::mView;
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

        if (!game->getLevel()->isTitle())
            UI::textOut(vec2( 0, 32), pageTitle[page], UI::aCenter, UI::width);

        if (canFlipPage(-1)) {
            UI::textOut(vec2(16, 32), "[", UI::aLeft, UI::width, UI::SHADE_NONE);
            UI::textOut(vec2( 0, 32), "[", UI::aRight, UI::width - 20, UI::SHADE_NONE);
        }

        if (canFlipPage(1)) {
            UI::textOut(vec2(16, 480 - 16), "]", UI::aLeft, UI::width, UI::SHADE_NONE);
            UI::textOut(vec2(0,  480 - 16), "]", UI::aRight, UI::width - 20, UI::SHADE_NONE);
        }

        if (index == targetIndex)
            renderItemText(items[getGlobalIndex(page, index)]);
    }
};

#endif
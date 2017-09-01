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

    struct Item {
        TR::Entity::Type    type;
        int                 count;
        float               angle;
        Animation           *anim;

        struct Desc {
            const char *name;
            Page        page;
            int         model;
        } desc;

        Item() : anim(NULL) {}

        Item(TR::Level *level, TR::Entity::Type type, int count = 1) : type(type), count(count), angle(0.0f) {
            switch (type) {
                case TR::Entity::INV_PASSPORT        : desc = { "Game",            PAGE_OPTION,    level->extra.inv.passport        }; break;
                case TR::Entity::INV_PASSPORT_CLOSED : desc = { "Game",            PAGE_OPTION,    level->extra.inv.passport_closed }; break;
                case TR::Entity::INV_MAP             : desc = { "Map",             PAGE_INVENTORY, level->extra.inv.map             }; break;
                case TR::Entity::INV_COMPASS         : desc = { "Compass",         PAGE_INVENTORY, level->extra.inv.compass         }; break;
                case TR::Entity::INV_HOME            : desc = { "Lara's Home",     PAGE_OPTION,    level->extra.inv.home            }; break;
                case TR::Entity::INV_DETAIL          : desc = { "Detail Levels",   PAGE_OPTION,    level->extra.inv.detail          }; break;
                case TR::Entity::INV_SOUND           : desc = { "Sound",           PAGE_OPTION,    level->extra.inv.sound           }; break;
                case TR::Entity::INV_CONTROLS        : desc = { "Controls",        PAGE_OPTION,    level->extra.inv.controls        }; break;
                case TR::Entity::INV_GAMMA           : desc = { "Gamma",           PAGE_OPTION,    level->extra.inv.gamma           }; break;
                                                                                   
                case TR::Entity::INV_PISTOLS         : desc = { "Pistols",         PAGE_INVENTORY, level->extra.inv.weapon[0]       }; break;
                case TR::Entity::INV_SHOTGUN         : desc = { "Shotgun",         PAGE_INVENTORY, level->extra.inv.weapon[1]       }; break;
                case TR::Entity::INV_MAGNUMS         : desc = { "Magnums",         PAGE_INVENTORY, level->extra.inv.weapon[2]       }; break;
                case TR::Entity::INV_UZIS            : desc = { "Uzis",            PAGE_INVENTORY, level->extra.inv.weapon[3]       }; break;
                                                                                   
                case TR::Entity::INV_AMMO_PISTOLS    : desc = { "Pistol Clips",    PAGE_INVENTORY, level->extra.inv.ammo[0]         }; break;
                case TR::Entity::INV_AMMO_SHOTGUN    : desc = { "Shotgun Shells",  PAGE_INVENTORY, level->extra.inv.ammo[1]         }; break;
                case TR::Entity::INV_AMMO_MAGNUMS    : desc = { "Magnum Clips",    PAGE_INVENTORY, level->extra.inv.ammo[2]         }; break;
                case TR::Entity::INV_AMMO_UZIS       : desc = { "Uzi Clips",       PAGE_INVENTORY, level->extra.inv.ammo[3]         }; break;

                case TR::Entity::INV_MEDIKIT_SMALL   : desc = { "Small Medi Pack", PAGE_INVENTORY, level->extra.inv.medikit[0]      }; break;
                case TR::Entity::INV_MEDIKIT_BIG     : desc = { "Large Medi Pack", PAGE_INVENTORY, level->extra.inv.medikit[1]      }; break;

                case TR::Entity::INV_PUZZLE_1        : desc = { "Puzzle",          PAGE_ITEMS,     level->extra.inv.puzzle[0]       }; break;
                case TR::Entity::INV_PUZZLE_2        : desc = { "Puzzle",          PAGE_ITEMS,     level->extra.inv.puzzle[1]       }; break;
                case TR::Entity::INV_PUZZLE_3        : desc = { "Puzzle",          PAGE_ITEMS,     level->extra.inv.puzzle[2]       }; break;
                case TR::Entity::INV_PUZZLE_4        : desc = { "Puzzle",          PAGE_ITEMS,     level->extra.inv.puzzle[3]       }; break;
                                                                                                   
                case TR::Entity::INV_KEY_1           : desc = { "Key",             PAGE_ITEMS,     level->extra.inv.key[0]          }; break;
                case TR::Entity::INV_KEY_2           : desc = { "Key",             PAGE_ITEMS,     level->extra.inv.key[1]          }; break;
                case TR::Entity::INV_KEY_3           : desc = { "Key",             PAGE_ITEMS,     level->extra.inv.key[2]          }; break;
                case TR::Entity::INV_KEY_4           : desc = { "Key",             PAGE_ITEMS,     level->extra.inv.key[3]          }; break;
                                                                                                   
                case TR::Entity::INV_LEADBAR         : desc = { "Lead Bar",        PAGE_ITEMS,     level->extra.inv.leadbar         }; break;
                case TR::Entity::INV_SCION           : desc = { "Scion",           PAGE_ITEMS,     level->extra.inv.scion           }; break;
                default                              : desc = { "unknown",         PAGE_ITEMS,     -1                               }; break;
            }

            if (desc.model > -1) {
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
            if (anim) anim->update();
        }

        void render(IGame *game, const Basis &basis) {
            if (!anim) return;

            TR::Level *level = game->getLevel();
            TR::Model &m     = level->models[desc.model];
            Basis joints[34];

            anim->getJoints(basis, -1, true, joints);

            Core::active.shader->setParam(uBasis, joints[0], m.mCount);

            game->getMesh()->renderModel(desc.model);
        }

        void choose() {
            if (anim) anim->setAnim(0, 0, false);
        }

    } *items[INVENTORY_MAX_ITEMS];

    static void loadTitleBG(Stream *stream, void *userData) {
        if (!stream) return;
        Inventory *inv = (Inventory*)userData;

        inv->background[0] = Texture::LoadPCX(*stream);
        delete stream;
    }

    Inventory(IGame *game) : game(game), active(false), chosen(false), index(0), targetIndex(0), page(PAGE_OPTION), targetPage(PAGE_OPTION), itemsCount(0) {
        TR::LevelID id = game->getLevel()->id;

        add(TR::Entity::INV_PASSPORT);
        add(TR::Entity::INV_DETAIL);
        add(TR::Entity::INV_SOUND);
        add(TR::Entity::INV_CONTROLS);

        if (id != TR::TITLE) {
/*
            add(TR::Entity::INV_COMPASS);
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
//              add(TR::Entity::INV_KEY_1, 1);
//              add(TR::Entity::INV_PUZZLE_1, 1);

            for (int i = 0; i < COUNT(background); i++)
                background[i] = new Texture(INVENTORY_BG_SIZE, INVENTORY_BG_SIZE, Texture::RGBA, false);
        } else {
            add(TR::Entity::INV_HOME);

            memset(background, 0, sizeof(background));

            new Stream("data/TITLEH.PCX", loadTitleBG, this);
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

    void add(TR::Entity::Type type, int count = 1) {
        type = TR::Entity::convToInv(type);

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

        int i = contains(type);
        if (i > -1) {
            items[i]->count += count;
            return;
        }

        ASSERT(itemsCount < INVENTORY_MAX_ITEMS);

        int pos = 0;
        for (int pos = 0; pos < itemsCount; pos++)
            if (items[pos]->type > type)
                break;

        if (pos - itemsCount) {
            for (int i = itemsCount; i > pos; i--)
                items[i] = items[i - 1];
        }

        items[pos] = new Item(game->getLevel(), type, count);
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
        if (type == TR::Entity::NONE)
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

    bool toggle(Page curPage = PAGE_INVENTORY, TR::Entity::Type type = TR::Entity::NONE) {
        if (phaseRing == 0.0f || phaseRing == 1.0f) {
            active = !active;
            vec3 p;
            game->playSound(active ? TR::SND_INV_SHOW : TR::SND_INV_HIDE, p, 0, 0);
            chosen = false;

            if (active) {
                for (int i = 0; i < itemsCount; i++)
                    items[i]->reset();

                phasePage   = 1.0f;
                phaseSelect = 1.0f;
                page      = targetPage  = curPage;

                if (type != TR::Entity::NONE) {
                    int i = contains(type);
                    if (i >= 0)
                        pageItemIndex[page] = getLocalIndex(i);
                }

                index = targetIndex = pageItemIndex[page];
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
        int idx = getGlobalIndex(page, index);
        TR::Entity::Type type = items[idx]->type;
        return active && phaseRing == 1.0f && index == targetIndex && phasePage == 1.0f && (type == TR::Entity::INV_MEDIKIT_SMALL || type == TR::Entity::INV_MEDIKIT_BIG);
    }

    void update() {
        if (phaseChoose == 0.0f)
            doPhase(active, 2.0f, phaseRing);
        doPhase(true,   1.6f, phasePage);
        doPhase(chosen, 4.0f, phaseChoose);
        doPhase(true,   2.5f, phaseSelect);

        if (page != targetPage && phasePage == 1.0f) {
            page  = targetPage;
            index = targetIndex = pageItemIndex[page];
        }

        if (index != targetIndex && phaseSelect == 1.0f)
            index = pageItemIndex[page] = targetIndex;

        int count = getItemsCount(page);

        bool ready = active && phaseRing == 1.0f && phasePage == 1.0f;

        if (index == targetIndex && targetPage == page && ready && !chosen) {
            if (Input::state[cLeft]  || Input::joy.L.x < -0.5f || Input::joy.R.x >  0.5f) { phaseSelect = 0.0f; targetIndex = (targetIndex - 1 + count) % count; }
            if (Input::state[cRight] || Input::joy.L.x >  0.5f || Input::joy.R.x < -0.5f) { phaseSelect = 0.0f; targetIndex = (targetIndex + 1) % count;         }
            if ((Input::state[cUp]   || Input::joy.L.y < -0.5f || Input::joy.R.y >  0.5f) && page < PAGE_ITEMS  && getItemsCount(page + 1)) { phasePage = 0.0f; targetPage = Page(page + 1); }
            if ((Input::state[cDown] || Input::joy.L.y >  0.5f || Input::joy.R.y < -0.5f) && page > PAGE_OPTION && getItemsCount(page - 1)) { phasePage = 0.0f; targetPage = Page(page - 1); }

            if (index != targetIndex) {
                vec3 p;
                game->playSound(TR::SND_INV_SPIN, p, 0, 0);
            }
        }

        vec3 p;
        
        Item *item = items[getGlobalIndex(page, index)];

        if (index == targetIndex && ready) {
            if (Input::state[cAction] && (phaseChoose == 0.0f || (phaseChoose == 1.0f && item->anim->isEnded))) {
                chosen = !chosen;
                if (!chosen) {
                    item->angle = 0.0f;
                } else {
                    switch (item->type) {
                        case TR::Entity::INV_COMPASS  : game->playSound(TR::SND_INV_COMPASS, p, 0, 0);   break;
                        case TR::Entity::INV_HOME     : game->playSound(TR::SND_INV_HOME, p, 0, 0);      break;
                        case TR::Entity::INV_CONTROLS : game->playSound(TR::SND_INV_CONTROLS, p, 0, 0);  break;
                        case TR::Entity::INV_PISTOLS  :
                        case TR::Entity::INV_SHOTGUN  :
                        case TR::Entity::INV_MAGNUMS  :
                        case TR::Entity::INV_UZIS     : game->playSound(TR::SND_INV_WEAPON, p, 0, 0);    break;
                        default                       : game->playSound(TR::SND_INV_SHOW, p, 0, 0);      break;
                    }
                    item->choose();
                }
            }
        }

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
    }

    void prepareBackground() {
        Core::setDepthTest(false);

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
            UI::textOut(pos, buf, UI::aRight, width);
        }
    }

    void renderItemText(const Item *item, float width) {
        UI::textOut(vec2(0, 480 - 16), item->desc.name, UI::aCenter, width);
        renderItemCount(item, vec2(width / 2 - 160, 480 - 96), 320);

        if (phaseChoose == 1.0f) {
            if (item->type == TR::Entity::INV_PASSPORT ||
                item->type == TR::Entity::INV_MAP      || 
                item->type == TR::Entity::INV_COMPASS  || 
                item->type == TR::Entity::INV_HOME     || 
                item->type == TR::Entity::INV_DETAIL   || 
                item->type == TR::Entity::INV_SOUND    || 
                item->type == TR::Entity::INV_CONTROLS || 
                item->type == TR::Entity::INV_GAMMA)
            {
                UI::textOut(vec2(0, 240), "Not implemented yet!", UI::aCenter, width);
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

        if (background[0]) {
            background[0]->bind(sDiffuse);  // orignal image
            if (background[1]) {
                game->setShader(Core::passFilter, Shader::FILTER_MIXER, false, false);
                Core::active.shader->setParam(uParam, vec4(phaseRing, 1.0f - phaseRing * 0.4f, 0, 0));;
                background[1]->bind(sNormal);   // blured grayscale image
            } else {
                game->setShader(Core::passFilter, Shader::DEFAULT, false, false);

                float aspect1 = float(background[0]->width) / float(background[0]->height);
                float aspect2 = float(Core::width) / float(Core::height);
                Core::active.shader->setParam(uParam, vec4(aspect2 / aspect1, -1.0f, 0, 0));
            }
            game->getMesh()->renderQuad();
        }

        Core::setDepthTest(true);
        Core::setBlending(bmAlpha);

        if (game->isCutscene())
            return;

    // items
        game->setupBinding();

        Core::mLightProj.identity();

        Core::mView.identity();
        Core::mView.translate(vec3(0, 0, -1286));   // y = -96 in title 

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

        static const char* pageTitle[PAGE_MAX] = { "OPTION", "INVENTORY", "ITEMS" };

        if (game->getLevel()->id != TR::TITLE)
            UI::textOut(vec2( 0, 32), pageTitle[page], UI::aCenter, UI::width);

        if (page < PAGE_ITEMS && getItemsCount(page + 1)) {
            UI::textOut(vec2(16, 32), "[", UI::aLeft, UI::width);
            UI::textOut(vec2( 0, 32), "[", UI::aRight, UI::width - 20);
        }

        if (page > PAGE_OPTION && getItemsCount(page - 1)) {
            UI::textOut(vec2(16, 480 - 16), "]", UI::aLeft, UI::width);
            UI::textOut(vec2(0,  480 - 16), "]", UI::aRight, UI::width - 20);
        }

        if (index == targetIndex)
            renderItemText(items[getGlobalIndex(page, index)], UI::width);
    }
};

#endif
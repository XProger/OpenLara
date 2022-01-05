#ifndef H_INVENTORY
#define H_INVENTORY

#include "common.h"

#define INV_CAMERA_HEIGHT   -1536
#define INV_CAMERA_Y        96
#define INV_CAMERA_Z        768
#define INV_RING_RADIUS     688

enum InvState {
    INV_STATE_NONE,
    INV_STATE_OPENING,
    INV_STATE_READY,
    INV_STATE_CLOSING,
    INV_STATE_SPIN,
    INV_STATE_PAGE_MAIN,
    INV_STATE_PAGE_KEYS,
    INV_STATE_PAGE_OPTIONS
};

enum InvPage {
    INV_PAGE_TITLE,
    INV_PAGE_SAVE,
    INV_PAGE_DEATH,
    INV_PAGE_MAIN,
    INV_PAGE_KEYS,
    INV_PAGE_OPTIONS
};

enum InvSlot {
// Items
    SLOT_LEADBAR,
    SLOT_KEY_ITEM_1,
    SLOT_KEY_ITEM_2,
    SLOT_KEY_ITEM_3,
    SLOT_KEY_ITEM_4,
    SLOT_PUZZLE_4,
    SLOT_PUZZLE_3,
    SLOT_PUZZLE_2,
    SLOT_PUZZLE_1,
    SLOT_SCION,
// Inventory
    SLOT_COMPASS,
    SLOT_PISTOLS,
    SLOT_AMMO_PISTOLS,
    SLOT_SHOTGUN,
    SLOT_AMMO_SHOTGUN,
    SLOT_MAGNUMS,
    SLOT_AMMO_MAGNUMS,
    SLOT_UZIS,
    SLOT_AMMO_UZIS,
    SLOT_MEDIKIT_BIG,
    SLOT_MEDIKIT_SMALL,
// Options
    SLOT_PASSPORT,
    SLOT_DETAIL,
    SLOT_SOUND,
    SLOT_CONTROLS,
    SLOT_HOME,
    SLOT_MAX,
};

struct InvItem
{
    uint8 type;
    StringID str;
    // TODO params
};

const InvItem INV_SLOTS[SLOT_MAX] = {
    { ITEM_INV_LEADBAR        , STR_LEAD_BAR      },
    { ITEM_INV_KEY_ITEM_1     , STR_KEY           },
    { ITEM_INV_KEY_ITEM_2     , STR_KEY           },
    { ITEM_INV_KEY_ITEM_3     , STR_KEY           },
    { ITEM_INV_KEY_ITEM_4     , STR_KEY           },
    { ITEM_INV_PUZZLE_4       , STR_PUZZLE        },
    { ITEM_INV_PUZZLE_3       , STR_PUZZLE        },
    { ITEM_INV_PUZZLE_2       , STR_PUZZLE        },
    { ITEM_INV_PUZZLE_1       , STR_PUZZLE        },
    { ITEM_INV_SCION          , STR_SCION         },
    { ITEM_INV_COMPASS        , STR_COMPASS       },
    { ITEM_INV_PISTOLS        , STR_PISTOLS       },
    { ITEM_INV_AMMO_PISTOLS   , STR_AMMO_PISTOLS  },
    { ITEM_INV_SHOTGUN        , STR_SHOTGUN       },
    { ITEM_INV_AMMO_SHOTGUN   , STR_AMMO_SHOTGUN  },
    { ITEM_INV_MAGNUMS        , STR_MAGNUMS       },
    { ITEM_INV_AMMO_MAGNUMS   , STR_AMMO_MAGNUMS  },
    { ITEM_INV_UZIS           , STR_UZIS          },
    { ITEM_INV_AMMO_UZIS      , STR_AMMO_UZIS     },
    { ITEM_INV_MEDIKIT_BIG    , STR_MEDI_BIG      },
    { ITEM_INV_MEDIKIT_SMALL  , STR_MEDI_SMALL    },
    { ITEM_INV_PASSPORT       , STR_GAME          },
    { ITEM_INV_DETAIL         , STR_DETAIL        },
    { ITEM_INV_SOUND          , STR_SOUND         },
    { ITEM_INV_CONTROLS       , STR_CONTROLS      },
    { ITEM_INV_HOME           , STR_HOME          },
};


struct Inventory
{
    InvSlot useSlot;

    int32 numKeys;

    int16 counts[SLOT_MAX];

    int32 height;
    int32 radius;
    int32 pitch;
    int16 rot;
    int16 rotItem;

    ItemObj* lara;
    InvPage page;
    InvState state;
    InvState nextState;
    int32 timer;

    int32 heightTarget;
    int32 heightInc;
    int32 radiusTarget;
    int32 radiusInc;
    int32 pitchTarget;
    int32 pitchInc;
    int16 rotTarget;
    int16 rotInc;

    InvSlot itemsList[SLOT_MAX];
    int32   itemsCount;
    int32   itemIndex;

    void* background;

    Inventory()
    {
        memset(counts, 0, sizeof(counts));

        useSlot = SLOT_MAX;
        numKeys = 0;

        add(ITEM_INV_PASSPORT);
        add(ITEM_INV_DETAIL);
        add(ITEM_INV_SOUND);
        add(ITEM_INV_CONTROLS);
        //add(ITEM_INV_GAMMA);
        //add(ITEM_INV_HOME);

        add(ITEM_INV_COMPASS);
        add(ITEM_INV_PISTOLS);

        //add(ITEM_INV_KEY_ITEM_1);
        //add(ITEM_INV_PUZZLE_1);
    }

    ItemType remapToInv(ItemType type)
    {
        switch (type)
        {
            case ITEM_PISTOLS               : return ITEM_INV_PISTOLS;
            case ITEM_SHOTGUN               : return ITEM_INV_SHOTGUN;
            case ITEM_MAGNUMS               : return ITEM_INV_MAGNUMS;
            case ITEM_UZIS                  : return ITEM_INV_UZIS;
            case ITEM_AMMO_PISTOLS          : return ITEM_INV_AMMO_PISTOLS;
            case ITEM_AMMO_SHOTGUN          : return ITEM_INV_AMMO_SHOTGUN;
            case ITEM_AMMO_MAGNUMS          : return ITEM_INV_AMMO_MAGNUMS;
            case ITEM_AMMO_UZIS             : return ITEM_INV_AMMO_UZIS;
            case ITEM_MEDIKIT_SMALL         : return ITEM_INV_MEDIKIT_SMALL;
            case ITEM_MEDIKIT_BIG           : return ITEM_INV_MEDIKIT_BIG;
            case ITEM_PUZZLE_1              : return ITEM_INV_PUZZLE_1;
            case ITEM_PUZZLE_2              : return ITEM_INV_PUZZLE_2;
            case ITEM_PUZZLE_3              : return ITEM_INV_PUZZLE_3;
            case ITEM_PUZZLE_4              : return ITEM_INV_PUZZLE_4;
            case ITEM_LEADBAR               : return ITEM_INV_LEADBAR;
            case ITEM_KEY_ITEM_1            : return ITEM_INV_KEY_ITEM_1;
            case ITEM_KEY_ITEM_2            : return ITEM_INV_KEY_ITEM_2;
            case ITEM_KEY_ITEM_3            : return ITEM_INV_KEY_ITEM_3;
            case ITEM_KEY_ITEM_4            : return ITEM_INV_KEY_ITEM_4;
            case ITEM_SCION_PICKUP_QUALOPEC :
            case ITEM_SCION_PICKUP_DROP     :
            case ITEM_SCION_PICKUP_HOLDER   : return ITEM_INV_SCION;
            default                         : return type;
        }
    }

    InvSlot remapToSlot(ItemType type)
    {
        type = remapToInv(type);

        for (int32 i = 0; i < X_COUNT(INV_SLOTS); i++)
        {
            if (INV_SLOTS[i].type == type)
                return (InvSlot)i;
        }

        ASSERT(false);
        return SLOT_COMPASS;
    }

    InvSlot remapHoleToSlot(ItemType type)
    {
        switch (type)
        {
            case ITEM_PUZZLEHOLE_1 : return SLOT_PUZZLE_1;
            case ITEM_PUZZLEHOLE_2 : return SLOT_PUZZLE_2;
            case ITEM_PUZZLEHOLE_3 : return SLOT_PUZZLE_3;
            case ITEM_PUZZLEHOLE_4 : return SLOT_PUZZLE_4;
            case ITEM_KEYHOLE_1    : return SLOT_KEY_ITEM_1;
            case ITEM_KEYHOLE_2    : return SLOT_KEY_ITEM_2;
            case ITEM_KEYHOLE_3    : return SLOT_KEY_ITEM_3;
            case ITEM_KEYHOLE_4    : return SLOT_KEY_ITEM_4;
            default                : return SLOT_MAX;
        }
    }

    void add(ItemType type, int32 count = 1)
    {
        InvSlot slot = remapToSlot(type);
        counts[slot] += count;
        // TODO check max

        if (slot < SLOT_COMPASS) {
            numKeys += count;
        }
    }

    void remove(InvSlot slot, int32 count)
    {
        counts[slot] -= count;

        if (slot < SLOT_COMPASS) {
            numKeys -= count;
        }
    }

    void show(ItemObj* lara, ItemObj* hole)
    {
        if (hole) {
            useSlot = remapHoleToSlot((ItemType)hole->type);
        }
    }

    bool applyItem(ItemObj* hole)
    {
        #define CHECK_CASE(A, B) case A: { if (useSlot != B) return false; break; }

        switch (hole->type)
        {
            CHECK_CASE(ITEM_PUZZLEHOLE_1, SLOT_PUZZLE_1);
            CHECK_CASE(ITEM_PUZZLEHOLE_2, SLOT_PUZZLE_2);
            CHECK_CASE(ITEM_PUZZLEHOLE_3, SLOT_PUZZLE_3);
            CHECK_CASE(ITEM_PUZZLEHOLE_4, SLOT_PUZZLE_4);
            CHECK_CASE(ITEM_KEYHOLE_1,    SLOT_KEY_ITEM_1);
            CHECK_CASE(ITEM_KEYHOLE_2,    SLOT_KEY_ITEM_2);
            CHECK_CASE(ITEM_KEYHOLE_3,    SLOT_KEY_ITEM_3);
            CHECK_CASE(ITEM_KEYHOLE_4,    SLOT_KEY_ITEM_4);
            default: return false;
        }

        remove(useSlot, 1);
        useSlot = SLOT_MAX;

        return true;
    }

    void setState(InvState state, InvState nextState, int32 timer)
    {
        this->state = state;
        this->nextState = nextState;
        this->timer = timer;
    }

    void setHeight(int32 target)
    {
        heightTarget = target;
        heightInc = (target - height) / timer;
    }

    void setRadius(int32 target)
    {
        radiusTarget = target;
        radiusInc = (target - radius) / timer; 
    }

    void setPitch(int32 target)
    {
        pitchTarget = target;
        pitchInc = (target - pitch) / timer; 
    }

    void setRot(int32 delta, int32 target)
    {
        rotTarget = target;
        rotInc = delta / timer;
    }

    void setPage(InvPage page)
    {
        this->page = page;

        itemsCount = 0;

        #define ADD_SLOT(slot) itemsList[itemsCount++] = slot;

        switch (page)
        {
            case INV_PAGE_SAVE:
            {
                break;
            }

            case INV_PAGE_TITLE:
            case INV_PAGE_DEATH:
            case INV_PAGE_OPTIONS:
            {
                ADD_SLOT(SLOT_PASSPORT);
                ADD_SLOT(SLOT_DETAIL);
                ADD_SLOT(SLOT_SOUND);
                ADD_SLOT(SLOT_CONTROLS);
                if (page == INV_PAGE_TITLE)
                {
                    ADD_SLOT(SLOT_HOME);
                }
                break;
            }

            case INV_PAGE_MAIN:
            {
                ADD_SLOT(SLOT_COMPASS);
                ADD_SLOT(SLOT_PISTOLS);
                ADD_SLOT(SLOT_SHOTGUN);
                ADD_SLOT(SLOT_MAGNUMS);
                ADD_SLOT(SLOT_UZIS);
                ADD_SLOT(SLOT_MEDIKIT_BIG);
                ADD_SLOT(SLOT_MEDIKIT_SMALL);
                break;
            }

            case INV_PAGE_KEYS:
            {
                ADD_SLOT(SLOT_LEADBAR);
                ADD_SLOT(SLOT_KEY_ITEM_1);
                ADD_SLOT(SLOT_KEY_ITEM_2);
                ADD_SLOT(SLOT_KEY_ITEM_3);
                ADD_SLOT(SLOT_KEY_ITEM_4);
                ADD_SLOT(SLOT_PUZZLE_4);
                ADD_SLOT(SLOT_PUZZLE_3);
                ADD_SLOT(SLOT_PUZZLE_2);
                ADD_SLOT(SLOT_PUZZLE_1);
                ADD_SLOT(SLOT_SCION);
                break;
            }
        }

        itemIndex = 0;

        setRot(ANGLE_180, itemIndex * ANGLE_360 / itemsCount); // TODO
        rot = rotTarget - ANGLE_180;
    }

    void open(ItemObj* lara, InvPage page)
    {
        if (gBrightness != 0)
            return;

        soundPlay(SND_INV_SHOW, NULL);

        this->lara = lara;

        background = copyBackground();

        height = INV_CAMERA_HEIGHT;
        pitch = (page == INV_PAGE_TITLE) ? 1024 : 0;
        radius = 0;

        setState(INV_STATE_OPENING, INV_STATE_READY, 16);
        setHeight(-256);
        setRadius(INV_RING_RADIUS);
        setPage(page);

        update(1);
    }

    void close()
    {
        soundPlay(SND_INV_HIDE, NULL);

        setState(INV_STATE_CLOSING, INV_STATE_NONE, 16);
        setHeight(-1536);
        setRadius(0);
        setRot(ANGLE_180, rot - ANGLE_180);
    }

    int32 getKeysCount()
    {
        int32 sum = 0;

        for (int32 i = SLOT_LEADBAR; i <= SLOT_SCION; i++)
        {
            sum += counts[i];
        }

        return sum;
    }

    void update(int32 frames)
    {
        if (timer > 0)
        {
            timer -= frames;

            height += heightInc * frames;
            radius += radiusInc * frames;
            pitch += pitchInc * frames;
            rot += rotInc * frames;
            //rotItem += rotItemInc * frames;

            if (timer <= 0)
            {
                timer = 0;
                state = nextState;
                height = heightTarget;
                radius = radiusTarget;
                pitch = pitchTarget;
                rot = rotTarget;

                heightInc = 0;
                radiusInc = 0;
                pitchInc = 0;
                rotInc = 0;
                rotItem = 0;
            }
        }

        rotItem += frames * 256;

        switch (state)
        {
            case INV_STATE_NONE:
            {
                break;
            }

            case INV_STATE_OPENING:
            {
                break;
            }

            case INV_STATE_READY:
            {
                if (lara->input & IN_LEFT) {
                    soundPlay(SND_INV_SPIN, NULL);
                    itemIndex++;
                    if (itemIndex >= itemsCount) {
                        itemIndex -= itemsCount;
                    }
                    setState(INV_STATE_SPIN, INV_STATE_READY, 12);
                    setRot(ANGLE_360 / itemsCount, itemIndex * ANGLE_360 / itemsCount);
                } else if (lara->input & IN_RIGHT) {
                    soundPlay(SND_INV_SPIN, NULL);
                    itemIndex--;
                    if (itemIndex < 0) {
                        itemIndex += itemsCount;
                    }
                    setState(INV_STATE_SPIN, INV_STATE_READY, 12);
                    setRot(-ANGLE_360 / itemsCount, itemIndex * ANGLE_360 / itemsCount);
                } else {
                    if (page != INV_PAGE_TITLE && page != INV_PAGE_SAVE && page != INV_PAGE_DEATH)
                    {
                        if (lara->input & IN_UP) {
                            if (page == INV_PAGE_OPTIONS) {
                                setState(INV_STATE_CLOSING, INV_STATE_PAGE_MAIN, 12);
                                setRadius(0);
                                setRot(ANGLE_180, rot - ANGLE_180);
                                setPitch(ANGLE_45);
                            } else if ((page == INV_PAGE_MAIN) && (getKeysCount() > 0)) {
                                setState(INV_STATE_CLOSING, INV_STATE_PAGE_KEYS, 12);
                                setRadius(0);
                                setRot(ANGLE_180, rot - ANGLE_180);
                                setPitch(ANGLE_45);
                            }
                        } else if (lara->input & IN_DOWN) {
                            if (page == INV_PAGE_KEYS) {
                                setState(INV_STATE_CLOSING, INV_STATE_PAGE_MAIN, 12);
                                setRadius(0);
                                setRot(ANGLE_180, rot - ANGLE_180);
                                setPitch(-ANGLE_45);
                            } else if (page == INV_PAGE_MAIN) {
                                setState(INV_STATE_CLOSING, INV_STATE_PAGE_OPTIONS, 12);
                                setRadius(0);
                                setRot(ANGLE_180, rot - ANGLE_180);
                                setPitch(-ANGLE_45);
                            }
                        } else if (lara->input & IN_SELECT) {
                            close();
                        }
                    }
                }

                break;
            }

            case INV_STATE_CLOSING:
            {
                break;
            }

            case INV_STATE_PAGE_MAIN:
            {
                pitch = -pitch;
                setState(INV_STATE_OPENING, INV_STATE_READY, 12);
                setRadius(INV_RING_RADIUS);
                setPitch(0);
                setPage(INV_PAGE_MAIN);
                break;
            }

            case INV_STATE_PAGE_KEYS:
            {
                pitch = -pitch;
                setState(INV_STATE_OPENING, INV_STATE_READY, 12);
                setRadius(INV_RING_RADIUS);
                setPitch(0);
                setPage(INV_PAGE_KEYS);
                break;
            }

            case INV_STATE_PAGE_OPTIONS:
            {
                pitch = -pitch;
                setState(INV_STATE_OPENING, INV_STATE_READY, 12);
                setRadius(INV_RING_RADIUS);
                setPitch(0);
                setPage(INV_PAGE_OPTIONS);
                break;
            }

            default: ;
        }
    }

    void drawSlot(InvSlot slot)
    {
        int32 type = INV_SLOTS[slot].type;

        if (type == ITEM_INV_PASSPORT) {
            type = ITEM_INV_PASSPORT_CLOSED;
        }

        ItemObj item;
        memset(&item, 0, sizeof(item));
        item.type        = type;
        item.intensity   = 255;
        item.visibleMask = 0xFFFFFFFF;
        item.animIndex   = level.models[item.type].animIndex; // ctor called on existing memory, type is already initialized
        item.frameIndex  = level.anims[item.animIndex].frameBegin;
        item.state       = uint8(level.anims[item.animIndex].state);
        item.nextState   = item.state;
        item.goalState   = item.state;

        const AnimFrame *frameA, *frameB;
    
        int32 frameRate;
        int32 frameDelta = item.getFrames(frameA, frameB, frameRate);

        calcLightingStatic(255 << 5);
        drawNodesLerp(&item, frameA, frameB, frameDelta, frameRate);
    }

    void drawPage()
    {
        int16 angleX, angleY;

        anglesFromVector(0, -(height + INV_CAMERA_Y), -INV_CAMERA_Z, angleX, angleY);

        vec3i pos = _vec3i(0, height, radius + INV_CAMERA_Z);

        matrixSetView(pos, angleX + pitch, angleY);
        matrixTranslateAbs(0, 0, 0);

        for (int32 i = 0; i < itemsCount; i++)
        {
            matrixPush();
            matrixRotateY(i * ANGLE_360 / itemsCount - rot - ANGLE_90);
            matrixTranslateRel(radius, 0, 0);
            matrixRotateY(ANGLE_90);
            matrixRotateX(-3616);

            if (itemIndex == i) {
                matrixRotateY(rotItem);
            }

            drawSlot(itemsList[i]);

            matrixPop();
        }
    }

    void draw()
    {
        //clear();
        ASSERT(background);
        renderBackground(background);
        
        StringID title = STR_EMPTY;

        switch (page)
        {
            case INV_PAGE_TITLE:
            {
                break;
            }

            case INV_PAGE_SAVE:
            {
                break;
            }

            case INV_PAGE_DEATH:
            {
                break;
            }

            case INV_PAGE_MAIN:
            {
                title = STR_INV_TITLE_MAIN;
                break;
            }

            case INV_PAGE_KEYS:
            {
                title = STR_INV_TITLE_KEYS;
                break;
            }

            case INV_PAGE_OPTIONS:
            {
                title = STR_INV_TITLE_OPTIONS;
                break;
            }
        }

        drawPage();

        if (state == INV_STATE_READY || state == INV_STATE_SPIN)
        {
            drawText(0, 20, STR[title], TEXT_ALIGN_CENTER);
            
            if ((page == INV_PAGE_OPTIONS) || (page == INV_PAGE_MAIN && getKeysCount()))
            {
                drawText(4, 4 + 16, "[", TEXT_ALIGN_LEFT);
                drawText(-6, 4 + 16, "[", TEXT_ALIGN_RIGHT);
            }

            if (page == INV_PAGE_MAIN)
            {
                drawText(4, FRAME_HEIGHT - 5, "]", TEXT_ALIGN_LEFT);
                drawText(-6, FRAME_HEIGHT - 5, "]", TEXT_ALIGN_RIGHT);
            }

            if (state != INV_STATE_SPIN)
            {
                drawText(0, FRAME_HEIGHT - 8, STR[INV_SLOTS[itemsList[itemIndex]].str], TEXT_ALIGN_CENTER);
            }
        }
    }
};

EWRAM_DATA Inventory inventory;

#endif

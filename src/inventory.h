#ifndef H_INVENTORY
#define H_INVENTORY

#include "format.h"

#define MAX_ITEMS 64

struct Inventory {

    struct Item {
        TR::Entity::Type    type;
        int                 count;
    } items[MAX_ITEMS];
    int itemsCount;

    Inventory() : itemsCount(0) {}

    int contains(TR::Entity::Type type) {
        for (int i = 0; i < itemsCount; i++)
            if (items[i].type == type)
                return i;
        return -1;
    }

    void add(TR::Entity::Type type, int count = 1) {
        int i = contains(type);
        if (i > -1) {
            items[i].count += count;
            return;
        }

        if(itemsCount < MAX_ITEMS) {
            items[itemsCount].type  = type;
            items[itemsCount].count = count;
            itemsCount++;
        }
    }

    int getCount(TR::Entity::Type type) {
        int i = contains(type);
        if (i < 0) return 0;
        return items[i].count;
    }

    void remove(TR::Entity::Type type, int count = 1) {
        int i = contains(type);
        if (i > -1)
            items[i].count -= count;
    }
};

#endif
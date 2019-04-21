#ifndef _H_JSON
#define _H_JSON

#include "utils.h"

struct JSON {

    enum Type { EMPTY, OBJECT, ARRAY, STRING, NUMBER, FLOAT, BOOL };

    JSON *prev;
    JSON *next;

    char *name;

    union {
        JSON *nodes;
        char *sValue;
        int   iValue;
        float fValue;
        bool  bValue;
    };

    Type type;

    JSON(Type type, const char *name = NULL) : nodes(NULL), prev(NULL), next(NULL), type(type) {
        this->name = String::copy(name);
    }

    ~JSON() {
        switch (type) {
            case OBJECT :
            case ARRAY  : {
                JSON *node = nodes;
                while (node) {
                    JSON *next = node->next;
                    delete node;
                    node = next;
                }
                break;
            }
            case STRING : {
                delete[] sValue;
                break;
            }
            default : ;
        }

        delete[] name;
    }

    JSON* add(Type type, const char *name = NULL) {
        ASSERT(this->type != ARRAY || (name == NULL && (!nodes || nodes->type == type)));
        return add(new JSON(type, name));
    }

    JSON* add(JSON *node) {
        node->prev = NULL;
        node->next = nodes;
        if (nodes) {
            nodes->prev = node;
        }
        nodes = node;

        return node;
    }


    void add(const char *name, const char *value) {
        add(STRING, name)->sValue = String::copy(value);
    }

    void add(const char *name, int value) {
        add(NUMBER, name)->iValue = value;
    }

    void add(const char *name, float value) {
        add(FLOAT, name)->fValue = value;
    }

    void add(const char *name, bool value) {
        add(BOOL, name)->bValue = value;
    }

    void save(char *buffer) {
        *buffer = 0;

        if (name) {
            strcat(buffer, "\"");
            strcat(buffer, name);
            strcat(buffer, "\":");
        }

        if (type == EMPTY) {
            strcat(buffer, "null");
        } else if (type == OBJECT || type == ARRAY) {
            bool isObject = (type == OBJECT);
            strcat(buffer, isObject ? "{" : "[");

            JSON *node = nodes;
            while (node && node->next) {
                node = node->next;
            }
            while (node) {
                node->save(buffer + strlen(buffer));
                node = node->prev;
                if (node) {
                    strcat(buffer, ",");
                }
            }
            strcat(buffer, isObject ? "}" : "]");
        } else if (type == STRING) {
            strcat(buffer, "\"");
            if (sValue) {
                strcat(buffer, sValue);
            }
            strcat(buffer, "\"");
        } else if (type == NUMBER) {
            char buf[64];
            _itoa(iValue, buf, 10);
            strcat(buffer, buf);
        } else if (type == FLOAT) {
            char buf[64];
            sprintf(buf, "%.9g", fValue);
            strcat(buffer, buf);

            //_gcvt(fValue, 8, buf);
            //strcat(buffer, buf);
            //strcat(buffer, "0");
        } else if (type == BOOL) {
            strcat(buffer, bValue ? "true" : "false");
        }
    }
};

#endif

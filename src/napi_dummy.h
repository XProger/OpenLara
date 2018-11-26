#ifndef H_NAPI_DUMMY
#define H_NAPI_DUMMY

#include "utils.h"

namespace NAPI {
    typedef int Peer;

    void init() {}
    void deinit() {}
    void listen(uint16 port) {}
    int  send(const Peer &to, const void *data, int size) { return 0; }
    int  recv(Peer &from, void *data, int size) { return 0; }
    void broadcast(const void *data, int size) {}
}

#endif
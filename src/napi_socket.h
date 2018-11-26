#ifndef H_NAPI_SOCKET
#define H_NAPI_SOCKET

#include "utils.h"

#ifdef _OS_WIN
    #include "winsock.h"
#endif

namespace NAPI {

    struct Peer {
        uint16 port;
        uint32 ip;

        inline bool operator == (const Peer &peer) const {
            return port == peer.port && ip == peer.ip;
        }
    };

    SOCKET       sock;
    sockaddr_in  addr;
    uint16       port;

    void init() {
        sock = INVALID_SOCKET;

	    WSAData wData;
	    WSAStartup(0x0101, &wData);
    }

    void deinit() {
        if (sock != INVALID_SOCKET) {
            shutdown(sock, 1);
        #ifdef _OS_WIN
            closesocket(sock);
        #else
            close(sock);
        #endif
        }
        WSACleanup();
    }

    void listen(uint16 port) {
        NAPI::port = port;

        if (sock != INVALID_SOCKET) return;

        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) {
            LOG("! network: failed to create socket\n");
            sock = INVALID_SOCKET;
            return;
        }
        
        u_long on = 1;
        if (ioctlsocket(sock, FIONBIO, &on) < 0) {
            LOG("! network: failed to set non-blocking mode\n");
            closesocket(sock);
            sock = INVALID_SOCKET;
            return;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(port);
        if (bind(sock, (sockaddr*)&addr, sizeof(addr)))
            LOG("! network: unable to bind socket on port (%d)\n", (int)port);

        on = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&on, sizeof(on)))
            LOG("! network: unable to enable broadcasting\n");
    }

    int send(const Peer &to, const void *data, int size) {
        if (sock == INVALID_SOCKET) return false;

        addr.sin_addr.s_addr = to.ip;
        addr.sin_port        = to.port;
        return sendto(sock, (const char*)data, size, 0, (sockaddr*)&addr, sizeof(addr));
    }

    int recv(Peer &from, void *data, int size) {
        if (sock == INVALID_SOCKET) return false;

        int i = sizeof(addr);
        int count = recvfrom(sock, (char*)data, size, 0, (sockaddr*)&addr,  &i);
        if (count > 0) {
            from.ip   = addr.sin_addr.s_addr;
            from.port = addr.sin_port;
        }
        return count;
    }

    void broadcast(const void *data, int size) {
        Peer peer;
        peer.ip   = INADDR_BROADCAST;
        peer.port = htons(port);
        send(peer, data, size);
    }
}

#endif
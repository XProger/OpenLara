#ifndef H_NAPI_SOCKET
#define H_NAPI_SOCKET

#include "utils.h"

#ifdef _OS_WIN
    #include "winsock.h"
#endif

namespace NAPI {

    static struct {
        char   host[32];
        uint16 port;
    } stunServers[] = {
        { "stun.l.google.com",  19302 },
        { "stun1.l.google.com", 19302 },
        { "stun2.l.google.com", 19302 },
        { "stun3.l.google.com", 19302 },
        { "stun4.l.google.com", 19302 },
    };

    struct stun_header {
        uint16 msgtype;
        uint16 msglen;
        uint32 id[4];
    };

    struct stun_attr {
        uint16 attr;
        uint16 len;
    };

    #pragma pack(push, 1)
    struct stun_addr {
        unsigned char unused;
        unsigned char family;
        unsigned short port;
        unsigned int addr;
    };
    #pragma pack(pop)

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

    Peer         peer;
    bool         waitAddress;

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

    void handleAddress(const uint8 *data, int size) {
        if (size < sizeof(stun_header))
            return;
        stun_header *hdr = (stun_header*)data;
        data += sizeof(stun_header);
        size -= sizeof(stun_header);
        int i = ntohs(hdr->msglen);
        if (i < size)
            size = i;
        
        while (size) {
            if (size < sizeof(stun_attr))
                return;

            stun_attr *attr = (stun_attr*)data;
            i = ntohs(attr->len) + sizeof(stun_attr);
            if (i > size)
                return;

            stun_addr *addr = (stun_addr*)(data + sizeof(stun_attr));

            if (attr->attr == 0x0100 && ntohs(attr->len) == 8) {
                waitAddress = false;
                peer.ip   = addr->addr;
                peer.port = addr->port;
                LOG("network: acquire UDP tunnel %s:%d\n", inet_ntoa(*(in_addr*)&peer.ip), ntohs(peer.port));
            }
            
            data += i;
            size -= i;
        }
    }

    int send(const Peer &to, const void *data, int size) {
        if (sock == INVALID_SOCKET) return false;
        LOG("network: -> %s:%d (%d)\n", inet_ntoa(*(in_addr*)&to.ip), ntohs(to.port), size);

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
            LOG("network: <- %s:%d (%d)\n", inet_ntoa(*(in_addr*)&from.ip), ntohs(from.port), size);
        }

        if (waitAddress) {
            handleAddress((uint8*)data, size);
            return 0;
        }

        return count;
    }

    void broadcast(const void *data, int size) {
        Peer peer;
        peer.ip   = INADDR_BROADCAST;
        peer.port = htons(port);
        send(peer, data, size);
    }

    void requestAddress() {
        int stunIndex = rand() % COUNT(stunServers);

        hostent *hostinfo = gethostbyname(stunServers[stunIndex].host);
        if (!hostinfo)
            return;

        stun_header req;

        for (int i = 0; i < 4 * 2; i++)
            ((uint16*)req.id)[i] = rand();

        req.msgtype = 0;
        req.msglen  = 0;
        req.msgtype = 0x0100;

        Peer peer;
        peer.ip   = *(uint32*)hostinfo->h_addr;
        peer.port = htons(stunServers[stunIndex].port);
        send(peer, &req, sizeof(req));

        waitAddress = true;
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
        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            LOG("! network: unable to bind socket on port (%d)\n", (int)port);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            addr.sin_port        = 0;
            if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
                LOG("! network: unable to bind socket on ANY port\n");
            }
        }

        on = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&on, sizeof(on)))
            LOG("! network: unable to enable broadcasting\n");

        requestAddress();
    }
}

#endif
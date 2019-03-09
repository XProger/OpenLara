#ifndef H_NET
#define H_NET

#include "core.h"
#include "utils.h"
#include "format.h"
#include "controller.h"
#include "ui.h"

#define NET_PROTOCOL            1
#define NET_PORT                21468

#define NET_PING_TIMEOUT        ( 1000 * 10   )
#define NET_PING_PERIOD         ( 1000 * 3    )
#define NET_SYMC_INPUT_PERIOD   ( 1000 / 25   )
#define NET_SYMC_STATE_PERIOD   ( 1000 / 1000 )

namespace Network {

    struct Packet {
        enum Type {
            HELLO, INFO, PING, PONG, JOIN, ACCEPT, REJECT, INPUT, STATE,
        };

        uint16 type;
        uint16 id;

        union {
            struct {
                uint8 protocol;
                uint8 game;
            } hello;

            struct {
                str16 name;
                uint8 level;
                uint8 players;
                struct {
                    uint16 secure:1;
                } flags;
            } info;

            struct {
                str16 nick;
                str16 pass;
            } join;

            struct {
                uint16 id;
                uint8  level;
                uint8  roomIndex;
                int16  posX;
                int16  posY;
                int16  posZ;
                int16  angle;
            } accept;

            struct {
                uint16 reason;
            } reject;

            struct {
                uint16 mask;
            } input;

            struct {
                uint8  roomIndex;
                uint8  reserved;
                int16  pos[3];
                int16  angle[2];
                uint8  frame;
                uint8  stand;
                uint16 animIndex;
            } state;
        };

        int getSize() const {
            const int sizes[] = {
                sizeof(hello),
                sizeof(info),
                0,
                0,
                sizeof(join),
                sizeof(accept),
                sizeof(reject),
                sizeof(input),
                sizeof(state),
            };

            if (type >= 0 && type < COUNT(sizes))
                return 2 + 2 + sizes[type];
            ASSERT(false);
            return 0;
        }
    };

    IGame *game;

    struct Player {
        NAPI::Peer peer;
        int        pingTime;
        int        pingIndex;
        Controller *controller;
    };

    Array<Player> players;

    int syncInputTime;
    int syncStateTime;

    void start(IGame *game) {
        Network::game = game;
        NAPI::listen(NET_PORT);
        syncInputTime = syncStateTime = Core::getTime();
    }

    void stop() {
        players.clear();
    }

    bool sendPacket(const NAPI::Peer &to, const Packet &packet) {
        return NAPI::send(to, &packet, packet.getSize()) > 0;
    }

    bool recvPacket(NAPI::Peer &from, Packet &packet) {
        int count = NAPI::recv(from, &packet, sizeof(packet));
        if (count > 0) {
            if (count != packet.getSize()) {
                ASSERT(false);
                return false;
            }
            return true;
        }
        return false;
    }

    void sayHello() {
        Packet packet;
        packet.type           = Packet::HELLO;
        packet.hello.protocol = NET_PROTOCOL;
        packet.hello.game     = game->getLevel()->version & TR::VER_VERSION;

        NAPI::broadcast(&packet, packet.getSize());
    }

    void joinGame(const NAPI::Peer &peer) {
        Packet packet;
        packet.type      = Packet::JOIN;
        packet.join.nick = "Player_2";
        packet.join.pass = "";
        LOG("join game\n");
        sendPacket(peer, packet);
    }

    void pingPlayers(int time) {
        int i = 0;
        while (i < players.length) {
            int delta = time - players[i].pingTime;

            if (delta > NET_PING_TIMEOUT) {
                players.removeFast(i);
                continue;
            }

            if (delta > NET_PING_PERIOD) {
                Packet packet;
                packet.type = Packet::PING;
                sendPacket(players[i].peer, packet);
            }

            i++;
        }
    }

    void syncInput(int time) {
        Lara *lara = (Lara*)game->getLara();
        if (!lara) return;

        if ((time - syncInputTime) < NET_SYMC_INPUT_PERIOD)
            return;

        Packet packet;
        packet.type = Packet::INPUT;
        packet.input.mask = lara->getInput();

        for (int i = 0; i < players.length; i++)
            sendPacket(players[i].peer, packet);

        syncInputTime = time;
    }

    void syncState(int time) {
        if ((time - syncStateTime) < NET_SYMC_STATE_PERIOD)
            return;
        // TODO    
        syncStateTime = time;
    } 

    Player* getPlayerByPeer(const NAPI::Peer &peer) {
        for (int i = 0; i < players.length; i++)
            if (players[i].peer == peer) {
                return &players[i];
            }
        return NULL;
    }

    void getSpawnPoint(uint8 &roomIndex, vec3 &pos, float &angle) {
        Controller *lara = game->getLara();
        roomIndex = lara->getRoomIndex();
        pos       = lara->getPos();
        angle     = normalizeAngle(lara->angle.y); // 0..2PI
    }

    void update() {
        int count;
        NAPI::Peer from;
        Packet packet, response;

        int time = Core::getTime();

        while ( (count = recvPacket(from, packet)) > 0 ) {
            Player *player = getPlayerByPeer(from);
            if (player)
                player->pingTime = time;

            switch (packet.type) {
                case Packet::HELLO :
                    if (game->getLevel()->isTitle())
                        break;

                    LOG("recv HELLO\n");
                    if (packet.hello.game != (game->getLevel()->version & TR::VER_VERSION))
                        break;
                    if (packet.hello.protocol != NET_PROTOCOL)
                        break;
                    LOG("send INFO\n");
                    response.type         = Packet::INFO;
                    response.info.name    = "MultiOpenLara";
                    response.info.level   = game->getLevel()->id;
                    response.info.players = players.length + 1;
                    response.info.flags.secure = false;

                    sendPacket(from, response);

                    break;

                case Packet::INFO : {
                    LOG("recv INFO\n");
                    char buf[sizeof(packet.info.name) + 1];
                    packet.info.name.get(buf);
                    LOG("name: %s\n", buf);
                    joinGame(from);
                    break;
                }

                case Packet::PING :
                    if (player) {
                        response.type = Packet::PONG;
                        sendPacket(from, response);
                    }
                    break;

                case Packet::PONG :
                    break;

                case Packet::JOIN :
                    if (!player) {
                        uint8 roomIndex;
                        vec3  pos;
                        float angle;

                        getSpawnPoint(roomIndex, pos, angle);

                        Player newPlayer;
                        newPlayer.peer       = from;
                        newPlayer.pingIndex  = 0;
                        newPlayer.pingTime   = time;
                        newPlayer.controller = game->addEntity(TR::Entity::LARA, roomIndex, pos, angle);
                        players.push(newPlayer);

                        ((Lara*)newPlayer.controller)->networkInput = 0;

                        char buf[32];
                        packet.join.nick.get(buf);
                        LOG("Player %s joined\n", buf);

                        ASSERT(newPlayer.controller);

                        TR::Room &room = game->getLevel()->rooms[roomIndex];
                        vec3 offset = pos - room.getOffset();

                        response.type = Packet::ACCEPT;
                        response.accept.id        = 0;
                        response.accept.level     = game->getLevel()->id;
                        response.accept.roomIndex = roomIndex;
                        response.accept.posX      = int16(offset.x);
                        response.accept.posY      = int16(offset.y);
                        response.accept.posZ      = int16(offset.z);
                        response.accept.angle     = int16(angle * RAD2DEG);

                        sendPacket(from, response);
                    }
                    break;

                case Packet::ACCEPT : {
                    LOG("accept!\n");
                    game->loadLevel(TR::LevelID(packet.accept.level));
                    inventory->toggle();
                    break;
                }

                case Packet::REJECT :
                    break;

                case Packet::INPUT :
                    if (game->getLevel()->isTitle())
                        break;

                    if (!player) {
                        uint8 roomIndex;
                        vec3  pos;
                        float angle;

                        getSpawnPoint(roomIndex, pos, angle);

                        Player newPlayer;
                        newPlayer.peer       = from;
                        newPlayer.pingIndex  = 0;
                        newPlayer.pingTime   = time;
                        newPlayer.controller = game->addEntity(TR::Entity::LARA, roomIndex, pos, angle);
                        players.push(newPlayer);

                        ((Lara*)newPlayer.controller)->networkInput = 0;

                        player = getPlayerByPeer(from);
                    }

                    if (player) {
                        ((Lara*)player->controller)->networkInput = packet.input.mask;
                    }
                    break;

                case Packet::STATE :
                    break;
            }
        }

        pingPlayers(time);
        syncInput(time);
        syncState(time);
    }
}

#endif

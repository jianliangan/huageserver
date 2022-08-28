#ifndef _HUAGE_APP_HEAD_
#define _HUAGE_APP_HEAD_ 
#include <string>
#include "../net/protocol/core/tools.h"
#include "../net/protocol/rtp_packet.h"
#include "session.h"
#include "room.h"
#include "../net/threads/hg_channel.h"
#define YUVMAXWIDTH 2000
#define YUVMAXHEIGHT 2000
typedef struct agentAddr{
    struct sockaddr_in serverAddr;
}agentAddr;
typedef struct msg {
    uint8_t *data;
    uint32_t size;
}msg;
typedef struct server_packet_roommerge_pa {
    SessionC *sess;
    uint32_t pts;
    int comp;
    media_frame_chain mfc; 
    unsigned int ssrc;
    void *room;
} server_packet_roommerge_pa;

typedef struct sever_packet_login_pa{
    uint32_t ssrc;
    void *sess;
    void *bysess;
}sever_packet_login_pa;

typedef struct ch_text_room_enter{
    SessionC *sess;
    unsigned int roomid;
    unsigned int uniqid;
    bool create;
} ch_text_room_enter;

class HgServer;
class App{
    public:
        HgChannel Channel;
        int threadNum=0;
        int workSessNum=0;
        HgServer *hgser=nullptr;
        std::unordered_map<std::string,Room *> *rooms=nullptr;
        App(void);
        int CreateRoom(int roomid,SessionC *sess);
};
extern App *hgapp;
#endif

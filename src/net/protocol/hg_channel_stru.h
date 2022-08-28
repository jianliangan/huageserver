//
// Created by ajl on 2021/11/24.
//

#ifndef HUAGERTP_APP_HG_CHANNEL_STRU_H
#define HUAGERTP_APP_HG_CHANNEL_STRU_H

#include <netinet/in.h>
#include "core/hg_buf_comm.h"
#include "../threads/hg_tevent.h"
#include "../threads/hg_channel.h"
//iocp handle
class HgSessionC;
class HgFdEvent;
typedef void(*IocpHandle)(void *ioctx, void *ctx, void *ptr);

//
typedef struct selfCB_s {
    HgChannel *chan;
    even_handle cbhandle;
} selfCB_s;
#define HG_NET_IO_SESS_ERROR -3
#define HG_NET_IO_ERROR -1
#define HG_NET_IO_CLOSED -2

#define HG_NET_IO_UN 0
#define HG_NET_IO_TCP 1
#define HG_NET_IO_UDP 2
inline const char *errorms(int l){
    switch (l) {
        case  HG_NET_IO_CLOSED:
            return "net fd close";
        case  HG_NET_IO_ERROR:
            return "net io error";
        case  HG_NET_IO_SESS_ERROR:
            return "session error";
        case  HG_NET_IO_UN:
            return "unknow";
        case HG_NET_IO_TCP:
            return "tcp";
        case HG_NET_IO_UDP:
            return "udp";
        default:
            return "unknow";
    }
}
typedef struct res_params_s {
    int id;
    int code;
} res_params_s;
//udp packet raw handle params
typedef struct client_packet_recv_pa {
    struct sockaddr_in sa;
    struct hg_Buf_Gen_t *data;
    int size;
    void *ctx;
    selffree_s sfree;
} client_packet_recv_pa;

typedef struct media_frame_chain{
    hg_chain_t hct;
    selffree_s sfree;
    int size;
    uint16_t pts;
    media_frame_chain():hct(),sfree(),size(0),pts(0){}
    void init(){
        this->hct.init();
        this->sfree.init();
        this->size=0;
        this->pts=0;
    }
    void reset(){
        this->hct.reset();
        this->size=0;
    }
}media_frame_chain;

typedef struct client_list_recv_pa {
    media_frame_chain mfc;
    void *ctx;

    void *ev_ptr;
} client_list_recv_pa;
///////////////////////////////////
typedef struct stream_pipe {
    unsigned char *data;
    void *ctx;
} stream_pipe;
//////////////////////////
typedef struct client_packet_send_pa {
    unsigned char *data;
    int size;
    unsigned int time;
    void *ctx;
    HgSessionC *sess;
    selffree_s sfree;
} client_packet_send_pa;
typedef struct client_packet_send_free_pa {
    void *data;
    void *ctx;
} client_packet_send_free_pa;

typedef struct client_frame_send_pa {
    media_frame_chain mfc;
int comptype;
    uint32_t ssrc;
    int pltype;
    int isfirst;
    HgSessionC *sess;
} client_frame_send_pa;



#endif //HUAGERTP_APP_HG_CHANNEL_STRU_H

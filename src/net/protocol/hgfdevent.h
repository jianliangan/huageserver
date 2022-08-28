//
// Created by ajl on 2021/12/24.
//

#ifndef HUAGERTP_APP_HGTCPCONN_H
#define HUAGERTP_APP_HGTCPCONN_H

#include "core/hg_pool.h"
#include "hg_channel_stru.h"
#define MY_NOBLOCK(s) fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
class HgSessionC;
class HgFdEvent {
public:
    myPool *fragCache=nullptr;
    media_frame_chain chainbufsR;

    HgSessionC *sess= nullptr;
    HgFdEvent *fnext= nullptr;
    void *ctx= nullptr;
    IocpHandle write= nullptr;
    IocpHandle read= nullptr;
    int fd=0;

    hg_Buf_Gen_t tcpbuf;
    bool isconnected=false;
    uint8_t status;//8 bit 1代表必须新建cache，0代表自己检查是否需新建，7 bit代表是否有头0标识没有，1标识有
    static int SetFreeFdEv(HgFdEvent **efreechain, HgFdEvent *fdevent);

    static HgFdEvent *GetFreeFdEv(HgFdEvent **efreechain);

    static void fdEvChainInit(HgFdEvent **efreechain, int eNums);
    static void fdEvRecvPtrStream1(HgFdEvent *sess, media_frame_chain **recvMergebuf);

};


#endif //HUAGERTP_APP_HGTCPCONN_H

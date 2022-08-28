//
// Created by ajl <429200247@qq.com> on 2020/12/20.
//contain audiostream vediostream comstream
//

#ifndef ANDROID_IOCP_CLIENT_H
#define ANDROID_IOCP_CLIENT_H



#include "../protocol/rtp_packet.h"
#include "../protocol/core/hg_queue.h"
#include "../protocol/huage_sendstream.h"
#include "../protocol/huage_recvstream.h"
#include "../protocol/hgsession.h"
#include "../protocol/hg_channel_stru.h"
#include "../protocol/hgfdevent.h"
#include "hg_pipe.h"
#include "hg_netcommon.h"
#include <sys/epoll.h>
#define MAX_OPEN_FD 20
class HgPipe;
class HgIocp {
public:
    uint32_t confd=0;
    HgPipe *aPipeClient= nullptr;
    myPool *fragCache= nullptr;
    struct epoll_event tep,ep[20];
    int efd=0;
    HgFdEvent *fdchainfree= nullptr;
    pthread_t runThread;//接收数据
    HgIocp();
    void StartRun();
    static int WritePipe(HgPipe *hp,char *data,int size);
    int AddFdEvt(int fd,int op,void *ctx,int opa);
    static void *Loop(void *ctx);
    ~HgIocp();

};


#endif //ANDROID_IOCP_CLIENT_H

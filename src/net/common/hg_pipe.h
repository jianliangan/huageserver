//
// Created by ajl <429200247@qq.com> on 2020/12/20.
//contain audiostream vediostream comstream
//

#ifndef ANDROID_PROJECT_HGPIPE_H
#define ANDROID_PROJECT_HGPIPE_H
#include "../protocol/rtp_packet.h"
#include "../protocol/core/hg_queue.h"
#include "../protocol/core/tools.h"
#include "../protocol/core/hg_buf.h"
#include "../protocol/hgsession.h"
#include "hg_netcommon.h"

//typedef void (*handleRecv_def)(void *ctx,char*,int);
class HgPipe {
public:

    myPool *fragCache=nullptr;
    hg_chain_node *freen= nullptr;
    hg_chain_t hct;

    int lock=0;
    int pipe_fd[2];
    hg_Buf_Gen_t tcpbuf;
    void *data= nullptr;
    void *ver_recv_ctx= nullptr;
    HgPipe();
    int Handlemsg(int pfd, hg_Buf_Gen_t *tcpb);
    static void PipeRecvmsg(void *ioctx, void *ctx, void *ptr);
    static void PipeSendmsg(void *ioctx, void *ctx, void *ptr);
    static int WritePipe(HgPipe *hp,void *data, int size);
    static int WritePipe0(HgPipe *hp,void *data, int size);

    ~HgPipe();

};

#endif //ANDROID_PROJECT_HGPIPE_H

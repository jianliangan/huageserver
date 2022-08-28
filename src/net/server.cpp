//
// Created by ajl on 2021/11/11.
//

#include "server.h"
#include "../app/session.h"
#include "protocol/hgsession.h"
#include "common/hg_tcpabstract.h"
#include "common/hg_udpabstract.h"
#include "protocol/rtp_packet.h"
#include "threads/hg_worker.h"
#include "common/hg_pipe.h"
#include "protocol/hg_channel_stru.h"
#include "protocol/hgfdevent.h"
HgServer::HgServer() {

    HgTcpabstract *aTcpinstan = new HgTcpabstract();
    HgUdpabstract *aUdpinstan = new HgUdpabstract();
    hnab=new HgNetabstract();
    hnab->aTcpinstan=aTcpinstan;
    hnab->aUdpinstan=aUdpinstan;
    HgSessBucket *hgsessbu=nullptr;
    HgSessionC::sess_cb=SessionC::sessFree;
    int sessnum=1000;
    for (int i = 0; i < WORKERNUM ;
            i++){
        hgworkers[i] = new HgWorker();
        hgsessbu = new HgSessBucket();
        HgSessionC::sessInit(sessnum, &hgsessbu);
        SessionC *sesstmp=SessionC::sessInit(sessnum);

        HgSessionC *hgsesstmp= hgsessbu->sessFreePtr;
        for(int j=0;j<sessnum;j++){
            sesstmp+=1;
            sesstmp->data=hgsesstmp;
            hgsesstmp->data=sesstmp;
            hgsesstmp=hgsesstmp->fnext;
        }
        hgworkers[i]->sess_s = hgsessbu;
    }

}
void HgServer::StartRun(const char *ip, int port) {

    HgUdpabstract *aUdpinstan=hnab->aUdpinstan;

    aUdpinstan->recvCallback = RecvCallback;
    aUdpinstan->verify_auth = Verify_Auth;
    aUdpinstan->asyncLogin = AsyncLogin;
    aUdpinstan->ver_recv_ctx = this;

    HgTcpabstract *aTcpinstan=hnab->aTcpinstan;
    aTcpinstan->recvCallback = RecvCallback;
    aTcpinstan->verify_auth = Verify_Auth;
    aTcpinstan->asyncLogin = AsyncLogin;
    aTcpinstan->ver_recv_ctx = this;



    /***iocp***/
    HgIocp *iocp = new HgIocp();
    HgFdEvent::fdEvChainInit(&iocp->fdchainfree,100000);

    aTcpinstan->iocp = iocp;
    aTcpinstan->Init();
    aTcpinstan->Listen(port+1,ip);


    iocp->StartRun();
    /****iocp2****/
    ///////////////////////
    HgIocp *iocp2 = new HgIocp();
    HgFdEvent::fdEvChainInit(&iocp2->fdchainfree,10);
    aUdpinstan->iocp=iocp2;
    aUdpinstan->Init();
    aUdpinstan->Listen(port,ip);

    iocp2->StartRun();


}

void HgServer::RecvCallback(PLTYPE pltype, void *ctx, media_frame_chain *mfc,
        HgSessionC *sess) {
    HgServer *us = (HgServer *) ctx;
    if (pltype == ENU_PLTYPEAUDIO) {
        if (us->audioctx == nullptr) {
            return;
        }
        us->recvaudioCallback(us->audioctx, mfc, sess);
    } else if (pltype == ENU_PLTYPEVIDEO) {
        if (us->videoctx == nullptr) {
            return;
        }
        us->recvvideoCallback(us->videoctx, mfc, sess);
    } else if (pltype == ENU_PLTYPETEXT) {
        if (us->textctx == nullptr) {
            return;
        }
        us->recvtextCallback(us->textctx, mfc, sess);
    }
}

void HgServer::AsyncLogin(void *ctx,media_frame_chain *mfc, HgSessionC *sess) {
    HgServer *us = (HgServer *) ctx;
    us->asyncLogin(us->textctx, mfc, sess);
}
bool HgServer::Verify_Auth(void *ctx,HgSessionC *sess, int fd) {
    HgServer *us = (HgServer *) ctx;
    return us->verify_auth(us->textctx, sess, fd);
}

//
// Created by ajl on 2022/1/12.
//

#include "hg_netabstract.h"

HgNetabstract::HgNetabstract() {

    isclient = false;
}

int
HgNetabstract::CreateFrameStr(myPool *fragCache, hg_chain_t *hct, uint32_t ssrc,
        int pltype, bool isfirst, int cmdtype, uint16_t time,
        uint8_t *data, int size, int ssize,int comptype) {

    bool first = true;
    int scap = 0;
    hg_Buf_Gen_t *hbgt = nullptr,*hbgt_re = nullptr;
    int tosize = size;
    int tmpsize = 0;
    int start=0;
    int pad=0;
    while (1) {
        if (tosize == 0) {
            break;
        }
        tmpsize = tosize;
        if (tosize > ssize) {
            tmpsize = ssize;

        }

        if (first) {
            start=RTP_PACKET_HEAD_SIZE;
            scap = RTP_PACKET_HEAD_SIZE + tmpsize;
            //if(padding==1){
           //     pad=ADD_PACKET_HEAD1_SIZE;
           // }
        } else {
            start=0;
           // pad=0;
            scap = tmpsize;
        }
        hbgt = AllocMemery_hbgt(fragCache, sizeof(hg_Buf_Gen_t) + scap);

        Hg_PushChainDataR(fragCache, hct, hbgt);
        if (first) {
            hbgt_re=hbgt;
            rtpHeader rh;
            rh.ssrc = ssrc;
            rh.payloadType = pltype;
                rh.comp=comptype;
            rh.first = 0;
            if (isfirst) {
                rh.first = 1;
            }

            rh.timestamp = time;
            rh.seq = 0;//到这里了
            rh.tail = 0;
            rh.cmd = cmdtype;
            rh.payloadType = pltype;

            rtpPacketHead((uint8_t *) hbgt + sizeof(hg_Buf_Gen_t), &rh);
        }

        hbgt->data = (char *) hbgt + sizeof(hg_Buf_Gen_t);
        hbgt->cap = scap;
        hbgt->len = tmpsize;
        hbgt->start = start;
        if(hbgt->start>=hbgt->cap){
            hbgt->start-=hbgt->cap;
        }
        hbgt->end = 0;
        if(data!= nullptr)
            memcpy((char *)hbgt->data+start+pad,data+(size-tosize),tmpsize-pad);
        tosize -= (tmpsize-pad);
        first = false;
    }


    return size+RTP_PACKET_HEAD_SIZE;
}

void HgNetabstract::PreSendData(void *ctx, media_frame_chain *mfc, uint32_t ssrc,
        int pltype, bool isfirst, HgSessionC *sess,
        bool direct, void *pth,int comptype) {
    HgNetabstract *hclnt = (HgNetabstract *) ctx;

    int lens = sizeof(hgtEvent) + sizeof(client_frame_send_pa);
    char hgteventptr[lens];
    hgtEvent *hgtevent = (hgtEvent *) hgteventptr;
    client_frame_send_pa *cfsp = (client_frame_send_pa *) (hgteventptr + sizeof(hgtEvent));
    cfsp->mfc = *mfc;
cfsp->comptype=comptype;


    cfsp->ssrc = ssrc;
    cfsp->pltype = pltype;//;
    cfsp->isfirst = isfirst ? 1 : 0;
    cfsp->sess = sess;
    //ALOGI(0,"111111111 prepre SendDataQ  %ld",sess);
    if (direct) {
        HgNetabstract::SendDataQueue(pth, ctx, cfsp, lens - sizeof(hgtEvent));
    } else {
        hgtevent->handle = HgNetabstract::SendDataQueue;
        hgtevent->i=1;
        hgtevent->psize = lens - sizeof(hgtEvent);
        hgtevent->ctx = ctx;
        HgWorker *hgworker = HgWorker::GetWorker(ssrc);
        hgworker->WriteChanWor(hgteventptr, lens);

    }

}

//client
void HgNetabstract::SendDataQueue(void *pth, void *ctx, void *params, int psize) {
    // ALOGI(0,"111111111 SendDataQ  1");
    client_frame_send_pa *cfsp = (client_frame_send_pa *) params;
    media_frame_chain *mfc1 = &cfsp->mfc;
    int comptype=cfsp->comptype;
    HgNetabstract *hgcl = (HgNetabstract *) ctx;
    HgTcpabstract *aTcpinstan = hgcl->aTcpinstan;
    HgUdpabstract *aUdpinstan = hgcl->aUdpinstan;
    HgWorker *hworker = (HgWorker *) pth;
    HgSessBucket *hgsessbu = (HgSessBucket *) hworker->sess_s;

    if (cfsp->sess == nullptr && hgcl->isclient) {
        HgSessionC *sesstmp = nullptr;

        sesstmp = HgSessionC::ActiveSess(hgsessbu, cfsp->ssrc);

        if (sesstmp == nullptr) {
            hgcl->CaBaChan(HG_NET_IO_UN, HG_NET_IO_SESS_ERROR);
            hgcl->ClearReqFram(mfc1);
            return;
        }
        cfsp->sess = sesstmp;

        HgFdEvent *hfe = aTcpinstan->fdev;
        if (hfe == nullptr) {
            hgcl->CaBaChan(HG_NET_IO_TCP, HG_NET_IO_CLOSED);
            hgcl->ClearReqFram(mfc1);


            return;
        }

        cfsp->sess->tcpfd = hfe;

        hfe->sess = cfsp->sess;

    }

    PLTYPE pltype = (PLTYPE) cfsp->pltype;
    bool isfirst = cfsp->isfirst ? true : false;
    // ALOGI(0,"111111111 SendDataQ  3,1");
    uint16_t time = cfsp->mfc.pts;
    //ALOGI(0,"111111111 SendDataQ  3,11");

    // ALOGI(0,"111111111 SendDataQ  3,12");
    uint32_t ssrc = cfsp->ssrc;
    //ALOGI(0,"111111111 SendDataQ  3,13");
    HgSessionC *sess = (HgSessionC *) cfsp->sess;
    //ALOGI(0,"111111111 SendDataQ  3,14, ssrc %d,sess %ld",ssrc,sess);
    HgFdEvent *fdptr = sess->tcpfd;
    // ALOGI(0,"111111111 SendDataQ  3,2");
    int usetcp = 0;
    char sendbuf[1024];
    CMDTYPE cmdtype;
    if (pltype == ENU_PLTYPETEXT) {
        if (sess->tproto == USETCP) {
            usetcp = 1;
        }
    } else {
        if (sess->avproto == USETCP) {
            usetcp = 1;
        }
    }


    if (usetcp) {
        //这里负责丢帧

        if (aTcpinstan->SendData(pth, pltype, isfirst, mfc1, ssrc, sess, fdptr,comptype) ==
                HG_EP_ERROR) {
            if ((hgetSysTimeMicros() / 1000) - aTcpinstan->lastclotim > 1) {
                aTcpinstan->Connect();
            }
        }
    } else {

        aUdpinstan->SendData(pth, pltype, isfirst, mfc1,ssrc, sess,comptype);
    }
    // ALOGI(0,"111111111 SendDataQ  5");
    if (mfc1->sfree.freehandle != nullptr)
        mfc1->sfree.freehandle(mfc1->sfree.ctx, mfc1->sfree.params);
    // ALOGI(0,"111111111 SendDataQ  6");
}

void HgNetabstract::ClearReqFram(media_frame_chain *mfc) {

    if (mfc->sfree.freehandle != nullptr)
        mfc->sfree.freehandle(mfc->sfree.ctx, mfc->sfree.params);

    hg_chain_node *hcnleft = mfc->hct.left;
    hg_Buf_Gen_t *hbgt = nullptr;

    while (hcnleft != nullptr) {
        hbgt = (hg_Buf_Gen_t *) hcnleft->data;
        selffree_s *ss = &hbgt->sfree;
        if (ss->freehandle)
            ss->freehandle(ss->ctx, ss->params);
        hcnleft = hcnleft->next;
    }

}

void HgNetabstract::CaBaChan(int id, int code) {
    int lens = sizeof(hgtEvent) + sizeof(res_params_s);
    char tmp[lens];
    res_params_s *sps = (res_params_s *) (tmp + sizeof(hgtEvent));
    sps->id = id;
    sps->code = code;
    hgtEvent *hgtevent = (hgtEvent *) tmp;
    hgtevent->handle = selfcb.cbhandle;
    hgtevent->i=2;
    hgtevent->ctx = this;
    hgtevent->psize = sizeof(res_params_s);
    selfcb.chan->WriteChan(tmp, lens, 1);
}

hg_Buf_Gen_t *HgNetabstract::AllocMemery_hbgt(myPool *fragCache, int size) {
    hg_Buf_Gen_t *tmp = (hg_Buf_Gen_t *) myPoolAlloc(fragCache, size);
    tmp->init();
    tmp->cap = size - sizeof(hg_Buf_Gen_t);
    tmp->data = (char *) tmp + sizeof(hg_Buf_Gen_t);

    return tmp;
}

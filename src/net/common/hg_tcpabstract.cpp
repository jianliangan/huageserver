//
// Created by ajl on 2022/1/11.
//

#include "hg_tcpabstract.h"
#include "../../app/app.h"
#include "../threads/hg_worker.h"
#include "hg_netabstract.h"
#include "../protocol/hgfdevent.h"

HgTcpabstract::HgTcpabstract() {
    finished = false;

    isclient = false;
    lastclotim = 0;
    verify_auth = nullptr;
    fdev = nullptr;
    iocp = nullptr;
    // 使用socket()，生成套接字文件描述符；
}

int HgTcpabstract::GetSocket() {
    int confd = 0;
    if ((confd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ALOGE(0, "tcp client socket error %s", strerror(errno));
    }

    return confd;
}

int HgTcpabstract::Connect() {
    if (!isclient) {
        return -1;
    }
    int confd = 0;
    confd = GetSocket();
    if (connect(confd, (struct sockaddr *) &serveraddr, sizeof(struct sockaddr)) !=
            0) {
        close(confd);
        return errno;
    }
    MY_NOBLOCK(confd);
    fdev = InitEvent(iocp, confd);
    return 0;
}

HgFdEvent *HgTcpabstract::InitEvent(HgIocp *iocp, int confd) {
    HgFdEvent *fdevtcp = HgFdEvent::GetFreeFdEv(&iocp->fdchainfree);
    fdevtcp->ctx = this;
    fdevtcp->read = HgTcpabstract::TCPRecvmsg;
    fdevtcp->write = HgTcpabstract::TCPSendmsg;
    fdevtcp->fd = confd;
    fdevtcp->fragCache = myPoolCreate(1024 * 2);
    fdevtcp->tcpbuf.data = myPoolAlloc(fdevtcp->fragCache, RECV_BUFFER_LEN);
    fdevtcp->tcpbuf.start = 0;
    fdevtcp->tcpbuf.end = 0;
    fdevtcp->tcpbuf.cap = RECV_BUFFER_LEN;
    fdevtcp->tcpbuf.len = 0;
    iocp->AddFdEvt(confd, EPOLL_CTL_ADD, fdevtcp,0);
    fdevtcp->isconnected = true;
    return fdevtcp;
}

HgFdEvent *HgTcpabstract::InitLisEvent(HgIocp *iocp, int confd) {
    HgFdEvent *fdevtcp = HgFdEvent::GetFreeFdEv(&iocp->fdchainfree);
    fdevtcp->ctx = this;
    fdevtcp->read = HgTcpabstract::TCPAccept;
    fdevtcp->write = nullptr;
    fdevtcp->fd = confd;
    iocp->AddFdEvt(confd, EPOLL_CTL_ADD, fdevtcp,1);
    fdevtcp->isconnected = true;
    return fdevtcp;
}

void HgTcpabstract::SetServer(const char *serverip, uint32_t port) {
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(serverip);
    serveraddr.sin_port = htons(port);
    bzero(&(serveraddr.sin_zero), 8);

}

HgTcpabstract::~HgTcpabstract() {

}

void HgTcpabstract::TCPAccept(void *ioctx, void *ctx, void *ptr) {
    HgIocp *iocp = (HgIocp *) ioctx;
    HgFdEvent *fdev = (HgFdEvent *) ptr;
    struct sockaddr_in clientAddr;
    socklen_t socklen;
    HgTcpabstract *htb = (HgTcpabstract *) ctx;
    int connfd = accept(fdev->fd, (struct sockaddr *) &clientAddr, &socklen);
    if (connfd == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    MY_NOBLOCK(connfd);
    htb->InitEvent(htb->iocp, connfd);
}

void HgTcpabstract::TCPRecvmsg(void *ioctx, void *ctx, void *ptr) {
    int n = 0;
    bool hashead;
    int framerest = 0, bodysize = 0;
    HgFdEvent *hfde = (HgFdEvent *) ptr;
    int pfd = hfde->fd;

    myPool *mpool = hfde->fragCache;
    HgTcpabstract *context = (HgTcpabstract *) ctx;
    hg_Buf_Gen_t *tcpb = &hfde->tcpbuf;

    uint8_t *recvLine = nullptr;
    PLTYPE pltype = ENU_PLTYPENONE;
    uint32_t ssrc = 0;
    int seq = 0;
    CMDTYPE cmdtype;
    int error = 0;
    uint8_t tmp[16];
    uint8_t isfirst;
    int canwrsize = 0;
    int canresize = 0;
    int needsize = 0;
    int len=0;
    media_frame_chain *hctrecvframe = nullptr;
    while (1) {
        n = 0;


        if (tcpb->start > tcpb->end) {
            len=tcpb->start - tcpb->end;
            n = recv(pfd, (char *) tcpb->data + tcpb->end, len, 0);
            // ALOGI(0,"d-%d--0000000ddddddid %s,",n,(char *) tcpb->data + tcpb->end+16);
            if (n >= 0) {
                tcpb->end = (tcpb->end + n);
                if(tcpb->end>=tcpb->cap){
                    tcpb->end-=tcpb->cap;
                }
                tcpb->len += n;
            }
        } else {
            len=tcpb->cap - tcpb->end;
            n = recv(pfd, (char *) tcpb->data + tcpb->end, len, 0);
            // ALOGI(0,"d+%d++0000000ddddddid %s,",n,(char *) tcpb->data + tcpb->end+16);
            if (n >= 0) {
                tcpb->end = (tcpb->end + n);
                if(tcpb->end>=tcpb->cap){
                    tcpb->end-=tcpb->cap;
                }
                tcpb->len += n;
            }

        }

        if (n <= 0) {
            error = errno;
            if (error == EAGAIN || error == EINTR) {
                break;
            } else {
                //io错误，去重连
                break;
            }
        }
        hashead=(hfde->status>>1)&0x01;
        ///检测可用的数据
        if (!hashead) {
            if (Hg_Buf_ReadN(tmp, tcpb, TCP_PACKET_HEAD_SIZE + RTP_PACKET_HEAD_SIZE,
                        TCP_PACKET_HEAD_SIZE) == 0) {
                bodysize = *((int *) tmp);
                if (bodysize <= 0 || bodysize > MAXBIGFRAMELEN) {
                    return;
                }
                framerest = bodysize;
                hashead = true;
                hfde->status=hfde->status|0x02;
                int pltype = 0;
                pltype = pktPLoadType(tmp + TCP_PACKET_HEAD_SIZE, RTP_PACKET_HEAD_SIZE);
                HgFdEvent::fdEvRecvPtrStream1(hfde, &hctrecvframe);
                hctrecvframe->hct.left= nullptr;
                hctrecvframe->hct.right= nullptr;
                hctrecvframe->size=0;
            } else {
                continue;
            }
        }
        if (hashead && hctrecvframe != nullptr) {
            int nextframe;
            while (true) {
                nextframe=((hfde->status&0x01)==0?1:0);
                recvLine = (uint8_t *) Hg_GetSingleOrIdleBuf(mpool, &hctrecvframe->hct,
                        nullptr, &canwrsize,
                        RECV_MAX_BUF_SIZE, nextframe, 0);
                if (nextframe == 1) {
                    hg_chain_node *hcn = hctrecvframe->hct.left;
                    hg_Buf_Gen_t *hbgt = (hg_Buf_Gen_t *) hcn->data;
                }
                nextframe=0;
                hfde->status=hfde->status|0x01;
                canresize = tcpb->len;
                needsize = canresize > canwrsize ? canwrsize : canresize;
                needsize = framerest > needsize ? needsize : framerest;

                if (Hg_Buf_ReadN(recvLine, tcpb, needsize, needsize) != -1) {
                    framerest -= needsize;
                    hg_chain_node *hcn = hctrecvframe->hct.right;
                    hg_Buf_Gen_t *hbgt = (hg_Buf_Gen_t *) hcn->data;
                    hbgt->len+=needsize;
                    hbgt->end+=needsize;
                    if(hbgt->end>=hbgt->cap){
                        hbgt->end -=hbgt->cap;
                    }
                } else {
                    break;//继续去拿
                }

                if (framerest == 0) {

                    //hctrecvframe->sfree.ctx = ptr;
                    hctrecvframe->sfree.params = nullptr;
                    hctrecvframe->sfree.freehandle = nullptr;
                    hctrecvframe->size = bodysize;


                    char jsonblock[4096]={0};
                    int total=0;
                    hg_Buf_Gen_t *hbgt=nullptr;
                    hg_chain_node *hcn=hctrecvframe->hct.left;
                    hbgt=(hg_Buf_Gen_t *)hcn->data;
                    hbgt->start=RTP_PACKET_HEAD_SIZE;

                    if(hbgt->end>=hbgt->cap){
                        hbgt->end-=hbgt->cap;
                    }

                    hbgt->len=hbgt->len-RTP_PACKET_HEAD_SIZE;
                    while (hcn!=nullptr) {
                        hbgt=(hg_Buf_Gen_t *)hcn->data;
                        memcpy((char *)jsonblock+total,(char *)hbgt->data+RTP_PACKET_HEAD_SIZE,hbgt->len);
                        total+=hbgt->len;
                        hcn=hcn->next;
                    }





                    context->RecvData(ptr, hctrecvframe);
                    //hctrecvframe->hct.right = nullptr;
                    //hctrecvframe->hct.left = nullptr;
                    //hctrecvframe->size = 0;

                    hashead = false;
                    nextframe=1;
                    hfde->status=hfde->status&0xfc;
                    break;
                } else if (tcpb->len > 0) {
                    continue;
                }
            }
            //////////////////
        }
        if(n<len){
            return;
        }
    }
}

void HgTcpabstract::TCPSendmsg(void *ioctx, void *ctx, void *ptr) {
    HgFdEvent *hfe = (HgFdEvent *) ptr;
    HgSessionC *sess = hfe->sess;
    PreSendDataChain(ioctx, ctx, hfe, sess->ssrc);
}

void HgTcpabstract::SendChain(void *pth, void *ctx, void *params, int psize) {

    twoaddr *paramptr = (twoaddr *) params;
    HgIocp *ioctx = (HgIocp *) paramptr->param1;
    HgTcpabstract *htcl = (HgTcpabstract *) ctx;
    int efd = ioctx->efd;
    HgFdEvent *hfe = (HgFdEvent *) paramptr->param2;
    HgSessionC *hgsess = hfe->sess;
    hg_chain_t *hct = nullptr;
    HgSessionC::sessSendPtrStreamTCP(hgsess, &hct);
    if (!hfe->isconnected) {
        HgTcpabstract::ClearSndChain(hct, hgsess, true);
        return;
    }


    if (htcl->PreSendArr(efd, hct, hgsess, hfe) == HG_EP_ERROR) {
        HgTcpabstract::ClearSndChain(hct, hgsess, true);
    }
    HgTcpabstract::ClearSndChain(hct, hgsess, false);


}

void HgTcpabstract::Init() {
    if (isclient) {
        int error = Connect();
    }

}

void HgTcpabstract::Listen(int port, const char *ip) {
    int confd = 0;
    confd = GetSocket();

    struct sockaddr_in serverAddr, clientAddr;
    int serverIp = 0, err = 0;
    struct epoll_event tep;
    // 通过struct sockaddr_in 结构设置服务器地址和监听端口；
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, (void *) &serverIp);
    serverAddr.sin_addr.s_addr = serverIp;

    //socklen_t reelen = sizeof(uint32_t);
    // int32_t ree;
    // int32_t rees;
    // err = getsockopt(confd, SOL_SOCKET, SO_RCVBUF, (void *) &ree, &reelen);
    // if (err == -1) {
    //     ALOGI(0, "err getsockopt");
    // }
    //  err = getsockopt(confd, SOL_SOCKET, SO_SNDBUF, (void *) &rees, &reelen);
    //  if (err == -1) {
    //      ALOGI(0, "err getsockopt");
    //  }
    //ALOGI(0, "start recv %s:%d,rcvbuf:%d,sendbuf:%d", ip, port, ree, rees);
    int ret = 0;
    int one = 1;
    if (setsockopt(confd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
        close(confd);
        return;
    }

    ret = bind(confd, (const struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (ret == -1) {
        ALOGI(0, "bind error %s", strerror(errno));

        return;
    }
    listen(confd, 20);
    InitLisEvent(iocp, confd);
}

int HgTcpabstract::PreSendArr(int efd, hg_chain_t *hct, HgSessionC *hgsess, HgFdEvent *hfe) {
    int total = 4;
    int hasnum = 0;
    int ret = 0;
    hg_chain_node *hcnleft = nullptr;
    hg_Buf_Gen_t *hbgtleft = nullptr;
    hg_Buf_Gen_t *hbgttmp[total];

    for (int i = 0; i < total; i++) {
        if (hcnleft == nullptr) {
            hcnleft = hct->left;
        } else {
            hcnleft = hcnleft->next;
        }
        if (hcnleft == nullptr)
            break;
        hasnum = i + 1;
        hbgttmp[i] = (hg_Buf_Gen_t *) hcnleft->data;
    }
    ret = SendDatapre(efd, hct, hbgttmp, hasnum, hgsess, hfe);

    return ret;
}

void HgTcpabstract::PreSendDataChain(void *ioctx, void *ctx, HgFdEvent *hfe, uint32_t ssrc) {
    twoaddr addrs = {ioctx, hfe};

    int lens = sizeof(hgtEvent) + sizeof(void *);
    char hgteventptr[lens];
    hgtEvent *hgtevent = (hgtEvent *) hgteventptr;

    hgtevent->handle = HgTcpabstract::SendChain;
    hgtevent->i=3;
    hgtevent->psize = lens - sizeof(hgtEvent);
    hgtevent->ctx = ctx;
    memcpy((char *) hgteventptr + sizeof(hgtEvent), &addrs, sizeof(twoaddr));
    HgWorker *hgworker = HgWorker::GetWorker(ssrc);
    hgworker->WriteChanWor(hgteventptr, lens);

}

void HgTcpabstract::THPOOLRecv(void *pth, void *ctx, void *params, int size) {

    client_list_recv_pa *clrp = (client_list_recv_pa *) params;

    media_frame_chain *mfc = &clrp->mfc;
    hg_chain_node *hcn = mfc->hct.left;
    HgFdEvent *ptr = (HgFdEvent *) clrp->ev_ptr;
    HgTcpabstract *context = (HgTcpabstract *) clrp->ctx;

    hg_Buf_Gen_t *hbgt = (hg_Buf_Gen_t *) hcn->data;
    uint8_t *rtpdata = (uint8_t *) hbgt->data;

    HgWorker *hworker = (HgWorker *) pth;

    HgSessBucket *hgsessbu = (HgSessBucket *) hworker->sess_s;
    int udpLength = mfc->size;
    PLTYPE pltype = pktPLoadType(rtpdata, RTP_PACKET_HEAD_SIZE);
    int isfirst = pktFirst(rtpdata, RTP_PACKET_HEAD_SIZE);
    int ssrc = pktSsrc(rtpdata, RTP_PACKET_HEAD_SIZE);
    HgSessionC *sessPtr= ptr->sess;
    if(sessPtr== nullptr) {
        sessPtr = HgSessionC::ActiveSess(hgsessbu, ssrc);
        ptr->sess = sessPtr;
        sessPtr->tcpfd = ptr;
    }


    if(isfirst) {
        if (context->asyncLogin != nullptr) {
            context->asyncLogin(context->ver_recv_ctx, mfc,sessPtr);
        }
        return;
    }
    if (context->verify_auth(context->ver_recv_ctx,sessPtr, 0) == false) {
        hg_chain_node *hcnR = mfc->hct.right;
        hg_Buf_Gen_t *hbgtR = (hg_Buf_Gen_t *) hcnR->data;
        if(hbgtR->sfree.freehandle!= nullptr)
            hbgtR->sfree.freehandle(hbgtR->sfree.ctx, hbgtR->sfree.params);
        return;
    }

    context->recvCallback(pltype, context->ver_recv_ctx, mfc, sessPtr);
}

void HgTcpabstract::ClearSndChain(hg_chain_t *hct, HgSessionC *hgsess, bool all) {
    hg_chain_node *hcnleft = hct->left;
    hg_Buf_Gen_t *hbgtleft = nullptr;
    if (all) {
        while (hcnleft != nullptr) {
            hbgtleft = (hg_Buf_Gen_t *) hcnleft->data;
            Hg_ChainDelNode(hgsess->fragCache, hcnleft);
            if (hbgtleft->sfree.freehandle != nullptr)
                hbgtleft->sfree.freehandle(hbgtleft->sfree.ctx, hbgtleft->sfree.params);
            hct->left=hcnleft = hcnleft->next;
        }
    } else {
        while (hcnleft != nullptr) {
            hbgtleft = (hg_Buf_Gen_t *) hcnleft->data;
            if (hbgtleft->len == 0) {

                Hg_ChainDelNode(hgsess->fragCache, hcnleft);
                if (hbgtleft->sfree.freehandle != nullptr)
                    hbgtleft->sfree.freehandle(hbgtleft->sfree.ctx, hbgtleft->sfree.params);
            }
            hct->left=hcnleft = hcnleft->next;
        }
    }
    if (hct->left == NULL || hct->right == NULL) {
        hct->left = hct->right = NULL;
    }
}

int HgTcpabstract::SendDatapre(int efd, hg_chain_t *hct, hg_Buf_Gen_t **hbgtarr, int num,
        HgSessionC *hgsess,
        HgFdEvent *hfe) {

    sendArr sendarr[num];
    for (int i = 0; i < num; i++) {
        hg_Buf_Gen_t *hbgtleft = *(hbgtarr + i);
        sendarr[i].data = (char *) hbgtleft->data + hbgtleft->start;
        sendarr[i].size = hbgtleft->len;
    }


    int n = 0, ret = 0, fd = 0;
    fd = hgsess->tcpfd->fd;
    n = SendData0(fd, sendarr, num);
    if (n >= 0) {
        int rest = n;
        for (int i = 0; i < num; i++) {
            hg_Buf_Gen_t *hbgtleft = *(hbgtarr + i);
            if (hbgtleft->len <= rest) {
                hbgtleft->len = 0;
                hbgtleft->start += n;

                if(hbgtleft->start>=hbgtleft->cap){
                    hbgtleft->start-=hbgtleft->cap;
                }

                rest -= hbgtleft->len;
            } else {
                hbgtleft->len -= rest;
                hbgtleft->start += rest;
                if(hbgtleft->start>=hbgtleft->cap){
                    hbgtleft->start-=hbgtleft->cap;
                }
                rest = 0;
            }
            if (rest == 0) {
                break;
            }
        }


    } else if (n == HG_EP_AGAIN) {
        /*struct epoll_event tep;
          tep.events = EPOLLIN | EPOLLOUT | EPOLLET;
          tep.data.ptr = hfe;
          ret = epoll_ctl(efd, EPOLL_CTL_MOD, hfe->fd, &tep);*/
        ret = HG_EP_AGAIN;
        return ret;
    } else {
        epoll_ctl(efd, EPOLL_CTL_DEL, hfe->fd, NULL);
        close(hfe->fd);
        hfe->sess->tcpfd= nullptr;
        hfe->sess= nullptr;

        HgFdEvent::SetFreeFdEv(&iocp->fdchainfree, hfe);
        this->lastclotim = hgetSysTimeMicros() / 1000;
        this->fdev = nullptr;
        return HG_EP_ERROR;
    }
    return n;
}

int HgTcpabstract::SendData(void *pth, PLTYPE pltype,bool isfirst, media_frame_chain *mfcsour,
        uint32_t ssrc, HgSessionC *hgsess, HgFdEvent *hfe,int comptype) {

    if (!hfe->isconnected) {
        return HG_EP_ERROR;
    }

uint16_t time=mfcsour->pts;
    HgIocp *hgio = (HgIocp *) (this->iocp);
    int efd = hgio->efd;

    hg_chain_t *sesshct = nullptr;

    int size = mfcsour->size;

    //pktSetCmd((uint8_t *) hbgt->data, (int) RTP_CMD_TCP);
    //pktSetSeq((uint8_t *) hbgt->data, 0);

    uint16_t *seq = nullptr;
    sendArr sendarr[2];
    HgSessionC::sessSendPtrStreamTCP(hgsess, &sesshct);

    HgSessionC::sessSendPtrStreamSeq(hgsess, pltype, &seq);

    int ret = 0;

    ///////////
    hg_Buf_Gen_t *head = CreateBuffer(pltype, isfirst, time,
            hgsess, size,comptype);
    //////////////////

    //if (hcnleft != nullptr) {
    PushFramCach(mfcsour, hgsess, sesshct, head);
    if (PreSendArr(efd, sesshct, hgsess, hfe) == HG_EP_ERROR) {
        HgTcpabstract::ClearSndChain(sesshct, hgsess, true);
        ret = HG_EP_ERROR;
    } else {
        HgTcpabstract::ClearSndChain(sesshct, hgsess, false);
    }
    ////////////
    return ret;
}

hg_Buf_Gen_t *HgTcpabstract::CreateBuffer(uint8_t pltype, bool isfirst, uint16_t time,
        HgSessionC *hgsess, int size,int comptype) {
    rtpHeader rh;
    rh.ssrc = hgsess->ssrc;
    rh.payloadType = (uint8_t) pltype;
rh.comp=comptype;
    rh.first = 0;
    if (isfirst) {
        rh.first = 1;
    }

    rh.cmd = RTP_CMD_TCP;
    rh.timestamp = time;
    rh.seq = 0;
    rh.tail = 0;

    hg_Buf_Gen_t *rhtmp = (hg_Buf_Gen_t *) myPoolAlloc(hgsess->fragCache,
            sizeof(hg_Buf_Gen_t) + RTP_PACKET_HEAD_SIZE +
            TCP_PACKET_HEAD_SIZE);
    rhtmp->init();


    int *tmphead = (int *) ((char *) rhtmp + sizeof(hg_Buf_Gen_t));
    *tmphead = size;
    rtpPacketHead((unsigned char *) rhtmp + sizeof(hg_Buf_Gen_t) + TCP_PACKET_HEAD_SIZE, &rh);
    rhtmp->data = (char *) rhtmp + sizeof(hg_Buf_Gen_t);
    rhtmp->start = 0;
    rhtmp->len = RTP_PACKET_HEAD_SIZE + TCP_PACKET_HEAD_SIZE;
    rhtmp->end = 0;
    rhtmp->cap = RTP_PACKET_HEAD_SIZE + TCP_PACKET_HEAD_SIZE;
    rhtmp->sfree.ctx = hgsess;
    rhtmp->sfree.params = rhtmp;
    rhtmp->sfree.freehandle = HgTcpabstract::DestroyHeads;
    return rhtmp;
}

void
HgTcpabstract::PushFramCach(media_frame_chain *mfcsour, HgSessionC *hgsess, hg_chain_t *sesshct,
        hg_Buf_Gen_t *head) {
    hg_chain_node *hcnlefttmp = mfcsour->hct.left;
    hg_Buf_Gen_t *hbgttmp = nullptr;
    Hg_PushChainDataR(hgsess->fragCache, sesshct, head);
    while (hcnlefttmp != nullptr) {
        hbgttmp = (hg_Buf_Gen_t *) hcnlefttmp->data;
        Hg_PushChainDataR(hgsess->fragCache, sesshct, hbgttmp);
        hcnlefttmp = hcnlefttmp->next;
    }
}

///////////////////////
int
HgTcpabstract::RecvData(void *ptr, media_frame_chain *mfc) {
    uint8_t *rtpdata = nullptr;
    if (mfc != nullptr) {
        hg_Buf_Gen_t *hbgt = nullptr;
        hg_chain_node *hcn = (hg_chain_node *) mfc->hct.left;
        hbgt = (hg_Buf_Gen_t *) hcn->data;

        hg_chain_node *hcnR = (hg_chain_node *) mfc->hct.right;
        hg_Buf_Gen_t *hbgtR;
        hbgtR = (hg_Buf_Gen_t *) hcnR->data;
        hbgtR->freenum = 0;
        hbgtR->sfree.ctx = ptr;
        hbgtR->sfree.params = mfc->hct.left;
        hbgtR->sfree.freehandle = HgTcpabstract::WritePipeFree;
        rtpdata = (uint8_t *) hbgt->data;

    }

    uint32_t ssrc = pktSsrc(rtpdata, mfc->size);

    int lens = sizeof(hgtEvent) + sizeof(client_list_recv_pa);
    char hgteventptr[lens];
    hgtEvent *hgtevent = (hgtEvent *) hgteventptr;
    client_list_recv_pa *clrp = (client_list_recv_pa *) (hgteventptr + sizeof(hgtEvent));

    clrp->mfc = *mfc;
    clrp->ctx = this;
    clrp->ev_ptr = ptr;

    hgtevent->handle = HgTcpabstract::THPOOLRecv;
    hgtevent->i=4;
    hgtevent->ctx = this;
    hgtevent->psize = sizeof(client_list_recv_pa);

    HgWorker *hgworker = HgWorker::GetWorker(ssrc);
    hgworker->WriteChanWor(hgteventptr, lens);

    return 0;
}

int HgTcpabstract::SendData0(int confd, sendArr *sendarr, int size) {

    struct msghdr snd_msg;
    struct iovec snd_iov[size];

    for (int i = 0; i < size; i++) {
        snd_iov[i].iov_base = sendarr[i].data;
        snd_iov[i].iov_len = sendarr[i].size;
    }


    snd_msg.msg_name = nullptr; // Socket is connected
    snd_msg.msg_namelen = 0;
    snd_msg.msg_iov = snd_iov;
    snd_msg.msg_iovlen = size;
    snd_msg.msg_control = 0;
    snd_msg.msg_controllen = 0;

    int n = 0;
    int error = 0;
    n = sendmsg(confd, &snd_msg, 0);

    //////////
    if (n >= 0) {
        return n;
    }
    error = errno;
    if (error == EAGAIN || error == EINTR) {
        return HG_EP_AGAIN;
        //这里需要延迟发送，nginx是放到延时里了  src/os/unix/ngx_send.c 44行  以及src/http/ngx_http_upstream.c 2594行
    } else {
        return HG_EP_ERROR;
    }
}

void HgTcpabstract::WritePipeFree(void *ctx, void *data) {
    HgFdEvent *hfe = (HgFdEvent *) ctx;
    HgTcpabstract *hta = (HgTcpabstract *) hfe->ctx;
    HgIocp *hicp = (HgIocp *) hta->iocp;
    HgPipe *hp = hicp->aPipeClient;

    hg_chain_node *hcn = (hg_chain_node *) data;

    int lens = sizeof(hgtEvent) + sizeof(void *) * 2;
    char tmp[lens];
    twoaddr params = {hfe, hcn};
    hgtEvent *hgtevent = (hgtEvent *) tmp;

    hgtevent->handle = HgTcpabstract::FreeMemery_local;
    hgtevent->i=5;
    hgtevent->ctx = hp;
    hgtevent->psize = sizeof(void *) * 2;

    memcpy((char *) tmp + sizeof(hgtEvent), &params, sizeof(params));
    HgPipe::WritePipe(hp, tmp, lens);
}


void HgTcpabstract::FreeMemery_local(void *pth, void *ctx, void *params, int psize) {
    twoaddr *twoparams = (twoaddr *) params;

    HgFdEvent *hfe = (HgFdEvent *) twoparams->param1;
    hg_chain_node *hcn = (hg_chain_node *) twoparams->param2;
    if (hcn->data != nullptr) {
        hg_Buf_Gen_t *hbgt = (hg_Buf_Gen_t *) hcn->data;
        hbgt->freenum--;
        if (hbgt->freenum <= 0) {
            while (hcn != nullptr) {
                myPoolFree(hfe->fragCache, (uint8_t *) hcn->data);
                myPoolFree(hfe->fragCache, (uint8_t *) hcn);
                hcn = hcn->next;
            }
        }

    }


}

void HgTcpabstract::DestroyHeads(void *ctx, void *data) {
    HgSessionC *hgsess = (HgSessionC *) ctx;
    myPoolFree(hgsess->fragCache, (uint8_t *) data);
}

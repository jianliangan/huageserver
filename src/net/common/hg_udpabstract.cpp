//
// Created by ajl on 2022/1/11.
//

#include "hg_udpabstract.h"
#include "../../app/app.h"
#include "../threads/hg_worker.h"
#include "hg_netabstract.h"

HgUdpabstract::HgUdpabstract() {
    finished = false;
    maxmtu = MAX_RTP_PACKET_SIZE;
    haslog = false;
    uint32_t recvbuff = 8 * 1024 * 1024;
    // 使用socket()，生成套接字文件描述符；
    isclient = false;

}

void HgUdpabstract::Init() {
    uint32_t err=0;

    HuageRecvStream::udpinstan = this;
    recvth = new HuageRecvStream();
    recvth->recvCallback = recvCallback;
    recvth->directSendMsg = DirectSendMsg;
    recvth->verify_auth = verify_auth;
    recvth->allowothers = true;
    recvth->ctx = this;
    recvth->asyncLogin = asyncLogin;
    recvth->ver_recv_ctx = ver_recv_ctx;


    sendth = new HuageSendStream();
    sendth->sendCallback = SendCallback;
    sendth->ctx = this;

    finished = true;
    if (isclient)
        Connect();
}

void HgUdpabstract::SetServer(const char *serverip, uint32_t port) {
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(serverip);
    serveraddr.sin_port = htons(port);
}

int HgUdpabstract::GetSocket() {
    int confd=0;
    int recvbuff=8*1024*1024;
    if ((confd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        ALOGE(0, "rtp client socket error %s", strerror(errno));
        finished = false;
    } else
        finished = true;
    if (setsockopt(confd, SOL_SOCKET, SO_RCVBUF, (const uint8_t *) &recvbuff, sizeof(uint32_t)) ==
            -1) {
        ALOGI(1, "errorerrorerrorerrorerrorerrorerrorerror%u\n");
    }
    if (setsockopt(confd, SOL_SOCKET, SO_SNDBUF, (const uint8_t *) &recvbuff, sizeof(uint32_t)) ==
            -1) {
        ALOGI(1, "errorerrorerrorerrorerrorerrorerrorerror%u\n");
    }
    socklen_t reelen = sizeof(uint32_t);
    uint32_t ree = 0;
    int err = getsockopt(confd, SOL_SOCKET, SO_SNDBUF, (uint8_t * ) & ree, &reelen);
    if (err != 0) {
        ALOGI(1, "error getsockopt \n");
    }
    return confd;
}

void HgUdpabstract::Connect() {
    int confd0=0;
    confd0 = GetSocket();
    MY_NOBLOCK(confd0);
    InitEvent(iocp, confd0);
    confd = confd0;
}

HgFdEvent *HgUdpabstract::InitEvent(HgIocp *iocp, int confd) {
    HgFdEvent *fdevudp = HgFdEvent::GetFreeFdEv(&iocp->fdchainfree);
    fdevudp->ctx = this;
    fdevudp->read = HgUdpabstract::UDPRecvmsg;
    fdevudp->write = nullptr;
    fdevudp->fd = confd;
    iocp->AddFdEvt(confd, EPOLL_CTL_ADD, fdevudp,1);
    return fdevudp;
}

HgUdpabstract::~HgUdpabstract() {
    close(confd);
}

void HgUdpabstract::Listen(int port, const char *ip) {
    int confd0=0;
    confd0 = GetSocket();
    MY_NOBLOCK(confd0);
    int one=1;
    if (setsockopt(confd0, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
        close(confd0);
        return ;
    }
    struct sockaddr_in serveraddr;
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(ip);
    serveraddr.sin_port = htons(port);

    if (bind(confd0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
        ALOGI(0, "bind error %s", strerror(errno));

        return;
    }
    InitEvent(iocp, confd0);
    confd = confd0;
}

int
HgUdpabstract::SendData(void *pth, PLTYPE pltype, bool isfirst, media_frame_chain *mfc,
                uint32_t ssrc,
        HgSessionC *sess,int comptype) {
/*
    hg_chain_node *hcntmp=mfc->hct.left;
    hg_Buf_Gen_t *hbgttmp= nullptr;
    FILE *f2 = fopen(APP_ROOT_"send0.data", "a+");
    while(hcntmp!= nullptr){
        hbgttmp=(hg_Buf_Gen_t *)hcntmp->data;
        fwrite((char *)hbgttmp->data + hbgttmp->start, 1, hbgttmp->len, f2);
        hcntmp=hcntmp->next;
    }
    fclose(f2);
*/

    return sendth->SendData(pth, pltype, isfirst, mfc, ssrc, sess,comptype);
}

void HgUdpabstract::UDPRecvmsg(void *ioctx, void *ctx, void *ptr) {
    HgFdEvent *hfde = (HgFdEvent *) ptr;
    int fd = hfde->fd;
    int recvLength = 0;
    HgUdpabstract *context = (HgUdpabstract *) ctx;

    uint32_t bufferlen = MAX_RTP_PACKET_SIZE;
    uint8_t recvLine[bufferlen];
    struct sockaddr_in clientAddr;
    bool isdrop;
    uint32_t ret = 0;
    socklen_t len = sizeof(struct sockaddr_in);
    int error;

    PLTYPE type = ENU_PLTYPENONE;

    uint32_t ssrc = 0;
    int seq=0;
    CMDTYPE cmdtype;
    //FILE *f2 = fopen("/data/data/org.libhuagertp.app/1.data", "a");
    while (true) {
        isdrop = false;
        recvLength = recvfrom(fd, recvLine, sizeof(recvLine), 0,
                (struct sockaddr *) &clientAddr, &len);
        if (recvLength <= 0) {
            //error = errno;
            //if (error == EAGAIN || error == EINTR) {
            return;
            //}else{
            //error
            //}

        }
        int abc=pktFlag(recvLine,13);

        if(recvLength>=RTP_PACKET_HEAD_SIZE) {
            char contents[512] = {0};
            int seq = pktSeq(recvLine, 13);
            int cmd = pktCmd(recvLine, 13);
            int time=pktTimestamp(recvLine, 13);
            int flag=pktFlag(recvLine,13);
            int comptype=pktComp(recvLine,13);
          //  if (cmd <= RTP_CMD_POST) {
            sprintf(contents, "seq %d cmd %d time %d flag %d comp %d\n", seq,cmd,time,flag,comptype);
                FILE *f2 = fopen(APP_ROOT_"recv0.data", "a+");
                fwrite(contents, 1, strlen(contents), f2);
                //fwrite(recvLine + RTP_PACKET_HEAD_SIZE, 1, recvLength - RTP_PACKET_HEAD_SIZE, f2);
                fclose(f2);

        }
        type = pktPLoadType(recvLine, recvLength);
        ssrc = pktSsrc(recvLine, recvLength);
        seq = pktSeq(recvLine, recvLength);
        cmdtype = (CMDTYPE) pktCmd(recvLine, recvLength);

        //ALOGI(0,"33333333333333333333333333 type %d  seq %d cmdtype %d flag %d",
        //      type,seq,cmdtype,ADEC_HEADER_FLAG(*(recvLine + 1)));

        if (type != ENU_PLTYPEVIDEO && type != ENU_PLTYPEAUDIO && type != ENU_PLTYPETEXT) {
            continue;
        }
        {
            char contents[512] = {0};
            int seq = pktSeq(recvLine, 13);
            int cmd = pktCmd(recvLine, 13);
            int time=pktTimestamp(recvLine, 13);
            int flat=pktFlag(recvLine,13);
            sprintf(contents, "%d %d %d %d\n", cmd, seq,time,flat);
            FILE *f2 = fopen(APP_ROOT_"recv1.data", "a+");
            fwrite(contents, 1, strlen(contents), f2);
            //fwrite(recvLine + RTP_PACKET_HEAD_SIZE, 1, recvLength - RTP_PACKET_HEAD_SIZE, f2);
            fclose(f2);
        }

        context->RecvData(recvLine, recvLength, &clientAddr, 0);
    }


}

int
HgUdpabstract::RecvData(uint8_t *fragdata, uint32_t size, sockaddr_in *sa, int fd) {
    uint32_t ssrc = pktSsrc(fragdata, size);

    hg_Buf_Gen_t *hbgt= nullptr;

    //uint8_t *Hg_Buf_GetInfinite2(hg_chain_t *rnode,hg_chain_t **free, int *size,int length);
    //

    hbgt = (hg_Buf_Gen_t *) AllocMemery_local(this, sizeof(hg_Buf_Gen_t) + size);
    hbgt->init();
    hbgt->data = (char *) hbgt + sizeof(hg_Buf_Gen_t);
    hbgt->end = 0;
    hbgt->cap = size;
    hbgt->len = size - RTP_PACKET_HEAD_SIZE;
    hbgt->start = RTP_PACKET_HEAD_SIZE;


    memcpy(hbgt->data, fragdata, size);

    //hbgt->sfree.ctx = this;
    hbgt->sfree.params = nullptr;
    hbgt->sfree.freehandle = nullptr;
    //////
    int lens = sizeof(hgtEvent) + sizeof(client_packet_recv_pa);
    char hgteventptr[lens];
    client_packet_recv_pa *uprp = (client_packet_recv_pa *) (hgteventptr + sizeof(hgtEvent));
    uprp->sa = *sa;
    uprp->data = hbgt;
    uprp->ctx = recvth;
    uprp->size = size;

    hgtEvent *hgtevent = (hgtEvent *) hgteventptr;
    hgtevent->handle = HuageRecvStream::THPOOLRecv;
    hgtevent->i=6;
    hgtevent->ctx = recvth;
    hgtevent->psize = sizeof(client_packet_recv_pa);

    HgWorker *hgworker = HgWorker::GetWorker(ssrc);

    hgworker->WriteChanWor(hgteventptr, lens);
    /////
    return 0;
}


void HgUdpabstract::SendCallback(void *param, sendArr *sendarr, int size, HgSessionC *sess) {
    struct msghdr snd_msg;
    struct iovec snd_iov[size];

    for (int i = 0; i < size; i++) {
        snd_iov[i].iov_base = sendarr[i].data;
        snd_iov[i].iov_len = sendarr[i].size;

    }
    if(size>0){
        char contents[512]={0};
        int seq=pktSeq((uint8_t *)sendarr[0].data, 13);
        int cmd=pktCmd((uint8_t *)sendarr[0].data, 13);
        int time=pktTimestamp((uint8_t *)sendarr[0].data, 13);
        int flag=pktFlag((uint8_t *)sendarr[0].data, 13);
       // if(cmd<=RTP_CMD_POST) {


               sprintf(contents, "seq %d cmd %d time %d flag %d\n", seq,cmd,time,flag);
               FILE *f2 = fopen(APP_ROOT_"send.data", "a+");
               fwrite(contents, 1, strlen(contents), f2);
              // fwrite((uint8_t *)sendarr[1].data, 1, sendarr[1].size, f2);
               fclose(f2);


      //  }else{
          //   return;
       // }
    }


    HgUdpabstract *uc = (HgUdpabstract *) param;
    struct sockaddr_in *sin= nullptr;
    if(uc->isclient){
        sin=&uc->serveraddr;
    }else{
        sin=sess->sa;
    }

    snd_msg.msg_name =sin ; // Socket is connected
    snd_msg.msg_namelen = sizeof(struct sockaddr);
    snd_msg.msg_iov = snd_iov;
    snd_msg.msg_iovlen = size;
    snd_msg.msg_control = 0;
    snd_msg.msg_controllen = 0;

    // int abc=ADEC_HEADER_FLAG(*(recvLine + 1));

    sendmsg(uc->confd, &snd_msg, 0);
}

void HgUdpabstract::DirectSendMsg(void *pth, CMDTYPE cmdtype, void *ctx, uint8_t *rtpdata,
        uint32_t udpsize, rtpHeader *rh, uint32_t ssrc,
        HgSessionC *sess) {
    HgUdpabstract *context = (HgUdpabstract *) ctx;
    context->sendth->DirectSendMsg(pth, cmdtype, rtpdata, udpsize, rh, ssrc,
            sess);
}

void *HgUdpabstract::AllocMemery_local(HgUdpabstract *ctx, int size) {
    return myPoolAlloc(ctx->iocp->fragCache, size);
}

void HgUdpabstract::FreeMemery_local(void *pth, void *ctx, void *params, int psize) {
    HgUdpabstract *huc = (HgUdpabstract *) ctx;
    unsigned char **ptmp = (unsigned char **) params;
    unsigned char *hcn=*ptmp;
    myPoolFree(huc->iocp->fragCache, (unsigned char *) hcn);
}

void HgUdpabstract::WritePipeFree(void *ctx, void *data) {
    HgPipe *hp= nullptr;
    HgUdpabstract *aUdpinstan = (HgUdpabstract *) ctx;
    hp = aUdpinstan->iocp->aPipeClient;

    int lens = sizeof(hgtEvent) + sizeof(void *);
    char tmp[lens];
    hgtEvent *hgtevent = (hgtEvent *) tmp;
    // ALOGI(0,"HgUdpabstract::FreeMemery_local %ld,,,,,",HgUdpabstract::FreeMemery_local);
    hgtevent->handle = HgUdpabstract::FreeMemery_local;
    hgtevent->i=7;
    hgtevent->ctx = aUdpinstan;
    hgtevent->psize = sizeof(data);
    *((void **) ((char *) tmp + sizeof(hgtEvent))) = data;
    HgPipe::WritePipe(hp, tmp, lens);
}

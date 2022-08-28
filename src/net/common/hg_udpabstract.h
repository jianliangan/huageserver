//
// Created by ajl on 2022/1/11.
//
#include "../protocol/rtp_packet.h"
#include "../protocol/core/hg_queue.h"
#include "../protocol/core/hg_buf.h"
#include "../threads/hg_worker.h"
#include "../protocol/hg_channel_stru.h"
#include "../protocol/huage_sendstream.h"
#include "../protocol/huage_recvstream.h"
#include "../protocol/hgsession.h"
#include "hg_iocp.h"
#include "hg_netcommon.h"
#ifndef HUAGERTP_APP_HG_UDPABSTRACT_H
#define HUAGERTP_APP_HG_UDPABSTRACT_H



class HgUdpabstract {
    public:
        HgIocp *iocp= nullptr;
        struct sockaddr_in serveraddr;

        void *ver_recv_ctx= nullptr;
        uint32_t confd=0;
        HuageRecvStream *recvth= nullptr;
        HuageSendStream *sendth= nullptr;
        bool isclient=false;
        uint32_t maxmtu=0;
        bool haslog;
        HgUdpabstract();
        pthread_t recvThread;//接收数据
        void Connect();
        void List();
        bool finished;
        void Init();
        int GetSocket();

        void Listen(int port, const char *ip);
        void SetServer(const char *serverip, uint32_t port);
        bool (*verify_auth)(void *ctx,HgSessionC *sess,int fd)= nullptr;
        void (*asyncLogin)(void *ctx, media_frame_chain *mfc, HgSessionC *sess)= nullptr;
        void
            (*recvCallback)(PLTYPE pltype, void *ctx, media_frame_chain *mfc,  HgSessionC *sess
                    )= nullptr;
        HgFdEvent *InitEvent(HgIocp *iocp, int confd);
        //udp

        static void UDPRecvmsg(void *ioctx, void *ctx, void *ptr);
        static void SendCallback(void *param, sendArr *sendarr,int size,HgSessionC *sess);
        //函数指针
        static void DirectSendMsg(void *pth,CMDTYPE cmdtype,void *ctx, uint8_t *rtpdata, uint32_t udpsize,rtpHeader *rh, uint32_t ssrc,
                HgSessionC *sess);
        static void *AllocMemery_local(HgUdpabstract *huc,int size);
        static void FreeMemery_local(void *pth,void *ctx,void *params,int psize);
        static void WritePipeFree(void *ctx,void * data);
        int SendData(void *pth,PLTYPE pltype,bool isfirst,media_frame_chain *mfc, uint32_t ssrc,
                HgSessionC *sess,int comptype);
        int RecvData(uint8_t *fragdata, uint32_t size, sockaddr_in *sa, int fd);
        ~HgUdpabstract();

};


#endif //HUAGERTP_APP_HG_UDPABSTRACT_H

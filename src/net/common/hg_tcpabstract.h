//
// Created by ajl on 2022/1/11.
//

#ifndef HUAGERTP_APP_HG_TCPABSTRACT_H
#define HUAGERTP_APP_HG_TCPABSTRACT_H

#include "../protocol/rtp_packet.h"
#include "../protocol/core/hg_queue.h"
#include "../protocol/core/tools.h"
#include "../protocol/core/hg_buf.h"
#include "../protocol/hgsession.h"
#include "../protocol/huage_stream.h"
#include "../threads/hg_worker.h"
#include "../protocol/hgfdevent.h"
#include "hg_netcommon.h"
#include "hg_iocp.h"


#define RECV_BUFFER_LEN 512
#define RECV_MAX_BUF_SIZE 512
typedef struct twoaddr {
    void *param1;
    void *param2;
} twoaddr;

//typedef void (*handleRecv_def)(void *ctx,char*,int);
class HgTcpabstract {

public:
    HgIocp *iocp= nullptr;
    HgFdEvent *fdev= nullptr;
    void *ver_recv_ctx= nullptr;
    bool isclient=false;
    uint64_t lastclotim=0;
/*

bool isconnected;

 hg_Buf_Gen_t tcpbuf;  64k,不用就删掉
 */
    struct sockaddr_in serveraddr;

    HgTcpabstract();

    bool finished=false;

    int Connect();

    void SetServer(const char *serverip, uint32_t port);

    bool (*verify_auth)(void *ctx,HgSessionC *sess, int fd)= nullptr;

    void (*asyncLogin)(void *ctx, media_frame_chain *mfc, HgSessionC *sess)= nullptr;

    void (*recvCallback)(PLTYPE pltype, void *ctx, media_frame_chain *mfc, HgSessionC *sess)= nullptr;

    //tcp
    HgFdEvent *InitEvent(HgIocp *iocp, int confd);

    HgFdEvent *InitLisEvent(HgIocp *iocp, int confd);

    static void TCPAccept(void *ioctx, void *ctx, void *ptr);

    static void TCPRecvmsg(void *ioctx, void *ctx, void *ptr);

    static void TCPSendmsg(void *ioctx, void *ctx, void *ptr);

    static void SendChain(void *pth, void *ctx, void *params, int psize);

    static void PreSendDataChain(void *ioctx, void *ctx, HgFdEvent *hfe, uint32_t ssrc);

    static void THPOOLRecv(void *pth, void *ctx, void *params, int size);

    static void WritePipeFree(void *ctx, void *data);

    static void FreeMemery_local(void *pth, void *ctx, void *params, int psize);

    static void ClearSndChain(hg_chain_t *hct, HgSessionC *hgsess, bool all);//new
    static void DestroyHeads(void *ctx, void *data);

    int PreSendArr(int efd, hg_chain_t *hct, HgSessionC *hgsess, HgFdEvent *hfe);

    hg_Buf_Gen_t *CreateBuffer(uint8_t pltype, bool isfirst, uint16_t time,
                               HgSessionC *hgsess, int size,int comptype);

    void PushFramCach(media_frame_chain *mfcsour, HgSessionC *hgsess, hg_chain_t *sesshct,
                      hg_Buf_Gen_t *head);

    int SendDatapre(int efd, hg_chain_t *hct, hg_Buf_Gen_t **hbgtarr, int num, HgSessionC *hgsess,
                    HgFdEvent *hfe);

    int SendData(void *pth, PLTYPE pltype,bool isfirst,  media_frame_chain *mfcsour,
                 uint32_t ssrc, HgSessionC *hgsess, HgFdEvent *hfe,int comptype);

    int RecvData(void *ptr, media_frame_chain *mfc);

    int SendData0(int confd, sendArr *sendarr, int size);

    void Init();

    void Listen(int port, const char *ip);
    int GetSocket();

    //函数指针
    ~HgTcpabstract();
};

#endif //HUAGERTP_APP_HG_TCPABSTRACT_H

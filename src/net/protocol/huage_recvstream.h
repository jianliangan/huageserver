//
// Created by ajl on 2020/12/27.
//


#ifndef HUAGE_PROTOCOL_RECVSTREAM_H
#define HUAGE_PROTOCOL_RECVSTREAM_H

#include "core/hg_queue.h"
#include "hg_channel_stru.h"
#include "../threads/hg_worker.h"
#include "rtp_packet.h"
#include "hgsession.h"
#include "huage_stream.h"
#include "core/hg_buf.h"
typedef struct THPOOLparams{
    sockaddr_in sa;
    uint8_t data;
}THPOOLparams;

class HuageRecvStream {
public:
    void *ctx;
    static void *udpinstan;
void *ver_recv_ctx= nullptr;
    HuageRecvStream();
    void clearPacket2(void *udpLine);
    void clearPacket( Hg_cacheStru_s *recvcache);
    int
    RecvData(uint8_t *fragdata, uint32_t size,sockaddr_in *sa,int fd);
    bool allowothers;
    void (*asyncLogin)(void *ctx, media_frame_chain *mfc, HgSessionC *sess)= nullptr;
    bool (*verify_auth)(void *ctx,HgSessionC *sess,int fd)= nullptr;
    void
    (*recvCallback)(PLTYPE pltype, void *ctx, media_frame_chain *mfc, HgSessionC *sess)= nullptr;

    void (*directSendMsg)(void *pth,CMDTYPE cmdtype, void *ctx,uint8_t *rtpdata, uint32_t udpsize,rtpHeader *rh, uint32_t ssrc,
                             HgSessionC *sess)= nullptr;
    static void THPOOLRecv(void *pth,void *ctx,void *params,int psize);
    param_handle udpfreehandle;
    void *freectx= nullptr;
    bool finished=false;

    ~HuageRecvStream();

private:
    uint32_t seq=0;
    pthread_t streamth;
    uint32_t headlen=0;
    uint8_t rtpHeadBuf[RTP_PACKET_HEAD_SIZE];

    uint8_t hg_event0[EV_HG_PTR_EVENTLEN];
    hgEvent hgev0;

    uint8_t *streamMaxFrame=nullptr;
    //myPool *
    myQueue *eventQueue=nullptr;

    sem_t eventSem;
    static void FreeByChan(void *pth, void *ctx, void *params, int psize);
    static void FreeChain(hg_chain_node *hcntmp);
    static void WritePipeFree(void *ctx, void *data);
    void writeTocache(uint16_t seq,uint16_t *maxseqs, hg_Buf_Gen_t *hbgt, Hg_cacheStru_s *recvcache);
    void FindUseful(void *pth,PLTYPE pltype,bool allclear,  media_frame_chain *bigframebuf, Hg_cacheStru_s *recvcache,
                    HgSessionC *sessPtr,
                    sockaddr_in *tmpsa, uint16_t maxseqs, uint16_t *CurPacketSeqs,
                    uint32_t *bigframeOffset);
    void MergeDataClear( media_frame_chain *bigframebuf, HgSessionC *sessPtr, uint32_t *bigframeOffset);
    int MergeData(PLTYPE pltype,bool allclear,int8_t evtype, media_frame_chain *bigframebuf,
                  Hg_cacheStru_s *udpdata, uint32_t tmpseq,
                  uint8_t flag, HgSessionC *sessPtr,
                  sockaddr_in *tmpsa,uint32_t *bigframeOffset,uint16_t CurPacketSeqs);
    void ReSend(void *pth,HgSessionC *sessPtr,PLTYPE pltype,uint16_t seq);

};

#endif //HUAGE_PROTOCOL_STREAM_H

//
// Created by ajl on 2020/12/27.
//

#ifndef HUAGE_PROTOCOL_SENDSTREAM_H
#define HUAGE_PROTOCOL_SENDSTREAM_H

#include "core/hg_queue.h"
#include "hg_channel_stru.h"
#include "../threads/hg_worker.h"
#include "rtp_packet.h"
#include "hgsession.h"
#include "huage_stream.h"


class HuageSendStream {
    public:
        void *ctx= nullptr;
        bool finished=false;

        HuageSendStream();

        void clearPacketWrBufer(HgSessionC *hgsess,uint32_t seq, uint8_t *udpLine,int size,void *alloc, Hg_cacheStru_s *sendcache,uint16_t *maxseqs,rtpHeader *rh);
        void clearPacket2(HgSessionC *hgsess,void *alloc);
        int
            SendData(void *pth,PLTYPE pltype,bool isfirst,media_frame_chain *mfc, uint32_t ssrc,
                    HgSessionC *sess,int comptype);

        void
            (*sendCallback)(void *ctx, sendArr *sendarr,int size, HgSessionC *sess)= nullptr;

        void DirectSendMsg(void *pth,CMDTYPE cmdtype, uint8_t *rtpdata, int udpsize,rtpHeader *rh,
                int ssrc,
                HgSessionC *sess);
        ~HuageSendStream();

    private:

        uint8_t hg_event0[EV_HG_PTR_EVENTLEN];
        static void THPOOLSend(void *pth,void *ctx,void *params,rtpHeader *rh,void *allocaddi);
        //发送的时候可能没有session，所以ssrc得有
        void InnerPushEvent(void *pth,CMDTYPE cmdtype, uint8_t *data, uint32_t size,rtpHeader *rh,
                uint32_t ssrc, HgSessionC *sess,void *alloc);


};

#endif //HUAGE_PROTOCOL_SENDSTREAM_H

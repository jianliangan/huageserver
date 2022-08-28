//
// Created by ajl on 2020/12/27.
//

#include "huage_sendstream.h"

#include "hgsession.h"
#include "hg_channel_stru.h"

using namespace std;

HuageSendStream::HuageSendStream() {
    finished = true;
}

void
HuageSendStream::THPOOLSend(void *pth, void *ctx, void *params, rtpHeader *rh, void *allocaddi) {
    client_packet_send_pa *upsp = (client_packet_send_pa *) params;
    HuageSendStream *hgStream = (HuageSendStream *) ctx;
    HgWorker *hworker = (HgWorker *) pth;
    HgSessBucket *hgsessbu = (HgSessBucket *) hworker->sess_s;
    uint8_t *hgevdata = upsp->data;
    int hgevcmd;
    uint16_t time = upsp->time;
    int hgevsize = upsp->size;
    HgSessionC *sessPtr = upsp->sess;
    //HuageSendStream *hgStream = (HuageSendStream *) ctx;
    //直接发送需要的变量
    uint32_t totalLength = EV_HG_PTR_EVENTLEN;
    uint32_t reallength = 0;
    uint8_t hg_event1[EV_HG_PTR_EVENTLEN];

    uint16_t *MaxPacketSeqs = nullptr;

    uint32_t rtpseq = 0;

    Hg_cacheStru_s *sendcache;
    PLTYPE pltype;

    //do {

    rtpseq = rh->seq;
    pltype = (PLTYPE) rh->payloadType;
    hgevcmd = rh->cmd;

    HgSessionC::sessSendPtrStreamUdp(sessPtr, pltype,
            &sendcache,
            &MaxPacketSeqs);

    if (*MaxPacketSeqs - (sess_cache_LEN-1) >= 0) {
        Hg_cacheStru_s css = sendcache[(*MaxPacketSeqs - (sess_cache_LEN-1)) % sess_cache_LEN];
        Hg_cacheStru_s css2 = sendcache[*MaxPacketSeqs % sess_cache_LEN];
        if (css.data != nullptr && css2.data != nullptr) {
            //    pktTimestamp(css.data, 100), pktTimestamp(css2.data, 100));
        }

    }

    //先加单播和广播，然后再优化速度看一下广播的锁是否和接收一直冲突
    if (hgevcmd <= RTP_CMD_POST) {
        uint8_t rhtmp[RTP_PACKET_HEAD_SIZE];
        rtpPacketHead(rhtmp, rh);
        sendArr sendarr[2];
        sendarr[0].size = RTP_PACKET_HEAD_SIZE;
        sendarr[0].data = rhtmp;
        sendarr[1].size = hgevsize;
        sendarr[1].data = hgevdata;

        /*FILE *f2 = fopen(APP_ROOT_"0-1.data", "a");
          fwrite(hgevdata, 1, hgevsize, f2);
          fclose(f2);
          */

        hgStream->sendCallback(hgStream->ctx, sendarr, 2,
                sessPtr);

        hgStream->clearPacketWrBufer(sessPtr, rtpseq, hgevdata, hgevsize, allocaddi, sendcache,
                MaxPacketSeqs, rh);
    } else if (hgevcmd == RTP_CMD_REQ_REPOST) {
        unsigned char rhtmp[RTP_PACKET_HEAD_SIZE];
        rtpPacketHead(rhtmp, rh);
        sendArr sendarr[1];
        sendarr[0].size = RTP_PACKET_HEAD_SIZE;
        sendarr[0].data = rhtmp;
        hgStream->sendCallback(hgStream->ctx, sendarr, 1,
                sessPtr);
        //hgStream->clearPacket2(sessPtr, upsp);

    } else if (hgevcmd == RTP_CMD_RES_REPOST) {

        int difftmp = uint16Sub(*MaxPacketSeqs, rtpseq);
        if (difftmp <= sess_cache_LEN && difftmp >= 0) {//判断缓存区内有没有，

            uint32_t index = rtpseq % sess_cache_LEN;
            Hg_cacheStru_s *cachestru = &sendcache[index];
            uint8_t *sendc = cachestru->data;
            uint32_t sendclen = cachestru->size;//根据pool的接口直接拿到要发送的长度
            void *rhtmp = cachestru->sndhead;
            pktSetCmd((uint8_t *) rhtmp, (int) hgevcmd);
            //rhtmp.cmd=(int)hgevcmd;
            if (sendc != nullptr) {
                sendArr sendarr[2];
                sendarr[0].size = RTP_PACKET_HEAD_SIZE;
                sendarr[0].data = rhtmp;
                sendarr[1].size = sendclen;
                sendarr[1].data = sendc;

                hgStream->sendCallback(hgStream->ctx,
                        sendarr, 2,
                        sessPtr);
            }
        } else {
            ALOGI(0, "resend packet error do not found %u", rtpseq);
        }
    }
}

void
HuageSendStream::DirectSendMsg(void *pth, CMDTYPE cmdtype, uint8_t *rtpdata, int udpsize,
        rtpHeader *rh,
        int ssrc,
        HgSessionC *sess) {
    InnerPushEvent(pth, cmdtype, rtpdata, udpsize, rh, ssrc, sess, nullptr);
}

void HuageSendStream::InnerPushEvent(void *pth, CMDTYPE cmdtype, uint8_t *data, uint32_t size,
        rtpHeader *rh,
        uint32_t ssrc, HgSessionC *sess, void *alloc) {
    //client_packet_send_pa *upsp = (client_packet_send_pa *) HgSessionC::AllocMemery_local(sess,
    //                                                                                       sizeof(client_packet_send_pa));
    client_packet_send_pa upsp;
    upsp.data = data;
    upsp.size = size;
    upsp.time = 0;
    upsp.ctx = this;
    upsp.sess = sess;

    HuageSendStream::THPOOLSend(pth, this, &upsp, rh, alloc);


    //
}

void HuageSendStream::clearPacket2(HgSessionC *hgsess, void *alloc) {
    //ALOGI(0, "send clearPacket2 packet ");
    if (alloc != nullptr) {
        hg_Buf_Gen_t *hbgt = (hg_Buf_Gen_t *) alloc;
        selffree_s *ss = &hbgt->sfree;
        if (ss->freehandle != nullptr)
            // ALOGI(0,"444444444444444 %ld",ss);
            ss->freehandle(ss->ctx, ss->params);
        //  ALOGI(0,"444444444444444 333 %ld",ss->freehandle);
    }
}

void
HuageSendStream::clearPacketWrBufer(HgSessionC *hgsess, uint32_t seq, uint8_t *udpLine, int size,
        void *alloc,
        Hg_cacheStru_s *sendcache,
        uint16_t *maxseqs, rtpHeader *rh) {
    uint32_t index = seq % sess_cache_LEN;
    Hg_cacheStru_s *cachestru = &sendcache[index];
    //ALOGI(0, "send clear packet %u", seq);
    if (cachestru->alloc != nullptr) {
        clearPacket2(hgsess, cachestru->alloc);


    } else {
        // ALOGI(0, "send clear packet");
    }
    cachestru->data = udpLine;
    cachestru->size = size;
    cachestru->alloc = alloc;
    rtpPacketHead(cachestru->sndhead, rh);

    int diffnew = uint16Sub(seq, *maxseqs);
    if (diffnew > 0) {
        *maxseqs = seq;
    }
}

int
HuageSendStream::SendData(void *pth, PLTYPE pltype, bool isfirst, media_frame_chain *mfc,
        uint32_t ssrc, HgSessionC *sess,int comptype) {
    uint32_t maxcopysize = 0;

    uint32_t shouldcopy = 0;
    uint32_t copiedLength = 0;
    int8_t tail = 0;
    void *alloctmp = nullptr;
    CMDTYPE cmdtype;
    uint16_t *seqtmp = nullptr;
    uint16_t time=mfc->pts;
    maxcopysize = MAX_RTP_PACKET_SIZE - RTP_PACKET_HEAD_SIZE;
    rtpHeader rh;
    rh.ssrc = ssrc;
    rh.payloadType = (uint8_t) pltype;

    rh.first = 0;
    if (isfirst) {
        rh.first = 1;
    }


    cmdtype = RTP_CMD_POST;
    rh.cmd = cmdtype;
    //ALOGI(0,"111111111 SendDataQ  4,1");
    HgSessionC::sessSendPtrStreamSeq(sess, pltype, &seqtmp);
    // ALOGI(0,"111111111 SendDataQ  4,2");

    /*
       hg_chain_node *hcntmp1 = mfc->hct.left;
       hg_Buf_Gen_t *hbgttmp = nullptr;
       FILE *f2 = fopen(APP_ROOT_"send1.data", "a+");
       while (hcntmp1 != nullptr) {
       hbgttmp = (hg_Buf_Gen_t *) hcntmp1->data;
       fwrite((char *) hbgttmp->data + hbgttmp->start, 1, hbgttmp->len, f2);
       hcntmp1 = hcntmp1->next;
       }
       char aaa[] = "1111111111111111111111111111111111222222222222222222222222";
       fwrite(aaa, 1, strlen(aaa), f2);
       fclose(f2);
       */

    hg_chain_node *hcntmp = mfc->hct.left;
    int restsize = 0;
    int framesize = mfc->size - RTP_PACKET_HEAD_SIZE;
    int sendsize = 0;
    while (hcntmp != nullptr) {

        shouldcopy = 0;
        copiedLength = 0;
        tail = 0;
        alloctmp = nullptr;


        hg_Buf_Gen_t *hbgt = (hg_Buf_Gen_t *) hcntmp->data;
        uint8_t *bigData = (uint8_t *) hbgt->data + hbgt->start;
        uint8_t *cursor = bigData;
        restsize = hbgt->len;


        while (true) {
            tail = 0;
            if (restsize > 0) {
                //recvLineRtp = rtpdata;//00代表独立的，10代表开始，01代表过程中，11代表结束

                shouldcopy = 0;
                if (maxcopysize < restsize) {//和节点剩余数据比较
                    shouldcopy = maxcopysize;
                } else {
                    shouldcopy = restsize;
                }
                cursor = (uint8_t *) bigData + copiedLength;
                if (sendsize == 0) {
                    if (shouldcopy >= framesize) {
                        tail = 0x00;
                    } else {
                        tail = 0x02;
                    }

                } else {
                    if (shouldcopy >= framesize - sendsize) {
                        tail = 0x03;
                    } else {
                        tail = 0x01;
                    }
                }
                /*
                   if (restsize-shouldcopy==0) {
                   if (copiedLength==0) {
                   tail = 0x00;
                // 4-5修改成00 独立的
                } else {
                tail = 0x03;
                // 修改成11 过程结束
                }
                } else {
                if (copiedLength==0) {
                tail = 0x02;
                // 修改成10 过程开始
                } else {
                tail = 0x01;
                // 修改成01 过程中
                }

                }
                */
            } else {
                break;
            }


            if (shouldcopy > 0) {
                rh.timestamp = time;
                *seqtmp = uint16Add(*seqtmp, 1);
                rh.seq = *seqtmp;//到这里了
                //test=(test==""?"":(test+","))+std::to_string(rh.seq);
                rh.tail = tail;
rh.comp=comptype;
                rh.payloadType = pltype;

                //rtpPacketHead(recvLineRtp, &rh);
                if (tail == 0x00 || tail == 0x03) {
                    alloctmp = hbgt;
                } else {
                    alloctmp = nullptr;
                }



                InnerPushEvent(pth, cmdtype, cursor, shouldcopy, &rh,
                        ssrc, sess, alloctmp);
                copiedLength += shouldcopy;
                restsize -= shouldcopy;
                sendsize += shouldcopy;
               // ALOGI(0,"1122222222222221111111 SendDataQ  time %d seq %d,len %ld,rest %ld,%ld",rh.timestamp,rh.seq,shouldcopy,restsize,hbgt->len);
            }
        }
        hcntmp = hcntmp->next;
    }
    return 0;
}

HuageSendStream::~HuageSendStream() {

}

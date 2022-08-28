//
// Created by ajl on 2020/12/27.
//

#include "huage_recvstream.h"
#include "hgsession.h"
#include "../common/hg_udpabstract.h"

using namespace std;
void *HuageRecvStream::udpinstan = nullptr;

HuageRecvStream::HuageRecvStream() {


    seq = 0;
    finished = false;
    uint32_t err;
    asyncLogin = nullptr;
    verify_auth = nullptr;

    finished = true;


}


void HuageRecvStream::THPOOLRecv(void *pth, void *ctx, void *params, int psize) {
    client_packet_recv_pa *uprp = (client_packet_recv_pa *) params;

    //ALOGI(0,"recv udp THPOOLRecv psize %d",psize);
    HuageRecvStream *hgRecvStream = (HuageRecvStream *) ctx;

    hg_Buf_Gen_t *hgevdata = (hg_Buf_Gen_t *) uprp->data;
    int size = uprp->size;
    void *allocptr = hgevdata;
    //接收流，流=自定义包头+udp包
    HgWorker *hworker = (HgWorker *) pth;
    HgSessBucket *hgsessbu = (HgSessBucket *) hworker->sess_s;
    media_frame_chain *bigframebuf = nullptr;//此处是单层chain，代表一个帧的chain，未来需要的话可以弄成两层多个帧的chain
    Hg_cacheStru_s *recvcache = nullptr;

    uint8_t *udpLine = nullptr;
    uint8_t *rtpBodyLine = nullptr;
    uint32_t udpLength = 0;
    uint32_t rtpBodyLength = 0;
    uint8_t flag = 0;
    uint16_t tmpseq = 0;
    uint16_t tmptimestamp = 0;
    //uint32_t reallength = 0;
    uint32_t bodylength = 0;

    uint32_t ssrc = 0;
    PLTYPE pltype = ENU_PLTYPENONE;
    HgSessionC *sessPtr = nullptr;
    HgSessionC *tmpsess = nullptr;
    sockaddr_in *tmpsa = nullptr;
    int tmpfd = 0;
    uint8_t hg_event[EV_HG_PTR_EVENTLEN];

    hgEvent hgev;

    int isfirst = false;
    int hgevcmd = 0;
    uint16_t *CurPacketSeqs = nullptr;
    uint16_t *MaxPacketSeqs = nullptr;
    uint32_t *bigframeOffset = nullptr;


    if (hgevdata == nullptr) {
        return;
    }
    tmpsa = nullptr;
    tmpsa = &(uprp->sa);
    tmpfd = 0;
    udpLine = (unsigned char *) hgevdata->data;//hgev.data;//udp包位置
    udpLength = hgevdata->cap;//udp包长度
    rtpBodyLine = udpLine + RTP_PACKET_HEAD_SIZE;
    rtpBodyLength = udpLength - RTP_PACKET_HEAD_SIZE;

    tmpseq = pktSeq(udpLine, udpLength);
    tmptimestamp = pktTimestamp(udpLine, udpLength);
    ssrc = pktSsrc(udpLine, udpLength);
    flag = pktFlag(udpLine,udpLength);
    pltype = pktPLoadType(udpLine, udpLength);
    isfirst = pktFirst(udpLine, udpLength);
    hgevcmd = pktCmd(udpLine, udpLength);

    {
        char contents[1000];
        sprintf(contents, "%d %d %d %d\n", hgevcmd, tmpseq,tmptimestamp,flag);
        FILE *f2 = fopen(APP_ROOT_"recv2.data", "a+");
        fwrite(contents, 1, strlen(contents), f2);
        //fwrite(recvLine + RTP_PACKET_HEAD_SIZE, 1, recvLength - RTP_PACKET_HEAD_SIZE, f2);
        fclose(f2);
    }
    //Does that peer have permission to write data to me,fetch access;
    if (isfirst == 1) {
        //Does that peer have permission to write data to me

        sessPtr = HgSessionC::ActiveSess(hgsessbu, ssrc);
        if (sessPtr == nullptr) {
            hgRecvStream->clearPacket2(allocptr);
            return;
        }
        HgSessionC::sessRecvPtrStreamUdp(sessPtr, pltype,
                                         &bigframebuf, &recvcache,
                                         &CurPacketSeqs, &bigframeOffset, &MaxPacketSeqs);
        HgSessionC::sessAttachAddr(hgsessbu, sessPtr, tmpsa, sizeof(sockaddr_in));
        hg_chain_t *bigframetmp = &bigframebuf->hct;

        if (hgRecvStream->asyncLogin != nullptr) {
            hgRecvStream->MergeDataClear( bigframebuf, sessPtr,bigframeOffset);
            Hg_PushChainDataR(sessPtr->fragCache, bigframetmp, hgevdata);
            //bigframebuf->sfree.ctx=sessPtr;
            bigframebuf->sfree.params = nullptr;
            bigframebuf->sfree.freehandle = nullptr;
            bigframebuf->size = size;
            *bigframeOffset = size;
            hg_chain_node *hcnR = bigframebuf->hct.right;
            hg_Buf_Gen_t *hbgtR = (hg_Buf_Gen_t *) hcnR->data;
            hbgtR->sfree.ctx = sessPtr;
            hbgtR->sfree.params = bigframebuf->hct.left;
            hbgtR->sfree.freehandle = HuageRecvStream::WritePipeFree;

            hgRecvStream->asyncLogin(hgRecvStream->ver_recv_ctx, bigframebuf,
                                     sessPtr);

            *bigframeOffset = 0;

            bigframetmp->left = nullptr;
            bigframetmp->right = nullptr;
            bigframebuf->size = 0;


        }
        return;
    }

    sessPtr = HgSessionC::sessGetSessInBuck(hgsessbu, ssrc);
    HgSessionC::sessRecvPtrStreamUdp(sessPtr, pltype,
                                     &bigframebuf, &recvcache,
                                     &CurPacketSeqs, &bigframeOffset, &MaxPacketSeqs);
    HgSessionC::sessAttachAddr(hgsessbu, sessPtr, tmpsa, sizeof(sockaddr_in));
    if (hgRecvStream->verify_auth(hgRecvStream->ver_recv_ctx, sessPtr, tmpfd) == false) {
        hgRecvStream->clearPacket2(allocptr);
        return;
    }
    bigframebuf->pts=tmptimestamp;
    if (hgevcmd <= RTP_CMD_RES_REPOST) {//如果是投递事件



        if (*CurPacketSeqs == 0xff) {
            *CurPacketSeqs = tmpseq;
        }
if(uint16Sub(*CurPacketSeqs, tmpseq)<=0 ){
    hgRecvStream->writeTocache(tmpseq, MaxPacketSeqs, hgevdata,
                               recvcache);//先写入缓存，并且清过期数据,修改最大seq
}


        hgRecvStream->FindUseful(pth, pltype, false, bigframebuf, recvcache,
                                 sessPtr, tmpsa, *MaxPacketSeqs, CurPacketSeqs,
                                 bigframeOffset);//处理无间断的数据，重发丢包,同时清理过期数据



    } else if (hgevcmd == RTP_CMD_REQ_REPOST) {
        rtpHeader rh;
        rh.ssrc = ssrc;
        rh.payloadType = pltype;
        rh.cmd = RTP_CMD_RES_REPOST;
        rh.seq = tmpseq;//到这里了

        //rtpPacketHead(hgRecvStream->rtpHeadBuf, &rh);
        hgRecvStream->directSendMsg(pth, RTP_CMD_RES_REPOST, hgRecvStream->ctx,
                                    nullptr,
                                    0, &rh, ssrc, sessPtr);
        hgRecvStream->clearPacket2(hgevdata);
    }

    //}
}


//检查哪些包需要重发，哪些包可以用了
void
HuageRecvStream::FindUseful(void *pth, PLTYPE pltype, bool allclear, media_frame_chain *bigframebuf,
                            Hg_cacheStru_s *recvcache,
                            HgSessionC *sessPtr,
                            sockaddr_in *tmpsa, uint16_t MaxPacketSeqs, uint16_t *CurPacketSeqs,
                            uint32_t *bigframeOffset) {
   
}


void HuageRecvStream::ReSend(void *pth, HgSessionC *sessPtr, PLTYPE pltype, uint16_t seq) {
    rtpHeader rh;
    rh.ssrc = sessPtr->ssrc;
    rh.payloadType = pltype;

    rh.cmd = RTP_CMD_REQ_REPOST;
    rh.timestamp = 0;
    rh.seq = seq;//到这里了
    rh.tail = 0;


    //rtpPacketHead(rtpHeadBuf, &rh);
    directSendMsg(pth, RTP_CMD_REQ_REPOST, ctx, nullptr,
                  0, &rh,
                  sessPtr->ssrc, sessPtr);

}
void
HuageRecvStream::MergeDataClear( media_frame_chain *bigframebuf,HgSessionC *sessPtr, uint32_t *bigframeOffset) {
    *bigframeOffset = 0;
    FreeChain(bigframebuf->hct.left);
    Hg_ChainDelChain(sessPtr->fragCache, bigframebuf->hct.left);
    bigframebuf->hct.left = nullptr;
    bigframebuf->hct.right = nullptr;
    bigframebuf->size = 0;
    bigframebuf->pts=0;

}
int
HuageRecvStream::MergeData(PLTYPE pltype, bool allclear, int8_t evtype,
                           media_frame_chain *bigframebuf,
                           Hg_cacheStru_s *udpdata,
                           uint32_t tmpseq,
                           uint8_t tailflag, HgSessionC *sessPtr,
                           sockaddr_in *tmpsa, uint32_t *bigframeOffset, uint16_t seqtmp) {
    /*
     *
     前面 尽量正序一下，下面如果遇到乱序的问题就直接忽略了
     */

    hg_Buf_Gen_t *hbgt = (hg_Buf_Gen_t *) udpdata->alloc;
    uint8_t *udpLine = (unsigned char *) hbgt->data;
    int udpLength = hbgt->len + hbgt->start;;
    uint32_t rtpBodyLength = (unsigned int) (udpLength - RTP_PACKET_HEAD_SIZE);
    uint8_t *rtpBodyLine = udpLine + RTP_PACKET_HEAD_SIZE;
    //此处默认前面保证顺

    if (tailflag == 0) {//音频大部分是这个
        //sessPtr->test->clear();
        // sessPtr->test->append(std::to_string(tmpseq));
        if (pltype == ENU_PLTYPEVIDEO) {
            char out[4096];

            //uint8_t result[16];
            //md5(udpLine+12,udpLength-12,result);
            //uint64_t *l1;
            // uint64_t *l2;
            // l1=(uint64_t *)result;
            // l2=(uint64_t *)(result+8);

            // FILE *f2 = fopen("/data/data/org.libhuagertp.app/3.data", "a");
            // sprintf(out,"%d %s %lu %lu\n",udpLength-12,sessPtr->test.c_str(),0,0);
            // fwrite(out, 1, strlen(out), f2);
            //  fclose(f2);
        }
        // 独立的音视频包直接拿去解码
        MergeDataClear( bigframebuf,sessPtr,bigframeOffset);
        hg_chain_t *bigframetmp = &bigframebuf->hct;
        Hg_PushChainDataR(sessPtr->fragCache, bigframetmp, hbgt);
        bigframebuf->size = udpLength;
        *bigframeOffset = udpLength;

        bigframebuf->sfree.params = nullptr;
        bigframebuf->sfree.freehandle = nullptr;

        hg_chain_node *hcnR = bigframebuf->hct.right;
        hg_Buf_Gen_t *hbgtR = (hg_Buf_Gen_t *) hcnR->data;
        hbgtR->sfree.ctx = sessPtr;
        hbgtR->sfree.params = bigframebuf->hct.left;
        hbgtR->sfree.freehandle = HuageRecvStream::WritePipeFree;
        //ALOGI(0,"recv udp THPOOLRecv audio udpLength %d",udpLength);
        recvCallback(pltype, ver_recv_ctx, bigframebuf,
                     sessPtr);

        *bigframeOffset = 0;
        //Hg_ChainDelChain(sessPtr->fragCache,bigframetmp->left);
        bigframebuf->hct.left = nullptr;
        bigframebuf->hct.right = nullptr;
        bigframebuf->size = 0;

    } else {
        if (tailflag == 2) {//大包的开始
            if (udpLength > MAXBIGFRAMELEN) {

                clearPacket(udpdata);
                return 0;
            }

            MergeDataClear( bigframebuf, sessPtr,bigframeOffset);
            Hg_PushChainDataR(sessPtr->fragCache, &bigframebuf->hct, hbgt);


            *bigframeOffset = udpLength;
        } else if (tailflag == 1 || tailflag == 3) {//大包过程中
            if (tmpseq == seqtmp) {
                if (*bigframeOffset + rtpBodyLength > MAXBIGFRAMELEN) {
                    MergeDataClear( bigframebuf, sessPtr,bigframeOffset);
                    clearPacket(udpdata);
                    return 0;
                }
                Hg_PushChainDataR(sessPtr->fragCache, &bigframebuf->hct, hbgt);

                *bigframeOffset += rtpBodyLength;

                if (tailflag == 3) {
                    bigframebuf->size = *bigframeOffset;
                    //bigframebuf->sfree.ctx=sessPtr;
                    bigframebuf->sfree.params = nullptr;
                    bigframebuf->sfree.freehandle = nullptr;

                    hg_chain_node *hcnR = bigframebuf->hct.right;
                    hg_Buf_Gen_t *hbgtR = (hg_Buf_Gen_t *) hcnR->data;
                    hbgtR->sfree.ctx = sessPtr;
                    hbgtR->sfree.params = bigframebuf->hct.left;
                    hbgtR->sfree.freehandle = HuageRecvStream::WritePipeFree;
                    //ALOGI(0,"recv udp THPOOLRecv udpLength %d",udpLength);
                    recvCallback(pltype, ver_recv_ctx, bigframebuf, sessPtr);

                    *bigframeOffset = 0;
                    //Hg_ChainDelChain(sessPtr->fragCache,bigframebuf->hct.left);
                    bigframebuf->hct.left = nullptr;
                    bigframebuf->hct.right = nullptr;
                    bigframebuf->size = 0;
                }
            } else {
                MergeDataClear( bigframebuf,sessPtr,bigframeOffset);
                clearPacket(udpdata);
                return 0;
            }

        } else {//非法包
            MergeDataClear( bigframebuf,sessPtr,bigframeOffset);
            clearPacket(udpdata);
            return 0;//不能参与逻辑直接忽略
        }
    }
    return 0;
}

void
HuageRecvStream::writeTocache(uint16_t seq, uint16_t *MaxPacketSeqs,
                              hg_Buf_Gen_t *hbgt,
                              Hg_cacheStru_s *recvcache) {
    uint16_t maxseq = *MaxPacketSeqs;
    void *allocptr = hbgt;
    int diffnew = uint16Sub(seq, maxseq);
    uint8_t *data = nullptr;
    void *alloc = nullptr;
    Hg_cacheStru_s *cachestru = nullptr;
    /*
    //清理数据，新来的seq和已收到最大的seq之间的空洞要清理置空
    for (int i = 1; i < diffnew; i++) {
    uint32_t index = uint16Add(maxseq, i) % sess_cache_LEN;
    cachestru = &recvcache[index];
    data = cachestru->data;
    alloc = cachestru->alloc;
    if (data != nullptr) {
    hg_Buf_Gen_t *hgbt=(hg_Buf_Gen_t *)alloc;
    HgUdpabstract::WritePipeFree(HuageRecvStream::udpinstan,hbgt);
    cachestru->data = nullptr;
    cachestru->alloc = nullptr;
    }
    if (i == sess_cache_LEN) {//空洞的长度不会超过缓冲长度
    break;
    }
    }*/
    //cache存入新的数据
    uint32_t index = seq % sess_cache_LEN;
    cachestru = &recvcache[index];
    if (cachestru->data != nullptr) {
        return;//如果缓冲中有数据不能覆盖
        /*
           hg_Buf_Gen_t *hgbt = (hg_Buf_Gen_t *) cachestru->alloc;
           HgUdpabstract::WritePipeFree(HuageRecvStream::udpinstan, hbgt);
           cachestru->data = nullptr;
           cachestru->alloc = nullptr;
           */
    }


    char contents[1000];
      sprintf(contents, "222222222222222222222222333 rescvcall seq %d maxseq %d\n", seq, maxseq);
      FILE *f2 = fopen(APP_ROOT_"recv000.data", "a+");
      fwrite(contents, 1, strlen(contents), f2);
      fclose(f2);


    cachestru->data = (unsigned char *) hbgt->data;//所有修改recvcache，sendcacke的地方都要先释放
    cachestru->alloc = allocptr;

    if (diffnew > 0) {
        *MaxPacketSeqs = seq;
    }

}

void HuageRecvStream::clearPacket2(void *allocptr) {
    //ALOGI(0, "send clearPacket2 packet ");
    if (allocptr != nullptr) {
        hg_Buf_Gen_t *hbgt = (hg_Buf_Gen_t *) allocptr;
        HgUdpabstract::WritePipeFree(HuageRecvStream::udpinstan, hbgt);
        // if(hgbt->sfree.freehandle!=null)
        //   hgbt->sfree.freehandle(hgbt->sfree.ctx,hgbt->sfree.params);
    }
}

void HuageRecvStream::clearPacket(Hg_cacheStru_s *cachestru) {
    if (cachestru->data != nullptr) {
        hg_Buf_Gen_t *hbgt = (hg_Buf_Gen_t *) cachestru->alloc;
        HgUdpabstract::WritePipeFree(HuageRecvStream::udpinstan, hbgt);
    }
}

void HuageRecvStream::FreeByChan(void *pth, void *ctx, void *params, int psize) {
    hg_chain_node **ptmp = (hg_chain_node **) params;
    hg_chain_node *hcn = *ptmp;
    //ALOGI(0,"111111111 freebychan %ld,%d",hcn,psize);
    HgSessionC *sessPtr = (HgSessionC *) ctx;
    hg_chain_node *hcntmp = hcn;
    hg_Buf_Gen_t *hbgttmp = (hg_Buf_Gen_t *) hcntmp->data;
    hbgttmp->freenum--;
    if (hbgttmp->freenum <= 0) {
        FreeChain(hcntmp);
        Hg_ChainDelChain(sessPtr->fragCache, hcn);
    }
}

void HuageRecvStream::FreeChain(hg_chain_node *hcntmp) {
    hg_Buf_Gen_t *hbgttmp = nullptr;
    int seq = 0;
    while (hcntmp != nullptr) {
        hbgttmp = (hg_Buf_Gen_t *) hcntmp->data;
        seq = pktSeq((unsigned char *) hbgttmp->data, RTP_PACKET_HEAD_SIZE);

        HgUdpabstract::WritePipeFree(HuageRecvStream::udpinstan, hbgttmp);
        hcntmp = hcntmp->next;
    }
}

void HuageRecvStream::WritePipeFree(void *ctx, void *data) {
    HgSessionC *sess = (HgSessionC *) ctx;

    int lens = sizeof(hgtEvent) + sizeof(void *);
    char tmp[lens];
    hgtEvent *hgtevent = (hgtEvent *) tmp;
    hgtevent->handle = HuageRecvStream::FreeByChan;
    hgtevent->i = 8;
    //ALOGI(0,"111111111 freebychan write to %ld  %d",data,lens-sizeof(hgtEvent));
    hgtevent->ctx = ctx;
    hgtevent->psize = lens - sizeof(hgtEvent);
    *((void **) ((char *) tmp + sizeof(hgtEvent))) = data;

    HgWorker *hgworker = HgWorker::GetWorker(sess->ssrc);
    hgworker->WriteChanWor(tmp, lens);
}

HuageRecvStream::~HuageRecvStream() {
}

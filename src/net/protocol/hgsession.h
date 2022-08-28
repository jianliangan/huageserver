#ifndef _HUAGE_PROTOCOL_SESSION_

#define _HUAGE_PROTOCOL_SESSION_
#include "../common/extern.h"
#include "hg_channel_stru.h"
#include "core/tools.h"
#include "core/hg_pool.h"
#include "rtp_packet.h"
#include <cassert>
#include <string>
#include <unordered_map>
#include "core/hg_buf.h"
#define sess_cache_LEN 200
#define USETCP 1
#define USEUDP 2

#define FIX_TCP 1
#define FIX_UDP 2
#define UDP_TCP 3
#define TCP_UDP 4

#define MAXBIGFRAMELEN   2097152

typedef void (*SESSION_CALLBACK)(void *v);
typedef struct Hg_cacheStru_s {
    uint8_t *data;
    void *alloc;

    uint64_t time;
    int size;
    uint8_t count;
    unsigned char sndhead[RTP_PACKET_HEAD_SIZE];
    void reset(){
        data= nullptr;
        alloc= nullptr;
        time=0;
        size=0;
        count=0;
    }
} Hg_cacheStru_s;

class HgSessionC;
class HgFdEvent;
class HgSessBucket {
    public:

        static uint8_t videoproto ;
        static uint8_t audioproto;
        static uint8_t textproto;
        std::unordered_map<uint32_t, HgSessionC *> *bucksmap=nullptr;
        myPool *sockaddrPool=nullptr;//先放这，暂时不知道放哪合适
        HgSessionC *sessFreePtr=nullptr;
};

class HgSessionC {
    public:
        HgSessionC *fnext=nullptr;
        HgFdEvent *tcpfd=nullptr;//tcp use
        void *data=nullptr;
        myPool *fragCache=nullptr;
        sockaddr_in *sa=nullptr;
        HgSessBucket *sessBucketPtr=nullptr;
        hg_chain_t chainbufsS;//tcp send
        media_frame_chain chainbufsR[3];//udp recv
        //下面这6个cache需要优化，暂时不动，2022-01-19
        Hg_cacheStru_s sendTextCache[sess_cache_LEN];//以后优化，占空间太大
        Hg_cacheStru_s recvTextCache[sess_cache_LEN];

        Hg_cacheStru_s sendAudioCache[sess_cache_LEN];//以后优化，占空间太大
        Hg_cacheStru_s recvAudioCache[sess_cache_LEN];

        Hg_cacheStru_s sendVideoCache[sess_cache_LEN];//以后优化，占空间太大
        Hg_cacheStru_s recvVideoCache[sess_cache_LEN];

        uint32_t ssrc=0;
        uint32_t bigVideoframeOffset=0;
        uint32_t bigAudioframeOffset=0;
        uint32_t bigTextframeOffset=0;

        uint16_t curVideoSndPacketSeqs=0;
        uint16_t curVideoPacketSeqs=0;
        uint16_t curVideoMaxSeq=0;

        uint16_t curAudioSndPacketSeqs=0;
        uint16_t curAudioPacketSeqs=0;
        uint16_t curAudioMaxSeq=0;

        uint16_t curTextSndPacketSeqs=0;
        uint16_t curTextPacketSeqs=0;
        uint16_t curTextMaxSeq=0;

        uint16_t curSendTextMaxSeq=0;
        uint16_t curSendAudioMaxSeq=0;
        uint16_t curSendVideoMaxSeq=0;
        uint8_t status=0;
        uint8_t tproto=0;
        uint8_t avproto=0;



        static SESSION_CALLBACK sess_cb;
        static HgSessionC *sessGetFreeSess(HgSessBucket *hgsessbu);
        static int sessSetFreeSess(HgSessBucket *hgsessbu,HgSessionC *sess);
        static int sessFree(HgSessBucket *hgsessbu,HgSessionC *sess);

        static HgSessionC *sessGetSessInBuck(HgSessBucket *hgsessbu,uint32_t ssrc);

        static void sessInsertBucket(HgSessBucket *hgsessbu,uint32_t ssrc, HgSessionC *sess);
        static void sessDelBucket(HgSessBucket *hgsessbu, uint32_t ssrc);

        static void sessInit(int sessNum,HgSessBucket **hgsessbu);

        static void sessAttachAddr(HgSessBucket *hgsessbu,HgSessionC *sess, void *sa, int size);

        static void sessSetStatus(HgSessionC *sess, uint8_t status);
    static void sessSetCurSeq(uint16_t *seq,int val,int path);
        static void
            sessRecvPtrStreamUdp(HgSessionC *sess, PLTYPE st, media_frame_chain **recvMergebuf,
                    Hg_cacheStru_s **recvcache, uint16_t **seq,
                    uint32_t **offset, uint16_t **maxseq);

        static void
            sessSendPtrStreamUdp(HgSessionC *sess, PLTYPE st, Hg_cacheStru_s **sendcache,
                    uint16_t **maxseq);
        static void
            sessSendPtrStreamTCP(HgSessionC *sess, hg_chain_t **recvMergebuf);
        void
            static sessSendPtrStreamSeq(HgSessionC *sess, PLTYPE st, uint16_t **seq);
        static HgSessionC *ActiveSess(HgSessBucket *hgsessbu,uint32_t ssrc);
        static void *AllocMemery_local(HgSessionC *sess, int size);
        static void FreeMemery_local(HgSessionC *sess,void *data);
};

#endif


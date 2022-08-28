#include "hgsession.h"
#include <unordered_map>
#include <utility>
#include "rtp_packet.h"

uint8_t HgSessBucket::videoproto = UDP_TCP;
uint8_t HgSessBucket::audioproto = UDP_TCP;
uint8_t HgSessBucket::textproto = FIX_TCP;//FIX_TCP;
SESSION_CALLBACK HgSessionC::sess_cb=nullptr;
void HgSessionC::sessInit(int sessNum, HgSessBucket **hgsessbu) {

    HgSessionC *sessFree = (HgSessionC *) malloc(sizeof(HgSessionC) * sessNum);
    //sessFree->fnext=NULL;
    for (int i = 0; i < sessNum - 1; i++) {

        (sessFree + i)->fnext = sessFree + i + 1;
        (sessFree + i)->tproto = USEUDP;
        (sessFree + i)->avproto = USETCP;
        if (HgSessBucket::textproto == FIX_TCP || HgSessBucket::textproto == TCP_UDP)
            (sessFree + i)->tproto = USETCP;
        if (HgSessBucket::audioproto == FIX_UDP || HgSessBucket::audioproto == UDP_TCP) {
            (sessFree + i)->avproto = USEUDP;
        }
        (sessFree + i)->fragCache = myPoolCreate(2046);
        //(sessFree+i)->test=new std::string();
    }
    (sessFree + sessNum - 1)->fnext = NULL;


    *hgsessbu = (HgSessBucket *) malloc(sizeof(HgSessBucket));


    (*hgsessbu)->sockaddrPool = myPoolCreate(512);
    (*hgsessbu)->sessFreePtr = sessFree;
    (*hgsessbu)->bucksmap = new std::unordered_map<uint32_t, HgSessionC *>();

}

HgSessionC *HgSessionC::sessGetFreeSess(HgSessBucket *hgsessbu) {
    HgSessionC *re= nullptr;
    HgSessionC *freePtr = hgsessbu->sessFreePtr;
    re = freePtr;
    freePtr = re->fnext;
    re->bigVideoframeOffset = 0;
    re->curVideoPacketSeqs = 0xff;


    re->bigAudioframeOffset = 0;
    re->curAudioPacketSeqs = 0xff;


    re->bigTextframeOffset = 0;
    re->curTextPacketSeqs = 0xff;

    re->tcpfd = nullptr;
    re->chainbufsR[0].init();
    re->chainbufsR[1].init();
    re->chainbufsR[2].init();
    re->chainbufsS.init();
    re->fnext= nullptr;

    re->sa= nullptr;

    memset(re->sendTextCache,'\0',sizeof(Hg_cacheStru_s)*sess_cache_LEN);
    memset(re->recvTextCache,'\0',sizeof(Hg_cacheStru_s)*sess_cache_LEN);
    memset(re->sendAudioCache,'\0',sizeof(Hg_cacheStru_s)*sess_cache_LEN);
    memset(re->recvAudioCache,'\0',sizeof(Hg_cacheStru_s)*sess_cache_LEN);
    memset(re->sendVideoCache,'\0',sizeof(Hg_cacheStru_s)*sess_cache_LEN);
    memset(re->recvVideoCache,'\0',sizeof(Hg_cacheStru_s)*sess_cache_LEN);
    HgSessionC::sess_cb(re->data);
    return re;

}

int HgSessionC::sessSetFreeSess(HgSessBucket *hgsessbu, HgSessionC *sess) {
    sess->fnext = hgsessbu->sessFreePtr;
    hgsessbu->sessFreePtr = sess;
    return 0;
}

int HgSessionC::sessFree(HgSessBucket *hgsessbu, HgSessionC *sess) {

    sessSetFreeSess(hgsessbu, sess);


    Hg_ChainDelChain(sess->fragCache, sess->chainbufsR->hct.left);
    Hg_ChainDelChain(sess->fragCache, (sess->chainbufsR + 1)->hct.left);
    Hg_ChainDelChain(sess->fragCache, (sess->chainbufsR + 2)->hct.left);
    sess->chainbufsR->hct.left= nullptr;
    sess->chainbufsR->hct.left= nullptr;
    (sess->chainbufsR + 1)->hct.left= nullptr;
    (sess->chainbufsR + 1)->hct.left= nullptr;

    (sess->chainbufsR + 2)->hct.left= nullptr;
    (sess->chainbufsR + 2)->hct.left= nullptr;
    sess->bigVideoframeOffset = 0;


    sess->curVideoMaxSeq = 0;

    sess->bigAudioframeOffset = 0;


    sess->curAudioMaxSeq = 0;

    sess->bigTextframeOffset = 0;


    sess->curTextMaxSeq = 0;

    //sess->test->clear();
    sessDelBucket(hgsessbu, sess->ssrc);
    sess->ssrc = 0;

    return 0;

}

HgSessionC *HgSessionC::sessGetSessInBuck(HgSessBucket *hgsessbu, uint32_t ssrc) {

    HgSessionC *sess = nullptr;
    std::unordered_map<uint32_t, HgSessionC *>::iterator bmapiter;

    std::unordered_map < uint32_t, HgSessionC * > *bmap= nullptr;
    bmap = hgsessbu->bucksmap;
    bmapiter = bmap->find(ssrc);
    if (bmapiter != bmap->end()) {
        sess = bmapiter->second;
    }

    return sess;
}

void HgSessionC::sessInsertBucket(HgSessBucket *hgsessbu, uint32_t ssrc, HgSessionC *sess) {
    std::unordered_map < uint32_t, HgSessionC * > *bmap= nullptr;
    bmap = hgsessbu->bucksmap;
    bmap->insert(std::unordered_map<uint32_t, HgSessionC *>::value_type(ssrc, sess));
}

void HgSessionC::sessDelBucket(HgSessBucket *hgsessbu, uint32_t ssrc) {

    HgSessionC *sess = nullptr;
    std::unordered_map<uint32_t, HgSessionC *>::iterator bmapiter;

    std::unordered_map < uint32_t, HgSessionC * > *bmap;

    bmap = hgsessbu->bucksmap;

    bmapiter = bmap->find(ssrc);
    if (bmapiter != bmap->end()) {
        sess = bmapiter->second;
    } else {

        assert(false);
        return;
    }

    myPoolFree(hgsessbu->sockaddrPool, (uint8_t *) sess->sa);
    bmap->erase(bmapiter);


}

void HgSessionC::sessAttachAddr(HgSessBucket *hgsessbu, HgSessionC *sess, void *sa, int size) {

    void *sockaddr = myPoolAlloc(hgsessbu->sockaddrPool, size);
    memcpy(sockaddr, sa, size);
    sess->sa = (sockaddr_in *) sockaddr;

}

void HgSessionC::sessSetStatus(HgSessionC *sess, uint8_t status) {
    sess->status = status;
}
void HgSessionC::sessSetCurSeq(uint16_t *seq,int val,int path){
    *seq=val;
}
void
HgSessionC::sessRecvPtrStreamUdp(HgSessionC *sess, PLTYPE st, media_frame_chain **recvMergebuf,
        Hg_cacheStru_s **recvcache, uint16_t **seq,
        uint32_t **offset, uint16_t **maxseq) {
    switch (st) {
        case ENU_PLTYPETEXT:

            *seq = &sess->curTextPacketSeqs;
            *offset = &sess->bigTextframeOffset;
            *recvMergebuf = sess->chainbufsR;

            *recvcache = sess->recvTextCache;
            *maxseq = &sess->curTextMaxSeq;
            break;
        case ENU_PLTYPEAUDIO:

            *seq = &sess->curAudioPacketSeqs;
            *offset = &sess->bigAudioframeOffset;
            *recvMergebuf = sess->chainbufsR + 1;

            *recvcache = sess->recvAudioCache;
            *maxseq = &sess->curAudioMaxSeq;
            break;
        case ENU_PLTYPEVIDEO:

            *seq = &sess->curVideoPacketSeqs;
            *offset = &sess->bigVideoframeOffset;
            *recvMergebuf = sess->chainbufsR + 2;

            *recvcache = sess->recvVideoCache;
            *maxseq = &sess->curVideoMaxSeq;
            break;
        case ENU_PLTYPENONE:
            ;
        default:
            ;
    }

}

void
HgSessionC::sessSendPtrStreamUdp(HgSessionC *sess, PLTYPE st, Hg_cacheStru_s **sendcache,
        uint16_t **maxseq) {
    switch (st) {
        case ENU_PLTYPETEXT:
            *sendcache = sess->sendTextCache;
            *maxseq = &sess->curSendTextMaxSeq;
            break;
        case ENU_PLTYPEAUDIO:
            *sendcache = sess->sendAudioCache;
            *maxseq = &sess->curSendAudioMaxSeq;
            break;
        case ENU_PLTYPEVIDEO:
            *sendcache = sess->sendVideoCache;
            *maxseq = &sess->curSendVideoMaxSeq;
            break;
        default:
            ;
    }

}

void
HgSessionC::sessSendPtrStreamTCP(HgSessionC *sess, hg_chain_t **sendMergebuf) {
    *sendMergebuf = &sess->chainbufsS;
}
void
HgSessionC::sessSendPtrStreamSeq(HgSessionC *sess, PLTYPE st, uint16_t **seq) {
    switch (st) {
        case ENU_PLTYPETEXT:
            *seq=&sess->curTextSndPacketSeqs;
            break;
        case ENU_PLTYPEAUDIO:
            *seq=&sess->curAudioSndPacketSeqs;
            break;
        case ENU_PLTYPEVIDEO:
            *seq=&sess->curVideoSndPacketSeqs;
            break;
        default:
            ;
    }

}
HgSessionC *HgSessionC::ActiveSess(HgSessBucket *hgsessbu, uint32_t ssrc) {
    HgSessionC *tmpsess = HgSessionC::sessGetSessInBuck(hgsessbu, ssrc);
    if (tmpsess == nullptr) {
        //The lowest level session,it will go up to the upper level and provide storage
        tmpsess = HgSessionC::sessGetFreeSess(hgsessbu);
        if (tmpsess == nullptr) {
            return nullptr;
        }
        tmpsess->ssrc = ssrc;
        HgSessionC::sessInsertBucket(hgsessbu, ssrc, tmpsess);
    }
    return tmpsess;
}
void *HgSessionC::AllocMemery_local(HgSessionC *sess, int size) {
    return myPoolAlloc(sess->fragCache, size);
}
void HgSessionC::FreeMemery_local(HgSessionC *sess,void *data) {
    myPoolFree(sess->fragCache, (uint8_t *)data);
}

#include "session.h"
#include <unordered_map>
#include <utility>
SessionC *SessionC::sessInit(int sessNum) {

    SessionC *sessFree = (SessionC *) malloc(sizeof(SessionC) * sessNum);

    for (int i = 0; i < sessNum; i++) {
        (sessFree + i)->bySubscList = nullptr;
        (sessFree + i)->meSubList = nullptr;


    }
    return sessFree;
}


void SessionC::sessFree(void *sess) {
    SessionC *re=(SessionC *)sess;
    re->status = SESSCLOSE;

    re->aconfigt.init();
    re->cpts=0;
    re->spts=0;
    memset(re->name,'\0',sizeof(re->name));
    memset(re->framecache,'\0',sizeof(re->framecache)*sizeof(int16_t));
    memset(re->framesize,'\0',sizeof(re->framesize));
    memset(re->vptsmsgca,'\0',sizeof(re->vptsmsgca));
    memset(re->views,'\0',sizeof(re->views)*sizeof(SessionC *));
    re->viewsn=0;
    re->rroom=nullptr;
    re->roomid=0;
    re->RooEntered=false;
    re->flags=0;
    if(re->bySubscList==nullptr){
        re->bySubscList=new std::unordered_map<uint32_t, SessSubInfo>();
    }
    if(re->meSubList==nullptr){
        re->meSubList=new std::unordered_map<uint32_t, SessSubInfo>();
    }

    re->bySubscList->clear();
    re->meSubList->clear();
    re->ssrc=0;

}

void SessionC::sessAddSubApi(SessionC *sess, SessionC *bysess,uint32_t ssrc, uint8_t flag) {
    std::unordered_map <uint32_t, SessSubInfo> *mesubptr;
    SessSubInfo ssi;
    ssi.flag = flag;
    ssi.sess = bysess;
    mesubptr = sess->meSubList;
    mesubptr->insert(std::make_pair(ssrc, ssi));
}

void SessionC::sessDelSubApi(SessionC *sess, SessionC *bysess) {
    std::unordered_map <uint32_t, SessSubInfo> *mesubptr;
    mesubptr = sess->meSubList;
    mesubptr->erase(bysess->ssrc);
}

void SessionC::sessAddBySubApi(SessionC *sess,uint32_t ssrc, SessionC *bysess, uint8_t flag) {
    std::unordered_map <uint32_t, SessSubInfo> *mebysubptr;
    SessSubInfo ssi;
    ssi.flag = flag;
    ssi.sess = sess;

    mebysubptr = bysess->bySubscList;
    mebysubptr->insert(std::make_pair(ssrc, ssi));

}

void SessionC::sessDelBySubApi(SessionC *sess, SessionC *bysess) {
    std::unordered_map <uint32_t, SessSubInfo> *mebysubptr;
    mebysubptr = bysess->bySubscList;
    mebysubptr->erase(sess->ssrc);

}

void SessionC::sessSetStatus(SessionC *sess, sessStatus status) {
    sess->status = status;
}
void SessionC::sessSetAudioConf(SessionC *sess,int chan,int deepn,int samrate,int frlen,int codec){
    sess->aconfigt.channels=chan;
    sess->aconfigt.deep=deepn;
    sess->aconfigt.samplesrate=samrate;
    sess->aconfigt.frameperbuf=frlen;
    sess->aconfigt.codec=(AUDIO_codecs)codec;
}

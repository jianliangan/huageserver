//
// Created by ajl on 2021/10/26.
//
#include "hgmain.h"
#include "hgsession.h"


uint32_t HgmainPro_GetSessVFSize(HgSessionC *sess){
    return sess->bigVideoframeOffset;
}
uint32_t HgmainPro_GetSessAFSize(HgSessionC *sess){
    return sess->bigAudioframeOffset;
}
uint32_t HgmainPro_GetSessTFSize(HgSessionC *sess){
    return sess->bigTextframeOffset;
}
void *HgmainPro_GetSessData(HgSessionC *sess){
    return sess->data;
}
void HgmainPro_FreeSess(HgSessBucket *hgsessbu,HgSessionC *sess){
    HgSessionC::sessFree(hgsessbu,sess);
}
struct sockaddr_in *HgmainPro_GetSessAddr(HgSessionC *sess) {
    return sess->sa;
}

void HgmainPro_CloseSess(HgSessBucket *hgsessbu,HgSessionC *sess){
    HgSessionC::sessFree(hgsessbu,sess);
}

void *HgmainPro_Alloc(HgSessionC *sess,int size){
return HgSessionC::AllocMemery_local(sess,size);
}
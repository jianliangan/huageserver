#include "../base/json.hpp"
#include "comm_consumer.h"
#include "app.h"
#include "session.h"
#include "../net/common/hg_netabstract.h"
using namespace std;
CommConsumer::CommConsumer(){
    int err;

}
void CommConsumer::AsyncLogin(void *ctx, media_frame_chain *mfc,HgSessionC *hgsess){

    RecvCallback0(ctx, mfc, hgsess);
}
void CommConsumer::RecvCallback0(void *ctx,media_frame_chain *mfc,HgSessionC *sess){

    CommConsumer *context=(CommConsumer *)ctx;
    SessionC *sessPtr=(SessionC *)HgmainPro_GetSessData(sess);
    int size=mfc->size;
    hg_chain_node *hcn=mfc->hct.left;
    //uint32_t ssrc=pktSsrc(framedata,size);//组合起来后的大包都带rtp头
    if(size<=RTP_PACKET_HEAD_SIZE)
        return;

    char jsonblock[MAX_TEXT_BUF_SIZE]={0};
    hg_Buf_Gen_t *hbgt=nullptr;

    uint32_t ssrc=0;
    if(hcn!=nullptr){
        hbgt=(hg_Buf_Gen_t *)hcn->data;
        ssrc=pktSsrc((uint8_t *)hbgt->data,14);
    }
    int total=0;
    while (hcn!=nullptr) {
        hbgt=(hg_Buf_Gen_t *)hcn->data;
        memcpy((char *)jsonblock+total,(char *)hbgt->data+hbgt->start,hbgt->len);
        total+=hbgt->len;
        hcn=hcn->next;
    }

    hg_chain_node *hcnR=mfc->hct.right;
    if(hcnR!=nullptr){
        hg_Buf_Gen_t *hbgt=(hg_Buf_Gen_t *)hcnR->data;
        hbgt->sfree.freehandle(hbgt->sfree.ctx,hbgt->sfree.params);
    }

    uint32_t length;
    uint32_t uniqid;
    uint32_t version;
    std::string uri;
    std::string body;
    int size1=size-RTP_PACKET_HEAD_SIZE;
    std::string err;
    nlohmann::json json;
    try{
        json = nlohmann::json::parse(std::string(jsonblock,size1));
    }catch(nlohmann::json::parse_error& e){

        ALOGI(0,"json post recv format error %s",e.what());
        return ;
    }
    try{
        version=json.at("v");
        uniqid=json.at("i");
        uri=json.at("c");
    }catch(nlohmann::json::type_error& e){
        ALOGI(0,"json post recv format error %s",e.what());
    }catch(nlohmann::json::out_of_range& e){
        ALOGI(0,"json post recv format error %s",e.what());
    }
    uint32_t roomid=0;
    int deep=0;
    int samplesrate=0;
    int framelen=0;
    int channels=0;
    AUDIO_codecs codec;

    const char *addbody=nullptr;
    char resbody[MAX_TEXT_BUF_SIZE];
    int ret;
    if(uri=="login"){
        context->Login(ssrc,sessPtr);
        sprintf(resbody,"{\"v\":0,\"i\":%u,\"b\":\"ok\"}",uniqid);
        length=strlen(resbody);

        hg_chain_t hct;
        int mfcs=HgNetabstract::CreateFrameStr(sess->fragCache,&hct, sess->ssrc,(int) ENU_PLTYPETEXT,false,(int)CMD_DEFAULT,0,(uint8_t *)resbody, length,HANDLE_MAX_FRAME_SLICE,-1);
        media_frame_chain bigframetmp;
        context->framebufSet(&bigframetmp,0,&hct,mfcs,sess);
        HgNetabstract::PreSendData(hgapp->hgser->hnab, &bigframetmp, ssrc,
                (int) ENU_PLTYPETEXT,
                true, sess, false, nullptr,0);
        return;

    }else{
        if(sessPtr==nullptr){
            return;
        }

        if(uri=="createenterroom"){
            try{

                roomid=json.at("req").at("roomid");

                channels=json.at("req").at("chan");
                deep=json.at("req").at("deep");
                samplesrate=json.at("req").at("samrate");
                framelen=json.at("req").at("frlen");
                codec=(AUDIO_codecs)json.at("req").at("codec");
            }catch(nlohmann::json::type_error& e){
                ALOGI(0,"json error %s",e.what());
                return;
            }catch(nlohmann::json::out_of_range& e){
                ALOGI(0,"json error %s",e.what());
            }

            sessPtr->aconfigt.channels=channels;
            sessPtr->aconfigt.deep=deep;
            sessPtr->aconfigt.samplesrate=samplesrate;
            sessPtr->aconfigt.frameperbuf=framelen;
            sessPtr->aconfigt.codec=codec;
            sessPtr->roomid=roomid;
            ret=Room::CreEnterRoom(uniqid,roomid,sessPtr,true);



        }
        else if(uri=="enterroom"){
            //ret=Room::CreEnterRoom(uniqid,roomid,sessPtr,false);
        }
    }
}
void CommConsumer::RecvCallback(void *ctx,media_frame_chain *mfc,HgSessionC *sess){

    RecvCallback0(ctx,mfc,sess);
}
//void *ctx, uint8_t *fragdata, uint32_t size, SessionC *sess
void CommConsumer::framebufSet(media_frame_chain *mfc,uint16_t time,hg_chain_t *hct,int length,void *ctx){
    mfc->size = length;
    mfc->hct = *hct;
    mfc->pts=time;
    mfc->sfree.params = nullptr;
    mfc->sfree.ctx = nullptr;
    mfc->sfree.freehandle = nullptr;

    hg_chain_node *hcnR=mfc->hct.right;
    hg_Buf_Gen_t *hbgtR=(hg_Buf_Gen_t *)hcnR->data;
    hbgtR->sfree.ctx=ctx;
    hbgtR->sfree.params=hct->left;
    hbgtR->sfree.freehandle=CommConsumer::WriteFree;
}
//Logout
void CommConsumer::SessOut(SessionC *sess){
    SessionC::sessFree(sess);
}

//Login
void CommConsumer::Login(uint32_t ssrc,SessionC *sess){//
    //Login
    SessionC::sessSetStatus(sess,SESSLOGIN);
    //test sub 
    sess->ssrc=ssrc;
    uint32_t byssrc=ssrc;
    SessionC *bysess=nullptr;
    bysess=sess;
    /****
      SessionC::sessAddSubApi(sess,bysess,sess_sub_AUDIO|sess_sub_VIDEO);
      SessionC::sessAddBySubApi(sess,bysess,sess_sub_AUDIO|sess_sub_VIDEO);
     ****/
    SessionC::sessAddSubApi(sess,bysess,bysess->ssrc,sess_sub_AUDIO|sess_sub_VIDEO);

    //async  other user sess
    int lens = sizeof(hgtEvent) + sizeof(sever_packet_login_pa);
    char hgteventptr[lens];
    sever_packet_login_pa *uprp = (sever_packet_login_pa *)(hgteventptr+sizeof(hgtEvent));
    uprp->ssrc=sess->ssrc;
    uprp->sess=sess;
    uprp->bysess=bysess;


    hgtEvent *hgtevent=(hgtEvent *)hgteventptr;

    hgtevent->handle=CommConsumer::WorkHanLogin;
    hgtevent->psize=lens-sizeof(hgtEvent);
    hgtevent->ctx =this;
    hgtevent->i=9;
    HgWorker *hgworker=HgWorker::GetWorker(bysess->ssrc);
    hgworker->WriteChanWor(hgteventptr,lens);
    //async end

}
void CommConsumer::WorkHanLogin(void *pth,void *ctx,void *params,int psize){
    sever_packet_login_pa *uprp=(sever_packet_login_pa *)params;
    uint32_t ssrc=uprp->ssrc;

    SessionC::sessAddBySubApi((SessionC *)uprp->sess,uprp->ssrc,(SessionC *)uprp->bysess,sess_sub_AUDIO|sess_sub_VIDEO);

}
void CommConsumer::Logout(SessionC *sess){
    std::unordered_map<uint32_t,SessSubInfo> *msl;
    std::unordered_map<uint32_t,SessSubInfo> *bsl;

    std::unordered_map<uint32_t,SessSubInfo>::iterator mslit;
    std::unordered_map<uint32_t,SessSubInfo>::iterator bslit;
    msl=new std::unordered_map<uint32_t,SessSubInfo>;
    bsl=new std::unordered_map<uint32_t,SessSubInfo>;
    msl->insert(sess->meSubList->begin(),sess->meSubList->end());
    bsl->insert(sess->bySubscList->begin(),sess->bySubscList->end());
    SessionC *tmpsess=nullptr;
    //1、删除我订阅的人的被订阅列表，删除别人的被我订阅的被订阅列表
    for ( mslit = msl->begin() ; mslit != msl->end(); ++mslit)
    {
        SessSubInfo *ssif=&mslit->second;
        tmpsess=ssif->sess;
        SessionC::sessDelBySubApi(sess,tmpsess);
    }
    //2、删除我被订阅列表的订阅列表,删除别人的订阅我的订阅列表
    for ( bslit = bsl->begin() ; bslit != bsl->end(); ++bslit)
    {
        SessSubInfo *ssif=&bslit->second;
        tmpsess=ssif->sess;
        SessionC::sessDelSubApi(tmpsess,sess);
    }

    SessOut(sess);
}
bool CommConsumer::Verify_auth(void *ctx,HgSessionC *sess ,int fd) {
    //判断是否是服务器发来的
    SessionC *sessc=(SessionC *)HgmainPro_GetSessData(sess);
    if(sessc->status==SESSLOGIN){
        return true;
    }else{
        return false;
    }
}
void CommConsumer::WriteFree(void *ctx, void *data) {
    HgSessionC *sess = (HgSessionC *) ctx;

    int lens = sizeof(hgtEvent) + sizeof(void *);
    char tmp[lens];
    hgtEvent *hgtevent = (hgtEvent *) tmp;
    hgtevent->handle = CommConsumer::FreeByChan;

    hgtevent->ctx = ctx;
    hgtevent->i=10;
    hgtevent->psize = lens-sizeof(hgtEvent);
    *((void **) ((char *) tmp + sizeof(hgtEvent))) = data;

    HgWorker *hgworker = HgWorker::GetWorker(sess->ssrc);
    hgworker->WriteChanWor(tmp, lens);
}

void CommConsumer::FreeByChan(void *pth, void *ctx, void *params, int psize) {
    hg_chain_node **ptmp = (hg_chain_node **) params;
    hg_chain_node *hcn=*ptmp;


    HgSessionC *hcl = (HgSessionC *) ctx;
    while (hcn != nullptr) {
        myPoolFree(hcl->fragCache, (uint8_t *)hcn);
        hcn = hcn->next;
    }
}




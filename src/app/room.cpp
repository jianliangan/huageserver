//
// Created by ajl on 2021/9/14.
//

#include "room.h"
#include "app.h"
#include "../net/protocol/hg_channel_stru.h"
#include "../net/common/hg_netabstract.h"
#include "../net/threads/hg_worker.h"
#include "../net/common/hg_netcommon.h"
#include "session.h"
#include "../net/server.h"
#include <unordered_map>
audio_config_t Room::audioconfig;
Room::Room(uint32_t roomid){



    audioCaNum=audioconfig.samplesrate/audioconfig.frameperbuf;//1秒分成几段
    rooBufinfo.size=(char *)malloc(sizeof(char)*audioCaNum);//用来存储加了多少个
    fragCache=myPoolCreate(4096);
    //rooBufinfo.rawcache=(uint8_t *)malloc(audioconfig.channels*audioconfig.samplesrate*audioconfig.deep/8);
    //1秒内非编码原始音频容量
    rooBufinfo.samptota=(int *)malloc(audioconfig.channels*audioconfig.samplesrate*4);//用来存储加以后的和


    id=roomid;
    audioFramRawByte=audioconfig.frameperbuf*audioconfig.deep*audioconfig.channels >> 3;//一帧的原始字节数
    audioFrameOutByte=audioconfig.channels*audioconfig.frameperbuf*32/8;//一帧编码后字节数
    for(int i=0;i<audioCaNum;i++){
        *(rooBufinfo.size+i)=0;
    }
    userList=new std::unordered_map<uint32_t, RoomUserInfo>;
}
int Room::subIndex(int newd,int oldd){

    if(newd<oldd){
        int ret;
        ret= (newd-0)+1+(audioCaNum-1)-oldd;
        return ret;
    }else{
        return newd-oldd;
    }
}
void Room::HandleCreEnterRoo(void *pth,void *ctx,void *params,int psize){

    ch_text_room_enter *ctre=(ch_text_room_enter *)params;
    SessionC *sess=ctre->sess;
    bool create=ctre->create;
    uint32_t roomid=ctre->roomid;
    uint32_t uniqid=ctre->uniqid;
    Room *roomptr=CreateRoom(uniqid,roomid,sess,create);
    roomptr->EnterRoom(sess);
    std::unordered_map<uint32_t,RoomUserInfo> *bsl;
    std::unordered_map<uint32_t,RoomUserInfo>::iterator bslit;
    bsl=roomptr->userList;
    /*这里代表用户打算看那些人，以后做成命令触发，这里是临时的写法*/
    sess->views[0]=sess;
    sess->viewsn=1;
    /**/
    nlohmann::json jss=nlohmann::json{
        { "ssrc", (double)sess->ssrc },
            { "room", (double)roomid }};

    nlohmann::json resobj;
    resJsoHead(uniqid,"ok",resobj,jss);
    std::string resbody=resobj.dump();
    int length=resbody.length();
    hg_chain_t hct;
    int mfcs=HgNetabstract::CreateFrameStr(roomptr->fragCache,&hct, sess->ssrc,(int) ENU_PLTYPETEXT,false,(int)CMD_DEFAULT,0,(uint8_t *)resbody.c_str(), length,HANDLE_MAX_FRAME_SLICE,-1);

    media_frame_chain bigframetmp;
    bigframetmp.hct=hct;
    bigframetmp.size=mfcs;
    bigframetmp.pts=0;
    hg_Buf_Gen_t *headhbgtL=(hg_Buf_Gen_t *)hct.left->data;
    hg_chain_node *hcnR=bigframetmp.hct.right;
    hg_Buf_Gen_t *hbgtR=(hg_Buf_Gen_t *)hcnR->data;
    hbgtR->sfree.ctx=roomptr;
    hbgtR->sfree.params=bigframetmp.hct.left;
    hbgtR->sfree.freehandle=Room::WriteFree;



    for (bslit = bsl->begin() ; bslit != bsl->end(); ++bslit)
    {
        SessionC *itsess=(SessionC *)bslit->second.sess;
        if(itsess->flags&0x01==1){
            HgNetabstract::PreSendData(hgapp->hgser->hnab, &roomptr->mfcULst, itsess->ssrc,
                    (int) ENU_PLTYPETEXT,
                    false, itsess->data,  false, nullptr,0);

        }else{
            headhbgtL->freenum++;
            HgNetabstract::PreSendData(hgapp->hgser->hnab, &bigframetmp, itsess->ssrc,
                    (int) ENU_PLTYPETEXT,
                    false, itsess->data, false, nullptr,0);
            itsess->flags=itsess->flags|0x01;

        }
    }


}
int Room::CreEnterRoom(unsigned int uniqid,unsigned int roomid,SessionC *sess,bool create) {
    //this thread scope set sess
    if (sess->status != SESSLOGIN) {
        return -1;
    }
    //this thread scope set room



    int lens = sizeof(hgtEvent) + sizeof(ch_text_room_enter);
    char hgteventptr[lens];
    hgtEvent *hgtevent = (hgtEvent *) hgteventptr;
    ch_text_room_enter *ctre = (ch_text_room_enter *) (hgteventptr + sizeof(hgtEvent));
    ctre->roomid=roomid;
    ctre->sess=sess;
    ctre->create=create;
    ctre->uniqid=uniqid;
    hgtevent->handle = Room::HandleCreEnterRoo;
    hgtevent->psize = lens-sizeof(hgtEvent);
    hgtevent->ctx = nullptr;
    hgtevent->i=0;
    HgWorker *hgworker = HgWorker::GetWorker(roomid);
    hgworker->WriteChanWor(hgteventptr, lens);
    return 0;
}
Room *Room::CreateRoom(uint32_t uniqid,int roomid,SessionC *sess,bool create) {
    if (sess->status != SESSLOGIN) {
        return nullptr;
    }
    std::unordered_map<std::string, Room *>::iterator roomit;
    std::unordered_map<uint32_t,RoomUserInfo> *bsl;
    std::unordered_map<uint32_t,RoomUserInfo>::iterator bslit;
    roomit = hgapp->rooms->find(std::to_string(roomid));
    Room *rm=nullptr;
    if (roomit != hgapp->rooms->end()) {
        rm= roomit->second;

        if(create==false){
            return rm;
        }

    }else{
        if(create==false){
            return nullptr;
        }


        rm = new Room(roomid);
        hgapp->rooms->insert(std::make_pair(std::to_string(roomid), rm));

    }
    //uint32_t roomid,audio_config_t *aconft,uint8_t *acache,int acachelen


    bsl=rm->userList;
    std::map <std::string, nlohmann::json> roomvs;
    SessionC *sesstmp=nullptr;
    for (bslit = bsl->begin() ; bslit != bsl->end(); ++bslit){
        sesstmp= bslit->second.sess;
        nlohmann::json jss=nlohmann::json{
            { "ssrc", (double)sesstmp->ssrc },
                { "key2", "ajl" }};
        roomvs.insert(std::make_pair(std::to_string(sesstmp->ssrc), jss));

    }
    nlohmann::json jss=nlohmann::json(roomvs);

    nlohmann::json resobj;
    resJsoHead(uniqid,"ok",resobj,jss);


    std::string resbody=resobj.dump();


    Hg_ChainDelChain(rm->fragCache,rm->mfcULst.hct.left);
    rm->mfcULst.hct.init();
    int mfcs=HgNetabstract::CreateFrameStr(rm->fragCache,&rm->mfcULst.hct, sess->ssrc,(int) ENU_PLTYPETEXT,false,(int)CMD_DEFAULT,0,(uint8_t *)resbody.c_str(), resbody.length(),HANDLE_MAX_FRAME_SLICE,-1);
    rm->mfcULst.size=mfcs;

    return rm;
}
int Room::EnterRoom(SessionC *sess){
    RoomUserInfo rui;
    rui.flag=0;
    sess->rroom = this;
    sess->cpts=0;
    sess->spts=0;
    rui.sess=sess;
    userList->insert(std::make_pair(id,rui));
    return 0;
}
int Room::addIndex(int add1,int add2){
    return (add1+add2)%(audioCaNum);
}
int Room::MergeAudioData(int index,media_frame_chain *mfc,SessionC *sess){
    //uint32_t pts,int8_t *data,int size
    //server_packet_roommerge_pa *sprp=(server_packet_roommerge_pa *)params.data;

    hg_chain_node *hcn=mfc->hct.left;
    int size=mfc->size-RTP_PACKET_HEAD_SIZE;
    if(2*size!=audioFramRawByte){//编码率为0.5
        //    if(3*size!=audioFramRawByte)//24deep
        return -1;//-1
    }

    int offset=index*audioFramRawByte;

    int frameoff=index*(audioconfig.rframeperbuf);

    int *curptrtotal=rooBufinfo.samptota+frameoff;
    int16_t *curptrfrca=sess->framecache+frameoff;
    int16_t val=0;
    int oldval=0;
    //int8_t *valptr=(int8_t *)&val;
    hg_Buf_Gen_t *hbgt;

    char *rsize=rooBufinfo.size+index;
    char *ffsize=sess->framesize+index;
    int rrsize=*rsize;
    int8_t *data=nullptr;
    while(hcn!=nullptr){
        hbgt=(hg_Buf_Gen_t *)hcn->data;
        data=(int8_t *)hbgt->data+hbgt->start;
        for(int j=0;j<hbgt->len;j++)
        {

            val=MuLaw_Decode(data[j]);
            if(rrsize==0){
                oldval=0;
            }else{
                oldval=*curptrtotal;//16deep
            }
            *curptrfrca=val;
            *curptrtotal =oldval+val;
            //val=(*curptrtotal)/(*rbinsize);
            //*curptr=*(valptr);
            //*(curptr+1)=*(valptr+1);//16deep
            //curptr+=2;//16deep
            curptrtotal++;
            curptrfrca++;
        }

        hcn=hcn->next;
    }
    *rsize=1;
    *ffsize=1;

    ///////////////////
    /*
       hg_chain_node *hcnttt=mfc->hct.left;
       hg_Buf_Gen_t *bddfdf=(hg_Buf_Gen_t *)hcnttt->data;
       uint8_t *dddd=(uint8_t *)bddfdf->data;
       uint16_t time=pktTimestamp(dddd,50);
       int *tmp=rooBufinfo.samptota+frameoff;

       int16_t tmpbody[1024]={0};
       char contents[512]={0};
       sprintf(contents, "yyyyyyyyyyyyyyyy %u 3yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\n", time);
       FILE *f2 = fopen(APP_ROOT_"send-1.data", "a+");
       fwrite(contents, 1, strlen(contents), f2);
       for(int i=0;i<size;i++){
       tmpbody[i]=*(tmp+i);

       }
       fwrite(tmpbody, 1, size*2, f2);
       fclose(f2);
       */
    /////////////////////

    return 0;
}
free_frame_chain_a *Room::GetFreeFrame(){
    free_frame_chain_a *re=nullptr;
    if(framefreea!=nullptr){
        re=framefreea;
        framefreea=framefreea->next;


    }else{
        re=free_frame_chain_a::instan(audioconfig.rframeperbuf*2+RTP_PACKET_HEAD_SIZE);
        hg_chain_node *hcn=re->mfc.hct.left;
        hg_Buf_Gen_t *hbgtR=(hg_Buf_Gen_t *)hcn->data;
        hbgtR->sfree.ctx=this;
        hbgtR->sfree.params=re;
        hbgtR->sfree.freehandle=Room::WriteFree2;
        hbgtR->start=RTP_PACKET_HEAD_SIZE;
        hbgtR->len=audioconfig.rframeperbuf*2;
        hbgtR->end=hbgtR->start+hbgtR->len;

    }
    frameusea++;
    return re;
}
void Room::WriteHeader(uint8_t *data,uint32_t ssrc,int pltype,int padding,bool isfirst,int channel,uint32_t time,int cmdtype,int comptype){
    rtpHeader rh;
    rh.ssrc = ssrc;
    rh.payloadType = pltype;

    rh.first = 0;
    if (isfirst) {
        rh.first = 1;
    }



    rh.timestamp = time;
    rh.seq = 0;//到这里了
    rh.tail = 0;
    rh.cmd = cmdtype;
    rh.payloadType = pltype;
    rh.comp=comptype;
    rtpPacketHead(data, &rh);

}
void Room::OutConsumeQueue(char rsize,uint32_t pts,int fromi,SessionC *sess){

    std::unordered_map<uint32_t,RoomUserInfo> *bsl;
    std::unordered_map<uint32_t,RoomUserInfo>::iterator bslit;
    uint32_t ssrc=0;
    uint32_t index=0;
    bool sendmsg0=false;
    SessionC *itsess=nullptr;
    uint64_t curtime=hgetSysTimeMicros();
    if(curtime-lasttime>1000000){
        lasttime=curtime;
        sendmsg0=true;
    }
    bsl=userList;

    int frameoff=fromi*(audioconfig.rframeperbuf);

    int *curptrtotal=rooBufinfo.samptota+frameoff;

    int framebodylen=audioconfig.rframeperbuf*2;

    for (bslit = bsl->begin() ; bslit != bsl->end(); ++bslit)
    {

        itsess=(SessionC *)bslit->second.sess;
        int16_t *curptrfrca=itsess->framecache+frameoff;
        char ssize=*(itsess->framesize+fromi);
        if(itsess->lastvptsmsg){
            free_frame_chain_a *ffca=GetFreeFrame();

            media_frame_chain * mfctmp=&ffca->mfc;
            hg_chain_node *hcntmp=mfctmp->hct.left;
            hg_Buf_Gen_t *hbgttmp=(hg_Buf_Gen_t *)hcntmp->data;

            ALOGI(0,"i4444444444444444444444444 %d ",itsess->lastvptsmsg);

            WriteHeader((uint8_t *)hbgttmp->data,itsess->ssrc,ENU_PLTYPEAUDIO,0,true,0,pts,CMD_DEFAULT,1);

            uint8_t *datatmp=(uint8_t *)((char *)hbgttmp->data+hbgttmp->start);
            int offset=0;
            int datamove=0;
            datatmp[0]=0;
            datatmp[1]=itsess->viewsn;
            datamove=2;
            datatmp+=datamove;
            for(int i=0;i<itsess->viewsn;i++){
                SessionC *tmpsess=itsess->views[i];
                memcpy(datatmp+offset,tmpsess->vptsmsgca,sizeof(tmpsess->vptsmsgca));
                offset+=sizeof(tmpsess->vptsmsgca);
            }
            mfctmp->pts=pts;
            mfctmp->size=offset+datamove+RTP_PACKET_HEAD_SIZE;
            hbgttmp->len=offset+datamove;
            hbgttmp->end=hbgttmp->start+offset+datamove;
            ALOGI(0,"i4444444444444444444444444 %d",mfctmp->size);

            HgNetabstract::PreSendData(hgapp->hgser->hnab, mfctmp, itsess->ssrc,
                    (int) ENU_PLTYPEAUDIO,
                    false, itsess->data,  false, nullptr,1);


        }
        free_frame_chain_a *ffca=GetFreeFrame();

        media_frame_chain * mfctmp=&ffca->mfc;
        hg_chain_node *hcntmp=mfctmp->hct.left;
        hg_Buf_Gen_t *hbgttmp=(hg_Buf_Gen_t *)hcntmp->data;
        mfctmp->pts=pts;
        ALOGI(0,"i4444444444444444444444444 %d ",itsess->lastvptsmsg);

        WriteHeader((uint8_t *)hbgttmp->data,itsess->ssrc,ENU_PLTYPEAUDIO,0,true,0,pts,CMD_DEFAULT,0);

        int16_t *datatmp=(int16_t *)((char *)hbgttmp->data+hbgttmp->start);

        for(int i=0;i<audioconfig.rframeperbuf;i++){
            int total=0;
            if(rsize!=0)
            {
                total=*(curptrtotal+i);
            }
            int curfrca=0;
            if(ssize!=0){
                curfrca=*(curptrfrca+i);
            }
            int cc= total-curfrca;

            if(cc>65535){
                cc=65535;
            }
            *datatmp=total;
            datatmp++;

        }
        *(itsess->framesize+fromi)=0;
        itsess->lastvptsmsg=false;
        //ALOGI(0,"yyyyyyyyyyyyyyyyyyyyyyyyyrrrrr size %d pts %d",mfctmp->size,pts);

        mfctmp->size=framebodylen+RTP_PACKET_HEAD_SIZE;
        hbgttmp->len=framebodylen;
        hbgttmp->end=hbgttmp->start+framebodylen;

        HgNetabstract::PreSendData(hgapp->hgser->hnab, mfctmp, itsess->ssrc,
                (int) ENU_PLTYPEAUDIO,
                false, itsess->data,  false, nullptr,0);
    }


}
void Room::RoomMergeData(void *pth,void *ctx,void *params,int psize){
    server_packet_roommerge_pa *sprp=(server_packet_roommerge_pa *)params;
    SessionC *sess=sprp->sess;
    uint32_t framepts=sprp->pts;
    uint32_t ssrc=sprp->ssrc;
    int comp=sprp->comp;
    media_frame_chain *mfc=&sprp->mfc;

    hg_chain_node *hcnR=mfc->hct.right;
    hg_Buf_Gen_t *hbgtR=(hg_Buf_Gen_t *)hcnR->data;
    param_handle freeHandle=hbgtR->sfree.freehandle;

    Room *room=(Room *)sprp->room;

    hg_chain_node *hcntmp=mfc->hct.left;
    hg_Buf_Gen_t *hbgttmp= nullptr;
    /*
       char tttt[1000];
       FILE *f2 = fopen(APP_ROOT_"recv0-.data", "a+");
       sprintf(tttt, "yyyyyyyyyyyyyyyy %d %d %dyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\n", 1, framepts,3);
       fwrite(tttt, 1, strlen(tttt), f2);

       while(hcntmp!= nullptr){
       hbgttmp=(hg_Buf_Gen_t *)hcntmp->data;
       fwrite((char *)hbgttmp->data + hbgttmp->start, 1, hbgttmp->len, f2);
       hcntmp=hcntmp->next;
       }
       fclose(f2);
       */
    ALOGI(0,"rrrrrrrrrrrrrrrrrrrrrrrrrrrruuuuuuuuuuuuuuu comp %d size %d ",comp,mfc->size);

    if(comp==1){
        if(mfc->size-RTP_PACKET_HEAD_SIZE==SINGLE_MSG0_LEN+sizeof(MsgObjHead)){
            ALOGI(0,"rrrrrrrrrrrrrrrrrrrrrrrrrrrruuuuuuuuuuuuuuu comp88 %d size %d ",comp,mfc->size);
            MsgObj0 msgarr;
            hg_chain_node *hcntmp=mfc->hct.left;
            hg_Buf_Gen_t *hbgttmp= nullptr;

            int offset=0;
            while(hcntmp!= nullptr){
                hbgttmp=(hg_Buf_Gen_t *)hcntmp->data;
                int datamove=0;
                if(hbgttmp->start!=0){
                    datamove=sizeof(MsgObjHead);
                }
                memcpy(sess->vptsmsgca + offset, (char *)hbgttmp->data+hbgttmp->start+datamove, hbgttmp->len-datamove);
                hcntmp=hcntmp->next;
                offset+=hbgttmp->len;
            }
            offset-=sizeof(MsgObjHead);
            sess->lastvptsmsg=true;
            MsgObj0::decodeone(sess->vptsmsgca,offset,&msgarr);

            uint16_t apts=msgarr.apts;
            uint16_t vpts=msgarr.vpts;

        }


gore:
        freeHandle(hbgtR->sfree.ctx,hbgtR->sfree.params);
        return;
    }
    if(framepts!=0&&uint16Sub(framepts,sess->cpts)<=0){

        freeHandle(hbgtR->sfree.ctx,hbgtR->sfree.params);
        return ; //-1
    }
    uint32_t realpts=0;
    uint64_t curtime=hgetSysTimeMicros();
    bool first=false;
    if(room->startplay==0){
        room->startplay=curtime;
        first=true;
    }
    uint64_t diffptsmic=curtime-room->startplay;
    uint32_t diffpts=(diffptsmic*room->audioCaNum)/1000000;

    if(first){

        room->nextptsplay=0;
        sess->cpts=framepts;
        sess->spts=diffpts;
    }
    uint32_t sess_spts=0;
    uint32_t sess_cpts=0;

    sess_cpts=sess->cpts;
    sess_spts=sess->spts;

    if(uint16Sub(sess_spts,diffpts)>=0){
        realpts=uint16Add(sess_spts,uint32Sub(framepts,sess_cpts));//相当于当音频流产生缓冲时，保存在缓冲后面
    }else{
        realpts=diffpts;//没有缓冲时直接播放，整个效果类似播放器的缓冲器原理
    }
    if(first){
        room->nextptsplay=realpts;
    }
    if(uint16Sub(realpts,room->nextptsplay)>=room->audioCaNum){//大于缓冲就扔掉了
        ALOGI(0,"ignored pts too big realpts real %d next %d",realpts,room->nextptsplay);
    }else{
        //这块得加一个写保护，不能覆盖缓冲中的数据，必要时加cache
        int index=realpts%room->audioCaNum;
        room->MergeAudioData(index,mfc,sess);                

    }
    //2 base on spts merge packet in server
    //for(int i=0;i<1;i++){
    sess->spts=realpts;
    sess->cpts=framepts;
    // }
    //clear data

    freeHandle(hbgtR->sfree.ctx,hbgtR->sfree.params);
    //end clear

    //3 find need to send frames ,first,tag self lock

    if(uint16Sub(diffpts,room->nextptsplay)>0){
        int ret;
        uint32_t nextpts=room->nextptsplay;
        int nexti;


        do{

            nexti=nextpts%room->audioCaNum;
            char rbinsize=*(room->rooBufinfo.size+nexti);

            if(rbinsize!=0){
                room->OutConsumeQueue(rbinsize,nextpts,nexti,sess);
            }

            nextpts=uint16Add(nextpts,1);
            *(room->rooBufinfo.size+nexti)=0;

            if(nextpts==diffpts){
                room->nextptsplay=diffpts;
                break;
            }


        }while(true);
    }
}
void Room::WriteFree2(void *ctx, void *data) {
    Room *room = (Room *) ctx;

    int lens = sizeof(hgtEvent) + sizeof(void *);
    char tmp[lens];
    hgtEvent *hgtevent = (hgtEvent *) tmp;
    hgtevent->handle = Room::FreeByChan2;
    hgtevent->i=12;
    hgtevent->ctx = ctx;
    hgtevent->psize = lens-sizeof(hgtEvent);
    *((void **) ((char *) tmp + sizeof(hgtEvent))) = data;

    HgWorker *hgworker = HgWorker::GetWorker(room->id);
    hgworker->WriteChanWor(tmp, lens);
}

void Room::WriteFree(void *ctx, void *data) {
    Room *room = (Room *) ctx;

    int lens = sizeof(hgtEvent) + sizeof(void *);
    char tmp[lens];
    hgtEvent *hgtevent = (hgtEvent *) tmp;
    hgtevent->handle = Room::FreeByChan;
    hgtevent->i=11;
    hgtevent->ctx = ctx;
    hgtevent->psize = lens-sizeof(hgtEvent);
    *((void **) ((char *) tmp + sizeof(hgtEvent))) = data;

    HgWorker *hgworker = HgWorker::GetWorker(room->id);
    hgworker->WriteChanWor(tmp, lens);
}
void Room::FreeByChan(void *pth, void *ctx, void *params, int psize) {
    hg_chain_node **ptmp = (hg_chain_node **) params;
    hg_chain_node *hcn=*ptmp;

    hg_Buf_Gen_t *hbgttmp = (hg_Buf_Gen_t *) hcn->data;
    hbgttmp->freenum--;

    if(hbgttmp->freenum<=0){
        Room *hcl = (Room *) ctx;
        while (hcn != nullptr) {
            myPoolFree(hcl->fragCache, (uint8_t *)hcn);
            hcn = hcn->next;
        }
    }
}
void Room::FreeByChan2(void *pth, void *ctx, void *params, int psize) {
    Room *room=(Room *)ctx;
    free_frame_chain_a **ptmp = (free_frame_chain_a **) params;
    free_frame_chain_a *ffca=*ptmp;
    ffca->next=room->framefreea;
    room->framefreea=ffca;
    room->frameusea--;

}

free_frame_chain_a::free_frame_chain_a(){
    next=nullptr;
    mfc.init();
}
void free_frame_chain_a::init(){
    mfc.init();
    next=nullptr;
}
int free_frame_chain_a::getlen(){
    return sizeof(free_frame_chain_a)+sizeof(hg_chain_node)+sizeof(hg_Buf_Gen_t);
}
free_frame_chain_a *free_frame_chain_a::instan(int relays){
    free_frame_chain_a *re= (free_frame_chain_a *)malloc(getlen()+relays);
    re->init();
    re->mfc.size=relays;
    hg_chain_node *hcn=(hg_chain_node *)((char *)re+sizeof(free_frame_chain_a));
    hcn->init();
    re->mfc.hct.left=hcn;
    re->mfc.hct.right=hcn;

    hg_Buf_Gen_t *hbgt=(hg_Buf_Gen_t *)((char *)re+sizeof(free_frame_chain_a)+sizeof(hg_chain_node));
    hbgt->init();
    hbgt->data=(char *)re+sizeof(free_frame_chain_a)+sizeof(hg_chain_node)+sizeof(hg_Buf_Gen_t);
    hbgt->cap=relays;
    hcn->data=hbgt;
    return re;
}
void free_frame_chain_a::reset(){
    //mfc.reset();
}

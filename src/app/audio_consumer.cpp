#include "audio_consumer.h"
#include "../app/app.h"
#include <atomic>
#include "../net/protocol/hg_channel_stru.h"
using namespace std;
AudioConsumer::AudioConsumer(){
    int err;
}

void AudioConsumer::RecvCallback(void *ctx,media_frame_chain *mfc,HgSessionC *sess0) {
    SessionC *sess=(SessionC *)HgmainPro_GetSessData(sess0);
    hg_Buf_Gen_t *hbgt=nullptr;
    hg_chain_node *hcn=mfc->hct.left;
    int size=mfc->size;
    uint8_t *leftdata=nullptr;


    if(hcn!=nullptr){
        hbgt=(hg_Buf_Gen_t *)hcn->data;
        leftdata=(uint8_t *)hbgt->data;
    }

    AudioConsumer * context=(AudioConsumer *)ctx;
    uint16_t packetPts=pktTimestamp(leftdata,size);
    int comp=pktComp(leftdata,size);
    uint32_t seq=pktSeq(leftdata,size);
    //1 calculate the spts and cpts
    Room * room=sess->rroom;
    if(room->id!=sess->roomid){//防止异步产生不同步问题
        return;
    }


    hg_chain_node *tmp11=mfc->hct.left;
    while(tmp11!=nullptr){
        hg_Buf_Gen_t *bttmp= (hg_Buf_Gen_t *)tmp11->data;
        uint32_t seq=pktSeq((unsigned char *)bttmp->data,12);
        tmp11=tmp11->next;
    }


    context->PreRoomMergeData(packetPts,comp,sess,mfc,room);

}
void AudioConsumer::PreRoomMergeData(uint32_t pts,int comp,SessionC *hgsess,media_frame_chain *mfc,Room *room){

    ////////////
    int lens = sizeof(hgtEvent) + sizeof(server_packet_roommerge_pa);
    char hgteventptr[lens];
    hgtEvent *hgtevent = (hgtEvent *) hgteventptr;
    server_packet_roommerge_pa *uprp = (server_packet_roommerge_pa *) (hgteventptr + sizeof(hgtEvent));
    uprp->sess=hgsess;
    uprp->pts=pts;
    uprp->comp=comp;
    uprp->mfc=*mfc;
    uprp->ssrc=hgsess->ssrc;
    uprp->room=room;
    hgtevent->handle = Room::RoomMergeData;
    hgtevent->psize = lens-sizeof(hgtEvent);
    hgtevent->ctx = this;
    hgtevent->i=12;
    HgWorker *hgworker = HgWorker::GetWorker(room->id);
    hgworker->WriteChanWor(hgteventptr, lens);
}



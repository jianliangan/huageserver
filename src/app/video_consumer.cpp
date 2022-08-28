#include "video_consumer.h"
#include "../app/app.h"
#include "../h264parse/h264_stream.h"
using namespace std;
VideoConsumer::VideoConsumer(){
    int err;


}

//目前没法先需要动态大小的udp包，暂时用源自己的seq，未来如果需要可以放到sess->bySubscList下的SessSubInfo里边
void VideoConsumer::OutConsumeQueue(media_frame_chain *mfc,SessionC *sess){
    std::unordered_map<uint32_t,SessSubInfo> * bsl;
    std::unordered_map<uint32_t,SessSubInfo>::iterator bslit;
    uint32_t ssrc=0;
    uint32_t index=0;
    uint64_t currentsec=0;
    SessionC *itsess=nullptr;
    VideoConsumer *vc=nullptr;
    currentsec=hgetSysTimeMicros()/1000000;
    bsl=sess->bySubscList;

    hg_Buf_Gen_t *hbgt=nullptr;
    hg_chain_node *lefthcn=mfc->hct.left;
    hbgt=(hg_Buf_Gen_t *)lefthcn->data;

    for (bslit = bsl->begin() ; bslit != bsl->end(); ++bslit)
    {
        if(bslit->second.flag&sess_sub_VIDEO!=sess_sub_VIDEO){
            continue;
        }

        itsess=bslit->second.sess;
        ssrc=itsess->ssrc;
        //
        hbgt->freenum++;
        HgNetabstract::PreSendData(hgapp->hgser->hnab, mfc, ssrc,
                (int) ENU_PLTYPEVIDEO,
                false, itsess->data, false, nullptr,0);


    }
}

void VideoConsumer::RecvCallback(void *ctx,media_frame_chain *mfc,HgSessionC *sess0) {
    VideoConsumer * context=(VideoConsumer *)ctx;
    uint8_t *nalstart=nullptr;
    SessionC *sess=(SessionC *)HgmainPro_GetSessData(sess0);
    int rbspsize;
    /*int naltype=h264_get_type(framedata+RTP_PACKET_HEAD_SIZE,size-RTP_PACKET_HEAD_SIZE,&nalstart,&rbspsize);
      if(naltype==7||naltype==8)
      {    
      sps_t *sps=new sps_t;
      int ret=h264_get_attr_sps(nalstart,rbspsize,sps);
      if(ret<0){
      return;
      }
      ALOGI(0, "aaaaaaaaaaaaaaaaaaaaaaaPic width : %d,fps:%f",sps->vui.sar_height,sps->vui.fixed_frame_rate_flag);
      }*/
    context->OutConsumeQueue(mfc,sess);
}


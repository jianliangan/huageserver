#include "net/protocol/core/tools.h"
#include "net/protocol/rtp_packet.h"
#include "net/server.h"
#include <fstream>
#include <ostream>
#include <string>
#include "base/json.hpp"
#include "app/timer.h"
#include "app/app.h"
#include "app/audio_consumer.h"
#include "app/comm_consumer.h"
#include "app/video_consumer.h"
#define vatSnum 1
/** 
 * */
int firstbuf;
bool islittleend=false;
App *hgapp=nullptr;
static void interval( void *param){
    printf("sig settimeout");
}


int main(int argc,char *argv[]){

    char *conf;
    std::string json_string;
    std::string tmps;
    conf=argv[1];
    islittleend=checkLitEndian();
    int pagesize=getpagesize();
    std::ifstream ifs;  
    int length;
    int thnum;
    int workSNum;
    hgapp=new App();
    int ok=hg_log_init();
    init16Num();
    if(ok!=0){
        printf("缺少log文件夹");
        return 0;

    }
    if(argc<2){
        printf("缺少配置文件");
        return 0;
    }

    ifs.open(conf);
    if (ifs.fail()) {
        printf("Failed to open the file %s\n", conf );
        ifs.close();
        return 0;  
    }
    ht=new HgTimer();
    hgTimerEvent hev;
    hev.handle=interval;
    hev.param=ht;
    uintptr_t ct=hgetSysTimeMicros()/1000;
    ht->AddEvent(hev,ct+1000*10);

    while (std::getline(ifs, tmps)) {
        json_string += tmps;
    }
    ifs.close();
    std::string err;
    std::string serverip;
    nlohmann::json json;
    int serverport;
    json = nlohmann::json::parse(json_string,nullptr,false);
    if(json.is_discarded())
    {
        ALOGI(0,"json format error  ");
        return 0;
    }
    //log_init(LL_TRACE, "rtp", "./log/");
    //ALOGI(0,"config ", json.dump());
    serverip=json.at("ip");
    serverport=json.at("port");
    firstbuf=json.at("firstbuf");
    thnum=json.at("thnum");
    workSNum=json.at("workSessNum");
    hgapp->workSessNum=workSNum;
    hgapp->threadNum=thnum;


    Room::audioconfig.channels=AUDIO_CHANNEL;
    Room::audioconfig.deep=AUDIO_DEEP;
    Room::audioconfig.frameperbuf=AUDIO_FRAMERPER;
    Room::audioconfig.samplesrate=AUDIO_SAMPLERATE;
    Room::audioconfig.rframeperbuf=AUDIO_FRAMERPER*AUDIO_CHANNEL;

    HgServer *hgser=new HgServer();
    hgser->audioctx =new AudioConsumer();
    hgser->videoctx = new VideoConsumer();
    hgser->textctx=new CommConsumer();
    hgapp->hgser=hgser;
    hgser->recvaudioCallback = AudioConsumer::RecvCallback;
    hgser->recvvideoCallback = VideoConsumer::RecvCallback;
    hgser->recvtextCallback = CommConsumer::RecvCallback;
    hgser->asyncLogin = CommConsumer::AsyncLogin;
    hgser->verify_auth=CommConsumer::Verify_auth;
    hgser->StartRun(serverip.c_str(),serverport);
    while (true) {
        hgapp->Channel.Drive(nullptr, 1);
    }

    return 0;
}

#ifndef _HUAGE_APP_SESSION_

#define _HUAGE_APP_SESSION_
#define sess_sub_AUDIO 0x01
#define sess_sub_VIDEO 0x02
#include "../net/protocol/core/tools.h"
#include "../net/protocol/core/hg_pool.h"
#include "../net/protocol/core/hg_buf.h"
#include "common.h"
#include "room.h"
#include <cassert>
#include <string>
#include <unordered_map>
#include "../net/protocol/hgmain.h"
#define VIEWS_NUM_MAX 25
#define SINGLE_MSG0_LEN 8
#define MSG0_MAX_DE 512
enum sessStatus {
    SESSCLOSE,SESSSTREAM, SESSLOGIN
};
enum SessType {
    SESS_TEXT, SESS_AUDIO, SESS_VIDEO
};
class SessionC;
class SessSubInfo {
    public:
        SessionC *sess;
        uint8_t flag;
};

class SessionC {
    public:
        enum sessStatus status;
        HgSessionC *data;
        audio_config_t aconfigt;
        char flags;//1bit标识是否收到房间全量信息,2bit 性别
        uint32_t spts;//标识最后的值
        uint32_t cpts;//标识最后的值
        char name[10];
        //uint32_t cptscache[AUDIO_MERGEBUF];
        int16_t framecache[AUDIO_CHANNEL*AUDIO_SAMPLERATE];
        char framesize[AUDIO_MERGEBUF];
        uint8_t vptsmsgca[SINGLE_MSG0_LEN];
        bool lastvptsmsg=false;
        uint8_t viewsn;
        SessionC *views[VIEWS_NUM_MAX];
        Room *rroom;
        uint32_t roomid;
        bool RooEntered;
        //void *data;//may pointer protocol sess
        std::unordered_map <uint32_t, SessSubInfo> *bySubscList;//这里改成地址了，原来不是，其他地方还没动呢
        std::unordered_map <uint32_t, SessSubInfo> *meSubList;


        uint32_t ssrc;


        static void sessAttachRecv(SessionC *sess, int vframesize, int aframesize, int tframesize);


        static void sessFree(void *sess);

        static void sessAddSubApi(SessionC *sess, SessionC *bysess,uint32_t ssrc, uint8_t);

        static void sessAddBySubApi(SessionC *sess,uint32_t ssrc, SessionC *bysess, uint8_t);

        static void sessDelSubApi(SessionC *sess, SessionC *bysess);

        static void sessDelBySubApi(SessionC *sess, SessionC *bysess);



        static void sessSetAudioConf(SessionC *sess,int chan,int deepn,int samrate,int frlen,int codec);

        static SessionC *sessInit(int sessNum);


        static void sessSetStatus(SessionC *sess, sessStatus status);
};

#endif


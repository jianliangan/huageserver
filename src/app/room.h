//
// Created by ajl on 2021/9/14.
//

#ifndef HUAGERTP_APP_ROOM_H
#define HUAGERTP_APP_ROOM_H
#include <stdint.h>
#include "../base/json.hpp"
#include "common.h"
#include "handler_consumer.h"
#include "comm_consumer.h"
#include "../net/protocol/core/tools.h"
#include "../net/protocol/core/hg_pool.h"
#include "../net/protocol/core/cctools.hpp"
#include "../base/g711.h"
#include "../net/protocol/core/hg_buf_comm.h"
#include "../net/protocol/hg_channel_stru.h"
#include <atomic>
#include <pthread.h>
#define AUDIO_CHANNEL 1
#define AUDIO_DEEP 16
#define AUDIO_FRAMERPER 160
#define AUDIO_SAMPLERATE 8000
#define AUDIO_MERGEBUF (AUDIO_SAMPLERATE/AUDIO_FRAMERPER)

/**
 * 1\ouput fixed 24bit deep
 * 2\client and server use the same framelen,samplerate  160,8000
 */
class SessionC;
typedef struct RooBufInfo{
    char *size;//每个位置帧总数
    int *samptota;//每个采样加起来的总和
    //uint8_t *rawcache;//音频原始数据

}RooBufInfo;

class free_frame_chain_a
{
    public:
        free_frame_chain_a *next;
        media_frame_chain mfc;
        free_frame_chain_a();
        void init();
        void reset();
        static int getlen();
        static free_frame_chain_a *instan(int relays);
        static void revert(free_frame_chain_a *ffca);
};

class RoomUserInfo {
    public:
        SessionC *sess;
        uint8_t flag;
};

class Room{//
    public:
        uint32_t id=0;
        uint64_t lasttime=0;
        uint64_t startplay=0;
        uint32_t nextptsplay=0;
        free_frame_chain_a *framefreea=nullptr;//音频cache
        int frameusea=0;
        int audioFramRawByte=0;
        int audioCaNum=0;
        int audioFrameOutByte=0;
        myPool *fragCache=nullptr;
        media_frame_chain mfcULst;
        RooBufInfo rooBufinfo;
        static audio_config_t audioconfig;
        std::unordered_map <uint32_t, RoomUserInfo> *userList;
        static Room *CreateRoom(uint32_t uniqid,int roomid,SessionC *sess,bool create);
        static int CreEnterRoom(unsigned uniqid,unsigned int roomid,SessionC *sess,bool create);
        static void HandleCreEnterRoo(void *pth,void *ctx,void *params,int psize);
        free_frame_chain_a *GetFreeFrame();
        int EnterRoom(SessionC *sess);
        void WriteHeader(uint8_t *data,uint32_t ssrc,int pltype,int padding,bool isfirst,int channel,uint32_t time,int cmdtype,int comptype);
        void OutConsumeQueue(char rsize,uint32_t pts,int fromi,SessionC *sess);
        Room(uint32_t roomid);
        int MergeAudioData(int index,media_frame_chain *mfc,SessionC *sess);
        static void RoomMergeData(void *pth,void *ctx,void *params,int psize);
        int subIndex(int newi,int oldi);
        int addIndex(int add1,int add2);
        static void WriteFree2(void *ctx, void *data);
        static void FreeByChan2(void *pth, void *ctx, void *params, int psize);

        static void WriteFree(void *ctx, void *data);
        static void FreeByChan(void *pth, void *ctx, void *params, int psize);
};
#endif //HUAGERTP_APP_ROOM_H


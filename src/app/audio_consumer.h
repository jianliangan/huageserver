#ifndef _HUAGE_APP_AUDIOCONSUMER_
#define _HUAGE_APP_AUDIOCONSUMER_
#include "session.h"
#include "../net/protocol/core/tools.h"
#include "../net/protocol/hgmain.h"
#include "../net/server.h"
#include "../net/protocol/huage_recvstream.h"
#include "../net/protocol/huage_sendstream.h"
#include "../net/protocol/core/hg_queue.h"
#include "../base/g711.h"
#include "../net/threads/hg_channel.h"
#include "../net/protocol/hg_channel_stru.h"
#include <unordered_map>
//音频，视频暂时不加session,因为不需要合包处理
class AudioConsumer {
    public:
        myPool *fragCache=nullptr;

        //bysubscrip,被订阅
        AudioConsumer();
        static void RecvCallback(void *ctx,media_frame_chain *mfc,HgSessionC *sess);
        void PreRoomMergeData(uint32_t pts,int comp,SessionC * hgsess,media_frame_chain *mfc,Room *room);
};
#endif

#ifndef _HUAGE_APP_VIDEOCONSUMER_
#define _HUAGE_APP_VIDEOCONSUMER_
#include "session.h"
#include "../net/protocol/core/tools.h"
#include "../net/server.h"
#include "../net/protocol/huage_recvstream.h"
#include "../net/protocol/huage_sendstream.h"
#include "../net/protocol/core/hg_queue.h"
#include "../net/threads/hg_channel.h"
#include <unordered_map>
//音频，视频暂时不加session,因为不需要合包处理
class VideoConsumer {
    public:
        //bysubscrip,被订阅
        VideoConsumer();
        void OutConsumeQueue(media_frame_chain *mfc,SessionC *sess);
        static void RecvCallback(void *ctx,media_frame_chain *mfc,HgSessionC *sess);
        void RemoveUidBroad(uint32_t ssrc);
        void DelBySubsTa(uint32_t bysub);//只能是comm执行
};
#endif

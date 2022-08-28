#ifndef _HUAGE_APP_COMMCONSUMER_
#define _HUAGE_APP_COMMCONSUMER_
#include "../net/protocol/core/tools.h"
#include "../net/server.h"
#include "../net/protocol/huage_recvstream.h"
#include "../net/protocol/huage_sendstream.h"
#include "../net/protocol/core/hg_queue.h"
#include "../net/threads/hg_channel.h"
#define MAX_TEXT_BUF_SIZE 4096
#define HANDLE_MAX_POOL 4096
#define HANDLE_MAX_FRAME_SLICE 2048


class SessionC;
typedef struct responseHead{
    uint32_t uniqId;
}responseHead;
//先确定信令的订阅列表是增量还是全量同步，然后是音频，视频只是只读备份，
class CommConsumer {
    public:

        CommConsumer();
        static void RecvCallback(void *param,media_frame_chain *mfc,HgSessionC *sess);
        static void RecvCallback0(void *ctx,media_frame_chain *mfc,HgSessionC *sess);
        static void AsyncLogin(void *ctx, media_frame_chain *mfc,HgSessionC *sess);
        static void WorkHanLogin(void *pth,void *ctx,void *params,int psize);
        static void WriteFree(void *ctx, void *data);
        static void FreeByChan(void *pth, void *ctx, void *params, int psize);

        void SessOut(SessionC *sess);
        void Logout(SessionC *sess);
        void framebufSet(media_frame_chain *mfc,uint16_t time,hg_chain_t *hct,int length,void *ctx);
        void Login(uint32_t ssrc,SessionC *sess);
        static bool Verify_auth(void *ctx,HgSessionC *sess, int fd);

};
#endif

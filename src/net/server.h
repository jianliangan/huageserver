#ifndef HUAGE_SERVER_H
#define HUAGE_SERVER_H
class TcpServer;
class UdpServer;
class HgSessionC;
struct sockaddr_in;
#include "common/extern.h"
#include "stdint.h"
#include "common/hg_pipe.h"
#include "common/hg_netabstract.h"
#include "hg_com.h"
class HgServer {
    public:
        HgSessionC *sess=nullptr;
        HgNetabstract *hnab=nullptr;
        HgServer();
        void *audioctx=nullptr;
        void *videoctx=nullptr;
        void *textctx=nullptr;
        void StartRun(const char *ip,int port);
        static void RecvCallback(PLTYPE pltype,void *ctx,media_frame_chain *mfc,HgSessionC *sess);
        void
            (*recvaudioCallback)( void *ctx, media_frame_chain *mfc, HgSessionC *sess);
        void
            (*recvvideoCallback)( void *ctx, media_frame_chain *mfc, HgSessionC *sess);
        void
            (*recvtextCallback)( void *ctx, media_frame_chain *mfc, HgSessionC *sess);
        static bool
            Verify_Auth(void *ctx,HgSessionC *sess, int fd);
        bool (*verify_auth)(void *ctx,HgSessionC *sess, int fd);
        static void
            AsyncLogin(void *ctx,media_frame_chain *mfc, HgSessionC *sess);
        void (*asyncLogin)(void *ctx, media_frame_chain *mfc, HgSessionC *sess);

};


#endif //HUAGE_SERVER_H

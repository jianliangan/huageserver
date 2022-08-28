//
// Created by ajl on 2022/1/12.
//

#ifndef HUAGERTP_APP_HG_NETABSTRACT_H
#define HUAGERTP_APP_HG_NETABSTRACT_H

#include "extern.h"
#include <stdint.h>
#include "hg_iocp.h"
#include "../threads/hg_worker.h"
#include "hg_pipe.h"
#include "hg_udpabstract.h"
#include "hg_tcpabstract.h"

class HgNetabstract {
    public:
        bool isclient ;
        HgTcpabstract *aTcpinstan= nullptr;
        HgUdpabstract *aUdpinstan= nullptr;
        selfCB_s selfcb;
        HgNetabstract();

        static int
            CreateFrameStr(myPool *fragCache, hg_chain_t *hct, uint32_t ssrc, int pltype,
                    bool isfirst, int cmdtype, uint16_t time,uint8_t *data,int size,int ssize,int comptype);
        void CaBaChan(int id,int code);
        void ClearReqFram(media_frame_chain *mfc);
        static hg_Buf_Gen_t *AllocMemery_hbgt(myPool *fragCache, int size);

        //client
        static void SendDataQueue(void *pth, void *ctx, void *params, int psize);
        //tosess 发送给某个socket，如果链接的是远端，就发给远端
        static void PreSendData(void *ctx, media_frame_chain *mfc, uint32_t ssrc,
                int pltype, bool isfirst, HgSessionC *tosess,
                bool direct, void *pth,int comptype);
};


#endif //HUAGERTP_APP_HG_NETABSTRACT_H

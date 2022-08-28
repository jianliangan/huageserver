//
// Created by ajl on 2021/12/22.
//

#ifndef HUAGERTP_APP_HG_CHANNEL_H
#define HUAGERTP_APP_HG_CHANNEL_H
#include "hg_tevent.h"
#include "../protocol/core/tools.h"
#include "../protocol/core/hg_buf_comm.h"
#include "../protocol/core/hg_buf.h"
class HgChannel {
public:
  uint64_t ttt=0;
  uint64_t ttt1=0;

    myPool *fragCache=nullptr;
    hg_chain_node *freen= nullptr;
    hg_chain_t hct;
    hg_Buf_Gen_t tcpbuf;
    int lock=0;
    sem_t eventSem;

    HgChannel();
    void Handlemsg(void *data,hg_Buf_Gen_t *tcpb);
    void Drive(void *data,int wait);
    int WriteChan(void *data, int size,int wait);

};


#endif //HUAGERTP_APP_HG_CHANNEL_H

//
// Created by ajl on 2021/11/22.
//

#ifndef HUAGERTP_APP_HG_TEVENT_H
#define HUAGERTP_APP_HG_TEVENT_H
#define EV_HG_EVENTLEN 1000;
class HgFdEvent;
typedef void (*even_handle)(void *pth,void *ctx,void *params,int psize);
extern const char *eventstr[];/*={
        "HandleCom::FreeByChan",
        "HgNetabstract::SendDataQueue",
        "selfcb.cbhandle",
        "HgTcpabstract::SendChain",
        "HgTcpabstract::THPOOLRecv",
        "HgTcpabstract::FreeMemery_local",
        "HuageRecvStream::THPOOLRecv",
        "HgUdpabstract::FreeMemery_local",
        "HuageRecvStream::FreeByChan"
};*/
typedef struct hgtEvent { //每帧的头
    char i;
    int psize;
    void *ctx;
    even_handle handle;
} hgtEvent;
#endif //HUAGERTP_APP_HG_TEVENT_H

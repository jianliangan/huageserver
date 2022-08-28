//
// Created by ajl <429200247@qq.com> on 2020/12/20.
//
#include <string>
#include "hg_iocp.h"
#include "../../app/app.h"
#include "../protocol/hgfdevent.h"
const char *eventstr[]={
        "HandleCom::FreeByChan",
        "HgNetabstract::SendDataQueue",
        "selfcb.cbhandle",
        "HgTcpabstract::SendChain",
        "HgTcpabstract::THPOOLRecv",
        "HgTcpabstract::FreeMemery_local",
        "HuageRecvStream::THPOOLRecv",
        "HgUdpabstract::FreeMemery_local",
        "HuageRecvStream::FreeByChan",
        "CommConsumer::WorkHanLogin",
        "CommConsumer::FreeByChan",
        "Room::FreeByChan",
        "Room::RoomMergeData"
};
HgIocp::HgIocp() {
    efd = epoll_create(MAX_OPEN_FD);
    fragCache = myPoolCreate(2046);
}
void HgIocp::StartRun() {
    HgFdEvent *fdevpipe = HgFdEvent::GetFreeFdEv(&fdchainfree);
    aPipeClient = new HgPipe();
    aPipeClient->ver_recv_ctx = this;
    fdevpipe->ctx = aPipeClient;
    fdevpipe->read = HgPipe::PipeRecvmsg;
    fdevpipe->write = HgPipe::PipeSendmsg;
    fdevpipe->fd = aPipeClient->pipe_fd[0];
    AddFdEvt(aPipeClient->pipe_fd[0], EPOLL_CTL_ADD, fdevpipe,1);
int err;
    err = pthread_create(&runThread, NULL, Loop, this);
    if (err != 0) {

        ALOGI(0, "client wait thread err");
    }

}

int
HgIocp::AddFdEvt(int fd,int op,void *ptr,int opa) {
    if(ptr== nullptr)
        return -1;
    tep.events = EPOLLIN |(opa==1?EPOLLOUT:0) | EPOLLRDHUP | EPOLLET;
    // 把监听socket 先添加到efd中
    tep.data.ptr=ptr;
    int ret;
    ret = epoll_ctl(efd, op, fd, &tep);
    return 0;
}

void *HgIocp::Loop(void *ctx) {
    HgIocp *hiocp=(HgIocp *)ctx;

    for (;;) {
        // 返回已就绪的epoll_event,-1表示阻塞,没有就绪的epoll_event,将一直等待
        int nready=0;
        do{
            nready= epoll_wait(hiocp->efd, hiocp->ep, MAX_OPEN_FD, -1);
        }while(nready<0&&errno==EINTR);

        for (int i = 0; i < nready; ++i) {

            if (hiocp->ep[i].events & EPOLLIN) {
                HgFdEvent *ptr=(HgFdEvent *)hiocp->ep[i].data.ptr;
                IocpHandle handle=ptr->read;
                if(handle!= nullptr){

                    handle(hiocp,ptr->ctx,ptr);
                }

            }
            if (hiocp->ep[i].events & EPOLLOUT) {
                HgFdEvent *ptr=(HgFdEvent *)hiocp->ep[i].data.ptr;
                IocpHandle handle=ptr->write;
                if(handle!= nullptr)
                handle(hiocp,ptr->ctx,ptr);
            }
            /*
            if (ep[i].events & EPOLLRDHUP) {
                printf("EPOLLRDHUP,");
            }
            if (ep[i].events & EPOLLET) {
                printf("EPOLLET,");
            }*/
        }
    }
}

HgIocp::~HgIocp() {
    close(confd);
}

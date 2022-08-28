#include<stdio.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#define MAXLEN (1024*1024)
#define SERV_PORT 82
#define MAX_OPEN_FD 1024

int main(int argc,char *argv[])
{
    int  listenfd,connfd,efd,ret,one;
    char buf[MAXLEN];
    struct sockaddr_in cliaddr,servaddr;
    socklen_t clilen = sizeof(cliaddr);
    struct epoll_event tep,ep[MAX_OPEN_FD];
    int rr=listenfd = socket(AF_INET,SOCK_STREAM,0);
    if(rr==-1){
        printf("errfd %s\n",strerror(errno));
    }
    one=1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
        close(listenfd);
        return -1;
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    rr=bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
    if(rr==-1){
        printf("errbind %s\n",strerror(errno));
    }

    rr=listen(listenfd,20);
    if(rr==-1){
        printf("err %s\n",strerror(errno));
    }
    // 创建一个epoll fd
    efd = epoll_create(MAX_OPEN_FD);
    tep.events = EPOLLIN;tep.data.fd = listenfd;
    // 把监听socket 先添加到efd中
    ret = epoll_ctl(efd,EPOLL_CTL_ADD,listenfd,&tep);
    // 循环等待
    int full=0;
    for (;;)
    {
        // 返回已就绪的epoll_event,-1表示阻塞,没有就绪的epoll_event,将一直等待
        size_t nready = epoll_wait(efd,ep,MAX_OPEN_FD,-1);

        for (int i = 0; i < nready; ++i)
        {
            // 如果是新的连接,需要把新的socket添加到efd中
            if (ep[i].data.fd == listenfd )
            {
                connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&clilen);
                tep.events = EPOLLIN|EPOLLRDHUP|EPOLLET;
                tep.data.fd = connfd;
                fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL) | O_NONBLOCK);
                ret = epoll_ctl(efd,EPOLL_CTL_ADD,connfd,&tep);

                //tep.events = EPOLLIN|EPOLLET;//EPOLLOUT|EPOLLET|EPOLLRDHUP;
                //  printf("MOD\n");
                //tep.data.fd = connfd;
                //ret = epoll_ctl(efd,EPOLL_CTL_MOD,connfd,&tep);


            }
            // 否则,读取数据
            else
            {
                if(ep[i].events&EPOLLIN){
                    printf("EPOLLIN,");
                }
                if(ep[i].events&EPOLLOUT){
                    printf("EPOLLOUT,");
                }
                if(ep[i].events&EPOLLRDHUP){
                    printf("EPOLLRDHUP,");
                }
                if(ep[i].events&EPOLLET){
                    printf("EPOLLET,");
                }
                printf("\n");
                sleep(10);
                for(int j=0;j<1;j++){
                    //int pp=write(ep[i].data.fd,buf,MAXLEN);
                    //if(pp==-1)
                    //{
                    tep.events = EPOLLIN|EPOLLOUT|EPOLLET;//EPOLLOUT|EPOLLET|EPOLLRDHUP;
                    //  printf("MOD\n");
                    tep.data.fd = ep[i].data.fd;
                    ret = epoll_ctl(efd,EPOLL_CTL_MOD,ep[i].data.fd,&tep);
                    //full=1;
                    //}
                }
                continue;
                //sleep(1);
                //read(connfd,buf,MAXLEN);
                printf("--------\n");
                continue;
                connfd = ep[i].data.fd;
                int bytes = read(connfd,buf,MAXLEN);
                // 客户端关闭连接
                if (bytes == 0){
                    ret =epoll_ctl(efd,EPOLL_CTL_DEL,connfd,NULL);
                    close(connfd);
                    printf("client[%d] closed\n", i);
                }
                else
                {
                    for (int j = 0; j < bytes; ++j)
                    {
                        buf[j] = toupper(buf[j]);
                    }
                    // 向客户端发送数据
                    write(connfd,buf,bytes);
                }
            }
        }
    }
    return 0;
}

#include<stdio.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>
#include<sys/time.h>
#include <stdint.h>
#define MAXLEN PIPE_BUF
#define SERV_PORT 82
#define MAX_OPEN_FD 1024
#define MAX 10000
#define abcnpro 1
int  listenfd,connfd,ret,one;
char buf[MAXLEN]={1};
char buf1[MAXLEN];

int64_t start;
int64_t end;

int pipe_fd[2];



struct sockaddr_in cliaddr,servaddr;
socklen_t clilen = sizeof(cliaddr);
struct epoll_event tep,ep[MAX_OPEN_FD];
int64_t microtime(){
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;

}
void * handlethread(void *context){
    // 创建一个epoll fd
    int efd = epoll_create(MAX_OPEN_FD);
    struct epoll_event tep;
    tep.events = EPOLLIN|EPOLLET;
    tep.data.fd = pipe_fd[0];
    // 把监听socket 先添加到efd中
    ret = epoll_ctl(efd,EPOLL_CTL_ADD,pipe_fd[0],&tep);
    int64_t total=0;
    for (;;)
    {
        // 返回已就绪的epoll_event,-1表示阻塞,没有就绪的epoll_event,将一直等待
        size_t nready = epoll_wait(efd,ep,MAX_OPEN_FD,-1);

        for (int i = 0; i < nready; ++i)
        {
            // 如果是新的连接,需要把新的socket添加到efd中
            if (!(ep[i].data.fd == pipe_fd[0]) )
            {


                connfd = ep[i].data.fd;//accept(listenfd,(struct sockaddr*)&cliaddr,&clilen);
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
                /*
                   printf("epoll event \n");
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
                   printf("\n");*/
                connfd=ep[i].data.fd;
                while(1){
                    //printf("read;\n");
                    int pp=read(connfd,buf1,MAXLEN);
                    if(pp<=0){
                        //printf("break;\n");
                        break;
                    }else{
                        total+=pp;
                    }
                }
                //printf("total:%ld\n",total);
                if(total==MAX*PIPE_BUF){
                    end=microtime();
                    printf("result %ld\n",end-start);
                }
                //printf("accept %s\n",buf1);
                // }
                continue;
                for(int j=0;j<1;j++){
                    //int pp=write(ep[i].data.fd,buf,MAXLEN);
                    //if(pp==-1)
                    //{
                    tep.events = EPOLLIN|EPOLLOUT|EPOLLET;//EPOLLOUT|EPOLLET|EPOLLRDHUP;
                    //  printf("MOD\n");
                    tep.data.fd = ep[i].data.fd;
                    ret = epoll_ctl(efd,EPOLL_CTL_MOD,ep[i].data.fd,&tep);
                    if(ret<0){
                        printf("error epoll\n");
                    }
                    //full=1;
                    //}
                }
                continue;
                //sleep(1);
                //read(connfd,buf,MAXLEN);
                printf("--------\n");
                continue;
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
}
void *work(void *context){
    start =microtime();
    printf("will send %ld size\n",MAX*PIPE_BUF);
    int abc=0;
    for(int i=0;i<MAX/abcnpro;i++){
        abc=0;
        while(abc!=sizeof(buf)){
            int tmp=write(pipe_fd[1],buf+abc,sizeof(buf)-abc);
            if(tmp>0){
                abc+=tmp;
            }
            //printf("work loop %d ,%d\n",tmp,sizeof(buf));
        }
    }

}
int main(int argc,char *argv[])
{


    if((ret=pipe(pipe_fd))<0)
    {
        printf("create pipe fail\n");
        return -1;
    }
    printf("bufpip %d\n",PIPE_BUF);
    int connfd1=pipe_fd[1];
    int connfd2=pipe_fd[0];
    fcntl(connfd1, F_SETFL, fcntl(connfd1, F_GETFL) | O_NONBLOCK);
    fcntl(connfd2, F_SETFL, fcntl(connfd2, F_GETFL) | O_NONBLOCK);

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
#define abcn 1

    pthread_t bs[abcn];
    for(int i=0;i<abcn;i++){
        pthread_create(&(bs[i]), NULL,handlethread, NULL);
    }
    //sleep(1);
    pthread_t bssend[abcnpro];
    for(int i=0;i<abcnpro;i++){
        pthread_create(&(bssend[i]),NULL,work,NULL);
    }
    sleep(100000);
    // 循环等待
    int full=0;
    return 0;
}

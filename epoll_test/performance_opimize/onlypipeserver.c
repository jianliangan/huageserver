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
#define abcnpro 4
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
    int connfd = pipe_fd[0];
    int64_t total=0;

    while(1){
        //printf("read;\n");
        int pp=read(connfd,buf1,MAXLEN);
        if(pp<0){
            printf("break;\n");
            break;
        }else{
            total+=pp;
        }
        if(total==MAX*PIPE_BUF){
            end=microtime();
            printf("result %ld\n",end-start);
        }
    }
    //printf("total:%ld\n",total);

    //printf("accept %s\n",buf1);
    // }

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

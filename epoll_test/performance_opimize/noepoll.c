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
#include <semaphore.h>
#include "hg_pool.h"
#include "hg_queue.h"
//#include <stdatomic.h>
#define MAXLEN PIPE_BUF
#define SERV_PORT 82
#define MAX_OPEN_FD 1024
#define MAX 10000
#define abcnpro 4
int  listenfd,connfd,ret,one;
char buf[MAXLEN]={1};
char buf1[MAXLEN]={1};
volatile int atomicint;
//volatile atomic_int atomicint;
int64_t start;
int64_t end;

int pipe_fd[2];


pthread_mutex_t eventMtx;
pthread_mutex_t cacheMtx;
sem_t eventSem;
myQueue *eventQueue;
myPool *fragCache;

typedef struct newabc{
    uint8_t * data;
    int size;
}newabc;
struct sockaddr_in cliaddr,servaddr;
socklen_t clilen = sizeof(cliaddr);
struct epoll_event tep,ep[MAX_OPEN_FD];
volatile int workint=0;
volatile int handleint=0;
int64_t microtime(){
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;

}
void * handlethread(void *context){
    uint8_t ptr[12];
    int64_t total=0;
    int size=PIPE_BUF;
    uint8_t *fragd;
    uint8_t pp[12];
    while (1) {
        sem_wait(&eventSem);
        //pthread_mutex_lock(&cacheMtx);
        //pthread_mutex_lock(&eventMtx);

        do{
            ;
        }while(__sync_val_compare_and_swap(&atomicint,0,1)==1);


        fragd = myPoolAlloc(fragCache, size);
        //printf("d %d\n",*(int *)(buf1));
        memcpy(fragd, buf1, size);
        *((uint8_t **)pp) = (uint8_t *) fragd;
        *((int *)(pp+8))=size;

        myQuStreamW(eventQueue, pp, sizeof(pp));


        int reallength = myQuStreamL(eventQueue, ptr, sizeof(ptr), NOHGISNEEDPEEK);
        //pthread_mutex_unlock(&eventMtx);
        if(reallength==sizeof(ptr)){
            uint8_t *ptrdata=*((uint8_t **)ptr);
            int size=*(int *)(ptr+8);
            //printf("size:%d\n",size);
            //pthread_mutex_lock(&cacheMtx);
            myPoolFree(fragCache, ptrdata);
            //pthread_mutex_unlock(&cacheMtx);
            //handleint++;
            //printf("handle %d\n",handleint);
            total+=PIPE_BUF;
            if(total==PIPE_BUF*MAX){
                end=microtime();
                printf("result %ld\n",end-start);

            }
        }
        //pthread_mutex_unlock(&cacheMtx);
        //pthread_mutex_unlock(&eventMtx);
        //atomic_store_explicit(&atomicint, 0, memory_order_seq_cst);
        atomicint=0;
    }


    return NULL;
    // 创建一个epoll fd
    //
    //
    //
}
void *work(void *context){
    start =microtime();
    printf("will send %ld size\n",MAX*PIPE_BUF);
    int size=PIPE_BUF;
    uint8_t pp[12];
    uint8_t *fragd=(uint8_t *)malloc(PIPE_BUF);
    int i=1;
    for(int i=0;i<MAX/abcnpro;i++){
        //pthread_mutex_lock(&cacheMtx);
        //pthread_mutex_lock(&eventMtx);

        do{
            ;
        }while(__sync_val_compare_and_swap(&atomicint,0,1)==1);

        memcpy(buf1,buf,size);
        *(int *)buf1=i;
        /*
           fragd = myPoolAlloc(fragCache, size);
        //pthread_mutex_unlock(&cacheMtx);
        memcpy(fragd, buf, size);
         *((uint8_t **)pp) = (uint8_t *) fragd;
         *((int *)(pp+8))=size;
         myQuStreamW(eventQueue, pp, sizeof(pp));
         */
        atomicint=0;
        //pthread_mutex_unlock(&cacheMtx);
        sem_post(&eventSem);
    }
    /*
       int abc=0;
       for(int i=0;i<MAX;i++){
       abc=0;
       while(abc!=sizeof(buf)){
       int tmp=write(pipe_fd[1],buf+abc,sizeof(buf)-abc);
       if(tmp>0){
       abc+=tmp;
       }
//printf("work loop %d ,%d\n",tmp,sizeof(buf));
}
}*/

}
int main(int argc,char *argv[])
{
    atomicint=0;
    //atomic_store_explicit(&atomicint, 0, memory_order_seq_cst);
    pthread_mutex_init(&eventMtx, NULL);
    pthread_mutex_init(&cacheMtx,NULL);
    sem_init(&eventSem, 0, 0);
    eventQueue = myQueueCreate(1024);
    fragCache = myPoolCreate(4096*2);

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
        pthread_create(&bssend[i],NULL,work,NULL);
    }
    sleep(100000);
    // 循环等待
    int full=0;
    return 0;
}

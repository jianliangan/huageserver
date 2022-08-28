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
sem_t eventSem;
int64_t start;
pthread_mutex_t lock;
pthread_cond_t cond;
int64_t microtime(){
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;

}
void * handlethread(void *context){
    // 创建一个epoll fd
    int n=0;
    for (;;)
    {
        // 返回已就绪的epoll_event,-1表示阻塞,没有就绪的epoll_event,将一直等待
        
           sem_wait(&eventSem);
           n++;
           if(n==1000000){

           printf("%ld,\n",microtime()-start);
           }
           
        /*
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&cond,&lock);
        pthread_mutex_unlock(&lock);
        n++;
        //printf("%d,\n",n);
        if(n>2190){
            printf("%ld,\n",microtime()-start);
        }*/

    }
}
void *work(void *context){
    int abc=0;
    for(int i=0;i<100000;i++){
        /*
           pthread_mutex_lock(&lock);

           pthread_cond_signal(&cond);
           pthread_mutex_unlock(&lock);
           */
        sem_post(&eventSem);
    }

}
int main(int argc,char *argv[])
{

    pthread_mutex_init(&lock,NULL);
    pthread_cond_init(&cond,NULL);

    sem_init(&eventSem, 0, 0);
    start =microtime();
    //sleep(1);
    int abcnpro=10; 
    pthread_t bssend[abcnpro];
    for(int i=0;i<abcnpro;i++){
        pthread_create(&(bssend[i]),NULL,work,NULL);
    }
    pthread_t temp;
    pthread_create(&temp,NULL,handlethread,NULL);
    sleep(100000);

}

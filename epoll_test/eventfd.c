//gcc eventfd.c -o eventfd -lpthread
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>

#define TOTO 10
int efd = -1;

void *read_thread(void *dummy)
{
    int ret = 0;
    uint64_t count = 0;
    int ep_fd = -1;
    struct epoll_event events[10];

    if (efd < 0)
    {
        printf("efd not inited.\n");
    }

    ep_fd = epoll_create(1024);
    if (ep_fd < 0)
    {
        perror("epoll_create fail: ");
    }


    struct epoll_event read_event;

    read_event.events = EPOLLHUP | EPOLLERR | EPOLLIN;
    read_event.data.fd = efd;

    ret = epoll_ctl(ep_fd, EPOLL_CTL_ADD, efd, &read_event);
    if (ret < 0)
    {
        perror("epoll ctl failed:");
    }


    while (1)
    {
        ret = epoll_wait(ep_fd, &events[0], 10, 5000);
        printf("aaaaa %d\n",ret);
        if (ret > 0)
        {
            int i = 0;

            for (; i < ret; i++)
            {
                if (events[i].events & EPOLLHUP)
                {
                    printf("111111111111111111111111.\n");
                }
                else if (events[i].events & EPOLLERR)
                {
                    printf("222222222222222222222222222.\n");
                }
                else if (events[i].events & EPOLLIN)
                {
                    int event_fd = events[i].data.fd;
                    ret = read(event_fd, &count, sizeof(count));

                    printf("ooooook %ld  %d\n",
                            count,sizeof(count));


                    if(count==TOTO){
                        struct timeval tv;

                        gettimeofday(&tv, NULL);
                        printf("ooooook %d at %lds %ldus\n",
                                count, tv.tv_sec, tv.tv_usec);
                    }
                    if (ret < 0)
                    {
                        perror("3333333333333333333:");
                    }
                    else
                    {
                    }
                }
            }
        }
        else if (ret == 0)
        {
            /* time out */
            printf("2222222222222222222\n");
        }
        else
        {
            perror("111111111111111\n");
        }
    }


    return NULL;
}
int main(int argc, char *argv[])
{
    pthread_t pid = 0;
    uint64_t count = 0;
    int ret = 0;
    int i = 0;

    //efd = eventfd(0, 0);
    printf("EFD_NONBLOCK:%d\n",EFD_NONBLOCK);
    efd = eventfd(0, EFD_SEMAPHORE );
    if (efd < 0)
    {
    }

    ret = pthread_create(&pid, NULL, read_thread, NULL);
    if (ret < 0)
    {
        perror("pthread create:");
    }

    struct timeval tv;

    gettimeofday(&tv, NULL);
    printf("start %d bytes(%llu) at %lds %ldus\n",
            ret, count, tv.tv_sec, tv.tv_usec);
    for (i = 0; i <=TOTO; i++)
    {
        count = 1;
        ret = write(efd, &count, sizeof(count));
        printf("ddd  %d %d\n",sizeof(count),ret);
        if (ret < 0)
        {
            perror("write event fd fail:");
        }
        else
        {

        }
        //        sleep(1);
    }

    if (0 != pid)
    {
        pthread_join(pid, NULL);
        pid = 0;
    }

    if (efd >= 0)
    {
        close(efd);
        efd = -1;
    }
    return ret;
}

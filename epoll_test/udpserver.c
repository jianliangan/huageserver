#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#define SERV_PORT   8000

#define MY_NOBLOCK(s) fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
int main()
{
    /* sock_fd --- socket文件描述符 创建udp套接字*/
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_fd < 0)
    {
        perror("socket");
        return 0;
    }
    struct sockaddr_in addr_serv;
    int len;
    memset(&addr_serv, 0, sizeof(struct sockaddr_in));
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(SERV_PORT);
    addr_serv.sin_addr.s_addr = htonl(INADDR_ANY);  //自动获取IP地址
    len = sizeof(addr_serv);
    MY_NOBLOCK(sock_fd);
    /* 绑定socket */
    if(bind(sock_fd, (struct sockaddr *)&addr_serv, sizeof(addr_serv)) < 0)
    {
        perror("bind error:");
        return 0;
    }
    printf("sock %d\n",sock_fd);

    int recv_num;
    int send_num;
    char send_buf[20] = "i am server!";
    char recv_buf[20];
    struct sockaddr_in addr_client;

    ///////////////////////////////////////////////////
    struct epoll_event tep,ep[20];
    int efd = epoll_create(10);
    tep.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    tep.data.fd = sock_fd;
    // 把监听socket 先添加到efd中
    //tep.data.ptr=NULL;
    int ret;
    ret = epoll_ctl(efd, EPOLL_CTL_ADD, sock_fd, &tep);
    //sleep(20);
    while(1){
        // 返回已就绪的epoll_event,-1表示阻塞,没有就绪的epoll_event,将一直等待
        printf("start wait\n");
        size_t nready = epoll_wait(efd, ep, 10, -1);
        printf("in wait\n");
        for (int i = 0; i < nready; ++i) {
            // 如果是新的连接,需要把新的socket添加到efd中
            if (ep[i].events & EPOLLIN) {
                printf("event sock %d\n",ep[i].data.fd);
                if(ep[i].data.fd<=0)
                    continue;
                while(1){
                    recv_num = recvfrom(ep[i].data.fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);
                    if(recv_num <= 0)
                    {
                        int error = errno;
                        if (error == EAGAIN || error == EINTR){
                            break;
                        }else{
                            perror("recvfrom error:");
                            exit(1);

                        }
                    }

                    recv_buf[recv_num] = '\0';
                    struct sockaddr_in *serveraddr=(struct sockaddr_in *)(&addr_client);
                    int abc=ntohs((*serveraddr).sin_port);
                    printf("client port: %d,\n", abc);
                }
            }
            if (ep[i].events & EPOLLOUT) {
            }

            if (ep[i].events & EPOLLRDHUP) {
                printf("EPOLLRDHUP,\n");
            }
            if (ep[i].events & EPOLLET) {
                printf("EPOLLET,\n");
            }
        }
    }
    //////////////////////////////////////////////////////////
    /*
       sleep(20);
       while(1)
       {
       printf("server wait:\n");

       recv_num = recvfrom(sock_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);

       if(recv_num < 0)
       {
       perror("recvfrom error:");
       exit(1);
       }

       recv_buf[recv_num] = '\0';
       printf("server receive %d bytes: %s,ip: %s,port:%d \n", recv_num, recv_buf,inet_ntoa(addr_client.sin_addr),addr_client.sin_port);
       */
    /*send_num = sendto(sock_fd, send_buf, recv_num, 0, (struct sockaddr *)&addr_client, len);

      if(send_num < 0)
      {
      perror("sendto error:");
      exit(1);
      }*/
    //}

    close(sock_fd);

    return 0;
}


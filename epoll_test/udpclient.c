#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXSIZE 512
#define SERVER_PORT 8000
#define RECV_PORT 12377
#define IP "127.0.0.1"

int main(int argc, char* argv[])
{
    char buff[MAXSIZE];
    /*创建socket，设置广播属性*/
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int on = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&on,sizeof(on));
    /*绑定接收端口*/
    struct sockaddr_in rcv_addr;
    bzero(&rcv_addr, sizeof(rcv_addr));
    rcv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &rcv_addr.sin_addr);
    rcv_addr.sin_port = htons(RECV_PORT);
    bind(sockfd,(struct sockaddr*)&rcv_addr,sizeof(rcv_addr));
    /*设置发送到服务器的IP，端口等信息*/
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, IP, &server_addr.sin_addr);
    server_addr.sin_port = htons(SERVER_PORT);
    printf("Udp Client Start....\n");
    while(fgets(buff, MAXSIZE, stdin) != NULL)
    {
        /*发送信息到服务器*/
        printf("send to\n");
        int ret = sendto(sockfd, buff, strlen(buff), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if(-1 == ret)
        {
            printf("sendto error!\n");
        }
        /*接收服务器返回信息*/
        /*ret = recvfrom(sockfd, buff, MAXSIZE, 0, NULL, 0);
          if(-1 == ret)
          {
          printf("recvfrom error!\n");
          }*/
        write(STDOUT_FILENO, buff, ret);
    }
    close(sockfd);
    return 0;
}

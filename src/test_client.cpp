#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <iostream>
#define MAXLINE 540
#define UDPPORT 9201
#define SERVERIP "103.10.85.58"

int udpport=0;
const uint8_t *ip;
int main(int argc,uint8_t *argv[]){
    if(argc<3){
        printf("缺少配置文件");
        return 0;
    }
    ip=argv[1];
    udpport= std::atoi(argv[2]);

    int confd;
    unsigned int addr_length;
    uint8_t recvline[MAXLINE];
    uint8_t sendline[MAXLINE];
    struct sockaddr_in serveraddr;

    // 使用socket()，生成套接字文件描述符；
    if( (confd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ){
        perror("socket() error");
        return 1;
    }

    //通过struct sockaddr_in 结构设置服务器地址和监听端口；
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(ip);
    serveraddr.sin_port = htons(udpport);
    addr_length = sizeof(serveraddr);

    int send_length = 0;
    for(int i=0;i<3;i++){
        // 向服务器发送数据，sendto() ；
        sprintf(sendline,"hello server!");
        send_length = sendto(confd, sendline, sizeof(sendline), 0, (struct sockaddr *) &serveraddr, addr_length);
        if(send_length < 0 ){
            perror("sendto() error");
            return 1;
        }
        printf("send_length = %d",send_length);
        usleep(1000);
    }
    // 接收服务器的数据，recvfrom() ；
    int recv_length = 0;
    recv_length = recvfrom(confd, recvline, sizeof(recvline), 0, (struct sockaddr *) &serveraddr, &addr_length);
    printf("recv_length = %d",recv_length);
    printf("%s", recvline);

    // 关闭套接字，close() ；
    close(confd);

    return 0;
}

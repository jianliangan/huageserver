//tcpclient.cc
#include <iostream>
#include <cstring>
#include <strings.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

int main(int argc, char *argv[])
{
    //创建套接字
    int sk = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(82);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    //连接服务器
    connect(sk, (struct sockaddr*)&server, sizeof(server));
    char buff[1024*1024] = {'\0'};
    //接收数据
    int abc;
    fcntl(sk, F_SETFL, fcntl(sk, F_GETFL) | O_NONBLOCK);

    //recv(sk, buff, sizeof(buff), 0);
    printf("writ\n");
    write(sk, buff, 512);
    printf("%d\n",abc);
    sleep(100000);
    printf("send %d\n",abc);
    recv(sk, buff, sizeof(buff), 0);
    cout << buff << endl;
    //发送数据

    sleep(10000);
    //关闭套接字
    return 0;
}

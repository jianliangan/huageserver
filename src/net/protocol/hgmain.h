//
// Created by ajl on 2021/10/26.
//
/*
 *
 *???????????????????????????????????????????????????????
 * 1\所有结构体声明定义，要改下，2、inputdata都要改掉
 * 发送这块需要再优化，结合链表功能，以及服务器方面
 tcp,udp发送不能再malloc大块地址再弄了，应该是直接入队列或者直接交给tcp
 *?????????????????????????????????????????????????????
 *
以音频为例，客户端走阻塞socket，服务端走非阻塞
 小缓冲区（一个块，固定大小，容量小，属于环形读写，适用范围：分析流帧），大缓冲区（多个块，容量大并且可变，属于环形读写，适用范围：分析流帧、缓存功能）
 有时候如果后面很慢数据必须缓存起来否则就没了，这时候就用大缓冲区；如果后面慢大不了前面也慢这时候用小缓冲区。
客户端：
    recvstream----udpsocket---异步线程---rtp包接收缓冲区---解包成frame---外部---merge/广播（有待优化，线程间通信走pipe）---转发走sendstream
    recvstream----tcpsocket---一线程---小缓冲区---hg协议---解包成frame---外部---（解码）---播放器队列
    sendstream----判断为udp---frame拆包---异步线程---rtp包发送缓冲区---socket
    sendstream----判断为tcp---封包---socket
服务端（非阻塞）：
    recvstream----udpsocket---异步线程---rtp包接收缓冲区---解包成frame---外部---merge/广播（有待优化，线程间通信走pipe）---转发走sendstream
    recvstream----tcpsocket---一线程---小缓冲区---解包成frame---外部---转发广播等---连接的sendstream
    sendstream----判断为udp---frame拆包---异步线程---rtp包发送缓冲区---socket
    sendstream----判断为tcp---封包---每个连接的大缓冲区---socket
*/
#ifndef HUAGERTP_PROTOCOL_HGMAIN_H
#define HUAGERTP_PROTOCOL_HGMAIN_H
#define USETCP 1
#define USEUDP 2
typedef struct HgSessionC HgSessionC;
class HgSessBucket;
//before hg core runing;

typedef void (*SESSION_CALLBACK)(void *v); 

void *HgmainPro_GetSessData(HgSessionC *sess);
unsigned int HgmainPro_GetSessVFSize(HgSessionC *sess);
unsigned int HgmainPro_GetSessAFSize(HgSessionC *sess);
unsigned int HgmainPro_GetSessTFSize(HgSessionC *sess);
struct sockaddr_in *HgmainPro_GetSessAddr(HgSessionC *sess);

void HgmainPro_FreeSess(HgSessionC *sess);
void HgmainPro_HoldSess(HgSessionC *sess);
void HgmainPro_CloseSess(HgSessionC *sess);

void *HgmainPro_Alloc(HgSessionC *sess,int size);
#endif //HUAGERTP_APP_MAIN_H

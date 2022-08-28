//
// Created by ajl on 2020/12/27.
//


#ifndef HUAGE_PROTOCOL_STREAM_H
#define HUAGE_PROTOCOL_STREAM_H

#include "core/hg_queue.h"
#include "rtp_packet.h"
#include "hgsession.h"



#define ISDTRANSFER true
#define EV_HG_PTR_SIZE(data) ((uint32_t *) (data))
#define EV_HG_PTR_CMD(data) ((uint8_t *) ((data) + sizeof(uint32_t)))
#define EV_HG_PTR_TIME(data) ((uint32_t * )((data) + sizeof(uint32_t) + 1))
#define EV_HG_PTR_SSRC(data) ((uint32_t * )((data) + sizeof(uint32_t) + 1+sizeof(uint32_t)))
#define EV_HG_PTR_SESS(data) ((HgSessionC ** )((data) + sizeof(uint32_t) + 1+sizeof(uint32_t)*2))
#define EV_HG_PTR_DATA(data) ((uint8_t ** )((data) + sizeof(uint32_t) + 1+sizeof(uint32_t)*2+sizeof(void *)))
#define EV_HG_PTR_ADDR(data) ((sockaddr_in *) ((data) + sizeof(uint32_t)+1+sizeof(uint32_t)*2+sizeof(void *)*2))
#define EV_HG_PTR_FROM(data) ((uint8_t *) ((data) + sizeof(uint32_t)+1+sizeof(uint32_t)*2+sizeof(void *)*2+sizeof(sockaddr_in)))
#define EV_HG_PTR_FD(data) ((int *) ((data) + sizeof(uint32_t)+1+sizeof(uint32_t)*2+sizeof(void *)*2+sizeof(sockaddr_in)+1))
#define EV_HG_PTR_EVENTLEN ( sizeof(uint32_t)+1+sizeof(uint32_t)*2+sizeof(void *)*2+sizeof(sockaddr_in)+1+4)
typedef struct sendArr{
    void *data;
    int size;
}sendArr;
typedef struct hgEvent { //每帧的头
    uint32_t size;//前4个字节标识长度
    int8_t cmd;
    int8_t first;
    uint16_t time;//时间戳
    uint32_t ssrc;
    HgSessionC *sess;
    uint8_t *data;
    sockaddr_in sa;//only recv need assign
    int8_t from;
    int fd;
} hgEvent;

#endif //HUAGE_PROTOCOL_STREAM_H

//
// Created by ajl on 2021/11/8.
//
#include "stdint.h"
#ifndef HUAGERTP_APP_HG_CIRCLEBUF_H
#define HUAGERTP_APP_HG_CIRCLEBUF_H

#include "hg_buf_comm.h"
#include "hg_pool.h"

int Hg_Buf_WriteN(uint8_t *src, hg_Buf_Gen_t *dist, int n,void *thiss);
    //only one buffer
    int Hg_Buf_ReadN(uint8_t *dist, hg_Buf_Gen_t *src, int n, int skip);
    //big buffer chain 固定长度的buf
    void Hg_SingleDelchainL(hg_chain_t *chain, hg_chain_node **free);
    //每次都能返回一个可用的point和可用的size，length是bufer长度,
    void Hg_SinglePushR(hg_chain_t *chain, hg_chain_node *newnode);
    uint8_t *Hg_GetSingleOrIdleBuf(myPool *fragCache, hg_chain_t *chain, hg_chain_node **free, int *size,
            int cap, int nextnode,int single);
    //level 1 big buffer chain 不固定长度的buf
    void Hg_PushChainDataR(myPool *fragCache, hg_chain_t *chain, void *data);

    void Hg_ChainDelNode(myPool *fragCache,hg_chain_node *node);
    void Hg_ChainDelChain(myPool *fragCache, hg_chain_node *node);

#endif //HUAGERTP_APP_HG_CIRCLEBUF_H

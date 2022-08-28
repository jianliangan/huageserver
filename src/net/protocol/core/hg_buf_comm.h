//
// Created by ajl on 2021/12/7.
//

#ifndef HUAGERTP_APP_HG_BUF_COMM_H
#define HUAGERTP_APP_HG_BUF_COMM_H
#include <stddef.h>
typedef void (*param_handle)(void *ctx,void *data);

typedef struct selffree_s {
    void *ctx;
    void *params;
    param_handle freehandle;
    selffree_s():ctx(nullptr),params(nullptr),freehandle(nullptr){}
    void init(){
        this->ctx=nullptr;
        this->params=nullptr;
        this->freehandle=nullptr;
    }
} selffree_s;

typedef struct hg_Buf_Gen_t {
    void *data;
    int start;
    int end;//虚拟位置，最大能到cap
    int len;
    int cap;
    selffree_s sfree;
    int freenum;
    hg_Buf_Gen_t():data(nullptr),start(0),end(0),len(0),cap(0),sfree(),freenum(0){}
    void init(){
        this->data=nullptr;
        this->start=0;
        this->end=0;
        this->len=0;
        this->cap=0;
        this->sfree.params= nullptr;
        this->sfree.freehandle= nullptr;
        this->sfree.ctx=nullptr;
        this->freenum=0;
    }
    void reset(){
        this->start=0;
        this->end=0;
        this->len=0;

        this->freenum=0;

    }
} hg_Buf_Gen_t;

//可以实现无数层级的chain
typedef struct hg_chain_node {
    void *data;
    struct hg_chain_node *next;
    struct hg_chain_node *pre;
    hg_chain_node():data(nullptr),next(nullptr),pre(nullptr){}
    void init(){
        this->data= nullptr;
        this->next= nullptr;
        this->pre= nullptr;
    }
    void reset(){
        hg_Buf_Gen_t *hbgt=(hg_Buf_Gen_t *)data;
        hbgt->reset();
    }
} hg_chain_node;

typedef struct hg_chain_t {
    hg_chain_node *left;
    hg_chain_node *right;
    hg_chain_t():left(nullptr),right(nullptr){}
    void init(){
        this->left= nullptr;
        this->right= nullptr;
    }
    void reset(){
        hg_chain_node *tmp=left;
        while(tmp!=nullptr){
            tmp->reset();
            tmp=tmp->next;
        }
    }
} hg_chain_t;

#endif //HUAGERTP_APP_HG_BUF_COMM_H

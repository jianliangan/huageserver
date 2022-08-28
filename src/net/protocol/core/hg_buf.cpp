//
// Created by ajl on 2021/11/8.
//
#include "hg_buf.h"
#include <string.h>
#include <stdlib.h>

//circle small buffer


static hg_chain_node *
Hg_SingleNewNode(myPool *fragCache, hg_chain_t *chain, hg_chain_node **free, int cap,int single);

static void Hg_PushChainNodeR(hg_chain_t *hct, hg_chain_node *node);

int Hg_Buf_WriteN(uint8_t *src, hg_Buf_Gen_t *dist, int n,void *thiss) {
    int total=0;
int cpy=0;
        //if(rest>=n){
        if(dist->len==dist->cap){
            return 0;
        }
            if(dist->start<=dist->end){
                if(dist->cap-dist->end>=n){
                    memcpy((char *)dist->data+dist->end,src,n);

/*
                    char fname[255]={0};
                    sprintf(fname,APP_ROOT_"1-1_%ld.data",(uint64_t)thiss);
                    FILE *f2 = fopen(fname, "a+");
                    fwrite(src, 1, n, f2);
                    fclose(f2);
*/


                    total=n;
                    //dist->end=dist->end+n;
                    if (dist->end+n>=dist->cap) {
                        dist->end=dist->end+n-dist->cap;
                    } else {
                        dist->end=dist->end+n;
                    }
                }else{
                    int cpy=dist->cap-dist->end;
                    memcpy((char *)dist->data+dist->end,src,cpy);
/*
                    char fname[255]={0};
                    sprintf(fname,APP_ROOT_"1-1_%ld.data",(uint64_t)thiss);
                    FILE *f2 = fopen(fname, "a+");
                    fwrite(src, 1, cpy, f2);
                    fclose(f2);
*/
                    total=cpy;
                    cpy=dist->start>=(n-cpy)?(n-cpy):dist->start;
                    memcpy((char *)dist->data,src+total,cpy);
/*
                    f2 = fopen(fname, "a+");
                    fwrite(src+total, 1, cpy, f2);
                    fclose(f2);
*/

                    total+=cpy;
                    dist->end=cpy;
                }
            }else if(dist->start>dist->end){
                cpy=(dist->start-dist->end)>n?n:(dist->start-dist->end);
                memcpy((char *)dist->data+dist->end,src,cpy);
/*
                char fname[255]={0};
                sprintf(fname,APP_ROOT_"1-1_%ld.data",(uint64_t)thiss);
                FILE *f2 = fopen(fname, "a+");
                fwrite(src, 1, cpy, f2);
                fclose(f2);
*/
                total=cpy;
                dist->end=dist->end+cpy;
                if(dist->end>=dist->cap){
                    dist->end-=dist->cap;
                }
            }
            dist->len+=total;

    return total;
}
//单个buffer
int Hg_Buf_ReadN(uint8_t *dist, hg_Buf_Gen_t *src, int n, int skip) {
    if (src->len < n||src->len==0)
        return -1;
    if (src->start >= src->end) {
        if (src->cap - src->start >= n) {
            if (dist != NULL)
                memcpy(dist, ((char *)src->data + src->start), n);
        } else {
            if (dist != NULL) {
                memcpy(dist, (char *)src->data + src->start, src->cap - src->start);
                memcpy(dist + (src->cap - src->start), src->data, n - (src->cap - src->start));
            }
        }
        if (src->start+skip>=src->cap) {
            src->start = src->start+skip-src->cap;
        } else {
            src->start = src->start+skip;
        }
    } else {
        if (dist != NULL)
            memcpy(dist, (char *)src->data + src->start, n);
        src->start = src->start+skip;
        if(src->start>=src->cap){
            src->start-=src->cap;
        }
    }
    src->len -= skip;
    return 0;
}

//
void Hg_SingleDelchainL(hg_chain_t *chain, hg_chain_node **free) {

    if (chain->left != NULL) {

        hg_chain_node *hcn = chain->left->next;
        chain->left->next = (*free);
        (*free) = chain->left;
        chain->left = hcn;
        if (chain->left == NULL) {
            chain->right = NULL;
        }

    }

}

void Hg_SinglePushR(hg_chain_t *chain, hg_chain_node *newnode) {
    if (chain->right == NULL) {
        chain->right = newnode;
        chain->left = newnode;
    } else {
        chain->right->next = newnode;
        chain->right = newnode;
    }
}

hg_chain_node *
Hg_SingleNewNode(myPool *fragCache, hg_chain_t *chain,hg_chain_node **free, int cap,int single) {
    if (free == NULL || *free == NULL) {

        hg_chain_node *hcn = (hg_chain_node *) myPoolAlloc(fragCache, sizeof(hg_chain_node) );
        hg_Buf_Gen_t *hbgt = (hg_Buf_Gen_t *) myPoolAlloc(fragCache, sizeof(hg_Buf_Gen_t) + cap);
        hbgt->init();
        hbgt->data = (char *) hbgt + sizeof(hg_Buf_Gen_t);
        hbgt->cap = cap;
        hcn->init();
        hcn->data = hbgt;

        if(single==1){
            Hg_SinglePushR(chain, hcn);
        }else{
            Hg_PushChainNodeR( chain, hcn);
        }

        return hcn;
    } else {
        hg_chain_node *hcn = *free;
            (*free) = (*free)->next;
        hcn->next = NULL;
        hcn->pre=NULL;

        hg_Buf_Gen_t *hbgt;

        hbgt = (hg_Buf_Gen_t *)hcn->data;
        void *tmp=hbgt->data;
        hbgt->init();
        hbgt->cap = cap;
        hbgt->data=tmp;
        if(single==1){
            Hg_SinglePushR(chain, hcn);
        }else{
            Hg_PushChainNodeR( chain, hcn);
        }

        return hcn;
    }
}
//can't use for circle buf
uint8_t *
Hg_GetSingleOrIdleBuf(myPool *fragCache, hg_chain_t *chain, hg_chain_node **free, int *size,
                      int cap, int next,int single) {//无限右侧可写
    int tmpsize;
    hg_chain_node *rnode=chain->right;
    if (rnode != NULL && next != 1) {
        hg_Buf_Gen_t *hbgt;

        hbgt = (hg_Buf_Gen_t *)(rnode)->data;
        tmpsize = hbgt->cap - hbgt->len-hbgt->start;

        if (tmpsize>0) {
            *size = tmpsize;
            return (unsigned char *)hbgt->data + hbgt->end;
        }
    }

    hg_chain_node *hcn = Hg_SingleNewNode(fragCache,chain,free, cap,single);
    hg_Buf_Gen_t *hbgttmp = (hg_Buf_Gen_t *) hcn->data;
    tmpsize = hbgttmp->cap;
    if (size != NULL) {
        *size = tmpsize;
    }
    return (uint8_t *)hbgttmp->data;
}


//以下都是按数据报接收，支持pre，next链
hg_Buf_Gen_t *Hg_New_Buf(myPool *fragCache, int size) {
    hg_Buf_Gen_t *hbgt = (hg_Buf_Gen_t *) myPoolAlloc(fragCache, sizeof(hg_Buf_Gen_t) + size);
  hbgt->init();
    hbgt->data = (char *) hbgt + sizeof(hg_Buf_Gen_t);
    hbgt->start = 0;
    hbgt->len = size;
    hbgt->cap = size;
    hbgt->end = 0;
    return hbgt;
}

void Hg_PushChainNodeR(hg_chain_t *hct, hg_chain_node *node) {
    if (hct == NULL) {
        return;
    }
    if (hct->right == NULL) {
        hct->right = node;
        hct->left = node;
        node->pre = NULL;
        node->next = NULL;
    } else {
        hg_chain_node *oldr = hct->right;
        oldr->next = node;
        node->pre = oldr;
        hct->right = node;
        node->next=NULL;
    }
}

void Hg_PushChainDataR(myPool *fragCache, hg_chain_t *chain, void *data) {

    hg_chain_node *hcn=NULL;

    if (chain == NULL) {

    }
    hcn = (hg_chain_node *) myPoolAlloc(fragCache, sizeof(hg_chain_node));
    hcn->init();
    hcn->data = data;
    Hg_PushChainNodeR( chain, hcn);
}

void Hg_ChainDelNode(myPool *fragCache, hg_chain_node *node) {

    if (node == NULL) {
        return;
    } else {
        if (node->pre == NULL) {

            if (node->next != NULL) {
                node->next->pre = NULL;
            }

        } else {
            if (node->next == NULL) {
                    node->pre->next = NULL;
            } else {
                node->pre->next = node->next;
                node->next->pre = node->pre;
            }
        }
    }
    myPoolFree(fragCache, (void *) node);


}
void Hg_ChainDelChain(myPool *fragCache, hg_chain_node *hcn) {
    while (hcn!=NULL) {
        hg_chain_node *tmp=hcn->next;
        Hg_ChainDelNode(fragCache, hcn);
        hcn = tmp;
    }
}
//////////
//不确定带下的bufferchain结束

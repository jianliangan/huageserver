#include "hg_pool.h"
#include <assert.h>

#include <inttypes.h>
//代替malloc的
//size 应该是512或者1024，固定的这个buf就是存小内存的
myPool *myPoolCreate(uint32_t size) {
    uint32_t ret;
    myPool *mypool, *mypoolfree;
    mypool=(myPool *)malloc(sizeof(myPool)*2);
    mypool->tLen = size+ADDRHEAD;
    mypool->left = NULL;
    mypool->right = NULL;
    mypool->size = 0;
    mypoolfree=mypool+1;

    mypoolfree->tLen = mypool->tLen;
    mypoolfree->left = NULL;
    mypoolfree->right = NULL;
    mypoolfree->size = 0;
    mypool->free = mypoolfree;
    return mypool;
}

//size直接写0就行了，这个是为了校验的
myPoolNode *_myPoolNewNode(myPool *mypool, uint32_t *re) {
    void *p;
    myPoolNode *newNode;
    //first get a node，先考虑链表，然后是首位
    uint32_t tsize = mypool->tLen;

    if (mypool->free!=NULL&&mypool->free->left != NULL) {
        p = _myPoolLeftPopNode(mypool->free);
    } else {
        p = malloc(tsize +sizeof(myPoolNode));
    }
    if (p == NULL) {
        *re = 2;
        assert(0);
    }

    newNode = (myPoolNode *) p;
    newNode->d = (uint8_t *) p + sizeof(myPoolNode);
    newNode->l = NULL;
    newNode->r = NULL;
    newNode->s = 0;
    newNode->roffset=0;
    *re = 0;
    return newNode;
}

myPoolNode *_myPoolLeftPopNode(myPool *mypool) {
    myPoolNode *leftNode;
    leftNode = mypool->left;
    if (leftNode == NULL) {
        return NULL;
    }
    mypool->left = mypool->left->r;
    leftNode->l = NULL;
    leftNode->r = NULL;

    if (mypool->left != NULL)
        mypool->left->l = NULL;

    mypool->size--;
    if (mypool->size == 0) {
        mypool->right = NULL;
    }
    return leftNode;
}

uint32_t _myPoolRightPush(myPool *mypool, myPoolNode *newNode) {
    newNode->l = NULL;
    newNode->r = NULL;

    if (mypool->right != NULL) {
        newNode->l = mypool->right;
        mypool->right->r = newNode;

    }
    mypool->right = newNode;
    mypool->size++;
    if (mypool->size == 1) {
        mypool->left = mypool->right;
    }
    return mypool->size;
}


void _myPoolLeftPopEndFree(myPool *mypool, myPoolNode *node) {
    _myPoolRightPush(mypool->free, node);
}
void _myPoolMidlePop(myPool *mypool, myPoolNode *node){
    mypool->size-=1;
    if(node->l!=NULL){
        node->l->r=node->r;
    }else{
        mypool->left=node->r;
    }
    if(node->r!=NULL){
        node->r->l=node->l;
    }else{
        mypool->right=node->l;
    }

    node->l=NULL;
    node->r=NULL;
    _myPoolLeftPopEndFree(mypool,node);
}
uint8_t *myPoolAlloc(myPool *mypool,uint32_t size){

    myPoolNode *newnode;
    uint32_t re = 0;
    uint32_t *offset;
    uint32_t *mysize;
    uint32_t reals=size+ADDRHEAD;
    uint8_t *alloc;
    if(mypool->tLen<reals){
        assert(0);
    }

    if (mypool->right==NULL||(mypool->tLen-mypool->right->roffset<reals)) {
        newnode = _myPoolNewNode(mypool, &re);
        _myPoolRightPush(mypool, newnode);
        // *page=mypool->right->d;
    }

    alloc=(uint8_t *)(mypool->right->d+mypool->right->roffset);
    offset=(uint32_t *)alloc;
    *offset=mypool->right->roffset;
    mysize=(uint32_t *)(alloc+ADDRHEAD/2);
    *mysize=size;

    mypool->right->s+=reals;
    mypool->right->roffset+=reals;
    //__android_log_print(ANDROID_LOG_INFO, " 222 "," rrrrrrrrrrrrrrrrrrrrrrrrrrrr offset=%d size=%d roffset=%d s=%d tlen=%d %" PRId64 "", *offset,size,mypool->right->roffset,mypool->right->s,mypool->tLen,(int64_t)mypool->right);

    return alloc+ADDRHEAD;

}
uint32_t myPoolSizeof(uint8_t *data){
    return *(uint32_t *)(data-ADDRHEAD/2);
}
//不要重复free,
void myPoolFree(myPool *mypool,uint8_t *data){

    if(data==NULL)
        return;
    uint32_t offset=*(uint32_t *)(data-ADDRHEAD);
    uint32_t size=*(uint32_t *)(data-ADDRHEAD/2);

    //int offs=offsetof(myPoolNode);
    myPoolNode *mypooln=(myPoolNode *)(data-ADDRHEAD-offset-sizeof(myPoolNode));
    mypooln->s-=(size+ADDRHEAD);
    //__android_log_print(ANDROID_LOG_INFO, " 111 "," rrrrrrrrrrrrrrrrrrrrrrrrrrrr offset=%d size=%d roffset=%d s=%d tlen=%d %" PRId64 "", offset,size,mypooln->roffset,mypooln->s,mypool->tLen,(int64_t)mypooln);

    if(mypooln->s==0){
        _myPoolMidlePop(mypool,mypooln);
    }
}

void myPoolDestroy(myPool *mypool){
    if(mypool==NULL)
        return ;
    myPoolNode *mypooln;

    while(1){
        mypooln=_myPoolLeftPopNode(mypool);
        if(mypooln!=NULL){
            free(mypooln);
        }else{
            break;
        }
    }
    while(1){
        mypooln=_myPoolLeftPopNode(mypool->free);
        if(mypooln!=NULL){
            free(mypooln);
        }else{
            break;
        }
    }
    free(mypool);
}

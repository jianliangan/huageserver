#include "hg_queue.h"
/********
 * 需要优化，queue只做链表关系，节点存在pool上，queue也不需要紧挨着，可以免去memcopy，写的时候就写进去了
 * @param size
 * @return
 */
//size:队列内可用内存长度
//支持两种读写方式，1、流式存取（包含头部长度），2、块存取或者叫固定尺寸存取存取,不能混用
//如果流式写入，一定要流式读出；如果块写入，一定要块读出。
//流式和链式，下一步需要把节点地址暴露出来，这样可以节省内存拷贝次数,其实裸露的链式存储主要是为了减少内存copy
myQueue *myQueueCreate(uint32_t size) {
    void *p=NULL;
    myQueue *mq=NULL, *mqfree=NULL;

    p = malloc(sizeof(myQueue) * 2);
    if (p == NULL) {
        return NULL;
    }
    mq = (myQueue *) p;
    //sizeof type
    mq->tLen = size;
    mq->left = NULL;
    mq->right = NULL;
    mq->size = 0;//按节点读写有用
    mq->cap=0;//按长度读写，有用
    mq->loffset = 0;
    mq->roffset = 0;
    mqfree = (myQueue *) ((uint8_t *) p + sizeof(myQueue));
    mqfree->tLen = 0;
    mqfree->left = NULL;
    mqfree->right = NULL;
    mqfree->size = 0;
    mq->free = mqfree;
    return mq;
}

//size直接写0就行了，这个是为了校验的
myQuNode *myQuMalloc(myQueue *mq, uint32_t *re) {
    void *p=NULL;
    myQuNode *newNode=NULL;
    //first get a node，先考虑链表，然后是首位
    uint32_t tsize = mq->tLen;

    if (mq->free->left != NULL) {
        p = myQuLeftPopNode(mq->free);
    } else {
        p = malloc(tsize + sizeof(myQuNode));

    }
    if (p == NULL) {
        *re = 2;
        return NULL;
    }
    newNode = (myQuNode *) p;
    newNode->d = (uint8_t *) p + sizeof(myQuNode);
    newNode->l = NULL;
    newNode->r = NULL;
    newNode->s = 0;
    *re = 0;
    return newNode;
}
uint32_t myQuSize(myQueue *mq){
    return mq->size;
}
myQuNode *myQuLeftPopNode(myQueue *mq) {
    myQuNode *leftNode=NULL;
    leftNode = mq->left;
    if (leftNode == NULL) {
        return NULL;
    }
    mq->left = mq->left->r;
    leftNode->l = NULL;
    leftNode->r = NULL;
    mq->loffset = 0;
    if (mq->left != NULL)
        mq->left->l = NULL;

    mq->size--;
    if (mq->size == 0) {
        mq->right = NULL;
    }
    return leftNode;
}

uint32_t myQuRightPush(myQueue *mq, myQuNode *newNode) {
    newNode->l = NULL;
    newNode->r = NULL;

    if (mq->right != NULL) {
        newNode->l = mq->right;
        mq->right->r = newNode;

    }
    mq->right = newNode;
    mq->size++;
    if (mq->size == 1) {
        mq->left = mq->right;
    }
    return mq->size;
}


void myQuLeftPopEndFree(myQueue *mq, myQuNode *node) {
    node->s = 0;
    myQuRightPush(mq->free, node);
}

//从右面写入
void myQuStreamW(myQueue *mq, uint8_t *src, uint32_t size) {
    myQuNode *newnode=NULL;
    uint32_t re = 0;
    uint32_t total = size;
    uint32_t shouldsize = 0;
    while (total > 0) {
        if (mq->roffset == mq->tLen||(mq->right==NULL)) {
            shouldsize = HG_min(total, mq->tLen);
            newnode = myQuMalloc( mq,&re);
            memcpy(newnode->d, src + (size - total), shouldsize);
            total -= shouldsize;
            mq->roffset = shouldsize;
            myQuRightPush(mq, newnode);
        } else {
            shouldsize = HG_min(total, mq->tLen - mq->roffset);
            memcpy(mq->right->d + mq->roffset, src + (size - total), shouldsize);
            total -= shouldsize;
            mq->roffset += shouldsize;
        }

    }
    mq->cap+=size;
}
//最满意的函数了
uint32_t myQuStreamL(myQueue *mq, uint8_t *dst, uint32_t size, enum enumispeek peek) {
    uint32_t total = size;//还剩total这么多要取
    uint32_t shouldsize = 0;
    uint32_t loffset = 0;
    myQuNode *node = mq->left;
    myQuNode *nodetmp=NULL;
    if (node == NULL||mq->cap<size) {
        return 0;
    }
    loffset = mq->loffset;
    while (total > 0) {
        if (node == NULL) {
            break;
        }
        shouldsize = HG_min(total, mq->tLen - loffset);
        memcpy(dst + (size - total), node->d + loffset, shouldsize);
        loffset += shouldsize;
        total -= shouldsize;
        if (loffset == mq->tLen) {
            node = node->r;
            loffset = 0;
            if (peek == NOHGISNEEDPEEK) {
                nodetmp=myQuLeftPopNode(mq);
                myQuLeftPopEndFree(mq, nodetmp);
            }
        }
        if (peek == NOHGISNEEDPEEK) {
            mq->loffset = loffset;
        }


    }
    if(peek==NOHGISNEEDPEEK){
        mq->cap-=(size - total);
    }
    return size - total;
}
void myQuDestroy(myQueue *mq){
    myQuNode *mqn=NULL;
    if(mq==NULL)
        return ;
    while(1){
        mqn=myQuLeftPopNode(mq);
        if(mqn!=NULL){
            free(mqn);
        }else{
            break;
        }
    }
    while(1){
        mqn=myQuLeftPopNode(mq->free);
        if(mqn!=NULL){
            free(mqn);
        }else{
            break;
        }
    }
    free(mq);
}

void myQuClear(myQueue *mq){
    myQuNode *mqn=NULL;

    while(1){
        mqn=myQuLeftPopNode(mq);
        if(mqn==NULL){
            break;
        }
    }

}

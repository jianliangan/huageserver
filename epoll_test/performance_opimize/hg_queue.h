#ifndef HUAGE_CORE_MYQUEUE_
#define HUAGE_CORE_MYQUEUE_
#include "tools.h"
/**
 * queue+buffe
 **/
enum enumispeek{
    HGISNEEDPEEK,NOHGISNEEDPEEK
};
typedef struct myQuNode_1 {
    void *d;
    uint32_t s;
    struct myQuNode_1 *l;
    struct myQuNode_1 *r;
} myQuNode ;

typedef struct myQueue_1 {
    myQuNode *left;
    myQuNode *right;
    uint32_t tLen;//节点可用存储区最大长度
    uint32_t size;
    uint32_t cap;
    uint32_t loffset;//当前left节点偏移
    uint32_t roffset;//当前left节点偏移
    struct myQueue_1 *free;
} myQueue ;

#ifdef __cplusplus
extern "C"{
#endif
myQueue *myQueueCreate(uint32_t size);
myQuNode *myQuMalloc(myQueue *mq,uint32_t*ret);
myQuNode *myQuLeftPopNode(myQueue *mq);
uint32_t myQuSize(myQueue *mq);
void myQuStreamW(myQueue *mq,uint8_t *src,uint32_t size);
uint32_t myQuStreamL(myQueue *mq, uint8_t *dst, uint32_t size, enum enumispeek peek);
uint32_t myQuRightPush(myQueue *mq, myQuNode *newNode);
void myQuDestroy(myQueue *mq);
void myQuLeftPopEndFree(myQueue *mq, myQuNode *node);
void myQuClear(myQueue *mq);
#ifdef __cplusplus
}
#endif
#endif

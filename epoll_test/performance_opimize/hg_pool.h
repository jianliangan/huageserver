#ifndef HUAGE_CORE_MYPOOL_
#define HUAGE_CORE_MYPOOL_
#include "tools.h"
/**
 * queue+buffe
 * */
#define ADDRHEAD 8
typedef struct myPoolNode_1 {
    void *d;
    struct myPoolNode_1 *l;
    struct myPoolNode_1 *r;
    uint32_t s;
    uint32_t roffset;//最大撑起范围，用来计算剩余可用位置

} myPoolNode ;

typedef struct myPool_1 {
    myPoolNode *left;
    myPoolNode *right;
    struct myPool_1 *free;

    uint32_t tLen;//节点可用存储区最大长度
    uint32_t size;
} myPool ;

#ifdef __cplusplus
extern "C"{
#endif
    myPool *myPoolCreate(uint32_t size);
    uint8_t *myPoolAlloc(myPool *mypool,uint32_t size);
    uint32_t myPoolSizeof(uint8_t *data);
    void myPoolFree(myPool *mypool,uint8_t *data);
    void myPoolDestroy(myPool *mypool);
#ifdef __cplusplus
}
#endif
myPoolNode *_myPoolNewNode(myPool *mypool,uint32_t *ret);
myPoolNode *_myPoolLeftPopNode(myPool *mypool);
void _myPoolMidlePop(myPool *mypool, myPoolNode *node);
uint32_t _myPoolRightPush(myPool *mypool, myPoolNode *newNode);
void _myPoolLeftPopEndFree(myPool *mypool, myPoolNode *node);
#endif

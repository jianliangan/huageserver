#ifndef HUAGE_CORE_TOP__
#define HUAGE_CORE_TOP__

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <semaphore.h>
#include <assert.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include<sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include <unistd.h>
#include "hg_log.h"
#define hgAlignPtr(p, a)                                                   \
    (uint8_t *) (((uintptr_t) (p) + ((uintptr_t) (a) - 1)) & ~((uintptr_t) (a) - 1))
#define HG_min(x, y) (((x) < (y)) ? (x) : (y))
#define HG_max(x, y) (((x) > (y)) ? (x) : (y))


#define hg_pthread_lock(p) hg_pthread_mutex_lock(__FILE__, __LINE__, __FUNCTION__,p)
#define hg_pthread_unlock(p) hg_pthread_mutex_unlock(__FILE__, __LINE__, __FUNCTION__,p)
typedef struct vatAddr {
    const char *ip;
    uint32_t port;
    struct sockaddr_in serverAddr;
    uint32_t addrLength;
} vatAddr;
typedef char * yuvchar;
#ifdef __cplusplus
extern "C"{
#endif

void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest);
void
YUV_420_888toNV21(yuvchar data, yuvchar planeY, yuvchar planeU, yuvchar planeV, int imWidth,
                  int imHeight,
                  int pitchY, int pitchU, int pitchV, int Ulenth, int pixPitchY, int pixPitchUV,
                  int rotate);
void
YUV_420_888toIyuv(yuvchar data, yuvchar planeY, yuvchar planeU, yuvchar planeV, int imWidth,
                  int imHeight,
                  int pitchY, int pitchU, int pitchV, int Ulenth, int pixPitchY, int pixPitchUV,
                  int rotate);
void toolsIntBigEndianV(uint8_t *d, uint8_t *val, uint32_t digits);
void toolsIntBigEndian(uint8_t *d, uint32_t val, uint32_t digits);
void toolsIntLitEndian(uint8_t *d, uint32_t val, uint32_t digits);
int uint16Sub(uint16_t newd,uint16_t oldd);
uint16_t uint16Add(uint16_t add1,uint16_t add2);
void init16Num();
int hgSemWait(sem_t * sem);
uint64_t hgetSysTimeMicros();
void  hg_nsleep(unsigned int miliseconds);
int checkLitEndian();
int hg_pthread_mutex_lock(const char *file,uint32_t line,const char *fun,pthread_mutex_t *pmt);
int hg_pthread_mutex_unlock(const char *file,uint32_t line,const char *fun,pthread_mutex_t *pmt);
void PrintBuffer(void* pBuff,uint32_t f1, uint32_t nLen);
uint32_t uint32Add(uint32_t add1,uint32_t add2);
int uint32Sub(uint32_t newd,uint32_t oldd);

static inline int _spin_init(int *lock, int pshared) {
    __asm__ __volatile__ ("" ::: "memory");
    *lock = 0;
    return 0;
}

static inline int _spin_destroy(int *lock) {
    return 0;
}

static inline int _spin_lock(int *lock) {
    while (1) {
        int i;
        for (i=0; i < 10000; i++) {
            if (__sync_bool_compare_and_swap(lock, 0, 1)) {
                return 0;
            }
        }
        sched_yield();
    }
}

static inline int _spin_trylock(int *lock) {
    if (__sync_bool_compare_and_swap(lock, 0, 1)) {
        return 0;
    }
    return -1;
}

static inline int _spin_unlock(int *lock) {
    __asm__ __volatile__ ("" ::: "memory");
    *lock = 0;
    return 0;
}
#ifdef __cplusplus
}
#endif
#endif

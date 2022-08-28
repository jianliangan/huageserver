#ifndef _HG_TIMER_
#define _HG_TIMER_
#include <map>
#include <pthread.h>
#include "../net/protocol/core/tools.h"
#define HGSIG SIGRTMIN
typedef void(*timerHandle)(void *);
typedef struct hgTimerEvent{
    timerHandle handle;
    void * param;
}hgTimerEvent;
class HgTimer{
    public:
        HgTimer();
        static bool isbusy;
        pthread_t threadIn;
        static timer_t timerid;
        sem_t semtimer;
        static uintptr_t hgCurrentTime;
        static std::multimap<uintptr_t,struct hgTimerEvent> events;
        void Handle();
        static void *Handle0(void *);
        std::multimap<uintptr_t,hgTimerEvent>::iterator  AddEvent(hgTimerEvent &,uintptr_t);
        void DelEvent(std::multimap<uintptr_t,hgTimerEvent>::iterator);
};
extern HgTimer *ht;
#endif

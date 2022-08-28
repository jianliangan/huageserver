#include "../app/timer.h"
bool HgTimer::isbusy;
timer_t HgTimer::timerid;
uintptr_t HgTimer::hgCurrentTime;
std::multimap<uintptr_t,struct hgTimerEvent> HgTimer::events;
HgTimer *ht=nullptr;
HgTimer::HgTimer(){
    pthread_create(&threadIn, NULL,Handle0, this);
    sem_init(&semtimer, 0, 0);
}
void *HgTimer::Handle0(void *ctx){
    HgTimer *ht=(HgTimer *)ctx;
    struct timespec ts;
    while(true){
        //clock_gettime(CLOCK_REALTIME,&ts);
        //ts.tv_nsec += 50000000;
        //ht->handle();
        //printf("dddd\n");
        usleep(50000);
        // sem_timedwait(&ht->semtimer,&ts);
    }
}
void HgTimer::Handle(){
    if(HgTimer::isbusy)
        return;
    HgTimer::isbusy=true;
    std::multimap<uintptr_t,struct hgTimerEvent>::iterator it;
    hgCurrentTime=hgetSysTimeMicros()/1000;
    timerHandle handle2;
    void * param2;
    //    ALOGI(0,"heartbeat\n");
    do{
        it=events.begin();
        if(it!=events.end()){
            if(it->first<hgCurrentTime){
                handle2=it->second.handle;
                param2=it->second.param;
                events.erase(it);
                handle2(param2);
            }else{
                break;
            }
        }else{

        }

    }while(it!=events.end());
    isbusy=false;
}
void HgTimer::DelEvent(std::multimap<uintptr_t,hgTimerEvent>::iterator it){
    events.erase(it);

}
std::multimap<uintptr_t,hgTimerEvent>::iterator HgTimer::AddEvent(hgTimerEvent &hgev,uintptr_t expire){
    std::multimap<uintptr_t,hgTimerEvent>::iterator it;
    it=events.insert(std::pair<uintptr_t,hgTimerEvent>(expire,hgev));
    return it;
}


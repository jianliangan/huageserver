//
// Created by ajl on 2020/12/27.
//


#ifndef HUAGE_WORKER_H
#define HUAGE_WORKER_H
#include <semaphore.h>
#include "hg_tevent.h"
#include <pthread.h>
#include <unistd.h>
#include "hg_channel.h"
#include "../hg_com.h"
class HgWorker {
public:
    void *sess_s;
    //void *conn;
    HgWorker();
    static HgWorker *GetWorker(uint32_t index);
    void WriteChanWor(void *data,int size);
    ~HgWorker();

public:
    pthread_t streamth;
    HgChannel hg_chan;
    static void *DoWork(void *ctx);
    static void *PipeLoop(void *ctx);
};
extern HgWorker *hgworkers[WORKERNUM];
#endif //HUAGE_WORKER_H

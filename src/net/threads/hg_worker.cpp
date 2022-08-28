//
// Created by ajl on 2020/12/27.
//
#include "hg_worker.h"

HgWorker *hgworkers[WORKERNUM];

HgWorker::HgWorker() {
    pthread_create(&streamth, NULL, HgWorker::DoWork, this);
}

void *HgWorker::DoWork(void *ctx) {
    HgWorker *hgworker = (HgWorker *) ctx;

    hgtEvent *hgtevent;
    while (true) {
        hgworker->hg_chan.Drive(hgworker, 1);
    }

    // }
    pthread_exit(NULL);

}

HgWorker *HgWorker::GetWorker(uint32_t index) {
    return hgworkers[index % WORKERNUM];
}

void HgWorker::WriteChanWor(void *data,int size) {
    hg_chan.WriteChan(data,size, 1);
}

HgWorker::~HgWorker() {
}

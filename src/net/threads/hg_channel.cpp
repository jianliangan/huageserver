//
// Created by ajl on 2021/12/22.
//

#include "hg_channel.h"
#include "../protocol/hg_channel_stru.h"
#include <sys/syscall.h>

#define HGCHANNELLENMAX 4 * 1024
#define MAX_BUF_BLOSIZE 512

HgChannel::HgChannel() {
    fragCache = myPoolCreate(HGCHANNELLENMAX);

    tcpbuf.data = new char[HGCHANNELLENMAX];
    tcpbuf.cap = HGCHANNELLENMAX;

    sem_init(&eventSem, 0, 0);
    _spin_init(&lock, 0);

    lock = 0;

}

int HgChannel::WriteChan(void *data, int size, int wait) {
    //the same to pipe
    int canwrsize = 0;
    uint8_t *recvLine;
    int rest = size;
    int realcp = 0;
    if (size <= 0) {
        return 0;
    }
    ttt += size;
    hg_chain_node *hcn = nullptr;
    hg_Buf_Gen_t *hbgt = nullptr;

    hgtEvent *head = (hgtEvent *) data;


    _spin_lock(&lock);


    char fname[255]={0};
    FILE *f2;
    /*sprintf(fname,APP_ROOT_"0_%ld.data",(uint64_t)this);
    f2 = fopen(fname, "a+");
    fwrite(data, 1, size, f2);
    fclose(f2);
    */
/*
       char bocontent[500]={0};
       sprintf(bocontent,"%d-psize-%d-handle-%s-ctx-%ld \n", (int)(size - sizeof(hgtEvent)), head->psize,
       eventstr[head->i], (long)head->ctx);
       sprintf(fname,APP_ROOT_"w_%ld.data",(long)this);
       f2 = fopen(fname, "a+");
       fwrite(bocontent, 1, strlen(bocontent), f2);
       fclose(f2);
*/
    while (1) {
        hg_Buf_Gen_t *hbbbb = nullptr;
        if (hct.right != nullptr) {
            hbbbb = (hg_Buf_Gen_t *) hct.right->data;
        }
        int starttmp = (hbbbb == nullptr ? -1 : hbbbb->start);
        int lentmp = (hbbbb == nullptr ? -1 : hbbbb->len);
        int endtmp = (hbbbb == nullptr ? -1 : hbbbb->end);
        recvLine = (uint8_t *) Hg_GetSingleOrIdleBuf(fragCache, &hct,
                &freen, &canwrsize,
                MAX_BUF_BLOSIZE, 0, 1);
        realcp = canwrsize <= rest ? canwrsize : rest;

        //ALOGI(0,
        //        "%ld 22222222222 write realcp %d rest %d canwrsize %d line %ld data %ld size %d prestart %d prelen %d preend %d",
       //         (uint64_t)this,realcp, rest, canwrsize, recvLine, data, size, starttmp, lentmp, endtmp);
        memcpy(recvLine, (char *) data + (size - rest), realcp);

        hcn = hct.right;
        hbgt = (hg_Buf_Gen_t *) hcn->data;

        hbgt->len += realcp;
        hbgt->end += realcp;
        if (hbgt->end >= hbgt->cap) {
            hbgt->end -= hbgt->cap;
        }
        rest -= realcp;
       // ALOGI(0, "%ld 3333333333333333333 1 %d %d %ld", (uint64_t)this,canwrsize, rest, recvLine);
        if (rest == 0) {
            break;
        }
    }
    _spin_unlock(&lock);
    //the same to pipe
    if (wait == 1) {
        sem_post(&eventSem);
    }
    return 0;
}

void HgChannel::Drive(void *data, int wait) {

    int lentmp = 0;

    if (wait == 1) {
        while (sem_wait(&eventSem) == -1) {
        };
    }
    _spin_lock(&lock);

    hg_chain_node *hcnleft = hct.left;
    hg_Buf_Gen_t *hbgt = nullptr;
    int total = 0;




    while (hcnleft != nullptr) {

        hbgt = (hg_Buf_Gen_t *) hcnleft->data;

        total = Hg_Buf_WriteN((uint8_t *) hbgt->data + hbgt->start, &tcpbuf, hbgt->len,this);
        if (total != 0) {
/*
            char fname[255]={0};
            sprintf(fname,APP_ROOT_"1_%ld.data",(uint64_t)this);
            FILE *f2 = fopen(fname, "a+");
            fwrite((uint8_t *) hbgt->data + hbgt->start, 1, total, f2);
            fclose(f2);
*/
        }


        //  ALOGI(0,"111111111 read44 %ld start %d end %d len %d total %d  ttt %ld",hbgt->data,hbgt->start,hbgt->end,hbgt->len,ttt1,ttt);
        //   ALOGI(0,"111111111 read55 start %d end %d len %d",tcpbuf.start,tcpbuf.end,tcpbuf.len);
        if (total < hbgt->len) {
            hbgt->start += total;
            if (hbgt->start >= hbgt->cap) {
                hbgt->start -= hbgt->cap;
            }
            hbgt->len -= total;
            break;
        } else if (total == hbgt->len) {
            hcnleft = hcnleft->next;
            Hg_SingleDelchainL(&hct, &freen);
        }
    }

    _spin_unlock(&lock);
    Handlemsg(data, &tcpbuf);
}

void HgChannel::Handlemsg(void *data, hg_Buf_Gen_t *tcpb) {
    uint8_t body[HGCHANNELLENMAX];
    uint8_t *tmp = nullptr;
    int headlen = sizeof(hgtEvent);
    hgtEvent head;
    int willgets = headlen;
    int skip = 0;

    int gethead = 0;

    while (true) {
        if (gethead == 0) {
            tmp = (unsigned char *) &head;
            willgets = headlen;
            skip = headlen;

        } else {
            tmp = body;
            willgets = head.psize;
            skip = head.psize;

        }
       // ALOGI(0, "%ld 111111111 read55  write start %d end %d len %d headlen %d gethead %d willgets %d",
        //        (uint64_t)this,tcpb->start, tcpb->end, tcpb->len, sizeof(hgtEvent), gethead, willgets);
        if (Hg_Buf_ReadN(tmp, tcpb, willgets, skip) == 0) {

            if (gethead == 0) {
               // ALOGI(0, "%ld 111111111 read0 will-%d-psize-%d-handle-%s-ctx-%ld", (uint64_t)this,willgets, head.psize,
              //          eventstr[head.i], head.ctx);
                gethead = 1;
            } else {
                //void *pth,void *ctx,void *params,int psize

                char fname[255]={0};
                FILE *f2;
/*                sprintf(fname,APP_ROOT_"2_%ld.data",(uint64_t)this);
                f2 = fopen(fname, "a+");
                fwrite(&head, 1, sizeof(head), f2);
                fwrite(tmp, 1, willgets, f2);
                fclose(f2);
*/

/*
                   char bocontent[500]={0};
                   sprintf(bocontent,"%d-psize-%d-handle-%s-ctx-%ld \n", willgets, head.psize,
                   eventstr[head.i], (long)head.ctx);
                   sprintf(fname,APP_ROOT_"r_%ld.data",(long)this);
                   f2 = fopen(fname, "a+");
                   fwrite(bocontent, 1, strlen(bocontent), f2);
                   fclose(f2);
*/
                head.handle(data, head.ctx, tmp, head.psize);

                gethead = 0;
            }//free

        } else {
           // ALOGI(0, "111111111 read0 will7777");
            if (gethead == 1) {
                if (tcpb->start >= headlen) {
                    tcpb->start -= headlen;
                } else {
                    tcpb->start = tcpb->cap - (headlen - tcpb->start);
                    if (tcpb->start >= tcpb->cap) {
                        tcpb->start -= tcpb->cap;
                    }
                }
                tcpb->len += headlen;
            }
            break;
        }

    }


}


#ifndef _HANDLER_CONSUMER_
#include <cstdint>
#include <cassert>
#include "../net/protocol/core/tools.h"
#define _HANDLER_CONSUMER_
class MsgObjHead{
    uint8_t type;
    uint8_t count;
};
class MsgObj0{
    public:
        uint32_t ssrc;
        uint16_t vpts;
        uint16_t apts;
        MsgObj0();
        static int getdelen(uint8_t *source,int size);
        static int getenlen(MsgObj0 *source,int size);
        static void decodeone(uint8_t *source,int ssize,MsgObj0 *dist);
        static void encode(uint8_t *dist,MsgObj0 *source,int ssize);
        static void decode(uint8_t *source,int ssize,MsgObj0 *dist,int dsize);
};
#endif

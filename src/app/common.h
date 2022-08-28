#ifndef _HUAGE_COMMON_HEAD_
#include <cstdio>
#include "../base/json.hpp"
#define _HUAGE_COMMON_HEAD_

enum AUDIO_codecs{
    AUDIO_G711
};

typedef struct audio_config_t{
    int channels;
    int deep;
    int samplesrate;
    int frameperbuf;
    int rframeperbuf;
    AUDIO_codecs codec;
    void init(){
        channels=0;
        deep=0;
        samplesrate=0;
        frameperbuf=0;
        rframeperbuf=0;
        codec=AUDIO_G711;
    }
} audio_config_t;
inline void resJsoHead(uint32_t uniqid,std::string status,nlohmann::json &jss,nlohmann::json const &body){

    jss["v"]= 0;
    jss["i"]=uniqid;
    jss["b"]=status;
    jss["res"]=body;
}
#endif

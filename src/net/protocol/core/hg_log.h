#ifndef   HUAGE_CORE_MYPOOL_LogModule
#define   HUAGE_CORE_MYPOOL_LogModule
#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>

#ifdef __this_android__
#include <android/log.h>
#define APP_ROOT_ "/data/data/org.libhuagertp.app/"
#define ALOG(level, f1, ...)    my__android_log_print(level, f1, __VA_ARGS__)
#define ALOGV(f1,...)  ALOG(ANDROID_LOG_VERBOSE,f1,  __VA_ARGS__)
#define ALOGD(f1,...)  ALOG(ANDROID_LOG_DEBUG,f1,  __VA_ARGS__)
#define ALOGI(f1,...)  ALOG(ANDROID_LOG_INFO,f1,  __VA_ARGS__)
#define ALOGW(f1,...)  ALOG(ANDROID_LOG_WARN,f1,  __VA_ARGS__)
#define ALOGE(f1,...)  ALOG(ANDROID_LOG_ERROR,f1,  __VA_ARGS__)
#ifdef __cplusplus
extern "C"{
#endif
    void my__android_log_print(uint32_t level, uint32_t tag,const char *fmt, ...);
#ifdef __cplusplus
}
#endif
#else
#define APP_ROOT_ "/root/huagertp_linux/build/log/"
#define LOG_TRACE(i,log_fmt, log_arg...) \
    do{ \
        hg_log(i,   "[%s:%d][%s] " log_fmt "\n", \
                __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)
#define ALOGI(i,m,n...) LOG_TRACE(i,m,##n)
#define ALOGE(i,m,n...) LOG_TRACE(i,m,##n)
#ifdef __cplusplus
extern "C"{
#endif
    void hg_log(int level,const char *fmt,...);
    int hg_log_init();
#ifdef __cplusplus
}
#endif
#endif
#endif

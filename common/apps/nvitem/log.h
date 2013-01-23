
#define NVITEMLOG
#define NVITEMDEBUG
#define LOGTOCARD

#include <utils/Log.h>

#define BUFFER_LEN		(128)

extern char gch;
extern FILE *gfp;
extern char gbuffer[];

#ifdef NVITEMDEBUG
#define NVITEM_DEBUG(fmt, arg...) ALOGD("NVITEM:%s "fmt, __FUNCTION__ , ## arg)
#else
#define NVITEM_DEBUG(fmt...)
#endif /* End #ifdef NVITEMDEBUG */

#ifdef NVITEMLOG
#define NVITEM_LOG(fmt, arg...) ALOGD(fmt, ## arg)
#else
#define NVITEM_LOG(fmt, arg...) 
#endif /* End #ifdef NVITEMLOG */

#ifdef LOGTOCARD
#define LOG2CARD(fmt, arg...) do{memset(gbuffer, 0, BUFFER_LEN);sprintf(gbuffer,fmt,## arg);fputs(gbuffer, gfp);fflush(gfp);} while(0)
#else
#define LOG2CARD(fmt, arg...) 
#endif /* End #ifdef LOGTOCARD */

int init_nvitem_log();

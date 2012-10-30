
#define NVITEMLOG
#define NVITEMDEBUG

#include <utils/Log.h>

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

int init_nvitem_log();

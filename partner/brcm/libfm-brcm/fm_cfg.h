#ifndef fm_cfg
//#define HAVE_BLUETOOTH
#define IS_STANDALONE_FM TRUE

/* Use BTL-IF */
#ifndef BRCM_BT_USE_BTL_IF
#define BRCM_BT_USE_BTL_IF
#endif

//Uncomment to use BTAPP calls directly
//#define BRCM_USE_BTAPP


//#include <jni.h>
//#include <JNIHelp.h>
//#include <android_runtime/AndroidRuntime.h>

//Define logging calls
#include <android/log.h>

#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGV(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)

#endif

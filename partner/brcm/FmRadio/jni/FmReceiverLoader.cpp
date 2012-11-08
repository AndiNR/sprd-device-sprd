#define BTA_FM_INCLUDED TRUE
#include "fm_cfg.h"
#include <jni.h>
#include <string.h>
#define LOG_TAG "FmRadioLoader"

extern int register_com_broadcom_bt_service_fm_FmReceiverService(JNIEnv *env);






jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGE("******************JNI_OnLoad called!!!!!**********\n");
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("ERROR: GetEnv failed\n");
        goto bail;
    }

    if (register_com_broadcom_bt_service_fm_FmReceiverService(env) != JNI_TRUE) {
            LOGE("ERROR: registerNatives failed");
            goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

    bail: return result;
}



/*
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#define DBUS_ADAPTER_IFACE BLUEZ_DBUS_BASE_IFC ".Adapter"
#define DBUS_DEVICE_IFACE BLUEZ_DBUS_BASE_IFC ".Device"
#define LOG_TAG "PowerManagementService.cpp"

#include "android_bluetooth_common.h"
#include "android_runtime/AndroidRuntime.h"
#include "JNIHelp.h"
#include "jni.h"
#include "utils/Log.h"
#include "utils/misc.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define HAVE_BLUETOOTH true
#ifdef HAVE_BLUETOOTH
//#include <dbus/dbus.h>
#include <bluedroid/bluetooth.h>
#include <bluedroid/brcmfm.h>
#endif

#include <cutils/properties.h>

namespace android {

#define BLUETOOTH_CLASS_ERROR 0xFF000000
#define PROPERTIES_NREFS 10

#ifdef HAVE_BLUETOOTH
#endif

// We initialize these variables when we load class
// android.server.BluetoothService


static void classInitNative(JNIEnv* env, jclass clazz) {
    ALOGV(__FUNCTION__);
#ifdef HAVE_BLUETOOTH
#endif
}

/* Returns true on success (even if adapter is present but disabled).
 * Return false if dbus is down, or another serious error (out of memory)
*/
static bool initializeNativeDataNative(JNIEnv* env, jobject object) {
    ALOGV(__FUNCTION__);
#ifdef HAVE_BLUETOOTH

#endif  /*HAVE_BLUETOOTH*/
    return true;
}

static void cleanupNativeDataNative(JNIEnv* env, jobject object) {
    ALOGV(__FUNCTION__);
#ifdef HAVE_BLUETOOTH    
    
#endif
}

static jint enableFmNative(JNIEnv *env, jobject object) {
#ifdef HAVE_BLUETOOTH
    ALOGV(__FUNCTION__);
    return fm_enable();
#endif
    return -1;
}

static jint disableFmNative(JNIEnv *env, jobject object) {
#ifdef HAVE_BLUETOOTH
    ALOGV(__FUNCTION__);
    return fm_disable();
#endif
    return -1;
}

static jint enableBtNative(JNIEnv *env, jobject object) {
#ifdef HAVE_BLUETOOTH
    ALOGV(__FUNCTION__);
    return bt_enable();
#endif
    return -1;
}

static jint disableBtNative(JNIEnv *env, jobject object) {
#ifdef HAVE_BLUETOOTH
    ALOGV(__FUNCTION__);
    return bt_disable();
#endif
    return -1;
}

static jint getCurrentStateNative(JNIEnv *env, jobject object) {
#ifdef HAVE_BLUETOOTH
    ALOGV(__FUNCTION__);
    return get_current_state();
#endif
    return -1;
}

static JNINativeMethod sMethods[] = {
     /* name, signature, funcPtr */
    {"classInitNative", "()V", (void*)classInitNative},
    {"initializeNativeDataNative", "()V", (void *)initializeNativeDataNative},
    {"cleanupNativeDataNative", "()V", (void *)cleanupNativeDataNative},
  
	{"enableFmNative", "()I", (void *)enableFmNative},
    {"disableFmNative", "()I", (void *)disableFmNative},
	{"enableBtNative", "()I", (void *)enableBtNative},
    {"disableBtNative", "()I", (void *)disableBtNative},
    {"getCurrentStateNative", "()I", (void *)getCurrentStateNative},    
};

/**
 * Register several native methods for one class.
 */
static int registerNativeMethods(JNIEnv* env, const char* className,
        JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;

    clazz = env->FindClass(className);
    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'", className);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        ALOGE("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}
} /* namespace android */

/*
 * Register native methods for all classes we know about.
 *
 * returns JNI_TRUE on success.
 */
int register_com_broadcom_bt_service_PowerManagementService(JNIEnv *env)
{
    if (!android::registerNativeMethods(env, "com/broadcom/bt/service/PowerManagementService",
            android::sMethods, sizeof(android::sMethods) / sizeof(android::sMethods[0]))) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

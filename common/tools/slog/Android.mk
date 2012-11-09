LOCAL_PATH:= $(call my-dir)

#slog
include $(CLEAR_VARS)
LOCAL_SRC_FILES := slog.c handler.c parse_conf.c screenshot.c
LOCAL_MODULE := slog
LOCAL_STATIC_LIBRARIES := libcutils libc
LOCAL_MODULE_TAGS := optional
ifeq ($(PLATFORM_VERSION),4.1.2)
LOCAL_CFLAGS += -DSLOG_ALOGD_ALOGE
endif
LOCAL_LDLIBS += -lpthread
LOCAL_C_INCLUDES += external/jpeg external/zlib
LOCAL_SHARED_LIBRARIES := liblog libz libjpeg
include $(BUILD_EXECUTABLE)

#slogctl
include $(CLEAR_VARS)
LOCAL_SRC_FILES := slogctl.c
LOCAL_MODULE := slogctl
LOCAL_STATIC_LIBRARIES := libcutils libc
LOCAL_MODULE_TAGS := optional
ifeq ($(PLATFORM_VERSION),4.1.2)
LOCAL_CFLAGS += -DSLOG_ALOGD_ALOGE
endif
LOCAL_LDLIBS += -lpthread
LOCAL_C_INCLUDES += external/jpeg external/zlib
LOCAL_SHARED_LIBRARIES := liblog libz libjpeg
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := slog.conf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

CUSTOM_MODULES += slog
CUSTOM_MODULES += slogctl
CUSTOM_MODULES += slog.conf

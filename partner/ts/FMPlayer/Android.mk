LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifneq ($(TARGET_PRODUCT_CARRIER), CTA)
    LOCAL_MODULE_TAGS := user eng
endif
LOCAL_CERTIFICATE := platform

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src) \
	src/com/thunderst/radio/IRadioService.aidl

LOCAL_PACKAGE_NAME := FMPlayer

include $(BUILD_PACKAGE)

# Use the following include to make our test apk.
#include $(call all-makefiles-under,$(LOCAL_PATH))

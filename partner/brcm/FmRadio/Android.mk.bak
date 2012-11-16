ifeq ($(BOARD_HAVE_BLUETOOTH_BCM),true)
ifeq ($(BOARD_HAVE_FM_BCM),true)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-subdir-java-files) \
		   $(call all-Iaidl-files-under) 

LOCAL_PACKAGE_NAME := FmRadio

LOCAL_JAVA_LIBRARIES := com.broadcom.bt.service
LOCAL_JNI_SHARED_LIBRARIES := libfmservice

LOCAL_PROGUARD_ENABLED:=disabled

LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
endif

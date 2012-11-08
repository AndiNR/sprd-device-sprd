ifeq ($(BOARD_HAVE_BLUETOOTH_BCM),true)
ifeq ($(BOARD_HAVE_FM_BCM),true)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

# Relative path from current dir to vendor brcm

LOCAL_SRC_FILES := $(call all-subdir-java-files) \
		   $(call all-Iaidl-files-under) 

LOCAL_MODULE := com.broadcom.bt.service
LOCAL_MODULE_CLASS := JAVA_LIBRARIES

LOCAL_JNI_SHARED_LIBRARIES := libfmpmservice libbluedroid

LOCAL_PROGUARD_ENABLED:=disabled


include $(BUILD_JAVA_LIBRARY)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
endif

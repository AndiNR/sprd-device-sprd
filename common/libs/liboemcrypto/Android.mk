LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/include \
                $(TOP)/device/sprd/common/apps/engineeringmodel/engcs \
                $(TOP)/external/openssl/include 
              

LOCAL_SRC_FILES:= \
		  src/sprdoemcrypto.c

LOCAL_MODULE_PATH := $(LOCAL_PATH)

LOCAL_MODULE := liboemcrypto

LOCAL_C_INCLUDES += $(common_C_INCLUDES)

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES:=liblog libcutils libc libengclient

include $(BUILD_STATIC_LIBRARY)

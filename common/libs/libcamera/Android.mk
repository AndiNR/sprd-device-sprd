ifeq ($(strip $(SPRDROID4.3_DEV)),true)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


include $(LOCAL_PATH)/Camera.mk
include $(LOCAL_PATH)/Camera_Utest.mk
endif

ifneq ($(shell ls -d vendor/sprd/proprietories-source 2>/dev/null),)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)

include $(LOCAL_PATH)/Android_vsp_enc.mk
include $(LOCAL_PATH)/Android_vsp_mpeg4dec.mk
include $(LOCAL_PATH)/Android_vsp_h264dec.mk

endif

endif

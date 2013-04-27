LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


LOCAL_MODULE := utest_vsp_mpeg4dec
LOCAL_MODULE_TAGS := debug
LOCAL_CFLAGS := -fno-strict-aliasing -D_VSP_LINUX_ -D_VSP_ -D_MP4CODEC_DATA_PARTITION_ -DCHIP_ENDIAN_LITTLE
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm



LOCAL_SRC_FILES := utest_vsp_mpeg4dec.cpp util.c



LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../../../vendor/sprd/proprietories-source/opencore/codecs_v2/video/m4v_h263_sprd/$(strip $(TARGET_BOARD_PLATFORM))/dec/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../../../vendor/sprd/proprietories-source/opencore/codecs_v2/video/vsp/$(strip $(TARGET_BOARD_PLATFORM))/inc

LOCAL_SHARED_LIBRARIES := libutils libbinder
LOCAL_STATIC_LIBRARIES := libsprdmp4decoder

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)


include $(BUILD_EXECUTABLE)


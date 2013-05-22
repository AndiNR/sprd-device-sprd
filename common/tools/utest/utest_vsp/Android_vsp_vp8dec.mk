LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


LOCAL_MODULE := utest_vsp_vp8dec
LOCAL_MODULE_TAGS := debug
LOCAL_CFLAGS := -fno-strict-aliasing -D_VSP_LINUX_ -D_VSP_ -DCHIP_ENDIAN_LITTLE -DITRANS_ASSEMBLY -DOR_CACHE_OFF
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm



LOCAL_SRC_FILES := utest_vsp_vp8dec.cpp util.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../../../vendor/sprd/proprietories-source/opencore/codecs_v2/video/vsp/$(strip $(TARGET_BOARD_PLATFORM))/inc

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../../../vendor/sprd/proprietories-source/opencore/codecs_v2/video/vp8_sprd/$(strip $(TARGET_BOARD_PLATFORM))/dec/include

							
LOCAL_SHARED_LIBRARIES := libutils libbinder
LOCAL_STATIC_LIBRARIES := libsprdvp8decoder

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)


include $(BUILD_EXECUTABLE)


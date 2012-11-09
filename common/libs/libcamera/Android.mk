LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8810)

# When zero we link against libqcamera; when 1, we dlopen libqcamera.
DLOPEN_LIBQCAMERA:= 1

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
        $(LOCAL_PATH)/sc8810 \
	$(LOCAL_PATH)/vsp/sc8810/inc	\
	$(LOCAL_PATH)/vsp/sc8810/src \
	$(LOCAL_PATH)/jpeg_fw_8810/inc \
	$(LOCAL_PATH)/jpeg_fw_8810/src \
	external/skia/include/images \
	external/skia/include/core\
        external/jhead \
        external/sqlite/dist \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/source/include/video \
	$(TOP)/device/sprd/common/libs/gralloc \
	$(TOP)/device/sprd/common/libs/mali/src/ump/include

LOCAL_SRC_FILES:= \
	sc8810/SprdOEMCamera.cpp \
        sc8810/SprdCameraHardwareInterface.cpp \
	vsp/sc8810/src/vsp_drv_sc8810.c \
	jpeg_fw_8810/src/jpegcodec_bufmgr.c \
	jpeg_fw_8810/src/jpegcodec_global.c \
	jpeg_fw_8810/src/jpegcodec_table.c \
	jpeg_fw_8810/src/jpegenc_bitstream.c \
	jpeg_fw_8810/src/jpegenc_frame.c \
	jpeg_fw_8810/src/jpegenc_header.c \
	jpeg_fw_8810/src/jpegenc_init.c \
	jpeg_fw_8810/src/jpegenc_interface.c \
	jpeg_fw_8810/src/jpegenc_malloc.c \
	jpeg_fw_8810/src/jpegenc_api.c \
        jpeg_fw_8810/src/jpegdec_bitstream.c \
	jpeg_fw_8810/src/jpegdec_frame.c \
	jpeg_fw_8810/src/jpegdec_init.c \
	jpeg_fw_8810/src/jpegdec_interface.c \
	jpeg_fw_8810/src/jpegdec_malloc.c \
	jpeg_fw_8810/src/jpegdec_dequant.c	\
	jpeg_fw_8810/src/jpegdec_out.c \
	jpeg_fw_8810/src/jpegdec_parse.c \
	jpeg_fw_8810/src/jpegdec_pvld.c \
	jpeg_fw_8810/src/jpegdec_vld.c \
	jpeg_fw_8810/src/jpegdec_api.c \
	jpeg_fw_8810/src/exif_writer.c \
	jpeg_fw_8810/src/jpeg_stream.c \
	ispvideo/isp_video.c


endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8825_bak)
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/vsp/tiger/inc	\
	$(LOCAL_PATH)/vsp/tiger/src \
	$(LOCAL_PATH)/jpeg_fw_tiger/inc \
	$(LOCAL_PATH)/jpeg_fw_tiger/src \
	$(LOCAL_PATH)/tiger/inc \
	$(LOCAL_PATH)/tiger/isp/inc \
	external/skia/include/images \
	external/skia/include/core\
        external/jhead \
        external/sqlite/dist \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(TOP)/device/sprd/common/libs/gralloc \
	$(TOP)/device/sprd/common/libs/mali/src/ump/include

LOCAL_SRC_FILES:= \
	tiger/src/SprdOEMCamera.c \
        tiger/src/SprdCameraHardwareInterface.cpp \
	tiger/src/cmr_oem.c \
	tiger/src/cmr_set.c \
	tiger/src/cmr_mem.c \
	tiger/src/cmr_msg.c \
	tiger/src/cmr_cvt.c \
	tiger/src/cmr_v4l2.c \
	tiger/src/jpeg_codec.c \
	tiger/src/dc_cfg.c \
	tiger/src/dc_product_cfg.c \
	tiger/src/sensor_cfg.c \
	tiger/src/sensor_drv_u.c \
	sensor/sensor_ov5640_raw.c  \
	sensor/sensor_ov5640.c  \
	sensor/sensor_ov2640.c  \
	sensor/sensor_ov2655.c  \
	sensor/sensor_gc0309.c  \
	sensor/sensor_ov7675.c  \
	vsp/tiger/src/vsp_drv_tiger.c \
	jpeg_fw_tiger/src/jpegcodec_bufmgr.c \
	jpeg_fw_tiger/src/jpegcodec_global.c \
	jpeg_fw_tiger/src/jpegcodec_table.c \
	jpeg_fw_tiger/src/jpegenc_bitstream.c \
	jpeg_fw_tiger/src/jpegenc_frame.c \
	jpeg_fw_tiger/src/jpegenc_header.c \
	jpeg_fw_tiger/src/jpegenc_init.c \
	jpeg_fw_tiger/src/jpegenc_interface.c \
	jpeg_fw_tiger/src/jpegenc_malloc.c \
	jpeg_fw_tiger/src/jpegenc_api.c \
        jpeg_fw_tiger/src/jpegdec_bitstream.c \
	jpeg_fw_tiger/src/jpegdec_frame.c \
	jpeg_fw_tiger/src/jpegdec_init.c \
	jpeg_fw_tiger/src/jpegdec_interface.c \
	jpeg_fw_tiger/src/jpegdec_malloc.c \
	jpeg_fw_tiger/src/jpegdec_dequant.c	\
	jpeg_fw_tiger/src/jpegdec_out.c \
	jpeg_fw_tiger/src/jpegdec_parse.c \
	jpeg_fw_tiger/src/jpegdec_pvld.c \
	jpeg_fw_tiger/src/jpegdec_vld.c \
	jpeg_fw_tiger/src/jpegdec_api.c  \
    	jpeg_fw_tiger/src/exif_writer.c  \
    	jpeg_fw_tiger/src/jpeg_stream.c
endif


LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_CFLAGS := -fno-strict-aliasing -D_VSP_ -DJPEG_ENC -D_VSP_LINUX_ -DCHIP_ENDIAN_LITTLE -DCONFIG_CAMERA_2M

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8810)
LOCAL_CFLAGS += -DCONFIG_CAMERA_5M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),5M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_5M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),3M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_3M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),2M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_2M
endif

ifeq ($(strip $(TARGET_BOARD_NO_FRONT_SENSOR)),true)
LOCAL_CFLAGS += -DCONFIG_DCAM_SENSOR_NO_FRONT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_Z788)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_788
endif

LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_ROTATION)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ROTATION
endif
        
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libexif libutils libbinder libcamera_client libskia libcutils libsqlite libhardware

include $(BUILD_SHARED_LIBRARY)


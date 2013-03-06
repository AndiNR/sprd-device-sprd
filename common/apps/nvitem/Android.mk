ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES    +=  $(LOCAL_PATH) \
			$(LOCAL_PATH)/$(BOARD_PRODUCT_NAME)/ \
			$(LOCAL_PATH)/common \
			device/sprd/common/apps/engineeringmodel/engcs
LOCAL_SRC_FILES:= \
	nvitem_nandless.c \
	package.c \
	log.c \
	nandless.c \
	save.c
 
LOCAL_SHARED_LIBRARIES := \
    libhardware_legacy \
    libc \
    libutils \
    libengclient

#ifeq ($(strip $(TARGET_USERIMAGES_USE_EXT4)),true)
#LOCAL_CFLAGS := -DCONFIG_EMMC
#endif

LOCAL_MODULE := nvitemd
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
endif

LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

LOCAL_CFLAGS += -DPLATFORM_ANDROID  -DBRCM_BT_USE_BTL_IF -DBT_USE_BTL_IF

# Relative path from current dir to vendor brcm
BRCM_BT_SRC_ROOT_PATH := /../../bt

# Relative path from <mydroid> to brcm base
BRCM_BT_INC_ROOT_PATH := $(LOCAL_PATH)/../../bt

#Add all JNI files in the below path
LOCAL_SRC_FILES := \
    $(BRCM_BT_SRC_ROOT_PATH)/adaptation/btl-if/client/btl_ifc_wrapper.c \
    $(BRCM_BT_SRC_ROOT_PATH)/adaptation/btl-if/client/btl_ifc.c\
    FmReceiverLoader.cpp \
    com_broadcom_bt_service_fm_FmReceiverService.cpp
 

LOCAL_C_INCLUDES :=  \
			$(LOCAL_PATH) \
                  	$(JNI_H_INCLUDE) \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/btl-if/client \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/btl-if/include \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/dtun/include \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/include \
			frameworks/base/include \
			system/core/include
			
LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE    := libfmservice

LOCAL_PROGUARD_ENABLED:=disabled

LOCAL_PRELINK_MODULE := false


include $(BUILD_SHARED_LIBRARY)

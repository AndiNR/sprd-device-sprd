LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

LOCAL_CFLAGS += -DPLATFORM_ANDROID  -DBRCM_BT_USE_BTL_IF -DBT_USE_BTL_IF

#Add all JNI files in the below path
LOCAL_SRC_FILES := \
    BtFmServiceLoader.cpp \
    com_broadcom_bt_service_PowerManagementService.cpp

LOCAL_C_INCLUDES :=  \
            $(LOCAL_PATH) \
            $(JNI_H_INCLUDE) \
            frameworks/base/include \
            system/core/include \
            frameworks/base/core/jni \
            system/bluetooth/bluedroid/include \
            system/bluetooth/bluez-clean-headers

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbluedroid

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE    := libfmpmservice

LOCAL_PROGUARD_ENABLED:=disabled

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

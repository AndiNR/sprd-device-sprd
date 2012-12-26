
MALI := libUMP libEGL_mali.so libGLESv1_CM_mali.so libGLESv2_mali.so libMali.so ump.ko mali.ko

MXD_CMMB_PLAYER := mxdcmmbplayer.apk  \
		   libiwcmmbdev.so libiwcmmb_jni.so  \
		   libiwmbbms_jni.so libiwmbbms.so libiwmbbms_tel.so \
		   libiwuam.so  libmxdcmmb.so libplayEngine.so mxdcmmbserver mxdid.id libiwcmmb_jni_demon.so libiwcmmbdev_demon.so mxdcmmbtest.apk
PRODUCT_PROPERTY_OVERRIDES :=

# original apps copied from generic_no_telephony.mk
PRODUCT_PACKAGES := \
	DeskClock \
	Bluetooth \
	Calculator \
	Calendar \
	CertInstaller \
	DrmProvider \
	Email \
	Exchange2 \
	Gallery2 \
	InputDevices \
	LatinIME \
	Launcher2 \
	Music \
	MusicFX \
	Provision \
	QuickSearchBox \
	SystemUI \
	CalendarProvider \
	bluetooth-health \
	hciconfig \
	hcitool \
	hcidump \
	FMPlayer \
	bttest\
	hostapd \
	wpa_supplicant.conf \
    SoundRecorder

# own copyright packages files
PRODUCT_PACKAGES += \
    AppBackup \
    AudioProfile \
    SprdNote \
    CallFireWall \
    ValidationTools

# prebuild files
PRODUCT_PACKAGES += \
    ES_File_Explorer.apk

PRODUCT_PACKAGES += \
	rawdatad \
	nvitemd \
	charge \
	vcharged \
	poweroff_alarm \
	mplayer \
	modemd \
	calibration_init \
	gralloc.$(TARGET_PLATFORM) \
	hwcomposer.$(TARGET_PLATFORM) \
	camera.$(TARGET_PLATFORM) \
	lights.$(TARGET_PLATFORM) \
	audio.primary.$(TARGET_PLATFORM) \
	tinymix \
	sensors.$(TARGET_BOARD)  \
	fm.$(TARGET_PLATFORM)  \
	libmbbms_tel_jni.so\
	$(MALI)

BRCMFM := \
	FmDaemon \
	FmTest

PRODUCT_PACKAGES += $(BRCMFM)

ifeq ($(BOARD_CMMB_HW), mxd)
PRODUCT_PACKAGES += $(MXD_CMMB_PLAYER)
else
PRODUCT_PACKAGES += $(SIANOMTV)
endif

PRODUCT_COPY_FILES := \
	$(BOARDDIR)/init.rc:root/init.rc \
	$(BOARDDIR)/init.sp8810.rc:root/init.sp8810.rc \
	$(BOARDDIR)/init.sp8810.usb.rc:root/init.sp8810.usb.rc \
	$(BOARDDIR)/ueventd.sp8810.rc:root/ueventd.sp8810.rc \
	$(BOARDDIR)/fstab.sp8810:root/fstab.sp8810 \
	$(BOARDDIR)/vold.fstab:system/etc/vold.fstab \
	device/sprd/common/libs/audio/apm/devicevolume.xml:system/etc/devicevolume.xml \
	device/sprd/common/libs/audio/apm/formatvolume.xml:system/etc/formatvolume.xml \
        $(BOARDDIR)/hw_params/tiny_hw.xml:system/etc/tiny_hw.xml \
        $(BOARDDIR)/hw_params/codec_pga.xml:system/etc/codec_pga.xml \
        $(BOARDDIR)/hw_params/audio_para:system/etc/audio_para \
	device/sprd/common/libs/mali/egl.cfg:system/lib/egl/egl.cfg \
	device/sprd/common/libs/audio/audio_policy.conf:system/etc/audio_policy.conf \
	device/sprd/common/res/media/media_codecs.xml:system/etc/media_codecs.xml \
	device/sprd/common/res/media/media_profiles.xml:system/etc/media_profiles.xml \
	frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
	frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	frameworks/native/data/etc/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml \
	frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
	frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
	frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
	frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
	frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
	device/sprd/common/res/apn/apns-conf.xml:system/etc/apns-conf.xml

BOARD_WLAN_DEVICE_REV       := bcm4330_b2
$(call inherit-product, $(BOARDDIR)/../common/apps/engineeringmodel/module.mk)
$(call inherit-product, hardware/broadcom/wlan/bcmdhd/firmware/bcm4330/device-bcm.mk)

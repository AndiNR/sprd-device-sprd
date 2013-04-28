MALI400 := gralloc.mali libUMP libEGL_mali.so libGLESv1_CM_mali.so libGLESv2_mali.so libMali.so hwcomposer.default ump.ko mali.ko



SPRD_FM_APP := FMPlayer

RTK_TOOL:= RtkService.apk

PRODUCT_PROPERTY_OVERRIDES :=

# original apps copied from generic_no_telephony.mk
PRODUCT_PACKAGES := \
	libjni_pinyinime \
	Camera \
	DeskClock \
	AlarmProvider \
	Bluetooth \
	Calculator \
	Calendar \
	CertInstaller \
	DrmProvider \
	Email \
	Exchange \
	Gallery2 \
	InputDevices \
	LatinIME \
	Launcher2 \
	Music \
	MusicFX \
	Provision \
	QuickSearchBox \
	Sync \
	SystemUI \
	Updater \
	CalendarProvider \
	SyncProvider \
	bluetooth-health \
	hciconfig \
	hcitool \
	hcidump \
	bttest\
	hostapd \
	wpa_supplicant.conf \
	audio.a2dp.default \
        SoundRecorder \
        libmorpho_facesolid.so

# own copyright packages files
PRODUCT_PACKAGES += \
    $(MALI400) \
    AudioProfile \
    ValidationTools \
    libvalidationtoolsjni \
    vtserver

# prebuild files
PRODUCT_PACKAGES += \
    ES_File_Explorer.apk

PRODUCT_PACKAGES += \
	libI420colorconvert \
	libstagefrighthw \
	libstagefright_soft_mpeg4dec_sprd \
	libstagefright_soft_h264dec_sprd \
	rawdatad \
	modem_control\
	nvitemd \
	charge \
	vcharged \
	poweroff_alarm \
	preloadapp.sh \
	mplayer \
	sqlite3 \
	modemd \
	calibration_init \
	gralloc.$(TARGET_PLATFORM) \
	hwcomposer.$(TARGET_PLATFORM) \
	camera.$(TARGET_PLATFORM) \
	lights.$(TARGET_PLATFORM) \
	audio.primary.$(TARGET_PLATFORM) \
	audio_policy.$(TARGET_PLATFORM) \
	tinymix \
	sensors.$(TARGET_PLATFORM) \
	libmbbms_tel_jni.so \
	$(MALI) \
	zram.sh \
	libsprdstreamrecoder \
	libvtmanager\
   fm.$(TARGET_PLATFORM)

PRODUCT_PACKAGES += \
        $(SPRD_FM_APP)

PRODUCT_PACKAGES += \
	$(RTK_TOOL)

PRODUCT_COPY_FILES := \
	$(BOARDDIR)/init.rc:root/init.rc \
	$(BOARDDIR)/init.sc8825.rc:root/init.sc8825.rc \
	$(BOARDDIR)/init.sc8825.usb.rc:root/init.sc8825.usb.rc \
	$(BOARDDIR)/ueventd.sc8825.rc:root/ueventd.sc8825.rc \
	$(BOARDDIR)/vold.fstab:system/etc/vold.fstab \
	$(BOARDDIR)/modem_images.info:root/modem_images.info \
	$(BOARDDIR)/nvitem.cfg:root/nvitem.cfg \
	device/sprd/common/res/CDROM/adb.iso:system/etc/adb.iso \
	device/sprd/common/libs/audio/apm/devicevolume.xml:system/etc/devicevolume.xml \
	device/sprd/common/libs/audio/apm/formatvolume.xml:system/etc/formatvolume.xml \
        $(BOARDDIR)/hw_params/tiny_hw.xml:system/etc/tiny_hw.xml \
        $(BOARDDIR)/hw_params/codec_pga.xml:system/etc/codec_pga.xml \
        $(BOARDDIR)/hw_params/audio_para:system/etc/audio_para \
	$(BOARDDIR)/scripts/ext_symlink.sh:system/bin/ext_symlink.sh \
	$(BOARDDIR)/scripts/ext_data.sh:system/bin/ext_data.sh \
	$(BOARDDIR)/scripts/ext_kill.sh:system/bin/ext_kill.sh \
	device/sprd/common/libs/mali/egl.cfg:system/lib/egl/egl.cfg \
	device/sprd/common/libs/audio/audio_policy.conf:system/etc/audio_policy.conf \
	device/sprd/common/res/media/media_codecs.xml:system/etc/media_codecs.xml \
	device/sprd/common/res/media/media_profiles.xml:system/etc/media_profiles.xml \
	frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
	frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
	frameworks/native/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
	frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
	frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
        frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
        frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
        frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
        frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
        frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.xml:system/etc/permissions/android.hardware.touchscreen.xml \
	frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
	frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
	frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	frameworks/native/data/etc/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml \
	device/sprd/common/res/apn/apns-conf.xml:system/etc/apns-conf.xml \
	frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml \
	frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.xml

#$(call inherit-product, device/sprd/partner/beken/libfm/device-beken-fm.mk)

#$(call inherit-product, device/sprd/partner/realtek/bt/firmware/device-rtl.mk)
#$(call inherit-product, device/sprd/partner/realtek/wlan/efuse/device-rtl.mk)
$(call inherit-product, device/sprd/partner/ublox/device-ublox-gps.mk)

$(call inherit-product, $(BOARDDIR)/../common/apps/engineeringmodel/module.mk)
$(call inherit-product, $(BOARDDIR)/../common/apps/modemassert/module.mk)

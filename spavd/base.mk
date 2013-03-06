
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
	Music \
	MusicFX \
	Provision \
	QuickSearchBox \
	SystemUI \
	CalendarProvider \
#	bluetooth-health \
	hciconfig \
	hcitool \
	hcidump \
	bttest\
	hostapd \
	wpa_supplicant.conf \
	calibration_init \
	rawdatad \
	nvm_daemon \
	modemd\
	audio.a2dp.default

# own copyright packages files
PRODUCT_PACKAGES += \
    AppBackup \
    AudioProfile \
    SprdNote \
    CallFireWall \
#    ValidationTools \
#    libvalidationtoolsjni \
#    vtserver

# prebuild files
PRODUCT_PACKAGES += \
    ES_File_Explorer.apk

#PRODUCT_PACKAGES += \
#	nvitemd \
#	charge \
#	vcharged \
#	poweroff_alarm \
#	mplayer \
#	gralloc.$(TARGET_PLATFORM) \
#	hwcomposer.$(TARGET_PLATFORM) \
#	camera.$(TARGET_PLATFORM) \
#	libisp.so \
#	lights.$(TARGET_PLATFORM) \
#	audio.primary.$(TARGET_PLATFORM) \
#	audio_policy.$(TARGET_PLATFORM) \
#	tinymix \
#	libvbeffect \
#      sensors.$(TARGET_PLATFORM) \
#      $(INVENSENSE) \
#	$(MALI) \
#	libsprdstreamrecoder \
#	libvtmanager

PRODUCT_COPY_FILES := \
	$(BOARDDIR)/vold.fstab:system/etc/vold.fstab 

# This is a build configuration for the product aspects that
# are specific to the emulator.
#PRODUCT_PROPERTY_OVERRIDES := \
#    ro.ril.hsxpa=1 \
#    ro.ril.gprsclass=10 \
#    ro.adb.qemud=1

#PRODUCT_COPY_FILES := \
#    device/generic/goldfish/data/etc/apns-conf.xml:system/etc/apns-conf.xml \
#    device/generic/goldfish/data/etc/vold.conf:system/etc/vold.conf \
#    $(call add-to-product-copy-files-if-exists,development/tools/emulator/system/camera/media_profiles.xml:system/etc/media_profiles.xml) \
#    $(call add-to-product-copy-files-if-exists,development/tools/emulator/system/camera/media_codecs.xml:system/etc/media_codecs.xml) \
#    hardware/libhardware_legacy/audio/audio_policy.conf:system/etc/audio_policy.conf

#PRODUCT_PACKAGES := \
#    audio.primary.goldfish \
#    power.goldfish


MALI := libUMP libEGL_mali.so libGLESv1_CM_mali.so libGLESv2_mali.so libMali.so ump.ko mali.ko

PRODUCT_PROPERTY_OVERRIDES :=

PRODUCT_PACKAGES := \
	gralloc.$(TARGET_PLATFORM) \
	hwcomposer.$(TARGET_PLATFORM) \
	lights.$(TARGET_PLATFORM) \
	audio.primary.$(TARGET_PLATFORM) \
	tinymix \
	sensors.$(TARGET_BOARD)  \
	$(MALI)

PRODUCT_COPY_FILES := \
	$(BOARDDIR)/init.sp8825.rc:root/init.sp8825.rc \
	$(BOARDDIR)/init.sp8825.usb.rc:root/init.sp8825.usb.rc \
	$(BOARDDIR)/ueventd.sp8825.rc:root/ueventd.sp8825.rc \
	$(BOARDDIR)/fstab.sp8825:root/fstab.sp8825 \
	$(BOARDDIR)/vold.fstab:system/etc/vold.fstab \
	device/sprd/common/libs/mali/egl.cfg:system/lib/egl/egl.cfg \
	device/sprd/common/libs/audio/audio_policy.conf:system/etc/audio_policy.conf \
	device/sprd/common/res/media/media_codecs.xml:system/etc/media_codecs.xml \
	device/sprd/common/res/media/media_profiles.xml:system/etc/media_profiles.xml \
	device/sprd/common/res/apn/apns-conf.xml:system/etc/apns-conf.xml


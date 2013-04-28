#
# Copyright (C) 2007 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

TARGET_PLATFORM := sc8825
TARGET_BOARD := sp6825ga
BOARDDIR := device/sprd/$(TARGET_BOARD)

DEVICE_PACKAGE_OVERLAYS := $(BOARDDIR)/overlay

PRODUCT_PROPERTY_OVERRIDES := \
	keyguard.no_require_sim=true \
	ro.com.android.dataroaming=false \
	ro.device.support.abroad.apn=1\
	persist.msms.phone_count=2 \
	persist.msms.phone_default=0 \
        lmk.autocalc=false \
        zram.support=true \
	persist.blcr.enable=0 \
        persist.sys.service.delay=false \
        persist.sys.lowmem=16
ifeq ($(TARGET_BUILD_VARIANT),user)
  PRODUCT_PROPERTY_OVERRIDES += persist.sys.sprd.modemreset=1
else
  PRODUCT_PROPERTY_OVERRIDES += persist.sys.sprd.modemreset=0
endif


# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mtp

PRODUCT_PACKAGES := \
	Settings \
	MsmsPhone \
	MsmsSettings \
	MsmsStk \
	Stk1 \
        ES_File_Explorer.apk \
        SecondClock.apk \
        CallFireWall   \
        AudioProfile \
        LiveWallpapers  \
        LiveWallpapersPicker \
        Galaxy4 \
        HoloSpiralWallpaper \
        MagicSmokeWallpapers \
        VisualizationWallpapers \
        ValidationTools \
        ValidationtoolsFm \
        vtserver \
        libvalidationtoolsjni \
        NoiseField \
        PhaseBeam \
        CellBroadcastReceiver \
        Monternet \
	AppBackup   \
	SearchCallLog



PRODUCT_COPY_FILES := \
	$(BOARDDIR)/sprd-keypad.kl:system/usr/keylayout/sprd-keypad.kl \
	$(BOARDDIR)/ft5x0x_ts.idc:system/usr/idc/ft5x0x_ts.idc \
	$(BOARDDIR)/headset-keyboard.kl:system/usr/keylayout/headset-keyboard.kl

$(call inherit-product, frameworks/native/build/phone-hdpi-dalvik-heap.mk)
# include classified configs
$(call inherit-product, $(BOARDDIR)/base.mk)
$(call inherit-product, $(BOARDDIR)/proprietories.mk)
$(call inherit-product, device/sprd/common/res/boot/boot_res.mk)

# include standard configs
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/telephony.mk)

# Overrides
PRODUCT_NAME := sp6825gaplus
PRODUCT_DEVICE := $(TARGET_BOARD)
PRODUCT_MODEL := sp6825ga
PRODUCT_BRAND := Spreadtrum
PRODUCT_MANUFACTURER := Spreadtrum

PRODUCT_LOCALES := zh_CN zh_TW en_US
ifeq ($(MULTILANGUAGE_SUPPORT),true)
  PRODUCT_PACKAGES += $(MULTILANGUAGE_PRODUCT_PACKAGES)
  PRODUCT_LOCALES :=
endif

PRODUCT_PROPERTY_OVERRIDES += ro.product.locale.language=zh
PRODUCT_PROPERTY_OVERRIDES += ro.product.locale.region=CN

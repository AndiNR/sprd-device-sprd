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

TARGET_PLATFORM := sc8810
TARGET_BOARD := mint
BOARDDIR := device/sprd/$(TARGET_BOARD)

DEVICE_PACKAGE_OVERLAYS := $(BOARDDIR)/overlay

PRODUCT_AAPT_CONFIG := hdpi

PRODUCT_PROPERTY_OVERRIDES := \
	keyguard.no_require_sim=true \
	ro.com.android.dataroaming=false \
	persist.msms.phone_count=2 \
	persist.msms.phone_default=0 \
	ro.modem.vlx.enable=1 \
	ro.modem.vlx.tty=/dev/ts0710mux \
	ro.modem.vlx.eth=veth \
	ro.modem.vlx.snd=1 \
	ro.modem.vlx.diag=/dev/vbpipe0 \
	ro.modem.vlx.nv=/dev/vbpipe1 \
	ro.modem.vlx.assert=/dev/vbpipe2 \
	ro.modem.vlx.vbc=/dev/vbpipe6 \
	ro.modem.vlx.msms.count=2 \
	persist.sys.sprd.modemreset=0

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mtp

PRODUCT_PACKAGES := \
	VoiceDialer \
	MsmsPhone \
    	framework2 \
	Settings \
        MsmsStk \
        Stk1

PRODUCT_COPY_FILES := \
	$(BOARDDIR)/sprd-keypad.kl:system/usr/keylayout/sprd-keypad.kl \
	$(BOARDDIR)/sec_touchscreen.idc:system/usr/idc/sec_touchscreen.idc

$(call inherit-product, frameworks/native/build/phone-hdpi-dalvik-heap.mk)

# include classified configs
$(call inherit-product, $(BOARDDIR)/base.mk)
$(call inherit-product, $(BOARDDIR)/proprietories.mk)
$(call inherit-product, device/sprd/common/res/boot/boot_res.mk)

# include standard configs
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/telephony.mk)

# Overrides
PRODUCT_NAME := mint
PRODUCT_DEVICE := $(TARGET_BOARD)
PRODUCT_MODEL := mint 
PRODUCT_BRAND := Spreadtrum
PRODUCT_MANUFACTURER := Spreadtrum

PRODUCT_LOCALES := zh_CN zh_TW en_US

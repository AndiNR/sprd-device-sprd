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
TARGET_BOARD := sp8810ea
BOARDDIR := device/sprd/$(TARGET_BOARD)

DEVICE_PACKAGE_OVERLAYS := $(BOARDDIR)/overlay
PRODUCT_PACKAGE_OVERLAYS := vendor/sprd/operator/cmcc/specA/overlay

PRODUCT_AAPT_CONFIG := hdpi

PRODUCT_PROPERTY_OVERRIDES := \
	keyguard.no_require_sim=true \
	ro.com.android.dataroaming=false \
	persist.msms.phone_count=2 \
	persist.msms.phone_default=0 \
	persist.sys.sprd.modemreset=1 \
	ro.modem.vlx.enable=1 \
	ro.modem.vlx.tty=/dev/ts0710mux \
	ro.modem.vlx.eth=veth \
	ro.modem.vlx.snd=1 \
	ro.modem.vlx.diag=/dev/vbpipe0 \
	ro.modem.vlx.nv=/dev/vbpipe1 \
	ro.modem.vlx.assert=/dev/vbpipe2 \
	ro.modem.vlx.vbc=/dev/vbpipe6 \
	ro.modem.vlx.msms.count=2

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mass_storage

PRODUCT_PACKAGES := \
    MsmsPhone \
    Settings \
    SprdDM \
    Launcher2 \
    MsmsStk \
    Stk1 \
    framework2

# prebuild files
PRODUCT_PACKAGES += \
	
# add  system properties
PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator=cmcc \
	ro.operator.version=specA
	
PRODUCT_COPY_FILES := \
	$(BOARDDIR)/sprd-keypad.kl:system/usr/keylayout/sprd-keypad.kl \
	$(BOARDDIR)/pixcir_ts.kl:system/usr/keylayout/pixcir_ts.kl \
	$(BOARDDIR)/pixcir_ts.idc:system/usr/idc/pixcir_ts.idc

$(call inherit-product, frameworks/native/build/phone-hdpi-dalvik-heap.mk)

# include classified configs
$(call inherit-product, $(BOARDDIR)/base.mk)
$(call inherit-product, $(BOARDDIR)/proprietories.mk)
$(call inherit-product, device/sprd/common/res/boot/boot_res.mk)

# include standard configs
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/telephony.mk)

# Overrides
PRODUCT_NAME := sp8810eaplus
PRODUCT_DEVICE := $(TARGET_BOARD)
PRODUCT_MODEL := ZTE U970
PRODUCT_BRAND := Spreadtrum
PRODUCT_MANUFACTURER := Spreadtrum

PRODUCT_LOCALES := zh_CN zh_TW en_US

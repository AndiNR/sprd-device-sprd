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

TARGET_PLATFORM := sc8830
TARGET_BOARD := sp8830eb
BOARDDIR := device/sprd/$(TARGET_BOARD)

DEVICE_PACKAGE_OVERLAYS := $(BOARDDIR)/overlay

PRODUCT_AAPT_CONFIG := hdpi

PRODUCT_PROPERTY_OVERRIDES := \
	keyguard.no_require_sim=true \
	universe_ui_support=true\
	ro.com.android.dataroaming=false \
	persist.msms.phone_count=2 \
	ro.msms.phone_count=2 \
 	persist.msms.phone_default=0 \
	ro.modem.count=2 \
	ro.digital.fm.support=1 \
	ro.modem.t.enable=1 \
	ro.modem.t.dev=/dev/cpt \
	ro.modem.t.tty=/dev/stty_td \
	ro.modem.t.eth=seth_td \
	ro.modem.t.snd=1 \
	ro.modem.t.diag=/dev/slog_td \
	ro.modem.t.loop=/dev/spipe_td0 \
	ro.modem.t.nv=/dev/spipe_td1 \
	ro.modem.t.assert=/dev/spipe_td2 \
	ro.modem.t.vbc=/dev/spipe_td6 \
	ro.modem.t.id=0 \
	ro.modem.t.count=1 \
	ro.modem.w.enable=1 \
	ro.modem.w.dev=/dev/cpw \
	ro.modem.w.tty=/dev/stty_w \
	ro.modem.w.eth=seth_w \
	ro.modem.w.snd=2 \
	ro.modem.w.diag=/dev/slog_w \
	ro.modem.w.loop=/dev/spipe_w0 \
	ro.modem.w.nv=/dev/spipe_w1 \
	ro.modem.w.assert=/dev/spipe_w2 \
	ro.modem.w.vbc=/dev/spipe_w6 \
	ro.modem.w.id=1 \
	ro.modem.w.count=1

ifeq ($(TARGET_BUILD_VARIANT),user)
  PRODUCT_PROPERTY_OVERRIDES += persist.sys.sprd.modemreset=1
else
  PRODUCT_PROPERTY_OVERRIDES += persist.sys.sprd.modemreset=0
endif

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
	persist.sys.usb.config=mass_storage

PRODUCT_PACKAGES := \
    MsmsPhone \
    Settings \
    MsmsStk \
    Stk1 \
    framework2
        


PRODUCT_COPY_FILES := \
	$(BOARDDIR)/sprd-keypad.kl:system/usr/keylayout/sprd-keypad.kl \
	$(BOARDDIR)/sci-keypad.kl:system/usr/keylayout/sci-keypad.kl \
	$(BOARDDIR)/pixcir_ts.kl:system/usr/keylayout/pixcir_ts.kl \
	$(BOARDDIR)/pixcir_ts.idc:system/usr/idc/pixcir_ts.idc \
	$(BOARDDIR)/ft5x0x_ts.idc:system/usr/idc/ft5x0x_ts.idc

$(call inherit-product, frameworks/native/build/phone-HD720-dalvik-heap.mk)

# include classified configs
$(call inherit-product, $(BOARDDIR)/base.mk)
$(call inherit-product, $(BOARDDIR)/proprietories.mk)
$(call inherit-product, device/sprd/common/res/boot/boot_res.mk)
$(call inherit-product, vendor/sprd/UniverseUI/universeui.mk)

# include standard configs
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/telephony.mk)

# Overrides
PRODUCT_NAME := sp8735ebplus_UUI
PRODUCT_DEVICE := $(TARGET_BOARD)
PRODUCT_MODEL := sp8830eb
PRODUCT_BRAND := Spreadtrum
PRODUCT_MANUFACTURER := Spreadtrum

PRODUCT_LOCALES := zh_CN zh_TW en_US


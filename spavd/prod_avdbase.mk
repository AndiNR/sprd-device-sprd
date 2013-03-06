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

TARGET_PLATFORM := spavd
TARGET_BOARD := spavd
BOARDDIR := device/sprd/$(TARGET_BOARD)


PRODUCT_PACKAGES := \
	VoiceDialer \
	Phone \
	framework2 \
	Stk \
	Settings \
	Launcher2

#PRODUCT_CHARACTERISTICS := tablet
#include frameworks/native/build/tablet-dalvik-heap.mk
#PRODUCT_COPY_FILES += \
#    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml

#$(call inherit-product, frameworks/base/build/phone-hdpi-dalvik-heap.mk)
$(call inherit-product, frameworks/native/build/tablet-dalvik-heap.mk)

# include classified configs
$(call inherit-product, $(BOARDDIR)/base.mk)
$(call inherit-product, $(BOARDDIR)/proprietories.mk)
# avd start loading animation resource. default is "android"
#$(call inherit-product, device/sprd/common/res/boot/boot_res.mk)

# include standard configs
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/telephony.mk)

# Overrides
PRODUCT_NAME := spavd
PRODUCT_DEVICE := $(TARGET_BOARD)
PRODUCT_MODEL := sprd-avd
PRODUCT_BRAND := Spreadtrum
PRODUCT_MANUFACTURER := Spreadtrum

PRODUCT_LOCALES := zh_CN zh_TW en_US

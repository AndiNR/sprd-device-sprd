#
# Copyright (C) 2011 The Android Open-Source Project
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

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
ARCH_ARM_HAVE_TLS_REGISTER := true

TARGET_BOARD_PLATFORM := sc8825
TARGET_BOOTLOADER_BOARD_NAME := sp8825ea

TARGET_VLX_ENABLE ?= true

# config u-boot
TARGET_NO_BOOTLOADER := false
ifeq ($(TARGET_VLX_ENABLE), true)
UBOOT_DEFCONFIG := sp8825ea
else
UBOOT_DEFCONFIG := sp8825ea_native
endif

# config kernel
TARGET_NO_KERNEL := false
ifeq ($(TARGET_VLX_ENABLE), true)
KERNEL_DEFCONFIG := sp8825ea-vlx_defconfig
else
KERNEL_DEFCONFIG := sp8825ea-native_defconfig
endif
USES_UNCOMPRESSED_KERNEL := true
BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_CMDLINE := console=ttyS1,115200n8

# use default init.rc
TARGET_PROVIDES_INIT_RC := true

# board specific modules
BOARD_USES_TINYALSA_AUDIO := true
BOARD_USES_ALSA_AUDIO := false
BUILD_WITH_ALSA_UTILS := false
BOARD_USE_VETH := true

TARGET_RECOVERY_UI_LIB := librecovery_ui_sp8825
TARGET_RECOVERY_PIXEL_FORMAT := "RGBA_8888"
# ext4 partition layout
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 300000000
BOARD_USERDATAIMAGE_PARTITION_SIZE := 2000000000
BOARD_CACHEIMAGE_PARTITION_SIZE := 150000000
BOARD_FLASH_BLOCK_SIZE := 4096
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
USE_OPENGL_RENDERER := true

USE_CAMERA_STUB := true

BOARD_USES_GENERIC_AUDIO := false

BOARD_HAVE_BLUETOOTH := true

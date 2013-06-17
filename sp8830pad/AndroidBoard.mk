LOCAL_PATH := $(call my-dir)

$(call add-radio-file,modem_bins/modem.bin)
$(call add-radio-file,modem_bins/nvitem.bin)
$(call add-radio-file,modem_bins/dsp.bin)
ifeq ($(strip $(TARGET_VLX_ENABLE)),true)
$(call add-radio-file,modem_bins/vmjaluna.bin)
endif


# Compile U-Boot
ifneq ($(strip $(TARGET_NO_BOOTLOADER)),true)

INSTALLED_UBOOT_TARGET := $(PRODUCT_OUT)/u-boot.bin
-include u-boot/AndroidUBoot.mk

INSTALLED_RADIOIMAGE_TARGET += $(PRODUCT_OUT)/u-boot.bin
INSTALLED_RADIOIMAGE_TARGET += $(PRODUCT_OUT)/u-boot-spl-16k.bin

endif # End of U-Boot

# Compile Linux Kernel
ifneq ($(strip $(TARGET_NO_KERNEL)),true)

-include kernel/AndroidKernel.mk

file := $(INSTALLED_KERNEL_TARGET)
ALL_PREBUILT += $(file)
$(file) : $(TARGET_PREBUILT_KERNEL) | $(ACP)
	$(transform-prebuilt-to-target)

endif # End of Kernel

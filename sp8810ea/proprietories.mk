
ifneq ($(shell ls -d vendor/sprd/proprietories-source 2>/dev/null),)
# for spreadtrum internal proprietories modules: only support package module names


PRODUCT_PACKAGES := \
	akmd8975

else
# for spreadtrum customer proprietories modules: only support direct copy

PROPMODS := \
	system/bin/akmd8975

PRODUCT_COPY_FILES := $(foreach f,$(PROPMODS),vendor/sprd/proprietories/sp8810ea/$(f):$(f))

endif

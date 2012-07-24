
ifneq ($(shell ls -d vendor/sprd/proprietories-source 2>/dev/null),)
# for spreadtrum internal proprietories modules: only support package module names

PRODUCT_PACKAGES :=

else
# for spreadtrum customer proprietories modules: only support direct copy

PROPMODS :=

PRODUCT_COPY_FILES := $(foreach f,$(PROPMODS),vendor/sprd/proprietories/sp8810ea/$(f):$(f))

endif

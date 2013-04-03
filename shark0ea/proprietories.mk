
ifneq ($(shell ls -d vendor/sprd/proprietories-source 2>/dev/null),)
# for spreadtrum internal proprietories modules: only support package module names

OPENCORE :=  libopencore_common libomx_sharedlibrary libomx_m4vdec_sharedlibrary libomx_m4venc_sharedlibrary \
	libomx_avcdec_sharedlibrary pvplayer.cfg

PRODUCT_PACKAGES := \
	$(OPENCORE) \
	rild_sp \
	libril_sp \
	libreference-ril_sp \
	sprd_monitor \
	akmd8975

	# remove phone
	#phoneserver \
	#phoneserver_2sim

else
# for spreadtrum customer proprietories modules: only support direct copy

PROPMODS := \
	system/lib/libopencore_common.so \
	system/lib/libomx_sharedlibrary.so \
	system/lib/libomx_m4vdec_sharedlibrary.so \
	system/lib/libomx_m4venc_sharedlibrary.so \
	system/lib/libomx_avcdec_sharedlibrary.so \
	system/etc/pvplayer.cfg \
	system/bin/rild_sp \
	system/lib/libril_sp.so \
	system/lib/libreference-ril_sp.so \
	system/bin/sprd_monitor \
	system/bin/akmd8975

	# remove phone
	#system/bin/phoneserver \
	#system/bin/phoneserver_2sim \

PRODUCT_COPY_FILES := $(foreach f,$(PROPMODS),vendor/sprd/proprietories/sp8810ea/$(f):$(f))

endif

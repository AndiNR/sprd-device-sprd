/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _ISP_DRV_V0000_H_
#define _ISP_DRV_V0000_H_

#include <sys/types.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <fcntl.h>              /* low-level i/o */
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "isp_drv_kernel.h"
#include "cmr_common.h"

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif
/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/
#define ISP_EB 0x01
#define ISP_UEB 0x00

#define ISP_ZERO 0x00
#define ISP_ONE 0x01
#define ISP_TWO 0x02

#define ISP_MSG_WAIT_TIME 1000 /* wait for 1 ms */
#if 0
#define ISP_DEBUG_STR      "%s, %s, %d line,: "
#define ISP_DEBUG_ARGS    __FILE__,__FUNCTION__,__LINE__
#else
#define ISP_DEBUG_STR      "%s, %d line,: "
#define ISP_DEBUG_ARGS    __FUNCTION__,__LINE__
#endif

#define ISP_LOG(format,...) ALOGE(ISP_DEBUG_STR format, ISP_DEBUG_ARGS, ##__VA_ARGS__)

#define ISP_TRAC(_x_) ISP_LOG _x_
#define ISP_RETURN_IF_FAIL(exp,warning) do{if(exp) {ISP_TRAC(warning); return exp;}}while(0)
#define ISP_TRACE_IF_FAIL(exp,warning) do{if(exp) {ISP_TRAC(warning);}}while(0)

//#define ISP_USER_DRV_DEBUG	0

/*------------------------------------------------------------------------------*
*					Data Structures				*
*-------------------------------------------------------------------------------*/
typedef enum
{
	ISP_SUCCESS=0x00,
	ISP_PARAM_NULL,
	ISP_PARAM_ERROR,
	ISP_CALLBACK_NULL,
	ISP_ALLOC_ERROR,
	ISP_ERROR,
	ISP_RETURN_MAX=0xffffffff
}ISP_RETURN_VALUE_E;

// FETCH
int32_t ispGetFetchStatus(uint32_t* status);
int32_t ispFetchBypass(uint8_t bypass);
int32_t ispFetchSubtract(uint8_t eb);
int32_t ispSetFetchSliceSize(uint16_t w, uint16_t h);
int32_t ispSetFetchYAddr(uint32_t y_addr);
int32_t ispSetFetchYPitch(uint32_t y_pitch);
int32_t ispSetFetchUAddr(uint32_t u_addr);
int32_t ispSetFetchUPitch(uint32_t u_pitch);
int32_t ispSetFetchMipiWordInfo(uint16_t word_num);
int32_t ispSetFetchMipiByteInfo(uint16_t byte_rel_pos);
int32_t ispSetFetchVAddr(uint32_t v_addr);
int32_t ispSetFetchVPitch(uint32_t v_pitch);

//BNLC
int32_t ispGetBNLCStatus(uint32_t* status);
int32_t ispBlcBypass(uint8_t bypass);
int32_t ispNlcBypass(uint8_t bypass);
int32_t ispSetBlcMode(uint8_t mode);
int32_t ispSetBlcCalibration(uint16_t r, uint16_t b, uint16_t gr, uint16_t gb);
int32_t ispSetNlcRNode(uint16_t* r_node_ptr);
int32_t ispSetNlcGNode(uint16_t* g_node_ptr);
int32_t ispSetNlcBNode(uint16_t* b_node_ptr);
int32_t ispSetNlcLNode(uint16_t* l_node_ptr);
int32_t ispSetBNLCSliceSize(uint16_t w, uint16_t h);
int32_t ispSetBNLCSliceInfo(uint8_t edge_info);

// Lens
int32_t ispGetLensStatus(uint32_t* status);
int32_t ispLensBypass(uint8_t bypass);
int32_t ispLensBufSel(uint8_t buf_mode);
int32_t ispLensParamAddr(uint32_t param_addr);
int32_t ispSetLensSliceStart(uint16_t x, uint16_t y);
int32_t ispSetLensLoaderEnable(uint8_t eb);
int32_t ispSetLensGridPitch(uint16_t pitch);
int32_t ispSetLensGridMode(uint8_t mode);
int32_t ispSetLensGridSize(uint16_t w, uint16_t h);
int32_t ispSetLensBuf(uint8_t buf_sel);
int32_t ispSetLensEndian(uint8_t endian);

//AWBM
int32_t ispGetAwbmStatus(uint32_t* status);
int32_t ispAwbmBypass(uint8_t bypass);
int32_t ispAwbmMode(uint8_t mode);
int32_t ispSetAwbmWinStart(uint16_t x, uint16_t y);
int32_t ispSetAwbmWinSize(uint16_t w, uint16_t h);
int32_t ispSetAwbmShift(uint16_t shift);

//AWBC
int32_t ispGetAwbcStatus(uint32_t* status);
int32_t ispAwbcBypass(uint8_t bypass);
int32_t ispSetAwbGain(uint16_t r_gain, uint16_t g_gain, uint16_t b_gain);
int32_t ispSetAwbGainThrd(uint16_t r_thr, uint16_t g_thr, uint16_t b_thr);
void ispGetAWBMStatistic(uint32_t* r_info, uint32_t* g_info, uint32_t* b_info);

//BPC
int32_t ispGetBpcStatus(uint32_t* status);
int32_t ispBpcBypass(uint8_t bypass);
int32_t ispBpcMode(uint16_t mode);
int32_t ispSetBpcThrd(uint16_t flat_thr, uint16_t std_thr, uint16_t texture_thr);
int32_t ispBpcMapAddr(uint32_t map_addr);
int32_t ispBpcPixelNum(uint32_t pixel_num);

//Wave denoise
int32_t ispGetWDenoiseStatus(uint32_t* status);
int32_t ispWDenoiseBypass(uint8_t bypass);
int32_t ispWDenoiseWriteBack(uint16_t write_back);
int32_t ispWDenoiseThrd(uint16_t r_thr, uint16_t g_thr, uint16_t b_thr);
int32_t ispWDenoiseSliceSize(uint16_t w, uint16_t h);
int32_t ispWDenoiseSliceInfo(uint16_t edge_info);
int32_t ispWDenoiseDiswei(uint8_t* diswei_ptr);
int32_t ispWDenoiseRanwei(uint8_t* ranwei_ptr);

//GrGb C
int32_t ispGetGrGbStatus(uint32_t* status);
int32_t ispGrGbbypass(uint8_t bypass);
int32_t ispSetGrGbThrd(uint16_t edge_thr, uint16_t diff_thr);

//CFA
int32_t ispGetCFAStatus(uint32_t* status);
int32_t ispSetCFAThrd(uint16_t edge_thr, uint16_t diff_thr);
int32_t ispCFASliceSize(uint16_t w, uint16_t h);
int32_t ispCFASliceInfo(uint16_t edge_info);

//CMC
int32_t ispGetCMCStatus(uint32_t* status);
int32_t ispCMCbypass(uint8_t bypass);
int32_t ispSetCMCMatrix(uint16_t* matrix_ptr);

//Gamma
int32_t ispGetGammaStatus(uint32_t* status);
int32_t ispGammabypass(uint8_t bypass);
int32_t ispSetGammaNode(uint16_t node[][2]);

//CCE
int32_t ispGetCCEStatus(uint32_t* status);
int32_t ispSetCCEMatrix(uint16_t* matrix_ptr);
int32_t ispSetCCEShift(uint16_t y_shift, uint16_t u_shift, uint16_t v_shift);
int32_t ispCCEUVDivBypass(uint8_t bypass);
int32_t ispSetCCEUVDiv(uint8_t* div_ptr);

//PREF
int32_t ispGetPrefStatus(uint32_t* status);
int32_t ispPrefBypass(uint8_t bypass);
int32_t ispPrefWriteBack(uint16_t write_back);
int32_t ispSetPrefThrd(uint8_t y_thr, uint8_t u_thr, uint8_t v_thr);
int32_t ispPrefSliceSize(uint16_t w, uint16_t h);
int32_t ispPrefSliceInfo(uint16_t edge_info);

//BRIGHT
int32_t ispGetBrightStatus(uint32_t* status);
int32_t ispBrightBypass(uint8_t bypass);
int32_t ispSetBrightFactor(uint8_t factor);
int32_t ispBrightSliceSize(uint16_t w, uint16_t h);
int32_t ispBrightSliceInfo(uint16_t edge_info);

//CONTRAST
int32_t ispGetContrastStatus(uint32_t* status);
int32_t ispContrastbypass(uint8_t bypass);
int32_t ispSetContrastFactor(uint8_t factor);

//HIST
int32_t ispGetHistStatus(uint32_t* status);
int32_t ispHistbypass(uint8_t bypass);
int32_t ispHistAutoRstDisenable(uint8_t off);
int32_t ispHistMode(uint8_t mode);
int32_t ispSetHistRatio(uint16_t low_ratio, uint16_t high_ratio);
int32_t ispSetHistMaxMin(uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);
int32_t ispHistClearEn(uint8_t eb);
int32_t ispGetHISTStatistic(uint32_t* param_ptr);

//Auto Contrast
int32_t ispGetAutoContrastStatus(uint32_t* status);
int32_t ispAutoContrastbypass(uint8_t bypass);
int32_t ispSetAutoContrastMode(uint16_t mode);
int32_t ispSetAutoContrastMaxMin(uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);

//AFM
int32_t ispGetAFMStatus(uint32_t* status);
int32_t ispAFMbypass(uint8_t bypass);
int32_t ispSetAFMShift(uint16_t shift);
int32_t ispAFMMode(uint8_t mode);
int32_t ispSetAFMWindow(uint16_t win[][4]);
int32_t ispGetAFMStatistic(uint32_t* statistic_ptr);

//EE
int32_t ispGetEdgeStatus(uint32_t* status);
int32_t ispEdgeBypass(uint8_t bypass);
int32_t ispSetEdgeParam(uint16_t detail_th, uint16_t smooth_th, uint16_t strength);

//Emboss
int32_t ispGetEmbossStatus(uint32_t* status);
int32_t ispEmbossypass(uint8_t bypass);
int32_t ispSetEmbossParam(uint16_t step);

//FS
int32_t ispGetFalseColorStatus(uint32_t* status);
int32_t ispFalseColorBypass(uint8_t bypass);

//CSS color saturation suppression
int32_t ispGetColorSaturationSuppressStatus(uint32_t* status);
int32_t ispColorSaturationSuppressBypass(uint8_t bypass);
int32_t ispSetColorSaturationSuppressThrd(uint8_t* low_thr, uint8_t* low_sum_thr, uint8_t lum_thr, uint chr_thr);
int32_t ispColorSaturationSuppressSliceSize(uint16_t w, uint16_t h);

//SATURATION
int32_t ispGetSaturationStatus(uint32_t* status);
int32_t ispSaturationbypass(uint8_t uint8_t);
int32_t ispSetSaturationFactor(uint8_t factor);

// STORE
int32_t ispGetStoreStatus(uint32_t* status);
int32_t ispStoreBypass(uint8_t bypass);
int32_t ispStoreSubtract(uint8_t subtract);
int32_t ispSetStoreSliceSize(uint16_t w, uint16_t h);
int32_t ispSetStoreYAddr(uint32_t y_addr);
int32_t ispSetStoreYPitch(uint32_t y_pitch);
int32_t ispSetStoreUAddr(uint32_t u_addr);
int32_t ispSetStoreUPitch(uint32_t u_pitch);
int32_t ispSetStoreVAddr(uint32_t v_addr);
int32_t ispSetStoreVPitch(uint32_t v_pitch);
int32_t ispSetStoreInt(uint16_t int_cnt_num, uint8_t int_eb);

// FEEDER
int32_t ispGetFeederStatus(uint32_t* status);
int32_t ispSetFeederDataType(uint16_t data_type);
int32_t ispSetFeederSliceSize(uint16_t w, uint16_t h);

// ARBITER
int32_t ispGetArbiterStatus(uint32_t* status);
int32_t ispArbiterReset(uint8_t rst);
int32_t ispSetArbiterPauseCycle(uint16_t cycle);

// HDR
int32_t ispGetHDRStatus(uint32_t* status);
int32_t ispHDRBypass(uint8_t bypass);
int32_t ispSetHDRLevel(uint16_t level);
int32_t ispSetHDRIndex(uint8_t r_index, uint8_t g_index, uint8_t b_index);
int32_t ispSetHDRIndexTab(uint8_t* com_ptr, uint8_t* p2e_ptr, uint8_t* e2p_ptr);

// COMMON
int32_t ispSetIspStatus(uint8_t end_status, uint8_t end_int);
int32_t isp_Start(uint8_t start);
int32_t ispInMode(uint32_t mode);
int32_t ispOutMode(uint32_t mode);
int32_t ispFetchEdian(uint32_t endian, uint32_t bit_reorder);
int32_t ispBPCEdian(uint32_t endian);
int32_t ispStoreEdian(uint32_t endian);
int32_t ispFetchDataFormat(uint32_t format);
int32_t ispStoreFormat(uint32_t format);
int32_t ispSetBurstSize(uint16_t burst_size);
int32_t ispMemSwitch(uint8_t mem_switch);
int32_t ispShadow(uint8_t shadow);
int32_t ispByerMode(uint32_t bayer_mode);
int32_t ispIntRegister(uint32_t int_num, void(*fun)());
int32_t ispIntClear(uint32_t int_num);
int32_t ispGetIntRaw(uint32_t* raw);
int32_t ispPMURamMask(uint8_t mask);
int32_t ispHwMask(uint32_t hw_logic);
int32_t ispHwEnable(uint32_t hw_logic);
int32_t ispPMUSel(uint8_t sel);
int32_t ispSwEnable(uint32_t sw_logic);
int32_t ispPreviewStop(uint8_t eb);
int32_t ispSetShadowConter(uint32_t conter);
int32_t ispShadowConterClear(uint8_t eb);
int32_t ispAXIStop(uint8_t eb);

//Glb gain
int32_t ispGetGlbGainStatus(uint32_t* status);
int32_t ispGlbGainBypass(uint8_t bypass);
int32_t ispSetGlbGain(uint32_t gain);
int32_t ispGlbGainSliceSize(uint16_t w, uint16_t h);

//RGB gain
int32_t ispGetChnGainStatus(uint32_t* status);
int32_t ispChnGainBypass(uint8_t bypass);
int32_t ispSetChnGain(uint16_t r_gain, uint16_t g_gain, uint16_t b_gain);
int32_t ispSetChnGainOffset(uint16_t r_offset, uint16_t g_offset, uint16_t b_offset);

int32_t ispGetSignalSize(uint16_t* width, uint16_t* height);
int32_t ispGetContinueSize(uint16_t* width, uint16_t* height);
uint32_t ispGetAwbDefaultGain(void);
int32_t ispOpenDev(uint32_t clk_on);
int32_t ispOpenClk(uint32_t clk_on);
int32_t ispReset(uint32_t rst);
int32_t ispGetIRQ(uint32_t *evt_ptr);
int32_t ispRegIRQ(uint32_t int_num);
int32_t ispCfgDcamIRQ(uint32_t param);
int32_t ispStop(void);

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End


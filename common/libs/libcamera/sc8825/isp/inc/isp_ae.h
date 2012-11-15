/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ISP_AE_H_
#define _ISP_AE_H_
/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/
#include <sys/types.h>
/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/
#define ISP_AE_ISO_NUM 0x05
#define ISP_AE_TAB_NUM 0x04
#define ISP_AE_WEIGHT_TAB_NUM 0x03

/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/
enum isp_ae_tab_mode{
	ISP_NORMAL_50HZ=0x00,
	ISP_NORMAL_60HZ,
	ISP_NIGHT_50HZ,
	ISP_NIGHT_60HZ,
	ISP_AE_TAB_MAX
};

struct isp_ae_tab{
	uint16_t* e_ptr;
	uint16_t* g_ptr;
	uint32_t start_index[ISP_AE_ISO_NUM];
	uint32_t max_index[ISP_AE_ISO_NUM];
};

struct isp_ae_param
{
	uint32_t bypass;
	uint32_t init;
	uint32_t valid;
	enum isp_ae_alg_mode alg_mode;
	enum isp_ae_frame_mode frame_mode;
	enum isp_ae_tab_mode tab_type;
	enum isp_iso iso;
	enum isp_ae_wditht weight;
	enum isp_flicker_mode flicker;
	enum isp_ae_mode mode;
	uint32_t skip_frame;
	uint32_t cur_skip_num;
	uint32_t fix_fps;
	int32_t max_index;
	int32_t cur_index;
	uint32_t frame_line;
	uint32_t cur_gain;
	uint32_t cur_exposure;
	uint32_t cur_dummy;
	uint32_t target_lum;
	uint32_t target_zone;
	int32_t ev;
	uint8_t* weight_tab[ISP_AE_WEIGHT_TAB_NUM];
	struct isp_ae_tab tab[ISP_AE_TAB_NUM];
	uint32_t line_time;
	uint32_t ae_set_eb;
};

/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/
uint32_t isp_ae_init(void);
uint32_t isp_ae_deinit(void);
uint32_t isp_ae_calculation(void);
uint32_t isp_ae_set_exposure_gain(void);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End


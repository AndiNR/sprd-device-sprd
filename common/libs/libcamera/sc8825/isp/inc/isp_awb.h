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
#ifndef _ISP_AWB_H_
#define _ISP_AWB_H_
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include <linux/types.h>
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
*				Micro Define					*
*-------------------------------------------------------------------------------*/
#define ISP_AWB_WEIGHT_TAB_NUM 0x03
#define ISP_AWB_TEMPERATRE_NUM 0x14
#define ISP_AWB_GAIN_MAX 0x09

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/

enum isp_awb_wditht{
	ISP_AWB_WDITHT_AVG=0x00,
	ISP_AWB_WDITHT_CENTER,
	ISP_AWB_WDITHT_CUSTOMER,
	ISP_AWB_WDITHT_MAX
};


struct isp_awb_stat{
	uint32_t* r_ptr;
	uint32_t* g_ptr;
	uint32_t* b_ptr;
};

struct isp_awb_coord{
	uint16_t r_g;
	uint16_t b_g;
	uint16_t g;
};

struct isp_awb_gain{
	uint16_t r;
	uint16_t g;
	uint16_t b;
};

struct isp_awb_rgb{
	uint16_t r;
	uint16_t g;
	uint16_t b;
};

struct isp_awb_estable{
	uint32_t valid;
	uint32_t invalid;
	struct isp_awb_rgb valid_rgb[1024];
	struct isp_awb_rgb invalid_rgb[1024];
};

struct isp_awb_param
{
	uint32_t bypass;
	uint32_t init;
	uint32_t valid;
	enum isp_awb_mode mode;
	enum isp_awb_wditht weight;
	uint8_t* weight_tab[ISP_AWB_WEIGHT_TAB_NUM];
	struct isp_awb_coord cal_tab[ISP_AWB_TEMPERATRE_NUM];
	uint8_t cal_num;
	uint32_t valid_block;
	struct isp_awb_estable east;
	struct isp_awb_rgb cur_rgb;
	struct isp_awb_coord prv_coord;
	struct isp_awb_coord cur_coord;
	struct isp_awb_gain target_gain;
	struct isp_awb_gain cur_gain;
	uint32_t matrix_zone;
	uint32_t cal_zone;
	uint32_t target_zone;
	uint32_t matrix_set_eb;
	uint32_t cur_color_temperature;
	uint32_t cur_index;
	uint32_t awb_set_eb;
};

/*------------------------------------------------------------------------------*
*				Data Prototype					*
*-------------------------------------------------------------------------------*/

uint32_t isp_awb_init(void);
uint32_t isp_awb_deinit(void);
uint32_t isp_awb_calculation(void);

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End



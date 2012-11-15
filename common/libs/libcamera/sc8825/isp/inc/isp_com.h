/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ISP_COM_H_
#define _ISP_COM_H_
/*----------------------------------------------------------------------------*
 ** 						 Dependencies									  *
 **---------------------------------------------------------------------------*/
#include <sys/types.h>
#include "isp_app.h"
#include "isp_drv_v0000.h"
#include "sensor_drv_u.h"
#include "isp_awb.h"
#include "isp_ae.h"
#include "isp_af.h"
/**---------------------------------------------------------------------------*
 ** 						 Compiler Flag									  *
 **---------------------------------------------------------------------------*/
#ifdef	 __cplusplus
extern	 "C"
{
#endif

/**---------------------------------------------------------------------------*
**								 Micro Define								 **
**----------------------------------------------------------------------------*/
#define ISP_SLICE_WIN_NUM 0x0b
#define ISP_ALGIN 0x02

#define ISP_CMC_NUM 0x09
#define ISP_COLOR_TEMPRATURE_NUM 0x09
#define ISP_SET_NUM 0x08
/**---------------------------------------------------------------------------*
**								 Data Prototype 							 **
**----------------------------------------------------------------------------*/

enum isp_status{
	ISP_CLOSE=0x00,
	ISP_IDLE,
	ISP_RUN,
	ISP_CONTINUE,
	ISP_SIGNAL,
	ISP_STATE_MAX
};

enum isp_bayer_pattern{
	ISP_GR=0x00,
	ISP_R,
	ISP_B,
	ISP_GB,
	ISP_BAYER_MAX
};

enum isp_fetch_format{
	ISP_FETCH_YUV422_3FRAME=0x00,
	ISP_FETCH_YUYV,
	ISP_FETCH_UYVY,
	ISP_FETCH_YVYU,
	ISP_FETCH_VYUY,
	ISP_FETCH_YUV422_2FRAME,
	ISP_FETCH_YVU422_2FRAME,
	ISP_FETCH_NORMAL_RAW10,
	ISP_FETCH_CSI2_RAW10,
	ISP_FETCH_FORMAT_MAX
};

enum isp_store_format{
	ISP_STORE_UYVY=0x00,
	ISP_STORE_YUV422_2FRAME,
	ISP_STORE_YVU422_2FRAME,
	ISP_STORE_YUV422_3FRAME,
	ISP_STORE_YUV420_2FRAME,
	ISP_STORE_YVU420_2FRAME,
	ISP_STORE_YUV420_3FRAME,
	ISP_STORE_FORMAT_MAX
};

enum isp_work_mode{
	ISP_SINGLE_MODE=0x00,
	ISP_CONTINUE_MODE,
	ISP_WORK_MAX
};

enum isp_match_mode{
	ISP_CAP_MODE=0x00,
	ISP_EMC_MODE,
	ISP_DCAM_MODE,
	ISP_MATCH_MAX
};

enum isp_endian{
	ISP_ENDIAN_LITTLE=0x00,
	ISP_ENDIAN_BIG,
	ISP_ENDIAN_LITTLE_HLF,
	ISP_ENDIAN_BIG_HLF,
	ISP_ENDIAN_MAX
};

enum isp_bit_reorder{
	ISP_LSB=0x00,
	ISP_HSB,
	ISP_BIT_REORDER_MAX
};

enum isp_lns_buf{
	ISP_LNS_BUF0=0x00,
	ISP_LNS_BUF1,
	ISP_LNS_BUF_MAX
};

enum isp_color_temprature{
	ISP_COLOR_TEMPRATURE0=0x00,
	ISP_COLOR_TEMPRATURE1,
	ISP_COLOR_TEMPRATURE2,
	ISP_COLOR_TEMPRATURE3,
	ISP_COLOR_TEMPRATURE4,
	ISP_COLOR_TEMPRATURE5,
	ISP_COLOR_TEMPRATURE6,
	ISP_COLOR_TEMPRATURE7,
	ISP_COLOR_TEMPRATURE8,
	ISP_COLOR_TEMPRATURE_MAX
};

enum isp_process_type{
	ISP_PROC_BAYER=0x00,
	ISP_PROC_YUV,
	ISP_PROCESS_TYPE_MAX
};

enum isp_slice_type{
	ISP_FETCH=0x00,
	ISP_BNLC,
	ISP_LENS,
	ISP_WAVE,
	ISP_CFA,
	ISP_PREF,
	ISP_BRIGHT,
	ISP_CSS,
	ISP_STORE,
	ISP_FEEDER,
	ISP_GLB_GAIN,
	ISP_SLICE_TYPE_MAX
};

enum isp_slice_pos_info{
	ISP_SLICE_ALL=0x00,
	ISP_SLICE_FIRST,
	ISP_SLICE_MID,
	ISP_SLICE_LAST,
	ISP_SLICE_POS_INFO_MAX
};

enum isp_slice_edge_info{
	ISP_SLICE_RIGHT=0x01,
	ISP_SLICE_LEFT=0x02,
	ISP_SLICE_DOWN=0x04,
	ISP_SLICE_UP=0x08,
	ISP_SLICE_EDGE_INFO_MAX
};

struct isp_pitch{
	uint32_t chn0;
	uint32_t chn1;
	uint32_t chn2;
};

struct isp_fetch_param{
	uint32_t bypass;
	uint32_t sub_stract;
	struct isp_addr addr;
	struct isp_pitch pitch;
	uint32_t mipi_word_num;
	uint32_t mipi_byte_rel_pos;
};

struct isp_feeder_param{
	uint32_t data_type;
};

struct isp_store_param{
	uint32_t bypass;
	uint32_t sub_stract;
	struct isp_addr addr;
	struct isp_pitch pitch;
};

struct isp_com_param{
	enum isp_process_type proc_type;
	uint32_t in_mode;
	uint32_t fetch_endian;
	uint32_t fetch_bit_reorder;
	uint32_t bpc_endian;
	uint32_t store_endian;
	uint32_t out_mode;
	uint32_t store_yuv_format;
	uint32_t fetch_color_format;
	uint32_t bayer_pattern;
};

struct isp_slice_param{
	enum isp_slice_pos_info pos_info;
	uint32_t slice_line;
	uint32_t complete_line;
	uint32_t all_line;
	struct isp_size max_size;
	struct isp_size all_slice_num;
	struct isp_size cur_slice_num;

	struct isp_trim_size size[ISP_SLICE_WIN_NUM];
	uint32_t edge_info;
};

struct isp_blc_param{
	uint32_t bypass;
	uint32_t mode;
	uint32_t r;
	uint32_t gr;
	uint32_t gb;
	uint32_t b;
};

struct isp_nlc_param{
	uint32_t bypass;
	uint16_t r_node[29];
	uint16_t g_node[29];
	uint16_t b_node[29];
	uint16_t l_node[29];
};

struct isp_lnc_map{
	uint32_t grid_mode;
	uint32_t grid_pitch;
	uint32_t param_addr;
};

struct isp_lnc_param{
	uint32_t bypass;
	uint32_t cur_use_buf;
	uint32_t load_buf;
	struct isp_lnc_map map;
};

struct isp_awbm_param{
	uint32_t bypass;
	struct isp_pos win_start;
	struct isp_size win_size;
};

struct isp_awbc_param{
	uint32_t bypass;
	uint32_t r_gain;
	uint32_t g_gain;
	uint32_t b_gain;
};

struct isp_awb_statistic_info{
	uint32_t r_info[1024];
	uint32_t g_info[1024];
	uint32_t b_info[1024];
};

struct isp_bpc_param{
	uint32_t bypass;
	uint32_t mode;
	uint16_t flat_thr;
	uint16_t std_thr;
	uint16_t texture_thr;
	uint16_t reserved;
	uint32_t map_addr;
};

struct isp_denoise_param
{
	uint32_t bypass;
	uint16_t write_back;
	uint16_t r_thr;
	uint16_t g_thr;
	uint16_t b_thr;
	uint8_t diswei[19];
	uint8_t ranwei[31];
	uint8_t reserved1;
	uint8_t reserved0;
};

struct isp_grgb_param
{
	uint32_t bypass;
	uint32_t edge_thr;
	uint32_t diff_thr;
};

struct isp_cfa_param
{
	uint32_t edge_thr;
	uint32_t diff_thr;
};

struct isp_cmc_param
{
	uint32_t bypass;
	uint16_t matrix[ISP_CMC_NUM][9];
	uint16_t reserved;
};

struct isp_gamma_param{
	uint32_t bypass;
	uint16_t axis[26][2];
};

struct isp_special_effect{
	enum isp_special_effect_mode mode;
};

struct isp_cce_matrix{
	uint16_t matrix[9];
	uint16_t y_shift;
	uint16_t u_shift;
	uint16_t v_shift;
};

struct isp_cce_uvdiv
{
	uint32_t bypass;
	uint8_t thrd[7];
};

struct isp_pref_param
{
	uint32_t bypass;
	uint32_t write_back;
	uint8_t y_thr;
	uint8_t u_thr;
	uint8_t v_thr;
	uint8_t reserved;
};

struct isp_bright_param{
	uint32_t bypass;
	uint8_t factor;
};

struct isp_contrast_param{
	uint32_t bypass;
	uint8_t factor;
};

struct isp_hist_param{
	uint32_t bypass;
	uint16_t low_ratio;
	uint16_t high_ratio;
	uint8_t mode;
	uint8_t in_min;
	uint8_t in_max;
	uint8_t out_min;
	uint8_t out_max;
};

struct isp_hist_statistic_info{
	uint32_t hist_info[256];
};

struct isp_auto_contrast_param{
	uint32_t bypass;
	uint8_t mode;
	uint8_t in_min;
	uint8_t in_max;
	uint8_t out_min;
	uint8_t out_max;
};

struct isp_saturation_param{
	uint32_t bypass;
	uint8_t factor;
};

struct isp_af_statistic_info{
	uint32_t info[9];
};

struct isp_edge_param{
	uint32_t bypass;
	uint8_t detail_thr;
	uint8_t smooth_thr;
	uint8_t strength;
	uint8_t reserved;
};

struct isp_emboss_param{
	uint32_t bypass;
	uint32_t step;
};

struct isp_fcs_param{
	uint32_t bypass;
};

struct isp_css_param{
	uint32_t bypass;
	uint8_t low_thr[7];
	uint8_t lum_thr;
	uint8_t low_sum_thr[7];
	uint8_t chr_thr;
};

struct isp_hdr_param{
	uint32_t bypass;
	uint32_t level;
};

struct isp_hdr_index{
	uint8_t r_index;
	uint8_t g_index;
	uint8_t b_index;
	uint8_t* com_ptr;
	uint8_t* p2e_ptr;
	uint8_t* e2p_ptr;
};

struct isp_global_gain_param{
	uint32_t bypass;
	uint32_t gain;
};

struct isp_chn_gain_param{
	uint32_t bypass;
	uint8_t r_gain;
	uint8_t g_gain;
	uint8_t b_gain;
	uint8_t reserved0;
	uint16_t r_offset;
	uint16_t g_offset;
	uint16_t b_offset;
	uint16_t reserved1;
};

struct isp_tune_block{
	uint8_t eb;
	uint8_t ae;
	uint8_t awb;
	uint8_t special_effect;
	uint8_t bright;
	uint8_t contrast;
	uint8_t hist;
	uint8_t auto_contrast;
	uint8_t saturation;
	uint8_t af;
	uint8_t css;
	uint8_t hdr;
	uint8_t global_gain;
	uint8_t chn_gain;
};

struct isp_data_param
{
	enum isp_work_mode work_mode;
	enum isp_match_mode input;
	enum isp_format input_format;
	uint32_t format_pattern;
	struct isp_size input_size;
	struct isp_addr input_addr;
	enum isp_format output_format;
	enum isp_match_mode output;
	struct isp_addr output_addr;
	uint16_t slice_height;
	isp_callback ctrl_callback;
	isp_callback af_notice_callback;
	isp_callback skip_frame_callback;
};

struct isp_cfg_param
{
	struct isp_data_param data;
	void* sensor_info_ptr;
};

struct isp_context{
	// isp param
	struct isp_cfg_param cfg;
	struct isp_fetch_param featch;
	struct isp_feeder_param feeder;
	struct isp_store_param store;
	struct isp_com_param com;
	struct isp_blc_param blc;
	struct isp_nlc_param nlc;
	struct isp_lnc_param lnc;
	struct isp_awbm_param awbm;
	struct isp_awbc_param awbc;
	struct isp_ae_param ae;
	struct isp_awb_param awb;
	struct isp_awb_statistic_info awb_stat;
	struct isp_bpc_param bpc;
	struct isp_denoise_param denoise;
	struct isp_grgb_param grgb;
	struct isp_cfa_param cfa;
	struct isp_cmc_param cmc;
	struct isp_gamma_param gamma;
//	  struct isp_special_effect special_effect;
	struct isp_cce_matrix cce_matrix;
	struct isp_cce_uvdiv uv_div;
	struct isp_pref_param pref;
	struct isp_bright_param bright;
	struct isp_contrast_param contrast;
	struct isp_hist_param hist;
	struct isp_hist_statistic_info hist_stat;
	struct isp_auto_contrast_param auto_contrast;
	struct isp_saturation_param saturation;
	struct isp_af_param af;
	struct isp_af_statistic_info af_stat;
	struct isp_edge_param edge;
	struct isp_emboss_param emboss;
	struct isp_fcs_param fcs;
	struct isp_css_param css;
	struct isp_hdr_param hdr;
	struct isp_hdr_index hdr_index;
	struct isp_global_gain_param global;
	struct isp_chn_gain_param chn;
	struct isp_size src;
	struct isp_slice_param slice;
	struct isp_tune_block tune;

	struct isp_lnc_map lnc_map_tab[ISP_SET_NUM][ISP_COLOR_TEMPRATURE_NUM];
	uint8_t bright_tab[16];
	uint8_t contrast_tab[16];
	uint8_t saturation_tab[16];
	uint8_t ev_tab[16];

	uint32_t awb_r_gain[9];
	uint32_t awb_g_gain[9];
	uint32_t awb_b_gain[9];

	uint32_t param_index;
	// isp system
	uint32_t isp_status;
	pthread_t monitor_thr;
	uint32_t monitor_queue;
	pthread_t ctrl_thr;
	uint32_t ctrl_queue;
	pthread_t proc_thr;
	uint32_t proc_queue;
	uint32_t monitor_status;
	uint32_t ctrl_status;
	uint32_t proc_status;
	pthread_mutex_t ctrl_mutex;
	pthread_mutex_t proc_mutex;
	pthread_mutex_t cond_mutex;

	pthread_cond_t init_cond;
	pthread_cond_t deinit_cond;
	pthread_cond_t continue_cond;
	pthread_cond_t continue_stop_cond;
	pthread_cond_t signal_cond;
	pthread_cond_t ioctrl_cond;
	pthread_cond_t thread_common_cond;
};

struct isp_context* ispGetContext(void);
uint32_t ISP_Algin(uint32_t pLen , uint16_t algin_blk, uint16_t algin_bits);

/**----------------------------------------------------------------------------*
**						   Compiler Flag									  **
**----------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End


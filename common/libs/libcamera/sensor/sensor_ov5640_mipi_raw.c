/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"


#define ov5640_I2C_ADDR_W        0x3c
#define ov5640_I2C_ADDR_R         0x3c

LOCAL uint32_t _ov5640_GetResolutionTrimTab(uint32_t param);
LOCAL uint32_t _ov5640_PowerOn(uint32_t power_on);
LOCAL uint32_t _ov5640_Identify(uint32_t param);

LOCAL uint32_t _ov5640_BeforeSnapshot(uint32_t param);
LOCAL uint32_t _ov5640_after_snapshot(uint32_t param);
LOCAL uint32_t _ov5640_StreamOn(uint32_t param);
LOCAL uint32_t _ov5640_StreamOff(uint32_t param);

static uint32_t g_flash_mode_en = 0;

LOCAL const SENSOR_REG_T ov5640_1280X960_mipi_raw[] = {
	{0x3103, 0x11},		/* sysclk from pad*/
	{0x3008, 0x82},		/*software reset*/
	{SENSOR_WRITE_DELAY, 0x0a},
	{0x3008, 0x42},		/*software power down*/
	{0x3103, 0x03},		/*sysclk from pll*/
	{0x3017, 0x00},		/*Frex, Vsync, Href, PCLK, D[9:6] output*/
	{0x3018, 0x00},		/*D[5:0], GPIO[1:0] output*/
	{0x3034, 0x1a},
//	{0x3035, 0x12},		/*0x11-->30fps; 0x21-->15fps*/
//	{0x3036, 0x46},
	{0x3035, 0x14},		/*0x11-->30fps; 0x21-->15fps*/
	{0x3036, 0x38},
	{0x3037, 0x13},
	{0x3108, 0x01},		/* clock divider*/
	{0x3630, 0x36},
	{0x3631, 0x0e},
	{0x3632, 0xe2},
	{0x3633, 0x12},
	{0x3621, 0xe0},
	{0x3704, 0xa0},
	{0x3703, 0x5a},
	{0x3715, 0x78},
	{0x3717, 0x01},
	{0x370b, 0x60},
	{0x3705, 0x1a},
	{0x3905, 0x02},
	{0x3906, 0x10},
	{0x3901, 0x0a},
	{0x3731, 0x12},
	{0x3600, 0x08},		/*VCM debug*/
	{0x3601, 0x33},		/*VCM debug*/
	{0x302d, 0x60},		/*system control*/
	{0x3620, 0x52},
	{0x371b, 0x20},
	{0x471c, 0x50},
	{0x3a13, 0x43},		/*pre-gain = 1.05x*/
	{0x3a18, 0x00},		/*gain ceiling*/
	{0x3a19, 0xf8},
	{0x3635, 0x13},
	{0x3636, 0x03},
	{0x3634, 0x40},
	{0x3622, 0x01},
	{0x3c01, 0x34},		/* sum auto, band counter enable, threshold = 4*/
	{0x3c04, 0x28},		/* threshold low sum*/
	{0x3c05, 0x98},		/* threshold high sum*/
	{0x3c06, 0x00},		/* light meter 1 threshold H*/
	{0x3c07, 0x07},		/* light meter 1 threshold L*/
	{0x3c08, 0x00},		/* light meter 2 threshold H*/
	{0x3c09, 0x1c},		/* light meter 2 threshold L*/
	{0x3c0a, 0x9c},		/* sample number H*/
	{0x3c0b, 0x40},		/* sample number L*/
	{0x3820, 0x41},		
	{0x3821, 0x03},		
	{0x3814, 0x31},		
	{0x3815, 0x31},	
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x0a},
	{0x3805, 0x3f},
	{0x3806, 0x07},
	{0x3807, 0x9b},
	{0x3808, 0x05},
	{0x3809, 0x00},
	{0x380a, 0x03},
	{0x380b, 0xc0},
	{0x380c, 0x07},
	{0x380d, 0x68},
	{0x380e, 0x03},
	{0x380f, 0xd8},
	{0x3810, 0x00},
	{0x3811, 0x10},
	{0x3812, 0x00},
	{0x3813, 0x06},
	{0x3618, 0x00},
	{0x3612, 0x29},
	{0x3708, 0x64},
	{0x3709, 0x52},
	{0x370c, 0x03},
	{0x3a02, 0x03},
	{0x3a03, 0xd8},
	{0x3a08, 0x01},
	{0x3a09, 0x27},
	{0x3a0a, 0x00},
	{0x3a0b, 0xf6},
	{0x3a0e, 0x03},
	{0x3a0d, 0x04},
	{0x3a14, 0x03},
	{0x3a15, 0xd8},
	{0x4001, 0x02},
	{0x4004, 0x02},
	{0x3000, 0x00},
	{0x3002, 0x1c},
	{0x3004, 0xff},
	{0x3006, 0xc3},
	{0x300e, 0x45},
	{0x302e, 0x08},
	{0x4300, 0xf8},//30
	{0x501f, 0x03},
	{0x4713, 0x03},
	{0x4407, 0x04},
	{0x440e, 0x00},
	{0x460b, 0x37},
	{0x460c, 0x20},
	{0x4837, 0x22},
	{0x3824, 0x04},
	{0x5000, 0x06},
	{0x5001, 0x00},
	{0x3a0f, 0x36},
	{0x3a10, 0x2e},
	{0x3a1b, 0x38},
	{0x3a1e, 0x2c},
	{0x3a11, 0x70},
	{0x3a1f, 0x18},

	{0x3034, 0x1a},
	{0x3035, 0x12},
	{0x3036, 0x54},
	{0x3037, 0x13},
	{0x4837, 0x0a},

//	{0x3008, 0x02}
};

LOCAL SENSOR_REG_TAB_INFO_T s_ov5640_resolution_Tab_RAW[] = {
	{NULL, 0, 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},

	{ADDR_AND_LEN_OF_ARRAY(ov5640_1280X960_mipi_raw), 1280, 960, 12, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov5640_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0},
	{0, 0, 1280, 960, 1220, 56},
	{0, 0, 1280, 960, 122, 42},
	{0, 0, 1600, 1200, 122, 42},
	{0, 0, 2048, 1536, 122, 42},
	{0, 0, 2592, 1944, 122, 42},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0}
};

static const uint8_t s_ov5640_lnc_00[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_01[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_02[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_03[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_04[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_05[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_06[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_07[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_08[]=
{
	0x00
};

static const uint8_t s_ov5640_ae_weight_customer[]=
{
#include "sensor_ov5640_ae_weight_customer.dat"
};

/* 00: 0x->50hz 1x->60hz x0->normal x1->night*/
static const uint16_t s_ov5640_aes_00[250]={
#include "sensor_ov5640_aes_00.dat"
};

static const uint16_t s_ov5640_aeg_00[250]={
#include "sensor_ov5640_aeg_00.dat"
};

static const uint16_t s_ov5640_aes_10[250]={
#include "sensor_ov5640_aes_10.dat"
};

static const uint16_t s_ov5640_aeg_10[250]={
#include "sensor_ov5640_aeg_10.dat"
};

static const uint16_t s_ov5640_aes_01[250]={
#include "sensor_ov5640_aes_01.dat"
};

static const uint16_t s_ov5640_aeg_01[250]={
#include "sensor_ov5640_aeg_01.dat"
};

static const uint16_t s_ov5640_aes_11[250]={
#include "sensor_ov5640_aes_11.dat"
};

static const uint16_t s_ov5640_aeg_11[250]={
#include "sensor_ov5640_aeg_11.dat"
};
#if 0
static const uint8_t s_ov5640_tune_info[sizeof(struct sensor_raw_tune_info)]={
#include "sensor_ov5640_tune_info.dat"
};
#else
static struct sensor_raw_tune_info s_ov5640_tune_info={
	0x00
};
#endif

#if 1
static struct sensor_raw_fix_info s_ov5640_fix_info={
//ae
{
	(uint8_t*)s_ov5640_ae_weight_customer,

	(uint16_t*)s_ov5640_aes_00,
	(uint16_t*)s_ov5640_aeg_00,
	128,
	250,
	128,
	250,
	128,
	250,
	128,
	250,
	128,
	250,

	(uint16_t*)s_ov5640_aes_01,
	(uint16_t*)s_ov5640_aeg_01,
	128,
	250,
	128,
	250,
	128,
	250,
	128,
	250,
	128,
	250,

	(uint16_t*)s_ov5640_aes_10,
	(uint16_t*)s_ov5640_aeg_10,
	128,
	250,
	128,
	250,
	128,
	250,
	128,
	250,
	128,
	250,

	(uint16_t*)s_ov5640_aes_11,
	(uint16_t*)s_ov5640_aeg_11,
	128,
	250,
	128,
	250,
	128,
	250,
	128,
	250,
	128,
	250,
},

//lnc
{
	0x00,
	(uint32_t*)s_ov5640_lnc_00,
	0x00,

	0x00,
	(uint32_t*)s_ov5640_lnc_01,
	0x00,

	0x00,
	(uint32_t*)s_ov5640_lnc_02,
	0x00,

	0x00,
	(uint32_t*)s_ov5640_lnc_03,
	0x00,

	0x00,
	(uint32_t*)s_ov5640_lnc_04,
	0x00,

	0x00,
	(uint32_t*)s_ov5640_lnc_05,
	0x00,

	0x00,
	(uint32_t*)s_ov5640_lnc_06,
	0x00,

	0x00,
	(uint32_t*)s_ov5640_lnc_07,
	0x00,

	0x00,
	(uint32_t*)s_ov5640_lnc_08,
	0x00
}
};
#endif
static struct sensor_version_info s_ov5640_version_info={
	SENSOR_RAW_VERSION_ID,
	sizeof(struct sensor_raw_info),
	0x00
};

static struct sensor_raw_info s_ov5640_mipi_raw_info={
	&s_ov5640_version_info,
	&s_ov5640_tune_info,
	&s_ov5640_fix_info,
};

struct sensor_raw_info* s_ov5640_mipi_raw_info_ptr=&s_ov5640_mipi_raw_info;

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov5640_ioctl_func_tab = {
	PNULL,
	_ov5640_PowerOn,
	PNULL,
	_ov5640_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_ov5640_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov5640_set_brightness,
	PNULL, // _ov5640_set_contrast,
	PNULL,
	PNULL,			//_ov5640_set_saturation,

	PNULL, //_ov5640_set_work_mode,
	PNULL, //_ov5640_set_image_effect,

	PNULL, //_ov5640_BeforeSnapshot,
	PNULL, //_ov5640_after_snapshot,
	PNULL, //_ov540_flash,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov5640_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov5640_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov5640_GetExifInfo,
	PNULL,
	PNULL, //_ov5640_set_anti_flicker,
	PNULL, //_ov5640_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov5640_StreamOn,
	_ov5640_StreamOff
};


SENSOR_INFO_T g_ov5640_mipi_raw_info = {
	ov5640_I2C_ADDR_W,	// salve i2c write address
	ov5640_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N | SENSOR_HW_SIGNAL_HSYNC_P,	// bit0: 0:negative; 1:positive -> polarily of pixel clock
	// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN |
	    SENSOR_IMAGE_EFFECT_BLUE |
	    SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,			// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	50,			// reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x0A, 0x56},		// supply two code to identify sensor.
	 {0x0B, 0x40}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	2592,			// max width of source image
	1944,			// max height of source image
	"ov5640",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_GB,// pattern of input image form sensor;

	s_ov5640_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov5640_ioctl_func_tab,	// point to ioctl function table
	&s_ov5640_mipi_raw_info,		// information and table about Rawrgb sensor
	NULL,			//&g_ov5640_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1500MV,	// dvdd
	3,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 1}
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov5640_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;

	raw_sensor_ptr->version_info->version_id=0x00000000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

	//bypass
	sensor_ptr->blc_bypass=0x01;
	sensor_ptr->nlc_bypass=0x01;
	sensor_ptr->lnc_bypass=0x01;
	sensor_ptr->ae_bypass=0x01;
	sensor_ptr->awb_bypass=0x01;
	sensor_ptr->bpc_bypass=0x01;
	sensor_ptr->denoise_bypass=0x01;
	sensor_ptr->grgb_bypass=0x01;
	sensor_ptr->cmc_bypass=0x01;
	sensor_ptr->gamma_bypass=0x01;
	sensor_ptr->uvdiv_bypass=0x01;
	sensor_ptr->pref_bypass=0x01;
	sensor_ptr->bright_bypass=0x01;
	sensor_ptr->contrast_bypass=0x01;
	sensor_ptr->hist_bypass=0x01;
	sensor_ptr->auto_contrast_bypass=0x01;
	sensor_ptr->af_bypass=0x01;
	sensor_ptr->edge_bypass=0x01;
	sensor_ptr->fcs_bypass=0x01;
	sensor_ptr->css_bypass=0x01;
	sensor_ptr->saturation_bypass=0x01;
	sensor_ptr->hdr_bypass=0x01;
	sensor_ptr->glb_gain_bypass=0x01;
	sensor_ptr->chn_gain_bypass=0x01;

	//blc
	sensor_ptr->blc.mode=0x00; // 0:software 1:camera interface
	sensor_ptr->blc.r=0x00;
	sensor_ptr->blc.gr=0x00;
	sensor_ptr->blc.gb=0x00;
	sensor_ptr->blc.b=0x00;
	//nlc
	sensor_ptr->nlc.r_node[0]=0;
	sensor_ptr->nlc.r_node[1]=16;
	sensor_ptr->nlc.r_node[2]=32;
	sensor_ptr->nlc.r_node[3]=64;
	sensor_ptr->nlc.r_node[4]=96;
	sensor_ptr->nlc.r_node[5]=128;
	sensor_ptr->nlc.r_node[6]=160;
	sensor_ptr->nlc.r_node[7]=192;
	sensor_ptr->nlc.r_node[8]=224;
	sensor_ptr->nlc.r_node[9]=256;
	sensor_ptr->nlc.r_node[10]=288;
	sensor_ptr->nlc.r_node[11]=320;
	sensor_ptr->nlc.r_node[12]=384;
	sensor_ptr->nlc.r_node[13]=448;
	sensor_ptr->nlc.r_node[14]=512;
	sensor_ptr->nlc.r_node[15]=576;
	sensor_ptr->nlc.r_node[16]=640;
	sensor_ptr->nlc.r_node[17]=672;
	sensor_ptr->nlc.r_node[18]=704;
	sensor_ptr->nlc.r_node[19]=736;
	sensor_ptr->nlc.r_node[20]=768;
	sensor_ptr->nlc.r_node[21]=800;
	sensor_ptr->nlc.r_node[22]=832;
	sensor_ptr->nlc.r_node[23]=864;
	sensor_ptr->nlc.r_node[24]=896;
	sensor_ptr->nlc.r_node[25]=928;
	sensor_ptr->nlc.r_node[26]=960;
	sensor_ptr->nlc.r_node[27]=992;
	sensor_ptr->nlc.r_node[28]=1023;

	sensor_ptr->nlc.g_node[0]=0;
	sensor_ptr->nlc.g_node[1]=16;
	sensor_ptr->nlc.g_node[2]=32;
	sensor_ptr->nlc.g_node[3]=64;
	sensor_ptr->nlc.g_node[4]=96;
	sensor_ptr->nlc.g_node[5]=128;
	sensor_ptr->nlc.g_node[6]=160;
	sensor_ptr->nlc.g_node[7]=192;
	sensor_ptr->nlc.g_node[8]=224;
	sensor_ptr->nlc.g_node[9]=256;
	sensor_ptr->nlc.g_node[10]=288;
	sensor_ptr->nlc.g_node[11]=320;
	sensor_ptr->nlc.g_node[12]=384;
	sensor_ptr->nlc.g_node[13]=448;
	sensor_ptr->nlc.g_node[14]=512;
	sensor_ptr->nlc.g_node[15]=576;
	sensor_ptr->nlc.g_node[16]=640;
	sensor_ptr->nlc.g_node[17]=672;
	sensor_ptr->nlc.g_node[18]=704;
	sensor_ptr->nlc.g_node[19]=736;
	sensor_ptr->nlc.g_node[20]=768;
	sensor_ptr->nlc.g_node[21]=800;
	sensor_ptr->nlc.g_node[22]=832;
	sensor_ptr->nlc.g_node[23]=864;
	sensor_ptr->nlc.g_node[24]=896;
	sensor_ptr->nlc.g_node[25]=928;
	sensor_ptr->nlc.g_node[26]=960;
	sensor_ptr->nlc.g_node[27]=992;
	sensor_ptr->nlc.g_node[28]=1023;

	sensor_ptr->nlc.b_node[0]=0;
	sensor_ptr->nlc.b_node[1]=16;
	sensor_ptr->nlc.b_node[2]=32;
	sensor_ptr->nlc.b_node[3]=64;
	sensor_ptr->nlc.b_node[4]=96;
	sensor_ptr->nlc.b_node[5]=128;
	sensor_ptr->nlc.b_node[6]=160;
	sensor_ptr->nlc.b_node[7]=192;
	sensor_ptr->nlc.b_node[8]=224;
	sensor_ptr->nlc.b_node[9]=256;
	sensor_ptr->nlc.b_node[10]=288;
	sensor_ptr->nlc.b_node[11]=320;
	sensor_ptr->nlc.b_node[12]=384;
	sensor_ptr->nlc.b_node[13]=448;
	sensor_ptr->nlc.b_node[14]=512;
	sensor_ptr->nlc.b_node[15]=576;
	sensor_ptr->nlc.b_node[16]=640;
	sensor_ptr->nlc.b_node[17]=672;
	sensor_ptr->nlc.b_node[18]=704;
	sensor_ptr->nlc.b_node[19]=736;
	sensor_ptr->nlc.b_node[20]=768;
	sensor_ptr->nlc.b_node[21]=800;
	sensor_ptr->nlc.b_node[22]=832;
	sensor_ptr->nlc.b_node[23]=864;
	sensor_ptr->nlc.b_node[24]=896;
	sensor_ptr->nlc.b_node[25]=928;
	sensor_ptr->nlc.b_node[26]=960;
	sensor_ptr->nlc.b_node[27]=992;
	sensor_ptr->nlc.b_node[28]=1023;

	sensor_ptr->nlc.l_node[0]=0;
	sensor_ptr->nlc.l_node[1]=16;
	sensor_ptr->nlc.l_node[2]=32;
	sensor_ptr->nlc.l_node[3]=64;
	sensor_ptr->nlc.l_node[4]=96;
	sensor_ptr->nlc.l_node[5]=128;
	sensor_ptr->nlc.l_node[6]=160;
	sensor_ptr->nlc.l_node[7]=192;
	sensor_ptr->nlc.l_node[8]=224;
	sensor_ptr->nlc.l_node[9]=256;
	sensor_ptr->nlc.l_node[10]=288;
	sensor_ptr->nlc.l_node[11]=320;
	sensor_ptr->nlc.l_node[12]=384;
	sensor_ptr->nlc.l_node[13]=448;
	sensor_ptr->nlc.l_node[14]=512;
	sensor_ptr->nlc.l_node[15]=576;
	sensor_ptr->nlc.l_node[16]=640;
	sensor_ptr->nlc.l_node[17]=672;
	sensor_ptr->nlc.l_node[18]=704;
	sensor_ptr->nlc.l_node[19]=736;
	sensor_ptr->nlc.l_node[20]=768;
	sensor_ptr->nlc.l_node[21]=800;
	sensor_ptr->nlc.l_node[22]=832;
	sensor_ptr->nlc.l_node[23]=864;
	sensor_ptr->nlc.l_node[24]=896;
	sensor_ptr->nlc.l_node[25]=928;
	sensor_ptr->nlc.l_node[26]=960;
	sensor_ptr->nlc.l_node[27]=992;
	sensor_ptr->nlc.l_node[28]=1023;

	//ae
	sensor_ptr->ae.fix_fps=0x00;
	sensor_ptr->ae.target_lum=70;
	sensor_ptr->ae.target_zone=8;
	sensor_ptr->ae.ev[0]=0x00;
	sensor_ptr->ae.ev[1]=0x00;
	sensor_ptr->ae.ev[2]=0x00;
	sensor_ptr->ae.ev[3]=0x00;
	sensor_ptr->ae.ev[4]=0x00;
	sensor_ptr->ae.ev[5]=0x00;
	sensor_ptr->ae.ev[6]=0x00;
	sensor_ptr->ae.ev[7]=0x00;
	sensor_ptr->ae.ev[8]=0x00;
	sensor_ptr->ae.ev[9]=0x00;
	sensor_ptr->ae.ev[10]=0x00;
	sensor_ptr->ae.ev[11]=0x00;
	sensor_ptr->ae.ev[12]=0x00;
	sensor_ptr->ae.ev[13]=0x00;
	sensor_ptr->ae.ev[14]=0x00;
	sensor_ptr->ae.ev[15]=0x00;

	//awb
	sensor_ptr->awb.win_start.x=0x00;
	sensor_ptr->awb.win_start.y=0x00;
	sensor_ptr->awb.win_size.w=40;
	sensor_ptr->awb.win_size.h=30;
	sensor_ptr->awb.r_gain[0]=0x40;
	sensor_ptr->awb.g_gain[0]=0x40;
	sensor_ptr->awb.b_gain[0]=0x40;
	sensor_ptr->awb.r_gain[1]=0x40;
	sensor_ptr->awb.g_gain[1]=0x40;
	sensor_ptr->awb.b_gain[1]=0x40;
	sensor_ptr->awb.r_gain[2]=0x40;
	sensor_ptr->awb.g_gain[2]=0x40;
	sensor_ptr->awb.b_gain[2]=0x40;
	sensor_ptr->awb.r_gain[3]=0x40;
	sensor_ptr->awb.g_gain[3]=0x40;
	sensor_ptr->awb.b_gain[3]=0x40;
	sensor_ptr->awb.r_gain[4]=0x40;
	sensor_ptr->awb.g_gain[4]=0x40;
	sensor_ptr->awb.b_gain[4]=0x40;
	sensor_ptr->awb.r_gain[5]=0x40;
	sensor_ptr->awb.g_gain[5]=0x40;
	sensor_ptr->awb.b_gain[5]=0x40;
	sensor_ptr->awb.r_gain[6]=0x40;
	sensor_ptr->awb.g_gain[6]=0x40;
	sensor_ptr->awb.b_gain[6]=0x40;
	sensor_ptr->awb.r_gain[7]=0x40;
	sensor_ptr->awb.g_gain[7]=0x40;
	sensor_ptr->awb.b_gain[7]=0x40;
	sensor_ptr->awb.r_gain[8]=0x40;
	sensor_ptr->awb.g_gain[8]=0x40;
	sensor_ptr->awb.b_gain[8]=0x40;
	sensor_ptr->awb.matrix_zone=0x40;
	sensor_ptr->awb.cal_zone=0x40;
	sensor_ptr->awb.target_zone=0x40;

	sensor_ptr->awb.cali_num=0x09;

	sensor_ptr->awb.cali[0].r=0x80;
	sensor_ptr->awb.cali[0].g=0x80;
	sensor_ptr->awb.cali[0].b=0x80;

	sensor_ptr->awb.cali[1].r=0x80;
	sensor_ptr->awb.cali[1].g=0x80;
	sensor_ptr->awb.cali[1].b=0x80;

	sensor_ptr->awb.cali[2].r=0x80;
	sensor_ptr->awb.cali[2].g=0x80;
	sensor_ptr->awb.cali[2].b=0x80;

	sensor_ptr->awb.cali[3].r=0x80;
	sensor_ptr->awb.cali[3].g=0x80;
	sensor_ptr->awb.cali[3].b=0x80;

	sensor_ptr->awb.cali[4].r=0x80;
	sensor_ptr->awb.cali[4].g=0x80;
	sensor_ptr->awb.cali[4].b=0x80;

	sensor_ptr->awb.cali[5].r=0x80;
	sensor_ptr->awb.cali[5].g=0x80;
	sensor_ptr->awb.cali[5].b=0x80;

	sensor_ptr->awb.cali[6].r=0x80;
	sensor_ptr->awb.cali[6].g=0x80;
	sensor_ptr->awb.cali[6].b=0x80;

	sensor_ptr->awb.cali[7].r=0x80;
	sensor_ptr->awb.cali[7].g=0x80;
	sensor_ptr->awb.cali[7].b=0x80;

	sensor_ptr->awb.cali[8].r=0x80;
	sensor_ptr->awb.cali[8].g=0x80;
	sensor_ptr->awb.cali[8].b=0x80;

	//bpc
	//    sensor_ptr->bpc.mode=0x01; // 0:adaptive 1:map
	sensor_ptr->bpc.flat_thr=80;
	sensor_ptr->bpc.std_thr=20;
	sensor_ptr->bpc.texture_thr=2;

	// denoise
	sensor_ptr->denoise.write_back=0x02;
	sensor_ptr->denoise.r_thr=0x08;
	sensor_ptr->denoise.g_thr=0x08;
	sensor_ptr->denoise.b_thr=0x08;

	sensor_ptr->denoise.diswei[0]=255;
	sensor_ptr->denoise.diswei[1]=247;
	sensor_ptr->denoise.diswei[2]=239;
	sensor_ptr->denoise.diswei[3]=232;
	sensor_ptr->denoise.diswei[4]=225;
	sensor_ptr->denoise.diswei[5]=218;
	sensor_ptr->denoise.diswei[6]=211;
	sensor_ptr->denoise.diswei[7]=204;
	sensor_ptr->denoise.diswei[8]=198;
	sensor_ptr->denoise.diswei[9]=192;
	sensor_ptr->denoise.diswei[10]=186;
	sensor_ptr->denoise.diswei[11]=180;
	sensor_ptr->denoise.diswei[12]=175;
	sensor_ptr->denoise.diswei[13]=169;
	sensor_ptr->denoise.diswei[14]=164;
	sensor_ptr->denoise.diswei[15]=159;
	sensor_ptr->denoise.diswei[16]=154;
	sensor_ptr->denoise.diswei[17]=149;
	sensor_ptr->denoise.diswei[18]=145;

	sensor_ptr->denoise.ranwei[0]=255;
	sensor_ptr->denoise.ranwei[1]=247;
	sensor_ptr->denoise.ranwei[2]=225;
	sensor_ptr->denoise.ranwei[3]=192;
	sensor_ptr->denoise.ranwei[4]=154;
	sensor_ptr->denoise.ranwei[5]=116;
	sensor_ptr->denoise.ranwei[6]=82;
	sensor_ptr->denoise.ranwei[7]=55;
	sensor_ptr->denoise.ranwei[8]=34;
	sensor_ptr->denoise.ranwei[9]=20;
	sensor_ptr->denoise.ranwei[10]=19;
	sensor_ptr->denoise.ranwei[11]=18;
	sensor_ptr->denoise.ranwei[12]=17;
	sensor_ptr->denoise.ranwei[13]=16;
	sensor_ptr->denoise.ranwei[14]=15;
	sensor_ptr->denoise.ranwei[15]=14;
	sensor_ptr->denoise.ranwei[16]=13;
	sensor_ptr->denoise.ranwei[17]=12;
	sensor_ptr->denoise.ranwei[18]=11;
	sensor_ptr->denoise.ranwei[19]=10;
	sensor_ptr->denoise.ranwei[20]=9;
	sensor_ptr->denoise.ranwei[21]=8;
	sensor_ptr->denoise.ranwei[22]=7;
	sensor_ptr->denoise.ranwei[23]=6;
	sensor_ptr->denoise.ranwei[24]=5;
	sensor_ptr->denoise.ranwei[25]=4;
	sensor_ptr->denoise.ranwei[26]=3;
	sensor_ptr->denoise.ranwei[27]=2;
	sensor_ptr->denoise.ranwei[28]=1;
	sensor_ptr->denoise.ranwei[29]=1;
	sensor_ptr->denoise.ranwei[30]=1;

	//GrGb
	sensor_ptr->grgb.edge_thr=26;
	sensor_ptr->grgb.diff_thr=80;
	//cfa
	sensor_ptr->cfa.edge_thr=0x1a;
	sensor_ptr->cfa.diff_thr=0x00;
	//cmc
	sensor_ptr->cmc.matrix[0][0]=0;
	sensor_ptr->cmc.matrix[0][1]=0;
	sensor_ptr->cmc.matrix[0][2]=0;
	sensor_ptr->cmc.matrix[0][3]=0;
	sensor_ptr->cmc.matrix[0][4]=0;
	sensor_ptr->cmc.matrix[0][5]=0;
	sensor_ptr->cmc.matrix[0][6]=0;
	sensor_ptr->cmc.matrix[0][7]=0;
	sensor_ptr->cmc.matrix[0][8]=0;
	//Gamma
	sensor_ptr->gamma.axis[0][0]=0;
	sensor_ptr->gamma.axis[1][0]=8;
	sensor_ptr->gamma.axis[2][0]=16;
	sensor_ptr->gamma.axis[3][0]=24;
	sensor_ptr->gamma.axis[4][0]=32;
	sensor_ptr->gamma.axis[5][0]=48;
	sensor_ptr->gamma.axis[6][0]=64;
	sensor_ptr->gamma.axis[7][0]=80;
	sensor_ptr->gamma.axis[8][0]=96;
	sensor_ptr->gamma.axis[9][0]=128;
	sensor_ptr->gamma.axis[10][0]=160;
	sensor_ptr->gamma.axis[11][0]=192;
	sensor_ptr->gamma.axis[12][0]=224;
	sensor_ptr->gamma.axis[13][0]=256;
	sensor_ptr->gamma.axis[14][0]=288;
	sensor_ptr->gamma.axis[15][0]=320;
	sensor_ptr->gamma.axis[16][0]=384;
	sensor_ptr->gamma.axis[17][0]=448;
	sensor_ptr->gamma.axis[18][0]=512;
	sensor_ptr->gamma.axis[19][0]=576;
	sensor_ptr->gamma.axis[20][0]=640;
	sensor_ptr->gamma.axis[21][0]=768;
	sensor_ptr->gamma.axis[22][0]=832;
	sensor_ptr->gamma.axis[23][0]=896;
	sensor_ptr->gamma.axis[24][0]=960;
	sensor_ptr->gamma.axis[25][0]=1023;

	sensor_ptr->gamma.axis[0][1]=0;
	sensor_ptr->gamma.axis[1][1]=5;
	sensor_ptr->gamma.axis[2][1]=9;
	sensor_ptr->gamma.axis[3][1]=14;
	sensor_ptr->gamma.axis[4][1]=19;
	sensor_ptr->gamma.axis[5][1]=28;
	sensor_ptr->gamma.axis[6][1]=37;
	sensor_ptr->gamma.axis[7][1]=46;
	sensor_ptr->gamma.axis[8][1]=54;
	sensor_ptr->gamma.axis[9][1]=71;
	sensor_ptr->gamma.axis[10][1]=85;
	sensor_ptr->gamma.axis[11][1]=99;
	sensor_ptr->gamma.axis[12][1]=111;
	sensor_ptr->gamma.axis[13][1]=121;
	sensor_ptr->gamma.axis[14][1]=131;
	sensor_ptr->gamma.axis[15][1]=140;
	sensor_ptr->gamma.axis[16][1]=155;
	sensor_ptr->gamma.axis[17][1]=169;
	sensor_ptr->gamma.axis[18][1]=180;
	sensor_ptr->gamma.axis[19][1]=191;
	sensor_ptr->gamma.axis[20][1]=201;
	sensor_ptr->gamma.axis[21][1]=219;
	sensor_ptr->gamma.axis[22][1]=228;
	sensor_ptr->gamma.axis[23][1]=237;
	sensor_ptr->gamma.axis[24][1]=246;
	sensor_ptr->gamma.axis[25][1]=255;

	//uv div
	sensor_ptr->uv_div.thrd[0]=252;
	sensor_ptr->uv_div.thrd[1]=250;
	sensor_ptr->uv_div.thrd[2]=248;
	sensor_ptr->uv_div.thrd[3]=246;
	sensor_ptr->uv_div.thrd[4]=244;
	sensor_ptr->uv_div.thrd[5]=242;
	sensor_ptr->uv_div.thrd[6]=240;

	//pref
	sensor_ptr->pref.write_back=0x00;
	sensor_ptr->pref.y_thr=0x04;
	sensor_ptr->pref.u_thr=0x04;
	sensor_ptr->pref.v_thr=0x04;
	//bright
	sensor_ptr->bright.factor[0]=0x40;
	sensor_ptr->bright.factor[1]=0x40;
	sensor_ptr->bright.factor[2]=0x40;
	sensor_ptr->bright.factor[3]=0x40;
	sensor_ptr->bright.factor[4]=0x40;
	sensor_ptr->bright.factor[5]=0x40;
	sensor_ptr->bright.factor[6]=0x40;
	sensor_ptr->bright.factor[7]=0x40;
	sensor_ptr->bright.factor[8]=0x40;
	sensor_ptr->bright.factor[9]=0x40;
	sensor_ptr->bright.factor[10]=0x40;
	sensor_ptr->bright.factor[11]=0x40;
	sensor_ptr->bright.factor[12]=0x40;
	sensor_ptr->bright.factor[13]=0x40;
	sensor_ptr->bright.factor[14]=0x40;
	sensor_ptr->bright.factor[15]=0x40;
	//contrast
	sensor_ptr->contrast.factor[0]=0x40;
	sensor_ptr->contrast.factor[1]=0x40;
	sensor_ptr->contrast.factor[2]=0x40;
	sensor_ptr->contrast.factor[3]=0x40;
	sensor_ptr->contrast.factor[4]=0x40;
	sensor_ptr->contrast.factor[5]=0x40;
	sensor_ptr->contrast.factor[6]=0x40;
	sensor_ptr->contrast.factor[7]=0x40;
	sensor_ptr->contrast.factor[8]=0x40;
	sensor_ptr->contrast.factor[9]=0x40;
	sensor_ptr->contrast.factor[10]=0x40;
	sensor_ptr->contrast.factor[11]=0x40;
	sensor_ptr->contrast.factor[12]=0x40;
	sensor_ptr->contrast.factor[13]=0x40;
	sensor_ptr->contrast.factor[14]=0x40;
	sensor_ptr->contrast.factor[15]=0x40;
	//hist
	sensor_ptr->hist.mode;
	sensor_ptr->hist.low_ratio;
	sensor_ptr->hist.high_ratio;
	//auto contrast
	sensor_ptr->auto_contrast.mode;
	//saturation
	sensor_ptr->saturation.factor[0]=0x40;
	sensor_ptr->saturation.factor[1]=0x40;
	sensor_ptr->saturation.factor[2]=0x40;
	sensor_ptr->saturation.factor[3]=0x40;
	sensor_ptr->saturation.factor[4]=0x40;
	sensor_ptr->saturation.factor[5]=0x40;
	sensor_ptr->saturation.factor[6]=0x40;
	sensor_ptr->saturation.factor[7]=0x40;
	sensor_ptr->saturation.factor[8]=0x40;
	sensor_ptr->saturation.factor[9]=0x40;
	sensor_ptr->saturation.factor[10]=0x40;
	sensor_ptr->saturation.factor[11]=0x40;
	sensor_ptr->saturation.factor[12]=0x40;
	sensor_ptr->saturation.factor[13]=0x40;
	sensor_ptr->saturation.factor[14]=0x40;
	sensor_ptr->saturation.factor[15]=0x40;
	//af win
	sensor_ptr->af.win[0].start_x=0x00;
	sensor_ptr->af.win[0].start_y=0x00;
	sensor_ptr->af.win[0].end_x=0x00;
	sensor_ptr->af.win[0].end_x=0x00;
	sensor_ptr->af.win[1].start_x=0x00;
	sensor_ptr->af.win[1].start_y=0x00;
	sensor_ptr->af.win[1].end_x=0x00;
	sensor_ptr->af.win[1].end_x=0x00;
	sensor_ptr->af.win[2].start_x=0x00;
	sensor_ptr->af.win[2].start_y=0x00;
	sensor_ptr->af.win[2].end_x=0x00;
	sensor_ptr->af.win[2].end_x=0x00;
	sensor_ptr->af.win[3].start_x=0x00;
	sensor_ptr->af.win[3].start_y=0x00;
	sensor_ptr->af.win[3].end_x=0x00;
	sensor_ptr->af.win[3].end_x=0x00;
	sensor_ptr->af.win[4].start_x=0x00;
	sensor_ptr->af.win[4].start_y=0x00;
	sensor_ptr->af.win[4].end_x=0x00;
	sensor_ptr->af.win[4].end_x=0x00;
	sensor_ptr->af.win[5].start_x=0x00;
	sensor_ptr->af.win[5].start_y=0x00;
	sensor_ptr->af.win[5].end_x=0x00;
	sensor_ptr->af.win[5].end_x=0x00;
	sensor_ptr->af.win[6].start_x=0x00;
	sensor_ptr->af.win[6].start_y=0x00;
	sensor_ptr->af.win[6].end_x=0x00;
	sensor_ptr->af.win[6].end_x=0x00;
	sensor_ptr->af.win[7].start_x=0x00;
	sensor_ptr->af.win[7].start_y=0x00;
	sensor_ptr->af.win[7].end_x=0x00;
	sensor_ptr->af.win[7].end_x=0x00;
	sensor_ptr->af.win[8].start_x=0x00;
	sensor_ptr->af.win[8].start_y=0x00;
	sensor_ptr->af.win[8].end_x=0x00;
	sensor_ptr->af.win[8].end_x=0x00;

	//edge
	sensor_ptr->edge.detail_thr=0x03;
	sensor_ptr->edge.smooth_thr=0x05;
	sensor_ptr->edge.strength=24;
	//emboss
	sensor_ptr->emboss.step=0x00;
	//global gain
	sensor_ptr->global.gain=0x40;
	//chn gain
	sensor_ptr->chn.r_gain=0x40;
	sensor_ptr->chn.g_gain=0x40;
	sensor_ptr->chn.b_gain=0x40;
	sensor_ptr->chn.r_offset=0x00;
	sensor_ptr->chn.r_offset=0x00;
	sensor_ptr->chn.r_offset=0x00;

	return rtn;
}


LOCAL uint32_t _ov5640_GetResolutionTrimTab(uint32_t param)
{
	SENSOR_PRINT("0x%x", s_ov5640_Resolution_Trim_Tab);
	return (uint32_t) s_ov5640_Resolution_Trim_Tab;
}
LOCAL uint32_t _ov5640_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov5640_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov5640_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov5640_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov5640_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov5640_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_ov5640_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(20*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		// Reset sensor
		Sensor_Reset(reset_level);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
				  SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR: _ov5640_Power_On(1:on, 0:off): %d \n", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov5640_Identify(uint32_t param)
{
#define ov5640_PID_VALUE    0x56
#define ov5640_PID_ADDR     0x300A
#define ov5640_VER_VALUE    0x40
#define ov5640_VER_ADDR     0x300B

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR:ov5640 mipi raw identify  .\n");

	pid_value = Sensor_ReadReg(ov5640_PID_ADDR);

	if (ov5640_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov5640_VER_ADDR);
		SENSOR_PRINT("SENSOR: ov5640_Identify: PID = %x, VER = %x \n",
			     pid_value, ver_value);
		if (ov5640_VER_VALUE == ver_value) {
			Sensor_InitRawTuneInfo();
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT("SENSOR: this is ov5640 sensor ! \n");
		} else {
			SENSOR_PRINT
			    ("SENSOR: ov5640_Identify this is OV%x%x sensor ! \n",
			     pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("SENSOR:ov5640 identify fail,pid_value=%d .\n",
			     pid_value);
	}
	
	return ret_value;
}

LOCAL uint32_t _ov5640_BeforeSnapshot(uint32_t param)
{
#define Capture_Framerate  150
#define g_Preview_FrameRate 300

	uint8_t ExposureLow, ExposureMid, ExposureHigh;
	uint8_t ret_l, ret_m, ret_h, Gain, Lines_10ms;
	uint16_t ulCapture_Exposure, Preview_Maxlines;
	uint32_t Capture_MaxLines, g_preview_exposure;

	SENSOR_PRINT("SENSOR: set flash on beforeSnapShot, g_flash_mode = %d \n",
	       g_flash_mode_en);
	if (g_flash_mode_en) {
		//Sensor_SetFlash(0x11);  // high light
		Sensor_SetFlash(1);
	}

	if (SENSOR_MODE_PREVIEW_ONE >= param) {
		return SENSOR_SUCCESS;
	}
	Sensor_WriteReg(0x3406, 0x01);
	Sensor_WriteReg(0x3503, 0x07);

	ret_h = ret_m = ret_l = 0;
	g_preview_exposure = 0;
	ret_h = (uint8_t) Sensor_ReadReg(0x3500);
	ret_m = (uint8_t) Sensor_ReadReg(0x3501);
	ret_l = (uint8_t) Sensor_ReadReg(0x3502);
	g_preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

	ret_h = ret_m = ret_l = 0;
	Preview_Maxlines = 0;
	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	Preview_Maxlines = (ret_h << 8) + ret_l;
	//Read back AGC Gain for preview
	Gain = (uint8_t) Sensor_ReadReg(0x350b);
	Sensor_SetMode(param);

	ret_h = ret_m = ret_l = 0;
	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	Capture_MaxLines = (ret_h << 8) + ret_l;

	Lines_10ms = (uint8_t) (Capture_Framerate * Capture_MaxLines / 12000);

	if (Preview_Maxlines == 0) {
		Preview_Maxlines = 1;
	}

	ulCapture_Exposure =
	    (g_preview_exposure * (Capture_Framerate) * (Capture_MaxLines)) /
	    (((Preview_Maxlines) * (g_Preview_FrameRate)));

	ulCapture_Exposure = g_preview_exposure;

	if (Gain > 32) {
		Gain = Gain / 2;
		ulCapture_Exposure = 2 * g_preview_exposure;
	}

	ExposureLow = ((unsigned char)ulCapture_Exposure) << 4;
	ExposureMid = (unsigned char)(ulCapture_Exposure >> 4) & 0xff;
	ExposureHigh = (unsigned char)(ulCapture_Exposure >> 12);

	//m_iWrite0x3502=ExposureLow;
	Sensor_WriteReg(0x3502, ExposureLow);
	//m_iWrite0x3501=ExposureMid;
	Sensor_WriteReg(0x3501, ExposureMid);
	//m_iWrite0x3500=ExposureHigh;
	Sensor_WriteReg(0x3500, ExposureHigh);
	//m_iWrite0x350b=Gain;
	Sensor_WriteReg(0x350b, Gain + 1);
	usleep(100*1000);
	Sensor_WriteReg(0x350b, Gain);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME,
				 (uint32_t) ulCapture_Exposure);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov5640_after_snapshot(uint32_t param)
{
	SENSOR_PRINT("SENSOR: _ov5640_after_snapshot =%d \n", param);
	Sensor_SetMode(param);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov5640_StreamOn(uint32_t param)
{
	SENSOR_PRINT("SENSOR: _ov5640_StreamOn");

	Sensor_WriteReg(0x3008, 0x02);

	return 0;
}

LOCAL uint32_t _ov5640_StreamOff(uint32_t param)
{
	SENSOR_PRINT("SENSOR: _ov5640_StreamOff");

	Sensor_WriteReg(0x3008, 0x42);

	return 0;
}


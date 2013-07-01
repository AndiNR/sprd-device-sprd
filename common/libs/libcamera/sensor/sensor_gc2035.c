/*
 * Copyright (C) 2008 The Android Open Source Project
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
#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"

#define GC2035_I2C_ADDR_W 0x3c 
#define GC2035_I2C_ADDR_R 0x3c 
#define SENSOR_GAIN_SCALE 16

 typedef enum
{
	FLICKER_50HZ = 0,
	FLICKER_60HZ,
	FLICKER_MAX
}FLICKER_E;

static uint32_t set_GC2035_ae_enable(uint32_t enable);
static uint32_t GC2035_PowerOn(uint32_t power_on);
static uint32_t set_preview_mode(uint32_t preview_mode);
static uint32_t GC2035_Identify(uint32_t param);
static uint32_t GC2035_BeforeSnapshot(uint32_t param);
static uint32_t GC2035_After_Snapshot(uint32_t param);
static uint32_t set_brightness(uint32_t level);
static uint32_t set_contrast(uint32_t level);
static uint32_t set_sharpness(uint32_t level);
static uint32_t set_saturation(uint32_t level);
static uint32_t set_image_effect(uint32_t effect_type);
static uint32_t read_ev_value(uint32_t value);
static uint32_t write_ev_value(uint32_t exposure_value);
static uint32_t read_gain_value(uint32_t value);
static uint32_t write_gain_value(uint32_t gain_value);
static uint32_t read_gain_scale(uint32_t value);
static uint32_t set_frame_rate(uint32_t param);
static uint32_t set_GC2035_ev(uint32_t level);
static uint32_t set_GC2035_awb(uint32_t mode);
static uint32_t set_GC2035_anti_flicker(uint32_t mode);
static uint32_t set_GC2035_video_mode(uint32_t mode);
static void GC2035_set_shutter();
static uint32_t set_sensor_flip(uint32_t param);

static SENSOR_REG_T GC2035_YUV_COMMON[]=
{
	{0xfe , 0x80},
	{0xfe , 0x80},
	{0xfe , 0x80},
	{0xfc , 0x06},
	{0xf2 , 0x00},
	{0xf3 , 0x00},
	{0xf4 , 0x00},
	{0xf5 , 0x00},
	{0xf9 , 0xfe},
	{0xfa , 0x00},
	{0xf6 , 0x00},
	{0xf7 , 0x15},

	{0xf8 , 0x83},
	{0xfe , 0x00},
	{0x82 , 0x00},
	{0xb3 , 0x60},
	{0xb4 , 0x40},
	{0xb5 , 0x60},

	{0x03 , 0x02},
	{0x04 , 0xdc},

	/*measure window*/
	{0xfe , 0x00},
	{0xec , 0x06},
	{0xed , 0x06},
	{0xee , 0x62},
	{0xef , 0x92},

	/*analog*/
	{0x0a , 0x00},
	{0x0c , 0x00},
	{0x0d , 0x04},
	{0x0e , 0xc0},
	{0x0f , 0x06},
	{0x10 , 0x58},
	{0x17 , 0x14},
	{0x18 , 0x0e},
	{0x19 , 0x0c},
	{0x1a , 0x01},
	{0x1b , 0x8b},
	{0x1c , 0x05},
	{0x1e , 0x88},
	{0x1f , 0x08},
	{0x20 , 0x05},
	{0x21 , 0x0f},
	{0x22 , 0xd0},
	{0x23 , 0xc3},
	{0x24 , 0x17},

	/*AEC*/
	{0xfe , 0x01},
	{0x11 , 0x20},
	{0x1f , 0xc0},
	{0x20 , 0x60},
	{0x47 , 0x30},
	{0x0b , 0x10},
	{0x13 , 0x75},
	{0xfe , 0x00},

	{0x05 , 0x01},
	{0x06 , 0x25},
	{0x07 , 0x00},
	{0x08 , 0x14},
	{0xfe , 0x01},
	{0x27 , 0x00},
	{0x28 , 0x83},
	{0x29 , 0x04},
	{0x2a , 0x9b},
	{0x2b , 0x04},
	{0x2c , 0x9b},
	{0x2d , 0x05},
	{0x2e , 0xa1},
	{0x2f , 0x07},
	{0x30 , 0x2a},
	{0xfe , 0x00},
	{0xfe , 0x00},
	{0xb6 , 0x03},
	{0xfe , 0x00},

	/*BLK*/
	{0x3f , 0x00},
	{0x40 , 0x77},
	{0x42 , 0x7f},
	{0x43 , 0x30},
	{0x5c , 0x08},
	{0x5e , 0x20},
	{0x5f , 0x20},
	{0x60 , 0x20},
	{0x61 , 0x20},
	{0x62 , 0x20},
	{0x63 , 0x20},
	{0x64 , 0x20},
	{0x65 , 0x20},

	/*block*/
	{0x80 , 0xff},
	{0x81 , 0x26},
	{0x87 , 0x90},
	{0x84 , 0x02},
	{0x86 , 0x02},
	{0x8b , 0xbc},
	{0xb0 , 0x80},
	{0xc0 , 0x40},

	/*lsc*/
	{0xfe , 0x01},
	{0xc2 , 0x38},
	{0xc3 , 0x25},
	{0xc4 , 0x21},
	{0xc8 , 0x19},
	{0xc9 , 0x12},
	{0xca , 0x0e},
	{0xbc , 0x43},
	{0xbd , 0x18},
	{0xbe , 0x1b},
	{0xb6 , 0x40},
	{0xb7 , 0x2e},
	{0xb8 , 0x26},
	{0xc5 , 0x05},
	{0xc6 , 0x03},
	{0xc7 , 0x04},
	{0xcb , 0x00},
	{0xcc , 0x00},
	{0xcd , 0x00},
	{0xbf , 0x14},
	{0xc0 , 0x22},
	{0xc1 , 0x1b},
	{0xb9 , 0x00},
	{0xba , 0x05},
	{0xbb , 0x05},
	{0xaa , 0x35},
	{0xab , 0x33},
	{0xac , 0x33},
	{0xad , 0x25},
	{0xae , 0x22},
	{0xaf , 0x27},
	{0xb0 , 0x1d},
	{0xb1 , 0x20},
	{0xb2 , 0x22},
	{0xb3 , 0x14},
	{0xb4 , 0x15},
	{0xb5 , 0x16},
	{0xd0 , 0x00},
	{0xd2 , 0x07},
	{0xd3 , 0x08},
	{0xd8 , 0x00},
	{0xda , 0x13},
	{0xdb , 0x17},
	{0xdc , 0x00},
	{0xde , 0x0a},
	{0xdf , 0x08},
	{0xd4 , 0x00},
	{0xd6 , 0x00},
	{0xd7 , 0x0c},
	{0xa4 , 0x00},
	{0xa5 , 0x00},
	{0xa6 , 0x00},
	{0xa7 , 0x00},
	{0xa8 , 0x00},
	{0xa9 , 0x00},
	{0xa1 , 0x80},
	{0xa2 , 0x80},

	/*cc*/
	{0xfe , 0x02},
	{0xc0 , 0x01},
	{0xc1 , 0x40},
	{0xc2 , 0xfc},
	{0xc3 , 0x05},
	{0xc4 , 0xec},
	{0xc5 , 0x42},
	{0xc6 , 0xf8},
	{0xc7 , 0x40},
	{0xc8 , 0xf8},
	{0xc9 , 0x06},
	{0xca , 0xfd},
	{0xcb , 0x3e},
	{0xcc , 0xf3},
	{0xcd , 0x36},
	{0xce , 0xf6},
	{0xcf , 0x04},
	{0xe3 , 0x0c},
	{0xe4 , 0x44},
	{0xe5 , 0xe5},
	{0xfe , 0x00},

	/*awb start*/
	{0xfe , 0x01},
	{0x4f , 0x00},
	{0x4d , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0x10},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0x20},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0x30},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0x40},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0x50},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0x60},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0x70},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0x80},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0x90},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0xa0},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0xb0},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0xc0},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4d , 0xd0},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4e , 0x00},
	{0x4f , 0x01},
	/*awb value*/
	{0xfe , 0x01},
	{0x4f , 0x00},
	{0x4d , 0x30},
	{0x4e , 0x00},
	{0x4e , 0x80},
	{0x4e , 0x80},
	{0x4e , 0x02},
	{0x4e , 0x02},
	{0x4d , 0x40},
	{0x4e , 0x00},
	{0x4e , 0x80},
	{0x4e , 0x80},
	{0x4e , 0x02},
	{0x4e , 0x02},
	{0x4e , 0x02},
	{0x4d , 0x53},
	{0x4e , 0x08},
	{0x4e , 0x04},
	{0x4d , 0x62},
	{0x4e , 0x10},
	{0x4d , 0x72},
	{0x4e , 0x20},
	{0x4f , 0x01},
	/*awb*/
	{0xfe , 0x01},
	{0x50 , 0x88},
	{0x52 , 0x40},
	{0x54 , 0x60},
	{0x56 , 0x06},
	{0x57 , 0x20},
	{0x58 , 0x01},
	{0x5b , 0x02},
	{0x61 , 0xaa},
	{0x62 , 0xaa},
	{0x71 , 0x00},
	{0x74 , 0x10},
	{0x77 , 0x08},
	{0x78 , 0xfd},
	{0x86 , 0x30},
	{0x87 , 0x00},
	{0x88 , 0x04},
	{0x8a , 0xc0},
	{0x89 , 0x75},
	{0x84 , 0x08},
	{0x8b , 0x00},
	{0x8d , 0x70},
	{0x8e , 0x70},
	{0x8f , 0xf4},
	{0xfe , 0x00},
	{0x82 , 0x02},

	/*asde*/
	{0xfe , 0x01},
	{0x21 , 0xbf},
	{0xfe , 0x02},
	{0xa4 , 0x00},
	{0xa5 , 0x40},
	{0xa2 , 0xa0},
	{0xa6 , 0x80},
	{0xa7 , 0x80},
	{0xab , 0x31},
	{0xa9 , 0x6f},
	{0xb0 , 0x99},
	{0xb1 , 0x34},
	{0xb3 , 0x80},
	{0xde , 0xb6},
	{0x38 , 0x0f},
	{0x39 , 0x60},
	{0xfe , 0x00},
	{0x81 , 0x26},
	{0xfe , 0x02},
	{0x83 , 0x00},
	{0x84 , 0x45},
	/*YCP*/
	{0xd1 , 0x38},
	{0xd2 , 0x38},
	{0xd3 , 0x40},
	{0xd4 , 0x80},
	{0xd5 , 0x00},
	{0xdc , 0x30},
	{0xdd , 0xb8},
	{0xfe , 0x00},
	/*dndd*/
	{0xfe , 0x02},
	{0x88 , 0x15},
	{0x8c , 0xf6},
	{0x89 , 0x03},
	/*EE */
	{0xfe , 0x02},
	{0x90 , 0x6c},
	{0x97 , 0x45},
	/*RGB Gamma*/
	{0xfe , 0x02},
	{0x15 , 0x0a},
	{0x16 , 0x12},
	{0x17 , 0x19},
	{0x18 , 0x1f},
	{0x19 , 0x2c},
	{0x1a , 0x38},
	{0x1b , 0x42},
	{0x1c , 0x4e},
	{0x1d , 0x63},
	{0x1e , 0x76},
	{0x1f , 0x87},
	{0x20 , 0x96},
	{0x21 , 0xa2},
	{0x22 , 0xb8},
	{0x23 , 0xca},
	{0x24 , 0xd8},
	{0x25 , 0xe3},
	{0x26 , 0xf0},
	{0x27 , 0xf8},
	{0x28 , 0xfd},
	{0x29 , 0xff},

	{0xfe , 0x02},
	{0x2b , 0x00},
	{0x2c , 0x04},
	{0x2d , 0x09},
	{0x2e , 0x18},
	{0x2f , 0x27},
	{0x30 , 0x37},
	{0x31 , 0x49},
	{0x32 , 0x5c},
	{0x33 , 0x7e},
	{0x34 , 0xa0},
	{0x35 , 0xc0},
	{0x36 , 0xe0},
	{0x37 , 0xff},

	{0xfe , 0x00},
	{0x90 , 0x01},
	{0x95 , 0x04},
	{0x96 , 0xb0},
	{0x97 , 0x06},
	{0x98 , 0x40},


	{0xfe , 0x03},
	{0x42 , 0x40},
	{0x43 , 0x06},
	{0x41 , 0x02},
	{0x40 , 0x40},
	{0x17 , 0x00},
	{0xfe , 0x00},

	{0xfe , 0x00},
	{0x82 , 0xfe},
	{0xf2 , 0x70},
	{0xf3 , 0xff},
	{0xf4 , 0x00},
	{0xf5 , 0x30},
};

static SENSOR_REG_T GC2035_YUV_800x600[]=
{
	{0xfe , 0x00},
	{0xfa , 0x00},
	{0x05 , 0x01},
	{0x06 , 0x25},
	{0x07 , 0x00},
	{0x08 , 0x14},
	{0xf7 , 0x15},
	{0xf8 , 0x87},

	{0xc8 , 0x40},
	{0x99 , 0x22},
	{0x9a , 0x06},
	{0x9b , 0x00},
	{0x9c , 0x00},
	{0x9d , 0x00},
	{0x9e , 0x00},
	{0x9f , 0x00},
	{0xa0 , 0x00},
	{0xa1 , 0x00},
	{0xa2  ,0x00},

	{0xfe , 0x03},
	{0x42 , 0x40},
	{0x43 , 0x06},
	{0xfe , 0x00},

	{0x90 , 0x01},
	{0x95 , 0x02},
	{0x96 , 0x58},
	{0x97 , 0x03},
	{0x98 , 0x20},
};

static SENSOR_REG_T GC2035_YUV_1280x960[]=
{
	{0xfe , 0x00},
	{0xfa , 0x00},
	{0x05 , 0x02},
	{0x06 , 0x80},
	{0x07 , 0x00},
	{0x08 , 0x10},
	{0xf7 , 0x05},
	{0xf8 , 0x83},
	{0xc8 , 0x40},

	{0x99 , 0x55},
	{0x9a , 0x06},
	{0x9b , 0x02},
	{0x9c , 0x03},
	{0x9d , 0x04},
	{0x9e , 0x00},
	{0x9f , 0x02},
	{0xa0 , 0x03},
	{0xa1 , 0x04},
	{0xa2 ,0x00},

	{0xfe , 0x03},
	{0x42 , 0x00},
	{0x43 , 0x0a},
	{0xfe , 0x00},

	{0x90 , 0x01},
	{0x95 , 0x03},
	{0x96 , 0xc0},
	{0x97 , 0x05},
	{0x98 , 0x00},
};

static SENSOR_REG_T GC2035_YUV_1600x1200[]=
{
	{0xfe , 0x00},
	{0xfa , 0x10},
	{0x05 , 0x01},
	{0x06 , 0x25},
	{0x07 , 0x00},
	{0x08 , 0x14},
	{0xf7 , 0x15},
	{0xf8 , 0x83},
	{0xc8 , 0x00},

	{0x99 , 0x11},
	{0x9a , 0x06},
	{0x9b , 0x00},
	{0x9c , 0x00},
	{0x9d , 0x00},
	{0x9e , 0x00},
	{0x9f , 0x00},
	{0xa0 , 0x00},
	{0xa1 , 0x00},
	{0xa2 ,0x00},

	{0xfe , 0x03},
	{0x42 , 0x80},
	{0x43 , 0x0c},
	{0xfe , 0x00},

	{0x90 , 0x01},
	{0x95 , 0x04},
	{0x96 , 0xb0},
	{0x97 , 0x06},
	{0x98 , 0x40},
};

static SENSOR_REG_TAB_INFO_T s_GC2035_resolution_Tab_YUV[]=
{
	// COMMON INIT
	{ADDR_AND_LEN_OF_ARRAY(GC2035_YUV_COMMON), 0, 0, 24, SENSOR_IMAGE_FORMAT_YUV422},

	// YUV422 PREVIEW 1
	{ADDR_AND_LEN_OF_ARRAY(GC2035_YUV_800x600), 800, 600, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(GC2035_YUV_1280x960), 1280, 960, 24, SENSOR_IMAGE_FORMAT_YUV422},
	//{ADDR_AND_LEN_OF_ARRAY(GC2035_YUV_1600x1200), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},

	// YUV422 PREVIEW 2
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

#ifdef CONFIG_CAMERA_SENSOR_NEW_FEATURE
LOCAL SENSOR_VIDEO_INFO_T s_GC2035_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};
#endif

static SENSOR_IOCTL_FUNC_TAB_T s_GC2035_ioctl_func_tab = 
{
	// Internal 
	PNULL,
	GC2035_PowerOn,
	PNULL,
	GC2035_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL, //set_sensor_flip
	PNULL,

	// External
	PNULL,//set_GC2035_ae_enable,
	PNULL,
	PNULL,

	set_brightness,
	set_contrast,
	PNULL,//set_sharpness,
	PNULL,//set_saturation,

	PNULL,//set_preview_mode,
	set_image_effect,

	GC2035_BeforeSnapshot,
	GC2035_After_Snapshot,

	PNULL,

	PNULL,//read_ev_value,
	PNULL,//write_ev_value,
	PNULL,//read_gain_value,
	PNULL,//write_gain_value,
	PNULL,//read_gain_scale,
	PNULL,//set_frame_rate,
	PNULL,
	PNULL,
	set_GC2035_awb,
	PNULL,
	PNULL,
	set_GC2035_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,//set_GC2035_anti_flicker,
	set_GC2035_video_mode,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
};

SENSOR_INFO_T g_GC2035_yuv_info =
{
	GC2035_I2C_ADDR_W,				//salve i2c write address
	GC2035_I2C_ADDR_R,				//salve i2c read address

	SENSOR_I2C_FREQ_400,						//bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
							//bit2: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
							//other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N|\
	SENSOR_HW_SIGNAL_VSYNC_N|\
	SENSOR_HW_SIGNAL_HSYNC_P,			//bit0: 0:negative; 1:positive -> polarily of pixel clock
							//bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
							//bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
							//other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL|\
	SENSOR_ENVIROMENT_NIGHT|\
	SENSOR_ENVIROMENT_SUNNY,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL|\
	SENSOR_IMAGE_EFFECT_BLACKWHITE|\
	SENSOR_IMAGE_EFFECT_RED|\
	SENSOR_IMAGE_EFFECT_GREEN|\
	SENSOR_IMAGE_EFFECT_BLUE|\
	SENSOR_IMAGE_EFFECT_YELLOW|\
	SENSOR_IMAGE_EFFECT_NEGATIVE|\
	SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,						//bit[0:7]: count of step in brightness, contrast, sharpness, saturation
							//bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,				//reset pulse level
	20,						//reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,				//power donw pulse level

	2,						//count of identify code
	{{0xf0, 0x20},					//supply two code to identify sensor.
	{0xf1, 0x35}},					//for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,				//voltage of avdd	

	1600,						//max width of source image
	1200,						//max height of source image
	"GC2035",					//name of sensor

	SENSOR_IMAGE_FORMAT_YUV422,			//define in SENSOR_IMAGE_FORMAT_E enum,
							//if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T
	SENSOR_IMAGE_PATTERN_YUV422_YUYV,		//pattern of input image form sensor;

	s_GC2035_resolution_Tab_YUV,			//point to resolution table information structure
	&s_GC2035_ioctl_func_tab,				//point to ioctl function table

	PNULL,						//information and table about Rawrgb sensor
	PNULL,						//extend information about sensor	
	SENSOR_AVDD_1800MV,				//iovdd
	SENSOR_AVDD_1800MV,				//dvdd
	1,						//skip frame num before preview 
	1,						//skip frame num before capture
	0,						//deci frame num during preview	
	0,						//deci frame num during video preview
	0,						//threshold enable(only analog TV)
	0,						//atv output mode 0 fix mode 1 auto mode	
	0,						//atv output start postion
	0,						//atv output end postion
	0,
#ifdef CONFIG_CAMERA_SENSOR_NEW_FEATURE
	{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1},
	s_GC2035_video_info,
	4,						//skip frame num while change setting
#else
	{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1}
#endif
};

static void GC2035_WriteReg( uint8_t  subaddr, uint8_t data )
{
	Sensor_WriteReg_8bits(subaddr, data);
}

static uint8_t GC2035_ReadReg( uint8_t subaddr)
{
	uint8_t value = 0;

	value = Sensor_ReadReg( subaddr);

	return value;
}

static uint32_t GC2035_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_GC2035_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_GC2035_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_GC2035_yuv_info.iovdd_val;
	BOOLEAN power_down = g_GC2035_yuv_info.power_down_level;
	BOOLEAN reset_level = g_GC2035_yuv_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(10*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		Sensor_Reset(reset_level);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
				SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return (uint32_t)SENSOR_SUCCESS;
}

static uint32_t GC2035_Identify(uint32_t param)
{
#define GC2035_PID_ADDR1 0xf0
#define GC2035_PID_ADDR2 0xf1
#define GC2035_SENSOR_ID 0x2035

	uint16_t sensor_id = 0;
	uint8_t pid_value = 0;
	uint8_t ver_value = 0;
	int i;
	BOOLEAN ret_value = SENSOR_FAIL;

	for (i=0; i<3; i++) {
		sensor_id = Sensor_ReadReg(GC2035_PID_ADDR1) << 8;
		sensor_id |= Sensor_ReadReg(GC2035_PID_ADDR2);
		SENSOR_PRINT("%s sensor_id is %x\n", __func__, sensor_id);
		
		if (sensor_id == GC2035_SENSOR_ID) {
			SENSOR_PRINT("the main sensor is GC2035\n");
			return SENSOR_SUCCESS;
		}
	}

	return ret_value;
}

static uint32_t set_GC2035_ae_enable(uint32_t enable)
{
	uint32_t temp_AE_reg =0;

	if (enable == 1) {
		temp_AE_reg = Sensor_ReadReg(0xb6);
		Sensor_WriteReg(0xb6, temp_AE_reg| 0x01);
	} else {
		temp_AE_reg = Sensor_ReadReg(0xb6);
		Sensor_WriteReg(0xb6, temp_AE_reg&~0x01);
	}

	return 0;
}


static void GC2035_set_shutter()
{
	uint32_t shutter = 0 ;

	Sensor_WriteReg(0xfe,0x00);
	Sensor_WriteReg(0xb6,0x00);
	shutter = (Sensor_ReadReg(0x03)<<8 )|( Sensor_ReadReg(0x04));

	shutter = shutter /2 ;

	if  (shutter < 1)
		shutter = 1;
	
	Sensor_WriteReg(0x03, (shutter >> 8)&0xff);
	Sensor_WriteReg(0x04, shutter&0xff);
}

static SENSOR_REG_T GC2035_brightness_tab[][4]=
{
	{{0xfe , 0x02},{0xd5 , 0xc0},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0xe0},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0xf0},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0x00},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0x10},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0x20},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0x30},{0xfe , 0x00},{0xff , 0xff}}
};

static uint32_t set_brightness(uint32_t level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC2035_brightness_tab[level];

	if (level > 6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++)
		GC2035_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);

	return 0;
}


static SENSOR_REG_T GC2035_ev_tab[][4]=
{
	{{0xfe , 0x01},{0x13 , 0x50},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0x60},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0x70},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0x75},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0x80},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0x90},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0xa0},{0xfe , 0x00},{0xff , 0xff}}
};

static uint32_t set_GC2035_ev(uint32_t level)
{
	uint16_t i; 
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC2035_ev_tab[level];

	if (level > 6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) ||(0xFF != sensor_reg_ptr[i].reg_value) ; i++)
		GC2035_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);

	return 0;
}

static uint32_t set_GC2035_anti_flicker(uint32_t param )
{
	switch (param) {
	case FLICKER_50HZ:
		GC2035_WriteReg(0x05 , 0x01);
		GC2035_WriteReg(0x06 , 0x25);
		GC2035_WriteReg(0x07 , 0x00);
		GC2035_WriteReg(0x08 , 0x14);
		GC2035_WriteReg(0xfe , 0x01);
		GC2035_WriteReg(0x27 , 0x00);
		GC2035_WriteReg(0x28 , 0x83);
		GC2035_WriteReg(0x29 , 0x04);
		GC2035_WriteReg(0x2a , 0x9b);
		GC2035_WriteReg(0x2b , 0x04);
		GC2035_WriteReg(0x2c , 0x9b);
		GC2035_WriteReg(0x2d , 0x05);
		GC2035_WriteReg(0x2e , 0xa1);
		GC2035_WriteReg(0x2f , 0x07);
		GC2035_WriteReg(0x30 , 0x2a);
		GC2035_WriteReg(0xfe , 0x00);
		break;
	case FLICKER_60HZ:
		GC2035_WriteReg(0x05,0x01);
		GC2035_WriteReg(0x06,0x08);
		GC2035_WriteReg(0x07,0x00);
		GC2035_WriteReg(0x08,0x14);
		GC2035_WriteReg(0xfe,0x01);
		GC2035_WriteReg(0x27,0x00);
		GC2035_WriteReg(0x28,0x70);
		GC2035_WriteReg(0x29,0x04);
		GC2035_WriteReg(0x2a,0xd0);
		GC2035_WriteReg(0x2b,0x04);
		GC2035_WriteReg(0x2c,0xd0);
		GC2035_WriteReg(0x2d,0x05);
		GC2035_WriteReg(0x2e,0xb0);
		GC2035_WriteReg(0x2f,0x07);
		GC2035_WriteReg(0x30,0x00);
		GC2035_WriteReg(0xfe,0x00);
		break;
	default:
		break;

	}

	return 0;
}

static uint32_t set_GC2035_video_mode(uint32_t mode)
{
	return 0;
}

static SENSOR_REG_T GC2035_awb_tab[][6]=
{
	//Auto
	{
		{0xfe , 0x00},
		{0xb3 , 0x60},
		{0xb4 , 0x40},
		{0xb5 , 0x60},
		{0x82 , 0xfe},
		{0xff , 0xff}
	},

	//Office
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0x50},
		{0xb4 , 0x40},
		{0xb5 , 0xa8},
		{0xff , 0xff}
	},

	//U30  //not use
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0x50},
		{0xb4 , 0x40},
		{0xb5 , 0xa8},
		{0xff , 0xff}
	},

	//CWF  //not use
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0x50},
		{0xb4 , 0x40},
		{0xb5 , 0xa8},
		{0xff , 0xff}
	},

	//HOME
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0xa0},
		{0xb4 , 0x45},
		{0xb5 , 0x40},
		{0xff , 0xff}
	},

	//SUN:
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0x78},
		{0xb4 , 0x40},
		{0xb5 , 0x50},
		{0xff , 0xff}
	},

	//CLOUD:
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0x58},
		{0xb4 , 0x40},
		{0xb5 , 0x50},
		{0xff , 0xff}
	}
};

static uint32_t set_GC2035_awb(uint32_t mode)
{
	uint8_t awb_en_value;
	uint16_t i;

	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC2035_awb_tab[mode];

	if (mode > 6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value); i++)
		GC2035_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);

	return 0;
}

static SENSOR_REG_T GC2035_contrast_tab[][4]=
{
	{{0xfe,0x02}, {0xd3,0x30}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x34}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x38}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x40}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x44}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x48}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x50}, {0xfe,0x00}, {0xff,0xff}}
};

static uint32_t set_contrast(uint32_t level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr;

	sensor_reg_ptr = (SENSOR_REG_T*)GC2035_contrast_tab[level];

	if (level > 6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++)
		GC2035_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);

	return 0;
}

static uint32_t set_sharpness(uint32_t level)
{
	return 0;
}

static uint32_t set_saturation(uint32_t level)
{
	return 0;
}

static uint32_t set_preview_mode(uint32_t preview_mode)
{
	SENSOR_PRINT("set_preview_mode: preview_mode = %d\n", preview_mode);

	set_GC2035_anti_flicker(0);
	
	switch (preview_mode) {
	case DCAMERA_ENVIRONMENT_NORMAL: 
		{
		//YCP_saturation
		GC2035_WriteReg(0xfe , 0x02);
		GC2035_WriteReg(0xd1 , 0x38);
		GC2035_WriteReg(0xd2 , 0x38);

		GC2035_WriteReg(0xfe , 0x01);
		GC2035_WriteReg(0x3e , 0x40);
		GC2035_WriteReg(0xfe , 0x00);
		SENSOR_PRINT("set_preview_mode: DCAMERA_ENVIRONMENT_NORMAL\n");
		break;
		}
	case 1://DCAMERA_ENVIRONMENT_NIGHT://1
		{
		//YCP_saturation
		GC2035_WriteReg(0xfe , 0x02);
		GC2035_WriteReg(0xd1 , 0x38);
		GC2035_WriteReg(0xd2 , 0x38);

		GC2035_WriteReg(0xfe , 0x01);
		GC2035_WriteReg(0x3e , 0x60);
		GC2035_WriteReg(0xfe , 0x00);
		SENSOR_PRINT("set_preview_mode: DCAMERA_ENVIRONMENT_NIGHT\n");
		break;
		}
	case 3://SENSOR_ENVIROMENT_PORTRAIT://3
		{
		//YCP_saturation
		GC2035_WriteReg(0xfe , 0x02);
		GC2035_WriteReg(0xd1 , 0x34);
		GC2035_WriteReg(0xd2 , 0x34);

		GC2035_WriteReg(0xfe , 0x01);
		GC2035_WriteReg(0x3e , 0x40);
		GC2035_WriteReg(0xfe , 0x00);
		SENSOR_PRINT("set_preview_mode: SENSOR_ENVIROMENT_PORTRAIT\n");
		break;
		}
	case 4://SENSOR_ENVIROMENT_LANDSCAPE://4
		{
		//nightmode disable
		GC2035_WriteReg(0xfe , 0x02);
		GC2035_WriteReg(0xd1 , 0x4c);
		GC2035_WriteReg(0xd2 , 0x4c);

		GC2035_WriteReg(0xfe , 0x01);
		GC2035_WriteReg(0x3e , 0x40);
		GC2035_WriteReg(0xfe , 0x00);
		SENSOR_PRINT("set_preview_mode: SENSOR_ENVIROMENT_LANDSCAPE\n");
		break;
		}
	case 2://SENSOR_ENVIROMENT_SPORTS://2
		{
		//nightmode disable
		//YCP_saturation
		GC2035_WriteReg(0xfe , 0x02);
		GC2035_WriteReg(0xd1 , 0x40);
		GC2035_WriteReg(0xd2 , 0x40);

		GC2035_WriteReg(0xfe , 0x01);
		GC2035_WriteReg(0x3e , 0x40);
		GC2035_WriteReg(0xfe , 0x00);
		SENSOR_PRINT("set_preview_mode: SENSOR_ENVIROMENT_SPORTS\n");
		break;
		}
	default:
		break;

	}

	return 0;
}

static SENSOR_REG_T GC2035_image_effect_tab[][2] =
{
	//Normal
	{{0x83 , 0xe0},{0xff , 0xff}},
	//BLACK&WHITE
	{{0x83 , 0x12},{0xff , 0xff}},
	//RED
	{{0x83 , 0x12},{0xff , 0xff}},
	//GREEN
	{{0x83 , 0x52},{0xff , 0xff}},
	//BLUE
	{{0x83 , 0x62},{0xff , 0xff}},
	//YELLOW
	{{0x83 , 0x12},{0xff , 0xff}},
	//NEGATIVE
	{{0x83 , 0x01},{0xff , 0xff}},
	//SEPIA
	{{0x83 , 0x82},{0xff , 0xff}}
};

static uint32_t set_image_effect(uint32_t effect_type)
{
	uint16_t i;

	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC2035_image_effect_tab[effect_type];

	if (effect_type > 7)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++)
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);

	return 0;
}

static uint32_t GC2035_After_Snapshot(uint32_t param)
{
	SENSOR_PRINT("GC2035_After_Snapshot param = %x \n",param);

	Sensor_SetMode(param);

	return 0;
}

static uint32_t GC2035_BeforeSnapshot(uint32_t sensor_snapshot_mode)
{
	Sensor_SetMode(sensor_snapshot_mode);

	switch (sensor_snapshot_mode) {
	case SENSOR_MODE_PREVIEW_ONE:
		SENSOR_PRINT("Capture VGA Size");
		break;
	case SENSOR_MODE_SNAPSHOT_ONE_FIRST:
	case SENSOR_MODE_SNAPSHOT_ONE_SECOND:
		SENSOR_PRINT("Capture 1.3M&2M Size");
		break;
	default:
		break;
	}

	SENSOR_PRINT("SENSOR_GC2035: Before Snapshot");

	return 0;

}

static uint32_t read_ev_value(uint32_t value)
{
	return 0;
}

static uint32_t write_ev_value(uint32_t exposure_value)
{
	return 0;	
}

static uint32_t read_gain_value(uint32_t value)
{
	return 0;
}

static uint32_t write_gain_value(uint32_t gain_value)
{
	return 0;
}

static uint32_t read_gain_scale(uint32_t value)
{
	return SENSOR_GAIN_SCALE;
}


static uint32_t set_frame_rate(uint32_t param)
{
	return 0;
}
static uint32_t set_sensor_flip(uint32_t param)
{
	return 0;
}

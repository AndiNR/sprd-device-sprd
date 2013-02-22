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
#ifndef _SPRD_HW_SCALE_ROTATE_H_
#define _SPRD_HW_SCALE_ROTATE_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
	HW_ROTATION_0 = 0,
	HW_ROTATION_90,
	HW_ROTATION_180,
	HW_ROTATION_270,
	HW_ROTATION_MIRROR,
	HW_ROTATION_ANGLE_MAX
}HW_ROTATION_MODE_E;

typedef enum {
	HW_ROTATION_DATA_YUV422 = 0,
	HW_ROTATION_DATA_YUV420,
	HW_ROTATION_DATA_YUV400,
	HW_ROTATION_DATA_RGB888,
	HW_ROTATION_DATA_RGB666,
	HW_ROTATION_DATA_RGB565,
	HW_ROTATION_DATA_RGB555,
	HW_ROTATION_DATA_FMT_MAX
} HW_ROTATION_DATA_FORMAT_E;

typedef enum  {
	HW_SCALE_DATA_YUV422 = 0,
	HW_SCALE_DATA_YUV420,
	HW_SCALE_DATA_YUV400,
	HW_SCALE_DATA_YUV420_3FRAME,
	HW_SCALE_DATA_RGB565,
	HW_SCALE_DATA_RGB888,
	HW_SCALE_DATA_FMT_MAX
}HW_SCALE_DATA_FORMAT_E;

struct sprd_rect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
};

int camera_rotation_copy_data(uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr);

int camera_rotation_copy_data_from_virtual(uint32_t width, uint32_t height, uint32_t in_virtual_addr, uint32_t out_addr);

int camera_rotation(HW_ROTATION_DATA_FORMAT_E rot_format, int degree, uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr);

int do_scaling_and_rotaion(HW_SCALE_DATA_FORMAT_E output_fmt,
	uint32_t output_width, uint32_t output_height,
	uint32_t output_yaddr,uint32_t output_uvaddr,
	HW_SCALE_DATA_FORMAT_E input_fmt,uint32_t input_uv_endian,
	uint32_t input_width,uint32_t input_height,
	uint32_t input_yaddr, uint32_t intput_uvaddr,
	struct sprd_rect *trim_rect, HW_ROTATION_MODE_E rotation, uint32_t tmp_addr);

#ifdef __cplusplus
}
#endif

#endif

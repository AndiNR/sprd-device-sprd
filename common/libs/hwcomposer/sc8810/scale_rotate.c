/*
 * Copyright (C) 2010 The Android Open Source Project
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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <cutils/log.h>
#include "sprd_scale.h"
#include "sprd_rotation.h"
#include "scale_rotate.h"


static ROTATION_DATA_FORMAT_E rotation_data_format_convertion(HW_ROTATION_DATA_FORMAT_E data_format)
{
	ROTATION_DATA_FORMAT_E result_format = ROTATION_MAX;

	switch(data_format)
	{
		case HW_ROTATION_DATA_YUV422:
			result_format = ROTATION_YUV422;
			break;
		case HW_ROTATION_DATA_YUV420:
			result_format = ROTATION_YUV420;
			break;
		case HW_ROTATION_DATA_YUV400:
			result_format = ROTATION_YUV400;
			break;
		case HW_ROTATION_DATA_RGB888:
			result_format = ROTATION_RGB888;
			break;
		case HW_ROTATION_DATA_RGB666:
			result_format = ROTATION_RGB666;
			break;
		case HW_ROTATION_DATA_RGB565:
			result_format = ROTATION_RGB565;
			break;
		case HW_ROTATION_DATA_RGB555:
			result_format = ROTATION_RGB555;
			break;
		default:
			result_format = ROTATION_MAX;
			break;
	}
	return result_format;
}

static ROTATION_DIR_E rotation_degree_convertion(int degree)
{
	ROTATION_DIR_E result_degree = ROTATION_DIR_MAX;

	switch(degree)
	{
		case -1:
			result_degree = ROTATION_MIRROR;
			break;
		case 90:
			result_degree = ROTATION_90;
			break;
		case 180:
			result_degree = ROTATION_180;
			break;
		case 270:
			result_degree = ROTATION_270;
			break;
		default:
			result_degree = ROTATION_DIR_MAX;
			break;
	}
	return result_degree;
}


int camera_rotation_copy_data(uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr)
{
	int fd = -1;
	ROTATION_PARAM_T rot_params;

	rot_params.data_format = ROTATION_RGB888;
	rot_params.img_size.w = width;
	rot_params.img_size.h = height;
	rot_params.src_addr.y_addr = in_addr;
	rot_params.src_addr.uv_addr = rot_params.src_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.src_addr.v_addr = rot_params.src_addr.uv_addr;
	rot_params.dst_addr.y_addr = out_addr;
	rot_params.dst_addr.uv_addr = rot_params.dst_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.dst_addr.v_addr = rot_params.dst_addr.uv_addr;

	fd = open("/dev/sprd_rotation", O_RDWR /* required */, 0);
	if (-1 == fd)
	{
		ALOGE("Fail to open rotation device.");
		return -1;
	}

	//done
	if (-1 == ioctl(fd, SPRD_ROTATION_DATA_COPY, &rot_params))
	{
		ALOGE("Fail to SC8800G_ROTATION_DATA_COPY");
		return -1;
	}

	if(-1 == close(fd))
	{
		ALOGE("Fail to close rotation device.");
		return -1;
	}

	fd = -1;
	return 0;
}

int camera_rotation_copy_data_from_virtual(uint32_t width, uint32_t height, uint32_t in_virtual_addr, uint32_t out_addr)
{
	int fd = -1;
	ROTATION_PARAM_T rot_params;

	rot_params.data_format = ROTATION_RGB888;
	rot_params.img_size.w = width;
	rot_params.img_size.h = height;
	rot_params.src_addr.y_addr = in_virtual_addr;
	rot_params.src_addr.uv_addr = rot_params.src_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.src_addr.v_addr = rot_params.src_addr.uv_addr;
	rot_params.dst_addr.y_addr = out_addr;
	rot_params.dst_addr.uv_addr = rot_params.dst_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.dst_addr.v_addr = rot_params.dst_addr.uv_addr;

	fd = open("/dev/sprd_rotation", O_RDWR /* required */, 0);
	if (-1 == fd)
	{
		ALOGE("Fail to open rotation device %s.", __FILE__);
		return -1;
	}

	//done
	if (-1 == ioctl(fd, SPRD_ROTATION_DATA_COPY_FROM_V_TO_P, &rot_params))
	{
		ALOGE("Fail to SC8800G_ROTATION_DATA_COPY %s", __FILE__);
		return -1;
	}

	if(-1 == close(fd))
	{
		ALOGE("Fail to close rotation device. %s", __FILE__);
		return -1;
	}
	fd = -1;
	return 0;
}

int camera_rotation(HW_ROTATION_DATA_FORMAT_E rot_format, int degree, uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr)
{
	int fd = -1;
	int ret = 0;
	ROTATION_PARAM_T rot_params;

	rot_params.data_format = rotation_data_format_convertion(rot_format);
	rot_params.rotation_dir = rotation_degree_convertion(degree);
	rot_params.img_size.w = width;
	rot_params.img_size.h = height;
	rot_params.src_addr.y_addr = in_addr;
	rot_params.src_addr.uv_addr = rot_params.src_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.src_addr.v_addr = rot_params.src_addr.uv_addr;
	rot_params.dst_addr.y_addr = out_addr;
	rot_params.dst_addr.uv_addr = rot_params.dst_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.dst_addr.v_addr = rot_params.dst_addr.uv_addr;

	fd = open("/dev/sprd_rotation", O_RDWR /* required */, 0);
	if (-1 == fd)
	{
		ALOGE("Fail to open rotation device.");
		return -1;
	}

	//done
	if (-1 == ioctl(fd, SPRD_ROTATION_DONE, &rot_params))
	{
		ALOGE("Fail to SC8800G_ROTATION_DONE");
		ret = -1;
	}

	if(-1 == close(fd))
	{
		ALOGE("Fail to close rotation device.");
		return -1;
	}
	fd = -1;
	return ret;
}

int do_scaling_and_rotaion(int fd, HW_SCALE_DATA_FORMAT_E output_fmt,
	uint32_t output_width, uint32_t output_height,
	uint32_t output_yaddr,uint32_t output_uvaddr,
	HW_SCALE_DATA_FORMAT_E input_fmt,uint32_t input_uv_endian,
	uint32_t input_width,uint32_t input_height,
	uint32_t input_yaddr, uint32_t intput_uvaddr,
	struct sprd_rect *trim_rect, HW_ROTATION_MODE_E rotation, uint32_t tmp_addr)
{
	SCALE_CONFIG_T scale_config;
	SCALE_SIZE_T scale_size;
	SCALE_RECT_T scale_rect;
	SCALE_ADDRESS_T scale_address;
	HW_SCALE_DATA_FORMAT_E data_format;
	SCALE_MODE_E scale_mode;
	uint32_t enable = 0, mode;
	uint32_t slice_height = 0;
	ISP_ENDIAN_T in_endian;
	ISP_ENDIAN_T out_endian;

	//set mode
	scale_config.id = SCALE_PATH_MODE;
	scale_mode = SCALE_MODE_SCALE;
	scale_config.param = &scale_mode;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
                return -1;
	}

	//set input data format
	scale_config.id = SCALE_PATH_INPUT_FORMAT;
	data_format = input_fmt;
	scale_config.param = &data_format;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set output data format
	scale_config.id = SCALE_PATH_OUTPUT_FORMAT;
	data_format = output_fmt;
	scale_config.param = &data_format;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set input size
	scale_config.id = SCALE_PATH_INPUT_SIZE;
	scale_size.w = input_width;// trim_rect->w;
	scale_size.h = input_height;//trim_rect->h;
	scale_config.param = &scale_size;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set output size
	scale_config.id = SCALE_PATH_OUTPUT_SIZE;
	scale_size.w = output_width;
	scale_size.h = output_height;
	scale_config.param = &scale_size;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set input size
	scale_config.id = SCALE_PATH_INPUT_RECT;
	scale_rect.x = trim_rect->x;
	scale_rect.y = trim_rect->y;
	scale_rect.w = trim_rect->w;
	scale_rect.h = trim_rect->h;
	scale_config.param = &scale_rect;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set input address
	scale_config.id = SCALE_PATH_INPUT_ADDR;
	scale_address.yaddr = input_yaddr;
	scale_address.uaddr = intput_uvaddr;
	if(SCALE_DATA_YUV420_3FRAME == input_fmt)
		scale_address.vaddr = scale_address.uaddr + input_width*input_height/4;//todo
	else
		scale_address.vaddr = scale_address.uaddr;//todo
	scale_config.param = &scale_address;
	ALOGV("scale input y addr:0x%x,uv addr:0x%x.",scale_address.yaddr,scale_address.uaddr);
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set output address
	scale_config.id = SCALE_PATH_OUTPUT_ADDR;
	scale_address.yaddr = output_yaddr;
	scale_address.uaddr = output_uvaddr;//output_addr + output_width * output_height;
	scale_address.vaddr = scale_address.uaddr;//todo
	scale_config.param = &scale_address;
	ALOGV("scale out  y addr:0x%x,uv addr:0x%x.",scale_address.yaddr,scale_address.uaddr);;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set input endian
	scale_config.id = SCALE_PATH_INPUT_ENDIAN;
	in_endian.endian_y = 1;
	in_endian.endian_uv = input_uv_endian;//1;
	scale_config.param = &in_endian;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}
	//set output endian
	scale_config.id = SCALE_PATH_OUTPUT_ENDIAN;
	out_endian.endian_y = 1;
	out_endian.endian_uv = 1;
	scale_config.param = &out_endian;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;;
	}

	//set rotation mode
	scale_config.id = SCALE_PATH_ROT_MODE;
	uint32_t scaling_mode = rotation;
	scale_config.param = &scaling_mode;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		return -1;
	}

	//done
	if (-1 == ioctl(fd, SCALE_IOC_DONE, 0))
	{
		ALOGE("Fail to SCALE_IOC_DONE");
		return -1;
	}

	return 0;
}


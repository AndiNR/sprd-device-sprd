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
#include "sprd_rot_k.h"
#include "scale_rotate.h"
#include "img_scale_u.h"
#include "cmr_common.h"


#define HWCOMPOSER_EXIT_IF_ERR(n)                      \
	do {                                                                 \
		if (n) {                                           \
			ALOGE("hwcomposer  rotate or scale  error  . Line:%d ", __LINE__);\
			goto exit;                   \
		}                                                    \
	} while(0)

static ROT_DATA_FORMAT_E rotation_data_format_convertion(HW_ROTATION_DATA_FORMAT_E data_format)
{
	ROT_DATA_FORMAT_E result_format = ROT_FMT_MAX;

	switch(data_format)
	{
		case HW_ROTATION_DATA_YUV422:
			result_format = ROT_YUV422;
			break;
		case HW_ROTATION_DATA_YUV420:
			result_format = ROT_YUV420;
			break;
		case HW_ROTATION_DATA_YUV400:
			result_format = ROT_YUV420;
			break;
		case HW_ROTATION_DATA_RGB888:
			result_format = ROT_RGB888;
			break;
		case HW_ROTATION_DATA_RGB666:
			result_format = ROT_RGB666;
			break;
		case HW_ROTATION_DATA_RGB565:
			result_format = ROT_RGB565;
			break;
		case HW_ROTATION_DATA_RGB555:
			result_format = ROT_RGB555;
			break;
		default:
			result_format = ROT_FMT_MAX;
			break;
	}
	return result_format;
}

static ROT_ANGLE_E rotation_degree_convertion(int degree)
{
	ROT_ANGLE_E result_degree = ROT_ANGLE_MAX;

	switch(degree)
	{
		case -1:
			result_degree = ROT_MIRROR;
			break;
		case 90:
			result_degree = ROT_90;
			break;
		case 180:
			result_degree = ROT_180;
			break;
		case 270:
			result_degree = ROT_270;
			break;
		default:
			result_degree = ROT_ANGLE_MAX;
			break;
	}
	return result_degree;
}

static ROT_ANGLE_E rotation_angle_convertion(HW_ROTATION_MODE_E rot)
{
	ROT_ANGLE_E result_degree = ROT_ANGLE_MAX;

	switch(rot)
	{
		case HW_ROTATION_90:
			result_degree = ROT_90;
			break;
		case HW_ROTATION_180:
			result_degree = ROT_180;
			break;
		case HW_ROTATION_270:
			result_degree = ROT_270;
			break;
		case HW_ROTATION_MIRROR:
			result_degree = ROT_MIRROR;
			break;
		default:
			result_degree = ROT_ANGLE_MAX;
			break;
	}
	return result_degree;
}

int camera_rotation_copy_data(uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr)
{
	int ret = 0, rotate_fd = -1;
	struct _rot_cfg_tag      rot_cfg;

	rotate_fd = open("/dev/sprd_rotation", O_RDWR, 0);
	if (-1 == rotate_fd) {
		ALOGE("camera_rotation_copy_data fail : open rotation device. Line:%d ", __LINE__);
		return -1;
	}

	rot_cfg.format = ROT_RGB888;
	rot_cfg.img_size.w = width;
	rot_cfg.img_size.h = height;
	rot_cfg.src_addr.y_addr = in_addr;
	rot_cfg.src_addr.u_addr = rot_cfg.src_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.src_addr.v_addr = rot_cfg.src_addr.u_addr;
	rot_cfg.dst_addr.y_addr = out_addr;
	rot_cfg.dst_addr.u_addr = rot_cfg.dst_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.dst_addr.v_addr = rot_cfg.dst_addr.u_addr;

	ret = ioctl(rotate_fd, ROT_IO_DATA_COPY, &rot_cfg);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret =  close(rotate_fd);
	if (ret){
		ALOGE("camera_rotation_copy_data fail :close rot_fd 1 . Line:%d ", __LINE__);
		return -1;
	}

exit:
	if (ret) {
		ret =  close(rotate_fd);
		if (ret){
			ALOGE("camera_rotation_copy_data fail :close rot_fd 2 . Line:%d ", __LINE__);
		}
		return -1;
	}else{
		return 0;
	}
}

int camera_rotation_copy_data_from_virtual(uint32_t width, uint32_t height, uint32_t in_virtual_addr, uint32_t out_addr)
{
	int ret = 0, rotate_fd = -1;
	struct _rot_cfg_tag      rot_cfg;

	rotate_fd = open("/dev/sprd_rotation", O_RDWR, 0);
	if (-1 == rotate_fd) {
		ALOGE("Camera_rotation copy from virtual fail : open rotation device. Line:%d ", __LINE__);
		return -1;
	}

	rot_cfg.format = ROT_RGB888;
	rot_cfg.img_size.w = width;
	rot_cfg.img_size.h = height;
	rot_cfg.src_addr.y_addr = in_virtual_addr;
	rot_cfg.src_addr.u_addr = rot_cfg.src_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.src_addr.v_addr = rot_cfg.src_addr.u_addr;
	rot_cfg.dst_addr.y_addr = out_addr;
	rot_cfg.dst_addr.u_addr = rot_cfg.dst_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.dst_addr.v_addr = rot_cfg.dst_addr.u_addr;

	ret = ioctl(rotate_fd, ROT_IO_DATA_COPY_FROM_VIRTUAL, &rot_cfg);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret =  close(rotate_fd);
	if (ret){
		ALOGE("Camera_rotation copy from virtual fail :close rot_fd 1. Line:%d ", __LINE__);
		return -1;
	}

exit:
	if (ret) {
		ret =  close(rotate_fd);
		if (ret){
			ALOGE("Camera_rotation copy from virtual fail :close rot_fd 2 . Line:%d ", __LINE__);
		}
		return -1;
	}else{
		return 0;
	}
}

int camera_rotation(HW_ROTATION_DATA_FORMAT_E rot_format, int degree, uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr)
{
	int ret = 0, rotate_fd = -1;
	uint32_t param = 0;
	struct _rot_cfg_tag rot_cfg;

	rotate_fd = open("/dev/sprd_rotation", O_RDWR, 0);
	if (-1 == rotate_fd) {
		ALOGE("Camera_rotation fail : open rotation device. Line:%d ", __LINE__);
		return -1;
	}

	rot_cfg.format = rotation_data_format_convertion( rot_format);
	rot_cfg.angle =  rotation_degree_convertion(degree);
	rot_cfg.img_size.w = width;
	rot_cfg.img_size.h = height;
	rot_cfg.src_addr.y_addr = in_addr;
	rot_cfg.src_addr.u_addr = rot_cfg.src_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.src_addr.v_addr = rot_cfg.src_addr.u_addr;
	rot_cfg.dst_addr.y_addr = out_addr;
	rot_cfg.dst_addr.u_addr = rot_cfg.dst_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.dst_addr.v_addr = rot_cfg.dst_addr.u_addr;

	ret = ioctl(rotate_fd, ROT_IO_CFG, &rot_cfg);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(rotate_fd, ROT_IO_START, 1);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(rotate_fd, ROT_IO_IS_DONE, &param);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret =  close(rotate_fd);
	if (ret){
		ALOGE("Camera_rotation fail : close rot_fd 1. Line:%d ", __LINE__);
		return -1;
	}

exit:
	if (ret) {
		ret =  close(rotate_fd);
		if (ret){
			ALOGE("Camera_rotation fail : close rot_fd 2. Line:%d ", __LINE__);
		}
		return -1;
	}else{
		return 0;
	}
}

int do_scaling_and_rotaion(int fd, HW_SCALE_DATA_FORMAT_E output_fmt,
	uint32_t output_width, uint32_t output_height,
	uint32_t output_yaddr,uint32_t output_uvaddr,
	HW_SCALE_DATA_FORMAT_E input_fmt,uint32_t input_uv_endian,
	uint32_t input_width,uint32_t input_height,
	uint32_t input_yaddr, uint32_t intput_uvaddr,
	struct sprd_rect *trim_rect, HW_ROTATION_MODE_E rotation, uint32_t tmp_addr)
{
	int ret = 0, rotate_fd = -1;
	uint32_t param = 0;
	struct img_frm src_img, dst_img;
	struct img_rect src_rect;
	enum scle_mode scale_mode = SCALE_MODE_NORMAL;
	struct scale_frame scale_frm;
	struct _rot_cfg_tag rot_cfg;

	/*Scaling Operation*/
	src_img.fmt = (uint32_t)input_fmt;
	src_img.size.width = input_width;
	src_img.size.height = input_height;
	src_img.addr_phy.addr_y =  input_yaddr;
	src_img.addr_phy.addr_u =  intput_uvaddr;
	src_img.addr_phy.addr_v = intput_uvaddr;
	src_img.data_end.y_endian = 1;
	src_img.data_end.uv_endian = input_uv_endian;

	src_rect.start_x = trim_rect->x;
	src_rect.start_y = trim_rect->y;
	src_rect.width = trim_rect->w;
	src_rect.height = trim_rect->h;

	dst_img.fmt = (uint32_t)output_fmt;
	dst_img.size.width = output_width;
	dst_img.size.height = output_height;
	if(HW_ROTATION_0 == rotation){
		dst_img.addr_phy.addr_y =  output_yaddr;
		dst_img.addr_phy.addr_u =  output_uvaddr;
		dst_img.addr_phy.addr_v = output_uvaddr;
	}else
	{
		dst_img.addr_phy.addr_y =  tmp_addr;
		dst_img.addr_phy.addr_u =  dst_img.addr_phy.addr_y + dst_img.size.width * dst_img.size.height ;
		dst_img.addr_phy.addr_v = dst_img.addr_phy.addr_u;
	}
	dst_img.data_end.y_endian = 1;
	dst_img.data_end.uv_endian = 1;

	ret = ioctl(fd, SCALE_IO_INPUT_SIZE, &src_img.size);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_INPUT_RECT, &src_rect);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_INPUT_FORMAT, &src_img.fmt );
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_INPUT_ENDIAN, &src_img.data_end);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_INPUT_ADDR, &src_img.addr_phy);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_OUTPUT_SIZE, &dst_img.size);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_OUTPUT_FORMAT, &dst_img.fmt);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_OUTPUT_ENDIAN, &dst_img.data_end);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_OUTPUT_ADDR, &dst_img.addr_phy);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_SCALE_MODE, &scale_mode);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_START, NULL);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_IS_DONE, &scale_frm);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	if(HW_ROTATION_0 == rotation)
		return 0;

	/*Rotation Operation*/
	rotate_fd = open("/dev/sprd_rotation", O_RDWR, 0);
	if (-1 == rotate_fd) {
		ALOGE("do_scaling_and_rotaion fail : open rotation device. Line:%d ", __LINE__);
		return -1;
	}

	rot_cfg.format = (ROT_DATA_FORMAT_E)dst_img.fmt ;
	rot_cfg.angle = rotation_angle_convertion(rotation);
	rot_cfg.img_size.w = dst_img.size.width;
	rot_cfg.img_size.h = dst_img.size.height;
	rot_cfg.src_addr.y_addr = dst_img.addr_phy.addr_y;
	rot_cfg.src_addr.u_addr = dst_img.addr_phy.addr_u;
	rot_cfg.src_addr.v_addr = dst_img.addr_phy.addr_v;
	rot_cfg.dst_addr.y_addr = output_yaddr;
	rot_cfg.dst_addr.u_addr = output_uvaddr;
	rot_cfg.dst_addr.v_addr = output_uvaddr;

	ret = ioctl(rotate_fd, ROT_IO_CFG, &rot_cfg);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(rotate_fd, ROT_IO_START, 1);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(rotate_fd, ROT_IO_IS_DONE, &param);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret =  close(rotate_fd);
	if (ret){
		ALOGE("do_scaling_and_rotaion fail : close rot_fd 1. Line:%d ", __LINE__);
		return -1;
	}

exit:
	if (ret) {
		if(-1 != rotate_fd){
			ret =  close(rotate_fd);
			if (ret)
			ALOGE("do_scaling_and_rotaion fail : close rot_fd 2. Line:%d ", __LINE__);
		}
		return -1;
	}else{
		return 0;
	}
}

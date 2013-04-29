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
#include <utils/String16.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <linux/ion.h>
#include <binder/MemoryHeapIon.h>
#include <camera/Camera.h>
#include "SprdOEMCamera.h"

using namespace android;

#define ERR(x...) fprintf(stderr, ##x)
#define INFO(x...) fprintf(stdout, ##x)

#define UTEST_PARAM_NUM 4
#define UTEST_PREVIEW_WIDTH 640
#define UTEST_PREVIEW_HEIGHT 480

enum utest_sensor_id {
	UTEST_SENSOR_MAIN = 0,
	UTEST_SENSOR_SUB,
	UTEST_SENSOR_ID_MAX
};

struct utest_cmr_context {
	uint32_t sensor_id;
	uint32_t capture_width;
	uint32_t capture_height;
	char *save_directory;

	sp<MemoryHeapIon> cap_pmem_hp;
	uint32_t cap_pmemory_size;
	int cap_physical_addr ;
	unsigned char* cap_virtual_addr;
	sp<MemoryHeapIon> misc_pmem_hp;
	uint32_t misc_pmemory_size;
	int misc_physical_addr;
	unsigned char* misc_virtual_addr;

	uint32_t captured_done;
};

static struct utest_cmr_context utest_cmr_cxt;
static struct utest_cmr_context *g_utest_cmr_cxt_ptr = &utest_cmr_cxt;

static void utest_dcam_usage()
{
	INFO("utest_dcam_usage:\n");
	INFO("utest_camera -id sensor_id -w capture_width -h capture_height -d output_directory \n");
	INFO("-id		: select sensor id(0: main sensor / 1: sub sensor)\n");
	INFO("-w		: captured picture size width\n");
	INFO("-h		: captured picture size width\n");
	INFO("-d		: directory for saving the captured picture\n");
	INFO("-help	: show this help message\n");
}

static int utest_dcam_param_set(int argc, char **argv)
{
	int i = 0;
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;
	
	if (argc < UTEST_PARAM_NUM) {
		utest_dcam_usage();
		return -1;
	}

	memset(cmr_cxt_ptr, 0, sizeof(utest_cmr_context));

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-id") == 0 && (i < argc-1)) {
			cmr_cxt_ptr->sensor_id = atoi(argv[++i]);
			if (UTEST_SENSOR_SUB < cmr_cxt_ptr->sensor_id) {
				ERR("the sensor id must be 0: main_sesor 1: sub_sensor");
				return -1;
			}
		} else if (strcmp(argv[i], "-w") == 0 && (i < argc-1)) {
			cmr_cxt_ptr->capture_width = atoi(argv[++i]);
			if((0 >= cmr_cxt_ptr->capture_width) || 
				(cmr_cxt_ptr->capture_width % 16)) {
				ERR("the width of captured picture is invalid");
				return -1;
			}
		} else if (strcmp(argv[i], "-h") == 0 && (i < argc-1)) {
			cmr_cxt_ptr->capture_height = atoi(argv[++i]);
			if((0 >= cmr_cxt_ptr->capture_height) ||
				(cmr_cxt_ptr->capture_height % 16)) {
				ERR("the height of captured picture is invalid");
				return -1;
			}
		} else if (strcmp(argv[i], "-d") == 0 && (i < argc-1)) {
			cmr_cxt_ptr->save_directory = argv[++i];
		} else if (strcmp(argv[i], "-help") == 0) {
			utest_dcam_usage();
			return -1;
		} else {
			utest_dcam_usage();
			return -1;
		}
	}

	return 0;
}

static void utest_dcam_memory_release(void)
{
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (cmr_cxt_ptr->cap_physical_addr)
		cmr_cxt_ptr->cap_pmem_hp.clear();

	if (cmr_cxt_ptr->misc_physical_addr)
		cmr_cxt_ptr->misc_pmem_hp.clear();
}

static int utest_dcam_memory_alloc(void)
{
	uint32_t local_width, local_height;
	uint32_t mem_size0, mem_size1;
	uint32_t buffer_size = 0;
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (camera_capture_max_img_size(&local_width, &local_height))
		return -1;

	if (camera_capture_get_buffer_size(cmr_cxt_ptr->sensor_id, local_width,
		local_height, &mem_size0, &mem_size1))
		return -1;

	buffer_size = camera_get_size_align_page(mem_size0);
	cmr_cxt_ptr->cap_pmem_hp = new MemoryHeapIon("/dev/ion", buffer_size,
		MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
	if (cmr_cxt_ptr->cap_pmem_hp->getHeapID() < 0) {
		ERR("Failed to alloc capture pmem buffer.\n");
		return -1;
	}
	cmr_cxt_ptr->cap_pmem_hp->get_phy_addr_from_ion((int *)(&cmr_cxt_ptr->cap_physical_addr),
		(int *)(&cmr_cxt_ptr->cap_pmemory_size));
	cmr_cxt_ptr->cap_virtual_addr = (unsigned char*)cmr_cxt_ptr->cap_pmem_hp->base();
	if (!cmr_cxt_ptr->cap_physical_addr) {
		ERR("Failed to alloc capture pmem buffer:addr is null.\n");
		return -1;
	}

	if (mem_size1) {
		buffer_size = camera_get_size_align_page(mem_size1);
		cmr_cxt_ptr->misc_pmem_hp = new MemoryHeapIon("/dev/ion", buffer_size,
			MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
		if (cmr_cxt_ptr->misc_pmem_hp->getHeapID() < 0) {
			ERR("Failed to alloc misc pmem buffer.\n");
			return -1;
		}
		cmr_cxt_ptr->misc_pmem_hp->get_phy_addr_from_ion((int *)(&cmr_cxt_ptr->misc_physical_addr),
			(int *)(&cmr_cxt_ptr->misc_pmemory_size));
		cmr_cxt_ptr->misc_virtual_addr = (unsigned char*)cmr_cxt_ptr->misc_pmem_hp->base();
		if (!cmr_cxt_ptr->misc_physical_addr) {
			ERR("Failed to alloc misc  pmem buffer:addr is null.\n");
			utest_dcam_memory_release();
			return -1;
		}
	}

	if(0 != mem_size1) {
		if (camera_set_capture_mem(0,
			(uint32_t)cmr_cxt_ptr->cap_physical_addr,
			(uint32_t)cmr_cxt_ptr->cap_virtual_addr,
			(uint32_t)cmr_cxt_ptr->cap_pmemory_size,
			(uint32_t)cmr_cxt_ptr->misc_physical_addr,
			(uint32_t)cmr_cxt_ptr->misc_virtual_addr,
			(uint32_t)cmr_cxt_ptr->misc_pmemory_size)) {
				utest_dcam_memory_release();
				return -1;
		}
	} else {
		if (camera_set_capture_mem(0,
			(uint32_t)cmr_cxt_ptr->cap_physical_addr,
			(uint32_t)cmr_cxt_ptr->cap_virtual_addr,
			(uint32_t)cmr_cxt_ptr->cap_pmemory_size,
			0,
			0,
			0)) {
				utest_dcam_memory_release();
				return -1;
		}
	}

	return 0;
}

static void utest_dcam_close(void)
{
	camera_set_cap_raw_mode(CAMERA_TOOL_CAP_RAW_NONE);
	utest_dcam_memory_release();
	camera_stop(NULL, NULL);
}
static void utest_dcam_cap_cb(camera_cb_type cb,
			const void *client_data,
			camera_func_type func,
			int32_t parm4)
{
	if (CAMERA_FUNC_TAKE_PICTURE == func) {

		struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

		switch(cb) {
		case CAMERA_RSP_CB_SUCCESS:
			break;
		case CAMERA_EVT_CB_SNAPSHOT_DONE:
			break;
		case CAMERA_EXIT_CB_DONE:
			{
				FILE *fp = NULL;
				camera_frame_type *frame = (camera_frame_type *)parm4;
				if(cmr_cxt_ptr->save_directory) {
					fp = fopen(cmr_cxt_ptr->save_directory, "wb");
					fwrite(frame->buf_Virt_Addr, 1, frame->captured_dx * 
						frame->captured_dy * 5/4, fp);
					fclose(fp);
				}
				cmr_cxt_ptr->captured_done = 1;
			}
			break;
		default:
			cmr_cxt_ptr->captured_done = 1;
			break;
		}
	}
}

int main(int argc, char **argv)
{
	struct utest_cmr_context *cmr_cxt_ptr = g_utest_cmr_cxt_ptr;

	if (utest_dcam_param_set(argc, argv))
		return -1;

	if (CAMERA_SUCCESS != camera_init(cmr_cxt_ptr->sensor_id))
		return -1;

	camera_set_dimensions(cmr_cxt_ptr->capture_width,
					cmr_cxt_ptr->capture_height,
					cmr_cxt_ptr->capture_width, //UTEST_PREVIEW_WIDTH,
					cmr_cxt_ptr->capture_height, //UTEST_PREVIEW_HEIGHT,
					NULL,
					NULL);

	camera_set_cap_raw_mode(CAMERA_TOOL_CAP_RAW_ENABLE);

	if (utest_dcam_memory_alloc())
		return -1;
	
	if (CAMERA_SUCCESS != camera_take_picture_raw(utest_dcam_cap_cb,
		NULL, CAMERA_NORMAL_MODE)) {
		utest_dcam_close();
		return -1;
	}

	while(1) {
			if (cmr_cxt_ptr->captured_done) {
				cmr_cxt_ptr->captured_done = 0;
				utest_dcam_close();
				break;
			}
			usleep(300000);
	}

	return 0;
}

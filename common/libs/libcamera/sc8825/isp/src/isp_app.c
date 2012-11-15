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

#include <sys/types.h>
#include "isp_app.h"
#include "isp_com.h"
#include "cmr_msg.h"

int isp_init(struct isp_init_param* ptr)
{
	int rtn=ISP_SUCCESS;


	return rtn;
}

/* isp_deinit --
*@
*@
*@ return:
*/
int isp_deinit(void)
{
	int rtn=ISP_SUCCESS;

	return rtn;
}

/* isp_capbility --
*@
*@
*@ return:
*/
int isp_capbility(enum isp_capbility_cmd cmd, void* param_ptr)
{
	int rtn=ISP_SUCCESS;

	return rtn;
}

/* isp_ioctl --
*@
*@
*@ return:
*/
int isp_ioctl(enum isp_ctrl_cmd cmd, void* param_ptr, int(*callback)())
{
	int rtn=ISP_SUCCESS;

	return rtn;
}

/* isp_video_start --
*@
*@
*@ return:
*/
int isp_video_start(struct isp_video_start* param_ptr)
{
	int rtn=ISP_SUCCESS;

	return rtn;
}

/* isp_video_start --
*@
*@
*@ return:
*/
int isp_video_stop(void)
{
	int rtn=ISP_SUCCESS;

	return rtn;
}

/* isp_proc_start --
*@
*@
*@ return:
*/
int isp_proc_start(struct ips_in_param* in_param_ptr, struct ips_out_param* out_param_ptr)
{
	int rtn=ISP_SUCCESS;

	return rtn;
}

/* isp_proc_next --
*@
*@
*@ return:
*/
int isp_proc_next(struct ipn_in_param* in_ptr, struct ips_out_param *out_ptr)
{
	int rtn = ISP_SUCCESS;

	return rtn;
}

/* isp_evt_reg --
*@
*@
*@ return:
*/
void isp_evt_reg(cmr_evt_cb  isp_event_cb)
{
	return;
}

/**---------------------------------------------------------------------------*/


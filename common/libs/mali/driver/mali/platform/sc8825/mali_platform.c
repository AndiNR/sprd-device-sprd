/*
 * Copyright (C) 2010-2011 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for a default platform
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/system.h>
#include <asm/io.h>

#include <mach/hardware.h>
#include <mach/regs_glb.h>
#include <mach/regs_ahb.h>
#include <mach/sci.h>

#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "mali_platform.h"

_mali_osk_errcode_t mali_platform_init(void)
{
	sci_glb_clr(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
	msleep(2);
	sci_glb_set(REG_GLB_PCTRL, BIT_MCU_GPLL_EN);
	sci_glb_set(REG_AHB_AHB_CTL0, BIT_G3D_EB);
	MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_deinit(void)
{
	sci_glb_clr(REG_AHB_AHB_CTL0, BIT_G3D_EB);
	sci_glb_clr(REG_GLB_PCTRL, BIT_MCU_GPLL_EN);
	sci_glb_set(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
	MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode)
{
	switch(power_mode)
	{
	case MALI_POWER_MODE_ON:
		sci_glb_clr(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
		msleep(2);
		sci_glb_set(REG_GLB_PCTRL, BIT_MCU_GPLL_EN);
		sci_glb_set(REG_AHB_AHB_CTL0, BIT_G3D_EB);
		break;
	case MALI_POWER_MODE_LIGHT_SLEEP:
		sci_glb_clr(REG_AHB_AHB_CTL0, BIT_G3D_EB);
		sci_glb_clr(REG_GLB_PCTRL, BIT_MCU_GPLL_EN);
		break;
	case MALI_POWER_MODE_DEEP_SLEEP:
		sci_glb_clr(REG_AHB_AHB_CTL0, BIT_G3D_EB);
		sci_glb_clr(REG_GLB_PCTRL, BIT_MCU_GPLL_EN);
		sci_glb_set(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
		break;
	};
	MALI_SUCCESS;
}

void mali_gpu_utilization_handler(u32 utilization)
{
}

void set_mali_parent_power_domain(void* dev)
{
}


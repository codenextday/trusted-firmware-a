/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>

#include <drivers/synopsys/dw_mmc.h>
#include <errno.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include <string.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <common/desc_image_load.h>
#include <drivers/arm/pl011.h>
#include <drivers/generic_delay_timer.h>
#include "qserver_def.h"
#include "qserver_private.h"



static meminfo_t bl2_tzram_layout;
static console_pl011_t console;



#pragma weak bl2_plat_handle_post_image_load

static void qserver_ddr_init()
{
}
void bl2_early_platform_setup2(u_register_t arg0, u_register_t arg1,
	u_register_t arg2, u_register_t arg3)
{
	struct meminfo *mem_layout = (struct meminfo *)arg1;
	//dw_mmc_params_t params;
	generic_delay_timer_init();
	/* Initialize the console to provide early debug support */
	console_pl011_register(CONSOLE_BASE, PL011_UART_CLK_IN_HZ, PL011_BAUDRATE, &console);

	/* Setup the BL2 memory layout */
	bl2_tzram_layout = *mem_layout;

	/* Clear SRAM since it'll be used by MCU right now. */
	/*memset((void *)SRAM_BASE, 0, SRAM_SIZE);*/

	dsb();
	qserver_ddr_init();


	/*reset_dwmmc_clk();
	memset(&params, 0, sizeof(dw_mmc_params_t));
	params.reg_base = DWMMC0_BASE;
	params.desc_base = QSERVER_MMC_DESC_BASE;
	params.desc_size = 1 << 20;
	params.clk_rate = 24 * 1000 * 1000;
	params.bus_width = EMMC_BUS_WIDTH_8;
	params.flags = EMMC_FLAG_CMD23;
	dw_mmc_init(&params);*/
	qserver_io_setup();
}

void bl2_plat_arch_setup(void)
{
	qserver_init_mmu_el1(bl2_tzram_layout.total_base,
		bl2_tzram_layout.total_size,
		BL_CODE_BASE,
		BL_CODE_END
#if USE_COHERENT_MEM
		,
		BL_COHERENT_RAM_BASE,
		BL_COHERENT_RAM_END
#endif
		);
}

void bl2_platform_setup(void)
{

}

#if 0
int bl2_handle_post_image_load(unsigned int image_id){
	int err = 0;
	bl_mem_params_node_t *bl_mem_params = get_bl_mem_params_node(image_id);
	assert(bl_mem_params);

	switch (image_id) {
#ifdef __aarch64__
		case BL32_IMAGE_ID:
		bl_mem_params->ep_info.spsr = arm_get_spsr_for_bl32_entry();
		break;
#endif

		case BL33_IMAGE_ID:
		/* BL33 expects to receive the primary CPU MPID (through r0) */
		bl_mem_params->ep_info.args.arg0 = 0xffff & read_mpidr();
		bl_mem_params->ep_info.spsr = arm_get_spsr_for_bl33_entry();
		break;

#ifdef SCP_BL2_BASE
		case SCP_BL2_IMAGE_ID:
		/* The subsequent handling of SCP_BL2 is platform specific */
		err = plat_arm_bl2_handle_scp_bl2(&bl_mem_params->image_info);
		if (err) {
			WARN("Failure in platform-specific handling of SCP_BL2 image.\n");
		}
		break;
#endif
	}

	return err;
}
#endif
/*******************************************************************************
 * This function can be used by the platforms to update/use image
 * information for given `image_id`.
 ******************************************************************************/
int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	return 0;
}

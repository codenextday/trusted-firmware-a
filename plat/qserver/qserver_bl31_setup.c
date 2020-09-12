/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <drivers/arm/cci.h>
#include <errno.h>
#include <platform_def.h>
#include <arch_helpers.h>
#include <bl31/interrupt_mgmt.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <common/interrupt_props.h>
#include <drivers/arm/cci.h>
#include <drivers/arm/gicv3.h>
#include <drivers/arm/pl011.h>
#include <drivers/console.h>
#include <drivers/generic_delay_timer.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>


#include <qserver.h>

#include "qserver_def.h"
#include "qserver_private.h"

static entry_point_info_t bl32_ep_info;
static entry_point_info_t bl33_ep_info;
static console_pl011_t console;

/******************************************************************************
 * On a GICv2 system, the Group 1 secure interrupts are treated as Group 0
 * interrupts.
 *****************************************************************************/
const unsigned int g0_interrupt_array[] = {
	IRQ_SEC_PHY_TIMER,
	IRQ_SEC_SGI_0
};


entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

	next_image_info = (type == NON_SECURE) ? &bl33_ep_info : &bl32_ep_info;

	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	return NULL;
}

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				u_register_t arg2, u_register_t arg3)
{
	void *from_bl2;

	from_bl2 = (void *) arg0;
	/* Initialize the console to provide early debug support */
	console_pl011_register(CONSOLE_BASE, PL011_UART_CLK_IN_HZ, PL011_BAUDRATE, &console);

	/* Initialize CCI driver */
	plat_qserver_interconnect_init();
	plat_qserver_interconnect_enter_coherency();
#if RESET_TO_BL31
    	/* Populate entry point information for BL33 */
	SET_PARAM_HEAD(&bl33_ep_info,
				PARAM_EP,
				VERSION_1,
				0);
	/*
	 * Tell BL31 where the non-trusted software image
	 * is located and the entry state information
	 */
	bl33_ep_info.pc = plat_get_ns_image_entrypoint();
	bl33_ep_info.spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	SET_SECURITY_STATE(bl33_ep_info.h.attr, NON_SECURE);
#else
	/*
	 * Copy BL3-2 and BL3-3 entry point information.
	 * They are stored in Secure RAM, in BL2's address space.
	 */
	/*
	 * Check params passed from BL2 should not be NULL,
	 */
	bl_params_t *params_from_bl2 = (bl_params_t *)from_bl2;
	assert(params_from_bl2 != NULL);
	assert(params_from_bl2->h.type == PARAM_BL_PARAMS);
	assert(params_from_bl2->h.version >= VERSION_2);

	bl_params_node_t *bl_params = params_from_bl2->head;

	/*
	 * Copy BL33 and BL32 (if present), entry point information.
	 * They are stored in Secure RAM, in BL2's address space.
	 */
	while (bl_params) {
		if (bl_params->image_id == BL32_IMAGE_ID)
			bl32_ep_info = *bl_params->ep_info;

		if (bl_params->image_id == BL33_IMAGE_ID)
			bl33_ep_info = *bl_params->ep_info;

		bl_params = bl_params->next_params_info;
	}

	if (bl33_ep_info.pc == 0)
		panic();
#endif
}

void bl31_plat_arch_setup(void)
{
	qserver_init_mmu_el3(BL31_BASE,
			BL31_LIMIT - BL31_BASE,
			BL_CODE_BASE,
			BL_CODE_END
#if USE_COHERENT_MEM
			,
			BL_COHERENT_RAM_BASE,
			BL_COHERENT_RAM_END
#endif
			);
}

void bl31_platform_setup(void)
{
	/* Initialize the GIC driver, cpu and distributor interfaces */
	plat_qserver_gic_driver_init();
	plat_qserver_gic_init();
}

void bl31_plat_runtime_setup(void)
{
}

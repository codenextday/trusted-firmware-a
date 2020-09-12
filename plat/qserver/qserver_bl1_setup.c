/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/synopsys/dw_mmc.h>
#include <errno.h>
#include <drivers/gpio.h>
#include <drivers/generic_delay_timer.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <string.h>
#include <bl1/tbbr/tbbr_img_desc.h>
#include <drivers/arm/pl011.h>

#include "../../bl1/bl1_private.h"
#include "qserver_private.h"

/* Data structure which holds the extents of the trusted RAM for BL1 */
static meminfo_t bl1_tzram_layout;
static console_pl011_t console;
enum {
	BOOT_NORMAL = 0,
	BOOT_USB_DOWNLOAD,
	BOOT_UART_DOWNLOAD,
};

meminfo_t* bl1_plat_sec_mem_layout(void)
{
	return &bl1_tzram_layout;
}

/*
 * Perform any BL1 specific platform actions.
 */
void bl1_early_platform_setup(void)
{
	/*const size_t bl1_size = BL1_RAM_LIMIT - BL1_RAM_BASE;*/
	/* sys counter enable */
	mmio_write_32(0x13000000, 0x3);
	generic_delay_timer_init();
	/* Initialize the console to provide early debug support */
	console_pl011_register(CONSOLE_BASE, PL011_UART_CLK_IN_HZ, PL011_BAUDRATE, &console);

	/* Allow BL1 to see the whole Trusted RAM */
	bl1_tzram_layout.total_base = QSERVER_BL_RAM_BASE;
	bl1_tzram_layout.total_size = QSERVER_BL_RAM_SIZE;


	INFO("BL1: 0x%lx - 0x%lx [size = %lu]\n", BL1_RAM_BASE, BL1_RAM_LIMIT,
		BL1_RAM_LIMIT - BL1_RAM_BASE);
}

/*
 * Perform the very early platform specific architecture setup here. At the
 * moment this only does basic initialization. Later architectural setup
 * (bl1_arch_setup()) does not do anything platform specific.
 */
void bl1_plat_arch_setup(void)
{
	qserver_init_mmu_el3(bl1_tzram_layout.total_base,
		bl1_tzram_layout.total_size,
		BL1_RO_BASE,
		BL1_RO_LIMIT
#if USE_COHERENT_MEM
		,
		BL_COHERENT_RAM_BASE,
		BL_COHERENT_RAM_END
#endif
		);
}


/* Initialize PLL of both eMMC and SD controllers. */
static void qserver_mmc_pll_init(void)
{
	dsb();
}

/*
 * Function which will perform any remaining platform-specific setup that can
 * occur after the MMU and data cache have been enabled.
 */
void bl1_platform_setup(void)
{
	/*dw_mmc_params_t params;

	assert((BL1_MMC_DESC_BASE >= SRAM_BASE) &&
	       ((SRAM_BASE + SRAM_SIZE) >=
		(BL1_MMC_DATA_BASE + BL1_MMC_DATA_SIZE)));*/

	qserver_mmc_pll_init();
	/*memset(&params, 0, sizeof(dw_mmc_params_t));
	params.reg_base = DWMMC0_BASE;
	params.desc_base = BL1_MMC_DESC_BASE;
	params.desc_size = 1 << 20;
	params.clk_rate = QSERVER_MMC0_FREQ;
	params.bus_width = EMMC_BUS_WIDTH_8;
	params.flags = EMMC_FLAG_CMD23;
	dw_mmc_init(&params);*/
	qserver_io_setup();

}

/*
 * The following function checks if Firmware update is needed,
 * by checking if TOC in FIP image is valid or not.
 */
unsigned int bl1_plat_get_next_image_id(void)
{
	int32_t boot_mode;
	unsigned int ret;

	boot_mode = BOOT_NORMAL;
	switch (boot_mode) {
	case BOOT_NORMAL:
		ret = BL2_IMAGE_ID;
		break;
	case BOOT_USB_DOWNLOAD:
	case BOOT_UART_DOWNLOAD:
		ret = NS_BL1U_IMAGE_ID;
		break;
	default:
		WARN("Invalid boot mode is found:%d\n", boot_mode);
		panic();
	}
	return ret;
}

image_desc_t* bl1_plat_get_image_desc(unsigned int image_id)
{
	unsigned int index = 0;

	while (bl1_tbbr_image_descs[index].image_id != INVALID_IMAGE_ID) {
		if (bl1_tbbr_image_descs[index].image_id == image_id)
			return &bl1_tbbr_image_descs[index];

		index++;
	}

	return NULL;
}

void bl1_plat_set_ep_info(unsigned int image_id,
	entry_point_info_t *ep_info)
{
	unsigned int data = 0;

	if (image_id == BL2_IMAGE_ID)
		return;
	/*inv_dcache_range(NS_BL1U_BASE, NS_BL1U_SIZE);*/
	/* CNTFRQ is read-only in EL1 */
	write_cntfrq_el0(plat_get_syscnt_freq2());
	data = read_cpacr_el1();
	do {
		data |= 3 << 20;
		write_cpacr_el1(data);
		data = read_cpacr_el1();
	} while ((data & (3 << 20)) != (3 << 20));
	INFO("cpacr_el1:0x%x\n", data);

	ep_info->args.arg0 = 0xffff & read_mpidr();
	ep_info->spsr = SPSR_64(MODE_EL1, MODE_SP_ELX,
		DISABLE_ALL_EXCEPTIONS);
}

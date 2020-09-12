/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include "../qserver_private.h"


#define MAP_DDR		MAP_REGION_FLAT(DDR_BASE,			\
					DDR_SIZE,			\
					MT_MEMORY | MT_RW | MT_NS)
#define MAP_RAM		MAP_REGION_FLAT(QSERVER_FIP_ALIGN_BASE,			\
					QSERVER_FIP_MAX_SIZE,			\
					MT_MEMORY | MT_RW | MT_SECURE)

#define MAP_DEVICE	MAP_REGION_FLAT(DEVICE_BASE,			\
					DEVICE_SIZE,			\
					MT_DEVICE | MT_RW | MT_SECURE)
#define MAP_SPI 	MAP_REGION_FLAT(XILINX_SPI_BASE,			\
					XILINX_SPI_SIZE,			\
					MT_DEVICE | MT_RW | MT_NS)
/*
#define MAP_ROM_PARAM	MAP_REGION_FLAT(BL1_RO_BASE,			\
					0x10000,		\
					MT_DEVICE | MT_RO | MT_SECURE)*/

#define MAP_SRAM	MAP_REGION_FLAT(SRAM_BASE,			\
					SRAM_SIZE,			\
					MT_MEMORY | MT_RW | MT_SECURE)

#define MAP_MAILBOX	MAP_REGION_FLAT(PLAT_QSERVER_TRUSTED_MAILBOX_BASE, \
					0x1000, \
					MT_DEVICE | MT_RW | MT_NS)
/*
 * BL1 needs to access the areas of MMC_SRAM.
 * BL1 loads BL2 from eMMC into SRAM before DDR initialized.
 */
/*#define MAP_MMC_SRAM	MAP_REGION_FLAT(HIKEY_BL1_MMC_DESC_BASE,	\
					HIKEY_BL1_MMC_DESC_SIZE +	\
					HIKEY_BL1_MMC_DATA_SIZE,	\
					MT_DEVICE | MT_RW | MT_SECURE)*/

/*
 * Table of regions for different BL stages to map using the MMU.
 * This doesn't include Trusted RAM as the 'mem_layout' argument passed to
 * hikey_init_mmu_elx() will give the available subset of that,
 */
#ifdef IMAGE_BL1
static const mmap_region_t qserver_mmap[] = {
	MAP_SPI,
	MAP_DEVICE,
	MAP_RAM,
	{0}
};
#endif

#ifdef IMAGE_BL2
static const mmap_region_t qserver_mmap[] = {
	MAP_DDR,
	MAP_SPI,
	MAP_DEVICE,
	MAP_RAM,
	{0}
};
#endif

#ifdef IMAGE_BL31
static const mmap_region_t qserver_mmap[] = {
	MAP_DEVICE,
	MAP_SRAM,
	MAP_MAILBOX,
	{0}
};
#endif

/*
 * Macro generating the code for the function setting up the pagetables as per
 * the platform memory map & initialize the mmu, for the given exception level
 */
#if USE_COHERENT_MEM
#define QSERVER_CONFIGURE_MMU_EL(_el)				\
	void qserver_init_mmu_el##_el(unsigned long total_base,	\
				  unsigned long total_size,	\
				  unsigned long ro_start,	\
				  unsigned long ro_limit,	\
				  unsigned long coh_start,	\
				  unsigned long coh_limit)	\
	{							\
	       mmap_add_region(total_base, total_base,		\
			       total_size,			\
			       MT_MEMORY | MT_RW | MT_SECURE);	\
	       mmap_add_region(ro_start, ro_start,		\
			       ro_limit - ro_start,		\
			       MT_MEMORY | MT_RO | MT_SECURE);	\
	       mmap_add_region(coh_start, coh_start,		\
			       coh_limit - coh_start,		\
			       MT_DEVICE | MT_RW | MT_SECURE);	\
	       mmap_add(qserver_mmap);				\
	       init_xlat_tables();				\
								\
	       enable_mmu_el##_el(0);				\
	}
#else
#define QSERVER_CONFIGURE_MMU_EL(_el)				\
	void qserver_init_mmu_el##_el(unsigned long total_base,	\
				  unsigned long total_size,	\
				  unsigned long ro_start,	\
				  unsigned long ro_limit)	\
	{							\
	       mmap_add_region(total_base, total_base,		\
			       total_size,			\
			       MT_MEMORY | MT_RW | MT_SECURE);	\
	       mmap_add_region(ro_start, ro_start,		\
			       ro_limit - ro_start,		\
			       MT_MEMORY | MT_RO | MT_SECURE);	\
	       mmap_add(qserver_mmap);				\
	       init_xlat_tables();				\
								\
	       enable_mmu_el##_el(0);				\
	}
#endif
/* Define EL1 and EL3 variants of the function initialising the MMU */
QSERVER_CONFIGURE_MMU_EL(1)
QSERVER_CONFIGURE_MMU_EL(3)

unsigned long plat_get_ns_image_entrypoint(void)
{
	return BL33_BASE;
}

unsigned int plat_get_syscnt_freq2(void)
{
	return 50000000;
}

/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

#include <arch.h>
#include "../qserver_def.h"

/*
 * Platform binary types for linking
 */
#define PLATFORM_LINKER_FORMAT          "elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH            aarch64

//#define IMAGE_IN_SF
//#define IMAGE_IN_MTD
#define MAX_IO_MTD_DEVICES	1
#define PLATFORM_MTD_MAX_PAGE_SIZE	1
#define SF_PAGE_SIZE    2048
/*
 * Generic platform constants
 */

/* Size of cacheable stacks */
#define PLATFORM_STACK_SIZE		0x800

#define FIRMWARE_WELCOME_STR		"Booting Trusted Firmware\n"

#define PLATFORM_CACHE_LINE_SIZE	64
#define PLATFORM_CLUSTER_COUNT		1
#define PLATFORM_CORE_COUNT_PER_CLUSTER	2
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER_COUNT *	\
					 PLATFORM_CORE_COUNT_PER_CLUSTER)
#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL2
#define PLAT_NUM_PWR_DOMAINS		(PLATFORM_CORE_COUNT + \
					 PLATFORM_CLUSTER_COUNT + 1)
#define QSERVER_PRIMARY_CPU		0x0
#define PLAT_MAX_RET_STATE		1
#define PLAT_MAX_OFF_STATE		2

#define MAX_IO_DEVICES			3
#define MAX_IO_HANDLES			4
/* eMMC RPMB and eMMC User Data */
#define MAX_IO_BLOCK_DEVICES		2

#if (ARM_GIC_ARCH == 3)
/* GIC related constants (no GICR in GIC-500) */
#define PLAT_QSERVER_GICD_BASE		0x20000000
/* GICD_BASE | (core_num) << 17 */
#define PLAT_QSERVER_GICR_BASE		0x20060000
/*#define PLAT_QSERVER_GICR_BASE		0x38200000*/
#else
/* GIC related constants (no GICR in GIC-500) */
#define PLAT_QSERVER_GICD_BASE		0x38001000
/* GICD_BASE | (core_num) << 17 */
#define PLAT_QSERVER_GICC_BASE		0x38002000
#endif
/*
 * Define a list of Group 1 Secure and Group 0 interrupts as per GICv3
 * terminology. On a GICv2 system or mode, the lists will be merged and treated
 * as Group 0 interrupts.
 */
#define QSERVER_G1S_IRQ_PROPS(grp) \
	INTR_PROP_DESC(IRQ_SEC_PHY_TIMER, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(IRQ_SEC_SGI_0, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(IRQ_SEC_SGI_1, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(IRQ_SEC_SGI_2, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(IRQ_SEC_SGI_3, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(IRQ_SEC_SGI_4, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(IRQ_SEC_SGI_5, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL)
#define QSERVER_G0_IRQ_PROPS(grp)



/*
 * Platform memory map related constants
 */

/* The first 4KB of Trusted SRAM are used as shared memory */
#ifdef IMAGE_IN_MTD
#define ROM_SIZE		(0x8000)
#else
#define ROM_SIZE		(0x6000)
#endif
#define SRAM_BASE		(ROM_SIZE)
#define SRAM_LIMIT		(0x30000)
#define SRAM_SIZE		(SRAM_LIMIT - SRAM_BASE)
/*(SRAM_LIMIT - ROM_SIZE)*/
#define QSERVER_TRUSTED_SRAM_BASE		(SRAM_BASE)
#define QSERVER_SHARED_RAM_BASE		QSERVER_TRUSTED_SRAM_BASE
#define QSERVER_SHARED_RAM_SIZE		0x00001000	/* 4 KB */

/* The remaining Trusted SRAM is used to load the BL images */
#define QSERVER_BL_RAM_BASE			(SRAM_BASE)
#define QSERVER_BL_RAM_SIZE			(SRAM_SIZE)

#define TZDRAM_BASE		0xbf000000
#define TZDRAM_SIZE		(16 * 1024 * 1024)
/*
 * BL1 specific defines.
 *
 * Both loader and BL1_RO region stay in SRAM since they are used to simulate
 * ROM.
 * Loader is used to switch Hi6220 SoC from 32-bit to 64-bit mode.
 *
 * ++++++++++  0xF980_0000
 * + loader +
 * ++++++++++  0xF980_1000
 * + BL1_RO +
 * ++++++++++  0xF981_0000
 * + BL1_RW +
 * ++++++++++  0xF989_8000
 */
/*#ifdef IMAGE_IN_SF
#define BL1_RO_BASE			(0x6000)
#else*/
#define BL1_RO_BASE			(0x0)
/*#endif*/
#define BL1_RO_LIMIT		(BL1_RO_BASE + ROM_SIZE)

#define BL1_RW_SIZE			(0x0008000)
#define BL1_RW_BASE			(SRAM_BASE + SRAM_SIZE - BL1_RW_SIZE)

#define BL1_RW_LIMIT		(SRAM_BASE + SRAM_SIZE)

/*
 * BL2 specific defines.
 */
#define BL2_SIZE		(0xc000)
#define BL2_BASE		(BL1_RW_BASE - BL2_SIZE)
#define BL2_LIMIT		(BL1_RW_BASE)

/*
 * BL31 specific defines.
 */
#define BL31_SIZE 			(0x12000)
#define BL31_BASE			(SRAM_BASE)/*(BL1_RW_BASE - BL31_SIZE)*/
#define BL31_LIMIT			(BL31_BASE + BL31_SIZE)


/* BL32 */
#ifdef HAVE_BL32
#define BL32_BASE			TZDRAM_BASE
#define BL32_LIMIT			TZDRAM_SIZE
#endif
#define QSERVER_BUF_BASE 0x00010000
#define QSERVER_BUF_SIZE 0x00060000
#define QSERVER_FIP_BASE FIP_OFFSET
#define QSERVER_FIP_ALIGN_BASE	(QSERVER_FIP_BASE & ~0xfff)
#define QSERVER_FIP_MAX_SIZE 0x000d0000



#define PLAT_QSERVER_TRUSTED_MAILBOX_BASE SRAM_LIMIT


/*
 * Platform specific page table and MMU setup constants
 */
#define PLAT_PHY_ADDR_SPACE_SIZE			(1ull << 32)
#define PLAT_VIRT_ADDR_SPACE_SIZE			(1ull << 32)



#if IMAGE_BL1 || IMAGE_BL2 || IMAGE_BL31
#define MAX_XLAT_TABLES			6
#endif

#define MAX_MMAP_REGIONS		16

/* flash support */
#define CONFIG_SPI_FLASH_GIGADEVICE 1
#define CONFIG_SPI_FLASH_MACRONIX   1
#define CONFIG_SPI_FLASH_SPANSION   1
#define CONFIG_SPI_FLASH_STMICRO    1

/*
 * Declarations and constants to access the mailboxes safely. Each mailbox is
 * aligned on the biggest cache line size in the platform. This is known only
 * to the platform as it might have a combination of integrated and external
 * caches. Such alignment ensures that two maiboxes do not sit on the same cache
 * line at any cache level. They could belong to different cpus/clusters &
 * get written while being protected by different locks causing corruption of
 * a valid mailbox address.
 */
#define CACHE_WRITEBACK_SHIFT		6
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)

#endif /* __PLATFORM_DEF_H__ */

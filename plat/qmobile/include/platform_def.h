/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

#include <arch.h>
#include "../qmobile_def.h"

/*
 * Platform binary types for linking
 */
#define PLATFORM_LINKER_FORMAT          "elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH            aarch64


/*
 * Generic platform constants
 */

/* Size of cacheable stacks */
#define PLATFORM_STACK_SIZE		0x800

#define FIRMWARE_WELCOME_STR		"Booting Trusted Firmware\n"

#define PLATFORM_CACHE_LINE_SIZE	64
#define PLATFORM_CLUSTER_COUNT		2
#define PLATFORM_CORE_COUNT_PER_CLUSTER	4
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER_COUNT *	\
					 PLATFORM_CORE_COUNT_PER_CLUSTER)
#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL2
#define PLAT_NUM_PWR_DOMAINS		(PLATFORM_CORE_COUNT + \
					 PLATFORM_CLUSTER_COUNT + 1)

#define PLAT_MAX_RET_STATE		1
#define PLAT_MAX_OFF_STATE		2

#define MAX_IO_DEVICES			3
#define MAX_IO_HANDLES			4
/* eMMC RPMB and eMMC User Data */
#define MAX_IO_BLOCK_DEVICES		2

/* GIC related constants (no GICR in GIC-400) */
#define PLAT_ARM_GICD_BASE		0x38001000
#define PLAT_ARM_GICC_BASE		0x38002000


/*
 * Platform memory map related constants
 */



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


/*
 * BL2 specific defines.
 */

/*
 * SCP_BL2 specific defines.
 * In HiKey, SCP_BL2 means MCU firmware. It's loaded into the temporary buffer
 * at 0x0100_0000. Then BL2 will parse the sections and loaded them into
 * predefined separated buffers.
 */
#define BL2_BASE		0x0
#define BL2_LIMIT		0x40000

/*
 * BL31 specific defines.
 */
#define BL31_BASE			0x0
#define BL31_LIMIT			0x02000000

#define PLAT_UNNAME_TRUSTED_MAILBOX_BASE 0x01000000


/*
 * Platform specific page table and MMU setup constants
 */
#define ADDR_SPACE_SIZE			(1ull << 32)

#if IMAGE_BL1 || IMAGE_BL2 || IMAGE_BL31
#define MAX_XLAT_TABLES			3
#endif

#define MAX_MMAP_REGIONS		16



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

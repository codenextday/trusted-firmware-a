/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __QSERVER_PRIVATE_H__
#define __QSERVER_PRIVATE_H__

#include <common/bl_common.h>
/*
 * Function and variable prototypes
 */
void qserver_init_mmu_el1(unsigned long total_base,
			unsigned long total_size,
			unsigned long ro_start,
			unsigned long ro_limit
#if USE_COHERENT_MEM
			,
			unsigned long coh_start,
			unsigned long coh_limit
#endif
			);
void qserver_init_mmu_el3(unsigned long total_base,
			unsigned long total_size,
			unsigned long ro_start,
			unsigned long ro_limit
#if USE_COHERENT_MEM
			,
			unsigned long coh_start,
			unsigned long coh_limit
#endif
			);

void qserver_io_setup(void);

#endif /* __HIKEY_PRIVATE_H__ */

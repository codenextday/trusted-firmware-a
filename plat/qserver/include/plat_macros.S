/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLAT_MACROS_S__
#define __PLAT_MACROS_S__

#include <drivers/arm/cci.h>
#include <platform_def.h>

/* ---------------------------------------------
 * The below macro prints out relevant GIC
 * registers whenever an unhandled exception is
 * taken in BL31.
 * ---------------------------------------------
 */
.macro plat_crash_print_regs

	/* Store to the crash buf and print to console */
	bl	str_in_crash_buf_print
.endm

#endif /* __PLAT_MACROS_S__ */

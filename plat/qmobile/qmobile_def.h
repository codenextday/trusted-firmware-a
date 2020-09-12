/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __HIKEY_DEF_H__
#define __HIKEY_DEF_H__

#include <common_def.h>
#include <tbbr_img_def.h>

/* Always assume DDR is 1GB size. */
#define DDR_BASE			0x40000000
#define DDR_SIZE			0x80000000

#define DEVICE_BASE         0x10000000
#define DEVICE_SIZE         0x30000000

#define qmobile_NS_IMAGE_OFFSET      0x78000000

#define SRAM_BASE			0x0
#define SRAM_SIZE			0x08000000


/*
 * PL011 related constants
 */
#define PL011_UART_BASE		0x12000000
#define PL011_BAUDRATE			24000000
#define PL011_UART_CLK_IN_HZ		24000000


#define CCI400_BASE			0x2c090000
#define CCI400_SL_IFACE3_CLUSTER_IX			0
#define CCI400_SL_IFACE4_CLUSTER_IX			1
/*
 * GIC400 interrupt handling related constants
 */
#define IRQ_SEC_PHY_TIMER			29
#define IRQ_SEC_SGI_0				8
#define IRQ_SEC_SGI_1				9
#define IRQ_SEC_SGI_2				10
#define IRQ_SEC_SGI_3				11
#define IRQ_SEC_SGI_4				12
#define IRQ_SEC_SGI_5				13
#define IRQ_SEC_SGI_6				14
#define IRQ_SEC_SGI_7				15
#define IRQ_SEC_SGI_8				16

#endif /* __HIKEY_DEF_H__ */

/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __QSERVER_DEF_H__
#define __QSERVER_DEF_H__

#include <plat/common/common_def.h>
#include <tbbr_img_def.h>

/* Always assume DDR is 1GB size. */
#define DDR_BASE			0x40000000
#define DDR_SIZE			0x80000000

#define DEVICE_BASE         0x10000000
#define DEVICE_SIZE         0x30000000

#define BL33_BASE			0x4f000000
#define BL33_SIZE			0x01000000

/*#define SRAM_BASE			0x0
#define SRAM_SIZE			0x08000000*/


/*
 * PL011 related constants
 */
#define PL011_UART_BASE		0x12000000
#define PL011_BAUDRATE			115200
#define PL011_UART_CLK_IN_HZ		24000000

#define XILINX_SPI_BASE			0x08000000
#define XILINX_SPI_SIZE			0x08000000
#define XILINX_SPI_FREQ			37500000
#ifdef HAVE_CCI
#define PLAT_QSERVER_CCI_BASE		0x28390000	/*0x14800000*/ /*0x2c090000*/
#define PLAT_QSERVER_CCI_CLUSTER0_SL_IFACE_IX			0
#define PLAT_QSERVER_CCI_CLUSTER1_SL_IFACE_IX			1
#else
#define PLAT_QSERVER_CCN_BASE		0x2c000000
#define PLAT_QSERVER_CLUSTER_TO_CCN_ID_MAP	1, 5, 7, 11
#endif

/* FIP image layout */
#define QSERVER_SF_FIP_BASE     (0)
#define QSERVER_SF_FIP_MAX_SIZE (8 << 20)

#define QSERVER_SF_DATA_BASE  DDR_BASE
#define QSERVER_SF_DATA_SIZE  (8 << 20)
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

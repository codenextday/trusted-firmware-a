/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "qserver_def.h"
#include <drivers/arm/gicv2.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include "qserver.h"

/******************************************************************************
 * The following functions are defined as weak to allow a platform to override
 * the way the GICv2 driver is initialised and used.
 *****************************************************************************/
#pragma weak plat_qserver_gic_driver_init
#pragma weak plat_qserver_gic_init
#pragma weak plat_qserver_gic_cpuif_enable
#pragma weak plat_qserver_gic_cpuif_disable
#pragma weak plat_qserver_gic_pcpu_init

/******************************************************************************
 * On a GICv2 system, the Group 1 secure interrupts are treated as Group 0
 * interrupts.
 *****************************************************************************/

/* Array of Group0 interrupts to be configured by the gic driver */
static const interrupt_prop_t qserver_interrupt_array[] = {
	QSERVER_G1S_IRQ_PROPS(GICV2_INTR_GROUP0),
	QSERVER_G0_IRQ_PROPS(GICV2_INTR_GROUP0)
};
static unsigned int target_mask_array[PLATFORM_CORE_COUNT];
const gicv2_driver_data_t qserver_gic_data = {
	.gicd_base = PLAT_QSERVER_GICD_BASE,
	.gicc_base = PLAT_QSERVER_GICC_BASE,
	.interrupt_props = qserver_interrupt_array,
	.interrupt_props_num = ARRAY_SIZE(qserver_interrupt_array),
	.target_masks = target_mask_array,
	.target_masks_num = ARRAY_SIZE(target_mask_array),
};

void plat_qserver_gic_driver_init(void)
{
	gicv2_driver_init(&qserver_gic_data);
}

/******************************************************************************
 * ARM common helper to initialize the GIC. Only invoked by BL31
 *****************************************************************************/
void plat_qserver_gic_init(void)
{
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();
}

/******************************************************************************
 * ARM common helper to enable the GIC CPU interface
 *****************************************************************************/
void plat_qserver_gic_cpuif_enable(void)
{
	gicv2_cpuif_enable();
}

/******************************************************************************
 * ARM common helper to disable the GIC CPU interface
 *****************************************************************************/
void plat_qserver_gic_cpuif_disable(void)
{
	gicv2_cpuif_disable();
}

/******************************************************************************
 * ARM common helper to initialize the per-cpu redistributor interface in GICv3
 *****************************************************************************/
void plat_qserver_gic_pcpu_init(void)
{
	gicv2_pcpu_distif_init();
}

/******************************************************************************
 * ARM common helpers to power GIC redistributor interface
 *****************************************************************************/
void plat_qserver_gic_redistif_on(void)
{
	return;
}

void plat_qserver_gic_redistif_off(void)
{
	return;
}

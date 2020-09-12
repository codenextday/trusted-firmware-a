/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "qserver_def.h"
#include <drivers/arm/gicv3.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include "qserver.h"

/******************************************************************************
 * The following functions are defined as weak to allow a platform to override
 * the way the GICv3 driver is initialised and used.
 *****************************************************************************/
#pragma weak plat_qserver_gic_driver_init
#pragma weak plat_qserver_gic_init
#pragma weak plat_qserver_gic_cpuif_enable
#pragma weak plat_qserver_gic_cpuif_disable
#pragma weak plat_qserver_gic_pcpu_init
#pragma weak plat_qserver_gic_redistif_on
#pragma weak plat_qserver_gic_redistif_off

/* The GICv3 driver only needs to be initialized in EL3 */
static uintptr_t rdistif_base_addrs[PLATFORM_CORE_COUNT];

/* Array of Group1 secure interrupts to be configured by the gic driver */
static const interrupt_prop_t qserver_interrupt_props[] = {
	QSERVER_G1S_IRQ_PROPS(INTR_GROUP1S),
	QSERVER_G0_IRQ_PROPS(INTR_GROUP0)
};


const gicv3_driver_data_t qserver_gic_data = {
	.gicd_base = PLAT_QSERVER_GICD_BASE,
	.gicr_base = PLAT_QSERVER_GICR_BASE,
	.interrupt_props = qserver_interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(qserver_interrupt_props),
	.rdistif_num = PLATFORM_CORE_COUNT,
	.rdistif_base_addrs = rdistif_base_addrs,
	.mpidr_to_core_pos = plat_qserver_calc_core_pos,
};


void plat_qserver_gic_driver_init(void)
{
	/*
	 * The GICv3 driver is initialized in EL3 and does not need
	 * to be initialized again in SEL1. This is because the S-EL1
	 * can use GIC system registers to manage interrupts and does
	 * not need GIC interface base addresses to be configured.
	 */
#if (!defined(__aarch64__) && defined(IMAGE_BL32)) || \
	(defined(IMAGE_BL31) && defined(__aarch64__))
	gicv3_driver_init(&qserver_gic_data);
#endif
}

/******************************************************************************
 * ARM common helper to initialize the GIC. Only invoked by BL31
 *****************************************************************************/
void plat_qserver_gic_init(void)
{
	gicv3_distif_init();
	gicv3_rdistif_init(plat_my_core_pos());
	gicv3_cpuif_enable(plat_my_core_pos());
}

/******************************************************************************
 * ARM common helper to enable the GIC CPU interface
 *****************************************************************************/
void plat_qserver_gic_cpuif_enable(void)
{
	gicv3_cpuif_enable(plat_my_core_pos());
}

/******************************************************************************
 * ARM common helper to disable the GIC CPU interface
 *****************************************************************************/
void plat_qserver_gic_cpuif_disable(void)
{
	gicv3_cpuif_disable(plat_my_core_pos());
}

/******************************************************************************
 * ARM common helper to initialize the per-cpu redistributor interface in GICv3
 *****************************************************************************/
void plat_qserver_gic_pcpu_init(void)
{
	gicv3_rdistif_init(plat_my_core_pos());
}

/******************************************************************************
 * ARM common helpers to power GIC redistributor interface
 *****************************************************************************/
void plat_qserver_gic_redistif_on(void)
{
	gicv3_rdistif_on(plat_my_core_pos());
}

void plat_qserver_gic_redistif_off(void)
{
	gicv3_rdistif_off(plat_my_core_pos());
}

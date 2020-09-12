/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <drivers/arm/ccn.h>
#include <arch_helpers.h>
#include "qserver_def.h"
#include <platform_def.h>
#include "qserver.h"

static const unsigned char master_to_rn_id_map[] = {
	PLAT_QSERVER_CLUSTER_TO_CCN_ID_MAP
};

static const ccn_desc_t qserver_ccn_desc = {
	.periphbase = PLAT_QSERVER_CCN_BASE,
	.num_masters = ARRAY_SIZE(master_to_rn_id_map),
	.master_to_rn_id_map = master_to_rn_id_map
};

CASSERT(PLATFORM_CLUSTER_COUNT == ARRAY_SIZE(master_to_rn_id_map),
		assert_invalid_cluster_count_for_ccn_variant);

/******************************************************************************
 * The following functions are defined as weak to allow a platform to override
 * the way ARM CCN driver is initialised and used.
 *****************************************************************************/
#pragma weak plat_qserver_interconnect_init
#pragma weak plat_qserver_interconnect_enter_coherency
#pragma weak plat_qserver_interconnect_exit_coherency


/******************************************************************************
 * Helper function to initialize ARM CCN driver.
 *****************************************************************************/
void plat_qserver_interconnect_init(void)
{
	ccn_init(&qserver_ccn_desc);
}

/******************************************************************************
 * Helper function to place current master into coherency
 *****************************************************************************/
void plat_qserver_interconnect_enter_coherency(void)
{
	ccn_enter_snoop_dvm_domain(1 << MPIDR_AFFLVL1_VAL(read_mpidr_el1()));
}

/******************************************************************************
 * Helper function to remove current master from coherency
 *****************************************************************************/
void plat_qserver_interconnect_exit_coherency(void)
{
	ccn_exit_snoop_dvm_domain(1 << MPIDR_AFFLVL1_VAL(read_mpidr_el1()));
}

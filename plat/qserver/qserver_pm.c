/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/arm/cci.h>
#include <drivers/arm/gicv2.h>
#include <drivers/arm/pl011.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <lib/psci/psci.h>

#include <plat/common/platform.h>

#include "qserver_def.h"
#include <qserver.h>

#define CORE_PWR_STATE(state) \
	((state)->pwr_domain_state[MPIDR_AFFLVL0])
#define CLUSTER_PWR_STATE(state) \
	((state)->pwr_domain_state[MPIDR_AFFLVL1])
#define SYSTEM_PWR_STATE(state) \
	((state)->pwr_domain_state[PLAT_MAX_PWR_LVL])

static uintptr_t qserver_sec_entrypoint;

static int qserver_pwr_domain_on(u_register_t mpidr)
{
	uintptr_t *mailbox = (void *) PLAT_QSERVER_TRUSTED_MAILBOX_BASE;
	uint32_t cpu_id;
	/*curr_cluster = MPIDR_AFFLVL1_VAL(read_mpidr());*/
	cpu_id = plat_qserver_calc_core_pos(mpidr);
	*(mailbox + 1) |= (1 << (cpu_id));
	*mailbox = qserver_sec_entrypoint;
	flush_dcache_range((uintptr_t)mailbox, 0x20);
	isb();
	dsb();
	return 0;
}

static void qserver_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	NOTICE("%s: \n", __func__);
	sev();
	isb();
	//int cpu, cluster;
	/*cluster = MPIDR_AFFLVL1_VAL(mpidr);
	cpu = MPIDR_AFFLVL0_VAL(mpidr);*/

	/*
	 * Enable CCI coherency for this cluster.
	 * No need for locks as no other cpu is active at the moment.
	 */
	if (CLUSTER_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE)
		plat_qserver_interconnect_enter_coherency();

	/* Zero the jump address in the mailbox for this cpu */
	//hisi_pwrc_set_core_bx_addr(cpu, cluster, 0);

	/* Program the GIC per-cpu distributor or re-distributor interface */
	plat_qserver_gic_pcpu_init();
	plat_qserver_gic_cpuif_enable();
}

void qserver_pwr_domain_off(const psci_power_state_t *target_state)
{
	NOTICE("%s: \n", __func__);
	//int cpu, cluster;

	/*cluster = MPIDR_AFFLVL1_VAL(mpidr);
	cpu = MPIDR_AFFLVL0_VAL(mpidr);*/

	plat_qserver_gic_cpuif_disable();
	plat_qserver_gic_redistif_off();
	//hisi_ipc_cpu_off(cpu, cluster);

	if (CLUSTER_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE) {
		//hisi_ipc_spin_lock(HISI_IPC_SEM_CPUIDLE);
		plat_qserver_interconnect_exit_coherency();
		//hisi_ipc_spin_unlock(HISI_IPC_SEM_CPUIDLE);

		//hisi_ipc_cluster_off(cpu, cluster);
	}
}

static void qserver_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	/*unsigned int cpu = mpidr & MPIDR_CPU_MASK;
	unsigned int cluster =
		(mpidr & MPIDR_CLUSTER_MASK) >> MPIDR_AFFINITY_BITS;*/

	if (CORE_PWR_STATE(target_state) != PLAT_MAX_OFF_STATE)
		return;

	if (CORE_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE) {

		/* Program the jump address for the target cpu */
		//hisi_pwrc_set_core_bx_addr(cpu, cluster, hikey_sec_entrypoint);

		plat_qserver_gic_cpuif_disable();

		/*if (SYSTEM_PWR_STATE(target_state) != PLAT_MAX_OFF_STATE)
			hisi_ipc_cpu_suspend(cpu, cluster);*/
	}

	/* Perform the common cluster specific operations */
	if (CLUSTER_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE) {
		//hisi_ipc_spin_lock(HISI_IPC_SEM_CPUIDLE);
		plat_qserver_interconnect_exit_coherency();
		//hisi_ipc_spin_unlock(HISI_IPC_SEM_CPUIDLE);

		/*if (SYSTEM_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE) {
			//hisi_pwrc_set_cluster_wfi(1);
			//hisi_pwrc_set_cluster_wfi(0);
			//hisi_ipc_psci_system_off();
		} else
			//hisi_ipc_cluster_suspend(cpu, cluster);*/
	}
}

static void qserver_pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{
	/*unsigned int cluster, cpu;*/

	/* Nothing to be done on waking up from retention from CPU level */
	if (CORE_PWR_STATE(target_state) != PLAT_MAX_OFF_STATE)
		return;

	/* Get the mpidr for this cpu */
	/*cluster = (mpidr & MPIDR_CLUSTER_MASK) >> MPIDR_AFF1_SHIFT;
	cpu = mpidr & MPIDR_CPU_MASK;*/

	/* Enable CCI coherency for cluster */
	if (CLUSTER_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE)
		plat_qserver_interconnect_enter_coherency();

	//hisi_pwrc_set_core_bx_addr(cpu, cluster, 0);*/

	if (SYSTEM_PWR_STATE(target_state) == PLAT_MAX_OFF_STATE) {
		plat_qserver_gic_init();
	} else {
		plat_qserver_gic_pcpu_init();
		plat_qserver_gic_cpuif_enable();
	}
}

static void qserver_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	int i;

	for (i = MPIDR_AFFLVL0; i <= PLAT_MAX_PWR_LVL; i++)
		req_state->pwr_domain_state[i] = PLAT_MAX_OFF_STATE;
}

static void __dead2 qserver_system_off(void)
{
	NOTICE("%s: off system\n", __func__);

	/* Pull down GPIO_0_0 to trigger PMIC shutdown */
	//mmio_write_32(0xF8001810, 0x2); /* Pinmux */
	//mmio_write_8(0xF8011400, 1);	/* Pin direction */
	//mmio_write_8(0xF8011004, 0);	/* Pin output value */

	/* Wait for 2s to power off system by PMIC */
	//sp804_timer_init(SP804_TIMER0_BASE, 10, 192);
	//mdelay(2000);

	/*
	 * PMIC shutdown depends on two conditions: GPIO_0_0 (PWR_HOLD) low,
	 * and VBUS_DET < 3.6V. For HiKey, VBUS_DET is connected to VDD_4V2
	 * through Jumper 1-2. So, to complete shutdown, user needs to manually
	 * remove Jumper 1-2.
	 */
	NOTICE("+------------------------------------------+\n");
	NOTICE("| IMPORTANT: Remove Jumper 1-2 to shutdown |\n");
	NOTICE("| DANGER:    SoC is still burning. DANGER! |\n");
	NOTICE("| Board will be reboot to avoid overheat   |\n");
	NOTICE("+------------------------------------------+\n");

	/* Send the system reset request */
	//mmio_write_32(AO_SC_SYS_STAT0, 0x48698284);

	wfi();
	panic();
}

static void __dead2 qserver_system_reset(void)
{
	/* Send the system reset request */
	//mmio_write_32(AO_SC_SYS_STAT0, 0x48698284);
	isb();
	dsb();

	wfi();
	panic();
}

int qserver_validate_power_state(unsigned int power_state,
			       psci_power_state_t *req_state)
{
	int pstate = psci_get_pstate_type(power_state);
	int pwr_lvl = psci_get_pstate_pwrlvl(power_state);
	int i;

	assert(req_state);

	if (pwr_lvl > PLAT_MAX_PWR_LVL)
		return PSCI_E_INVALID_PARAMS;

	/* Sanity check the requested state */
	if (pstate == PSTATE_TYPE_STANDBY) {
		/*
		 * It's possible to enter standby only on power level 0
		 * Ignore any other power level.
		 */
		if (pwr_lvl != MPIDR_AFFLVL0)
			return PSCI_E_INVALID_PARAMS;

		req_state->pwr_domain_state[MPIDR_AFFLVL0] =
					PLAT_MAX_RET_STATE;
	} else {
		for (i = MPIDR_AFFLVL0; i <= pwr_lvl; i++)
			req_state->pwr_domain_state[i] =
					PLAT_MAX_OFF_STATE;
	}

	/*
	 * We expect the 'state id' to be zero.
	 */
	if (psci_get_pstate_id(power_state))
		return PSCI_E_INVALID_PARAMS;

	return PSCI_E_SUCCESS;
}

static int qserver_validate_ns_entrypoint(uintptr_t entrypoint)
{
	/*
	 * Check if the non secure entrypoint lies within the non
	 * secure DRAM.
	 */
	if ((entrypoint > DDR_BASE) && (entrypoint < (DDR_BASE + DDR_SIZE)))
		return PSCI_E_SUCCESS;

	return PSCI_E_INVALID_ADDRESS;
}

static const plat_psci_ops_t qserver_psci_ops = {
	.cpu_standby			= NULL,
	.pwr_domain_on			= qserver_pwr_domain_on,
	.pwr_domain_on_finish		= qserver_pwr_domain_on_finish,
	.pwr_domain_off			= qserver_pwr_domain_off,
	.pwr_domain_suspend		= qserver_pwr_domain_suspend,
	.pwr_domain_suspend_finish	= qserver_pwr_domain_suspend_finish,
	.system_off			= qserver_system_off,
	.system_reset			= qserver_system_reset,
	.validate_power_state		= qserver_validate_power_state,
	.validate_ns_entrypoint		= qserver_validate_ns_entrypoint,
	.get_sys_suspend_power_state	= qserver_get_sys_suspend_power_state,
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
    uintptr_t *mailbox = (void *) PLAT_QSERVER_TRUSTED_MAILBOX_BASE;
	qserver_sec_entrypoint = sec_entrypoint;

	NOTICE("%s: sec_entrypoint=0x%lx\n", __func__,
	     (unsigned long)qserver_sec_entrypoint);
	//*mailbox = qserver_sec_entrypoint;
	*(mailbox + 1) = 0;

	/*
	 * Initialize PSCI ops struct
	 */
	*psci_ops = &qserver_psci_ops;
	return 0;
}

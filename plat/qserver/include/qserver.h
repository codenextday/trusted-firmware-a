/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __QSERVER_H__
#define __QSERVER_H__
#if PLATFORM_CLUSTER_COUNT > 1
void plat_qserver_interconnect_init(void);
void plat_qserver_interconnect_enter_coherency(void);
void plat_qserver_interconnect_exit_coherency(void);
#else
static inline void plat_qserver_interconnect_init(void)
{
}
static inline void plat_qserver_interconnect_enter_coherency(void)
{
}
static inline void plat_qserver_interconnect_exit_coherency(void)
{
}
#endif
void plat_qserver_gic_driver_init(void);
void plat_qserver_gic_init(void);
void plat_qserver_gic_cpuif_enable(void);
void plat_qserver_gic_cpuif_disable(void);
void plat_qserver_gic_pcpu_init(void);
void plat_qserver_gic_redistif_on(void);
void plat_qserver_gic_redistif_off(void);

unsigned int plat_qserver_calc_core_pos(u_register_t mpidr);

#endif	/* __QSERVER_H__ */

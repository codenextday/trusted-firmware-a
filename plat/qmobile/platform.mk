#
# Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

CONSOLE_BASE			:=	PL011_UART_BASE
CRASH_CONSOLE_BASE		:=	PL011_UART_BASE
#PLAT_PARTITION_MAX_ENTRIES	:=	12
#PLAT_PL061_MAX_GPIOS		:=	160
#COLD_BOOT_SINGLE_CPU		:=	1
PROGRAMMABLE_RESET_ADDRESS	:=	1

# Process flags
$(eval $(call add_define,CONSOLE_BASE))
$(eval $(call add_define,CRASH_CONSOLE_BASE))
#$(eval $(call add_define,PLAT_PL061_MAX_GPIOS))
#$(eval $(call add_define,PLAT_PARTITION_MAX_ENTRIES))
#$(eval $(call FIP_ADD_IMG,SCP_BL2,--scp-fw))

ENABLE_PLAT_COMPAT	:=	0

USE_COHERENT_MEM	:=	1

PLAT_INCLUDES		:=	-Iinclude/common/tbbr			\
				-Iinclude/drivers/synopsys		\
				-Iplat/unname/include

PLAT_BL_COMMON_SOURCES	:=	drivers/arm/pl011/pl011_console.S	\
				lib/aarch64/xlat_tables.c		\
				plat/qmobile/aarch64/qmobile_common.c

#BL1_SOURCES		+=	bl1/tbbr/tbbr_img_desc.c		\
				drivers/delay_timer/delay_timer.c       \
				lib/cpus/aarch64/cortex_a53.S		\
				plat/unname/aarch64/unname_helpers.S \
				plat/unname/unname_bl1_setup.c	

#BL2_SOURCES		+=	drivers/arm/sp804/sp804_delay_timer.c	\
				drivers/delay_timer/delay_timer.c	\
				plat/unname/aarch64/unname_helpers.S \
				plat/unname/unname_bl2_setup.c	

UNNAME_GIC_SOURCES	:=	drivers/arm/gic/common/gic_common.c	\
				drivers/arm/gic/v2/gicv2_main.c		\
				drivers/arm/gic/v2/gicv2_helpers.c	\
				plat/common/plat_gic.c \
				drivers/arm/gic/arm_gic.c

BL31_SOURCES		+=	drivers/arm/cci/cci.c			\
				drivers/arm/sp804/sp804_delay_timer.c	\
				drivers/delay_timer/delay_timer.c	\
				lib/cpus/aarch64/cortex_a53.S		\
				lib/cpus/aarch64/cortex_a72.S		\
				plat/common/aarch64/plat_psci_common.c	\
				plat/qmobile/aarch64/qmobile_helpers.S \
				plat/qmobile/qmobile_bl31_setup.c	\
				plat/qmobile/qmobile_pm.c		\
				plat/qmobile/qmobile_topology.c	\
				${UNNAME_GIC_SOURCES}


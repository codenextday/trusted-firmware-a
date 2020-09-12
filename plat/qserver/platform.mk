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
ARM_GIC_ARCH			:=	2

FIP_OFFSET			:=	$(FIP_OFFSET)

MULTI_CONSOLE_API		:= 1

INTERCONNECT_UNIT		:=
ifneq ($(BL32),)
$(eval $(call add_define,HAVE_BL32))
endif

CRASH_REPORTING         :=      1
$(eval $(call add_define,CRASH_REPORTING))


ifeq ($(BOOT_DEVICE), MTD)
$(eval $(call add_define,IMAGE_IN_MTD))
endif
# Process flags
$(eval $(call add_define,CONSOLE_BASE))
$(eval $(call add_define,CRASH_CONSOLE_BASE))
$(eval $(call add_define,ARM_GIC_ARCH))
#$(eval $(call add_define,PLAT_PL061_MAX_GPIOS))
#$(eval $(call add_define,PLAT_PARTITION_MAX_ENTRIES))
#$(eval $(call FIP_ADD_IMG,SCP_BL2,--scp-fw))
$(eval $(call add_define,FIP_OFFSET))

include lib/xlat_tables_v2/xlat_tables.mk

ENABLE_PLAT_COMPAT	:=	0

#USE_COHERENT_MEM	:=	1

PLAT_INCLUDES		:=	-Iinclude/common/tbbr			\
				-Iinclude/drivers/synopsys		\
				-Iplat/qserver/include

include lib/libfdt/libfdt.mk

ifeq (${LOAD_IMAGE_V2},1)
# Because BL1/BL2 execute in AArch64 mode but BL32 in AArch32 we need to use
# the AArch32 descriptors.
#BL2_SOURCES             +=      plat/arm/common/${ARCH}/arm_bl2_mem_params_desc.c
BL2_SOURCES             +=      plat/qserver/qserver_image_load.c                \
                                common/desc_image_load.c \
				plat/qserver/qserver_image_desc.c

endif

SPI_FLASH_SOURCES	:=
ifeq ($(BOOT_DEVICE),MTD)
SPI_FLASH_SOURCES	+= 	drivers/io/io_sf.c                         \
				drivers/io/io_mtd.c                         \
				drivers/mtd/nand/core.c			\
				drivers/mtd/nor/spi_nor.c		\
				drivers/mtd/spi-mem/spi_mem.c		\
				drivers/spi_flash/spi_flash.c 		\
				drivers/spi_flash/spi_flash_ids.c	\
				drivers/spi_flash/spi_xilinx.c
endif

PLAT_BL_COMMON_SOURCES	:=	drivers/arm/pl011/aarch64/pl011_console.S	\
				plat/qserver/aarch64/qserver_common.c \
				${XLAT_TABLES_LIB_SRCS}

BL1_SOURCES		+=	bl1/tbbr/tbbr_img_desc.c		\
				drivers/delay_timer/generic_delay_timer.c       \
				drivers/delay_timer/delay_timer.c       \
				lib/cpus/aarch64/cortex_a53.S		\
				lib/cpus/aarch64/cortex_a55.S		\
				lib/cpus/aarch64/cortex_a72.S		\
				lib/cpus/aarch64/cortex_a76.S		\
				drivers/io/io_fip.c                             \
                                drivers/io/io_memmap.c                          \
                                drivers/io/io_storage.c                         \
				plat/qserver/aarch64/qserver_helpers.S 	\
				plat/qserver/qserver_bl1_setup.c	\
				plat/qserver/qserver_io_storage.c	\
				${SPI_FLASH_SOURCES}
				

BL2_SOURCES		+=	drivers/delay_timer/delay_timer.c	\
				drivers/delay_timer/generic_delay_timer.c       \
				drivers/io/io_fip.c                             \
                                drivers/io/io_memmap.c                          \
                                drivers/io/io_storage.c                         \
				plat/qserver/aarch64/qserver_helpers.S \
				plat/qserver/qserver_bl2_setup.c	\
				plat/qserver/qserver_io_storage.c	\
				${SPI_FLASH_SOURCES}
ifeq (${ARM_GIC_ARCH},3)
QSERVER_GIC_SOURCES	:=	drivers/arm/gic/common/gic_common.c	\
				drivers/arm/gic/v3/gic600.c		\
				drivers/arm/gic/v3/gicv3_main.c		\
				drivers/arm/gic/v3/gicv3_helpers.c	\
				plat/common/plat_gicv3.c \
				plat/qserver/qserver_gicv3.c
else
QSERVER_GIC_SOURCES	:=	drivers/arm/gic/common/gic_common.c	\
				drivers/arm/gic/v2/gicv2_main.c		\
				drivers/arm/gic/v2/gicv2_helpers.c	\
				plat/common/plat_gicv2.c \
				plat/qserver/qserver_gicv2.c
endif

ifeq (${INTERCONNECT_UNIT}, cci)
$(eval $(call add_define,HAVE_CCI))
QSERVER_CCN_SOURCES	:=	drivers/arm/cci/cci.c			\
				plat/qserver/qserver_cci.c
else ifeq (${INTERCONNECT_UNIT}, ccn)
$(eval $(call add_define,HAVE_CCN))
QSERVER_CCN_SOURCES	:=	drivers/arm/ccn/ccn.c			\
				plat/qserver/qserver_ccn.c
else
QSERVER_CCN_SOURCES	:=
endif
BL31_SOURCES		+=	drivers/arm/sp804/sp804_delay_timer.c	\
				drivers/delay_timer/delay_timer.c	\
				lib/cpus/aarch64/cortex_a53.S		\
				lib/cpus/aarch64/cortex_a55.S		\
				lib/cpus/aarch64/cortex_a72.S		\
				lib/cpus/aarch64/cortex_a76.S		\
				plat/common/plat_psci_common.c	\
				plat/qserver/aarch64/qserver_helpers.S \
				plat/qserver/qserver_bl31_setup.c	\
				plat/qserver/qserver_pm.c		\
				plat/qserver/qserver_topology.c	\
				${QSERVER_GIC_SOURCES}			\
				${QSERVER_CCN_SOURCES}


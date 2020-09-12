/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <common/debug.h>
#include <errno.h>
#include <tools_share/firmware_image_package.h>
#include <drivers/io/io_block.h>
#include <drivers/io/io_mtd.h>
#include <drivers/io/io_sf.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_fip.h>
#include <drivers/io/io_memmap.h>
#include <drivers/io/io_storage.h>
#include <drivers/xilinx/spi/spi.h>
#include <drivers/spi_nand.h>
#include <drivers/spi_nor.h>
#include <drivers/nand.h>
#include <lib/mmio.h>
#include <lib/utils.h>
#include <lib/semihosting.h>
#include <tools_share/firmware_image_package.h>

#include <platform_def.h>

#include <string.h>

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_dev_handle;
#if defined(IMAGE_IN_EMMC)
static const io_dev_connector_t *emmc_dev_con;
static uintptr_t emmc_dev_handle;
static int check_emmc(const uintptr_t spec);
#elif defined(IMAGE_IN_SF)
static const io_dev_connector_t *sf_dev_con;
static uintptr_t sf_dev_handle;
static int check_sf(const uintptr_t spec);
static size_t	sf_read_blocks(unsigned int offset, uintptr_t buf, size_t size);
#elif defined(IMAGE_IN_MTD)
static const io_dev_connector_t *mtd_dev_con;
static uintptr_t mtd_dev_handle;
static int check_mtd(const uintptr_t spec);
#else
static const io_dev_connector_t *memmap_dev_con;
static uintptr_t memmap_dev_handle;
static int open_memmap(const uintptr_t spec);
#endif
static int check_fip(const uintptr_t spec);

#if defined(IMAGE_IN_EMMC)
static const io_block_spec_t emmc_fip_spec = {
	.offset		= QSERVER_FIP_BASE,
	.length		= QSERVER_FIP_MAX_SIZE,
};

static const io_block_spec_t emmc_data_spec = {
	.offset		= 0,
	.length		= 256 << 20,
};

static const io_block_dev_spec_t emmc_dev_spec = {
	/* It's used as temp buffer in block driver. */
#if IMAGE_BL1
	.buffer		= {
		.offset	= QSERVER_BL1_MMC_DATA_BASE,
		.length	= QSERVER_BL1_MMC_DATA_SIZE,
	},
#else
	.buffer		= {
		.offset	= QSERVER_MMC_DATA_BASE,
		.length	= QSERVER_MMC_DATA_SIZE,
	},
#endif
	.ops		= {
		.read	= emmc_read_blocks,
		.write	= emmc_write_blocks,
	},
	.block_size	= EMMC_BLOCK_SIZE,
};
#elif defined (IMAGE_IN_SF)
static const io_block_spec_t sf_fip_spec = {
	.offset = QSERVER_SF_FIP_BASE,
	.length = QSERVER_SF_FIP_MAX_SIZE
};


static const io_sf_dev_spec_t sf_dev_spec = {
	/* It's used as temp buffer in block driver. */
#if IMAGE_BL1
	.buffer		= {
		.offset	= QSERVER_BUF_BASE,
		.length	= QSERVER_BUF_SIZE,
	},
#else
	.buffer		= {
		.offset	= QSERVER_SF_DATA_BASE,
		.length	= QSERVER_SF_DATA_SIZE,
	},
#endif
	.ops		= {
		.read	= sf_read_blocks,
		.write	= NULL,
	},
	.page_size	= SF_PAGE_SIZE,
};
#elif defined (IMAGE_IN_MTD)
static const io_block_spec_t mtd_fip_spec = {
	.offset = 0,
	.length = QSERVER_SF_FIP_MAX_SIZE
};
static io_mtd_dev_spec_t mtd_dev_spec = {
	.ops = {
		.init = spi_nor_init,
		.read = spi_nor_read,
	},
};

#else
static const io_block_spec_t fip_block_spec = {
	.offset = QSERVER_FIP_BASE,
	.length = QSERVER_FIP_MAX_SIZE
};
#endif
/*
static const io_block_dev_spec_t ram_dev_spec = {
#if IMAGE_BL1
	.buffer		= {
		.offset	= QSERVER_BL1_RAM_DATA_BASE,
		.length	= QSERVER_BL1_RAM_DATA_SIZE,
	},
#else
	.buffer		= {
		.offset	= QSERVER_RAM_DATA_BASE,
		.length	= QSERVER_RAM_DATA_SIZE,
	},
#endif
	.ops		= {
		.read	= ram_read_blocks,
		.write	= ram_write_blocks,
	},
	.block_size	= EMMC_BLOCK_SIZE,
};*/

static const io_uuid_spec_t bl2_uuid_spec = {
	.uuid = UUID_TRUSTED_BOOT_FIRMWARE_BL2,
};

static const io_uuid_spec_t scp_bl2_uuid_spec = {
	.uuid = UUID_SCP_FIRMWARE_SCP_BL2,
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl32_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

static const struct plat_io_policy policies[] = {
#if defined(IMAGE_IN_EMMC)
	[FIP_IMAGE_ID] = {
		&emmc_dev_handle,
		(uintptr_t)&emmc_fip_spec,
		check_emmc
	},
#elif defined(IMAGE_IN_SF)
	[FIP_IMAGE_ID] = {
		&sf_dev_handle,
		(uintptr_t)&sf_fip_spec,
		check_sf
	},
#elif defined(IMAGE_IN_MTD)
	[FIP_IMAGE_ID] = {
		&mtd_dev_handle,
		(uintptr_t)&mtd_fip_spec,
		check_mtd
	},
#else
	[FIP_IMAGE_ID] = {
		&memmap_dev_handle,
		(uintptr_t)&fip_block_spec,
		open_memmap
	},
#endif
	[BL2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl2_uuid_spec,
		check_fip
	},
	[SCP_BL2_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&scp_bl2_uuid_spec,
		check_fip
	},
	[BL31_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		check_fip
	},
	[BL32_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_uuid_spec,
		check_fip
	},
	[BL33_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl33_uuid_spec,
		check_fip
	},
	/*[BL2U_IMAGE_ID] = {
		&emmc_dev_handle,
		(uintptr_t)&emmc_data_spec,
		check_emmc
	}*/
};
#if defined(IMAGE_IN_EMMC)
static int check_emmc(const uintptr_t spec)
{
	int result;
	uintptr_t local_handle;

	result = io_dev_init(emmc_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(emmc_dev_handle, spec, &local_handle);
		if (result == 0)
			io_close(local_handle);
	}
	return result;
}
#elif defined(IMAGE_IN_SF)
static size_t	sf_read_blocks(unsigned int offset, uintptr_t buf, size_t size)
{
    return spi_flash_cmd_read_ops(offset, size, (void *)buf);
}
extern int spi_flash_init(unsigned int busnum, unsigned int cs,
	unsigned int max_hz, unsigned int spi_mode);
static int check_sf(const uintptr_t spec)
{
	int result;
	uintptr_t local_handle;

	result = io_dev_init(sf_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(sf_dev_handle, spec, &local_handle);
		if (result == 0)
			io_close(local_handle);
	}
	return result;
}
#elif defined(IMAGE_IN_MTD)
/*int plat_get_spi_nand_data(struct spinand_device *device)
{
	zeromem(&device->spi_read_cache_op, sizeof(struct spi_mem_op));
	device->spi_read_cache_op.cmd.opcode = SPI_NAND_OP_READ_FROM_CACHE;
	device->spi_read_cache_op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->spi_read_cache_op.addr.nbytes = 3U;
	device->spi_read_cache_op.addr.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->spi_read_cache_op.dummy.nbytes = 0U;
	device->spi_read_cache_op.dummy.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->spi_read_cache_op.data.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->spi_read_cache_op.data.dir = SPI_MEM_DATA_IN;
	return 0;
}*/
int plat_get_nor_data(struct nor_device *device)
{
	device->size = 0x8000000U;

	zeromem(&device->read_op, sizeof(struct spi_mem_op));
	device->read_op.cmd.opcode = SPI_NOR_OP_READ_FAST;
	device->read_op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->read_op.addr.nbytes = 3U;
	device->read_op.addr.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->read_op.dummy.nbytes = 1U;
	device->read_op.dummy.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->read_op.data.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	device->read_op.data.dir = SPI_MEM_DATA_IN;

	return 0;
}
static int check_mtd(const uintptr_t spec)
{
	int result;
	uintptr_t local_handle;

	result = io_dev_init(mtd_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(mtd_dev_handle, spec, &local_handle);
		if (result == 0)
			io_close(local_handle);
	}
	return result;
}
#else
static int open_memmap(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	result = io_dev_init(memmap_dev_handle, (uintptr_t)NULL);
	if (result == 0) {
		result = io_open(memmap_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using Memmap\n");
			io_close(local_image_handle);
		}
	}
	return result;
}
#endif
static int check_fip(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	/* See if a Firmware Image Package is available */
	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if (result == 0) {
		result = io_open(fip_dev_handle, spec, &local_image_handle);
		if (result == 0) {
			VERBOSE("Using FIP\n");
			io_close(local_image_handle);
		}
	}
	return result;
}


void qserver_io_setup(void)
{
	int result;
#if defined(IMAGE_IN_EMMC)
	result = register_io_dev_block(&emmc_dev_con);
	assert(result == 0);
#elif defined(IMAGE_IN_SF)
	spi_initial(0, 100000, 0);
	spi_flash_init(0, 0, 100000, 0);
	result = register_io_dev_sf(&sf_dev_con);
	assert(result == 0);
#elif defined(IMAGE_IN_MTD)
	spi_initial(0, 100000, 0);
	result = register_io_dev_mtd(&mtd_dev_con);
	assert(result == 0);
#else
	result = register_io_dev_memmap(&memmap_dev_con);
	assert(result == 0);
#endif
	result = register_io_dev_fip(&fip_dev_con);
	assert(result == 0);
#if defined(IMAGE_IN_EMMC)
	result = io_dev_open(emmc_dev_con, (uintptr_t)&emmc_dev_spec,
			     &emmc_dev_handle);
	assert(result == 0);
#elif defined(IMAGE_IN_SF)
	result = io_dev_open(sf_dev_con, (uintptr_t)&sf_dev_spec,
			     &sf_dev_handle);
	assert(result == 0);
#elif defined(IMAGE_IN_MTD)
	result = io_dev_open(mtd_dev_con, (uintptr_t)&mtd_dev_spec,
			     &mtd_dev_handle);
	assert(result == 0);
#else
	result = io_dev_open(memmap_dev_con, (uintptr_t)NULL,
				&memmap_dev_handle);
	assert(result == 0);
#endif

	result = io_dev_open(fip_dev_con, (uintptr_t)NULL, &fip_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}

/* Return an IO device handle and specification which can be used to access
 * an image. Use this to enforce platform load policy
 */
int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	int result;
	const struct plat_io_policy *policy;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];
	result = policy->check(policy->image_spec);
	assert(result == 0);

	*image_spec = policy->image_spec;
	*dev_handle = *(policy->dev_handle);

	return result;
}


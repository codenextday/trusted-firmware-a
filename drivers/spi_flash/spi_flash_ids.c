/*
 * SPI Flash ID's.
 *
 * Copyright (C) 2016 Jagan Teki <jagan@openedev.com>
 * Copyright (C) 2013 Jagannadha Sutradharudu Teki, Xilinx Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <stdint.h>
#include <string.h>
#include <platform_def.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/io/io_block.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <lib/libc/assert.h>
#include <drivers/delay_timer.h>
#include <drivers/xilinx/spi/spi.h>
//#include "spi_flash.h""

#include "sf_internal.h"

/* Used when the "_ext_id" is two bytes at most */
#define SF_INFO(_jedec_id, _ext_id, _sector_size, _n_sectors, _flags)	\
		.id = {							\
			((_jedec_id) >> 16) & 0xff,			\
			((_jedec_id) >> 8) & 0xff,			\
			(_jedec_id) & 0xff,				\
			((_ext_id) >> 8) & 0xff,			\
			(_ext_id) & 0xff,				\
			},						\
		.id_len = (!(_jedec_id) ? 0 : (3 + ((_ext_id) ? 2 : 0))),	\
		.sector_size = (_sector_size),				\
		.n_sectors = (_n_sectors),				\
		.page_size = 256,					\
		.flags = (_flags),

#define SF_INFO6(_jedec_id, _ext_id, _sector_size, _n_sectors, _flags)	\
		.id = {							\
			((_jedec_id) >> 16) & 0xff,			\
			((_jedec_id) >> 8) & 0xff,			\
			(_jedec_id) & 0xff,				\
			((_ext_id) >> 16) & 0xff,			\
			((_ext_id) >> 8) & 0xff,			\
			(_ext_id) & 0xff,				\
			},						\
		.id_len = 6,						\
		.sector_size = (_sector_size),				\
		.n_sectors = (_n_sectors),				\
		.page_size = 256,					\
		.flags = (_flags),

const struct spi_flash_info spi_flash_ids[] = {
#ifdef CONFIG_SPI_FLASH_ATMEL		/* ATMEL */
	{"at45db011d",	   SF_INFO(0x1f2200, 0x0, 64 * 1024,     4, SECT_4K) },
	{"at45db021d",	   SF_INFO(0x1f2300, 0x0, 64 * 1024,     8, SECT_4K) },
	{"at45db041d",	   SF_INFO(0x1f2400, 0x0, 64 * 1024,     8, SECT_4K) },
	{"at45db081d",	   SF_INFO(0x1f2500, 0x0, 64 * 1024,    16, SECT_4K) },
	{"at45db161d",	   SF_INFO(0x1f2600, 0x0, 64 * 1024,    32, SECT_4K) },
	{"at45db321d",	   SF_INFO(0x1f2700, 0x0, 64 * 1024,    64, SECT_4K) },
	{"at45db641d",	   SF_INFO(0x1f2800, 0x0, 64 * 1024,   128, SECT_4K) },
	{"at25df321a",     SF_INFO(0x1f4701, 0x0, 64 * 1024,    64, SECT_4K) },
	{"at25df321",      SF_INFO(0x1f4700, 0x0, 64 * 1024,    64, SECT_4K) },
	{"at26df081a",     SF_INFO(0x1f4501, 0x0, 64 * 1024,    16, SECT_4K) },
#endif
#ifdef CONFIG_SPI_FLASH_EON		/* EON */
	{"en25q32b",	   SF_INFO(0x1c3016, 0x0, 64 * 1024,    64, 0) },
	{"en25q64",	   SF_INFO(0x1c3017, 0x0, 64 * 1024,   128, SECT_4K) },
	{"en25q128b",	   SF_INFO(0x1c3018, 0x0, 64 * 1024,   256, 0) },
	{"en25s64",	   SF_INFO(0x1c3817, 0x0, 64 * 1024,   128, 0) },
#endif
#ifdef CONFIG_SPI_FLASH_GIGADEVICE	/* GIGADEVICE */
	{"gd25q64b",	   SF_INFO(0xc84017, 0x0, 64 * 1024,   128, SECT_4K) },
	{"gd25lq32",	   SF_INFO(0xc86016, 0x0, 64 * 1024,    64, SECT_4K) },
#endif
#ifdef CONFIG_SPI_FLASH_ISSI		/* ISSI */
	{"is25lp032",	   SF_INFO(0x9d6016, 0x0, 64 * 1024,    64, 0) },
	{"is25lp064",	   SF_INFO(0x9d6017, 0x0, 64 * 1024,   128, 0) },
	{"is25lp128",	   SF_INFO(0x9d6018, 0x0, 64 * 1024,   256, 0) },
#endif
#ifdef CONFIG_SPI_FLASH_MACRONIX	/* MACRONIX */
	{"mx25l2006e",	   SF_INFO(0xc22012, 0x0, 64 * 1024,     4, 0) },
	{"mx25l4005",	   SF_INFO(0xc22013, 0x0, 64 * 1024,     8, 0) },
	{"mx25l8005",	   SF_INFO(0xc22014, 0x0, 64 * 1024,    16, 0) },
	{"mx25l1605d",	   SF_INFO(0xc22015, 0x0, 64 * 1024,    32, 0) },
	{"mx25l3205d",	   SF_INFO(0xc22016, 0x0, 64 * 1024,    64, 0) },
	{"mx25l6405d",	   SF_INFO(0xc22017, 0x0, 64 * 1024,   128, 0) },
	{"mx25l12805",	   SF_INFO(0xc22018, 0x0, 64 * 1024,   256, RD_FULL | WR_QPP) },
	{"mx25l25635f",	   SF_INFO(0xc22019, 0x0, 64 * 1024,   512, RD_FULL | WR_QPP) },
	{"mx25l51235f",	   SF_INFO(0xc2201a, 0x0, 64 * 1024,  1024, RD_FULL | WR_QPP) },
	{"mx25l12855e",	   SF_INFO(0xc22618, 0x0, 64 * 1024,   256, RD_FULL | WR_QPP) },
	{"mx66u51235f",    SF_INFO(0xc2253a, 0x0, 64 * 1024,  1024, RD_FULL | WR_QPP) },
	{"mx66l1g45g",     SF_INFO(0xc2201b, 0x0, 64 * 1024,  2048, RD_FULL | WR_QPP) },
#endif
#ifdef CONFIG_SPI_FLASH_SPANSION	/* SPANSION */
	{"s25fl008a",	   SF_INFO(0x010213, 0x0, 64 * 1024,    16, 0) },
	{"s25fl016a",	   SF_INFO(0x010214, 0x0, 64 * 1024,    32, 0) },
	{"s25fl032a",	   SF_INFO(0x010215, 0x0, 64 * 1024,    64, 0) },
	{"s25fl064a",	   SF_INFO(0x010216, 0x0, 64 * 1024,   128, 0) },
	{"s25fl116k",	   SF_INFO(0x014015, 0x0, 64 * 1024,   128, 0) },
	{"s25fl164k",	   SF_INFO(0x014017, 0x0140,  64 * 1024,   128, 0) },
	{"s25fl128p_256k", SF_INFO(0x012018, 0x0300, 256 * 1024,    64, RD_FULL | WR_QPP) },
	{"s25fl128p_64k",  SF_INFO(0x012018, 0x0301,  64 * 1024,   256, RD_FULL | WR_QPP) },
	{"s25fl032p",	   SF_INFO(0x010215, 0x4d00,  64 * 1024,    64, RD_FULL | WR_QPP) },
	{"s25fl064p",	   SF_INFO(0x010216, 0x4d00,  64 * 1024,   128, RD_FULL | WR_QPP) },
	{"s25fl128s_256k", SF_INFO(0x012018, 0x4d00, 256 * 1024,    64, RD_FULL | WR_QPP) },
	{"s25fl128s_64k",  SF_INFO(0x012018, 0x4d01,  64 * 1024,   256, RD_FULL | WR_QPP) },
	{"s25fl256s_256k", SF_INFO(0x010219, 0x4d00, 256 * 1024,   128, RD_FULL | WR_QPP) },
	{"s25fl256s_64k",  SF_INFO(0x010219, 0x4d01,  64 * 1024,   512, RD_FULL | WR_QPP) },
	{"s25fs256s_64k",  SF_INFO6(0x010219, 0x4d0181, 64 * 1024, 512, RD_FULL | WR_QPP | SECT_4K) },
	{"s25fs512s",      SF_INFO6(0x010220, 0x4d0081, 128 * 1024, 512, RD_FULL | WR_QPP | SECT_4K) },
	{"s25fl512s_256k", SF_INFO(0x010220, 0x4d00, 256 * 1024,   256, RD_FULL | WR_QPP) },
	{"s25fl512s_64k",  SF_INFO(0x010220, 0x4d01,  64 * 1024,  1024, RD_FULL | WR_QPP) },
	{"s25fl512s_512k", SF_INFO(0x010220, 0x4f00, 256 * 1024,   256, RD_FULL | WR_QPP) },
#endif
#ifdef CONFIG_SPI_FLASH_STMICRO		/* STMICRO */
	{"m25p10",	   SF_INFO(0x202011, 0x0, 32 * 1024,     4, 0) },
	{"m25p20",	   SF_INFO(0x202012, 0x0, 64 * 1024,     4, 0) },
	{"m25p40",	   SF_INFO(0x202013, 0x0, 64 * 1024,     8, 0) },
	{"m25p80",	   SF_INFO(0x202014, 0x0, 64 * 1024,    16, 0) },
	{"m25p16",	   SF_INFO(0x202015, 0x0, 64 * 1024,    32, 0) },
	{"m25pE16",	   SF_INFO(0x208015, 0x1000, 64 * 1024, 32, 0) },
	{"m25pX16",	   SF_INFO(0x207115, 0x1000, 64 * 1024, 32, RD_QUAD | RD_DUAL) },
	{"m25p32",	   SF_INFO(0x202016, 0x0,  64 * 1024,    64, 0) },
	{"m25p64",	   SF_INFO(0x202017, 0x0,  64 * 1024,   128, 0) },
	{"m25p128",	   SF_INFO(0x202018, 0x0, 256 * 1024,    64, 0) },
	{"m25pX64",	   SF_INFO(0x207117, 0x0,  64 * 1024,   128, SECT_4K) },
	{"n25q016a",       SF_INFO(0x20bb15, 0x0,	64 * 1024,    32, SECT_4K) },
	{"n25q32",	   SF_INFO(0x20ba16, 0x0,  64 * 1024,    64, RD_FULL | WR_QPP | SECT_4K) },
	{"n25q32a",	   SF_INFO(0x20bb16, 0x0,  64 * 1024,    64, RD_FULL | WR_QPP | SECT_4K) },
	{"n25q64",	   SF_INFO(0x20ba17, 0x0,  64 * 1024,   128, RD_FULL | WR_QPP | SECT_4K) },
	{"n25q64a",	   SF_INFO(0x20bb17, 0x0,  64 * 1024,   128, RD_FULL | WR_QPP | SECT_4K) },
	{"n25q128",	   SF_INFO(0x20ba18, 0x0,  64 * 1024,   256, RD_FULL | WR_QPP) },
	{"n25q128a",	   SF_INFO(0x20bb18, 0x0,  64 * 1024,   256, RD_FULL | WR_QPP) },
	{"n25q256",	   SF_INFO(0x20ba19, 0x0,  64 * 1024,   512, RD_FULL | WR_QPP | SECT_4K) },
	{"n25q256a",	   SF_INFO(0x20bb19, 0x0,  64 * 1024,   512, RD_FULL | WR_QPP | SECT_4K) },
	{"n25q512",	   SF_INFO(0x20ba20, 0x0,  64 * 1024,  1024, RD_FULL | WR_QPP | E_FSR | SECT_4K) },
	{"n25q512a",	   SF_INFO(0x20bb20, 0x0,  64 * 1024,  1024, RD_FULL | WR_QPP | E_FSR | SECT_4K) },
	{"n25q1024",	   SF_INFO(0x20ba21, 0x0,  64 * 1024,  2048, RD_FULL | WR_QPP | E_FSR | SECT_4K) },
	{"n25q1024a",	   SF_INFO(0x20bb21, 0x0,  64 * 1024,  2048, RD_FULL | WR_QPP | E_FSR | SECT_4K) },
	{"mt25qu02g",	   SF_INFO(0x20bb22, 0x0,  64 * 1024,  4096, RD_FULL | WR_QPP | E_FSR | SECT_4K) },
	{"mt25ql02g",	   SF_INFO(0x20ba22, 0x0,  64 * 1024,  4096, RD_FULL | WR_QPP | E_FSR | SECT_4K) },
#endif
#ifdef CONFIG_SPI_FLASH_SST		/* SST */
	{"sst25vf040b",	   SF_INFO(0xbf258d, 0x0,	64 * 1024,     8, SECT_4K | SST_WR) },
	{"sst25vf080b",	   SF_INFO(0xbf258e, 0x0,	64 * 1024,    16, SECT_4K | SST_WR) },
	{"sst25vf016b",	   SF_INFO(0xbf2541, 0x0,	64 * 1024,    32, SECT_4K | SST_WR) },
	{"sst25vf032b",	   SF_INFO(0xbf254a, 0x0,	64 * 1024,    64, SECT_4K | SST_WR) },
	{"sst25vf064c",	   SF_INFO(0xbf254b, 0x0,	64 * 1024,   128, SECT_4K) },
	{"sst25wf512",	   SF_INFO(0xbf2501, 0x0,	64 * 1024,     1, SECT_4K | SST_WR) },
	{"sst25wf010",	   SF_INFO(0xbf2502, 0x0,	64 * 1024,     2, SECT_4K | SST_WR) },
	{"sst25wf020",	   SF_INFO(0xbf2503, 0x0,	64 * 1024,     4, SECT_4K | SST_WR) },
	{"sst25wf040",	   SF_INFO(0xbf2504, 0x0,	64 * 1024,     8, SECT_4K | SST_WR) },
	{"sst25wf040b",	   SF_INFO(0x621613, 0x0,	64 * 1024,     8, SECT_4K) },
	{"sst25wf080",	   SF_INFO(0xbf2505, 0x0,	64 * 1024,    16, SECT_4K | SST_WR) },
#endif
#ifdef CONFIG_SPI_FLASH_WINBOND		/* WINBOND */
	{"w25p80",	   SF_INFO(0xef2014, 0x0,	64 * 1024,    16, 0) },
	{"w25p16",	   SF_INFO(0xef2015, 0x0,	64 * 1024,    32, 0) },
	{"w25p32",	   SF_INFO(0xef2016, 0x0,	64 * 1024,    64, 0) },
	{"w25x40",	   SF_INFO(0xef3013, 0x0,	64 * 1024,     8, SECT_4K) },
	{"w25x16",	   SF_INFO(0xef3015, 0x0,	64 * 1024,    32, SECT_4K) },
	{"w25x32",	   SF_INFO(0xef3016, 0x0,	64 * 1024,    64, SECT_4K) },
	{"w25x64",	   SF_INFO(0xef3017, 0x0,	64 * 1024,   128, SECT_4K) },
	{"w25q80bl",	   SF_INFO(0xef4014, 0x0,	64 * 1024,    16, RD_FULL | WR_QPP | SECT_4K) },
	{"w25q16cl",	   SF_INFO(0xef4015, 0x0,	64 * 1024,    32, RD_FULL | WR_QPP | SECT_4K) },
	{"w25q32bv",	   SF_INFO(0xef4016, 0x0,	64 * 1024,    64, RD_FULL | WR_QPP | SECT_4K) },
	{"w25q64cv",	   SF_INFO(0xef4017, 0x0,	64 * 1024,   128, RD_FULL | WR_QPP | SECT_4K) },
	{"w25q128bv",	   SF_INFO(0xef4018, 0x0,	64 * 1024,   256, RD_FULL | WR_QPP | SECT_4K) },
	{"w25q256",	   SF_INFO(0xef4019, 0x0,	64 * 1024,   512, RD_FULL | WR_QPP | SECT_4K) },
	{"w25q80bw",	   SF_INFO(0xef5014, 0x0,	64 * 1024,    16, RD_FULL | WR_QPP | SECT_4K) },
	{"w25q16dw",	   SF_INFO(0xef6015, 0x0,	64 * 1024,    32, RD_FULL | WR_QPP | SECT_4K) },
	{"w25q32dw",	   SF_INFO(0xef6016, 0x0,	64 * 1024,    64, RD_FULL | WR_QPP | SECT_4K) },
	{"w25q64dw",	   SF_INFO(0xef6017, 0x0,	64 * 1024,   128, RD_FULL | WR_QPP | SECT_4K) },
	{"w25q128fw",	   SF_INFO(0xef6018, 0x0,	64 * 1024,   256, RD_FULL | WR_QPP | SECT_4K) },
#endif
	{},	/* Empty entry to terminate the list */
	/*
	 * Note:
	 * Below paired flash devices has similar spi_flash params.
	 * (s25fl129p_64k, s25fl128s_64k)
	 * (w25q80bl, w25q80bv)
	 * (w25q16cl, w25q16dv)
	 * (w25q32bv, w25q32fv_spi)
	 * (w25q64cv, w25q64fv_spi)
	 * (w25q128bv, w25q128fv_spi)
	 * (w25q32dw, w25q32fv_qpi)
	 * (w25q64dw, w25q64fv_qpi)
	 * (w25q128fw, w25q128fv_qpi)
	 */
};

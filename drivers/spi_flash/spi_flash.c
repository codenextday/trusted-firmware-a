/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

#include "sf_internal.h"

struct spi_flash sf;

int spi_flash_read_common(struct spi_flash *flash, const uint8_t *cmd,
		size_t cmd_len, void *data, size_t data_len);

static int spi_flash_read_write(
				const uint8_t *cmd, size_t cmd_len,
				const uint8_t *data_out, uint8_t *data_in,
				size_t data_len)
{
	unsigned long flags = SPI_XFER_BEGIN;
	int ret;

	if (data_len == 0)
		flags |= SPI_XFER_END;

	ret = spi_xfer(cmd_len * 8, cmd, NULL, flags);
	if (ret) {
		ERROR("send cmd (%zu bytes): %d\n",
		      cmd_len, ret);
	} else if (data_len != 0) {
		ret = spi_xfer(data_len * 8, data_out, data_in,
					SPI_XFER_END);
		if (ret)
			ERROR("send data %zu B: %d\n",
			      data_len, ret);
	}

	return ret;
}

int spi_flash_cmd_read(const uint8_t *cmd,
		size_t cmd_len, void *data, size_t data_len)
{
	return spi_flash_read_write(cmd, cmd_len, NULL, data, data_len);
}

int spi_flash_cmd(uint8_t cmd, void *response, size_t len)
{
	return spi_flash_cmd_read(&cmd, 1, response, len);
}

int spi_flash_cmd_write(const uint8_t *cmd, size_t cmd_len,
		const void *data, size_t data_len)
{
	return spi_flash_read_write(cmd, cmd_len, data, NULL, data_len);
}


static void spi_flash_addr(uint32_t  addr, uint8_t *cmd)
{
	/* cmd[0] is actual command */
	cmd[1] = addr >> 16;
	cmd[2] = addr >> 8;
	cmd[3] = addr >> 0;
}

static int read_sr(struct spi_flash *flash, uint8_t *rs)
{
	int ret;
	uint8_t cmd;

	cmd = CMD_READ_STATUS;
	ret = spi_flash_read_common(flash, &cmd, 1, rs, 1);
	if (ret < 0) {
		ERROR("read sr\n");
		return ret;
	}

	return 0;
}

static int read_fsr(struct spi_flash *flash, uint8_t *fsr)
{
	int ret;
	const uint8_t cmd = CMD_FLAG_STATUS;

	ret = spi_flash_read_common(flash, &cmd, 1, fsr, 1);
	if (ret < 0) {
		ERROR("read fsr\n");
		return ret;
	}

	return 0;
}

static int write_sr(struct spi_flash *flash, uint8_t ws)
{
	uint8_t cmd;
	int ret;

	cmd = CMD_WRITE_STATUS;
	ret = spi_flash_write_common(flash, &cmd, 1, &ws, 1);
	if (ret < 0) {
		ERROR("write sr\n");
		return ret;
	}

	return 0;
}

#if defined(CONFIG_SPI_FLASH_SPANSION) || defined(CONFIG_SPI_FLASH_WINBOND)
static int read_cr(struct spi_flash *flash, uint8_t *rc)
{
	int ret;
	uint8_t cmd;

	cmd = CMD_READ_CONFIG;
	ret = spi_flash_read_common(flash, &cmd, 1, rc, 1);
	if (ret < 0) {
		ERROR("read cfg reg\n");
		return ret;
	}

	return 0;
}

static int write_cr(struct spi_flash *flash, uint8_t wc)
{
	uint8_t data[2];
	uint8_t cmd;
	int ret;

	ret = read_sr(flash, &data[0]);
	if (ret < 0)
		return ret;

	cmd = CMD_WRITE_STATUS;
	data[1] = wc;
	ret = spi_flash_write_common(flash, &cmd, 1, &data, 2);
	if (ret) {
		ERROR("SF: fail to write config register\n");
		return ret;
	}

	return 0;
}
#endif

#ifdef CONFIG_SPI_FLASH_BAR
static int write_bar(struct spi_flash *flash, uint32_t  offset)
{
	uint8_t cmd, bank_sel;
	int ret;

	bank_sel = offset / (SPI_FLASH_16MB_BOUN << flash->shift);
	if (bank_sel == flash->bank_curr)
		goto bar_end;

	cmd = flash->bank_write_cmd;
	ret = spi_flash_write_common(flash, &cmd, 1, &bank_sel, 1);
	if (ret < 0) {
		ERROR("SF: fail to write bank register\n");
		return ret;
	}

bar_end:
	flash->bank_curr = bank_sel;
	return flash->bank_curr;
}

static int read_bar(struct spi_flash *flash, const struct spi_flash_info *info)
{
	uint8_t curr_bank = 0;
	int ret;

	if (flash->size <= SPI_FLASH_16MB_BOUN)
		goto bar_end;

	switch (JEDEC_MFR(info)) {
	case SPI_FLASH_CFI_MFR_SPANSION:
		flash->bank_read_cmd = CMD_BANKADDR_BRRD;
		flash->bank_write_cmd = CMD_BANKADDR_BRWR;
		break;
	default:
		flash->bank_read_cmd = CMD_EXTNADDR_RDEAR;
		flash->bank_write_cmd = CMD_EXTNADDR_WREAR;
	}

	ret = spi_flash_read_common(flash, &flash->bank_read_cmd, 1,
				    &curr_bank, 1);
	if (ret) {
		ERROR("SF: fail to read bank addr register\n");
		return ret;
	}

bar_end:
	flash->bank_curr = curr_bank;
	return 0;
}
#endif

#ifdef CONFIG_SF_DUAL_FLASH
static void spi_flash_dual(struct spi_flash *flash, uint32_t  *addr)
{
	switch (flash->dual_flash) {
	case SF_DUAL_STACKED_FLASH:
		if (*addr >= (flash->size >> 1)) {
			*addr -= flash->size >> 1;
			flash->flags |= SNOR_F_USE_UPAGE;
		} else {
			flash->flags &= ~SNOR_F_USE_UPAGE;
		}
		break;
	case SF_DUAL_PARALLEL_FLASH:
		*addr >>= flash->shift;
		break;
	default:
		ERROR("SF: Unsupported dual_flash=%d\n", flash->dual_flash);
		break;
	}
}
#endif

static int spi_flash_sr_ready(struct spi_flash *flash)
{
	uint8_t sr;
	int ret;

	ret = read_sr(flash, &sr);
	if (ret < 0)
		return ret;

	return !(sr & STATUS_WIP);
}

static int spi_flash_fsr_ready(struct spi_flash *flash)
{
	uint8_t fsr;
	int ret;

	ret = read_fsr(flash, &fsr);
	if (ret < 0)
		return ret;

	return fsr & STATUS_PEC;
}

static int spi_flash_ready(struct spi_flash *flash)
{
	int sr, fsr;

	sr = spi_flash_sr_ready(flash);
	if (sr < 0)
		return sr;

	fsr = 1;
	if (flash->flags & SNOR_F_USE_FSR) {
		fsr = spi_flash_fsr_ready(flash);
		if (fsr < 0)
			return fsr;
	}

	return sr && fsr;
}

static int spi_flash_wait_till_ready(struct spi_flash *flash,
				     unsigned long timeout)
{
	int ret;

	timeout = timeout * 10;
	while (timeout--) {
		ret = spi_flash_ready(flash);
		if (ret < 0)
			return ret;
		if (ret)
			return 0;
		udelay(100);
	}

	ERROR("SF: Timeout!\n");

	return -ETIMEDOUT;
}

int spi_flash_write_common(struct spi_flash *flash, const uint8_t *cmd,
		size_t cmd_len, const void *buf, size_t buf_len)
{
	struct spi_slave *spi = flash->spi;
	unsigned long timeout = SPI_FLASH_PROG_TIMEOUT;
	int ret = 0;

	if (buf == NULL)
		timeout = SPI_FLASH_PAGE_ERASE_TIMEOUT;

	/*ret = spi_claim_bus(spi);
	if (ret) {
		ERROR("SF: unable to claim SPI bus\n");
		return ret;
	}*/

	ret = spi_flash_cmd_write_enable(flash);
	if (ret < 0) {
		ERROR("en w failed\n");
		return ret;
	}

	ret = spi_flash_cmd_write(cmd, cmd_len, buf, buf_len);
	if (ret < 0) {
		ERROR("w cmd failed\n");
		return ret;
	}

	ret = spi_flash_wait_till_ready(flash, timeout);
	if (ret < 0) {
		ERROR("write %s timed out\n",
		      timeout == SPI_FLASH_PROG_TIMEOUT ?
			"program" : "page erase");
		return ret;
	}

	spi_release_bus(spi);

	return ret;
}

int spi_flash_cmd_erase_ops(struct spi_flash *flash, uint32_t  offset, size_t len)
{
	uint32_t  erase_size, erase_addr;
	uint8_t cmd[SPI_FLASH_CMD_LEN];
	int ret = -1;

	erase_size = flash->erase_size;
	if (offset % erase_size || len % erase_size) {
		ERROR("Unalign\n");
		return -1;
	}

	if (flash->flash_is_locked) {
		if (flash->flash_is_locked(flash, offset, len) > 0) {
			ERROR("offset 0x%x is protected\n",
			       offset);
			return -EINVAL;
		}
	}

	cmd[0] = flash->erase_cmd;
	while (len) {
		erase_addr = offset;

#ifdef CONFIG_SF_DUAL_FLASH
		if (flash->dual_flash > SF_SINGLE_FLASH)
			spi_flash_dual(flash, &erase_addr);
#endif
#ifdef CONFIG_SPI_FLASH_BAR
		ret = write_bar(flash, erase_addr);
		if (ret < 0)
			return ret;
#endif
		spi_flash_addr(erase_addr, cmd);

		INFO("erase %2x %2x %2x %2x (%x)\n", cmd[0], cmd[1],
		      cmd[2], cmd[3], erase_addr);

		ret = spi_flash_write_common(flash, cmd, sizeof(cmd), NULL, 0);
		if (ret < 0) {
			ERROR("erase failed\n");
			break;
		}

		offset += erase_size;
		len -= erase_size;
	}

	return ret;
}

int spi_flash_cmd_write_ops(uint32_t  offset,
		size_t len, const void *buf)
{
	struct spi_flash *flash = &sf;
	struct spi_slave *spi = flash->spi;
	unsigned long byte_addr, page_size;
	uint32_t  write_addr;
	size_t chunk_len, actual;
	uint8_t cmd[SPI_FLASH_CMD_LEN];
	int ret = -1;

	page_size = flash->page_size;

	if (flash->flash_is_locked) {
		if (flash->flash_is_locked(flash, offset, len) > 0) {
			ERROR("offset 0x%x is protected\n",
			       offset);
			return -EINVAL;
		}
	}

	cmd[0] = flash->write_cmd;
	for (actual = 0; actual < len; actual += chunk_len) {
		write_addr = offset;

#ifdef CONFIG_SF_DUAL_FLASH
		if (flash->dual_flash > SF_SINGLE_FLASH)
			spi_flash_dual(flash, &write_addr);
#endif
#ifdef CONFIG_SPI_FLASH_BAR
		ret = write_bar(flash, write_addr);
		if (ret < 0)
			return ret;
#endif
		byte_addr = offset % page_size;
		chunk_len = MIN(len - actual, (size_t)(page_size - byte_addr));

		if (spi->max_write_size)
			chunk_len = MIN(chunk_len,
					(size_t)spi->max_write_size);

		spi_flash_addr(write_addr, cmd);

		INFO("SF: 0x%p => cmd = { 0x%02x 0x%02x%02x%02x } chunk_len = %zu\n",
		      buf + actual, cmd[0], cmd[1], cmd[2], cmd[3], chunk_len);

		ret = spi_flash_write_common(flash, cmd, sizeof(cmd),
					buf + actual, chunk_len);
		if (ret < 0) {
			ERROR("write failed\n");
			break;
		}

		offset += chunk_len;
	}

	return ret;
}

int spi_flash_read_common(struct spi_flash *flash, const uint8_t *cmd,
		size_t cmd_len, void *data, size_t data_len)
{
	int ret;

	/*ret = spi_claim_bus();
	if (ret) {
		ERROR("SF: unable to claim SPI bus\n");
		return ret;
	}*/

	ret = spi_flash_cmd_read(cmd, cmd_len, data, data_len);
	if (ret < 0) {
		ERROR("r cmd failed\n");
		return ret;
	}

	spi_release_bus();

	return ret;
}

/*
 * TODO: remove the weak after all the other spi_flash_copy_mmap
 * implementations removed from drivers
 */
void spi_flash_copy_mmap(void *data, void *offset, size_t len)
{
	memcpy(data, offset, len);
}

size_t spi_flash_cmd_read_ops(uint32_t  offset, size_t len, void *data)
{
	struct spi_flash *flash = &sf;
	//struct spi_slave *spi = flash->spi;
	uint8_t cmd[8], cmdsz;
	uint32_t  remain_len, read_len, read_addr;
	uint32_t  len_have_read = 0;
	int bank_sel = 0;
	int ret = -1;

	NOTICE("sf_r cmd(%x - %d) addr(%x) len(%lx) buf(%p)\n", flash->read_cmd, flash->dummy_byte, offset, len, data);
	cmdsz = SPI_FLASH_CMD_LEN + flash->dummy_byte;
	
	cmd[0] = flash->read_cmd;
	while (len) {
		read_addr = offset;

		remain_len = ((SPI_FLASH_16MB_BOUN << flash->shift) *
				(bank_sel + 1)) - offset;
		if (len < remain_len)
			read_len = len;
		else
			read_len = remain_len;

		spi_flash_addr(read_addr, cmd);

		ret = spi_flash_read_common(flash, cmd, cmdsz, data, read_len);
		if (ret < 0) {
			ERROR("read failed\n");
			break;
		}

		offset += read_len;
		len -= read_len;
		data += read_len;
		len_have_read += read_len;
	}
	NOTICE("0x%x bytes\n", len_have_read);
	return len_have_read;
}

#ifdef CONFIG_SPI_FLASH_MACRONIX
static int macronix_quad_enable(struct spi_flash *flash)
{
	uint8_t qeb_status;
	int ret;

	ret = read_sr(flash, &qeb_status);
	if (ret < 0)
		return ret;

	if (qeb_status & STATUS_QEB_MXIC)
		return 0;

	ret = write_sr(flash, qeb_status | STATUS_QEB_MXIC);
	if (ret < 0)
		return ret;

	/* read SR and check it */
	ret = read_sr(flash, &qeb_status);
	if (!(ret >= 0 && (qeb_status & STATUS_QEB_MXIC))) {
		ERROR("Macronix SR Quad bit not clear\n");
		return -EINVAL;
	}

	return ret;
}
#endif

#if defined(CONFIG_SPI_FLASH_SPANSION) || defined(CONFIG_SPI_FLASH_WINBOND)
static int spansion_quad_enable(struct spi_flash *flash)
{
	uint8_t qeb_status;
	int ret;

	ret = read_cr(flash, &qeb_status);
	if (ret < 0)
		return ret;

	if (qeb_status & STATUS_QEB_WINSPAN)
		return 0;

	ret = write_cr(flash, qeb_status | STATUS_QEB_WINSPAN);
	if (ret < 0)
		return ret;

	/* read CR and check it */
	ret = read_cr(flash, &qeb_status);
	if (!(ret >= 0 && (qeb_status & STATUS_QEB_WINSPAN))) {
		ERROR("SF: Spansion CR Quad bit not clear\n");
		return -EINVAL;
	}

	return ret;
}
#endif

static const struct spi_flash_info *spi_flash_read_id(void)
{
	int				tmp;
	uint8_t			id[SPI_FLASH_MAX_ID_LEN];
	const struct spi_flash_info	*info;

	NOTICE("spi_flash_read_id\n");
	tmp = spi_flash_cmd(CMD_READ_ID, id, SPI_FLASH_MAX_ID_LEN);
	if (tmp < 0) {
		ERROR("error %d read ID\n", tmp);
		return NULL;
	}

	info = spi_flash_ids;
	for (; info->name != NULL; info++) {
		if (info->id_len) {
			if (!memcmp(info->id, id, info->id_len))
				return info;
		}
	}

	INFO("unrecognized id: %02x, %02x, %02x\n",
	       id[0], id[1], id[2]);
	return NULL;
}

static int set_quad_mode(struct spi_flash *flash,
			 const struct spi_flash_info *info)
{
	switch (JEDEC_MFR(info)) {
#ifdef CONFIG_SPI_FLASH_MACRONIX
	case SPI_FLASH_CFI_MFR_MACRONIX:
		return macronix_quad_enable(flash);
#endif
#if defined(CONFIG_SPI_FLASH_SPANSION) || defined(CONFIG_SPI_FLASH_WINBOND)
	case SPI_FLASH_CFI_MFR_SPANSION:
	case SPI_FLASH_CFI_MFR_WINBOND:
		return spansion_quad_enable(flash);
#endif
#ifdef CONFIG_SPI_FLASH_STMICRO
	case SPI_FLASH_CFI_MFR_STMICRO:
		ERROR("SF: QEB is volatile for %02x flash\n", JEDEC_MFR(info));
		return 0;
#endif
	default:
		ERROR("Need QEB func for %02x flash\n",
		       JEDEC_MFR(info));
		return -1;
	}
}

int spi_flash_scan(struct spi_flash *flash)
{
	//struct spi_slave *spi = flash->spi;
	const struct spi_flash_info *info = NULL;
	int ret, timeout = 5;

	while (!info) {
		info = spi_flash_read_id();
		if (timeout-- == 0) {
			ERROR("read id timeout\n");
			return  -ENODEV;
		}
	}

	NOTICE("scan id %x %x\n", JEDEC_MFR(info), JEDEC_ID(info));
	/* Flash powers up read-only, so clear BP# bits */
	if (JEDEC_MFR(info) == SPI_FLASH_CFI_MFR_ATMEL ||
	    JEDEC_MFR(info) == SPI_FLASH_CFI_MFR_MACRONIX ||
	    JEDEC_MFR(info) == SPI_FLASH_CFI_MFR_SST)
		write_sr(flash, 0);


	/* Compute the flash size */
	flash->shift = (flash->dual_flash & SF_DUAL_PARALLEL_FLASH) ? 1 : 0;
	flash->page_size = info->page_size;
	/*
	 * The Spansion S25FL032P and S25FL064P have 256b pages, yet use the
	 * 0x4d00 Extended JEDEC code. The rest of the Spansion flashes with
	 * the 0x4d00 Extended JEDEC code have 512b pages. All of the others
	 * have 256b pages.
	 */
	if (JEDEC_EXT(info) == 0x4d00) {
		if ((JEDEC_ID(info) != 0x0215) &&
		    (JEDEC_ID(info) != 0x0216))
			flash->page_size = 512;
	}
	flash->page_size <<= flash->shift;
	flash->sector_size = info->sector_size << flash->shift;
	flash->size = flash->sector_size * info->n_sectors << flash->shift;
#ifdef CONFIG_SF_DUAL_FLASH
	if (flash->dual_flash & SF_DUAL_STACKED_FLASH)
		flash->size <<= 1;
#endif

#ifdef CONFIG_SPI_FLASH_USE_4K_SECTORS
	/* Compute erase sector and command */
	if (info->flags & SECT_4K) {
		flash->erase_cmd = CMD_ERASE_4K;
		flash->erase_size = 4096 << flash->shift;
	} else
#endif
	{
		flash->erase_cmd = CMD_ERASE_64K;
		flash->erase_size = flash->sector_size;
	}

	/* Now erase size becomes valid sector size */
	flash->sector_size = flash->erase_size;

	/* Look for read commands */
	flash->read_cmd = CMD_READ_ARRAY_FAST;

	/* Go for default supported write cmd */
	flash->write_cmd = CMD_PAGE_PROGRAM;

	/* Set the quad enable bit - only for quad commands */
	if ((flash->read_cmd == CMD_READ_QUAD_OUTPUT_FAST) ||
	    (flash->read_cmd == CMD_READ_QUAD_IO_FAST) ||
	    (flash->write_cmd == CMD_QUAD_PAGE_PROGRAM)) {
		ret = set_quad_mode(flash, info);
		if (ret) {
			ERROR("Fail to set QEB for %02x\n",
			      JEDEC_MFR(info));
			return -EINVAL;
		}
	}

	/* Read dummy_byte: dummy byte is determined based on the
	 * dummy cycles of a particular command.
	 * Fast commands - dummy_byte = dummy_cycles/8
	 * I/O commands- dummy_byte = (dummy_cycles * no.of lines)/8
	 * For I/O commands except cmd[0] everything goes on no.of lines
	 * based on particular command but incase of fast commands except
	 * data all go on single line irrespective of command.
	 */
	switch (flash->read_cmd) {
	case CMD_READ_QUAD_IO_FAST:
		flash->dummy_byte = 2;
		break;
	case CMD_READ_ARRAY_SLOW:
		flash->dummy_byte = 0;
		break;
	default:
		flash->dummy_byte = 1;
	}

	return 0;
}

static int spi_flash_probe_slave(void)
{
	int ret;
	struct spi_flash *flash = &sf;
	ret = spi_flash_scan(flash);
	if (ret){
		ERROR("spi_flash_scan failed\n");
		return ret;
	}
	return ret;
}

int spi_flash_init(unsigned int busnum, unsigned int cs,
		unsigned int max_hz, unsigned int spi_mode)
{
	int ret;
	NOTICE("sf init\n");
//	ret = spi_initial(cs, max_hz,spi_mode);
	ret = spi_flash_probe_slave();
	/*ret = spi_setup_slave(busnum, cs, max_hz, spi_mode);*/
	if (ret)
		return ret;
	return 0;
}

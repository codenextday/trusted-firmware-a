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
#include <drivers/xilinx/spi/spi.h>
#include <drivers/spi_mem.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <lib/libc/assert.h>
#include <drivers/delay_timer.h>

/*
 * [0]: http://www.xilinx.com/support/documentation
 *
 * Xilinx SPI Register Definitions
 * [1]:	[0]/ip_documentation/xps_spi.pdf
 *	page 8, Register Descriptions
 * [2]:	[0]/ip_documentation/axi_spi_ds742.pdf
 *	page 7, Register Overview Table
 */

/* SPI Control Register (spicr), [1] p9, [2] p8 */
#define SPICR_LSB_FIRST		BIT(9)
#define SPICR_MASTER_INHIBIT	BIT(8)
#define SPICR_MANUAL_SS		BIT(7)
#define SPICR_RXFIFO_RESEST	BIT(6)
#define SPICR_TXFIFO_RESEST	BIT(5)
#define SPICR_CPHA		BIT(4)
#define SPICR_CPOL		BIT(3)
#define SPICR_MASTER_MODE	BIT(2)
#define SPICR_SPE		BIT(1)
#define SPICR_LOOP		BIT(0)

/* SPI Status Register (spisr), [1] p11, [2] p10 */
#define SPISR_COMMAND_ERR	BIT(10)
#define SPISR_LOOPBACK_ERR	BIT(9)
#define SPISR_MSB_ERR		BIT(8)
#define SPISR_SLAVE_MODE_ERR	BIT(7)
#define SPISR_CPOL_CPHA_ERR	BIT(6)
#define SPISR_SLAVE_MODE_SELECT	BIT(5)
#define SPISR_MODF		BIT(4)
#define SPISR_TX_FULL		BIT(3)
#define SPISR_TX_EMPTY		BIT(2)
#define SPISR_RX_FULL		BIT(1)
#define SPISR_RX_EMPTY		BIT(0)

/* SPI Data Transmit Register (spidtr), [1] p12, [2] p12 */
#define SPIDTR_8BIT_MASK	GENMASK(7, 0)
#define SPIDTR_16BIT_MASK	GENMASK(15, 0)
#define SPIDTR_32BIT_MASK	GENMASK(31, 0)

/* SPI Data Receive Register (spidrr), [1] p12, [2] p12 */
#define SPIDRR_8BIT_MASK	GENMASK(7, 0)
#define SPIDRR_16BIT_MASK	GENMASK(15, 0)
#define SPIDRR_32BIT_MASK	GENMASK(31, 0)

/* SPI Slave Select Register (spissr), [1] p13, [2] p13 */
#define SPISSR_MASK(cs)		(1 << (cs))
#define SPISSR_ACT(cs)		~SPISSR_MASK(cs)
#define SPISSR_OFF		0xffffffff

/* SPI Software Reset Register (ssr) */
#define SPISSR_RESET_VALUE	0x0a

#define XILSPI_MAX_XFER_BITS	8
#define XILSPI_SPICR_DFLT_ON	(SPICR_MANUAL_SS | SPICR_MASTER_MODE | \
				SPICR_SPE)
#define XILSPI_SPICR_DFLT_OFF	(SPICR_MASTER_INHIBIT | SPICR_MANUAL_SS)

#ifndef CONFIG_XILINX_SPI_IDLE_VAL
#define CONFIG_XILINX_SPI_IDLE_VAL	GENMASK(7, 0)
#endif

#ifndef CONFIG_SYS_XILINX_SPI_LIST
#define CONFIG_SYS_XILINX_SPI_LIST	{ CONFIG_SYS_SPI_BASE }
#endif

/* xilinx spi register set */
struct xilinx_spi_regs {
	uint32_t __space0__[7];
	uint32_t dgier;	/* Device Global Interrupt Enable Register (DGIER) */
	uint32_t ipisr;	/* IP Interrupt Status Register (IPISR) */
	uint32_t __space1__;
	uint32_t ipier;	/* IP Interrupt Enable Register (IPIER) */
	uint32_t __space2__[5];
	uint32_t srr;	/* Softare Reset Register (SRR) */
	uint32_t __space3__[7];
	uint32_t spicr;	/* SPI Control Register (SPICR) */
	uint32_t spisr;	/* SPI Status Register (SPISR) */
	uint32_t spidtr;	/* SPI Data Transmit Register (SPIDTR) */
	uint32_t spidrr;	/* SPI Data Receive Register (SPIDRR) */
	uint32_t spissr;	/* SPI Slave Select Register (SPISSR) */
	uint32_t spitfor;	/* SPI Transmit FIFO Occupancy Register (SPITFOR) */
	uint32_t spirfor;	/* SPI Receive FIFO Occupancy Register (SPIRFOR) */
};

/* xilinx spi priv */
struct xilinx_spi_priv {
	struct xilinx_spi_regs *regs;
	unsigned int freq;
	unsigned int mode;
};

struct xilinx_spi_priv xilinx_spi;

void spi_cs_activate(uint32_t cs)
{
	struct xilinx_spi_priv *priv = &xilinx_spi;
	struct xilinx_spi_regs *regs = priv->regs;

	mmio_write_32((unsigned long)&regs->spissr, SPISSR_ACT(cs));
}

void spi_cs_deactivate(void)
{
	struct xilinx_spi_priv *priv = &xilinx_spi;
	struct xilinx_spi_regs *regs = priv->regs;

	mmio_write_32((unsigned long)&regs->spissr, SPISSR_OFF);
}

static int xilinx_spi_claim_bus(unsigned int cs)
{
	struct xilinx_spi_priv *priv = &xilinx_spi;
	struct xilinx_spi_regs *regs = priv->regs;

	mmio_write_32((unsigned long)&regs->spissr, SPISSR_OFF);
	mmio_write_32((unsigned long)&regs->spicr, XILSPI_SPICR_DFLT_ON);

	return 0;
}

void xilinx_buffer_print(void *data, unsigned int len)
{
	unsigned int *buf = data;
	int i = 0;
	if (!len)
		return;
	if (len < 4) {
		unsigned char *d = data;
		printf("\n%p:\t ", d);
		do {
			printf("%x ", d[i]);
		} while (i++ < len);
		printf("\n");
		return;
	}

	len = len >> 2;
	if (!len)
		return;
	len = (len <= 8) ? len : 8;
	for (i = 0; i < len; i++) {
		if (!(i % 4)) {
			printf("\n%p:\t", &buf[i]);
		}
		printf("%x ", buf[i]);
	}
}
int xilinx_spi_xfer(unsigned int bitlen, const void *dout,
                           void *din, unsigned long flags)
{
	struct xilinx_spi_priv *priv = &xilinx_spi;
	struct xilinx_spi_regs *regs = priv->regs;
	/* assume spi core configured to do 8 bit transfers */
	unsigned int bytes = bitlen / XILSPI_MAX_XFER_BITS;
	const unsigned char *txp = dout;
	unsigned char *rxp = din;
	unsigned rxecount = 17;	/* max. 16 elements in FIFO, leftover 1 */
	unsigned global_timeout;

	INFO("xilinx_spi_xfer, din %p, dout %p, bitlen %d, flags 0x%lx\n", din, dout, bitlen, flags);

	if (bitlen == 0)
		goto done;

	if (bitlen % XILSPI_MAX_XFER_BITS) {
		ERROR("Not a multiple of %d bits\n",
		       XILSPI_MAX_XFER_BITS);
		flags |= SPI_XFER_END;
		goto done;
	}

	/* empty read buffer */
	while (rxecount && !(mmio_read_32((unsigned long)&regs->spisr) & SPISR_RX_EMPTY)) {
		mmio_read_32((unsigned long)&regs->spidrr);
		rxecount--;
	}

	if (!rxecount) {
		ERROR("Rx buffer not empty\n");
		return -1;
	}

	if (flags & SPI_XFER_BEGIN)
		spi_cs_activate(0);

	/* at least 1usec or greater, leftover 1 */
	global_timeout = priv->freq > XILSPI_MAX_XFER_BITS * 1000000 ? 2 :
			(XILSPI_MAX_XFER_BITS * 1000000 / priv->freq) + 1;

	while (bytes--) {
		unsigned timeout = global_timeout;
		/* get Tx element from data out buffer and count up */
		unsigned char d = txp ? *txp++ : CONFIG_XILINX_SPI_IDLE_VAL;
		INFO("spi_xfer: tx:%x ", d);

		/* write out and wait for processing (receive data) */
		mmio_write_32((unsigned long)&regs->spidtr, d & SPIDTR_8BIT_MASK);
		while (timeout && mmio_read_32((unsigned long)&regs->spisr)
						& SPISR_RX_EMPTY) {
			timeout--;
			udelay(1);
		}

		if (!timeout) {
			ERROR("Xfer timeout\n");
			return -1;
		}

		/* read Rx element and push into data in buffer */
		d = mmio_read_32((unsigned long)&regs->spidrr) & SPIDRR_8BIT_MASK;
		if (rxp)
			*rxp++ = d;
		INFO("spi_xfer: rx:%x\n", d);
	}
	/*if (din)
		xilinx_buffer_print(din, bitlen / XILSPI_MAX_XFER_BITS);*/

 done:
	if (flags & SPI_XFER_END)
		spi_cs_deactivate();

	return 0;
}
static int xilinx_spi_set_mode(unsigned int mode)
{
	struct xilinx_spi_priv *priv = &xilinx_spi;
	struct xilinx_spi_regs *regs = priv->regs;
	unsigned int spicr;
	INFO("xilinx_spi_set_mode\n");
	spicr = mmio_read_32((unsigned long)&regs->spicr);
	if (mode & SPI_LSB_FIRST)
		spicr |= SPICR_LSB_FIRST;
	if (mode & SPI_CPHA)
		spicr |= SPICR_CPHA;
	if (mode & SPI_CPOL)
		spicr |= SPICR_CPOL;
	/*if (mode & SPI_LOOP)
		spicr |= SPICR_LOOP;*/

	mmio_write_32((unsigned long)&regs->spicr, spicr);
	priv->mode = mode;

	INFO("xilinx_spi_set_mode: regs=%p, mode=%d\n", priv->regs,
	      priv->mode);

	return 0;
}


#ifdef IMAGE_IN_MTD
static void xilinx_spi_release_bus(void)
{
}

int xilinx_spi_exec_op(const struct spi_mem_op *op)
{
	//unsigned long mode = 0;
	int ret, i;
	uint8_t *din, *dout;
	uint8_t cmd[8], cmdsz;
	unsigned long flags = SPI_XFER_BEGIN;

	INFO("%s: cmd:%x mode:%d.%d.%d.%d addr:%llx len:%x\n",
		__func__, op->cmd.opcode, op->cmd.buswidth, op->addr.buswidth,
		op->dummy.buswidth, op->data.buswidth,
		op->addr.val, op->data.nbytes);
	if (op->data.dir == SPI_MEM_DATA_IN) {
		din = op->data.buf;
		dout = NULL;
	} else if (op->data.dir == SPI_MEM_DATA_OUT) {
		din = NULL;
		dout = op->data.buf;
	} else {
		return -EINVAL;
	}
	if (!op->data.nbytes)
		flags |= SPI_XFER_END;

	cmdsz = op->addr.nbytes + op->dummy.nbytes + 1;
	cmd[0] = op->cmd.opcode;
	for (i = 1; i <= op->addr.nbytes; i++) {
		cmd[i] = op->addr.val >> ((op->addr.nbytes - i) << 3);
	}
	ret = xilinx_spi_xfer(cmdsz << 3, cmd, NULL, flags);
	if (ret) {
		ERROR("%s: failed to send cmd %x\n", __func__, op->cmd.opcode);
		return ret;
	} else if (op->data.nbytes) {
		ret = xilinx_spi_xfer(op->data.nbytes << 3, dout, din, SPI_XFER_END);
	}

	return ret;

}
#endif

int  spi_xfer(unsigned int bitlen, const void *dout,
		void *din, unsigned long flags)
{
	return xilinx_spi_xfer(bitlen, dout, din, flags);
}

void spi_release_bus(void)
{
}
int xilinx_spi_set_speed(unsigned int hz)
{
	return 0;
}
#ifdef IMAGE_IN_MTD
const struct spi_bus_ops xilinx_spi_ops = {
	.claim_bus = xilinx_spi_claim_bus,
	.release_bus = xilinx_spi_release_bus,
	.exec_op = xilinx_spi_exec_op,
	.set_mode = xilinx_spi_set_mode,
	.set_speed = xilinx_spi_set_speed,
};
int spi_initial(unsigned int cs, unsigned int max_hz, unsigned int mode)
{
	struct xilinx_spi_priv *priv = &xilinx_spi;
	struct xilinx_spi_regs *regs;

	NOTICE("spi init for image in mtd\n");
	priv->regs = (struct xilinx_spi_regs *)XILINX_SPI_BASE;
	regs = priv->regs;
	mmio_write_32((unsigned long)&regs->srr, SPISSR_RESET_VALUE);
	priv->freq = XILINX_SPI_FREQ;
	spi_mem_init_slave(cs, max_hz, mode, &xilinx_spi_ops);
	return 0;
}
#elif defined(IMAGE_IN_SF)
int spi_initial(unsigned int cs, unsigned int max_hz, unsigned int mode)
{
	struct xilinx_spi_priv *priv = &xilinx_spi;
	struct xilinx_spi_regs *regs;

	NOTICE("spi init sf\n");
	udelay(1);
	priv->regs = (struct xilinx_spi_regs *)XILINX_SPI_BASE;
	regs = priv->regs;

	mmio_write_32((unsigned long)&regs->srr, SPISSR_RESET_VALUE);
	priv->freq = XILINX_SPI_FREQ;
	xilinx_spi_claim_bus(0);
	xilinx_spi_set_mode(mode);
	return 0;
}
#else
int spi_initial(unsigned int cs, unsigned int max_hz, unsigned int mode)
{
	struct xilinx_spi_priv *priv = &xilinx_spi;
	struct xilinx_spi_regs *regs;

	NOTICE("spi init ram\n");
	udelay(1);
	priv->regs = (struct xilinx_spi_regs *)XILINX_SPI_BASE;
	regs = priv->regs;

	mmio_write_32((unsigned long)&regs->srr, SPISSR_RESET_VALUE);
	priv->freq = XILINX_SPI_FREQ;
	xilinx_spi_claim_bus(0);
	xilinx_spi_set_mode(mode);
	return 0;
}
#endif

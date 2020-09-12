/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef IO_SF_H
#define IO_SF_H

#include <drivers/io/io_storage.h>

/* block devices ops */
typedef struct io_sf_ops {
	size_t	(*read)(unsigned int offset, uintptr_t buf, size_t size);
	size_t	(*write)(unsigned int offset, const uintptr_t buf, size_t size);
} io_sf_ops_t;

typedef struct io_sf_dev_spec {
	io_block_spec_t	buffer;
	io_sf_ops_t	ops;
	size_t		page_size;
} io_sf_dev_spec_t;

struct io_dev_connector;

int register_io_dev_sf(const struct io_dev_connector **dev_con);
size_t spi_flash_cmd_read_ops(uint32_t offset,
		size_t len, void *data);

#endif /* IO_SF_H */

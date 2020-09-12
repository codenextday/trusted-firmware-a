/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <platform_def.h>

#include <common/debug.h>
#include <drivers/io/io_block.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_storage.h>
#include <drivers/io/io_sf.h>
#include <lib/utils.h>

typedef struct {
	io_sf_dev_spec_t	*dev_spec;
	uintptr_t		base;
	size_t			file_pos;
	size_t			size;
} sf_dev_state_t;

#define is_power_of_2(x)	((x != 0) && ((x & (x - 1)) == 0))

io_type_t device_type_block(void);

static int sf_open(io_dev_info_t *dev_info, const uintptr_t spec,
		      io_entity_t *entity);
static int sf_seek(io_entity_t *entity, int mode, signed long long offset);
static int sf_read(io_entity_t *entity, uintptr_t buffer, size_t length,
		      size_t *length_read);
static int sf_write(io_entity_t *entity, const uintptr_t buffer,
		       size_t length, size_t *length_written);
static int sf_close(io_entity_t *entity);
static int sf_dev_open(const uintptr_t dev_spec, io_dev_info_t **dev_info);
static int sf_dev_close(io_dev_info_t *dev_info);

static const io_dev_connector_t sf_dev_connector = {
	.dev_open	= sf_dev_open
};

static const io_dev_funcs_t sf_dev_funcs = {
	.type		= device_type_block,
	.open		= sf_open,
	.seek		= sf_seek,
	.size		= NULL,
	.read		= sf_read,
	.write		= sf_write,
	.close		= sf_close,
	.dev_init	= NULL,
	.dev_close	= sf_dev_close,
};

static sf_dev_state_t state_pool[MAX_IO_BLOCK_DEVICES];
static io_dev_info_t dev_info_pool[MAX_IO_BLOCK_DEVICES];

/* Track number of allocated block state */
static unsigned int sf_dev_count;

io_type_t device_type_block(void)
{
	return IO_TYPE_SF;
}

/* Locate a block state in the pool, specified by address */
static int find_first_sf_state(const io_sf_dev_spec_t *dev_spec,
				  unsigned int *index_out)
{
	unsigned int index;
	int result = -ENOENT;

	for (index = 0U; index < MAX_IO_BLOCK_DEVICES; ++index) {
		/* dev_spec is used as identifier since it's unique */
		if (state_pool[index].dev_spec == dev_spec) {
			result = 0;
			*index_out = index;
			break;
		}
	}
	return result;
}

/* Allocate a device info from the pool and return a pointer to it */
static int allocate_dev_info(io_dev_info_t **dev_info)
{
	int result = -ENOMEM;
	assert(dev_info != NULL);

	if (sf_dev_count < MAX_IO_BLOCK_DEVICES) {
		unsigned int index = 0;
		result = find_first_sf_state(NULL, &index);
		assert(result == 0);
		/* initialize dev_info */
		dev_info_pool[index].funcs = &sf_dev_funcs;
		dev_info_pool[index].info = (uintptr_t)&state_pool[index];
		*dev_info = &dev_info_pool[index];
		++sf_dev_count;
	}

	return result;
}


/* Release a device info to the pool */
static int free_dev_info(io_dev_info_t *dev_info)
{
	int result;
	unsigned int index = 0;
	sf_dev_state_t *state;
	assert(dev_info != NULL);

	state = (sf_dev_state_t *)dev_info->info;
	result = find_first_sf_state(state->dev_spec, &index);
	if (result ==  0) {
		/* free if device info is valid */
		zeromem(state, sizeof(sf_dev_state_t));
		zeromem(dev_info, sizeof(io_dev_info_t));
		--sf_dev_count;
	}

	return result;
}

static int sf_open(io_dev_info_t *dev_info, const uintptr_t spec,
		      io_entity_t *entity)
{
	sf_dev_state_t *cur;
	io_block_spec_t *region;

	assert((dev_info->info != (uintptr_t)NULL) &&
	       (spec != (uintptr_t)NULL) &&
	       (entity->info == (uintptr_t)NULL));

	region = (io_block_spec_t *)spec;
	cur = (sf_dev_state_t *)dev_info->info;
	assert(((region->offset % cur->dev_spec->page_size) == 0) &&
	       ((region->length % cur->dev_spec->page_size) == 0));

	cur->base = region->offset;
	cur->size = region->length;
	cur->file_pos = 0;
	NOTICE("%s: %lx %lx\n", __func__, region->offset, region->length);
	entity->info = (uintptr_t)cur;
	return 0;
}

/* parameter offset is relative address at here */
static int sf_seek(io_entity_t *entity, int mode, signed long long offset)
{
	sf_dev_state_t *cur;

	assert(entity->info != (uintptr_t)NULL);

	cur = (sf_dev_state_t *)entity->info;
	assert((offset >= 0) && (offset < cur->size));
	NOTICE("%s:%llx %x\n", __func__, offset, mode);
	switch (mode) {
	case IO_SEEK_SET:
		cur->file_pos = offset;
		break;
	case IO_SEEK_CUR:
		cur->file_pos += offset;
		break;
	default:
		return -EINVAL;
	}
	assert(cur->file_pos < cur->size);
	return 0;
}

/*
 * This function allows the caller to read any number of bytes
 * from any position. It hides from the caller that the low level
 * driver only can read aligned blocks of data. For this reason
 * we need to handle the use case where the first byte to be read is not
 * aligned to start of the block, the last byte to be read is also not
 * aligned to the end of a block, and there are zero or more blocks-worth
 * of data in between.
 *
 * In such a case we need to read more bytes than requested (i.e. full
 * blocks) and strip-out the leading bytes (aka skip) and the trailing
 * bytes (aka padding). See diagram below
 *
 * cur->file_pos ------------
 *                          |
 * cur->base                |
 *  |                       |
 *  v                       v<----  length   ---->
 *  --------------------------------------------------------------
 * |           |         block#1    |        |   block#n          |
 * |  block#0  |            +       |   ...  |     +              |
 * |           | <- skip -> +       |        |     + <- padding ->|
 *  ------------------------+----------------------+--------------
 *             ^                                                  ^
 *             |                                                  |
 *             v    iteration#1                iteration#n        v
 *              --------------------------------------------------
 *             |                    |        |                    |
 *             |<----  request ---->|  ...   |<----- request ---->|
 *             |                    |        |                    |
 *              --------------------------------------------------
 *            /                   /          |                    |
 *           /                   /           |                    |
 *          /                   /            |                    |
 *         /                   /             |                    |
 *        /                   /              |                    |
 *       /                   /               |                    |
 *      /                   /                |                    |
 *     /                   /                 |                    |
 *    /                   /                  |                    |
 *   /                   /                   |                    |
 *  <---- request ------>                    <------ request  ----->
 *  ---------------------                    -----------------------
 *  |        |          |                    |          |           |
 *  |<-skip->|<-nbytes->|           -------->|<-nbytes->|<-padding->|
 *  |        |          |           |        |          |           |
 *  ---------------------           |        -----------------------
 *  ^        \           \          |        |          |
 *  |         \           \         |        |          |
 *  |          \           \        |        |          |
 *  buf->offset \           \   buf->offset  |          |
 *               \           \               |          |
 *                \           \              |          |
 *                 \           \             |          |
 *                  \           \            |          |
 *                   \           \           |          |
 *                    \           \          |          |
 *                     \           \         |          |
 *                      --------------------------------
 *                      |           |        |         |
 * buffer-------------->|           | ...    |         |
 *                      |           |        |         |
 *                      --------------------------------
 *                      <-count#1->|                   |
 *                      <----------  count#n   -------->
 *                      <----------  length  ---------->
 *
 * Additionally, the IO driver has an underlying buffer that is at least
 * one block-size and may be big enough to allow.
 */
static int sf_read(io_entity_t *entity, uintptr_t buffer, size_t length,
		      size_t *length_read)
{
	sf_dev_state_t *cur;
	io_block_spec_t *buf;
	io_sf_ops_t *ops;
	unsigned int offset;
	size_t left;
	size_t request; /* number of requested bytes in one iteration */
	size_t count;   /* number of bytes already read */

	assert(entity->info != (uintptr_t)NULL);
	cur = (sf_dev_state_t *)entity->info;
	ops = &(cur->dev_spec->ops);
	buf = &(cur->dev_spec->buffer);
	assert((length <= cur->size) &&
	       (length > 0) &&
	       (ops->read != 0));

	/*
	 * We don't know the number of bytes that we are going
	 * to read in every iteration, because it will depend
	 * on the low level driver.
	 */
	count = 0;
	NOTICE("%s:%lx %lx %lx\n", __func__, length, cur->file_pos, cur->base);

	for (left = length; left > 0; left -= request) {

		offset = (cur->file_pos + cur->base);

		if (left > buf->length)
			request = buf->length;
		else
			request = left;
		request = ops->read(offset, buf->offset, request);

		if (request <= 0) {
			/*
			 * We couldn't read enough bytes to jump over
			 * the skip bytes, so we should have to read
			 * again the same block, thus generating
			 * the same error.
			 */
			ERROR("sf read failed %lx\n", request);
			return -EIO;
		}

		memcpy((void *)(buffer + count),
		       (void *)(buf->offset),
		       request);

		cur->file_pos += request;
		count += request;
	}
	assert(count == length);
	*length_read = count;


	return 0;
}

/*
 * This function allows the caller to write any number of bytes
 * from any position. It hides from the caller that the low level
 * driver only can write aligned blocks of data.
 * See comments for block_read for more details.
 */
static int sf_write(io_entity_t *entity, const uintptr_t buffer,
		       size_t length, size_t *length_written)
{
	NOTICE("Unsupported for sf write");

	return 0;
}

static int sf_close(io_entity_t *entity)
{
	entity->info = (uintptr_t)NULL;
	return 0;
}

static int sf_dev_open(const uintptr_t dev_spec, io_dev_info_t **dev_info)
{
	sf_dev_state_t *cur;
	io_block_spec_t *buffer;
	io_dev_info_t *info;
	size_t page_size;
	int result;

	assert(dev_info != NULL);
	result = allocate_dev_info(&info);
	if (result)
		return -ENOENT;

	cur = (sf_dev_state_t *)info->info;
	/* dev_spec is type of io_block_dev_spec_t. */
	cur->dev_spec = (io_sf_dev_spec_t *)dev_spec;
	buffer = &(cur->dev_spec->buffer);
	page_size = cur->dev_spec->page_size;
	assert((page_size > 0) &&
	       (is_power_of_2(page_size) != 0) &&
	       ((buffer->offset % page_size) == 0) &&
	       ((buffer->length % page_size) == 0));

	*dev_info = info;	/* cast away const */
	(void)page_size;
	(void)buffer;
	return 0;
}

static int sf_dev_close(io_dev_info_t *dev_info)
{
	return free_dev_info(dev_info);
}

/* Exported functions */

/* Register the Block driver with the IO abstraction */
int register_io_dev_sf(const io_dev_connector_t **dev_con)
{
	int result;

	assert(dev_con != NULL);

	/*
	 * Since dev_info isn't really used in io_register_device, always
	 * use the same device info at here instead.
	 */
	result = io_register_device(&dev_info_pool[0]);
	if (result == 0)
		*dev_con = &sf_dev_connector;
	return result;
}

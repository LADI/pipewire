/* Simple Plugin API
 * Copyright © 2018 Wim Taymans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef __SPA_BUFFER_H__
#define __SPA_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <spa/utils/defs.h>
#include <spa/buffer/meta.h>

/** \page page_buffer Buffers
 *
 * Buffers describe the data and metadata that is exchanged between
 * ports of a node.
 */

enum spa_data_type {
	SPA_DATA_Invalid,
	SPA_DATA_MemPtr,		/**< pointer to memory, the data field in
					  *  struct spa_data is set. */
	SPA_DATA_MemFd,			/**< generic fd, mmap to get to memory */
	SPA_DATA_DmaBuf,		/**< fd to dmabuf memory */

	SPA_DATA_LAST,			/**< not part of ABI */
};

/** Chunk of memory */
struct spa_chunk {
	uint32_t offset;		/**< offset of valid data. Should be taken
					  *  modulo the data maxsize to get the offset
					  *  in the data memory. */
	uint32_t size;			/**< size of valid data. Should be clamped to
					  *  maxsize. */
	int32_t stride;			/**< stride of valid data */
	int32_t dummy;			/**< dummy field for alignment */
};

/** Data for a buffer */
struct spa_data {
	uint32_t type;			/**< memory type, one of enum spa_data_type */
#define SPA_DATA_FLAG_NONE	 0
#define SPA_DATA_FLAG_CORRUPTED	(1<<0)	/**< data is corrupted in some way */
	uint32_t flags;			/**< data flags */
	int fd;				/**< optional fd for data */
	uint32_t mapoffset;		/**< offset to map fd at */
	uint32_t maxsize;		/**< max size of data */
	void *data;			/**< optional data pointer */
	struct spa_chunk *chunk;	/**< valid chunk of memory */
};

/** A Buffer */
struct spa_buffer {
	uint32_t n_metas;		/**< number of metadata */
	uint32_t n_datas;		/**< number of data members */
	struct spa_meta *metas;		/**< array of metadata */
	struct spa_data *datas;		/**< array of data members */
};

/** Find metadata in a buffer */
static inline struct spa_meta *spa_buffer_find_meta(const struct spa_buffer *b, uint32_t type)
{
	uint32_t i;

	for (i = 0; i < b->n_metas; i++)
		if (b->metas[i].type == type)
			return &b->metas[i];

	return NULL;
}

static inline void *spa_buffer_find_meta_data(const struct spa_buffer *b, uint32_t type, size_t size)
{
	struct spa_meta *m;
	if ((m = spa_buffer_find_meta(b, type)) && m->size >= size)
		return m->data;
	return NULL;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* __SPA_BUFFER_H__ */

#ifndef BROTLI_WRAPPER_H
#define BROTLI_WRAPPER_H

/*
 * Squashfs - Brotli Support
 *
 * Copyright (c) 2024
 * Aeybel Varghese <aeybelvarghese@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * brotli_wrapper.h
 *
 */

#include <brotli/encode.h>
#define BROTLI_DEFAULT_COMPRESSION_LEVEL BROTLI_MAX_QUALITY
#define BROTLI_COMPRESSION_MIN BROTLI_MIN_QUALITY
#define BROTLI_COMPRESSION_MAX BROTLI_MAX_QUALITY

#include "endian_compat.h"

#if __BYTE_ORDER == __BIG_ENDIAN
extern unsigned int inswap_le32(unsigned int);

#define SQUASHFS_INSWAP_COMP_OPTS(s) { \
	(s)->version = inswap_le32((s)->version); \
	(s)->flags = inswap_le32((s)->flags); \
}
#else
#define SQUASHFS_INSWAP_COMP_OPTS(s)
#endif

struct brotli_comp_opts {
	int compression_level;
	// short window_size; // TODO: Window size - Implement as configurable option
};

#endif

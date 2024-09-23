
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <brotli/encode.h>
#include <brotli/decode.h>

#include "compressor.h"
#include "brotli_wrapper.h"
#include "print_pager.h"
#include "squashfs_fs.h"



/* default compression level */
static int compression_level = BROTLI_DEFAULT_COMPRESSION_LEVEL;

/* default window size */
// TODO: Window size - Implement as configurable option
/* static int window_size = BROTLI_DEFAULT_WINDOW_SIZE; */

/*
 * This function is called by the options parsing code in mksquashfs.c
 * to parse any -X compressor option.
 *
 * This function returns:
 *	>=0 (number of additional args parsed) on success
 *	-1 if the option was unrecognised, or
 *	-2 if the option was recognised, but otherwise bad in
 *	   some way (e.g. invalid parameter)
 *
 * Note: this function sets internal compressor state, but does not
 * pass back the results of the parsing other than success/failure.
 * The gzip_dump_options() function is called later to get the options in
 * a format suitable for writing to the filesystem.
 */
static int brotli_options(char *argv[], int argc) {
  if (strcmp(argv[0], "-Xcompression-level") == 0) {
    if (argc < 2) {
      fprintf(stderr, "brotli: -Xcompression-level missing "
                      "compression level\n");
      fprintf(stderr, "brotli: -Xcompression-level it "
                      "should be 0 >= n <= 11\n");
      goto failed;
    }

    compression_level = atoi(argv[1]);
    if (compression_level < BROTLI_COMPRESSION_MIN || compression_level > BROTLI_COMPRESSION_MAX) {
      fprintf(stderr, "brotli: -Xcompression-level invalid, it "
                      "should be 0 >= n <= 11\n"); // TODO: Dont have the compression level limits hardcoded in string?
      goto failed;
    }

    return 1;
  }

  // TODO: Window size - Implement as configurable option

  /* else if (strcmp(argv[0], "-Xwindow-size") == 0) { */
  /*   if (argc < 2) { */
  /*     fprintf(stderr, "gzip: -Xwindow-size missing window " */
  /*                     "	size\n"); */
  /*     fprintf(stderr, "gzip: -Xwindow-size <window-size>\n"); */
  /*     goto failed; */
  /*   } */
  /**/
  /*   window_size = atoi(argv[1]); */
  /*   if (window_size < 8 || window_size > 15) { */
  /*     fprintf(stderr, "gzip: -Xwindow-size invalid, it " */
  /*                     "should be 8 >= n <= 15\n"); */
  /*     goto failed; */
  /*   } */
  /**/
  /*   return 1; */
  /* } */

failed:
    return -1;
}

/*
 * This function is called after all options have been parsed.
 * It is used to do post-processing on the compressor options using
 * values that were not expected to be known at option parse time.
 *
 * This function returns 0 on successful post processing, or
 *			-1 on error
 */
static int brotli_options_post(int block_size) {

  return 0;
}

/*
 * This function is called by mksquashfs to dump the parsed
 * compressor options in a format suitable for writing to the
 * compressor options field in the filesystem (stored immediately
 * after the superblock).
 *
 * This function returns a pointer to the compression options structure
 * to be stored (and the size), or NULL if there are no compression
 * options
 *
 */
static void *brotli_dump_options(int block_size, int *size) {
  static struct brotli_comp_opts comp_opts;

  comp_opts.compression_level = compression_level;

  SQUASHFS_INSWAP_COMP_OPTS(&comp_opts);

  *size = sizeof(comp_opts);
  return &comp_opts;
}

/*
 * This function is a helper specifically for the append mode of
 * mksquashfs.  Its purpose is to set the internal compressor state
 * to the stored compressor options in the passed compressor options
 * structure.
 *
 * In effect this function sets up the compressor options
 * to the same state they were when the filesystem was originally
 * generated, this is to ensure on appending, the compressor uses
 * the same compression options that were used to generate the
 * original filesystem.
 *
 * Note, even if there are no compressor options, this function is still
 * called with an empty compressor structure (size == 0), to explicitly
 * set the default options, this is to ensure any user supplied
 * -X options on the appending mksquashfs command line are over-ridden
 *
 * This function returns 0 on sucessful extraction of options, and
 *			-1 on error
 */
static int brotli_extract_options(int block_size, void *buffer, int size) {
  struct brotli_comp_opts *comp_opts = buffer;

  if (size == 0) {
    /* Set default values */
    compression_level = BROTLI_DEFAULT_COMPRESSION_LEVEL;
    // TODO: Window size - Implement as configurable option
    /* window_size = GZIP_DEFAULT_WINDOW_SIZE; */
    return 0;
  }
  
  // Sanity checks to verify the comp_opts struct is present
  if (size < sizeof(*comp_opts))
    goto failed;

  SQUASHFS_INSWAP_COMP_OPTS(comp_opts);

  /* Check comp_opts structure for correctness */
  if (comp_opts->compression_level < BROTLI_COMPRESSION_MIN || comp_opts->compression_level > BROTLI_COMPRESSION_MAX) {
    fprintf(stderr, "brotli: bad compression level in "
                    "compression options structure\n");
    goto failed;
  }
  compression_level = comp_opts->compression_level;

  /* // TODO: Window size - Implement as configurable option */
  /* if (comp_opts->window_size < BROTLI_WINDOW_MIN || comp_opts->window_size > BROTLI_WINDOW_MAX) { */
  /*   fprintf(stderr, "brotli: bad window size in " */
  /*                   "compression options structure\n"); */
  /*   goto failed; */
  /* } */
  /* window_size = comp_opts->window_size; */

  return 0;

failed:
  fprintf(stderr, "brotli: error reading stored compressor options from "
                  "filesystem!\n");

  return -1;
}

static void brotli_display_options(void *buffer, int size) {
  struct brotli_comp_opts *comp_opts = buffer;

  /* we expect a comp_opts structure of sufficient size to be present */
  if (size < sizeof(*comp_opts))
    goto failed;

  SQUASHFS_INSWAP_COMP_OPTS(comp_opts);

  /* Check comp_opts structure for correctness */
  if (comp_opts->compression_level < BROTLI_COMPRESSION_MIN || comp_opts->compression_level > BROTLI_COMPRESSION_MAX) {
    fprintf(stderr, "brotli: bad compression level in "
                    "compression options structure\n");
    goto failed;
  }
  printf("\tcompression-level %d\n", comp_opts->compression_level);

  /* // TODO: Window size - Implement as configurable option */
  /* if (comp_opts->window_size < BROTLI_WINDOW_MIN || comp_opts->window_size > BROTLI_WINDOW_MAX) { */
  /*   fprintf(stderr, "brotli: bad window size in " */
  /*                   "compression options structure\n"); */
  /*   goto failed; */
  /* } */
  /* printf("\twindow-size %d\n", comp_opts->window_size); */

  return;

failed:
  fprintf(stderr, "brotli: error reading stored compressor options from "
                  "filesystem!\n");
}

/*
 * This function is called by mksquashfs to initialise the
 * compressor, before compress() is called.
 *
 * This function returns 0 on success, and
 *			-1 on error
 */
static int brotli_init(void **strm, int block_size, int datablock) {
  // TODO: Maybe do something here? 
  return 0; // Return 0 to indicate success
}

static int brotli_compress(void *strm, void *d, void *s, int size, int block_size, int *error){
  // TODO: Handle block_size
  
  size_t max_compressed_size = BrotliEncoderMaxCompressedSize(size);

  /* // TODO: Window size - Implement as configurable option */
  BROTLI_BOOL res = BrotliEncoderCompress(compression_level,    // Compression level (0-11)
                  BROTLI_DEFAULT_WINDOW,     // Window size
                  BROTLI_MODE_GENERIC,
                  size,
                  s,
                  &max_compressed_size,
                  d);
  if(!res){
    *error = res;
    fprintf(stderr, "brotli: compression failed.\n");
    return -1;
  }

  return (int) max_compressed_size;
}

static int brotli_uncompress(void *d, void *s, int size, int outsize,
                             int *error) {
  return 0;
}

static void brotli_usage(FILE *stream, int cols) {
  autowrap_print(stream, "\t  -Xcompression-level <compression-level>\n", cols);
  autowrap_printf(stream, cols,
                  "\t\t<compression-level> should be 0 .. "
                  "11 (default %d)\n",
                  BROTLI_DEFAULT_COMPRESSION_LEVEL);
  // TODO: Window size - Implement as configurable option */
  /* autowrap_print(stream, "\t  -Xwindow-size <window-size>\n", cols); */
  /* autowrap_printf(stream, cols, */
  /*                 "\t\t<window-size> should be 8 .. 15 " */
  /*                 "(default %d)\n", */
  /*                 BROTLI_DEFAULT_WINDOW_SIZE); */
}

static int option_args(char *option) {
  if (strcmp(option, "-Xcompression-level") == 0) 
    return 1;

  return 0;
}

struct compressor brotli_comp_ops = {.init = brotli_init,
                                     .compress = brotli_compress,
                                     .uncompress = brotli_uncompress,
                                     .options = brotli_options,
                                     .options_post = brotli_options_post,
                                     .dump_options = brotli_dump_options,
                                     .extract_options = brotli_extract_options,
                                     .display_options = brotli_display_options,
                                     .usage = brotli_usage,
                                     .option_args = option_args,
                                     .id = BROTLI_COMPRESSION,
                                     .name = "brotli",
                                     .supported = 1};

/*
 * Copyright (c) 2014 Jeff Boody
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "texgz/texgz_tex.h"
#include "texgz/texgz_png.h"
#include "libpak/pak_file.h"

#define LOG_TAG "unpaktex"
#include "libpak/pak_log.h"

#define SUBTILE_COUNT 8
#define SUBTILE_SIZE  256

static void merge(texgz_tex_t* dst, pak_file_t* pak, int i, int j)
{
	assert(dst);
	assert(pak);
	LOGD("debug i=%i, j=%i", i, j);

	char key[256];
	snprintf(key, 256, "%i_%i", j, i);
	int size = pak_file_seek(pak, key);
	if(size == 0)
	{
		return;
	}

	texgz_tex_t* src = texgz_tex_importf(pak->f, size);
	if(src == NULL)
	{
		return;
	}

	if(texgz_tex_convert(src, TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA) == 0)
	{
		goto fail_convert;
	}

	// copy from src to dst
	int m;
	int bpp = texgz_tex_bpp(dst); // same format for src and dst
	unsigned char* dst_base = &(dst->pixels[SUBTILE_SIZE*i*dst->stride*bpp +
	                                        SUBTILE_SIZE*j*bpp]);
	for(m = 0; m < SUBTILE_SIZE; ++m)
	{
		unsigned char* dst_pixels = &(dst_base[m*dst->stride*bpp]);
		unsigned char* src_pixels = &(src->pixels[m*src->stride*bpp]);
		memcpy(dst_pixels, src_pixels, SUBTILE_SIZE*bpp);
	}

	texgz_tex_delete(&src);

	// success
	return;

	// failure
	fail_convert:
		texgz_tex_delete(&src);
	return;
}

int main(int argc, char** argv)
{
	if(argc != 3)
	{
		LOGE("usage: %s file.pak file.png", argv[0]);
		return EXIT_FAILURE;
	}

	pak_file_t* pak;
	pak = pak_file_open(argv[1], PAK_FLAG_READ);
	if(pak == NULL)
	{
		return EXIT_FAILURE;
	}

	texgz_tex_t* dst = texgz_tex_new(2048, 2048, 2048, 2048,
	                                 TEXGZ_UNSIGNED_BYTE,
	                                 TEXGZ_RGBA, NULL);
	if(dst == NULL)
	{
		goto fail_dst;
	}

	int i;
	int j;
	for(i = 0; i < SUBTILE_COUNT; ++i)
	{
		for(j = 0; j < SUBTILE_COUNT; ++j)
		{
			merge(dst, pak, i, j);
		}
	}

	if(texgz_png_export(dst, argv[2]) == 0)
	{
		goto fail_export;
	}

	pak_file_close(&pak);

	// success
	return EXIT_SUCCESS;

	// failure
	fail_export:
	fail_dst:
		pak_file_close(&pak);
	return EXIT_FAILURE;
}

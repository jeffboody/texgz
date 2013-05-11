/*
 * Copyright (c) 2013 Jeff Boody
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

#include <stdio.h>
#include <png.h>
#include <stdlib.h>
#include <assert.h>
#include "texgz_png.h"

#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr)   ((png_ptr)->jmpbuf)
#endif

#define LOG_TAG "texgz"
#include "texgz_log.h"

texgz_tex_t* texgz_png_import(const char* fname)
{
	assert(fname);
	LOGD("debug fname=%s", fname);

	FILE* f = fopen(fname, "r");
	if(f == NULL)
	{
		LOGE("fopen failed");
		return NULL;
	}

	unsigned char sig[8];
	if(fread(sig, 1, 8, f) != 8)
	{
		LOGE("fread failed");
		goto fail_fread;
	}

	if(png_sig_cmp(sig, 0, 8))
	{
		LOGE("png_sig_cmp failed");
		goto fail_sig;
	}

	png_info* info_ptr  = NULL;
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
	                                             NULL, NULL, NULL);
	if(png_ptr == NULL)
	{
		LOGE("png_create_read_struct failed");
		goto fail_read;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL)
	{
		LOGE("png_create_info_struct failed");
		goto fail_info;
	}

	if(setjmp(png_jmpbuf(png_ptr)))
	{
		LOGE("png_jmpbuf failed");
		goto fail_jmpbuf;
	}

	png_init_io(png_ptr, f);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);

	png_uint_32 width;
	png_uint_32 height;
	int         bit_depth;
	int         color_type;
	if(png_get_IHDR(png_ptr, info_ptr,
	                &width, &height,
	                &bit_depth, &color_type,
	                NULL, NULL, NULL) == 0)
	{
		LOGE("png_get_IHDR");
		goto fail_get_IHDR;
	}

	if(bit_depth != 8)
	{
		LOGE("invalid bit_depth=%i", bit_depth);
		goto fail_bit_depth;
	}

	texgz_tex_t* tex = NULL;
	int stride_bytes = (int) png_get_rowbytes(png_ptr, info_ptr);
	if(color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		int stride = stride_bytes / 4;
		if(stride_bytes % 4 != 0)
		{
			LOGE("invalid stride_bytes=%i", stride_bytes);
			goto fail_stride;
		}

		tex = texgz_tex_new((int) width, (int) height,
		                    stride, (int) height,
		                    TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA,
		                    NULL);
	}
	else if(color_type == PNG_COLOR_TYPE_RGB)
	{
		int stride = stride_bytes / 3;
		if(stride_bytes % 3 != 0)
		{
			LOGE("invalid stride_bytes=%i", stride_bytes);
			goto fail_stride;
		}

		tex = texgz_tex_new((int) width, (int) height,
		                    stride, (int) height,
		                    TEXGZ_UNSIGNED_BYTE, TEXGZ_RGB,
		                    NULL);
	}
	else
	{
		LOGE("invalid color_type=%i", color_type);
		goto fail_color_type;
	}

	if(tex == NULL)
	{
		goto fail_tex;
	}

	int i;
	unsigned char* pixels = tex->pixels;
	for(i = 0; i < tex->height; ++i)
	{
		png_read_row(png_ptr, pixels, NULL);
		pixels += stride_bytes;
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(f);

	// success
	return tex;

	// failure
	fail_tex:
	fail_color_type:
	fail_stride:
	fail_bit_depth:
	fail_get_IHDR:
	fail_jmpbuf:
	fail_info:
		png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : NULL, NULL);
	fail_read:
	fail_sig:
	fail_fread:
		fclose(f);
	return NULL;
}

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
#include <jpeglib.h>
#include <stdlib.h>
#include <assert.h>
#include "texgz_jpeg.h"

#define LOG_TAG "texgz"
#include "texgz_log.h"

texgz_tex_t* texgz_jpeg_import(const char* fname)
{
	assert(fname);
	LOGD("debug fname=%s", fname);

	FILE *f = fopen(fname, "r");
	if(f == NULL)
	{
		LOGE("fopen %s failed", fname);
		return NULL;
	}

	// start decompressing the jpeg
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, f);
	if(jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK)
	{
		LOGE("jpeg_read_header failed");
		goto fail_header;
	}
	jpeg_start_decompress(&cinfo);

	// check for errors
	if((cinfo.num_components == 3) &&
	   (cinfo.image_width == cinfo.output_width) &&
	   (cinfo.image_height == cinfo.output_height))
	{
		// ok
	}
	else
	{
		LOGE("invalid num_components=%i, image=%ix%i, output=%ix%i",
		     cinfo.num_components,
		     cinfo.image_width, cinfo.image_height,
		     cinfo.output_width, cinfo.output_height);
		goto fail_format;
	}

	// create the texgz tex
	texgz_tex_t* tex = texgz_tex_new(cinfo.image_width, cinfo.image_height,
	                                 cinfo.image_width, cinfo.image_height,
	                                 TEXGZ_UNSIGNED_BYTE, TEXGZ_RGB,
	                                 NULL);
	if(tex == NULL)
	{
		goto fail_tex;
	}

	unsigned char* pixels = tex->pixels;
	int stride_bytes = cinfo.image_width*cinfo.num_components;
	while(cinfo.output_scanline < cinfo.image_height)
	{
		if(jpeg_read_scanlines(&cinfo, &pixels, 1) != 1)
		{
			LOGE("jpeg_read_scanlines failed");
			goto fail_scanline;
		}
		pixels += stride_bytes;
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(f);

	// success
	return tex;

	// failure
	fail_scanline:
		texgz_tex_delete(&tex);
	fail_tex:
	fail_format:
		jpeg_finish_decompress(&cinfo);
	fail_header:
		jpeg_destroy_decompress(&cinfo);
		fclose(f);
	return NULL;
}

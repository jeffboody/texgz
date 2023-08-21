/*
 * Copyright (c) 2023 Jeff Boody
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
#include <string.h>

#define LOG_TAG "texgz"
#include "libcc/math/cc_pow2n.h"
#include "libcc/cc_log.h"
#include "texgz/texgz_png.h"
#include "texgz/pil_lanczos.h"

#define TEXGZ_MIPMAP_SIZE_MAX 257

#define TEXGZ_MIPMAP_METHOD_BOX      0
#define TEXGZ_MIPMAP_METHOD_LANCZOS3 1

/***********************************************************
* private                                                  *
***********************************************************/

static int
texgz_mipmap(texgz_tex_t* src, int method, int level,
             texgz_tex_t* conv, texgz_tex_t* dst)
{
	ASSERT(src);
	ASSERT(conv);
	ASSERT(dst);

	float support[] =
	{
		0.5f, // TEXGZ_MIPMAP_METHOD_BOX
		3.0f, // TEXGZ_MIPMAP_METHOD_LANCZOS3
	};

	// default box filter
	float mask[TEXGZ_MIPMAP_SIZE_MAX] =
	{
		0.5f,
		0.5f,
	};

	// determine filter size
	int   scale  = cc_pow2n(level);
	float scalef = (float) scale;
	int   n      = (int) (scalef*support[method] + 0.01f);
	int   size   = 2*n;
	if(size >= TEXGZ_MIPMAP_SIZE_MAX)
	{
		LOGE("unsupported method=%i, level=%i",
		     method, level);
		return 0;
	}

	// generate masks
	if(method == TEXGZ_MIPMAP_METHOD_LANCZOS3)
	{
		float step = 1.0f/scalef;
		float x    = support[method] - step/2.0f;
		float y;
		int   i;
		for(i = 0; i < n; ++i)
		{
			y = pil_lanczos3_filter(x)/scale;
			mask[i]            = y;
			mask[size - i - 1] = y;
			x -= step;
		}
	}

	// apply filter and decimate
	texgz_tex_convolveF(src,  conv,
	                    size, 1, scale, 1, mask);
	texgz_tex_convolveF(conv, dst,
	                    1, size, 1, scale, mask);

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

int main(int argc, char** argv)
{
	if(argc != 5)
	{
		LOGE("usage: %s method level src.png dst.png", argv[0]);
		LOGE("method: box | lanczos3");
		LOGE("level: mipmap level (1 to N)");
		return EXIT_FAILURE;
	}

	int method = TEXGZ_MIPMAP_METHOD_BOX;
	int level  = (int) strtol(argv[2], NULL, 0);
	if(strcmp(argv[1], "lanczos3") == 0)
	{
		method = TEXGZ_MIPMAP_METHOD_LANCZOS3;
		if(level < 1)
		{
			LOGE("invalid level=%i", level);
			return EXIT_FAILURE;
		}
	}
	else
	{
		// box only supports zoom level 1
		// e.g. recursively apply box filter
		if(level != 1)
		{
			LOGE("invalid level=%i", level);
			return EXIT_FAILURE;
		}
	}

	texgz_tex_t* src;
	src = texgz_png_import(argv[3]);
	if(src == NULL)
	{
		return EXIT_FAILURE;
	}

	int dst_width  = src->width/cc_pow2n(level);
	int dst_height = src->height/cc_pow2n(level);

	if((src->width%dst_width) || (src->height%dst_height))
	{
		LOGE("invalid width=%i:%i, height=%i:%i",
		     src->width,  dst_width,
		     src->height, dst_height);
		goto fail_src_size;
	}

	if(texgz_tex_convertF(src, 0.0f, 1.0f,
	                      TEXGZ_FLOAT,
	                      TEXGZ_RGBA) == 0)
	{
		goto fail_convert_src;
	}

	texgz_tex_t* conv;
	conv = texgz_tex_new(dst_width, src->height,
	                     dst_width, src->height,
	                     TEXGZ_FLOAT, TEXGZ_RGBA,
	                     NULL);
	if(conv == NULL)
	{
		goto fail_conv;
	}

	texgz_tex_t* dst;
	dst = texgz_tex_new(dst_width, dst_height,
	                    dst_width, dst_height,
	                    TEXGZ_FLOAT, TEXGZ_RGBA,
	                    NULL);
	if(dst == NULL)
	{
		goto fail_dst;
	}

	if(texgz_mipmap(src, method, level, conv, dst) == 0)
	{
		goto fail_mipmap;
	}

	if(texgz_tex_convertF(dst, 0.0f, 1.0f,
	                      TEXGZ_UNSIGNED_BYTE,
	                      TEXGZ_RGBA) == 0)
	{
		goto fail_convert_dst;
	}

	if(texgz_png_export(dst, argv[4]) == 0)
	{
		goto fail_export;
	}

	texgz_tex_delete(&dst);
	texgz_tex_delete(&conv);
	texgz_tex_delete(&src);

	// success
	return EXIT_SUCCESS;

	// failure
	fail_export:
	fail_convert_dst:
	fail_mipmap:
		texgz_tex_delete(&dst);
	fail_dst:
		texgz_tex_delete(&conv);
	fail_conv:
	fail_convert_src:
	fail_src_size:
		texgz_tex_delete(&src);
	return EXIT_FAILURE;
}

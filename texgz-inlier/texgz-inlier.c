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

#define LOG_TAG "texgz"
#include "libcc/cc_log.h"
#include "texgz_inlier.h"
#include "../texgz_png.h"

/***********************************************************
* public                                                   *
***********************************************************/

int main(int argc, char** argv)
{
	if(argc != 4)
	{
		LOGE("usage: %s s sdx prefix",
		     argv[0]);
		LOGE("s: superpixel size (sxs)");
		LOGE("sdx: stddev threshold");
		return EXIT_FAILURE;
	}

	int   s   = (int) strtol(argv[1], NULL, 0);
	float sdx = strtof(argv[2], NULL);

	const char* prefix = argv[3];

	char fname_input[256];
	char fname_avg[256];
	char fname_inlier[256];
	snprintf(fname_input, 256, "%s.png", prefix);
	snprintf(fname_avg, 256, "%s-%i-avg.png", prefix, s);
	snprintf(fname_inlier, 256, "%s-%i-%i-inlier.png",
	         prefix, s, (int) (10.0f*sdx));

	texgz_tex_t* tex = texgz_png_import(fname_input);
	if(tex == NULL)
	{
		return EXIT_FAILURE;
	}

	if(texgz_tex_convert(tex, TEXGZ_UNSIGNED_BYTE,
	                     TEXGZ_RGBA) == 0)
	{
		goto fail_convert;
	}

	texgz_tex_t* tex_avg;
	tex_avg = texgz_tex_inlier(tex, s, 0.0f);
	if(tex_avg == NULL)
	{
		goto fail_avg;
	}

	texgz_tex_t* tex_inlier;
	tex_inlier = texgz_tex_inlier(tex, s, sdx);
	if(tex_inlier == NULL)
	{
		goto fail_inlier;
	}

	// save results
	if((texgz_png_export(tex_avg, fname_avg) == 0) ||
	   (texgz_png_export(tex_inlier, fname_inlier) == 0))
	{
		goto fail_export;
	}

	texgz_tex_delete(&tex_inlier);
	texgz_tex_delete(&tex_avg);
	texgz_tex_delete(&tex);

	// success
	return EXIT_SUCCESS;

	// failure
	fail_export:
		texgz_tex_delete(&tex_inlier);
	fail_inlier:
		texgz_tex_delete(&tex_avg);
	fail_avg:
	fail_convert:
		texgz_tex_delete(&tex);
	return EXIT_FAILURE;
}

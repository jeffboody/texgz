/*
 * Copyright (c) 2022 Jeff Boody
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

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "texgz"
#include "libcc/cc_log.h"
#include "libcc/cc_memory.h"
#include "texgz_slic.h"
#include "../texgz_png.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
save_output(texgz_slic_t* slic, texgz_tex_t* sp,
            float min, float max,
            const char* fname)
{
	ASSERT(slic);
	ASSERT(sp);
	ASSERT(fname);

	texgz_tex_t* out;
	out = texgz_slic_output(slic, sp);
	if(out == NULL)
	{
		return 0;
	}

	if(texgz_tex_convertF(out, min, max,
	                      TEXGZ_UNSIGNED_BYTE,
	                      TEXGZ_RGBA) == 0)
	{
		goto fail_convert_out;
	}

	if(texgz_png_export(out, fname) == 0)
	{
		goto fail_export;
	}

	texgz_tex_delete(&out);

	// success
	return 1;

	// failure
	fail_export:
	fail_convert_out:
		texgz_tex_delete(&out);
	return 0;
}

/***********************************************************
* public                                                   *
***********************************************************/

int main(int argc, char** argv)
{
	if(argc != 7)
	{
		LOGE("usage: %s s m n r steps prefix",
		     argv[0]);
		LOGE("s: superpixel size (sxs)");
		LOGE("m: compactness control");
		LOGE("n: gradient neighborhood (nxn)");
		LOGE("r: recenter clusters");
		LOGE("N: maximum step count");
		return EXIT_FAILURE;
	}

	int   s = (int) strtol(argv[1], NULL, 0);
	float m = strtof(argv[2], NULL);
	int   n = (int) strtol(argv[3], NULL, 0);
	int   r = (int) strtol(argv[4], NULL, 0);
	int   N = (int) strtol(argv[5], NULL, 0);

	const char* prefix = argv[6];

	char input[256];
	snprintf(input, 256, "%s.png", prefix);

	texgz_tex_t* tex = texgz_png_import(input);
	if(tex == NULL)
	{
		return EXIT_FAILURE;
	}

	if(texgz_tex_convertF(tex, 0.0f, 1.0f,
	                      TEXGZ_FLOAT, TEXGZ_RGBA) == 0)
	{
		goto fail_convert_tex;
	}

	texgz_slic_t* slic = texgz_slic_new(tex, s, m, n, r);
	if(slic == NULL)
	{
		goto fail_slic;
	}

	// solve slic superpixels
	// TODO - loop for N steps or until E <= thresh
	int idx;
	for(idx = 0; idx < N; ++idx)
	{
		texgz_slic_step(slic);
	}

	// TODO - enforce connectivity

	// output names
	char base[256];
	char fname_avg[256];
	char fname_stddev[256];
	snprintf(base, 256, "%s-%i-%i-%i-%i",
	         prefix, s, (int) (10.0f*m), n, r);
	snprintf(fname_avg,    256, "%s-avg.png",    base);
	snprintf(fname_stddev, 256, "%s-stddev.png", base);

	// save avg
	save_output(slic, slic->sp_avg,
	            0.0f, 1.0f, fname_avg);

	// rescale/save stddev
	int x;
	int y;
	float max = 0.0f;
	float pixel[4];
	for(y = 0; y < slic->sp_stddev->height; ++y)
	{
		for(x = 0; x < slic->sp_stddev->width; ++x)
		{
			texgz_tex_getPixelF(slic->sp_stddev, x, y, pixel);

			if(pixel[0] > max)
			{
				max = pixel[0];
			}
			if(pixel[1] > max)
			{
				max = pixel[1];
			}
			if(pixel[2] > max)
			{
				max = pixel[2];
			}
		}
	}
	save_output(slic, slic->sp_stddev,
	            0.0f, max, fname_stddev);

	texgz_slic_delete(&slic);
	texgz_tex_delete(&tex);

	// success
	return EXIT_SUCCESS;

	// failure
	fail_slic:
	fail_convert_tex:
		texgz_tex_delete(&tex);
	return EXIT_FAILURE;
}

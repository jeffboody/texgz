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

static int
save_image(texgz_tex_t* tex, float min, float max,
           const char* fname)
{
	ASSERT(tex);
	ASSERT(fname);

	texgz_tex_t* out;
	out = texgz_tex_convertFcopy(tex, min, max,
	                             TEXGZ_UNSIGNED_BYTE,
	                             TEXGZ_RGBA);
	if(out == NULL)
	{
		return 0;
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
		texgz_tex_delete(&out);
	return 0;
}

/***********************************************************
* public                                                   *
***********************************************************/

int main(int argc, char** argv)
{
	if(argc != 8)
	{
		LOGE("usage: %s s m sdx n r steps prefix",
		     argv[0]);
		LOGE("s: superpixel size (sxs)");
		LOGE("m: compactness control");
		LOGE("sdx: stddev threshold");
		LOGE("n: gradient neighborhood (nxn)");
		LOGE("r: recenter clusters");
		LOGE("steps: maximum step count");
		return EXIT_FAILURE;
	}

	int   s     = (int) strtol(argv[1], NULL, 0);
	float m     = strtof(argv[2], NULL);
	float sdx   = strtof(argv[3], NULL);
	int   n     = (int) strtol(argv[4], NULL, 0);
	int   r     = (int) strtol(argv[5], NULL, 0);
	int   steps = (int) strtol(argv[6], NULL, 0);

	const char* prefix = argv[7];

	char input[256];
	snprintf(input, 256, "%s.png", prefix);

	texgz_tex_t* tex = texgz_png_import(input);
	if(tex == NULL)
	{
		return EXIT_FAILURE;
	}

	if(texgz_tex_convert(tex, TEXGZ_UNSIGNED_BYTE,
	                     TEXGZ_RGBA) == 0)
	{
		goto fail_convert_tex;
	}

	if(texgz_tex_convertF(tex, 0.0f, 1.0f,
	                      TEXGZ_FLOAT, TEXGZ_RGBA) == 0)
	{
		goto fail_convert_tex;
	}

	texgz_tex_t* gray = texgz_tex_grayscaleF(tex);
	if(gray == NULL)
	{
		goto fail_gray;
	}

	texgz_tex_t* gx;
	gx = texgz_tex_new(gray->width, gray->height,
	                   gray->stride, gray->vstride,
	                   gray->type, gray->format,
	                   NULL);
	if(gx == NULL)
	{
		goto fail_gx;
	}

	texgz_tex_t* gy;
	gy = texgz_tex_new(gray->width, gray->height,
	                   gray->stride, gray->vstride,
	                   gray->type, gray->format,
	                   NULL);
	if(gy == NULL)
	{
		goto fail_gy;
	}

	float sobelx[] =
	{
		-0.25f, 0.0f, 0.25f,
		-0.5f,  0.0f, 0.5f,
		-0.25f, 0.0f, 0.25f,
	};
	float sobely[] =
	{
		-0.25f, -0.5f, -0.25f,
		 0.0f,   0.0f,  0.0f,
		 0.25f,  0.5f,  0.25f,
	};
	texgz_tex_convolveF(gray, gx, 3, 3, 1, 1, sobelx);
	texgz_tex_convolveF(gray, gy, 3, 3, 1, 1, sobely);

	texgz_slic_t* slic = texgz_slic_new(tex, s, m, sdx, n, r);
	if(slic == NULL)
	{
		goto fail_slic;
	}

	// solve slic superpixels
	// TODO - loop for steps or until E <= thresh
	int step;
	for(step = 0; step < steps; ++step)
	{
		texgz_slic_step(slic, step);
	}

	// TODO - enforce connectivity

	// output names
	char base[256];
	char fname_avg[256];
	char fname_slic[256];
	char fname_stddev[256];
	char fname_outlier[256];
	char fname_gx[256];
	char fname_gy[256];
	snprintf(base, 256, "%s-%i-%i-%i-%i-%i",
	         prefix, s, (int) (10.0f*m), (int) (10.0f*sdx),
	         n, r);
	snprintf(fname_avg,    256, "%s-avg.png",    base);
	snprintf(fname_slic,    256, "%s-slic.png",    base);
	snprintf(fname_stddev, 256, "%s-stddev.png", base);
	snprintf(fname_outlier, 256, "%s-outlier.png", base);
	snprintf(fname_gx, 256, "%s-gx.png", base);
	snprintf(fname_gy, 256, "%s-gy.png", base);

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
	save_image(gx, -0.25f, 0.25f, fname_gx);
	save_image(gy, -0.25f, 0.25f, fname_gy);
	save_image(slic->sp_outlier, 0.0f, 1.0f, fname_outlier);
	save_image(slic->sp_avg, 0.0f, 1.0f, fname_slic);

	texgz_slic_delete(&slic);
	texgz_tex_delete(&gy);
	texgz_tex_delete(&gx);
	texgz_tex_delete(&gray);
	texgz_tex_delete(&tex);

	// success
	return EXIT_SUCCESS;

	// failure
	fail_slic:
		texgz_tex_delete(&gy);
	fail_gy:
		texgz_tex_delete(&gx);
	fail_gx:
		texgz_tex_delete(&gray);
	fail_gray:
	fail_convert_tex:
		texgz_tex_delete(&tex);
	return EXIT_FAILURE;
}

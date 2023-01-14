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

#include <math.h>
#include <stdlib.h>

#define LOG_TAG "texgz"
#include "libcc/math/cc_float.h"
#include "libcc/cc_log.h"
#include "texgz_inlier.h"

/***********************************************************
* public                                                   *
***********************************************************/

texgz_tex_t* texgz_tex_inlier(texgz_tex_t* tex,
                              int s, float sdx)
{
	ASSERT(self);
	ASSERT(tex);

	// check the size
	if((tex->width%s != 0) || (tex->height%s != 0))
	{
		LOGE("invalid width=%i, height=%i, s=%i",
		     tex->width, tex->height, s);
		return NULL;
	}

	// convert input to float if needed
	texgz_tex_t* texf = NULL;
	if((tex->type == TEXGZ_FLOAT) &&
	   (tex->format == TEXGZ_RGBA))
	{
		texf = tex;
	}
	else if((tex->type == TEXGZ_UNSIGNED_BYTE) &&
	        (tex->format == TEXGZ_RGBA))
	{
		texf = texgz_tex_convertFcopy(tex, 0.0f, 1.0f,
		                              TEXGZ_FLOAT, TEXGZ_RGBA);
		if(texf == NULL)
		{
			return NULL;
		}
	}
	else
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     tex->type, tex->format);
		return NULL;
	}

	int w = tex->width/s;
	int h = tex->height/s;

	// create inlier output
	texgz_tex_t* tex_in;
	tex_in = texgz_tex_new(w, h, w, h,
	                       TEXGZ_UNSIGNED_BYTE,
	                       TEXGZ_RGBA, NULL);
	if(tex_in == NULL)
	{
		goto fail_tex_in;
	}

	// compute inlers
	unsigned char pixel[4];
	float pixelf[4];
	float in[4];
	float dp[4];
	float avg[4];
	float stddev[4];
	float ss = s*s;
	int x;
	int y;
	int i;
	int j;
	int count[4];
	for(y = 0; y < h; ++y)
	{
		for(x = 0; x < w; ++x)
		{
			// compute avg
			avg[0] = 0.0f;
			avg[1] = 0.0f;
			avg[2] = 0.0f;
			avg[3] = 0.0f;
			for(i = 0; i < s; ++i)
			{
				for(j = 0; j < s; ++j)
				{
					texgz_tex_getPixelF(texf, s*x + j, s*y + i, pixelf);

					avg[0] += pixelf[0];
					avg[1] += pixelf[1];
					avg[2] += pixelf[2];
					avg[3] += pixelf[3];
				}
			}
			avg[0] /= ss;
			avg[1] /= ss;
			avg[2] /= ss;
			avg[3] /= ss;

			// check if stddev threshold disabled
			// e.g. downsampling with box filter
			if(sdx == 0.0f)
			{
				pixel[0] = (unsigned char) cc_clamp(255.0f*avg[0], 0.0f, 255.0f);
				pixel[1] = (unsigned char) cc_clamp(255.0f*avg[1], 0.0f, 255.0f);
				pixel[2] = (unsigned char) cc_clamp(255.0f*avg[2], 0.0f, 255.0f);
				pixel[3] = (unsigned char) cc_clamp(255.0f*avg[3], 0.0f, 255.0f);

				texgz_tex_setPixel(tex_in, x, y, pixel);
				continue;
			}

			// compute stddev
			stddev[0] = 0.0f;
			stddev[1] = 0.0f;
			stddev[2] = 0.0f;
			stddev[3] = 0.0f;
			for(i = 0; i < s; ++i)
			{
				for(j = 0; j < s; ++j)
				{
					texgz_tex_getPixelF(texf, s*x + j, s*y + i, pixelf);

					dp[0] = pixelf[0] - avg[0];
					dp[1] = pixelf[1] - avg[1];
					dp[2] = pixelf[2] - avg[2];
					dp[3] = pixelf[3] - avg[3];

					stddev[0] += dp[0]*dp[0];
					stddev[1] += dp[1]*dp[1];
					stddev[2] += dp[2]*dp[2];
					stddev[3] += dp[3]*dp[3];
				}
			}
			stddev[0] = sqrtf(stddev[0]/ss);
			stddev[1] = sqrtf(stddev[1]/ss);
			stddev[2] = sqrtf(stddev[2]/ss);
			stddev[3] = sqrtf(stddev[3]/ss);

			// average inlier samples below sdx threshold
			count[0] = 0;
			count[1] = 0;
			count[2] = 0;
			count[3] = 0;
			in[0]    = 0.0f;
			in[1]    = 0.0f;
			in[2]    = 0.0f;
			in[3]    = 0.0f;
			for(i = 0; i < s; ++i)
			{
				for(j = 0; j < s; ++j)
				{
					texgz_tex_getPixelF(texf, s*x + j, s*y + i, pixelf);

					if(fabsf(pixelf[0] - avg[0]) <= (sdx*stddev[0]))
					{
						in[0] += pixelf[0];
						++count[0];
					}

					if(fabsf(pixelf[1] - avg[1]) <= (sdx*stddev[1]))
					{
						in[1] += pixelf[1];
						++count[1];
					}

					if(fabsf(pixelf[2] - avg[2]) <= (sdx*stddev[2]))
					{
						in[2] += pixelf[2];
						++count[2];
					}

					if(fabsf(pixelf[3] - avg[3]) <= (sdx*stddev[3]))
					{
						in[3] += pixelf[3];
						++count[3];
					}
				}
			}

			if(count[0]) in[0] /= count[0];
			if(count[1]) in[1] /= count[1];
			if(count[2]) in[2] /= count[2];
			if(count[3]) in[3] /= count[3];

			pixel[0] = (unsigned char) cc_clamp(255.0f*in[0], 0.0f, 255.0f);
			pixel[1] = (unsigned char) cc_clamp(255.0f*in[1], 0.0f, 255.0f);
			pixel[2] = (unsigned char) cc_clamp(255.0f*in[2], 0.0f, 255.0f);
			pixel[3] = (unsigned char) cc_clamp(255.0f*in[3], 0.0f, 255.0f);

			texgz_tex_setPixel(tex_in, x, y, pixel);
		}
	}

	// delete texf if needed
	if(texf != tex)
	{
		texgz_tex_delete(&texf);
	}

	// success
	return tex_in;

	// failure
	fail_tex_in:
	{
		if(texf != tex)
		{
			texgz_tex_delete(&texf);
		}
	}
	return NULL;
}

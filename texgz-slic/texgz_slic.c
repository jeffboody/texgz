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
#include "texgz/texgz_png.h"
#include "texgz_slic.h"

/***********************************************************
* private                                                  *
***********************************************************/

static texgz_slicCluster_t*
texgz_slic_cluster(texgz_slic_t* self, int i, int j)
{
	ASSERT(self);
	ASSERT(i < self->kh);
	ASSERT(j < self->kw);

	return &self->clusters[i*self->kw + j];
}

static texgz_slicSample_t*
texgz_slic_sample(texgz_slic_t* self, int x, int y)
{
	ASSERT(self);

	texgz_tex_t* input = self->input;
	ASSERT(x < input->width);
	ASSERT(y < input->height);

	return &self->samples[y*input->stride + x];
}

static float
texgz_slic_gradient(texgz_slic_t* self, int x, int y)
{
	ASSERT(self);

	int x0 = x;
	int y0 = y;
	int xp = x + 1;
	int yp = y + 1;
	int xm = x - 1;
	int ym = y - 1;

	// plus/minus pixels
	float pixel_p0[4];
	float pixel_m0[4];
	float pixel_0p[4];
	float pixel_0m[4];
	texgz_tex_getPixelF(self->input, xp, y0, pixel_p0);
	texgz_tex_getPixelF(self->input, xm, y0, pixel_m0);
	texgz_tex_getPixelF(self->input, x0, yp, pixel_0p);
	texgz_tex_getPixelF(self->input, x0, ym, pixel_0m);

	// r/g/b/a deltas
	float dxr = pixel_p0[0] - pixel_m0[0];
	float dyr = pixel_0p[0] - pixel_0m[0];
	float dxg = pixel_p0[1] - pixel_m0[1];
	float dyg = pixel_0p[1] - pixel_0m[1];
	float dxb = pixel_p0[2] - pixel_m0[2];
	float dyb = pixel_0p[2] - pixel_0m[2];
	float dxa = pixel_p0[3] - pixel_m0[3];
	float dya = pixel_0p[3] - pixel_0m[3];

	// solve L2 norms
	// https://mathworld.wolfram.com/L2-Norm.html
	return sqrtf(dxr*dxr + dxg*dxg + dxb*dxb + dxa*dxa) +
	       sqrtf(dyr*dyr + dyg*dyg + dyb*dyb + dya*dya);
}

static float
texgz_slic_dist(texgz_slic_t* self,
                texgz_slicCluster_t* cluster,
                float* pixel, int x, int y)
{
	ASSERT(self);
	ASSERT(cluster);
	ASSERT(pixel);

	int i = cluster->i;
	int j = cluster->j;
	float avg[4];
	texgz_tex_getPixelF(self->sp_avg, j, i, avg);
	float dr = pixel[0] - avg[0];
	float dg = pixel[1] - avg[1];
	float db = pixel[2] - avg[2];
	float da = pixel[3] - avg[3];
	float dp = sqrtf(dr*dr + dg*dg + db*db * da*da);
	float dx = (float) (x - cluster->x);
	float dy = (float) (y - cluster->y);
	float dxy = sqrtf(dx*dx + dy*dy);

	return dp + (self->m/self->s)*dxy;
}

static void texgz_slic_reset(texgz_slic_t* self)
{
	ASSERT(self);

	// reset cluster indices/centers
	int i;
	int j;
	int s  = self->s;
	int s2 = s/2;
	texgz_slicCluster_t* cluster;
	for(i = 0; i < self->kh; ++i)
	{
		for(j = 0; j < self->kw; ++j)
		{
			cluster = texgz_slic_cluster(self, i, j);

			cluster->i = i;
			cluster->j = j;
			cluster->x = s*j + s2;
			cluster->y = s*i + s2;
		}
	}

	int   x;
	int   y;
	int   cx    = 0;
	int   cy    = 0;
	float gbest = 0.0f;
	float g;
	float avg[4];
	float pixel[4];
	for(i = 0; i < self->kh; ++i)
	{
		for(j = 0; j < self->kw; ++j)
		{
			cluster = texgz_slic_cluster(self, i, j);

			// perterb cluster centers in a neighborhood
			// to the lowest gradient position
			int x0 = cluster->x - self->n/2;
			int y0 = cluster->y - self->n/2;
			int x1 = x0 + self->n;
			int y1 = y0 + self->n;
			for(y = y0; y < y1; ++y)
			{
				for(x = x0; x < x1; ++x)
				{
					g = texgz_slic_gradient(self, x, y);
					if(((x == x0) && (y == y0)) || (g < gbest))
					{
						cx    = x;
						cy    = y;
						gbest = g;
					}
				}
			}

			// compute average cluster pixel
			x0     = j*s;
			y0     = i*s;
			x1     = x0 + s;
			y1     = y0 + s;
			avg[0] = 0;
			avg[1] = 0;
			avg[2] = 0;
			avg[3] = 0;
			for(y = y0; y < y1; ++y)
			{
				for(x = x0; x < x1; ++x)
				{
					texgz_tex_getPixelF(self->input, x, y, pixel);
					avg[0] += pixel[0];
					avg[1] += pixel[1];
					avg[2] += pixel[2];
					avg[3] += pixel[3];
				}
			}
			int S = s*s;
			avg[0] /= S;
			avg[1] /= S;
			avg[2] /= S;
			avg[3] /= S;

			// update cluster
			cluster->x = cx;
			cluster->y = cy;

			texgz_tex_setPixelF(self->sp_avg, j, i, avg);
		}
	}
}

static int
texgz_slic_outlier(texgz_slic_t* self, float* pixel,
                   float* avg, float* stddev)
{
	ASSERT(self);
	ASSERT(pixel);
	ASSERT(avg);
	ASSERT(stddev);

	if((fabsf(pixel[0] - avg[0]) <= (self->sdx*stddev[0])) &&
	   (fabsf(pixel[1] - avg[1]) <= (self->sdx*stddev[1])) &&
	   (fabsf(pixel[2] - avg[2]) <= (self->sdx*stddev[2])) &&
	   (fabsf(pixel[3] - avg[3]) <= (self->sdx*stddev[3])))
	{
		return 0;
	}

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

texgz_slic_t*
texgz_slic_new(texgz_tex_t* input, int s, float m,
               float sdx, int n, int recenter)
{
	ASSERT(input);

	texgz_slic_t* self;
	self = (texgz_slic_t*)
	       CALLOC(1, sizeof(texgz_slic_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	// check required input attributes
	if((input->type   != TEXGZ_FLOAT) ||
	   (input->format != TEXGZ_RGBA))
	{
		LOGE("invalid width=%i, height=%i, "
		     "type=0x%X, format=0x%X",
		     input->width, input->height,
		     input->type, input->format);
		goto fail_input_attr;
	}

	// check required slic attributes
	// width and height must be greater than s
	// width and height must be a multiple of s
	// n must smaller than s
	// n must be odd size
	if((input->width  < s)      ||
	   (input->height < s)      ||
	   ((input->width%s)  != 0) ||
	   ((input->height%s) != 0) ||
	   (n >= s) || (n%2 != 1))
	{
		LOGE("invalid s=%i, n=%i, width=%i, height=%i",
		     s, n, input->width, input->height);
		goto fail_slic_attr;
	}

	self->s   = s;
	self->m   = m;
	self->sdx = sdx;
	self->n   = n;
	self->kw  = input->width/s;
	self->kh  = input->height/s;
	self->recenter = recenter;
	self->input    = input;

	self->clusters = (texgz_slicCluster_t*)
	                 CALLOC(self->kw*self->kh,
	                        sizeof(texgz_slicCluster_t));
	if(self->clusters == NULL)
	{
		goto fail_clusters;
	}

	self->samples = (texgz_slicSample_t*)
	               CALLOC(input->width*input->height,
	                      sizeof(texgz_slicSample_t));
	if(self->samples == NULL)
	{
		goto fail_samples;
	}

	self->sp_avg = texgz_tex_new(self->kw, self->kh,
	                             self->kw, self->kh,
	                             TEXGZ_FLOAT,
	                             TEXGZ_RGBA, NULL);
	if(self->sp_avg == NULL)
	{
		goto fail_sp_avg;
	}

	self->sp_stddev = texgz_tex_new(self->kw, self->kh,
	                                self->kw, self->kh,
	                                TEXGZ_FLOAT,
	                                TEXGZ_RGBA, NULL);
	if(self->sp_stddev == NULL)
	{
		goto fail_sp_stddev;
	}

	self->sp_outlier = texgz_tex_new(input->width, input->height,
	                                 input->stride, input->vstride,
	                                 TEXGZ_FLOAT,
	                                 TEXGZ_RGBA, NULL);
	if(self->sp_outlier == NULL)
	{
		goto fail_sp_outlier;
	}

	texgz_slic_reset(self);

	// success
	return self;

	// failure
	fail_sp_outlier:
		texgz_tex_delete(&self->sp_stddev);
	fail_sp_stddev:
		texgz_tex_delete(&self->sp_avg);
	fail_sp_avg:
		FREE(self->samples);
	fail_samples:
		FREE(self->clusters);
	fail_clusters:
	fail_slic_attr:
	fail_input_attr:
		FREE(self);
	return NULL;
}

void texgz_slic_delete(texgz_slic_t** _self)
{
	ASSERT(_self);

	texgz_slic_t* self = *_self;
	if(self)
	{
		texgz_tex_delete(&self->sp_outlier);
		texgz_tex_delete(&self->sp_stddev);
		texgz_tex_delete(&self->sp_avg);
		FREE(self->samples);
		FREE(self->clusters);
		FREE(self);
		*_self = NULL;
	}
}

float texgz_slic_step(texgz_slic_t* self, int step)
{
	ASSERT(self);

	texgz_tex_t* input = self->input;

	// reset clusters and samples
	int i;
	int j;
	texgz_slicCluster_t* cluster;
	if(step)
	{
		for(i = 0; i < self->kh; ++i)
		{
			for(j = 0; j < self->kw; ++j)
			{
				cluster = texgz_slic_cluster(self, i, j);

				cluster->count            = 0;
				cluster->sum_x            = 0;
				cluster->sum_y            = 0;
				cluster->sum_pixel1[0]    = 0.0f;
				cluster->sum_pixel1[1]    = 0.0f;
				cluster->sum_pixel1[2]    = 0.0f;
				cluster->sum_pixel1[3]    = 0.0f;
				cluster->sum_pixel2[0]    = 0.0f;
				cluster->sum_pixel2[1]    = 0.0f;
				cluster->sum_pixel2[2]    = 0.0f;
				cluster->sum_pixel2[3]    = 0.0f;
			}
		}

		size_t size = MEMSIZEPTR(self->samples);
		memset(self->samples, 0, size);
	}

	// assign samples to clusters
	int x;
	int y;
	texgz_slicSample_t* sample;
	float               pixel[4];
	for(i = 0; i < self->kh; ++i)
	{
		for(j = 0; j < self->kw; ++j)
		{
			cluster = texgz_slic_cluster(self, i, j);

			// compute/clamp cluster neighborhood
			int x0 = cluster->x - self->s;
			int y0 = cluster->y - self->s;
			int x1 = x0 + 2*self->s;
			int y1 = y0 + 2*self->s;
			if(x0 < 0)
			{
				x0 = 0;
			}
			if(y0 < 0)
			{
				y0 = 0;
			}
			if(x1 >= input->width)
			{
				x1 = input->width - 1;
			}
			if(y1 >= input->height)
			{
				y1 = input->height - 1;
			}

			// assign samples in cluster neighborhood to
			// cluster with lowest distance
			float dist;
			for(y = y0; y <= y1; ++y)
			{
				for(x = x0; x <= x1; ++x)
				{
					texgz_tex_getPixelF(input, x, y, pixel);
					dist   = texgz_slic_dist(self, cluster, pixel, x, y);
					sample = texgz_slic_sample(self, x, y);
					if((sample->cluster == NULL) ||
					   (sample->dist > dist))
					{
						sample->dist    = dist;
						sample->cluster = cluster;
					}
				}
			}
		}
	}

	// compute center and pixel sums
	float avg[4];
	float stddev[4];
	float red[4]   = { 1.0f, 0.0f, 0.0f, 1.0f };
	float clear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for(y = 0; y < input->height; ++y)
	{
		for(x = 0; x < input->width; ++x)
		{
			sample  = texgz_slic_sample(self, x, y);
			cluster = sample->cluster;
			if(cluster == NULL)
			{
				continue;
			}

			texgz_tex_getPixelF(input, x, y, pixel);

			if((step == 0) || (self->sdx == 0.0f))
			{
				++cluster->count;
				cluster->sum_x         += x;
				cluster->sum_y         += y;
				cluster->sum_pixel1[0] += pixel[0];
				cluster->sum_pixel1[1] += pixel[1];
				cluster->sum_pixel1[2] += pixel[2];
				cluster->sum_pixel1[3] += pixel[3];
			}
			else
			{
				i = cluster->i;
				j = cluster->j;
				texgz_tex_getPixelF(self->sp_avg, j, i, avg);
				texgz_tex_getPixelF(self->sp_stddev, j, i, stddev);

				// discard samples which were identified as outliers
				// in the previous iteration
				if(texgz_slic_outlier(self, pixel, avg, stddev))
				{
					texgz_tex_setPixelF(self->sp_outlier, x, y, red);
				}
				else
				{
					++cluster->count;
					cluster->sum_x         += x;
					cluster->sum_y         += y;
					cluster->sum_pixel1[0] += pixel[0];
					cluster->sum_pixel1[1] += pixel[1];
					cluster->sum_pixel1[2] += pixel[2];
					cluster->sum_pixel1[3] += pixel[3];
					texgz_tex_setPixelF(self->sp_outlier, x, y, clear);
				}
			}
		}
	}

	// compute avg
	for(i = 0; i < self->kh; ++i)
	{
		for(j = 0; j < self->kw; ++j)
		{
			cluster = texgz_slic_cluster(self, i, j);

			if(cluster->count)
			{
				// optionally recenter cluster centers
				if(self->recenter)
				{
					cluster->x = cluster->sum_x/cluster->count;
					cluster->y = cluster->sum_y/cluster->count;
				}

				pixel[0] = cluster->sum_pixel1[0]/cluster->count;
				pixel[1] = cluster->sum_pixel1[1]/cluster->count;
				pixel[2] = cluster->sum_pixel1[2]/cluster->count;
				pixel[3] = cluster->sum_pixel1[3]/cluster->count;
				texgz_tex_setPixelF(self->sp_avg, j, i, pixel);
			}
		}
	}

	// compute stddev sums
	float dp[4];
	for(y = 0; y < input->height; ++y)
	{
		for(x = 0; x < input->width; ++x)
		{
			sample  = texgz_slic_sample(self, x, y);
			cluster = sample->cluster;
			if(cluster == NULL)
			{
				continue;
			}

			texgz_tex_getPixelF(input, x, y, pixel);

			i = cluster->i;
			j = cluster->j;
			texgz_tex_getPixelF(self->sp_avg, j, i, avg);

			dp[0] = pixel[0] - avg[0];
			dp[1] = pixel[1] - avg[1];
			dp[2] = pixel[2] - avg[2];
			dp[3] = pixel[3] - avg[3];

			cluster->sum_pixel2[0] += dp[0]*dp[0];
			cluster->sum_pixel2[1] += dp[1]*dp[1];
			cluster->sum_pixel2[2] += dp[2]*dp[2];
			cluster->sum_pixel2[3] += dp[3]*dp[3];
		}
	}

	// compute stddev
	// stddev = sqrt((1/N)*SUM((x-mu)^2))
	// https://en.wikipedia.org/wiki/Standard_deviation
	for(i = 0; i < self->kh; ++i)
	{
		for(j = 0; j < self->kw; ++j)
		{
			cluster = texgz_slic_cluster(self, i, j);

			if(cluster->count)
			{
				pixel[0] = sqrtf(cluster->sum_pixel2[0]/cluster->count);
				pixel[1] = sqrtf(cluster->sum_pixel2[1]/cluster->count);
				pixel[2] = sqrtf(cluster->sum_pixel2[2]/cluster->count);
				pixel[3] = sqrtf(cluster->sum_pixel2[3]/cluster->count);
				texgz_tex_setPixelF(self->sp_stddev, j, i, pixel);
			}
		}
	}

	// TODO - compute residual error

	return 0.0f;
}

texgz_tex_t* texgz_slic_output(texgz_slic_t* self,
                               texgz_tex_t* sp)
{
	ASSERT(self);
	ASSERT(sp);

	texgz_tex_t* input = self->input;

	texgz_tex_t* out;
	out = texgz_tex_new(input->width, input->height,
	                    input->width, input->height,
	                    input->type, input->format,
	                    NULL);
	if(out == NULL)
	{
	        return NULL;
	}

	// copy pixels
	int   x;
	int   y;
	float pixel[4];
	texgz_slicSample_t* sample;
	for(y = 0; y < out->height; ++y)
	{
		for(x = 0; x < out->width; ++x)
		{
			sample = texgz_slic_sample(self, x, y);
			if(sample->cluster)
			{
				texgz_tex_getPixelF(sp,
				                    sample->cluster->j,
				                    sample->cluster->i,
				                    pixel);
				texgz_tex_setPixelF(out, x, y, pixel);
			}
		}
	}

	return out;
}

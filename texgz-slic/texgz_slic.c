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
#include "libcc/math/cc_pow2n.h"
#include "libcc/cc_log.h"
#include "libcc/cc_memory.h"
#include "texgz/texgz_png.h"
#include "texgz_slic.h"

/***********************************************************
* private                                                  *
***********************************************************/

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
	unsigned char rgbap0[4];
	unsigned char rgbam0[4];
	unsigned char rgba0p[4];
	unsigned char rgba0m[4];
	texgz_tex_getPixel(self->tex, xp, y0, rgbap0);
	texgz_tex_getPixel(self->tex, xm, y0, rgbam0);
	texgz_tex_getPixel(self->tex, x0, yp, rgba0p);
	texgz_tex_getPixel(self->tex, x0, ym, rgba0m);

	// r/g/b/a float pixels
	float rp0 = (float) rgbap0[0];
	float rm0 = (float) rgbam0[0];
	float r0p = (float) rgba0p[0];
	float r0m = (float) rgba0m[0];
	float gp0 = (float) rgbap0[1];
	float gm0 = (float) rgbam0[1];
	float g0p = (float) rgba0p[1];
	float g0m = (float) rgba0m[1];
	float bp0 = (float) rgbap0[2];
	float bm0 = (float) rgbam0[2];
	float b0p = (float) rgba0p[2];
	float b0m = (float) rgba0m[2];
	float ap0 = (float) rgbap0[3];
	float am0 = (float) rgbam0[3];
	float a0p = (float) rgba0p[3];
	float a0m = (float) rgba0m[3];

	// r/g/b/a deltas
	float rdx = rp0 - rm0;
	float rdy = r0p - r0m;
	float gdx = gp0 - gm0;
	float gdy = g0p - g0m;
	float bdx = bp0 - bm0;
	float bdy = b0p - b0m;
	float adx = ap0 - am0;
	float ady = a0p - a0m;

	// solve L2 norms
	// https://mathworld.wolfram.com/L2-Norm.html
	return sqrtf(rdx*rdx + gdx*gdx + bdx*bdx + adx*adx) +
	       sqrtf(rdy*rdy + gdy*gdy + bdy*bdy + ady*ady);
}

static float
texgz_slic_dist(texgz_slic_t* self,
                texgz_slicCluster_t* cluster,
                unsigned char* pixel,
                int x, int y)
{
	ASSERT(self);
	ASSERT(cluster);
	ASSERT(pixel);

	float dr = ((float) cluster->pixel[0]) -
	           ((float) pixel[0]);
	float dg = ((float) cluster->pixel[1]) -
	           ((float) pixel[1]);
	float db = ((float) cluster->pixel[2]) -
	           ((float) pixel[2]);
	float da = ((float) cluster->pixel[3]) -
	           ((float) pixel[3]);
	dr /= 255.0f;
	dg /= 255.0f;
	db /= 255.0f;
	da /= 255.0f;
	float drgba = sqrtf(dr*dr + dg*dg + db*db * da*da);

	float dx = (float) (x - cluster->x);
	float dy = (float) (y - cluster->y);
	float dxy = sqrtf(dx*dx + dy*dy);

	return drgba + (self->m/self->s)*dxy;
}

static void texgz_slic_reset(texgz_slic_t* self)
{
	ASSERT(self);

	texgz_tex_t* tex = self->tex;

	int i;
	int j;
	int x;
	int y;
	int w   = tex->width;
	int h   = tex->height;
	int wk  = w/self->k;
	int hk  = h/self->k;
	int wk2 = wk/2;
	int hk2 = hk/2;

	// reset cluster centers
	texgz_slicCluster_t* cluster;
	for(i = 0; i < self->k; ++i)
	{
		for(j = 0; j < self->k; ++j)
		{
			cluster = texgz_slic_cluster(self, i, j);

			cluster->x = wk*j + wk2;
			cluster->y = hk*i + hk2;
		}
	}

	int   xbest = 0;
	int   ybest = 0;
	float gbest = 0.0f;
	float g;
	uint32_t      rgba[4];
	unsigned char pixel[4];
	for(i = 0; i < self->k; ++i)
	{
		for(j = 0; j < self->k; ++j)
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
						xbest = x;
						ybest = y;
						gbest = g;
					}
				}
			}

			// compute average cluster pixel
			x0      = j*self->s;
			y0      = i*self->s;
			x1      = x0 + self->s;
			y1      = y0 + self->s;
			rgba[0] = 0;
			rgba[1] = 0;
			rgba[2] = 0;
			rgba[3] = 0;
			for(y = y0; y < y1; ++y)
			{
				for(x = x0; x < x1; ++x)
				{
					texgz_tex_getPixel(self->tex, x, y, pixel);
					rgba[0] += (uint32_t) pixel[0];
					rgba[1] += (uint32_t) pixel[1];
					rgba[2] += (uint32_t) pixel[2];
					rgba[3] += (uint32_t) pixel[3];
				}
			}
			int S = self->s*self->s;
			rgba[0] /= S;
			rgba[1] /= S;
			rgba[2] /= S;
			rgba[3] /= S;

			// update cluster
			cluster->x        = xbest;
			cluster->y        = ybest;
			cluster->pixel[0] = (unsigned char) rgba[0];
			cluster->pixel[1] = (unsigned char) rgba[1];
			cluster->pixel[2] = (unsigned char) rgba[2];
			cluster->pixel[3] = (unsigned char) rgba[3];
		}
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

texgz_slic_t*
texgz_slic_new(texgz_tex_t* tex, int s, float m, int n,
               int r)
{
	ASSERT(tex);

	texgz_slic_t* self;
	self = (texgz_slic_t*)
	       CALLOC(1, sizeof(texgz_slic_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	// check for required tex attributes
	if((tex->width  != tex->height)               ||
	   (tex->width  != cc_next_pow2n(tex->width)) ||
	   (tex->type   != TEXGZ_UNSIGNED_BYTE)       ||
	   (tex->format != TEXGZ_RGBA))
	{
		LOGE("invalid width=%i, height=%i, "
		     "type=0x%X, format=0x%X",
		     tex->width, tex->height,
		     tex->type, tex->format);
		goto fail_tex_attr;
	}

	// check slic attributes
	// s must be power-of-two
	// s must be less than width
	// n must smaller than s
	// n must be odd size
	if((s != cc_next_pow2n(s)) ||
	   (s >= tex->width) ||
	   (n >= s) || (n%2 != 1))
	{
		LOGE("invalid s=%i, width=%i, n=%i",
		     s, tex->width, n);
		goto fail_slic_attr;
	}

	self->tex = tex;
	self->s   = s;
	self->m   = m;
	self->n   = n;
	self->r   = r;
	self->k   = self->tex->width/s;

	self->supers = (texgz_slicSuper_t*)
	               CALLOC(tex->width*tex->height,
	                      sizeof(texgz_slicSuper_t));
	if(self->supers == NULL)
	{
		goto fail_supers;
	}

	self->clusters = (texgz_slicCluster_t*)
	                 CALLOC(self->k*self->k,
	                        sizeof(texgz_slicCluster_t));
	if(self->clusters == NULL)
	{
		goto fail_clusters;
	}

	texgz_slic_reset(self);

	// success
	return self;

	// failure
	fail_clusters:
		FREE(self->supers);
	fail_supers:
	fail_slic_attr:
	fail_tex_attr:
		FREE(self);
	return NULL;
}

void texgz_slic_delete(texgz_slic_t** _self)
{
	ASSERT(_self);

	texgz_slic_t* self = *_self;
	if(self)
	{
		FREE(self->clusters);
		FREE(self->supers);
		FREE(self);
		*_self = NULL;
	}
}

float texgz_slic_step(texgz_slic_t* self)
{
	ASSERT(self);

	texgz_tex_t* tex = self->tex;

	// reset supers
	int    w    = tex->width;
	int    h    = tex->height;
	size_t size = w*h*sizeof(texgz_slicSuper_t);
	memset(self->supers, 0, size);

	// reset clusters
	int i;
	int j;
	texgz_slicCluster_t* cluster;
	for(i = 0; i < self->k; ++i)
	{
		for(j = 0; j < self->k; ++j)
		{
			cluster = texgz_slic_cluster(self, i, j);

			cluster->step_count    = 0;
			cluster->step_x        = 0;
			cluster->step_y        = 0;
			cluster->step_pixel[0] = 0;
			cluster->step_pixel[1] = 0;
			cluster->step_pixel[2] = 0;
			cluster->step_pixel[3] = 0;
		}
	}

	// update clusters
	for(i = 0; i < self->k; ++i)
	{
		for(j = 0; j < self->k; ++j)
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
			if(x1 >= tex->width)
			{
				x1 = tex->width - 1;
			}
			if(y1 >= tex->height)
			{
				y1 = tex->height - 1;
			}

			// update pixels in cluster neighborhood with
			// best dist/center
			int   x;
			int   y;
			float dist;
			texgz_slicSuper_t* super;
			unsigned char      pixel[4];
			for(y = y0; y <= y1; ++y)
			{
				for(x = x0; x <= x1; ++x)
				{
					texgz_tex_getPixel(self->tex, x, y, pixel);
					dist  = texgz_slic_dist(self, cluster, pixel, x, y);
					super = texgz_slic_super(self, x, y);
					if((super->step_cluster == NULL) ||
					   (super->step_dist > dist))
					{
						super->step_cluster = cluster;
						super->step_dist    = dist;

						++cluster->step_count;
						cluster->step_x        += x;
						cluster->step_y        += y;
						cluster->step_pixel[0] += (uint32_t) pixel[0];
						cluster->step_pixel[1] += (uint32_t) pixel[1];
						cluster->step_pixel[2] += (uint32_t) pixel[2];
						cluster->step_pixel[3] += (uint32_t) pixel[3];
					}
				}
			}
		}
	}

	// optionally recenter clusters
	// compute average cluster pixel
	for(i = 0; i < self->k; ++i)
	{
		for(j = 0; j < self->k; ++j)
		{
			cluster = texgz_slic_cluster(self, i, j);

			if(cluster->step_count)
			{
				if(self->r)
				{
					cluster->x = cluster->step_x/cluster->step_count;
					cluster->y = cluster->step_y/cluster->step_count;
				}
				cluster->pixel[0] = (unsigned char)
				                    (cluster->step_pixel[0]/
				                     cluster->step_count);
				cluster->pixel[1] = (unsigned char)
				                    (cluster->step_pixel[1]/
				                     cluster->step_count);
				cluster->pixel[2] = (unsigned char)
				                    (cluster->step_pixel[2]/
				                     cluster->step_count);
				cluster->pixel[3] = (unsigned char)
				                    (cluster->step_pixel[3]/
				                     cluster->step_count);
			}
		}
	}

	// TODO - compute residual error

	return 0.0f;
}

texgz_slicSuper_t*
texgz_slic_super(texgz_slic_t* self, int x, int y)
{
	ASSERT(self);

	texgz_tex_t* tex = self->tex;

	return &self->supers[y*tex->stride + x];
}

texgz_slicCluster_t*
texgz_slic_cluster(texgz_slic_t* self, int i, int j)
{
	ASSERT(self);

	return &self->clusters[i*self->k + j];
}

texgz_tex_t* texgz_slic_output(texgz_slic_t* self)
{
	ASSERT(self);

	texgz_tex_t* tex = self->tex;

	texgz_tex_t* out;
	out = texgz_tex_new(tex->width, tex->height,
	                    tex->width, tex->height,
	                    tex->type, tex->format,
	                    NULL);
	if(out == NULL)
	{
		return NULL;
	}

	// copy pixels
	int x;
	int y;
	unsigned char        pixel[4];
	texgz_slicSuper_t*   super;
	texgz_slicCluster_t* cluster;
	for(y = 0; y < out->height; ++y)
	{
		for(x = 0; x < out->width; ++x)
		{
			super   = texgz_slic_super(self, x, y);
			cluster = super->step_cluster;
			if(cluster)
			{
				texgz_tex_getPixel(tex, cluster->x, cluster->y,
				                   pixel);
				texgz_tex_setPixel(out, x, y, pixel);
			}
		}
	}

	return out;
}

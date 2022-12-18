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

#ifndef texgz_slic_H
#define texgz_slic_H

#include "texgz/texgz_tex.h"

typedef struct
{
	// index
	int i;
	int j;

	// center
	int x;
	int y;

	// center, avg and stddev sums
	int   count;
	int   sum_x;
	int   sum_y;
	float sum_pixel1[4];
	float sum_pixel2[4];
} texgz_slicCluster_t;

typedef struct
{
	float                dist;
	texgz_slicCluster_t* cluster;
} texgz_slicSample_t;

typedef struct
{
	int   s;   // superpixel size
	float m;   // compactness control
	float sdx; // stddev threshold
	int   n;   // gradient neighborhood
	int   kw;  // cluster count K = kw*kh
	int   kh;  // where kw,kh = width/s,height/s
	int   recenter;

	// input reference
	texgz_tex_t* input;

	// step state
	texgz_slicCluster_t* clusters;
	texgz_slicSample_t*  samples;

	// superpixel features
	texgz_tex_t* sp_avg;     // kwxkh
	texgz_tex_t* sp_stddev;  // kwxky
	texgz_tex_t* sp_outlier; // wxh
} texgz_slic_t;

texgz_slic_t* texgz_slic_new(texgz_tex_t* input,
                             int s, float m, float sdx,
                             int n, int recenter);
void          texgz_slic_delete(texgz_slic_t** _self);
float         texgz_slic_step(texgz_slic_t* self,
                              int step);
texgz_tex_t*  texgz_slic_output(texgz_slic_t* self,
                                texgz_tex_t* sp);

#endif

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
	// cluster state
	int           x;
	int           y;
	unsigned char pixel[4];

	// step state
	uint32_t step_count;
	uint32_t step_x;
	uint32_t step_y;
	uint32_t step_pixel[4];
} texgz_slicCluster_t;

typedef struct
{
	// step state
	float                step_dist;
	texgz_slicCluster_t* step_cluster;
} texgz_slicSuper_t;

typedef struct
{
	int                  s;   // superpixel size
	float                m;   // compactness control
	int                  n;   // gradient neighborhood
	int                  k;   // cluster count K = k*k
	int                  r;   // recenter
	texgz_tex_t*         tex; // reference
	texgz_slicSuper_t*   supers;
	texgz_slicCluster_t* clusters;
} texgz_slic_t;

texgz_slic_t*        texgz_slic_new(texgz_tex_t* tex,
                                    int s, float m, int n,
                                    int r);
void                 texgz_slic_delete(texgz_slic_t** _self);
float                texgz_slic_step(texgz_slic_t* self);
texgz_slicSuper_t*   texgz_slic_super(texgz_slic_t* self,
                                      int x, int y);
texgz_slicCluster_t* texgz_slic_cluster(texgz_slic_t* self,
                                        int i, int j);
texgz_tex_t*         texgz_slic_output(texgz_slic_t* self);

#endif

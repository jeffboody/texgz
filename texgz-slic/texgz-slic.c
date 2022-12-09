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
* public                                                   *
***********************************************************/

int main(int argc, char** argv)
{
	if(argc != 8)
	{
		LOGE("usage: %s s m n r steps input.png output.png",
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

	const char* input  = argv[6];
	const char* output = argv[7];

	texgz_tex_t* tex = texgz_png_import(input);
	if(tex == NULL)
	{
		return EXIT_FAILURE;
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

	// save output
	texgz_tex_t* out = texgz_slic_output(slic);
	if(out == NULL)
	{
		goto fail_out;
	}

	if(texgz_png_export(out, output) == 0)
	{
		goto fail_export;
	}

	texgz_tex_delete(&out);
	texgz_slic_delete(&slic);
	texgz_tex_delete(&tex);

	// success
	return EXIT_SUCCESS;

	// failure
	fail_export:
		texgz_tex_delete(&out);
	fail_out:
		texgz_slic_delete(&slic);
	fail_slic:
		texgz_tex_delete(&tex);
	return EXIT_FAILURE;
}

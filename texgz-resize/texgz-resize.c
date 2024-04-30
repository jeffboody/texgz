/*
 * Copyright (c) 2024 Jeff Boody
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
#include "texgz/texgz_tex.h"
#include "texgz/texgz_png.h"

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		LOGE("usage: %s base", argv[0]);
		return EXIT_FAILURE;
	}

	const char* base = argv[1];

	char name0[256];
	char name31[256];
	char name32[256];
	snprintf(name0, 256,  "%s.png", base);
	snprintf(name31, 256, "%s-3LR.png", base);
	snprintf(name32, 256, "%s-3HR.png", base);

	texgz_tex_t* tex0 = texgz_png_import(name0);
	if(tex0 == NULL)
	{
		return EXIT_FAILURE;
	}

	if(texgz_tex_convert(tex0, TEXGZ_UNSIGNED_BYTE,
	                     TEXGZ_RGBA) == 0)
	{
		goto fail_convert;
	}

	texgz_tex_t* tex31 = texgz_tex_lanczos3(tex0, 1);
	if(tex31 == NULL)
	{
		goto fail_tex31;
	}

	texgz_tex_t* tex32;
	tex32 = texgz_tex_resize(tex31, tex0->width, tex0->height);
	if(tex32 == NULL)
	{
		goto fail_tex32;
	}

	if((texgz_png_export(tex31, name31) == 0) ||
	   (texgz_png_export(tex32, name32) == 0))
	{
		goto fail_export;
	}

	texgz_tex_delete(&tex32);
	texgz_tex_delete(&tex31);
	texgz_tex_delete(&tex0);

	// success
	return EXIT_SUCCESS;

	// failure
	fail_export:
		texgz_tex_delete(&tex32);
	fail_tex32:
		texgz_tex_delete(&tex31);
	fail_tex31:
	fail_convert:
		texgz_tex_delete(&tex0);
	return EXIT_FAILURE;
}

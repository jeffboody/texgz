/*
 * Copyright (c) 2018 Jeff Boody
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
#include <string.h>

#define LOG_TAG "texgz"
#include "libcc/cc_log.h"
#include "texgz/texgz_tex.h"
#include "texgz/texgz_png.h"

int main(int argc, char** argv)
{
	if(argc != 4)
	{
		LOGE("usage: %s [size] [src.png] [dst.png]", argv[0]);
		return EXIT_FAILURE;
	}

	texgz_tex_t* tex = texgz_png_import(argv[2]);
	if(tex == NULL)
	{
		return EXIT_FAILURE;
	}

	// required to outline
	// input should be grayscale on black image
	if(texgz_tex_convert(tex,
	                     TEXGZ_UNSIGNED_BYTE,
	                     TEXGZ_LUMINANCE) == 0)
	{
		goto fail_convert1;
	}

	int          size = (int) strtol(argv[1], NULL, 0);
	texgz_tex_t* out  = texgz_tex_outline(tex, size);
	if(out == NULL)
	{
		goto fail_outline;
	}

	// required to export as PNG
	if(texgz_tex_convert(out,
	                     TEXGZ_UNSIGNED_BYTE,
	                     TEXGZ_RGBA) == 0)
	{
		goto fail_convert2;
	}

	if(texgz_png_export(out, argv[3]) == 0)
	{
		goto fail_export;
	}

	texgz_tex_delete(&out);
	texgz_tex_delete(&tex);

	// success
	return EXIT_SUCCESS;

	// failure
	fail_export:
	fail_convert2:
		texgz_tex_delete(&out);
	fail_convert1:
	fail_outline:
		texgz_tex_delete(&tex);
	return EXIT_FAILURE;
}

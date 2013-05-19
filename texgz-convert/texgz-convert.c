/*
 * Copyright (c) 2013 Jeff Boody
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
#include <assert.h>
#include <string.h>
#include <texgz/texgz_tex.h>
#include <texgz/texgz_jpeg.h>
#include <texgz/texgz_png.h>
#include <texgz/texgz_mgm.h>

#define LOG_TAG "texgz"
#include <texgz/texgz_log.h>

static int check_ext(const char* fname, const char* ext)
{
	assert(fname);
	assert(ext);

	size_t len_fname = strlen(fname);
	size_t len_ext   = strlen(ext);

	if((len_fname > 0) &&
	   (len_ext   > 0) &&
	   (len_fname >= len_ext) &&
	   (strcmp(&fname[len_fname - len_ext], ext) == 0))
	{
		return 1;
	}

	return 0;
}

static void usage(const char* argv0)
{
	LOGE("usage1: %s [format] [src] [dst]", argv0);
	LOGE("usage2: %s [format] [dx] [dy] [src.mgm] [dst]", argv0);
	LOGE("RGBA-8888   - texgz, png");
	LOGE("BGRA-8888   - texgz");
	LOGE("RGB-565     - texgz");
	LOGE("RGBA-4444   - texgz");
	LOGE("RGB-888     - texgz, png, jpg");
	LOGE("RGBA-5551   - texgz");
	LOGE("LUMINANCE   - texgz");
	LOGE("LUMINANCE-F - texgz");
}

int main(int argc, char** argv)
{
	unsigned char dx = 0;
	unsigned char dy = 0;

	const char* arg_format = NULL;
	const char* arg_src    = NULL;
	const char* arg_dst    = NULL;

	if(argc == 6)
	{
		dx = (unsigned char) strtoul(argv[2], (char**) NULL, 10);
		dy = (unsigned char) strtoul(argv[3], (char**) NULL, 10);

		arg_format = argv[1];
		arg_src    = argv[4];
		arg_dst    = argv[5];
	}
	else if(argc == 4)
	{
		arg_format = argv[1];
		arg_src    = argv[2];
		arg_dst    = argv[3];

		if(check_ext(arg_src, "mgm"))
		{
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}
	else
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	// parse format
	int type;
	int format;
	if(strcmp(arg_format, "RGBA-8888") == 0)
	{
		type  = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_RGBA;
	}
	else if(strcmp(arg_format, "BGRA-8888") == 0)
	{
		type  = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_BGRA;
	}
	else if(strcmp(arg_format, "RGB-565") == 0)
	{
		type  = TEXGZ_UNSIGNED_SHORT_5_6_5;
		format = TEXGZ_RGB;
	}
	else if(strcmp(arg_format, "RGBA-4444") == 0)
	{
		type  = TEXGZ_UNSIGNED_SHORT_4_4_4_4;
		format = TEXGZ_RGBA;
	}
	else if(strcmp(arg_format, "RGB-888") == 0)
	{
		type  = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_RGB;
	}
	else if(strcmp(arg_format, "RGBA-5551") == 0)
	{
		type  = TEXGZ_UNSIGNED_SHORT_5_5_5_1;
		format = TEXGZ_RGBA;
	}
	else if(strcmp(arg_format, "LUMINANCE") == 0)
	{
		type  = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_LUMINANCE;
	}
	else if(strcmp(arg_format, "LUMINANCE-F") == 0)
	{
		type  = TEXGZ_FLOAT;
		format = TEXGZ_LUMINANCE;
	}
	else
	{
		LOGE("invalid format=%s", arg_format);
		return EXIT_FAILURE;
	}

	// import src
	texgz_tex_t* tex = NULL;
	if(check_ext(arg_src, "texgz"))
	{
		tex = texgz_tex_import(arg_src);
	}
	else if(check_ext(arg_src, "jpg"))
	{
		tex = texgz_jpeg_import(arg_src);
	}
	else if(check_ext(arg_src, "png"))
	{
		tex = texgz_png_import(arg_src);
	}
	else if(check_ext(arg_src, "mgm"))
	{
		tex = texgz_mgm_import(arg_src, dx, dy);
	}
	else
	{
		LOGE("invalid src=%s", arg_src);
		return EXIT_FAILURE;
	}

	if(tex == NULL)
	{
		return EXIT_FAILURE;
	}

	// convert to format
	if(texgz_tex_convert(tex, type, format) == 0)
	{
		goto fail_convert;
	}

	// export dst
	int ret = 0;
	if(check_ext(arg_dst, "texgz"))
	{
		ret = texgz_tex_export(tex, arg_dst);
	}
	else if(check_ext(arg_dst, "jpg"))
	{
		ret = texgz_jpeg_export(tex, arg_dst);
	}
	else if(check_ext(arg_dst, "png"))
	{
		ret = texgz_png_export(tex, arg_dst);
	}
	else
	{
		LOGE("invalid dst=%s", arg_dst);
		goto fail_dst;
	}

	if(ret == 0)
	{
		goto fail_export;
	}

	texgz_tex_delete(&tex);

	// success
	return EXIT_SUCCESS;

	// failure
	fail_export:
	fail_dst:
	fail_convert:
		texgz_tex_delete(&tex);
	return EXIT_FAILURE;
}

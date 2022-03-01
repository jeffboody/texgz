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
#include <string.h>

#define LOG_TAG "texgz"
#include "libcc/cc_log.h"
#include "texgz/texgz_tex.h"
#include "texgz/texgz_jpeg.h"
#include "texgz/texgz_png.h"
#ifdef TEXGZ_USE_JP2
	// To enable JP2 support
	// TEXGZ_USE_JP2=1 make
	#include "texgz/texgz_jp2.h"
#endif

static int check_ext(const char* fname, const char* ext)
{
	ASSERT(fname);
	ASSERT(ext);

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
	printf("Check Image Info:\n");
	printf("%s [check-image]\n", argv0);
	printf("\n");
	printf("Convert Image Format:\n");
	printf("%s [format] [src-image] [dst-image]\n",
	     argv0);
	printf("RGBA-8888   - texgz, texz, png\n");
	printf("BGRA-8888   - texgz, texz\n");
	printf("RGB-565     - texgz, texz\n");
	printf("RGBA-4444   - texgz, texz\n");
	printf("RGB-888     - texgz, texz, png, jpg\n");
	printf("RGBA-5551   - texgz, texz\n");
	printf("LUMINANCE   - texgz, texz, png\n");
	printf("ALPHA       - texgz, texz, png\n");
	printf("LUMINANCE-A - texgz, texz\n");
	printf("LUMINANCE-F - texgz, texz\n");
}

int main(int argc, char** argv)
{
	const char* arg_format = NULL;
	const char* arg_src    = NULL;
	const char* arg_dst    = NULL;

	int check_info = 0;
	if(argc == 2)
	{
		arg_src    = argv[1];
		check_info = 1;
	}
	else if(argc == 4)
	{
		arg_format = argv[1];
		arg_src    = argv[2];
		arg_dst    = argv[3];
	}
	else
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	// parse format
	int type   = 0;
	int format = 0;
	if(arg_format == NULL)
	{
		// skip
	}
	else if(strcmp(arg_format, "RGBA-8888") == 0)
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
	else if(strcmp(arg_format, "ALPHA") == 0)
	{
		type  = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_ALPHA;
	}
	else if(strcmp(arg_format, "LUMINANCE-A") == 0)
	{
		type  = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_LUMINANCE_ALPHA;
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
	else if(check_ext(arg_src, "texz"))
	{
		tex = texgz_tex_importz(arg_src);
	}
	else if(check_ext(arg_src, "jpg"))
	{
		tex = texgz_jpeg_import(arg_src, TEXGZ_RGB);
	}
	else if(check_ext(arg_src, "png"))
	{
		tex = texgz_png_import(arg_src);
	}
	#ifdef TEXGZ_USE_JP2
		else if(check_ext(arg_src, "jp2"))
		{
			tex = texgz_jp2_import(arg_src);
		}
	#endif
	else
	{
		LOGE("invalid src=%s", arg_src);
		return EXIT_FAILURE;
	}

	if(tex == NULL)
	{
		return EXIT_FAILURE;
	}

	if(check_info)
	{
		LOGI("width=%i, height=%i",
		     tex->width, tex->height);
		LOGI("stirde=%i, vstride=%i",
		     tex->stride, tex->vstride);
		LOGI("type=0x%X, format=0x%X",
		     tex->type, tex->format);

		texgz_tex_delete(&tex);
		return EXIT_SUCCESS;
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
	else if(check_ext(arg_dst, "texz"))
	{
		ret = texgz_tex_exportz(tex, arg_dst);
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

/*
 * Copyright (c) 2016 Jeff Boody
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

#include <openjpeg.h>
#include <stdlib.h>

#define LOG_TAG "texgz"
#include "libcc/cc_log.h"
#include "texgz_jp2.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
info_callback(const char *msg, void *client_data)
{
	// ignore
}

static void
warning_callback(const char *msg, void *client_data)
{
	// ignore
}

static void
error_callback(const char *msg, void *client_data)
{
	// client_data may be NULL
	ASSERT(msg);
	LOGE("%s", msg);
}

static void texgz_jp2_packRGBA(texgz_tex_t* self,
                               opj_image_t* image)
{
	ASSERT(self);
	ASSERT(image);

	// conversion derived from OpenJPEG convertpng.c
	int prec = (int) image->comps[0].prec;
	OPJ_INT32 adjust;
	adjust = image->comps[0].sgnd ? 1 << (prec - 1): 0;

	// data pointers
	OPJ_INT32*     src0 = image->comps[0].data;
	OPJ_INT32*     src1 = image->comps[1].data;
	OPJ_INT32*     src2 = image->comps[2].data;
	OPJ_INT32*     src3 = image->comps[3].data;
	unsigned char* dst  = self->pixels;

	int s = 0;
	int d = 0;
	int idx;
	int count = self->width*self->height;
	for(idx = 0; idx < count; ++idx)
	{
		dst[d++] = (unsigned char) (src0[s] + adjust);
		dst[d++] = (unsigned char) (src1[s] + adjust);
		dst[d++] = (unsigned char) (src2[s] + adjust);
		dst[d++] = (unsigned char) (src3[s] + adjust);
		++s;
	}
}

static void texgz_jp2_packRGB(texgz_tex_t* self,
                              opj_image_t* image)
{
	ASSERT(self);
	ASSERT(image);

	// conversion derived from OpenJPEG convertpng.c
	int prec = (int) image->comps[0].prec;
	OPJ_INT32 adjust;
	adjust = image->comps[0].sgnd ? 1 << (prec - 1): 0;

	// data pointers
	OPJ_INT32*     src0 = image->comps[0].data;
	OPJ_INT32*     src1 = image->comps[1].data;
	OPJ_INT32*     src2 = image->comps[2].data;
	unsigned char* dst  = self->pixels;

	int s = 0;
	int d = 0;
	int idx;
	int count = self->width*self->height;
	for(idx = 0; idx < count; ++idx)
	{
		dst[d++] = (unsigned char) (src0[s] + adjust);
		dst[d++] = (unsigned char) (src1[s] + adjust);
		dst[d++] = (unsigned char) (src2[s] + adjust);
		++s;
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

texgz_tex_t* texgz_jp2_import(const char* fname)
{
	ASSERT(fname);

	opj_stream_t* l_stream;
	l_stream = opj_stream_create_default_file_stream(fname, 1);
	if(l_stream == NULL)
	{
		LOGE("opj_stream_create_default_file_stream failed");
		return NULL;
	}

	opj_codec_t* l_codec = NULL;
	l_codec = opj_create_decompress(OPJ_CODEC_JP2);
	if(l_codec == NULL)
	{
		LOGE("opj_create_decompress failed");
		goto fail_codec;
	}

	opj_set_info_handler(l_codec, info_callback, NULL);
	opj_set_warning_handler(l_codec, warning_callback, NULL);
	opj_set_error_handler(l_codec, error_callback, NULL);

	opj_image_t* image = NULL;
	if(opj_read_header(l_stream, l_codec, &image) == 0)
	{
		// opj_decompress.c destroys the image even on failure
		opj_image_destroy(image);

		LOGE("opj_read_header failed");
		goto fail_header;
	}

	if(opj_decode(l_codec, l_stream, image) == 0)
	{
		LOGE("opj_decode failed");
		goto fail_decode;
	}

	if(opj_end_decompress(l_codec, l_stream) == 0)
	{
		LOGE("opj_end_decompress failed");
		goto fail_end;
	}

	/*
	 * validate image
	 */

	if((image->x0 != 0) || (image->y0 != 0))
	{
		LOGE("invalid x0=%u, y0=%u", image->x0, image->y0);
		goto fail_validate;
	}

	if(image->color_space != OPJ_CLRSPC_GRAY)
	{
		LOGW("invalid color_space=0x%X", image->color_space);
	}

	int format;
	if(image->numcomps == 3)
	{
		format = TEXGZ_RGB;
	}
	else if(image->numcomps == 4)
	{
		format = TEXGZ_RGBA;
	}
	else
	{
		LOGE("invalid numcomps=%u", image->numcomps);
		goto fail_validate;
	}

	// all components should have the same parameters
	int i;
	for(i = 0; i < image->numcomps; ++i)
	{
		if((image->comps[i].x0   != 0)         ||
		   (image->comps[i].y0   != 0)         ||
		   (image->comps[i].prec != 8)         ||
		   (image->comps[i].w    != image->x1) ||
		   (image->comps[i].h    != image->y1))
		{
			LOGE("i=%i: numcomps=%i, x0=%i, y0=%i, "
			     "prec=%i, w=%i, h=%i, x1=%i, y1=%i",
			     i, image->numcomps,
			     image->comps[i].x0, image->comps[i].y0,
			     image->comps[i].prec,
			     image->comps[i].w, image->comps[i].h,
			     image->x1, image->y1);
			goto fail_validate;
		}
	}

	int type = TEXGZ_UNSIGNED_BYTE;
	texgz_tex_t* self = texgz_tex_new(image->comps[0].w,
	                                  image->comps[0].h,
	                                  image->comps[0].w,
	                                  image->comps[0].h,
	                                  type, format, NULL);
	if(self == NULL)
	{
		goto fail_new;
	}

	// pack jp2 image
	if(format == TEXGZ_RGBA)
	{
		texgz_jp2_packRGBA(self, image);
	}
	else
	{
		texgz_jp2_packRGB(self, image);
	}

	opj_image_destroy(image);
	opj_destroy_codec(l_codec);
	opj_stream_destroy(l_stream);

	// success
	return self;

	// failure
	fail_new:
	fail_validate:
	fail_end:
	fail_decode:
		opj_image_destroy(image);
	fail_header:
		opj_destroy_codec(l_codec);
	fail_codec:
		opj_stream_destroy(l_stream);
	return NULL;
}

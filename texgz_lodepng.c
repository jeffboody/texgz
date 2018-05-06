/*
 * Copyright (c) 2015 Jeff Boody
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
#include "texgz_png.h"
#include "../lodepng/lodepng.cpp"

#define LOG_TAG "texgz"
#include "texgz_log.h"

/*
 * private
 */

static int texgz_png_exportRGBA(texgz_tex_t* self, const char* fname)
{
	assert(self);
	assert(fname);
	LOGD("debug fname=%s", fname);

	texgz_tex_t* tex = NULL;
	tex = texgz_tex_convertcopy(self, TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA);
	if(tex == NULL)
	{
		return 0;
	}

	unsigned err;
	err = lodepng_encode32_file(fname, tex->pixels,
	                            tex->stride, tex->vstride);
	if(err)
	{
		LOGE("invalid %s : %s", fname, lodepng_error_text(err));
		goto fail_encode;
	}
	texgz_tex_delete(&tex);

	// success
	return 1;

	// failure
	fail_encode:
		texgz_tex_delete(&tex);
	return 0;
}

static int texgz_png_exportRGB(texgz_tex_t* self, const char* fname)
{
	assert(self);
	assert(fname);
	LOGD("debug fname=%s", fname);

	texgz_tex_t* tex = NULL;
	tex = texgz_tex_convertcopy(self, TEXGZ_UNSIGNED_BYTE, TEXGZ_RGB);
	if(tex == NULL)
	{
		return 0;
	}

	unsigned err;
	err = lodepng_encode24_file(fname, tex->pixels,
	                            tex->stride, tex->vstride);
	if(err)
	{
		LOGE("invalid %s : %s", fname, lodepng_error_text(err));
		goto fail_encode;
	}
	texgz_tex_delete(&tex);

	// success
	return 1;

	// failure
	fail_encode:
		texgz_tex_delete(&tex);
	return 0;
}

/*
 * public
 */

texgz_tex_t* texgz_png_import(const char* fname)
{
	assert(fname);
	LOGD("debug fname=%s", fname);

	unsigned char* img;
	unsigned w;
	unsigned h;
	unsigned err = lodepng_decode32_file(&img, &w, &h, fname);
	if(err)
	{
		LOGE("invalid %s : %s", fname, lodepng_error_text(err));
		return NULL;
	}

	texgz_tex_t* self;
	self = texgz_tex_new(w, h, w, h,
	                     TEXGZ_UNSIGNED_BYTE,
	                     TEXGZ_RGBA,
	                     img);
	if(self == NULL)
	{
		goto fail_tex;
	}
	free(img);

	// success
	return self;

	// failure
	fail_tex:
		free(img);
	return NULL;
}

int texgz_png_export(texgz_tex_t* self, const char* fname)
{
	assert(self);
	assert(fname);
	LOGD("debug fname=%s", fname);

	if(self->format == TEXGZ_RGBA)
	{
		return texgz_png_exportRGBA(self, fname);
	}
	return texgz_png_exportRGB(self, fname);
}

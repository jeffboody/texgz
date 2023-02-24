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

#define LOG_TAG "texgz"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "../lodepng/lodepng.cpp"
#include "texgz_png.h"

/*
 * private
 */

static int
texgz_png_exportAs(texgz_tex_t* self,
                   int format,
                   LodePNGColorType colortype,
                   const char* fname)
{
	ASSERT(self);
	ASSERT(fname);

	int delete_tex = 0;
	texgz_tex_t* tex = self;
	if((tex->type   != TEXGZ_UNSIGNED_BYTE) ||
	   (tex->format != format)              ||
	   (tex->width  != tex->stride)         ||
	   (tex->height != tex->vstride))
	{
		tex = texgz_tex_convertcopy(self, TEXGZ_UNSIGNED_BYTE,
		                            format);
		if(tex == NULL)
		{
			return 0;
		}
		delete_tex = 1;

		if(texgz_tex_crop(tex, 0, 0, tex->height - 1,
		                  tex->width - 1) == 0)
		{
			goto fail_tex;
		}
	}

	unsigned err;
	err = lodepng_encode_file(fname, tex->pixels,
	                          tex->stride, tex->vstride,
	                          colortype, 8);
	if(err)
	{
		LOGE("invalid %s : %s", fname, lodepng_error_text(err));
		goto fail_encode;
	}

	if(delete_tex)
	{
		texgz_tex_delete(&tex);
	}

	// success
	return 1;

	// failure
	fail_encode:
	fail_tex:
		if(delete_tex)
		{
			texgz_tex_delete(&tex);
		}
	return 0;
}

static int
texgz_png_compressAs(texgz_tex_t* self,
                     int format,
                     LodePNGColorType colortype,
                     void** _data,
                     size_t* _size)
{
	ASSERT(self);
	ASSERT(_data);
	ASSERT(_size);

	// _data is allocated by C malloc

	int delete_tex = 0;
	texgz_tex_t* tex = self;
	if((tex->type   != TEXGZ_UNSIGNED_BYTE) ||
	   (tex->format != format)              ||
	   (tex->width  != tex->stride)         ||
	   (tex->height != tex->vstride))
	{
		tex = texgz_tex_convertcopy(self, TEXGZ_UNSIGNED_BYTE,
		                            format);
		if(tex == NULL)
		{
			return 0;
		}
		delete_tex = 1;

		if(texgz_tex_crop(tex, 0, 0, tex->height - 1,
		                  tex->width - 1) == 0)
		{
			goto fail_tex;
		}
	}

	unsigned err;
	err = lodepng_encode_memory((unsigned char**) _data,
	                            _size, tex->pixels,
	                            tex->stride, tex->vstride,
	                            colortype, 8);
	if(err)
	{
		LOGE("invalid %s", lodepng_error_text(err));
		goto fail_encode;
	}

	if(delete_tex)
	{
		texgz_tex_delete(&tex);
	}

	// success
	return 1;

	// failure
	fail_encode:
	fail_tex:
		if(delete_tex)
		{
			texgz_tex_delete(&tex);
		}
	return 0;
}

/*
 * public
 */

texgz_tex_t* texgz_png_import(const char* fname)
{
	ASSERT(fname);

	FILE* f = fopen(fname, "r");
	if(f == NULL)
	{
		LOGE("invalid %s", fname);
		return NULL;
	}

	// get file lenth
	if(fseek(f, (long) 0, SEEK_END) == -1)
	{
		LOGE("fseek_end fname=%s", fname);
		goto fail_fseek_end;
	}
	size_t size = ftell(f);

	// rewind to start
	if(fseek(f, 0, SEEK_SET) == -1)
	{
		LOGE("fseek_set fname=%s", fname);
		goto fail_fseek_set;
	}

	texgz_tex_t* self;
	self = texgz_png_importf(f, size);
	if(self == NULL)
	{
		goto fail_import;
	}

	fclose(f);

	// success
	return self;

	// failure
	fail_import:
	fail_fseek_set:
	fail_fseek_end:
		fclose(f);
	return NULL;
}

texgz_tex_t* texgz_png_importf(FILE* f, size_t size)
{
	ASSERT(f);

	unsigned char* buf;
	buf = (unsigned char*)
	      CALLOC(size, sizeof(unsigned char));
	if(buf == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	if(fread(buf, size*sizeof(unsigned char), 1, f) != 1)
	{
		LOGE("fread failed");
		goto fail_fread;
	}

	texgz_tex_t* self;
	self = texgz_png_importd(size, buf);
	if(self == NULL)
	{
		goto fail_tex;
	}

	FREE(buf);

	// success
	return self;

	// failure
	fail_tex:
	fail_fread:
		FREE(buf);
	return NULL;
}

texgz_tex_t*
texgz_png_importd(size_t size, const void* data)
{
	ASSERT(data);

	unsigned err;
	unsigned w = 0;
	unsigned h = 0;
	LodePNGState state = { 0 };
	err = lodepng_inspect(&w, &h, &state, data, size);
	if(err)
	{
		LOGE("invalid %s", lodepng_error_text(err));
		return NULL;
	}

	int              format    = 0;
	LodePNGColorType colortype = state.info_png.color.colortype;
	if(colortype == LCT_GREY)
	{
		format = TEXGZ_ALPHA;
	}
	else if(colortype == LCT_GREY_ALPHA)
	{
		format = TEXGZ_LUMINANCE_ALPHA;
	}
	else if(colortype == LCT_RGB)
	{
		format = TEXGZ_RGB;
	}
	else
	{
		colortype = LCT_RGBA;
		format    = TEXGZ_RGBA;
	}

	unsigned char* img = NULL;
	err = lodepng_decode_memory(&img, &w, &h, data,
	                            size, colortype, 8);
	if(err)
	{
		LOGE("invalid %s", lodepng_error_text(err));
		return NULL;
	}

	texgz_tex_t* self;
	self = texgz_tex_new(w, h, w, h, TEXGZ_UNSIGNED_BYTE,
	                     format, img);
	if(self == NULL)
	{
		goto fail_tex;
	}

	// img allocated by standard C library
	free(img);

	// success
	return self;

	// failure
	fail_tex:
		// img allocated by standard C library
		free(img);
	return NULL;
}

int texgz_png_export(texgz_tex_t* self, const char* fname)
{
	ASSERT(self);
	ASSERT(fname);

	if(self->format == TEXGZ_RGB)
	{
		return texgz_png_exportAs(self,
		                          TEXGZ_RGB,
		                          LCT_RGB,
		                          fname);
	}
	else if(self->format == TEXGZ_LUMINANCE_ALPHA)
	{
		return texgz_png_exportAs(self,
		                          TEXGZ_LUMINANCE_ALPHA,
		                          LCT_GREY_ALPHA,
		                          fname);
	}
	else if(self->format == TEXGZ_LUMINANCE)
	{
		return texgz_png_exportAs(self,
		                          TEXGZ_LUMINANCE,
		                          LCT_GREY,
		                          fname);
	}
	else if(self->format == TEXGZ_ALPHA)
	{
		return texgz_png_exportAs(self,
		                          TEXGZ_ALPHA,
		                          LCT_GREY,
		                          fname);
	}
	else
	{
		return texgz_png_exportAs(self,
		                          TEXGZ_RGBA,
		                          LCT_RGBA,
		                          fname);
	}
}

int texgz_png_compress(texgz_tex_t* self,
                       void** _data,
                       size_t* _size)
{
	ASSERT(self);
	ASSERT(_data);
	ASSERT(_size);

	if(self->format == TEXGZ_RGB)
	{
		return texgz_png_compressAs(self,
		                            TEXGZ_RGB,
		                            LCT_RGB,
		                            _data, _size);
	}
	else if(self->format == TEXGZ_LUMINANCE_ALPHA)
	{
		return texgz_png_compressAs(self,
		                            TEXGZ_LUMINANCE_ALPHA,
		                            LCT_GREY_ALPHA,
		                            _data, _size);
	}
	else if(self->format == TEXGZ_LUMINANCE)
	{
		return texgz_png_compressAs(self,
		                            TEXGZ_LUMINANCE,
		                            LCT_GREY,
		                            _data, _size);
	}
	else if(self->format == TEXGZ_ALPHA)
	{
		return texgz_png_compressAs(self,
		                            TEXGZ_ALPHA,
		                            LCT_GREY,
		                            _data, _size);
	}
	else
	{
		return texgz_png_compressAs(self,
		                            TEXGZ_RGBA,
		                            LCT_RGBA,
		                            _data, _size);
	}
}

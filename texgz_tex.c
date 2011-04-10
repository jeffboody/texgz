/*
 * Copyright (c) 2011 Jeff Boody
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

#include "texgz_tex.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <zlib.h>
#include <math.h>

/*
 * private - log api
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

static void texgz_log(const char* func, int line, const char* fmt, ...)
{
	assert(func);
	assert(fmt);

	char buf[256];
	snprintf(buf, 256, "%s@%i ", func, line);

	int size = (int) strlen(buf);
	if(size < 256)
	{
		va_list argptr;
		va_start(argptr, fmt);
		vsnprintf(&buf[size], 256 - size, fmt, argptr);
		va_end(argptr);
	}
	printf("%s\n", buf);
}

#ifdef LOG_DEBUG
	#define LOGD(...) (texgz_log(__func__, __LINE__, __VA_ARGS__))
#else
	#define LOGD(...)
#endif
#define LOGI(...) (texgz_log(__func__, __LINE__, __VA_ARGS__))
#define LOGE(...) (texgz_log(__func__, __LINE__, __VA_ARGS__))

/*
 * private - table conversion functions
 */

static void texgz_table_1to8(unsigned char* table)
{
	assert(table);
	LOGD("debug");

	int i;
	for(i = 0; i < 2; ++i)
		table[i] = (unsigned char) (i * 255.0f + 0.5f);
}

static void texgz_table_4to8(unsigned char* table)
{
	assert(table);
	LOGD("debug");

	int i;
	for(i = 0; i < 16; ++i)
		table[i] = (unsigned char) (i * 255.0f / 15.0f + 0.5f);
}

static void texgz_table_5to8(unsigned char* table)
{
	assert(table);
	LOGD("debug");

	int i;
	for(i = 0; i < 32; ++i)
		table[i] = (unsigned char) (i * 255.0f / 31.0f + 0.5f);
}

static void texgz_table_6to8(unsigned char* table)
{
	assert(table);
	LOGD("debug");

	int i;
	for(i = 0; i < 64; ++i)
		table[i] = (unsigned char) (i * 255.0f / 63.0f + 0.5f);
}

/*
 * private - conversion functions
 */

static texgz_tex_t* texgz_tex_4444to8888(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	if((self->type != TEXGZ_UNSIGNED_SHORT_4_4_4_4) || (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
                                     self->stride, self->vstride,
                                     TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA,
                                     NULL);
	if(tex == NULL)
		return NULL;

	unsigned char table_4to8[16];
	texgz_table_4to8(table_4to8);

	int x, y, idx;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[2*idx];
			unsigned char* dst = &tex->pixels[4*idx];

			dst[0] = table_4to8[(src[1] >> 4) & 0xF];   // r
			dst[1] = table_4to8[src[1]        & 0xF];   // g
			dst[2] = table_4to8[(src[0] >> 4) & 0xF];   // b
			dst[3] = table_4to8[src[0]        & 0xF];   // a
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_565to8888(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	if((self->type != TEXGZ_UNSIGNED_SHORT_5_6_5) || (self->format != TEXGZ_RGB))
	{
		LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
                                     self->stride, self->vstride,
                                     TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA,
                                     NULL);
	if(tex == NULL)
		return NULL;

	unsigned char table_5to8[32];
	unsigned char table_6to8[64];
	texgz_table_5to8(table_5to8);
	texgz_table_6to8(table_6to8);

	int x, y, idx;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned short* src = (unsigned short*) (&self->pixels[2*idx]);
			unsigned char*  dst = &tex->pixels[4*idx];

			dst[0] = table_5to8[(src[0] >> 11) & 0x1F];   // r
			dst[1] = table_6to8[(src[0] >> 5) & 0x3F];    // g
			dst[2] = table_5to8[src[0] & 0x1F];           // b
			dst[3] = 0xFF;                                // a
		}                                              
	}                                                  

	return tex;
}

static texgz_tex_t* texgz_tex_5551to8888(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	if((self->type != TEXGZ_UNSIGNED_SHORT_5_5_5_1) || (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
                                     self->stride, self->vstride,
                                     TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA,
                                     NULL);
	if(tex == NULL)
		return NULL;

	unsigned char table_5to8[32];
	unsigned char table_1to8[2];
	texgz_table_5to8(table_5to8);
	texgz_table_1to8(table_1to8);

	int x, y, idx;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned short* src = (unsigned short*) (&self->pixels[2*idx]);
			unsigned char*  dst = &tex->pixels[4*idx];

			dst[0] = table_5to8[(src[0] >> 11) & 0x1F];   // r
			dst[1] = table_5to8[(src[0] >> 6)  & 0x1F];   // g
			dst[2] = table_5to8[(src[0] >> 1)  & 0x1F];   // b
			dst[3] = table_1to8[src[0]         & 0x1];    // a
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_888to8888(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	if((self->type != TEXGZ_UNSIGNED_BYTE) || (self->format != TEXGZ_RGB))
	{
		LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
                                     self->stride, self->vstride,
                                     TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA,
                                     NULL);
	if(tex == NULL)
		return NULL;

	int x, y, idx;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[3*idx];
			unsigned char* dst = &tex->pixels[4*idx];

			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = 0xFF;
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8to8888(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	if((self->type != TEXGZ_UNSIGNED_BYTE) || (self->format != TEXGZ_LUMINANCE))
	{
		LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
                                     self->stride, self->vstride,
                                     TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA,
                                     NULL);
	if(tex == NULL)
		return NULL;

	int x, y, idx;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[idx];
			unsigned char* dst = &tex->pixels[4*idx];

			dst[0] = src[0];
			dst[1] = src[0];
			dst[2] = src[0];
			dst[3] = 0xFF;
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888to4444(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	if((self->type != TEXGZ_UNSIGNED_BYTE) || (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
                                     self->stride, self->vstride,
                                     TEXGZ_UNSIGNED_SHORT_4_4_4_4, TEXGZ_RGBA,
                                     NULL);
	if(tex == NULL)
		return NULL;

	int x, y, idx;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[4*idx];
			unsigned char* dst = &tex->pixels[2*idx];

            unsigned char r = (src[0] >> 4) & 0x0F;
            unsigned char g = (src[1] >> 4) & 0x0F;
            unsigned char b = (src[2] >> 4) & 0x0F;
            unsigned char a = (src[3] >> 4) & 0x0F;

			dst[0] = a | (b << 4);
			dst[1] = g | (r << 4);
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888to565(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	if((self->type != TEXGZ_UNSIGNED_BYTE) || (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
                                     self->stride, self->vstride,
                                     TEXGZ_UNSIGNED_SHORT_5_6_5, TEXGZ_RGB,
                                     NULL);
	if(tex == NULL)
		return NULL;

	int x, y, idx;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[4*idx];
			unsigned char* dst = &tex->pixels[2*idx];

            unsigned char r = (src[0] >> 3) & 0x1F;
            unsigned char g = (src[1] >> 2) & 0x3F;
            unsigned char b = (src[2] >> 3) & 0x1F;

			// RGB <- least significant
			dst[0] = b | ((g << 5) & 0xE0);   // GB
			dst[1] = (g >> 3) | (r << 3);     // RG
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888to5551(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	if((self->type != TEXGZ_UNSIGNED_BYTE) || (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
                                     self->stride, self->vstride,
                                     TEXGZ_UNSIGNED_SHORT_5_5_5_1, TEXGZ_RGBA,
                                     NULL);
	if(tex == NULL)
		return NULL;

	int x, y, idx;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[4*idx];
			unsigned char* dst = &tex->pixels[2*idx];

            unsigned char r = (src[0] >> 3) & 0x1F;
            unsigned char g = (src[1] >> 3) & 0x1F;
            unsigned char b = (src[2] >> 3) & 0x1F;
            unsigned char a = (src[3] >> 7) & 0x01;

			// RGBA <- least significant
			dst[0] = a | ((b << 1) & 0x3E) | ((g << 6) & 0xC0);   // GBA
			dst[1] = (g >> 2) | ((r << 3) & 0xF8);                // RG
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888to888(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	if((self->type != TEXGZ_UNSIGNED_BYTE) || (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
                                     self->stride, self->vstride,
                                     TEXGZ_UNSIGNED_BYTE, TEXGZ_RGB,
                                     NULL);
	if(tex == NULL)
		return NULL;

	int x, y, idx;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[4*idx];
			unsigned char* dst = &tex->pixels[3*idx];

			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888to8(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	if((self->type != TEXGZ_UNSIGNED_BYTE) || (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
                                     self->stride, self->vstride,
                                     TEXGZ_UNSIGNED_BYTE, TEXGZ_LUMINANCE,
                                     NULL);
	if(tex == NULL)
		return NULL;

	int x, y, idx;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[4*idx];
			unsigned char* dst = &tex->pixels[idx];

			// use the red channel for luminance
			dst[0] = src[0];
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888format(texgz_tex_t* self, int format)
{
	assert(self);
	LOGD("debug format=0x%X", format);

	if(self->type != TEXGZ_UNSIGNED_BYTE)
	{
		LOGE("invalid type=%i", self->type);
		return NULL;
	}

	int r, g, b, a;
	if((self->format == TEXGZ_RGBA) && (format == TEXGZ_BGRA))
	{
		r = 2;
		g = 1;
		b = 0;
		a = 3;
	}
	else if((self->format == TEXGZ_BGRA) && (format == TEXGZ_RGBA))
	{
		r = 2;
		g = 1;
		b = 0;
		a = 3;
	}
	else
	{
		LOGE("invalid type=0x%X, format=0x%X, dst_format=0x%X", self->type, self->format, format);
		return NULL;
	}

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
                                     self->stride, self->vstride,
                                     self->type, format,
                                     NULL);
	if(tex == NULL)
		return NULL;

	int x, y, idx;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[4*idx];
			unsigned char* dst = &tex->pixels[4*idx];

			dst[0] = src[r];
			dst[1] = src[g];
			dst[2] = src[b];
			dst[3] = src[a];
		}
	}

	return tex;
}

/*
 * private
 */

static int texgz_readint(const unsigned char* buffer, int offset)
{
	assert(buffer);
	assert(offset >= 0);

	int b0 = (int) buffer[offset + 0];
	int b1 = (int) buffer[offset + 1];
	int b2 = (int) buffer[offset + 2];
	int b3 = (int) buffer[offset + 3];
	int o = (b3 << 24) & 0xFF000000;
	o = o | ((b2 << 16) & 0x00FF0000);
	o = o | ((b1 << 8) & 0x0000FF00);
	o = o | (b0 & 0x000000FF);
	return o;
}

static int texgz_swapendian(int i)
{
	int o = (i << 24) & 0xFF000000;
	o = o | ((i << 8) & 0x00FF0000);
	o = o | ((i >> 8) & 0x0000FF00);
	o = o | ((i >> 24) & 0x000000FF);
	return o;
}

/*
 * public
 */

texgz_tex_t* texgz_tex_new(int width, int height,
                           int stride, int vstride,
                           int type, int format,
                           unsigned char* pixels)
{
	// pixels can be NULL
	LOGD("debug width=%i, height=%i, stride=%i, vstride=%i, type=0x%X, format=0x%X, pixels=%p",
	     width, height, stride, vstride, type, format, pixels);

	if((stride <= 0) || (width > stride))
	{
		LOGE("invalid width=%i, stride=%i", width, stride);
		return NULL;
	}

	if((vstride <= 0) || (height > vstride))
	{
		LOGE("invalid height=%i, vstride=%i", height, vstride);
		return NULL;
	}

	if((type == TEXGZ_UNSIGNED_SHORT_4_4_4_4) && (format == TEXGZ_RGBA))
		; // ok
	else if((type == TEXGZ_UNSIGNED_SHORT_5_5_5_1) && (format == TEXGZ_RGBA))
		; // ok
	else if((type == TEXGZ_UNSIGNED_SHORT_5_6_5) && (format == TEXGZ_RGB))
		; // ok
	else if((type == TEXGZ_UNSIGNED_BYTE) && (format == TEXGZ_RGBA))
		; // ok
	else if((type == TEXGZ_UNSIGNED_BYTE) && (format == TEXGZ_RGB))
		; // ok
	else if((type == TEXGZ_UNSIGNED_BYTE) && (format == TEXGZ_LUMINANCE))
		; // ok
	else if((type == TEXGZ_UNSIGNED_BYTE) && (format == TEXGZ_BGRA))
		; // ok
	else
	{
		LOGE("invalid type=0x%X, format=0x%X", type, format);
		return NULL;
	}

	texgz_tex_t* self = (texgz_tex_t*) malloc(sizeof(texgz_tex_t));
	if(self == NULL)
	{
		LOGE("malloc failed");
		return NULL;
	}

	self->width   = width;
	self->height  = height;
	self->stride  = stride;
	self->vstride = vstride;
	self->type    = type;
	self->format  = format;

	int size = texgz_tex_size(self);
	if(size == 0)
	{
		LOGE("invalid type=%i, format=%i", type, format);
		goto fail_bpp;
	}

	self->pixels = (unsigned char*) malloc((size_t) size);
	if(self->pixels == NULL)
	{
		LOGE("malloc failed");
		goto fail_pixels;
	}

	if(pixels == NULL)
		memset(self->pixels, 0, size);
	else
		memcpy(self->pixels, pixels, size);

	// success
	return self;

	// failure
	fail_pixels:
	fail_bpp:
		free(self);
	return NULL;
}

void texgz_tex_delete(texgz_tex_t** _self)
{
	assert(_self);

	texgz_tex_t* self = *_self;
	if(self)
	{
		LOGD("debug");
		free(self->pixels);
		free(self);
		*_self = NULL;
	}
}

texgz_tex_t* texgz_tex_copy(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	return texgz_tex_new(self->width, self->height,
                         self->stride, self->vstride,
                         self->type, self->format,
                         self->pixels);
}

texgz_tex_t* texgz_tex_import(const char* filename)
{
	assert(filename);
	LOGD("debug filename=%s", filename);

	// open texture
	gzFile f = gzopen(filename, "rb");
	if(!f)
	{
		LOGE("gzopen failed");
		return NULL;
	}

	// read header (28 bytes)
	int type;
	int format;
	int width;
	int height;
	int stride;
	int vstride;
	unsigned char buffer[4096];   // 4KB
	int bytes_read = gzread(f, buffer, 28);
	if(bytes_read != 28)
	{
		LOGE("gzread failed to read header");
		goto fail_header;
	}

	int magic = texgz_readint(buffer, 0);
	if(magic == 0x000B00D9)
	{
		type    = texgz_readint(buffer, 4);
		format  = texgz_readint(buffer, 8);
		width   = texgz_readint(buffer, 12);
		height  = texgz_readint(buffer, 16);
		stride  = texgz_readint(buffer, 20);
		vstride = texgz_readint(buffer, 24);
	}
	else if(magic == 0xD9000B00)
	{
		type    = texgz_swapendian(texgz_readint(buffer, 4));
		format  = texgz_swapendian(texgz_readint(buffer, 8));
		width   = texgz_swapendian(texgz_readint(buffer, 12));
		height  = texgz_swapendian(texgz_readint(buffer, 16));
		stride  = texgz_swapendian(texgz_readint(buffer, 20));
		vstride = texgz_swapendian(texgz_readint(buffer, 24));
	}
	else
	{
		LOGE("bad magic=0x%.8X", magic);
		goto fail_magic;
	}

	texgz_tex_t* self = texgz_tex_new(width, height, stride, vstride, type, format, NULL);
	if(self == NULL)
		goto fail_tex;

	// get texture size and check for supported formats
	int bytes = texgz_tex_size(self);
	if(bytes == 0)
		goto fail_size;

	// read pixels
	unsigned char* pixels = self->pixels;
	while(bytes > 0)
	{
		int bytes_read = gzread(f, pixels, bytes);
		if(bytes_read == 0)
		{
			LOGE("failed to read pixels");
			goto fail_pixels;
		}
		pixels += bytes_read;
		bytes -= bytes_read;
	}

	// success
	gzclose(f);
	return self;

	// failure
	fail_pixels:
	fail_size:
		texgz_tex_delete(&self);
	fail_tex:
	fail_magic:
	fail_header:
		gzclose(f);
	return NULL;
}

int texgz_tex_export(texgz_tex_t* self, const char* filename)
{
	assert(self);
	assert(filename);
	LOGD("debug filename=%s", filename);

	gzFile f = gzopen(filename, "wb");
	if(f == NULL)
	{
		LOGE("fopen failed for %s", filename);
		return 0;
	}

	// write header
	int magic = TEXGZ_MAGIC;
	if(gzwrite(f, (const void*) &magic, sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write MAGIC");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->type, sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write type");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->format, sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write format");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->width, sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write width");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->height, sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write height");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->stride, sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write stride");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->vstride, sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write vstride");
		goto fail_header;
	}

	// get texture size and check for supported formats
	int bytes = texgz_tex_size(self);
	if(bytes == 0)
		goto fail_size;

	// write pixels
	unsigned char* pixels = self->pixels;
	while(bytes > 0)
	{
		int bytes_written = gzwrite(f, (const void*) pixels, bytes);
		if(bytes_written == 0)
		{
			LOGE("failed to write pixels");
			goto fail_pixels;
		}
		pixels += bytes_written;
		bytes -= bytes_written;
	}

	gzclose(f);

	// success
	return 1;

	// failure
	fail_pixels:
	fail_size:
	fail_header:
		gzclose(f);
	return 0;
}

int texgz_tex_convert(texgz_tex_t* self, int type, int format)
{
	assert(self);
	LOGD("debug type=0x%X, format=0x%X", type, format);

	texgz_tex_t* tex = texgz_tex_convertcopy(self, type, format);
	if(tex == NULL)
		return 0;

	// swap the data
	texgz_tex_t tmp = *self;
	*self = *tex;
	*tex = tmp;

	texgz_tex_delete(&tex);
	return 1;
}

texgz_tex_t* texgz_tex_convertcopy(texgz_tex_t* self, int type, int format)
{
	assert(self);
	LOGD("debug type=0x%X, format=0x%X", type, format);

	// copy in the case that the src and dst formats match
	// in particular this case prevents the conversion of 8888 to 8888
	if((type == self->type) && (format == self->format))
		return texgz_tex_copy(self);

	// convert to RGBA-8888
	texgz_tex_t* tmp        = NULL;
	int      tmp_delete = 1;   // delete if self is not 8888
	if((self->type == TEXGZ_UNSIGNED_SHORT_4_4_4_4) && (self->format == TEXGZ_RGBA))
		tmp = texgz_tex_4444to8888(self);
	else if((self->type == TEXGZ_UNSIGNED_SHORT_5_6_5) && (self->format == TEXGZ_RGB))
		tmp = texgz_tex_565to8888(self);
	else if((self->type == TEXGZ_UNSIGNED_SHORT_5_5_5_1) && (self->format == TEXGZ_RGBA))
		tmp = texgz_tex_5551to8888(self);
	else if((self->type == TEXGZ_UNSIGNED_BYTE) && (self->format == TEXGZ_RGB))
		tmp = texgz_tex_888to8888(self);
	else if((self->type == TEXGZ_UNSIGNED_BYTE) && (self->format == TEXGZ_LUMINANCE))
		tmp = texgz_tex_8to8888(self);
	else if((self->type == TEXGZ_UNSIGNED_BYTE) && (self->format == TEXGZ_BGRA))
		tmp = texgz_tex_8888format(self, TEXGZ_RGBA);
	else if((self->type == TEXGZ_UNSIGNED_BYTE) && (self->format == TEXGZ_RGBA))
	{
		tmp        = self;
		tmp_delete = 0;
	}

	// check for errors
	if(tmp == NULL)
	{
		LOGE("could not convert to 8888");
		return NULL;
	}

	// convert to requested format
	texgz_tex_t* tex = NULL;
	if((type == TEXGZ_UNSIGNED_SHORT_4_4_4_4) && (format == TEXGZ_RGBA))
		tex = texgz_tex_8888to4444(tmp);
	else if((type == TEXGZ_UNSIGNED_SHORT_5_6_5) && (format == TEXGZ_RGB))
		tex = texgz_tex_8888to565(tmp);
	else if((type == TEXGZ_UNSIGNED_SHORT_5_5_5_1) && (format == TEXGZ_RGBA))
		tex = texgz_tex_8888to5551(tmp);
	else if((type == TEXGZ_UNSIGNED_BYTE) && (format == TEXGZ_RGB))
		tex = texgz_tex_8888to888(tmp);
	else if((type == TEXGZ_UNSIGNED_BYTE) && (format == TEXGZ_LUMINANCE))
		tex = texgz_tex_8888to8(tmp);
	else if((type == TEXGZ_UNSIGNED_BYTE) && (format == TEXGZ_BGRA))
		tex = texgz_tex_8888format(tmp, TEXGZ_BGRA);
	else if((type == TEXGZ_UNSIGNED_BYTE) && (format == TEXGZ_RGBA))
	{
		// note that tex will never equal self because of the shortcut copy
		tex        = tmp;
		tmp_delete = 0;
	}

	// cleanup
	if(tmp_delete == 1)
		texgz_tex_delete(&tmp);

	// check for errors
	if(tex == NULL)
	{
		LOGE("could not convert to type=0x%X, format=0x%X", type, format);
		return NULL;
	}

	// success
	return tex;
}

int texgz_tex_flipvertical(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	texgz_tex_t* tex = texgz_tex_flipverticalcopy(self);
	if(tex == NULL)
		return 0;

	// swap the data
	texgz_tex_t tmp = *self;
	*self = *tex;
	*tex = tmp;

	texgz_tex_delete(&tex);
	return 1;
}

texgz_tex_t* texgz_tex_flipverticalcopy(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	texgz_tex_t* tex = texgz_tex_new(self->width, self->height,
	                                 self->stride, self->vstride,
	                                 self->type, self->format,
	                                 NULL);
	if(tex == NULL)
		return NULL;

	// vertical flip
	int y;
	int bpp     = texgz_tex_bpp(self);
	int stride  = self->stride;
	int vstride = self->vstride;
	for(y = 0; y < vstride; ++y)
	{
		unsigned char* src = &self->pixels[y*bpp*stride];
		unsigned char* dst = &tex->pixels[(vstride - 1 - y)*bpp*stride];
		memcpy(dst, src, bpp*stride);
	}
	return tex;
}

int texgz_tex_bpp(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	int bpp = 0;   // bytes-per-pixel
	if     ((self->type == TEXGZ_UNSIGNED_BYTE)          && (self->format == TEXGZ_RGB))       bpp = 3;
	else if((self->type == TEXGZ_UNSIGNED_BYTE)          && (self->format == TEXGZ_RGBA))      bpp = 4;
	else if((self->type == TEXGZ_UNSIGNED_BYTE)          && (self->format == TEXGZ_BGRA))      bpp = 4;
	else if((self->type == TEXGZ_UNSIGNED_BYTE)          && (self->format == TEXGZ_LUMINANCE)) bpp = 1;
	else if((self->type == TEXGZ_UNSIGNED_SHORT_5_6_5)   && (self->format == TEXGZ_RGB))       bpp = 2;
	else if((self->type == TEXGZ_UNSIGNED_SHORT_4_4_4_4) && (self->format == TEXGZ_RGBA))      bpp = 2;
	else if((self->type == TEXGZ_UNSIGNED_SHORT_5_5_5_1) && (self->format == TEXGZ_RGBA))      bpp = 2;
	else LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);

	return bpp;
}

int texgz_tex_size(texgz_tex_t* self)
{
	assert(self);
	LOGD("debug");

	return texgz_tex_bpp(self) * self->stride * self->vstride;
}

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

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define LOG_TAG "texgz"
#include "../libcc/math/cc_float.h"
#include "../libcc/math/cc_pow2n.h"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "../libcc/math/cc_float.h"
#include "pil_lanczos.h"
#include "texgz_tex.h"

#define TEXGZ_LANCZOS3_MAXSIZE 257

/*
 * private - optimizations
 */

#define TEXGZ_UNROLL_EDGE3X3

/*
 * private - outline mask
 * see outline-mask.xcf
 */

float TEXGZ_TEX_OUTLINE3[9] =
{
	0.5f, 1.0f, 0.5f,
	1.0f, 1.0f, 1.0f,
	0.5f, 1.0f, 0.5f,
};

float TEXGZ_TEX_OUTLINE5[25] =
{
	0.19f, 0.75f, 1.00f, 0.75f, 0.19f,
	0.75f, 1.00f, 1.00f, 1.00f, 0.75f,
	1.00f, 1.00f, 1.00f, 1.00f, 1.00f,
	0.75f, 1.00f, 1.00f, 1.00f, 0.75f,
	0.19f, 0.75f, 1.00f, 0.75f, 0.19f,
};

float TEXGZ_TEX_OUTLINE7[49] =
{
	0.00f, 0.31f, 0.88f, 1.00f, 0.88f, 0.31f, 0.00f,
	0.31f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.31f,
	0.88f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.88f,
	1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f,
	0.88f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.88f,
	0.31f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.31f,
	0.00f, 0.31f, 0.88f, 1.00f, 0.88f, 0.31f, 0.00f,
};

float TEXGZ_TEX_OUTLINE9[81] =
{
	0.00f, 0.06f, 0.50f, 0.88f, 1.00f, 0.88f, 0.50f, 0.06f, 0.00f,
	0.06f, 0.81f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.81f, 0.06f,
	0.50f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.50f,
	0.88f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.88f,
	1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f,
	0.88f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.88f,
	0.50f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.50f,
	0.06f, 0.81f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.81f, 0.06f,
	0.00f, 0.06f, 0.50f, 0.88f, 1.00f, 0.88f, 0.50f, 0.06f, 0.00f,
};

float TEXGZ_TEX_OUTLINE11[121] =
{
	0.00f, 0.00f, 0.13f, 0.63f, 0.88f, 1.00f, 0.88f, 0.63f, 0.13f, 0.00f, 0.00f,
	0.00f, 0.38f, 0.94f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.94f, 0.38f, 0.00f,
	0.13f, 0.94f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.94f, 0.13f,
	0.63f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.63f,
	0.88f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.88f,
	1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f,
	0.88f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.88f,
	0.63f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.63f,
	0.13f, 0.94f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.94f, 0.13f,
	0.00f, 0.38f, 0.94f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.94f, 0.38f, 0.00f,
	0.00f, 0.00f, 0.13f, 0.63f, 0.88f, 1.00f, 0.88f, 0.63f, 0.13f, 0.00f, 0.00f,
};

static void
texgz_tex_sampleOutline(texgz_tex_t* self, int i, int j,
                        float* mask, int size)
{
	ASSERT(self);
	ASSERT(mask);

	// sample state
	int           off = size/2;
	float         max = 0.0f;
	unsigned char val = 0;
	float         o;
	float         f;
	unsigned char v;

	// clip outline mask
	int m0 = 0;
	int m1 = size - 1;
	int n0 = 0;
	int n1 = size - 1;
	int b  = self->height - 1;
	int r  = self->width - 1;
	if(i - off < 0)
	{
		m0 = off - i;
	}
	if(j - off < 0)
	{
		n0 = off - j;
	}
	if(i + off > b)
	{
		m1 = m1 - ((i + off) - b);
	}
	if(j + off > r)
	{
		n1 = n1 - ((j + off) - r);
	}

	// determine the max sample
	int idx;
	int m;
	for(m = m0 + 1; m < m1; ++m)
	{
		// left edge
		idx = 2*((i + m - off)*self->stride + (j + n0 - off));
		o = mask[m*size + n0];
		v = self->pixels[idx];
		f = o*((float) v);
		if(f > max)
		{
			max = f;
			val = v;
		}

		// right edge
		idx = 2*((i + m - off)*self->stride + (j + n1 - off));
		o = mask[m*size + n1];
		v = self->pixels[idx];
		f = o*((float) v);
		if(f > max)
		{
			max = f;
			val = v;
		}
	}

	int n;
	for(n = n0; n <= n1; ++n)
	{
		// top edge
		idx = 2*((i + m0 - off)*self->stride + (j + n - off));
		o = mask[m0*size + n];
		v = self->pixels[idx];
		f = o*((float) v);
		if(f > max)
		{
			max = f;
			val = v;
		}

		// bottom edge
		idx = 2*((i + m1 - off)*self->stride + (j + n - off));
		o = mask[m1*size + n];
		v = self->pixels[idx];
		f = o*((float) v);
		if(f > max)
		{
			max = f;
			val = v;
		}
	}

	// store the outline sample
	idx = 2*(i*self->stride + j);
	self->pixels[idx + 1] = val;
}

/*
 * private - table conversion functions
 */

static void texgz_table_1to8(unsigned char* table)
{
	ASSERT(table);

	int i;
	for(i = 0; i < 2; ++i)
		table[i] = (unsigned char) (i*255.0f + 0.5f);
}

static void texgz_table_4to8(unsigned char* table)
{
	ASSERT(table);

	int i;
	for(i = 0; i < 16; ++i)
		table[i] = (unsigned char) (i*255.0f/15.0f + 0.5f);
}

static void texgz_table_5to8(unsigned char* table)
{
	ASSERT(table);

	int i;
	for(i = 0; i < 32; ++i)
		table[i] = (unsigned char) (i*255.0f/31.0f + 0.5f);
}

static void texgz_table_6to8(unsigned char* table)
{
	ASSERT(table);

	int i;
	for(i = 0; i < 64; ++i)
		table[i] = (unsigned char) (i*255.0f/63.0f + 0.5f);
}

/*
 * private - conversion functions
 */

static texgz_tex_t* texgz_tex_4444to8888(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_SHORT_4_4_4_4) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X", self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
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
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_SHORT_5_6_5) ||
	   (self->format != TEXGZ_RGB))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
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
			unsigned short* src;
			unsigned char*  dst;
			src = (unsigned short*) (&self->pixels[2*idx]);
			dst = &tex->pixels[4*idx];

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
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_SHORT_5_5_5_1) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
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
			unsigned short* src;
			unsigned char*  dst;
			src = (unsigned short*) (&self->pixels[2*idx]);
			dst = &tex->pixels[4*idx];

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
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGB))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
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

static texgz_tex_t* texgz_tex_Lto8888(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_LUMINANCE))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
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

static texgz_tex_t* texgz_tex_Ato8888(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_ALPHA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
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

			dst[0] = 0xFF;
			dst[1] = 0xFF;
			dst[2] = 0xFF;
			dst[3] = src[0];
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_LAto8888(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_LUMINANCE_ALPHA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
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
			unsigned char* src = &self->pixels[2*idx];
			unsigned char* dst = &tex->pixels[4*idx];

			dst[0] = src[0];
			dst[1] = src[0];
			dst[2] = src[0];
			dst[3] = src[1];
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_LAto8800(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_LUMINANCE_ALPHA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
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
			unsigned char* src = &self->pixels[2*idx];
			unsigned char* dst = &tex->pixels[4*idx];

			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = 0;
			dst[3] = 0;
		}
	}

	return tex;
}

static texgz_tex_t*
texgz_tex_Fto8888(texgz_tex_t* self, float min, float max)
{
	ASSERT(self);

	if((self->type != TEXGZ_FLOAT) ||
	   (self->format != TEXGZ_LUMINANCE))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA,
                        NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	int x, y, idx;
	float* fpixels = (float*) self->pixels;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			float*         src = &fpixels[idx];
			unsigned char* dst = &tex->pixels[4*idx];

			dst[0] = (unsigned char)
			         cc_clamp(255.0f*(src[0] - min)/(max - min),
			                  0.0f, 255.0f);
			dst[1] = dst[0],
			dst[2] = dst[0],
			dst[3] = 0xFF;
		}
	}

	return tex;
}

static texgz_tex_t*
texgz_tex_Fto8(texgz_tex_t* self, float min, float max)
{
	ASSERT(self);

	if((self->type != TEXGZ_FLOAT) ||
	   (self->format != TEXGZ_LUMINANCE))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        TEXGZ_UNSIGNED_BYTE, TEXGZ_LUMINANCE,
                        NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	int x, y, idx;
	float* fpixels = (float*) self->pixels;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			float*         src = &fpixels[idx];
			unsigned char* dst = &tex->pixels[idx];

			dst[0] = (unsigned char)
			         cc_clamp(255.0f*(src[0] - min)/(max - min),
			                  0.0f, 255.0f);
		}
	}

	return tex;
}

static texgz_tex_t*
texgz_tex_FFFFto8888(texgz_tex_t* self, float min, float max)
{
	ASSERT(self);

	if((self->type != TEXGZ_FLOAT) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA,
                        NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	int x, y, idx;
	float* fpixels = (float*) self->pixels;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = 4*(y*tex->stride + x);
			float*         src = &fpixels[idx];
			unsigned char* dst = &tex->pixels[idx];

			dst[0] = (unsigned char)
			         cc_clamp(255.0f*(src[0] - min)/(max - min),
			                  0.0f, 255.0f);
			dst[1] = (unsigned char)
			         cc_clamp(255.0f*(src[1] - min)/(max - min),
			                  0.0f, 255.0f);
			dst[2] = (unsigned char)
			         cc_clamp(255.0f*(src[2] - min)/(max - min),
			                  0.0f, 255.0f);
			dst[3] = (unsigned char)
			         cc_clamp(255.0f*(src[3] - min)/(max - min),
			                  0.0f, 255.0f);
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888to4444(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        TEXGZ_UNSIGNED_SHORT_4_4_4_4,
	                    TEXGZ_RGBA,
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
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        TEXGZ_UNSIGNED_SHORT_5_6_5,
	                    TEXGZ_RGB, NULL);
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
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        TEXGZ_UNSIGNED_SHORT_5_5_5_1,
	                    TEXGZ_RGBA, NULL);
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
			dst[0] = a | ((b << 1) & 0x3E) | ((g << 6) & 0xC0); // GBA
			dst[1] = (g >> 2) | ((r << 3) & 0xF8);              // RG
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888to888(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
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

static texgz_tex_t* texgz_tex_8888toL(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        TEXGZ_UNSIGNED_BYTE,
	                    TEXGZ_LUMINANCE, NULL);
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

			unsigned int luminance;
			luminance = ((unsigned int) src[0] +
			             (unsigned int) src[1] +
			             (unsigned int) src[2])/3;
			if(luminance > 0xFF)
			{
				luminance = 0xFF;
			}
			dst[0] = (unsigned char) luminance;
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888toLABL(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
	                    self->stride, self->vstride,
	                    TEXGZ_UNSIGNED_BYTE,
	                    TEXGZ_LABL, NULL);
	if(tex == NULL)
		return NULL;

	// See rgb2lab
	// https://github.com/antimatter15/rgb-lab/blob/master/color.js
	float r;
	float g;
	float b;
	float yy;
	float labl;
	int x, y, idx;
	for(y = 0; y < tex->height; ++y)
	{
		for(x = 0; x < tex->width; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[4*idx];
			unsigned char* dst = &tex->pixels[idx];

			r = ((float) src[0])/255.0f;
			g = ((float) src[1])/255.0f;
			b = ((float) src[2])/255.0f;

			r = (r > 0.04045f) ? powf((r + 0.055f)/1.055f, 2.4f) : r/12.92f;
			g = (g > 0.04045f) ? powf((g + 0.055f)/1.055f, 2.4f) : g/12.92f;
			b = (b > 0.04045f) ? powf((b + 0.055f)/1.055f, 2.4f) : b/12.92f;

			yy = (r*0.2126f + g*0.7152f + b*0.0722f)/1.00000f;
			yy = (yy > 0.008856f) ? powf(yy, 0.333333f) : (7.787f*yy) + 16.0f/116.0f;

			labl = (255.0f/100.0f)*(116.0f*yy - 16.0f);

			if(labl > 255.0f)
			{
				labl = 255.0f;
			}
			else if(labl < 0.0f)
			{
				labl = 0.0f;
			}
			dst[0] = (unsigned char) labl;
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888toA(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        TEXGZ_UNSIGNED_BYTE, TEXGZ_ALPHA,
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

			dst[0] = src[3];
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888toLA(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
	                    self->stride, self->vstride,
	                    TEXGZ_UNSIGNED_BYTE,
	                    TEXGZ_LUMINANCE_ALPHA, NULL);
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

			unsigned int luminance = (src[0] + src[1] + src[2])/3;
			dst[0] = (unsigned char) luminance;
			dst[1] = src[3];
		}
	}

	return tex;
}

static texgz_tex_t* texgz_tex_8888toF(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        TEXGZ_FLOAT, TEXGZ_LUMINANCE,
                        NULL);
	if(tex == NULL)
		return NULL;

	int x, y, idx;
	float* fpixels = (float*) tex->pixels;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[4*idx];
			float*         dst = &fpixels[idx];

			// use the red channel for luminance
			dst[0] = ((float) src[0])/255.0f;
		}
	}

	return tex;
}

static texgz_tex_t*
texgz_tex_8888toFFFF(texgz_tex_t* self,
                     float min, float max)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        TEXGZ_FLOAT, TEXGZ_RGBA,
                        NULL);
	if(tex == NULL)
		return NULL;

	int x, y, idx;
	float* fpixels = (float*) tex->pixels;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = 4*(y*tex->stride + x);
			unsigned char* src = &self->pixels[idx];
			float*         dst = &fpixels[idx];

			dst[0] = (max - min)*((float) src[0])/255.0f - min;
			dst[1] = (max - min)*((float) src[1])/255.0f - min;
			dst[2] = (max - min)*((float) src[2])/255.0f - min;
			dst[3] = (max - min)*((float) src[3])/255.0f - min;
		}
	}

	return tex;
}

static texgz_tex_t*
texgz_tex_8toF(texgz_tex_t* self,
               float min, float max)
{
	ASSERT(self);

	if((self->type != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_LUMINANCE))
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        TEXGZ_FLOAT, TEXGZ_LUMINANCE,
                        NULL);
	if(tex == NULL)
		return NULL;

	int x, y, idx;
	float* fpixels = (float*) tex->pixels;
	for(y = 0; y < tex->vstride; ++y)
	{
		for(x = 0; x < tex->stride; ++x)
		{
			idx = y*tex->stride + x;
			unsigned char* src = &self->pixels[idx];
			float*         dst = &fpixels[idx];

			dst[0] = (max - min)*((float) src[0])/255.0f - min;
		}
	}

	return tex;
}

static texgz_tex_t*
texgz_tex_8888format(texgz_tex_t* self, int format)
{
	ASSERT(self);

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
	else if((self->format == TEXGZ_BGRA) &&
	        (format == TEXGZ_RGBA))
	{
		r = 2;
		g = 1;
		b = 0;
		a = 3;
	}
	else
	{
		LOGE("invalid type=0x%X, format=0x%X, dst_format=0x%X",
		     self->type, self->format, format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
                        self->stride, self->vstride,
                        self->type, format, NULL);
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

#define TEXGZ_TEX_HSIZE 28

static int
texgz_readint(const unsigned char* buffer, int offset)
{
	ASSERT(buffer);
	ASSERT(offset >= 0);

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

static int
texgz_parseh(const unsigned char* buffer,
             int* type, int* format,
             int* width, int* height,
             int* stride, int* vstride)
{
	ASSERT(buffer);
	ASSERT(type);
	ASSERT(format);
	ASSERT(width);
	ASSERT(height);
	ASSERT(stride);
	ASSERT(vstride);

	int magic = texgz_readint(buffer, 0);
	if(magic == 0x000B00D9)
	{
		*type    = texgz_readint(buffer, 4);
		*format  = texgz_readint(buffer, 8);
		*width   = texgz_readint(buffer, 12);
		*height  = texgz_readint(buffer, 16);
		*stride  = texgz_readint(buffer, 20);
		*vstride = texgz_readint(buffer, 24);
	}
	else if(texgz_swapendian(magic) == 0x000B00D9)
	{
		*type    = texgz_swapendian(texgz_readint(buffer, 4));
		*format  = texgz_swapendian(texgz_readint(buffer, 8));
		*width   = texgz_swapendian(texgz_readint(buffer, 12));
		*height  = texgz_swapendian(texgz_readint(buffer, 16));
		*stride  = texgz_swapendian(texgz_readint(buffer, 20));
		*vstride = texgz_swapendian(texgz_readint(buffer, 24));
	}
	else
	{
		LOGE("bad magic=0x%.8X", magic);
		return 0;
	}

	return 1;
}

static int texgz_nextpot(int x)
{
	int xp = 1;
	while(x > xp)
	{
		xp *= 2;
	}
	return xp;
}

static float fx(float a, float b, float c, float y)
{
	return -(b/a)*y - c/a;
}

static float fy(float a, float b, float c, float x)
{
	return -(a/b)*x - c/b;
}

static float max3(float i, float j, float k)
{
	if(i > j)
	{
		return (i > k) ? i : k;
	}
	else
	{
		return (j > k) ? j : k;
	}
}

static float min3(float i, float j, float k)
{
	if(i < j)
	{
		return (i < k) ? i : k;
	}
	else
	{
		return (j < k) ? j : k;
	}
}

static int
texgz_tex_lineClip(texgz_tex_t* self,
                   int* _direction,
                   float* _x0, float* _y0,
                   float* _x1, float* _y1)
{
	ASSERT(self);
	ASSERT(_direction);
	ASSERT(_x0);
	ASSERT(_y0);
	ASSERT(_x1);
	ASSERT(_y1);

	float x0 = *_x0;
	float y0 = *_y0;
	float x1 = *_x1;
	float y1 = *_y1;

	float T = 0.0f;
	float L = 0.0f;
	float B = (float) (self->height - 1);
	float R = (float) (self->width  - 1);

	// swap points so we always clip left to right
	if((x1 - x0) < 0.0f)
	{
		float x_temp = x1;
		float y_temp = y1;
		x1          = x0;
		y1          = y0;
		x0          = x_temp;
		y0          = y_temp;
		*_direction = -1;
	}
	else
	{
		*_direction = 1;
	}

	// compute coefficients for ax + by + c = 0
	float a = y1 - y0;
	float b = x0 - x1;
	float c = x0*(y0 - y1) + y0*(x1 - x0);

	// case 1: not a line
	if((a == 0.0f) && (b == 0.0f))
	{
		return 1;
	}

	// case 2: line not visible
	else if(x1 < L || x0 > R)
	{
		// horizontal axis
		return 1;
	}
	else if((a >= 0.0f) && (y0 > B || y1 < T))
	{
		// increasing or horizontal lines
		return 1;
	}
	else if((a < 0.0f) && (y0 < T || y1 > B))
	{
		// decreasing lines
		return 1;
	}

	// case 3: horizontal line (i.e. left to right)
	else if(a == 0.0f)
	{
		// clip the line
		if(x0 < L)
		{
			x0 = L;
		}

		if(x1 > R)
		{
			x1 = R;
		}
	}

	// case 4: increasing vertical line (i.e. top to bottom)
	else if((a > 0.0f) && (b == 0.0f))
	{
		// clip the line
		if(y0 < T)
		{
			y0 = T;
		}

		if(y1 > B)
		{
			y1 = B;
		}
	}

	// case 5: decreasing vertical line (i.e. bottom to top)
	else if((a < 0.0f) && (b == 0.0f))
	{
		// clip the line
		if(y0 > B)
		{
			y0 = B;
		}

		if(y1 < T)
		{
			y1 = T;
		}
	}

	// case 6: increasing line (i.e. top-left to bottom-right)
	else if(a > 0.0f)
	{
		// compute the left point
		x0 = max3(x0, L, fx(a, b, c, T));
		y0 = max3(y0, T, fy(a, b, c, L));

		// compute the right point
		x1 = min3(x1, R, fx(a, b, c, B));
		y1 = min3(y1, B, fy(a, b, c, R));
	}

	// case 7: decreasing line (i.e. bottom-left to top-right)
	else if(a < 0.0f)
	{
		// compute the left point
		x0 = max3(x0, L, fx(a, b, c, B));
		y0 = min3(y0, B, fy(a, b, c, L));

		// compute the right point
		x1 = min3(x1, R, fx(a, b, c, T));
		y1 = max3(y1, T, fy(a, b, c, R));
	}

	// in case 6 and case 7 the line may not actually cross the BB
	if((x0 < L) || (x0 > R))
	{
		return 1;
	}

	if((x1 < L) || (x1 > R))
	{
		return 1;
	}

	if((y0 < T) || (y0 > B))
	{
		return 1;
	}

	if((y1 < T) || (y1 > B))
	{
		return 1;
	}

	*_x0 = x0;
	*_y0 = y0;
	*_x1 = x1;
	*_y1 = y1;
	return 0;
}

static void
texgz_tex_lineDrawClipped(texgz_tex_t* self,
                          float x0, float y0,
                          float x1, float y1,
                          unsigned char* pixel)
{
	ASSERT(self);
	ASSERT(pixel);
	ASSERT(x0 <= x1);

	int d;
	int incr1, incr2;
	int px1_incr, py1_incr, px2_incr, py2_incr;
	int* curpos;
	int* finalpos;

	// convert line endpoints to pixel coordinates
	int px0 = (int) x0;
	int py0 = (int) y0;
	int px1 = (int) x1;
	int py1 = (int) y1;

	// initialize slope and start point
	int dx = px1 - px0;
	int dy = py1 - py0;
	int px = px0;
	int py = py0;

	// avoid 4 cases
	if((dx == 0) ||
	   (((float) dy / (float) dx) > 1.0f) ||
	   (((float) dy / (float) dx) < -1.0f))
	{
		curpos = &py;
		finalpos = &py1;
		if(dy > 0)
		{
			// slope m > 1
			d = dy - 2 * dx;
			incr2 = 2 * -dx;         // north
			incr1 = 2 * (dy - dx);   // north east
			px2_incr = 0;            // reverse px and py
			py2_incr = 1;
			px1_incr = 1;
			py1_incr = 1;
		}
		else
		{
			// slope m < -1
			d = dy + 2 * dx;
			incr1 = 2 * dx;          // south
			incr2 = 2 * (dy + dx);   // south east
			px1_incr = 0;            // reverse px and py, switch sign of py
			py1_incr = -1;
			px2_incr = 1;
			py2_incr = -1;
		}
	}
	else if((((float) dy / (float) dx) <= 1.0f) &&
	        (((float) dy / (float) dx) >= 0.0f))
	{
		// slope 0 < m < 1
		curpos = &px;
		finalpos = &px1;
		d = 2 * dy - dx;
		incr1 = 2 * dy;          // east
		incr2 = 2 * (dy - dx);   // north east
		px1_incr = 1;
		py1_incr = 0;
		px2_incr = 1;
		py2_incr = 1;
	}
	else if((((float) dy / (float) dx) >= -1.0f) &&
	        (((float) dy / (float) dx) <= 0.0f))
	{
		// slope 0 > m > -1
		curpos = &px;
		finalpos = &px1;
		d = 2 * dy + dx;
		incr2 = 2 * dy;         // east
		incr1 = 2 * (dy + dx);	// south east
		px2_incr = 1;
		py2_incr = 0;
		px1_incr = 1;
		py1_incr = -1;	        // switch sign of py
	}
	else
	{
		// unhandled case ...
		return;
	}

	// fill first pixel
	texgz_tex_setPixel(self, px, py, pixel);
	while((*finalpos) - (*curpos))
	{
		if(d <= 0)
		{
			d  += incr1;
			px += px1_incr;
			py += py1_incr;
		}
		else
		{
			d  += incr2;
			px += px2_incr;
			py += py2_incr;
		}
		texgz_tex_setPixel(self, px, py, pixel);
	}
}

static void
texgz_tex_lineDrawClippedF(texgz_tex_t* self,
                           float x0, float y0,
                           float x1, float y1,
                           float pixel)
{
	ASSERT(self);
	ASSERT(x0 <= x1);

	int d;
	int incr1, incr2;
	int px1_incr, py1_incr, px2_incr, py2_incr;
	int* curpos;
	int* finalpos;

	// convert line endpoints to pixel coordinates
	int px0 = (int) x0;
	int py0 = (int) y0;
	int px1 = (int) x1;
	int py1 = (int) y1;

	// initialize slope and start point
	int dx = px1 - px0;
	int dy = py1 - py0;
	int px = px0;
	int py = py0;

	// avoid 4 cases
	if((dx == 0) ||
	   (((float) dy / (float) dx) > 1.0f) ||
	   (((float) dy / (float) dx) < -1.0f))
	{
		curpos = &py;
		finalpos = &py1;
		if(dy > 0)
		{
			// slope m > 1
			d = dy - 2 * dx;
			incr2 = 2 * -dx;         // north
			incr1 = 2 * (dy - dx);   // north east
			px2_incr = 0;            // reverse px and py
			py2_incr = 1;
			px1_incr = 1;
			py1_incr = 1;
		}
		else
		{
			// slope m < -1
			d = dy + 2 * dx;
			incr1 = 2 * dx;          // south
			incr2 = 2 * (dy + dx);   // south east
			px1_incr = 0;            // reverse px and py, switch sign of py
			py1_incr = -1;
			px2_incr = 1;
			py2_incr = -1;
		}
	}
	else if((((float) dy / (float) dx) <= 1.0f) &&
	        (((float) dy / (float) dx) >= 0.0f))
	{
		// slope 0 < m < 1
		curpos = &px;
		finalpos = &px1;
		d = 2 * dy - dx;
		incr1 = 2 * dy;          // east
		incr2 = 2 * (dy - dx);   // north east
		px1_incr = 1;
		py1_incr = 0;
		px2_incr = 1;
		py2_incr = 1;
	}
	else if((((float) dy / (float) dx) >= -1.0f) &&
	        (((float) dy / (float) dx) <= 0.0f))
	{
		// slope 0 > m > -1
		curpos = &px;
		finalpos = &px1;
		d = 2 * dy + dx;
		incr2 = 2 * dy;         // east
		incr1 = 2 * (dy + dx);	// south east
		px2_incr = 1;
		py2_incr = 0;
		px1_incr = 1;
		py1_incr = -1;	        // switch sign of py
	}
	else
	{
		// unhandled case ...
		return;
	}

	// fill first pixel
	texgz_tex_setPixelF(self, px, py, &pixel);
	while((*finalpos) - (*curpos))
	{
		if(d <= 0)
		{
			d  += incr1;
			px += px1_incr;
			py += py1_incr;
		}
		else
		{
			d  += incr2;
			px += px2_incr;
			py += py2_incr;
		}
		texgz_tex_setPixelF(self, px, py, &pixel);
	}
}

static void
texgz_tex_samplePixelF(texgz_tex_t* self, int id,
                       int x, int y,
                       int direction, int* _offset,
                       int* _count, int* _size,
                       texgz_sampleF_t** _samples)
{
	ASSERT(self);
	ASSERT(_offset);
	ASSERT(_count);
	ASSERT(_size);
	ASSERT(_samples);
	ASSERT(self->type   == TEXGZ_FLOAT);
	ASSERT(self->format == TEXGZ_LUMINANCE);

	texgz_sampleF_t* samples = *_samples;

	// resize samples
	int offset = *_offset;
	int count  = *_count;
	int size   = *_size;
	while(offset >= size)
	{
		if(size == 0)
		{
			size = 32;
		}
		else
		{
			size *= 2;
		}

		samples = (texgz_sampleF_t*)
		          REALLOC((void*) samples,
		                  size*sizeof(texgz_sampleF_t));
		if(samples == NULL)
		{
			LOGE("REALLOC failed");
			return;
		}

		*_samples = samples;
		*_size    = size;
	}

	texgz_sampleF_t* sample = &samples[offset];
	float*           pixels = (float*) self->pixels;
	int              idx    = y*self->stride + x;

	// assign sample
	sample->id    = id;
	sample->x     = (float) x;
	sample->y     = (float) y;
	sample->pixel = pixels[idx];
	*_offset      = offset + direction;
	*_count       = count + 1;
}

static void
texgz_tex_lineSampleClippedF(texgz_tex_t* self, int id,
                             float x0, float y0,
                             float x1, float y1,
                             int direction,
                             int* _count,
                             int* _size,
                             texgz_sampleF_t** _samples)
{
	ASSERT(self);
	ASSERT(_count);
	ASSERT(_size);
	ASSERT(_samples);
	ASSERT(x0 <= x1);

	int d;
	int incr1, incr2;
	int px1_incr, py1_incr, px2_incr, py2_incr;
	int* curpos;
	int* finalpos;

	// convert line endpoints to pixel coordinates
	int px0 = (int) x0;
	int py0 = (int) y0;
	int px1 = (int) x1;
	int py1 = (int) y1;

	// initialize slope and start point
	int dx = px1 - px0;
	int dy = py1 - py0;
	int px = px0;
	int py = py0;

	// avoid 4 cases
	if((dx == 0) ||
	   (((float) dy / (float) dx) > 1.0f) ||
	   (((float) dy / (float) dx) < -1.0f))
	{
		curpos = &py;
		finalpos = &py1;
		if(dy > 0)
		{
			// slope m > 1
			d = dy - 2 * dx;
			incr2 = 2 * -dx;         // north
			incr1 = 2 * (dy - dx);   // north east
			px2_incr = 0;            // reverse px and py
			py2_incr = 1;
			px1_incr = 1;
			py1_incr = 1;
		}
		else
		{
			// slope m < -1
			d = dy + 2 * dx;
			incr1 = 2 * dx;          // south
			incr2 = 2 * (dy + dx);   // south east
			px1_incr = 0;            // reverse px and py, switch sign of py
			py1_incr = -1;
			px2_incr = 1;
			py2_incr = -1;
		}
	}
	else if((((float) dy / (float) dx) <= 1.0f) &&
	        (((float) dy / (float) dx) >= 0.0f))
	{
		// slope 0 < m < 1
		curpos = &px;
		finalpos = &px1;
		d = 2 * dy - dx;
		incr1 = 2 * dy;          // east
		incr2 = 2 * (dy - dx);   // north east
		px1_incr = 1;
		py1_incr = 0;
		px2_incr = 1;
		py2_incr = 1;
	}
	else if((((float) dy / (float) dx) >= -1.0f) &&
	        (((float) dy / (float) dx) <= 0.0f))
	{
		// slope 0 > m > -1
		curpos = &px;
		finalpos = &px1;
		d = 2 * dy + dx;
		incr2 = 2 * dy;         // east
		incr1 = 2 * (dy + dx);	// south east
		px2_incr = 1;
		py2_incr = 0;
		px1_incr = 1;
		py1_incr = -1;	        // switch sign of py
	}
	else
	{
		// unhandled case ...
		return;
	}

	// compute offset to restore pixel order
	int offset = *_count;
	if(direction < 0)
	{
		offset = *_count + abs((*finalpos) - (*curpos));
	}

	// fill first pixel
	texgz_tex_samplePixelF(self, id, px, py,
	                       direction, &offset,
	                       _count, _size,
	                       _samples);
	while((*finalpos) - (*curpos))
	{
		if(d <= 0)
		{
			d  += incr1;
			px += px1_incr;
			py += py1_incr;
		}
		else
		{
			d  += incr2;
			px += px2_incr;
			py += py2_incr;
		}
		texgz_tex_samplePixelF(self, id, px, py,
		                       direction, &offset,
		                       _count, _size,
		                       _samples);
	}
}

// https://en.wikipedia.org/wiki/Normal_distribution
static float
texgz_tex_gaussianPdf(float sigma, float mu, float x)
{
	float a = 1.0f/(sigma*sqrtf(2.0f*M_PI));
	float b = (x - mu)/sigma;
	float c = -0.5f*b*b;
	return a*expf(c);
}

static int
texgz_tex_gaussianCoef(float sigma, float mu,
                       int size, float* coefficients)
{
	ASSERT(coefficients);

	if(sigma < 0.0f)
	{
		LOGE("invalid sigma=%f", sigma);
		return 0;
	}

	if((size < 0) || (size%2 == 0))
	{
		LOGE("invalid size=%i", size);
		return 0;
	}

	float sum = 0.0f;
	int   w   = size/2;

	// compute coefficients
	int i;
	for(i = 0; i < size; ++i)
	{
		float x          = (float) (i - w);
		coefficients[i]  = texgz_tex_gaussianPdf(sigma, mu, x);
		sum             += coefficients[i];

	}

	// scale coefficients so sum is 1.0f
	for(i = 0; i < size; ++i)
	{
		coefficients[i] = coefficients[i]/sum;
	}

	return 1;
}

/*
 * public
 */

texgz_tex_t*
texgz_tex_new(int width, int height,
              int stride, int vstride,
              int type, int format,
              unsigned char* pixels)
{
	// pixels can be NULL

	if((stride <= 0) || (width > stride))
	{
		LOGE("invalid width=%i, stride=%i",
		     width, stride);
		return NULL;
	}

	if((vstride <= 0) || (height > vstride))
	{
		LOGE("invalid height=%i, vstride=%i",
		     height, vstride);
		return NULL;
	}

	if((type == TEXGZ_UNSIGNED_SHORT_4_4_4_4) &&
	   (format == TEXGZ_RGBA))
		; // ok
	else if((type == TEXGZ_UNSIGNED_SHORT_5_5_5_1) &&
	        (format == TEXGZ_RGBA))
		; // ok
	else if((type == TEXGZ_UNSIGNED_SHORT_5_6_5) &&
	        (format == TEXGZ_RGB))
		; // ok
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_RGBA))
		; // ok
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_RGB))
		; // ok
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_LUMINANCE))
		; // ok
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_LABL))
		; // ok
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_ALPHA))
		; // ok
	else if((type == TEXGZ_SHORT) &&
	        (format == TEXGZ_LUMINANCE))
		; // ok
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_LUMINANCE_ALPHA))
		; // ok
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_BGRA))
		; // ok
	else if((type == TEXGZ_FLOAT) &&
	        (format == TEXGZ_LUMINANCE))
		; // ok
	else if((type == TEXGZ_FLOAT) &&
	        (format == TEXGZ_RGBA))
		; // ok
	else
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     type, format);
		return NULL;
	}

	texgz_tex_t* self;
	self = (texgz_tex_t*) MALLOC(sizeof(texgz_tex_t));
	if(self == NULL)
	{
		LOGE("MALLOC failed");
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

	self->pixels = (unsigned char*) MALLOC((size_t) size);
	if(self->pixels == NULL)
	{
		LOGE("MALLOC failed");
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
		FREE(self);
	return NULL;
}

void texgz_tex_delete(texgz_tex_t** _self)
{
	ASSERT(_self);

	texgz_tex_t* self = *_self;
	if(self)
	{
		FREE(self->pixels);
		FREE(self);
		*_self = NULL;
	}
}

texgz_tex_t* texgz_tex_copy(texgz_tex_t* self)
{
	ASSERT(self);

	return texgz_tex_new(self->width, self->height,
                         self->stride, self->vstride,
                         self->type, self->format,
                         self->pixels);
}

texgz_tex_t* texgz_tex_downscale(texgz_tex_t* self)
{
	ASSERT(self);

	// only support even/one w/h textures
	int w = self->width;
	int h = self->height;
	if(((w == 1) || (w%2 == 0)) &&
	   ((h == 1) || (h%2 == 0)))
	{
		// continue
	}
	else
	{
		LOGE("invalid w=%i, h=%i", w, h);
		return NULL;
	}

	// handle 1x1 special case
	if((w == 1) && (h == 1))
	{
		return texgz_tex_copy(self);
	}

	// convert to unsigned bytes
	int type = self->type;
	texgz_tex_t* src;
	if(type == TEXGZ_UNSIGNED_BYTE)
	{
		src = self;
	}
	else if((type == TEXGZ_UNSIGNED_SHORT_4_4_4_4) ||
	        (type == TEXGZ_UNSIGNED_SHORT_5_5_5_1))
	{
		src = texgz_tex_convertcopy(self,
		                            TEXGZ_UNSIGNED_BYTE,
		                            TEXGZ_RGBA);
	}
	else if(type == TEXGZ_UNSIGNED_SHORT_5_6_5)
	{
		src = texgz_tex_convertcopy(self,
		                            TEXGZ_UNSIGNED_BYTE,
		                            TEXGZ_RGB);
	}
	else
	{
		LOGE("invalid type=0x%X", type);
		return NULL;
	}

	// check the conversion (if needed)
	if(src == NULL)
	{
		return NULL;
	}

	// create downscale texture
	w = (w == 1) ? 1 : w/2;
	h = (h == 1) ? 1 : h/2;
	texgz_tex_t* down;
	down = texgz_tex_new(w, h, w, h, src->type, src->format,
	                     NULL);
	if(down == NULL)
	{
		goto fail_new;
	}

	// downscale with box filter
	int   i;
	int   x;
	int   y;
	float p00;
	float p01;
	float p10;
	float p11;
	float avg;
	int   bpp        = texgz_tex_bpp(src);
	int   bpp2       = 2*bpp;
	int   src_step   = bpp*src->stride;
	int   dst_step   = bpp*down->stride;
	int   src_offset00;
	int   src_offset01;
	int   src_offset10;
	int   src_offset11;
	int   dst_offset;
	unsigned char* src_pixels = src->pixels;
	unsigned char* dst_pixels = down->pixels;
	if(src->width == 1)
	{
		for(y = 0; y < src->height; y += 2)
		{
			src_offset00 = y*src_step;
			src_offset10 = src_offset00 + src_step;
			dst_offset   = (y/2)*dst_step;
			for(i = 0; i < bpp; ++i)
			{
				p00 = (float) src_pixels[src_offset00 + i];
				p10 = (float) src_pixels[src_offset10 + i];
				avg = (p00 + p10)/2.0f;
				dst_pixels[dst_offset + i] = (unsigned char) avg;
			}
		}
	}
	else if(src->height == 1)
	{
		src_offset00 = 0;
		src_offset01 = bpp;
		dst_offset   = 0;
		for(x = 0; x < src->width; x += 2)
		{
			for(i = 0; i < bpp; ++i)
			{
				p00 = (float) src_pixels[src_offset00 + i];
				p01 = (float) src_pixels[src_offset01 + i];
				avg = (p00 + p01)/2.0f;
				dst_pixels[dst_offset + i] = (unsigned char) avg;
			}
			src_offset00 += bpp2;
			src_offset01 += bpp2;
			dst_offset   += bpp;
		}
	}
	else
	{
		for(y = 0; y < src->height; y += 2)
		{
			src_offset00 = y*src_step;
			src_offset01 = src_offset00 + bpp;
			src_offset10 = src_offset00 + src_step;
			src_offset11 = src_offset10 + bpp;
			dst_offset   = (y/2)*dst_step;
			for(x = 0; x < src->width; x += 2)
			{
				for(i = 0; i < bpp; ++i)
				{
					p00 = (float) src_pixels[src_offset00 + i];
					p01 = (float) src_pixels[src_offset01 + i];
					p10 = (float) src_pixels[src_offset10 + i];
					p11 = (float) src_pixels[src_offset11 + i];
					avg = (p00 + p01 + p10 + p11)/4.0f;
					dst_pixels[dst_offset + i] = (unsigned char) avg;
				}
				src_offset00 += bpp2;
				src_offset01 += bpp2;
				src_offset10 += bpp2;
				src_offset11 += bpp2;
				dst_offset   += bpp;
			}
		}
	}

	// convert to input type
	if(type == TEXGZ_UNSIGNED_SHORT_4_4_4_4)
	{
		if(texgz_tex_convert(down,
		                     TEXGZ_UNSIGNED_SHORT_4_4_4_4,
		                     TEXGZ_RGBA) == 0)
		{
			goto fail_convert;
		}
	}
	else if(type == TEXGZ_UNSIGNED_SHORT_5_5_5_1)
	{
		if(texgz_tex_convert(down,
		                     TEXGZ_UNSIGNED_SHORT_5_5_5_1,
		                     TEXGZ_RGBA) == 0)
		{
			goto fail_convert;
		}
	}
	else if(type == TEXGZ_UNSIGNED_SHORT_5_6_5)
	{
		if(texgz_tex_convert(down,
		                     TEXGZ_UNSIGNED_SHORT_5_6_5,
		                     TEXGZ_RGB) == 0)
		{
			goto fail_convert;
		}
	}

	// delete the temporary texture
	if(type != TEXGZ_UNSIGNED_BYTE)
	{
		texgz_tex_delete(&src);
	}

	// success
	return down;

	// failure
	fail_convert:
		texgz_tex_delete(&down);
	fail_new:
		if(type != TEXGZ_UNSIGNED_BYTE)
		{
			texgz_tex_delete(&src);
		}
	return NULL;
}

texgz_tex_t*
texgz_tex_lanczos3(texgz_tex_t* self, int level)
{
	ASSERT(self);

	int src_width  = self->width;
	int src_height = self->height;
	int dst_width  = src_width/cc_pow2n(level);
	int dst_height = src_height/cc_pow2n(level);

	if((src_width%dst_width) || (src_height%dst_height))
	{
		LOGE("invalid width=%i:%i, height=%i:%i",
		     src_width,  dst_width,
		     src_height, dst_height);
		return NULL;
	}

	texgz_tex_t* src;
	src = texgz_tex_convertFcopy(self, 0.0f, 1.0f,
	                             TEXGZ_FLOAT, TEXGZ_RGBA);
	if(src == NULL)
	{
		return NULL;
	}

	texgz_tex_t* conv;
	conv = texgz_tex_new(dst_width, src_height,
	                     dst_width, src_height,
	                     TEXGZ_FLOAT, TEXGZ_RGBA,
	                     NULL);
	if(conv == NULL)
	{
		goto fail_conv;
	}

	texgz_tex_t* dst;
	dst = texgz_tex_new(dst_width, dst_height,
	                    dst_width, dst_height,
	                    TEXGZ_FLOAT, TEXGZ_RGBA,
	                    NULL);
	if(dst == NULL)
	{
		goto fail_dst;
	}

	// determine filter size
	int   scale   = cc_pow2n(level);
	float support = 3.0f;
	float scalef  = (float) scale;
	int   n       = (int) (scalef*support + 0.01f);
	int   size    = 2*n;
	if(size >= TEXGZ_LANCZOS3_MAXSIZE)
	{
		LOGE("invalid level=%i", level);
		goto fail_size;
	}

	// generate masks
	// for example
	// 1: 0.007,  0.030,
	//   -0.068, -0.133,
	//    0.270,  0.890,
	// 2: 0.002,  0.016,  0.030,  0.020,
	//   -0.031, -0.105, -0.147, -0.085,
	//    0.121,  0.437,  0.764,  0.971,
	float step = 1.0f/scalef;
	float x    = support - step/2.0f;
	float y;
	int   i;
	float mask[TEXGZ_LANCZOS3_MAXSIZE];
	for(i = 0; i < n; ++i)
	{
		y = pil_lanczos3_filter(x)/scale;
		mask[i]            = y;
		mask[size - i - 1] = y;
		x -= step;
	}

	// apply filter and decimate
	texgz_tex_convolveF(src,  conv,
	                    size, 1, scale, 1, mask);
	texgz_tex_convolveF(conv, dst,
	                    1, size, 1, scale, mask);

	if(texgz_tex_convertF(dst, 0.0f, 1.0f,
	                      TEXGZ_UNSIGNED_BYTE,
	                      TEXGZ_RGBA) == 0)
	{
		goto fail_convert;
	}

	texgz_tex_delete(&conv);
	texgz_tex_delete(&src);

	// success
	return dst;

	// failure
	fail_convert:
	fail_size:
		texgz_tex_delete(&dst);
	fail_dst:
		texgz_tex_delete(&conv);
	fail_conv:
		texgz_tex_delete(&src);
	return NULL;
}

texgz_tex_t* texgz_tex_resize(texgz_tex_t* self,
                              int width,
                              int height)
{
	ASSERT(self);
	ASSERT(self->type == TEXGZ_UNSIGNED_BYTE);
	ASSERT((self->format == TEXGZ_RGB) ||
	       (self->format == TEXGZ_RGBA));

	texgz_tex_t* copy;
	copy = texgz_tex_new(width, height,
	                     width, height,
	                     self->type, self->format,
	                     NULL);
	if(copy == NULL)
	{
		return NULL;
	}

	int bpp = texgz_tex_bpp(self);

	int   i;
	int   j;
	float u;
	float v;
	float w = (float) width + 1;
	float h = (float) height + 1;
	unsigned char pixel[4];
	for(i = 0; i < height; ++i)
	{
		for(j = 0; j < width; ++j)
		{
			u = (j + 1)/w;
			v = (i + 1)/h;
			texgz_tex_sample(self, u, v, bpp, pixel);
			texgz_tex_setPixel(copy, j, i, pixel);
		}
	}

	return copy;
}

texgz_tex_t* texgz_tex_import(const char* filename)
{
	ASSERT(filename);

	// open texture
	gzFile f = gzopen(filename, "rb");
	if(!f)
	{
		LOGE("gzopen failed filename=%s", filename);
		return NULL;
	}

	// read header
	unsigned char buffer[4096];   // 4KB
	int bytes_read = gzread(f, buffer, TEXGZ_TEX_HSIZE);
	if(bytes_read != TEXGZ_TEX_HSIZE)
	{
		LOGE("gzread failed to read header");
		goto fail_header;
	}

	int type;
	int format;
	int width;
	int height;
	int stride;
	int vstride;
	if(texgz_parseh(buffer, &type, &format,
                    &width, &height, &stride, &vstride) == 0)
	{
		goto fail_parseh;
	}

	texgz_tex_t* self;
	self = texgz_tex_new(width, height, stride, vstride,
	                     type, format, NULL);
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
	fail_parseh:
	fail_header:
		gzclose(f);
	return NULL;
}

texgz_tex_t* texgz_tex_importz(const char* filename)
{
	ASSERT(filename);

	FILE* f = fopen(filename, "r");
	if(f == NULL)
	{
		LOGE("invalid filename=%s", filename);
		return NULL;
	}

	// determine the file size
	fseek(f, (long) 0, SEEK_END);
	int fsize = (int) ftell(f);
	rewind(f);

	texgz_tex_t* tex = texgz_tex_importf(f, fsize);
	fclose(f);
	return tex;
}

texgz_tex_t* texgz_tex_importf(FILE* f, int size)
{
	ASSERT(f);
	ASSERT(size > 0);

	// allocate src buffer
	char* src = (char*) MALLOC(size*sizeof(char));
	if(src == NULL)
	{
		LOGE("MALLOC failed");
		return NULL;
	}

	// read buffer
	long start = ftell(f);
	if(fread((void*) src, sizeof(char), size, f) != size)
	{
		LOGE("fread failed");
		goto fail_read;
	}

	texgz_tex_t* self;
	self = texgz_tex_importd((size_t) size, (const void*) src);
	if(self == NULL)
	{
		goto fail_tex;
	}

	FREE(src);

	// success
	return self;

	// failure
	fail_tex:
		FREE(src);
	fail_read:
		fseek(f, start, SEEK_SET);
		FREE(src);
	return NULL;
}

texgz_tex_t*
texgz_tex_importd(size_t size, const void* data)
{
	ASSERT(size > 0);
	ASSERT(data);

	// uncompress the header
	unsigned char header[TEXGZ_TEX_HSIZE];
	uLong         hsize    = TEXGZ_TEX_HSIZE;
	uLong         src_size = (uLong) size;
	uncompress((Bytef*) header, &hsize, (const Bytef*) data,
	           src_size);
	if(hsize != TEXGZ_TEX_HSIZE)
	{
		LOGE("uncompress failed hsize=%i", (int) hsize);
		return NULL;
	}

	int type;
	int format;
	int width;
	int height;
	int stride;
	int vstride;
	if(texgz_parseh(header, &type, &format,
                    &width, &height, &stride,
	                &vstride) == 0)
	{
		return NULL;
	}

	// create tex
	texgz_tex_t* self;
	self = texgz_tex_new(width, height, stride, vstride,
	                     type, format, NULL);
	if(self == NULL)
	{
		return NULL;
	}

	// allocate dst buffer
	int   bytes    = texgz_tex_size(self);
	uLong dst_size =  TEXGZ_TEX_HSIZE + bytes;
	unsigned char* dst;
	dst = (unsigned char*)
	      MALLOC(dst_size*sizeof(unsigned char));
	if(dst == NULL)
	{
		LOGE("MALLOC failed");
		goto fail_dst;
	}

	if(uncompress((Bytef*) dst, &dst_size,
	              (const Bytef*) data, src_size) != Z_OK)
	{
		LOGE("fail uncompress");
		goto fail_uncompress;
	}

	if(dst_size != (TEXGZ_TEX_HSIZE + bytes))
	{
		LOGE("invalid dst_size=%i, expected=%i",
		     (int) dst_size, (int) (TEXGZ_TEX_HSIZE + bytes));
		goto fail_dst_size;
	}

	// copy buffer into tex
	memcpy(self->pixels, &dst[TEXGZ_TEX_HSIZE], bytes);

	FREE(dst);

	// success
	return self;

	// failure
	fail_dst_size:
	fail_uncompress:
		FREE(dst);
	fail_dst:
		texgz_tex_delete(&self);
	return NULL;
}

int texgz_tex_export(texgz_tex_t* self,
                     const char* filename)
{
	ASSERT(self);
	ASSERT(filename);

	gzFile f = gzopen(filename, "wb");
	if(f == NULL)
	{
		LOGE("fopen failed for %s", filename);
		return 0;
	}

	// write header
	int magic = TEXGZ_MAGIC;
	if(gzwrite(f, (const void*) &magic,
	           sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write MAGIC");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->type,
	           sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write type");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->format,
	           sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write format");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->width,
	           sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write width");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->height,
	           sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write height");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->stride,
	           sizeof(int)) != sizeof(int))
	{
		LOGE("failed to write stride");
		goto fail_header;
	}
	if(gzwrite(f, (const void*) &self->vstride,
	           sizeof(int)) != sizeof(int))
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
		int bytes_written = gzwrite(f, (const void*) pixels,
		                            bytes);
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

int texgz_tex_exportz(texgz_tex_t* self,
                      const char* filename)
{
	ASSERT(filename);

	FILE* f = fopen(filename, "w");
	if(f == NULL)
	{
		LOGE("invalid filename=%s", filename);
		return 0;
	}

	int ret = texgz_tex_exportf(self, f);
	fclose(f);
	return ret;
}

int texgz_tex_exportf(texgz_tex_t* self, FILE* f)
{
	ASSERT(self);
	ASSERT(f);

	int bytes = texgz_tex_size(self);
	if(bytes == 0)
	{
		return 0;
	}

	// allocate src buffer
	int size = bytes + TEXGZ_TEX_HSIZE;
	unsigned char* src;
	src = (unsigned char*) MALLOC(size*sizeof(unsigned char));
	if(src == NULL)
	{
		LOGE("MALLOC failed");
		return 0;
	}

	// copy tex to src buffer
	int* srci = (int*) src;
	srci[0] = TEXGZ_MAGIC;
	srci[1] = self->type;
	srci[2] = self->format;
	srci[3] = self->width;
	srci[4] = self->height;
	srci[5] = self->stride;
	srci[6] = self->vstride;
	memcpy(&src[TEXGZ_TEX_HSIZE], self->pixels, bytes);

	// allocate dst buffer
	uLong src_size = (uLong) (TEXGZ_TEX_HSIZE + bytes);
	uLong dst_size = compressBound(src_size);
	unsigned char* dst;
	dst = (unsigned char*)
	      MALLOC(dst_size*sizeof(unsigned char));
	if(dst == NULL)
	{
		LOGE("MALLOC failed");
		goto fail_dst;
	}

	// compress buffer
	if(compress((Bytef*) dst, (uLongf*) &dst_size,
	            (const Bytef*) src, src_size) != Z_OK)
	{
		LOGE("compress failed");
		goto fail_compress;
	}

	// write buffer
	if(fwrite(dst, sizeof(unsigned char), dst_size,
	          f) != dst_size)
	{
		LOGE("fwrite failed");
		goto fail_fwrite;
	}

	// free src and dst buffer
	FREE(src);
	FREE(dst);

	// success
	return 1;

	// failure
	fail_fwrite:
	fail_compress:
		FREE(dst);
	fail_dst:
		FREE(src);
	return 0;
}

int texgz_tex_convert(texgz_tex_t* self, int type,
                      int format)
{
	ASSERT(self);

	// already in requested format
	if((type == self->type) && (format == self->format))
		return 1;

	// convert L,A,LABL
	if((type == TEXGZ_UNSIGNED_BYTE)         &&
	   (self->type   == TEXGZ_UNSIGNED_BYTE) &&
	   ((format == TEXGZ_ALPHA)     ||
	    (format == TEXGZ_LUMINANCE) ||
	    (format == TEXGZ_LABL)) &&
	   ((self->format == TEXGZ_ALPHA)     ||
	    (self->format == TEXGZ_LUMINANCE) ||
	    (self->format == TEXGZ_LABL)))
	{
		self->format = format;
		return 1;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_convertcopy(self, type, format);
	if(tex == NULL)
		return 0;

	// swap the data
	texgz_tex_t tmp = *self;
	*self = *tex;
	*tex = tmp;

	texgz_tex_delete(&tex);
	return 1;
}

int texgz_tex_convertF(texgz_tex_t* self,
                       float min, float max,
                       int type, int format)
{
	ASSERT(self);

	// already in requested format
	if((type == self->type) && (format == self->format))
		return 1;

	texgz_tex_t* tex;
	tex = texgz_tex_convertFcopy(self, min, max, type, format);
	if(tex == NULL)
		return 0;

	// swap the data
	texgz_tex_t tmp = *self;
	*self = *tex;
	*tex = tmp;

	texgz_tex_delete(&tex);
	return 1;
}

texgz_tex_t*
texgz_tex_convertcopy(texgz_tex_t* self, int type,
                      int format)
{
	ASSERT(self);

	// copy in the case that the src and dst formats match
	// in particular this case prevents the conversion of
	// 8888 to 8888
	if((type == self->type) && (format == self->format))
		return texgz_tex_copy(self);

	// convert to RGBA-8888
	// No conversions are allowed on TEXGZ_SHORT
	texgz_tex_t* tmp        = NULL;
	int      tmp_delete = 1;   // delete if self is not 8888
	if((self->type == TEXGZ_UNSIGNED_SHORT_4_4_4_4) &&
	   (self->format == TEXGZ_RGBA))
		tmp = texgz_tex_4444to8888(self);
	else if((self->type == TEXGZ_UNSIGNED_SHORT_5_6_5) &&
	        (self->format == TEXGZ_RGB))
		tmp = texgz_tex_565to8888(self);
	else if((self->type == TEXGZ_UNSIGNED_SHORT_5_5_5_1) &&
	        (self->format == TEXGZ_RGBA))
		tmp = texgz_tex_5551to8888(self);
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_RGB))
		tmp = texgz_tex_888to8888(self);
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_LUMINANCE))
		tmp = texgz_tex_Lto8888(self);
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_ALPHA))
		tmp = texgz_tex_Ato8888(self);
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_LUMINANCE_ALPHA))
	{
		if(format == TEXGZ_RG00)
		{
			tmp    = texgz_tex_LAto8800(self);
			format = TEXGZ_RGBA;
		}
		else
		{
			tmp = texgz_tex_LAto8888(self);
		}
	}
	else if((self->type == TEXGZ_FLOAT) &&
	        (self->format == TEXGZ_LUMINANCE))
		tmp = texgz_tex_Fto8888(self, 0.0f, 1.0f);
	else if((self->type == TEXGZ_FLOAT) &&
	        (self->format == TEXGZ_RGBA))
		tmp = texgz_tex_FFFFto8888(self, 0.0f, 1.0f);
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_BGRA))
		tmp = texgz_tex_8888format(self, TEXGZ_RGBA);
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_RGBA))
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
	if((type == TEXGZ_UNSIGNED_SHORT_4_4_4_4) &&
	   (format == TEXGZ_RGBA))
		tex = texgz_tex_8888to4444(tmp);
	else if((type == TEXGZ_UNSIGNED_SHORT_5_6_5) &&
	        (format == TEXGZ_RGB))
		tex = texgz_tex_8888to565(tmp);
	else if((type == TEXGZ_UNSIGNED_SHORT_5_5_5_1) &&
	        (format == TEXGZ_RGBA))
		tex = texgz_tex_8888to5551(tmp);
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_RGB))
		tex = texgz_tex_8888to888(tmp);
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_LUMINANCE))
		tex = texgz_tex_8888toL(tmp);
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_LABL))
		tex = texgz_tex_8888toLABL(tmp);
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_ALPHA))
		tex = texgz_tex_8888toA(tmp);
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_LUMINANCE_ALPHA))
		tex = texgz_tex_8888toLA(tmp);
	else if((type == TEXGZ_FLOAT) &&
	        (format == TEXGZ_LUMINANCE))
		tex = texgz_tex_8888toF(tmp);
	else if((type == TEXGZ_FLOAT) &&
	        (format == TEXGZ_RGBA))
		tex = texgz_tex_8888toFFFF(tmp, 0.0f, 1.0f);
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_BGRA))
		tex = texgz_tex_8888format(tmp, TEXGZ_BGRA);
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_RGBA))
	{
		// note that tex will never equal self because of the
		// shortcut copy
		tex        = tmp;
		tmp_delete = 0;
	}

	// cleanup
	if(tmp_delete == 1)
		texgz_tex_delete(&tmp);

	// check for errors
	if(tex == NULL)
	{
		LOGE("could not convert to type=0x%X, format=0x%X",
		     type, format);
		return NULL;
	}

	// success
	return tex;
}

texgz_tex_t*
texgz_tex_convertFcopy(texgz_tex_t* self,
                       float min, float max,
                       int type, int format)
{
	ASSERT(self);

	if((self->type   == TEXGZ_FLOAT)         &&
	   (self->format == TEXGZ_RGBA)          &&
	   (type         == TEXGZ_UNSIGNED_BYTE) &&
	   (format       == TEXGZ_RGBA))
	{
		return texgz_tex_FFFFto8888(self, min, max);
	}
	else if((self->type   == TEXGZ_FLOAT)         &&
	        (self->format == TEXGZ_LUMINANCE)     &&
	        (type         == TEXGZ_UNSIGNED_BYTE) &&
	        (format       == TEXGZ_RGBA))
	{
		return texgz_tex_Fto8888(self, min, max);
	}
	else if((self->type   == TEXGZ_FLOAT)         &&
	        (self->format == TEXGZ_LUMINANCE)     &&
	        (type         == TEXGZ_UNSIGNED_BYTE) &&
	        (format       == TEXGZ_LUMINANCE))
	{
		return texgz_tex_Fto8(self, min, max);
	}
	else if((self->type   == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_LUMINANCE)     &&
	        (type         == TEXGZ_FLOAT)         &&
	        (format       == TEXGZ_LUMINANCE))
	{
		return texgz_tex_8toF(self, min, max);
	}
	else if((self->type   == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_RGBA)          &&
	        (type         == TEXGZ_FLOAT)         &&
	        (format       == TEXGZ_RGBA))
	{
		return texgz_tex_8888toFFFF(self, min, max);
	}
	else
	{
		LOGE("invalid type=0x%X:0x%X, format=0x%X:0x%X",
		     self->type, type, self->format, format);
		return NULL;
	}
}

texgz_tex_t* texgz_tex_grayscaleF(texgz_tex_t* self)
{
	ASSERT(self);
	ASSERT(self->type == TEXGZ_FLOAT);

	// channels
	int n;
	if(self->format == TEXGZ_RGBA)
	{
		n = 4;
	}
	else if(self->format == TEXGZ_RGB)
	{
		n = 3;
	}
	else
	{
		LOGE("invalid format=0x%X", self->format);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
	                    self->stride, self->vstride,
	                    TEXGZ_FLOAT, TEXGZ_LUMINANCE,
	                    NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	float src[4] = { 0 };
	float dst;
	int   x;
	int   y;
	int   i;
	for(y = 0; y < self->height; ++y)
	{
		for(x = 0; x < self->width; ++x)
		{
			dst = 0.0f;
			texgz_tex_getPixelF(self, x, y, src);
			for(i = 0; i < n; ++i)
			{
				dst += src[i];
			}
			dst /= n;

			texgz_tex_setPixelF(tex, x, y, &dst);
		}
	}

	return tex;
}

int texgz_tex_RGB2LABF(texgz_tex_t* self,
                       texgz_tex_t** _labl,
                       texgz_tex_t** _laba,
                       texgz_tex_t** _labb)
{
	ASSERT(self);
	ASSERT(_labl);
	ASSERT(_laba);
	ASSERT(_labb);

	// check for supported type/format
	int channels = 0;
	if(self->type == TEXGZ_UNSIGNED_BYTE)
	{
		if(self->format == TEXGZ_RGBA)
		{
			channels = 4;
		}
		else if(self->format == TEXGZ_RGB)
		{
			channels = 3;
		}
	}

	if(channels == 0)
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
		return 0;
	}

	texgz_tex_t* labl;
	labl = texgz_tex_new(self->width, self->height,
	                     self->stride, self->vstride,
	                     TEXGZ_FLOAT, TEXGZ_LUMINANCE,
	                     NULL);
	if(labl == NULL)
	{
		return 0;
	}

	texgz_tex_t* laba;
	laba = texgz_tex_new(self->width, self->height,
	                     self->stride, self->vstride,
	                     TEXGZ_FLOAT, TEXGZ_LUMINANCE,
	                     NULL);
	if(laba == NULL)
	{
		goto fail_laba;
	}

	texgz_tex_t* labb;
	labb = texgz_tex_new(self->width, self->height,
	                     self->stride, self->vstride,
	                     TEXGZ_FLOAT, TEXGZ_LUMINANCE,
	                     NULL);
	if(labb == NULL)
	{
		goto fail_labb;
	}

	// See rgb2lab
	// https://github.com/antimatter15/rgb-lab/blob/master/color.js
	float r;
	float g;
	float b;
	float xx;
	float yy;
	float zz;
	float pixel_labl[4] = { 0 };
	float pixel_laba[4] = { 0 };
	float pixel_labb[4] = { 0 };
	int x, y, idx;
	unsigned char* pixel_src;
	for(y = 0; y < self->height; ++y)
	{
		for(x = 0; x < self->width; ++x)
		{
			idx       = y*self->stride + x;
			pixel_src = &self->pixels[channels*idx];

			r = ((float) pixel_src[0])/255.0f;
			g = ((float) pixel_src[1])/255.0f;
			b = ((float) pixel_src[2])/255.0f;

			r = (r > 0.04045f) ? powf((r + 0.055f)/1.055f, 2.4f) : r/12.92f;
			g = (g > 0.04045f) ? powf((g + 0.055f)/1.055f, 2.4f) : g/12.92f;
			b = (b > 0.04045f) ? powf((b + 0.055f)/1.055f, 2.4f) : b/12.92f;

			xx = (r*0.4124f + g*0.3576f + b*0.1805f)/0.95047f;
			yy = (r*0.2126f + g*0.7152f + b*0.0722f)/1.00000f;
			zz = (r*0.0193f + g*0.1192f + b*0.9505f)/1.08883f;

			xx = (xx > 0.008856f) ? powf(xx, 0.333333f) : (7.787f*xx) + 16.0f/116.0f;
			yy = (yy > 0.008856f) ? powf(yy, 0.333333f) : (7.787f*yy) + 16.0f/116.0f;
			zz = (zz > 0.008856f) ? powf(zz, 0.333333f) : (7.787f*zz) + 16.0f/116.0f;

			pixel_labl[0] = 116.0f*yy - 16.0f;
			pixel_laba[0] = 500.0f*(xx - yy);
			pixel_labb[0] = 200.0f*(yy - zz);

			texgz_tex_setPixelF(labl, x, y, pixel_labl);
			texgz_tex_setPixelF(laba, x, y, pixel_laba);
			texgz_tex_setPixelF(labb, x, y, pixel_labb);
		}
	}

	*_labl = labl;
	*_laba = laba;
	*_labb = labb;

	// success
	return 1;

	// failure
	fail_labb:
		texgz_tex_delete(&laba);
	fail_laba:
		texgz_tex_delete(&labl);
	return 0;
}

texgz_tex_t*
texgz_tex_channelF(texgz_tex_t* self, int channel,
                   float min, float max)
{
	ASSERT(self);

	int channels = texgz_tex_channels(self);
	if((channels == 0) || (channel >= channels))
	{
		LOGE("invalid format=0x%X, channel=%i",
		     self->format, channel);
		return NULL;
	}

	// check type
	if((self->type == TEXGZ_FLOAT) ||
	   (self->type == TEXGZ_UNSIGNED_BYTE))
	{
		// ok
	}
	else
	{
		LOGE("invalid type=0x%X", self->type);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
	                    self->stride, self->vstride,
	                    TEXGZ_FLOAT, TEXGZ_LUMINANCE,
	                    NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	// copy channel
	int x;
	int y;
	unsigned char srcu[4] = { 0 };
	float         srcf[4] = { 0 };
	float         dstf[4] = { 0 };
	if(self->type == TEXGZ_FLOAT)
	{
		for(y = 0; y < self->height; ++y)
		{
			for(x = 0; x < self->width; ++x)
			{
				// ignore min/max for floats
				texgz_tex_getPixelF(self, x, y, srcf);
				dstf[0] = srcf[channel];
				texgz_tex_setPixelF(tex, x, y, dstf);
			}
		}
	}
	else
	{
		for(y = 0; y < self->height; ++y)
		{
			for(x = 0; x < self->width; ++x)
			{
				texgz_tex_getPixel(self, x, y, srcu);
				srcf[0] = (float) srcu[channel];
				dstf[0] = (max - min)*srcf[0]/255.0f + min;
				texgz_tex_setPixelF(tex, x, y, dstf);
			}
		}
	}

	return tex;
}

void
texgz_tex_convolveF(texgz_tex_t* src, texgz_tex_t* dst,
                    int mw, int mh,
                    int stride, int vstride,
                    float* mask)
{
	ASSERT(src);
	ASSERT(dst);
	ASSERT(mask);
	ASSERT(src->width  == (stride*dst->width));
	ASSERT(src->height == (vstride*dst->height));
	ASSERT(src->type == TEXGZ_FLOAT);
	ASSERT(dst->type == TEXGZ_FLOAT);
	ASSERT(texgz_tex_channels(src) ==
	       texgz_tex_channels(dst));

	// compute center offset
	// examples:
	// 1) Sobel Edge: stride=1, mw=3
	//    +---+---+---+
	//    | 0 | 1 | 2 |
	//    +---+---+---+
	//    |   | C |   |
	//    +---+---+---+
	//
	// 2) Box Downsample: stride=2, mw=2
	//    +---+---+
	//    | 0 | 1 |
	//    +---+---+
	//    | C |   |
	//    +---+---+
	//
	// 3) Lanczos3 Downsample: stride=2, mw=12
	//    +---+---+---+---+---+---+---+---+---+---+---+---+
	//    | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
	//    +---+---+---+---+---+---+---+---+---+---+---+---+
	//    |   |   |   |   |   | C |   |   |   |   |   |   |
	//    +---+---+---+---+---+---+---+---+---+---+---+---+
	int cm = (mh - vstride)/2;
	int cn = (mw - stride)/2;

	int i;
	int j;
	int c;
	int m;
	int n;
	int w        = src->width;
	int h        = src->height;
	int channels = texgz_tex_channels(src);
	float pixel[4];
	float f[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for(i = 0; i < h; i += vstride)
	{
		for(j = 0; j < w; j += stride)
		{
			for(c = 0; c < channels; ++c)
			{
				f[c] = 0.0f;
			}

			// convolve
			for(m = 0; m < mh; ++m)
			{
				for(n = 0; n < mw; ++n)
				{
					texgz_tex_getClampedPixelF(src, j + n - cn,
					                           i + m - cm, pixel);

					for(c = 0; c < channels; ++c)
					{
						f[c] += mask[m*mw + n]*pixel[c];
					}
				}
			}

			// decimate and fill pixel
			texgz_tex_setPixelF(dst, j/stride, i/vstride, f);
		}
	}
}

int texgz_tex_blur(texgz_tex_t* self, float sigma,
                   float mu, int size)
{
	texgz_tex_t* tex;
	tex = texgz_tex_blurcopy(self, sigma, mu, size);
	if(tex == NULL)
	{
		return 0;
	}

	// swap the data
	texgz_tex_t tmp = *self;
	*self = *tex;
	*tex = tmp;

	texgz_tex_delete(&tex);
	return 1;
}

texgz_tex_t*
texgz_tex_blurcopy(texgz_tex_t* self, float sigma,
                   float mu, int size)
{
	ASSERT(self);

	// blur is designed for 4-channel RGBA
	if((self->type   != TEXGZ_UNSIGNED_BYTE) ||
	   (self->format != TEXGZ_RGBA))
	{
		LOGE("invalid type=%i, format=%i",
		     self->type, self->format);
		return NULL;
	}

	const int max_size = 7;
	if(size > max_size)
	{
		LOGE("invalid size=%i", size);
		return NULL;
	}

	float mask[max_size];
	if(texgz_tex_gaussianCoef(sigma, mu, size, mask) == 0)
	{
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_convertFcopy(self, 0.0f, 1.0f,
	                             TEXGZ_FLOAT, TEXGZ_RGBA);
	if(tex == NULL)
	{
		return NULL;
	}

	texgz_tex_t* conv;
	conv = texgz_tex_new(tex->width, tex->width,
	                     tex->stride, tex->vstride,
	                     tex->type, tex->format,
	                     NULL);
	if(conv == NULL)
	{
		goto fail_conv;
	}

	texgz_tex_convolveF(tex, conv, size, 1, 1, 1, mask);
	texgz_tex_convolveF(conv, tex, 1, size, 1, 1, mask);

	if(texgz_tex_convertF(tex, 0.0f, 1.0f,
	                      TEXGZ_UNSIGNED_BYTE,
	                      TEXGZ_RGBA) == 0)
	{
		goto fail_convert;
	}

	texgz_tex_delete(&conv);

	// success
	return tex;

	fail_convert:
		texgz_tex_delete(&conv);
	fail_conv:
		texgz_tex_delete(&tex);
	return NULL;
}

int texgz_tex_rotate90(texgz_tex_t* self)
{
	texgz_tex_t* tex;
	tex = texgz_tex_rotate90copy(self);
	if(tex == NULL)
	{
		return 0;
	}

	// swap the data
	texgz_tex_t tmp = *self;
	*self = *tex;
	*tex = tmp;

	texgz_tex_delete(&tex);
	return 1;
}

texgz_tex_t*
texgz_tex_rotate90copy(texgz_tex_t* self)
{
	ASSERT(self);

	if(self->width != self->height)
	{
		LOGE("invalid width=%i, height=%i",
		     self->width, self->height);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
	                    self->stride, self->vstride,
	                    self->type, self->format, NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	int x;
	int y;
	int w = self->width;
	unsigned char pixel[4];
	for(y = 0; y < self->height; ++y)
	{
		for(x = 0; x < self->width; ++x)
		{
			texgz_tex_getPixel(self, x, y, pixel);
			texgz_tex_setPixel(tex, (w - 1 - y), x, pixel);
		}
	}

	return tex;
}

int texgz_tex_rotate180(texgz_tex_t* self)
{
	texgz_tex_t* tex;
	tex = texgz_tex_rotate180copy(self);
	if(tex == NULL)
	{
		return 0;
	}

	// swap the data
	texgz_tex_t tmp = *self;
	*self = *tex;
	*tex = tmp;

	texgz_tex_delete(&tex);
	return 1;
}

texgz_tex_t*
texgz_tex_rotate180copy(texgz_tex_t* self)
{
	ASSERT(self);

	if(self->width != self->height)
	{
		LOGE("invalid width=%i, height=%i",
		     self->width, self->height);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
	                    self->stride, self->vstride,
	                    self->type, self->format, NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	int x;
	int y;
	int w = self->width;
	unsigned char pixel[4];
	for(y = 0; y < self->height; ++y)
	{
		for(x = 0; x < self->width; ++x)
		{
			texgz_tex_getPixel(self, x, y, pixel);
			texgz_tex_setPixel(tex, (w - 1 - x), (w - 1 - y), pixel);
		}
	}

	return tex;
}

int texgz_tex_rotate270(texgz_tex_t* self)
{
	texgz_tex_t* tex;
	tex = texgz_tex_rotate270copy(self);
	if(tex == NULL)
	{
		return 0;
	}

	// swap the data
	texgz_tex_t tmp = *self;
	*self = *tex;
	*tex = tmp;

	texgz_tex_delete(&tex);
	return 1;
}

texgz_tex_t*
texgz_tex_rotate270copy(texgz_tex_t* self)
{
	ASSERT(self);

	if(self->width != self->height)
	{
		LOGE("invalid width=%i, height=%i",
		     self->width, self->height);
		return NULL;
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
	                    self->stride, self->vstride,
	                    self->type, self->format, NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	int x;
	int y;
	int w = self->width;
	unsigned char pixel[4];
	for(y = 0; y < self->height; ++y)
	{
		for(x = 0; x < self->width; ++x)
		{
			texgz_tex_getPixel(self, x, y, pixel);
			texgz_tex_setPixel(tex, y, (w - 1 - x), pixel);
		}
	}

	return tex;
}

int texgz_tex_flipvertical(texgz_tex_t* self)
{
	ASSERT(self);

	texgz_tex_t* tex = texgz_tex_flipverticalcopy(self);
	if(tex == NULL)
	{
		return 0;
	}

	// swap the data
	texgz_tex_t tmp = *self;
	*self = *tex;
	*tex = tmp;

	texgz_tex_delete(&tex);
	return 1;
}

texgz_tex_t* texgz_tex_flipverticalcopy(texgz_tex_t* self)
{
	ASSERT(self);

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
	                    self->stride, self->vstride,
	                    self->type, self->format, NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	int y;
	int bpp    = texgz_tex_bpp(self);
	int stride = self->stride;
	int width  = self->width;
	int height = self->height;
	for(y = 0; y < height; ++y)
	{
		unsigned char* src;
		unsigned char* dst;
		src = &self->pixels[y*bpp*stride];
		dst = &tex->pixels[(height - 1 - y)*bpp*stride];
		memcpy(dst, src, bpp*width);
	}
	return tex;
}

int texgz_tex_crop(texgz_tex_t* self, int top, int left,
                   int bottom, int right)
{
	ASSERT(self);

	texgz_tex_t* tex;
	tex = texgz_tex_cropcopy(self, top, left, bottom, right);
	if(tex == NULL)
		return 0;

	// swap the data
	texgz_tex_t tmp = *self;
	*self = *tex;
	*tex = tmp;

	texgz_tex_delete(&tex);
	return 1;
}

texgz_tex_t*
texgz_tex_cropcopy(texgz_tex_t* self, int top, int left,
                   int bottom, int right)
{
	ASSERT(self);

	// crop rectangle is inclusive
	// i.e. {0, 0, 0, 0} is a single pixel at {0, 0}
	if((top < 0) ||
	   (top > bottom) ||
	   (left < 0) ||
	   (left > right) ||
	   (right >= self->width) ||
	   (bottom >= self->height))
	{
		LOGE("invalid top=%i, left=%i, bottom=%i, right=%i",
		     top, left, bottom, right);
		return NULL;
	}


	int width  = right - left + 1;
	int height = bottom - top + 1;
	texgz_tex_t* tex;
	tex = texgz_tex_new(width, height, width, height,
	                    self->type, self->format, NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	// blit
	int y;
	int bpp = texgz_tex_bpp(self);
	for(y = 0; y < height; ++y)
	{
		int src_y = y + top;
		unsigned char* src;
		unsigned char* dst;
		src = &self->pixels[src_y*bpp*self->stride];
		dst = &tex->pixels[y*bpp*width];
		memcpy(dst, src, bpp*width);
	}
	return tex;
}

int texgz_tex_pad(texgz_tex_t* self)
{
	ASSERT(self);

	int pot_stride  = texgz_nextpot(self->stride);
	int pot_vstride = texgz_nextpot(self->vstride);
	if((pot_stride == self->stride) &&
	   (pot_vstride == self->vstride))
	{
		// already power-of-two
		return 1;
	}

	texgz_tex_t* tex = texgz_tex_padcopy(self);
	if(tex == NULL)
		return 0;

	// swap the data
	texgz_tex_t tmp = *self;
	*self = *tex;
	*tex = tmp;

	texgz_tex_delete(&tex);
	return 1;
}

texgz_tex_t* texgz_tex_padcopy(texgz_tex_t* self)
{
	ASSERT(self);

	int pot_stride  = texgz_nextpot(self->stride);
	int pot_vstride = texgz_nextpot(self->vstride);
	if((pot_stride == self->stride) &&
	   (pot_vstride == self->vstride))
	{
		// already power-of-two
		return texgz_tex_copy(self);
	}

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
	                    pot_stride, pot_vstride,
	                    self->type, self->format, NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	// blit
	int y;
	int bpp = texgz_tex_bpp(self);
	for(y = 0; y < tex->height; ++y)
	{
		unsigned char* src = &self->pixels[y*bpp*self->stride];
		unsigned char* dst = &tex->pixels[y*bpp*tex->stride];
		memcpy(dst, src, bpp*tex->width);
	}
	return tex;
}

texgz_tex_t* texgz_tex_outline(texgz_tex_t* self, int size)
{
	ASSERT(self);

	// validate the outline circle
	int    off     = size/2;
	float* mask = NULL;
	if(size == 3)
	{
		mask = TEXGZ_TEX_OUTLINE3;
	}
	else if(size == 5)
	{
		mask = TEXGZ_TEX_OUTLINE5;
	}
	else if(size == 7)
	{
		mask = TEXGZ_TEX_OUTLINE7;
	}
	else if(size == 9)
	{
		mask = TEXGZ_TEX_OUTLINE9;
	}
	else if(size == 11)
	{
		mask = TEXGZ_TEX_OUTLINE11;
	}
	else
	{
		LOGE("invalid size=%i", size);
		return NULL;
	}

	// validate the input tex
	if(((self->format == TEXGZ_ALPHA)     ||
	    (self->format == TEXGZ_LUMINANCE) ||
	    (self->format == TEXGZ_LABL)) &&
	   (self->type == TEXGZ_UNSIGNED_BYTE))
	{
		// OK
	}
	else
	{
		LOGE("invalid format=0x%X, type=0x%X",
		     self->format, self->type);
		return NULL;
	}

	// create the dst tex
	int w2 = self->width  + 2*off;
	int h2 = self->height + 2*off;
	int w2r = w2 + (w2%2);
	int h2r = h2 + (h2%2);
	texgz_tex_t* tex;
	tex = texgz_tex_new(w2r, h2r, w2r, h2r,
	                    TEXGZ_UNSIGNED_BYTE,
	                    TEXGZ_LUMINANCE_ALPHA, NULL);
	if(tex == NULL)
	{
		return NULL;
	}

	// copy the base
	int i;
	int j;
	unsigned char* ps = self->pixels;
	unsigned char* pd = tex->pixels;
	for(i = 0; i < self->height; ++i)
	{
		for(j = 0; j < self->width; ++j)
		{
			int i2 = i + off;
			int j2 = j + off;
			pd[2*(i2*tex->stride + j2)] = ps[i*self->stride + j];
		}
	}

	// sample the outline
	for(i = 0; i < h2; ++i)
	{
		for(j = 0; j < w2; ++j)
		{
			texgz_tex_sampleOutline(tex, i, j, mask, size);
		}
	}

	// success
	return tex;

	// failure
	return NULL;
}

int texgz_tex_blit(texgz_tex_t* src, texgz_tex_t* dst,
                   int width, int height,
                   int xs, int ys, int xd, int yd)
{
	ASSERT(src);
	ASSERT(dst);

	if((src->type != dst->type) ||
	   (src->format != dst->format))
	{
		LOGE("invalid src: type=0x%X, format=0x%X, dst: type=0x%x, format=0x%X",
		     src->type, src->format, dst->type, dst->format);
		return 0;
	}

	if((width <= 0) || (height <= 0) ||
	   (xs + width > src->width) || (ys + height > src->height) ||
	   (xd + width > dst->width) || (yd + height > dst->height))
	{
		LOGE("invalid width=%i, height=%i, xs=%i, ys=%i, xd=%i, yd=%i",
		     width, height, xs, ys, xd, yd);
		return 0;
	}

	// blit
	int i;
	int bpp   = texgz_tex_bpp(src);
	int bytes = width*bpp;
	for(i = 0; i < height; ++i)
	{
		int os = bpp*((ys + i)*src->stride + xs);
		int od = bpp*((yd + i)*dst->stride + xd);
		unsigned char* ps = &src->pixels[os];
		unsigned char* pd = &dst->pixels[od];

		memcpy((void*) pd, (void*) ps, bytes);
	}

	return 1;
}

void texgz_tex_clear(texgz_tex_t* self)
{
	ASSERT(self);

	memset(self->pixels, 0, MEMSIZEPTR(self->pixels));
}

void texgz_tex_fill(texgz_tex_t* self,
                    int top, int left,
                    int width, int height,
                    unsigned int color)
{
	ASSERT(self);
	ASSERT(self->type   == TEXGZ_UNSIGNED_BYTE);
	ASSERT(self->format == TEXGZ_RGBA);

	if((width  <= 0) || (height <= 0))
	{
		// ignore
		return;
	}

	// clip rectangle
	int bottom = top  + height - 1;
	int right  = left + width  - 1;
	if((top    >= self->height) ||
	   (left   >= self->width)  ||
	   (bottom < 0)             ||
	   (right  < 0))
	{
		return;
	}

	// resize rectangle
	if(top < 0)
	{
		top = 0;
	}
	if(left < 0)
	{
		left = 0;
	}
	if(bottom >= self->height)
	{
		bottom = self->height - 1;
	}
	if(right >= self->width)
	{
		right = self->width - 1;
	}

	unsigned char r = (unsigned char) ((color >> 24)&0xFF);
	unsigned char g = (unsigned char) ((color >> 16)&0xFF);
	unsigned char b = (unsigned char) ((color >> 8)&0xFF);
	unsigned char a = (unsigned char) (color&0xFF);

	// fill first scanline
	int i;
	int j;
	int bpp = texgz_tex_bpp(self);
	unsigned char* pix;
	for(i = top; i <= bottom; ++i)
	{
		int roff = bpp*i*self->stride;
		for(j = left; j <= right; ++j)
		{
			int coff = bpp*j;
			pix = &self->pixels[roff + coff];
			pix[0] = r;
			pix[1] = g;
			pix[2] = b;
			pix[3] = a;
		}
	}
}

void texgz_tex_lineDraw(texgz_tex_t* self,
                        float x0, float y0,
                        float x1, float y1,
                        unsigned char* pixel)
{
	ASSERT(self);
	ASSERT(pixel);

	ASSERT(self->type    == TEXGZ_UNSIGNED_BYTE);
	ASSERT((self->format == TEXGZ_RGB)  ||
	       (self->format == TEXGZ_RGBA) ||
	       (self->format == TEXGZ_LUMINANCE));

	int direction = 1;
	if(texgz_tex_lineClip(self, &direction, &x0, &y0, &x1, &y1))
	{
		return;
	}

	texgz_tex_lineDrawClipped(self, x0, y0, x1, y1, pixel);
}

void texgz_tex_lineDrawF(texgz_tex_t* self,
                         float x0, float y0,
                         float x1, float y1,
                         float pixel)
{
	ASSERT(self);
	ASSERT(self->type   == TEXGZ_FLOAT);
	ASSERT(self->format == TEXGZ_LUMINANCE);

	int direction = 1;
	if(texgz_tex_lineClip(self, &direction, &x0, &y0, &x1, &y1))
	{
		return;
	}

	texgz_tex_lineDrawClippedF(self, x0, y0, x1, y1, pixel);
}

void texgz_tex_lineSampleF(texgz_tex_t* self, int id,
                           float x0, float y0,
                           float x1, float y1,
                           int* _count,
                           int* _size,
                           texgz_sampleF_t** _samples)
{
	ASSERT(self);
	ASSERT(_count);
	ASSERT(_size);
	ASSERT(_samples);

	int direction = 1;
	if(texgz_tex_lineClip(self, &direction, &x0, &y0, &x1, &y1))
	{
		return;
	}

	texgz_tex_lineSampleClippedF(self, id, x0, y0, x1, y1,
	                             direction,
	                             _count, _size, _samples);
}

void texgz_tex_sample(texgz_tex_t* self, float u, float v,
                      int bpp, unsigned char* pixel)
{
	ASSERT(self);
	ASSERT(pixel);
	ASSERT((u >= 0.0f) && (u <= 1.0f));
	ASSERT((v >= 0.0f) && (v <= 1.0f));
	ASSERT(self->type == TEXGZ_UNSIGNED_BYTE);

	// skip expensive ASSERT
	// ASSERT(bpp == texgz_tex_bpp(self));

	// "float indices"
	float x = u*(self->width  - 1);
	float y = v*(self->height - 1);

	// determine indices to sample
	int x0 = (int) x;
	int y0 = (int) y;
	int x1 = x0 + 1;
	int y1 = y0 + 1;

	// double check the indices
	if((x1 >= self->width) || (y1 >= self->height))
	{
		texgz_tex_getPixel(self, x0, y0, pixel);
		return;
	}

	// compute interpolation coefficients
	float s = x - ((float) x0);
	float t = y - ((float) y0);

	// sample interpolation values
	int i;
	int offset00 = bpp*(y0*self->stride + x0);
	int offset01 = bpp*(y1*self->stride + x0);
	int offset10 = bpp*(y0*self->stride + x1);
	int offset11 = bpp*(y1*self->stride + x1);
	unsigned char* pixels = self->pixels;
	for(i = 0; i < bpp; ++i)
	{
		// convert component to float
		float f00 = (float) pixels[offset00 + i];
		float f01 = (float) pixels[offset01 + i];
		float f10 = (float) pixels[offset10 + i];
		float f11 = (float) pixels[offset11 + i];

		// interpolate x
		float f0010 = f00 + s*(f10 - f00);
		float f0111 = f01 + s*(f11 - f01);

		// interpolate y
		pixel[i] = (unsigned char)
		           (f0010 + t*(f0111 - f0010) + 0.5f);
	}
}

static void
texgz_tex_cubicInterpolateRGBA(unsigned char* p, float s,
                               unsigned char* out)
{
	ASSERT(p);
	ASSERT(out);

	// interpolate each component

	float p0 = (float) p[0];
	float p1 = (float) p[4];
	float p2 = (float) p[8];
	float p3 = (float) p[12];
	float pi = cc_clamp(p1 + 0.5f*s*(p2 - p0 + s*(2.0f*p0 - 5.0f*p1 + 4.0f*p2 - p3 + s*(3.0f*(p1 - p2) + p3 - p0))), 0.0f, 255.0f);
	out[0]   = (unsigned char) pi;

	p0     = (float) p[1];
	p1     = (float) p[5];
	p2     = (float) p[9];
	p3     = (float) p[13];
	pi     = cc_clamp(p1 + 0.5f*s*(p2 - p0 + s*(2.0f*p0 - 5.0f*p1 + 4.0f*p2 - p3 + s*(3.0f*(p1 - p2) + p3 - p0))), 0.0f, 255.0f);
	out[1] = (unsigned char) pi;

	p0     = (float) p[2];
	p1     = (float) p[6];
	p2     = (float) p[10];
	p3     = (float) p[14];
	pi     = cc_clamp(p1 + 0.5f*s*(p2 - p0 + s*(2.0f*p0 - 5.0f*p1 + 4.0f*p2 - p3 + s*(3.0f*(p1 - p2) + p3 - p0))), 0.0f, 255.0f);
	out[2] = (unsigned char) pi;

	p0     = (float) p[3];
	p1     = (float) p[7];
	p2     = (float) p[11];
	p3     = (float) p[15];
	pi     = cc_clamp(p1 + 0.5f*s*(p2 - p0 + s*(2.0f*p0 - 5.0f*p1 + 4.0f*p2 - p3 + s*(3.0f*(p1 - p2) + p3 - p0))), 0.0f, 255.0f);
	out[3] = (unsigned char) pi;
}

void
texgz_tex_sampleBicubicRGBA(texgz_tex_t* self,
                            float u, float v,
                            unsigned char* pixel)
{
	ASSERT(self);
	ASSERT(pixel);
	ASSERT((u >= 0.0f) && (u <= 1.0f));
	ASSERT((v >= 0.0f) && (v <= 1.0f));
	ASSERT(self->type   == TEXGZ_UNSIGNED_BYTE);
	ASSERT(self->format == TEXGZ_RGBA);

	// http://paulbourke.net/miscellaneous/interpolation/
	// https://www.paulinternet.nl/?page=bicubic

	// "float indices"
	float x = u*(self->width  - 1);
	float y = v*(self->height - 1);

	// determine indices to sample
	int x1 = (int) x;
	int y1 = (int) y;
	int x0 = x1 - 1;
	int y0 = y1 - 1;
	int y2 = y1 + 1;
	int x3 = x1 + 2;
	int y3 = y1 + 2;

	// double check the indices
	if((x0 < 0) ||
	   (y0 < 0) ||
	   (x3 >= self->width) ||
	   (y3 >= self->height))
	{
		texgz_tex_sample(self, u, v, 4, pixel);
		return;
	}

	// compute interpolation coefficients
	float s = x - ((float) x1);
	float t = y - ((float) y1);

	// sample interpolation values
	int bpp = 4;
	unsigned char* pixels  = self->pixels;
	unsigned char* pixels0 = &pixels[bpp*(y0*self->stride + x0)];
	unsigned char* pixels1 = &pixels[bpp*(y1*self->stride + x0)];
	unsigned char* pixels2 = &pixels[bpp*(y2*self->stride + x0)];
	unsigned char* pixels3 = &pixels[bpp*(y3*self->stride + x0)];
	unsigned char  pixelsi[4*bpp];
	texgz_tex_cubicInterpolateRGBA(pixels0, s, pixelsi),
	texgz_tex_cubicInterpolateRGBA(pixels1, s, &pixelsi[bpp]),
	texgz_tex_cubicInterpolateRGBA(pixels2, s, &pixelsi[2*bpp]),
	texgz_tex_cubicInterpolateRGBA(pixels3, s, &pixelsi[3*bpp]),
	texgz_tex_cubicInterpolateRGBA(pixelsi, t, pixel);
}

void texgz_tex_getPixel(texgz_tex_t* self,
                        int x, int y,
                        unsigned char* pixel)
{
	ASSERT(self);
	ASSERT(pixel);
	ASSERT(self->type == TEXGZ_UNSIGNED_BYTE);
	ASSERT((self->format == TEXGZ_RGB)  ||
	       (self->format == TEXGZ_RGBA) ||
	       (self->format == TEXGZ_LUMINANCE));

	int idx;
	if(self->format == TEXGZ_RGB)
	{
		idx      = 3*(y*self->stride + x);
		pixel[0] = self->pixels[idx];
		pixel[1] = self->pixels[idx + 1];
		pixel[2] = self->pixels[idx + 2];
		pixel[3] = 0xFF;
	}
	else if(self->format == TEXGZ_RGBA)
	{
		idx      = 4*(y*self->stride + x);
		pixel[0] = self->pixels[idx];
		pixel[1] = self->pixels[idx + 1];
		pixel[2] = self->pixels[idx + 2];
		pixel[3] = self->pixels[idx + 3];
	}
	else
	{
		idx      = y*self->stride + x;
		pixel[0] = self->pixels[idx];
		pixel[1] = pixel[0];
		pixel[2] = pixel[0];
		pixel[3] = 0xFF;
	}
}

void texgz_tex_getPixelF(texgz_tex_t* self,
                         int x, int y, float* pixel)
{
	ASSERT(self);
	ASSERT(self->type == TEXGZ_FLOAT);
	ASSERT((self->format == TEXGZ_LUMINANCE) ||
	       (self->format == TEXGZ_RGBA));

	int    idx;
	float* pixels = (float*) self->pixels;
	if(self->format == TEXGZ_RGBA)
	{
		idx = 4*(y*self->stride + x);
		pixel[0] = pixels[idx];
		pixel[1] = pixels[idx + 1];
		pixel[2] = pixels[idx + 2];
		pixel[3] = pixels[idx + 3];
	}
	else
	{
		idx = y*self->stride + x;
		pixel[0] = pixels[idx];
	}
}

void texgz_tex_getClampedPixelF(texgz_tex_t* self,
                                int x, int y, float* pixel)
{
	ASSERT(self);
	ASSERT(self->type == TEXGZ_FLOAT);
	ASSERT((self->format == TEXGZ_LUMINANCE) ||
	       (self->format == TEXGZ_RGBA));

	if(x < 0)
	{
		x = 0;
	}
	else if(x >= self->width)
	{
		x = self->width - 1;
	}

	if(y < 0)
	{
		y = 0;
	}
	else if(y >= self->height)
	{
		y = self->height - 1;
	}

	int    idx;
	float* pixels = (float*) self->pixels;
	if(self->format == TEXGZ_RGBA)
	{
		idx = 4*(y*self->stride + x);
		pixel[0] = pixels[idx];
		pixel[1] = pixels[idx + 1];
		pixel[2] = pixels[idx + 2];
		pixel[3] = pixels[idx + 3];
	}
	else
	{
		idx = y*self->stride + x;
		pixel[0] = pixels[idx];
	}
}

void texgz_tex_setPixel(texgz_tex_t* self,
                        int x, int y,
                        unsigned char* pixel)
{
	ASSERT(self);
	ASSERT(pixel);
	ASSERT(self->type == TEXGZ_UNSIGNED_BYTE);
	ASSERT((self->format == TEXGZ_RGB)  ||
	       (self->format == TEXGZ_RGBA) ||
	       (self->format == TEXGZ_LUMINANCE));

	int idx;
	if(self->format == TEXGZ_RGB)
	{
		idx = 3*(y*self->stride + x);
		self->pixels[idx]     = pixel[0];
		self->pixels[idx + 1] = pixel[1];
		self->pixels[idx + 2] = pixel[2];
	}
	else if(self->format == TEXGZ_RGBA)
	{
		idx = 4*(y*self->stride + x);
		self->pixels[idx]     = pixel[0];
		self->pixels[idx + 1] = pixel[1];
		self->pixels[idx + 2] = pixel[2];
		self->pixels[idx + 3] = pixel[3];
	}
	else
	{
		idx = y*self->stride + x;
		self->pixels[idx] = pixel[0];
	}
}

void texgz_tex_setPixelF(texgz_tex_t* self,
                         int x, int y,
                         float* pixel)
{
	ASSERT(self);
	ASSERT(pixel);
	ASSERT(self->type == TEXGZ_FLOAT);
	ASSERT((self->format == TEXGZ_LUMINANCE) ||
	       (self->format == TEXGZ_RGBA));

	int    idx;
	float* pixels = (float*) self->pixels;
	if(self->format == TEXGZ_RGBA)
	{
		idx = 4*(y*self->stride + x);
		pixels[idx]     = pixel[0];
		pixels[idx + 1] = pixel[1];
		pixels[idx + 2] = pixel[2];
		pixels[idx + 3] = pixel[3];
	}
	else
	{
		idx = y*self->stride + x;
		pixels[idx] = pixel[0];
	}
}

int texgz_tex_mipmap(texgz_tex_t* self, int miplevels,
                     texgz_tex_t** mipmaps)
{
	ASSERT(self);
	ASSERT(mipmaps);

	// note that mipmaps[0] is self

	// set mipmaps[l]
	int l;
	texgz_tex_t* mip = self;
	texgz_tex_t* down;
	for(l = 1; l < miplevels; ++l)
	{
		down = texgz_tex_downscale(mip);
		if(down == NULL)
		{
			goto fail_downscale;
		}
		mipmaps[l] = down;
		mip = down;
	}

	// set mipmaps[0]
	mipmaps[0] = self;

	// success
	return 1;

	// failure
	fail_downscale:
	{
		int k;
		for(k = 1; k < l; ++k)
		{
			texgz_tex_delete(&mipmaps[k]);
		}
	}
	return 0;
}

int texgz_tex_channels(texgz_tex_t* self)
{
	ASSERT(self);

	if((self->format == TEXGZ_RGBA) ||
	   (self->format == TEXGZ_BGRA))
	{
		return 4;
	}
	else if(self->format == TEXGZ_RGB)
	{
		return 3;
	}
	else if(self->format == TEXGZ_LUMINANCE_ALPHA)
	{
		return 2;
	}
	else if((self->format == TEXGZ_ALPHA) ||
	        (self->format == TEXGZ_LUMINANCE))
	{
		return 1;
	}
	else
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
	}

	return 0;
}

int texgz_tex_bpp(texgz_tex_t* self)
{
	ASSERT(self);

	int bpp = 0;   // bytes-per-pixel
	if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	   (self->format == TEXGZ_RGB))
	{
		bpp = 3;
	}
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_RGBA))
	{
		bpp = 4;
	}
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_BGRA))
	{
		bpp = 4;
	}
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_LUMINANCE))
	{
		bpp = 1;
	}
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_LABL))
	{
		bpp = 1;
	}
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_ALPHA))
	{
		bpp = 1;
	}
	else if((self->type == TEXGZ_SHORT) &&
	        (self->format == TEXGZ_LUMINANCE))
	{
		bpp = 2;
	}
	else if((self->type == TEXGZ_UNSIGNED_BYTE) &&
	        (self->format == TEXGZ_LUMINANCE_ALPHA))
	{
		bpp = 2;
	}
	else if((self->type == TEXGZ_FLOAT) &&
	        (self->format == TEXGZ_LUMINANCE))
	{
		bpp = 4;
	}
	else if((self->type == TEXGZ_FLOAT) &&
	        (self->format == TEXGZ_RGBA))
	{
		bpp = 16;
	}
	else if((self->type == TEXGZ_UNSIGNED_SHORT_5_6_5) &&
	        (self->format == TEXGZ_RGB))
	{
		bpp = 2;
	}
	else if((self->type == TEXGZ_UNSIGNED_SHORT_4_4_4_4) &&
	        (self->format == TEXGZ_RGBA))
	{
		bpp = 2;
	}
	else if((self->type == TEXGZ_UNSIGNED_SHORT_5_5_5_1) &&
	        (self->format == TEXGZ_RGBA))
	{
		bpp = 2;
	}
	else
	{
		LOGE("invalid type=0x%X, format=0x%X",
		     self->type, self->format);
	}

	return bpp;
}

int texgz_tex_size(texgz_tex_t* self)
{
	ASSERT(self);

	return texgz_tex_bpp(self)*self->stride*self->vstride;
}

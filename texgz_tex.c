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
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define LOG_TAG "texgz"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "texgz_tex.h"

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
	int m;
	int n;
	int idx;
	for(m = m0; m <= m1; ++m)
	{
		for(n = n0; n <= n1; ++n)
		{
			idx = 2*((i + m - off)*self->stride + (j + n - off));
			o = mask[m*size + n];
			v = self->pixels[idx];
			f = o*((float) v);
			if(f > max)
			{
				max = f;
				val = v;
			}
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

static texgz_tex_t* texgz_tex_Fto8888(texgz_tex_t* self)
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

			dst[0] = (unsigned char) (255.0f*src[0]);
			dst[1] = (unsigned char) (255.0f*src[0]);
			dst[2] = (unsigned char) (255.0f*src[0]);
			dst[3] = 0xFF;
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

	// uncompress the header
	unsigned char header[TEXGZ_TEX_HSIZE];
	uLong         hsize    = TEXGZ_TEX_HSIZE;
	uLong         src_size = (uLong) size;
	uncompress((Bytef*) header, &hsize, (const Bytef*) src,
	           src_size);
	if(hsize != TEXGZ_TEX_HSIZE)
	{
		LOGE("uncompress failed hsize=%i", (int) hsize);
		goto fail_uncompress_header;
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
		goto fail_parseh;
	}

	// create tex
	texgz_tex_t* self;
	self = texgz_tex_new(width, height, stride, vstride,
	                     type, format, NULL);
	if(self == NULL)
	{
		goto fail_tex;
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
	              (const Bytef*) src, src_size) != Z_OK)
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

	// free src and dst buffers
	FREE(src);
	FREE(dst);

	// success
	return self;

	// failure
	fail_dst_size:
	fail_uncompress:
		FREE(dst);
	fail_dst:
		texgz_tex_delete(&self);
	fail_tex:
	fail_parseh:
	fail_uncompress_header:
	fail_read:
		fseek(f, start, SEEK_SET);
		FREE(src);
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

	// convert L-to-A
	if((type         == TEXGZ_UNSIGNED_BYTE) &&
	   (format       == TEXGZ_ALPHA)         &&
	   (self->type   == TEXGZ_UNSIGNED_BYTE) &&
	   (self->format == TEXGZ_LUMINANCE))
	{
		self->format = TEXGZ_ALPHA;
		return 1;
	}

	// convert A-to-L
	if((type         == TEXGZ_UNSIGNED_BYTE) &&
	   (format       == TEXGZ_LUMINANCE)     &&
	   (self->type   == TEXGZ_UNSIGNED_BYTE) &&
	   (self->format == TEXGZ_ALPHA))
	{
		self->format = TEXGZ_LUMINANCE;
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
		tmp = texgz_tex_Fto8888(self);
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
	        (format == TEXGZ_ALPHA))
		tex = texgz_tex_8888toA(tmp);
	else if((type == TEXGZ_UNSIGNED_BYTE) &&
	        (format == TEXGZ_LUMINANCE_ALPHA))
		tex = texgz_tex_8888toLA(tmp);
	else if((type == TEXGZ_FLOAT) &&
	        (format == TEXGZ_LUMINANCE))
		tex = texgz_tex_8888toF(tmp);
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

int texgz_tex_flipvertical(texgz_tex_t* self)
{
	ASSERT(self);

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
	ASSERT(self);

	texgz_tex_t* tex;
	tex = texgz_tex_new(self->width, self->height,
	                    self->stride, self->vstride,
	                    self->type, self->format, NULL);
	if(tex == NULL)
		return NULL;

	// vertical flip
	int y;
	int bpp     = texgz_tex_bpp(self);
	int stride  = self->stride;
	int vstride = self->vstride;
	for(y = 0; y < vstride; ++y)
	{
		unsigned char* src;
		unsigned char* dst;
		src = &self->pixels[y*bpp*stride];
		dst = &tex->pixels[(vstride - 1 - y)*bpp*stride];
		memcpy(dst, src, bpp*stride);
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
	if(((self->format == TEXGZ_ALPHA) ||
	    (self->format == TEXGZ_LUMINANCE)) &&
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

void texgz_tex_sample(texgz_tex_t* self, float u, float v,
                      int bpp, unsigned char* pixel)
{
	ASSERT(self);
	ASSERT(pixel);
	ASSERT(self->type == TEXGZ_UNSIGNED_BYTE);

	// skip expensive ASSERT
	// ASSERT(bpp == texgz_tex_bpp(self));

	// "float indices"
	float pu = u*(self->width  - 1);
	float pv = v*(self->height - 1);

	// determine indices to sample
	int u0 = (int) pu;
	int v0 = (int) pv;
	int u1 = u0 + 1;
	int v1 = v0 + 1;

	// double check the indices
	if(u0 < 0)
	{
		u0 = 0;
	}
	if(u1 < 0)
	{
		u1 = 0;
	}
	if(u0 >= self->width)
	{
		u0 = self->width - 1;
	}
	if(u1 >= self->width)
	{
		u1 = self->width - 1;
	}
	if(v0 < 0)
	{
		v0 = 0;
	}
	if(v1 < 0)
	{
		v1 = 0;
	}
	if(v0 >= self->height)
	{
		v0 = self->height - 1;
	}
	if(v1 >= self->height)
	{
		v1 = self->height - 1;
	}

	// compute interpolation coordinates
	float u0f = (float) u0;
	float v0f = (float) v0;
	float uf  = pu - u0f;
	float vf  = pv - v0f;

	// sample interpolation values
	int i;
	unsigned char* pixels   = self->pixels;
	int            offset00 = bpp*(v0*self->stride + u0);
	int            offset01 = bpp*(v0*self->stride + u1);
	int            offset10 = bpp*(v1*self->stride + u0);
	int            offset11 = bpp*(v1*self->stride + u1);
	for(i = 0; i < bpp; ++i)
	{
		// convert component to float
		float f00 = (float) pixels[offset00 + i];
		float f01 = (float) pixels[offset01 + i];
		float f10 = (float) pixels[offset10 + i];
		float f11 = (float) pixels[offset11 + i];

		// interpolate u
		float f0010 = f00 + uf*(f10 - f00);
		float f0111 = f01 + uf*(f11 - f01);

		// interpolate v
		pixel[i] = (unsigned char)
		           (f0010 + vf*(f0111 - f0010) + 0.5f);
	}
}

void texgz_tex_getPixel(texgz_tex_t* self,
                        int x, int y,
                        unsigned char* pixel)
{
	ASSERT(self);
	ASSERT(pixel);
	ASSERT(self->type == TEXGZ_UNSIGNED_BYTE);
	ASSERT((self->format == TEXGZ_RGB) ||
	       (self->format == TEXGZ_RGBA));

	int idx;
	if(self->format == TEXGZ_RGB)
	{
		idx      = 3*(y*self->stride + x);
		pixel[0] = self->pixels[idx];
		pixel[1] = self->pixels[idx + 1];
		pixel[2] = self->pixels[idx + 2];
		pixel[3] = 0xFF;
	}
	else
	{
		idx      = 4*(y*self->stride + x);
		pixel[0] = self->pixels[idx];
		pixel[1] = self->pixels[idx + 1];
		pixel[2] = self->pixels[idx + 2];
		pixel[3] = self->pixels[idx + 3];
	}
}

void texgz_tex_setPixel(texgz_tex_t* self,
                        int x, int y,
                        unsigned char* pixel)
{
	ASSERT(self);
	ASSERT(pixel);
	ASSERT(self->type == TEXGZ_UNSIGNED_BYTE);
	ASSERT((self->format == TEXGZ_RGB) ||
	       (self->format == TEXGZ_RGBA));

	int idx;
	if(self->format == TEXGZ_RGB)
	{
		idx = 3*(y*self->stride + x);
		self->pixels[idx]     = pixel[0];
		self->pixels[idx + 1] = pixel[1];
		self->pixels[idx + 2] = pixel[2];
	}
	else
	{
		idx = 4*(y*self->stride + x);
		self->pixels[idx]     = pixel[0];
		self->pixels[idx + 1] = pixel[1];
		self->pixels[idx + 2] = pixel[2];
		self->pixels[idx + 3] = pixel[3];
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

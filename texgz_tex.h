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

#ifndef texgz_tex_H
#define texgz_tex_H

#include <stdio.h>

// used to determine the file type and endianess
#define TEXGZ_MAGIC 0x000B00D9

// OpenGL ES type
#define TEXGZ_UNSIGNED_SHORT_4_4_4_4 0x8033
#define TEXGZ_UNSIGNED_SHORT_5_5_5_1 0x8034
#define TEXGZ_UNSIGNED_SHORT_5_6_5   0x8363
#define TEXGZ_UNSIGNED_BYTE          0x1401
#define TEXGZ_SHORT                  0x1402
#define TEXGZ_FLOAT                  0x1406

// OpenGL ES format
#define TEXGZ_RGB             0x1907
#define TEXGZ_RGBA            0x1908
#define TEXGZ_BGRA            0x80E1
#define TEXGZ_ALPHA           0x1906
#define TEXGZ_LUMINANCE       0x1909
#define TEXGZ_LUMINANCE_ALPHA 0x190A

typedef struct
{
	int width;
	int height;
	int stride;
	int vstride;
	int type;
	int format;
	unsigned char* pixels;
} texgz_tex_t;

texgz_tex_t* texgz_tex_new(int width, int height,
                           int stride, int vstride,
                           int type, int format,
                           unsigned char* pixels);
void         texgz_tex_delete(texgz_tex_t** _self);
texgz_tex_t* texgz_tex_copy(texgz_tex_t* self);
texgz_tex_t* texgz_tex_downscale(texgz_tex_t* self);
texgz_tex_t* texgz_tex_import(const char* filename);
texgz_tex_t* texgz_tex_importz(const char* filename);
texgz_tex_t* texgz_tex_importf(FILE* f, int size);
int          texgz_tex_export(texgz_tex_t* self, const char* filename);
int          texgz_tex_exportz(texgz_tex_t* self, const char* filename);
int          texgz_tex_exportf(texgz_tex_t* self, FILE* f);
int          texgz_tex_convert(texgz_tex_t* self, int type, int format);
texgz_tex_t* texgz_tex_convertcopy(texgz_tex_t* self, int type, int format);
int          texgz_tex_flipvertical(texgz_tex_t* self);
texgz_tex_t* texgz_tex_flipverticalcopy(texgz_tex_t* self);
int          texgz_tex_crop(texgz_tex_t* self, int top, int left, int bottom, int right);
texgz_tex_t* texgz_tex_cropcopy(texgz_tex_t* self, int top, int left, int bottom, int right);
int          texgz_tex_pad(texgz_tex_t* self);
texgz_tex_t* texgz_tex_padcopy(texgz_tex_t* self);
texgz_tex_t* texgz_tex_outline(texgz_tex_t* self, int size);
int          texgz_tex_blit(texgz_tex_t* src, texgz_tex_t* dst,
                            int width, int height,
                            int xs, int ys, int xd, int yd);
void         texgz_tex_sample(texgz_tex_t* self,
                              float u, float v,
                              int bpp, unsigned char* pixel);
int          texgz_tex_mipmap(texgz_tex_t* self,
                              int miplevels,
                              texgz_tex_t** mipmaps);
int          texgz_tex_bpp(texgz_tex_t* self);
int          texgz_tex_size(texgz_tex_t* self);

#endif

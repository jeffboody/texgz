/*
 * Copyright (c) 2014 Jeff Boody
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

#ifndef texgz_mtex_H
#define texgz_mtex_H

#include "texgz_tex.h"

typedef struct texgz_mtex_s
{
	unsigned char dx;
	unsigned char dy;
	texgz_tex_t*  tex;
	struct texgz_mtex_s* next;
} texgz_mtex_t;

texgz_mtex_t* texgz_mtex_new(unsigned char dx,
                             unsigned char dy,
                             texgz_tex_t* tex);
texgz_mtex_t* texgz_mtex_import(const char* fname);
texgz_tex_t*  texgz_mtex_importxy(const char* fname,
                                  unsigned char dx,
                                  unsigned char dy);
void          texgz_mtex_delete(texgz_mtex_t** _self);
int           texgz_mtex_join(texgz_mtex_t* self,
                              unsigned char dx,
                              unsigned char dy,
                              texgz_tex_t* tex);
int           texgz_mtex_export(texgz_mtex_t* self,
                                const char* fname);

#endif

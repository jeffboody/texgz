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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "texgz_mtex.h"

#define LOG_TAG "texgz"
#include "texgz_log.h"

/***********************************************************
* private                                                  *
***********************************************************/

// start offset
#define TEXGZ_MTEX_OFFSET (2 + 6*64)

// offset is bytes to end of texture
// size is the difference between start/end offset
typedef struct
{
	unsigned char dx;
	unsigned char dy;
	unsigned int  offset;
} texgz_mtex_node_t;

typedef struct
{
	unsigned short    count;
	texgz_mtex_node_t node[64];
} texgz_mtex_header_t;

static int texgz_mtex_readuint(FILE* f, unsigned int* data)
{
	assert(f);
	assert(data);

	if(fread((void*) data, sizeof(unsigned int), 1, f) != 1)
	{
		LOGE("fread failed");
		return 0;
	}

	return 1;
}

static int texgz_mtex_readushort(FILE* f, unsigned short* data)
{
	assert(f);
	assert(data);

	if(fread((void*) data, sizeof(unsigned short), 1, f) != 1)
	{
		LOGE("fread failed");
		return 0;
	}

	return 1;
}

static int texgz_mtex_readuchar(FILE* f, unsigned char* data)
{
	assert(f);
	assert(data);

	if(fread((void*) data, sizeof(unsigned char), 1, f) != 1)
	{
		LOGE("fread failed");
		return 0;
	}

	return 1;
}

static unsigned short texgz_mtex_count(texgz_mtex_t* self)
{
	assert(self);
	LOGD("debug");

	unsigned short count = 0;
	while(self)
	{
		++count;
		self = self->next;
	}

	return count;
}

static FILE* texgz_mtex_readh(const char* fname, texgz_mtex_header_t* header)
{
	assert(fname);
	assert(header);
	LOGD("debug fname=%s", fname);

	FILE* f = fopen(fname, "rb");
	if(f == NULL)
	{
		LOGE("failed fname=%s", fname);
		return NULL;
	}

	// read the count
	if((texgz_mtex_readushort(f, &header->count) == 0) ||
	   (header->count > 64))
	{
		goto fail_count;
	}

	// read the header
	int i;
	unsigned char dx     = 0;
	unsigned char dy     = 0;
	unsigned int  offset = 0;
	for(i = 0; i < header->count; ++i)
	{
		int success = 1;
		success &= texgz_mtex_readuchar(f, &dx);
		success &= texgz_mtex_readuchar(f, &dy);
		success &= texgz_mtex_readuint (f, &offset);

		if((success == 0) || (dx >= 8) || (dy >= 8))
		{
			LOGE("texgz_mtex_read failed");
			goto fail_header;
		}

		header->node[i].dx     = dx;
		header->node[i].dy     = dy;
		header->node[i].offset = offset;
	}

	// success
	return f;

	// failure
	fail_header:
	fail_count:
		fclose(f);
	return NULL;
}

static int texgz_mtex_writeh(FILE* f, texgz_mtex_header_t* header)
{
	assert(f);
	assert(header);
	LOGD("debug");

	// write the count
	if(fwrite(&header->count, sizeof(unsigned short), 1, f) != 1)
	{
		LOGE("fwrite failed");
		return 0;
	}

	// write the header
	// dx, dy, offset
	int i;
	for(i = 0; i < 64; ++i)
	{
		if(fwrite(&header->node[i].dx, sizeof(unsigned char), 1, f) != 1)
		{
			LOGE("fwrite failed");
			return 0;
		}

		if(fwrite(&header->node[i].dy, sizeof(unsigned char), 1, f) != 1)
		{
			LOGE("fwrite failed");
			return 0;
		}

		if(fwrite(&header->node[i].offset, sizeof(unsigned int), 1, f) != 1)
		{
			LOGE("fwrite failed");
			return 0;
		}
	}

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

texgz_mtex_t* texgz_mtex_new(unsigned char dx,
                             unsigned char dy,
                             texgz_tex_t* tex)
{
	assert(tex);
	LOGD("debug dx=%i, dy=%i", (int) dx, (int) dy);

	if((dx >= 8) || (dy >= 8))
	{
		LOGE("invalid %i, %i", (int) dx, (int) dy);
		return 0;
	}

	texgz_mtex_t* self = (texgz_mtex_t*) malloc(sizeof(texgz_mtex_t));
	if(self == NULL)
	{
		LOGE("malloc failed");
		return NULL;
	}

	self->dx   = dx;
	self->dy   = dy;
	self->tex  = tex;
	self->next = NULL;
	return self;
}

texgz_mtex_t* texgz_mtex_import(const char* fname)
{
	assert(fname);
	LOGD("debug fname=%s", fname);

	texgz_mtex_header_t header;
	FILE* f = texgz_mtex_readh(fname, &header);
	if(f == NULL)
	{
		return NULL;
	}

	// read images
	int i;
	texgz_mtex_t* self  = NULL;
	texgz_tex_t*  tex   = NULL;
	unsigned int  start = TEXGZ_MTEX_OFFSET;
	for(i = 0; i < header.count; ++i)
	{
		unsigned char dx  = header.node[i].dx;
		unsigned char dy  = header.node[i].dy;
		unsigned int  end = header.node[i].offset;

		if(end <= start)
		{
			LOGE("size failed end=%u, start=%u", end, start);
			goto fail_size;
		}

		// import the tile
		if(fseek(f, (long) start, SEEK_SET) == -1)
		{
			LOGE("fseek failed");
			goto fail_fseek;
		}

		tex = texgz_tex_importf(f, end - start);
		if(tex == NULL)
		{
			goto fail_import;
		}

		if(self)
		{
			if(texgz_mtex_join(self, dx, dy, tex) == 0)
			{
				goto fail_join;
			}
		}
		else
		{
			self = texgz_mtex_new(dx, dy, tex);
			if(self == NULL)
			{
				goto fail_new;
			}
		}

		start = end;
	}

	fclose(f);

	// success
	return self;

	// failure
	fail_new:
	fail_join:
		texgz_tex_delete(&tex);
	fail_import:
	fail_fseek:
	fail_size:
		texgz_mtex_delete(&self);
		fclose(f);
	return NULL;
}

texgz_tex_t* texgz_mtex_importxy(const char* fname,
                                 unsigned char dx,
                                 unsigned char dy)
{
	assert(fname);
	LOGD("debug fname=%s, dx=%i, dy=%i", fname, dx, dy);

	texgz_mtex_header_t header;
	FILE* f = texgz_mtex_readh(fname, &header);
	if(f == NULL)
	{
		return NULL;
	}

	// find texture
	int i;
	unsigned int start = TEXGZ_MTEX_OFFSET;
	unsigned int end   = 0;
	for(i = 0; i < header.count; ++i)
	{
		if((dx == header.node[i].dx) &&
		   (dy == header.node[i].dy))
		{
			if(i > 0)
			{
				start = header.node[i - 1].offset;
			}
			end = header.node[i].offset;
			break;
		}
	}

	if(end <= start)
	{
		LOGE("failed %s, dx=%i, dy=%i, end=%u, start=%u",
		     fname, (int) dx, (int) dy, end, start);
		goto fail_find;
	}

	// import the tile
	if(fseek(f, (long) start, SEEK_SET) == -1)
	{
		LOGE("fseek failed");
		goto fail_fseek;
	}

	texgz_tex_t* self = texgz_tex_importf(f, end - start);
	if(self == NULL)
	{
		goto fail_import;
	}

	fclose(f);

	// success
	return self;

	// failure
	fail_import:
	fail_fseek:
	fail_find:
		fclose(f);
	return NULL;
}

void texgz_mtex_delete(texgz_mtex_t** _self)
{
	assert(_self);

	texgz_mtex_t* self = *_self;
	if(self)
	{
		LOGD("debug");

		texgz_mtex_delete(&self->next);
		texgz_tex_delete(&self->tex);
		free(self);
		*_self = NULL;
	}
}

int texgz_mtex_join(texgz_mtex_t* self,
                    unsigned char dx,
                    unsigned char dy,
                    texgz_tex_t* tex)
{
	assert(self);
	assert(tex);
	LOGD("debug dx=%i, dy=%i", (int) dx, (int) dy);

	if((dx >= 8) || (dy >= 8))
	{
		LOGE("invalid %i, %i", (int) dx, (int) dy);
		return 0;
	}

	texgz_mtex_t* prev = self;
	while(self)
	{
		if((self->dx == dx) && (self->dy == dy))
		{
			LOGE("%i, %i already exists");
			return 0;
		}

		prev = self;
		self = self->next;
	}

	texgz_mtex_t* mtex = texgz_mtex_new(dx, dy, tex);
	if(mtex == NULL)
	{
		LOGE("malloc failed");
		return 0;
	}
	prev->next = mtex;

	return 1;
}

int texgz_mtex_export(texgz_mtex_t* self,
                      const char* fname)
{
	assert(self);
	assert(fname);
	LOGD("debug fname=%s", fname);

	FILE* f = fopen(fname, "wb");
	if(f == NULL)
	{
		LOGE("failed fname=%s", fname);
		return 0;
	}

	// init header
	texgz_mtex_header_t header;
	memset(&header, 0, sizeof(texgz_mtex_header_t));
	header.count = texgz_mtex_count(self);
	if(texgz_mtex_writeh(f, &header) == 0)
	{
		goto fail_init;
	}

	// write images
	int index = 0;
	texgz_mtex_t* mtex = self;
	while(mtex)
	{
		if(texgz_tex_exportf(mtex->tex, f) == 0)
		{
			goto fail_export;
		}

		header.node[index].dx     = mtex->dx;
		header.node[index].dy     = mtex->dy;
		header.node[index].offset = (unsigned int) ftell(f);
		++index;
		mtex = mtex->next;
	}

	// write header
	rewind(f);
	if(texgz_mtex_writeh(f, &header) == 0)
	{
		goto fail_header;
	}

	fclose(f);
	return 1;

	// failure
	fail_header:
	fail_export:
	fail_init:
		fclose(f);
	return 0;
}

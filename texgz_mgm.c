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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "texgz_mgm.h"
#include "texgz_jpeg.h"

#define LOG_TAG "texgz"
#include "texgz_log.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int texgz_mgm_readuint(FILE* f, unsigned int* data)
{
	assert(f);
	assert(data);

	unsigned char buffer[4];
	size_t bytes_read = fread((void*) buffer, sizeof(unsigned char), 4, f);
	if(bytes_read != 4)
	{
		LOGE("fread failed");
		return 0;
	}

	// swap order for big endian
	unsigned int b0 = (unsigned int) buffer[3];
	unsigned int b1 = (unsigned int) buffer[2];
	unsigned int b2 = (unsigned int) buffer[1];
	unsigned int b3 = (unsigned int) buffer[0];
	*data = (b3 << 24) & 0xFF000000;
	*data |= ((b2 << 16) & 0x00FF0000);
	*data |= ((b1 << 8) & 0x0000FF00);
	*data |= (b0 & 0x000000FF);
	return 1;
}

static int texgz_mgm_readushort(FILE* f, unsigned short* data)
{
	assert(f);
	assert(data);

	unsigned char buffer[2];
	size_t bytes_read = fread((void*) buffer, sizeof(unsigned char), 2, f);
	if(bytes_read != 2)
	{
		LOGE("fread failed");
		return 0;
	}

	// swap order for big endian
	unsigned short b0 = (unsigned short) buffer[1];
	unsigned short b1 = (unsigned short) buffer[0];
	*data  = ((b1 << 8) & 0xFF00);
	*data |= (b0 & 0x00FF);
	return 1;
}

static int texgz_mgm_readuchar(FILE* f, unsigned char* data)
{
	assert(f);
	assert(data);

	size_t bytes_read = fread((void*) data, sizeof(unsigned char), 1, f);
	if(bytes_read != 1)
	{
		LOGE("fread failed");
		return 0;
	}

	return 1;
}

/***********************************************************
* public texgz_mgm_t                                       *
***********************************************************/

texgz_tex_t* texgz_mgm_import(const char* fname,
                              unsigned char dx,
                              unsigned char dy)
{
	assert(fname);
	LOGD("debug fname=%s, dx=%i, dy=%i", fname, dx, dy);

	FILE* f = fopen(fname, "r");
	if(f == NULL)
	{
		LOGE("fopen %s failed", fname);
		return NULL;
	}

	// read the count
	unsigned short count;
	if(texgz_mgm_readushort(f, &count) == 0)
	{
		goto fail_count;
	}

	// read the header
	// dx, dy, offset
	int i;
	unsigned char tmp_dx     = 0;
	unsigned char tmp_dy     = 0;
	unsigned int  tmp_offset = 0;
	unsigned int  offset     = 2 + 6*64;
	int           found      = 0;
	for(i = 0; i < count; ++i)
	{
		int success = 1;
		success &= texgz_mgm_readuchar(f, &tmp_dx);
		success &= texgz_mgm_readuchar(f, &tmp_dy);
		success &= texgz_mgm_readuint (f, &tmp_offset);

		if(success == 0)
		{
			LOGE("texgz_mgm_read failed");
			goto fail_header;
		}

		if((dx == tmp_dx) && (dy == tmp_dy))
		{
			found = 1;
			break;
		}

		offset = tmp_offset;
	}

	if(found == 0)
	{
		LOGE("failed to find %u,%u in %s",
		     (unsigned int) dx, (unsigned int) dy, fname);
		goto fail_find;
	}

	// import the tile
	if(fseek(f, (long) offset, SEEK_SET) == -1)
	{
		LOGE("fseek failed");
		goto fail_fseek;
	}

	texgz_tex_t* self = texgz_jpeg_importf(f);
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
	fail_header:
	fail_count:
		fclose(f);
	return NULL;
}

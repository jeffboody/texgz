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
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "texgz/texgz_tex.h"
#include "texgz/texgz_png.h"
#include "libpak/pak_file.h"

#define LOG_TAG "osm2pak"
#include "texgz/texgz_log.h"

/***********************************************************
* public                                                   *
***********************************************************/

int main(int argc, char** argv)
{
	// osm.list
	// zoom x y
	if(argc != 2)
	{
		LOGE("usage: %s [osm.list]", argv[0]);
		return EXIT_FAILURE;
	}

	// open the list
	FILE* f = fopen(argv[1], "r");
	if(f == NULL)
	{
		LOGE("failed to open %s", argv[1]);
		return EXIT_FAILURE;
	}

	// iteratively pak osm images
	char* line = NULL;
	size_t n   = 0;
	int index = 0;
	while(getline(&line, &n, f) > 0)
	{
		int x;
		int y;
		int zoom;
		if(sscanf(line, "%i %i %i", &zoom, &x, &y) != 3)
		{
			LOGE("invalid line=%s", line);
			continue;
		}

		LOGI("%i: zoom=%i, x=%i, y=%i", index++, zoom, x, y);

		// create directories if necessary
		char dname[256];
		snprintf(dname, 256, "osm/%i", zoom);
		if(mkdir(dname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
		{
			if(errno == EEXIST)
			{
				// already exists
			}
			else
			{
				LOGE("mkdir %s failed", dname);
				continue;
			}
		}

		char fname[256];
		snprintf(fname, 256, "osm/%i/%i_%i.pak", zoom, x, y);
		pak_file_t* pak = pak_file_open(fname, PAK_FLAG_WRITE);
		if(pak == NULL)
		{
			continue;
		}

		int i;
		int j;
		for(i = 0; i < 8; ++i)
		{
			for(j = 0; j < 8; ++j)
			{
				int xj = 8*x + j;
				int yi = 8*y + i;

				snprintf(fname, 256, "localhost/osm/%i/%i/%i.png", zoom, xj, yi);
				texgz_tex_t* tex = texgz_png_import(fname);
				if(tex == NULL)
				{
					continue;
				}

				if(texgz_tex_convert(tex,
				                     TEXGZ_UNSIGNED_SHORT_4_4_4_4,
				                     TEXGZ_RGBA) == 0)
				{
					texgz_tex_delete(&tex);
					continue;
				}

				// j=dx, i=dy
				snprintf(fname, 256, "%i_%i", j, i);
				pak_file_writek(pak, fname);
				texgz_tex_exportf(tex, pak->f);
				texgz_tex_delete(&tex);
			}
		}

		pak_file_close(&pak);
	}
	free(line);

	return EXIT_SUCCESS;
}

about
=====

This package includes code to import/export the texgz image format
into Gimp (via a plugin) or other apps. The texgz format is specifically
designed to work well with OpenGL ES and uses zlib for lossless
compression. The following formats are supported by texgz:

* RGBA-8888
* RGB-888
* BGRA-8888
* RGB-565
* RGBA-4444
* RGBA-5551
* Luminance
* Luminance-Float

The texz format may be uncompressed in memory while the
texgz format may be uncompressed from a file. The texz
format is useful when embedding the image in an archive or
when the image does not need to be saved to disk. There is
a slight difference in texz vs texgz data because libz
compresses data in memory differently from when written to
a file.

Send questions or comments to Jeff Boody - jeffboody@gmail.com

additional file types
=====================

For convenience the texgz library can also import/export these file types.

* jpg
* png

texgz-convert
=============

A conversion utility that also serves as an example for using the
texgz library.

example using texgz as a texture in OpenGL ES
=============================================

	// width/height may not equal stride/vstride for power-of-two textures
	// and you may need to adjust texture coordinates accordingly
	texgz_tex_t* tex = texgz_tex_import("sample.texgz");
	glTexImage2D(GL_TEXTURE_2D, 0, tex->format,
	             tex->stride, tex->vstride,
	             0, tex->format, tex->type,
	             tex->pixels);
	texgz_tex_delete(tex);

example using texgz to save a screenshot in OpenGL ES
=====================================================

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &format);
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &type);
	texgz_tex_t* tex = texgz_tex_new(width, height,
	                                 width, height,
	                                 type, format,
	                                 NULL);
	glReadPixels(0, 0, width, height, format, type, (void*) tex->pixels);
	texgz_tex_flipvertical(tex);
	texgz_tex_export(tex, "sample.texgz");
	texgz_tex_delete(&tex);

resources
=========

	http://developer.gimp.org/api/2.0/libgimp/libgimp.html

	# Gimp documentation is difficult to search on the website but you can
	# enter the following search string in Google as a workaround
	site:http://developer.gimp.org <function>

	# texgz plugin was based loosely on the pnm plugin
	http://gimp.sourcearchive.com/documentation/2.6.5/file-pnm_8c-source.html

install the Gimp plugin
=======================

	# requires libgimp2.0-dev
	sudo apt-get install libgimp2.0-dev

	./build-plugin.sh

license
=======

	Copyright (c) 2011 Jeff Boody

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.

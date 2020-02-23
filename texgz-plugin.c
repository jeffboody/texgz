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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <string.h>

#include "libcc/cc_log.c"
#include "texgz_tex.c"

static int texgz_isTexz(const gchar *filename)
{
	ASSERT(filename);

	if(strstr(filename, ".texz"))
	{
		return 1;
	}

	return 0;
}

static gint32 texgz_import(const gchar *filename)
{
	ASSERT(filename);

	int is_texz = texgz_isTexz(filename);

	texgz_tex_t* tex;
	if(is_texz)
	{
		tex = texgz_tex_importz(filename);
	}
	else
	{
		tex = texgz_tex_import(filename);
	}
	if(tex == NULL)
		return -1;

	if(texgz_tex_convert(tex, TEXGZ_UNSIGNED_BYTE,
	                     TEXGZ_RGBA) == 0)
		goto fail_convert;

	if(texgz_tex_crop(tex, 0, 0, tex->height - 1,
	                  tex->width - 1) == 0)
		goto fail_crop;

	gint32 image_ID = gimp_image_new(tex->stride,
	                                 tex->vstride, GIMP_RGB);
	if(image_ID == -1)
	{
		LOGE("gimp_image_new failed");
		goto fail_gimp_image_new;
	}

	if(gimp_image_set_filename(image_ID, filename) == FALSE)
	{
		LOGE("gimp_image_set_filename failed");
		goto fail_gimp_image_set_filename;
	}

	gint32 layer_ID;
	if(is_texz)
	{
		layer_ID = gimp_layer_new(image_ID, "texz",
		                          tex->stride, tex->vstride,
		                          GIMP_RGBA_IMAGE, 100,
		                          GIMP_NORMAL_MODE);
	}
	else
	{
		layer_ID = gimp_layer_new(image_ID, "texgz",
		                          tex->stride, tex->vstride,
		                          GIMP_RGBA_IMAGE, 100,
		                          GIMP_NORMAL_MODE);
	}
	if(layer_ID == -1)
	{
		LOGE("gimp_layer_new failed");
		goto fail_gimp_layer_new;
	}

	if(gimp_image_add_layer(image_ID, layer_ID, -1) == FALSE)
	{
		LOGE("gimp_image_add_layer failed");
		goto fail_gimp_image_add_layer;
	}

	if(gimp_layer_set_offsets(layer_ID, 0, 0) == FALSE)
	{
		LOGE("gimp_layer_set_offsets failed");
		goto fail_gimp_layer_set_offsets;
	}

	if(gimp_layer_set_lock_alpha(layer_ID, FALSE) == FALSE)
	{
		LOGE("gimp_layer_set_lock_alpha failed");
		goto fail_gimp_layer_set_lock_alpha;
	}

	GimpDrawable* drawable = gimp_drawable_get(layer_ID);
	if(drawable == NULL)
	{
		LOGE("gimp_drawable_get failed");
		goto fail_gimp_drawable_get;
	}

	GimpPixelRgn  pixel_rgn;
	gimp_pixel_rgn_init(&pixel_rgn, drawable,
	                    0, 0, drawable->width, drawable->height,
	                    FALSE, FALSE);
	gimp_pixel_rgn_set_rect(&pixel_rgn, tex->pixels,
	                        0, 0, tex->stride, tex->vstride);
	gimp_drawable_flush(drawable);
	texgz_tex_delete(&tex);

	// success
	return image_ID;

	// failure
	fail_gimp_drawable_get:
	fail_gimp_layer_set_lock_alpha:
	fail_gimp_layer_set_offsets:
	fail_gimp_image_add_layer:
		// gimp_layer_delete does not exist
	fail_gimp_layer_new:
	fail_gimp_image_set_filename:
		gimp_image_delete(image_ID);
	fail_gimp_image_new:
	fail_crop:
	fail_convert:
		texgz_tex_delete(&tex);
	return -1;
}

typedef struct
{
	gint format;
	gint pad;
} texgz_dialog_t;

static gint
texgz_export_dialog(texgz_dialog_t* self, int is_texz)
{
	ASSERT(self);

	GtkWidget* dialog;
	if(is_texz)
	{
		dialog = gimp_dialog_new("Save as texz",
		                         "texgz-export",
		                         NULL, 0,
		                         NULL,
		                         NULL,
		                         GTK_STOCK_CANCEL,
		                         GTK_RESPONSE_CANCEL,
		                         GTK_STOCK_SAVE,
		                         GTK_RESPONSE_OK,
		                         NULL);
	}
	else
	{
		dialog = gimp_dialog_new("Save as texgz",
		                         "texgz-export",
		                         NULL, 0,
		                         NULL,
		                         NULL,
		                         GTK_STOCK_CANCEL,
		                         GTK_RESPONSE_CANCEL,
		                         GTK_STOCK_SAVE,
		                         GTK_RESPONSE_OK,
		                         NULL);
	}
	if(dialog == NULL)
	{
		LOGE("gimp_dialog_new failed");
		return 0;
	}

	gimp_window_set_transient(GTK_WINDOW(dialog));

	GtkWidget* radio_format;
	radio_format = gimp_int_radio_group_new(TRUE,
	                                        "Format",
	                                        G_CALLBACK(gimp_radio_button_update),
	                                        &self->format,
	                                        8888,
	                                        "RGBA-8888",   8888, NULL,
	                                        "BGRA-8888",   8889, NULL,
	                                        "RGB-565",     565,  NULL,
	                                        "RGBA-4444",   4444, NULL,
	                                        "RGB-888",     888,  NULL,
	                                        "RGBA-5551",   5551, NULL,
	                                        "LUMINANCE",   8,    NULL,
	                                        "ALPHA",       9,    NULL,
	                                        "LUMINANCE-A", 16,   NULL,
	                                        "LUMINANCE-F", 0xF,  NULL,
	                                        NULL);
	if(radio_format == NULL)
	{
		LOGE("gimp_int_radio_group_new failed");
		goto fail_radio_format;
	}

	GtkWidget* radio_pad;
	radio_pad = gimp_int_radio_group_new(TRUE,
	                                     "Pad for power-of-two?",
	                                     G_CALLBACK(gimp_radio_button_update),
	                                     &self->pad,
	                                     0,
	                                     "No",  0, NULL,
	                                     "Yes", 1, NULL,
	                                     NULL);
	if(radio_pad == NULL)
	{
		LOGE("gimp_int_radio_group_new failed");
		goto fail_radio_pad;
	}

	gtk_container_set_border_width(GTK_CONTAINER(radio_format), 6);
	gtk_container_set_border_width(GTK_CONTAINER(radio_pad), 6);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), radio_format, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), radio_pad, FALSE, TRUE, 0);
	gtk_widget_show(radio_format);
	gtk_widget_show(radio_pad);
	gtk_widget_show(dialog);

	if(gimp_dialog_run(GIMP_DIALOG(dialog)) != GTK_RESPONSE_OK)
	{
		LOGE("gimp_dialog_run failed");
		goto fail_gimp_dialog_run;
	}

	gtk_widget_destroy(dialog);

	// success
	return 1;

	// failure
	fail_gimp_dialog_run:
	fail_radio_format:
	fail_radio_pad:
		gtk_widget_destroy(dialog);   // Does this delete children also?
	return 0;
}

static int texgz_export(const gchar*    filename,
                        gint32          image_ID,
                        gint32          drawable_ID,
                        texgz_dialog_t* dialog)
{
	ASSERT(filename);

	GimpImageType drawable_type;
	drawable_type = gimp_drawable_type(drawable_ID);
	if(drawable_type != GIMP_RGBA_IMAGE)
	{
		LOGE("invalid drawable type");
		return 0;
	}

	int type;
	int format;
	if(dialog->format == 8888)
	{
		type   = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_RGBA;
	}
	else if(dialog->format == 8889)
	{
		type   = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_BGRA;
	}
	else if(dialog->format == 565)
	{
		type   = TEXGZ_UNSIGNED_SHORT_5_6_5;
		format = TEXGZ_RGB;
	}
	else if(dialog->format == 4444)
	{
		type   = TEXGZ_UNSIGNED_SHORT_4_4_4_4;
		format = TEXGZ_RGBA;
	}
	else if(dialog->format == 888)
	{
		type   = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_RGB;
	}
	else if(dialog->format == 5551)
	{
		type   = TEXGZ_UNSIGNED_SHORT_5_5_5_1;
		format = TEXGZ_RGBA;
	}
	else if(dialog->format == 8)
	{
		type   = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_LUMINANCE;
	}
	else if(dialog->format == 9)
	{
		type   = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_ALPHA;
	}
	else if(dialog->format == 16)
	{
		type   = TEXGZ_UNSIGNED_BYTE;
		format = TEXGZ_LUMINANCE_ALPHA;
	}
	else if(dialog->format == 0xF)
	{
		type   = TEXGZ_FLOAT;
		format = TEXGZ_LUMINANCE;
	}
	else
	{
		LOGE("invalid format=%i", dialog->format);
		return 0;
	}

	GimpDrawable* drawable = gimp_drawable_get(drawable_ID);
	if(drawable == NULL)
	{
		LOGE("could not get drawable");
		return 0;
	}

	GimpPixelRgn pixel_rgn;
	gimp_pixel_rgn_init(&pixel_rgn, drawable,
	                    0, 0, drawable->width, drawable->height,
	                    FALSE, FALSE);

	texgz_tex_t* tex;
	tex = texgz_tex_new(drawable->width, drawable->height,
	                    drawable->width, drawable->height,
	                    TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA,
	                    NULL);
	if(tex == NULL)
		goto fail_texgz_tex_new;

	gimp_pixel_rgn_get_rect(&pixel_rgn, tex->pixels,
	                        0, 0, tex->stride, tex->vstride);

	if(dialog->pad)
	{
		if(texgz_tex_pad(tex) == 0)
			goto fail_texgz_tex_pad;
	}

	if(texgz_tex_convert(tex, type, format) == 0)
		goto fail_texgz_tex_convert;

	int is_texz = texgz_isTexz(filename);
	if(is_texz)
	{
		if(texgz_tex_exportz(tex, filename) == 0)
			goto fail_texgz_tex_export;
	}
	else
	{
		if(texgz_tex_export(tex, filename) == 0)
			goto fail_texgz_tex_export;
	}

	texgz_tex_delete(&tex);
	gimp_drawable_detach(drawable);

	// success
	return 1;

	// failure
	fail_texgz_tex_export:
	fail_texgz_tex_convert:
	fail_texgz_tex_pad:
		texgz_tex_delete(&tex);
	fail_texgz_tex_new:
		gimp_drawable_detach(drawable);
	return 0;
}

static void query(void)
{
	static const GimpParamDef s_params_load[] =
	{
		{ GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
		{ GIMP_PDB_STRING, "filename", "The name of the file to load" },
		{ GIMP_PDB_STRING, "raw_filename", "The name of the file to load" }
	};

	static const GimpParamDef s_params_save[] =
	{
		{ GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
		{ GIMP_PDB_IMAGE,    "image",        "Input image"                  },
		{ GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save"             },
		{ GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
		{ GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" },
		{ GIMP_PDB_INT32,    "raw",          "Specify non-zero for raw output, zero for ascii output" }
	};

	static GimpParamDef s_return_vals[] =
	{
    	{ GIMP_PDB_IMAGE, "image", "Output image" }
	};

	gimp_install_procedure("texz-import",
	                       "https://github.com/jeffboody/texgz",
	                       "Loads texz images",
	                       "Jeff Boody",
	                       "Copyright (c) 2011 Jeff Boody",
	                       "2011",
	                       "texz image",
	                       NULL,
	                       GIMP_PLUGIN,
	                       G_N_ELEMENTS(s_params_load),
	                       G_N_ELEMENTS(s_return_vals),
	                       s_params_load,
	                       s_return_vals);

	gimp_install_procedure("texgz-import",
	                       "https://github.com/jeffboody/texgz",
	                       "Loads texz/texgz images",
	                       "Jeff Boody",
	                       "Copyright (c) 2011 Jeff Boody",
	                       "2011",
	                       "texgz image",
	                       NULL,
	                       GIMP_PLUGIN,
	                       G_N_ELEMENTS(s_params_load),
	                       G_N_ELEMENTS(s_return_vals),
	                       s_params_load,
	                       s_return_vals);

	gimp_install_procedure("texz-export",
	                       "https://github.com/jeffboody/texgz",
	                       "Saves texz/texgz images",
	                       "Jeff Boody",
	                       "Copyright (c) 2011 Jeff Boody",
	                       "2011",
	                       "texz image",
	                       "RGBA",
	                       GIMP_PLUGIN,
	                       G_N_ELEMENTS(s_params_save),
	                       0,
	                       s_params_save,
	                       NULL);

	gimp_install_procedure("texgz-export",
	                       "https://github.com/jeffboody/texgz",
	                       "Saves texz/texgz images",
	                       "Jeff Boody",
	                       "Copyright (c) 2011 Jeff Boody",
	                       "2011",
	                       "texgz image",
	                       "RGBA",
	                       GIMP_PLUGIN,
	                       G_N_ELEMENTS(s_params_save),
	                       0,
	                       s_params_save,
	                       NULL);

	gimp_register_file_handler_mime("texz-import", "image/texz");
	gimp_register_file_handler_mime("texz-export", "image/texz");
	gimp_register_load_handler("texz-import", "texz", "");
	gimp_register_save_handler("texz-export", "texz", "");

	gimp_register_file_handler_mime("texgz-import", "image/texgz");
	gimp_register_file_handler_mime("texgz-export", "image/texgz");
	gimp_register_load_handler("texgz-import", "texgz", "");
	gimp_register_save_handler("texgz-export", "texgz", "");
}

static void
run(const gchar *name, gint nparams,
    const GimpParam* param, gint* nreturn_vals,
    GimpParam** return_vals)
{
	static GimpParam   s_return_vals[2];
	gint32             image_ID;
	gint32             drawable_ID;
	const gchar*       filename;
	GimpRunMode        run_mode = param[0].data.d_int32;

	ASSERT(name);
	ASSERT(param);
	ASSERT(nreturn_vals);
	ASSERT(return_vals);

	*nreturn_vals                  = 1;
	*return_vals                   = s_return_vals;
	s_return_vals[0].type          = GIMP_PDB_STATUS;
	s_return_vals[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

	if((strcmp(name, "texz-import") == 0) ||
	   (strcmp(name, "texgz-import") == 0))
	{
		image_ID = texgz_import(param[1].data.d_string);
		if(image_ID == -1)
		{
			LOGE("texgz_import failed");
			return;
		}

		*nreturn_vals = 2;
		s_return_vals[0].data.d_status = GIMP_PDB_SUCCESS;
		s_return_vals[1].type = GIMP_PDB_IMAGE;
		s_return_vals[1].data.d_image = image_ID;
	}
	else if((strcmp(name, "texz-export") == 0) ||
	        (strcmp(name, "texgz-export") == 0))
	{
		image_ID    = param[1].data.d_int32;
		drawable_ID = param[2].data.d_int32;
		filename    = param[3].data.d_string;

		int is_texz = texgz_isTexz(filename);
		if(run_mode == GIMP_RUN_INTERACTIVE)
		{
			if(is_texz)
			{
				gimp_ui_init("texz-export", FALSE);
				if(gimp_export_image(&image_ID, &drawable_ID, "texz",
				                     GIMP_EXPORT_CAN_HANDLE_RGB |
				                     GIMP_EXPORT_NEEDS_ALPHA
				                    ) == GIMP_EXPORT_CANCEL)
				{
					LOGE("gimp_export_image failed");
					s_return_vals[0].data.d_status = GIMP_PDB_CANCEL;
					return;
				}
			}
			else
			{
				gimp_ui_init("texgz-export", FALSE);
				if(gimp_export_image(&image_ID, &drawable_ID, "texgz",
				                     GIMP_EXPORT_CAN_HANDLE_RGB |
				                     GIMP_EXPORT_NEEDS_ALPHA
				                    ) == GIMP_EXPORT_CANCEL)
				{
					LOGE("gimp_export_image failed");
					s_return_vals[0].data.d_status = GIMP_PDB_CANCEL;
					return;
				}
			}

			texgz_dialog_t dialog =
			{
				.format = 8888,
				.pad    = 0,
			};
			if(texgz_export_dialog(&dialog, is_texz) == 0)
			{
				LOGE("texgz_export_dialog failed");
				s_return_vals[0].data.d_status = GIMP_PDB_CANCEL;
				gimp_image_delete(image_ID);
				return;
			}

			if(texgz_export(filename, image_ID, drawable_ID, &dialog) == 0)
			{
				LOGE("texgz_export failed");
				s_return_vals[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
				gimp_image_delete(image_ID);
				return;
			}
			s_return_vals[0].data.d_status = GIMP_PDB_SUCCESS;
		}
	}
	else
	{
		LOGE("invalid name=%s", name);
		s_return_vals[0].data.d_status = GIMP_PDB_CALLING_ERROR;
		return;
	}
}

GimpPlugInInfo PLUG_IN_INFO =
{
	NULL,
	NULL,
	query,
	run
};

MAIN()

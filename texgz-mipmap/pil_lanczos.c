/*
 * The Python Imaging Library (PIL) is
 *
 *     Copyright (c) 1997-2011 by Secret Labs AB
 *     Copyright (c) 1995-2011 by Fredrik Lundh
 *
 * Pillow is the friendly PIL fork. It is
 *
 *     Copyright (c) 2010-2023 by Jeffrey A. Clark (Alex) and contributors.
 *
 * Like PIL, Pillow is licensed under the open source HPND License:
 *
 * By obtaining, using, and/or copying this software and/or its associated
 * documentation, you agree that you have read, understood, and will comply
 * with the following terms and conditions:
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies, and that
 * both that copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Secret Labs AB or the author not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 *
 * SECRET LABS AB AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL SECRET LABS AB OR THE AUTHOR BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

// Ported from Pillow
// https://github.com/python-pillow/Pillow/blob/main/src/libImaging/Resample.c

#include <math.h>

#include "pil_lanczos.h"

/***********************************************************
* private                                                  *
***********************************************************/

static double
pil_sinc_filter(double x) {
    if (x == 0.0) {
        return 1.0;
    }
    x = x * M_PI;
    return sin(x) / x;
}

/***********************************************************
* public                                                   *
***********************************************************/

double
pil_lanczos3_filter(double x) {
    /* truncated sinc */
    if (-3.0 <= x && x < 3.0) {
        return pil_sinc_filter(x) * pil_sinc_filter(x / 3);
    }
    return 0.0;
}

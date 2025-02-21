/* raster_font.h - Font for raster images (NOTE: was "font.h") */
/*
    libzint - the open source barcode library
    Copyright (C) 2008-2023 Robin Stuart <rstuart114@gmail.com>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the project nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
 */
/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef Z_RASTER_FONT_H
#define Z_RASTER_FONT_H

typedef unsigned short raster_font_item;

#define NORMAL_FONT_WIDTH   7
#define NORMAL_FONT_HEIGHT  14

static const raster_font_item ascii_font[] = {
    /* Each character is 7 x 14 pixels */
    0, 0, 8, 8, 8, 8, 8, 8, 8, 0, 8, 8, 0, 0, /* ! */
    0, 20, 20, 20, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* " */
    0, 0, 20, 20, 20, 62, 20, 20, 62, 20, 20, 20, 0, 0, /* # */
    0, 0, 8, 60, 74, 74, 40, 28, 10, 74, 74, 60, 8, 0, /* $ */
    0, 0, 50, 74, 76, 56, 8, 16, 28, 50, 82, 76, 0, 0, /* % */
    0, 0, 24, 36, 36, 36, 24, 50, 74, 68, 76, 50, 0, 0, /* & */
    0, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ' */
    0, 2, 4, 8, 8, 16, 16, 16, 16, 16, 8, 8, 4, 2, /* ( */
    0, 32, 16, 8, 8, 4, 4, 4, 4, 4, 8, 8, 16, 32, /* ) */
    0, 0, 0, 0, 8, 42, 28, 8, 28, 42, 8, 0, 0, 0, /* * */
    0, 0, 0, 0, 8, 8, 8, 62, 8, 8, 8, 0, 0, 0, /* + */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 8, 8, 16, /* , */
    0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 0, 0, 0, /* - */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 28, 8, 0, /* . */
    0, 2, 2, 4, 4, 8, 8, 8, 16, 16, 32, 32, 64, 64, /* / */
    0, 0, 24, 36, 66, 66, 66, 66, 66, 66, 36, 24, 0, 0, /* 0 */
    0, 0, 8, 24, 40, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* 1 */
    0, 0, 60, 66, 66, 2, 4, 4, 8, 16, 32, 126, 0, 0, /* 2 */
    0, 0, 126, 2, 4, 8, 28, 2, 2, 66, 66, 60, 0, 0, /* 3 */
    0, 0, 4, 12, 20, 20, 36, 36, 68, 126, 4, 4, 0, 0, /* 4 */
    0, 0, 126, 64, 64, 124, 66, 2, 2, 66, 66, 60, 0, 0, /* 5 */
    0, 0, 28, 32, 64, 64, 92, 98, 66, 66, 66, 60, 0, 0, /* 6 */
    0, 0, 126, 2, 4, 4, 8, 8, 16, 16, 32, 32, 0, 0, /* 7 */
    0, 0, 60, 66, 66, 36, 24, 36, 66, 66, 66, 60, 0, 0, /* 8 */
    0, 0, 60, 66, 66, 66, 70, 58, 2, 66, 68, 56, 0, 0, /* 9 */
    0, 0, 0, 0, 8, 28, 8, 0, 0, 8, 28, 8, 0, 0, /* : */
    0, 0, 0, 0, 0, 24, 24, 0, 0, 24, 8, 8, 16, 0, /* ; */
    0, 0, 0, 2, 4, 8, 16, 32, 16, 8, 4, 2, 0, 0, /* < */
    0, 0, 0, 0, 0, 126, 0, 0, 126, 0, 0, 0, 0, 0, /* = */
    0, 0, 0, 32, 16, 8, 4, 2, 4, 8, 16, 32, 0, 0, /* > */
    0, 0, 60, 66, 66, 4, 8, 8, 8, 0, 8, 8, 0, 0, /* ? */
    0, 0, 28, 34, 78, 82, 82, 82, 82, 78, 32, 30, 0, 0, /* @ */
    0, 0, 24, 36, 66, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* A */
    0, 0, 120, 68, 66, 68, 120, 68, 66, 66, 68, 120, 0, 0, /* B */
    0, 0, 60, 66, 66, 64, 64, 64, 64, 66, 66, 60, 0, 0, /* C */
    0, 0, 120, 68, 66, 66, 66, 66, 66, 66, 68, 120, 0, 0, /* D */
    0, 0, 126, 64, 64, 64, 120, 64, 64, 64, 64, 126, 0, 0, /* E */
    0, 0, 126, 64, 64, 64, 120, 64, 64, 64, 64, 64, 0, 0, /* F */
    0, 0, 60, 66, 66, 64, 64, 78, 66, 66, 70, 58, 0, 0, /* G */
    0, 0, 66, 66, 66, 66, 126, 66, 66, 66, 66, 66, 0, 0, /* H */
    0, 0, 62, 8, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* I */
    0, 0, 14, 4, 4, 4, 4, 4, 4, 68, 68, 56, 0, 0, /* J */
    0, 0, 66, 68, 72, 80, 96, 80, 72, 68, 66, 66, 0, 0, /* K */
    0, 0, 64, 64, 64, 64, 64, 64, 64, 64, 64, 126, 0, 0, /* L */
    0, 0, 66, 102, 102, 90, 90, 66, 66, 66, 66, 66, 0, 0, /* M */
    0, 0, 66, 66, 98, 98, 82, 74, 70, 70, 66, 66, 0, 0, /* N */
    0, 0, 60, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* O */
    0, 0, 124, 66, 66, 66, 66, 124, 64, 64, 64, 64, 0, 0, /* P */
    0, 0, 60, 66, 66, 66, 66, 66, 114, 74, 70, 60, 4, 2, /* Q */
    0, 0, 124, 66, 66, 66, 66, 124, 72, 68, 66, 66, 0, 0, /* R */
    0, 0, 60, 66, 66, 64, 48, 12, 2, 66, 66, 60, 0, 0, /* S */
    0, 0, 127, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0, 0, /* T */
    0, 0, 66, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* U */
    0, 0, 66, 66, 66, 66, 36, 36, 36, 24, 24, 24, 0, 0, /* V */
    0, 0, 34, 34, 34, 34, 34, 34, 42, 42, 42, 20, 0, 0, /* W */
    0, 0, 66, 66, 36, 36, 24, 24, 36, 36, 66, 66, 0, 0, /* X */
    0, 0, 34, 34, 34, 20, 20, 8, 8, 8, 8, 8, 0, 0, /* Y */
    0, 0, 126, 2, 4, 8, 8, 16, 32, 32, 64, 126, 0, 0, /* Z */
    0, 30, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 30, /* [ */
    0, 64, 64, 32, 32, 16, 16, 16, 8, 8, 4, 4, 2, 2, /* \ */
    0, 60, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 60, /* ] */
    0, 24, 36, 66, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ^ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 126, /* _ */
    0, 16, 8, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ` */
    0, 0, 0, 0, 0, 60, 66, 2, 62, 66, 66, 62, 0, 0, /* a */
    0, 0, 64, 64, 64, 92, 98, 66, 66, 66, 98, 92, 0, 0, /* b */
    0, 0, 0, 0, 0, 60, 66, 64, 64, 64, 66, 60, 0, 0, /* c */
    0, 0, 2, 2, 2, 58, 70, 66, 66, 66, 70, 58, 0, 0, /* d */
    0, 0, 0, 0, 0, 60, 66, 66, 126, 64, 66, 60, 0, 0, /* e */
    0, 0, 12, 18, 16, 16, 124, 16, 16, 16, 16, 16, 0, 0, /* f */
    0, 0, 0, 0, 0, 58, 68, 68, 68, 56, 32, 92, 66, 60, /* g */
    0, 0, 64, 64, 64, 92, 98, 66, 66, 66, 66, 66, 0, 0, /* h */
    0, 0, 8, 8, 0, 24, 8, 8, 8, 8, 8, 62, 0, 0, /* i */
    0, 0, 2, 2, 0, 6, 2, 2, 2, 2, 2, 34, 34, 28, /* j */
    0, 0, 64, 64, 64, 68, 72, 80, 112, 72, 68, 66, 0, 0, /* k */
    0, 0, 24, 8, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* l */
    0, 0, 0, 0, 0, 52, 42, 42, 42, 42, 42, 34, 0, 0, /* m */
    0, 0, 0, 0, 0, 92, 98, 66, 66, 66, 66, 66, 0, 0, /* n */
    0, 0, 0, 0, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* o */
    0, 0, 0, 0, 0, 92, 98, 66, 66, 66, 98, 92, 64, 64, /* p */
    0, 0, 0, 0, 0, 58, 70, 66, 66, 66, 70, 58, 2, 2, /* q */
    0, 0, 0, 0, 0, 92, 98, 66, 64, 64, 64, 64, 0, 0, /* r */
    0, 0, 0, 0, 0, 60, 66, 32, 24, 4, 66, 60, 0, 0, /* s */
    0, 0, 16, 16, 16, 124, 16, 16, 16, 16, 18, 12, 0, 0, /* t */
    0, 0, 0, 0, 0, 66, 66, 66, 66, 66, 70, 58, 0, 0, /* u */
    0, 0, 0, 0, 0, 34, 34, 34, 20, 20, 8, 8, 0, 0, /* v */
    0, 0, 0, 0, 0, 34, 34, 42, 42, 42, 42, 20, 0, 0, /* w */
    0, 0, 0, 0, 0, 66, 66, 36, 24, 36, 66, 66, 0, 0, /* x */
    0, 0, 0, 0, 0, 66, 66, 66, 66, 70, 58, 2, 66, 60, /* y */
    0, 0, 0, 0, 0, 126, 4, 8, 16, 16, 32, 126, 0, 0, /* z */
    0, 6, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 6, /* { */
    0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, /* | */
    0, 48, 8, 8, 8, 8, 8, 4, 8, 8, 8, 8, 8, 48, /* } */
    0, 32, 82, 74, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ~ */
    0, 0, 8, 8, 0, 8, 8, 8, 8, 8, 8, 8, 0, 0, /* ¡ */
    0, 0, 0, 0, 16, 60, 82, 80, 80, 80, 82, 60, 16, 0, /* ¢ */
    0, 0, 0, 12, 18, 16, 16, 60, 16, 16, 60, 18, 0, 0, /* £ */
    0, 0, 0, 0, 66, 60, 36, 36, 60, 66, 0, 0, 0, 0, /* ¤ */
    0, 0, 34, 20, 20, 8, 62, 8, 62, 8, 8, 8, 0, 0, /* ¥ */
    0, 0, 8, 8, 8, 8, 0, 0, 8, 8, 8, 8, 0, 0, /* ¦ */
    0, 60, 66, 32, 24, 36, 66, 36, 24, 4, 66, 60, 0, 0, /* § */
    0, 36, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ¨ */
    0, 60, 66, 90, 102, 98, 98, 98, 102, 90, 66, 60, 0, 0, /* © */
    0, 28, 34, 30, 34, 38, 26, 0, 62, 0, 0, 0, 0, 0, /* ª */
    0, 0, 0, 0, 0, 10, 20, 40, 80, 40, 20, 10, 0, 0, /* « */
    0, 0, 0, 0, 0, 0, 0, 0, 62, 2, 2, 2, 0, 0, /* ¬ */
    0, 0, 0, 0, 0, 0, 0, 60, 0, 0, 0, 0, 0, 0, /* ­ */
    0, 60, 66, 122, 102, 102, 122, 102, 102, 102, 66, 60, 0, 0, /* ® */
    0, 0, 62, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ¯ */
    0, 24, 36, 36, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ° */
    0, 0, 0, 0, 0, 0, 8, 8, 62, 8, 8, 62, 0, 0, /* ± */
    0, 24, 36, 4, 8, 16, 32, 60, 0, 0, 0, 0, 0, 0, /* ² */
    0, 24, 36, 4, 24, 4, 36, 24, 0, 0, 0, 0, 0, 0, /* ³ */
    0, 4, 8, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* ´ */
    0, 0, 0, 0, 0, 0, 34, 34, 34, 34, 54, 42, 32, 32, /* µ */
    0, 0, 30, 42, 42, 42, 42, 26, 10, 10, 10, 10, 10, 14, /* ¶ */
    0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, /* · */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 16, /* ¸ */
    0, 8, 24, 8, 8, 8, 8, 28, 0, 0, 0, 0, 0, 0, /* ¹ */
    0, 0, 24, 36, 36, 24, 0, 60, 0, 0, 0, 0, 0, 0, /* º */
    0, 0, 0, 0, 0, 80, 40, 20, 10, 20, 40, 80, 0, 0, /* » */
    0, 0, 32, 98, 36, 36, 40, 18, 22, 42, 78, 66, 0, 0, /* ¼ */
    0, 0, 32, 98, 36, 36, 40, 20, 26, 34, 68, 78, 0, 0, /* ½ */
    0, 0, 98, 18, 36, 24, 104, 18, 38, 42, 78, 2, 0, 0, /* ¾ */
    0, 0, 0, 16, 16, 0, 16, 16, 16, 16, 32, 66, 66, 60, /* ¿ */
    16, 8, 0, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* À */
    8, 16, 0, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* Á */
    24, 36, 0, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* Â */
    50, 76, 0, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* Ã */
    0, 36, 0, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* Ä */
    0, 24, 36, 24, 36, 66, 66, 126, 66, 66, 66, 66, 0, 0, /* Å */
    0, 0, 30, 40, 72, 72, 126, 72, 72, 72, 72, 78, 0, 0, /* Æ */
    0, 0, 60, 66, 66, 64, 64, 64, 64, 66, 66, 60, 8, 16, /* Ç */
    16, 8, 0, 126, 64, 64, 64, 124, 64, 64, 64, 126, 0, 0, /* È */
    8, 16, 0, 126, 64, 64, 64, 124, 64, 64, 64, 126, 0, 0, /* É */
    24, 36, 0, 126, 64, 64, 64, 124, 64, 64, 64, 126, 0, 0, /* Ê */
    0, 36, 0, 126, 64, 64, 64, 124, 64, 64, 64, 126, 0, 0, /* Ë */
    16, 8, 0, 62, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* Ì */
    4, 8, 0, 62, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* Í */
    8, 20, 0, 62, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* Î */
    0, 20, 0, 62, 8, 8, 8, 8, 8, 8, 8, 62, 0, 0, /* Ï */
    0, 0, 60, 34, 33, 33, 121, 33, 33, 33, 34, 60, 0, 0, /* Ð */
    50, 76, 0, 98, 98, 82, 82, 74, 74, 74, 70, 70, 0, 0, /* Ñ */
    16, 8, 0, 60, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ò */
    8, 16, 0, 60, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ó */
    24, 36, 0, 60, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ô */
    50, 76, 0, 60, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Õ */
    0, 36, 0, 60, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ö */
    0, 0, 0, 0, 0, 65, 34, 20, 8, 20, 34, 65, 0, 0, /* × */
    2, 2, 60, 70, 74, 74, 74, 82, 82, 82, 98, 60, 64, 64, /* Ø */
    16, 8, 0, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ù */
    8, 16, 0, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ú */
    24, 36, 0, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Û */
    0, 36, 0, 66, 66, 66, 66, 66, 66, 66, 66, 60, 0, 0, /* Ü */
    4, 8, 0, 34, 34, 20, 20, 8, 8, 8, 8, 8, 0, 0, /* Ý */
    0, 0, 64, 64, 124, 66, 66, 66, 66, 124, 64, 64, 0, 0, /* Þ */
    0, 0, 24, 36, 36, 36, 56, 36, 34, 34, 34, 124, 0, 0, /* ß */
    0, 0, 16, 8, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* à */
    0, 0, 4, 8, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* á */
    0, 0, 24, 36, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* â */
    0, 0, 50, 76, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* ã */
    0, 0, 0, 36, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* ä */
    0, 24, 36, 24, 0, 60, 66, 14, 50, 66, 70, 58, 0, 0, /* å */
    0, 0, 0, 0, 0, 62, 73, 25, 47, 72, 73, 62, 0, 0, /* æ */
    0, 0, 0, 0, 0, 60, 66, 64, 64, 64, 66, 60, 8, 16, /* ç */
    0, 0, 16, 8, 0, 60, 66, 66, 126, 64, 66, 60, 0, 0, /* è */
    0, 0, 8, 16, 0, 60, 66, 66, 126, 64, 66, 60, 0, 0, /* é */
    0, 0, 24, 36, 0, 60, 66, 66, 126, 64, 66, 60, 0, 0, /* ê */
    0, 0, 0, 36, 0, 60, 66, 66, 126, 64, 66, 60, 0, 0, /* ë */
    0, 0, 16, 8, 0, 24, 8, 8, 8, 8, 8, 62, 0, 0, /* ì */
    0, 0, 4, 8, 0, 24, 8, 8, 8, 8, 8, 62, 0, 0, /* í */
    0, 0, 24, 36, 0, 24, 8, 8, 8, 8, 8, 62, 0, 0, /* î */
    0, 0, 0, 20, 0, 24, 8, 8, 8, 8, 8, 62, 0, 0, /* ï */
    0, 20, 8, 20, 2, 30, 34, 34, 34, 34, 34, 28, 0, 0, /* ð */
    0, 0, 50, 76, 0, 92, 98, 66, 66, 66, 66, 66, 0, 0, /* ñ */
    0, 0, 16, 8, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* ò */
    0, 0, 8, 16, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* ó */
    0, 0, 24, 36, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* ô */
    0, 0, 50, 76, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* õ */
    0, 0, 0, 36, 0, 60, 66, 66, 66, 66, 66, 60, 0, 0, /* ö */
    0, 0, 0, 0, 0, 0, 0, 24, 0, 126, 0, 24, 0, 0, /* ÷ */
    0, 0, 0, 2, 4, 60, 74, 74, 82, 82, 98, 60, 64, 64, /* ø */
    0, 0, 16, 8, 0, 66, 66, 66, 66, 66, 70, 58, 0, 0, /* ù */
    0, 0, 8, 16, 0, 66, 66, 66, 66, 66, 70, 58, 0, 0, /* ú */
    0, 0, 24, 36, 0, 66, 66, 66, 66, 66, 70, 58, 0, 0, /* û */
    0, 0, 0, 36, 0, 66, 66, 66, 66, 66, 70, 58, 0, 0, /* ü */
    0, 0, 8, 16, 0, 66, 66, 34, 36, 20, 28, 8, 72, 48, /* ý */
    0, 0, 64, 64, 64, 92, 98, 66, 66, 66, 98, 92, 64, 64, /* þ */
    0, 0, 0, 36, 0, 66, 66, 34, 36, 20, 28, 8, 72, 48, /* ÿ */
};

#define SMALL_FONT_WIDTH    5
#define SMALL_FONT_HEIGHT   9

static const raster_font_item small_font[] = {
    /* Each character is 5 x 9 pixels */
    0, 2, 2, 2, 2, 0, 2, 0, 0, /* ! */
    0, 5, 5, 5, 0, 0, 0, 0, 0, /* " */
    0, 0, 5, 15, 5, 15, 5, 0, 0, /* # */
    0, 0, 7, 26, 7, 18, 7, 0, 0, /* $ */
    0, 8, 9, 2, 4, 25, 1, 0, 0, /* % */
    0, 0, 4, 10, 4, 10, 5, 0, 0, /* & */
    0, 2, 2, 2, 0, 0, 0, 0, 0, /* ' */
    0, 2, 4, 4, 4, 4, 2, 0, 0, /* ( */
    0, 4, 2, 2, 2, 2, 4, 0, 0, /* ) */
    0, 0, 5, 2, 7, 2, 5, 0, 0, /* * */
    0, 0, 2, 2, 15, 2, 2, 0, 0, /* + */
    0, 0, 0, 0, 16, 3, 2, 4, 0, /* , */
    0, 0, 0, 0, 15, 0, 0, 0, 0, /* - */
    0, 0, 0, 0, 0, 6, 6, 0, 0, /* . */
    0, 0, 1, 2, 4, 8, 0, 0, 0, /* / */
    0, 2, 5, 5, 5, 5, 2, 0, 0, /* 0 */
    0, 2, 6, 2, 2, 2, 7, 0, 0, /* 1 */
    0, 6, 9, 1, 2, 4, 15, 0, 0, /* 2 */
    0, 15, 1, 6, 1, 9, 6, 0, 0, /* 3 */
    0, 2, 6, 10, 15, 2, 2, 0, 0, /* 4 */
    0, 15, 8, 14, 1, 9, 6, 0, 0, /* 5 */
    0, 6, 8, 14, 9, 9, 6, 0, 0, /* 6 */
    0, 15, 1, 2, 2, 4, 4, 0, 0, /* 7 */
    0, 6, 9, 6, 9, 9, 6, 0, 0, /* 8 */
    0, 6, 9, 9, 7, 1, 6, 0, 0, /* 9 */
    0, 0, 6, 6, 0, 6, 6, 0, 0, /* : */
    0, 0, 6, 6, 0, 6, 4, 8, 0, /* ; */
    0, 0, 1, 2, 4, 2, 1, 0, 0, /* < */
    0, 0, 0, 15, 0, 15, 0, 0, 0, /* = */
    0, 0, 4, 2, 1, 2, 4, 0, 0, /* > */
    0, 2, 5, 1, 2, 0, 2, 0, 0, /* ? */
    0, 6, 9, 11, 11, 8, 6, 0, 0, /* @ */
    0, 6, 9, 9, 15, 9, 9, 0, 0, /* A */
    0, 14, 9, 14, 9, 9, 14, 0, 0, /* B */
    0, 6, 9, 8, 8, 9, 6, 0, 0, /* C */
    0, 14, 9, 9, 9, 9, 14, 0, 0, /* D */
    0, 15, 8, 14, 8, 8, 15, 0, 0, /* E */
    0, 15, 8, 14, 8, 8, 8, 0, 0, /* F */
    0, 6, 9, 8, 11, 9, 7, 0, 0, /* G */
    0, 9, 9, 15, 9, 9, 9, 0, 0, /* H */
    0, 7, 2, 2, 2, 2, 7, 0, 0, /* I */
    0, 1, 1, 1, 1, 9, 6, 0, 0, /* J */
    0, 9, 10, 12, 12, 10, 9, 0, 0, /* K */
    0, 8, 8, 8, 8, 8, 15, 0, 0, /* L */
    0, 9, 15, 15, 9, 9, 9, 0, 0, /* M */
    0, 9, 13, 13, 11, 11, 9, 0, 0, /* N */
    0, 6, 9, 9, 9, 9, 6, 0, 0, /* O */
    0, 14, 9, 9, 14, 8, 8, 0, 0, /* P */
    0, 6, 9, 9, 9, 13, 6, 1, 0, /* Q */
    0, 14, 9, 9, 14, 10, 9, 0, 0, /* R */
    0, 6, 9, 4, 2, 9, 6, 0, 0, /* S */
    0, 7, 2, 2, 2, 2, 2, 0, 0, /* T */
    0, 9, 9, 9, 9, 9, 6, 0, 0, /* U */
    0, 9, 9, 9, 9, 6, 6, 0, 0, /* V */
    0, 9, 9, 9, 15, 15, 9, 0, 0, /* W */
    0, 9, 9, 6, 6, 9, 9, 0, 0, /* X */
    0, 5, 5, 5, 2, 2, 2, 0, 0, /* Y */
    0, 15, 1, 2, 4, 8, 15, 0, 0, /* Z */
    0, 7, 4, 4, 4, 4, 7, 0, 0, /* [ */
    0, 0, 8, 4, 2, 1, 0, 0, 0, /* \ */
    0, 7, 1, 1, 1, 1, 7, 0, 0, /* ] */
    0, 2, 5, 0, 0, 0, 0, 0, 0, /* ^ */
    0, 0, 0, 0, 0, 0, 15, 0, 0, /* _ */
    0, 4, 2, 0, 0, 0, 0, 0, 0, /* ` */
    0, 0, 0, 7, 9, 11, 5, 0, 0, /* a */
    0, 8, 8, 14, 9, 9, 14, 0, 0, /* b */
    0, 0, 0, 6, 8, 8, 6, 0, 0, /* c */
    0, 1, 1, 7, 9, 9, 7, 0, 0, /* d */
    0, 0, 0, 6, 11, 12, 6, 0, 0, /* e */
    0, 2, 5, 4, 14, 4, 4, 0, 0, /* f */
    0, 0, 0, 7, 9, 6, 8, 7, 0, /* g */
    0, 8, 8, 14, 9, 9, 9, 0, 0, /* h */
    0, 2, 0, 6, 2, 2, 7, 0, 0, /* i */
    0, 1, 0, 1, 1, 1, 5, 2, 0, /* j */
    0, 8, 8, 10, 12, 10, 9, 0, 0, /* k */
    0, 6, 2, 2, 2, 2, 7, 0, 0, /* l */
    0, 0, 0, 10, 15, 9, 9, 0, 0, /* m */
    0, 0, 0, 14, 9, 9, 9, 0, 0, /* n */
    0, 0, 0, 6, 9, 9, 6, 0, 0, /* o */
    0, 0, 0, 14, 9, 9, 14, 8, 0, /* p */
    0, 0, 0, 7, 9, 9, 7, 1, 0, /* q */
    0, 0, 0, 14, 9, 8, 8, 0, 0, /* r */
    0, 0, 0, 7, 12, 3, 14, 0, 0, /* s */
    0, 4, 4, 14, 4, 4, 3, 0, 0, /* t */
    0, 0, 0, 9, 9, 9, 7, 0, 0, /* u */
    0, 0, 0, 5, 5, 5, 2, 0, 0, /* v */
    0, 0, 0, 9, 9, 15, 15, 0, 0, /* w */
    0, 0, 0, 9, 6, 6, 9, 0, 0, /* x */
    0, 0, 0, 9, 9, 5, 2, 4, 0, /* y */
    0, 0, 0, 15, 2, 4, 15, 0, 0, /* z */
    0, 1, 2, 6, 2, 2, 1, 0, 0, /* { */
    0, 2, 2, 2, 2, 2, 2, 0, 0, /* | */
    0, 4, 2, 3, 2, 2, 4, 0, 0, /* } */
    0, 5, 10, 0, 0, 0, 0, 0, 0, /* ~ */
    0, 2, 0, 2, 2, 2, 2, 0, 0, /* ¡ */
    0, 0, 2, 7, 10, 10, 7, 2, 0, /* ¢ */
    0, 0, 3, 4, 14, 4, 11, 0, 0, /* £ */
    0, 0, 8, 7, 5, 7, 8, 0, 0, /* ¤ */
    0, 5, 21, 2, 7, 2, 18, 0, 0, /* ¥ */
    0, 0, 2, 2, 0, 2, 2, 0, 0, /* ¦ */
    0, 3, 4, 6, 5, 3, 1, 6, 0, /* § */
    0, 5, 0, 0, 0, 0, 0, 0, 0, /* ¨ */
    0, 7, 8, 10, 12, 10, 8, 7, 0, /* © */
    0, 6, 26, 22, 16, 16, 16, 0, 0, /* ª */
    0, 0, 0, 4, 9, 4, 0, 0, 0, /* « */
    0, 0, 0, 16, 15, 17, 0, 0, 0, /* ¬ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, /* ­ */
    0, 7, 8, 14, 12, 12, 8, 7, 0, /* ® */
    0, 15, 16, 16, 16, 16, 16, 0, 0, /* ¯ */
    0, 2, 5, 2, 0, 0, 0, 0, 0, /* ° */
    0, 2, 2, 15, 2, 2, 15, 0, 0, /* ± */
    0, 6, 2, 20, 6, 0, 16, 0, 0, /* ² */
    0, 6, 6, 2, 6, 0, 0, 0, 0, /* ³ */
    0, 2, 4, 0, 0, 0, 0, 0, 0, /* ´ */
    0, 0, 0, 9, 9, 9, 14, 8, 0, /* µ */
    0, 7, 13, 13, 5, 5, 5, 0, 0, /* ¶ */
    0, 0, 0, 6, 6, 0, 0, 0, 0, /* · */
    0, 0, 0, 0, 0, 0, 2, 4, 0, /* ¸ */
    0, 2, 6, 2, 7, 0, 0, 0, 0, /* ¹ */
    0, 4, 10, 4, 0, 0, 0, 0, 0, /* º */
    0, 0, 0, 9, 4, 9, 0, 0, 0, /* » */
    0, 8, 8, 8, 25, 3, 7, 1, 0, /* ¼ */
    0, 8, 8, 8, 11, 1, 2, 3, 0, /* ½ */
    0, 12, 12, 4, 13, 3, 7, 1, 0, /* ¾ */
    0, 2, 0, 2, 4, 5, 2, 0, 0, /* ¿ */
    0, 6, 9, 9, 15, 9, 9, 0, 0, /* À */
    0, 6, 9, 9, 15, 9, 9, 0, 0, /* Á */
    0, 6, 9, 9, 15, 9, 9, 0, 0, /* Â */
    0, 6, 9, 9, 15, 9, 9, 0, 0, /* Ã */
    0, 9, 6, 9, 15, 9, 9, 0, 0, /* Ä */
    0, 6, 6, 9, 15, 9, 9, 0, 0, /* Å */
    0, 7, 10, 11, 14, 10, 11, 0, 0, /* Æ */
    0, 6, 9, 8, 8, 9, 6, 4, 0, /* Ç */
    0, 15, 8, 14, 8, 8, 15, 0, 0, /* È */
    0, 15, 8, 14, 8, 8, 15, 0, 0, /* É */
    0, 15, 8, 14, 8, 8, 15, 0, 0, /* Ê */
    0, 15, 8, 14, 8, 8, 15, 0, 0, /* Ë */
    0, 7, 2, 2, 2, 2, 7, 0, 0, /* Ì */
    0, 7, 2, 2, 2, 2, 7, 0, 0, /* Í */
    0, 7, 2, 2, 2, 2, 7, 0, 0, /* Î */
    0, 7, 2, 2, 2, 2, 7, 0, 0, /* Ï */
    0, 14, 5, 13, 5, 5, 14, 0, 0, /* Ð */
    0, 11, 9, 13, 11, 11, 9, 0, 0, /* Ñ */
    0, 6, 9, 9, 9, 9, 6, 0, 0, /* Ò */
    0, 6, 9, 9, 9, 9, 6, 0, 0, /* Ó */
    0, 6, 9, 9, 9, 9, 6, 0, 0, /* Ô */
    0, 6, 9, 9, 9, 9, 6, 0, 0, /* Õ */
    0, 9, 6, 9, 9, 9, 6, 0, 0, /* Ö */
    0, 0, 0, 9, 6, 6, 9, 0, 0, /* × */
    0, 7, 11, 11, 13, 13, 14, 0, 0, /* Ø */
    0, 9, 9, 9, 9, 9, 6, 0, 0, /* Ù */
    0, 9, 9, 9, 9, 9, 6, 0, 0, /* Ú */
    0, 9, 9, 9, 9, 9, 6, 0, 0, /* Û */
    0, 9, 0, 9, 9, 9, 6, 0, 0, /* Ü */
    0, 5, 5, 5, 2, 2, 2, 0, 0, /* Ý */
    0, 8, 14, 9, 14, 8, 8, 0, 0, /* Þ */
    0, 6, 9, 10, 9, 9, 10, 0, 0, /* ß */
    0, 4, 2, 7, 9, 11, 5, 0, 0, /* à */
    0, 2, 4, 7, 9, 11, 5, 0, 0, /* á */
    0, 2, 5, 7, 9, 11, 5, 0, 0, /* â */
    0, 5, 10, 7, 9, 11, 5, 0, 0, /* ã */
    0, 5, 0, 7, 9, 11, 5, 0, 0, /* ä */
    0, 6, 6, 7, 9, 11, 5, 0, 0, /* å */
    0, 0, 0, 7, 11, 10, 7, 0, 0, /* æ */
    0, 0, 0, 3, 4, 4, 3, 2, 0, /* ç */
    0, 4, 2, 6, 11, 12, 6, 0, 0, /* è */
    0, 2, 4, 6, 11, 12, 6, 0, 0, /* é */
    0, 4, 10, 6, 11, 12, 6, 0, 0, /* ê */
    0, 10, 0, 6, 11, 12, 6, 0, 0, /* ë */
    0, 4, 2, 6, 2, 2, 7, 0, 0, /* ì */
    0, 2, 4, 6, 2, 2, 7, 0, 0, /* í */
    0, 2, 5, 6, 2, 2, 7, 0, 0, /* î */
    0, 5, 0, 6, 2, 2, 7, 0, 0, /* ï */
    0, 4, 3, 6, 9, 9, 6, 0, 0, /* ð */
    0, 5, 10, 14, 9, 9, 9, 0, 0, /* ñ */
    0, 4, 2, 6, 9, 9, 6, 0, 0, /* ò */
    0, 2, 4, 6, 9, 9, 6, 0, 0, /* ó */
    0, 6, 0, 6, 9, 9, 6, 0, 0, /* ô */
    0, 5, 10, 6, 9, 9, 6, 0, 0, /* õ */
    0, 5, 0, 6, 9, 9, 6, 0, 0, /* ö */
    0, 0, 6, 0, 15, 0, 6, 0, 0, /* ÷ */
    0, 0, 0, 7, 11, 13, 14, 0, 0, /* ø */
    0, 4, 2, 9, 9, 9, 7, 0, 0, /* ù */
    0, 2, 4, 9, 9, 9, 7, 0, 0, /* ú */
    0, 6, 0, 9, 9, 9, 7, 0, 0, /* û */
    0, 5, 0, 9, 9, 9, 7, 0, 0, /* ü */
    0, 2, 4, 9, 9, 5, 2, 4, 0, /* ý */
    0, 0, 8, 14, 9, 9, 14, 8, 0, /* þ */
    0, 5, 0, 9, 9, 5, 2, 4, 0, /* ÿ */
};

/* Adapted from OCR-B font version 0.2 Matthew Skala
 *   https://tsukurimashou.osdn.jp/ocr.php.en
 *
 * Copyright Matthew Skala (2011); based on code by Norbert Schwarz (1986, 2011)
 *
 * "The version in this package descends from a set of Metafont
 *  definitions by Norbert Schwarz of Ruhr-Universitaet Bochum,
 *  bearing dates ranging from 1986 to 2010. He originally
 *  distributed it under a "non-commercial use only"
 *  restriction but has since released it for unrestricted use
 *  and distribution. See the README file for more details."
 *
 * The README states (http://mirrors.ctan.org/fonts/ocr-b.zip)
 *
 * "As far as the digitization in METAFONT input which I have
 *  developed, you may freely use, modify, and/or distribute any of
 *  these files or the resulting fonts, without limitation.  A previous
 *  release of ocr-b only granted rights for non-commercial use; that
 *  restriction is now lifted."
 *
 * Used OCRB.otf with FontForge to create OCRB-18.bdf (normal) and OCRB-16.bdf (small) and then touched up
 * using gbdfed
 * Note there's no bold version of OCR-B.
 */

#define UPCEAN_FONT_WIDTH   9
#define UPCEAN_FONT_HEIGHT  14

/* Each character is 9 x 14 pixels */
static const raster_font_item upcean_font[] = {
    /*30*/ 0x007C, 0x00FE, 0x00C6, 0x0183, 0x0183, 0x0183, 0x0183, 0x0183, 0x0183, 0x0183, 0x0183, 0x00C6, 0x00FE, 0x007C, /* 0 */
    /*31*/ 0x000C, 0x001C, 0x003C, 0x006C, 0x004C, 0x000C, 0x000C, 0x000C, 0x000C, 0x000C, 0x000C, 0x000C, 0x000C, 0x000C, /* 1 */
    /*32*/ 0x007C, 0x00FE, 0x0183, 0x0003, 0x0007, 0x000E, 0x001C, 0x0038, 0x0070, 0x00E0, 0x01C0, 0x0180, 0x01FE, 0x00FF, /* 2 */
    /*33*/ 0x01FE, 0x01FE, 0x0006, 0x000C, 0x0018, 0x0038, 0x003C, 0x0006, 0x0003, 0x0003, 0x0003, 0x0006, 0x01FE, 0x00F8, /* 3 */
    /*34*/ 0x0018, 0x0018, 0x0030, 0x0030, 0x0060, 0x0060, 0x00C4, 0x00CC, 0x018C, 0x01FF, 0x01FF, 0x000C, 0x000C, 0x000C, /* 4 */
    /*35*/ 0x00FE, 0x00FE, 0x00C0, 0x00C0, 0x00C0, 0x00F8, 0x00FC, 0x0006, 0x0006, 0x0006, 0x0006, 0x000C, 0x01FC, 0x00F8, /* 5 */
    /*36*/ 0x000C, 0x001C, 0x0038, 0x0030, 0x0060, 0x00F8, 0x00FE, 0x01C7, 0x0183, 0x0183, 0x0183, 0x01C3, 0x00FE, 0x007C, /* 6 */
    /*37*/ 0x01FF, 0x01FF, 0x0003, 0x0006, 0x000E, 0x000C, 0x0018, 0x0030, 0x0030, 0x0030, 0x0060, 0x0060, 0x0060, 0x0060, /* 7 */
    /*38*/ 0x007C, 0x00FE, 0x00C6, 0x0082, 0x00C6, 0x007C, 0x007C, 0x00EE, 0x01C7, 0x0183, 0x0183, 0x0183, 0x00FE, 0x007C, /* 8 */
    /*39*/ 0x007C, 0x00FE, 0x00C7, 0x0183, 0x0183, 0x0183, 0x01C3, 0x00FE, 0x007E, 0x000C, 0x000C, 0x0018, 0x0030, 0x0060, /* 9 */
    /*3A*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* : (blank) */
    /*3B*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* ; (blank) */
    /*3C*/ 0x0000, 0x0000, 0x0007, 0x000E, 0x003C, 0x0070, 0x01E0, 0x01C0, 0x00F0, 0x0038, 0x001E, 0x0007, 0x0003, 0x0000, /* < */
    /*3D*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* = (blank) */
    /*3E*/ 0x0000, 0x0000, 0x01C0, 0x00E0, 0x0078, 0x001C, 0x000F, 0x0007, 0x001E, 0x0038, 0x00F0, 0x01C0, 0x0180, 0x0000, /* > */
};

#define UPCEAN_SMALL_FONT_WIDTH   8
#define UPCEAN_SMALL_FONT_HEIGHT  13

/* Each character is 8 x 13 pixels */
static const raster_font_item upcean_small_font[] = {
    /*30*/ 0x3C, 0x7E, 0x66, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0x66, 0x7E, 0x3C, /* 0 */
    /*31*/ 0x00, 0x0E, 0x1E, 0x36, 0x26, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, /* 1 */
    /*32*/ 0x38, 0x7C, 0xC6, 0x02, 0x02, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0xFC, 0x7E, /* 2 */
    /*33*/ 0x00, 0xFE, 0xFE, 0x06, 0x1C, 0x38, 0x3E, 0x07, 0x03, 0x03, 0x06, 0xFC, 0x78, /* 3 */
    /*34*/ 0x00, 0x18, 0x18, 0x30, 0x30, 0x60, 0x64, 0xCC, 0xCC, 0xFF, 0xFF, 0x0C, 0x0C, /* 4 */
    /*35*/ 0x00, 0x7E, 0x7E, 0x40, 0x40, 0x78, 0x7C, 0x06, 0x02, 0x02, 0x06, 0xFC, 0x78, /* 5 */
    /*36*/ 0x00, 0x0C, 0x1C, 0x30, 0x30, 0x7C, 0xFE, 0xC7, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, /* 6 */
    /*37*/ 0x00, 0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, /* 7 */
    /*38*/ 0x3C, 0x7E, 0x66, 0x42, 0x66, 0x3C, 0x3C, 0x66, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C, /* 8 */
    /*39*/ 0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xE3, 0x7E, 0x1E, 0x0C, 0x18, 0x30, 0x60, /* 9 */
    /*3A*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* : (blank) */
    /*3B*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ; (blank) */
    /*3C*/ 0x00, 0x00, 0x03, 0x0E, 0x1C, 0x70, 0xE0, 0xE0, 0x30, 0x1C, 0x06, 0x03, 0x00, /* < */
    /*3D*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* = (blank) */
    /*3E*/ 0x00, 0x00, 0xC0, 0xE0, 0x38, 0x1C, 0x07, 0x07, 0x1C, 0x38, 0xE0, 0xC0, 0x00, /* > */
};

/* vim: set ts=4 sw=4 et : */
#endif /* Z_RASTER_FONT_H */

/* telepen.c - Handles Telepen and Telepen numeric */
/*
    libzint - the open source barcode library
    Copyright (C) 2008-2026 Robin Stuart <rstuart114@gmail.com>

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

/* Telepen Barcode Symbology information and History (BSiH)
   https://advanova.co.uk/wp-content/uploads/2022/05/Barcode-Symbology-information-and-History.pdf */
/* AIM Europe USS Telepen (1991) */

#define SODIUM_X_F        (IS_NUM_F | IS_UX__F | IS_LX__F) /* SODIUM "0123456789Xx" */

#define TELE_SYMBOL_MAX     69      /* 16 * (1 (Start) + 69 + 1 (check digit) + 1 (Stop)) = 1152 */
#define TELE_SYMBOL_MAX_S   "69"    /* String version of above */
#define TELE_LENGTH_MAX     200     /* Max possible is 138 - use > for better error messages */

#include <assert.h>
#include <stdio.h>
#include "common.h"

/* Copied into "test_telepen.c" for lengths generator - see below */
static const char TeleTable[132][16] = {
    { "31313131"       }, { "1131313111"     }, { "33313111"              }, { "1111313131"             }, /*00-03*/
    { "3111313111"     }, { "11333131"       }, { "13133131"              }, { "111111313111"           }, /*04-07*/
    { "31333111"       }, { "1131113131"     }, { "33113131"              }, { "1111333111"             }, /*08-0B*/
    { "3111113131"     }, { "1113133111"     }, { "1311133111"            }, { "111111113131"           }, /*0C-0F*/
    { "3131113111"     }, { "11313331"       }, { "333331"                }, { "111131113111"           }, /*10-13*/
    { "31113331"       }, { "1133113111"     }, { "1313113111"            }, { "1111113331"             }, /*14-17*/
    { "31131331"       }, { "113111113111"   }, { "3311113111"            }, { "1111131331"             }, /*18-1B*/
    { "311111113111"   }, { "1113111331"     }, { "1311111331"            }, { "11111111113111"         }, /*1C-1F*/
    { "31313311"       }, { "1131311131"     }, { "33311131"              }, { "1111313311"             }, /*20-23*/
    { "3111311131"     }, { "11333311"       }, { "13133311"              }, { "111111311131"           }, /*24-27*/
    { "31331131"       }, { "1131113311"     }, { "33113311"              }, { "1111331131"             }, /*28-2B*/
    { "3111113311"     }, { "1113131131"     }, { "1311131131"            }, { "111111113311"           }, /*2C-2F*/
    { "3131111131"     }, { "1131131311"     }, { "33131311"              }, { "111131111131"           }, /*30-33*/
    { "3111131311"     }, { "1133111131"     }, { "1313111131"            }, { "111111131311"           }, /*34-37*/
    { "3113111311"     }, { "113111111131"   }, { "3311111131"            }, { "111113111311"           }, /*38-3B*/
    { "311111111131"   }, { "111311111311"   }, { "131111111311"          }, { "11111111111131"         }, /*3C-3F*/
    { "3131311111"     }, { "11313133"       }, { "333133"                }, { "111131311111"           }, /*40-43*/
    { "31113133"       }, { "1133311111"     }, { "1313311111"            }, { "1111113133"             }, /*44-47*/
    { "313333"         }, { "113111311111"   }, { "3311311111"            }, { "11113333"               }, /*48-4B*/
    { "311111311111"   }, { "11131333"       }, { "13111333"              }, { "11111111311111"         }, /*4C-4F*/
    { "31311133"       }, { "1131331111"     }, { "33331111"              }, { "1111311133"             }, /*50-53*/
    { "3111331111"     }, { "11331133"       }, { "13131133"              }, { "111111331111"           }, /*54-57*/
    { "3113131111"     }, { "1131111133"     }, { "33111133"              }, { "111113131111"           }, /*58-5B*/
    { "3111111133"     }, { "111311131111"   }, { "131111131111"          }, { "111111111133" /*START*/ }, /*5C-5F*/
    { "31311313"       }, { "113131111111"   }, { "3331111111"            }, { "1111311313"             }, /*60-63*/
    { "311131111111"   }, { "11331313"       }, { "13131313"              }, { "11111131111111"         }, /*64-67*/
    { "3133111111"     }, { "1131111313"     }, { "33111313"              }, { "111133111111"           }, /*68-6B*/
    { "3111111313"     }, { "111313111111"   }, { "131113111111"          }, { "111111111313"           }, /*6C-6F*/
    { "313111111111"   }, { "1131131113"     }, { "33131113"              }, { "11113111111111"         }, /*70-73*/
    { "3111131113"     }, { "113311111111"   }, { "131311111111"          }, { "111111131113"           }, /*74-77*/
    { "3113111113"     }, { "11311111111111" }, { "331111111111" /*STOP*/ }, { "111113111113"           }, /*78-7B*/
    { "31111111111111" }, { "111311111113"   }, { "131111111113"          },                               /*7C-7E*/
    {'1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1'},                                     /*7F*/
    { "111111113113" /*START 2*/ }, { "311311111111" /*STOP 2*/ },                                         /*80-81*/
    { "111111311113" /*START 3*/ }, { "311113111111" /*STOP 3*/ },                                         /*82-83*/
};

/* Generated by "backend/tests/test_telepen -f generate_lens -g" */
static const char TeleLens[128] = {
     8, 10,  8, 10, 10,  8,  8, 12,  8, 10,  8, 10, 10, 10, 10, 12, /*00-0F*/
    10,  8,  6, 12,  8, 10, 10, 10,  8, 12, 10, 10, 12, 10, 10, 14, /*10-1F*/
     8, 10,  8, 10, 10,  8,  8, 12,  8, 10,  8, 10, 10, 10, 10, 12, /*20-2F*/
    10, 10,  8, 12, 10, 10, 10, 12, 10, 12, 10, 12, 12, 12, 12, 14, /*30-3F*/
    10,  8,  6, 12,  8, 10, 10, 10,  6, 12, 10,  8, 12,  8,  8, 14, /*40-4F*/
     8, 10,  8, 10, 10,  8,  8, 12, 10, 10,  8, 12, 10, 12, 12, 12, /*50-5F*/
     8, 12, 10, 10, 12,  8,  8, 14, 10, 10,  8, 12, 10, 12, 12, 12, /*60-6F*/
    12, 10,  8, 14, 10, 12, 12, 12, 10, 14, 12, 12, 14, 12, 12, 16  /*70-7F*/
};

/* Expand numeric data */
static char *tele_num(struct zint_symbol *symbol, const unsigned char source[], const int length, int i, char *d,
                        int *p_count) {
    assert((length & 1) == (i & 1));
    for (; i < length; i += 2) {
        int glyph;
        if (source[i] == 'X') {
            (void) z_errtxtf(0, symbol, 394, "Invalid odd position %d of \"X\" in Telepen data", i + 1);
            return NULL;
        }
        glyph = source[i + 1] == 'X' ? z_ctoi(source[i]) + 17 : 10 * z_ctoi(source[i]) + z_ctoi(source[i + 1]) + 27;
        *p_count += glyph;
        memcpy(d, TeleTable[glyph], TeleLens[glyph]);
        d += TeleLens[glyph];
    }
    return d;
}

/* Set height according to various Telepen docs and AIM USS Telepen (1991) */
static int tele_set_height(struct zint_symbol *symbol, const int asc_length, const int num_length,
            const int have_dle) {
    int warn_number = 0;

    assert(asc_length > 0 || num_length > 0);
    assert(!(num_length & 1));

    if (symbol->output_options & COMPLIANT_HEIGHT) {
        /* Following based on various Telepen docs and USS Telepen Section 3.2 Dimensions.
           Recommended X between 0.01" & 0.0125" (0.254mm & 0.3175mm), so average is 0.01125" (0.28575mm).
           Default height based on default 26pt at average X 0.01125" is then (26 / 72) / 0.01125 ~ 32X.
           Per USS Telepen min height is 6.35mm or 15% of the symbol length whichever is greater, with
           symbol length calculated as 16 * X * (3 + C + D/2 + S) + 2Q where
           C is ASCII count, D numeric count, S whether have DLE or not, and Q quiet zone width (10X).
           As don't have max X, use 15% as min height */
        const float mult = 0.4584f; /* 0.191 * 16 * 15% */
        const float min_height = z_stripf(mult * (3.0f + asc_length + num_length / 2 + have_dle) + 3.0f /*20*15%*/);
        if (symbol->debug & ZINT_DEBUG_PRINT) printf("Min height: %.8g\n", min_height);
        warn_number = z_set_height(symbol, min_height, min_height > 32.0f ? min_height : 32.0f, 0, 0 /*no_errtxt*/);
    } else {
        (void) z_set_height(symbol, 0.0f, 50.0f, 0, 1 /*no_errtxt*/);
    }
    return warn_number;
}

/* Telepen Alpha (Full ASCII) */
INTERNAL int zint_telepen(struct zint_symbol *symbol, unsigned char source[], int length) {
    int count, check_digit;
    int error_number;
    int i;
    int symbol_chars = length;
    char dest[1145]; /* 12 (Start) + 69 * 16 (max for DELs) + 16 (Check) + 12 (stop) + 1 = 1145 */
    char *d = dest;
    unsigned char local_source[138];
    int asc_length = length, num_length = 0, have_dle = 0;
    const int asc_comp_num = symbol->option_2 == 1; /* AIM Full ASCII + Compressed Numeric mode */
    const int content_segs = symbol->output_options & BARCODE_CONTENT_SEGS;

    /* Check for DLE (Compressed Numeric tail) only if AIM mode enabled */
    if (asc_comp_num && length <= TELE_LENGTH_MAX) {
        for (i = 0; i < length; i++) {
            if (source[i] == 0x10 /*DLE*/) {
                if (i == 0) {
                    return z_errtxt(ZINT_ERROR_INVALID_DATA, symbol, 397,
                                "DLE (ASCII 16) cannot be first character in Full ASCII + Compressed Numeric Mode");
                }
                asc_length = i + 1; /* Include DLE */
                num_length = length - asc_length;
                symbol_chars = asc_length + ((num_length + 1) >> 1);
                have_dle = 1;
                break;
            }
        }
    }

    if (symbol_chars > TELE_SYMBOL_MAX) {
        if (length <= TELE_LENGTH_MAX) {
            return ZEXT z_errtxtf(ZINT_ERROR_TOO_LONG, symbol, 390,
                                    "Input length %1$d too long, requires %2$d symbol characters (maximum "
                                    TELE_SYMBOL_MAX_S ")", length, symbol_chars);
        }
        return z_errtxtf(ZINT_ERROR_TOO_LONG, symbol, 399,
                            "Input length %d too long, requires too many symbol characters (maximum "
                            TELE_SYMBOL_MAX_S ")", length);
    }

    /* Start character */
    memcpy(d, TeleTable[asc_comp_num ? 0x82 : 0x5F], 12);
    d += 12;

    count = 0;
    for (i = 0; i < asc_length; i++) {
        const unsigned char ch = source[i];
        if (!z_isascii(ch)) {
            /* Cannot encode extended ASCII */
            return z_errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 391,
                            "Invalid character at position %d in input, extended ASCII not allowed", i + 1);
        }
        memcpy(d, TeleTable[ch], TeleLens[ch]);
        d += TeleLens[ch];
        count += ch;
    }
    if (num_length) {
        /* Compressed Numeric Mode tail */
        if ((i = z_not_sane(SODIUM_X_F, source + asc_length, num_length))) {
            return z_errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 396,
                            "Invalid character at position %d in input (digits and \"X\" only)", asc_length + i);
        }

        memcpy(local_source, source, asc_length - 1); /* Exclude DLE */
        /* Add a leading zero if required */
        if (num_length & 1) {
            local_source[asc_length - 1] = '0'; /* Replaces DLE */
            memcpy(local_source + asc_length, source + asc_length, num_length++);
        } else {
            memcpy(local_source + asc_length - 1, source + asc_length, num_length);
            length--; /* Less DLE */
        }
        asc_length--; /* Less DLE */
        z_to_upper(local_source + asc_length, num_length);

        if (!(d = tele_num(symbol, local_source, length, asc_length, d, &count))) {
            return ZINT_ERROR_INVALID_DATA;
        }
    }

    check_digit = 127 - (count % 127);
    if (check_digit == 127) {
        check_digit = 0;
    }
    memcpy(d, TeleTable[check_digit], TeleLens[check_digit]);
    d += TeleLens[check_digit];

    /* Stop character */
    memcpy(d, TeleTable[asc_comp_num ? 0x83 : 0x7A], 12);
    d += 12;

    if (symbol->debug & ZINT_DEBUG_PRINT) {
        printf("Check digit: %d\nBinary (%d): %.*s\n", check_digit, (int) (d - dest), (int) (d - dest), dest);
    }

    z_expand(symbol, dest, (int) (d - dest));

    error_number = tele_set_height(symbol, asc_length, num_length, have_dle);

    if (num_length) {
        z_hrt_cpy_iso8859_1(symbol, local_source, length); /* `local_source` HRT-ready */
    } else {
        /* Chop off any trailing DLE if Full ASCII + Compressed Numeric mode */
        z_hrt_cpy_iso8859_1(symbol, source, length - (asc_comp_num && source[length - 1] == 0x10));
    }

    if (content_segs) {
        if (asc_length < length) {
            local_source[length++] = check_digit;
            if (z_ct_cpy_cat(symbol, source, asc_length, '\x10' /*DLE*/,
                                local_source + asc_length, length - asc_length)) {
                return ZINT_ERROR_MEMORY; /* `z_ct_cpy_cat()` only fails with OOM */
            }
        } else if (z_ct_cpy_cat(symbol, source, length, (char) check_digit, NULL /*cat*/, 0)) {
            return ZINT_ERROR_MEMORY; /* `z_ct_cpy_cat()` only fails with OOM */
        }
    }

    return error_number;
}

/* Telepen Numeric (Compressed Numeric Mode) */
INTERNAL int zint_telepen_num(struct zint_symbol *symbol, unsigned char source[], int length) {
    int count, check_digit;
    int error_number = 0;
    int i;
    int symbol_chars = (length + 1) >> 1;
    char dest[1145]; /* 12 (Start) + 69 * 16 (max for DELs) + 16 (Check) + 12 (stop) + 1 = 1145 */
    char *d = dest;
    unsigned char local_source[136 + 1 + 1];
    int num_length = length, have_dle = 0;
    const int comp_num_asc = symbol->option_2 == 1; /* AIM Compressed Numeric + Full ASCII mode */
    const int content_segs = symbol->output_options & BARCODE_CONTENT_SEGS;

    /* Check for DLE (Full ASCII tail) whether AIM mode or not */
    if (length <= TELE_LENGTH_MAX) { /* Max possible is 138 digits */
        for (i = 0; i < length; i++) {
            if (source[i] == 0x10 /*DLE*/) {
                if (i == 0) {
                    return z_errtxt(ZINT_ERROR_INVALID_DATA, symbol, 398,
                                    "DLE (ASCII 16) cannot be first character in Compressed Numeric Mode");
                }
                num_length = i; /* DLE not counted */
                symbol_chars = length - num_length + ((num_length + 1) >> 1);
                have_dle = 1;
                break;
            }
        }
    }

    if (symbol_chars > TELE_SYMBOL_MAX) {
        if (length <= TELE_LENGTH_MAX) {
            return ZEXT z_errtxtf(ZINT_ERROR_TOO_LONG, symbol, 392,
                                    "Input length %1$d too long, requires %2$d symbol characters (maximum "
                                    TELE_SYMBOL_MAX_S ")", length, symbol_chars);
        }
        return z_errtxtf(ZINT_ERROR_TOO_LONG, symbol, 400,
                            "Input length %d too long, requires too many symbol characters (maximum "
                            TELE_SYMBOL_MAX_S ")", length);
    }

    if ((i = z_not_sane(SODIUM_X_F, source, num_length))) {
        return z_errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 393,
                        "Invalid character at position %d in input (digits and \"X\" only)", i);
    }

    /* Add a leading zero if required */
    if (num_length & 1) {
        memcpy(local_source + 1, source, length++);
        local_source[0] = '0';
        num_length++;
    } else {
        memcpy(local_source, source, length);
    }
    z_to_upper(local_source, num_length);

    /* Start character */
    memcpy(d, TeleTable[comp_num_asc ? 0x80 : 0x5F], 12);
    d += 12;

    count = 0;
    if (!(d = tele_num(symbol, local_source, num_length, 0 /*i*/, d, &count))) {
        return ZINT_ERROR_INVALID_DATA;
    }
    for (i = num_length; i < length; i++) {
        /* Full ASCII tail */
        const unsigned char ch = local_source[i];
        assert(i > num_length || ch == '\x10'); /* 1st char DLE */
        if (!z_isascii(ch)) {
            /* Cannot encode extended ASCII */
            return z_errtxtf(ZINT_ERROR_INVALID_DATA, symbol, 395,
                            "Invalid character at position %d in input, extended ASCII not allowed", i + 1);
        }
        memcpy(d, TeleTable[ch], TeleLens[ch]);
        d += TeleLens[ch];
        count += ch;
    }

    check_digit = 127 - (count % 127);
    if (check_digit == 127) {
        check_digit = 0;
    }
    memcpy(d, TeleTable[check_digit], TeleLens[check_digit]);
    d += TeleLens[check_digit];

    /* Stop character */
    memcpy(d, TeleTable[comp_num_asc ? 0x81 : 0x7A], 12);
    d += 12;

    if (symbol->debug & ZINT_DEBUG_PRINT) {
        printf("Check digit: %d\nBinary (%d): %.*s\n", check_digit, (int) (d - dest), (int) (d - dest), dest);
    }

    z_expand(symbol, dest, (int) (d - dest));

    error_number = tele_set_height(symbol, length - num_length /*asc_length*/, num_length, have_dle);

    if (num_length < length) {
        /* Omit DLE */
        z_hrt_cpy_cat_nochk(symbol, local_source, num_length, '\xFF' /*separator (none)*/,
                            local_source + num_length + 1, length - (num_length + 1));
    } else {
        z_hrt_cpy_nochk(symbol, local_source, length);
    }

    if (content_segs) {
        if (num_length < length) {
            local_source[length++] = check_digit;
            if (z_ct_cpy_cat(symbol, local_source, num_length, '\xFF' /*separator (none)*/,
                                local_source + num_length, length - num_length)) {
                return ZINT_ERROR_MEMORY; /* `z_ct_cpy_cat()` only fails with OOM */
            }
        } else if (z_ct_cpy_cat(symbol, local_source, length, (char) check_digit, NULL /*cat*/, 0)) {
            return ZINT_ERROR_MEMORY; /* `z_ct_cpy_cat()` only fails with OOM */
        }
    }

    return error_number;
}

/* vim: set ts=4 sw=4 et : */

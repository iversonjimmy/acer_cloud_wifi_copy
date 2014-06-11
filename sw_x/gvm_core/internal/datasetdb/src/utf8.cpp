/*
 *  Copyright 2012 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud Technology, Inc.
 */

#include "vplu_types.h"
#include "utf8.hpp"

extern u16 unicode_upcase_table[];

#define UNICODE_UPPER(x)   (((x) < 0x2500) ? unicode_upcase_table[(x)] : (((x) >= 0xff41) && ((x) < 0xff5b)) ? ((x) - 0x20) : (x))

// Input is an UTF-8 string
// Return the Unicode representation of the next character
static int get_next_code(const char *s, int *skip)
{
    int code;

    if ((s[0] & 0x80) == 0x0) {
        *skip = 1;
        code = s[0];
    } else if ((s[0] & 0xe0) == 0xc0) {
        *skip = 2;
        code = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
    } else if ((s[0] & 0xf0) == 0xe0) {
        *skip = 3;
        code = ((s[0] & 0x0f) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
    } else {
        *skip = 4;
        code = ((s[0] & 0x07) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
    }

    return code;
}

// Convert an unicode representation to its UTF-8 encoding
static void unicode_to_utf8(int code, std::string &s)
{
    char c_utf8[5];

    if (code <= 0x7f) {
        c_utf8[0] = code;
        c_utf8[1] = '\0';
    } else if (code <= 0x7ff) {
        c_utf8[0] = 0xc0 | ((code & 0x7c0) >> 6);
        c_utf8[1] = 0x80 | (code & 0x3f);
        c_utf8[2] = '\0';
    } else if (code <= 0xffff) {
        c_utf8[0] = 0xe0 | ((code & 0xf000) >> 12);
        c_utf8[1] = 0x80 | ((code & 0xfc0) >> 6);
        c_utf8[2] = 0x80 | (code & 0x3f);
        c_utf8[3] = '\0';
    } else {
        c_utf8[0] = 0xf0 | ((code & 0x1c0000) >> 16);
        c_utf8[1] = 0x80 | ((code & 0x3f000) >> 12);
        c_utf8[2] = 0x80 | ((code & 0xfc0) >> 6);
        c_utf8[3] = 0x80 | (code & 0x3f);
        c_utf8[4] = '\0';
    }

    s = c_utf8;
}

void utf8_upper(const char *s, std::string &s_upper)
{
    std::string c_upper;
    int i = 0, skip, code;

    while (s[i] != '\0') {
        code = UNICODE_UPPER(get_next_code(&s[i], &skip));
        unicode_to_utf8(code, c_upper);
        s_upper += c_upper;

        i += skip;
    }
}

int utf8_casencmp(int s1_len, const char *s1, int s2_len, const char *s2)
{
    int i = 0, min_len, code_1, code_2, skip_1, skip_2;

    min_len = s1_len < s2_len ? s1_len : s2_len;

    while (i < min_len) {
        code_1 = UNICODE_UPPER(get_next_code(&s1[i], &skip_1));
        code_2 = UNICODE_UPPER(get_next_code(&s2[i], &skip_2));

        if (code_1 < code_2) {
            return -1;
        }
        if (code_1 > code_2) {
            return 1;
        }

        i += skip_1;
    }

    if (s1_len < s2_len) {
        return -1;
    }
    if (s1_len > s2_len) {
        return 1;
    }

    return 0;
}

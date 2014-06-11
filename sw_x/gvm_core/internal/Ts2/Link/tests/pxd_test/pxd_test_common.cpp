//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//
#include "pxd_test_common.hpp"

#define per_line  4

static void
make_printable(const char *contents, char *printable, int length)
{
    memcpy(printable, contents, length);
#ifndef _WIN32
    // FIXME: there is assertion failure on win32
    for (int c = 0; c < length; c++) {
        if
        (
           !(isprint(printable[c]) && !isspace(printable[c]))
        && printable[c] != ' '
        ) {
            printable[c] = '.';
        }
    }
#endif
    printable[length] = 0;
}

void
hex_dump(const char *title, const char *contents, int length)
{
    int      i;
    int      stride;
    int      count;
    int32_t  data1;
    int32_t  data2;
    int32_t  data3;
    int32_t  data4;
    int      start;
    char     printable[per_line * sizeof(data1) + 1];

    VPLTRACE_LOG_INFO(TRACE_APP, 0, "dump %s (%d bytes)", title, length);

    count  = sizeof(data1);
    stride = per_line * count;

    for (i = 0; i < length / stride; i++) {
        memcpy(&data1, contents + (i * stride) + 0 * count, count);
        memcpy(&data2, contents + (i * stride) + 1 * count, count);
        memcpy(&data3, contents + (i * stride) + 2 * count, count);
        memcpy(&data4, contents + (i * stride) + 3 * count, count);

        make_printable(contents + i * stride, printable, per_line * sizeof(data1));

        VPLTRACE_LOG_INFO(TRACE_APP, 0, "  0x%8.8x  0x%8.8x  0x%8.8x  0x%8.8x    %s",
            data1, data2, data3, data4, printable);
    }

    start = (i * stride) / count;

    for (i = start; i < length / count; i++) {
        memcpy(&data1, contents + (i * count), count);
        make_printable(contents + i * count, printable, count);
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "  0x%8.8x    %s", data1, printable);
    }

    start = i * count;

    for (i = start; i < length; i++) {
        make_printable(contents + i, printable, 1);
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "  0x%2.2x   %s", contents[i] & 0xff, printable);
    }
}



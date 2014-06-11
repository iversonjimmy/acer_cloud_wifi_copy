#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#if defined LINUX && !defined LINUX_EMB
#include <execinfo.h>
#endif

#include "TestFailed.hpp"

#include "utils.hpp"

void fatal(const char *fmt, ...)
{
    fprintf(stderr, "FATAL: ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");

#if defined LINUX && !defined LINUX_EMB
    void *array[30];
    size_t size;
    char **strings;
    size_t i;
    size = backtrace(array, sizeof(array)/sizeof(array[0]));
    strings = backtrace_symbols(array, size);
    fprintf(stderr, "backtrace:\n");
    for (i = 0; i < size; i++) {
        fprintf(stderr, "%s\n", strings[i]);
    }
    free(strings);
#endif

    throw TestFailed();
}

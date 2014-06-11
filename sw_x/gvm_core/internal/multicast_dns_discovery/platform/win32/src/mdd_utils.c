#include "mdd_utils.h"

#include <stdio.h>
#include <string.h>

char* MDDUtils_Strdup(const char *str)
{
    char *dup_str;
    unsigned int size = strlen(str) + 1;
    dup_str = (char *)malloc(size);
    memset(dup_str, 0, size);
    strncpy(dup_str, str, strlen(str));
    dup_str[size - 1] = '\0';
    return dup_str;
}

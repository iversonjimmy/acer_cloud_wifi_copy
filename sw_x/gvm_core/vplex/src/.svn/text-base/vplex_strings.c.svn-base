//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_strings.h"

int VPLString_FindLastInstance(char charToFind, char const* str)
{
    int last = -1;
    int currPos = 0;
    for(;
        str[currPos] != '\0';
        ++currPos)
    {
        if(str[currPos] == charToFind)
        {
            last = currPos;
        }
    }
    return last;
}

void VPLString_SafeStrncpy(char* dest,  size_t destBufferSize, char const* src) {
    if(destBufferSize > 0) {

#if defined(NDEBUG) || defined(VPL_NO_ASSERT)
        // Don't do overlap checks.
#else // !defined(NDEBUG) && !defined(VPL_NO_ASSERT)
        // Quick check for overlap
        size_t const p1 = PTR_TO_SIZE_T(dest);
        size_t const p2 = PTR_TO_SIZE_T(src);
        ASSERT((p1 < p2) == (p1+destBufferSize < p2));
#endif // !defined(NDEBUG) && !defined(VPL_NO_ASSERT)

        ASSERT_NOT_NULL(dest);
        ASSERT_NOT_NULL(src);

#ifdef _WIN32
#ifdef _MSC_VER
    #pragma warning( push )
    // We explicitly add the null-terminator, so strncpy_s is not needed.
    #pragma warning( disable : 4996 )
#endif
#endif

        // TODO: use strlcpy if supported?
        (IGNORE_RESULT)strncpy(dest, src, (destBufferSize-1));
        dest[destBufferSize-1] = '\0';

#ifdef _WIN32
#ifdef _MSC_VER
    #pragma warning( pop )
#endif
#endif

    }
}

char* VPLString_MallocAndSprintf(char const* formatMsg, ...)
{
    size_t requiredSize;
    char* newBuffer;
    va_list args;
    va_start(args, formatMsg);
    requiredSize = 1 + VPL_vsnprintf(NULL, 0, formatMsg, args);
    newBuffer = (char*)malloc(requiredSize);
    va_end(args);
    if (newBuffer != NULL) {
        va_start(args, formatMsg);
        vsnprintf(newBuffer, requiredSize, formatMsg, args);
        va_end(args);
    }
    return newBuffer;
}

int VPLString_GetIndexOfLastSlash(const char* string)
{
    int rv = -1;
    int i = 0;
    while (string[i] != '\0') {
        if ((string[i] == '/') || (string[i] == '\\')) {
            rv = i;
        }
        i++;
    }
    return rv;
}

char* VPLString_strnstr(const char* haystack, const char* needle, size_t n)
{
    size_t needleLen = strlen(needle);
    if (needleLen <= n) {
        size_t maxPos = n - needleLen;
        size_t i;
        for (i = 0; i <= maxPos; i++) {
            if (strncmp(&haystack[i], needle, needleLen) == 0) {
                return (char*)&haystack[i];
            }
        }
    }
    return NULL;
}


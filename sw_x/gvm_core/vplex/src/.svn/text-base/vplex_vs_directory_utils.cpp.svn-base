#include "vplex_vs_directory_utils.hpp"
#include "vplex_private.h"

#include <stdlib.h>
#include <errno.h>

VPL_BOOL
VPLVSDirectoryUtil_ParseContentId(const char* str, u32* contentId_out)
{
    *contentId_out = VPL_strToU32(str, NULL, 16);
    if (errno != 0) {
        VPL_LIB_LOG_WARN(VPL_SG_VS, "Failed to parse content id \"%s\": %s", str, strerror(errno));
        return VPL_FALSE;
    }
    return VPL_TRUE;
}

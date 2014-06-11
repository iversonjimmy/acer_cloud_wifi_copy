#include "vpl_plat.h"
#include "vplu.h"

#include <Shlobj.h>
#include <string>


void _VPL__ShellChangeNotify(const char* path)
{
    wchar_t *wcsPath = NULL;

    if (_VPL__utf8_to_wstring(path, &wcsPath) == VPL_OK) {
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT,
            wcsPath, NULL);
    }

    if (wcsPath != NULL) {
        free(wcsPath);
    } 
}

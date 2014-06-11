/*
*                Copyright (C) 2005, BroadOn Communications Corp.
*
*   These coded instructions, statements, and computer programs contain
*   unpublished  proprietary information of BroadOn Communications Corp.,
*   and  are protected by Federal copyright law. They may not be disclosed
*   to  third  parties or copied or duplicated in any form, in whole or in
*   part, without the prior written consent of BroadOn Communications Corp.
*
*/

#include "vplex_plat.h"
#include "vplex_trace.h"
#include "vpl_th.h"
#include <assert.h>
#ifdef VPL_PLAT_IS_WINRT
#include <string>
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking;
#else
#include <winsock2.h>
#endif

#ifndef _WIN32
#   error "_WIN32 should be defined"
#endif

int VPL_GetLocalHostname(char *name, size_t len)
{
#ifdef VPL_PLAT_IS_WINRT
    int rv = VPL_ERR_FAIL;
    Windows::Foundation::Collections::IVectorView<HostName^>^ hostnames = Windows::Networking::Connectivity::NetworkInformation::GetHostNames();

    for (int i = 0; i < hostnames->Size; ++i) {  
        if (hostnames->GetAt(i)->Type == Windows::Networking::HostNameType::DomainName) {
            Platform::String ^psHostName = hostnames->GetAt(i)->DisplayName;
            std::wstring wstrHostName = std::wstring(psHostName->Begin(), psHostName->End());
            if (wstrHostName.length() > 0) {
                _VPL__wstring_to_utf8(wstrHostName.c_str(), wstrHostName.length(), name, len);
                rv = VPL_OK;
                break;
            }
        }
    }    
#else
    int rv = VPL_OK;
    int rc;

    assert(name != NULL);

    rc = gethostname(name, (int)len);

    if (rc == SOCKET_ERROR) {
        rv = VPLError_XlatSockErrno(WSAGetLastError());
    }
#endif
    return rv;
}

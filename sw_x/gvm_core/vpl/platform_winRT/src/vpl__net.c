/*
 *                Copyright (C) 2008, BroadOn Communications Corp.
 *
 *   These coded instructions, statements, and computer programs contain
 *   unpublished proprietary information of BroadOn Communications Corp.,
 *   and are protected by Federal copyright law. They may not be disclosed
 *   to third parties or copied or duplicated in any form, in whole or in
 *   part, without the prior written consent of BroadOn Communications Corp.
 *
 */
#include "vpl_net.h"
#include "vplu.h"
#include "vpl__socket_priv.h"
#include "vpl_lazy_init.h"
#include "vplu_mutex_autolock.hpp"

#include <string>

using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking;

static VPLLazyInitMutex_t g_GetAddrMutex = VPLLAZYINITMUTEX_INIT;

static int VPLNet_GetHostName(char *hostname_out)
{
    IVectorView<HostName^>^ hostnames = Windows::Networking::Connectivity::NetworkInformation::GetHostNames();

    for (int i = 0; i < hostnames->Size; ++i)
    {  
        if (hostnames->GetAt(i)->Type == Windows::Networking::HostNameType::DomainName) {
            Platform::String ^psHostName = hostnames->GetAt(i)->DisplayName;
            std::wstring wstrHostName = std::wstring(psHostName->Begin(), psHostName->End());
            if (wstrHostName.find_last_of(L".local") == std::wstring::npos && wstrHostName.find_last_of(L".local") != wstrHostName.length() - 6) {
                int nIndex = WideCharToMultiByte(CP_ACP, 0, wstrHostName.c_str(), -1, NULL, 0, NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, wstrHostName.c_str(), -1, hostname_out, nIndex, NULL, NULL);
                break;
            }
        }
    }

    return VPL_OK;
}

VPLNet_addr_t VPLNet_GetLocalAddr(void)
{
    VPLNet_addr_t ret = VPLNET_ADDR_INVALID;
    ConnectionProfile^ InternetConnectionProfile = NetworkInformation::GetInternetConnectionProfile();
    IVectorView<HostName^>^ hostnames = NetworkInformation::GetHostNames();

    for (int i = 0; i < hostnames->Size; ++i)
    { 
        HostName^ hostname = hostnames->GetAt(i);
        if (hostname->Type == Windows::Networking::HostNameType::Ipv4 
            && hostname->IPInformation->NetworkAdapter != nullptr &&
            hostname->IPInformation->NetworkAdapter->NetworkAdapterId == InternetConnectionProfile->NetworkAdapter->NetworkAdapterId) {
                ret =   _SocketData::StringToAddr(hostname->DisplayName);
        }
    }

    return ret;
}

int VPLNet_GetLocalAddrList(VPLNet_addr_t* pp_addrs,
                            VPLNet_addr_t* pp_netmasks) 
{
    int iCurrentIndex = 0;
    IVectorView<HostName^>^ hostnames;

    if( pp_addrs == NULL) {
        return VPLNET_ADDR_INVALID;
    }

    hostnames = Windows::Networking::Connectivity::NetworkInformation::GetHostNames();

    for (int i = 0; i < hostnames->Size && iCurrentIndex < VPLNET_MAX_INTERFACES ; ++i)
    {    
        if (hostnames->GetAt(i)->Type == Windows::Networking::HostNameType::Ipv4) {
            pp_addrs[iCurrentIndex] = _SocketData::StringToAddr(hostnames->GetAt(i)->DisplayName);
            iCurrentIndex++;
        }
        else if (hostnames->GetAt(i)->Type == Windows::Networking::HostNameType::Ipv6) {
            //currently not support
        }        
    }
    return VPL_OK;
}

VPLNet_addr_t VPLNet_GetAddr(const char* hostname)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&g_GetAddrMutex));
    VPLNet_addr_t rv = 0;
    wchar_t* hostnameW = NULL;

    if(hostname == NULL || strlen(hostname) == 0) {
        rv = VPLNET_ADDR_INVALID;
        goto done;
    }

    {
        HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        if( !completedEvent ) {
            rv = VPLNET_ADDR_INVALID;
            goto done;
        }
    
        _VPL__utf8_to_wstring(hostname,&hostnameW);
        try {
            auto getEndPointOP = DatagramSocket::GetEndpointPairsAsync(ref new HostName(ref new String(hostnameW)),"0");
            getEndPointOP->Completed = ref new AsyncOperationCompletedHandler<IVectorView<EndpointPair^>^>(
                [&rv,&completedEvent] (IAsyncOperation<IVectorView<EndpointPair^>^>^ op, AsyncStatus statue) {
                    try {
                        // return first IP if multiple IPs are resolved.
                        IVectorView<EndpointPair^>^ ipList = op->GetResults();
                        if( ipList->Size >= 1) {
                            EndpointPair^ ipInfo = ipList->GetAt(0);
                            rv = _SocketData::StringToAddr(ipInfo->RemoteHostName->RawName);
                        }
                        else {
                            rv = VPLNET_ADDR_INVALID;
                        }
                    }
                    catch (Exception^ exception) {
                        rv = VPLNET_ADDR_INVALID;
                    }
                    SetEvent(completedEvent);
                }
            );
        }
        catch (Exception^ exception) {
            rv = VPLNET_ADDR_INVALID;
        }
        WaitForSingleObjectEx(completedEvent, INFINITE, TRUE);
        CloseHandle(completedEvent);
    }

done:
    if(hostnameW)
        free(hostnameW);

    return rv;
}

const char* VPLNet_Ntop(const VPLNet_addr_t *src, char *dst, size_t cnt) {

    if(src == NULL) {
        return NULL;
    }

    if(dst == NULL) {
        return NULL;
    }

    if(cnt < 1) {
        return NULL;
    }

    Platform::String^ netAddr = _SocketData::AddrToString(*src);
    
    int nIndex = WideCharToMultiByte(CP_ACP, 0, netAddr->Data(), -1, NULL, 0, NULL, NULL);
    if( nIndex > cnt ) {
        return NULL;
    }
    WideCharToMultiByte(CP_ACP, 0, netAddr->Data(), -1, dst, nIndex, NULL, NULL);
    
    return dst;
}

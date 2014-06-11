#include "mdd_iface.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h.>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "IPHLPAPI.lib")

#include "mdd_mutex.h"
#include "mdd_utils.h"

#include "log.h"

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

int(*g_iface_changed_cb)(MDDIface_t iface, MDDIfaceState_t state, void *arg) = 0;
HANDLE g_NotificationHandle;

typedef struct _Cached_Iface {
    MDDIface_t iface;
    struct _Cached_Iface *next;
} Cached_Iface;

Cached_Iface *available_ifaces = NULL;
MDDMutex_t iface_list_mutex;

int MDDIface_Equal(MDDIface_t *a, MDDIface_t *b)
{
    if (a == NULL || b == NULL) {
        return MDD_FALSE;
    }

    if (a->ifindex != b->ifindex) {
        return MDD_FALSE;
    } else {
        if (a->addr.family == MDDNet_INET) {
            if (a->addr.family == b->addr.family &&
                    a->addr.ip.ipv4 == b->addr.ip.ipv4) {
                return MDD_TRUE;
            }
        } else {
            if (a->addr.family == b->addr.family &&
                    !memcmp(a->addr.ip.data, b->addr.ip.data, sizeof(a->addr.ip))) {
                return MDD_TRUE;
            }
        }
        return MDD_FALSE;
    }
}

int MDDIface_Listall(MDDIface_t **iflist, int *count)
{
    DWORD rv, ifsz;
    int i = 0, n = 0;
    IP_ADAPTER_ADDRESSES *ifalist, *ifa;
    MDDIface_t *outlist = NULL;
    char last_prefix = 0x00;

    if (iflist == NULL) {
        return MDD_ERROR;
    }

    // get all inet interfaces;
    ifalist = (IP_ADAPTER_ADDRESSES *)MALLOC(sizeof(*ifalist));
    ifsz = sizeof(*ifalist);
    rv = GetAdaptersAddresses(AF_UNSPEC,
        GAA_FLAG_INCLUDE_GATEWAYS, 0, ifalist, &ifsz);

    if(rv == ERROR_BUFFER_OVERFLOW) {
        FREE(ifalist);
        ifalist = (IP_ADAPTER_ADDRESSES *)MALLOC(ifsz);
        if(!ifalist)
            return MDD_ERROR;
        rv = GetAdaptersAddresses(AF_UNSPEC, 0, 0, ifalist, &ifsz);
    }

    if(rv != NO_ERROR) {
        FREE(ifalist);
        return MDD_ERROR;
    }

    for (ifa = ifalist; ifa != NULL; ifa = ifa->Next)
    {
        PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;

        if ( !(ifa->OperStatus & IfOperStatusUp) ||
             (ifa->Flags & IP_ADAPTER_RECEIVE_ONLY) ||
             (ifa->Flags & IP_ADAPTER_NO_MULTICAST) ||
             (ifa->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
           )
        {
            LOG_DEBUG("MDDIface got un-support interface, name: %s\n", ifa->AdapterName);
            continue;
        }

        // Convert each PIP_ADAPTER_UNICAST_ADDRESS to MDDIface_t
        pUnicast = ifa->FirstUnicastAddress;
        if (pUnicast != NULL)
        {
            for (i = 0; pUnicast != NULL; i++)
            {
                SOCKADDR *sap = pUnicast->Address.lpSockaddr;

                if ( ((SOCKADDR_IN *)sap)->sin_family == AF_INET ) // for IPv4 address
                {
                    IN_ADDR in = ((SOCKADDR_IN *)sap)->sin_addr;
                    // allocate memory space
                    if (n == 0)
                    {
                        outlist = (MDDIface_t *)malloc(sizeof(MDDIface_t));
                    } else {
                        outlist = (MDDIface_t *)realloc(outlist, sizeof(MDDIface_t) * (n + 1));
                    }
                    outlist[n].ifindex = ifa->IfIndex;
                    outlist[n].addr.family = MDDNet_INET;
                    outlist[n].addr.ip.ipv4 = in.s_addr;
                }
                else if ( ((SOCKADDR_IN *)sap)->sin_family == AF_INET6 ) // for IPv6 address
                {
                    IN6_ADDR in6 = ((SOCKADDR_IN6 *)sap)->sin6_addr;
                    // allocate memory space
                    if (n == 0)
                    {
                        outlist = (MDDIface_t *)malloc(sizeof(MDDIface_t));
                    } else {
                        outlist = (MDDIface_t *)realloc(outlist, sizeof(MDDIface_t) * (n + 1));
                    }
                    outlist[n].ifindex = ifa->Ipv6IfIndex;
                    outlist[n].addr.family = MDDNet_INET6;
                    memcpy (outlist[n].addr.ip.ipv6, in6.u.Byte, 16);
                }
                // fill network mask
                memset(outlist[n].netmask.ip.data, 0, 16);
                memset(outlist[n].netmask.ip.data, 0xff, (int)(pUnicast->OnLinkPrefixLength / 8));
                if (pUnicast->OnLinkPrefixLength % 8) {
                    last_prefix = 0x00;
                    switch ((pUnicast->OnLinkPrefixLength % 8) % 4) {
                        case 1:
                            last_prefix = 0x80;
                            break;
                        case 2:
                            last_prefix = 0xc0;
                            break;
                        case 3:
                            last_prefix = 0xe0;
                            break;
                        default:
                            break;
                    }
                    if ((pUnicast->OnLinkPrefixLength % 8) >= 4) {
                        last_prefix >>= 4;
                        last_prefix |= 0xf0;
                    }
                    outlist[n].netmask.ip.data[(int)(pUnicast->OnLinkPrefixLength / 8)] = last_prefix;
                }
                n++;

                pUnicast = pUnicast->Next;
            }
        }
    }

    if (ifalist)
        FREE(ifalist);

    *iflist = outlist;
    *count = n;

    return MDD_OK;
}

void WINAPI unicastIpAddressChanged(void* callerContext, PMIB_UNICASTIPADDRESS_ROW row, MIB_NOTIFICATION_TYPE type)
{
    void *arg = callerContext;
    char last_prefix = 0x00;
    MDDIfaceState_t state = MDDIfaceState_Remove;
    MDDIface_t iface;

    if (row->Address.si_family != AF_INET && row->Address.si_family != AF_INET6)
    {
        return;
    }

    MDDMutex_Lock(&iface_list_mutex);

    iface.ifindex = row->InterfaceIndex;
    // fill ip address
    if (row->Address.si_family == AF_INET)
    {
        iface.addr.family = MDDNet_INET;
        iface.addr.ip.ipv4 = row->Address.Ipv4.sin_addr.S_un.S_addr;
        iface.ifindex = row->InterfaceIndex;

        iface.netmask.family = MDDNet_INET;
    }
    else if (row->Address.si_family == AF_INET6)
    {
        iface.addr.family = MDDNet_INET6;
        memcpy (iface.addr.ip.ipv6, row->Address.Ipv6.sin6_addr.u.Byte, 16);
        iface.ifindex = row->InterfaceIndex;

        iface.netmask.family = MDDNet_INET6;
    }

    // fill network mask
    memset(iface.netmask.ip.data, 0, 16);
    memset(iface.netmask.ip.data, 0xff, (int)(row->OnLinkPrefixLength / 8));
    if (row->OnLinkPrefixLength % 8)
    {
        last_prefix = 0x00;
        switch ((row->OnLinkPrefixLength % 8) % 4)
        {
            case 1:
                last_prefix = 0x80;
                break;
            case 2:
                last_prefix = 0xc0;
                break;
            case 3:
                last_prefix = 0xe0;
                break;
            default:
                break;
        }
        if ((row->OnLinkPrefixLength % 8) >= 4)
        {
            last_prefix >>= 4;
            last_prefix |= 0xf0;
        }
        iface.netmask.ip.data[(int)(row->OnLinkPrefixLength / 8)] = last_prefix;
    }

    // By interface driver implementation, some interface will not report add/delete, only parameter change
    // So it's necessary to implement parameter change and compare the address with current list
    if (type == MibParameterNotification)
    {
        BOOL exist = FALSE;
        Cached_Iface *iface_ptr, *new_iface, *previous_ptr;

        DWORD rv, ifsz;
        int i = 0;
        IP_ADAPTER_ADDRESSES *ifalist, *ifa;

        previous_ptr = NULL;
        iface_ptr = available_ifaces;
        while (iface_ptr != NULL)
        {
            if (MDDIface_Equal(&iface_ptr->iface, &iface))
            {
                exist = TRUE;
                break;
            }

            previous_ptr = iface_ptr;
            iface_ptr = iface_ptr->next;
        }

        // get all inet interfaces;
        ifalist = (IP_ADAPTER_ADDRESSES *)MALLOC(sizeof(*ifalist));
        ifsz = sizeof(*ifalist);
        
        rv = GetAdaptersAddresses(row->Address.si_family,
            GAA_FLAG_INCLUDE_GATEWAYS, 0, ifalist, &ifsz);

        if (rv == ERROR_BUFFER_OVERFLOW) {
            FREE(ifalist);
            ifalist = (IP_ADAPTER_ADDRESSES *)MALLOC(ifsz);
            if (!ifalist)
            {
                MDDMutex_Unlock(&iface_list_mutex);
                return;
            }
            rv = GetAdaptersAddresses(row->Address.si_family, 0, 0, ifalist, &ifsz);
        }

        if (rv != NO_ERROR)
        {
            FREE(ifalist);
            MDDMutex_Unlock(&iface_list_mutex);
            return ;
        }

        for (ifa = ifalist; ifa != NULL; ifa = ifa->Next)
        {
            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;

            if ( !(ifa->OperStatus & IfOperStatusUp) ||
                 (ifa->Flags & IP_ADAPTER_RECEIVE_ONLY) ||
                 (ifa->Flags & IP_ADAPTER_NO_MULTICAST) ||
                 (ifa->IfType == IF_TYPE_SOFTWARE_LOOPBACK) ||
                 (ifa->IfIndex != row->InterfaceIndex)
               )
            {
                continue;
            }

            // Convert each PIP_ADAPTER_UNICAST_ADDRESS to MDDIface_t
            pUnicast = ifa->FirstUnicastAddress;
            if (pUnicast != NULL)
            {
                for (i = 0; pUnicast != NULL; i++)
                {
                    SOCKADDR *sap = pUnicast->Address.lpSockaddr;

                    if ( ((SOCKADDR_IN *)sap)->sin_family == AF_INET ) // for IPv4 address
                    {
                        IN_ADDR in = ((SOCKADDR_IN *)sap)->sin_addr;
                        if (row->Address.Ipv4.sin_addr.S_un.S_addr == in.S_un.S_addr)
                        {
                            state = MDDIfaceState_Add;
                            break;
                        }
                    }
                    else if ( ((SOCKADDR_IN *)sap)->sin_family == AF_INET6 ) // for IPv6 address
                    {
                        IN6_ADDR in6 = ((SOCKADDR_IN6 *)sap)->sin6_addr;
                        if (strcmp((char*)in6.u.Byte, (char*)row->Address.Ipv6.sin6_addr.u.Byte) == 0)
                        {
                            state = MDDIfaceState_Add;
                            break;
                        }
                    }
                    pUnicast = pUnicast->Next;
                }
            }
        }

        if (ifalist)
            FREE(ifalist);

        if (exist == FALSE && state == MDDIfaceState_Add)
        {
            new_iface = (Cached_Iface *)malloc(sizeof(Cached_Iface));
            new_iface->iface = iface;
            new_iface->next = NULL;

            if (available_ifaces == NULL)
            {
                available_ifaces = new_iface;
            }
            else
            {
                new_iface->next = available_ifaces;
                available_ifaces = new_iface;
            }
        }
        else if (exist == TRUE && state == MDDIfaceState_Remove)
        {
            // Free this Cached_Iface
            if (iface_ptr == available_ifaces)
            {
                available_ifaces = iface_ptr->next;
            }
            else
            {
                if (previous_ptr != NULL)
                {
                    previous_ptr->next = iface_ptr->next;
                }
            }
            free(iface_ptr);
        } else {
            MDDMutex_Unlock(&iface_list_mutex);
            return;
        }
    }
    else if (type == MibAddInstance)
    {
        DWORD rv, ifsz;
        int i = 0;
        IP_ADAPTER_ADDRESSES *ifalist, *ifa;
        PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;

        BOOL exist = FALSE;
        Cached_Iface *new_iface, *iface_ptr;

        // get all inet interfaces;
        ifalist = (IP_ADAPTER_ADDRESSES *)MALLOC(sizeof(*ifalist));
        ifsz = sizeof(*ifalist);

        rv = GetAdaptersAddresses(row->Address.si_family,
            GAA_FLAG_INCLUDE_GATEWAYS, 0, ifalist, &ifsz);

        if (rv == ERROR_BUFFER_OVERFLOW) {
            FREE(ifalist);
            ifalist = (IP_ADAPTER_ADDRESSES *)MALLOC(ifsz);
            if (!ifalist)
            {
                MDDMutex_Unlock(&iface_list_mutex);
                return;
            }
            rv = GetAdaptersAddresses(row->Address.si_family, 0, 0, ifalist, &ifsz);
        }

        if (rv != NO_ERROR)
        {
            FREE(ifalist);
            MDDMutex_Unlock(&iface_list_mutex);
            return ;
        }

        for (ifa = ifalist; ifa != NULL; ifa = ifa->Next)
        {
            if ( !(ifa->OperStatus & IfOperStatusUp) ||
                 (ifa->Flags & IP_ADAPTER_RECEIVE_ONLY) ||
                 (ifa->Flags & IP_ADAPTER_NO_MULTICAST) ||
                 (ifa->IfType == IF_TYPE_SOFTWARE_LOOPBACK) ||
                 (ifa->IfIndex != row->InterfaceIndex)
               )
            {
                continue;
            }

            pUnicast = ifa->FirstUnicastAddress;
        }

        if (pUnicast == NULL)
        {
            // Not available interface, filter and return directly
            if (ifalist)
                FREE(ifalist);
            MDDMutex_Unlock(&iface_list_mutex);
            return;
        }

        if (ifalist)
            FREE(ifalist);

        iface_ptr = available_ifaces;
        while (iface_ptr != NULL)
        {
            if (MDDIface_Equal(&iface_ptr->iface, &iface))
            {
                exist = TRUE;
                break;
            }

            iface_ptr = iface_ptr->next;
        }

        if (exist == FALSE)
        {
            new_iface = (Cached_Iface *)malloc(sizeof(Cached_Iface));
            new_iface->iface = iface;
            new_iface->next = NULL;

            if (available_ifaces == NULL)
            {
                available_ifaces = new_iface;
            }
            else
            {
                new_iface->next = available_ifaces;
                available_ifaces = new_iface;
            }
        }

        state = MDDIfaceState_Add;
    }
    else if (type == MibDeleteInstance)
    {
        BOOL exist = FALSE;
        Cached_Iface *iface_ptr, *previous_ptr;

        previous_ptr = NULL;
        iface_ptr = available_ifaces;
        while (iface_ptr != NULL)
        {
            if (MDDIface_Equal(&iface_ptr->iface, &iface))
            {
                exist = TRUE;

                // Free this Cached_Iface
                if (iface_ptr == available_ifaces)
                {
                    available_ifaces = iface_ptr->next;
                }
                else
                {
                    if (previous_ptr != NULL)
                    {
                        previous_ptr->next = iface_ptr->next;
                    }
                }
                free(iface_ptr);
                break;
            }

            previous_ptr = iface_ptr;
            iface_ptr = iface_ptr->next;
        }

        // The removed interface is not available, return
        if (exist == FALSE)
        {
            MDDMutex_Unlock(&iface_list_mutex);
            return;
        }

        state = MDDIfaceState_Remove;
    }

    if (g_iface_changed_cb(iface, state, arg) != MDD_OK) {
        // Unregister if callback fail
        CancelMibChangeNotify2(g_NotificationHandle);
        MDDMutex_Unlock(&iface_list_mutex);
        MDDMutex_Destroy(&iface_list_mutex);
    } else {
        MDDMutex_Unlock(&iface_list_mutex);
    }
}

void Cache_Available_Iface_List()
{
    MDDIface_t *ifacelist = NULL;
    int i = 0;
    int iface_count = 0;

    MDDIface_Listall(&ifacelist, &iface_count);

    MDDMutex_Lock(&iface_list_mutex);
    for (i = 0; i < iface_count; i++) {
        Cached_Iface *new_iface = NULL;
        new_iface = (Cached_Iface *)malloc(sizeof(Cached_Iface));
        new_iface->iface = ifacelist[i];
        new_iface->next = NULL;

        if (available_ifaces == NULL)
        {
            available_ifaces = new_iface;
        }
        else
        {
            new_iface->next = available_ifaces;
            available_ifaces = new_iface;
        }
    }
    MDDMutex_Unlock(&iface_list_mutex);
    free(ifacelist);
}

int MDDIface_Monitoriface(int(*iface_changed)(MDDIface_t iface, MDDIfaceState_t state, void *arg), void *arg)
{
    if (iface_changed == NULL)
    {
        return MDD_ERROR;
    }

    MDDMutex_Init(&iface_list_mutex);

    Cache_Available_Iface_List();

    g_iface_changed_cb = iface_changed;
    
    if (NotifyUnicastIpAddressChange (AF_UNSPEC,
        (PUNICAST_IPADDRESS_CHANGE_CALLBACK)unicastIpAddressChanged,
        arg,
        FALSE,
        &g_NotificationHandle) != NO_ERROR)
    {
        return MDD_ERROR;
    }

    return MDD_OK;
}

MDDIfaceType_t MDDIface_Gettype(MDDIface_t iface)
{
    DWORD rv, ifsz;
    int i = 0, n = 0;
    IP_ADAPTER_ADDRESSES *ifalist, *ifa;
    MDDIface_t *outlist = NULL;
    char last_prefix = 0x00;

    MDDIfaceType_t iftype = MDDIfaceType_Unknown;

    // get all inet interfaces;
    ifalist = (IP_ADAPTER_ADDRESSES *)MALLOC(sizeof(*ifalist));
    ifsz = sizeof(*ifalist);
    rv = GetAdaptersAddresses(AF_UNSPEC,
        GAA_FLAG_INCLUDE_GATEWAYS, 0, ifalist, &ifsz);

    if (rv == ERROR_BUFFER_OVERFLOW) {
        FREE(ifalist);
        ifalist = (IP_ADAPTER_ADDRESSES *)MALLOC(ifsz);
        if(!ifalist)
            return MDDIfaceType_Unknown;
        rv = GetAdaptersAddresses(AF_INET, 0, 0, ifalist, &ifsz);
    }

    if (rv != NO_ERROR) {
        FREE(ifalist);
        return MDDIfaceType_Unknown;
    }

    for (ifa = ifalist; ifa != NULL; ifa = ifa->Next)
    {
        if (ifa->IfIndex == iface.ifindex)
        {
            if (ifa->IfType == IF_TYPE_ETHERNET_CSMACD)
            {
                iftype = MDDIfaceType_Ethernet;
                break;
            }
            else if (ifa->IfType == IF_TYPE_IEEE80211)
            {
                iftype = MDDIfaceType_Wifi;
                break;
            }
            // TODO: Add USB3 code
        }
    }

    if (ifalist)
        FREE(ifalist);

    return iftype;
}

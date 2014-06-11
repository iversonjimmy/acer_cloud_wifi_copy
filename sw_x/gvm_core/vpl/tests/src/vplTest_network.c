//
//  Copyright (C) 2007-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include <vpl_net.h>
#include <vpl_fs.h>
#include "vplTest.h"

static void testInvalidParameters(void)
{
    VPLNet_addr_t pp_netmasks[VPLNET_MAX_INTERFACES];
    char dst[256];

    VPLNet_addr_t addr = VPLNet_GetAddr("badHostName");
    if (addr != VPLNET_ADDR_INVALID) {
        VPLTEST_FAIL("VPLNet_GetAddr(\"badHostName\")");
    }

    addr = VPLNet_GetAddr(NULL);
    if (addr != VPLNET_ADDR_INVALID) {
        VPLTEST_FAIL("VPLNet_GetAddr(NULL)");
    }

    VPLTEST_CALL_AND_CHK_RV(VPLNet_GetLocalAddrList(NULL, pp_netmasks),
            (int)VPLNET_ADDR_INVALID);

    {
        VPLNet_addr_t localAddr = VPLNet_GetLocalAddr();
        if (localAddr == VPLNET_ADDR_INVALID) {
            VPLTEST_FAIL("VPLNet_GetLocalAddr");
        }
        if (VPLNet_Ntop(NULL, dst, 256) != NULL) {
            VPLTEST_FAIL("VPLNet_Ntop(NULL, dst, 256)");
        }
        if (VPLNet_Ntop(&localAddr, NULL, 256) != NULL) {
            VPLTEST_FAIL("VPLNet_Ntop(&localAddr, NULL, 256)");
        }
        if (VPLNet_Ntop(&localAddr, dst, 0) != NULL) {
            VPLTEST_FAIL("VPLNet_Ntop(&localAddr, dst, 0)");
        }
    }
}

static void testByteOrderConversion(void)
{
    // If we translate an address from local/host byte-ordering to
    // network byte-ordering, then back to local/host byte-ordering,
    // we should get the old address back.

    VPLNet_port_t localPort = 1234;
    VPLNet_port_t netPort = VPLNet_port_hton(localPort);
    VPLNet_port_t resultPort = VPLNet_port_ntoh(netPort);
    if (resultPort != localPort) {
        VPLTEST_FAIL("VPLNet_port_hton and VPLNet_port_ntoh");
    }
}

static void testGetLocalAddr(void)
{
    VPLNet_addr_t localAddr = VPLNet_GetLocalAddr();
    if (localAddr == VPLNET_ADDR_INVALID) {
        VPLTEST_FAIL("VPLNet_GetLocalAddr");
    }
    else {
        VPLTEST_LOG("VPLNet_GetLocalAddr(): "FMT_VPLNet_addr_t,
            VAL_VPLNet_addr_t(localAddr));
    }
}

#ifndef VPL_PLAT_IS_WINRT
static void testGetDefaultGateway(void)
{
    VPLNet_addr_t gateway = VPLNet_GetDefaultGateway();
    if (gateway == VPLNET_ADDR_INVALID) {
        VPLTEST_FAIL("VPLNet_GetDefaultGateway");
    }
    else {
        VPLTEST_LOG("VPLNet_GetDefaultGateway(): "FMT_VPLNet_addr_t,
            VAL_VPLNet_addr_t(gateway));
    }
}
#endif

static void testGetLocalAddrList(void)
{
    int i;
    VPLNet_addr_t pp_addrs[VPLNET_MAX_INTERFACES];
    VPLNet_addr_t pp_netmasks[VPLNET_MAX_INTERFACES];
    int rv = VPLNet_GetLocalAddrList(pp_addrs, pp_netmasks);
    VPLTEST_ENSURE_GREATER_OR_EQ(rv, 0, "%d", "VPLNet_GetLocalAddrList");
    for (i = 0; i < rv; i++) {
        VPLTEST_LOG("Addr = "FMT_VPLNet_addr_t", Netmask = "FMT_VPLNet_addr_t,
                VAL_VPLNet_addr_t(pp_addrs[i]), VAL_VPLNet_addr_t(pp_netmasks[i]));
    }
}

static void testGetAddr(void)
{
    // bug 6947:
    //  www.broadon.com is no longer in service, change to www.acer.com
    VPLNet_addr_t addr = VPLNet_GetAddr("www.acer.com");
    if (addr == VPLNET_ADDR_INVALID) {
        VPLTEST_FAIL("VPLNet_GetAddr");
    }
    else {
        VPLTEST_LOG("VPLNet_GetAddr(\"www.acer.com\"): "FMT_VPLNet_addr_t,
                VAL_VPLNet_addr_t(addr));
    }
}

static void testNtop(void)
{
    char dst[256];

    VPLNet_addr_t localAddr = VPLNet_GetLocalAddr();
    if (localAddr == VPLNET_ADDR_INVALID) {
        VPLTEST_FAIL("VPLNet_GetLocalAddr");
    }

    if (VPLNet_Ntop(&localAddr, dst, 256) != dst) {
        VPLTEST_FAIL("VPLNet_Ntop");
    }
    else {
        VPLTEST_LOG("VPLNet_GetLocalAddr(): %s", dst);
    }
}

void testVPLNetwork(void)
{
    VPLTEST_LOG("testInvalidParameters");
    testInvalidParameters();

    VPLTEST_LOG("testByteOrderConversion");
    testByteOrderConversion();

    VPLTEST_LOG("testGetLocalAddr");
    testGetLocalAddr();
#ifndef VPL_PLAT_IS_WINRT
    VPLTEST_LOG("testGetDefaultGateway");
    testGetDefaultGateway();
#endif
    VPLTEST_LOG("testGetLocalAddrList");
    testGetLocalAddrList();

    VPLTEST_LOG("testGetAddr");
    testGetAddr();

    VPLTEST_LOG("testNtop");
    testNtop();
}

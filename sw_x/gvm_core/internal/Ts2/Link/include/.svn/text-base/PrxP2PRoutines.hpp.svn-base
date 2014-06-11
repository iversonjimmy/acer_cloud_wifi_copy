/*
 *  Copyright 2013 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

#ifndef __TS2_LINK_PRXP2PROUTINES_HPP__
#define __TS2_LINK_PRXP2PROUTINES_HPP__

#include <sstream>

// No Ipv6 supported
#define PXD_ADDR_2_VPL_ADDR(p,v)                                           \
    do {                                                                   \
        u32 address = 0;                                                   \
        for (int n = 3 ; n >= 0 ; n--) {                                   \
            address = address << 8 | (p.ip_address[n] & 0x000000ff);       \
        }                                                                  \
        v.addr = address;                                                  \
        v.port = VPLNet_port_hton(p.port);                                 \
        v.family = VPL_PF_INET;                                            \
    } while(0)

#define PXD_ADDR_2_CSTR(p)                                                 \
    static_cast<std::ostringstream &>(                                     \
    std::ostringstream() << (p.ip_address[0] & 0xff) << "." <<             \
    (p.ip_address[1] & 0xff) << "." <<                                     \
    (p.ip_address[2] & 0xff) << "." <<                                     \
    (p.ip_address[3] & 0xff) << ":" <<                                     \
    p.port).str().c_str()

#endif /* __TS2_LINK_PRXP2PROUTINES_HPP__ */

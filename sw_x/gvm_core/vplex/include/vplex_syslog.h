//
//               Copyright (C) 2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and  are protected by Federal copyright law. They may not be disclosed
//  to  third  parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//
//

#ifndef __VPLEX_SYSLOG_H__
#define __VPLEX_SYSLOG_H__

#include "vplex_plat.h"

#ifdef __cplusplus
extern "C" {
#endif

/// As per RFC 3164, max syslog message is 1024 bytes.
#define VPLSYSLOG_MAX_MSG_LEN  ((size_t)(1024))

#define VPLSYSLOG_PORT  514

/**
 * From RFC: The Priority value is calculated by first multiplying the Facility
 * number by 8 and then adding the numerical value of the Severity.
 */
#define VPLSYSLOG_FACILITY_USER  1
#define VPLSYSLOG_SEVERITY_WARN  4
#define VPLSYSLOG_PRIORITY  ((VPLSYSLOG_FACILITY_USER * 8) + VPLSYSLOG_SEVERITY_WARN)

/// Can be either syslog implementation or shim.
#ifdef VPL_SYSLOG_ENABLED

    /// Set the host that logging messages will be sent to, and also connect to it.
    /// To allow a remote system to write to syslog on your server:
    /// - Modify /etc/sysconfig/sysconfig to add -r and -x to the syslogd options:
    ///   e.g., SYSLOGD_OPTIONS="-m 0 -r -x"
    /// - Restart syslogd: /etc/init.d/syslogd restart
    /// @note Sockets must be initialized before calling this.
    int VPLSyslog_SetServer(char const* hostname);

    /// Log a message to the host that was specified via #_SHR_set_syslog_server().
    /// \a msg must be cache-line aligned
    int VPLSyslog(char const* msg, size_t msglen);

#else

#define VPLSyslog_SetServer(hostname)  (VPL_ERR_DISABLED)

#define VPLSyslog(msg, msglen)  (VPL_ERR_DISABLED)

#endif

#ifdef  __cplusplus
}
#endif

#endif // include guard

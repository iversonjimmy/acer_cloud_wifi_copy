#ifndef __VPLEX_SOAP_PRIV_H__
#define __VPLEX_SOAP_PRIV_H__

#include "vplex_soap.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VPLSoapProxy {
    
    VPLSoapProtocol protocol;
    const char* serverHostname;
    u16 serverPort;
    const char* serviceName;
    const char* authRequesterName;
    const char* authRequesterSecret;

} VPLSoapProxy;


/// @note None of the string buffers are copied.  You must make sure that the memory stays valid
///     until you are done with the #VPLSoapProxy.  You are allowed to change the serverHostname
///     string as long as you ensure (via a mutex for example) that nothing is using \a proxy
///     concurrently with the change.
//%     CCD relies on this behavior, so don't change it without updating CCD first.
int VPLSoapProxy_Init(
        VPLSoapProxy* proxy,
        VPLSoapProtocol protocol,
        const char* serverHostname,
        u16 serverPort,
        const char* serviceName,
        const char* authRequesterName,
        const char* authRequesterSecret);


int VPLSoapProxy_Cleanup(VPLSoapProxy* proxy);

#ifdef  __cplusplus
}
#endif

#endif // include guard

#include "DxRemoteCCDAgent.hpp"
#include <vplex_file.h>
#include <vplex_socket.h>
#include "log.h"

#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
#include <ccd_core.h>
#include <ccd_core_service.hpp>
#include <ByteArrayProtoChannel.h>
#elif defined(WIN32)
#include <ccdi_client_named_socket.hpp>
#include <ccdi_client.hpp>
#include "dx_remote_agent_util_win.h"
#elif defined(LINUX)
#include <ccdi_client_named_socket.hpp>
#include <ccdi_client.hpp>
#else
#endif

DxRemoteCCDAgent::DxRemoteCCDAgent(VPLSocket_t skt, char *buf, uint32_t pktSize) :
    IDxRemoteAgent(skt, buf, pktSize)
{
}

DxRemoteCCDAgent::~DxRemoteCCDAgent()
{
}

int DxRemoteCCDAgent::doAction()
{
    return handleProtoBuf(recvBuf, recvBufSize);
}

int DxRemoteCCDAgent::handleProtoBuf(char *requestBuf, int size)
{
    int rv = 0;
    int rc;

    LOG_INFO("handleProtoBuf(buf=%p, size=%d)", requestBuf, size);
#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
    ccd::CCDIServiceImpl service;
    ByteArrayProtoChannel channel(requestBuf, size);
    service.CCDIService::handleRpc(channel,
                                   ccd::CCDProtobufRpcDebugGenericCallback,
                                   NULL,
                                   NULL);
    std::string ccd_response = channel.getOutputAsString();

    LOG_INFO("Start to send serialize response for "FMT_size_t" bytes.", ccd_response.size());
    rc = VPLSocket_Write(clienttcpsocket, ccd_response.data(), ccd_response.size(), VPLTime_FromSec(10));
    rv = -1; // close connection when done
#else
#if defined (WIN32)
    std::string ccdName = get_dx_ccd_name();
    if(!ccdName.empty() && is_sid(ccdName)) {
        ccdi::client::CCDIClient_SetOsUserId(get_dx_ccd_name().c_str());
    }
    // use port number as instance id to launch multiple ccd under same user env
    int testInstanceNum = (int)VPLNet_port_ntoh(VPLSocket_GetPort(clienttcpsocket));
    ccdi::client::CCDIClient_SetTestInstanceNum(testInstanceNum);
#elif defined(LINUX)
    // use port number as instance id to launch multiple ccd under same user env
    int testInstanceNum = (int)VPLNet_port_ntoh(VPLSocket_GetPort(clienttcpsocket));
    ccdi::client::CCDIClient_SetTestInstanceNum(testInstanceNum);
#endif
    
    VPLNamedSocketClient_t namedsock;
    int cfd = 0;
    char resBuf[igware::dxshell::DX_REMOTE_PKT_SIZE];
    int bytesSent = 0;
    rc = ccdi::client::CCDIClient_OpenNamedSocket(&namedsock);
    if(rc != 0) {
        LOG_INFO("Unable to connect to the CCDI named socket: %d", rc);
        rv = rc;
        goto exit;
    }

    cfd = VPLNamedSocketClient_GetFd(&namedsock);
    
    while(bytesSent < size) {
        rc = VPLFile_Write((VPLFile_handle_t)cfd, requestBuf + bytesSent, size - bytesSent);
        if(rc < 0) {
            LOG_ERROR("Error sending CCDI request: (%d)", rc);
            rv = rc;
            goto fail_ccdi;
        }
        else if(rc == 0) {
            LOG_ERROR("EOF sending CCDI request: (%d)", rc);
            goto fail_ccdi;
        }
        bytesSent += rc;
    }
    if(bytesSent != size) {
        LOG_ERROR("Truncated send for CCDI request: %d/%d", rc, size);
        goto fail_ccdi;
    }

    LOG_INFO("Start to send serialize response");
    do {
        rc = VPLFile_Read((VPLFile_handle_t)cfd, resBuf, igware::dxshell::DX_REMOTE_PKT_SIZE);
        if(rc < 0) {
            LOG_ERROR("Error receiving CCDI response: (%d)", rc);
            rv = rc;
            goto fail_ccdi;
        }
        if(rc == 0) {
            rv = -1; // close connection when done
            break;
        }

        bytesSent = VPLSocket_Write(clienttcpsocket, resBuf, rc, VPL_TIMEOUT_NONE);
        if(bytesSent < 0) {
            LOG_ERROR("Error resending CCDI response: (%d)", bytesSent);
            rv = bytesSent;
            goto fail_ccdi;
        }
        else if(rc != bytesSent) {
            LOG_ERROR("Truncated resend for CCDI response: %d/%d", bytesSent, rc);
            rv = -1;
            goto fail_ccdi;
        }
    } while(rc > 0);


    LOG_INFO("End send serialize response");

 fail_ccdi:
    VPLNamedSocketClient_Close(&namedsock);
    if(bytesSent == 0) {
        LOG_ERROR("No CCDI response sent.");
        rv = -1;
    }
 exit:
#endif

    LOG_DEBUG("handleProtoBuf End");
    return rv;
}


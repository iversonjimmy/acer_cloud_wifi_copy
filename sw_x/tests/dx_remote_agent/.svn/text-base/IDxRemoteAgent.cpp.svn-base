#include "IDxRemoteAgent.h"
#include "log.h"
#include <vplex_socket.h>
#include "vplu_types.h"
#include "vplu_format.h"

IDxRemoteAgent::IDxRemoteAgent(VPLSocket_t skt, char *buf, uint32_t pktSize) :
    clienttcpsocket(skt), 
    recvBuf(buf),
    recvBufSize(pktSize), 
    resSize(0), 
    sentSize(0)
{}

IDxRemoteAgent::~IDxRemoteAgent()
{
    delete[] recvBuf;
}

int IDxRemoteAgent::SendFinalData()
{
    int rv = false;
    int rc;

    resSize = response.size();
    resSize = VPLConv_hton_u32(resSize);
    rc = VPLSocket_Write(clienttcpsocket, &resSize, sizeof(resSize), VPL_TIMEOUT_NONE);
    if (rc < 0) {
        LOG_ERROR("Send response length failed: (%d).", rc);
        rv = rc;
        goto exit;
    }
    if (rc != sizeof(resSize)) {
        LOG_ERROR("Send response length truncated: %d/"FMT_size_t".", 
                  rc, sizeof(resSize));
        rv = -1;
        goto exit;
    }

    rc = VPLSocket_Write(clienttcpsocket, response.data(), response.size(), VPL_TIMEOUT_NONE);
    if (rc < 0) {
        LOG_ERROR("Send response length failed: (%d).", rc);
        rv = rc;
        goto exit;
    }
    if (rc != response.size()) {
        LOG_ERROR("Send response length truncated: %d/"FMT_size_t".", rc, response.size());
        rv = -1;
        goto exit;
    }

 exit:
    return rv;
}

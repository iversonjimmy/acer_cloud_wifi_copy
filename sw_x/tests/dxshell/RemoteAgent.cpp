#include "RemoteAgent.hpp"
#include <vplu_format.h>
#include <vpl_net.h>
#include <vplex_socket.h>
#include <vpl_conv.h>
#include <vpl_time.h>
#include <log.h>
#include <exception>

RemoteAgent::RemoteAgent(VPLNet_addr_t ipaddr, u16 port) :
    ipaddr(ipaddr),
    port(port),
    socket(VPLSOCKET_INVALID),
    sock_err(0)
{}

RemoteAgent::~RemoteAgent()
{
    if(!VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        VPLSocket_Close(socket);
        socket = VPLSOCKET_INVALID;
    }
}

int RemoteAgent::connectAgentSocket()
{
    int rv = 0;
    VPLSocket_addr_t sockAddr = { VPL_PF_INET, ipaddr, VPLNet_port_hton(port) };
    
    socket = VPLSocket_Create(sockAddr.family, VPLSOCKET_STREAM, VPL_FALSE);
    if(VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        LOG_ERROR("VPLSocket_Create failed!");
        rv = VPL_ERR_FAIL;
        goto exit;
    }
    
    rv = VPLSocket_ConnectWithTimeout(socket, &sockAddr, sizeof(sockAddr),
                                      VPLTime_FromSec(30));
    if(rv < 0) {
        LOG_ERROR("VPLSocket_ConnectWithTimeout("FMT_VPLNet_addr_t":"FMTu16") with 30s timeout failed: %d",
                  VAL_VPLNet_addr_t(ipaddr), port, rv);
        VPLSocket_Close(socket);
        socket = VPLSOCKET_INVALID;
    }
   
 exit:
    return rv;
}

int RemoteAgent::sendAgentSocket(const void* buf, size_t len)
{
    int rv = VPLSocket_Write(socket, buf, len, VPL_TIMEOUT_NONE);
    if(rv != len) {
        if(rv < 0) {
            LOG_ERROR("VPLSocket_Write failed: (%d)", rv);
        }
        else if(rv == 0) {
            LOG_ERROR("VPLSocket_Write detects lost connection.");
            rv = -1;
        }
        else if(rv != len) {
            LOG_ERROR("VPLSocket_Write sent %d/"FMT_size_t" bytes. Close on short-send.",
                      rv, len);
        }
        VPLSocket_Close(socket);
        socket = VPLSOCKET_INVALID;
    }
    else {
        rv = 0; // success
    }

    return rv;
}

int RemoteAgent::recvAgentSocket(void* buf, size_t len)
{
    int rv = VPLSocket_Read(socket, buf, len, VPL_TIMEOUT_NONE);
    if(rv != len) {
        if(rv < 0) {
            LOG_ERROR("VPLSocket_Read failed: (%d)", rv);
        }
        else if(rv == 0) {
            LOG_ERROR("VPLSocket_Read detects lost connection.");
            rv = -1;
        }
        else if(rv != len) {
            LOG_ERROR("VPLSocket_Read sent %d/"FMT_size_t" bytes. Close on short-receive.",
                      rv, len);
        }
        VPLSocket_Close(socket);
        socket = VPLSOCKET_INVALID;
    }
    else {
        rv = 0; // success
    }

    return rv;
}

int RemoteAgent::checkSocketConnect()
{
    int rv = 0;

    if(VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        rv = connectAgentSocket();
    }

    return rv;
}

int RemoteAgent::send(int command, const std::string &input, std::string &output)
{
    int rv = 0;
    char *buf = NULL;

    u8 commandByte;
    u32 inputDataSize;
    u32 res_size;

    output.clear();

    if(VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        rv = connectAgentSocket();
        if(rv != 0) {
            goto exit;
        }
    }

    commandByte = VPLConv_hton_u8(command);
    rv = sendAgentSocket(&commandByte, sizeof(commandByte));
    if(rv != 0) {
        goto exit;
    }

    inputDataSize = VPLConv_hton_u32(input.size());
    rv = sendAgentSocket(&inputDataSize, sizeof(inputDataSize));
    if(rv != 0) {
        goto exit;
    }

    rv = sendAgentSocket(input.data(), input.size());
    if(rv != 0) {
        goto exit;
    }

    rv = recvAgentSocket(&res_size, sizeof(res_size));
    if(rv != 0) {
        goto exit;
    }
    res_size = VPLConv_ntoh_u32(res_size);

    try {
        buf = new char[res_size];
        
        rv = recvAgentSocket(buf, res_size);
        if(rv != 0) {
            goto exit;
        }
        output.assign(buf, res_size);        
    } 
    catch(std::exception& e) {
        LOG_ERROR("Fatal exception receiving response: %s", e.what());
        
        rv = -1;
        goto exit;
    }

 exit:
    delete[] buf;

    return rv;
}

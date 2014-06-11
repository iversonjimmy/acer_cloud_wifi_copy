#include "DxRemoteAgentManager.hpp"
#include "vplu_types.h"
#include "vplu_format.h"
#include "vplex_socket.h"
#include "log.h"
#include <exception>

DxRemoteAgentManager::DxRemoteAgentManager(VPLSocket_t socket) :
    socket(socket)
{
}

DxRemoteAgentManager::~DxRemoteAgentManager()
{
    LOG_ALWAYS("Ended manager for socket "FMT_VPLSocket_t".",
               VAL_VPLSocket_t(socket));
    
    VPLSocket_Shutdown(socket, VPLSOCKET_SHUT_RDWR);
    VPLSocket_Close(socket);
}

void DxRemoteAgentManager::Run()
{
    int rc = 0;
    char type;
    igware::dxshell::RequestType reqType;
    int agentError;

    LOG_DEBUG("Started manager for socket "FMT_VPLSocket_t".",
               VAL_VPLSocket_t(socket));
    do {
        rc = VPLSocket_Recv(socket, &type, sizeof(type));
        if(rc == 0) {
            LOG_INFO("Socket "FMT_VPLSocket_t" disconnected.",
                     VAL_VPLSocket_t(socket));
            goto exit;
        }
        if(rc < 0) {
            LOG_ERROR("Recv failed for request type: (%d).", rc);
            goto exit;
        }
        if(rc != sizeof(type)) {
            LOG_ERROR("Received only %d/"FMT_size_t" bytes of request type.",
                      rc, sizeof(type));
            goto exit;
        }
        reqType = (igware::dxshell::RequestType)VPLConv_ntoh_u8(type);

        agentError = RunAgent(reqType);
    } while(!agentError);
    
 exit:
    return;
}

int DxRemoteAgentManager::RunAgent(igware::dxshell::RequestType reqType)
{
    int rv = 0;
    IDxRemoteAgent* myAgent = NULL;
    int pktsize = 0;
    char *payload = NULL;

    if(reqType == igware::dxshell::DX_REQUEST_PROTORPC) {
        pktsize = GetProtoRPCPacket(payload);
    }
    else {
        pktsize = GetBlobPacket(payload);
    }

    try {
        switch(reqType) {
        case igware::dxshell::DX_REQUEST_PROTORPC:
            LOG_ALWAYS("DX_REQUEST_PROTORPC, size=%d", pktsize);
            myAgent = new DxRemoteCCDAgent(socket, payload, pktsize);
            break;
            
        case igware::dxshell::DX_REQUEST_HTTP_GET:
            LOG_ALWAYS("DX_REQUEST_HTTP_GET");
            myAgent = new DxRemoteHTTPAgent(socket, payload, pktsize);
            break;
            
        case igware::dxshell::DX_REQUEST_QUERY_DEVICE:
            LOG_ALWAYS("DX_REQUEST_QUERY_DEVICE");
            myAgent = new DxRemoteQueryDeviceAgent(socket, payload, pktsize);
            break;
            
        case igware::dxshell::DX_REQUEST_DXREMOTE_PROTOCOL:
            LOG_ALWAYS("DX_REQUEST_DXREMOTE_PROTOCOL");
            myAgent = new DxRemoteOSAgent(socket, payload, pktsize);
            break;
            
#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
#else
        case igware::dxshell::DX_REQUEST_DXREMOTE_MSA:
            LOG_ALWAYS("DX_REQUEST_DXREMOTE_MSA");
            myAgent = new DxRemoteMSAAgent(socket, payload, pktsize);
            break;
#endif
            
        case igware::dxshell::DX_REQUEST_DXREMOTE_TRANSFER_FILES:
            LOG_ALWAYS("DX_REQUEST_DXREMOTE_TRANSFER_FILES");
            myAgent = new DxRemoteFileTransferAgent(socket, payload, pktsize);
            break;

        case igware::dxshell::DX_REQUEST_DXREMOTE_TS_TEST:
            LOG_ALWAYS("DX_REQUEST_DXREMOTE_TS_TEST");
            myAgent = new DxRemoteTSTestAgent(socket, payload, pktsize);
            break;

        default:
            LOG_ALWAYS("Request type %d unknown.", reqType);
            rv = -1;
            delete[] payload;
            goto exit;
            break;
        }
    }
    catch(std::exception& e) {
        delete[] payload;
        LOG_ERROR("Fatal exception creating agent: %s", e.what());
        rv = -1;
        goto exit;
    }
    
    try {
        rv = myAgent->doAction();
        LOG_INFO("doAction:(%d)", rv);
        if(rv == 0) {
            rv = myAgent->SendFinalData();
            LOG_DEBUG("SendFinalData:(%d)", rv);
        }
    }
    catch(std::exception& e) {
        LOG_ERROR("Fatal exception performing agent action: %s", e.what());
        rv = -1;
    }
    
    delete myAgent;
    
 exit:
    return rv;
}

int DxRemoteAgentManager::GetProtoRPCPacket(char*& packet)
{
    // Proto packet structure:
    // * header length (base 128 varint) no more than 246 bytes
    // * header, for header length bytes
    // * payload length (base 128 varint)
    // * payload for payload length bytes

    int rv = 0;
    int rc;
    int header_len, payload_len;
    std::string header_len_bytes, payload_len_bytes;
    char* header = NULL;
    char* payload = NULL;
    packet = NULL;

    header_len = GetProtoLength(header_len_bytes);
    if(header_len < 0 || header_len > 246) {
        LOG_ERROR("Get protorpc header length failed: (%d)", header_len);
        rv = -1;
        goto exit;
    }

    if(header_len > 0)  {
        header = new char[header_len];
        rc = VPLSocket_Read(socket, header, header_len, VPL_TIMEOUT_NONE);        
        if(rc < 0) {
            LOG_ERROR("Get protorpc header failed: (%d)", rc);
            rv = rc;
            goto exit;
        }
        if(rc != header_len) {
            LOG_ERROR("Get protorpc header short: %d/%d.",
                      rc, header_len);
            rv = -1;
            goto exit;
        }
    }

    payload_len = GetProtoLength(payload_len_bytes);
    if(payload_len < 0) {
        LOG_ERROR("Get protorpc payload length failed: (%d)", payload_len);
        rv = payload_len;
        goto exit;
    }

    if(payload_len > 0) {
        payload = new char[payload_len];
        rc = VPLSocket_Read(socket, payload, payload_len, VPL_TIMEOUT_NONE);        
        if(rc < 0) {
            LOG_ERROR("Get protorpc payload failed: (%d)", rc);
            rv = rc;
            goto exit;
        }
        if(rc != payload_len) {
            LOG_ERROR("Get protorpc payload short: %d/%d",
                      rc, payload_len);
            rv = -1;
            goto exit;
        }
    }

    packet = new char[header_len_bytes.size() +
                      payload_len_bytes.size() +
                      header_len + payload_len];
    memcpy(packet, header_len_bytes.data(), header_len_bytes.size());
    rv = header_len_bytes.size();
    if(header) {
        memcpy(packet + rv, header, header_len);
        rv += header_len;
    }
    memcpy(packet + rv, payload_len_bytes.data(), payload_len_bytes.size());
    rv += payload_len_bytes.size();
    if(payload) {
        memcpy(packet + rv, payload, payload_len);
        rv += payload_len;
    }

 exit:
    delete[] header;
    delete[] payload;
    return rv;
}

int DxRemoteAgentManager::GetProtoLength(std::string& raw_bytes)
{
    int rv = 0;
    int rc;
    char byte;
    
    do {
        rc = VPLSocket_Recv(socket, &byte, 1);
        if(rc < 0) {
            LOG_ERROR("Failed to receive protobuf message length: (%d).", rc);
            rv = rc;
            goto exit;
        }

        rv |= ((byte & 0x7f) << (7 * raw_bytes.size()));
        raw_bytes += byte;
    } while(byte & 0x80);

 exit:
    return rv;
}

int DxRemoteAgentManager::GetBlobPacket(char*& packet)
{
    int rv = 0;
    int length = 0;
    int rc;
    packet = NULL;
    
    // 4-bytes should be in the socket buffer without fail.
    rc = VPLSocket_Recv(socket, &length, sizeof(length));
    if(rc < 0) {
        LOG_ERROR("Failed to get blob size: (%d).", rc);
        rv = rc;
        goto exit;
    }
    else if(rc != sizeof(length)) {
        LOG_ERROR("Incomplete blob size. %d/"FMT_size_t" bytes received.",
                  rc, sizeof(length));
        rv = -1;
        goto exit;
    }
    length = VPLConv_ntoh_u32(length);    
    LOG_DEBUG("Blob size: %u", length);
    
    if(length > 0) {
        packet = new char[length];
        // Failure causes program crash on out of memory.
        
        LOG_DEBUG("Receive %u byte blob...", length);
        rc = VPLSocket_Read(socket, packet, length, VPL_TIMEOUT_NONE);        
        LOG_DEBUG("Received %d/%u bytes of blob.",
                  rc, length);
        if(rc != length) {
            LOG_ERROR("Blob incomplete: %u/%d received.",
                      length, rc);
            delete packet;
            packet = NULL;
            rv = -1;
            goto exit;
        }
    }
    rv = length;

 exit:
    return rv;
}

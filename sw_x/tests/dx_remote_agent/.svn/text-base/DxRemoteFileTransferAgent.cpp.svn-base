#include "DxRemoteFileTransferAgent.hpp"
#include <vplex_file.h>
#include <vplex_socket.h>
#include <vpl_fs.h>
#include "log.h"

#ifdef VPL_PLAT_IS_WINRT
#include "dx_remote_agent_util_winrt.h"
using namespace Windows::Storage;
#endif

DxRemoteFileTransferAgent::DxRemoteFileTransferAgent(VPLSocket_t skt, char *buf, uint32_t pktSize) : IDxRemoteAgent(skt, buf, pktSize)
{
}

DxRemoteFileTransferAgent::~DxRemoteFileTransferAgent()
{
}

int DxRemoteFileTransferAgent::doAction()
{
    int rv;
    igware::dxshell::DxRemoteFileTransfer myReq, myRes;

    LOG_DEBUG("DxRemoteFileTransferAgent::doAction() Start");

    myReq.ParseFromArray(this->recvBuf, this->recvBufSize);
    igware::dxshell::DxRemoteAgentFileTransfer_Type myType = myReq.type();
    myRes.set_type(myType);

    switch(myType) {
    case igware::dxshell::DX_REMOTE_PUSH_FILE:
        LOG_INFO("DX_REMOTE_PUSH_FILE \"%s\" size "FMTu64".",
                 myReq.path_on_agent().c_str(), myReq.file_size());
        rv = PushFile(myReq, myRes);
        break;

    case igware::dxshell::DX_REMOTE_GET_FILE:
        LOG_INFO("DX_REMOTE_GET_FILE \"%s\".",
                 myReq.path_on_agent().c_str());
        rv = GetFile(myReq, myRes);
        break;

    default:
        LOG_ERROR("Unknown file transfer type: %d", myType);
        rv = -1;
        break;
    }

    LOG_DEBUG("DxRemoteFileTransferAgent::doAction() End");

    return rv;
}

int DxRemoteFileTransferAgent::PushFile(igware::dxshell::DxRemoteFileTransfer &myReq, igware::dxshell::DxRemoteFileTransfer &myRes)
{
    int rv = VPL_OK;
    int rc;
    VPLFile_handle_t fHandle = VPLFILE_INVALID_HANDLE;
    const int flagDst = VPLFILE_OPENFLAG_CREATE |
                        VPLFILE_OPENFLAG_TRUNCATE |
                        VPLFILE_OPENFLAG_WRITEONLY;
    char tempChunkBuf[igware::dxshell::DX_REMOTE_FILE_TRANS_PKT_SIZE];

    std::string path_on_agent(myReq.path_on_agent());
    u64 totalSize = myReq.file_size();
    u64 recvSize = 0;
    u64 fileSize = 0;
    bool writeFail = false;

    fHandle = VPLFile_Open(path_on_agent.c_str(), flagDst, 0666);
    if(!VPLFile_IsValidHandle(fHandle)) {
        LOG_ERROR("Open \"%s\" failed: (%d)",
                  path_on_agent.c_str(), fHandle);
        rv = VPL_ERR_INVALID;
        myRes.set_vpl_return_code(rv);
    }
    else {
        myRes.set_vpl_return_code(VPL_OK);
    }

    LOG_INFO("Serialize vpl_return_code for FileTransferAgent: %d", myRes.vpl_return_code());

    rv = SendProtoSize(myRes.ByteSize());
    if(rv != VPL_OK) {
        goto exit;
    }
    
    rv = SendProtoResponse(myRes);
    if(rv != VPL_OK) {
        goto exit;
    }

    if(myRes.vpl_return_code() != VPL_OK) {
        // Operation already reported failed.
        goto exit;
    }

    // Must receive all contracted data even if file write fails.
    // If receive fails, truncate file.
    while(recvSize < totalSize) {
        size_t recvLimit = igware::dxshell::DX_REMOTE_FILE_TRANS_PKT_SIZE;
        if(recvLimit > (totalSize - recvSize)) {
            recvLimit = (size_t)(totalSize - recvSize);
        }
        int recvCnt = VPLSocket_Recv(clienttcpsocket, tempChunkBuf, recvLimit);
        if(recvCnt < 0) {
            LOG_ERROR("I/O error receiving file data: (%d)", recvCnt);
            rv = recvCnt;
            goto exit;
        }
        else if(recvCnt == 0) {
            LOG_ERROR("Connection lost receiving file data.");
            rv = VPL_ERR_IO;
            goto exit;
        }
        recvSize += recvCnt;

        if(!writeFail) {
            int writeCnt = 0;
            while(writeCnt < recvCnt) {
                rc = VPLFile_Write(fHandle, tempChunkBuf + writeCnt, recvCnt - writeCnt);
                if(rc < 0) {
                    LOG_ERROR("Disk I/O error writing file data: (%d)", rc);
                    writeFail = true;
                    break;
                }
                else {
                    writeCnt += rc;
                    fileSize += rc;
                }
            }
        }
    }

    fileSize = VPLConv_hton_u64(fileSize);
    rc = VPLSocket_Write(clienttcpsocket, &fileSize, sizeof(fileSize), VPL_TIMEOUT_NONE);
    if(rc < 0) {
        rv = rc;
        LOG_ERROR("I/O error sending received file size: (%d).", rv);        
    }
    else if(rc != sizeof(fileSize)) {
        LOG_ERROR("Short send of received file size. %d/"FMT_size_t".",
                  rc, sizeof(fileSize));
        rv = -1;
    }

 exit:
    if(VPLFile_IsValidHandle(fHandle)) {
        VPLFile_Close(fHandle);
    }
    
    return rv;
}

int DxRemoteFileTransferAgent::GetFile(igware::dxshell::DxRemoteFileTransfer &myReq, igware::dxshell::DxRemoteFileTransfer &myRes)
{
    int rv = VPL_OK;
    VPLFile_handle_t fHandle = VPLFILE_INVALID_HANDLE;
    char tempChunkBuf[igware::dxshell::DX_REMOTE_FILE_TRANS_PKT_SIZE];
    VPLFS_stat_t fStat;

    std::string path_on_agent(myReq.path_on_agent());
    u64 totalSize = 0;
    u64 sentSize = 0;
    u64 receivedSize = 0;

    rv = VPLFS_Stat(path_on_agent.c_str(), &fStat);
    if(rv != VPL_OK) {
        LOG_ERROR("Stat \"%s\" failed: (%d)",
                  path_on_agent.c_str(), rv);
        rv = VPL_ERR_INVALID;
        myRes.set_vpl_return_code(rv);
    }
    else {
        fHandle = VPLFile_Open(path_on_agent.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
        if(!VPLFile_IsValidHandle(fHandle)) {
            LOG_ERROR("Open \"%s\" failed: (%d)",
                      path_on_agent.c_str(), fHandle);
            rv = VPL_ERR_INVALID;
            myRes.set_vpl_return_code(rv);
        }
        else {
            totalSize = fStat.size;
            myRes.set_file_size(totalSize);
            myRes.set_vpl_return_code(VPL_OK);
        }
    }

    LOG_INFO("Serialize vpl_return_code for FileTransferAgent: %d", myRes.vpl_return_code());
    
    rv = SendProtoSize(myRes.ByteSize());
    if(rv != VPL_OK) {
        goto exit;
    }
    
    rv = SendProtoResponse(myRes);
    if(rv != VPL_OK) {
        goto exit;
    }
    
    if(myRes.vpl_return_code() != VPL_OK) {
        // Operation already reported failed.
        goto exit;
    }

    // Send file data. If file I/O error occurs, must drop connection. 
    // File will be partially written on other side.
    // Stop on I/O error.
    while(sentSize < totalSize) {
        size_t readLimit = sizeof(tempChunkBuf);
        if(readLimit > (totalSize - sentSize)) {
            readLimit = (size_t)(totalSize - sentSize);
        }
        int readBytes;
        readBytes = VPLFile_Read(fHandle, tempChunkBuf, readLimit);
        if(readBytes <= 0) {
            rv = readBytes;
            LOG_ERROR("Read from \"%s\" failed: (%d)",
                      path_on_agent.c_str(), rv);
            goto exit;
        }
        
        sentSize += readBytes;
        
        rv = VPLSocket_Write(clienttcpsocket, tempChunkBuf, readBytes, VPL_TIMEOUT_NONE);
        if(rv < 0) {
            LOG_ERROR("I/O error sending file data: (%d).", rv);
            goto exit;
        }
        else if(rv != readBytes) {
            LOG_ERROR("Incomplete send of file data: %d/%d bytes.",
                      rv, readBytes);
            rv = VPL_ERR_FAIL;
            goto exit;
        }
    }

    rv = VPLSocket_Read(clienttcpsocket, &receivedSize, sizeof(receivedSize), VPL_TIMEOUT_NONE);
    if(rv < 0) {
        LOG_ERROR("I/O error receiving sent size: (%d).", rv);
        goto exit;
    }
    else if(rv != sizeof(receivedSize)) {
        LOG_ERROR("Incomplete receive of received size: %d/"FMT_size_t" bytes.",
                  rv, sizeof(receivedSize));
        rv = VPL_ERR_FAIL;
        goto exit;
    }
        
    receivedSize = VPLConv_ntoh_u64(receivedSize);
    if(receivedSize < totalSize) {
        LOG_ERROR("Incomplete transfer: "FMTu64"/"FMTu64" bytes.",
                  receivedSize, totalSize);
        rv = VPL_ERR_FAIL;
        goto exit;
    }

 exit:
    if(VPLFile_IsValidHandle(fHandle)) {
        VPLFile_Close(fHandle);
    }

    return rv;
}

int DxRemoteFileTransferAgent::SendProtoSize(uint32_t resSize)
{
    int rc;
    int rv = VPL_OK;
    uint32_t size = VPLConv_hton_u32(resSize);

    rc = VPLSocket_Write(clienttcpsocket, &size, sizeof(size), VPL_TIMEOUT_NONE);
    if(rc < 0) {
        LOG_ERROR("I/O error sending size: (%d).", rc);
        rv = rc;
    }
    else if(rc != sizeof(size)) {
        LOG_ERROR("Incomplete send of size: %d/"FMT_size_t" bytes.",
                  rv, sizeof(size));
        rv = -1;
    }
    return rv;
}

int DxRemoteFileTransferAgent::SendProtoResponse(igware::dxshell::DxRemoteFileTransfer &myRes)
{
    int rc;
    int rv = VPL_OK;
    char *data = NULL;

    data = new (std::nothrow) char[myRes.ByteSize()];
    if(!data) {
        rv = VPL_ERR_NOMEM;
        LOG_ERROR("Memory allocation failure.");
        goto exit;
    }
    
    if(!myRes.SerializeToArray(data, myRes.ByteSize())) {
        LOG_ERROR("Serialization error");
        rv= -1;
        goto exit;
    }

    rc = VPLSocket_Write(clienttcpsocket, data, myRes.ByteSize(), VPL_TIMEOUT_NONE);
    if(rc < 0) {
        rv = rc;
        LOG_ERROR("I/O error sending protobuf data: (%d).", rv);
    }
    else if(rc != myRes.ByteSize()) {
        LOG_ERROR("Incomplete send of protobuf data: %d/%d bytes.",
                  rc, myRes.ByteSize());
        rv = -1;
    }

exit:
    delete[] data;

    return rv;
}

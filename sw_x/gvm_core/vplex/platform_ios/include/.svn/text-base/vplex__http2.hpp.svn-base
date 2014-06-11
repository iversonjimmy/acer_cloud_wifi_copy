#ifndef __VPLEX__HTTP2_HPP__
#define __VPLEX__HTTP2_HPP__

#include <vpl_th.h>
#include <vpl_time.h>
#include <vplu_types.h>

#include "vplex_http2_cb.hpp"

#include <string>
#include <map>

class VPLHttp2;

class VPLHttp2__Impl {
public:
    VPLHttp2__Impl(VPLHttp2 *interface);
    ~VPLHttp2__Impl();

    // @see VPLHttp2::Init()
    static int Init(void);

    // @see VPLHttp2::Shutdown()
    static void Shutdown(void);

    // @see VPLHttp2::SetDebug()
    int SetDebug(bool debug);
    
    // @see VPLHttp2::SetTimeout()
    int SetTimeout(VPLTime_t timeout);
    
    // @see VPLHttp2::SetUri()
    int SetUri(const std::string &uri);
    
    // @see VPLHttp2::AddRequestHeader()
    int AddRequestHeader(const std::string &field, const std::string &value);
    
    // @see VPLHttp2::Gett()
    int Get(std::string &respBody);
    int Get(const std::string &respBodyFilePath, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);
    int Get(VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);

    // @see VPLHttp2::Put()
    int Put(const std::string &reqBody,
            std::string &respBody);
    int Put(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
            std::string &respBody);
    int Put(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
            std::string &respBody);

    // @see VPLHttp2::Post()
    int Post(const std::string &reqBody,
             std::string &respBody);
    int Post(const std::string &reqBodyFilePath, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
             std::string &respBody);
    int Post(VPLHttp2_SendCb sendCb, void *sendCtx, u64 sendSize, VPLHttp2_ProgressCb sendProgCb, void *sendProgCtx,
             std::string &respBody);
    int Post(const std::string &reqBody,
             VPLHttp2_RecvCb recvCb, void *recvCtx, VPLHttp2_ProgressCb recvProgCb, void *recvProgCtx);

    // @see VPLHttp2::Delete()
    int Delete(std::string& respBody);
    
    // @see VPLHttp2::GetStatusCode()
    int GetStatusCode();
    
    // @see VPLHttp2::FindResponseHeader()
    const std::string *FindResponseHeader(const std::string &field);
    
    // @see VPLHttp2::Cancel()
    int Cancel();

private:
    int startToConnect(const char* method);

    void *connectionHelper;
    VPLHttp2 *vplHttp2Interface;
    bool isDebugging;
};

#endif // include guard

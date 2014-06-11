#define _CRT_SECURE_NO_WARNINGS // for cross-platform getenv

#include "HttpAgent.hpp"
#include "HttpAgentLocal.hpp"
#include "HttpAgentRemote.hpp"

HttpAgent::HttpAgent() : http_statuscode(-1), maxfilebytes(0), maxbytes_delay(0)
{
    memset(&stats, 0, sizeof(stats));
}

HttpAgent::~HttpAgent()
{
}

HttpAgent *getHttpAgent()
{
#if !defined(VPL_PLAT_IS_WINRT) && !defined(IOS)
    if (getenv("DX_REMOTE_IP") == NULL)
#endif
        return new HttpAgentLocal();
#if !defined(VPL_PLAT_IS_WINRT) && !defined(IOS)
    else
        return new HttpAgentRemote();
#endif
}

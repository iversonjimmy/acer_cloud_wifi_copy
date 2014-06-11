#include "HttpStream_Helper.hpp"

#include "HttpStream.hpp"

#include <vpl_time.h>

#include <ctime>
#include <sstream>

int HttpStream_Helper::AddStdRespHeaders(HttpStream *hs)
{
    int err = 0;

    char dateStr[64];
    time_t curTime = time(NULL);
    strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y %H:%M:%S GMT" , gmtime(&curTime));
    hs->SetRespHeader("Date", dateStr);

    hs->SetRespHeader("Ext", "");
    hs->SetRespHeader("Server","Acer Media Streaming Server");
    hs->SetRespHeader("realTimeInfo.dlna.org","DLNA.ORG_TLAG=*");
    hs->SetRespHeader("transferMode.dlna.org","Interactive");
    hs->SetRespHeader("Accept-Ranges", "bytes");
    hs->SetRespHeader("Connection", "Keep-Alive");

    return err;
}

int HttpStream_Helper::SetCompleteResponse(HttpStream *hs, int code)
{
    int err = 0;

    AddStdRespHeaders(hs);
    hs->SetStatusCode(code);
    if (code >= 200 && code != 204 && code != 304) {
        hs->SetRespHeader("Content-Length", "0");
    }

    hs->Flush();

    return err;
}

int HttpStream_Helper::SetCompleteResponse(HttpStream *hs, int code, const std::string &content, const std::string &contentType)
{
    int err = 0;

    AddStdRespHeaders(hs);
    hs->SetStatusCode(code);

    std::ostringstream oss;
    oss << content.size();
    hs->SetRespHeader("Content-Length", oss.str());

    if (!contentType.empty()) {
        hs->SetRespHeader("Content-Type", contentType);
    }

    hs->Flush();

    hs->Write(content.data(), content.size());

    return err;
}

#ifndef __HTTPSVC_UTILS_HPP__
#define __HTTPSVC_UTILS_HPP__

#include <vplu_types.h>
#include <vpl_thread.h>

#include <string>

class HttpStream;

namespace HttpSvc {
    namespace Utils {
        template <class T>
        T *CopyPtr(T *p) {
            if (p) {
                ++p->RefCounter;
            }
            return p;
        }

        template <class T>
        void DestroyPtr(T *p) {
            if (p) {
                if (--p->RefCounter <= 0) {
                    delete p;
                }
            }
        }

        extern const std::string HttpHeader_ContentLength;
        extern const std::string HttpHeader_ContentRange;
        extern const std::string HttpHeader_ContentType;
        extern const std::string HttpHeader_Date;
        extern const std::string HttpHeader_Host;
        extern const std::string HttpHeader_Range;

        extern const std::string HttpHeader_ac_collectionId;
        extern const std::string HttpHeader_ac_origDeviceId;
        extern const std::string HttpHeader_ac_serviceTicket;
        extern const std::string HttpHeader_ac_sessionHandle;
        extern const std::string HttpHeader_ac_srcfile;
        extern const std::string HttpHeader_ac_userId;
        extern const std::string HttpHeader_ac_deviceId;
        extern const std::string HttpHeader_ac_datasetRelPath;
        extern const std::string HttpHeader_ac_tolerateFileModification;
        extern const std::string HttpHeader_ac_version;

        extern const std::string Mime_ApplicationJson;
        extern const std::string Mime_ApplicationOctetStream;
        extern const std::string Mime_ImageUnknown;
        extern const std::string Mime_AudioUnknown;
        extern const std::string Mime_VideoUnknown;
        extern const std::string Mime_TextPlain;

        enum ThreadState {
            ThreadState_NoThread = 0,
            ThreadState_Spawning,  // thread is being spawned
            ThreadState_Running,   // thread is running
        };

        const std::string &GetThreadStateStr(ThreadState state);

        enum CancelState {
            CancelState_NoCancel = 0,
            CancelState_Canceling,  // cancel requested
        };

        const std::string &GetCancelStateStr(CancelState state);

        int CheckReqHeaders(HttpStream *hs);
        int AddStdRespHeaders(HttpStream *hs);
        int SetCompleteResponse(HttpStream *hs, int code);
        int SetCompleteResponse(HttpStream *hs, int code, const std::string &content, const std::string &contentType);

        int SetLocalDeviceIdInReq(HttpStream *hs);

        // "oid" is based64 encoded
        // "collectionId" is percent-encoded
        int GetOidCollectionId(u64 deviceId, const std::string &oid, std::string &collectionId);
    }
}

#endif // incl guard

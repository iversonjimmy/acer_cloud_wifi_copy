#ifndef __HTTPSVC_CCD_ASYNCAGENT_HPP__
#define __HTTPSVC_CCD_ASYNCAGENT_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>

#include <string>

#include "AsyncUploadQueue.hpp"

#include "HttpSvc_Ccd_Agent.hpp"
#include "HttpSvc_Utils.hpp"

class stream_transaction;
class HttpFileUploadStream;

namespace HttpSvc {
    namespace Ccd {
        class Server;

        class AsyncAgent : public Agent {
        public:
            AsyncAgent(Server *server, u64 userId);
            ~AsyncAgent();

            int Start();
            int AsyncStop(bool isUserLogout);

            class RequestStatus {
            public:
                u64 requestId;
                UploadStatusType status;
                u64 size;
                u64 sent_so_far;
            };

            // Interface to async_upload_queue.
            // These functions should be reviewed later.
            int GetJsonTaskStatus(/*OUT*/std::string &response);
            int QueryTransaction(stream_transaction &st, u64 id, u64 userId, bool &found);
            int Dequeue(stream_transaction &st);
            int Enqueue(u64 uid, u64 deviceid, u64 sent_so_far, u64 size, const char *request_content, int status, u64 &handle);
            int UpdateStatus(stream_transaction &st);
            int UpdateUploadStatus(u64 requestId, UploadStatusType upload_status, u64 userId);
            int UpdateSentSoFar(u64 requestId, u64 sent_so_far);

            int AddRequest(u64 uid, u64 deviceid, u64 size, const std::string &request_content, u64 &requestId);
            int CancelRequest(u64 requestId);
            int GetRequestStatus(u64 requestId, RequestStatus &status);

            static void FileReadCb(void *ctx, size_t bytes, const char* buf);
            void FileReadCb(size_t bytes);

        private:
            VPL_DISABLE_COPY_AND_ASSIGN(AsyncAgent);

            Utils::ThreadState threadState;
            Utils::CancelState cancelState;
            std::string getStateStr() const;
            int getStateNum() const;

            async_upload_queue *upload_queue;
            bool uploadQueueReady;
            bool emptyDbOnDestroy;

            VPLDetachableThreadHandle_t thread;
            void thread_main();
            static VPLTHREAD_FN_DECL thread_main(void* param);

            mutable VPLCond_t has_req_cond;  // there is some request to process in db

            u64 curReqId;  // 0 means no request in progress
            HttpStream *curHs;
            u64 curSentSoFar;  // number of bytes sent of file
            VPLTime_t lastUpdateTimestamp;  // when sent-so-far was last updated in db

            stream_transaction *tryGetRequest();
            int checkNoExistence(stream_transaction *async_req);
            int uploadFile(stream_transaction *async_req);
        };
    } // namespace Ccd
} // namespace HttpSvc

#endif // incl guard

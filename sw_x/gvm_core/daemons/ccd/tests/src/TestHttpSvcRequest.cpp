#include <vplu_types.h>

#include "HttpSvc_Request.hpp"
#include "HttpSvc_Utils.hpp"

#include "stream_transaction.hpp"

#include <log.h>

#include <iostream>
#include <cassert>

class TestHttpSvcRequest {
public:
    void RunTests();
private:
    void testReqAccessInterlock();
};

const std::string http_req_header_reqline = "POST /aaa\r\n";
const std::string http_req_header_content_length = "Content-Length: 65536\r\n";
const std::string http_req_header_blankline = "\r\n";

static VPLTHREAD_FN_DECL req_reader_thread(void* param)
{
    HttpSvc::Request *request = static_cast<HttpSvc::Request*>(param);

    size_t total_read = 0;
    while (total_read < 65536) {
        request->WaitReqBodyToGetNext();
        std::string buffer;
        request->GetNextReqBody(buffer);
        total_read += buffer.size();
        //std::cout << "\t\t\t\t\tPulled " << buffer.size() << "/" << total_read << std::endl;
        LOG_INFO("\tPulled %d/%d", buffer.size(), total_read);
    }

    return VPLTHREAD_RETURN_VALUE;
}


void TestHttpSvcRequest::testReqAccessInterlock()
{
    HttpSvc::Request *request = new HttpSvc::Request(/*agent*/NULL, /*server*/NULL, /*userId*/12345);
    stream_transaction *st = request->_GetTransactionObj();

    VPLDetachableThreadHandle_t thread;
    int err = HttpSvc::Utils::SpawnThread(thread, true, 128*1024, req_reader_thread, (void*)request);
    if (err) {
        LOG_ERROR("Failed to spawn req reader thread: %d", err);
        exit(1);
    }

    // Initial state is PREP.
    assert(request->state.req == HttpSvc::Request::ReqState_Receiving);

    // Until a blank line is read, the state remains in PREP.
    request->AddReqData(http_req_header_reqline.data(), http_req_header_reqline.size());
    assert(request->state.req == HttpSvc::Request::ReqState_Receiving);
    request->AddReqData(http_req_header_content_length.data(), http_req_header_content_length.size());
    assert(request->state.req == HttpSvc::Request::ReqState_Receiving);

    // After a blank line is read, the state changes to READY.
    request->AddReqData(http_req_header_blankline.data(), http_req_header_blankline.size());
    assert(request->state.req == HttpSvc::Request::ReqState_HeaderRcvd);

    int total_pushed = 0;
    for (int i = 0; i < 16; i++) {
        char buffer[4096];
        request->WaitReqBodyToAdd();
        size_t used = request->AddReqData(buffer, ARRAY_SIZE_IN_BYTES(buffer));
        total_pushed += used;
        //        std::cout << "Pushed " << used << "/" << total_pushed << ", available " << st->req->content_available() << std::endl;
        LOG_INFO("Pushed %d/%d avail %d", used, total_pushed, st->req->content_available());
    }

    VPLThread_Sleep(VPLTime_FromSec(1));
}

void TestHttpSvcRequest::RunTests()
{
#define RUN_TEST(name)                                  \
    test##name();                                       \
    std::cout << "test" #name << ": OK" << std::endl

    RUN_TEST(ReqAccessInterlock);
#undef RUN_TEST
}

int main(int argc, char *argv[])
{
    LOGInit("TestHttpSvcRequest", NULL);

    TestHttpSvcRequest thsr;
    thsr.RunTests();

    return 0;
}

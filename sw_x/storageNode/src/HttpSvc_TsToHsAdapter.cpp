#include "HttpSvc_TsToHsAdapter.hpp"

#include "HttpSvc_Sn_Dispatcher.hpp"

#include <gvm_errors.h>
#include <log.h>
#include <HttpTsStream.hpp>

HttpSvc::TsToHsAdapter::TsToHsAdapter(TSServiceRequest_t *tssr)
    : tssr(tssr)
{
    LOG_INFO("TsToHsAdapter[%p]: Created for TSServiceRequest[%p]", this, tssr);
}

HttpSvc::TsToHsAdapter::~TsToHsAdapter()
{
    LOG_INFO("TsToHsAdapter[%p]: Destroyed", this);
}

int HttpSvc::TsToHsAdapter::Run()
{
    int err = 0;

    HttpTsStream *hts = new (std::nothrow) HttpTsStream(tssr->io_handle);
    if (!hts) {
        LOG_ERROR("TsToHsAdapter[%p]: Not enough memory", this);
        err = CCD_ERROR_NOMEM;
        goto end;
    }

    hts->SetUserId(tssr->client_user_id);

    err = HttpSvc::Sn::Dispatcher::Dispatch(hts);
    delete hts;

 end:
    return err;
}

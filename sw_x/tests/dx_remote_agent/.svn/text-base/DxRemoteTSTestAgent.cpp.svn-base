#include "DxRemoteTSTestAgent.hpp"
#include "log.h"
#include "ts_test.hpp"

DxRemoteTSTestAgent::DxRemoteTSTestAgent(VPLSocket_t skt, char *buf, uint32_t pktSize) : IDxRemoteAgent(skt, buf, pktSize)
{
}

DxRemoteTSTestAgent::~DxRemoteTSTestAgent()
{
}

int DxRemoteTSTestAgent::doAction()
{
    igware::dxshell::DxRemoteTSTest input, output;

    LOG_DEBUG("DxRemoteTSTestAgent::doAction() Start");

    if (!input.ParseFromArray(recvBuf, recvBufSize)) {
        output.set_return_value(igware::dxshell::DX_ERR_BAD_REQUEST);
        output.set_error_msg("Parse request failed");
    }
    else {
        TSTestParameters test(input.log_enable_level());
        TSTestResult    result;

        const igware::dxshell::DxRemoteTSTest_TSOpenParms& tsOpenParms = input.ts_open_parms();

        test.tsOpenParms.user_id = tsOpenParms.user_id();
        test.tsOpenParms.device_id = tsOpenParms.device_id();
        test.tsOpenParms.service_name = tsOpenParms.service_name();
        test.tsOpenParms.credentials = tsOpenParms.credentials();
        test.tsOpenParms.flags = tsOpenParms.flags();
        test.tsOpenParms.timeout = tsOpenParms.timeout();

        test.testId = input.test_id();
        test.xfer_cnt = input.xfer_cnt();
        test.xfer_size = input.xfer_size();
        test.nTestIterations = input.num_test_iterations();
        test.nClients = input.num_clients();
        test.client_write_delay = input.client_write_delay();
        test.server_read_delay = input.server_read_delay();

        // If doAction() returns an error, the response will not be sent back to the dxshell.
        // The runTsTest result is returned in the result object.
        // There is no reason to ever return an error from doAction().
        runTsTest(test, result);

        output.set_return_value(result.return_value);
        output.set_error_msg(result.error_msg);
    }

    response = output.SerializeAsString();

    return 0;
}




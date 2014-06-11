/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */


#ifndef __ECHO_TEST_SERVER_HPP__
#define __ECHO_TEST_SERVER_HPP__

#include "ts_types.hpp"

namespace EchoSvc {

    // used for test ClientCloseWhileServiceWriting
    #define EchoMinBytesBeforeClientCloseWhileSvcWriting 512

    enum EchoServiceTestIdEnum {
        TunnelAndConnectionWorking = 0,
        ClientCloseAfterWrite = 1,
        ServiceCloseAfterWrite = 2,
        ClientCloseWhileServiceWriting = 3,
        ServiceCloseWhileClientWritng = 4,
        ServiceCloseWith_TS_CLOSE = 5,
        LostConnection = 6,
        ServiceIgnoreIncoming = 7,
        ServiceDoNothing = 8,
        ServiceDontEcho = 9,
        MaxEchoServiceTestId = 9, // should be the same as last test id
        MaxPossibleEchoServiceTestId = 127
    };

    #define ECHO_SVC_TEST_CMD_MSG_ID_STR "ECHO SERVICE TEST CMD MSG IDENT"

    struct EchoSvcTestCmd
    {
        u8   msgId[32];
        u32  msgSize;
        u32  extraReadDelay; // msec
        s32  testId;
        u32  clientInstanceId;

        void init()
        {
            u32 i;
            for (i=0; i < sizeof msgId; ++i) {msgId[i] = 0;}
            msgSize = 0;
            extraReadDelay = 0;
            testId = 0;
            clientInstanceId = 0;
        }

        EchoSvcTestCmd() { init(); }
        
        inline size_t msgWriteSize() const
        {
            return sizeof msgId + 
                sizeof msgSize +
                sizeof extraReadDelay +
                sizeof testId +
                sizeof clientInstanceId;
        }

        inline size_t expectedMsgSize() const
        {
            return msgWriteSize();
        }
    };

    void handler(TSServiceRequest_t& request);

} // namespace EchoSvc

#endif // include guard

//
//  VPLEXTestWrapper.c
//  CCDLibrary
//
//  Created by wukon hsieh on 12/5/30.
//  Copyright (c) 2012 Acer Inc. All rights reserved.
//

#include "VPLEXTestWrapper.h"

#include "vplexTest.h"
#include "vplTest_suite_common.h"
#include "vplex_trace.h"

#include <setjmp.h>
#include <signal.h>
#include <assert.h>
#include <getopt.h>

static VPLTest_test vplTest_allTests[] = {
    { testVPLMath,                    "Math",                    VPL_TRUE },
//    { testVPLMemHeap,                 "MemHeap",                 VPL_TRUE }, //TODO: It will block the testing, block this function until vplex_mem is ready.
    { testVPLSafeSerialization,       "SafeSerialization",       VPL_TRUE },
    { testVPLSerialization,           "Serialization",           VPL_TRUE },
    { testVPLXmlReader,               "XmlReader",               VPL_TRUE },
    { testVPLXmlReaderUnescape,       "XmlReaderUnescape",       VPL_TRUE },
    { testVPLXmlWriter,               "XmlWriter",               VPL_TRUE },
    { testVPLWstring,                 "Wstring",                 VPL_TRUE },
    { testVPLHttp,                    "Http",                    VPL_TRUE },
};

static jmp_buf vplTestSignalExit;

const char* download_url = "http://vanadium.acer.com.tw/test/download";
const char* cut_short_url = "http://10.36.141.93:38080/cut_short";

void initEnv();

int runVPLEXTests()
{
    initEnv();

    if (setjmp(vplTestSignalExit) == 0) { // initial invocation of setjmp

        VPLTrace_Init(NULL);

        VPLTEST_START("_Init");
        VPLTEST_CALL_AND_CHK_RV(VPL_Init(), VPL_OK);
        VPLTEST_END();

        VPLTest_RunAll(vplTest_allTests, ARRAY_ELEMENT_COUNT(vplTest_allTests)); 

        VPLTEST_START("_Quit");
        VPLTEST_CALL_AND_CHK_RV(VPL_Quit(), VPL_OK);
        VPLTEST_END();
    }

    if (vplTest_getTotalErrCount() > 0) {
        printf("*** TEST SUITE FAILED, %d error(s)\n", vplTest_getTotalErrCount());
    }
    printf("\nCLEAN EXIT\n");
    return vplTest_getTotalErrCount();
}

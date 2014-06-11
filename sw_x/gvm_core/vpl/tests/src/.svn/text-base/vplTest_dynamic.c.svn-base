//
//  Copyright (C) 2007-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplTest.h"

#ifndef WIN32

#include "vpl_dynamic.h"

#define SHARED_LIBRARY_FILE  "lib/libExampleSharedLibrary.so"

static void testInvalidParameters(void)
{
    VPLDL_LibHandle handle, badHandle;

    // Open the library.
    VPLTEST_CALL_AND_ENSURE_OK(VPLDL_Open(SHARED_LIBRARY_FILE, VPL_TRUE, &handle));

    float (*testFunc)(float, float);

    // Check invalid parameters.

    VPLTEST_CALL_AND_CHK_RV(VPLDL_Open(NULL, VPL_TRUE, &badHandle), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLDL_Open("badFileName", VPL_TRUE, NULL), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLDL_Open("badFileName", VPL_TRUE, &badHandle), VPL_ERR_FAIL);

    VPLTEST_CALL_AND_CHK_RV(VPLDL_Close(badHandle), VPL_ERR_BADF);

    VPLTEST_CALL_AND_CHK_RV(VPLDL_Sym(handle, NULL, (void**)&testFunc), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLDL_Sym(handle, "badSymbol", NULL), VPL_ERR_INVALID);

    VPLTEST_CALL_AND_CHK_RV(VPLDL_Sym(badHandle, "badSymbol", NULL), VPL_ERR_BADF);

    VPLTEST_CALL_AND_CHK_RV(VPLDL_Sym(handle, "badSymbol", (void**)&testFunc), VPL_ERR_FAIL);

    // Done with the library, close it now.
    VPLTEST_CALL_AND_CHK_RV(VPLDL_Close(handle), VPL_OK);
}

static void testDynamicLoader(void)
{
    // Open the library.
    VPLDL_LibHandle handle;
    VPLTEST_CALL_AND_ENSURE_OK(VPLDL_Open(SHARED_LIBRARY_FILE, VPL_TRUE, &handle));

    // Find and use the testAddFloats() function.
    {
        float (*testFunc)(float, float);
        VPLTEST_CALL_AND_ENSURE_OK(VPLDL_Sym(handle, "testAddFloats", (void**)&testFunc));
        {
            float result = testFunc(3.5, 7.25);
            VPLTEST_CHK_EQUAL(result, 10.75f, "%f", "testFunc(3.5, 7.25)");
        }
    }

    float (*bogusTestFunc)(float, float);
    
    // Try a bogus library
    {
        VPLDL_LibHandle failedHandle;
        VPLTEST_CALL_AND_CHK_RV(VPLDL_Open("./invalid_library.so", VPL_TRUE, &failedHandle), VPL_ERR_FAIL);
        VPLTEST_CALL_AND_CHK_RV(VPLDL_Sym(failedHandle, "testAddFloats", (void**)&bogusTestFunc),
                VPL_ERR_BADF, "Handle from failed open should be rejected");
        VPLTEST_CALL_AND_CHK_RV(VPLDL_Close(failedHandle),
                VPL_ERR_BADF, "Handle from failed open should be rejected");
    }
    
    // Done with the library, close it now.
    VPLTEST_CALL_AND_CHK_RV(VPLDL_Close(handle), VPL_OK);

//% TODO: crashes:
//%    VPLTEST_CALL_AND_CHK_RV(VPLDL_Close(handle), VPL_ERR_FAIL, "Redundant call to close should be rejected");
//%    VPLTEST_CALL_AND_CHK_RV(VPLDL_Sym(handle, "testAddFloats", (void**)&testFunc), VPL_ERR_FAIL, "Lookup from closed library should be rejected");

    {
        VPLDL_LibHandle uninitHandle;
        VPLTEST_CALL_AND_CHK_RV(VPLDL_Sym(uninitHandle, "testAddFloats", (void**)&bogusTestFunc),
                VPL_ERR_BADF, "Uninitialized handle should be rejected");
        VPLTEST_CALL_AND_CHK_RV(VPLDL_Close(uninitHandle),
                VPL_ERR_BADF, "Uninitialized handle should be rejected");
    }
}

void testVPLDL(void)
{
    // First, a sanity check to make sure the shared library is available:
    {
        FILE * pFile;
        pFile = fopen(SHARED_LIBRARY_FILE, "r");
        VPLTEST_ENSURE_NOT_NULL(pFile, "fopen("SHARED_LIBRARY_FILE")");
        {
            long lSize;
            fseek (pFile , 0 , SEEK_END);
            lSize = ftell(pFile);
            VPLTEST_LOG("File size is %ld", lSize);
            fclose(pFile);
        }
    }

    VPLTEST_LOG("testInvalidParameters");
    testInvalidParameters();

    VPLTEST_LOG("testDynamicLoader");
    testDynamicLoader();
}

#endif

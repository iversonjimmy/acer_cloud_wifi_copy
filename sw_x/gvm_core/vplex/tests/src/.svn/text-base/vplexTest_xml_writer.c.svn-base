//
//  Copyright (C) 2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_xml_writer.h"

#include "vplexTest.h"

#include <stdlib.h>

static void testPopulateWriter(VPLXmlWriter* writer)
{
    const char* attrNameArray[] = { "empty", "alpha", "numeric" };
    const char* attrValueArray[] = { "", "two", "3" };
    VPLXmlWriter_InsertXml(writer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    
    VPLXmlWriter_OpenTagV(writer, "rootElement", 1, "attr_name", "attr_value");
    VPLXmlWriter_OpenTagV(writer, "nestedElement1", 0);
    VPLXmlWriter_OpenTagV(writer, "nestedElement1_1", 4,
            "a", "1",
            "b", "2",
            "c", "3",
            "d", "4");
    VPLXmlWriter_AddData(writer, "testData; \"this & that\" !@#$%^&*()");
    VPLXmlWriter_CloseTag(writer); // nestedElement2
    VPLXmlWriter_InsertSimpleElement(writer, "simpleElement", "simpleData");
    VPLXmlWriter_OpenTagV(writer, "nestedElement1_2", 0);
    VPLXmlWriter_OpenTagV(writer, "nestedElement1_2_1", 0);
    VPLXmlWriter_OpenTagV(writer, "nestedElement1_2_1_1", 0);
    VPLXmlWriter_AddData(writer, "data for 1.2.1.1");
    VPLXmlWriter_CloseTag(writer); // nestedElement1_2_1_1
    VPLXmlWriter_OpenTagV(writer, "nestedElement1_2_1_2", 0);
    VPLXmlWriter_OpenTagV(writer, "nestedElement1_2_1_2_1", 1, "nesting", "a bunch");
    VPLXmlWriter_AddData(writer, "data for 1.2.1.2.1");
    VPLXmlWriter_CloseTag(writer); // nestedElement1_2_1_2_1
    VPLXmlWriter_InsertSimpleElement(writer, "nestedElement1_2_1_2_2", "next element is empty");
    VPLXmlWriter_InsertSimpleElement(writer, "nestedElement1_2_1_2_3", "");
    VPLXmlWriter_InsertSimpleElement(writer, "nestedElement1_2_1_2_4", "previous element is empty");
    VPLXmlWriter_CloseTag(writer); // nestedElement1_2_1_2
    VPLXmlWriter_CloseTag(writer); // nestedElement1_2_1
    VPLXmlWriter_CloseTag(writer); // nestedElement1_2
    VPLXmlWriter_CloseTag(writer); // nestedElement1
    VPLXmlWriter_InsertSimpleElement(writer, "nestedElement2", "Hi!");
    VPLXmlWriter_OpenTagV(writer, "nestedElement3", 0);
    VPLXmlWriter_AddData(writer, "Before ");
    VPLXmlWriter_InsertSimpleElement(writer, "nestedElement3_1", "nested data");
    VPLXmlWriter_AddData(writer, " After");
    VPLXmlWriter_OpenTagV(writer, "nestedElement3_2", 0);
    VPLXmlWriter_OpenTag(writer, "nestedElement3_2_1", 3, attrNameArray, attrValueArray);
    VPLXmlWriter_AddData(writer, "FINE < GREAT > OKAY \" ' & ?");
}

static void doTooSmallTest(size_t realRequiredLen, size_t numBytesMissing)
{
    size_t sizeToMalloc = realRequiredLen + sizeof(VPLXmlWriter) - numBytesMissing;
    //Force cast from (void*) for WOA compilation
    VPLXmlWriter* tooSmall1 = (VPLXmlWriter*)malloc(sizeToMalloc);
    size_t requiredLen;
    const char* emptyString;
    VPLTEST_ENSURE_NOT_NULL(tooSmall1, "malloc");
    VPLTEST_ENSURE_LESS(sizeToMalloc, (size_t)UINT16_MAX, FMTuSizeT, "");
    VPLXmlWriter_Init(tooSmall1, (u16)sizeToMalloc);
    testPopulateWriter(tooSmall1);
    emptyString = VPLXmlWriter_GetString(tooSmall1, &requiredLen);
    VPLTEST_ENSURE_EQUAL(requiredLen, realRequiredLen, FMTuSizeT, "tooSmall length");
    VPLTEST_ENSURE_EQUAL(emptyString, NULL, FMT0xPTR, "tooSmall string");
    free(tooSmall1);
}

static void doSuccessfulTest(size_t realRequiredLen, size_t numBytesExtra, bool dumpResult)
{
    const char* result;
    size_t sizeToMalloc = realRequiredLen + sizeof(VPLXmlWriter) + numBytesExtra;
    //Force cast from (void*) for WOA compilation
    VPLXmlWriter* writer = (VPLXmlWriter*)malloc(sizeToMalloc);
    size_t requiredLen;
    VPLTEST_ENSURE_NOT_NULL(writer, "malloc");
    VPLTEST_ENSURE_LESS(sizeToMalloc, (size_t)UINT16_MAX, FMTuSizeT, "");
    VPLXmlWriter_Init(writer, (u16)sizeToMalloc);
    testPopulateWriter(writer);
    result = VPLXmlWriter_GetString(writer, &requiredLen);
    VPLTEST_ENSURE_NOT_NULL(result, "string");
    VPLTEST_ENSURE_EQUAL(requiredLen, realRequiredLen, FMTuSizeT, "length");
    if (dumpResult) {
        VPLTEST_LOG("result:\n%s", result);
    }
    free(writer);
}

void testVPLXmlWriter(void)
{
    VPLXmlWriter dummyWriter;
    VPLXmlWriter_Init(&dummyWriter, sizeof(VPLXmlWriter));
    VPLTEST_LOG("Init complete.");
    testPopulateWriter(&dummyWriter);
    {
        size_t requiredLen1;
        size_t requiredLen2;
        VPL_BOOL overflowed = VPLXmlWriter_HasOverflowed(&dummyWriter, &requiredLen1);
        const char* dummyString = VPLXmlWriter_GetString(&dummyWriter, &requiredLen2);
        
        VPLTEST_ENSURE_EQUAL(overflowed, VPL_TRUE, FMT_VPL_BOOL, "dummyWriter overflow");
        VPLTEST_ENSURE_EQUAL(requiredLen1, requiredLen2, FMTuSizeT, "dummyWriter length");
        VPLTEST_ENSURE_EQUAL(dummyString, NULL, FMT0xPTR, "dummyWriter string");
        
        VPLTEST_LOG(FMTuSizeT" bytes", requiredLen1);
        
        VPLTEST_LOG("Begin \"Too small by 3\"");
        doTooSmallTest(requiredLen1, 3);
        VPLTEST_LOG("Begin \"Too small by 2\"");
        doTooSmallTest(requiredLen1, 2);
        VPLTEST_LOG("Begin \"Too small by 1\"");
        doTooSmallTest(requiredLen1, 1);
        VPLTEST_LOG("Begin \"Just right\"");
        doSuccessfulTest(requiredLen1, 0, false);
        VPLTEST_LOG("Begin \"1 extra byte\"");
        doSuccessfulTest(requiredLen1, 1, true);
    }
}

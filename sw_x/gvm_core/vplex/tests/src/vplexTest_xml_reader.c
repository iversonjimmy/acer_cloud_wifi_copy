//
//  Copyright (C) 2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

// TODO: add negative tests for buffer overflow

#include "vplex_xml_reader.h"

#include "vplexTest.h"

#include <stdio.h>

#define DEBUG_LOG 0

static void assertEquals_str(const char* message, const char* expected,
        const char* actual)
{
    if (expected == NULL) {
        if (actual == NULL) {
            VPLTEST_DEBUG("%s: Got NULL as expected.", message);
        } else {
            VPLTEST_NONFATAL_ERROR("%s: expected null but was (%s)", message, actual);
        }
    } else {
        if (actual == NULL) {
            VPLTEST_NONFATAL_ERROR("%s: expected (%s) but was null", message, expected);
        } else {
            if (strcmp(expected, actual) == 0) {
                VPLTEST_DEBUG("%s: Got (%s) as expected", message, expected);
            } else {
                VPLTEST_NONFATAL_ERROR("%s: expected (%s) but was (%s)", message, expected, actual);
            }
        }
    }
}

static void assertEquals_strArray(const char* msg,
        const char* const expected[], const char* const actual[])
{
    int i = -1;
    char nmsg[64];
    do {
        i++;
        sprintf(nmsg, "%s% d", msg, i);
        assertEquals_str(nmsg, expected[i], actual[i]);
    } while (expected[i] != 0 && actual[i] != 0);
}

// Calls to the open and close callbacks are checked against the expected sequence.
// Because the data callback can be called any number of times, it appends data to
// a buffer, and at each call to the open or close callback, we inspect the buffer
// and reset it.

typedef struct {
    const char callType;
    const char* tag;
    const char* data;
    const char* attrName[3];
    const char* attrValue[3];
} CallbackExpectation_t;

typedef struct {
    const int num_calls_expected;
    int callnum;
    const CallbackExpectation_t* expectedCalls;
} ExpectationState_t;

#define TEST_STRING_1 \
        "<Tag1>" \
        "<Tag2 name1=\"value1\" name2=\'value2\'/>data1  \t\r\n" \
        "<Tag3 name3=\"value3\"\t\n/ >&quot;data2&lt;&gt;&apos;&amp;" \
        "</Tag1\t>"
static CallbackExpectation_t expectedCalls1[] = {
        { 'o', "Tag1", "", {0, 0, 0}, {0, 0, 0} },
        { 'o', "Tag2", "", {"name1", "name2", 0}, {"value1", "value2", 0} },
        { 'c', "Tag2", "", {0, 0, 0}, {0, 0, 0} },
        { 'o', "Tag3", "data1  \t\r\n", {"name3", 0, 0}, {"value3", 0, 0} },
        { 'c', "Tag3", "", {0, 0, 0}, {0, 0, 0} },
        { 'c', "Tag1", "\"data2<>'&", {0, 0, 0}, {0, 0, 0} },
};
static const int NUM_CALLS_1 = ARRAY_ELEMENT_COUNT(expectedCalls1);

#define TEST_STRING_2 "<Tag1>&quot;12345&amp;</Tag1>"
static CallbackExpectation_t expectedCalls2[] = {
        { 'o', "Tag1", "", {0, 0, 0}, {0, 0, 0} },
        { 'c', "Tag1", "\"12345&", {0, 0, 0}, {0, 0, 0} },
};
static const int NUM_CALLS_2 = ARRAY_ELEMENT_COUNT(expectedCalls2);

#define TEST_STRING_3 "<TagBad>&lt;/TagBad&gt;&gt;&amp;&amp;0123456&gt;&gt;&gt;&gt;&gt;&gt;&gt;&gt;&lt;>&lt;>&lt;></TagBad>"
static CallbackExpectation_t expectedCalls3[] = {
        { 'o', "TagBad", "", {0, 0, 0}, {0, 0, 0} },
        { 'c', "TagBad", "</TagBad>>&&0123456>>>>>>>><><><>", {0, 0, 0}, {0, 0, 0} },
};
static const int NUM_CALLS_3 = ARRAY_ELEMENT_COUNT(expectedCalls3);

#define TEST_STRING_4 "<T     a='v1'  /><TagEvil>Hi</TagEvil>abc<TagEvil/><T a='v'> def </T><TagEvil/>"
static CallbackExpectation_t expectedCalls4[] = {
        { 'o', "T", "", {"a", 0, 0}, {"v1", 0, 0} },
        { 'c', "T", "", {0, 0, 0}, {0, 0, 0} },
        { 'o', "TagEvil", "", {0, 0, 0}, {0, 0, 0} },
        { 'c', "TagEvil", "Hi", {0, 0, 0}, {0, 0, 0} },
        { 'o', "TagEvil", "abc", {0, 0, 0}, {0, 0, 0} },
        { 'c', "TagEvil", "", {0, 0, 0}, {0, 0, 0} },
        { 'o', "T", "", {"a", 0, 0}, {"v", 0, 0} },
        { 'c', "T", " def ", {0, 0, 0}, {0, 0, 0} },
        { 'o', "TagEvil", "", {0, 0, 0}, {0, 0, 0} },
        { 'c', "TagEvil", "", {0, 0, 0}, {0, 0, 0} },
};

static const int NUM_CALLS_4 = ARRAY_ELEMENT_COUNT(expectedCalls4);

#define TEST_STRING_5 "<T a=\"&#x4EF2;\"/><TagNCR>&#22823;&#22823;1&#22823;&#22823;12&#22823;&#22823;123&#22823;&#22823;&#x725B;&#x5927;&#22823;12345&#x5BA5;12&#x5BA5;  12345&#x5BA5;#&#x963F;&#x725B;&#22823;&#x5927;&#23567;&#x4E0A;&#x4E39;</TagNCR>"
static CallbackExpectation_t expectedCalls5[] = {
        { 'o', "T", "", {"a", 0, 0}, {"仲", 0, 0} },
        { 'c', "T", "", {0, 0, 0}, {0, 0, 0} },
        { 'o', "TagNCR", "", {0, 0, 0}, {0, 0, 0} },
        { 'c', "TagNCR", "大大1大大12大大123大大牛大大12345宥12宥  12345宥#阿牛大大小上丹", {0, 0, 0}, {0, 0, 0} },
};
static const int NUM_CALLS_5 = ARRAY_ELEMENT_COUNT(expectedCalls5);

static char dataBuffer[256];
static int dataBufPos = 0;

static void openCallback(const char* tag, const char* attr_name[],
        const char* attr_value[], void* param)
{
    ExpectationState_t* state = (ExpectationState_t*)param;
#if DEBUG_LOG
    int i;
    VPLTEST_LOG("Open callback, tag (%s)", tag);
    for (i = 0; attr_name[i] != 0; i++) {
        VPLTEST_LOG("Attr name %d (%s)", i, attr_name[i]);
    }
    for (i = 0; attr_name[i] != 0; i++) {
        VPLTEST_LOG("Attr value %d (%s)", i, attr_value[i]);
    }
#endif
    VPLTEST_CHK_LESS(state->callnum, state->num_calls_expected, "%d", "Called callback too many times");
    VPLTEST_CHK_EQUAL(state->expectedCalls[state->callnum].callType, 'o', "%c", "Called wrong callback");
    assertEquals_str("Opening tag name",
            state->expectedCalls[state->callnum].tag, tag);
    assertEquals_strArray("Attribute name",
            state->expectedCalls[state->callnum].attrName, attr_name);
    assertEquals_strArray("Attribute value",
            state->expectedCalls[state->callnum].attrValue, attr_value);
    dataBuffer[dataBufPos] = 0;
    assertEquals_str("Data since previous tag",
            state->expectedCalls[state->callnum].data, dataBuffer);
    dataBufPos = 0;
    state->callnum++;
}

static void closeCallback(const char* tag, void* param)
{
    ExpectationState_t* state = (ExpectationState_t*)param;
#if DEBUG_LOG
    VPLTEST_LOG("Close callback, tag (%s)", tag);
#endif
    VPLTEST_CHK_LESS(state->callnum, state->num_calls_expected, "%d", "Called callback too many times");
    VPLTEST_CHK_EQUAL(state->expectedCalls[state->callnum].callType, 'c', "%c", "Called wrong callback");
    assertEquals_str("Closing tag name",
            state->expectedCalls[state->callnum].tag, tag);
    dataBuffer[dataBufPos] = 0;
    assertEquals_str("Data since previous tag",
            state->expectedCalls[state->callnum].data, dataBuffer);
    dataBufPos = 0;
    state->callnum++;
}

static void dataCallback(const char* data, void* param)
{
    size_t dataLen = strlen(data);
    VPLTEST_DEBUG("Data callback, data=\"%s\"", data);
    memcpy(dataBuffer + dataBufPos, data, dataLen);
    dataBufPos += (int)dataLen;
    // TODO: could test that the data always ends with a complete UTF-8 character (but only when
    //   testing with a reasonable buffer size).
}

void testVPLXmlReader(void)
{
    char buffer[2000];
    // Incoming buffer size (simulates how libcurl gives us one chunk at a time).
    int cLen;
    // VPLXmlReader buffer size.
    int vLen;
    int cPos, xmlLen, segmentLen;
    char xml[] = TEST_STRING_1;
    size_t rv;
    ExpectationState_t callbackState = { NUM_CALLS_1, 0, expectedCalls1 };

    strcpy(buffer + 980, "Magic String len 20");
    {
        xmlLen = (int)strlen(xml);
        for (cLen = 160; cLen >= 2; cLen /= 2) {
            // Need 37 byte page to parse Tag2
            for (vLen = 320; vLen >= 40; vLen /= 2) {
                _VPLXmlReader reader;
                VPLTEST_LOG("vplex buffer size %d, curl buffer size %d", vLen, cLen);
                callbackState.callnum = 0;
                VPLXmlReader_Init(&reader, buffer + 1000, vLen, openCallback, closeCallback,
                        dataCallback, &callbackState);
                for (cPos = 0; cPos < xmlLen; cPos += cLen) {
                    segmentLen = xmlLen - cPos;
                    if (segmentLen > cLen) {
                        segmentLen = cLen;
                    }
                    rv = VPLXmlReader_CurlWriteCallback(xml + cPos, 1, segmentLen, &reader);
                    VPLTEST_CHK_EQUAL((int)rv, segmentLen, "%d", "curl callback");
                }
                VPLTEST_CHK_EQUAL(callbackState.callnum,
                        callbackState.num_calls_expected, "%d", "Wrong number of total calls to callbacks");
                if (memcmp(buffer + 980, "Magic String len 20", 20) != 0) {
                    VPLTEST_NONFATAL_ERROR("buffer underrun detected");
                    strcpy(buffer + 980, "Magic String len 20");
                }
            }
        }
    }
}

// You should never use a buffer smaller than 16 bytes for real code, but smaller buffers are
// used here to exercise more cases.
static const int READER_BUF_LEN[] = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 31, 32, 33, 63, 64, 65, 128, 255, 256, 257 };

//% Bug 7761
void testVPLXmlReaderUnescape(void)
{
    char buffer[1024];
    // Incoming buffer size (simulates how libcurl gives us one chunk at a time).
    size_t cLen;
    // VPLXmlReader buffer size.
    int vLen;
    int cPos;
    size_t rv, segmentLen;
    int vLenIdx;

    {
        const char xml[] = TEST_STRING_2;
        const int xmlLen = (int)strlen(xml);
        ExpectationState_t callbackState = { NUM_CALLS_2, 0, expectedCalls2 };
        _VPLXmlReader reader;

        strcpy(buffer + 230, "Magic String len 20");
        cLen = 160;
        vLen = 12;
        strcpy(buffer + 250 + vLen, "Str#2 String len 20");
        VPLXmlReader_Init(&reader, buffer + 250, vLen, openCallback, closeCallback,
                dataCallback, &callbackState);
        for (cPos = 0; cPos < xmlLen; cPos += cLen) {
            segmentLen = xmlLen - cPos;
            if (segmentLen > cLen) {
                segmentLen = cLen;
            }
            rv = VPLXmlReader_CurlWriteCallback(xml + cPos, 1, segmentLen, &reader);
            VPLTEST_CHK_EQUAL(rv, segmentLen, FMTu_size_t, "curl callback");
        }
        VPLTEST_CHK_EQUAL(callbackState.callnum,
                callbackState.num_calls_expected, "%d", "Wrong number of total calls to callbacks");
        if (memcmp(buffer + 230, "Magic String len 20", 20) != 0) {
            VPLTEST_NONFATAL_ERROR("buffer underrun detected");
        }
        if (memcmp(buffer + 250 + vLen, "Str#2 String len 20", 20) != 0) {
            VPLTEST_NONFATAL_ERROR("buffer overrun detected");
        }
    }
    for (vLenIdx = 0; vLenIdx < ARRAY_ELEMENT_COUNT(READER_BUF_LEN); vLenIdx++) {
        const char xml[] = TEST_STRING_3;
        const int xmlLen = (int)strlen(xml);
        ExpectationState_t callbackState = { NUM_CALLS_3, 0, expectedCalls3 };
        _VPLXmlReader reader;

        vLen = READER_BUF_LEN[vLenIdx];
        strcpy(buffer + 230, "Magic String len 20");
        cLen = 160;
        strcpy(buffer + 250 + vLen, "Str#2 String len 20");
        VPLTEST_LOG("vplex buffer size %d, curl buffer size "FMTu_size_t, vLen, cLen);
        VPLXmlReader_Init(&reader, buffer + 250, vLen, openCallback, closeCallback,
                dataCallback, &callbackState);
        for (cPos = 0; cPos < xmlLen; cPos += cLen) {
            segmentLen = xmlLen - cPos;
            if (segmentLen > cLen) {
                segmentLen = cLen;
            }
            rv = VPLXmlReader_CurlWriteCallback(xml + cPos, 1, segmentLen, &reader);
            VPLTEST_CHK_EQUAL(rv, segmentLen, FMTu_size_t, "curl callback");
        }
        VPLTEST_CHK_EQUAL(callbackState.callnum,
                callbackState.num_calls_expected, "%d", "Wrong number of total calls to callbacks");
        if (memcmp(buffer + 230, "Magic String len 20", 20) != 0) {
            VPLTEST_NONFATAL_ERROR("buffer underrun detected");
        }
        if (memcmp(buffer + 250 + vLen, "Str#2 String len 20", 20) != 0) {
            VPLTEST_NONFATAL_ERROR("buffer overrun detected");
        }
    }
    for (vLenIdx = 0; vLenIdx < ARRAY_ELEMENT_COUNT(READER_BUF_LEN); vLenIdx++) {
        const char xml[] = TEST_STRING_4;
        const int xmlLen = (int)strlen(xml);
        ExpectationState_t callbackState = { NUM_CALLS_4, 0, expectedCalls4 };
        _VPLXmlReader reader;

        vLen = READER_BUF_LEN[vLenIdx];
        strcpy(buffer + 230, "Magic String len 20");
        cLen = 160;
        strcpy(buffer + 250 + vLen, "Str#2 String len 20");
        VPLTEST_LOG("vplex buffer size %d, curl buffer size "FMTu_size_t, vLen, cLen);
        VPLXmlReader_Init(&reader, buffer + 250, vLen, openCallback, closeCallback,
                dataCallback, &callbackState);
        for (cPos = 0; cPos < xmlLen; cPos += cLen) {
            segmentLen = xmlLen - cPos;
            if (segmentLen > cLen) {
                segmentLen = cLen;
            }
            rv = VPLXmlReader_CurlWriteCallback(xml + cPos, 1, segmentLen, &reader);
            VPLTEST_CHK_EQUAL(rv, segmentLen, FMTu_size_t, "curl callback");
        }
        VPLTEST_CHK_EQUAL(callbackState.callnum,
                callbackState.num_calls_expected, "%d", "Wrong number of total calls to callbacks");
        if (memcmp(buffer + 230, "Magic String len 20", 20) != 0) {
            VPLTEST_NONFATAL_ERROR("buffer underrun detected");
        }
        if (memcmp(buffer + 250 + vLen, "Str#2 String len 20", 20) != 0) {
            VPLTEST_NONFATAL_ERROR("buffer overrun detected");
        }
    }

    //% Bug 9243
    // Start with vLenIdx = 3, since this test needs at least 11 bytes for the opening tag.
    for (vLenIdx = 3; vLenIdx < ARRAY_ELEMENT_COUNT(READER_BUF_LEN); vLenIdx++) {
        const char xml[] = TEST_STRING_5;
        const int xmlLen = (int)strlen(xml);
        ExpectationState_t callbackState = { NUM_CALLS_5, 0, expectedCalls5 };
        _VPLXmlReader reader;

        vLen = READER_BUF_LEN[vLenIdx];
        strcpy(buffer + 230, "Magic String len 20");
        cLen = 160;
        strcpy(buffer + 250 + vLen, "Str#2 String len 20");
        VPLTEST_LOG("vplex buffer size %d, curl buffer size "FMTu_size_t, vLen, cLen);
        VPLXmlReader_Init(&reader, buffer + 250, vLen, openCallback, closeCallback,
                dataCallback, &callbackState);
        for (cPos = 0; cPos < xmlLen; cPos += cLen) {
            segmentLen = xmlLen - cPos;
            if (segmentLen > cLen) {
                segmentLen = cLen;
            }
            rv = VPLXmlReader_CurlWriteCallback(xml + cPos, 1, segmentLen, &reader);
            VPLTEST_CHK_EQUAL(rv, segmentLen, FMTu_size_t, "curl callback");
        }
        VPLTEST_CHK_EQUAL(callbackState.callnum,
                callbackState.num_calls_expected, "%d", "Wrong number of total calls to callbacks");
        if (memcmp(buffer + 230, "Magic String len 20", 20) != 0) {
            VPLTEST_NONFATAL_ERROR("buffer underrun detected");
        }
        if (memcmp(buffer + 250 + vLen, "Str#2 String len 20", 20) != 0) {
            VPLTEST_NONFATAL_ERROR("buffer overrun detected");
        }
    }
}

//
//  Copyright (C) 2009, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_safe_serialization.h"

#include "vplex_strings.h"
#include "vplexTest.h"

/// a list of the testing primitives defined here, in lieu of a header for this file...
void testSerializePrimitives(void);
void testOverrunOutput(void);
void testOverrunInput(void);
void testCallAfterOverrun(void);
void testPackString(void);
void testTruncatePackString(void);
void testUnpackUnterminatedString(void);

void testSerializePrimitives(void) {
    const s8 _s8 = -105;
    const u8 _u8 = 97;
    const s16 _s16 = -2456;
    const u16 _u16 = 35757;
    const s32 _s32 = 0xb71c0952;
    const u32 _u32 = 0xc7e85992;
    const s64 _s64 = 0x90158ac728f3be77ll;
    const u64 _u64 = 0xa81be0a8f9cea98ell;
    s8 v_s8;
    u8 v_u8;
    s16 v_s16;
    u16 v_u16;
    s32 v_s32;
    u32 v_u32;
    s64 v_s64;
    u64 v_u64;
    char buffer[256];
    VPLOutputBuffer out;
    VPLInputBuffer in;

    VPLTEST_LOG("Pack and unpack primitives");
    VPLInitOutputBuffer(&out, buffer, 256);
    VPLPackS8(&out, _s8);
    VPLPackU8(&out, _u8);
    VPLPackS16(&out, _s16);
    VPLPackU16(&out, _u16);
    VPLPackS32(&out, _s32);
    VPLPackU32(&out, _u32);
    VPLPackS64(&out, _s64);
    VPLPackU64(&out, _u64);
    if (VPLHasOverrunOut(&out)) {
        VPLTEST_NONFATAL_ERROR("Should not have overrun output buffer");
    }

    VPLInitInputBuffer(&in, buffer, 256);
    VPLUnpackS8(&in, &v_s8);
    VPLUnpackU8(&in, &v_u8);
    VPLUnpackS16(&in, &v_s16);
    VPLUnpackU16(&in, &v_u16);
    VPLUnpackS32(&in, &v_s32);
    VPLUnpackU32(&in, &v_u32);
    VPLUnpackS64(&in, &v_s64);
    VPLUnpackU64(&in, &v_u64);
    if (VPLHasOverrunIn(&in)) {
        VPLTEST_NONFATAL_ERROR("Should not have overrun input buffer");
    }
    VPLTEST_CHK_EQUAL(v_s8, _s8, FMTs8, "s8 value");
    VPLTEST_CHK_EQUAL(v_u8, _u8, FMTu8, "u8 value");
    VPLTEST_CHK_EQUAL(v_s16, _s16, FMTs16, "s16 value");
    VPLTEST_CHK_EQUAL(v_u16, _u16, FMTu16, "u16 value");
    VPLTEST_CHK_EQUAL(v_s32, _s32, FMTs32, "s32 value");
    VPLTEST_CHK_EQUAL(v_u32, _u32, FMTu32, "u32 value");
    VPLTEST_CHK_EQUAL(v_s64, _s64, FMTs64, "s64 value");
    VPLTEST_CHK_EQUAL(v_u64, _u64, FMTu64, "u64 value");
}

void testOverrunOutput(void) {
    char buffer[5];
    VPLOutputBuffer out;

    VPLTEST_LOG("Attempt to overrun the output");
    VPLInitOutputBuffer(&out, buffer, 5);
    VPLPackS32(&out, 12345);
    VPLPackU16(&out, (u16)52);
    if (!VPLHasOverrunOut(&out)) {
        VPLTEST_NONFATAL_ERROR("Should have detected overrun");
    }
}

void testOverrunInput(void) {
    char buffer[5] = {0,0,0,0,0};
    VPLInputBuffer in;
    s32 _s32;
    u16 _u16;

    VPLTEST_LOG("Attempt to overrun the input");
    VPLInitInputBuffer(&in, buffer, 5);
    VPLUnpackS32(&in, &_s32);
    VPLUnpackU16(&in, &_u16);
    if (!VPLHasOverrunIn(&in)) {
        VPLTEST_NONFATAL_ERROR("Should have detected overrun");
    }
}

void testCallAfterOverrun(void) {
    char buffer[5] = {0,0,0,0,0};
    VPLInputBuffer in;
    s32 _s32;
    u16 _u16;

    VPLTEST_LOG("Attempt to make more calls after an overrun");
    VPLInitInputBuffer(&in, buffer, 5);
    VPLUnpackS32(&in, &_s32);
    VPLUnpackU16(&in, &_u16);
    if (!VPLHasOverrunIn(&in)) {
        VPLTEST_NONFATAL_ERROR("Should have detected overrun");
    }
    VPLUnpackU16(&in, &_u16);
    VPLUnpackU16(&in, &_u16);
    if (!VPLHasOverrunIn(&in)) {
        VPLTEST_NONFATAL_ERROR("Should have detected overrun");
    }
}

void testPackString(void) {
    const char * str = "A string";
    char buffer[256];
    VPLOutputBuffer out;
    VPLInputBuffer in;
    char strIn[256];

    VPLTEST_LOG("Pack and unpack a string");
    VPLInitOutputBuffer(&out, buffer, 256);
    VPLPackString(&out, str, 100);
    VPLInitInputBuffer(&in, buffer, 256);
    VPLUnpackString(&in, strIn, 100);
    VPLTEST_CHK_EQUAL(in.pos, 9, FMTu32, "Input buffer position");
    VPLTEST_CHK_EQ_STRING(strIn, str, "After packing and unpacking");
}

void testTruncatePackString(void) {
    const char * str = "A string";
    const char * truncStr = "A st";
    char buffer[256];
    VPLOutputBuffer out;
    VPLInputBuffer in;
    char strIn[256];

    VPLTEST_LOG("Truncate a string for packing");
    VPLInitOutputBuffer(&out, buffer, 256);
    VPLPackString(&out, str, 4);
    VPLInitInputBuffer(&in, buffer, 256);
    VPLUnpackString(&in, strIn, 4);
    VPLTEST_CHK_EQUAL(in.pos, 5, FMTu32, "Input buffer position");
    VPLTEST_CHK_EQ_STRING(strIn, truncStr, "After being truncated");
}

void testUnpackUnterminatedString(void) {
    const char * str = "aaaaaaaaaaaaaaaa";
    VPLInputBuffer in;
    char strIn[20];

    VPLTEST_LOG("Unpack unterminated string");
    VPLInitInputBuffer(&in, str, 10); // string goes past end
    VPLUnpackString(&in, strIn, 9);
    if (!VPLHasOverrunIn(&in)) {
        VPLTEST_NONFATAL_ERROR("Expected overrun on unterminated string");
    }
}

void testVPLSafeSerialization(void)
{
    // It would be good to have a test that detects reading past the end of a
    // buffer but that is hard to do.
    testSerializePrimitives();
    testOverrunOutput();
    testOverrunInput();
    testCallAfterOverrun();
    testPackString();
    testTruncatePackString();
    testUnpackUnterminatedString();
}

#include <vpl_conv.h>
#include "vplTest.h"
#include <stdio.h>

/// forward declarations of test entrypoints.

int Test_s64(void);
int Test_u64(void);

int Test_u32(void);
int Test_s32(void);

int Test_u16(void);
int Test_s16(void);

int Test_u8(void);
int Test_s8(void);


int Test_s64(void)
{
#if VPL_HOST_IS_LITTLE_ENDIAN
  s64 converted = 0x12EFCDAB78563412ll;
#else
  s64 converted = 0x12345678ABCDEF12ll;
#endif

  s64 source = 0x12345678ABCDEF12ll;

  if ( VPLConv_ntoh_s64(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
  if ( VPLConv_hton_s64(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
}

int Test_u64(void)
{
#if VPL_HOST_IS_LITTLE_ENDIAN
  u64 converted = 0x12EFCDAB78563412ll;
#else
  u64 converted = 0x12345678ABCDEF12ll;
#endif

  u64 source = 0x12345678ABCDEF12ll;

  if ( VPLConv_ntoh_u64(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
  if ( VPLConv_hton_u64(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
}

int Test_s32(void)
{
#if VPL_HOST_IS_LITTLE_ENDIAN
  s32 converted = 0x78563412;
#else
  s32 converted = 0x12345678;
#endif

  s32 source = 0x12345678;

  if ( VPLConv_ntoh_s32(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
  if ( VPLConv_hton_s32(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
}

int Test_u32(void)
{
#if VPL_HOST_IS_LITTLE_ENDIAN
  u32 converted = 0x78563412;
#else
  u32 converted = 0x12345678;
#endif

  u32 source = 0x12345678;

  if ( VPLConv_ntoh_u32(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
  if ( VPLConv_hton_u32(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
}

int Test_s16(void)
{
#if VPL_HOST_IS_LITTLE_ENDIAN
  s16 converted = 0x3412;
#else
  s16 converted = 0x1234;
#endif

  s16 source = 0x1234;

  if ( VPLConv_ntoh_s16(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
  if ( VPLConv_hton_s16(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
}

int Test_u16(void)
{
#if VPL_HOST_IS_LITTLE_ENDIAN
  u16 converted = 0x3412;
#else
  u16 converted = 0x1234;
#endif

  u16 source = 0x1234;

  if ( VPLConv_ntoh_u16(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
  if ( VPLConv_hton_u16(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
}

int Test_s8(void)
{
  s8 converted = 0x12;
  s8 source = 0x12;

  if ( VPLConv_ntoh_s8(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
  if ( VPLConv_hton_s8(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
}

int Test_u8(void)
{
  u8 converted = 0x12;
  u8 source = 0x12;

  if ( VPLConv_ntoh_u8(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
  if ( VPLConv_hton_u8(source) != converted ) {
    return -1;
  } else {
    return 0;
  }
}

static void Test_VPLConv_strToU64()
{
    // VPLConv_strToU64 promises to support an input range of
    //  [INT64_MIN (-2^63), UINT64_MAX (2^64 - 1)].

    // First, check positive values:
    {
        // 2^64 - 1 (UINT64_MAX)
        VPLTEST_CALL_AND_CHK_UNSIGNED(VPLConv_strToU64("18446744073709551615", NULL, 10), UINT64_MAX);

        // 2^63 - 1 (INT64_MAX)
        VPLTEST_CALL_AND_CHK_UNSIGNED(VPLConv_strToU64("9223372036854775807", NULL, 10), INT64_MAX);

        // 2^63 (INT64_MAX + 1)
        VPLTEST_CALL_AND_CHK_UNSIGNED(VPLConv_strToU64("9223372036854775808", NULL, 10), UINT64_C(9223372036854775808));

        // 0
        VPLTEST_CALL_AND_CHK_UNSIGNED(VPLConv_strToU64("0", NULL, 10), UINT64_C(0));
    }

    // Check that negative values are remapped to unsigned:
    {
        // -1 => UINT64_MAX
        VPLTEST_CALL_AND_CHK_UNSIGNED(VPLConv_strToU64("-1", NULL, 10), UINT64_MAX);

        // -2 => UINT64_MAX - 1
        VPLTEST_CALL_AND_CHK_UNSIGNED(VPLConv_strToU64("-2", NULL, 10), UINT64_C(18446744073709551614));

        // INT64_MIN => INT64_MAX + 1
        VPLTEST_CALL_AND_CHK_UNSIGNED(VPLConv_strToU64("-9223372036854775808", NULL, 10), UINT64_C(9223372036854775808));

        // INT64_MIN + 1 => INT64_MAX + 2
        VPLTEST_CALL_AND_CHK_UNSIGNED(VPLConv_strToU64("-9223372036854775807", NULL, 10), UINT64_C(9223372036854775809));
    }
}

void testVPLConv(void)
{
  APP_LOG("s64 conversions\n");
  if ( Test_s64() ) {
    APP_LOG("FAIL: s64 conversions\n");
    vplTest_incrErrCount();
  }

  APP_LOG("u64 conversions\n");
  if ( Test_u64() ) {
    APP_LOG("FAIL: u64 conversions\n");
    vplTest_incrErrCount();
  }

  APP_LOG("s32 conversions\n");
  if ( Test_s32() ) {
    APP_LOG("FAIL: s32 conversions\n");
    vplTest_incrErrCount();
  }

  APP_LOG("u32 conversions\n");
  if ( Test_u32() ) {
    APP_LOG("FAIL: u32 conversions\n");
    vplTest_incrErrCount();
  }

  APP_LOG("s16 conversions\n");
  if ( Test_s16() ) {
    APP_LOG("FAIL: s16 conversions\n");
    vplTest_incrErrCount();
  }

  APP_LOG("u16 conversions\n");
  if ( Test_u16() ) {
    APP_LOG("FAIL: u16 conversions\n");
    vplTest_incrErrCount();
  }

  APP_LOG("s8 conversions\n");
  if ( Test_s8() ) {
    APP_LOG("FAIL: s8 conversions\n");
    vplTest_incrErrCount();
  }

  APP_LOG("u8 conversions\n");
  if ( Test_u8() ) {
    APP_LOG("FAIL: u8 conversions\n");
    vplTest_incrErrCount();
  }

  VPLTEST_LOG("VPLConv_strToU64 robustness");
  Test_VPLConv_strToU64();
}

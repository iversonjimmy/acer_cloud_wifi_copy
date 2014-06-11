//
//  Copyright (C) 2007-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vpl_error.h"
#include "vplex_math.h"
#include <math.h>
#include "vplexTest.h"

// NIST monobit frequency test has a small chance that it could fail.
// Keep it disabled unless PRNG code has been changed.
//#define RUN_NIST_FREQ_TEST

// TODO: Need to somehow test that the random numbers are good enough.
// The definition of "good enough" is also needed.

#define NUMBERS_TO_TEST 10

static void runCountZerosTest(void)
{
    int rc;
    u32 numbers[NUMBERS_TO_TEST];
    int i, zeroes;

    VPLTEST_LOG("Initializing random numbers.");
    rc = VPLMath_InitRand();
    VPLTEST_CHK_OK(rc, "Random numbers initialization");

    VPLTEST_LOG("Get some random numbers\n");
    zeroes = 0;
    for(i = 0; i < NUMBERS_TO_TEST; i++) {
        numbers[i] = 0;
        numbers[i] = VPLMath_Rand();
        VPLTEST_LOG("Numbers %d: %d", i, numbers[i]);

        if(numbers[i] == 0) {
            zeroes++;
        }
    }
    
    // Pretty arbitrary pass condition.
    // More than two zeroes is possible, but extremely unlikely.
    if(zeroes > 2) {
        VPLTEST_NONFATAL_ERROR("%d zeroes were seen in the random numbers, which isn't likely.", zeroes);
    }
}

#ifdef RUN_NIST_FREQ_TEST
// Frequency (Monobit) Test
// cf. NIST SP800-22 Sec 2.1.
static void runFrequencyMonobitTest(void)
{
    int rc;
    int i, j, k;

    // test params
    double alpha = 0.01;
    int nSamples = 1000;
    int seqLenInU32 = 10;  // each sample is a 320-bit sequence

    int nPass = 0;

    rc = VPLMath_InitRand();
    VPLTEST_CHK_OK(rc, "PRNG initialized OK");

    for (i = 0; i < nSamples; i++) {  // for each sample
        int Sn = 0;
        for (j = 0; j < seqLenInU32; j++) {  // for each u32 in a sample
            uint32_t num = VPLMath_Rand();
            for (k = 0; k < 32; k++) {  // for each bit pos in u32
                if (num & (1 << k)) {
                    Sn++;
                }
                else {
                    Sn--;
                }
            }
        }
        {
            double Sobs = fabs((double)Sn) / sqrt(seqLenInU32 * 32);
            // erfc(2.57584659/sqrt(2)) = 0.01 (alpha)
            if (Sobs <= 2.57584659) {
                nPass++;
            }
        }
    }

    {
        // cf. SP800-22 Sec 4.2.1
        double passRate = (double)nPass / nSamples;
        double passRateLowerBound = (1.0 - alpha) - 3.0 * sqrt((1.0 - alpha) * alpha / nSamples);
        VPLTEST_CHK_GREATER_OR_EQ(passRate, passRateLowerBound, "%f", "Frequency (Monobit) Test");
    }
}
#endif

void testVPLMath(void)
{
    runCountZerosTest();

#ifdef RUN_NIST_FREQ_TEST
    runFrequencyMonobitTest();
#endif
}

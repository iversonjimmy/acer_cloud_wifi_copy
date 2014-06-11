#include <vpl_types.h>
#include <vpl_time.h>
#include <vpl_th.h>

#include "gvmtest_load.h"
#include "microbench_profil.h"

#include "vpl_microbenchmark.h"

#define NSAMPLES (1024 * 1024)


int
main(int argc, char *argv[])
{
    VPLMutex__t myMutex;
    unsigned i;
    int nSamples = NSAMPLES;

    VPLProfil_SampleVector_t *MutexCreate_timings = 0;
    VPLProfil_SampleVector_t *MutexDestroy_timings = 0;
    VPLProfil_SampleVector_t *MutexAcquire_timings = 0;
    VPLProfil_SampleVector_t *MutexRelease_timings  = 0;
    int samples_flags = 0;

    // Parse arguments...
    parse_args(argc, argv, &nSamples, &samples_flags);

    MutexCreate_timings = VPLProfil_SampleVector_Create(nSamples, "MutexCreate", samples_flags);

    /// Time mutex creation...
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;

        VPLProfil_Sample_Start(&theSample);
        VPLMutex_Init(&myMutex);
        VPLProfil_Sample_End(&theSample);

        
        VPLMutex_Destroy(&myMutex);

        // save the sample...
        VPLProfil_SampleVector_Save(MutexCreate_timings, &theSample);

    }


    VPLProfil_SampleVector_Print(stdout,MutexCreate_timings);
    VPLProfil_SampleVector_Destroy(MutexCreate_timings);

    /// Time mutex destruction...
    MutexDestroy_timings = VPLProfil_SampleVector_Create(nSamples, "MutexDestroy", samples_flags);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;

        VPLMutex_Init(&myMutex);
        
        VPLProfil_Sample_Start(&theSample);
        VPLMutex_Destroy(&myMutex);
        VPLProfil_Sample_End(&theSample);

        // save the sample...
        VPLProfil_SampleVector_Save(MutexDestroy_timings, &theSample);

    }
    VPLProfil_SampleVector_Print(stdout, MutexDestroy_timings);
    VPLProfil_SampleVector_Destroy(MutexDestroy_timings);


    /// Time mutex acquire, no contenion...
    MutexAcquire_timings = VPLProfil_SampleVector_Create(nSamples, "MutexAcquire", samples_flags);
    VPLMutex_Init(&myMutex);
    
    for (i = 0; i < nSamples; i++) {
        
        struct sample theSample;

        VPLProfil_Sample_Start(&theSample);
        VPLMutex_Lock(&myMutex);
        VPLProfil_Sample_End(&theSample);

        VPLMutex_Unlock(&myMutex);
        // save the sample...
        VPLProfil_SampleVector_Save(MutexAcquire_timings, &theSample);

    }
    VPLMutex_Destroy(&myMutex);
    VPLProfil_SampleVector_Print(stdout,MutexAcquire_timings);
    VPLProfil_SampleVector_Destroy(MutexAcquire_timings);


    VPLMutex_Init(&myMutex);

    /// Time mutex release, no contenion...
    MutexRelease_timings = VPLProfil_SampleVector_Create(nSamples, "MutexRelease", samples_flags);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;

        VPLMutex_Lock(&myMutex);

        VPLProfil_Sample_Start(&theSample);
        VPLMutex_Unlock(&myMutex);
        VPLProfil_Sample_End(&theSample);

        // save the sample...
        VPLProfil_SampleVector_Save(MutexRelease_timings, &theSample);

    }
    VPLMutex_Destroy(&myMutex);
    VPLProfil_SampleVector_Print(stdout,MutexRelease_timings);
    VPLProfil_SampleVector_Destroy(MutexRelease_timings);

    return 0;
}

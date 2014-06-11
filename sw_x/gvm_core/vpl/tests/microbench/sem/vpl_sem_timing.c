
#include <vpl_types.h>
#include <vpl_time.h>
#include <vpl_th.h>

#include "gvmtest_load.h"

#include "microbench_profil.h"
#include "vpl_microbenchmark.h"  // shared parse_args()


/// Default number of samples to collect.
#define NSAMPLES (1024 * 1024)



int
main(int argc, char *argv[])
{
    VPLSem__t mySem;
    unsigned i;

    int nSamples = NSAMPLES;
    int samples_flags = 0;

    VPLProfil_SampleVector_t *SemCreate_timings = 0;
    VPLProfil_SampleVector_t *SemDestroy_timings = 0;
    VPLProfil_SampleVector_t *SemPost_timings = 0;
    VPLProfil_SampleVector_t *SemWait_timings  = 0;


    // Parse arguments...
    parse_args(argc, argv, &nSamples, &samples_flags);

    /// Time sem creation...
    SemCreate_timings = VPLProfil_SampleVector_Create(nSamples, "SemCreate", samples_flags);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;

        VPLProfil_Sample_Start(&theSample);
        VPLSem_Init(&mySem, 31, 0);
        VPLProfil_Sample_End(&theSample);
        
        VPLSem_Destroy(&mySem);

        // save the sample...
        VPLProfil_SampleVector_Save(SemCreate_timings, &theSample);

    }
    VPLProfil_SampleVector_Print(stdout, SemCreate_timings);
    VPLProfil_SampleVector_Destroy(SemCreate_timings);

    
    /// Time sem destruction...
    SemDestroy_timings = VPLProfil_SampleVector_Create(nSamples, "SemDestroy", samples_flags);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;

        VPLSem_Init(&mySem, 31, 0);
        
        VPLProfil_Sample_Start(&theSample);
        VPLSem_Destroy(&mySem);
        VPLProfil_Sample_End(&theSample);

        // save the sample...
        VPLProfil_SampleVector_Save(SemDestroy_timings, &theSample);

    }
    VPLProfil_SampleVector_Print(stdout, SemDestroy_timings);
    VPLProfil_SampleVector_Destroy(SemDestroy_timings);



    /// Time sem post, no contenion...
    SemPost_timings = VPLProfil_SampleVector_Create(nSamples, "SemPost", samples_flags);
    VPLSem_Init(&mySem, 1, 0);
    for (i = 0; i < nSamples; i++) {
        
        struct sample theSample;

        VPLProfil_Sample_Start(&theSample);
        VPLSem_Post(&mySem);
        VPLProfil_Sample_End(&theSample);

        // make it ready to post again...
        VPLSem_Wait(&mySem);
        // save the sample...

        VPLProfil_SampleVector_Save(SemPost_timings,  &theSample);

    }
    VPLSem_Destroy(&mySem);
    VPLProfil_SampleVector_Print(stdout, SemPost_timings);
    VPLProfil_SampleVector_Destroy(SemPost_timings);


    /// Time sem wait, no contention...
    VPLSem_Init(&mySem, 1, 0);
    SemWait_timings = VPLProfil_SampleVector_Create(nSamples, "SemWait", samples_flags);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;

        VPLSem_Post(&mySem);

        VPLProfil_Sample_Start(&theSample);
        VPLSem_Wait(&mySem);
        VPLProfil_Sample_End(&theSample);

        // save the sample...
        VPLProfil_SampleVector_Save(SemWait_timings, &theSample);
    }
    VPLSem_Destroy(&mySem);
    VPLProfil_SampleVector_Print(stdout, SemWait_timings);
    VPLProfil_SampleVector_Destroy(SemWait_timings);

    return 0;
}

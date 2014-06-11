#include <vpl_types.h>
#include <vplu_atomic.h>

#include <stdio.h>

#include "gvmtest_load.h"

#include "microbench_profil.h"
#include "vpl_microbenchmark.h"


/// Default number of samples to collect.
#define NSAMPLES (1024 * 1024)

int
main(int argc, char *argv[])
{
// Bug 6069: Atomic Ops only supported on x86
#ifdef VPL_PLAT_IS_X86
    int i;
    int nSamples = NSAMPLES;
    int samples_flags = 0;
    int32_t counter = 0;
    VPLProfil_SampleVector_t *AtomicAdd_timings = 0;


    // Parse arguments...
    parse_args(argc, argv, &nSamples, &samples_flags);

    /// Time sem creation...
    AtomicAdd_timings = VPLProfil_SampleVector_Create(nSamples, "AtomicAdd", samples_flags);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;

        VPLProfil_Sample_Start(&theSample);
        VPLAtomic_add(&counter, 1);
        VPLProfil_Sample_End(&theSample);
        
        // save the sample...
        VPLProfil_SampleVector_Save(AtomicAdd_timings, &theSample);

    }
    VPLProfil_SampleVector_Print(stdout, AtomicAdd_timings);
    VPLProfil_SampleVector_Destroy(AtomicAdd_timings);
#else
#error "Unsupported architecture"
#endif
    return 0;
}

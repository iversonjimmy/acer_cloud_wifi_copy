#include <vpl_types.h>
#include <vpl_time.h>
#include <vpl_th.h>


#include "gvmtest_load.h"
#include "microbench_profil.h"
#include "vpl_microbenchmark.h"  // shared parse_args()


#define NSAMPLES (1024 * 1024)


/// Globals, for capturing timestamps in the context of a child thread...
VPLProfil_SampleVector_t *gThreadSamples = 0;

struct sample     gThreadSample = { {0,} };


int
main(int argc, char *argv[])
{
    int nSamples = NSAMPLES;
    int samples_flags = 0;

    VPLThread_t myThread;
    volatile unsigned i;

    VPLProfil_SampleVector_t *ThreadSetPrio_timings =  0;
    VPLProfil_SampleVector_t *ThreadGetPrio_timings =  0;


    // Parse arguments...
    parse_args(argc, argv, &nSamples, &samples_flags);
    

    /// Time set thread-priority...
    ThreadSetPrio_timings = VPLProfil_SampleVector_Create(nSamples, "ThreadSetPrio", samples_flags);

    myThread = VPLThread_Self();
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;

        VPLProfil_Sample_Start(&theSample);
        VPLThread_SetSchedPriority(VPL_PRIO_MIN);
        VPLProfil_Sample_End(&theSample);

        VPLProfil_SampleVector_Save(ThreadSetPrio_timings, &theSample);
    }

    VPLProfil_SampleVector_Print(stdout,ThreadSetPrio_timings);
    VPLProfil_SampleVector_Destroy(ThreadSetPrio_timings);

    // ////////////////////////////////////////////////////////////////////////

    /// Time get-thread-priority
    ThreadGetPrio_timings = VPLProfil_SampleVector_Create(nSamples, "ThreadgetPrio", samples_flags);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;
        int myPrio;

        VPLProfil_Sample_Start(&theSample);

        myPrio = VPLThread_GetSchedPriority();
        VPLProfil_Sample_End(&theSample);

        VPLProfil_SampleVector_Save(ThreadGetPrio_timings, &theSample);
    }

    VPLProfil_SampleVector_Print(stdout,ThreadGetPrio_timings);
    VPLProfil_SampleVector_Destroy(ThreadGetPrio_timings);


    return 0;
}

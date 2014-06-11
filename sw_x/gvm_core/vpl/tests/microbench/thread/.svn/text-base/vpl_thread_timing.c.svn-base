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

/// Dummy thread, for measurement purposes.
/// Just exit.
static VPLThread_return_t
dummyThreadEntry(VPLThread_arg_t arg)
{
    return ((VPLThread_return_t)arg);
}



static VPLThread_return_t
timeThreadEntry(VPLThread_arg_t arg)
{

    // First thing we do, is time-stamp when this thread started.
    VPLProfil_Sample_End(&gThreadSample);

    // Save that sample. Our creator set the start-time in gThreadSample.
    VPLProfil_SampleVector_Save(gThreadSamples, &gThreadSample);

    // All done.
    return ((VPLThread_return_t)arg);
}


int
main(int argc, char *argv[])
{
    int nSamples = NSAMPLES;
    int samples_flags = 0;

    VPLThread_t myThread;
    volatile unsigned i;

    VPLProfil_SampleVector_t *ThreadCreate_timings =  0;
#if 0
    VPLProfil_SampleVector_t *ThreadDestroy_timings =  0;
#endif

    // Parse arguments...
    parse_args(argc, argv, &nSamples, &samples_flags);
    
    // check for resource leak...
    for (i = 0; i < nSamples; i++) {

        VPLThread_Create(&myThread,  &dummyThreadEntry, (VPLThread_arg_t)i, 0, "timed thread" );
        VPLThread_Join(&myThread, 0);
    }
    //exit(0);

    /// Time thread-creation...
    ThreadCreate_timings = VPLProfil_SampleVector_Create(nSamples, "ThreadCreate", samples_flags);

    for (i = 0; i < nSamples; i++) {
        struct sample theSample;

        VPLProfil_Sample_Start(&theSample);
        VPLThread_Create(&myThread,  &dummyThreadEntry, (VPLThread_arg_t)i, 0, "timed thread" );
        VPLProfil_Sample_End(&theSample);

        VPLThread_Join(&myThread, 0); // avoid any potential resource-leaks...
        VPLProfil_SampleVector_Save(ThreadCreate_timings, &theSample);
    }

    VPLProfil_SampleVector_Print(stdout,ThreadCreate_timings);
    VPLProfil_SampleVector_Destroy(ThreadCreate_timings);

    // ////////////////////////////////////////////////////////////////////////

#if 0
    /// Time thread-destruction...
    ThreadDestroy_timings = VPLProfil_SampleVector_Create(nSamples);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;
        VPLThread_Create(&myThread,  &dummyThreadEntry, (VPLThread_arg_t)i, 0, "timed thread" );
        
        Sample_Start(&theSample);;
        (void) VPLThread_Destroy(myThread);
        VPLProfil_Sample_End(&theSample);

        VPLProfil_SampleVector_Save(ThreadDestroy_timings, &theSample);
    }

    VPLProfil_SampleVector_Print(stdout,ThreadDestroy_timings);
    VPLProfil_SampleVector_Destroy(ThreadDestroy_timings);

#endif
    /// Time thread-create call to wakeup  in child thread.

    gThreadSamples = VPLProfil_SampleVector_Create(nSamples, "ThreadCreate-to-entry", samples_flags);
    for (i = 0; i < nSamples; i++) {

         VPLProfil_Sample_Start(&gThreadSample);

        // Create a thread. The thread will save a sample, of
        // gThreadCreated and the thread's entry time.
        VPLThread_Create(&myThread,  &timeThreadEntry, (VPLThread_arg_t)i, 0, "timestamp thread-entry" );

        // let the child do the rest...
        // but make sure it has exited before we start another iteration.
        VPLThread_Join(&myThread, 0);
    }

    VPLProfil_SampleVector_Print(stdout, gThreadSamples);
    VPLProfil_SampleVector_Destroy(gThreadSamples);
    gThreadSamples  = 0;

    return 0;
}

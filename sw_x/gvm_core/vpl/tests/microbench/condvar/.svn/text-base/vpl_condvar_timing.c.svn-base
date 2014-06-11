#include <vpl_types.h>
#include <vpl_time.h>
#include <vpl_th.h>

#include "gvmtest_load.h"
#include "microbench_profil.h"

#include "vpl_microbenchmark.h"


#define NSAMPLES (1024 * 1024)


typedef struct condvar_args {
    VPLMutex_t *mutex;
    VPLCond_t *condvar;
} Condvar_args_t;

//// Thread to passively wait on a condvar, so we can
/// time sending that thread a wakeup.
static VPLThread_return_t
condWaiterThreadEntry(VPLThread_arg_t arg)
{
    Condvar_args_t *myArgs = (Condvar_args_t *)arg;

    // now just wait on the given cond-var...
    VPLCond_TimedWait(myArgs->condvar, myArgs->mutex, VPL_TIMEOUT_NONE);

    return ((VPLThread_return_t)arg);
}



int
main(int argc, char *argv[])
{
    VPLCond__t myCondvar;
    VPLMutex_t myMutex;
    VPLThread_t myThread;

    unsigned i;
    int nSamples = NSAMPLES;

    VPLProfil_SampleVector_t *CondvarCreate_timings = 0;
    VPLProfil_SampleVector_t *CondvarDestroy_timings = 0;
    VPLProfil_SampleVector_t *CondvarSignal_timings = 0;
#ifdef notyet
    VPLProfil_SampleVector_t *CondvarWait_timings  = 0;
#endif

    int samples_flags = 0;
    
    // Parse arguments...
    parse_args(argc, argv, &nSamples, &samples_flags);


    VPLMutex_Init(&myMutex);

    /// Time condvar creation...
    CondvarCreate_timings = VPLProfil_SampleVector_Create(nSamples, "CondCreate", samples_flags);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;

        VPLProfil_Sample_Start(&theSample);
        VPLCond_Init(&myCondvar);
        VPLProfil_Sample_End(&theSample);
        
        VPLCond_Destroy(&myCondvar);

        // save the sample...
        VPLProfil_SampleVector_Save(CondvarCreate_timings, &theSample);

    }
    VPLProfil_SampleVector_Print(stdout, CondvarCreate_timings);
    VPLProfil_SampleVector_Destroy(CondvarCreate_timings);



    /// Time condvar destruction...
    CondvarDestroy_timings = VPLProfil_SampleVector_Create(nSamples, "CondDestroy", samples_flags);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;

        VPLCond_Init(&myCondvar);
        
        VPLProfil_Sample_Start(&theSample);
        VPLCond_Destroy(&myCondvar);
        VPLProfil_Sample_End(&theSample);


        // save the sample...
        VPLProfil_SampleVector_Save(CondvarDestroy_timings, &theSample);

    }
    VPLProfil_SampleVector_Print(stdout, CondvarDestroy_timings);
    VPLProfil_SampleVector_Destroy(CondvarDestroy_timings);



    /// Time condvar signal, no contenion...
    VPLCond_Init(&myCondvar);
    CondvarSignal_timings = VPLProfil_SampleVector_Create(nSamples, "CondSignal", samples_flags);
    
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;
    
        // Create Waiting thread ...
        VPLThread_Create(&myThread,
                         &condWaiterThreadEntry, (VPLThread_arg_t)&myCondvar,
                         0, "timed thread" );
        VPLThread_Yield(); // yield, to be sure the child thread is waiting on the convdar.

        // Time the signal.
        VPLProfil_Sample_Start(&theSample);
        VPLCond_Signal(&myCondvar);
        VPLProfil_Sample_End(&theSample);


        // save the sample...
        VPLProfil_SampleVector_Save(CondvarSignal_timings, &theSample);

    }
    VPLCond_Destroy(&myCondvar);
    VPLProfil_SampleVector_Print(stdout, CondvarSignal_timings);
    VPLProfil_SampleVector_Destroy(CondvarSignal_timings);



    /// Time condvar wakeup?? how?
#ifdef notyet
    VPLCond_Init(&myCondvar);

    for (i = 0; i < nSamples; i++) {
        
        struct sample theSample;

        VPLProfil_Sample_Start(&theSample);
        VPLCond_Lock(&myCondvar);

        // TODO: fire off a thread which will wait on the condvar???

        VPLCond_Unlock(&myCondvar);
        VPLProfil_Sample_End(&theSample);
        // save the sample...
        VPLProfil_SampleVector_Save(0, &theSample);

    }
    VPLCond_Destroy(&myCondvar);
#endif //notyet
    
    VPLMutex_Destroy(&myMutex);

    return 0;
}

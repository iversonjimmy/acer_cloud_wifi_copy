#include <vpl_types.h>
#include <vpl_time.h>
#include <vpl_th.h>

#include <errno.h>
#include <sysexits.h>

#include <vpl_microbenchmark.h>

#include "gvmtest_load.h"
#include "microbench_profil.h"

#define NSAMPLES (1024 * 1024)





int
main(int argc, char *argv[])
{
    VPLMsgQ_t myMsgQ;
    unsigned i;

    VPLProfil_SampleVector_t *MsgQCreate_timings = 0;
    VPLProfil_SampleVector_t *MsgQDestroy_timings = 0;
    VPLProfil_SampleVector_t *MsgQPut_timings = 0;
    VPLProfil_SampleVector_t *MsgQGet_timings = 0;

    char* msgPutPtr = (char*) 0xbabeface;
    void* msgGetPtr = 0;

    int nSamples = NSAMPLES;
    int samples_flags = 0;

    parse_args(argc, argv, &nSamples, &samples_flags);

    /// Time msgQ creation...
    MsgQCreate_timings = VPLProfil_SampleVector_Create(nSamples, "MsgQCreate", samples_flags);

    for (i = 0; i < NSAMPLES; i++) {
        struct sample theSample;
        int status;

        VPLProfil_Sample_Start(&theSample);
        status = VPLMsgQ_Init(&myMsgQ, 31);
        VPLProfil_Sample_End(&theSample);

        if (status != VPL_OK) {
            fprintf(stderr, "VPLMsgQ_Init() failed: vpl %d err %s\n", status, strerror(errno));
            exit(EX_OSERR);
        }
        VPLMsgQ_Destroy(&myMsgQ);

        /// save sample...
        VPLProfil_SampleVector_Save(MsgQCreate_timings, &theSample);

    }
    VPLProfil_SampleVector_Print(stdout, MsgQCreate_timings);
    VPLProfil_SampleVector_Destroy(MsgQCreate_timings);


    /// Time msgQ destruction...
    MsgQDestroy_timings = VPLProfil_SampleVector_Create(nSamples, "MsgQDestroy", samples_flags);
    for (i = 0; i < NSAMPLES; i++) {
        struct sample theSample;

        VPLMsgQ_Init(&myMsgQ, 31);
        
        VPLProfil_Sample_Start(&theSample);
        VPLMsgQ_Destroy(&myMsgQ);
        VPLProfil_Sample_End(&theSample);

        // save the sample...
        VPLProfil_SampleVector_Save(MsgQDestroy_timings, &theSample);

    }
    VPLProfil_SampleVector_Print(stdout, MsgQDestroy_timings);
    VPLProfil_SampleVector_Destroy(MsgQDestroy_timings);



    /// Time msgQ acquire, no contenion...
    VPLMsgQ_Init(&myMsgQ, 31);
    MsgQPut_timings = VPLProfil_SampleVector_Create(nSamples, "MsgQPut", samples_flags);
    
    for (i = 0; i < NSAMPLES; i++) {
        struct sample theSample;

        VPLProfil_Sample_Start(&theSample);
        VPLMsgQ_Put(&myMsgQ, msgPutPtr);
        VPLProfil_Sample_End(&theSample);

        VPLMsgQ_Get(&myMsgQ, &msgGetPtr);

        // save the sample...
        VPLProfil_SampleVector_Save(MsgQPut_timings, &theSample);

    }
    VPLMsgQ_Destroy(&myMsgQ);
    VPLProfil_SampleVector_Print(stdout, MsgQPut_timings);
    VPLProfil_SampleVector_Destroy(MsgQDestroy_timings);

    VPLMsgQ_Init(&myMsgQ, 31);
    MsgQGet_timings = VPLProfil_SampleVector_Create(nSamples, "MsgQGet", samples_flags);
    /// Time msgQ release, no contenion...
    for (i = 0; i < NSAMPLES; i++) {
        
        struct sample theSample;

        VPLMsgQ_Put(&myMsgQ, msgPutPtr);

        VPLProfil_Sample_Start(&theSample);
        VPLMsgQ_Get(&myMsgQ, &msgGetPtr);
        VPLProfil_Sample_End(&theSample);


        // save the sample...
        VPLProfil_SampleVector_Save(MsgQGet_timings, &theSample);

    }
    VPLProfil_SampleVector_Print(stdout, MsgQGet_timings);
    VPLProfil_SampleVector_Destroy(MsgQGet_timings);
    VPLMsgQ_Destroy(&myMsgQ);

    return 0;
}

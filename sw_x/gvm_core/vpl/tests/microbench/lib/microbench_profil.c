//
//  Copyright (C) 2009-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

// Include our exported API first of all, to make sure those declarations 
// are self-contained and do not require any other headers.
#include <vpl_time.h>
#include "microbench_profil.h" // Nanosecond-resolution VPLex types for profiling purposes.

#include <malloc.h>
#include <string.h>

/// Definition of the opaque abstract type...
struct VPLProfil_SampleVector {
    struct sample* samples;
    int flags;
    unsigned numSamples;
    unsigned maxSamples;
    const char *suffix;
    struct {
      VPLProf_nanosec_t min;
      VPLProf_nanosec_t max;
      double tot;
    } stats;
};




/// ctor...
VPLProfil_SampleVector_t *
VPLProfil_SampleVector_Create(unsigned limit, const char *suffix, int flags)
{
    VPLProfil_SampleVector_t *save = NULL;
    size_t nBytes = ((limit+1) * sizeof(struct sample));

    save = malloc(sizeof(*save));
    memset(save, 0, sizeof(*save));
    
    save->samples = malloc(nBytes);
    memset(save->samples,  0, nBytes);

    save->numSamples = 0;
    save->maxSamples = limit;

    save->suffix = suffix;

    return save;
}

/// dtor...
void VPLProfil_SampleVector_Destroy(VPLProfil_SampleVector_t *save)
{
    free(save->samples);
    memset(save, 0, sizeof(*save));
    free(save);
}


void
VPLProfil_SampleVector_Save(VPLProfil_SampleVector_t *save, const struct sample *newSample)
{
    struct sample *s;

    // silently discard if full..
    if (save->numSamples >= save->maxSamples)
        return;

    VPLProf_nanosec_t delta = SampleTime_Delta(newSample->end_tstamp,  newSample->start_tstamp);

    if (save->numSamples == 0) {
      save->stats.min = delta;
      save->stats.max = delta;
      save->stats.tot += delta;
    } else {
      save->stats.tot += delta;
      if (delta < save->stats.min) 
        save->stats.min = delta;
      if (delta > save->stats.max) 
        save->stats.max = delta;
    }
    
    s = &(save->samples[save->numSamples++]);
    *s =  *newSample;

}




#if 0
static void*
the_VPLProfil_SampleVector(VPLProfil_SampleVector_t *save, int n)
{
    if (n >= save->numSamples) {
        return 0;
    }
    return &(save->samples[n]);
}
#endif

static const struct sample*
const_VPLProfil_SampleVector(const VPLProfil_SampleVector_t *save, int n)
{
    if (n >= save->numSamples) {
        return 0;
    }
    return &(save->samples[n]);
}


int

VPLProfil_SampleVector_Count(const VPLProfil_SampleVector_t * const save)
{
    return save->numSamples;
}


void
VPLProfil_SampleVector_Print(FILE *f, const VPLProfil_SampleVector_t *save)
{
    int i;
    int lim = VPLProfil_SampleVector_Count(save);

    // Print ident line, including our clock-skew estimate.
    if ((save->flags & VPLPROFIL_SAVEDSAMPLES_SUMMARY_ONLY) != 0) {
        lim = 0;
    }
    for (i = 0; i < lim; i++) {
        const struct sample *s = const_VPLProfil_SampleVector(save, i);

        fprintf(f,
                "%d %llu ", //" %llu %lu",
                i, //s->start_tstamp, s->start_tstamp,
                SampleTime_Delta(s->end_tstamp,  s->start_tstamp));

        if (save->suffix != NULL) {
            fprintf(f, " # %s", save->suffix);
        }

        fprintf(f, "\n");
    }

    fprintf(f, "#%s# min: %llu mean: %g max: %llu\n", save->suffix,
            save->stats.min, (save->stats.tot / save->numSamples), save->stats.max );
    fflush(f);
}

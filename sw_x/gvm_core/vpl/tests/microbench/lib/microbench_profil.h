//
//  Copyright (C) 2009-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef  MICROBENCH_TIME_20100408
#define MICROBENCH_TIME_20100408

// a hidden ADT...

#include <vpl_time.h>
#include <vplex_time.h>

#include <time.h>     // Host-supplied declaration of ts_sec, ts_nsec

/// VPL's representation of a nanosecond-granularit time.
/// Compare and contrast to VPLTime_t, which is (currently) microsecond resolution.
typedef uint64_t VPLProf_nanosec_t;


/// TODO: move nanosecond-resolution time to vplex_time.h??
typedef struct VPLTime_spec {
    uint64_t ts_sec;
    VPLProf_nanosec_t ts_nsec;
} VPLTimespec_t;

/// As-yet unused.

/// TODO: create a compact representation for time deltas?
// for exapmle, a raw 64-bit nanosecond counter??

#if 0
typedef VPLTime_t SampleTime_t;
#else
typedef VPLTimespec_t SampleTime_t;
#endif

struct sample  {
    SampleTime_t start_tstamp;
    SampleTime_t end_tstamp;
    SampleTime_t user1;
    SampleTime_t user2;
    SampleTime_t user3;
    SampleTime_t user4;
};


///
/// Encapslaute how we start and stop a single sample timer.
///
/// That way, we can change resolution --- for example, from
/// VPLTime_Gettime(), in milliseconds, to clock_gettime() in
/// nanoseconds ---in just one place. (Here.)

//static inline VPLProf_nanosec_t
// VPLProfil_Sample_GetTime(void) { return clock_gettime(CLOCK_REALTIME, 0); }
static volatile inline SampleTime_t
VPLProfil_Sample_GetTime(void) {
#if 0
    return VPLTime_GetTime();
#else
    SampleTime_t t;
    struct timespec ts;

    //clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    clock_gettime(CLOCK_MONOTONIC/*_RAW*/, &ts);
    t.ts_sec = ts.tv_sec;
    t.ts_nsec = ts.tv_nsec;
    return t;
#endif    
}

/// Return a 64-bit delta, in nanoesconds, between \a end and \a start.
/// @param[in] end end-time of sample
/// @param[in] start start-time of sample
/// @resolt 64-bit count of nanoseconds  between \a end and \a start.
static volatile inline VPLProf_nanosec_t
SampleTime_Delta(const SampleTime_t end, SampleTime_t start)
{
    VPLProf_nanosec_t delta;
    delta = (end.ts_nsec - start.ts_nsec);
    delta += (end.ts_sec - start.ts_sec) * (1000*1000*1000);

    return (VPLProf_nanosec_t)delta;
}

/// Mark the start-point of a single VPLProfil timing-sample.
static inline void
VPLProfil_Sample_Start(volatile struct sample* s) { s->start_tstamp = VPLProfil_Sample_GetTime();}

/// Mark the end-point of a single VPLProfil timing-sample.
static inline void
VPLProfil_Sample_End(volatile struct sample* s) { s->end_tstamp = VPLProfil_Sample_GetTime(); }


#if 0
// The including file must define the type of the hidden "sample". Exmaple types:
// struct  VPLProfil_SampleVector
// struct sample
#endif

/// The hidden, abstract data type, representing a collection of samples.
/// C++ programmers should think of this type as a non-growwable vector.
struct VPLProfil_SampleVector;
typedef struct VPLProfil_SampleVector VPLProfil_SampleVector_t;

/// Create a new #VPLProfile_SampleVector*.
///
/// @param[in] limit Upper bound on number of samples which can be stored in the returned 
///             #VPLProfil_SampleVector*.
/// @param[in] suffix A string suffix to be printed as a "comment", in summary lines
///            printed by #VPLProfil_SampleVector_Print().
///
/// @result  A pointer to an object of type #VPLProfil_SampleVector_t.
///          The caller is responsible for calling #VPLSampleVector_Destroy() on the returned
///          pointer.  Failure to call #VPLSampleVector_Destroy() will leak memory.
VPLProfil_SampleVector_t *VPLProfil_SampleVector_Create(unsigned limit, const char *suffix, int flags);
#define VPLPROFIL_SAVEDSAMPLES_SUMMARY_ONLY 0x0001


/// Destory and deallocate a valid \a VPLProfile_SampleVector* .
void   VPLProfil_SampleVector_Destroy(VPLProfil_SampleVector_t *save);

/// \brief Print all saved samples to <code>FILE *</code> \a f.
///
/// @param[in] f  File on which to print.
/// @param[in] save The #VPLProfil_SampleVector* object to print to file \a f.
/// @note
/// The format of the outpout is idiosyncratic.   The output format is
/// chosen to be easy to manipulate with Unix command-line tools: awk, sort, gnuplot.
/// Lines beginning with '#" should be considered comments.
void   VPLProfil_SampleVector_Print(FILE *f, const VPLProfil_SampleVector_t *save);

/// Returns the number of samples stored in \a save.
/// 
/// @param save  The #VPLSaved_samples_t * object whose sample-count is returned.
int    VPLProfil_SampleVector_Count(const VPLProfil_SampleVector_t *save);

/// Return the saved' sample(?)
void   VPLProfil_SampleVector_Sample(const VPLProfil_SampleVector_t *save);

/// Save a sample.
void   VPLProfil_SampleVector_Save( VPLProfil_SampleVector_t *save, const struct sample *newSample);

#endif // include guard

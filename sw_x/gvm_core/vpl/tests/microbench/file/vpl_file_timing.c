#include <vpl_types.h>
#include <vpl_fs.h>

#include <stdlib.h>
#include <stdio.h>

#include <gvmtest_load.h>

#include "gvmtest_load.h"
#include "microbench_profil.h"

#include "vpl_microbenchmark.h"   // shared parse_args() 

// Trips over Linux kernel bug(?)
//#define NSAMPLES (1024 * 1024)

#define NSAMPLES (10 * 1024)


int
main(int argc, char *argv[])
{
    int samples_flags = 0;

    VPLProfil_SampleVector_t *FileOpen_timings = 0;
    VPLProfil_SampleVector_t *FileClose_timings = 0;
    VPLProfil_SampleVector_t *FileDelete_timings = 0;
    //VPLProfil_SampleVector_t *FileWait_timings  = 0;

    int nSamples = NSAMPLES;
    unsigned i;
    VPLFile_t myFile;
    int status;

    // Parse arguments...
    parse_args(argc, argv, &nSamples, &samples_flags);

    memset(&myFile, 0, sizeof(myFile));

    /// Time file creation...
    FileOpen_timings = VPLProfil_SampleVector_Create(nSamples, "FileOpen.nosync", samples_flags);
    for (i = 0; i < nSamples; i++) {
        char fileNameBuf[1024];
        struct sample theSample;

        snprintf(fileNameBuf, sizeof(fileNameBuf), "/tmp/vplfs.open.%d", i);
        VPLProfil_Sample_Start(&theSample);
        status = VPLFile_Open(&myFile, fileNameBuf,
                            (VPLFILE_OPEN_TRUNC|VPLFILE_OPEN_CREAT|VPLFILE_OPEN_RDWR),
                            VPLFS_MODE_FULL);
        VPLProfil_Sample_End(&theSample);
        
        // save the sample...
        VPLProfil_SampleVector_Save(FileOpen_timings, &theSample);

        // cleanup...
        status = VPLFile_Close(myFile);
        status = VPLFS_Delete(fileNameBuf);
    }
    VPLProfil_SampleVector_Print(stdout, FileOpen_timings);
    VPLProfil_SampleVector_Destroy(FileOpen_timings);

    /// Time file creation, including cost of an fsync after the open ...
    FileOpen_timings = VPLProfil_SampleVector_Create(nSamples, "FileOpen.sync", samples_flags);
    for (i = 0; i < nSamples; i++) {
        char fileNameBuf[1024];
        struct sample theSample;

        snprintf(fileNameBuf, sizeof(fileNameBuf), "/tmp/vplfs.open.%d", i);
        VPLProfil_Sample_Start(&theSample);
        status = VPLFile_Open(&myFile, fileNameBuf,
                            (VPLFILE_OPEN_TRUNC|VPLFILE_OPEN_CREAT|VPLFILE_OPEN_RDWR),
                            VPLFS_MODE_FULL);
        VPLFile_FSync(myFile);
        VPLProfil_Sample_End(&theSample);
        
        // save the sample...
        VPLProfil_SampleVector_Save(FileOpen_timings, &theSample);

        // cleanup...
        status = VPLFile_Close(myFile);
        status = VPLFS_Delete(fileNameBuf);
    }
    VPLProfil_SampleVector_Print(stdout, FileOpen_timings);
    VPLProfil_SampleVector_Destroy(FileOpen_timings);


    /// Time file close...
    FileClose_timings = VPLProfil_SampleVector_Create(nSamples, "FileClose", samples_flags);
    for (i = 0; i < nSamples; i++) {
        char fileNameBuf[1024];
        struct sample theSample;

        snprintf(fileNameBuf, sizeof(fileNameBuf), "/tmp/vplfs.open.%d", i);
        VPLProfil_Sample_Start(&theSample);
        status = VPLFile_Open(&myFile, fileNameBuf,
                            (VPLFILE_OPEN_TRUNC|VPLFILE_OPEN_CREAT|VPLFILE_OPEN_RDWR),
                            VPLFS_MODE_FULL);
        VPLProfil_Sample_End(&theSample);
        
        // save the sample...
        VPLProfil_SampleVector_Save(FileOpen_timings, &theSample);

        // cleanup...
        status = VPLFile_Close(myFile);
        status = VPLFS_Delete(fileNameBuf);
    }
    VPLProfil_SampleVector_Print(stdout, FileOpen_timings);
    VPLProfil_SampleVector_Destroy(FileOpen_timings);


    
    /// Time file delete...
    FileDelete_timings = VPLProfil_SampleVector_Create(nSamples, "FileDelete", samples_flags);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;
        char fileNameBuf[1024];

        snprintf(fileNameBuf, sizeof(fileNameBuf), "/tmp/vplfs.remfile.%d", i);
        status = VPLFile_Open(&myFile, fileNameBuf,
                            (VPLFILE_OPEN_TRUNC|VPLFILE_OPEN_CREAT|VPLFILE_OPEN_RDWR),
                              VPLFS_MODE_FULL);
                 
        VPLFile_Close(myFile);

        VPLProfil_Sample_Start(&theSample);
        status = VPLFS_Delete(fileNameBuf);
        VPLProfil_Sample_End(&theSample);

        // save the sample...
        VPLProfil_SampleVector_Save(FileClose_timings, &theSample);

    }
    VPLProfil_SampleVector_Print(stdout, FileClose_timings);
    VPLProfil_SampleVector_Destroy(FileClose_timings);


    
#if 0 // No VPL_FS Rename??
    /// Time file rename...
    FileClose_timings = VPLProfil_SampleVector_Create(nSamples, "FileRename");
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;
        char fileNameFrom[1024];
        char fileNameTo[1024];

        snprintf(fileNameFrom, sizeof(fileNameFrom), "/tmp/vplfs.srcfile.%d", i);
        snprintf(fileNameTo, sizeof(fileNameTo), "/tmp/vplfs.dstfile.%d", i);
        // create an empty file.
        status = VPLFile_Open(&myFile, fileNameFrom,
                            (VPLFILE_OPEN_TRUNC|VPLFILE_OPEN_CREAT|VPLFILE_OPEN_RDWR),
                            VPLFS_MODE_FULL);
                 
        VPLFile_Close(myFile);

        VPLProfil_Sample_Start(&theSample);
        status = VPLFS_Rename(fileNameFrom, FilenameTo);
        VPLProfil_Sample_End(&theSample);

        // save the sample...
        VPLProfil_SampleVector_Save(FileClose_timings, &theSample);

        status = VPLFS_Delete(fileNameFrom);
        status = VPLFS_Delete(fileNameTo);
    }
    VPLProfil_SampleVector_Print(stdout, FileClose_timings);
    VPLProfil_SampleVector_Destroy(FileClose_timings);
#endif

    return (0);
}

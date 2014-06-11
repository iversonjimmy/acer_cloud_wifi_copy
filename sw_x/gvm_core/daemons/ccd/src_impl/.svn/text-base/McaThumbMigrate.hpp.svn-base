//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef MCA_THUMB_MIGRATE_HPP_5_2_2013
#define MCA_THUMB_MIGRATE_HPP_5_2_2013

//int GetMcaThumbDir(catagory, std::string dir_out);
#include "vpl_plat.h"

#include <ccd_types.pb.h>
#include "vpl_thread.h"
#include "vpl_th.h"
#include "vpl_user.h"
#include "vplu_types.h"

class McaMigrateThumb {
public:
    McaMigrateThumb();
    ~McaMigrateThumb();

    static int McaThumbInitMigrate();
    // Abort of migrate does not exist
    //static int McaThumbAbortMigrate();

    /// This will perform a migrate if one is currently ongoing or set.
    /// If there is no migration initialized, this call will be a no-op.
    /// If successful, you must eventually call McaThumbStopMigrate() to release
    /// resources.
    int McaThumbResumeMigrate(u32 activationId);
    int McaThumbStopMigrate();
private:
    static VPLTHREAD_FN_DECL runMcaThumbMigrate(void* arg);
    void copyFilesPhase();
    void deleteFilesPhase();

    /// Protects the following member variables:
    /// - m_isMigrateThreadInit
    /// - m_migrateThread
    /// .
    /// @note Rule to avoid deadlock: you must *not* acquire the CCD Cache lock after acquiring this
    ///   mutex. If you are going to access the CCD Cache, make sure to acquire the Cache lock first.
    VPLMutex_t m_apiMutex;

    bool m_isMigrateThreadInit;
    VPLDetachableThreadHandle_t m_migrateThread;
    VPLSem_t m_threadFinishedSem;

    volatile bool m_run;

    bool m_migrationExists;

    // The following member variables are only intended for use by (1) the worker thread and (2) the
    // thread that spawns the worker thread (and only before spawning the worker thread).
    // As such, no mutex is needed to access them.
    //@{
    ccd::MediaMetadataThumbMigrateInternal m_migrateState;
    VPLUser_Id_t m_user_id;
    u64 m_dataset_id;
    u32 m_activation_id;
    //@}
};

McaMigrateThumb& getMcaMigrateThumbInstance();

#endif // MCA_THUMB_MIGRATE_HPP_5_2_2013

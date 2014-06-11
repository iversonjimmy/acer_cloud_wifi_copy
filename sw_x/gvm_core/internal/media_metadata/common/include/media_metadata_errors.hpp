//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __MEDIA_METADATA_ERRORS_HPP__
#define __MEDIA_METADATA_ERRORS_HPP__

//============================================================================
/// @file
/// Error codes for the Media Metadata Cloud components.
//============================================================================

#include <vpl_types.h>

// -14400 to -14499
typedef int32_t MMError;

/// Invalid input parameter(s).
#define MM_ERR_INVALID           -14400

/// Must call the appropriate init function first.
///  #MSAInitForDiscovery(), etc.
#define MM_ERR_NOT_INIT          -14401

/// Already initialized or already within a transaction.
#define MM_ERR_ALREADY           -14402

/// Collection ID is invalid.
#define MM_ERR_BAD_COLLECTION_ID -14403

/// Thumbnail has no extension.
#define MM_ERR_NO_THUMBNAIL_EXT  -14404

/// Access/Permission error (is file open?)
#define MM_ERR_NO_ACCESS         -14405

/// The local device is not enabled as a media server.  This is a prerequisite for this operation.
#define MM_ERR_NOT_MEDIA_SERVER  -14406

/// The request object was not found.
#define MM_ERR_NOT_FOUND         -14409

/// Cannot proceed without a signed-in user (you may need to invoke the Sync Agent UI).
#define MM_ERR_NOT_SIGNED_IN     -14411

/// Must call #MSABeginMetadataTransaction() first.
#define MM_ERR_NO_TRANSACTION    -14413

/// Invalid UUID format; must be 32 hexadecimal characters, dashes are optional.
#define MM_ERR_BAD_UUID          -14414

/// I/O error when writing to a file.
#define MM_ERR_WRITE             -14415

/// I/O error when opening directory for reading.
#define MM_ERR_OPENDIR           -14416

/// The user does not appear to have a suitable dataset.
/// Most likely, the cached list of datasets needs to be updated from VSDS.
/// If the cache is up-to-date, then this may indicate a problem with
/// the user's account.
#define MM_ERR_NO_DATASET        -14417

/// The user does not appear to be subscribed to a suitable dataset.
/// User should be directed to the control panel for making subscription.
#define MM_ERR_NOT_SUBSCRIBED    -14418

/// Must call #MSABeginCatalog() first.
#define MM_ERR_NO_BEG_CATALOG    -14419

/// In catalog transaction, must end transaction first.
#define MM_ERR_IN_CATALOG        -14420

/// Bad parameters
#define MM_ERR_BAD_PARAMS        -14421

/// Could not parse the URL in the expected media proxy format.
#define MM_INVALID_URL           -14441

#define MCA_CONTROLDB_OK                           0
#define MCA_CONTROLDB_DB_ALREADY_OPEN         -14460
#define MCA_CONTROLDB_DB_OPEN_FAIL            -14461
#define MCA_CONTROLDB_DB_NOT_OPEN             -14462
#define MCA_CONTROLDB_DB_CREATE_TABLE_FAIL    -14463
#define MCA_CONTROLDB_DB_NO_VERSION           -14464
#define MCA_CONTROLDB_DB_INTERNAL_ERR         -14465
#define MCA_CONTROLDB_DB_BAD_VALUE            -14466
#define MCA_CONTROLDB_DB_NOT_PRESENT          -14467


/// Invalid filter type of the sql query
#define MCA_ERR_INVALID_FILTER_TYPE           -14468

#define MSA_CONTROLDB_OK                           0
#define MSA_CONTROLDB_DB_ALREADY_OPEN         -14470
#define MSA_CONTROLDB_DB_OPEN_FAIL            -14471
#define MSA_CONTROLDB_DB_NOT_OPEN             -14472
#define MSA_CONTROLDB_DB_CREATE_TABLE_FAIL    -14473
#define MSA_CONTROLDB_DB_NO_VERSION           -14474
#define MSA_CONTROLDB_DB_INTERNAL_ERR         -14475
#define MSA_CONTROLDB_DB_BAD_VALUE            -14476
#define MSA_CONTROLDB_DB_NOT_PRESENT          -14477
#define MSA_CONTROLDB_DB_BUILD_CANCELED       -14478

/// Unspecified failure within a Media Metadata Cloud component.
#define MM_ERR_FAIL              -14499

#endif // include guard

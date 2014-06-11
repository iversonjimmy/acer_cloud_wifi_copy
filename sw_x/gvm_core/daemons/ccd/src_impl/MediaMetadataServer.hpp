//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __MEDIAMETADATA_SERVER_HPP_
#define __MEDIAMETADATA_SERVER_HPP_

//============================================================================
/// @file
/// Media Server Agent APIs for use by the Media Discovery Agent.
//============================================================================

#include "media_metadata_errors.hpp"
#include "vplu_types.h"
#include "ccdi_rpc.pb.h"

/// Please refer to MSA section in ccdi_rpc.proto and
/// http://www.ctbg.acer.com/wiki/index.php/MSA_API for documentation

MMError MSAInitForDiscovery(const u64 user_id, const u64 device_id);

MMError MSABeginCatalog(const ccd::BeginCatalogInput& input);

MMError MSACommitCatalog(const ccd::CommitCatalogInput& input);

// Read-only version of MSACommitCatalog
MMError MSAEndCatalog(const ccd::EndCatalogInput& input);

MMError MSADestroy(void);

MMError MSABeginMetadataTransaction(const ccd::BeginMetadataTransactionInput& input);

MMError MSAUpdateMetadata(const ccd::UpdateMetadataInput& input);

MMError MSADeleteMetadata(const ccd::DeleteMetadataInput& input);

MMError MSACommitMetadataTransaction(void);

MMError MSAGetMetadataSyncState(media_metadata::GetMetadataSyncStateOutput& output);

MMError MSAGetContentObjectMetadata(const std::string& objectId,
                                    const std::string& collectionId,
                                    const media_metadata::CatalogType_t& catalogType,
                                    media_metadata::GetObjectMetadataOutput& output);

MMError MSADeleteCollection(const ccd::DeleteCollectionInput& input);

MMError MSADeleteCatalog(const ccd::DeleteCatalogInput& input);

MMError MSAListCollections(media_metadata::ListCollectionsOutput& output);

MMError MSAGetCollectionDetails(const ccd::GetCollectionDetailsInput& input,
                                ccd::GetCollectionDetailsOutput& output);

#endif // include guard

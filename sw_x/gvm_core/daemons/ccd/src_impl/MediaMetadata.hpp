//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef MEDIAMETADATA_HPP_
#define MEDIAMETADATA_HPP_

#include "media_metadata_errors.hpp"
#include "media_metadata_types.pb.h"
#include "ccd_features.h"
#include <vplu_types.h>
#include <vpl_user.h>

/// Path within the "Media MetaData VCS" dataset for app-managed playlists.
#define MEDIA_METADATA_SUBDIR_PLAYLISTS  "/playlists"

MMError MCAInit(VPLUser_Id_t userId);
void MCALogout();

MMError MCAUpdateMetadataDB(media_metadata::CatalogType_t catalogType);

// This function efficiently updates the DB data when the DB schema is upgraded
// from version 10 to 11.  In this schema upgrade, the new columns comp_id and
// special_format_flag are added.  This function scans all the photos' metadata, but only
// in cases where these new fields are present will the database be updated.
// The assumption is that because this is a new feature, there will be relatively
// few metadata/collections that fit this profile.
//
// From version 11 to 12, photo_albums was added and updated.
// album_id_ref is added to photos table and also updated.
//
MMError MCAUpgradePhotoDBforVersion10to12(VPLUser_Id_t userId, const bool &v10to11, const bool& v11to12);

MMError MCAQueryMetadataObjects(u64 deviceId,
                                media_metadata::DBFilterType_t filter_type,
                                const std::string& selection,
                                const std::string& sort_order,
                                google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject > &output);

#endif /* MEDIAMETADATA_HPP_ */

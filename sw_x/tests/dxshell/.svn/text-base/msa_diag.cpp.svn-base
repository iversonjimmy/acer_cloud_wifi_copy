//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include <ccdi.hpp>
#include <vplex_file.h>
#include <vpl_fs.h>
#include <vplu_types.h>
#include "vpl_th.h"

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <deque>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

#include <log.h>
#include <vpl_plat.h>

#include "cslsha.h"
#include "dx_common.h"
#include "gvm_file_utils.hpp"
#include "util_mime.hpp"
#include "ccd_utils.hpp"
#include "TargetDevice.hpp"

#include "common_utils.hpp"

#include "mca_diag.hpp"

static u64 s_timestamp = 0;

class AutoTargetDevice {
    // Note that unlike other Auto* classes,
    // AutoTargetDevice does not automatically create an object in the constructor.
    // To create an object, the client must explicitly call create().
    // This is done so that a TargetDevice object can be shared across multiple functions.
public:
    AutoTargetDevice() : target(NULL) {}
    ~AutoTargetDevice() {
        if (target) {
            delete target;
        }
    }
    TargetDevice *create() { return target = getTargetDevice(); }
    TargetDevice *get() { return target; }
private:
    TargetDevice *target;
};

static int msaInit(TargetDevice *target = NULL)
{
    LOG_INFO("start");
    int rv;

    ccd::UpdateAppStateInput uasInput;
    ccd::UpdateAppStateOutput uasOutput;
    uasInput.set_app_id("MSA_DIAG_TEST");
    uasInput.set_app_type(ccd::CCD_APP_ALL_MEDIA);
    uasInput.set_foreground_mode(true);
    rv = CCDIUpdateAppState(uasInput, uasOutput);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateAppState:%d, err:%d",
                  rv, uasOutput.foreground_mode_err());
        return rv;
    }else{
        LOG_INFO("CCDIUpdateAppState ok");
    }

    AutoTargetDevice atd;
    if (target == NULL) {
        target = atd.create();
    }

    return rv;
}

static int msaBeginCatalog(media_metadata::CatalogType_t catType,
                           TargetDevice *target = NULL)
{
    LOG_INFO(">>>> MSABeginCatalog, catType:%d", (int)catType);
    int rv = 0;
    ccd::BeginCatalogInput input;
    input.set_catalog_type(catType);
    AutoTargetDevice atd;
    if (target == NULL) {
        target = atd.create();
    }
    rv = target->MSABeginCatalog(input);
    if(rv != 0) {
        LOG_ERROR("MSABeginCatalog:%d", rv);
    }
    return rv;
}

static int msaCommitCatalog(media_metadata::CatalogType_t catType,
                            TargetDevice *target = NULL)
{
    LOG_INFO(">>>> MSACommitCatalog, catType:%d", (int)catType);
    ccd::CommitCatalogInput ccInput;
    ccInput.set_catalog_type(catType);
    AutoTargetDevice atd;
    if (target == NULL) {
        target = atd.create();
    }
    int rv = target->MSACommitCatalog(ccInput);
    if(rv != 0) {
        LOG_ERROR("MSACommitCatalog failed:%d", rv);
    }
    return rv;
}

static int msaEndCatalog(media_metadata::CatalogType_t catType,
                         TargetDevice *target = NULL)
{
    LOG_INFO(">>>> MSAEndCatalog, catType:%d", (int)catType);
    ccd::EndCatalogInput ecInput;
    ecInput.set_catalog_type(catType);
    AutoTargetDevice atd;
    if (target == NULL) {
        target = atd.create();
    }
    int rv = target->MSAEndCatalog(ecInput);
    if(rv != 0) {
        LOG_ERROR("MSAEndCatalog failed:%d", rv);
    }
    return rv;
}

int msaGetMetadataUploadPath(std::string& uploadPath_out)
{
    int rv;
    ccd::GetSyncStateInput in;
    ccd::GetSyncStateOutput out;

    in.set_get_media_metadata_upload_path(true);
    rv = CCDIGetSyncState(in, out);
    if(rv != 0) {
        LOG_ERROR("CCDIGetSyncState:%d", rv);
        return rv;
    }
    uploadPath_out = out.media_metadata_upload_path();
    return rv;
}

int msaDiagDeleteCatalog(TargetDevice *target)  // target is optional arg
{
    int rv = 0;
    int rc;
    VPL_Init();

    AutoTargetDevice atd;
    if (target == NULL) {
        target = atd.create();
    }

    rv = msaInit(target);
    if (rv != 0) {
        LOG_ERROR("msaInit failed: %d", rv);
        return rv;
    }

    ccd::DeleteCatalogInput deleteCatalogInput;

    LOG_ALWAYS(">>>> MSADeleteCatalog Music");
    deleteCatalogInput.set_catalog_type(media_metadata::MM_CATALOG_MUSIC);
    rc = target->MSADeleteCatalog(deleteCatalogInput);
    if (rc != 0) {
        LOG_ERROR("MSADeleteCatalog Music failed: %d", rc);
        rv = rc;
        goto error;
    }

    LOG_ALWAYS(">>>> MSADeleteCatalog Photo");
    deleteCatalogInput.set_catalog_type(media_metadata::MM_CATALOG_PHOTO);
    rc = target->MSADeleteCatalog(deleteCatalogInput);
    if (rc != 0) {
        LOG_ERROR("MSADeleteCatalog Photo failed: %d", rc);
        rv = rc;
        goto error;
    }

    LOG_INFO("MSADeleteCatalog OK");
 error:
    return rv;
}

#define MUSIC_ALBUM "MusicAlbum"
#define PHOTO_ALBUM "PhotoAlbum"


// Long paths
#define MUSIC_COLLECTION_LARGE_NAME_SUFFIX "-12345678901234567890123456789012345678901234567"
// 181 chars.
#define MUSIC_ALBUM_LARGE_NAME_SUFFIX "-123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"

// Shorter path versions to ensure that the test works.
//#define MUSIC_COLLECTION_LARGE_NAME_SUFFIX "-123456"
//#define MUSIC_ALBUM_LARGE_NAME_SUFFIX "-12345678"

// Test client:  1) Creates NUM_MUSIC_ALBUMS+1 collections.
//               2) First music album collection (index 0) has NUM_MUSIC_TRACKS_PER_ALBUM+2
//                  media items (including album object), the rest have
//                  NUM_MUSIC_TRACKS_PER_ALBUM+1 media items (including album object)
//               3) Commit each music album
//               3.1) Update album0 track0 title
//               4) Create photo album(s)
//                  First photo album collection (index 0) has NUM_PHOTOS_PER_ALBUM+1
//                  media items, but rest have NUM_PHOTOS_PER_ALBUM media items
//               5) Place num_photos_per_album photos in each album
//               6) Commit each photo album
//               6.1) Update album0 photo0 title
//               7) Delete the last music album so there are NUM_MUSIC_ALBUMS total
//               7.1) Delete the last photo album so there are NUM_PHOTO_ALBUMS total
//               8) Remove the last music track in album 0, so that there are NUM_MUSIC_TRACKS_PER_ALBUM music tracks
//                  (NUM_MUSIC_TRACK_PER_ALBUM+1 including album object) in every album.
//              8.1) Remove the first photo item in album 0, so that there are NUM_PHOTOS_PER_ALBUM photo items
//                   in every album
//              9) Test ListCollections and confirm there are NUM_MUSIC_ALBUMS music collections
//                  and num_photo_albums photo albums
//              10) Test MSAGetCollectionDetails and check there are the appropriate number of media items, NUM_MUSIC_TRACKS_PER_ALBUM+1 items.
int msa_diag(const std::string &test_clip_path,
             const std::string &test_clip_name,
             const std::string &test_thumb_name_path,
             bool useLargeNames,
             bool incrementTimestamp,
             bool addOnly,
             const u32 num_music_albums,
             const u32 num_music_tracks_per_album,
             const u32 num_photo_albums,
             const u32 num_photos_per_album,
             bool using_remote_agent)
{
    LOG_INFO("start");
    int rv = 0;
    int rc;
    int object_id = 0;
    ccd::UpdateMetadataInput updateMetaReq;
    std::string object_id_to_remove;
    std::string photo_object_id_to_remove;
    std::string photo_object_id_to_edit;
#if !defined(CLOUDNODE)
    VPLFS_stat_t stat;
#endif
    std::string test_clip_path_clean, test_thumb_name_path_clean;
    u32 actual_num_music_albums = num_music_albums;
    u32 actual_num_photo_albums = num_photo_albums;

    VPL_Init();

    test_clip_path_clean = Util_CleanupPath(test_clip_path);
    test_thumb_name_path_clean = Util_CleanupPath(test_thumb_name_path);

    LOG_INFO("test_clip_path:%s test_clip_name:%s test_thumb_name_path:%s",
             test_clip_path_clean.c_str(), test_clip_name.c_str(), test_thumb_name_path_clean.c_str());

#if !defined(CLOUDNODE)
    if (VPLFS_Stat(test_clip_path_clean.c_str(), &stat) != 0) {
        LOG_ERROR("Test clip %s not found, please copy the test clip to the expected path", test_clip_path_clean.c_str());
        return -1;
    }

    if (VPLFS_Stat(test_thumb_name_path_clean.c_str(), &stat) != 0) {
        LOG_ERROR("Test thumbnail %s not found, please copy the test thumbnail to the expected path", test_thumb_name_path_clean.c_str());
        return -1;
    }
#endif

    AutoTargetDevice atd;
    TargetDevice *target = atd.create();

    std::string workdir;
    rv = target->getWorkDir(workdir);
    if (rv != 0) {
        LOG_ERROR("Failed to get workdir on target device: %d", rv);
        return -1;
    }
    LOG_INFO("workdir = %s", workdir.c_str());
    rv = target->createDir(workdir, 0777);
    if (rv != 0) {
        LOG_ERROR("Failed to createDir at %s: %d", workdir.c_str(), rv);
        return -1;
    }
    std::string separator;
    rv = target->getDirectorySeparator(separator);
    if (rv != 0) {
        LOG_ERROR("Failed to get directory separator on target device: %d", rv);
        return -1;
    }
    LOG_INFO("separator = %s", separator.c_str());

    std::string target_test_clip_path, target_test_thumb_path;
#ifdef CLOUDNODE
    target_test_clip_path.assign(test_clip_path_clean);
    target_test_thumb_path.assign(test_thumb_name_path_clean);
#else
    target_test_clip_path.assign(workdir);
    target_test_clip_path.append(separator);
    size_t pos = test_clip_path_clean.find_last_of("/\\");
    if (pos == test_clip_path_clean.npos) {
        target_test_clip_path.append(test_clip_path_clean);
    }
    else {
        target_test_clip_path.append(test_clip_path_clean, pos + 1, test_clip_path_clean.npos);
    }
    target_test_thumb_path.assign(workdir);
    target_test_thumb_path.append(separator);
    pos = test_thumb_name_path_clean.find_last_of("/\\");
    if (pos == test_thumb_name_path_clean.npos) {
        target_test_thumb_path.append(test_thumb_name_path_clean);
    }
    else {
        target_test_thumb_path.append(test_thumb_name_path_clean, pos + 1, test_thumb_name_path_clean.npos);
    }
    LOG_INFO("target_test_clip_path = %s", target_test_clip_path.c_str());
    LOG_INFO("target_test_thumb_path = %s", target_test_thumb_path.c_str());

    rv = target->pushFile(test_clip_path_clean, target_test_clip_path);
    if (rv != 0) {
        LOG_ERROR("Failed to push %s to %s: %d", test_clip_path_clean.c_str(), target_test_clip_path.c_str(), rv);
        return -1;
    }
    rv = target->pushFile(test_thumb_name_path_clean, target_test_thumb_path);
    if (rv != 0) {
        LOG_ERROR("Failed to push %s to %s: %d", test_thumb_name_path_clean.c_str(), target_test_thumb_path.c_str(), rv);
        return -1;
    }
#endif // CLOUDNODE

    rv = msaInit(target);
    if (rv != 0) {
        LOG_ERROR("msaInit failed: %d", rv);
        return -1;
    }

    {
        ccd::DeleteCatalogInput deleteCatalogInput;

        LOG_ALWAYS(">>>> MSADeleteCatalog Photo");
        deleteCatalogInput.set_catalog_type(media_metadata::MM_CATALOG_PHOTO);
        rv = target->MSADeleteCatalog(deleteCatalogInput);
        if (rv != 0) {
            LOG_ERROR("MSADeleteCatalog Photo failed: %d", rv);
            goto exit;
        } else {
            LOG_INFO("MSADeleteCatalog Photo OK");
        }

        LOG_ALWAYS(">>>> MSADeleteCatalog Music");
        deleteCatalogInput.set_catalog_type(media_metadata::MM_CATALOG_MUSIC);
        rv = target->MSADeleteCatalog(deleteCatalogInput);
        if (rv != 0) {
            LOG_ERROR("MSADeleteCatalog Music failed: %d", rv);
            goto exit;
        } else {
            LOG_INFO("MSADeleteCatalog Music OK");
        }
    }

    rv = msaBeginCatalog(media_metadata::MM_CATALOG_MUSIC, target);
    if (rv != 0) {
        LOG_ERROR("msaBeginCatalog failed: %d", rv);
        goto exit;
    }

    if(addOnly) {
        LOG_INFO("AddOnly test");
    }else{
        actual_num_music_albums += 1;
        actual_num_photo_albums += 1;
        LOG_INFO("Not addOnly.  Will add extra items to test delete functionality.");
    }

    LOG_INFO("Updating %d music items, %d albums, %d tracks per album...",
             actual_num_music_albums * num_music_tracks_per_album,
             actual_num_music_albums,
             num_music_tracks_per_album);

    // 1) See function comment above
    for (u32 collectionIndex = 0; collectionIndex < actual_num_music_albums; collectionIndex++) {
        std::string album_name = MUSIC_ALBUM;
        std::ostringstream album_number;
        std::ostringstream album_oid;

        album_number << collectionIndex;
        if(useLargeNames) {
            album_number << MUSIC_COLLECTION_LARGE_NAME_SUFFIX;
        }
        album_name += album_number.str();

        LOG_INFO(">>>> MSABeginMetadataTransaction %s", album_name.c_str());
        ccd::BeginMetadataTransactionInput beginReq1;
        beginReq1.set_collection_id(album_name);
        beginReq1.set_reset_collection(false);
        beginReq1.set_collection_timestamp(s_timestamp);
        if(incrementTimestamp) {
            s_timestamp++;
        }
        rv = target->MSABeginMetadataTransaction(beginReq1);
        if (rv != 0) {
            LOG_ERROR("MSABeginMetadataTransaction failed: %d", rv);
            goto exit;
        } else {
            LOG_INFO("MSABeginMetadataTransaction OK");
        }

        album_oid << object_id;
        if(useLargeNames) {
            album_oid << MUSIC_ALBUM_LARGE_NAME_SUFFIX;
        }
        object_id++;

        // 2) See function comment above
        int numMusicTracksPerAlbum = num_music_tracks_per_album;
        if(!addOnly && collectionIndex == 0) {
            numMusicTracksPerAlbum++;
        }
        for (int mediaItemIndex = 0; mediaItemIndex < numMusicTracksPerAlbum; mediaItemIndex++) {
            std::string track_name = "MusicTrack";
            std::ostringstream track_number;

            track_number << mediaItemIndex;
            track_name += track_number.str();
            track_name += "In";
            track_name += album_name;

            updateMetaReq.Clear();

            {
                std::ostringstream oid;
                oid << object_id;
                object_id++;
                updateMetaReq.mutable_metadata()->set_object_id(oid.str());
                if(mediaItemIndex == num_music_tracks_per_album) {
                    object_id_to_remove = oid.str();
                }
            }
            updateMetaReq.mutable_metadata()->set_source(media_metadata::MEDIA_SOURCE_ITUNES);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_absolute_path(target_test_clip_path);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_artist("iGware");
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_title(track_name);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_album_ref(album_oid.str());
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_track_number(mediaItemIndex);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_file_format(get_extension(target_test_clip_path));

            rv = target->MSAUpdateMetadata(updateMetaReq);  // music track
            if (rv != 0) {
                LOG_ERROR("MSAUpdateMetadata failed: %d", rv);
                goto exit;
            }
        }

        // Testing that adding album after music track does not cause constraint failure.
        updateMetaReq.Clear();
        updateMetaReq.mutable_metadata()->set_object_id(album_oid.str());
        updateMetaReq.mutable_metadata()->set_source(media_metadata::MEDIA_SOURCE_ITUNES);
        updateMetaReq.mutable_metadata()->mutable_music_album()->set_album_artist("iGware");
        updateMetaReq.mutable_metadata()->mutable_music_album()->set_album_name(album_name);
        updateMetaReq.mutable_metadata()->mutable_music_album()->set_album_thumbnail(target_test_thumb_path);
        rv = target->MSAUpdateMetadata(updateMetaReq);  // album object
        if (rv != 0) {
            LOG_ERROR("MSAUpdateMetadata failed: %d", rv);
            goto exit;
        }

        // 3) See function comment above
        LOG_INFO(">>>> Incremental MSACommitMetadataTransaction");
        rv = target->MSACommitMetadataTransaction();
        if (rv != 0) {
            LOG_ERROR("MSACommitMetadataTransaction failed: %d", rv);
            goto exit;
        }
    }

    rv = msaCommitCatalog(media_metadata::MM_CATALOG_MUSIC, target);
    if (rv != 0) {
        LOG_ERROR("msaCommitCatalog failed: %d", rv);
        goto exit;
    }

    
    // 3.1) Update album0 track0 title
    if(!addOnly){
        if(num_music_albums && num_music_tracks_per_album){
            rv = msaBeginCatalog(media_metadata::MM_CATALOG_MUSIC, target);
            if (rv != 0) {
                LOG_ERROR("msaBeginCatalog failed: %d", rv);
                goto exit;
            }

            ccd::BeginMetadataTransactionInput beginReq2;
            beginReq2.set_collection_id(std::string(MUSIC_ALBUM)+"0");
            beginReq2.set_reset_collection(false);
            beginReq2.set_collection_timestamp(s_timestamp);
            if(incrementTimestamp) {
                s_timestamp++;
            }
            int rc = target->MSABeginMetadataTransaction(beginReq2);
            if (rc != 0) {
                LOG_ERROR("MSABeginMetadataTransaction failed: %d", rc);
                rv = rc;
                goto exit;
            } else {
                LOG_INFO("MSABeginMetadataTransaction OK");
            }

            updateMetaReq.Clear();
            updateMetaReq.mutable_metadata()->set_object_id("1");
            updateMetaReq.mutable_metadata()->set_source(media_metadata::MEDIA_SOURCE_ITUNES);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_absolute_path(target_test_clip_path);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_artist("iGware");
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_title(MUSIC_ALBUM0_TRACK0_NEW_TITLE);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_album_ref("0");
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_track_number(0);

            rv = target->MSAUpdateMetadata(updateMetaReq);  // music track
            if (rv != 0) {
                LOG_ERROR("MSAUpdateMetadata failed: %d", rv);
                goto exit;
            }

            rv = target->MSACommitMetadataTransaction();
            if (rv != 0) {
                LOG_ERROR("MSACommitMetadataTransaction failed: %d", rv);
                goto exit;
            }

            rv = msaCommitCatalog(media_metadata::MM_CATALOG_MUSIC, target);
            if (rv != 0) {
                LOG_ERROR("msaCommitCatalog failed: %d", rv);
                goto exit;
            }
        }
    }

    LOG_INFO("Updating %d photo items, %d albums, %d photos per album...",
             actual_num_photo_albums * num_photos_per_album,
             actual_num_photo_albums,
             num_photos_per_album);

    rv = msaBeginCatalog(media_metadata::MM_CATALOG_PHOTO, target);
    if (rv != 0) {
        LOG_ERROR("msaBeginCatalog failed: %d", rv);
        goto exit;
    }

    // 4) See function comment above
    for (u32 collectionIndex = 0; collectionIndex < actual_num_photo_albums; collectionIndex++) {
        std::string album_name = PHOTO_ALBUM;
        std::ostringstream album_number;
        std::ostringstream album_oid;

        album_number << collectionIndex;
        album_name += album_number.str();

        LOG_INFO(">>>> MSABeginMetadataTransaction %d photos, %s", num_photos_per_album, album_name.c_str());
        ccd::BeginMetadataTransactionInput beginReq1;
        beginReq1.set_collection_id(album_name);
        beginReq1.set_reset_collection(false);
        beginReq1.set_collection_timestamp(s_timestamp);
        if(incrementTimestamp) {
            s_timestamp++;
        }
        rv = target->MSABeginMetadataTransaction(beginReq1);
        if (rv != 0) {
            LOG_ERROR("MSABeginMetadataTransaction failed: %d", rv);
            goto exit;
        } else {
            LOG_INFO("MSABeginMetadataTransaction OK");
        }

        // 5) See function comment above
        int numPhotosPerAlbum = num_photos_per_album;
        if(!addOnly && collectionIndex == 0) {
            numPhotosPerAlbum++;
        }

        // 5) See function comment above
        for (int mediaItemIndex = 0; mediaItemIndex < numPhotosPerAlbum; mediaItemIndex++) {
            std::string photo_name = "privPhoto";
            std::ostringstream track_number;

            track_number << mediaItemIndex;
            photo_name += track_number.str();
            photo_name += "In";
            photo_name += album_name;

            updateMetaReq.Clear();

            {
                std::ostringstream oid;
                oid << object_id;
                object_id++;
                updateMetaReq.mutable_metadata()->set_object_id(oid.str());
                //Remove the first photo in album 0, Edit the second photo in album0
                if(photo_object_id_to_remove.empty()) {
                    photo_object_id_to_remove = oid.str();
                }else if(photo_object_id_to_edit.empty()){
                    photo_object_id_to_edit = oid.str();
                }
            }
            updateMetaReq.mutable_metadata()->set_source(media_metadata::MEDIA_SOURCE_ITUNES);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_absolute_path(target_test_clip_path);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_title(photo_name);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_thumbnail(target_test_thumb_path);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_album_name(album_name);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_date_time(777);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_file_format(get_extension(target_test_clip_path));
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_orientation(200);

            rv = target->MSAUpdateMetadata(updateMetaReq);  // Photo
            if (rv != 0) {
                LOG_ERROR("MSAUpdateMetadata failed: %d", rv);
                goto exit;
            }
        }

        // 6) See function comment above
        LOG_INFO(">>>> Incremental MSACommitMetadataTransaction");
        rv = target->MSACommitMetadataTransaction();
        if (rv != 0) {
            LOG_ERROR("MSACommitMetadataTransaction failed: %d", rv);
            goto exit;
        }
    }

    LOG_INFO("msaCommitCatalog for %d photos and %d albums.  May take a while for "
             "large numbers of thumbnails. See replay directory for progress.",
             actual_num_photo_albums * num_photos_per_album, actual_num_photo_albums);
    rv = msaCommitCatalog(media_metadata::MM_CATALOG_PHOTO, target);
    if (rv != 0) {
        LOG_ERROR("msaCommitCatalog failed: %d", rv);
        goto exit;
    }
    LOG_INFO("msaCommitCatalog OK");

    if(!addOnly) {
        // 6.1) Update album0 photo0 title
        if(!photo_object_id_to_edit.empty()){
            rv = msaBeginCatalog(media_metadata::MM_CATALOG_PHOTO, target);
            if (rv != 0) {
                LOG_ERROR("msaBeginCatalog failed: %d", rv);
                goto exit;
            }

            ccd::BeginMetadataTransactionInput beginReq2;
            beginReq2.set_collection_id(std::string(PHOTO_ALBUM)+"0");
            beginReq2.set_reset_collection(false);
            beginReq2.set_collection_timestamp(s_timestamp);
            if(incrementTimestamp) {
                s_timestamp++;
            }
            int rc = target->MSABeginMetadataTransaction(beginReq2);
            if (rc != 0) {
                LOG_ERROR("MSABeginMetadataTransaction failed: %d", rc);
                rv = rc;
                goto exit;
            } else {
                LOG_INFO("MSABeginMetadataTransaction OK");
            }

            updateMetaReq.Clear();
            updateMetaReq.mutable_metadata()->set_object_id(photo_object_id_to_edit);
            updateMetaReq.mutable_metadata()->set_source(media_metadata::MEDIA_SOURCE_ITUNES);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_absolute_path(target_test_clip_path);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_title(PHOTO_ALBUM0_PHOTO0_NEW_TITLE);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_thumbnail(target_test_thumb_path);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_album_name(std::string(PHOTO_ALBUM)+"0");
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_date_time(888);

            rv = target->MSAUpdateMetadata(updateMetaReq);  // photo item
            if (rv != 0) {
                LOG_ERROR("MSAUpdateMetadata failed: %d", rv);
                goto exit;
            }

            rv = target->MSACommitMetadataTransaction();
            if (rv != 0) {
                LOG_ERROR("MSACommitMetadataTransaction failed: %d", rv);
                goto exit;
            }

            rv = msaCommitCatalog(media_metadata::MM_CATALOG_PHOTO, target);
            if (rv != 0) {
                LOG_ERROR("msaCommitCatalog failed: %d", rv);
                goto exit;
            }
        }

        // Sleep 1 sec. The reason is that CCD now is based on monitoring the file
        // changes before deciding to upload. If the change is within one sec from
        // previous one, it may not think there is a change and will not sync up
        VPLThread_Sleep(VPLTime_FromSec(1));

        rv = msaBeginCatalog(media_metadata::MM_CATALOG_MUSIC, target);
        if (rv != 0) {
            LOG_ERROR("msaBeginCatalog failed: %d", rv);
            goto exit;
        }

        {   // 7) See function comment above
            LOG_INFO("Deleting 1 music album.");
            std::string album_name = MUSIC_ALBUM;
            std::ostringstream album_number;
            album_number << num_music_albums;
            album_name += album_number.str();
            if(useLargeNames) {
                album_name += MUSIC_COLLECTION_LARGE_NAME_SUFFIX;
            }
            ccd::DeleteCollectionInput deleteCollectionInput;
            deleteCollectionInput.set_collection_id(album_name);

            rv = target->MSADeleteCollection(deleteCollectionInput);
            if (rv != 0) {
                LOG_ERROR("MSADeleteCollection failed: %d", rv);
                goto exit;
            }
            LOG_INFO("MSADeleteCollection(%s) OK", album_name.c_str());

        }

        // 8) Remove the last music track in album 0, so that there are
        //    NUM_MUSIC_TRACKS_PER_ALBUM media items in every album.
        {   // 8) See function comment above
            LOG_INFO("Removing 1 music track for first music album.");
            std::string album_name = MUSIC_ALBUM;
            std::ostringstream album_number;
            album_number << 0;
            album_name += album_number.str();
            if(useLargeNames) {
                album_name += MUSIC_COLLECTION_LARGE_NAME_SUFFIX;
            }
            ccd::BeginMetadataTransactionInput beginReq2;
            beginReq2.set_collection_id(album_name);
            beginReq2.set_reset_collection(false);
            beginReq2.set_collection_timestamp(s_timestamp);
            if(incrementTimestamp) {
                s_timestamp++;
            }
            int rc = target->MSABeginMetadataTransaction(beginReq2);
            if (rc != 0) {
                LOG_ERROR("MSABeginMetadataTransaction failed: %d", rc);
                rv = rc;
                goto exit;
            } else {
                LOG_INFO("MSABeginMetadataTransaction(%s) OK", album_name.c_str());
            }

            ccd::DeleteMetadataInput deleteMetadataInput;
            deleteMetadataInput.set_object_id(object_id_to_remove);
            rc = target->MSADeleteMetadata(deleteMetadataInput);
            if(rc != 0) {
                LOG_ERROR("MSADeleteMetadata: %d", rc);
                rv = rc;
                goto exit;
            }

            rv = target->MSACommitMetadataTransaction();
            if (rv != 0) {
                LOG_ERROR("MSACommitMetadataTransaction failed: %d", rv);
                goto exit;
            }
            LOG_INFO("MSADeleteMetadata(%s): OK", object_id_to_remove.c_str());
        }
        LOG_INFO("msaCommitCatalog for deleting 1 music album and deleting a track "
                 "from each of the other music albums.  May take a while for large "
                 "numbers of music albums. See replay directory for progress.");
        rv = msaCommitCatalog(media_metadata::MM_CATALOG_MUSIC, target);
        if (rv != 0) {
            LOG_ERROR("msaLogout failed: %d", rv);
            goto exit;
        }

        rv = msaBeginCatalog(media_metadata::MM_CATALOG_PHOTO, target);
        if (rv != 0) {
            LOG_ERROR("msaBeginCatalog failed: %d", rv);
            goto exit;
        }

        {   // 7.1) See function comment above
            LOG_INFO("Deleting 1 photo album.");
            std::string album_name = PHOTO_ALBUM;
            std::ostringstream album_number;
            album_number << num_photo_albums;
            album_name += album_number.str();
            if(useLargeNames) {
                album_name += MUSIC_COLLECTION_LARGE_NAME_SUFFIX;
            }
            ccd::DeleteCollectionInput deleteCollectionInput;
            deleteCollectionInput.set_collection_id(album_name);

            rv = target->MSADeleteCollection(deleteCollectionInput);
            if (rv != 0) {
                LOG_ERROR("MSADeleteCollection(%s) failed: %d", album_name.c_str(),rv);
                goto exit;
            }
            LOG_INFO("MSADeleteCollection(%s) OK", album_name.c_str());

        }
        // 8.1) Remove the first photo in album 0, so that there are
        //    NUM_PHOTO_TRACKS_PER_ALBUM media items in every album.
        {   // 8.1) See function comment above
            LOG_INFO("Removing 1 photo for first photo album.");
            std::string album_name = PHOTO_ALBUM;
            std::ostringstream album_number;
            album_number << 0;
            album_name += album_number.str();
            if(useLargeNames) {
                album_name += MUSIC_COLLECTION_LARGE_NAME_SUFFIX;
            }
            ccd::BeginMetadataTransactionInput beginReq2;
            beginReq2.set_collection_id(album_name);
            beginReq2.set_reset_collection(false);
            beginReq2.set_collection_timestamp(s_timestamp);
            if(incrementTimestamp) {
                s_timestamp++;
            }
            int rc = target->MSABeginMetadataTransaction(beginReq2);
            if (rc != 0) {
                LOG_ERROR("MSABeginMetadataTransaction failed: %d", rc);
                rv = rc;
                goto exit;
            } else {
                LOG_INFO("MSABeginMetadataTransaction(%s) OK", album_name.c_str());
            }

            ccd::DeleteMetadataInput deleteMetadataInput;
            deleteMetadataInput.set_object_id(photo_object_id_to_remove);
            rc = target->MSADeleteMetadata(deleteMetadataInput);
            if(rc != 0) {
                LOG_ERROR("MSADeleteMetadata: %d", rc);
                rv = rc;
                goto exit;
            }

            rv = target->MSACommitMetadataTransaction();
            if (rv != 0) {
                LOG_ERROR("MSACommitMetadataTransaction failed: %d", rv);
                goto exit;
            }
            LOG_INFO("MSADeleteMetadata(%s): OK", photo_object_id_to_remove.c_str());
        }
        LOG_INFO("msaCommitCatalog for deleting 1 photo album and deleting a photo "
                 "from each of the other photo albums.  May take a while for large "
                 "numbers of photo albums. See replay directory for progress.");

        rv = msaCommitCatalog(media_metadata::MM_CATALOG_PHOTO, target);
        if (rv != 0) {
            LOG_ERROR("msaLogout failed: %d", rv);
            goto exit;
        }

        LOG_INFO("msaCommitCatalog OK");
    }

    // Seems like a delete takes time after the actual function call?
    VPLThread_Sleep(VPLTime_FromSec(1));

    LOG_INFO("Printing collection details");
    // 9) See function comment above
    {
        int countedMusicAlbums = 0;
        int countedPhotoAlbums = 0;
        media_metadata::ListCollectionsOutput listCollectionsOutput;

        rv = msaBeginCatalog(media_metadata::MM_CATALOG_MUSIC, target);
        if (rv != 0) {
            LOG_ERROR("msaBeginCatalog failed: %d", rv);
            goto exit;
        }

        rc = target->MSAListCollections(listCollectionsOutput);
        if(rc != VPL_OK) {
            LOG_ERROR("MSAListCollections error: %d", rc);
            rv = rc;
            goto exit;
        }
        for(int albumIndex = 0; albumIndex<listCollectionsOutput.collection_id_size(); albumIndex++) {
            std::string beginsWith(MUSIC_ALBUM);
            if(listCollectionsOutput.collection_id(albumIndex).compare(0, beginsWith.size(), beginsWith) != 0)
            {   // Not a music album
                continue;
            }
            std::string albumNumberStr = listCollectionsOutput.collection_id(albumIndex).substr(beginsWith.size());
            albumNumberStr = albumNumberStr.substr(0, albumNumberStr.size()-strlen(MUSIC_COLLECTION_LARGE_NAME_SUFFIX));
            int albumNumber = atoi(albumNumberStr.c_str());
            countedMusicAlbums++;
            ccd::GetCollectionDetailsInput getCollectionDetailsInput;
            ccd::GetCollectionDetailsOutput getCollectionDetailsOutput;
            getCollectionDetailsInput.set_collection_id(listCollectionsOutput.collection_id(albumIndex));

            rc = target->MSAGetCollectionDetails(getCollectionDetailsInput,
                                                 getCollectionDetailsOutput);
            if(rc != 0) {
                LOG_ERROR("MSAGetCollectionDetails:%d", rc);
                rv = rc;
            }

            LOG_INFO("Collection list Music index %d: ID:%s, albumNum:%d, TIMESTAMP:"FMTx64", NumMetadata:%d",
                     albumIndex, listCollectionsOutput.collection_id(albumIndex).c_str(), albumNumber,
                     listCollectionsOutput.collection_timestamp(albumIndex),
                     getCollectionDetailsOutput.metadata_size());
            // 8) See function comment above
            if(getCollectionDetailsOutput.metadata_size()!=num_music_tracks_per_album+1)
            {
                LOG_ERROR("There should be %d metadatas, instead there are %d metadatas for albumNumber:%d",
                          num_music_tracks_per_album+1, // num tracks + album object
                          getCollectionDetailsOutput.metadata_size(),
                          albumNumber);
                rv = -5;
            }
        }
        // 10) See function comment above
        if(countedMusicAlbums != num_music_albums) {
            LOG_ERROR("ERROR, number of albums should be %d, not %d",
                      num_music_albums, listCollectionsOutput.collection_id_size());
            rv = -3;
        }
        rv = msaCommitCatalog(media_metadata::MM_CATALOG_MUSIC, target);
        if (rv != 0) {
            LOG_ERROR("msaCommitCatalog failed: %d", rv);
            goto exit;
        }

        rv = msaBeginCatalog(media_metadata::MM_CATALOG_PHOTO, target);
        if (rv != 0) {
            LOG_ERROR("msaBeginCatalog failed: %d", rv);
            goto exit;
        }

        listCollectionsOutput.Clear();
        rc = target->MSAListCollections(listCollectionsOutput);
        if(rc != VPL_OK) {
            LOG_ERROR("MSAListCollections error: %d", rc);
            rv = rc;
            goto exit;
        }

        // 11) See function comment above
        for(int albumIndex = 0; albumIndex<listCollectionsOutput.collection_id_size(); albumIndex++) {
            std::string beginsWith(PHOTO_ALBUM);
            if(listCollectionsOutput.collection_id(albumIndex).compare(0, beginsWith.size(), beginsWith) != 0)
            {   // Not a music album
                continue;
            }
            std::string albumNumberStr = listCollectionsOutput.collection_id(albumIndex).substr(beginsWith.size());
            int albumNumber = atoi(albumNumberStr.c_str());
            countedPhotoAlbums++;
            ccd::GetCollectionDetailsInput getCollectionDetailsInput;
            ccd::GetCollectionDetailsOutput getCollectionDetailsOutput;
            getCollectionDetailsInput.set_collection_id(listCollectionsOutput.collection_id(albumIndex));

            rc = target->MSAGetCollectionDetails(getCollectionDetailsInput,
                                                 getCollectionDetailsOutput);
            if(rc != 0) {
                LOG_ERROR("MSAGetCollectionDetails:%d", rc);
                rv = rc;
            }

            LOG_INFO("Collection list Photo index %d: ID:%s, TIMESTAMP:"FMTx64", NumMetadata:%d",
                     albumIndex, listCollectionsOutput.collection_id(albumIndex).c_str(),
                     listCollectionsOutput.collection_timestamp(albumIndex),
                     getCollectionDetailsOutput.metadata_size());
            // 8) See function comment above
            if(getCollectionDetailsOutput.metadata_size()!=num_photos_per_album)
            {
                LOG_ERROR("There should be %d photos, instead there are %d metadatas for collectionIndex:%d, albumNum:%d",
                          num_photos_per_album, // num tracks + album object
                          getCollectionDetailsOutput.metadata_size(),
                          albumIndex, albumNumber);
                rv = -5;
            }
        }
        // 12) See function comment above
        if(countedPhotoAlbums != num_photo_albums) {
            LOG_ERROR("ERROR, number of photo albums should be %d, not %d",
                      num_photo_albums, countedPhotoAlbums);
            rv = -3; // Temporary set to 0, pending investigation by Robert why album number mismatched
        }
        rv = msaEndCatalog(media_metadata::MM_CATALOG_PHOTO, target);
        if (rv != 0) {
            LOG_ERROR("msaEndCatalog failed: %d", rv);
            goto exit;
        }
    }
 exit:
    return rv;
}

static bool isJpeg(const std::string& filepath)
{
    std::string jpgExt(".jpg");
    if(filepath.size() < jpgExt.size()) {
        return false;
    }
    std::string toCompare = filepath.substr(filepath.size() - jpgExt.size(),
                                            jpgExt.size());
    for(u32 i=0; i<toCompare.size(); ++i) {
        toCompare[i] = toLower(toCompare[i]);
    }
    if(toCompare == jpgExt) {
        return true;
    }
    return false;
}

static bool isMusic(const std::string& filepath)
{
    // TODO: For now use mp3 or wma, later use sw_x/gvm_core/internal/file_utils/src/util_mime.cpp
    std::string mp3Ext(".mp3");
    std::string wmaExt(".wma");
    if(filepath.size() < mp3Ext.size() && filepath.size()<wmaExt.size()) {
        return false;
    }
    std::string toCompare = filepath.substr(filepath.size() - mp3Ext.size(),
                                            mp3Ext.size());
    for(u32 i=0; i<toCompare.size(); ++i) {
        toCompare[i] = toLower(toCompare[i]);
    }
    if(toCompare == mp3Ext || toCompare == wmaExt) {
        return true;
    }
    return false;
}

static bool isPhoto(const std::string& filepath)
{
    std::map<std::string, std::string, case_insensitive_less> photoMap;
    Util_CreatePhotoMimeMap(photoMap);
    std::string fPath = std::string(filepath);
    transform(fPath.begin(), fPath.end(), fPath.begin(), toLower);
    
    std::string::size_type idx = fPath.find_last_of('.');
    if (idx == std::string::npos || idx == (fPath.length() - 1))
    {
        return false;
    }

    std::string fileExt = fPath.substr(idx + 1);
    if (photoMap.find(fileExt) == photoMap.end())
    {
        return false;
    }

    return true;
}

static void toHashString(const u8* hash, std::string& hashString)
{
    // Write recorded hash
    hashString.clear();
    for(int hashIndex = 0; hashIndex<CSL_SHA1_DIGESTSIZE; hashIndex++) {
        char byteStr[4];
        snprintf(byteStr, sizeof(byteStr), "%02"PRIx8, hash[hashIndex]);
        hashString.append(byteStr);
    }
}

static int calcHash(const std::string str_in, std::string& hash_out)
{
    int rv;
    CSL_ShaContext hashState;
    u8 sha1digest[CSL_SHA1_DIGESTSIZE];

    rv = CSL_ResetSha(&hashState);
    if(rv != 0) {
        LOG_ERROR("CSL_ResetSha should never fail:%d", rv);
        return rv;
    }

    rv = CSL_InputSha(&hashState,
                      str_in.c_str(),
                      str_in.size());
    if(rv != 0) {
        LOG_ERROR("CSL_InputSha should never fail:%d", rv);
        return rv;
    }

    rv = CSL_ResultSha(&hashState, sha1digest);
    if(rv != 0) {
        LOG_ERROR("CSL_ResultSha should never fail:%d", rv);
        return rv;
    }

    toHashString(sha1digest, hash_out);
    return 0;
}


#if defined(CLOUDNODE)
#define CHECK_AND_PUSH_THUMB(thumbPath) { \
    VPLFS_stat_t stat_dst; \
    rc = target->statFile(thumbPath, stat_dst); \
    if(rc != VPL_OK){ \
        rc = target->createDir(thumbPath, 0755, VPL_FALSE); \
        if (rc != 0) { \
            LOG_ERROR("createDir failed! rc = %d", rc); \
            return -1; \
        } \
        rc = target->pushFile(thumbPath, thumbPath); \
        if (rc != 0) { \
            LOG_ERROR("Failed to push %s to %s: %d", thumbPath.c_str(), thumbPath.c_str(), rv); \
            return -1; \
        } \
    } \
}
#endif


int msa_add_cloud_music(const std::string& musicPath)
{
    int rv = 0;
    int rc;
    VPLFS_dir_t dir_folder;
    std::deque<std::string> localDirs;
    localDirs.push_back(musicPath);
    int numMusic = 0;

#if defined(CLOUDNODE)
    u64 userId;
    u64 datasetVDId;
    std::stringstream datasetVDPrefix;
    rc = getUserIdBasic(&userId);
    if (rc != 0) {
        LOG_ERROR("Fail to get user id:%d", rc);
        rv = rc;
        return -1;
    }
    rv = getDatasetId(userId, "Virt Drive", datasetVDId);
    if (rv != 0) {
        LOG_ERROR("Fail to get dataset id");
        return -1;
    }
    datasetVDPrefix << "/dataset/" << userId << "/" << datasetVDId ;
#endif

    AutoTargetDevice atd;
    TargetDevice *target = atd.create();

    rv = msaInit(target);
    if (rv != 0) {
        LOG_ERROR("msaInit failed: %d", rv);
        return -1;
    }

    bool catalogCreated = false;
    while(!localDirs.empty()) {
        std::string dirPath(localDirs.front());
        localDirs.pop_front();

        rc = VPLFS_Opendir(dirPath.c_str(), &dir_folder);
        if (rc != VPL_OK) {
            LOG_ERROR("Failed to open directory \"%s\":%d", dirPath.c_str(), rc);
            return -1;
        }

        std::string folderName;
        std::string path;
        splitAbsPath(dirPath, path, folderName);

        std::string folderNameHash;
        rc = calcHash(folderName, folderNameHash);
        if(rc != 0) {
            LOG_ERROR("Getting hash for %s:%d", folderName.c_str(), rc);
            continue;
        }

        int trackNum = 1;
        bool collectionCreated = false;
        VPLFS_dirent_t folderDirent;
        std::string thumbnail = "";
        while((rc = VPLFS_Readdir(&dir_folder, &folderDirent))==VPL_OK)
        {
            std::string dirent(folderDirent.filename);
            std::string fullPath = dirPath+"/"+dirent;
            if(folderDirent.type != VPLFS_TYPE_FILE) {
                if(dirent=="." || dirent==".." || dirent==".sync_temp") {
                    // We want to skip these directories
                }else{
                    // save directory for later processing
                    localDirs.push_back(fullPath);
                }
                continue;
            }

            if(isJpeg(dirent)) {  // Choose any jpg as a thumbnail.
#if !defined(CLOUDNODE)
                thumbnail = fullPath;
#else
                CHECK_AND_PUSH_THUMB(fullPath);

                thumbnail.assign("/native");
                thumbnail.append(fullPath);
#endif
                continue;
            }
#if defined(CLOUDNODE)
            fullPath = datasetVDPrefix.str() + fullPath;
#endif

            if(!isMusic(dirent)) {
                continue;
            }
            numMusic++;

            if(!catalogCreated) {
                rc = msaBeginCatalog(media_metadata::MM_CATALOG_MUSIC, target);
                if (rc != 0) {
                    LOG_ERROR("msaBeginCatalog failed: %d", rc);
                    rv = rc;
                    goto error_beginCatalog;
                }
                catalogCreated = true;
            }

            if(!collectionCreated) {
                LOG_INFO(">>>> MSABeginMetadataTransaction");
                ccd::BeginMetadataTransactionInput beginMetaTrans;
                beginMetaTrans.set_collection_id(folderNameHash);
                beginMetaTrans.set_collection_timestamp(VPLTime_GetTime()); // This should be the folder time, setting current time for now.
                beginMetaTrans.set_reset_collection(true);
                rc = target->MSABeginMetadataTransaction(beginMetaTrans);
                if (rc != 0) {
                    LOG_ERROR("MSABeginMetadataTransaction failed: %d", rc);
                    rv = rc;
                    continue;
                } else {
                    LOG_INFO("MSABeginMetadataTransaction OK:%s", folderName.c_str());
                }
                collectionCreated = true;
            }

            // Update metadata with mp3.
            std::string oid;
            rc = calcHash(fullPath, oid);
            if(rc != 0)
            {
                LOG_ERROR("Getting obj hash for %s:%d", fullPath.c_str(), rc);
                rv = rc;
                continue;
            }

            ccd::UpdateMetadataInput updateMetaReq;
            updateMetaReq.mutable_metadata()->set_object_id(oid);
            updateMetaReq.mutable_metadata()->set_source(media_metadata::MEDIA_SOURCE_LIBRARY);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_absolute_path(fullPath);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_artist("iGwareTrack");
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_title(dirent);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_album_ref(folderNameHash);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_track_number(trackNum++);
            updateMetaReq.mutable_metadata()->mutable_music_track()->set_file_format(get_extension(fullPath));
            rv = target->MSAUpdateMetadata(updateMetaReq);  // music track
            if (rv != 0) {
                LOG_ERROR("MSAUpdateMetadata failed: %d", rv);
                continue;
            }
        }  // while((rc = VPLFS_Readdir(&dir_folder, &folderDirent))==VPL_OK)
        if (rc != VPL_ERR_MAX) {
            LOG_WARN("VPLFS_Readdir failed: %d", rc);
        }

        rc = VPLFS_Closedir(&dir_folder);
        if(rc != VPL_OK) {
            LOG_ERROR("Closing %s:%d", dirPath.c_str(), rc);
        }

        if(collectionCreated) {
            // Create album object after the all the tracks (copy app behavior)
            ccd::UpdateMetadataInput updateMetaAlbumReq;
            updateMetaAlbumReq.mutable_metadata()->set_object_id(folderNameHash);
            updateMetaAlbumReq.mutable_metadata()->set_source(media_metadata::MEDIA_SOURCE_ITUNES);
            updateMetaAlbumReq.mutable_metadata()->mutable_music_album()->set_album_artist("iGwareAlbum");
            updateMetaAlbumReq.mutable_metadata()->mutable_music_album()->set_album_name(folderName);
            if (!thumbnail.empty()) {
                updateMetaAlbumReq.mutable_metadata()->mutable_music_album()->set_album_thumbnail(thumbnail);
            }
            rc = target->MSAUpdateMetadata(updateMetaAlbumReq);  // album object
            if (rc != 0) {
                LOG_ERROR("MSAUpdateMetadata failed: %d", rc);
                rv = rc;
                continue;
            }

            // Commit collection
            LOG_INFO(">>>> Incremental MSACommitMetadataTransaction");
            rc = target->MSACommitMetadataTransaction();
            if (rc != 0) {
                LOG_ERROR("MSACommitMetadataTransaction failed: %d", rc);
                rv = rc;
                continue;
            }
        }
    }

    if(catalogCreated) {
        // Commit catalog
        rc = msaCommitCatalog(media_metadata::MM_CATALOG_MUSIC, target);
        if (rc != 0) {
            LOG_ERROR("msaCommitCatalog failed: %d", rc);
            rv = rc;
        }
    }
 error_beginCatalog:

    return rv;
}

int msa_delete_object(media_metadata::CatalogType_t catType,
                      const std::string& collectionId,
                      const std::string& objectId)
{
    int rc;
    int rv = 0;

    AutoTargetDevice atd;
    TargetDevice *target = atd.create();

    rv = msaInit(target);
    if (rv != 0) {
        LOG_ERROR("msaInit failed: %d", rv);
        return -1;
    }

    ccd::BeginMetadataTransactionInput beginMetaTrans;
    ccd::DeleteMetadataInput deleteMetaInput;
    LOG_INFO(">>>> msaBeginCatalog");
    rc = msaBeginCatalog(catType, target);
    if (rc != 0) {
        LOG_ERROR("msaBeginCatalog failed: %d", rc);
        rv = rc;
        goto error_beginCatalog;
    }

    LOG_INFO(">>>> MSABeginMetadataTransaction");
    beginMetaTrans.set_collection_id(collectionId);
    beginMetaTrans.set_collection_timestamp(VPLTime_GetTime()); // This should be the folder time, setting current time for now.
    rc = target->MSABeginMetadataTransaction(beginMetaTrans);
    if (rc != 0) {
        LOG_ERROR("MSABeginMetadataTransaction failed: %d", rc);
        rv = rc;
        goto error;
    } else {
        LOG_INFO("MSABeginMetadataTransaction OK:%s", collectionId.c_str());
    }

    deleteMetaInput.set_object_id(objectId);
    rc = target->MSADeleteMetadata(deleteMetaInput);
    if(rc != 0)
    {
        LOG_ERROR("MSADeleteMetadata:%s,%d", objectId.c_str(), rc);
        rv = rc;
        goto error;
    }

    rc = target->MSACommitMetadataTransaction();
    if (rc != 0) {
        LOG_ERROR("MSACommitMetadataTransaction failed: %d", rc);
        rv = rc;
        goto error;
    }

    // Commit catalog
    rc = msaCommitCatalog(catType, target);
    if (rc != 0) {
        LOG_ERROR("msaCommitCatalog failed: %d", rc);
        rv = rc;
    }

 error:

 error_beginCatalog:
    return rv;
}

int msa_delete_collection(media_metadata::CatalogType_t catType,
                          const std::string& collectionId)
{
    int rc;
    int rv = 0;

    AutoTargetDevice atd;
    TargetDevice *target = atd.create();

    rv = msaInit(target);
    if (rv != 0) {
        LOG_ERROR("msaInit failed: %d", rv);
        return -1;
    }

    ccd::DeleteCollectionInput deleteCollectionInput;
    LOG_INFO(">>>> msaBeginCatalog");
    rc = msaBeginCatalog(catType, target);
    if (rc != 0) {
        LOG_ERROR("msaBeginCatalog failed: %d", rc);
        rv = rc;
        goto error_beginCatalog;
    }

    LOG_INFO(">>>> MSADeleteCollection");
    deleteCollectionInput.set_collection_id(collectionId);
    rc = target->MSADeleteCollection(deleteCollectionInput);
    if (rc != 0) {
        LOG_ERROR("MSADeleteCollection failed: %d", rc);
        rv = rc;
        goto error;
    } else {
        LOG_INFO("MSADeleteCollection OK:%s", collectionId.c_str());
    }

    // Commit catalog
    rc = msaCommitCatalog(catType, target);
    if (rc != 0) {
        LOG_ERROR("msaCommitCatalog failed: %d", rc);
        rv = rc;
    }

 error:
 error_beginCatalog:
    return rv;
}

int msa_add_cloud_photo(const std::string& photoPath,
                        int intervalSec,
                        const std::string& albumName,
                        const std::string& thumbnailPath)
{
    int rv = 0;
    int rc;
    VPLFS_dir_t dir_folder;
    std::deque<std::string> localDirs;
    localDirs.push_back(photoPath);
    int numPhoto = 0;
    bool isVirtualAlbum = false;

    AutoTargetDevice atd;
    TargetDevice *target = atd.create();

    rv = msaInit(target);
    if (rv != 0) {
        LOG_ERROR("msaInit failed: %d", rv);
        return -1;
    }

#if defined(CLOUDNODE)
    u64 userId;
    u64 datasetVDId;
    std::stringstream datasetVDPrefix;
    rc = getUserIdBasic(&userId);
    if (rc != 0) {
        LOG_ERROR("Fail to get user id:%d", rc);
        rv = rc;
        return -1;
    }
    rv = getDatasetId(userId, "Virt Drive", datasetVDId);
    if (rv != 0) {
        LOG_ERROR("Fail to get dataset id");
        return -1;
    }
    datasetVDPrefix << "/dataset/" << userId << "/" << datasetVDId ;
#endif
    bool catalogCreated = false;
    while(!localDirs.empty()) {
        std::string dirPath(localDirs.front());
        localDirs.pop_front();

        rc = VPLFS_Opendir(dirPath.c_str(), &dir_folder);
        if (rc != VPL_OK) {
            LOG_ERROR("Failed to open directory \"%s\":%d", dirPath.c_str(), rc);
            return -1;
        }

        std::string folderName;
        std::string path;
        splitAbsPath(dirPath, path, folderName);

        std::string folderNameHash;
        if(albumName.length() <= 0) {
            rc = calcHash(folderName, folderNameHash);

            if(rc != 0) {
                LOG_ERROR("Getting hash for %s:%d", folderName.c_str(), rc);
                continue;
            }
            isVirtualAlbum = false;
        } else {
            folderNameHash = albumName + "Collection";
            isVirtualAlbum = true;
        }

        bool collectionCreated = false;
        VPLFS_dirent_t folderDirent;
        std::string albumObjId;
        while((rc = VPLFS_Readdir(&dir_folder, &folderDirent))==VPL_OK)
        {
            std::string dirent(folderDirent.filename);
            std::string fullPath = dirPath;
#if defined(CLOUDNODE)
            std::string fullPathOri = "";
#endif
            fullPath += ("/" + dirent);
            std::string direntLowercase(dirent);
            transform(direntLowercase.begin(), direntLowercase.end(), direntLowercase.begin(), tolower);
            if(folderDirent.type != VPLFS_TYPE_FILE) {
                if(dirent=="." || dirent==".." || dirent==".sync_temp" || (direntLowercase.compare("thumb") == 0) ) {
                    // We want to skip these directories
                }else{
                    // save directory for later processing
                    localDirs.push_back(fullPath);
                }
                continue;
            }

            if(!isPhoto(dirent)) {
                LOG_INFO("%s is not a photo", dirent.c_str());
                continue;
            }
            ++numPhoto;

            if(!catalogCreated && intervalSec < 0) {
                rc = msaBeginCatalog(media_metadata::MM_CATALOG_PHOTO, target);
                if (rc != 0) {
                    LOG_ERROR("msaBeginCatalog failed: %d", rc);
                    rv = rc;
                    goto error_beginCatalog;
                }
                catalogCreated = true;
            }

            if(!collectionCreated && intervalSec < 0) {
                LOG_INFO(">>>> MSABeginMetadataTransaction");
                ccd::BeginMetadataTransactionInput beginMetaTrans;
                beginMetaTrans.set_collection_id(folderNameHash);
                beginMetaTrans.set_collection_timestamp(VPLTime_GetTime()); // This should be the folder time, setting current time for now.
                beginMetaTrans.set_reset_collection(/*true*/!isVirtualAlbum);
                rc = target->MSABeginMetadataTransaction(beginMetaTrans);
                if (rc != 0) {
                    LOG_ERROR("MSABeginMetadataTransaction failed: %d", rc);
                    rv = rc;
                    continue;
                } else {
                    LOG_INFO("MSABeginMetadataTransaction OK:%s", folderName.c_str());
                }
                collectionCreated = true;
                if (isVirtualAlbum) {
                    rc = calcHash(albumName, albumObjId);
                    if(rc != 0)
                    {
                        LOG_ERROR("Getting obj hash for %s:%d", albumName.c_str(), rc);
                        rv = rc;
                        continue;
                    }
                    ccd::UpdateMetadataInput updateAlbumMetaReq;
                    updateAlbumMetaReq.mutable_metadata()->set_object_id(albumObjId);
                    updateAlbumMetaReq.mutable_metadata()->set_source(media_metadata::MEDIA_SOURCE_LIBRARY);
                    updateAlbumMetaReq.mutable_metadata()->mutable_photo_album()->set_album_name(albumName);
                    if (thumbnailPath.length() > 0 ) {
                        updateAlbumMetaReq.mutable_metadata()->mutable_photo_album()->set_album_thumbnail(thumbnailPath);
                    }
                    updateAlbumMetaReq.mutable_metadata()->mutable_photo_album()->set_timestamp(VPLTime_GetTime());
                    rv = target->MSAUpdateMetadata(updateAlbumMetaReq);
                    if (rv != 0) {
                        LOG_ERROR("MSAUpdateMetadata for photo alubm failed: %d", rv);
                        continue;
                    } else {
                        LOG_INFO("MSAUpdateMetadata  for photo alubm Success");
                    }
                }
            }

            // Update metadata with image.
            std::string oid;
#if defined(CLOUDNODE)
            fullPathOri.assign(fullPath);
            fullPath = datasetVDPrefix.str() + fullPath;
#endif
            rc = calcHash(fullPath, oid);

            if(rc != 0)
            {
                LOG_ERROR("Getting obj hash for %s:%d", fullPath.c_str(), rc);
                rv = rc;
                continue;
            }

            VPLTime_t photoDatetime = VPLTime_GetTime();
            VPLFS_stat_t photoStat;
#if !defined(CLOUDNODE)
            if ((rc = VPLFS_Stat(fullPath.c_str(), &photoStat)) != VPL_OK) {
                LOG_ERROR("VPLFS_Stat failed:%s, %d", fullPath.c_str(), rc);
#else
            if ((rc = VPLFS_Stat(fullPathOri.c_str(), &photoStat)) != VPL_OK) {
                LOG_ERROR("VPLFS_Stat failed:%s, %d", fullPathOri.c_str(), rc);
#endif
                continue;
            }

            LOG_INFO("Add photo item:%s", fullPath.c_str());
            photoDatetime = photoStat.ctime;
            ccd::UpdateMetadataInput updateMetaReq;
            updateMetaReq.mutable_metadata()->set_object_id(oid);
            updateMetaReq.mutable_metadata()->set_source(media_metadata::MEDIA_SOURCE_LIBRARY);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_absolute_path(fullPath);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_title(dirent);
            if (isVirtualAlbum) {
                updateMetaReq.mutable_metadata()->mutable_photo_item()->set_album_name(albumName);
            } else {
                updateMetaReq.mutable_metadata()->mutable_photo_item()->set_album_name(folderName);
            }
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_date_time(photoDatetime);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_file_format(get_extension(fullPath));
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_file_size(photoStat.size);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_date_time_updated(photoStat.mtime);
            updateMetaReq.mutable_metadata()->mutable_photo_item()->set_orientation(0);
            if (isVirtualAlbum) {
                updateMetaReq.mutable_metadata()->mutable_photo_item()->set_album_ref(albumObjId);
            }

            {
#if !defined(CLOUDNODE)
                std::string direntExt = get_extension(fullPath);
#else
                std::string direntExt = get_extension(fullPathOri);
#endif
                std::string thumbExtJpg = "thumb.jpg";
                std::string thumbDirentJpg = std::string(dirent);
                thumbDirentJpg.replace(thumbDirentJpg.find_last_of('.') + 1, direntExt.size(), thumbExtJpg);
                std::string thumbPathJpg = dirPath;
                thumbPathJpg += ("/thumb/" + thumbDirentJpg);

                std::string thumbExtPng = "thumb.png";
                std::string thumbDirentPng = std::string(dirent);
                thumbDirentPng.replace(thumbDirentPng.find_last_of('.') + 1, direntExt.size(), thumbExtPng);
                std::string thumbPathPng = dirPath;
                thumbPathPng += ("/thumb/" + thumbDirentPng);

                VPLFS_stat_t thumbStat;
                if (VPLFS_Stat(thumbPathJpg.c_str(), &thumbStat) == VPL_OK)
                {
#if defined(CLOUDNODE)
                    CHECK_AND_PUSH_THUMB(thumbPathJpg); 

                    thumbPathJpg = "/native" + thumbPathJpg;
#endif
                    updateMetaReq.mutable_metadata()->mutable_photo_item()->set_thumbnail(thumbPathJpg);
                }
                else if (VPLFS_Stat(thumbPathPng.c_str(), &thumbStat) == VPL_OK)
                {
#if defined(CLOUDNODE)
                    CHECK_AND_PUSH_THUMB(thumbPathPng); 

                    thumbPathPng = "/native" + thumbPathPng;
#endif
                    updateMetaReq.mutable_metadata()->mutable_photo_item()->set_thumbnail(thumbPathPng);
                }
                else
                {
                    LOG_INFO("Thumbnail for %s is NOT existed. skip set_thumbnail method.", dirent.c_str());
                }
            }

            if(intervalSec >= 0) {
                rc = msaBeginCatalog(media_metadata::MM_CATALOG_PHOTO, target);
                if (rc != 0) {
                    LOG_ERROR("msaBeginCatalog failed: %d", rc);
                    rv = rc;
                    goto error_beginCatalog;
                }

                LOG_INFO(">>>> MSABeginMetadataTransaction");
                ccd::BeginMetadataTransactionInput beginMetaTrans;
                beginMetaTrans.set_collection_id(folderNameHash);
                beginMetaTrans.set_collection_timestamp(VPLTime_GetTime()); // This should be the folder time, setting current time for now.
                beginMetaTrans.set_reset_collection(false);
                rc = target->MSABeginMetadataTransaction(beginMetaTrans);
                if (rc != 0) {
                    LOG_ERROR("MSABeginMetadataTransaction failed: %d", rc);
                    rv = rc;
                    continue;
                } else {
                    LOG_INFO("MSABeginMetadataTransaction OK:%s", folderName.c_str());
                }
                if (isVirtualAlbum) {
                    std::string oid;
                    rc = calcHash(albumName, oid);
                    if(rc != 0)
                    {
                        LOG_ERROR("Getting obj hash for %s:%d", albumName.c_str(), rc);
                        rv = rc;
                        continue;
                    }
                    ccd::UpdateMetadataInput updateAlbumMetaReq;
                    updateAlbumMetaReq.mutable_metadata()->set_object_id(oid);
                    updateAlbumMetaReq.mutable_metadata()->set_source(media_metadata::MEDIA_SOURCE_LIBRARY);
                    updateAlbumMetaReq.mutable_metadata()->mutable_photo_album()->set_album_name(albumName);
                    //updateAlbumMetaReq.mutable_metadata()->mutable_photo_album()->set_album_thumbnail("thumbnail.jpg");
                    updateAlbumMetaReq.mutable_metadata()->mutable_photo_album()->set_timestamp(VPLTime_GetTime());
                    updateMetaReq.mutable_metadata()->mutable_photo_item()->set_album_ref(oid);
                    rv = target->MSAUpdateMetadata(updateAlbumMetaReq);
                    if (rv != 0) {
                        LOG_ERROR("MSAUpdateMetadata for photo alubm failed: %d", rv);
                        continue;
                    } else {
                        LOG_INFO("MSAUpdateMetadata  for photo alubm Success");
                    }
                }
            }

            rv = target->MSAUpdateMetadata(updateMetaReq);  // photo item
            if (rv != 0) {
                LOG_ERROR("MSAUpdateMetadata failed: %d", rv);
                continue;
            } else {
                LOG_INFO("MSAUpdateMetadata Success");
            }
            if(intervalSec >= 0) {
                // Commit collection
                LOG_INFO(">>>> Incremental MSACommitMetadataTransaction for %s:%s at %s",
                         folderNameHash.c_str(), oid.c_str(), fullPath.c_str());
                rc = target->MSACommitMetadataTransaction();
                if (rc != 0) {
                    LOG_ERROR("MSACommitMetadataTransaction failed: %d", rc);
                    rv = rc;
                }
                // Commit catalog
                rc = msaCommitCatalog(media_metadata::MM_CATALOG_PHOTO, target);
                if (rc != 0) {
                    LOG_ERROR("msaCommitCatalog failed: %d", rc);
                    rv = rc;
                } else {
                    LOG_INFO("msaCommitCatalog Success");
                }
                LOG_INFO("Sleeping %d seconds", intervalSec);
                VPLThread_Sleep(VPLTIME_FROM_SEC(intervalSec));
            }
        } // while((rc = VPLFS_Readdir(&dir_folder, &folderDirent))==VPL_OK)
        if (rc != VPL_ERR_MAX) {
            LOG_WARN("VPLFS_Readdir failed: %d", rc);
        }

        rc = VPLFS_Closedir(&dir_folder);
        if(rc != VPL_OK) {
            LOG_ERROR("VPLFS_Closedir(%s) failed: %d", dirPath.c_str(), rc);
        }

        if(collectionCreated) {
            // Commit collection
            LOG_INFO(">>>> Incremental MSACommitMetadataTransaction");
            rc = target->MSACommitMetadataTransaction();
            if (rc != 0) {
                LOG_ERROR("MSACommitMetadataTransaction failed: %d", rc);
                rv = rc;
                continue;
            }
        }
    }

    if(catalogCreated) {
        // Commit catalog
        rc = msaCommitCatalog(media_metadata::MM_CATALOG_PHOTO, target);
        if (rc != 0) {
            LOG_ERROR("msaCommitCatalog failed: %d", rc);
            rv = rc;
        } else {
            LOG_INFO("msaCommitCatalog Success");
        }
    }
 error_beginCatalog:

    return rv;
}

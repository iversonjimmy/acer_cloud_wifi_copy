//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "mca_diag.hpp"
#include <ccdi.hpp>
#include <vplex_file.h>
#include <vpl_fs.h>
#include <vplu_types.h>
#include "vpl_th.h"
#include "ccd_utils.hpp"
#include "gvm_file_utils.hpp"
#include "media_metadata_errors.hpp"

#include <iostream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <deque>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>

#include <log.h>

#include "common_utils.hpp"
#include "dx_common.h"

const char* MCA_STR = "Mca";

int mca_migrate_thumb_cmd(int argc, const char* argv[])
{
    if(checkHelp(argc, argv)) {
        std::cout << MCA_STR << " " << argv[0]
            << " [-d dst_directory (when not specified, default internal dir is used)]"
            << std::endl;
        return 0;
    }
    std::string strDashD = std::string("-d");
    std::string dst_dir;
    for (int i = 1; i < argc; ++i)
    {
        std::string strTmp(argv[i]);
        if (strTmp.compare("-d") == 0 && (i+1 <argc))
        {
            dst_dir = argv[i + 1];
            i++;
        } else {
            LOG_ERROR("Unrecognized option:%s, %d, %d", argv[i], i, argc);
            std::cout << "Usage: " << argv[0]
                      << " [-s src_directory] <-d dst_directory>" << std::endl;
            return -1;
        }
    }
    int rv = mca_migrate_thumb(dst_dir);
    if(rv != 0) {
        LOG_ERROR("mca_migrate_thumb(%s):%d", dst_dir.c_str(), rv);
    }
    return rv;
}

int mca_stop_thumb_sync_cmd(int argc, const char* argv[])
{
    if (checkHelp(argc, argv) || (argc < 1)) {
        printf("%s %s [Photo] [Video] [Music] [All]\n", MCA_STR, argv[0]);
        return 0;
    }

    ccd::UpdateSyncSettingsInput updateSyncSettingsInput;
    ccd::UpdateSyncSettingsOutput updateSyncSettingsOutput;

    u64 userId;
    int rv = 0;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }
    updateSyncSettingsInput.set_user_id(userId);

    updateSyncSettingsInput.set_enable_mm_thumb_sync(false);

    while (--argc > 0) {
        std::string token = *++argv;
        bool all = false;
        bool validToken = false;
        if(token.find("All") != std::string::npos) {
            all = true;
            validToken = true;
        }
        if (token.find("Photo") != std::string::npos) {
            LOG_ALWAYS("add Photo Thumbnails");
            validToken = true;
            updateSyncSettingsInput.add_enable_mm_thumb_sync_types(ccd::SYNC_FEATURE_PHOTO_THUMBNAILS);
        }
        if (token.find("Video") != std::string::npos) {
            LOG_ALWAYS("add Video Thumbnails");
            validToken = true;
            updateSyncSettingsInput.add_enable_mm_thumb_sync_types(ccd::SYNC_FEATURE_VIDEO_THUMBNAILS);
        }
        if (token.find("Music") != std::string::npos) {
            LOG_ALWAYS("add Music Thumbnails");
            validToken = true;
            updateSyncSettingsInput.add_enable_mm_thumb_sync_types(ccd::SYNC_FEATURE_MUSIC_THUMBNAILS);
        }
        if(!validToken) {
            LOG_ERROR("token(%s) is not valid arg. Exiting", token.c_str());
            printf("%s %s [Photo] [Video] [Music] [All]\n", MCA_STR, argv[0]);
            return -1;
        }
    }

    rv = CCDIUpdateSyncSettings(updateSyncSettingsInput, updateSyncSettingsOutput);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d", rv);
        return rv;
    }

    rv = updateSyncSettingsOutput.enable_mm_thumb_sync_err();
    if(rv != 0) {
        LOG_ERROR("Result:%d",rv);
    }

    return rv;
}

int mca_resume_thumb_sync_cmd(int argc, const char* argv[])
{
    if (checkHelp(argc, argv) || (argc < 1)) {
        printf("%s %s [Photo] [Video] [Music] [All]\n", MCA_STR, argv[0]);
        return 0;
    }

    ccd::UpdateSyncSettingsInput updateSyncSettingsInput;
    ccd::UpdateSyncSettingsOutput updateSyncSettingsOutput;

    u64 userId;
    int rv = 0;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }
    updateSyncSettingsInput.set_user_id(userId);

    updateSyncSettingsInput.set_enable_mm_thumb_sync(true);

    while (--argc > 0) {
        std::string token = *++argv;
        bool all = false;
        bool validToken = false;
        if(token.find("All") != std::string::npos) {
            all = true;
            validToken = true;
        }
        if (token.find("Photo") != std::string::npos) {
            LOG_ALWAYS("add Photo Thumbnails");
            validToken = true;
            updateSyncSettingsInput.add_enable_mm_thumb_sync_types(ccd::SYNC_FEATURE_PHOTO_THUMBNAILS);
        }
        if (token.find("Video") != std::string::npos) {
            LOG_ALWAYS("add Video Thumbnails");
            validToken = true;
            updateSyncSettingsInput.add_enable_mm_thumb_sync_types(ccd::SYNC_FEATURE_VIDEO_THUMBNAILS);
        }
        if (token.find("Music") != std::string::npos) {
            LOG_ALWAYS("add Music Thumbnails");
            validToken = true;
            updateSyncSettingsInput.add_enable_mm_thumb_sync_types(ccd::SYNC_FEATURE_MUSIC_THUMBNAILS);
        }

        if(!validToken) {
            LOG_ERROR("token(%s) is not valid arg. Exiting", token.c_str());
            printf("%s %s [Photo] [Video] [Music] [All]\n", MCA_STR, argv[0]);
            return -1;
        }
    }

    rv = CCDIUpdateSyncSettings(updateSyncSettingsInput, updateSyncSettingsOutput);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings:%d", rv);
        return rv;
    }

    rv = updateSyncSettingsOutput.enable_mm_thumb_sync_err();
    if(rv != 0) {
        LOG_ERROR("Result:%d",rv);
    }

    return rv;
}

int mca_status_cmd(int argc, const char* argv[])
{
    if (checkHelp(argc, argv) || (argc != 1)) {
        printf("%s %s\n", MCA_STR, argv[0]);
        return 0;
    }

    ccd::GetSyncStateInput gssInput;
    gssInput.set_only_use_cache(true);
    gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PHOTO_METADATA);
    gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PHOTO_THUMBNAILS);
    gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_MUSIC_METADATA);
    gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_MUSIC_THUMBNAILS);
    gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_VIDEO_METADATA);
    gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_VIDEO_THUMBNAILS);
    gssInput.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PLAYLISTS);
    gssInput.set_get_media_metadata_download_path(true);
    gssInput.set_get_media_playlist_path(true);
    gssInput.set_get_mm_thumb_download_path(true);
    gssInput.set_get_mm_thumb_sync_enabled(true);

    LOG_ALWAYS("Calling CCDIGetSyncState");
    ccd::GetSyncStateOutput gssOutput;
    int rv = CCDIGetSyncState(gssInput, gssOutput);
    if (rv != 0) {
        LOG_ERROR("CCDIGetSyncState failed: %d", rv);
        goto exit;
    }
    LOG_ALWAYS("CCD GetSyncState Mca:\n%s", gssOutput.DebugString().c_str());
 exit:
    return rv;
}

int mca_list_metadata(int argc, const char* argv[])
{
    if (checkHelp(argc, argv)) {
        std::cout << MCA_STR << " ListMetadata" << std::endl;
        std::cout << MCA_STR << " ListMetadata [-s]" << std::endl;
        std::cout << MCA_STR << " ListMetadata [-d metadataDumpfolder]" << std::endl;//
        std::cout << "  w/o arg = simulates MCA command that immediately lists metadata" << std::endl;
        std::cout << "  -s = prints a summary of number of collections and objects per collections" << std::endl;
        std::cout << "  -d = outputs object information in CSV format" << std::endl;
        std::cout << "  metadataDumpfolder = the folder you want to place the csv format metadata" << std::endl;
        std::cout << "McaListMetadata  (legacy, use Mca ListMetadata)" << std::endl;
        return 0;
    }

    int iDumpFileIndex = -1;
    std::string strDumpFileFolder = std::string();
    bool bSummary = false;
    std::string strDashD = std::string("-d");
    std::string strDashS = std::string("-s");
    for (int i = 0; i < argc; ++i)
    {
        std::string strTmp(argv[i]);
        if (strTmp.compare(strDashD) == 0)
        {
            iDumpFileIndex = i + 1;
            if (iDumpFileIndex < argc)
                strDumpFileFolder = std::string(argv[iDumpFileIndex]);
            break;
        }
        else if (strTmp.compare(strDashS) == 0)
        {
            bSummary = true;
            break;
        }
    }

    int rv;
    setDebugLevel(LOG_LEVEL_INFO);
    if (bSummary)
        rv = mca_summary_metadata();
    else if (!strDumpFileFolder.empty())
        rv = mca_dump_metadata(strDumpFileFolder);
    else
        rv = mca_list_metadata();

    resetDebugLevel();
    if(rv != 0) {
        LOG_ERROR("mca_list_metadata failed: %d", rv);
        goto exit;
    }
 exit:
    return rv;
}

int mca_count_metadata_files_cmd(int argc, const char* argv[])
{
    u64 count = 0;
    u64 indexFiles = 0;
    u64 photoFiles = 0;
    int rv = 0;
    if(checkHelp(argc, argv)) {
        printf("%s %s\n", MCA_STR, argv[0]);
        printf("%s (legacy, use Mca %s)\n", argv[0], argv[0]);
        return 0;
    }
    rv = mca_count_metadata_files(count, indexFiles, photoFiles);
    if(rv != 0) {
        LOG_ERROR("mca_count_metadata_files:%d", rv);
        return rv;
    }

    LOG_ALWAYS("Metadata AllFile Count:"FMTu64", indexFiles("FMTu64"), photoFiles("FMTu64")",
               count, indexFiles, photoFiles);
    return rv;
}

static int listMetadataContentHelper(u64 cloudPcId,
                                     const std::string& test_clip_name,
                                     std::string& test_url,
                                     std::string& test_thumb_url,
                                     bool summaryMode,
                                     int& objCnt_out);

static int dumpMetadataContentHelper(u64 cloudPcId,
                                     const std::string& test_clip_name,
                                     std::string& test_url,
                                     std::string& test_thumb_url,
                                     const std::string& dumpfile_name_folder);

int mca_get_metadata_dl_path(std::string& downloadPath_out)
{
    int rv;
    ccd::GetSyncStateInput in;
    ccd::GetSyncStateOutput out;

    in.set_get_media_metadata_download_path(true);
    rv = CCDIGetSyncState(in, out);
    if(rv != 0) {
        LOG_ERROR("CCDIGetSyncState:%d", rv);
        return rv;
    }
    downloadPath_out = out.media_metadata_download_path();
    return rv;
}

#include "MediaMetadataCache.hpp"
int mca_count_metadata_files(u64& allFiles_out, u64& index_out, u64& photoFile_out)
{
    int rc;
    int rv = 0;
    std::string downloadPath;

    rv = mca_get_metadata_dl_path(downloadPath);
    if(rv != 0) {
        LOG_ERROR("mca_get_metadata_dl_path:%d", rv);
        return rv;
    }

    allFiles_out = 0;
    index_out = 0;
    photoFile_out = 0;
    VPLFS_dir_t dir_folder;
    std::deque<std::string> localDirs;
    localDirs.push_back(downloadPath);

    while(!localDirs.empty()) {
        std::string dirPath(localDirs.front());
        localDirs.pop_front();

        rc = VPLFS_Opendir(dirPath.c_str(), &dir_folder);
        if(rc == VPL_ERR_NOENT){
            // no photos
            continue;
        }else if(rc != VPL_OK) {
            LOG_ERROR("Unable to open %s:%d", dirPath.c_str(), rc);
            continue;
        }

        VPLFS_dirent_t folderDirent;
        while((rc = VPLFS_Readdir(&dir_folder, &folderDirent))==VPL_OK)
        {
            std::string dirent(folderDirent.filename);
            if(folderDirent.type != VPLFS_TYPE_FILE) {
                if(dirent=="." || dirent==".." || dirent==".sync_temp") {
                    // We want to skip these directories
                }else{
                    // save directory for later processing
                    std::string deeperDir = dirPath+"/"+dirent;
                    localDirs.push_back(deeperDir);
                }
                continue;
            }
            allFiles_out++;
            if(isPhotoFile(dirent)) {
                photoFile_out++;
            }
            std::string collectionId;
            u64 timestamp;
            rc = media_metadata::MediaMetadataCache::
                    parseMetadataCollectionFilename(dirent,
                                                    collectionId,
                                                    timestamp);
            if(rc == 0) {  // parse successful
                index_out++;
            }
        }

        rc = VPLFS_Closedir(&dir_folder);
        if(rc != VPL_OK) {
            LOG_ERROR("Closing %s:%d", dirPath.c_str(), rc);
        }
    }

    return rv;
}

int mca_diag(const std::string &test_clip_name,
             std::string &test_url,
             std::string &test_thumb_url,
             bool requireCloudPcOnline,
             bool requireClientOnline,
             bool using_remote_agent)
{
    int rc;
    int rv = 0;

    VPL_Init();

    int retry = 0;
    int retryTimeout = 30;
    ccd::ListLinkedDevicesOutput lldOutput;
    rc = mca_get_media_servers(lldOutput, retryTimeout, requireCloudPcOnline);
    if (rc != 0) {
        LOG_ERROR("mca_get_media_servers failed in %d retry.", retryTimeout);
        return rc;
    }

    u64 userId;
    u64 dummyDatasetId;
    rc = getUserIdAndClearFiDatasetId(/*out*/ userId, /*out*/ dummyDatasetId);
    if(rc != 0) {
        LOG_ERROR("getUserIdAndClearFiDatasetId:%d", rc);
        return rc;
    }
    // Waiting for metadata to be ready
    while (1) {
        int objCnt = 0;
        rc = listMetadataContentHelper(lldOutput.devices(0).device_id(), test_clip_name, test_url, test_thumb_url, false, /*out*/objCnt);
        if (rc != 0) {
            LOG_ERROR("listMetadataContentHelper:%d", rc);
            return rc;
        }

        if (objCnt != 0) {
            int waitSyncCnt = 0;
            LOG_ALWAYS("Start receiving metadata object...");

            while (1) {
                // Get sync state
                ccd::GetSyncStateInput request;
                ccd::GetSyncStateOutput response;
                request.set_user_id(userId);
                request.set_only_use_cache(true);
                request.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PHOTO_METADATA);
                request.add_get_sync_states_for_features(ccd::SYNC_FEATURE_MUSIC_METADATA);
                request.add_get_sync_states_for_features(ccd::SYNC_FEATURE_VIDEO_METADATA);
                request.add_get_sync_states_for_features(ccd::SYNC_FEATURE_PHOTO_THUMBNAILS);
                request.add_get_sync_states_for_features(ccd::SYNC_FEATURE_MUSIC_THUMBNAILS);
                request.add_get_sync_states_for_features(ccd::SYNC_FEATURE_VIDEO_THUMBNAILS);
                const int numFeaturesToCheck = request.get_sync_states_for_features_size();
                rc = CCDIGetSyncState(request, response);
                if (rc != 0) {
                    LOG_ERROR("CCDIGetSyncState:%d", rc);
                    return rc;
                }
                if (response.feature_sync_state_summary_size() != numFeaturesToCheck) {
                    LOG_ERROR("Missing feature state");
                    return -5;
                }
                bool isInSync = true;
                for (int i = 0; i < numFeaturesToCheck; ++i) {
                    if (response.feature_sync_state_summary(i).status() != ccd::CCD_FEATURE_STATE_IN_SYNC) {
                        isInSync = false;
                        break;
                    }
                }

                // We may never by in sync for the offline case.
                if (isInSync || !requireClientOnline) {
                    if (isInSync) {
                        LOG_INFO("Metadata fully in sync, list the updated metadata");
                    } else {
                        LOG_INFO("Metadata not fully in sync yet, but list the metadata anyways for offline testing");
                    }

                    // List one more time to get the list after fully in sync.
                    rc = listMetadataContentHelper(lldOutput.devices(0).device_id(), test_clip_name, test_url, test_thumb_url, false, /*out*/objCnt);
                    if (rc != 0) {
                        LOG_ERROR("listMetadataContentHelper:%d", rc);
                        return rc;
                    } else  {
                        LOG_ALWAYS("Final total object count %d", objCnt);
                        {
                            std::string objectId;
                            rv = mca_get_photo_object_id(PHOTO_ALBUM0_PHOTO0_NEW_TITLE, objectId);
                            if(rv != 0){
                                LOG_ERROR("Can not find photo title: %s", PHOTO_ALBUM0_PHOTO0_NEW_TITLE); 
                                return rv;
                            }
                        }
                        {
                            std::string objectId;
                            rv = mca_get_music_object_id(MUSIC_ALBUM0_TRACK0_NEW_TITLE, objectId);
                            if(rv != 0){
                                LOG_ERROR("Can not find music title: %s", MUSIC_ALBUM0_TRACK0_NEW_TITLE); 
                                return rv;
                            }
                        }

                        return 0;
                    }
                } else {
                    LOG_INFO("Metadata not in-sync yet: %s", response.ShortDebugString().c_str());
                    if (waitSyncCnt++ < 300) {  
                        VPLThread_Sleep(VPLTime_FromSec(1));
                    } else {
                        LOG_ERROR("Time out waiting for metadata to be in sync");
                        return -1;
                    }
                }
            }
        }

        if (retry++ < 300) {
            LOG_INFO("No object found yet. waiting.. (%d)", retry);
            VPLThread_Sleep(VPLTime_FromSec(1));
        } else {
            LOG_ERROR("Time out waiting for metadata object"); 
            rv = -1;
            break;
        }
    }

    return rv;
}

struct SummaryCollectionStats
{
    SummaryCollectionStats();
    u32 musicAlbums;
    u32 musicTracks;
    u32 photos;
    u32 videos;
};

SummaryCollectionStats::SummaryCollectionStats()
:  musicAlbums(0),
   musicTracks(0),
   photos(0),
   videos(0)
{}

static int listMetadataContentHelper(u64 cloudPcId,
                                     const std::string& test_clip_name,
                                     std::string& test_url,
                                     std::string& test_thumb_url,
                                     bool summaryMode,
                                     int& objCnt_out)
{
    int rc;
    int rv = 0;
    std::map<std::string, SummaryCollectionStats> collectionSummaries;
    int statMusicAlbums = 0;
    int statMusicTracks = 0;
    int statPhotoAlbums = 0;
    int statPhotos = 0;
    int statVideoAlbums = 0;
    int statVideos = 0;

    objCnt_out = 0;
    LOG_INFO(">>>> List all Metadata Content items for "FMTu64, cloudPcId);
    {
        ccd::MCAQueryMetadataObjectsInput getMusicAlbumsInput;
        ccd::MCAQueryMetadataObjectsOutput getMusicAlbumsOutput;
        getMusicAlbumsInput.set_cloud_device_id(cloudPcId);
        getMusicAlbumsInput.set_filter_field(media_metadata::MCA_MDQUERY_MUSICALBUM);
        getMusicAlbumsInput.set_sort_field("album_name");
        rc = CCDIMCAQueryMetadataObjects(getMusicAlbumsInput, getMusicAlbumsOutput);
        if(rc != CCD_OK) {
            LOG_ERROR("CCDIMCAQueryMetadataObjects:%d", rc);
            rv = rc;
        }

        for(int musAlbumIdx=0;
            musAlbumIdx < getMusicAlbumsOutput.content_objects_size();
            ++musAlbumIdx)
        {
            const media_metadata::ContentDirectoryObject& mAlbumCdo =
                    getMusicAlbumsOutput.content_objects(musAlbumIdx).cdo();
            std::string thumbnail_url;

            statMusicAlbums++;

            if (getMusicAlbumsOutput.content_objects(musAlbumIdx).has_thumbnail_url()) {
                thumbnail_url = getMusicAlbumsOutput.content_objects(musAlbumIdx).thumbnail_url();
            } else {
                thumbnail_url = "none";
            }
            if(!summaryMode) {
                LOG_INFO("Music Album #%d:%s: name(%s) artist(%s) albumart_url(%s)",
                         musAlbumIdx,
                         mAlbumCdo.object_id().c_str(),
                         mAlbumCdo.music_album().album_name().c_str(),
                         mAlbumCdo.music_album().album_artist().c_str(),
                         thumbnail_url.c_str());
            }else{
                const std::string& cid = getMusicAlbumsOutput.
                        content_objects(musAlbumIdx).collection_id();
                collectionSummaries[cid].musicAlbums++;
            }

            ccd::MCAQueryMetadataObjectsInput getTracksInput;
            ccd::MCAQueryMetadataObjectsOutput getTracksOutput;
            getTracksInput.set_cloud_device_id(cloudPcId);
            getTracksInput.set_filter_field(media_metadata::MCA_MDQUERY_MUSICTRACK);
            getTracksInput.set_sort_field("title");
            std::stringstream searchField;
            searchField << "album_id_ref='" <<
                           getMusicAlbumsOutput.content_objects(musAlbumIdx).cdo().object_id().c_str() <<
                           "'";
            getTracksInput.set_search_field(searchField.str());

            rc = CCDIMCAQueryMetadataObjects(getTracksInput, getTracksOutput);
            if(rc != CCD_OK) {
                LOG_ERROR("CCDIMCAQueryMetadataObjects %d:%s",
                          rc, mAlbumCdo.music_album().album_name().c_str());
                rv = rc;
                continue;
            }
            for(int musTrackIdx=0;
                musTrackIdx < getTracksOutput.content_objects_size();
                ++musTrackIdx)
            {
                const media_metadata::ContentDirectoryObject& mTrackCdo =
                        getTracksOutput.content_objects(musTrackIdx).cdo();
                if(!summaryMode) {
                    LOG_INFO("Music Track #%d:%s: title(%s) track(%d) album(%s) collection_id(%s)",
                             musTrackIdx,
                             mTrackCdo.object_id().c_str(),
                             mTrackCdo.music_track().title().c_str(),
                             (mTrackCdo.music_track().has_track_number())?
                                     mTrackCdo.music_track().track_number() : -1,
                             (mTrackCdo.music_album().has_album_name())?
                                     mTrackCdo.music_album().album_name().c_str(): "NONE",
                             (getTracksOutput.content_objects(musTrackIdx).has_collection_id())?
                                     getTracksOutput.content_objects(musTrackIdx).collection_id().c_str():"NONE");
                }else{
                    const std::string& cid = getTracksOutput.
                            content_objects(musTrackIdx).collection_id();
                    collectionSummaries[cid].musicTracks++;
                }
                statMusicTracks++;
            }
        }
    }

    {
        ccd::MCAQueryMetadataObjectsInput getPhotoAlbumsInput;
        ccd::MCAQueryMetadataObjectsOutput getPhotoAlbumsOutput;
        getPhotoAlbumsInput.set_cloud_device_id(cloudPcId);
        getPhotoAlbumsInput.set_filter_field(media_metadata::MCA_MDQUERY_PHOTOALBUM);
        rc = CCDIMCAQueryMetadataObjects(getPhotoAlbumsInput, getPhotoAlbumsOutput);
        if(rc != 0) {
            LOG_ERROR("CCDIMCAQueryMetadataObjects:%d", rc);
            rv = rc;
        }

        for(int phoAlbumIdx=0;
            phoAlbumIdx < getPhotoAlbumsOutput.content_objects_size();
            ++phoAlbumIdx)
        {
            const media_metadata::ContentDirectoryObject& pAlbumCdo =
                    getPhotoAlbumsOutput.content_objects(phoAlbumIdx).cdo();
            statPhotoAlbums++;
            if(!summaryMode) {
                std::string thumbnail_url;

                if (getPhotoAlbumsOutput.content_objects(phoAlbumIdx).has_thumbnail_url()) {
                    thumbnail_url = getPhotoAlbumsOutput.content_objects(phoAlbumIdx).thumbnail_url();
#if 0
                    if(test_thumb_url.empty()){
                        test_thumb_url = thumbnail_url;
                    }
#endif
                } else {
                    thumbnail_url = "none";
                }

                LOG_INFO("Photo Album #%d:objectId(%s): name(%s) thumbnail(%s) url(%s) item_count("FMTu32") item_total_size("FMTu64")",
                         phoAlbumIdx,
                         pAlbumCdo.object_id().c_str(),
                         pAlbumCdo.photo_album().album_name().c_str(),
                         pAlbumCdo.photo_album().album_thumbnail().c_str(),
                         thumbnail_url.c_str(),
                         pAlbumCdo.photo_album().item_count(),
                         pAlbumCdo.photo_album().item_total_size());
            }

            ccd::MCAQueryMetadataObjectsInput getPhotosInput;
            ccd::MCAQueryMetadataObjectsOutput getPhotosOutput;
            getPhotosInput.set_cloud_device_id(cloudPcId);
            getPhotosInput.set_filter_field(media_metadata::MCA_MDQUERY_PHOTOITEM);
            getPhotosInput.set_sort_field("title");
            std::stringstream searchField;
            searchField << "album_id_ref='" <<
                           getPhotoAlbumsOutput.content_objects(phoAlbumIdx).cdo().object_id().c_str() <<
                           "'";
            getPhotosInput.set_search_field(searchField.str());

            rc = CCDIMCAQueryMetadataObjects(getPhotosInput, getPhotosOutput);
            if(rc != 0) {
                LOG_ERROR("CCDIMCAQueryMetadataObjects %d:%s",
                          rc, pAlbumCdo.photo_album().album_name().c_str());
                rv = rc;
                continue;
            }
            for(int phoItemIdx=0;
                phoItemIdx < getPhotosOutput.content_objects_size();
                ++phoItemIdx)
            {
                const media_metadata::ContentDirectoryObject& pTrackCdo =
                        getPhotosOutput.content_objects(phoItemIdx).cdo();
                std::string url, thumbnail_url;

                statPhotos++;

                if (getPhotosOutput.content_objects(phoItemIdx).has_url()) {
                    url = getPhotosOutput.content_objects(phoItemIdx).url();
                    if(test_url.empty()){
                        test_url = url;
                    }
                } else {
                    url = "none";
                }

                if (getPhotosOutput.content_objects(phoItemIdx).has_thumbnail_url()) {
                    thumbnail_url = getPhotosOutput.content_objects(phoItemIdx).thumbnail_url();
                    if(test_thumb_url.empty()){
                        test_thumb_url = thumbnail_url;
                    }
                } else {
                    thumbnail_url = "none";
                }

                if(!summaryMode) {
                    LOG_INFO("Photo Item #%d:album_id_ref(%s) objId(%s): abs_path(%s) title(%s) thumbnail(%s) "
                             "url(%s) thumbnail_url(%s) time(%"PRIu64") orientation("FMTu32")",
                             phoItemIdx,
                             pTrackCdo.photo_item().album_ref().c_str(),
                             pTrackCdo.object_id().c_str(),
                             pTrackCdo.photo_item().absolute_path().c_str(),
                             pTrackCdo.photo_item().title().c_str(),
                             (pTrackCdo.photo_item().has_thumbnail())?
                                    pTrackCdo.photo_item().thumbnail().c_str():"none",
                             url.c_str(),
                             thumbnail_url.c_str(),
                             pTrackCdo.photo_item().date_time(),
                             pTrackCdo.photo_item().orientation());
                }else{
                    const std::string& cid = getPhotosOutput.
                            content_objects(phoItemIdx).collection_id();
                    collectionSummaries[cid].photos++;
                }


            }
        }

        LOG_INFO("Setting test_url (%s)  test_thumb_url (%s)", test_url.c_str(), test_thumb_url.c_str());
    }
    {
        ccd::MCAQueryMetadataObjectsInput getVideoAlbumsInput;
        ccd::MCAQueryMetadataObjectsOutput getVideoAlbumsOutput;
        getVideoAlbumsInput.set_cloud_device_id(cloudPcId);
        getVideoAlbumsInput.set_filter_field(media_metadata::MCA_MDQUERY_VIDEOALBUM);
        rc = CCDIMCAQueryMetadataObjects(getVideoAlbumsInput, getVideoAlbumsOutput);
        if(rc != 0) {
            LOG_ERROR("CCDIMCAQueryMetadataObjects:%d", rc);
            rv = rc;
        }

        for(int vidAlbumIdx=0;
            vidAlbumIdx < getVideoAlbumsOutput.content_objects_size();
            ++vidAlbumIdx)
        {
            const media_metadata::MCAMetadataQueryObject& pAlbumCdo =
                    getVideoAlbumsOutput.content_objects(vidAlbumIdx);
            statVideoAlbums++;
            if(!summaryMode) {
                LOG_INFO("Video Album #%d:%s: name(%s) item_count("FMTu32") item_total_size("FMTu64")",
                         vidAlbumIdx,
                         pAlbumCdo.video_album().collection_id_ref().c_str(),
                         pAlbumCdo.video_album().album_name().c_str(),
                         pAlbumCdo.video_album().item_count(),
                         pAlbumCdo.video_album().item_total_size());
            }

            ccd::MCAQueryMetadataObjectsInput getVideosInput;
            ccd::MCAQueryMetadataObjectsOutput getVideosOutput;
            getVideosInput.set_cloud_device_id(cloudPcId);
            getVideosInput.set_filter_field(media_metadata::MCA_MDQUERY_VIDEOITEM);
            getVideosInput.set_sort_field("title");
            std::stringstream searchField;
            searchField << "album_name='" <<
                           pAlbumCdo.video_album().album_name().c_str() <<
                           "'";
            getVideosInput.set_search_field(searchField.str());

            rc = CCDIMCAQueryMetadataObjects(getVideosInput, getVideosOutput);
            if(rc != 0) {
                LOG_ERROR("CCDIMCAQueryMetadataObjects %d:%s",
                          rc, pAlbumCdo.video_album().album_name().c_str());
                rv = rc;
                continue;
            }
            for(int vidItemIdx=0;
                vidItemIdx < getVideosOutput.content_objects_size();
                ++vidItemIdx)
            {
                const media_metadata::ContentDirectoryObject& pTrackCdo =
                        getVideosOutput.content_objects(vidItemIdx).cdo();
                std::string url, thumbnail_url;

                statVideos++;

                if (getVideosOutput.content_objects(vidItemIdx).has_url()) {
                    url = getVideosOutput.content_objects(vidItemIdx).url();
                } else {
                    url = "none";
                }

                if (getVideosOutput.content_objects(vidItemIdx).has_thumbnail_url()) {
                    thumbnail_url = getVideosOutput.content_objects(vidItemIdx).thumbnail_url();
                } else {
                    thumbnail_url = "none";
                }

                if(!summaryMode) {
                    LOG_INFO("Video Item #%d:%s: abs_path(%s) title(%s) thumbnail(%s) url(%s) thumbnail_url(%s) time(%"PRIu64")",
                             vidItemIdx,
                             pTrackCdo.object_id().c_str(),
                             pTrackCdo.video_item().absolute_path().c_str(),
                             pTrackCdo.video_item().title().c_str(),
                             (pTrackCdo.video_item().has_thumbnail())?
                                    pTrackCdo.video_item().thumbnail().c_str():"none",
                             url.c_str(),
                             thumbnail_url.c_str(),
                             pTrackCdo.video_item().date_time());
                }else{
                    const std::string& cid = getVideosOutput.
                            content_objects(vidItemIdx).collection_id();
                    collectionSummaries[cid].videos++;
                }
            }
        }
    }

    if(summaryMode) {
        std::map<std::string, SummaryCollectionStats>::iterator iter =
                collectionSummaries.begin();
        for(;iter!=collectionSummaries.end();++iter) {
            LOG_INFO("CollectionId(%s), NumPhotos(%d), NumMusicTracks(%d), NumMusicAlbums(%d), NumVideos(%d)",
                     iter->first.c_str(),
                     iter->second.photos,
                     iter->second.musicTracks, iter->second.musicAlbums,
                     iter->second.videos);
        }
        LOG_INFO("Total Collections:%d", collectionSummaries.size());
    }

    LOG_INFO("Totals:  "
             "MusicAlbums:%d, MusicTracks:%d, "
             "PhotoAlbums:%d, Photos:%d, "
             "VideoAlbums:%d, Videos:%d",
             statMusicAlbums, statMusicTracks,
             statPhotoAlbums, statPhotos,
             statVideoAlbums, statVideos);
    if(test_clip_name != "") {
        LOG_INFO("   clip_name:%s, test_url:%s test_thumb_url:%s",
                 test_clip_name.c_str(), test_url.c_str(), test_thumb_url.c_str());
    }

    objCnt_out = statPhotos + statMusicTracks + statVideos + statMusicAlbums;
    return rv;
}

static int dumpMetadataContentHelper(u64 cloudPcId,
                                     const std::string& test_clip_name,
                                     std::string& test_url,
                                     std::string& test_thumb_url,
                                     const std::string& dumpfile_name_folder)
{
    int rc = 0;
    int rv = 0;
    int statMusicAlbums = 0;
    int statMusicTracks = 0;
    int statPhotoAlbums = 0;
    int statPhotos = 0;
    int statVideoAlbums = 0;
    int statVideos = 0;

    // Dump Tracks
    do
    {
        ccd::MCAQueryMetadataObjectsInput getMusicAlbumsInput;
        ccd::MCAQueryMetadataObjectsOutput getMusicAlbumsOutput;
        getMusicAlbumsInput.set_cloud_device_id(cloudPcId);
        getMusicAlbumsInput.set_filter_field(media_metadata::MCA_MDQUERY_MUSICALBUM);
        getMusicAlbumsInput.set_sort_field("album_name");
        rc = CCDIMCAQueryMetadataObjects(getMusicAlbumsInput, getMusicAlbumsOutput);
        if (rc != CCD_OK)
        {
            LOG_ERROR("CCDIMCAQueryMetadataObjects: %d", rc);
            rv = rc;
            break;
        }

        for (int musicAlbumIdx = 0; musicAlbumIdx < getMusicAlbumsOutput.content_objects_size(); ++musicAlbumIdx)
        {
            const media_metadata::ContentDirectoryObject& mAlbumCdo = getMusicAlbumsOutput.content_objects(musicAlbumIdx).cdo();
            std::string thumbnail_url = std::string();
            ++statMusicAlbums;

            if (getMusicAlbumsOutput.content_objects(musicAlbumIdx).has_thumbnail_url())
            {
                thumbnail_url = getMusicAlbumsOutput.content_objects(musicAlbumIdx).thumbnail_url();
            }

            std::stringstream ss;
            ss << dumpfile_name_folder << "/" << mAlbumCdo.object_id() << ".list";

            std::ofstream ofDump(ss.str().c_str());
            ss.str("");
            ss.clear();

            ofDump    << "# AlbumIndex: " << musicAlbumIdx << ", Oid: " << mAlbumCdo.object_id() << ", Name: " << mAlbumCdo.music_album().album_name()
                    << ", Artist: " <<  mAlbumCdo.music_album().album_artist() << ", ThumbnailUrl: " << thumbnail_url << std::endl;
            ofDump << "# musicTrackIdx,musicTrackOid,musicTrackAbsPath,contenturl,musicTrackTitle,TrackNumber,AlbumName,CollectionId" << std::endl;

            ccd::MCAQueryMetadataObjectsInput getTracksInput;
            ccd::MCAQueryMetadataObjectsOutput getTracksOutput;
            getTracksInput.set_cloud_device_id(cloudPcId);
            getTracksInput.set_filter_field(media_metadata::MCA_MDQUERY_MUSICTRACK);
            getTracksInput.set_sort_field("title");
            std::stringstream searchField;
            searchField << "album_id_ref='" << mAlbumCdo.object_id() << "'";
            getTracksInput.set_search_field(searchField.str());
            searchField.str("");
            searchField.clear();
            rc = CCDIMCAQueryMetadataObjects(getTracksInput, getTracksOutput);
            if (rc != CCD_OK)
            {
                LOG_ERROR("CCDIMCAQueryMetadataObjects: %d", rc);
                rv = rc;
                continue;
            }

            for (int musTrackIdx = 0; musTrackIdx < getTracksOutput.content_objects_size(); ++musTrackIdx)
            {
                const media_metadata::ContentDirectoryObject& mTrackCdo = getTracksOutput.content_objects(musTrackIdx).cdo();
                ofDump    << musTrackIdx << ",\"" << mTrackCdo.object_id() << "\",\"" << mTrackCdo.music_track().absolute_path() << "\"," << getTracksOutput.content_objects(musTrackIdx).url() << ",\""
                        << mTrackCdo.music_track().title() << "\","
                        << (mTrackCdo.music_track().has_track_number() ? mTrackCdo.music_track().track_number() : -1) << ",\""
                        << (mTrackCdo.music_album().has_album_name() ? mTrackCdo.music_album().album_name() : "") << "\","
                        << (getTracksOutput.content_objects(musTrackIdx).has_collection_id() ? getTracksOutput.content_objects(musTrackIdx).collection_id() : "" ) << std::endl;
                ++statMusicTracks;
            }

            ofDump.close();
        }

    } while (false);

    // Dump Photo
    do
    {
        ccd::MCAQueryMetadataObjectsInput getPhotoAlbumsInput;
        ccd::MCAQueryMetadataObjectsOutput getPhotoAlbumsOutput;
        getPhotoAlbumsInput.set_cloud_device_id(cloudPcId);
        getPhotoAlbumsInput.set_filter_field(media_metadata::MCA_MDQUERY_PHOTOALBUM);
        rc = CCDIMCAQueryMetadataObjects(getPhotoAlbumsInput, getPhotoAlbumsOutput);
        if (rc != CCD_OK)
        {
            LOG_ERROR("CCDIMCAQueryMetadataObjects: %d", rc);
            rv = rc;
            break;
        }

        for (int photoAlbumIdx = 0; photoAlbumIdx < getPhotoAlbumsOutput.content_objects_size(); ++photoAlbumIdx)
        {
            const media_metadata::ContentDirectoryObject& pAlbumCdo = getPhotoAlbumsOutput.content_objects(photoAlbumIdx).cdo();
            ++statPhotoAlbums;

            std::stringstream ss;
            ss << dumpfile_name_folder << "/" << pAlbumCdo.object_id() << ".list";

            std::ofstream ofDump(ss.str().c_str());
            ss.str("");
            ss.clear();

            ofDump    << "# AlbumIdx: " << photoAlbumIdx << ", Oid: " << pAlbumCdo.object_id() << ", name: " << pAlbumCdo.photo_album().album_name()
                << ". itemCount: " << pAlbumCdo.photo_album().item_count() << ", item_total_size: " << pAlbumCdo.photo_album().item_total_size() << std::endl;
            ofDump << "# photoItemIdx,photoItemOid,photoItemAbsolute_path,contenturl,photoItemTitle,photoItemThumbnailAbsolute_path,thumbnail_url,datetime" << std::endl;

            ccd::MCAQueryMetadataObjectsInput getPhotosInput;
            ccd::MCAQueryMetadataObjectsOutput getPhotosOutput;
            getPhotosInput.set_cloud_device_id(cloudPcId);
            getPhotosInput.set_filter_field(media_metadata::MCA_MDQUERY_PHOTOITEM);
            getPhotosInput.set_sort_field("title");
            std::stringstream searchField;
            searchField << "album_id_ref='" << pAlbumCdo.object_id() << "'";
            getPhotosInput.set_search_field(searchField.str());

            rc = CCDIMCAQueryMetadataObjects(getPhotosInput, getPhotosOutput);
            if (rc != CCD_OK)
            {
                LOG_ERROR("CCDIMCAQueryMetadataObjects: %d", rc);
                rv = rc;
                continue;
            }

            for (int photoItemIdx = 0; photoItemIdx < getPhotosOutput.content_objects_size(); ++photoItemIdx)
            {
                const media_metadata::ContentDirectoryObject& photoItemCdo = getPhotosOutput.content_objects(photoItemIdx).cdo();

                std::string url = std::string(), thumbnail_url = std::string();
                statPhotos++;

                if (getPhotosOutput.content_objects(photoItemIdx).has_url())
                {
                    url = getPhotosOutput.content_objects(photoItemIdx).url();
                }

                if (getPhotosOutput.content_objects(photoItemIdx).has_thumbnail_url())
                {
                    thumbnail_url = getPhotosOutput.content_objects(photoItemIdx).thumbnail_url();
                }

                ofDump    << photoItemIdx << ",\"" << photoItemCdo.object_id() << "\",\"" << photoItemCdo.photo_item().absolute_path() << "\"," << url << ",\"" << photoItemCdo.photo_item().title() << "\",\""
                    << (photoItemCdo.photo_item().has_thumbnail() ? photoItemCdo.photo_item().thumbnail() : "") << "\","
                    << thumbnail_url << "," << photoItemCdo.photo_item().date_time() << std::endl;
            }

            ofDump.close();
        }

    } while (false);

    // Dump Video
    do
    {
        ccd::MCAQueryMetadataObjectsInput getVideoAlbumsInput;
        ccd::MCAQueryMetadataObjectsOutput getVideoAlbumsOutput;
        getVideoAlbumsInput.set_cloud_device_id(cloudPcId);
        getVideoAlbumsInput.set_filter_field(media_metadata::MCA_MDQUERY_VIDEOALBUM);
        rc = CCDIMCAQueryMetadataObjects(getVideoAlbumsInput, getVideoAlbumsOutput);
        if(rc != 0) {
            LOG_ERROR("CCDIMCAQueryMetadataObjects:%d", rc);
            rv = rc;
        }

        for(int vidAlbumIdx=0;
            vidAlbumIdx < getVideoAlbumsOutput.content_objects_size();
            ++vidAlbumIdx)
        {
            const media_metadata::MCAMetadataQueryObject& pAlbumCdo =
                    getVideoAlbumsOutput.content_objects(vidAlbumIdx);
            statVideoAlbums++;

            std::stringstream ss;

            ss << dumpfile_name_folder << "/" << pAlbumCdo.video_album().collection_id_ref() << ".list";

            std::ofstream ofDump(ss.str().c_str());
            ss.str("");
            ss.clear();

            ofDump    << "# AlbumIdx: " << vidAlbumIdx << ", collectionIdRef: " << pAlbumCdo.video_album().collection_id_ref() << ", name: " << pAlbumCdo.video_album().album_name()
                << ". itemCount: " << pAlbumCdo.video_album().item_count() << ", item_total_size: " << pAlbumCdo.video_album().item_total_size() << std::endl;
            ofDump << "# photoItemIdx,photoItemOid,photoItemAbsolute_path,contenturl,photoItemTitle,photoItemThumbnailAbsolute_path,thumbnail_url,datetime" << std::endl;

            ccd::MCAQueryMetadataObjectsInput getVideosInput;
            ccd::MCAQueryMetadataObjectsOutput getVideosOutput;
            getVideosInput.set_cloud_device_id(cloudPcId);
            getVideosInput.set_filter_field(media_metadata::MCA_MDQUERY_VIDEOITEM);
            getVideosInput.set_sort_field("title");
            std::stringstream searchField;
            searchField << "album_name='" <<
                           pAlbumCdo.video_album().album_name().c_str() <<
                           "'";
            getVideosInput.set_search_field(searchField.str());

            rc = CCDIMCAQueryMetadataObjects(getVideosInput, getVideosOutput);
            if(rc != 0) {
                LOG_ERROR("CCDIMCAQueryMetadataObjects %d:%s",
                          rc, pAlbumCdo.video_album().album_name().c_str());
                rv = rc;
                continue;
            }
            for(int vidItemIdx=0;
                vidItemIdx < getVideosOutput.content_objects_size();
                ++vidItemIdx)
            {
                const media_metadata::ContentDirectoryObject& pVideoItemCdo =
                        getVideosOutput.content_objects(vidItemIdx).cdo();
                std::string url, thumbnail_url;

                statVideos++;

                if (getVideosOutput.content_objects(vidItemIdx).has_url())
                {
                    url = getVideosOutput.content_objects(vidItemIdx).url();
                }

                if (getVideosOutput.content_objects(vidItemIdx).has_thumbnail_url())
                {
                    thumbnail_url = getVideosOutput.content_objects(vidItemIdx).thumbnail_url();
                }

                ofDump    << vidItemIdx << ",\"" << pVideoItemCdo.object_id() << "\",\"" << pVideoItemCdo.video_item().absolute_path() << "\"," << url << ",\"" << pVideoItemCdo.video_item().title() << "\",\""
                    << (pVideoItemCdo.video_item().has_thumbnail() ? pVideoItemCdo.video_item().thumbnail() : "") << "\","
                    << thumbnail_url << "," << pVideoItemCdo.video_item().date_time() << std::endl;
            }
        }
    } while (false);

    return rv;
}

int mca_get_media_servers(ccd::ListLinkedDevicesOutput& mediaServerDevice_out, int retryTimeout, bool requireMediaServerOnline)
{
    int retry = 0;
    int rc = 0;

    u64 userId;
    rc = getUserId(userId);
    if(rc != 0) {
        LOG_ERROR("getUserId failed:%d", rc);
        return rc;
    }

    while (1) {
        mediaServerDevice_out.Clear();
        {
            ccd::ListLinkedDevicesInput lldIn;
            ccd::ListLinkedDevicesOutput lldOut;
            lldIn.set_user_id(userId);
            lldIn.set_storage_nodes_only(true);
            lldIn.set_only_use_cache(true);

            rc = CCDIListLinkedDevices(lldIn, lldOut);
            if(rc != 0) {
                LOG_WARN("Waiting for CCDIListLinkedDevices %d", rc);
            }else{
                ccd::ListUserStorageInput listUserStorageIn;
                ccd::ListUserStorageOutput listUserStorageOut;
                listUserStorageIn.set_user_id(userId);
                listUserStorageIn.set_only_use_cache(true);
                rc = CCDIListUserStorage(listUserStorageIn, listUserStorageOut);
                if(rc != 0){
                    LOG_WARN("Waiting for CCDIListUserStorage %d", rc);
                }else{
                    for(int lldIter=0; lldIter < lldOut.devices_size(); lldIter++)
                    {
                        if(!lldOut.devices(lldIter).feature_media_server_capable())
                        {   // Skip this device, this is not media server capable.
                            continue;
                        }

                        //check user_storage().featuremediaserverenabled()
                        for(int lusIter=0; lusIter<listUserStorageOut.user_storage_size(); lusIter++){
                            if(!listUserStorageOut.user_storage(lusIter).featuremediaserverenabled())
                            {
                                // Skip this device, this is not media server capable.
                                continue;
                            }
                            if(lldOut.devices(lldIter).device_id() != 
                               listUserStorageOut.user_storage(lusIter).storageclusterid())
                            {
                                continue;
                            }
                            ccd::LinkedDeviceInfo* deviceInfo = mediaServerDevice_out.add_devices();
                            *deviceInfo = lldOut.devices(lldIter);

                            break;
                        }
                    }
                }
            }
        }

        if (mediaServerDevice_out.devices_size() > 0) {
            if (mediaServerDevice_out.devices_size() > 1) {
                LOG_ERROR("Found %d media servers on this account!  There should only be one.", mediaServerDevice_out.devices_size());
                LOG_ERROR("Please unlink the other devices and try the test again.");
                return -2;
            }
            LOG_ALWAYS("Media Server is device "FMTu64" {%s}",
                    mediaServerDevice_out.devices(0).device_id(),
                    mediaServerDevice_out.devices(0).connection_status().ShortDebugString().c_str());
            if (requireMediaServerOnline &&
                    (mediaServerDevice_out.devices(0).connection_status().state() != ccd::DEVICE_CONNECTION_ONLINE))
            {
                LOG_ALWAYS("Waiting for Media Server "FMTu64" to be ONLINE...", mediaServerDevice_out.devices(0).device_id());
            } else {
                break;
            }
        } else {
            LOG_ALWAYS("Waiting for a Media Server to be added to this account...");
        }
        if (++retry >= retryTimeout) {
            if (requireMediaServerOnline) {
                LOG_ERROR("Timeout retry (%d) waiting for Media Server to be ONLINE", retry);
            } else {
                LOG_ERROR("Timeout retry (%d) waiting for Media Server to exist", retry);
            }
            return -1;
        }
        VPLThread_Sleep(VPLTIME_FROM_SEC(1));
    }

    return rc;
}

int mca_list_metadata()
{
    int rc;
    int retryTimeout = 30;
    ccd::ListLinkedDevicesOutput lldOutput;
    rc = mca_get_media_servers(lldOutput, retryTimeout, false);
    if (rc != 0) {
        LOG_ERROR("mca_get_media_servers failed in %d retry.", retryTimeout);
        return rc;
    }

    std::string unusedVar1;
    std::string unusedVar2;
    int         unusedVar3;
    for (int i = 0; i < lldOutput.devices_size(); i++) {
        rc = listMetadataContentHelper(lldOutput.devices(i).device_id(),
                                       "", unusedVar1, unusedVar2, false, unusedVar3);
        if(rc != 0) {
            LOG_ERROR("listMetadataContentHelper:%d", rc);
        }
    }
    return rc;
}

int mca_dump_metadata(const std::string strDumpFileFolder)
{
    int rc;
    int retryTimeout = 30;
    ccd::ListLinkedDevicesOutput lldOutput;
    rc = mca_get_media_servers(lldOutput, retryTimeout, false);
    if (rc != 0) {
        LOG_ERROR("mca_get_media_servers failed in %d retry.", retryTimeout);
        return rc;
    }

    std::string unusedVar1;
    std::string unusedVar2;

    LOG_INFO("DumpFileFolder %s", strDumpFileFolder.c_str());

    if (!strDumpFileFolder.empty()) {

        rc = Util_CreatePath(strDumpFileFolder.c_str(), true);
        if (rc != VPL_OK) {
            LOG_ERROR("Fail to create directory %s to dump list", strDumpFileFolder.c_str());
            rc = -1;
            goto exit;
        }

        for (int i = 0; i < lldOutput.devices_size(); i++) {
            rc = dumpMetadataContentHelper(lldOutput.devices(i).device_id(),
                                           "", unusedVar1, unusedVar2, strDumpFileFolder);

            if (rc != 0) {
                LOG_ERROR("dumpMetadataContentHelper:%d", rc);
            }
        }
    }
exit:
    return rc;
}

int mca_summary_metadata()
{
    int rc;
    int retryTimeout = 30;
    ccd::ListLinkedDevicesOutput lldOutput;
    rc = mca_get_media_servers(lldOutput, retryTimeout, false);
    if (rc != 0) {
        LOG_ERROR("mca_get_media_servers failed in %d retry.", retryTimeout);
        return rc;
    }

    std::string unusedVar1;
    std::string unusedVar2;
    int         unusedVar3;

    for (int i = 0; i < lldOutput.devices_size(); i++) {
        rc = listMetadataContentHelper(lldOutput.devices(i).device_id(),
                                       "", unusedVar1, unusedVar2, true, unusedVar3);

        if (rc != 0)
            {
                LOG_ERROR("summaryMetadataContentHelper:%d", rc);
            }
    }

    return rc;
}

int mca_get_photo_object_id(const std::string &title, std::string &photoObjectId)
{
    int rc = 0;

    int retryTimeout = 30;
    ccd::ListLinkedDevicesOutput lldOutput;
    rc = mca_get_media_servers(lldOutput, retryTimeout, false);
    if (rc != 0) {
        LOG_ERROR("mca_get_media_servers failed in %d retry.", retryTimeout);
        return rc;
    }

    {
        ccd::MCAQueryMetadataObjectsInput getPhotosInput;
        ccd::MCAQueryMetadataObjectsOutput getPhotosOutput;
        getPhotosInput.set_cloud_device_id(lldOutput.devices(0).device_id());
        getPhotosInput.set_filter_field(media_metadata::MCA_MDQUERY_PHOTOITEM);
        std::stringstream searchField;
        searchField << "title='" << title.c_str() << "'";
        getPhotosInput.set_search_field(searchField.str());
        rc = CCDIMCAQueryMetadataObjects(getPhotosInput, getPhotosOutput);
        if(rc != 0) {
            LOG_ERROR("CCDIMCAQueryMetadataObjects(%s):%d", title.c_str(), rc);
            return rc;
        }

        if(getPhotosOutput.content_objects_size() != 1){
            LOG_ERROR("Got more than one object(%s):%d",
                      title.c_str(), getPhotosOutput.content_objects_size());
            return -1;
        }

        const media_metadata::ContentDirectoryObject& pTrackCdo =
                        getPhotosOutput.content_objects(0).cdo();

        photoObjectId = pTrackCdo.object_id();
        LOG_INFO("title: %s, photoObjectId: %s", title.c_str(), photoObjectId.c_str());
    }

    return rc;
}

int mca_get_photo_object_num(u32& photoAlbumNum, u32& photoNum)
{
    int rc = 0;
    int statPhotoAlbums = 0;
    int statPhotos = 0;

    int retryTimeout = 30;
    ccd::ListLinkedDevicesOutput lldOutput;
    rc = mca_get_media_servers(lldOutput, retryTimeout, false);
    if (rc != 0) {
        LOG_ERROR("mca_get_media_servers failed in %d retry.", retryTimeout);
        return rc;
    }

    {
        ccd::MCAQueryMetadataObjectsInput getPhotoAlbumsInput;
        ccd::MCAQueryMetadataObjectsOutput getPhotoAlbumsOutput;
        getPhotoAlbumsInput.set_cloud_device_id(lldOutput.devices(0).device_id());
        getPhotoAlbumsInput.set_filter_field(media_metadata::MCA_MDQUERY_PHOTOALBUM);
        rc = CCDIMCAQueryMetadataObjects(getPhotoAlbumsInput, getPhotoAlbumsOutput);
        if(rc != 0) {
            LOG_ERROR("CCDIMCAQueryMetadataObjects:%d", rc);
            return rc;
        }

        for(int phoAlbumIdx=0;
            phoAlbumIdx < getPhotoAlbumsOutput.content_objects_size();
            ++phoAlbumIdx)
        {
            const media_metadata::ContentDirectoryObject& pAlbumCdo =
                    getPhotoAlbumsOutput.content_objects(phoAlbumIdx).cdo();
            statPhotoAlbums++;
            statPhotos += pAlbumCdo.photo_album().item_count();
        }
    }

    photoAlbumNum = statPhotoAlbums;
    photoNum = statPhotos;

    return rc;
}

int mca_get_music_object_id(const std::string &title, std::string &musicObjectId)
{
    int rc = 0;

    int retryTimeout = 30;
    ccd::ListLinkedDevicesOutput lldOutput;
    rc = mca_get_media_servers(lldOutput, retryTimeout, false);
    if (rc != 0) {
        LOG_ERROR("mca_get_media_servers failed in %d retry.", retryTimeout);
        return rc;
    }

    {
        ccd::MCAQueryMetadataObjectsInput getTracksInput;
        ccd::MCAQueryMetadataObjectsOutput getTracksOutput;
        getTracksInput.set_cloud_device_id(lldOutput.devices(0).device_id());
        getTracksInput.set_filter_field(media_metadata::MCA_MDQUERY_MUSICTRACK);
        std::stringstream searchField;
        searchField << "title='" << title.c_str() << "'";
        getTracksInput.set_search_field(searchField.str());
        rc = CCDIMCAQueryMetadataObjects(getTracksInput, getTracksOutput);
        if(rc != 0) {
            LOG_ERROR("CCDIMCAQueryMetadataObjects(%s):%d", title.c_str(), rc);
            return rc;
        }

        if(getTracksOutput.content_objects_size() != 1){
            LOG_ERROR("Got more than one object for %s:%d",
                      title.c_str(), getTracksOutput.content_objects_size());
            return -1;
        }

        const media_metadata::ContentDirectoryObject& pTrackCdo =
                        getTracksOutput.content_objects(0).cdo();

        musicObjectId = pTrackCdo.object_id();
        LOG_INFO("title: %s, musicObjectId: %s", title.c_str(), musicObjectId.c_str());
    }

    return rc;
}

int mca_get_music_object_num(u32& musicAlbumNum, u32& trackNum)
{
    int rc = 0;
    int statMusicAlbums = 0;
    int statTracks = 0;

    int retryTimeout = 30;
    ccd::ListLinkedDevicesOutput lldOutput;
    rc = mca_get_media_servers(lldOutput, retryTimeout, false);
    if (rc != 0) {
        LOG_ERROR("mca_get_media_servers failed in %d retry.", retryTimeout);
        return rc;
    }

    {
        ccd::MCAQueryMetadataObjectsInput getMusicAlbumsInput;
        ccd::MCAQueryMetadataObjectsOutput getMusicAlbumsOutput;
        getMusicAlbumsInput.set_cloud_device_id(lldOutput.devices(0).device_id());
        getMusicAlbumsInput.set_filter_field(media_metadata::MCA_MDQUERY_MUSICALBUM);
        getMusicAlbumsInput.set_sort_field("album_name");
        rc = CCDIMCAQueryMetadataObjects(getMusicAlbumsInput, getMusicAlbumsOutput);
        if(rc != CCD_OK) {
            LOG_ERROR("CCDIMCAQueryMetadataObjects:%d", rc);
            return rc;
        }

        for(int musAlbumIdx=0;
            musAlbumIdx < getMusicAlbumsOutput.content_objects_size();
            ++musAlbumIdx)
        {
            statMusicAlbums++;
            ccd::MCAQueryMetadataObjectsInput getTracksInput;
            ccd::MCAQueryMetadataObjectsOutput getTracksOutput;
            getTracksInput.set_cloud_device_id(lldOutput.devices(0).device_id());
            getTracksInput.set_filter_field(media_metadata::MCA_MDQUERY_MUSICTRACK);
            getTracksInput.set_sort_field("title");
            std::stringstream searchField;
            searchField << "album_id_ref='" <<
                           getMusicAlbumsOutput.content_objects(musAlbumIdx).cdo().object_id().c_str() <<
                           "'";
            getTracksInput.set_search_field(searchField.str());

            rc = CCDIMCAQueryMetadataObjects(getTracksInput, getTracksOutput);
            if(rc != CCD_OK) {
                continue;
            }
            statTracks += getTracksOutput.content_objects_size();
        }
    }

    musicAlbumNum = statMusicAlbums;
    trackNum = statTracks;

    return rc;
}

int mca_migrate_thumb(const std::string& dstDir)
{
    ccd::UpdateSyncSettingsInput updateSyncSettingsInput;
    ccd::UpdateSyncSettingsOutput updateSyncSettingsOutput;

    u64 userId;
    int rv = 0;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }
    updateSyncSettingsInput.set_user_id(userId);
    ccd::MediaMetadataThumbMigrate * migrate =
            updateSyncSettingsInput.mutable_migrate_mm_thumb_download_path();
    if(!dstDir.empty()) {
        migrate->set_mm_dest_dir(dstDir);
    }
    rv = CCDIUpdateSyncSettings(updateSyncSettingsInput, updateSyncSettingsOutput);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings(%s):%d", dstDir.c_str(), rv);
        return rv;
    }

    rv = updateSyncSettingsOutput.migrate_mm_thumb_download_path_err();
    if(rv != 0) {
        LOG_ERROR("Result:%d",rv);
    }
    return rv;
}

int mca_list_allphotos(int argc, const char* argv[])
{
    if (checkHelp(argc, argv)) {
        std::cout << MCA_STR << " ListAllPhotos " << 
               "List all photos. Only one item is shown for all duplications." << std::endl;
        return 0;
    }

    int rv = 0, rc;
    setDebugLevel(LOG_LEVEL_INFO);

    int retryTimeout = 30;
    ccd::ListLinkedDevicesOutput lldOutput;
    rc = mca_get_media_servers(lldOutput, retryTimeout, false);
    if (rc != 0) {
        LOG_ERROR("mca_get_media_servers failed in %d retry.", retryTimeout);
        rv = rc;
        goto end;
    }


    for (int i = 0; i < lldOutput.devices_size(); i++) {
        u64 cloudPcId = lldOutput.devices(i).device_id();

        LOG_INFO(">>>> List all photos for CloudPC: "FMTu64, cloudPcId);

        int statPhotos = 0;
        ccd::MCAQueryMetadataObjectsInput getPhotosInput;
        ccd::MCAQueryMetadataObjectsOutput getPhotosOutput;
        getPhotosInput.set_cloud_device_id(cloudPcId);
        getPhotosInput.set_filter_field(media_metadata::MCA_MDQUERY_PHOTOITEM);
        getPhotosInput.set_sort_field("title");
        std::stringstream searchField;

        rc = CCDIMCAQueryMetadataObjects(getPhotosInput, getPhotosOutput);
        if(rc != 0) {
            LOG_ERROR("CCDIMCAQueryMetadataObjects %d",rc);
            rv = rc;
            goto end;
        }
        for(int phoItemIdx=0;
            phoItemIdx < getPhotosOutput.content_objects_size();
            ++phoItemIdx)
        {
            const media_metadata::ContentDirectoryObject& pTrackCdo =
                    getPhotosOutput.content_objects(phoItemIdx).cdo();
            std::string url, thumbnail_url;

            statPhotos++;

            if (getPhotosOutput.content_objects(phoItemIdx).has_url()) {
                url = getPhotosOutput.content_objects(phoItemIdx).url();
            } else {
                url = "none";
            }

            if (getPhotosOutput.content_objects(phoItemIdx).has_thumbnail_url()) {
                thumbnail_url = getPhotosOutput.content_objects(phoItemIdx).thumbnail_url();
            } else {
                thumbnail_url = "none";
            }

            LOG_INFO("Photo Item #%d:album_ref(%s) objId(%s): abs_path(%s) title(%s) thumbnail(%s) "
                     "url(%s) thumbnail_url(%s) time(%"PRIu64") orientation("FMTu32")",
                     phoItemIdx,
                     pTrackCdo.photo_item().album_ref().c_str(),
                     pTrackCdo.object_id().c_str(),
                     pTrackCdo.photo_item().absolute_path().c_str(),
                     pTrackCdo.photo_item().title().c_str(),
                     (pTrackCdo.photo_item().has_thumbnail())?
                            pTrackCdo.photo_item().thumbnail().c_str():"none",
                     url.c_str(),
                     thumbnail_url.c_str(),
                     pTrackCdo.photo_item().date_time(),
                     pTrackCdo.photo_item().orientation());

        }

    }
    rv = rc;
end:
    resetDebugLevel();
    return rv;
}


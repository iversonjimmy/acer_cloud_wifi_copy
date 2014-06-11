#include <vpl_plat.h>
#include "setup_stream_test.hpp"
#include "dx_common.h"
#include "ccd_utils.hpp"
#include "common_utils.hpp"

#include <ccdi.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include "gvm_file_utils.hpp"

#include <vpl_fs.h>
#include <vpl_th.h>
#include <log.h>

int setup_stream_test_create_metadata(u64 userId,
                                      u64 deviceId,
                                      media_metadata::CatalogType_t catType,
                                      const std::string &collectionId,
                                      const std::string &testFilesFolder)
{
    int rv = 0;

    {
        ccd::BeginCatalogInput req;
        req.set_catalog_type(catType);
        rv = CCDIMSABeginCatalog(req);
        if (rv != 0) {
            LOG_ERROR("MSABeginCatalog failed: %d", rv);
            goto end;
        }
    }

    {
        ccd::BeginMetadataTransactionInput req;
        req.set_collection_id(collectionId);
        req.set_collection_timestamp(VPLTime_GetTime());
        req.set_reset_collection(false);
        rv = CCDIMSABeginMetadataTransaction(req);
        if (rv != 0) {
            LOG_ERROR("MSABeginMetadataTransaction failed: %d", rv);
            goto end;
        }
    }

    {
        VPLFS_dir_t dir;
        VPLFS_dirent_t dirent;
        rv = VPLFS_Opendir(testFilesFolder.c_str(), &dir);
        if (rv != 0) {
            LOG_ERROR("VPLFS_OpenDir failed: %d", rv);
            goto end;
        }
        while (VPLFS_Readdir(&dir, &dirent) == 0) {
            if (dirent.type != VPLFS_TYPE_FILE) continue;
            ccd::UpdateMetadataInput req;
            media_metadata::ContentDirectoryObject *o = req.mutable_metadata();
            media_metadata::VideoItemFields *v = o->mutable_video_item();

            std::string filepath = testFilesFolder + "/" + dirent.filename;

            o->set_object_id(dirent.filename);
            o->set_source(media_metadata::MEDIA_SOURCE_LIBRARY);
            v->set_absolute_path(filepath);
            v->set_title(dirent.filename);
            v->set_album_name("SampleVideos");
            v->set_file_format(get_extension(filepath));

            rv = CCDIMSAUpdateMetadata(req);
            if (rv != 0) {
                LOG_ERROR("MSAUpdateMetadata failed: %d", rv);
                goto end;
            }
            LOG_INFO("Added %s", filepath.c_str());
        }
        VPLFS_Closedir(&dir);
    }

    rv = CCDIMSACommitMetadataTransaction();
    if (rv != 0) {
        LOG_ERROR("MSACommitMetadataTransaction failed: %d", rv);
        goto end;
    }
    LOG_INFO("Committed metadata");

    {
        ccd::CommitCatalogInput ccInput;
        ccInput.set_catalog_type(catType);
        rv = CCDIMSACommitCatalog(ccInput);
        if (rv != 0) {
            LOG_ERROR("MSACommitCatalog failed: %d", rv);
            goto end;
        }
        LOG_INFO("Committed catalog");
    }

    LOG_INFO("Logged out of MSA");

end:
    return rv;
}

int setup_stream_test_write_dumpfile(u64 userId, u64 deviceId, 
                                     const std::string &collectionid,
                                     const std::string &dumpfile,
                                     bool downloadMusic,
                                     bool downloadPhoto,
                                     bool downloadVideo,
                                     s32 dumpCountMax)
{
    int rv = 0;
    u64 psnDeviceId = 0;
    u64 userId_out = 0;
    u64 datasetId_out = 0;
    s32 dumpCount = 0;

    {
        ccd::ListLinkedDevicesOutput mediaServerDevices;
        rv = waitCloudPCOnline(mediaServerDevices);
        if (rv != 0) goto end;
        psnDeviceId = mediaServerDevices.devices(0).device_id();
    }


    rv = getUserIdAndClearFiDatasetId(userId_out, datasetId_out);
    if(rv != 0) {
        LOG_ERROR("getUserIdAndClearFiDatasetId:%d", rv);
        goto end;
    }

    {
        bool dump = !dumpfile.empty();
        std::ofstream f;

        ccd::MCAQueryMetadataObjectsInput req;
        ccd::MCAQueryMetadataObjectsOutput res;
        ccd::MCAQueryMetadataObjectsInput req1;
        ccd::MCAQueryMetadataObjectsOutput res1;
        ccd::MCAQueryMetadataObjectsInput req2;
        ccd::MCAQueryMetadataObjectsOutput res2;

        if(downloadPhoto) {
            req.set_cloud_device_id(psnDeviceId);
            req.set_filter_field(media_metadata::MCA_MDQUERY_PHOTOITEM);
            rv = CCDIMCAQueryMetadataObjects(req, res);
            if (rv != 0) {
                LOG_ERROR("CCDIMCAQueryMetadataObjects failed (%d)", rv);
                goto end;
            }
        }

        if(downloadVideo) {
            req1.set_cloud_device_id(psnDeviceId);
            req1.set_filter_field(media_metadata::MCA_MDQUERY_VIDEOITEM);
            rv = CCDIMCAQueryMetadataObjects(req1, res1);
            if (rv != 0) {
                LOG_ERROR("CCDIMCAQueryMetadataObjects failed (%d)", rv);
                goto end;
            }
        }

        if(downloadMusic) {
            req2.set_cloud_device_id(psnDeviceId);
            req2.set_filter_field(media_metadata::MCA_MDQUERY_MUSICTRACK);
            rv = CCDIMCAQueryMetadataObjects(req2, res2);
            if (rv != 0) {
                LOG_ERROR("CCDIMCAQueryMetadataObjects failed (%d)", rv);
                goto end;
            }
        }

        res.mutable_content_objects()->MergeFrom(res1.content_objects());
        res.mutable_content_objects()->MergeFrom(res2.content_objects());

        LOG_INFO("Number of items found %d\n", res.content_objects_size());

        if (dump) {
            f.open(dumpfile.c_str(), std::ios::out | std::ios::trunc);
            f << "# oid,filename,content_url,thumbnail_url" << std::endl;
        }

        for (int i = 0; i < res.content_objects_size(); i++) {
            if (dumpCountMax >= 0 && dumpCount >= dumpCountMax) {
                break;
            }

            const media_metadata::ContentDirectoryObject &o = res.content_objects(i).cdo();
            const media_metadata::MCAMetadataQueryObject &q = res.content_objects(i);
            if (o.has_photo_item()) {
                LOG_INFO("%s %s %s %s", o.object_id().c_str(), o.photo_item().title().c_str(), q.has_url() ? q.url().c_str() : NULL, q.has_thumbnail_url() ? q.thumbnail_url().c_str() : NULL);
                if (dump) {
                    if(collectionid == "streamtestcollection" || (collectionid != "streamtestcollection" && collectionid == q.collection_id())) {
                        f << "\"" << o.object_id().c_str() << "\",\"" << o.photo_item().title().c_str() << "\",";
                        if (q.has_url()) {
                            f << q.url().c_str();
                        }
                        f << ",";
                        if (q.has_thumbnail_url()) {
                            f << q.thumbnail_url().c_str();
                        }
                        f << std::endl;
                        dumpCount++;
                    }
                }
            }
            else if (o.has_video_item()) {
                LOG_INFO("%s %s %s %s", o.object_id().c_str(), o.video_item().title().c_str(), q.has_url() ? q.url().c_str() : NULL, q.has_thumbnail_url() ? q.thumbnail_url().c_str() : NULL);
                if (dump) {
                    if(collectionid == "streamtestcollection" || (collectionid != "streamtestcollection" && collectionid == q.collection_id())) {
                        f << "\"" << o.object_id().c_str() << "\",\"" << o.video_item().title().c_str() << "\",";
                        if (q.has_url()) {
                            f << q.url().c_str();
                        }
                        f << ",";
                        if (q.has_thumbnail_url()) {
                            f << q.thumbnail_url().c_str();
                        }
                        f << std::endl;
                        dumpCount++;
                    }
                }
            }
            else if (o.has_music_track()) {
                LOG_INFO("%s %s %s %s", o.object_id().c_str(), o.music_track().title().c_str(), q.has_url() ? q.url().c_str() : NULL, q.has_thumbnail_url() ? q.thumbnail_url().c_str() : NULL);
                if (dump) {
                    if(collectionid == "streamtestcollection" || (collectionid != "streamtestcollection" && collectionid == q.collection_id())) {
                        f << "\"" << o.object_id().c_str() << "\",\"" << o.music_track().title().c_str() << "\",";
                        if (q.has_url()) {
                            f << q.url().c_str();
                        }
                        f << ",";
                        if (q.has_thumbnail_url()) {
                            f << q.thumbnail_url().c_str();
                        }
                        f << std::endl;
                        dumpCount++;
                    }
                }
            }
        }  // for
    }

end:
    return rv;
}

//--------------------------------------------------
// dispatch

int setup_stream_test(int argc, const char* argv[])
{
    int rv = 0;

    if (checkHelp(argc, argv)) {
        std::cout << "SetupStreamTest [-c collectionid] [-D testfilesfolder] [-d dumpfile [-M dumpCountMax]] [-f filterCategory 0|1|2|3]" << std::endl;
        std::cout << "  collectionid    = Collection ID. Default is \"streamtestcollection\"" << std::endl;
        std::cout << "  testfilesfolder = Folder containing test files. Ignored on client." << std::endl;
        std::cout << "  dumpfile        = Dump URLs to this file." << std::endl;
        std::cout << "  dumpCountMax    = Dump at most this many files." << std::endl;
        std::cout << "  filterCategory  = 0 - All(Disabled) | 1 - Music | 2 - Photo | 3 - Music&Photo(Disabled) | 4 - Video | 5 - Music&Video(Disabled) | 6 - Photo&Video(Disabled). Default is 1 - dump music." << std::endl;
        return 0;
    }

    std::string collectionid;
    std::string testfilesfolder;
    std::string dumpfile;
    int dumpCountMax = -1;  // meaning no limit
    bool downloadMusic = true;
    bool downloadPhoto = false;
    bool downloadVideo = false;
    media_metadata::CatalogType_t catType = media_metadata::MM_CATALOG_MUSIC;
    collectionid.assign("streamtestcollection");  // default collection ID

    setDebugLevel(LOG_LEVEL_INFO);

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'c':
                if (i + 1 < argc) {
                    collectionid.assign(argv[++i]);
                }
                break;
            case 'D':
                if (i + 1 < argc) {
                    testfilesfolder.assign(argv[++i]);
                }
                break;
            case 'd':
                if (i + 1 < argc) {
                    dumpfile.assign(argv[++i]);
                }
                break;
            case 'M':
                if (i + 1 < argc) {
                    dumpCountMax = atoi(argv[++i]);
                }
                break;
            case 'f':
                if(i + 1 < argc) {
                    if(atoi(argv[i+1]) == 0) {
                        downloadMusic = true;
                        downloadPhoto = true;
                        downloadVideo = true;
                        LOG_ERROR("Not supported");
                        return -1;
                    }
                    else if(atoi(argv[i+1]) == 1) {
                        downloadMusic = true;
                        downloadPhoto = false;
                        downloadVideo = false;
                        catType = media_metadata::MM_CATALOG_MUSIC;
                    }
                    else if(atoi(argv[i+1]) == 2) {
                        downloadMusic = false;
                        downloadPhoto = true;
                        downloadVideo = false;
                        catType = media_metadata::MM_CATALOG_PHOTO;
                    }
                    else if(atoi(argv[i+1]) == 3) {
                        downloadMusic = true;
                        downloadPhoto = true;
                        downloadVideo = false;
                        LOG_ERROR("Not supported");
                        return -1;
                    }
                    else if(atoi(argv[i+1]) == 4) {
                        downloadMusic = false;
                        downloadPhoto = false;
                        downloadVideo = true;
                        catType = media_metadata::MM_CATALOG_VIDEO;
                    }
                    else if(atoi(argv[i+1]) == 5) {
                        downloadMusic = true;
                        downloadPhoto = false;
                        downloadVideo = true;
                        LOG_ERROR("Not supported");
                        return -1;
                    }
                    else {
                        downloadMusic = false;
                        downloadPhoto = true;
                        downloadVideo = true;
                        LOG_ERROR("Not supported");
                        return -1;
                    }
                }
                break;
            default:
                LOG_ERROR("Unknown option %s", argv[i]);
                return -2;
            }
        }
    }

    u64 userId = 0;
    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get user ID: %d", rv);
        return rv;
    }

    u64 deviceId = 0;
    rv = getDeviceId(&deviceId);
    if (rv != 0) {
        LOG_ERROR("Failed to get device ID: %d", rv);
        return rv;
    }

    if (!testfilesfolder.empty()) {
        if (isCloudpc(userId, deviceId)) {
            rv = setup_stream_test_create_metadata(userId,
                                                   deviceId,
                                                   catType,
                                                   collectionid,
                                                   testfilesfolder);
        }
        else {
            LOG_WARN("-D ignored on client PC");
        }
    }

    if (!dumpfile.empty()) {
        rv = setup_stream_test_write_dumpfile(userId,
                                              deviceId,
                                              collectionid,
                                              dumpfile,
                                              downloadMusic,
                                              downloadPhoto,
                                              downloadVideo,
                                              dumpCountMax);
    }

    resetDebugLevel();

    return rv;
}


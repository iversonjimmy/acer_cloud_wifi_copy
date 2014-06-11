//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "MediaMetadataCache.hpp"
#include "gvm_errors.h"
#include "gvm_file_utils.hpp"
#include "gvm_file_utils.h"
#include "gvm_misc_utils.h"
#include "log.h"
#include "media_metadata_errors.hpp"
#include "media_metadata_utils.hpp"
#include "protobuf_file_reader.hpp"
#include "protobuf_file_writer.hpp"
#include "scopeguard.hpp"
#include "vpl_fs.h"
#include "vplex_file.h"
#include "vplex_serialization.h"
#include <map>

using namespace media_metadata;

/// mode_t value for open(), required when O_CREAT flag is specified.
#define MM_NEW_FILE_MODE  (VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR)
#define MM_CATALOG_PREFIX "ca"
#define MM_COLLECTION_DIR_PREFIX "co"
#define MM_COLLECTION_FILE_PREFIX "m_"
#define MM_COLLECTION_DELETED_STR "DELETED"

#define MM_CATALOG_TYPE_STR_MUSIC "mu"
#define MM_CATALOG_TYPE_STR_PHOTO "ph"
#define MM_CATALOG_TYPE_STR_VIDEO "vi"

#define MM_FORMAT_STR_INDEX "i"
#define MM_FORMAT_STR_THUMBNAIL "t"
#define MM_FORMAT_STR_REPLAY "replay"

const char* MediaMetadataCache::getMetadataIndexStr()
{
    // Look for the catalog prefix.
    return MM_CATALOG_PREFIX;
}

static int g_debug=0;
void MediaMetadataCache::formMetadataTypeDir(const std::string& mediaDir,
                                             media_metadata::CatalogType_t type,
                                             std::string& typeDir_out)
{
    typeDir_out = "";
    if(mediaDir!="") {
        typeDir_out = mediaDir + "/";
    }
    switch(type) {
    case media_metadata::MM_CATALOG_MUSIC:
        typeDir_out += MM_CATALOG_TYPE_STR_MUSIC;
        break;
    case media_metadata::MM_CATALOG_PHOTO:
        typeDir_out += MM_CATALOG_TYPE_STR_PHOTO;
        break;
    case media_metadata::MM_CATALOG_VIDEO:
        typeDir_out += MM_CATALOG_TYPE_STR_VIDEO;
        break;
    default:
        LOG_ERROR("Should never happen:%d", (int)type);
        if(g_debug++>500) {
            LOG_ERROR("Something really wrong.");
            exit(-4567);
        }
        break;
    }
}

int MediaMetadataCache::parseMetadataTypeDirname(const std::string& dirent,
                                                 media_metadata::CatalogType_t& type_out)
{
    if(dirent == MM_CATALOG_TYPE_STR_MUSIC) {
        type_out = media_metadata::MM_CATALOG_MUSIC;
    }else if(dirent == MM_CATALOG_TYPE_STR_PHOTO) {
        type_out = media_metadata::MM_CATALOG_PHOTO;
    }else if(dirent == MM_CATALOG_TYPE_STR_VIDEO) {
        type_out = media_metadata::MM_CATALOG_VIDEO;
    }else {
        LOG_ERROR("%s not recognized", dirent.c_str());
        return -1;
    }
    return 0;
}

void MediaMetadataCache::formMetadataFormatDir(const std::string& mediaDir,
                                               media_metadata::CatalogType_t type,
                                               MetadataFormat metaFormat,
                                               std::string& formatDir_out)
{
    formMetadataTypeDir(mediaDir,
                        type,
                        formatDir_out);
    formatDir_out += std::string("/");
    switch(metaFormat) {
    case METADATA_FORMAT_INDEX:
        formatDir_out += MM_FORMAT_STR_INDEX;
        break;
    case METADATA_FORMAT_THUMBNAIL:
        formatDir_out += MM_FORMAT_STR_THUMBNAIL;
        break;
    case METADATA_FORMAT_REPLAY:
        formatDir_out += MM_FORMAT_STR_REPLAY;
        break;
    default:
        LOG_ERROR("%d not recognized", (int)metaFormat);
        break;
    }
}

int MediaMetadataCache::parseMetadataFormatDirname(const std::string& dirent,
                                                   MetadataFormat& metaFormat_out)
{
    if(dirent == MM_FORMAT_STR_INDEX) {
        metaFormat_out = METADATA_FORMAT_INDEX;
    } else if(dirent == MM_FORMAT_STR_THUMBNAIL) {
        metaFormat_out = METADATA_FORMAT_THUMBNAIL;
    } else {
        LOG_ERROR("%s not recognized", dirent.c_str());
        return -1;
    }
    return 0;
}

void MediaMetadataCache::formMetadataCatalogDir(const std::string& mediaDir,
                                                media_metadata::CatalogType_t type,
                                                MetadataFormat metaFormat,
                                                u64 serverDeviceId,
                                                std::string& catalogDir_out)
{
    formMetadataFormatDir(mediaDir,
                          type,
                          metaFormat,
                          catalogDir_out);
    catalogDir_out += std::string("/"MM_CATALOG_PREFIX) +
                      MMDeviceIdToString(serverDeviceId);
}

int MediaMetadataCache::parseMetadataCatalogDirname(const std::string& dirname,
                                                    u64& serverDeviceId_out)
{
    serverDeviceId_out = 0;
    std::string deviceIdStr;
    std::string beginsWith(MM_CATALOG_PREFIX);
    if(dirname.compare(0, beginsWith.size(), beginsWith) != 0) {
        return -1;
    }
    deviceIdStr = dirname.substr(beginsWith.size(),
                                 dirname.size()-beginsWith.size());

    serverDeviceId_out = Util_ParseStrictHex64(deviceIdStr.c_str());
    if(serverDeviceId_out == 0) {
        LOG_ERROR("Not a deviceId:%s", deviceIdStr.c_str());
        return -2;
    }
    return 0;
}

void MediaMetadataCache::formThumbnailCollectionDir(const std::string& mediaDir,
                                                    media_metadata::CatalogType_t catType,
                                                    MetadataFormat metaFormat,
                                                    u64 serverDeviceId,
                                                    const std::string& collectionId,
                                                    bool includeTimestamp,
                                                    u64 timestamp,
                                                    std::string& thumbDir_out)
{
    formMetadataCatalogDir(mediaDir,
                           catType,
                           metaFormat,
                           serverDeviceId,
                           thumbDir_out);

    thumbDir_out += std::string("/"MM_COLLECTION_DIR_PREFIX) +
                   collectionId;
    if(includeTimestamp) {
        thumbDir_out += std::string("_") + MMTimestampToString(timestamp);
    }
}

int MediaMetadataCache::parseThumbnailCollectionDirname(const std::string& collectionName,
                                                     std::string& collectionId_out)
{
    collectionId_out.clear();
    std::string beginsWith(MM_COLLECTION_DIR_PREFIX);
    if(collectionName.compare(0, beginsWith.size(), beginsWith) != 0) {
        return -1;
    }
    collectionId_out = collectionName.substr(beginsWith.size(),
                                             collectionName.size()-beginsWith.size());
    return 0;
}

int MediaMetadataCache::formThumbnailFilename(const std::string& objectId,
                                              const std::string& ext,
                                              std::string& thumbnail_out)
{
    char* base64EncodedObjId;
    int rv = 0;
    thumbnail_out.clear();
    rv = Util_EncodeBase64(objectId.c_str(),
                           objectId.size(),
                           &base64EncodedObjId,
                           NULL, VPL_FALSE, VPL_TRUE);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", "Util_EncodeBase64", rv);
        return rv;
    }
    ON_BLOCK_EXIT(free, base64EncodedObjId);

    thumbnail_out.assign(base64EncodedObjId);
    thumbnail_out.append(".");
    thumbnail_out.append(ext);

    return rv;
}

int MediaMetadataCache::parseThumbnailFilename(const std::string& thumbnailFile,
                                               std::string& objectId,
                                               std::string& ext)
{
    objectId.clear();
    ext.clear();
    std::string::size_type periodIndex = thumbnailFile.find_last_of(".");
    if(periodIndex == std::string::npos) {
        LOG_ERROR("Thumbnail file has no extension.");
        return -1;
    }
    std::string base64ObjId = thumbnailFile.substr(0, periodIndex);
    char decodedBuf[VPLFS_DIRENT_FILENAME_MAX];
    size_t decodedSize = VPLFS_DIRENT_FILENAME_MAX;

    VPL_DecodeBase64(base64ObjId.c_str(),
                     base64ObjId.size(),
                     decodedBuf,  // Result may not be null terminated
                     &decodedSize);
    if(decodedSize == 0) {
        LOG_ERROR("Could not decode base64:%s, %s",
                  thumbnailFile.c_str(), base64ObjId.c_str());
        return -2;
    }
    objectId.assign(decodedBuf, decodedSize);
    ext =  thumbnailFile.substr(periodIndex+1);

    return 0;
}

void MediaMetadataCache::formMetadataCollectionFilename(const std::string& collectionId,
                                                        u64 timestamp,
                                                        std::string& metadataFilename_out)
{
    metadataFilename_out = std::string(MM_COLLECTION_FILE_PREFIX) +
                           collectionId + "_" +
                           MMTimestampToString(timestamp);
}

void MediaMetadataCache::formReplayCollectionFilenameDeleted(const std::string& collectionId,
                                                               std::string& metadataFilename_out)
{
    metadataFilename_out = std::string(MM_COLLECTION_FILE_PREFIX) +
                           collectionId +
                           std::string("_"MM_COLLECTION_DELETED_STR);
}

int MediaMetadataCache::parseMetadataCollectionFilename(const std::string& dirent,
                                                        std::string& parsedCollectionId_out,
                                                        u64& parsedCollectionTimestamp_out)
{
    parsedCollectionId_out.clear();
    parsedCollectionTimestamp_out=0;

    std::string beginsWith(MM_COLLECTION_FILE_PREFIX);
    if(dirent.compare(0, beginsWith.size(), beginsWith) != 0) {
        LOG_DEBUG("Not a metadata file: %s", dirent.c_str());
        return -1;
    }
    std::size_t secondDashIndex = dirent.find("_", beginsWith.size());
    if(secondDashIndex == std::string::npos) {
        LOG_ERROR("Filename bad format:%s", dirent.c_str());
        return -2;
    }
    std::size_t thirdDashIndex = dirent.find("_", secondDashIndex+1);
    if(thirdDashIndex != std::string::npos) {
        LOG_ERROR("Filename bad format:%s", dirent.c_str());
        return -3;
    }

    parsedCollectionId_out = dirent.substr(beginsWith.size(),
                                           secondDashIndex - beginsWith.size());
    std::size_t secondDashIndexPlusOne = secondDashIndex+1;
    std::string timestampStr = dirent.substr(secondDashIndexPlusOne,
                                             dirent.size()-secondDashIndexPlusOne);
    // Must be 16 chars.
    if(timestampStr.size() != 16) {
        LOG_WARN("Timestamp not a 16 digit hex:%s (from %s)",
                 timestampStr.c_str(), dirent.c_str());
        return -4;
    }
    parsedCollectionTimestamp_out = Util_ParseStrictHex64(timestampStr.c_str());
    return 0;
}

void MediaMetadataCache::formReplayFilename(const std::string& collectionId,
                                            bool deleted,
                                            u64 timestamp,
                                            std::string& replayFilename_out)
{
    if(deleted) {
        formReplayCollectionFilenameDeleted(collectionId,
                                              replayFilename_out);
    } else {
        formMetadataCollectionFilename(collectionId,
                                       timestamp,
                                       replayFilename_out);
    }
}

int MediaMetadataCache::parseReplayFilename(const std::string& replayFilename,
                                            std::string& collectionId,
                                            bool& deleted_out,
                                            u64& timestamp)
{
    collectionId.clear();
    deleted_out = false;
    timestamp = 0;

    std::string beginsWith(MM_COLLECTION_FILE_PREFIX);
    if(replayFilename.compare(0, beginsWith.size(), beginsWith) != 0) {
        LOG_DEBUG("Not a metadata file: %s", replayFilename.c_str());
        return -1;
    }
    std::size_t secondDashIndex = replayFilename.find("_", beginsWith.size());
    if(secondDashIndex == std::string::npos) {
        LOG_ERROR("Filename bad format:%s", replayFilename.c_str());
        return -2;
    }
    std::size_t thirdDashIndex = replayFilename.find("_", secondDashIndex+1);
    if(thirdDashIndex != std::string::npos) {
        LOG_ERROR("Filename bad format:%s", replayFilename.c_str());
        return -3;
    }

    collectionId = replayFilename.substr(beginsWith.size(),
                                         secondDashIndex - beginsWith.size());
    std::size_t secondDashIndexPlusOne = secondDashIndex+1;
    std::string timestampStr = replayFilename.substr(secondDashIndexPlusOne,
                                                     replayFilename.size()-secondDashIndexPlusOne);
    if(timestampStr == MM_COLLECTION_DELETED_STR) {
        deleted_out = true;
    }else if(timestampStr.size() != 16) {    // Must be 16 chars.
        LOG_ERROR("Timestamp not a 16 digit hex:%s (from %s)",
                  timestampStr.c_str(), replayFilename.c_str());
        return -4;
    }else{
        timestamp = Util_ParseStrictHex64(timestampStr.c_str());
    }
    return 0;
}

s32 MediaMetadataCache::readHelper(const std::string& subscriptionPath,
                                   media_metadata::CatalogType_t catType,
                                   u64 serverDeviceId,
                                   const char* collection_id,
                                   u64* collectionTimestamp,
                                   bool errorIfNotExist)
{
    s32 rv = VPL_OK;
    std::string finalDataDir;
    formMetadataCatalogDir(subscriptionPath,
                           catType,
                           METADATA_FORMAT_INDEX,
                           serverDeviceId,
                           finalDataDir);
    finalDataDir = Util_CleanupPath(finalDataDir);

    VPLFS_dir_t dirHandle;
    VPLFS_dirent_t dirEnt;
    typedef std::map<std::string, u64> CollectionIdToTimestampMap;
    CollectionIdToTimestampMap encounteredCollectionId;
    int totalEntries = 0;
    int rc;

    reset();

    rc = VPLFS_Opendir(finalDataDir.c_str(), &dirHandle);
    if(rc == VPL_ERR_NOENT) {  // does not exist
        LOG_WARN("Cannot open file (%s) that doesn't exist (%d)", finalDataDir.c_str(), rc);
        goto failed_read;
    }else if(rc != VPL_OK) {
        LOG_ERROR("Error opening (%s) err=%d", finalDataDir.c_str(), rc);
        rv = rc;
        goto failed_read;
    }

    while (((rc = VPLFS_Readdir(&dirHandle, &dirEnt)) == VPL_OK) &&
            (rv==VPL_OK)) {
        if(dirEnt.type != VPLFS_TYPE_FILE) {
            continue;
        }
        std::string dirent(dirEnt.filename);
        std::string parsedCollectionId;
        u64 parsedCollectionTimestamp;

        rc = parseMetadataCollectionFilename(dirent,
                                             parsedCollectionId,
                                             parsedCollectionTimestamp);
        if(rc != 0) {
            if(rc != -1) {  // This error unexpected
                LOG_WARN("Parse filename error: %s, %d", dirent.c_str(), rc);
            }
            continue;
        }

        if(collection_id) {
            std::string collectionIdStr(collection_id);
            if(collectionIdStr != parsedCollectionId) {
                continue;
            }
        }

        if(collectionTimestamp) {
            if(parsedCollectionTimestamp != *collectionTimestamp) {
                continue;
            }
        }

        CollectionIdToTimestampMap::const_iterator collectionIdEntry =
                encounteredCollectionId.find(parsedCollectionId);
        if(collectionIdEntry != encounteredCollectionId.end()) {
            if(parsedCollectionTimestamp <= collectionIdEntry->second) {
                // already encountered this a more current version of this collection Id
                continue;
            }
        }

        ProtobufFileReader reader;
        std::string dataFile = finalDataDir + std::string("/") + dirent;
        rc = reader.open(dataFile.c_str(), errorIfNotExist);
        if(rc != 0) {
            if(errorIfNotExist || (rc != UTIL_ERR_FOPEN)) {
                LOG_ERROR("Can't open %s: %d", dataFile.c_str(), rc);
                rv = rc;
            }
            continue;
        }

        u32 version;
        u64 numEntries;
        google::protobuf::io::CodedInputStream tempStream(reader.getInputStream());
        if (!tempStream.ReadVarint32(&version)) {
            LOG_ERROR("Failed to read version in %s", dataFile.c_str());
            if(errorIfNotExist) {
                rv = rc;
            }
            continue;
        }

        if(version != METADATA_COLLECTION_FORMAT_VERSION) {
            LOG_INFO("Metadata collection %s format version %d is not %d.  Ignoring.",
                     dataFile.c_str(), version, METADATA_COLLECTION_FORMAT_VERSION);
            continue;
        }

        if (!tempStream.ReadVarint64(&numEntries)) {
            LOG_ERROR("Failed to read numEntries in %s", dataFile.c_str());
            if(errorIfNotExist) {
                rv = rc;
            }
            continue;
        }
        int count = 0;
        for (u64 i = 0; i < numEntries; i++) {
            u32 currMsgLen;
            if (!tempStream.ReadVarint32(&currMsgLen)) {
                LOG_ERROR("Failed to read size of entry["FMTu64"] (of "FMTu64"), %s",
                          i, numEntries, dataFile.c_str());
                if(errorIfNotExist) {
                    rv = rc;
                }
                continue;
            }
            google::protobuf::io::CodedInputStream::Limit limit =
                    tempStream.PushLimit(static_cast<int>(currMsgLen));
            ContentDirectoryObject currObj;
            if (!currObj.ParseFromCodedStream(&tempStream)) {
                LOG_ERROR("Failed to read entry["FMTu64"] (of "FMTu64"), %s",
                          i, numEntries, dataFile.c_str());
                if(errorIfNotExist) {
                    rv = rc;
                }
                continue;;
            }
            tempStream.PopLimit(limit);
            LOG_DEBUG("Read [%d]: %s", count++, currObj.DebugString().c_str());
            m_entries[currObj.object_id()] = currObj;
            m_collection_id[currObj.object_id()] = parsedCollectionId;
            totalEntries++;
        }
        encounteredCollectionId[parsedCollectionId] = parsedCollectionTimestamp;
    }

    rc = VPLFS_Closedir(&dirHandle);
    if(rc != VPL_OK) {
        LOG_ERROR("Closedir failed %s:%d", finalDataDir.c_str(), rc);
        rv = rc;
    }

    if(totalEntries==0 && errorIfNotExist && rv==0) {
        rv = MM_ERR_NO_DATASET;
    }

    return rv;
failed_read:
    reset();
    return rv;
}

s32 MediaMetadataCache::deleteCollections(const std::string& subscriptionPath,
                                          media_metadata::CatalogType_t catType,
                                          u64 serverDeviceId,
                                          const char* collection_id,
                                          u64* preserveTimestamp)
{
    s32 rv = VPL_OK;
    std::string finalDataDir;
    formMetadataCatalogDir(subscriptionPath,
                           catType,
                           METADATA_FORMAT_INDEX,
                           serverDeviceId,
                           finalDataDir);
    finalDataDir = Util_CleanupPath(finalDataDir);

    VPLFS_dir_t dirHandle;
    VPLFS_dirent_t dirEnt;
    int rc;

    rc = VPLFS_Opendir(finalDataDir.c_str(), &dirHandle);
    if(rc == VPL_ERR_NOENT) { // does not exist
        goto failed_read;
    }else if (rc != VPL_OK) {
        LOG_ERROR("Error opening (%s) err=%d", finalDataDir.c_str(), rc);
        rv = rc;
        goto failed_read;
    }

    while (((rc = VPLFS_Readdir(&dirHandle, &dirEnt)) == VPL_OK) &&
            (rv==VPL_OK)) {
        if(dirEnt.type != VPLFS_TYPE_FILE) {
            continue;
        }

        std::string dirent(dirEnt.filename);
        std::string parsedCollectionId;
        u64 parsedCollectionTimestamp;

        rc = parseMetadataCollectionFilename(dirent,
                                             parsedCollectionId,
                                             parsedCollectionTimestamp);
        if(rc != 0) {
            if(rc != -1) {  // This error unexpected
                LOG_WARN("Parse filename error: %s, %d", dirent.c_str(), rc);
            }
            continue;
        }

        if(collection_id) {
            std::string collectionIdStr(collection_id);
            if(collectionIdStr != parsedCollectionId) {
                continue;
            }
            if(preserveTimestamp) {
                // No need to delete something that we're just going to overwrite
                // (in another function)
                if(*preserveTimestamp == parsedCollectionTimestamp) {
                    continue;
                }
            }
        }

        std::string dataFile = finalDataDir + std::string("/") + dirent;
        rc = VPLFile_Delete(dataFile.c_str());
        if(rc != 0) {
            LOG_ERROR("Deleting %s:%d", dataFile.c_str(), rc);
            rv = rc;
            continue;
        }
        LOG_INFO("Removed collection %s by deleting %s.",
                 parsedCollectionId.c_str(), dataFile.c_str());
    }

    rc = VPLFS_Closedir(&dirHandle);
    if(rc != VPL_OK) {
        LOG_ERROR("Closedir failed %s:%d", finalDataDir.c_str(), rc);
        rv = rc;
    }

    return rv;
failed_read:
    return rv;
}

s32 MediaMetadataCache::listCollections(const std::string& subscriptionPath,
                                        media_metadata::CatalogType_t catType,
                                        u64 serverDeviceId,
                                        ListCollectionsOutput& listCollectionOutput_out)
{
    s32 rv = VPL_OK;
    std::string finalDataDir;
    formMetadataCatalogDir(subscriptionPath,
                           catType,
                           METADATA_FORMAT_INDEX,
                           serverDeviceId,
                           finalDataDir);
    finalDataDir = Util_CleanupPath(finalDataDir);
    listCollectionOutput_out.Clear();

    VPLFS_dir_t dirHandle;
    VPLFS_dirent_t dirEnt;
    std::map<std::string, u64> collectionIdAndTimestampMap;
    int rc;

    rc = VPLFS_Opendir(finalDataDir.c_str(), &dirHandle);
    if(rc == VPL_ERR_NOENT) { // Does not exist
        return 0;
    }else if (rc != VPL_OK) {
        LOG_ERROR("Error opening (%s) err=%d", finalDataDir.c_str(), rc);
        rv = rc;
        return rv;
    }

    while (((rc = VPLFS_Readdir(&dirHandle, &dirEnt)) == VPL_OK) &&
            (rv==VPL_OK)) {
        if(dirEnt.type != VPLFS_TYPE_FILE) {
            continue;
        }
        std::string dirent(dirEnt.filename);
        std::string parsedCollectionId;
        u64 parsedCollectionTimestamp;
        std::map<std::string,u64>::iterator foundCollectionId;
        bool isNewerCollectionId = false;

        rc = parseMetadataCollectionFilename(dirent,
                                             parsedCollectionId,
                                             parsedCollectionTimestamp);
        if(rc != 0) {
            if(rc != -1) {  // This error more unexpected, but should be still ok
                LOG_WARN("Parse filename error: %s, %d", dirent.c_str(), rc);
            }
            continue;
        }

        foundCollectionId = collectionIdAndTimestampMap.find(parsedCollectionId);
        if(foundCollectionId == collectionIdAndTimestampMap.end())
        {
            isNewerCollectionId = true;
        }else if(foundCollectionId->second < parsedCollectionTimestamp)
        {
            isNewerCollectionId = true;
        }

        if(isNewerCollectionId) {
            collectionIdAndTimestampMap[parsedCollectionId] =
                                parsedCollectionTimestamp;
        }
    }

    rc = VPLFS_Closedir(&dirHandle);
    if(rc != VPL_OK) {
        LOG_ERROR("Closing dir:%s", finalDataDir.c_str());
        // Continue though;
    }

    std::map<std::string,u64>::iterator iterateMap =
            collectionIdAndTimestampMap.begin();
    for(;iterateMap != collectionIdAndTimestampMap.end(); ++iterateMap)
    {
        listCollectionOutput_out.add_collection_id(iterateMap->first);
        listCollectionOutput_out.add_collection_timestamp(iterateMap->second);
    }

    return rv;
}

s32 MediaMetadataCache::read(const std::string& subscriptionPath,
                             media_metadata::CatalogType_t catType,
                             u64 serverDeviceId,
                             bool errorIfNotExist)
{
    return readHelper(subscriptionPath,
                      catType,
                      serverDeviceId,
                      NULL,
                      NULL,
                      errorIfNotExist);

}

s32 MediaMetadataCache::readCollection(const std::string& subscriptionPath,
                                       const std::string& collectionId,
                                       u64* collectionTimestamp,
                                       media_metadata::CatalogType_t catType,
                                       u64 serverDeviceId,
                                       bool errorIfNotExist)
{
    return readHelper(subscriptionPath,
                      catType,
                      serverDeviceId,
                      collectionId.c_str(),
                      collectionTimestamp,
                      errorIfNotExist);
}

s32 MediaMetadataCache::writeCollectionToTemp(const std::string& tempPath,
                                              const std::string& collectionIdStr,
                                              u64 timestamp,
                                              media_metadata::CatalogType_t catType,
                                              const MediaServerInfo& serverInfo)
{
    s32 rv;
    // TODO: could use VPL to get temp file instead
    std::string collFilename;
    formMetadataCollectionFilename(collectionIdStr, timestamp, collFilename);
    std::string metadataDir_out;
    formMetadataCatalogDir(tempPath,
                           catType,
                           METADATA_FORMAT_INDEX,
                           serverInfo.cloud_device_id(),
                           metadataDir_out);
    std::string tempDataFile = metadataDir_out + "/" + collFilename;
    {
        ProtobufFileWriter writer;
        rv = writer.open(tempDataFile.c_str(), MM_NEW_FILE_MODE);
        if (rv != 0) {
            LOG_ERROR("Failed to open \"%s\" for writing: %d", tempDataFile.c_str(), rv);
            goto out;
        }
        bool success = true;
        google::protobuf::io::CodedOutputStream tempStream(writer.getOutputStream());
        tempStream.WriteVarint32(METADATA_COLLECTION_FORMAT_VERSION);
        success &= !tempStream.HadError();
        tempStream.WriteVarint64(numEntries());
        success &= !tempStream.HadError();
        int count = 0;
        for (m_entries_type::iterator it = m_entries.begin(); it != m_entries.end(); it++) {
            const ContentDirectoryObject& currObj = (*it).second;
            LOG_DEBUG("Writing [%d]: %s", count++, currObj.DebugString().c_str());
            tempStream.WriteVarint32(currObj.ByteSize());
            success &= currObj.SerializeToCodedStream(&tempStream);
        }
        if (!success) {
            LOG_ERROR("Failed to write to \"%s\"", tempDataFile.c_str());
            rv = MM_ERR_WRITE;
            goto out;
        }
    } // Must close the tempDataFile before attempting to rename it (for Windows).
      // Explicitly ending scope (closing datafile) in case we add more code
 out:
    return rv;
}

s32 MediaMetadataCache::moveCollectionTempToDataset(const std::string& subscriptionPath,
                                                    const std::string& tempPath,
                                                    const std::string& collectionIdStr,
                                                    u64 timestamp,
                                                    media_metadata::CatalogType_t catType,
                                                    const MediaServerInfo& serverInfo)
{
    s32 rv;
    LOG_DEBUG("Writing metadata for "FMTx64" to \"%s\"",
              serverInfo.cloud_device_id(), subscriptionPath.c_str());
    std::string finalDataDir;
    formMetadataCatalogDir(subscriptionPath,
                           catType,
                           METADATA_FORMAT_INDEX,
                           serverInfo.cloud_device_id(),
                           finalDataDir);
    // See http://intwww.routefree.com/wiki/index.php/MSA#Implementation_Details
    // for path specification.
    int rc;
    std::string collFilename;
    formMetadataCollectionFilename(collectionIdStr, timestamp, collFilename);
    std::string metadataDir_out;
    formMetadataCatalogDir(tempPath,
                           catType,
                           METADATA_FORMAT_INDEX,
                           serverInfo.cloud_device_id(),
                           metadataDir_out);
    std::string tempDataFile = metadataDir_out + "/" + collFilename;
    std::string finalDataFile = finalDataDir + "/" + collFilename;
    // Ensure parent directory exists
    rv = Util_CreatePath(finalDataFile.c_str(), VPL_FALSE);
    if(rv != 0) {
        LOG_ERROR("Failed to create parent path of %s: %d",
                  finalDataFile.c_str(), rv);
        goto out;
    }

    // Make sure all other files with the same collection Id are deleted
    // before moving the collection in.
    rc = deleteCollections(subscriptionPath,
                           catType,
                           serverInfo.cloud_device_id(),
                           collectionIdStr.c_str(),
                           &timestamp);
    if(rc != 0) {
        LOG_ERROR("deleteCollections:%d, subscriptionPath:%s, cloudId:"FMTx64
                 ", collectionStr:%s, timestamp:"FMTx64,
                 rc,
                 subscriptionPath.c_str(),
                 serverInfo.cloud_device_id(),
                 collectionIdStr.c_str(),
                 timestamp);
        rv = rc;
        goto out;
    }

    rv = VPLFile_Rename(tempDataFile.c_str(), finalDataFile.c_str());
    if (rv != 0) {
        LOG_ERROR("Failed to rename \"%s\" to \"%s\": %d",
                tempDataFile.c_str(), finalDataFile.c_str(), rv);
        goto out;
    }
 out:
    return rv;
}

void MediaMetadataCache::formDeviceInfoFilename(u64 serverDeviceId,
                                                std::string& deviceInfoFilename_out)
{
    deviceInfoFilename_out = MMDeviceIdToString(serverDeviceId);
}

// This file is currently read by mca_control_point::MCAEnumerateMediaServers
s32 MediaMetadataCache::writeDeviceInfo(const std::string& subscriptionPath,
                                        const std::string& tempPath,
                                        media_metadata::CatalogType_t catType,
                                        const MediaServerInfo& serverInfo)
{
    s32 rv;
    LOG_DEBUG("Writing metadata for "FMTx64" to \"%s\"",
              serverInfo.cloud_device_id(), subscriptionPath.c_str());
    std::string finalDataDir;
    formMetadataCatalogDir(subscriptionPath,
                           catType,
                           METADATA_FORMAT_INDEX,
                           serverInfo.cloud_device_id(),
                           finalDataDir);
    // Write the server device info for the client to enumerate.
    {
        std::string deviceInfoFilename;
        formDeviceInfoFilename(serverInfo.cloud_device_id(),
                               deviceInfoFilename);
        std::string tempDataFile = tempPath + "/" + deviceInfoFilename;
        {
            ProtobufFileWriter writer;
            rv = writer.open(tempDataFile.c_str(), MM_NEW_FILE_MODE);
            if (rv != 0) {
                LOG_ERROR("Failed to open \"%s\" for writing: %d", tempDataFile.c_str(), rv);
                goto out;
            }
            bool success = serverInfo.SerializeToZeroCopyStream(writer.getOutputStream());
            if (!success) {
                LOG_ERROR("Failed to write to \"%s\"", tempDataFile.c_str());
                rv = MM_ERR_WRITE;
                goto out;
            }
        } // Must close the tempDataFile before attempting to rename it (for Windows).
        std::string finalDataFile = finalDataDir + "/" + deviceInfoFilename;
        // Ensure parent directory exists
        rv = Util_CreatePath(finalDataFile.c_str(), VPL_FALSE);
        if(rv != 0) {
            LOG_ERROR("Failed to create parent path of %s: %d",
                      finalDataFile.c_str(), rv);
            goto out;
        }
        rv = VPLFile_Rename(tempDataFile.c_str(), finalDataFile.c_str());
        if (rv != 0) {
            LOG_ERROR("Failed to rename \"%s\" to \"%s\": %d",
                    tempDataFile.c_str(), finalDataFile.c_str(), rv);
            goto out;
        }
    }
out:
    return rv;
}

int MediaMetadataCache::readDeviceInfo(const std::string& deviceInfoFile,
                                       MediaServerInfo& mediaServerInfo_out)
{
    ProtobufFileReader reader;
    int rc = reader.open(deviceInfoFile.c_str(), true);
    if (rc != 0) {
        LOG_WARN("Failed to open %s: %d; skipping", deviceInfoFile.c_str(), rc);
        return rc;
    } else {
        if (!mediaServerInfo_out.ParseFromZeroCopyStream(reader.getInputStream())) {
            LOG_WARN("Failed to read %s; skipping", deviceInfoFile.c_str());
            return -1;
        }
        return 0;
    }
}

const ContentDirectoryObject* MediaMetadataCache::get(const std::string& objectId) const
{
    m_entries_type::const_iterator it = m_entries.find(objectId);
    if (it != m_entries.end()) {
        return &((*it).second);
    } else {
        return NULL;
    }
}

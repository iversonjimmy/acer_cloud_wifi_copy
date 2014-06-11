//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//


#ifndef __MEDIAMETADATACACHE_HPP__
#define __MEDIAMETADATACACHE_HPP__

//============================================================================
/// @file
/// 
//============================================================================

#include "vplu.h"
#include "media_metadata_types.pb.h"

namespace media_metadata {

const static u32 METADATA_COLLECTION_FORMAT_VERSION = 1;

enum MetadataFormat {
    METADATA_FORMAT_INDEX,
    METADATA_FORMAT_THUMBNAIL,
    METADATA_FORMAT_REPLAY
};

// TODO: lock on each method?
class MediaMetadataCache
{
public:
    MediaMetadataCache() {}
    ~MediaMetadataCache() {}

    static const char* getMetadataIndexStr();  // returns "MM_INDEX_PATH" string

    /////////////////////////////////////////////////
    //////////// Metadata file paths ////////////////

    // See http://wiki.ctbg.acer.com/wiki/index.php/MSA_Impl#Format_as_of_2.5.0
    // for path examples

    static void formMetadataTypeDir(const std::string& mediaDir,
                                    media_metadata::CatalogType_t type,
                                    std::string& typeDir_out);
    static int parseMetadataTypeDirname(const std::string& dirent,
                                        media_metadata::CatalogType_t& type_out);

    static void formMetadataFormatDir(const std::string& mediaDir,
                                      media_metadata::CatalogType_t type,
                                      MetadataFormat metaFormat,
                                      std::string& formatDir_out);
    static int parseMetadataFormatDirname(const std::string& dirname,
                                          MetadataFormat& metaFormat_out);

    static void formMetadataCatalogDir(const std::string& mediaDir,
                                       media_metadata::CatalogType_t type,
                                       MetadataFormat metaFormat,
                                       u64 serverDeviceId,
                                       std::string& catalogDir_out);
    static int parseMetadataCatalogDirname(const std::string& dirname,
                                           u64& serverDeviceId_out);

    static void formMetadataCollectionFilename(const std::string& collectionId,
                                               u64 timestamp,
                                               std::string& metadataFilename_out);
    static int parseMetadataCollectionFilename(const std::string& dirent,
                                               std::string& parsedCollectionId_out,
                                               u64& parsedCollectionTimestamp_out);
    //////////// Metadata file paths ////////////////
    /////////////////////////////////////////////////

    //////////////////////////////////////////////////
    //////////// Thumbnail file paths ////////////////
    static void formThumbnailCollectionDir(const std::string& mediaDir,
                                           media_metadata::CatalogType_t type,
                                           MetadataFormat metaFormat,
                                           u64 serverDeviceId,
                                           const std::string& collectionId,
                                           bool includeTimestamp,
                                           u64 timestamp,
                                           std::string& thumbDir_out);
    static int parseThumbnailCollectionDirname(const std::string& collectionName,
                                            std::string& collectionId_out);
    static int formThumbnailFilename(const std::string& objectId,
                                     const std::string& ext,
                                     std::string& thumbnail_out);
    static int parseThumbnailFilename(const std::string& thumbnailFile,
                                      std::string& objectId,
                                      std::string& ext);
    //////////// Thumbnail file paths ////////////////
    //////////////////////////////////////////////////

    static void formReplayCollectionFilenameDeleted(const std::string& collectionId,
                                                      std::string& metadataFilename_out);
    static void formReplayFilename(const std::string& collectionId,
                                   bool deleted,
                                   u64 timestamp,
                                   std::string& replayFilename_out);

    static int parseReplayFilename(const std::string& replayFilename,
                                   std::string& collectionId,
                                   bool& deleted_out,
                                   u64& timestamp);

    static void formDeviceInfoFilename(u64 serverDeviceId,
                                       std::string& deviceInfoFilename_out);

    static int readDeviceInfo(const std::string& deviceInfoFile,
                              MediaServerInfo& mediaServerInfo_out);

    s32 read(const std::string& subscriptionPath,
             media_metadata::CatalogType_t catType,
             u64 serverDeviceId,
             bool errorIfNotExist);
    s32 readCollection(const std::string& subscriptionPath,
                       const std::string& collectionId,
                       u64* collectionTimestamp, // Optional arg, can be null.
                       media_metadata::CatalogType_t catType,
                       u64 serverDeviceId,
                       bool errorIfNotExist);

    // step 1) of write collection
    s32 writeCollectionToTemp(const std::string& tempPath,
                              const std::string& collectionIdStr,
                              u64 timestamp,
                              media_metadata::CatalogType_t catType,
                              const MediaServerInfo& serverInfo);
    // step 2) of write collection
    s32 moveCollectionTempToDataset(const std::string& subscriptionPath,
                                    const std::string& tempPath,
                                    const std::string& collectionIdStr,
                                    u64 timestamp,
                                    media_metadata::CatalogType_t catType,
                                    const MediaServerInfo& serverInfo);

    s32 writeDeviceInfo(const std::string& subscriptionPath,
                        const std::string& tempPath,
                        media_metadata::CatalogType_t catType,
                        const MediaServerInfo& serverInfo);

    static s32 deleteCollections(const std::string& subscriptionPath,
                                 media_metadata::CatalogType_t catType,
                                 u64 serverDeviceId,
                                 const char* collection_id,
                                 u64* preserveTimestamp);

    static s32 listCollections(const std::string& subscriptionPath,
                               media_metadata::CatalogType_t catType,
                               u64 serverDeviceId,
                               ListCollectionsOutput& listCollectionOutput_out);

    const ContentDirectoryObject* get(const std::string& objectId) const;

    void update(const ContentDirectoryObject& metadata)
    {
        // TODO: validity check?
        m_entries[metadata.object_id()] = metadata;
    }

    void remove(const std::string& metadata_object_id)
    {
        m_entries.erase(metadata_object_id);
        m_collection_id.erase(metadata_object_id);
    }

    void reset()
    {
        m_entries.clear();
        m_collection_id.clear();
    }

    size_t numEntries() const
    {
        return m_entries.size();
    }

    typedef std::map<std::string, ContentDirectoryObject> m_entries_type;

    class ObjIterator
    {
    public:
        ObjIterator(const MediaMetadataCache& cache/*, typeFilter*/) :
            cache(cache)
        {
            it = cache.m_entries.begin();
        }

        const ContentDirectoryObject* next()
        {
            while (it != cache.m_entries.end()) {
                const ContentDirectoryObject* result = &((*it).second);
                it++;
#if 0
                if (result_is_of_type(result, type_filter))
#endif
                {
                    return result;
                }
            }
            return NULL;
        }
    private:
        const MediaMetadataCache& cache;
        m_entries_type::const_iterator it;
    };

    const std::map<std::string, std::string> &collection_id() {
        return m_collection_id;
    }
private:
    VPL_DISABLE_COPY_AND_ASSIGN(MediaMetadataCache);

    s32 readHelper(const std::string& subscriptionPath,
                   media_metadata::CatalogType_t catType,
                   u64 serverDeviceId,
                   const char* collection_id,  // optional arg, can be NULL
                   u64* collectionTimestamp,   // optional arg, can be NULL
                   bool errorIfNotExist);
    m_entries_type m_entries;
    std::map<std::string, std::string> m_collection_id;  // map: objectid -> collectionid
};

} // namespace media_metadata

#endif // include guard

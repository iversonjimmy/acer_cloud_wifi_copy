//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __MCA_DB_HPP__
#define __MCA_DB_HPP__

#include <vplu_types.h>

#include "media_metadata_errors.hpp"
#include "media_metadata_types.pb.h"

#include <sqlite3.h>
#include <string>

const static u64 MCA_CONTROL_DB_SCHEMA_VERSION = 12;

typedef std::map<std::string, u64> CollectionIdAndTimestampMap;
typedef std::vector<std::string> CollectionList;

class McaControlDB {
 public:
    McaControlDB();
    virtual ~McaControlDB();

    MMError openDB(const std::string &dbpath);
    MMError initTables();

    MMError insertMcaDbVersion(u64 user_id,
                               u64 version_id);  // This will clear out all other fields in version table.
    MMError getMcaDbVersion(u64 user_id,
                            u64 &version_out);
    MMError setMcaDbInitialized(bool isInit);
    MMError getMcaDbInitialized(bool& isInit_out);

    MMError updateContentDirectoryObject(u64 cloudPcId,
                                         const std::string& collectionId,
                                         const media_metadata::ContentDirectoryObject& cdo);

    MMError beginTransaction();
    MMError commitTransaction();
    // MMError rollbackTransaction(); Not supported due to journaling turned off
    MMError getCloudPcIds(std::vector<u64>& cloudPcIds_out);
    MMError deleteCollection(u64 cloudPcId,
                             media_metadata::CatalogType_t type,
                             const std::string& collectionId);
    MMError getCollections(u64 cloudPcId,
                           media_metadata::CatalogType_t type,
                           CollectionIdAndTimestampMap& map_out);
    MMError updateCollection(u64 cloudPcId,
                             media_metadata::CatalogType_t type,
                             const std::string& collectionId,
                             u64 timestamp);
    MMError deleteCatalog(u64 cloudPcId);
    MMError closeDB();

    MMError generatePhotoAlbum2Db(u64 cloudPcId,
                                  const std::string& collectionId,
                                  const std::string& album_name,
                                  const std::string& thumbnail,
                                  const u64 timestamp);

    MMError updatePhotoAlbum(u64 cloudPcId,
                    const std::string& collectionId,
                    const media_metadata::ContentDirectoryObject& photoAlbum);

    MMError updatePhotoAlbumRelation( u64 cloudPcId,
                                      const std::string& photoObjectId,
                                      const std::string& albumIdRef);

    ///////////////////////////////////////////////////////////////////////////
    ////////////////////////////  SQL Queries /////////////////////////////////

    // Example sql query (no need to begin or commit transaction)
    MMError getAllObjectIdsSorted(
            google::protobuf::RepeatedPtrField< media_metadata::ContentDirectoryObject > &cdos);

    // get music tracks
    MMError getMusicTracks(u64 cloudPcId,
                           const std::string& searchQuery,
                           const std::string& orderQuery,
                           google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject > & output);
    // get music albums
    MMError getMusicAlbums(u64 cloudPcId,
                           const std::string& searchQuery,
                           const std::string& orderQuery,
                           google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject > & output);
    // get photos
    MMError getPhotos(u64 cloudPcId,
                      const std::string& searchQuery,
                      const std::string& orderQuery,
                      google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject > & output);
    // get videos
    MMError getVideos(u64 cloudPcId,
                      const std::string& searchQuery,
                      const std::string& orderQuery,
                      google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject > & output);

    // get groups of music artists
    MMError getMusicArtist(u64 cloudPcId,
                           const std::string& searchQuery,
                           const std::string& orderQuery,
                           google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject> &output);
    // get groups of music artists
    MMError getMusicGenre(u64 cloudPcId,
                          const std::string& searchQuery,
                          const std::string& orderQuery,
                          google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject> &output);
    // get photo albums
    MMError getPhotoAlbums(u64 cloudPcId,
                           const std::string& searchQuery,
                           const std::string& orderQuery,
                           google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject> &output);
    // get video albums
    MMError getVideoAlbums(u64 cloudPcId,
                           const std::string& searchQuery,
                           const std::string& orderQuery,
                           google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject> &output);

    ////////////////////////////  SQL Queries /////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // Helper functions to open a DB.
    static MMError createOrOpenMetadataDB(McaControlDB& mcaControlDb,
                                          const std::string& dbDirectory,
                                          u64 user_id,
                                          bool& dbNeedsToBePopulated_out,
                                          bool& dbNeedsToBeUpgradedFromV10ToV11_out,
                                          bool& dbNeedsToBeUpgradedFromV11ToV12_out);

 private:
    MMError updateMusicTrack(u64 cloudPcId,
                             const std::string& collectionId,
                             const media_metadata::ContentDirectoryObject& musicTrack);
    MMError updateMusicAlbum(u64 cloudPcId,
                             const std::string& collectionId,
                             const media_metadata::ContentDirectoryObject& musicAlbum);
    MMError updateVideo(u64 cloudPcId,
                        const std::string& collectionId,
                        const media_metadata::ContentDirectoryObject& video);
    MMError updatePhoto(u64 cloudPcId,
                        const std::string& collectionId,
                        const media_metadata::ContentDirectoryObject& photo);
    MMError deleteMusicTracks(u64 cloudPcId, const std::string& collectionId);
    MMError deleteMusicAlbums(u64 cloudPcId, const std::string& collectionId);
    MMError deleteVideos(u64 cloudPcId, const std::string& collectionId);
    MMError deletePhotos(u64 cloudPcId, const std::string& collectionId);
    MMError deleteMusicTracksByCloudPcId(u64 cloudPcId);
    MMError deleteMusicAlbumsByCloudPcId(u64 cloudPcId);
    MMError deleteVideosByCloudPcId(u64 cloudPcId);
    MMError deletePhotosByCloudPcId(u64 cloudPcId);

    void alterTableAddInitializedColumnVersion10();
    void alterPhotoTableVersion11();
    void addPhotoAlbumTableAndAlbumIdRefVersion12();

    MMError deletePhotosByAlbumId(u64 cloudPcId, const std::string& album_id);
    MMError deletePhotosByCollectionId(u64 cloudPcId, const std::string& collection_id);
    sqlite3 *m_db;
    std::string m_dbpath;
};


#endif /* __MCA_DB_HPP__ */


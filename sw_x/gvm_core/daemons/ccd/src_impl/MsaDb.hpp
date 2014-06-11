//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __MSA_DB_HPP__
#define __MSA_DB_HPP__

#include <vplu_types.h>

#include "media_metadata_errors.hpp"
#include "media_metadata_types.pb.h"

#include <sqlite3.h>
#include <string>

const static u64 MSA_CONTROL_DB_SCHEMA_VERSION = 4;

const static u64 MSA_CONTROL_DB_NOT_UPDATED = 0;
const static u64 MSA_CONTROL_DB_UPDATED = 1;

typedef std::vector<std::string> CollectionList;

class MsaControlDB {
public:
    MsaControlDB();
    virtual ~MsaControlDB();

    MMError openDB(const std::string &dbpath);
    MMError initTables();

    MMError insertDbVersion(u64 user_id,
                            u64 version_id);
    MMError getDbVersion(u64 user_id,
                         u64 &version_out);

    MMError setDbUpdated(u64 user_id,
                         u64 updated);
    MMError getDbUpdated(u64 user_id,
                         u64 &updated_out);

    MMError updateCollection(const std::string& collection_id,
                             const media_metadata::CatalogType_t catalog_type);

    MMError updateContentObject(const std::string& collection_id,
                                const media_metadata::ContentDirectoryObject& object);

    MMError beginTransaction();

    MMError commitTransaction();

    MMError rollbackTransaction();

    MMError deleteCatalog(const media_metadata::CatalogType_t& catalog_type);

    MMError deleteCollection(const std::string& collection_id);

//    MMError deleteContentObject(const std::string& collection_id,
//                                const std::string& object_id);

    MMError deleteContentObjectsByCollectionId(const std::string& collection_id);

    MMError getCollectionCatalogType(const std::string& collection_id,
                                     media_metadata::CatalogType_t& catalog_type);

    MMError getObjectMetadata(const std::string& objectId,
                              const std::string& collectionId,
                              media_metadata::GetObjectMetadataOutput& output);

    MMError closeDB();

    void addPhotoAlbumRelationTableVersion4();

    MMError updatePhotoAlbumRelation( const std::string& photoObjectId,
                                      const std::string& albumIdRef,
                                      const std::string& collectionId);

private:

    MMError getCollectionsByCatalogType(const media_metadata::CatalogType_t& catalog_type,
                                        CollectionList& collections);

    MMError getObjectMetadataByObjId(const std::string& objectId,
                                     media_metadata::CatalogType_t catalogType,
                                     media_metadata::GetObjectMetadataOutput& output);

    MMError deletePhotoContentObjectsByCollectionId(const std::string& collection_id);

    sqlite3 *m_db;
    std::string m_dbpath;
};

MMError createOrOpenMediaServerDB(MsaControlDB& msaControlDb,
                                  const std::string& dbDirectory,
                                  u64 user_id,
                                  bool& dbNeedsToBePopulated_out,
                                  bool& dbNeedsToBeUpgradedFromV3ToV4_out);

#endif /* __MSA_DB_HPP__ */


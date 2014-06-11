//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "MsaDb.hpp"

#include <vpl_plat.h>
#include <vpl_error.h>
#include <vpl_fs.h>

#include "ccd_features.h"
#include "media_metadata_errors.hpp"
#include "MediaDbUtil.hpp"
#include "util_open_db_handle.hpp"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <vplex_file.h>
#include <vplu_format.h>
#include <vplu_types.h>
#include <list>

#include <log.h>

#if !CCD_ENABLE_MEDIA_SERVER_AGENT
# error "This platform/configuration does not support MSA; this source file should not be included in the build."
#endif

static const char SQL_CREATE_MSA_TABLES[] =
"CREATE TABLE IF NOT EXISTS version ("
        "id INTEGER PRIMARY KEY,"
        "user_id INTEGER,"
        "updated INTEGER,"
        "msa_schema_version INTEGER);"
"CREATE TABLE IF NOT EXISTS collections ("
        "collection_id TEXT,"
        "catalog_type INTEGER,"
        "PRIMARY KEY(collection_id));"
"CREATE INDEX IF NOT EXISTS collections_index ON collections(collection_id);"
"CREATE TABLE IF NOT EXISTS musics ("
        "object_id TEXT,"
        "collection_id_ref TEXT,"
        "media_type INTEGER,"
        "file_format TEXT,"
        "absolute_path TEXT,"
        "thumbnail TEXT,"
        "PRIMARY KEY(object_id, collection_id_ref));"
"CREATE INDEX IF NOT EXISTS musics_index ON musics(object_id);"
"CREATE TABLE IF NOT EXISTS videos ("
        "object_id TEXT PRIMARY KEY,"
        "collection_id_ref TEXT,"
        "media_type INTEGER,"
        "file_format TEXT,"
        "absolute_path TEXT,"
        "thumbnail TEXT);"
"CREATE INDEX IF NOT EXISTS videos_index ON videos(object_id);"
"CREATE TABLE IF NOT EXISTS photos ("
        "object_id TEXT PRIMARY KEY,"
        "collection_id_ref TEXT,"
        "media_type INTEGER,"
        "file_format TEXT,"
        "absolute_path TEXT,"
        "thumbnail TEXT);"
"CREATE INDEX IF NOT EXISTS photos_index ON photos(object_id);"
//   Version 4 Start //
"CREATE TABLE IF NOT EXISTS photos_albums_relation ("
      "object_id TEXT, "
      "album_id_ref TEXT, "
      "collection_id_ref TEXT, "
      "PRIMARY KEY(object_id, album_id_ref));"
"CREATE INDEX IF NOT EXISTS relation_index ON photos_albums_relation(album_id_ref);";
//   Version 4 end //

static void metadataServerDBFilename(u64 userId,
                                     const std::string& metadataFileDir,
                                     std::string& metadataFile_out)
{
    char user_dataset_str[64];
    sprintf(user_dataset_str, "/"FMTx64, userId);

    metadataFile_out.assign(metadataFileDir);
    metadataFile_out.append(user_dataset_str);
    metadataFile_out.append(".msa_metadb");
}

MsaControlDB::MsaControlDB()
:  m_db(NULL)
{
    ;
}

MsaControlDB::~MsaControlDB()
{
    (void) closeDB();
}

MMError MsaControlDB::openDB(const std::string &dbpath)
{
    if (m_db && (dbpath==m_dbpath)) {
        return MSA_CONTROLDB_DB_ALREADY_OPEN;
    }else if(m_db) {
        LOG_WARN("Db still open:%s.  Closing and opening %s instead.",
                 m_dbpath.c_str(), dbpath.c_str());
        int rc = sqlite3_close(m_db);
        if(rc != 0) {
            LOG_ERROR("Error closing db:%d", rc);
        }
        m_db = NULL;
    }

    m_dbpath.assign(dbpath);

    int rv = Util_OpenDbHandle(m_dbpath, true, true, &m_db);
    if (rv != SQLITE_OK) {
        LOG_ERROR("Failed to create/open db at %s: %d", dbpath.c_str(), rv);
        return MSA_CONTROLDB_DB_OPEN_FAIL;
    }

    return MSA_CONTROLDB_OK;
}

MMError MsaControlDB::initTables()
{
    char *errmsg = NULL;
    int rc;
    if(m_db) {
        // make sure db has needed tables defined
        rc = sqlite3_exec(m_db, SQL_CREATE_MSA_TABLES, NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            LOG_ERROR("Failed to create tables: %d: %s", rc, errmsg);
            sqlite3_free(errmsg);
            return MSA_CONTROLDB_DB_CREATE_TABLE_FAIL;
        }
        return MSA_CONTROLDB_OK;
    }
    return MSA_CONTROLDB_DB_NOT_OPEN;
}

MMError MsaControlDB::insertDbVersion(u64 user_id,
                                      u64 version_id)
{
    int rv = MSA_CONTROLDB_OK;
    int rc;
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_SET_MANIFEST_VERSION =
            "INSERT OR REPLACE INTO version (id,"
                                            "user_id,"
                                            "updated,"
                                            "msa_schema_version) "
            "VALUES (?, ?, ?, ?)";
    rc = sqlite3_prepare_v2(m_db, SQL_SET_MANIFEST_VERSION, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_SET_MANIFEST_VERSION, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, 1);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 2, user_id);
    CHECK_BIND(rv, rc, m_db, end);
    // reset updated to MSA_CONTROL_DB_NOT_UPDATED
    // when version_id is changed, it implies DB needs to be updated
    rc = sqlite3_bind_int64(stmt, 3, MSA_CONTROL_DB_NOT_UPDATED);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 4, version_id);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::getDbVersion(u64 user_id,
                                   u64 &version_out)
{
    int rc;
    int rv = MSA_CONTROLDB_OK;
    version_out=0;

    static const char* SQL_GET_MANIFEST_VERSION =
            "SELECT msa_schema_version "
            "FROM version "
            "WHERE id=1 and user_id=?";
    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(m_db, SQL_GET_MANIFEST_VERSION, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_MANIFEST_VERSION, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, user_id);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

    if (rc == SQLITE_DONE) {
        rv = MSA_CONTROLDB_DB_NO_VERSION;
        goto end;
    }else if(rc != SQLITE_ROW) {
        LOG_ERROR("Error: %d", rc);
        rv = MSA_CONTROLDB_DB_INTERNAL_ERR;
        goto end;
    }
    // rv == SQLITE_ROW
    if (sqlite3_column_type(stmt, 0) != SQLITE_INTEGER) {
        rv = MSA_CONTROLDB_DB_BAD_VALUE;
        goto end;
    }
    version_out = sqlite3_column_int64(stmt, 0);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::setDbUpdated(u64 user_id,
                                   u64 updated)
{
    int rv = MSA_CONTROLDB_OK;
    int rc;
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_SET_DB_UPDATED =
            "UPDATE version "
            "SET updated = ? "
            "WHERE id = ? AND user_id = ?";
    rc = sqlite3_prepare_v2(m_db, SQL_SET_DB_UPDATED, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_SET_DB_UPDATED, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, updated);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 2, 1);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 3, user_id);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::getDbUpdated(u64 user_id,
                                   u64 &updated_out)
{
    int rc;
    int rv = MSA_CONTROLDB_OK;
    updated_out=0;

    static const char* SQL_GET_DB_UPDATED =
            "SELECT updated "
            "FROM version "
            "WHERE id=1 and user_id=?";
    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(m_db, SQL_GET_DB_UPDATED, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_DB_UPDATED, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, user_id);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

    if (rc == SQLITE_DONE) {
        updated_out = MSA_CONTROL_DB_NOT_UPDATED;
        goto end;
    }else if(rc != SQLITE_ROW) {
        LOG_ERROR("Error: %d", rc);
        rv = MSA_CONTROLDB_DB_INTERNAL_ERR;
        goto end;
    }
    // rv == SQLITE_ROW
    if (sqlite3_column_type(stmt, 0) != SQLITE_INTEGER) {
        rv = MSA_CONTROLDB_DB_BAD_VALUE;
        goto end;
    }
    updated_out = sqlite3_column_int64(stmt, 0);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::updateCollection(const std::string& collection_id,
                                       const media_metadata::CatalogType_t catalog_type)
{
    int rc = 0;
    MMError rv = MSA_CONTROLDB_OK;

    static const char* SQL_UPDATE_COLLECTIONs =
            "INSERT OR REPLACE INTO collections(collection_id,"
                                               "catalog_type) "
             "VALUES (?,?)";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE_COLLECTIONs, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE_COLLECTIONs, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, collection_id.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_int64(stmt, 2, (u64)catalog_type);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}
MMError MsaControlDB::updateContentObject(const std::string& collection_id,
                                          const media_metadata::ContentDirectoryObject& cdo)
{
    int rc = 0;
    MMError rv = MSA_CONTROLDB_OK;
    media_metadata::CatalogType_t catalog_type;

    char SQL_UPDATE_OBJECT[SQL_MAX_QUERY_LENGTH] = {0};
    static const char* SQL_UPDATE_OBJECT_FORMAT =
            "INSERT OR REPLACE INTO %s(object_id,"
                                      "collection_id_ref,"
                                      "media_type,"
                                      "file_format,"
                                      "absolute_path,"
                                      "thumbnail) "
            "VALUES (?,?,?,?,?,?)";
    sqlite3_stmt *stmt = NULL;

    // choose table according to catalog_type
    rv = getCollectionCatalogType(collection_id, catalog_type);
    if (rv != MSA_CONTROLDB_OK) {
        goto end;
    }
    switch (catalog_type) {
    case media_metadata::MM_CATALOG_MUSIC:
        rc = sprintf(SQL_UPDATE_OBJECT, SQL_UPDATE_OBJECT_FORMAT, "musics");
        break;
    case media_metadata::MM_CATALOG_VIDEO:
        rc = sprintf(SQL_UPDATE_OBJECT, SQL_UPDATE_OBJECT_FORMAT, "videos");
        break;
    case media_metadata::MM_CATALOG_PHOTO:
        rc = sprintf(SQL_UPDATE_OBJECT, SQL_UPDATE_OBJECT_FORMAT, "photos");
        break;
    default:
        LOG_WARN("Not a valid catalog type:%s", cdo.object_id().c_str());
        rv = MM_ERR_INVALID;
        goto end;
    }

    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE_OBJECT, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE_OBJECT, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, cdo.object_id().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, collection_id.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    if (cdo.has_music_album()) {
        // media_type
        rc = sqlite3_bind_int64(stmt, 3, (u64)media_metadata::MEDIA_MUSIC_ALBUM);
        CHECK_BIND(rv, rc, m_db, end);
        // file_format
        rc = sqlite3_bind_null(stmt, 4);
        CHECK_BIND(rv, rc, m_db, end);
        // absolute_path
        rc = sqlite3_bind_null(stmt, 5);
        CHECK_BIND(rv, rc, m_db, end);
        // thumbnail
        if(cdo.music_album().has_album_thumbnail()) {
            rc = sqlite3_bind_text(stmt, 6, cdo.music_album().album_thumbnail().c_str(), -1, SQLITE_STATIC);
            CHECK_BIND(rv, rc, m_db, end);
        }
        else {
            rc = sqlite3_bind_null(stmt, 6);
            CHECK_BIND(rv, rc, m_db, end);
        }
    }
    else if (cdo.has_music_track()) {
        // media_type
        rc = sqlite3_bind_int64(stmt, 3, (u64)media_metadata::MEDIA_MUSIC_TRACK);
        CHECK_BIND(rv, rc, m_db, end);
        // file_format
        if(cdo.music_track().has_file_format()) {
            rc = sqlite3_bind_text(stmt, 4, cdo.music_track().file_format().c_str(), -1, SQLITE_STATIC);
            CHECK_BIND(rv, rc, m_db, end);
        }
        else {
            rc = sqlite3_bind_null(stmt, 4);
            CHECK_BIND(rv, rc, m_db, end);
        }
        // absolute_path
        if(cdo.music_track().has_absolute_path()) {
            rc = sqlite3_bind_text(stmt, 5, cdo.music_track().absolute_path().c_str(), -1, SQLITE_STATIC);
            CHECK_BIND(rv, rc, m_db, end);
        }
        else {
            rc = sqlite3_bind_null(stmt, 5);
            CHECK_BIND(rv, rc, m_db, end);
        }
        // thumbnail
        rc = sqlite3_bind_null(stmt, 6);
        CHECK_BIND(rv, rc, m_db, end);
    }
    else if (cdo.has_photo_item()) {
        // media_type
        rc = sqlite3_bind_int64(stmt, 3, (u64)media_metadata::MEDIA_PHOTO);
        CHECK_BIND(rv, rc, m_db, end);
        // file_format
        if(cdo.photo_item().has_file_format()) {
            rc = sqlite3_bind_text(stmt, 4, cdo.photo_item().file_format().c_str(), -1, SQLITE_STATIC);
            CHECK_BIND(rv, rc, m_db, end);
        }
        else {
            rc = sqlite3_bind_null(stmt, 4);
            CHECK_BIND(rv, rc, m_db, end);
        }
        // absolute_path
        if(cdo.photo_item().has_absolute_path()) {
            rc = sqlite3_bind_text(stmt, 5, cdo.photo_item().absolute_path().c_str(), -1, SQLITE_STATIC);
            CHECK_BIND(rv, rc, m_db, end);
        }
        else {
            rc = sqlite3_bind_null(stmt, 5);
            CHECK_BIND(rv, rc, m_db, end);
        }
        // thumbnail
        if(cdo.photo_item().has_thumbnail()) {
            rc = sqlite3_bind_text(stmt, 6, cdo.photo_item().thumbnail().c_str(), -1, SQLITE_STATIC);
            CHECK_BIND(rv, rc, m_db, end);
        }
        else {
            rc = sqlite3_bind_null(stmt, 6);
            CHECK_BIND(rv, rc, m_db, end);
        }
    }
    else if (cdo.has_video_item()) {
        // media_type
        rc = sqlite3_bind_int64(stmt, 3, (u64)media_metadata::MEDIA_VIDEO);
        CHECK_BIND(rv, rc, m_db, end);
        // file_format
        if(cdo.video_item().has_file_format()) {
            rc = sqlite3_bind_text(stmt, 4, cdo.video_item().file_format().c_str(), -1, SQLITE_STATIC);
            CHECK_BIND(rv, rc, m_db, end);
        }
        else {
            rc = sqlite3_bind_null(stmt, 4);
            CHECK_BIND(rv, rc, m_db, end);
        }
        // absolute_path
        if(cdo.video_item().has_absolute_path()) {
            rc = sqlite3_bind_text(stmt, 5, cdo.video_item().absolute_path().c_str(), -1, SQLITE_STATIC);
            CHECK_BIND(rv, rc, m_db, end);
        }
        else {
            rc = sqlite3_bind_null(stmt, 5);
            CHECK_BIND(rv, rc, m_db, end);
        }
        // thumbnail
        if(cdo.video_item().has_thumbnail()) {
            rc = sqlite3_bind_text(stmt, 6, cdo.video_item().thumbnail().c_str(), -1, SQLITE_STATIC);
            CHECK_BIND(rv, rc, m_db, end);
        }
        else {
            rc = sqlite3_bind_null(stmt, 6);
            CHECK_BIND(rv, rc, m_db, end);
        }
    } else if (cdo.has_photo_album()) {
        LOG_INFO("Insert photo album collection id: %s [obj_id=%s]", collection_id.c_str(), cdo.object_id().c_str() );
        // media_type
        rc = sqlite3_bind_int64(stmt, 3, (u64)media_metadata::MEDIA_PHOTO_ALBUM);
        CHECK_BIND(rv, rc, m_db, end);
        // file_format
        rc = sqlite3_bind_null(stmt, 4);
        CHECK_BIND(rv, rc, m_db, end);
        // absolute_path
        rc = sqlite3_bind_null(stmt, 5);
        CHECK_BIND(rv, rc, m_db, end);
        // thumbnail
        if(cdo.photo_album().has_album_thumbnail()) {
            rc = sqlite3_bind_text(stmt, 6, cdo.photo_album().album_thumbnail().c_str(), -1, SQLITE_STATIC);
            CHECK_BIND(rv, rc, m_db, end);
        }
        else {
            rc = sqlite3_bind_null(stmt, 6);
            CHECK_BIND(rv, rc, m_db, end);
        }
    }
    else {
        LOG_ERROR("Wrong type of content object: %s", cdo.object_id().c_str());
        rv = MSA_CONTROLDB_DB_BAD_VALUE;
        goto end;
    }

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);

    if (rv == 0 && cdo.has_photo_item()) {
        if (cdo.photo_item().has_album_ref())
            rv = updatePhotoAlbumRelation(cdo.object_id(), cdo.photo_item().album_ref(), collection_id);
        else
            rv = updatePhotoAlbumRelation(cdo.object_id(), collection_id, collection_id);
    }

    return rv;
}

MMError MsaControlDB::updatePhotoAlbumRelation( const std::string& photoObjectId,
                                                const std::string& albumIdRef,
                                                const std::string& collectionId)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_UPDATE =
        "INSERT OR REPLACE INTO photos_albums_relation(object_id, album_id_ref, collection_id_ref) VALUES (?,?,?)";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, photoObjectId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 2, albumIdRef.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 3, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::beginTransaction()
{
    int rc;
    int rv = MSA_CONTROLDB_OK;

    static const char* SQL_BEGIN = "BEGIN IMMEDIATE";
    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(m_db, SQL_BEGIN, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_BEGIN, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::commitTransaction()
{
    int rc;
    int rv = MSA_CONTROLDB_OK;

    static const char* SQL_COMMIT = "COMMIT";
    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(m_db, SQL_COMMIT, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_COMMIT, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::rollbackTransaction()
{
    int rc;
    int rv = MSA_CONTROLDB_OK;

    static const char* SQL_ROLLBACK = "ROLLBACK";
    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(m_db, SQL_ROLLBACK, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_ROLLBACK, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::deleteCatalog(const media_metadata::CatalogType_t& catalog_type)
{
    int rc;
    int rv = MSA_CONTROLDB_OK;
    CollectionList collections;

    static const char* SQL_DELETE_COLLECTIONS =
            "DELETE FROM collections "
            "WHERE catalog_type=?";
    sqlite3_stmt *stmt = NULL;

    rc = getCollectionsByCatalogType(catalog_type, collections);
    if (rc != 0) {
        rv = rc;
        goto end;
    }

    if (collections.size() > 0) {
        for (int i=0; i < (int)collections.size(); i++) {
            rc = deleteCollection (collections.at(i));
            if (rc != 0) {
                LOG_ERROR("deleteCollection(%s):%d", collections.at(i).c_str(), rc);
                rv = rc;
                break;
            }
        }
    }

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_COLLECTIONS, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_COLLECTIONS, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, (u64)catalog_type);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::deleteCollection(const std::string& collection_id)
{
    int rc;
    int rv = MSA_CONTROLDB_OK;

    static const char* SQL_DELETE_COLLECTION =
            "DELETE FROM collections WHERE collection_id=?";
    sqlite3_stmt *stmt = NULL;

    rc = deleteContentObjectsByCollectionId(collection_id);
    if(rc == MSA_CONTROLDB_DB_NOT_PRESENT) {
        // Bug 12901: count as success if collection not found yet in db
        // also print proper warning logs
        LOG_WARN("deleteContentObjectsByCollectionId(%s):%d. Count as success.",
                  collection_id.c_str(), rc);
        rv = rc;
    }
    else if(rc != MSA_CONTROLDB_OK) {
        LOG_ERROR("deleteContentObjectsByCollectionId(%s):%d",
                  collection_id.c_str(), rc);
        rv = rc;
        goto end;
    }

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_COLLECTION, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_COLLECTION, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, collection_id.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

//MMError MsaControlDB::deleteContentObject(const std::string& collection_id,
//                                          const std::string& object_id)
//{
//    int rc = 0;
//    MMError rv = MSA_CONTROLDB_OK;
//    char SQL_DELETE_OBJECT[SQL_MAX_QUERY_LENGTH] = {0};
//    static const char* SQL_DELETE_OBJECT_FORMAT =
//            "DELETE FROM %s "
//            "WHERE object_id=?";
//    sqlite3_stmt *stmt = NULL;
//    media_metadata::CatalogType_t catalog_type;
//
//    rv = getCollectionCatalogType(collection_id, catalog_type);
//    if (rv != MSA_CONTROLDB_OK) {
//        goto end;
//    }
//    switch(catalog_type) {
//    case media_metadata::MM_CATALOG_MUSIC:
//        rc = sprintf(SQL_DELETE_OBJECT, SQL_DELETE_OBJECT_FORMAT, "musics");
//        break;
//    case media_metadata::MM_CATALOG_VIDEO:
//        rc = sprintf(SQL_DELETE_OBJECT, SQL_DELETE_OBJECT_FORMAT, "videos");
//        break;
//    case media_metadata::MM_CATALOG_PHOTO:
//        rc = sprintf(SQL_DELETE_OBJECT, SQL_DELETE_OBJECT_FORMAT, "photos");
//        break;
//    default:
//        LOG_ERROR("Should never happen: %d", catalog_type);
//        rv = MSA_CONTROLDB_DB_NOT_PRESENT;
//        goto end;
//    }
//
//    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_OBJECT, -1, &stmt, NULL);
//    CHECK_PREPARE(rv, rc, SQL_DELETE_OBJECT, m_db, end);
//
//    rc = sqlite3_bind_text(stmt, 1, object_id.c_str(), -1, SQLITE_STATIC);
//    CHECK_BIND(rv, rc, m_db, end);
//
//    rc = sqlite3_step(stmt);
//    CHECK_STEP(rv, rc, m_db, end);
//
//end:
//    FINALIZE_STMT(rv, rc, m_db, stmt);
//    return rv;
//}

MMError MsaControlDB::deleteContentObjectsByCollectionId(const std::string& collection_id)
{
    int rc = 0;
    MMError rv = MSA_CONTROLDB_OK;
    char SQL_DELETE_CONTENTS_BY_COLLECTION_ID[SQL_MAX_QUERY_LENGTH] = {0};
    static const char* SQL_DELETE_CONTENTS_FORMAT =
            "DELETE FROM %s WHERE collection_id_ref=?";
    sqlite3_stmt *stmt = NULL;
    media_metadata::CatalogType_t catalog_type;

    rv = getCollectionCatalogType(collection_id, catalog_type);
    if (rv != MSA_CONTROLDB_OK) {
        goto end;
    }
    switch(catalog_type) {
    case media_metadata::MM_CATALOG_MUSIC:
        rc = sprintf(SQL_DELETE_CONTENTS_BY_COLLECTION_ID, SQL_DELETE_CONTENTS_FORMAT, "musics");
        break;
    case media_metadata::MM_CATALOG_VIDEO:
        rc = sprintf(SQL_DELETE_CONTENTS_BY_COLLECTION_ID, SQL_DELETE_CONTENTS_FORMAT, "videos");
        break;
    case media_metadata::MM_CATALOG_PHOTO:
        //rc = sprintf(SQL_DELETE_CONTENTS_BY_COLLECTION_ID, SQL_DELETE_CONTENTS_FORMAT, "photos");
        return deletePhotoContentObjectsByCollectionId(collection_id);
        break;
    default:
        LOG_ERROR("Should never happen: %d", catalog_type);
        rv = MSA_CONTROLDB_DB_NOT_PRESENT;
        goto end;
    }

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_CONTENTS_BY_COLLECTION_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_CONTENTS_BY_COLLECTION_ID, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, collection_id.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::deletePhotoContentObjectsByCollectionId(const std::string& collection_id)
{
    int rc = 0;
    MMError rv = MSA_CONTROLDB_OK;

    static const char* SQL_DELETE_RELATION = "DELETE FROM photos_albums_relation WHERE collection_id_ref=?";
    static const char* SQL_DELETE_PHOTOS = "DELETE FROM photos "
                                           "WHERE object_id NOT IN (SELECT object_id FROM photos_albums_relation) "
                                           " AND media_type=4 "; //type=photo_item
    static const char* SQL_DELETE_COLLECTIONS = "DELETE FROM photos WHERE media_type=5 AND collection_id_ref=?"; //type=photo_album

    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_RELATION, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_RELATION, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, collection_id.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_PHOTOS, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_PHOTOS, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_COLLECTIONS, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_COLLECTIONS, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, collection_id.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::getCollectionCatalogType(const std::string& collection_id,
                                               media_metadata::CatalogType_t& catalog_type)
{
    int rc = 0;
    MMError rv = MSA_CONTROLDB_OK;
    int resIndex = 0;
    int sqlType;

    static const char* SQL_GET_COLLECTION =
            "SELECT catalog_type "
            "FROM collections "
            "WHERE collection_id=?";
    
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_GET_COLLECTION, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_COLLECTION, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, collection_id.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

    if (rc == SQLITE_DONE) {
        rv = MSA_CONTROLDB_DB_NOT_PRESENT;
        goto end;
    }

    // catalog_type
    sqlType = sqlite3_column_type(stmt, resIndex);
    if (sqlType == SQLITE_INTEGER) {
        u64 catalogIntType = sqlite3_column_int64(stmt, resIndex);
        catalog_type = (media_metadata::CatalogType_t)catalogIntType;
    }else{
        LOG_ERROR("Bad column type index:%d", resIndex);
        rv = MSA_CONTROLDB_DB_BAD_VALUE;
        goto end;
    }

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError MsaControlDB::getObjectMetadataByObjId(const std::string& objectId,
                                               media_metadata::CatalogType_t catalogType,
                                               media_metadata::GetObjectMetadataOutput& output)
{
    MMError rv = MSA_CONTROLDB_OK;
    int rc = 0;
    int sqlType = SQLITE_NULL;
    int resIndex = 0;
    char SQL_GET_OBJECT[SQL_MAX_QUERY_LENGTH] = {0};
    static const char* SQL_GET_OBJECT_FORMAT =
            "SELECT media_type, file_format, absolute_path, thumbnail "
            "FROM %s "
            "WHERE object_id=?";
    sqlite3_stmt *stmt = NULL;

    switch (catalogType) {
    case media_metadata::MM_CATALOG_MUSIC:
        rc = sprintf(SQL_GET_OBJECT, SQL_GET_OBJECT_FORMAT, "musics");
        break;
    case media_metadata::MM_CATALOG_VIDEO:
        rc = sprintf(SQL_GET_OBJECT, SQL_GET_OBJECT_FORMAT, "videos");
        break;
    case media_metadata::MM_CATALOG_PHOTO:
        rc = sprintf(SQL_GET_OBJECT, SQL_GET_OBJECT_FORMAT, "photos");
        break;
    default:
        LOG_ERROR("Not a valid catalog type:%s", objectId.c_str());
        rv = MM_ERR_INVALID;
        goto end;
    }

    rc = sqlite3_prepare_v2(m_db, SQL_GET_OBJECT, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_OBJECT, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, objectId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

    if (rc == SQLITE_DONE) {
        rv = MSA_CONTROLDB_DB_NOT_PRESENT;
        goto end;
    }

    // media_type
    sqlType = sqlite3_column_type(stmt, resIndex);
    if (sqlType == SQLITE_INTEGER) {
        u64 mediaType = sqlite3_column_int64(stmt, resIndex);
        output.set_media_type((media_metadata::MediaType_t)mediaType);
    }else{
        LOG_ERROR("Bad column type index:%d", resIndex);
        rv = MSA_CONTROLDB_DB_BAD_VALUE;
        goto end;
    }

    // file_format
    sqlType = sqlite3_column_type(stmt, ++resIndex);
    if (sqlType == SQLITE_TEXT) {
        std::string file_format = reinterpret_cast<const char*>(
                                  sqlite3_column_text(stmt, resIndex));
        output.set_file_format(file_format);
    }else if (sqlType != SQLITE_NULL){
        LOG_ERROR("Bad column type index:%d", resIndex);
        rv = MSA_CONTROLDB_DB_BAD_VALUE;
        goto end;
    }

    // absolute_path
    sqlType = sqlite3_column_type(stmt, ++resIndex);
    if (sqlType == SQLITE_TEXT) {
        std::string absolute_path = reinterpret_cast<const char*>(
                                    sqlite3_column_text(stmt, resIndex));
        output.set_absolute_path(absolute_path);
    }else if (sqlType != SQLITE_NULL){
        LOG_ERROR("Bad column type index:%d", resIndex);
        rv = MSA_CONTROLDB_DB_BAD_VALUE;
        goto end;
    }

    // thumbnail
    sqlType = sqlite3_column_type(stmt, ++resIndex);
    if (sqlType == SQLITE_TEXT) {
        std::string thumbnail = reinterpret_cast<const char*>(
                                sqlite3_column_text(stmt, resIndex));
        output.set_thumbnail(thumbnail);
    }else if (sqlType != SQLITE_NULL){
        LOG_ERROR("Bad column type index:%d", resIndex);
        rv = MSA_CONTROLDB_DB_BAD_VALUE;
        goto end;
    }

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;

}

MMError MsaControlDB::getObjectMetadata(const std::string& objectId,
                                        const std::string& collectionId,
                                        media_metadata::GetObjectMetadataOutput& output)
{
    MMError rv = MSA_CONTROLDB_OK;

    if (collectionId.empty()) {
        // bug 15879: query all the tables for specific object id
        for (int i = media_metadata::CatalogType_t_MIN;
            i <= media_metadata::CatalogType_t_MAX;
            i++) {
            rv = getObjectMetadataByObjId(objectId, (media_metadata::CatalogType_t)i, output);
            if (rv == MSA_CONTROLDB_DB_NOT_PRESENT) {
                LOG_DEBUG("Cannot find %s in %d table", objectId.c_str(), i);
            }
            else if (rv != MSA_CONTROLDB_OK) {
                LOG_ERROR("Failed to query %s in %d table: %d", objectId.c_str(), i, rv);
                goto end;
            }
            else {
                LOG_INFO("Found %s in %d table", objectId.c_str(), i);
                goto end;
            }
        }
        LOG_WARN("Cannot find %s in any table", objectId.c_str());
    }
    else {
        media_metadata::CatalogType_t catalog_type;
        rv = getCollectionCatalogType(collectionId, catalog_type);
        if (rv != MSA_CONTROLDB_OK) {
            goto end;
        }
        rv = getObjectMetadataByObjId(objectId, catalog_type, output);
        if (rv != MSA_CONTROLDB_OK) {
            goto end;
        }
    }

end:
    return rv;

}

MMError MsaControlDB::closeDB()
{
    if (m_db) {
        int rc = sqlite3_close(m_db);
        if(rc != 0) {
            LOG_ERROR("Error closing db:%d", rc);
        }
        m_db = NULL;
    }

    return MSA_CONTROLDB_OK;
}

MMError MsaControlDB::getCollectionsByCatalogType(const media_metadata::CatalogType_t& catalog_type,
                                                  CollectionList& collections)
{
    int rc;
    int rv = MSA_CONTROLDB_OK;

    static const char* SQL_GET_COLLECTIONS =
            "SELECT collection_id "
            "FROM collections "
            "WHERE catalog_type=?";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_GET_COLLECTIONS, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_COLLECTIONS, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, (u64)catalog_type);
    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string collectionId;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        // collection_id
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            collectionId = reinterpret_cast<const char*>(
                           sqlite3_column_text(stmt, resIndex));
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MSA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

        collections.push_back(collectionId);
    }

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

void MsaControlDB::addPhotoAlbumRelationTableVersion4()
{
    int rc = 0;
    char *errmsg = NULL;

    rc = sqlite3_exec(m_db, "CREATE TABLE IF NOT EXISTS photos_albums_relation ("
                            "object_id TEXT,"
                            "album_id_ref TEXT,"
                            "collection_id_ref TEXT,"
                            "PRIMARY KEY(object_id, album_id_ref));"
                            "CREATE INDEX IF NOT EXISTS relation_index ON photos_albums_relation(album_id_ref);", NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Create photos_albums_relation TABLE: %d(%d): %s", rc, sqlite3_extended_errcode(m_db), errmsg);
        sqlite3_free(errmsg);
        sqlite3_free(errmsg);
    } else {
        LOG_INFO("Version 4: create table photos_albums_relation successfully.");
    }

}

MMError createOrOpenMediaServerDB(MsaControlDB& msaControlDb,
                                  const std::string& dbDirectory,
                                  u64 user_id,
                                  bool& dbNeedsToBePopulated_out,
                                  bool& dbNeedsToBeUpgradedFromV3ToV4_out)
{
    int rc;
    bool initDb = false;
    bool fileMissing = false;
    dbNeedsToBePopulated_out = false;
    dbNeedsToBeUpgradedFromV3ToV4_out = false;

    std::string msaDbFile;
    metadataServerDBFilename(user_id,
                             dbDirectory,
                             msaDbFile);

    VPLFS_stat_t statBuf;
    rc = VPLFS_Stat(msaDbFile.c_str(), &statBuf);
    if (rc != VPL_OK) {
        LOG_INFO("Missing server manifest file(%d).  Starting replace for %s.",
                 rc, msaDbFile.c_str());
        initDb = true;
        fileMissing = true;
    } else {
        // close db before opening 
        rc = msaControlDb.closeDB();
        if (rc != MSA_CONTROLDB_OK) {
            LOG_WARN("Closing server manifest db %s, %d", msaDbFile.c_str(), rc);
        }
        rc = msaControlDb.openDB(msaDbFile);
        if((rc != MSA_CONTROLDB_OK) && (rc != MSA_CONTROLDB_DB_ALREADY_OPEN)) {
            LOG_WARN("Opening server manifest db %s, %d", msaDbFile.c_str(), rc);
            initDb = true;
        }else{
            u64 version_out;
            rc = msaControlDb.getDbVersion(user_id,
                                           version_out);
            if(rc != MSA_CONTROLDB_OK) {
                LOG_WARN("Unable to get server manifestVersion %d", rc);
                initDb = true;
            } else if(version_out < 3) {
                LOG_WARN("Server manifest version "FMTu64" does not match! "FMTu64" Rebuild DB! ",
                          version_out, MSA_CONTROL_DB_SCHEMA_VERSION);
                initDb = true;
            } else if(version_out == 3) {
                LOG_INFO("Opened local server manifest file: %s of version:"FMTu64,
                         msaDbFile.c_str(), version_out);
                // check msa db is updated or not
                u64 updated_out;
                rc = msaControlDb.getDbUpdated(user_id, updated_out);
                if(rc != MSA_CONTROLDB_OK) {
                    LOG_WARN("Unable to get server updated %d", rc);
                    initDb = true;
                } else if (updated_out != MSA_CONTROL_DB_UPDATED) {
                    LOG_ERROR("Local DB needs to be updated: "FMTu64,
                          updated_out);
                    initDb = true;
                } else {
                    //LOG_INFO("Local DB is updated");
                    /*MSA_CONTROL_DB_SCHEMA_VERSION is 4 */
                    LOG_WARN("Server manifest version "FMTu64" does not match "FMTu64 "! Upgrade! ",
                          version_out, MSA_CONTROL_DB_SCHEMA_VERSION);
                    dbNeedsToBeUpgradedFromV3ToV4_out = true;
                }
            } else {
                LOG_INFO("Opened local server manifest file: %s of version:"FMTu64,
                         msaDbFile.c_str(), version_out);
            }

            if (dbNeedsToBeUpgradedFromV3ToV4_out) {
                msaControlDb.addPhotoAlbumRelationTableVersion4();
            }
        }
    }

    if(initDb) {
        LOG_INFO("Replacing corrupt/outdated msaDB file %s", msaDbFile.c_str());
        rc = msaControlDb.closeDB();
        if(!fileMissing){
            rc = VPLFile_Delete(msaDbFile.c_str());
            if(rc != 0) {
                LOG_WARN("Failure to unlink %s, %d", msaDbFile.c_str(), rc);
            }
        }
        rc = msaControlDb.openDB(msaDbFile);
        if(rc != 0) {
            LOG_ERROR("Opening DB %s, %d", msaDbFile.c_str(), rc);
            msaControlDb.closeDB();
            return rc;
        }
        rc = msaControlDb.initTables();
        if(rc != MSA_CONTROLDB_OK) {
            LOG_ERROR("Init Tables for %s, %d", msaDbFile.c_str(), rc);
            return rc;
        }
        rc = msaControlDb.insertDbVersion(user_id,
                                          MSA_CONTROL_DB_SCHEMA_VERSION);
        if(rc != MSA_CONTROLDB_OK) {
            LOG_ERROR("Cannot set server manifest file format version: %s to version "FMTu64", err:%d",
                      msaDbFile.c_str(), MSA_CONTROL_DB_SCHEMA_VERSION, rc);
            return rc;
        }
        rc = msaControlDb.setDbUpdated(user_id,
                                       MSA_CONTROL_DB_NOT_UPDATED);
        if (rc != MSA_CONTROLDB_OK) {
            // DB still can be populated
            LOG_WARN("Cannot set server db updated value: %s to "FMTu64", err:%d",
                      msaDbFile.c_str(), MSA_CONTROL_DB_NOT_UPDATED, rc);
        }
        // DB needs to be populated if not found or schema version does not match
        dbNeedsToBePopulated_out = true;
    } else if (dbNeedsToBeUpgradedFromV3ToV4_out) {
        rc = msaControlDb.setDbUpdated(user_id,
                                       MSA_CONTROL_DB_NOT_UPDATED);
        if (rc != MSA_CONTROLDB_OK) {
            // DB still can be populated
            LOG_WARN("Cannot set server db updated value: %s to "FMTu64", err:%d",
                      msaDbFile.c_str(), MSA_CONTROL_DB_NOT_UPDATED, rc);
        }
    }

    return MSA_CONTROLDB_OK;
}


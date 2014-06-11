//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "McaDb.hpp"

#include <vpl_plat.h>
#include <vpl_error.h>
#include <vpl_fs.h>

#include "db_util_access_macros.hpp"
#include "media_metadata_errors.hpp"
#include "MediaDbUtil.hpp"
#include "util_open_db_handle.hpp"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <vplex_assert.h>
#include <vplex_file.h>
#include <vplu_format.h>
#include <vplu_types.h>
#include <list>

#include <log.h>

static const char SQL_CREATE_MCA_TABLES[] =
"CREATE TABLE IF NOT EXISTS version ("
        "id INTEGER PRIMARY KEY,"
        "user_id INTEGER,"
        "mca_schema_version INTEGER,"
        "initialized INTEGER);"
"CREATE TABLE IF NOT EXISTS collections ("
        "cloud_pc_id INTEGER,"
        "collection_id TEXT,"
        "catalog_type INTEGER,"
        "timestamp INTEGER,"
        "PRIMARY KEY(cloud_pc_id, collection_id));"
"CREATE TABLE IF NOT EXISTS music_tracks ("
        "object_id TEXT,"
        "cloud_pc_id_ref INTEGER,"
        "collection_id_ref TEXT,"
        "media_source INTEGER,"
        "absolute_path TEXT,"
        "title TEXT,"
        "artist TEXT,"
        "album_id_ref TEXT,"
        "album_name TEXT,"
        "album_artist TEXT,"
        "track_number INTEGER,"
        "genre TEXT,"
        "duration_sec INTEGER,"
        "file_size INTEGER,"
        "date_time INTEGER,"
        "file_format TEXT,"
        "date_time_updated INTEGER,"
        "checksum TEXT,"
        "length INTEGER,"
        "composer TEXT,"
        "disk_number TEXT,"
        "year TEXT,"
        "PRIMARY KEY(object_id, cloud_pc_id_ref));"
"CREATE INDEX IF NOT EXISTS music_tracks_index ON music_tracks(title, track_number);"
"CREATE TABLE IF NOT EXISTS music_albums ("
        "object_id TEXT,"
        "cloud_pc_id_ref INTEGER,"
        "collection_id_ref TEXT,"
        "media_source INTEGER,"
        "album_name TEXT,"
        "album_artist TEXT,"
        "album_thumbnail TEXT,"
        "PRIMARY KEY(object_id, cloud_pc_id_ref));"
"CREATE INDEX IF NOT EXISTS music_albums_index ON music_albums(album_name);"
"CREATE TABLE IF NOT EXISTS videos ("
        "object_id TEXT,"
        "cloud_pc_id_ref INTEGER,"
        "collection_id_ref TEXT,"
        "media_source INTEGER,"
        "absolute_path TEXT,"
        "title TEXT,"
        "thumbnail TEXT,"
        "album_name TEXT,"
        "duration_sec INTEGER,"
        "file_size INTEGER,"
        "date_time INTEGER,"
        "file_format TEXT,"
        "date_time_updated INTEGER,"
        "PRIMARY KEY(object_id, cloud_pc_id_ref));"
"CREATE INDEX IF NOT EXISTS videos_index ON videos(title);"
"CREATE TABLE IF NOT EXISTS photos ("
        "object_id TEXT,"
        "cloud_pc_id_ref INTEGER,"
        "collection_id_ref TEXT,"
        "media_source INTEGER,"
        "absolute_path TEXT,"
        "title TEXT,"
        "thumbnail TEXT,"
        "album_name TEXT,"
        "file_size INTEGER,"
        "date_time INTEGER,"
        "file_format TEXT,"
        "date_time_updated INTEGER,"
        "dimensions TEXT,"
        "orientation INTEGER,"
        "comp_id TEXT,"          //version 11
        "special_format_flag INTEGER," //version 11
        "PRIMARY KEY(object_id, cloud_pc_id_ref));"
"CREATE INDEX IF NOT EXISTS photos_index ON photos(title);"
//   Version 12 Start //
"CREATE TABLE IF NOT EXISTS photo_albums ("
        "object_id TEXT,"
        "cloud_pc_id_ref INTEGER,"
        "collection_id_ref TEXT,"
        "media_source INTEGER,"
        "album_name TEXT,"
        "album_thumbnail TEXT,"
        "timestamp INTEGER,"
        "PRIMARY KEY(object_id, cloud_pc_id_ref));"
"CREATE TABLE IF NOT EXISTS photos_albums_relation ("
      "object_id TEXT,"
      "album_id_ref TEXT,"
      "cloud_pc_id_ref INTEGER,"
      "PRIMARY KEY(object_id, album_id_ref, cloud_pc_id_ref));"
"CREATE INDEX IF NOT EXISTS relation_index ON photos_albums_relation(album_id_ref);";
//   Version 12 End //


McaControlDB::McaControlDB()
:  m_db(NULL)
{
}

MMError McaControlDB::openDB(const std::string &dbpath)
{
    if (m_db && (dbpath==m_dbpath)) {
        return MCA_CONTROLDB_DB_ALREADY_OPEN;
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
        return MCA_CONTROLDB_DB_OPEN_FAIL;
    }

    return MCA_CONTROLDB_OK;
}

MMError McaControlDB::closeDB()
{
    if (m_db) {
        int rc = sqlite3_close(m_db);
        if(rc != 0) {
            LOG_ERROR("Error closing db:%d", rc);
        }
        m_db = NULL;
    }

    return MCA_CONTROLDB_OK;
}

McaControlDB::~McaControlDB()
{
    (void) closeDB();
}

MMError McaControlDB::initTables()
{
    char *errmsg = NULL;
    int rc;
    if(m_db) {
        // make sure db has needed tables defined
        rc = sqlite3_exec(m_db, SQL_CREATE_MCA_TABLES, NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            LOG_ERROR("Failed to create tables: %d: %s", rc, errmsg);
            sqlite3_free(errmsg);
            return MCA_CONTROLDB_DB_CREATE_TABLE_FAIL;
        }
        return MCA_CONTROLDB_OK;
    }
    return MCA_CONTROLDB_DB_NOT_OPEN;
}

MMError McaControlDB::beginTransaction()
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

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

MMError McaControlDB::commitTransaction()
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

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

// Not supported due to journaling turned off.
//MMError McaControlDB::rollbackTransaction()
//{
//    int rc;
//    int rv = MCA_CONTROLDB_OK;
//
//    static const char* SQL_ROLLBACK = "ROLLBACK";
//    sqlite3_stmt *stmt = NULL;
//    rc = sqlite3_prepare_v2(m_db, SQL_ROLLBACK, -1, &stmt, NULL);
//    CHECK_PREPARE(rv, rc, m_db, end);
//
//    rc = sqlite3_step(stmt);
//    CHECK_STEP(rv, rc, m_db, end);
//
// end:
//    FINALIZE_STMT(rv, rc, m_db, stmt);
//    return rv;
//}

MMError McaControlDB::updateContentDirectoryObject(u64 cloudPcId,
                                                   const std::string& collectionId,
                                                   const media_metadata::ContentDirectoryObject& cdo)
{
    if(cdo.has_music_track()) {
        return updateMusicTrack(cloudPcId, collectionId, cdo);
    }else if(cdo.has_music_album()) {
        return updateMusicAlbum(cloudPcId, collectionId, cdo);
    }else if(cdo.has_video_item()) {
        return updateVideo(cloudPcId, collectionId, cdo);
    }else if(cdo.has_photo_item()) {
        return updatePhoto(cloudPcId, collectionId, cdo);
    }else if(cdo.has_photo_album()) {
        return updatePhotoAlbum(cloudPcId, collectionId, cdo);
    }

    LOG_ERROR("Not a valid CDO");
    return MM_ERR_INVALID;
}

MMError McaControlDB::updateMusicTrack(u64 cloudPcId,
                                       const std::string& collectionId,
                                       const media_metadata::ContentDirectoryObject& musicTrack)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_UPDATE_MUSIC_TRACK =
            "INSERT OR REPLACE INTO music_tracks(object_id,"
                                                "cloud_pc_id_ref,"
                                                "collection_id_ref,"
                                                "media_source,"
                                                "absolute_path,"
                                                "title,"
                                                "artist,"
                                                "album_id_ref,"
                                                "album_name,"
                                                "album_artist,"
                                                "track_number,"
                                                "genre,"
                                                "duration_sec,"
                                                "file_size,"
                                                "file_format,"
                                                "date_time,"
                                                "date_time_updated,"
                                                "checksum,"
                                                "length,"
                                                "composer,"
                                                "disk_number,"
                                                "year) "
            "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
    sqlite3_stmt *stmt = NULL;
    if(!musicTrack.has_music_track()) {
        LOG_ERROR("Not a music track:%s", musicTrack.object_id().c_str());
        return MM_ERR_INVALID;
    }

    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE_MUSIC_TRACK, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE_MUSIC_TRACK, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, musicTrack.object_id().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 2, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 3, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 4, static_cast<u64>(musicTrack.source()));
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 5, musicTrack.music_track().absolute_path().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 6, musicTrack.music_track().title().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 7, musicTrack.music_track().artist().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 8, musicTrack.music_track().album_ref().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    if(musicTrack.music_track().has_album_name()) {
        rc = sqlite3_bind_text(stmt, 9, musicTrack.music_track().album_name().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 9);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_album_artist()) {
        rc = sqlite3_bind_text(stmt, 10, musicTrack.music_track().album_artist().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 10);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_track_number()) {
        rc = sqlite3_bind_int64(stmt, 11, static_cast<u64>(musicTrack.music_track().track_number()));
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 11);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_genre()) {
        rc = sqlite3_bind_text(stmt, 12, musicTrack.music_track().genre().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 12);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_duration_sec()) {
        rc = sqlite3_bind_int64(stmt, 13, musicTrack.music_track().duration_sec());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 13);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_file_size()) {
        rc = sqlite3_bind_int64(stmt, 14, musicTrack.music_track().file_size());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 14);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_file_format()) {
        rc = sqlite3_bind_text(stmt, 15, musicTrack.music_track().file_format().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 15);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_date_time()) {
        rc = sqlite3_bind_int64(stmt, 16, musicTrack.music_track().date_time());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 16);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_date_time_updated()) {
        rc = sqlite3_bind_int64(stmt, 17, musicTrack.music_track().date_time_updated());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 17);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_checksum()) {
        rc = sqlite3_bind_text(stmt, 18, musicTrack.music_track().checksum().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 18);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_length()) {
        rc = sqlite3_bind_int64(stmt, 19, musicTrack.music_track().length());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 19);
        CHECK_BIND(rv, rc, m_db, end);
    }
    // Bug 5096
    if(musicTrack.music_track().has_composer()) {
        rc = sqlite3_bind_text(stmt, 20, musicTrack.music_track().composer().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 20);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_disk_number()) {
        rc = sqlite3_bind_text(stmt, 21, musicTrack.music_track().disk_number().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 21);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicTrack.music_track().has_year()) {
        rc = sqlite3_bind_text(stmt, 22, musicTrack.music_track().year().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 22);
        CHECK_BIND(rv, rc, m_db, end);
    }

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::updateMusicAlbum(u64 cloudPcId,
                                       const std::string& collectionId,
                                       const media_metadata::ContentDirectoryObject& musicAlbum)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_UPDATE_MUSIC_ALBUM =
            "INSERT OR REPLACE INTO music_albums(object_id,"
                                                "cloud_pc_id_ref,"
                                                "collection_id_ref,"
                                                "media_source,"
                                                "album_name,"
                                                "album_artist,"
                                                "album_thumbnail) "
            "VALUES (?,?,?,?,?,?,?)";
    sqlite3_stmt *stmt = NULL;
    if(!musicAlbum.has_music_album()) {
        LOG_ERROR("Not a music album:%s", musicAlbum.object_id().c_str());
        return MM_ERR_INVALID;
    }

    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE_MUSIC_ALBUM, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE_MUSIC_ALBUM, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, musicAlbum.object_id().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 2, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 3, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 4, static_cast<int>(musicAlbum.source()));
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 5, musicAlbum.music_album().album_name().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    if(musicAlbum.music_album().has_album_artist()) {
        rc = sqlite3_bind_text(stmt, 6, musicAlbum.music_album().album_artist().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 6);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(musicAlbum.music_album().has_album_thumbnail()) {
        rc = sqlite3_bind_text(stmt, 7, musicAlbum.music_album().album_thumbnail().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 7);
        CHECK_BIND(rv, rc, m_db, end);
    }

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);
 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::updateVideo(u64 cloudPcId,
                                  const std::string& collectionId,
                                  const media_metadata::ContentDirectoryObject& video)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_UPDATE_VIDEO =
            "INSERT OR REPLACE INTO videos(object_id,"
                                          "cloud_pc_id_ref,"
                                          "collection_id_ref,"
                                          "media_source,"
                                          "absolute_path,"
                                          "title,"
                                          "thumbnail,"
                                          "album_name,"
                                          "duration_sec,"
                                          "file_size,"
                                          "date_time,"
                                          "file_format,"
                                          "date_time_updated) "
            "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)";
    sqlite3_stmt *stmt = NULL;
    if(!video.has_video_item()) {
        LOG_ERROR("Not a video:%s", video.object_id().c_str());
        return MM_ERR_INVALID;
    }

    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE_VIDEO, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE_VIDEO, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, video.object_id().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 2, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 3, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 4, static_cast<int>(video.source()));
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 5, video.video_item().absolute_path().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 6, video.video_item().title().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    if(video.video_item().has_thumbnail()) {
        rc = sqlite3_bind_text(stmt, 7, video.video_item().thumbnail().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 7);
        CHECK_BIND(rv, rc, m_db, end);
    }
    rc = sqlite3_bind_text(stmt, 8, video.video_item().album_name().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    if(video.video_item().has_duration_sec()) {
        rc = sqlite3_bind_int64(stmt, 9, video.video_item().duration_sec());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 9);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(video.video_item().has_file_size()) {
        rc = sqlite3_bind_int64(stmt, 10, video.video_item().file_size());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 10);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(video.video_item().has_date_time()) {
        rc = sqlite3_bind_int64(stmt, 11, video.video_item().date_time());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 11);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(video.video_item().has_file_format()) {
        rc = sqlite3_bind_text(stmt, 12, video.video_item().file_format().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 12);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if(video.video_item().has_date_time_updated()) {
        rc = sqlite3_bind_int64(stmt, 13, video.video_item().date_time_updated());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 13);
        CHECK_BIND(rv, rc, m_db, end);
    }

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);
 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::updatePhoto(u64 cloudPcId,
                                  const std::string& collectionId,
                                  const media_metadata::ContentDirectoryObject& photo)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;
    std::string albumId;

    static const char* SQL_UPDATE_PHOTO =
            "INSERT OR REPLACE INTO photos(object_id,"
                                          "cloud_pc_id_ref,"
                                          "collection_id_ref,"
                                          "media_source,"
                                          "absolute_path,"
                                          "title,"
                                          "thumbnail,"
                                          "album_name,"
                                          "file_size,"
                                          "date_time,"
                                          "file_format,"
                                          "date_time_updated,"
                                          "dimensions,"
                                          "orientation,"
                                          "comp_id,"
                                          "special_format_flag"
                                          ") "
            "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
    sqlite3_stmt *stmt = NULL;
    if(!photo.has_photo_item()) {
        LOG_ERROR("Not a video:%s", photo.object_id().c_str());
        return MM_ERR_INVALID;
    }

    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE_PHOTO, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE_PHOTO, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, photo.object_id().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 2, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 3, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 4, static_cast<int>(photo.source()));
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 5, photo.photo_item().absolute_path().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 6, photo.photo_item().title().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    if(photo.photo_item().has_thumbnail()) {
        rc = sqlite3_bind_text(stmt, 7, photo.photo_item().thumbnail().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 7);
        CHECK_BIND(rv, rc, m_db, end);
    }
    rc = sqlite3_bind_text(stmt, 8, photo.photo_item().album_name().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    if(photo.photo_item().has_file_size()) {
        rc = sqlite3_bind_int64(stmt, 9, photo.photo_item().file_size());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 9);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if (photo.photo_item().has_date_time()) {
        rc = sqlite3_bind_int64(stmt, 10, photo.photo_item().date_time());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rv = sqlite3_bind_null(stmt, 10);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if (photo.photo_item().has_file_format()) {
        rc = sqlite3_bind_text(stmt, 11, photo.photo_item().file_format().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rv = sqlite3_bind_null(stmt, 11);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if (photo.photo_item().has_date_time_updated()) {
        rc = sqlite3_bind_int64(stmt, 12, photo.photo_item().date_time_updated());
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rv = sqlite3_bind_null(stmt, 12);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if (photo.photo_item().has_dimensions()) {
        rc = sqlite3_bind_text(stmt, 13, photo.photo_item().dimensions().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rv = sqlite3_bind_null(stmt, 13);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if (photo.photo_item().has_orientation()) {
        // BUG 2976: orientation should be 0-359
        google::protobuf::uint64 orientation = (google::protobuf::uint64)photo.photo_item().orientation() % 360;
        rc = sqlite3_bind_int64(stmt, 14, orientation);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rv = sqlite3_bind_null(stmt, 14);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if (photo.photo_item().has_comp_id()) {
        rc = sqlite3_bind_text(stmt, 15, photo.photo_item().comp_id().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rv = sqlite3_bind_null(stmt, 15);
        CHECK_BIND(rv, rc, m_db, end);
    }
    if (photo.photo_item().has_special_format_flag()) {
        google::protobuf::uint64 special_format_flag = (google::protobuf::uint64)photo.photo_item().special_format_flag();
        rc = sqlite3_bind_int64(stmt, 16, special_format_flag);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_int64(stmt, 16, 0);
        CHECK_BIND(rv, rc, m_db, end);
    }
    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);

    if (rv == 0) {

        if (photo.photo_item().has_album_ref()) {
            albumId = photo.photo_item().album_ref();
        }else{
            // old style metadata file
            albumId = collectionId;
        }

        rv = updatePhotoAlbumRelation(cloudPcId, photo.object_id(), albumId);
    }
    return rv;
}

MMError McaControlDB::updatePhotoAlbumRelation( u64 cloudPcId,
                                                const std::string& photoObjectId,
                                                const std::string& albumIdRef)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_UPDATE =
        "INSERT OR REPLACE INTO photos_albums_relation(object_id, album_id_ref, cloud_pc_id_ref) VALUES (?,?,?)";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, photoObjectId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 2, albumIdRef.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 3, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::updatePhotoAlbum(u64 cloudPcId,
                                      const std::string& collectionId,
                                      const media_metadata::ContentDirectoryObject& photoAlbum)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_UPDATE_PHOTO_ALBUM =
            "INSERT OR REPLACE INTO photo_albums(object_id,"
                                                "cloud_pc_id_ref,"
                                                "collection_id_ref,"
                                                "media_source,"
                                                "album_name,"
                                                "timestamp,"
                                                "album_thumbnail) "
            "VALUES (?,?,?,?,?,?,?)";
    sqlite3_stmt *stmt = NULL;
    if(!photoAlbum.has_photo_album()) {
        LOG_ERROR("Not a photo album:%s", photoAlbum.object_id().c_str());
        return MM_ERR_INVALID;
    }

    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE_PHOTO_ALBUM, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE_PHOTO_ALBUM, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, photoAlbum.object_id().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 2, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 3, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 4, static_cast<int>(photoAlbum.source()));
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_text(stmt, 5, photoAlbum.photo_album().album_name().c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 6, static_cast<int>(photoAlbum.photo_album().timestamp()));
    CHECK_BIND(rv, rc, m_db, end);
    if(photoAlbum.photo_album().has_album_thumbnail()) {
        rc = sqlite3_bind_text(stmt, 7, photoAlbum.photo_album().album_thumbnail().c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);
    }else{
        rc = sqlite3_bind_null(stmt, 7);
        CHECK_BIND(rv, rc, m_db, end);
    }

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);
 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::deleteMusicTracks(u64 cloudPcId, const std::string& collectionId)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_DELETE_MUSIC_TRACKS_BY_COLLECTION_ID =
            "DELETE FROM music_tracks WHERE cloud_pc_id_ref=? AND collection_id_ref=?";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_MUSIC_TRACKS_BY_COLLECTION_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_MUSIC_TRACKS_BY_COLLECTION_ID, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::deleteMusicAlbums(u64 cloudPcId, const std::string& collectionId)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_DELETE_MUSIC_ALBUMS_BY_COLLECTION_ID =
            "DELETE FROM music_albums WHERE cloud_pc_id_ref=? AND collection_id_ref=?";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_MUSIC_ALBUMS_BY_COLLECTION_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_MUSIC_ALBUMS_BY_COLLECTION_ID, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::deleteVideos(u64 cloudPcId, const std::string& collectionId)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_DELETE_VIDEOS_BY_COLLECTION_ID =
            "DELETE FROM videos WHERE cloud_pc_id_ref=? AND collection_id_ref=?";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_VIDEOS_BY_COLLECTION_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_VIDEOS_BY_COLLECTION_ID, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::deletePhotos(u64 cloudPcId, const std::string& collectionId)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_DELETE_PHOTOES_BY_COLLECTION_ID =
            "DELETE FROM photos WHERE cloud_pc_id_ref=? AND collection_id_ref=?";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_PHOTOES_BY_COLLECTION_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_PHOTOES_BY_COLLECTION_ID, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}
#include <queue>
MMError McaControlDB::deletePhotosByCollectionId(u64 cloudPcId, const std::string& collection_id)
{
    int rc = 0;
    MMError rv = MSA_CONTROLDB_OK;
    static const char* SQL_SELECT_ALBUM_BY_COLLECTION_ID =
            "SELECT object_id FROM photo_albums WHERE cloud_pc_id_ref=? AND collection_id_ref=? ";
    sqlite3_stmt *stmt = NULL;
    std::queue<std::string> albumIds;

    rc = sqlite3_prepare_v2(m_db, SQL_SELECT_ALBUM_BY_COLLECTION_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_SELECT_ALBUM_BY_COLLECTION_ID, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, collection_id.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string albumId;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            albumId = reinterpret_cast<const char*>(
                           sqlite3_column_text(stmt, resIndex));
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MSA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

        albumIds.push(albumId);
    }

    while(albumIds.size() > 0) {
        std::string albumId = albumIds.front();
        albumIds.pop();
        LOG_INFO("Delete album data: %s", albumId.c_str());
        rv = deletePhotosByAlbumId(cloudPcId, albumId);
        if (rv != 0) {
            LOG_ERROR("Delete album %s failed: %d", albumId.c_str(), rv);
        }
    }

    if (rv == 0) {
        static const char* SQL_DELETE_COLLECTION =
            "DELETE FROM photo_albums WHERE cloud_pc_id_ref=? AND collection_id_ref=? ";

        rc = sqlite3_prepare_v2(m_db, SQL_DELETE_COLLECTION, -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, SQL_DELETE_COLLECTION, m_db, end);

        rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
        CHECK_BIND(rv, rc, m_db, end);

        rc = sqlite3_bind_text(stmt, 2, collection_id.c_str(), -1, SQLITE_STATIC);
        CHECK_BIND(rv, rc, m_db, end);

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);
    }
end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::deletePhotosByAlbumId(u64 cloudPcId, const std::string& album_id)
{
    int rc = 0;
    MMError rv = MSA_CONTROLDB_OK;

    static const char* SQL_DELETE_RELATION = "DELETE FROM photos_albums_relation WHERE cloud_pc_id_ref=? AND album_id_ref=?";
    static const char* SQL_DELETE_PHOTOS = "DELETE FROM photos "
                                           "WHERE object_id NOT IN (SELECT object_id FROM photos_albums_relation) AND cloud_pc_id_ref=? ";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_RELATION, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_RELATION, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, album_id.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_PHOTOS, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_PHOTOS, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::insertMcaDbVersion(u64 user_id,
                                         u64 version_id)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    sqlite3_stmt *stmt = NULL;

    static const char* SQL_SET_MANIFEST_VERSION =
            "INSERT OR REPLACE INTO version (id,"
                                            "user_id,"
                                            "mca_schema_version) "
            "VALUES (?, ?, ?)";
    rc = sqlite3_prepare_v2(m_db, SQL_SET_MANIFEST_VERSION, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_SET_MANIFEST_VERSION, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, 1);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 2, user_id);
    CHECK_BIND(rv, rc, m_db, end);
    rc = sqlite3_bind_int64(stmt, 3, version_id);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::getMcaDbVersion(u64 user_id,
                                      u64 &version_out)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;
    version_out=0;

    static const char* SQL_GET_MANIFEST_VERSION =
            "SELECT mca_schema_version "
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
        rv = MCA_CONTROLDB_DB_NO_VERSION;
        goto end;
    }else if(rc != SQLITE_ROW) {
        LOG_ERROR("Error: %d", rc);
        rv = MCA_CONTROLDB_DB_INTERNAL_ERR;
        goto end;
    }
    // rv == SQLITE_ROW
    if (sqlite3_column_type(stmt, 0) != SQLITE_INTEGER) {
        rv = MCA_CONTROLDB_DB_BAD_VALUE;
        goto end;
    }
    version_out = sqlite3_column_int64(stmt, 0);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::setMcaDbInitialized(bool isInit)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    sqlite3_stmt *stmt = NULL;
    int bindPos=0;  // Left-most position is 1

    static const char* SQL_UPDATE_INITIALIZED =
            "UPDATE version SET initialized=? WHERE id=1";
    rc = sqlite3_prepare_v2(m_db, SQL_UPDATE_INITIALIZED, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_UPDATE_INITIALIZED, m_db, end);

    if(isInit) {
        rc = sqlite3_bind_int64(stmt, ++bindPos, 1);
        CHECK_BIND(rv, rc, m_db, end);
    } else {
        rc = sqlite3_bind_int64(stmt, ++bindPos, 0);
        CHECK_BIND(rv, rc, m_db, end);
    }

    ASSERT(bindPos==sqlite3_bind_parameter_count(stmt));

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::getMcaDbInitialized(bool& isInit_out)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;
    bool isInit_exists_unused = false;
    isInit_out=false;

    static const char* SQL_GET_INITIALIZED =
            "SELECT initialized "
            "FROM version "
            "WHERE id=1";
    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(m_db, SQL_GET_INITIALIZED, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_INITIALIZED, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

    if (rc == SQLITE_DONE) {
        goto end;
    }else if(rc != SQLITE_ROW) {
        LOG_ERROR("Error: %d", rc);
        rv = MCA_CONTROLDB_DB_INTERNAL_ERR;
        goto end;
    }
    ASSERT(rc == SQLITE_ROW);

    DB_UTIL_GET_SQLITE_BOOL_NULL(stmt, isInit_exists_unused, isInit_out, 0, rv, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::getCloudPcIds(std::vector<u64>& cloudPcIds_out)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    cloudPcIds_out.clear();

    static const char* SQL_GET_CLOUD_PC_IDS =
            "SELECT DISTINCT cloud_pc_id FROM collections";
    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_prepare_v2(m_db, SQL_GET_CLOUD_PC_IDS, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_CLOUD_PC_IDS, m_db, end);

    while(rv == 0) {
        u64 cloudpc_id = 0;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        // cloud_pc_id
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_INTEGER) {
            cloudpc_id = static_cast<u64>(sqlite3_column_int64(stmt, resIndex));
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        cloudPcIds_out.push_back(cloudpc_id);
    }

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

static int catalogTypeToInt(media_metadata::CatalogType_t type)
{
    int toReturn = -1;
    switch(type)
    {
    case media_metadata::MM_CATALOG_MUSIC:
        toReturn = 1;
        break;
    case media_metadata::MM_CATALOG_PHOTO:
        toReturn = 2;
        break;
    case media_metadata::MM_CATALOG_VIDEO:
        toReturn = 3;
        break;
    default:
        LOG_ERROR("Should never happen:%d", (int)type);
        break;
    }
    return toReturn;
}

MMError McaControlDB::getCollections(u64 cloudPcId,
                                     media_metadata::CatalogType_t catType,
                                     CollectionIdAndTimestampMap& map_out)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    map_out.clear();

    static const char* SQL_GET_COLLECTIONS =
            "SELECT collection_id, timestamp "
            "FROM collections "
            "WHERE cloud_pc_id=? AND catalog_type=?";
    sqlite3_stmt *stmt = NULL;
    int catIntType;
    rc = sqlite3_prepare_v2(m_db, SQL_GET_COLLECTIONS, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_COLLECTIONS, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    catIntType = catalogTypeToInt(catType);
    rc = sqlite3_bind_int64(stmt, 2, catIntType);
    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string collectionId;
        u64 timestamp = 0;
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
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

        // timestamp
        resIndex++;
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_INTEGER) {
            timestamp = sqlite3_column_int64(stmt, resIndex);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

        map_out[collectionId] = timestamp;
    }

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::updateCollection(u64 cloudPcId,
                                       media_metadata::CatalogType_t catType,
                                       const std::string& collectionId,
                                       u64 timestamp)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    static const char* SQL_SET_COLLECTION =
            "INSERT OR REPLACE INTO collections (collection_id,"
                                                "timestamp,"
                                                "cloud_pc_id,"
                                                "catalog_type) "
            "VALUES (?,?,?,?)";

    sqlite3_stmt *stmt = NULL;
    int catIntType;
    rc = sqlite3_prepare_v2(m_db, SQL_SET_COLLECTION, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_SET_COLLECTION, m_db, end);

    rc = sqlite3_bind_text(stmt, 1, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_int64(stmt, 2, timestamp);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_int64(stmt, 3, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    catIntType = catalogTypeToInt(catType);
    rc = sqlite3_bind_int64(stmt, 4, catIntType);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::deleteCollection(u64 cloudPcId,
                                       media_metadata::CatalogType_t type,
                                       const std::string& collectionId)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;


    switch(type) {
    case media_metadata::MM_CATALOG_MUSIC:
    rc = deleteMusicTracks(cloudPcId, collectionId);
    if(rc != MCA_CONTROLDB_OK) {
        LOG_ERROR("deleteMusicTracks for %s @ "FMTx64" error:%d",
                  collectionId.c_str(), cloudPcId, rc);
        rv = rc;
    }
    rc = deleteMusicAlbums(cloudPcId, collectionId);
    if(rc != MCA_CONTROLDB_OK) {
        LOG_ERROR("deleteMusicAlbums for %s @ "FMTx64" error:%d",
                  collectionId.c_str(), cloudPcId, rc);
        rv = rc;
    }
    break;

    case media_metadata::MM_CATALOG_PHOTO:
    //rc = deletePhotos(cloudPcId, collectionId);
    rc = deletePhotosByCollectionId(cloudPcId, collectionId);
    if(rc != MCA_CONTROLDB_OK) {
        LOG_ERROR("deletePhotos for %s @ "FMTx64" error:%d",
                  collectionId.c_str(), cloudPcId, rc);
        rv = rc;
    }
    break;

    case media_metadata::MM_CATALOG_VIDEO:
    rc = deleteVideos(cloudPcId, collectionId);
    if(rc != MCA_CONTROLDB_OK) {
        LOG_ERROR("deleteVideos for %s @ "FMTx64" error:%d",
                  collectionId.c_str(), cloudPcId, rc);
        rv = rc;
    }
    break;

    default:
        LOG_ERROR("Should never happen:%d", (int)type);
        break;
    }

    static const char* SQL_DELETE_COLLECTION =
            "DELETE FROM collections "
            "WHERE cloud_pc_id=? AND collection_id=? AND catalog_type=?";
    sqlite3_stmt *stmt = NULL;
    int catIntType;
    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_COLLECTION, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_COLLECTION, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_bind_text(stmt, 2, collectionId.c_str(), -1, SQLITE_STATIC);
    CHECK_BIND(rv, rc, m_db, end);

    catIntType = catalogTypeToInt(type);
    rc = sqlite3_bind_int64(stmt, 3, catIntType);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::getAllObjectIdsSorted(
        google::protobuf::RepeatedPtrField< media_metadata::ContentDirectoryObject > &cdos)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    cdos.Clear();

    static const char* SQL_GET_SORTED_OBJECT_IDS =
            "SELECT object_id, some_text FROM "
            "(SELECT object_id, title AS some_text FROM music_tracks "
                "UNION "
             "SELECT object_id, album_name AS some_text FROM music_albums "
                "UNION "
             "SELECT object_id, title AS some_text FROM videos "
                "UNION "
             "SELECT object_id, title AS some_text FROM photos "
            ") ORDER BY object_id";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_GET_SORTED_OBJECT_IDS, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_SORTED_OBJECT_IDS, m_db, end);

    // One can bind variables here if needed (see other SQL queries with '?' literal)
    // Also can look at the getCollections (above) as another example.
    // In this particular query, there is no variable to bind.
    //    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);  // stmt, bind variable # (not zero indexed), value
    //    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string objectId;
        std::string someText;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        // object_id
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            objectId = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, resIndex));
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

        // some_text
        resIndex++;
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            someText = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, resIndex));
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

        media_metadata::ContentDirectoryObject* tempCdo = cdos.Add();
        tempCdo->set_object_id(objectId);
        tempCdo->set_source(media_metadata::MEDIA_SOURCE_LIBRARY);
        tempCdo->add_optional_fields(someText);
    }

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::getMusicTracks(u64 cloudPcId,
                                     const std::string& searchQuery,
                                     const std::string& orderQuery,
                                     google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject > & output)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    output.Clear();

    std::string SQL_GET_MUSICTRACKS =
            "SELECT object_id, collection_id_ref, media_source, absolute_path, "
            "title, artist, album_id_ref, album_name, album_artist, track_number, "
            "genre, duration_sec, file_size, date_time, file_format, date_time_updated, checksum, length, composer, disk_number, year "
            "FROM music_tracks "
            "WHERE cloud_pc_id_ref=? ";
    sqlite3_stmt *stmt = NULL;
    
    if (searchQuery.size() > 0)
        SQL_GET_MUSICTRACKS.append("AND (" + searchQuery + ") ");
    if (orderQuery.size() > 0)
        SQL_GET_MUSICTRACKS.append("ORDER BY " + orderQuery + " ");
    rc = sqlite3_prepare_v2(m_db, SQL_GET_MUSICTRACKS.c_str(), -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_MUSICTRACKS.c_str(), m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string object_id, collection_id, abs_path, title, artist, album_id_ref, album_name, album_artist, genre, file_format, checksum, composer, disk_number, year;
        u64 media_source=0, track_number=0, duration_sec=0, file_size=0, date_time=0, date_time_updated=0, length=0;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        media_metadata::MCAMetadataQueryObject* obj = output.Add();

        // object_id
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            object_id = reinterpret_cast<const char*>(
                        sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->set_object_id(object_id);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // collection_id
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            collection_id = reinterpret_cast<const char*>(
                        sqlite3_column_text(stmt, resIndex));
            obj->set_collection_id(collection_id);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // media_source
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            media_source = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->set_source((media_metadata::MediaSource_t)media_source);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // absolute_path
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            abs_path = reinterpret_cast<const char*>(
                       sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_absolute_path(abs_path);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // title
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            title = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_title(title);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // artist
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            artist = reinterpret_cast<const char*>(
                     sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_artist(artist);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // album_id_ref
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_id_ref = reinterpret_cast<const char*>(
                           sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_album_ref(album_id_ref);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        //album_name 
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_name = reinterpret_cast<const char*>(
                         sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_album_name(album_name);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // album_artist
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_artist = reinterpret_cast<const char*>(
                           sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_album_artist(album_artist);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // track_number
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            track_number = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_music_track()->set_track_number((google::protobuf::int32)track_number);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // genre
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            genre = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_genre(genre);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // duration_sec
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            duration_sec = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_music_track()->set_duration_sec(duration_sec);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // file_size
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            file_size = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_music_track()->set_file_size(file_size);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // date_time
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            date_time = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_music_track()->set_date_time(date_time);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // file_format 
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            file_format = reinterpret_cast<const char*>(
                          sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_file_format(file_format);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // date_time_updated
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            date_time_updated = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_music_track()->set_date_time_updated(date_time_updated);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // checksum
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            checksum = reinterpret_cast<const char*>(
                       sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_checksum(checksum);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // length
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            length = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_music_track()->set_length(length);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // Bug 5096
        // composer
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            composer = reinterpret_cast<const char*>(
                           sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_composer(composer);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // disk_number
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            disk_number = reinterpret_cast<const char*>(
                           sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_disk_number(disk_number);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // year
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            year = reinterpret_cast<const char*>(
                           sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_track()->set_year(year);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
    }
 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::getMusicAlbums(u64 cloudPcId,
                                     const std::string& searchQuery,
                                     const std::string& orderQuery,
                                     google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject > & output)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    output.Clear();

    std::string SQL_GET_MUSICALBUMS =
            "SELECT object_id, collection_id_ref, media_source, album_name, album_artist, album_thumbnail "
            "FROM music_albums "
            "WHERE cloud_pc_id_ref=? ";

    std::string SQL_GET_MUSICALBUM_TRACKSINFO =
        "SELECT collection_id_ref, COUNT(DISTINCT object_id), SUM(file_size) "
        "FROM music_tracks "
        "WHERE cloud_pc_id_ref=? "
        "GROUP BY collection_id_ref ";

    sqlite3_stmt *stmt = NULL;

    sqlite3_stmt *stmt_tracks_info = NULL;
    
    if (searchQuery.size() > 0)
        SQL_GET_MUSICALBUMS.append("AND (" + searchQuery + ") ");
    if (orderQuery.size() > 0)
        SQL_GET_MUSICALBUMS.append("ORDER BY " + orderQuery);
    rc = sqlite3_prepare_v2(m_db, SQL_GET_MUSICALBUMS.c_str(), -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_MUSICALBUMS.c_str(), m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string object_id, collection_id, album_name, album_artist, album_thumbnail;
        u64 media_source;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        media_metadata::MCAMetadataQueryObject* obj = output.Add();

        // object_id
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            object_id = reinterpret_cast<const char*>(
                        sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->set_object_id(object_id);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // collection_id
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            collection_id = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            obj->set_collection_id(collection_id);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // media_source
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            media_source = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->set_source((media_metadata::MediaSource_t)media_source);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // album_name
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_name = reinterpret_cast<const char*>(
                         sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_album()->set_album_name(album_name);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // album_artist
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_artist = reinterpret_cast<const char*>(
                           sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_album()->set_album_artist(album_artist);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // album_thumbnail
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_thumbnail = reinterpret_cast<const char*>(
                              sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_music_album()->set_album_thumbnail(album_thumbnail);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
    }
    /// calculate album_trackcount and album_tracksize
    rc = sqlite3_prepare_v2(m_db, SQL_GET_MUSICALBUM_TRACKSINFO.c_str(), -1, &stmt_tracks_info, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_MUSICALBUM_TRACKSINFO.c_str(), m_db, end);

    rc = sqlite3_bind_int64(stmt_tracks_info, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string collection_id;
        u64 album_trackcount, album_tracksize;
        int resIndex_info = 0;
        int sqlType;

        rc = sqlite3_step(stmt_tracks_info);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject >::iterator it;
        //media_metadata::MCAMetadataQueryObject* obj = output.Add();
        
        // collection_id
        sqlType = sqlite3_column_type(stmt_tracks_info, resIndex_info);
        if (sqlType == SQLITE_TEXT) {
            collection_id = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt_tracks_info, resIndex_info));
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex_info);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        for (it = output.begin(); it != output.end(); ++it) {
            if (collection_id.compare(it->collection_id()) == 0) {
                // album_trackcount
                sqlType = sqlite3_column_type(stmt_tracks_info, ++resIndex_info);
                if (sqlType == SQLITE_INTEGER) {
                    album_trackcount = sqlite3_column_int64(stmt_tracks_info, resIndex_info);
                    it->mutable_cdo()->mutable_music_album()->set_album_trackcount((google::protobuf::uint32)album_trackcount);
                }else if(sqlType == SQLITE_NULL) {
                    it->mutable_cdo()->mutable_music_album()->set_album_trackcount(0);
                }else{
                    LOG_ERROR("Bad column type index:%d", resIndex_info);
                    album_trackcount = 0;
                }
                // album_tracksize
                sqlType = sqlite3_column_type(stmt_tracks_info, ++resIndex_info);
                if (sqlType == SQLITE_INTEGER) {
                    album_tracksize = sqlite3_column_int64(stmt_tracks_info, resIndex_info);
                    it->mutable_cdo()->mutable_music_album()->set_album_tracksize(album_tracksize);
                }else if(sqlType == SQLITE_NULL) {
                    it->mutable_cdo()->mutable_music_album()->set_album_tracksize(0);
                }else{
                    LOG_ERROR("Bad column type index:%d", resIndex_info);
                    album_tracksize = 0;
                }
                break;
            }
        }
    }
 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    FINALIZE_STMT(rv, rc, m_db, stmt_tracks_info);
    return rv;
}

static inline
std::string Mca_u64ToString(u64 value)
{
    char tempBuf[20];
    snprintf(tempBuf, ARRAY_SIZE_IN_BYTES(tempBuf), FMTu64, value);
    return std::string(tempBuf);
}

MMError McaControlDB::getPhotos(u64 cloudPcId,
                                const std::string& searchQuery,
                                const std::string& orderQuery,
                                google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject > & output)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    output.Clear();

    std::string SQL_GET_PHOTOS =
           "SELECT photos.object_id as object_id , collection_id_ref, media_source, absolute_path, title, thumbnail, album_name, file_size, "
           " date_time, file_format, date_time_updated, dimensions, orientation, comp_id, special_format_flag, "
           " photos_albums_relation.album_id_ref as album_id_ref "
           " FROM photos, photos_albums_relation "
           "WHERE photos.object_id=photos_albums_relation.object_id AND photos_albums_relation.cloud_pc_id_ref=? ";

    sqlite3_stmt *stmt = NULL;

    if (searchQuery.size() > 0)
        SQL_GET_PHOTOS.append("AND (" + searchQuery + ") ");
    if (orderQuery.size() > 0)
        SQL_GET_PHOTOS.append("ORDER BY " + orderQuery);
    rc = sqlite3_prepare_v2(m_db, SQL_GET_PHOTOS.c_str(), -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_PHOTOS.c_str(), m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string object_id, collection_id, absolute_path, title, thumbnail, album_name, file_format, dimensions, comp_id, album_id_ref;
        u64 media_source=0, date_time=0, file_size=0, date_time_updated=0, orientation=0, special_format_flag=0;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        media_metadata::MCAMetadataQueryObject* obj = output.Add();

        // object_id
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            object_id = reinterpret_cast<const char*>(
                        sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->set_object_id(object_id);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // collection_id
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            collection_id = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            obj->set_collection_id(collection_id);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // media_source
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            media_source = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->set_source((media_metadata::MediaSource_t)media_source);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // absolute_path
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            absolute_path = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_photo_item()->set_absolute_path(absolute_path);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // title
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            title = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_photo_item()->set_title(title);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // thumbnail
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            thumbnail = reinterpret_cast<const char*>(
                        sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_photo_item()->set_thumbnail(thumbnail);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // album_name
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_name = reinterpret_cast<const char*>(
                         sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_photo_item()->set_album_name(album_name);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // file_size
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            file_size = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_photo_item()->set_file_size(file_size);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // date_time
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            date_time = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_photo_item()->set_date_time(date_time);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // file_format
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            file_format = reinterpret_cast<const char*>(
                          sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_photo_item()->set_file_format(file_format);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // date_time_updated
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            date_time_updated = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_photo_item()->set_date_time_updated(date_time_updated);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // dimensions 
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            dimensions = reinterpret_cast<const char*>(
                         sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_photo_item()->set_dimensions(dimensions);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // orientation
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            orientation = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_photo_item()->set_orientation((google::protobuf::uint32)orientation);
        }else if(sqlType == SQLITE_NULL) {
            obj->mutable_cdo()->mutable_photo_item()->set_orientation(0);
        }else {
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // comp_id
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            comp_id = reinterpret_cast<const char*>(
                          sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_photo_item()->set_comp_id(comp_id);
        }else if(sqlType != SQLITE_NULL) {
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // special_format_flag
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            special_format_flag = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_photo_item()->set_special_format_flag((google::protobuf::uint32)special_format_flag);
        }else if(sqlType == SQLITE_NULL) {
            obj->mutable_cdo()->mutable_photo_item()->set_special_format_flag(0);
        }else {
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // album_id_ref
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_id_ref = reinterpret_cast<const char*>(
                          sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_photo_item()->set_album_ref(album_id_ref);
        }else if(sqlType != SQLITE_NULL) {
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
    }
 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::getVideos(u64 cloudPcId,
                                const std::string& searchQuery,
                                const std::string& orderQuery,
                                google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject > & output)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    output.Clear();

    std::string SQL_GET_VIDEOS =
            "SELECT object_id, collection_id_ref, media_source, absolute_path, title, thumbnail, album_name, duration_sec, file_size, date_time, file_format, date_time_updated "
            "FROM videos "
            "WHERE cloud_pc_id_ref=? ";
    sqlite3_stmt *stmt = NULL;
    
    if (searchQuery.size() > 0)
        SQL_GET_VIDEOS.append("AND (" + searchQuery + ") ");
    if (orderQuery.size() > 0)
        SQL_GET_VIDEOS.append("ORDER BY " + orderQuery);
    rc = sqlite3_prepare_v2(m_db, SQL_GET_VIDEOS.c_str(), -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_VIDEOS.c_str(), m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string object_id, collection_id, absolute_path, title, thumbnail, album_name, file_format;
        u64 media_source=0, duration_sec=0, file_size=0, date_time=0, date_time_updated;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        media_metadata::MCAMetadataQueryObject* obj = output.Add();

        // object_id
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            object_id = reinterpret_cast<const char*>(
                        sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->set_object_id(object_id);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // collection_id
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            collection_id = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            obj->set_collection_id(collection_id);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // media_source
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            media_source = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->set_source((media_metadata::MediaSource_t)media_source);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // absolute_path
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            absolute_path = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_video_item()->set_absolute_path(absolute_path);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // title
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            title = reinterpret_cast<const char*>(
                    sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_video_item()->set_title(title);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // thumbnail
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            thumbnail = reinterpret_cast<const char*>(
                        sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_video_item()->set_thumbnail(thumbnail);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // album_name
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_name = reinterpret_cast<const char*>(
                         sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_video_item()->set_album_name(album_name);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // duration_sec
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            duration_sec = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_video_item()->set_duration_sec(duration_sec);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // file_size
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            file_size = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_video_item()->set_file_size(file_size);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // date_time
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            date_time = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_video_item()->set_date_time(date_time);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // file_format
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            file_format = reinterpret_cast<const char*>(
                          sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_video_item()->set_file_format(file_format);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // date_time_updated
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            date_time_updated = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_video_item()->set_date_time_updated(date_time_updated);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

    }
 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::getMusicArtist(u64 cloudPcId,
                                     const std::string& searchQuery,
                                     const std::string& orderQuery,
                                     google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject> &output)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    output.Clear();

    std::string SQL_GET_MUSICARTISTS =
            "SELECT artist, COUNT(DISTINCT collection_id_ref), COUNT(DISTINCT object_id), SUM(file_size) "
            "FROM music_tracks "
            "WHERE cloud_pc_id_ref=? ";
    sqlite3_stmt *stmt = NULL;

    if (searchQuery.size() > 0)
        SQL_GET_MUSICARTISTS.append("AND (" + searchQuery + ") ");
    SQL_GET_MUSICARTISTS.append("GROUP BY artist ");
    if (orderQuery.size() > 0)
        SQL_GET_MUSICARTISTS.append("ORDER BY " + orderQuery);
    rc = sqlite3_prepare_v2(m_db, SQL_GET_MUSICARTISTS.c_str(), -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_MUSICARTISTS.c_str(), m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string artist;
        u64 album_count=0, track_count=0, item_total_size=0;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        media_metadata::MCAMetadataQueryObject* obj = output.Add();
        
        // artist
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            artist = reinterpret_cast<const char*>(
                     sqlite3_column_text(stmt, resIndex));
            obj->mutable_music_artist()->set_artist(artist);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // album_count
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            album_count = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_music_artist()->set_album_count((google::protobuf::uint32)album_count);
        }else if(sqlType == SQLITE_NULL) {
            obj->mutable_music_artist()->set_album_count(0);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // track_count
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            track_count = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_music_artist()->set_track_count((google::protobuf::uint32)track_count);
        }else if(sqlType == SQLITE_NULL) {
            obj->mutable_music_artist()->set_track_count(0);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // item_total_size
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            item_total_size = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_music_artist()->set_item_total_size((google::protobuf::uint64)item_total_size);
        }else if(sqlType == SQLITE_NULL) {
            obj->mutable_music_artist()->set_item_total_size(0);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

    }
 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::getMusicGenre(u64 cloudPcId,
                                    const std::string& searchQuery,
                                    const std::string& orderQuery,
                                    google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject> &output)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    output.Clear();

    std::string SQL_GET_MUSICGENRE =
            "SELECT genre, COUNT(DISTINCT object_id), COUNT(DISTINCT collection_id_ref), SUM(file_size) "
            "FROM music_tracks "
            "WHERE cloud_pc_id_ref=? ";
    sqlite3_stmt *stmt = NULL;

    if (searchQuery.size() > 0)
        SQL_GET_MUSICGENRE.append("AND (" + searchQuery + ") ");
    SQL_GET_MUSICGENRE.append("GROUP BY genre ");
    if (orderQuery.size() > 0)
        SQL_GET_MUSICGENRE.append("ORDER BY " + orderQuery);
    rc = sqlite3_prepare_v2(m_db, SQL_GET_MUSICGENRE.c_str(), -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_MUSICGENRE.c_str(), m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string genre;
        u64 track_count=0, album_count=0, item_total_size=0;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        media_metadata::MCAMetadataQueryObject* obj = output.Add();
        
        // genre
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            genre = reinterpret_cast<const char*>(
                     sqlite3_column_text(stmt, resIndex));
            obj->mutable_music_genre()->set_genre(genre);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // track_count
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            track_count = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_music_genre()->set_track_count((google::protobuf::uint32)track_count);
        }else if(sqlType == SQLITE_NULL) {
            obj->mutable_music_genre()->set_track_count(0);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // album_count
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            album_count = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_music_genre()->set_album_count((google::protobuf::uint32)album_count);
        }else if(sqlType == SQLITE_NULL) {
            obj->mutable_music_genre()->set_album_count(0);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // item_total_size
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            item_total_size = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_music_genre()->set_item_total_size((google::protobuf::uint64)item_total_size);
        }else if(sqlType == SQLITE_NULL) {
            obj->mutable_music_genre()->set_item_total_size(0);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

    }
 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::getPhotoAlbums(u64 cloudPcId,
                                     const std::string& searchQuery,
                                     const std::string& orderQuery,
                                     google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject > & output)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    output.Clear();
    google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject >::iterator it;

    std::string SQL_GET_PHOTOALBUMS =
            "SELECT object_id, collection_id_ref, media_source, album_name, album_thumbnail, timestamp "
            "FROM photo_albums "
            "WHERE cloud_pc_id_ref=? ";

    const char *SQL_GET_PHOTOALBUM_PHOTOSINFO =
        "SELECT COUNT(photos.object_id) , SUM(photos.file_size) "
        " FROM photos, photos_albums_relation "
        " WHERE photos.object_id=photos_albums_relation.object_id "
        " AND photos_albums_relation.cloud_pc_id_ref="FMTu64
        " AND photos_albums_relation.album_id_ref='%s'";


    sqlite3_stmt *stmt = NULL;

    sqlite3_stmt *stmt_photos_info = NULL;

    if (searchQuery.size() > 0)
        SQL_GET_PHOTOALBUMS.append("AND (" + searchQuery + ") ");
    if (orderQuery.size() > 0)
        SQL_GET_PHOTOALBUMS.append("ORDER BY " + orderQuery);
    rc = sqlite3_prepare_v2(m_db, SQL_GET_PHOTOALBUMS.c_str(), -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_PHOTOALBUMS.c_str(), m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);
    LOG_DEBUG("SQL:%s", SQL_GET_PHOTOALBUMS.c_str());
    while(rv == 0) {
        std::string object_id, collection_id, album_name, album_thumbnail;
        u64 timestamp;
        u64 media_source;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        media_metadata::MCAMetadataQueryObject* obj = output.Add();

        // object_id
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            object_id = reinterpret_cast<const char*>(
                        sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->set_object_id(object_id);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

        // collection_id
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            collection_id = reinterpret_cast<const char*>(
                            sqlite3_column_text(stmt, resIndex));
            obj->set_collection_id(collection_id);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

        // media_source
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            media_source = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->set_source((media_metadata::MediaSource_t)media_source);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

        // album_name
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_name = reinterpret_cast<const char*>(
                         sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_photo_album()->set_album_name(album_name);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

        // album_thumbnail
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_thumbnail = reinterpret_cast<const char*>(
                              sqlite3_column_text(stmt, resIndex));
            obj->mutable_cdo()->mutable_photo_album()->set_album_thumbnail(album_thumbnail);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

        // modify timestamp
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            timestamp = (u64) sqlite3_column_int64(stmt, resIndex);
            obj->mutable_cdo()->mutable_photo_album()->set_timestamp(timestamp);
        }else if (sqlType != SQLITE_NULL){
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

    }

    for (it = output.begin(); rv == 0 && it != output.end(); ++it) {

        char sqlCmd[SQL_MAX_QUERY_LENGTH];
        u64 count = 0;
        u64 totalSize = 0;
        int resIndex_info = 0;
        int sqlType;

        sprintf(sqlCmd, SQL_GET_PHOTOALBUM_PHOTOSINFO,cloudPcId, it->mutable_cdo()->object_id().c_str() );

        LOG_DEBUG("SQL:%s", sqlCmd);

        rc = sqlite3_prepare_v2(m_db, sqlCmd, -1, &stmt_photos_info, NULL);
        CHECK_PREPARE(rv, rc, sqlCmd, m_db, end);

        rc = sqlite3_step(stmt_photos_info);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            LOG_INFO("No data for this album! collection id:%s", it->collection_id().c_str());
            continue;
        }

        // item count
        sqlType = sqlite3_column_type(stmt_photos_info, resIndex_info);
        if (sqlType == SQLITE_INTEGER) {
            count = (u64)sqlite3_column_int64(stmt_photos_info, resIndex_info);
            it->mutable_cdo()->mutable_photo_album()->set_item_count((google::protobuf::uint32)count);
        }else if(sqlType == SQLITE_NULL) {
            it->mutable_cdo()->mutable_photo_album()->set_item_count(0);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex_info);
            it->mutable_cdo()->mutable_photo_album()->set_item_count(0);
            count = 0;
        }

        // total file size
        sqlType = sqlite3_column_type(stmt_photos_info, ++resIndex_info);
        if (sqlType == SQLITE_INTEGER) {
            totalSize = (u64)sqlite3_column_int64(stmt_photos_info, resIndex_info);
            it->mutable_cdo()->mutable_photo_album()->set_item_total_size(totalSize);
        }else if(sqlType == SQLITE_NULL) {
            it->mutable_cdo()->mutable_photo_album()->set_item_total_size(0);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex_info);
            totalSize = 0;
            it->mutable_cdo()->mutable_photo_album()->set_item_total_size(0);
        }
    }

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    FINALIZE_STMT(rv, rc, m_db, stmt_photos_info);
    return rv;
}
MMError McaControlDB::getVideoAlbums(u64 cloudPcId,
                                     const std::string& searchQuery,
                                     const std::string& orderQuery,
                                     google::protobuf::RepeatedPtrField< media_metadata::MCAMetadataQueryObject> &output)
{
    int rv = MCA_CONTROLDB_OK;
    int rc;
    output.Clear();

    std::string SQL_GET_VIDEOALBUMS =
            "SELECT collection_id_ref, album_name, COUNT(DISTINCT object_id), SUM(file_size)"
            "FROM videos "
            "WHERE cloud_pc_id_ref=? ";
    sqlite3_stmt *stmt = NULL;

    if (searchQuery.size() > 0)
        SQL_GET_VIDEOALBUMS.append("AND (" + searchQuery + ") ");
    SQL_GET_VIDEOALBUMS.append("GROUP BY collection_id_ref ");
    if (orderQuery.size() > 0)
        SQL_GET_VIDEOALBUMS.append("ORDER BY " + orderQuery);
    rc = sqlite3_prepare_v2(m_db, SQL_GET_VIDEOALBUMS.c_str(), -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_GET_VIDEOALBUMS.c_str(), m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    while(rv == 0) {
        std::string collection_id_ref, album_name;
        u64 item_count=0, item_total_size=0;
        int resIndex = 0;
        int sqlType;

        rc = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, m_db, end);

        if (rc == SQLITE_DONE) {
            break;
        }

        media_metadata::MCAMetadataQueryObject* obj = output.Add();
        
        // collection_id_ref
        sqlType = sqlite3_column_type(stmt, resIndex);
        if (sqlType == SQLITE_TEXT) {
            collection_id_ref = reinterpret_cast<const char*>(
                     sqlite3_column_text(stmt, resIndex));
            obj->mutable_video_album()->set_collection_id_ref(collection_id_ref);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // album_name
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_TEXT) {
            album_name = reinterpret_cast<const char*>(
                     sqlite3_column_text(stmt, resIndex));
            obj->mutable_video_album()->set_album_name(album_name);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // item_count
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            item_count = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_video_album()->set_item_count((google::protobuf::uint32)item_count);
        }else if(sqlType == SQLITE_NULL) {
            obj->mutable_video_album()->set_item_count(0);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }
        // item_total_size
        sqlType = sqlite3_column_type(stmt, ++resIndex);
        if (sqlType == SQLITE_INTEGER) {
            item_total_size = sqlite3_column_int64(stmt, resIndex);
            obj->mutable_video_album()->set_item_total_size((google::protobuf::uint64)item_total_size);
        }else if(sqlType == SQLITE_NULL) {
            obj->mutable_video_album()->set_item_total_size(0);
        }else{
            LOG_ERROR("Bad column type index:%d", resIndex);
            rv = MCA_CONTROLDB_DB_BAD_VALUE;
            goto end;
        }

    }
 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

static void metadataDBFilename(u64 userId,
                               const std::string& metadataFileDir,
                               std::string& metadataFile_out)
{
    char user_dataset_str[64];
    sprintf(user_dataset_str, "/"FMTx64, userId);

    metadataFile_out.assign(metadataFileDir);
    metadataFile_out.append(user_dataset_str);
    metadataFile_out.append(".metadb2");
}

MMError McaControlDB::createOrOpenMetadataDB(McaControlDB& mcaControlDb,
                                             const std::string& dbDirectory,
                                             u64 user_id,
                                             bool& dbNeedsToBePopulated_out,
                                             bool& dbNeedsToBeUpgradedFromV10ToV11_out,
                                             bool& dbNeedsToBeUpgradedFromV11ToV12_out)
{
    int rc;
    bool initDb = false;
    bool fileMissing = false;
    dbNeedsToBePopulated_out = false;
    dbNeedsToBeUpgradedFromV10ToV11_out = false;
    dbNeedsToBeUpgradedFromV11ToV12_out = false;

    std::string mcaDbFile;
    metadataDBFilename(user_id,
                       dbDirectory,
                       mcaDbFile);

    VPLFS_stat_t statBuf;
    rc = VPLFS_Stat(mcaDbFile.c_str(), &statBuf);
    if (rc != VPL_OK) {
        LOG_INFO("Missing manifest file.  Starting replace for %s.", mcaDbFile.c_str());
        initDb = true;
        fileMissing = true;
    } else {
        rc = mcaControlDb.openDB(mcaDbFile);
        if((rc != MCA_CONTROLDB_OK) && (rc != MCA_CONTROLDB_DB_ALREADY_OPEN)) {
            LOG_WARN("Opening manifest db %s, %d", mcaDbFile.c_str(), rc);
            initDb = true;
        }else{
            //
            //  Version_out <= 9, delete DB and re-generate.
            //  Version_out == 10, alterTableAddInitializedColumnVersion10(), 
            //                     alterPhotoTableVersion11()
            //                     Because Ver. 9 -> 10 or 10 -> 10 will re-generate DB, no re-scan is needed.
            //                     If ver. 10-> 11, re-scan photo metadata and update photos table
            //                     After all done, upgrad DB schema version to 11.
            //  Version_out == 11, up-to-dated
            u64 version_out;
            rc = mcaControlDb.getMcaDbVersion(user_id,
                                              version_out);
            if(rc != MCA_CONTROLDB_OK) {
                LOG_WARN("Unable to get manifestVersion %d", rc);
                initDb = true;
            } else if(version_out <= 9) {
                LOG_ERROR("Manifest version "FMTu64" does not match "FMTu64,
                          version_out, MCA_CONTROL_DB_SCHEMA_VERSION);
                initDb = true;
            } else if(version_out == 10) {
                LOG_INFO("Manifest version "FMTu64" does not match "FMTu64". Upgrade!",
                          version_out, MCA_CONTROL_DB_SCHEMA_VERSION);
                // could be Ver. 10 -> 10 or 10 -> 11
                dbNeedsToBeUpgradedFromV10ToV11_out = true;
                dbNeedsToBeUpgradedFromV11ToV12_out = true;
            } else if(version_out == 11) {
                LOG_INFO("Manifest version "FMTu64" does not match "FMTu64". Upgrade!",
                          version_out, MCA_CONTROL_DB_SCHEMA_VERSION);
                // 11 -> 12
                dbNeedsToBeUpgradedFromV11ToV12_out = true;
            } else {
                LOG_INFO("Opened local manifest file: %s of version:"FMTu64,
                         mcaDbFile.c_str(), version_out);
            }

            switch (version_out) {
                case 10:
                    mcaControlDb.alterTableAddInitializedColumnVersion10();
                    mcaControlDb.alterPhotoTableVersion11();
                    //fall-through !
                case 11:
                    mcaControlDb.addPhotoAlbumTableAndAlbumIdRefVersion12();
                default:;
            }
        }
    }

    if(initDb) {
        LOG_INFO("Replacing corrupt/outdated mcaDB file %s", mcaDbFile.c_str());
        rc = mcaControlDb.closeDB();
        if(!fileMissing){
            rc = VPLFile_Delete(mcaDbFile.c_str());
            if(rc != 0) {
                LOG_WARN("Failure to unlink %s, %d", mcaDbFile.c_str(), rc);
            }
        }
        rc = mcaControlDb.openDB(mcaDbFile);
        if(rc != 0) {
            LOG_ERROR("Opening DB %s, %d", mcaDbFile.c_str(), rc);
            mcaControlDb.closeDB();
            return rc;
        }
        rc = mcaControlDb.initTables();
        if(rc != MCA_CONTROLDB_OK) {
            LOG_ERROR("Init Tables for %s, %d", mcaDbFile.c_str(), rc);
            return rc;
        }
        rc = mcaControlDb.insertMcaDbVersion(user_id,
                                             MCA_CONTROL_DB_SCHEMA_VERSION);
        if(rc != MCA_CONTROLDB_OK) {
            LOG_ERROR("Cannot set manifest file format version: %s to version "FMTu64", err:%d",
                      mcaDbFile.c_str(), MCA_CONTROL_DB_SCHEMA_VERSION, rc);
            return rc;
        }
        // Bug 10634: The DB was just initialized and is empty; be sure to populate it.
        dbNeedsToBePopulated_out = true;
        dbNeedsToBeUpgradedFromV10ToV11_out = false;
        dbNeedsToBeUpgradedFromV11ToV12_out = false;
    } else {
        bool isInit = false;
        rc = mcaControlDb.getMcaDbInitialized(/*OUT*/ isInit);
        if (rc != MCA_CONTROLDB_OK) {
            LOG_ERROR("getMcaDbInitialized:%d", rc);
            return rc;
        }
        if(!isInit) {//Ver. 10 -> 10, DB will be re-generated.
            dbNeedsToBePopulated_out = true;
            //corner case, refer to: https://bugs.ctbg.acer.com/show_bug.cgi?id=16670#c13
            dbNeedsToBeUpgradedFromV10ToV11_out = true;
            dbNeedsToBeUpgradedFromV11ToV12_out = true;
        }
    }
    return MCA_CONTROLDB_OK;
}

void McaControlDB::alterTableAddInitializedColumnVersion10()
{
    // Special code to add a column to the schema.  If column
    // already exists, there's no change.  Version 10 is when the
    // column was introduced.
    int rc = 0;
    char *errmsg = NULL;

    rc = sqlite3_exec(m_db, "ALTER TABLE version ADD COLUMN initialized INTEGER;", NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        std::string errmsgstr = errmsg;
        if(errmsgstr == std::string("duplicate column name: initialized")) {
            // Expected if column already exists
        } else {
            LOG_ERROR("ALTER TABLE: %d(%d): %s", rc, sqlite3_extended_errcode(m_db), errmsg);
        }
        sqlite3_free(errmsg);
    } else {
        LOG_INFO("Version 10 table altered, initialized added to version table");
    }
}

void McaControlDB::alterPhotoTableVersion11()
{
    int rc = 0;
    char *errmsg = NULL;

    rc = sqlite3_exec(m_db, "ALTER TABLE photos ADD COLUMN comp_id TEXT;"
                            "ALTER TABLE photos ADD COLUMN special_format_flag INTEGER;", NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        std::string errmsgstr = errmsg;
        if(errmsgstr == std::string("duplicate column name: initialized")) {
            // Expected if column already exists
        } else {
            LOG_ERROR("ALTER TABLE: %d(%d): %s", rc, sqlite3_extended_errcode(m_db), errmsg);
        }
        sqlite3_free(errmsg);
    } else {
        LOG_INFO("Version 11 table altered, comp_id and special_format_flag added to photos table.");
    }
}

MMError McaControlDB::deleteMusicTracksByCloudPcId(u64 cloudPcId)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_DELETE_MUSIC_TRACKS_BY_CLOUD_PC_ID =
            "DELETE FROM music_tracks WHERE cloud_pc_id_ref=?";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_MUSIC_TRACKS_BY_CLOUD_PC_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_MUSIC_TRACKS_BY_CLOUD_PC_ID, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::deleteMusicAlbumsByCloudPcId(u64 cloudPcId)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_DELETE_MUSIC_ALBUMS_BY_CLOUD_PC_ID =
            "DELETE FROM music_albums WHERE cloud_pc_id_ref=?";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_MUSIC_ALBUMS_BY_CLOUD_PC_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_MUSIC_ALBUMS_BY_CLOUD_PC_ID, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::deleteVideosByCloudPcId(u64 cloudPcId)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_DELETE_VIDEOS_BY_CLOUD_PC_ID =
            "DELETE FROM videos WHERE cloud_pc_id_ref=?";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_VIDEOS_BY_CLOUD_PC_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_VIDEOS_BY_CLOUD_PC_ID, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::deletePhotosByCloudPcId(u64 cloudPcId)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    static const char* SQL_DELETE_PHOTOS_BY_CLOUD_PC_ID =
            "DELETE FROM photos WHERE cloud_pc_id_ref=?";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_PHOTOS_BY_CLOUD_PC_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_PHOTOS_BY_CLOUD_PC_ID, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::deleteCatalog(u64 cloudPcId)
{
    int rc;
    int rv = MCA_CONTROLDB_OK;

    rc = deleteMusicTracksByCloudPcId(cloudPcId);
    if(rc != 0) {
        LOG_ERROR("deleteMusicTracksByCloudPcId:%d", rc);
        rv = rc;
    }

    rc = deleteMusicAlbumsByCloudPcId(cloudPcId);
    if(rc != 0) {
        LOG_ERROR("deleteMusicAlbumsByCloudPcId:%d", rc);
        rv = rc;
    }

    rc = deleteVideosByCloudPcId(cloudPcId);
    if(rc != 0) {
        LOG_ERROR("deleteVideosByCloudPcId:%d", rc);
        rv = rc;
    }

    rc = deletePhotosByCloudPcId(cloudPcId);
    if(rc != 0) {
        LOG_ERROR("deletePhotosByCloudPcId:%d", rc);
        rv = rc;
    }

    static const char* SQL_DELETE_COLLECTIONS_BY_CLOUD_PC_ID =
            "DELETE FROM collections WHERE cloud_pc_id=?";
    sqlite3_stmt *stmt = NULL;

    rc = sqlite3_prepare_v2(m_db, SQL_DELETE_COLLECTIONS_BY_CLOUD_PC_ID, -1, &stmt, NULL);
    CHECK_PREPARE(rv, rc, SQL_DELETE_COLLECTIONS_BY_CLOUD_PC_ID, m_db, end);

    rc = sqlite3_bind_int64(stmt, 1, cloudPcId);
    CHECK_BIND(rv, rc, m_db, end);

    rc = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, m_db, end);

 end:
    FINALIZE_STMT(rv, rc, m_db, stmt);
    return rv;
}

MMError McaControlDB::generatePhotoAlbum2Db(u64 cloudPcId,
                                       const std::string& collectionId,
                                       const std::string& album_name,
                                       const std::string& thumbnail,
                                       const u64 timestamp)
{
    std::string oid;

    media_metadata::ContentDirectoryObject cdo;
    int rv;

    oid = collectionId;
    cdo.set_object_id(oid);
    cdo.set_source(media_metadata::MEDIA_SOURCE_LIBRARY);
    cdo.mutable_photo_album()->set_album_name(album_name);
    cdo.mutable_photo_album()->set_album_thumbnail(thumbnail);
    cdo.mutable_photo_album()->set_timestamp(timestamp);
    LOG_INFO("add album:%s oid:%s collectionId:%s ",album_name.c_str(), oid.c_str(), collectionId.c_str());
    rv = updatePhotoAlbum(cloudPcId, collectionId, cdo);
    if (rv != 0) {
        LOG_ERROR("Add photo album failed! collectinId: %s", collectionId.c_str());
    }

    return rv;
}

void McaControlDB::addPhotoAlbumTableAndAlbumIdRefVersion12()
{
    int rc = 0;
    char *errmsg = NULL;

    rc = sqlite3_exec(m_db, "CREATE TABLE IF NOT EXISTS photo_albums ("
                            "object_id TEXT,"
                            "cloud_pc_id_ref INTEGER,"
                            "collection_id_ref TEXT,"
                            "media_source INTEGER,"
                            "album_name TEXT,"
                            "album_thumbnail TEXT,"
                            "timestamp INTEGER,"
                            "PRIMARY KEY(object_id, cloud_pc_id_ref));", NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Create photo_albums TABLE: %d(%d): %s", rc, sqlite3_extended_errcode(m_db), errmsg);
        sqlite3_free(errmsg);
    } else {
        LOG_INFO("Version 12: create table photo_albums successfully.");
    }

    rc = sqlite3_exec(m_db, "CREATE TABLE IF NOT EXISTS photos_albums_relation ("
                            "object_id TEXT,"
                            "album_id_ref TEXT,"
                            "cloud_pc_id_ref INTEGER,"
                            "PRIMARY KEY(object_id, album_id_ref));"
                            "CREATE INDEX IF NOT EXISTS relation_index ON photos_albums_relation(album_id_ref);", NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Create photos_albums_relation TABLE: %d(%d): %s", rc, sqlite3_extended_errcode(m_db), errmsg);
        sqlite3_free(errmsg);
        sqlite3_free(errmsg);
    } else {
        LOG_INFO("Version 12: create table photos_albums_relation successfully.");
    }


}


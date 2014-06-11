/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef __VSTEST_PERSONAL_CLOUD_DATA_HPP__
#define __VSTEST_PERSONAL_CLOUD_DATA_HPP__

#include "vpl_time.h"
#include "vplex_vs_directory.h"
#include "vssts.hpp"

#include <map>
#include <string>

enum vs_psn_verify_flag {
    VS_PSN_VERIFY_NONE = 0x0,
    VS_PSN_VERIFY_VERSION = 0x1,
    VS_PSN_VERIFY_CTIME = 0x1 << 1,
    VS_PSN_VERIFY_MTIME = 0x1 << 2,
    VS_PSN_VERIFY_ALL = VS_PSN_VERIFY_VERSION | VS_PSN_VERIFY_CTIME | VS_PSN_VERIFY_MTIME
};


/// Main test entry point.
int test_personal_cloud(
                        u64 user_id,
                        u64 dataset_id,
                        bool run_cloudnode_tests);

/// File entry
class vs_file
{
public:
    vs_file() : 
        ctime(0), mtime(0), 
        confirmed(false)
    {};
    ~vs_file() {};

    /// Reset the confirmed flag.
    void reset_verify();

    /// Read data from server and compare to local data.
    /// @param name Name of the file
    /// @param received_metadata Received file metadata.
    /// @param received_version Received file change verson.
    /// @param received_ctime Received file creation time.
    /// @param received_mtime Received file modification time.
    /// @param flag specify the type of data you want to verify.
    /// @return #VSSI_SUCCESS File present and data matches expectations.
    /// @return 1 Data mismatches.
    /// @return other VSSI error Error occurred attempting to access data.
    int verify(VSSI_Object handle, 
               const std::string& name,
               u64 received_version,
               u64 received_ctime,
               u64 received_mtime,
               vs_psn_verify_flag flag = VS_PSN_VERIFY_ALL);

    /// Return the confirmed flag. Flag is set on a successful #verify() call.
    bool is_confirmed();

    /// Write specified data to the file, replacing any previous data.
    /// @param name File path and name
    /// @param data Data to write
    /// @return #VSSI_SUCCESS File written to server.
    /// @return other VSSI error Error occurred attempting to write data.
    int write(VSSI_Object handle,
              const std::string& path,
              const std::string& data);

    /// Truncate a specified file.
    /// @param name File path and name
    /// @param size New size for the file.
    /// @return #VSSI_SUCCESS File written to server.
    /// @return other VSSI error Error occurred attempting to truncate.
    int truncate(VSSI_Object handle,
                 const std::string& path,
                 u64 size);

    // This is only needed when db-mode is on.
    void update_version(u64 change_version);

    /// "file" change version
    u64 version;

    // Timestamps for verification, when not zero
    u64 ctime;
    u64 mtime;

    /// "file" contents
    std::string contents;

    /// "file" metadata
    std::string metadata;

    bool confirmed;
};

/// Directory entry
class vs_dir
{
public:
    vs_dir() : 
        ctime(0), mtime(0),
        confirmed(false) 
    {};
    ~vs_dir() {};

    /// Reset the confirmed flag for this and all contained elements.
    void reset_verify();

    /// Verify this directory and all contained elements against the server.
    /// @param name Name of the directory
    /// @param received_version Received change version of the directory.
    /// @param received_ctime Received creation time of the directory.
    /// @param received_mtime Received modification time of the directory.
    /// @param flag specify the type of data you want to verify.
    /// @return #VSSI_SUCCESS Directory present and all expected entries are
    ///         at the server with correct data.
    /// @return 1 Data mismatches. This or a child element has wrong data.
    /// @return other VSSI error Error occurred attempting to access data.
    /// @note Errors verifying contained entries may be combined.
    int verify(VSSI_Object handle,
               const std::string& name,
               u64 received_version,
               u64 received_ctime,
               u64 received_mtime,
               vs_psn_verify_flag flag = VS_PSN_VERIFY_ALL);

    /// Return the confirmed flag. Flag is set on a successful #verify() call.
    bool is_confirmed();

    /// Write to a file contained in this directory.
    /// @param path Path of this directory.
    /// @param name Name of file, including path relative to this directory.
    /// @param data Data to write to the file. File contents are replaced.
    /// @return #VSSI_SUCCESS Write successful.
    /// @return other VSSI Error Error occurred processing the write.
    int write_file(VSSI_Object handle,
                   const std::string& path,
                   const std::string& name,
                   const std::string& data);

    /// Truncate a file contained in this directory.
    /// @param path Path of this directory.
    /// @param name Name of file, including path relative to this directory.
    /// @param size New size of the file.
    /// @return #VSSI_SUCCESS Change successful.
    /// @return other VSSI Error Error occurred processing the change.
    int truncate_file(VSSI_Object handle,
                      const std::string& path,
                      const std::string& name,
                      u64 size);

    /// Set timestamps for a file or directory. 
    /// This is a local no-op if file/directory does not exist.
    /// @param name Name of file or directory including path.
    /// @param ctime New ctime of file or directory if not zero.
    /// @param mtime New mtime of file or directory if not zero.
    /// @return #VSSI_SUCCESS Change successful.
    /// @return other VSSI Error Error occurred processing the change.
    int set_times(VSSI_Object handle,
                  const std::string& name,
                  u64 ctime, u64 mtime);
    // Set timestamps in local data.
    void set_times(const std::string& name, u64 change_version,
                   u64 ctime, u64 mtime);

    /// Create a directory.
    /// @param path Path of this directory.
    /// @param name Name of directory to create, including path relative to
    ///        this directory.
    /// @return #VSSI_SUCCESS Mkdir successful.
    /// @return other VSSI Error Error occurred processing the command.
    /// @note If any path element of @a name is a file, the file will be
    ///       deleted and a directory created in its place.
    int add_dir(VSSI_Object handle,
                const std::string& path,
                const std::string& name);

    /// Read the specified directory.
    /// @param path Path of this directory.
    /// @param name Name of directory to read, including path relative to
    ///        this directory.
    /// @return #VSSI_SUCCESS Read and verify successful.
    /// @return other VSSI Error Error occurred processing the command.
    /// @return 1 if the directory exists but does not have the same contents
    ///         on server as it should.
    int read_dir(VSSI_Object handle,
                 const std::string& path,
                 const std::string& name);

    /// Read a file.
    /// @param path Path of this directory.
    /// @param name Name of file to read, including path relative to
    ///        this directory.
    /// @return #VSSI_SUCCESS Read and verify successful.
    /// @return other VSSI Error Error occurred processing the command.
    /// @return 1 if the file exists but does not have the same contents
    ///         on server as it should.
    int read_file(VSSI_Object handle,
                  const std::string& path,
                  const std::string& name);

    /// Remove a file or directory (rm -rf)
    /// This operation must always be performed on the root directory.
    /// @param name Name of file or directory to delete, including path
    ///        relative to the object.
    /// @return #VSSI_SUCCESS Removal successful.
    /// @return other VSSI Error Error occurred processing the command.
    int remove(VSSI_Object handle,
               const std::string& name);
    /// Same, with no server call.
    void remove(const std::string& name,
                u64 change_version);

    /// Rename a file or directory (rename(old, new)). 
    /// This operation must always be performed on the root directory.
    /// @param name Name of file or directory to move, including path
    ///        relative to the object.
    /// @param new_name New name for the file or directory, including path
    ///        relative to the object.
    /// @return #VSSI_SUCCESS Rename successful.
    /// @return other VSSI Error Error occurred processing the command.
    int rename(VSSI_Object handle,
               const std::string& name,
               const std::string& new_name);

    /// Copy a file or directory (copy(old, new)). 
    /// This operation must always be performed on the root directory.
    /// @param source Name of file or directory to copy, including path
    ///        relative to the object.
    /// @param destination Destination of copied file or directory,
    ///        including path relative to the object.
    /// @return #VSSI_SUCCESS Rename successful.
    /// @return other VSSI Error Error occurred processing the command.
    int copy(VSSI_Object handle,
             const std::string& name,
             const std::string& new_name);

    /// Copy an existing file by reference using it's hash and size.
    /// This operation must always be performed on the root directory.
    /// @param source Name of file or directory to copy, including path
    ///        relative to the object.
    /// @param destination Destination of copied file or directory,
    ///        including path relative to the object.
    /// @return #VSSI_SUCCESS Rename successful.
    /// @return other VSSI Error Error occurred processing the command.
    int copy_match(VSSI_Object handle,
                   const std::string& name,
                   const std::string& new_name);

    /// Get disk space information
    /// @return #VSSI_SUCCESS Rename successful.
    /// @return other VSSI Error Error occurred processing the command.
    int get_space(VSSI_Object handle);

    /// Get a file element residing in this directory.
    /// @param path Path of this directory.
    /// @param name Name of file to get, including path
    ///        relative to this directory.
    /// @param change_version Version in which the change occurs.
    /// @return NULL if file not found where it should be.
    /// @return Pointer to file if found. File is removed from the tree.
    vs_file* get_file(const std::string& path,
                      const std::string& name,
                      u64 change_version);

    /// Find a file element residing in this directory.
    /// @param path Path of this directory.
    /// @param name Name of file to get, including path
    ///        relative to this directory.
    /// @param change_version Version in which the change occurs.
    /// @return NULL if file not found where it should be.
    /// @return Pointer to file if found.
    vs_file* find_file(const std::string& path,
                       const std::string& name,
                       u64 change_version);

    /// Copy file element residing in this directory (deep-copy).
    /// @param path Path of this directory.
    /// @param name Name of file to get, including path
    ///        relative to this directory.
    /// @return NULL if file not found where it should be.
    /// @return Pointer to copy of file if found.
    vs_file* copy_file(const std::string& path,
                       const std::string& name);

    /// Get a directory element residing in this directory.
    /// @param path Path of this directory.
    /// @param name Name of directory to get, including path
    ///        relative to this directory.
    /// @param change_version Version in which the change occurs.
    /// @return NULL if directory not found where it should be.
    /// @return Pointer to directory if found. Directory is removed from the tree.
    vs_dir* get_dir(const std::string& path,
                    const std::string& name,
                    u64 change_version);

    /// Find a directory element residing in this directory.
    /// @param path Path of this directory.
    /// @param name Name of directory to get, including path
    ///        relative to this directory.
    /// @param change_version Version in which the change occurs.
    /// @return NULL if directory not found where it should be.
    /// @return Pointer to directory if found.
    vs_dir* find_dir(const std::string& path,
                     const std::string& name,
                     u64 change_version);

    /// Copy a directory element residing in this directory (deep-copy).
    /// @param path Path of this directory.
    /// @param name Name of directory to get, including path
    ///        relative to this directory.
    /// @return NULL if directory not found where it should be.
    /// @return Pointer to directory if found. Directory is removed from the tree.
    vs_dir* copy_dir(const std::string& path,
                     const std::string& name,
                     u64 change_version);

    /// Put a file element into this directory.
    /// @param path Path of this directory.
    /// @param name Name of file to put, including path
    ///        relative to this directory.
    /// @param change_version Version in which the change occurs.
    /// @note Files in the way of the file to put are deleted.
    ///       Any file/directory at the location for this file is deleted.
    ///       Directories needed for the file's path are created.
    void put_file(vs_file* file,
                  const std::string& path,
                  const std::string& name,
                  u64 change_version);

    /// Put a directory element into this directory.
    /// @param path Path of this directory.
    /// @param name Name of directory to put, including path
    ///        relative to this directory.
    /// @param change_version Version in which the change occurs.
    /// @note Files in the way of the directory to put are deleted.
    ///       Any file/directory at the location for this directory is deleted.
    ///       Directories needed for the file's path are created.
    void put_dir(vs_dir* dir,
                 const std::string& path,
                 const std::string& name,
                 u64 change_version);

    // Propagate change version to descendents.
    // This is only needed when db-mode is on.
    void update_version(u64 change_version);

    void reset();

    /// "directory" change version
    u64 version;

    /// Timestamps for verification when not zero
    u64 ctime;
    u64 mtime;

    // Contained entries
    std::map<std::string, vs_dir> dirs;
    std::map<std::string, vs_file> files;

    bool confirmed;
};
#endif // include guard

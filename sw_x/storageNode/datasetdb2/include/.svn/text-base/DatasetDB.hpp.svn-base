#ifndef __PSN_DATASETDB_HPP__
#define __PSN_DATASETDB_HPP__

#include "vplu_types.h"
#include "vplu_common.h"
#include "vpl_time.h"
#include "vpl_th.h"
#include "vplex_time.h"

#include <string>
#include <vector>
#include <sqlite3.h>

#define SQLITE_ERROR_BASE -16900
#define MAP_SQLITE_ERROR(ec) DATASETDB_ERR_ ## ec = SQLITE_ERROR_BASE - ec
#define DEFAULT_NUM_ENTRIES_TEST 1000

typedef int DatasetDBError;
enum {
    DATASETDB_OK = 0,
    DATASETDB_ERR_DB_ALREADY_OPEN = -16999,
    DATASETDB_ERR_DB_OPEN_FAIL,
    DATASETDB_ERR_IS_DIRECTORY,
    DATASETDB_ERR_NOT_DIRECTORY,
    DATASETDB_ERR_BAD_DATA,
    DATASETDB_ERR_BAD_REQUEST,
    DATASETDB_ERR_UNKNOWN_VERSION,
    DATASETDB_ERR_UNKNOWN_COMPONENT,
    DATASETDB_ERR_UNKNOWN_CONTENT,
    DATASETDB_ERR_UNKNOWN_METADATA,
    DATASETDB_ERR_UNKNOWN_TRASH,
    DATASETDB_ERR_WRONG_COMPONENT_TYPE,
    DATASETDB_ERR_FUTURE_SCHEMA_VERSION,
    DATASETDB_ERR_TABLES_MISSING,
    DATASETDB_ERR_CORRUPTED,
    DATASETDB_ERR_NOT_OPEN,
    DATASETDB_ERR_BUSY,
    DATASETDB_ERR_RESTART,
    DATASETDB_ERR_REACH_COMPONENT_END,
    MAP_SQLITE_ERROR(SQLITE_ERROR),
    MAP_SQLITE_ERROR(SQLITE_INTERNAL),
    MAP_SQLITE_ERROR(SQLITE_PERM),
    MAP_SQLITE_ERROR(SQLITE_ABORT),
    MAP_SQLITE_ERROR(SQLITE_BUSY),
    MAP_SQLITE_ERROR(SQLITE_LOCKED),
    MAP_SQLITE_ERROR(SQLITE_NOMEM),
    MAP_SQLITE_ERROR(SQLITE_READONLY),
    MAP_SQLITE_ERROR(SQLITE_INTERRUPT),
    MAP_SQLITE_ERROR(SQLITE_IOERR),
    MAP_SQLITE_ERROR(SQLITE_CORRUPT),
    MAP_SQLITE_ERROR(SQLITE_NOTFOUND),
    MAP_SQLITE_ERROR(SQLITE_FULL),
    MAP_SQLITE_ERROR(SQLITE_CANTOPEN),
    MAP_SQLITE_ERROR(SQLITE_PROTOCOL),
    MAP_SQLITE_ERROR(SQLITE_EMPTY),
    MAP_SQLITE_ERROR(SQLITE_SCHEMA),
    MAP_SQLITE_ERROR(SQLITE_TOOBIG),
    MAP_SQLITE_ERROR(SQLITE_CONSTRAINT),
    MAP_SQLITE_ERROR(SQLITE_MISMATCH),
    MAP_SQLITE_ERROR(SQLITE_MISUSE),
    MAP_SQLITE_ERROR(SQLITE_NOLFS),
    MAP_SQLITE_ERROR(SQLITE_AUTH),
    MAP_SQLITE_ERROR(SQLITE_FORMAT),
    MAP_SQLITE_ERROR(SQLITE_RANGE),
    MAP_SQLITE_ERROR(SQLITE_NOTADB),
};

enum DatasetDBComponentType {
    // 0 is invalid - to distinguish from NULL
    DATASETDB_COMPONENT_TYPE_FILE = 1,
    DATASETDB_COMPONENT_TYPE_DIRECTORY,
    DATASETDB_COMPONENT_TYPE_ANY,  // ONLY used internally; never in DB
};

#define DATASETDB_OPTION_CASE_INSENSITIVE   0x1     // Case-insensitive component name comparison

#define DATASETDB_LOST_AND_FOUND_PATH       "lost+found"

// only used internally
struct ComponentAttrs {
    u64 id;
    std::string name;
    std::string upname;
    u64 parent;
    DatasetDBComponentType type;
    u32 perm;
    VPLTime_t ctime;
    VPLTime_t mtime;
    u64 version;
    u64 origin_dev;
    u64 cur_rev;
};

// only used internally
struct ContentAttrs {
    u64 compid;
    std::string location;
    u64 size;
    VPLTime_t mtime;
    u64 baserev;
    u64 update_dev;
    u64 rev;
    u64 version;
};

struct ComponentInfo {
    std::string name;  // IMPORTANT: this is the path of the component
    VPLTime_t ctime, mtime;
    int type;
    u32 perm;  // permission bits, aka "file attributes"
    u64 version;
    std::string path;
    u64 size;
    std::string hash;   // DEPRECATED
    std::vector<std::pair<int, std::string> > metadata; // DEPRECATED
};

class DatasetDB {
    friend class Test;

    /* Method naming convention:#
     * Public methods start with an upper-case character.
     * Private methods start with a lower-case character.
     */

public:
    DatasetDB();
    ~DatasetDB();

    DatasetDBError RegisterContentPathConstructFunction(int (*constructContentPathFromNum)(u64 num, std::string &path), int (*constructNumFromContentPath)(const std::string &path, u64 &num));

    DatasetDBError CloseDB();

    DatasetDBError GetDatasetFullVersion(u64 &version);
    DatasetDBError SetDatasetFullVersion(u64 version);

    // DEPRECATED - RETURNS DATASETDB_ERR_BAD_REQUEST
    DatasetDBError SetDatasetFullMergeVersion(u64 version);

    DatasetDBError BeginTransaction();
    DatasetDBError CommitTransaction();
    DatasetDBError RollbackTransaction();

    // SOON TO BE DEPRECATED
    // Test presence of component and create if missing.
    DatasetDBError TestAndCreateComponent(const std::string &path,
                                          int type,
                                          u32 perm,
                                          u64 version,
                                          VPLTime_t time,
                                          std::string *location = NULL,
                                          bool (*locationExists)(const std::string &data_path, const std::string &path) = NULL,
                                          std::string *data_path = NULL);

    DatasetDBError TestAndCreateLostAndFound(u64 version,
                                             bool lost_and_found_system,
                                             VPLTime_t time);

    // Test presence of component
    DatasetDBError TestExistComponent(const std::string &path);

    // Get various info of a component.
    DatasetDBError GetComponentInfo(const std::string &path,
                                    ComponentInfo &info);

    DatasetDBError GetComponentType(const std::string &path,
                                    int &type);

    DatasetDBError GetComponentPermission(const std::string &path,
                                          u32 &perm);
    DatasetDBError SetComponentPermission(const std::string &path,
                                          u32 perm,
                                          u64 version);

    DatasetDBError GetComponentCreateTime(const std::string &path,
                                          VPLTime_t &ctime);
    DatasetDBError SetComponentCreationTime(const std::string &path,
                                            u64 version,
                                            VPLTime_t ctime);

    DatasetDBError GetComponentLastModifyTime(const std::string &path,
                                              VPLTime_t &mtime);
    DatasetDBError SetComponentLastModifyTime(const std::string &path,
                                              u64 version,
                                              VPLTime_t mtime);

    DatasetDBError GetComponentSize(const std::string &path,
                                    u64 &size);
    DatasetDBError SetComponentSize(const std::string &path,
                                    u64 size,
                                    u64 version,
                                    VPLTime_t time,
                                    bool update_modify_time = false);

    DatasetDBError GetComponentVersion(const std::string &path,
                                       u64 &version);
    DatasetDBError SetComponentVersion(const std::string &path,
                                       u32 perm,
                                       u64 version,
                                       VPLTime_t time);

    // Get the content location of the component.
    // The component must not be deleted and must have the content path stored in the content object.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:       The component is unknown or is deleted.
    //   DATASETDB_ERR_IS_DIRECTORY:            The component is a directory.
    //   DATASETDB_ERR_UNKNOWN_CONTENT:         No content object associated with the component.
    DatasetDBError GetComponentPath(const std::string &path,
                                    std::string &location);

    // SOON TO BE DEPRECATED
    // Set the content location of the component.
    // If the component does not exist, it will be created as a file, together with any missing ancestor directory components.
    // New components will get the ctime from the time param.
    // If the component exists, it cannot be a directory.
    // All components from the given component to the root will have their change version and mtime updated according to the supplied params.
    // If (version==0 && time==0), dataset change version won't be updated for any components in the dataset.
    // However, in this case, the component (in the arg) must already exist.
    // ERRORS:
    //   DATASETDB_ERR_IS_DIRECTORY:            The component is a directory.
    // First form is for un-trashed components.
    DatasetDBError SetComponentPath(const std::string &path,
                                    const std::string &location,
                                    u32 perm,
                                    u64 version,
                                    VPLTime_t time);

    // DEPRECATED - RETURNS DATASETDB_ERR_BAD_REQUEST
    DatasetDBError SetComponentMetadata(const std::string &path,
                                        int type, const std::string &value);

    // DEPRECATED - RETURNS DATASETDB_ERR_BAD_REQUEST
    DatasetDBError DeleteComponentMetadata(const std::string &path,
                                           int type);

    // Delete a component.
    // If the component is a directory, it must be empty.
    DatasetDBError DeleteComponent(const std::string &path,
                                   u64 version,
                                   VPLTime_t time);

    // Move the component.
    // If the component is a directory, all components below it are moved together.
    // Moved components will be updated with the supplied version.
    // All ancestor components (both source and destination) will be updated with the supplied version.
    // Both parent components (both source and destination) will have their mtime updated with the supplied time.
    // ASSUMPTIONS:
    //   - new_name does not exist as a visible component.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:       The component is unknown or is deleted.
    //   DATASETDB_ERR_BAD_REQUEST:             Old_name and new_name are the same.
    /// DES: Probably good to follow the filesystem convention of not moving a component directory into itself or a sub-directory of itself.
    //  FO: I'll add a check for that.
    DatasetDBError MoveComponent(const std::string &old_path,
                                 const std::string &new_path,
                                 u32 perm,
                                 u64 version,
                                 VPLTime_t time);

    // List components in the given component.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:       The component is unknown or is deleted.
    //   DATASETDB_ERR_NOT_DIRECTORY:           The component is not a directory.
    DatasetDBError ListComponents(const std::string &path,
                                  std::vector<std::string> &names);

    // List components' info in the given component.
    DatasetDBError ListComponentsInfo(const std::string &path,
                                      std::vector<ComponentInfo> &components_info);

    // Get the total number of the sibling components in the given component.
    DatasetDBError GetSiblingComponentsCount(const std::string &path,
                                             int &count);

    DatasetDBError GetContentByLocation(const std::string &location,
                                        ContentAttrs& content);

    DatasetDBError GetContentRefCount(const std::string &path,
                                      int &count);

    // DEPRECATED - NOOP
    DatasetDBError SetContentHash(const std::string &path,
                                  const std::string &hash);

    DatasetDBError AllocateContentPath(std::string &path);
   
    // Delete content/component entries related to location, currently it shoud only be used in fsck.
    DatasetDBError DeleteContentComponentByLocation(const std::string &location);

    // Delete all file-type components without a content entry in range
    // @param   id_offset  specify which component you want to start to test,
    //                     this paramete will be set to (id_offset = id_offset + range)
    //                     if the function succeed.
    // @param   num_deleted  number of component entries deleted
    // @param   range  specify the maximum number of entry you want to test. To prevent
    //                 OOM in limited system, such as Orbe, this value should not be large
    DatasetDBError DeleteComponentFilesWithoutContent(u64 &id_offset,
                                                      u64 &num_deleted,
                                                      u64 range = DEFAULT_NUM_ENTRIES_TEST);

    // This function combines two dataset checks:
    //  1) Delete file components without a content entry
    //  2) Move components without a valid parent to "lost+found"
    DatasetDBError CheckComponentConsistency(u64 &id_offset,
                                             u64 &num_deleted_this_round,
                                             u64 &num_missing_parents_total,
                                             VPLTime_t &dir_time,
                                             u64 maximum_components,
                                             u64 version,
                                             u64 range = DEFAULT_NUM_ENTRIES_TEST);

    // Move an unreferenced content file to the "lost+found" directory.
    DatasetDBError MoveContentToLostAndFound(const std::string& location,
                                             int count,
                                             VPLTime_t dir_time,
                                             u64 size,
                                             u64 version,
                                             VPLTime_t time);

    // Set the size of a content file. To be used when the file on disk may be of different size than the
    // content record in the database. Saves a look-up for component names.
    DatasetDBError SetContentSizeByLocation(const std::string& location, u64 size);

    // Get the next content record.
    // Next record is the record with the next greater record ID.
    // Set content.id to 0 for first record.
    DatasetDBError GetNextContent(ContentAttrs& content);

    DatasetDBError OpenDB(const std::string &dbpath, u32 options = 0);

    // backup and restore
    DatasetDBError Backup(bool& was_interrupted, VPLTime_t& end_time);
    DatasetDBError Restore(const std::string& dbpath, bool& had_backup);
    void BackupCancel(void);
    DatasetDBError SwapInBackup(void);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(DatasetDB);

    VPLMutex_t mutex;
    VPLMutex_t cnt_mutex;
    VPLCond_t backup_cond;
    u32 options;

    sqlite3 *db;
    std::string dbpath;
    int (*constructContentPathFromNum)(u64 num, std::string &path);
    int (*constructNumFromContentPath)(const std::string &path, u64 &num);
    std::vector<sqlite3_stmt*> dbstmts;

    u32 client_cnt;
    bool force_backup_stop;
    void swapInBackupRecover(void);

    int file_cp(const std::string& src, const std::string& dst,
                bool allow_interrupt, bool& was_interrupted);
    void db_lock(void);
    void db_unlock(void);
    DatasetDBError updateSchema();
    DatasetDBError beginTransaction();
    DatasetDBError commitTransaction();
    DatasetDBError rollbackTransaction();

    DatasetDBError getVersion(int versionId, u64 &version);
    DatasetDBError setVersion(int versionId, u64 version);

    DatasetDBError testAndCreateRootComponent(int type,
                                              u32 perm,
                                              u64 version,
                                              VPLTime_t time);
    DatasetDBError testAndCreateNonRootComponent(const std::string &path,
                                                 int type,
                                                 u32 perm,
                                                 u64 version,
                                                 VPLTime_t time,
                                                 std::string *location = NULL,
                                                 bool (*locationExists)(const std::string &data_path, const std::string &path) = NULL,
                                                 std::string *data_path = NULL,
                                                 bool is_lost_and_found = false,
                                                 u64 content_size = 0);
    DatasetDBError addComponentByNameParent(const std::string &name,
                                            u64 parent,
                                            u64 version,
                                            VPLTime_t time,
                                            int type,
                                            u32 perm,
                                            u64 &component_id);
    DatasetDBError addContentByCompIdRev(u64 compId,
                                         const std::string &location,
                                         u64 size,
                                         VPLTime_t mtime,
                                         u64 rev,
                                         u64 version);
    DatasetDBError getComponentFromStmt(sqlite3_stmt *stmt,
                                        ComponentAttrs &comp);
    DatasetDBError getComponentById(u64 id,
                                    ComponentAttrs &comp);
    DatasetDBError getComponentByPath(const std::string &path,
                                      ComponentAttrs &comp);
    DatasetDBError getComponentByNameParent(const std::string &name, u64 parent,
                                            ComponentAttrs &comp);
    DatasetDBError getContentByLocation(const std::string &location,
                                        ContentAttrs &content);
    DatasetDBError getComponentPathById(u64 id,
                                        std::string &path);
    DatasetDBError getComponentsOnPath(const std::string &path,
                                       std::vector<ComponentAttrs> &comps);

    DatasetDBError getContentByStmt(sqlite3_stmt *stmt,
                                    ContentAttrs &content);
    DatasetDBError getContentByCompIdRev(u64 compId,
                                         u64 rev,
                                         ContentAttrs &content);

    DatasetDBError setComponentIdById(u64 old_id,
                                      u64 new_id);
    DatasetDBError setComponentPermissionById(u64 id,
                                              u32 perm);
    DatasetDBError setComponentCTimeById(u64 id,
                                         VPLTime_t ctime);
    DatasetDBError setComponentMTimeById(u64 id,
                                         VPLTime_t mtime);
    DatasetDBError setContentMTimeByCompIdRev(u64 compid,
                                              u64 rev,
                                              VPLTime_t mtime);
    DatasetDBError setContentSizeByCompIdRev(u64 compid,
                                             u64 rev,
                                             u64 size);
    DatasetDBError setComponentVersionById(u64 id,
                                           u64 version);
    DatasetDBError setContentVersionByCompIdRev(u64 compid,
                                                u64 rev,
                                                u64 version);
    DatasetDBError setContentLocationByCompIdRev(u64 compid,
                                                 u64 rev,
                                                 const std::string &location);
    DatasetDBError setComponentParentById(u64 id,
                                          u64 parent_id);
    DatasetDBError setComponentParentByParentId(u64 old_parent,
                                                u64 new_parent);
    DatasetDBError setComponentNameById(u64 id,
                                        const std::string &name);
    DatasetDBError setComponentsVersion(const std::vector<ComponentAttrs> &comps,
                                        u64 version);

    DatasetDBError setContentSizeByLocation(const std::string &location, u64 size);
    DatasetDBError getNextContent(ContentAttrs& content);
    DatasetDBError getFileComponentIdInRange(u64 id_offset, u64 range, std::vector<u64> &ids);
    DatasetDBError getComponentInRange(u64 id_offset, u64 range, std::vector<ComponentAttrs> &comps);
    DatasetDBError getMaxComponentId(u64 &id);
    DatasetDBError recoverMissingParentComponent(DatasetDBError missing_type,
                                                 const ComponentAttrs &child,
                                                 const ComponentAttrs &parent,
                                                 u64 num_missing_parents,
                                                 u64 maximum_components,
                                                 VPLTime_t dir_time,
                                                 u64 version, VPLTime_t time);

    DatasetDBError deleteComponentById(u64 id);
    DatasetDBError deleteContentByCompId(u64 compid);
    DatasetDBError listComponentsByParent(u64 parent,
                                          std::vector<std::string> &names);
    // Retrieve ComponentInfo from a complete joint table via single sql query
    DatasetDBError listComponentsInfoByParent(u64 parent,
                                              std::vector<ComponentInfo> &components_info);

    DatasetDBError generateLocation(std::string &location);
    DatasetDBError getContentRefCount(const std::string &location,
                                      int &count);
    DatasetDBError getChildComponentsCount(u64 parent,
                                           int &count);
};

#endif

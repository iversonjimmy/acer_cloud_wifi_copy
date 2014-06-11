#ifndef __PSN_DATASETDB_HPP__
#define __PSN_DATASETDB_HPP__

#include "vplu_types.h"
#include "vplu_common.h"
#include "vpl_time.h"
#include "vpl_th.h"

#include <string>
#include <vector>
#include <sqlite3.h>

#define SQLITE_ERROR_BASE -16900
#define MAP_SQLITE_ERROR(ec) DATASETDB_ERR_ ## ec = SQLITE_ERROR_BASE - ec

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

enum {
    // 0 is invalid - to distinguish from NULL
    DATASETDB_COMPONENT_TYPE_FILE = 1,
    DATASETDB_COMPONENT_TYPE_DIRECTORY,
    DATASETDB_COMPONENT_TYPE_ANY,  // ONLY used internally; never in DB
};

#define DATASETDB_OPTION_CASE_INSENSITIVE   0x1     // Case-insensitive component name comparison

/*
  The DatasetDB essentially consists of 4 classes of objects: component, content, metadata, and trash.
  The attributes in each of the classes can be found below.

  NOTE TO DAVID: In the structs below, ignore the members ending in "_type".
  They are used internally to distinguish between NULL and an actual value.
 */

struct ComponentAttrs {
    u64 id;
    std::string name;	// MANDATORY
    u64 content_id;	// content.id, 0 means none
    VPLTime_t ctime, mtime;	// MANDATORY
    int type;		// MANDATORY: 0=file, 1=directory
    u64 version;	// MANDATORY
    u64 trash_id;	// trash.id, 0 means none
    u32 perm;           // permission bits, aka "attributes"
};

struct ContentAttrs {
    u64 id;
    std::string path;	// MANDATORY
    u64 size;		// If size_type==SQLITE_INTEGER, size of contents in content; otherwise, size unknown.
    int size_type;
    std::string hash;	// empty string means not set
    time_t mtime;    // only valid for external content files
    u64 restore_id; // only for files eligible for restore from backup, else 0
};

struct MetadataAttrs {
    u64 id;
    u64 component_id;	// MANDATORY
    int type;		// MANDATORY
    std::string value;	// If value_type==SQLITE_BLOB, value of the metadata; otherwise, unknown.
    int value_type;
};

struct TrashAttrs {
    u64 id;
    u64 version;	// MANDATORY
    u32 index;		// MANDATORY
    VPLTime_t dtime;		// MANDATORY
    u64 component_id;	// MANDATORY
    u64 size;
};

struct ComponentInfo {
    std::string name;
    VPLTime_t ctime, mtime;
    int type;
    u32 perm;  // permission bits, aka "file attributes"
    u64 version;
    std::string path;
    u64 size;
    std::string hash;
    std::vector<std::pair<int, std::string> > metadata;
    u64 restore_id; // files only, set to restore ID if file may be restored
                    // from backup, 0 otherwise
};

struct TrashInfo {
    std::string name;
    VPLTime_t ctime, mtime, dtime;
    int type;
    u64 version;
    u32 index;
    u64 size;
};

class DatasetDB {
  friend class Test;  // TODO: remove from production

    /* Method naming convention:
     * Public methods start with upper-case character.
     * Private methods start with lower-case character.
     */
public:
    DatasetDB();
    ~DatasetDB();

    DatasetDBError RegisterContentPathConstructFunction(int (*constructContentPathFromNum)(u64 num, std::string &path));

    DatasetDBError OpenDB(const std::string &dbpath, bool &fsck_needed, u32 options = 0);
    DatasetDBError CloseDB();

    void ClearModMarker();

    DatasetDBError GetDatasetFullVersion(u64 &version);
    DatasetDBError SetDatasetFullVersion(u64 version);
    DatasetDBError GetDatasetFullMergeVersion(u64 &version);
    DatasetDBError SetDatasetFullMergeVersion(u64 version);
    DatasetDBError GetDatasetSchemaVersion(u64 &version);
    DatasetDBError GetDatasetRestoreVersion(u64 &version);
    DatasetDBError SetDatasetRestoreVersion(u64 version);

    DatasetDBError BeginTransaction();
    DatasetDBError CommitTransaction();
    DatasetDBError RollbackTransaction();

    // Test presence of component (outside trashcan) and create if missing
    DatasetDBError TestAndCreateComponent(const std::string &name,
					  int type,
                                          u32 perm,
					  u64 version,
					  VPLTime_t time);

    // Test presence of component (outside trashcan)
    DatasetDBError TestExistComponent(const std::string &name);

    // Get various info of a component.
    // First form is for un-trashed components.
    // Second form is for trashed components.
    DatasetDBError GetComponentInfo(const std::string &name,
				    ComponentInfo &info);
    DatasetDBError GetComponentInfo(u64 version, u32 index, 
                                    const std::string &name,
				    ComponentInfo &info);

    // Get the type of the component (e.g., directory or file)
    // The information returned is subsumed by GetComponentInfo(), but this is light weight.
    DatasetDBError GetComponentType(const std::string &name,
                                    int &type);
    
    // Get the content path of the component.
    // The component must not be deleted and must have the content path stored in the content object.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    //   DATASETDB_ERR_IS_DIRECTORY:		The component is a directory.
    //   DATASETDB_ERR_UNKNOWN_CONTENT:		No content object associated with the component.
    DatasetDBError GetComponentPath(const std::string &name,
                                    std::string &path);
    DatasetDBError GetComponentPath(u64 version, u32 index,
                                    const std::string &name,
                                    std::string &path);

    // Set the content path of the component.
    // If the component does not exist, it will be created as a file, together with any missing ancestor directory components.
    // New components will get the ctime from the time param.
    // If the component exists, it cannot be a directory.
    // All components from the given component to the root will have their change version and mtime updated according to the supplied params.
    // If (version==0 && time==0), dataset change version won't be updated for any components in the dataset.
    // However, in this case, the component (in the arg) must already exist.
    // ERRORS:
    //   DATASETDB_ERR_IS_DIRECTORY:		The component is a directory.
    // First form is for un-trashed components.
    // Second form is for trashed components.
    DatasetDBError SetComponentPath(const std::string &name,
                                    const std::string &path,
                                    u32 perm,
                                    u64 version,
                                    VPLTime_t time);
    DatasetDBError SetComponentPath(u64 trash_version, u32 index,
                                    const std::string &name,
                                    const std::string &path,
                                    u64 version,
                                    VPLTime_t time);

    // DEPRECATED - use GetComponentInfo instead
    // Get creation time of the component.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    DatasetDBError GetComponentCreateTime(const std::string &name,
					  VPLTime_t &ctime);

    // DEPRECATED - use GetComponentInfo instead
    // Get the last modified time of the component.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    DatasetDBError GetComponentLastModifyTime(const std::string &name,
					      VPLTime_t &mtime);

    // Set the creation time of the component.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    DatasetDBError SetComponentCreationTime(const std::string &name,
                                            u64 version,
                                            VPLTime_t ctime);
    // Set the last modified time of the component.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    DatasetDBError SetComponentLastModifyTime(const std::string &name,
                                              u64 version,
					      VPLTime_t mtime);

    DatasetDBError GetComponentPermission(const std::string &name,
                                          u32 &perm);
    DatasetDBError SetComponentPermission(const std::string &name,
                                          u32 perm,
                                          u64 version);

    // DEPRECATED - use GetComponentInfo instead
    // Get the size of the component.
    // If the component is a directory, sum of space occupied by all files below it is returned.
    // If the component is a file, the size must be stored in the associated content object.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    //   DATASETDB_ERR_UNKNOWN_CONTENT:		Component is a file but no content object is associated with it.
    DatasetDBError GetComponentSize(const std::string &name,
				    u64 &size);

    // Set the size of the component.
    // The component must exist and must be associated with a content object.
    // All components from the given component to the root will have their change version and mtime updated according to the supplied params.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    //   DATASETDB_ERR_IS_DIRECTORY:		The component is a directory.
    //   DATASETDB_ERR_UNKNOWN_CONTENT:		No content object associated with the component.
    // First form is for un-trashed components.
    // Second form is for trashed components.
    DatasetDBError SetComponentSize(const std::string &name,
				    u64 size,
                                    u64 version,
				    VPLTime_t time);
    DatasetDBError SetComponentSize(u64 trash_version, u32 index,
                                    const std::string &name,
				    u64 size,
                                    u64 version,
				    VPLTime_t time);

    // Set the size of the component only if the given size is larger than what's already in the DB.
    DatasetDBError SetComponentSizeIfLarger(const std::string &name,
					    u64 size,
					    u64 version,
					    VPLTime_t time);

    // Set the size of a content file. To be used when the file on disk may be of different size than the
    // content record in the database. Saves a look-up for component names.
    DatasetDBError SetContentSize(const std::string& path, u64 size);

    // DEPRECATED - use GetComponentInfo instead
    // Get the hash of the component.
    // The component must not be deleted and must have the hash stored in the associated content object.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    //   DATASETDB_ERR_IS_DIRECTORY:		The component is a directory.
    //   DATASETDB_ERR_UNKNOWN_CONTENT:		No content object associated with the component.
    DatasetDBError GetComponentHash(const std::string &name,
				    std::string &hash);

    // Set the hash of the component.
    // The component must not be deleted and must be associated with a content object.
    // This function does NOT update any change versions nor mtimes.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    //   DATASETDB_ERR_IS_DIRECTORY:		The component is a directory.
    //   DATASETDB_ERR_UNKNOWN_CONTENT:		No content object associated with the component.
    // First form is for un-trashed components.
    // Second form is for trashed components.
    DatasetDBError SetComponentHash(const std::string &name,
				    const std::string &hash);
    DatasetDBError SetComponentHash(u64 version, u32 index,
                                    const std::string &name,
				    const std::string &hash);

    // Get the metadata of the component.
    // The component must exist and must not be deleted.
    // The component can be either a file or a directory.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    //   DATASETDB_ERR_UNKNOWN_METADATA:	No metadata for the given type.
    DatasetDBError GetComponentMetadata(const std::string &name,
					int type, std::string &value);

    DatasetDBError GetComponentAllMetadata(const std::string &name,
                                           std::vector<std::pair<int, std::string> > &metadata);

    // Set the metadata on the component.
    // The component must exist and must not be deleted.
    // The component can be either a file or a directory.
    // This function does NOT update any change versions nor mtimes.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    // First form is for un-trashed components.
    // Second form is for trashed components.
    DatasetDBError SetComponentMetadata(const std::string &name,
					int type, const std::string &value);
    DatasetDBError SetComponentMetadata(u64 version, u32 index,
                                        const std::string &name,
					int type, const std::string &value);

    DatasetDBError DeleteComponentMetadata(const std::string &name,
                                           int type);

    // DEPRECATED - use GetComponentInfo instead
    // Get the change version of the component.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    DatasetDBError GetComponentVersion(const std::string &name,
				       u64 &version);

    // Set the change version of the component.
    // First form is for un-trashed components.
    // Second form is for trashed components.
    DatasetDBError SetComponentVersion(const std::string &name,
                                       u32 perm,
				       u64 version,
				       VPLTime_t time,
                                       int type = DATASETDB_COMPONENT_TYPE_FILE);
    DatasetDBError SetComponentVersion(u64 trash_version, u32 index,
                                       const std::string &name,
				       u64 version,
				       VPLTime_t time,
                                       int type = DATASETDB_COMPONENT_TYPE_FILE);

    // Delete a component (and any descendant components)
    // Unlike TrashComponent(), DeleteComponent() will immediately delete the component.
    DatasetDBError DeleteComponent(const std::string &name,
                                   u64 version,
                                   VPLTime_t time);

    // Delete all components with a specific content ID.
    DatasetDBError DeleteComponentsByContentId(u64 content_id);

    // Trash a component
    // The component can be either a file or a directory.
    // If the component is a directory, all components below it are trashed together.
    // All components from the parent of the given component to the root will have their change version and mtime updated according to the supplied params.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    DatasetDBError TrashComponent(const std::string &name,
				  u64 version, u32 index,
				  VPLTime_t time);

    // Add a new trash record to the dataset with the given properties.
    DatasetDBError AddTrashRecord(TrashInfo& info);
                                  
    DatasetDBError GetTrashInfo(u64 version, u32 index,
                                TrashInfo &info);


    // Restore trashed component (and possibly its descendents) as component name.
    // ASSUMPTIONS:
    // - name does not exist as a visible component.
    DatasetDBError RestoreTrash(u64 trash_version, u32 index,
                                const std::string &name,
                                u32 perm,
                                u64 version,
                                VPLTime_t time);

    DatasetDBError DeleteTrash(u64 version, u32 index);

    DatasetDBError DeleteAllTrash();

    DatasetDBError DeleteAllComponents();

    // Delete all components that have not been modified since a given time.
    DatasetDBError DeleteComponentsUnmodifiedSince(VPLTime_t time);

    // Delete all file-type components with NULL content ID
    DatasetDBError DeleteComponentFilesWithoutContent();

    // Move the component.
    // If the component is a directory, all components below it are moved together.
    // Moved components will be updated with the supplied version.
    // All ancestor components (both source and destination) will be updated with the supplied version.
    // All ancestor components (both source and destination) will have their mtime updated with the supplied time.
    // ASSUMPTIONS:
    //   - new_name does not exist as a visible component.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    //   DATASETDB_ERR_BAD_REQUEST:		Old_name and new_name are the same.
    /// DES: Probably good to follow the filesystem convention of not moving a component directory into itself or a sub-directory of itself.
    //  FO: I'll add a check for that.
    DatasetDBError MoveComponent(const std::string &old_name,
                                 const std::string &new_name,
                                 u32 perm,
                                 u64 version,
                                 VPLTime_t time);

    // Copy the component.
    // If the component is a directory, all components below it are copied together.
    // Copied components will be updated with the supplied version.
    // All ancestor components of destination will be updated with the supplied version.
    // All ancestor components of destination will have their mtime updated with the supplied time.
    // ASSUMPTIONS:
    //   - new_name does not exist as a visible component.
    // Following components will have their version and mtime updated: copied components, destination location and upto root.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    //   DATASETDB_ERR_BAD_REQUEST:		Old_name and new_name are the same.
    /// DES: Probably good to follow the filesystem convention of not copying a component directory into itself or a sub-directory of itself.
    //  FO: I'll add a check for that.
    DatasetDBError CopyComponent(const std::string &old_name,
                                 const std::string &new_name,
                                 u32 perm,
                                 u64 version,
                                 VPLTime_t time);

    // List components in the given component.
    // ERRORS:
    //   DATASETDB_ERR_UNKNOWN_COMPONENT:	The component is unknown or is deleted.
    //   DATASETDB_ERR_NOT_DIRECTORY:		The component is not a directory.
    DatasetDBError ListComponents(const std::string &name,
                                  std::vector<std::string> &names);
    DatasetDBError ListComponents(const std::string &name,
                                  u64 version, u32 index,
                                  std::vector<std::string> &names);

    // List trash.
    // ERRORS:
    DatasetDBError ListTrash(std::vector<std::pair<u64,u32> > &trashvec);

    DatasetDBError GetContentPathBySizeHash(u64 size,
					    const std::string &hash,
					    std::string &path);
    DatasetDBError GetContentRefCount(const std::string &path,
                                      int &count);
    DatasetDBError GetContentHash(const std::string &path,
                                  std::string &hash);
    DatasetDBError SetContentHash(const std::string &path,
                                  const std::string &hash);
    DatasetDBError SetContentRestoreId(const std::string &path,
                                       u64 restore_id);
    DatasetDBError GetContentLastModifyTime(const std::string &path,
                                            time_t &mtime);
    DatasetDBError SetContentLastModifyTime(const std::string &path,
                                            time_t mtime);
    DatasetDBError GetContentByPath(const std::string &path,
                                    ContentAttrs& attribs);
    DatasetDBError AllocateContentPath(std::string &path);

    // Get the next content record.
    // Next record is the record with the next greater record ID.
    // Set attribs.id to 0 for first record.
    DatasetDBError GetNextContent(ContentAttrs& attribs);

    DatasetDBError DeleteAllContent();
    DatasetDBError DeleteContent(const std::string &path);

    DatasetDBError ListUnrefContents(std::vector<std::string> &paths);

    DatasetDBError Backup(void);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(DatasetDB);

    VPLMutex_t mutex;
    VPLCond_t condvar;
    u32 options;

    sqlite3 *db;
    std::string dbpath;
    int (*constructContentPathFromNum)(u64 num, std::string &path);
    std::vector<sqlite3_stmt*> dbstmts;

    bool is_modified;

    DatasetDBError SetDatasetSchemaVersion(u64 version);
    DatasetDBError openDBUnlocked(const std::string &dbpath, bool &fsck_needed, u32 options = 0);

    DatasetDBError updateSchema();

    DatasetDBError beginTransaction();
    DatasetDBError commitTransaction();
    DatasetDBError rollbackTransaction();

    DatasetDBError getDatasetVersion(int versionId, u64 &version);
    DatasetDBError setDatasetVersion(int versionId, u64 version);

    DatasetDBError createComponentAncestryList(const std::string &name,
                                               int componentType,
                                               bool includeComponent,
					       std::vector<std::pair<std::string, int> > &names);

    DatasetDBError createComponent(const std::string &name,
                                   u64 version,
                                   VPLTime_t time,
                                   int type,
                                   u32 perm,
                                   u64 &component_id);
    DatasetDBError createComponent(u64 trash_version, u32 index,
                                   const std::string &name,
                                   u64 version,
                                   VPLTime_t time,
                                   int type,
                                   u64 &component_id);
    DatasetDBError createComponentIfMissing(const std::string &name,
                                            u64 version,
					    VPLTime_t time,
					    int type,
                                            u32 perm,
                                            u64 &component_id);
    DatasetDBError createComponentIfMissing(u64 trash_version, u32 trash_index,
                                            const std::string &name,
                                            u64 version,
					    VPLTime_t time,
					    int type,
                                            u64 &component_id);
    DatasetDBError createComponentsIfMissing(const std::vector<std::pair<std::string, int> > &names,
                                             u32 perm,
                                             u64 version,
					     VPLTime_t time);
    DatasetDBError createComponentsIfMissing(u64 trash_version, u32 trash_index,
                                             const std::vector<std::pair<std::string, int> > &names,
                                             u64 version,
					     VPLTime_t time);
    DatasetDBError createComponentsToRootIfMissing(const std::string &name,
                                                   u32 perm,
                                                   u64 version,
						   VPLTime_t time);
    DatasetDBError testExistComponent(const std::string &name);
    DatasetDBError getComponentByStmt(sqlite3_stmt *stmt,
				      ComponentAttrs &attrs);
    DatasetDBError getComponentByName(const std::string &name,
				      ComponentAttrs &attrs);
    DatasetDBError getComponentByNameTrashId(const std::string &name,
                                             u64 trash_id,
                                             ComponentAttrs &attrs);
    DatasetDBError getRootComponentByTrashId(u64 trash_id,
                                             ComponentAttrs &attrs);
    DatasetDBError getComponentByVersionIndexName(u64 version, u32 index,
                                                  const std::string &name,
                                                  ComponentAttrs &attrs);
    DatasetDBError getComponentById(u64 id,
				    ComponentAttrs &attrs);
    DatasetDBError getComponentsTotalSize(const std::string &name,
                                          u64 &size);
    DatasetDBError updateComponentContentIdByName(const std::string &name,
                                                  u64 content_id);
    DatasetDBError updateComponentContentIdByVersionIndexName(u64 version,
                                                              u32 index,
                                                              const std::string &name,
                                                              u64 content_id);
    DatasetDBError updateComponentCreationTime(const std::string &name,
                                               VPLTime_t time);
    DatasetDBError updateComponentLastModifyTime(const std::string &name,
						 VPLTime_t time);
    DatasetDBError updateComponentParentLastModifyTime(const std::string &name,
                                                       VPLTime_t time);
    DatasetDBError updateComponentTrashIdByName(const std::string &name,
                                                u64 trash_id);
    DatasetDBError updateComponentTrashIdByTrashId(u64 old_trash_id,
                                                   u64 new_trash_id,
                                                   VPLTime_t time);
    DatasetDBError updateComponentNameByTrashId(u64 trash_id,
						const std::string &old_name,
						const std::string &new_name,
						VPLTime_t time);
    DatasetDBError updateComponentNameByName(const std::string &old_name,
                                             const std::string &new_name,
					     u64 version);
    DatasetDBError updateComponentVersionByTrashId(u64 trash_id,
                                                   u64 version,
                                                   VPLTime_t time);
    DatasetDBError updateComponentPermission(const std::string &name,
                                             u32 perm);
    DatasetDBError copyComponentsByName(const std::string &old_name,
					const std::string &new_name,
					u64 version);
    DatasetDBError updateComponentVersion(const std::string &name,
                                          u64 version,
                                          int type);
    DatasetDBError updateTrashComponentVersion(u64 trash_version, u32 index,
                                               const std::string &name,
                                               u64 version,
                                               VPLTime_t time);
    DatasetDBError updateComponentsVersion(const std::vector<std::pair<std::string, int> > &names,
                                           u64 version);
    DatasetDBError deleteComponentByName(const std::string &name);
    DatasetDBError deleteComponentsByContentId(u64 content_id);
    DatasetDBError deleteAllComponents();
    DatasetDBError listComponentsByName(const std::string &name,
                                        std::vector<std::string> &names);
    DatasetDBError listComponentsByNameTrashId(const std::string &name,
                                               u64 trash_id,
                                               std::vector<std::string> &names);

    DatasetDBError createContent(const std::string &path,
                                  u64 &content_id);
    DatasetDBError createContentIfMissing(const std::string &path,
                                          u64 &content_id);
    DatasetDBError allocateContentPath(std::string &path,
                                       u64 &content_id);
    DatasetDBError getContentByStmt(sqlite3_stmt *stmt,
                                    ContentAttrs &attrs);
    DatasetDBError getContentById(u64 id,
                                  ContentAttrs &attrs);
    DatasetDBError getContentByPath(const std::string &path,
                                    ContentAttrs &attrs);
    DatasetDBError getContentBySizeHash(u64 size,
					const std::string &hash,
					ContentAttrs &attrs);
    DatasetDBError getNextContent(ContentAttrs &attrs);
    DatasetDBError getContentRefCount(const std::string &path,
                                      int &count);
    DatasetDBError updateContentPathById(u64 id, 
                                         const std::string &path);

    DatasetDBError updateContentSizeById(u64 id, 
                                         u64 size);
    DatasetDBError updateContentSizeIfLargerById(u64 id, 
						 u64 size);
    DatasetDBError updateContentSizeByPath(const std::string &path,
                                           u64 size);
    DatasetDBError updateContentHashById(u64 id, 
                                         const std::string &hash);
    DatasetDBError updateContentHashByPath(const std::string &path,
                                           const std::string &hash);
    DatasetDBError updateContentRestoreIdByPath(const std::string &path,
                                                u64 restore_id);
    DatasetDBError updateContentLastModifyTimeByPath(const std::string &path,
                                                     time_t mtime);

    DatasetDBError deleteAllContent();
    DatasetDBError deleteContent(const std::string &path);

    DatasetDBError createMetadata(u64 component_id,
                                  int type,
                                  u64 &metadata_id);
    DatasetDBError createMetadataIfMissing(u64 component_id,
                                           int type,
                                           u64 &metadata_id);
    DatasetDBError getMetadataByComponentIdType(u64 component_id,
                                                int type,
                                                MetadataAttrs &metadataAttrs);
    DatasetDBError getMetadataByComponentId(u64 component_id,
                                            std::vector<std::pair<int, std::string> > &metadata);
    DatasetDBError updateMetadataValueById(u64 id,
                                           const std::string &value);
    DatasetDBError copyMetadataByComponentName(const std::string &old_name,
                                               const std::string &new_name);
    DatasetDBError deleteMetadata(u64 component_id,
                                  int type);

    DatasetDBError createTrash(u64 version, u32 index,
                               VPLTime_t time,
                               u64 component_id,
                               u64 size,
                               u64 &trash_id);
    DatasetDBError getTrashByVersionIndex(u64 version, u32 index,
                                          TrashAttrs &trashAttrs);
    DatasetDBError deleteTrashById(u64 id);
    DatasetDBError deleteAllTrash();
    DatasetDBError listTrash(std::vector<std::pair<u64, u32> > &trashvec);

    DatasetDBError listUnrefContents(std::vector<std::string> &paths);

    void setModMarker(void);    
    void clearModMarker(void);
    bool testModMarker(void);
};

// Return normalized form of input name. Use before name is used in a query.
std::string normalizeComponentName(const std::string& name);

#ifdef TESTDATASETDB
void replaceprefix(const std::string &name,
                   const std::string &prefix,
                   const std::string &replstr,
                   std::string &newname,
                   bool nocase = false);
#endif

#endif

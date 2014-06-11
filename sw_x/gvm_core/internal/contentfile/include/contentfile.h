/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#ifndef __CONTENTFILE_H_06_15_2010__
#define __CONTENTFILE_H_06_15_2010__

#include <vplu_types.h>
#include <vplex_plat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONTENTFILE_VERSION 1

typedef struct contentfile* contentfile_handle_t;

contentfile_handle_t contentfile_open(const char *name, u32 blocksize, u32 numBlocks);
int contentfile_close(contentfile_handle_t handle);
// Resets a contentfile handle without the need to close and then open again.
// The contentfile handle still needs to be closed as if it were just opened.
// Autosync variable will NOT be reset.
int contentfile_reset(struct contentfile* handle,
                      const char *name,
                      u32 blocksize,
                      u32 numBlocks);

// Resizes the contentfile size.  If unsuccessful, size remains the same.
// @param newNumBlocks New contentfile size in blocksize blocks.
int contentfile_resize_total_blocks(struct contentfile* handle,
                                    u32 newNumBlocks);


int contentfile_read(contentfile_handle_t handle, u32 blockNum, u8* buf_out);
int contentfile_write(contentfile_handle_t handle, u32 blockNum, u32 numBlocks, const u8* buf);
int contentfile_invalidate(contentfile_handle_t handle, u32 blockNum);

/// Rename contentfile data.
/// @param old_name Name of existing contentfile data.
/// @param new_name Desired new name for the same data.
/// @return 0 on successful rename, negative error on failure.
int contentfile_rename(const char* old_name, 
                       const char* new_name);

/// Copy an existing contentfile's data to a new disk image.
/// @param handle Contentfile to copy.
/// @param new_name Name for the data copy.
/// @return 0 on successful copy, negative error on failure.
int contentfile_copy(contentfile_handle_t handle,
                     const char* new_name);

/// Delete an existing contentfile.
/// @param name Name of contentfile to delete.
/// @return 0 on successful delete, negative error on failure.
int contentfile_delete(const char* name);

/// Sets variable that determines whether syncing is done automatically.  
/// @param b_auto, if set to 0, contentfile_sync must be called to save to disk. Default value is 1.
void contentfile_set_autosync(contentfile_handle_t handle, int b_autosync);
/// Gets variable that determines whether syncing is done automatically.
/// @return 0 if contentfile_sync must be called to save to disk. 
int contentfile_get_autosync(contentfile_handle_t handle);
/// Calls fdatasync on the map and data file.  Otherwise, changes may be lost during a power failure.
void contentfile_sync(contentfile_handle_t handle);

// Sets valid, 1 if valid, and 0 if not valid.
// Returns 0 if operation successful
// Returns -1 if blockNum is out of range.
int contentfile_get_valid_bit(contentfile_handle_t handle,
                              u32 blockNum,
                              int *valid);

void contentfile_get_progress(contentfile_handle_t handle,
                              u32 *blockSize_out,
                              u32 *blocksValid_out,
                              u32 *blocksTotal_out);

#ifdef __cplusplus
}
#endif

#endif

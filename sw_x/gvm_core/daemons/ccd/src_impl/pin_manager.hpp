#ifndef __PINNED_MEDIA_MANAGER_HPP__
#define __PINNED_MEDIA_MANAGER_HPP__

//============================================================================
/// @file
/// Manages media files that are "pinned" (downloaded for offline use on the local device).
/// See http://www.ctbg.acer.com/wiki/index.php/CCD_DLNA_DMS_Support
//============================================================================

#include <string>
#include <vector>
#include <vplu_types.h>

typedef struct PinnedMediaItem {
    std::string path;
    std::string object_id;
    u64 device_id;
} PinnedMediaItem_t;

int PinnedMediaManager_Init();

int PinnedMediaManager_Destroy();

int PinnedMediaManager_InsertOrUpdatePinnedMediaItem(const std::string& path, const std::string& object_id, u64 device_id);

int PinnedMediaManager_RemovePinnedMediaItem(const std::string& path);

int PinnedMediaManager_GetPinnedMediaItem(const std::string& path, PinnedMediaItem& output_pinned_media_item);

int PinnedMediaManager_GetPinnedMediaItem(const std::string& object_id, u64 device_id, PinnedMediaItem& output_pinned_media_item);

int PinnedMediaManager_RemoveAllPinnedMediaItems();

int PinnedMediaManager_RemoveAllPinnedMediaItemsByDeviceId(u64 device_id);

#endif // __PINNED_MEDIA_MANAGER_HPP__

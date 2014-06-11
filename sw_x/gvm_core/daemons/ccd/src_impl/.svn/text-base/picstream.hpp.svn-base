#ifndef __PICSTREAM_HPP__
#define __PICSTREAM_HPP__

#include "vpl_time.h"
#include "vplu_types.h"
#include <string>

#define MAX_PICSTREAM_DIR 6

int Picstream_Init(u64 userId, const std::string& internalPicDir, VPLTime_t waitQTime);

int Picstream_Destroy();

int Picstream_Enable(bool enable);
int Picstream_GetEnable(bool& enabled);

int Picstream_AddMonitorDir(const char* srcDir,
                            u32 index,
                            bool rmPrevSrcTime);

// TODO: Remove userId from this function.
int Picstream_PerformFullSyncDir(u64 userId, u32 index);

int Picstream_RemoveMonitorDir(u32 index);

// Dataset directory that sync agent should be looking at.
const std::string Picstream_GetDsetDir(const std::string& internalPicDir);

/// The same functionality as Picstream_Enable(false).
/// Difference is this function does not require Picstream to be initialized.
int Picstream_UnInitClearEnable(const std::string& internalPicDir);

// TODO: Remove userId from this function.
/// One-off to send a single file to the camera roll stream.
int Picstream_SendToCameraRoll(u64 userId, const std::string& file);

#endif

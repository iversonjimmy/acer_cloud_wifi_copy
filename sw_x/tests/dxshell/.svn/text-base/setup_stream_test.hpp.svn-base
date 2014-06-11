#ifndef _STREAM_TEST_SETUP_HPP_
#define _STREAM_TEST_SETUP_HPP_

#include <vplu_types.h>
#include "media_metadata_types.pb.h"

#include <string>

int setup_stream_test(int argc, const char* argv[]);

int setup_stream_test_create_metadata(u64 userId, u64 deviceId,
                                      media_metadata::CatalogType_t catType,
                                      const std::string &collectionId,
                                      const std::string &testFilesFolder);

int setup_stream_test_write_dumpfile(u64 userId, u64 deviceId, 
                                     const std::string &collectionId,
                                     const std::string &dumpFile,
                                     bool downloadMusic,
                                     bool downloadPhoto,
                                     bool downloadVideo,
                                     s32 dumpCountMax = -1);

#endif // _STREAM_TEST_SETUP_HPP_

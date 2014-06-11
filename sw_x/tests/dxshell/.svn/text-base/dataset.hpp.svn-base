#ifndef __DATASET_HPP__
#define __DATASET_HPP__

#include <vplu_types.h>

int dispatch_dataset_cmd(int argc, const char* argv[]);
int dispatch_dataset_file_cmd(int argc, const char* argv[]);

int dataset_list(u64 userId);
int dataset_add(u64 userId,
                const std::string &datasetName,
                ccd::NewDatasetType_t datasetType);
int dataset_delete(u64 userId,
                   const std::string &datasetName);

int dataset_file_list(u64 userId,
                      const std::string &datasetName,
                      const std::string &directory);

#endif // __DATASET_HPP__

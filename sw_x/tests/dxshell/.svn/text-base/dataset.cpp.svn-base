#include <ccdi.hpp>
#include <vplu_types.h>

#include <iostream>
#include <iomanip>
#include <string>

#include <log.h>
#include <vpl_plat.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/repeated_field.h>

#include "dx_common.h"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "dataset.hpp"

#ifndef VSCORE_ERR_NO_DATASET
#define VSCORE_ERR_NO_DATASET (-16020)
#endif

static int get_datasets(u64 userId, 
                        google::protobuf::RepeatedPtrField<vplex::vsDirectory::DatasetDetail> &datasetDetails)
{
    int rv = 0;

    ccd::ListOwnedDatasetsInput req;
    ccd::ListOwnedDatasetsOutput res;
    req.set_user_id(userId);
    rv = CCDIListOwnedDatasets(req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("Failed to get list of owned datasets from CCD: %d", rv);
        goto end;
    }

    datasetDetails = res.dataset_details();

 end:
    return rv;
}

static int find_dataset(u64 userId, const std::string &datasetName, u64 &datasetId)
{
    int rv = 0;
    google::protobuf::RepeatedPtrField<vplex::vsDirectory::DatasetDetail> datasetDetails;
    google::protobuf::RepeatedPtrField<vplex::vsDirectory::DatasetDetail>::const_iterator it;

    rv = get_datasets(userId, datasetDetails);
    if (rv != CCD_OK) {
        LOG_ERROR("Failed to get list of datasets: %d", rv);
        goto end;
    }

    for (it = datasetDetails.begin(); it != datasetDetails.end(); it++) {
        if (it->datasetname() == datasetName) {
            datasetId = it->datasetid();
            goto end;
        }
    }

    // failed to find dataset
    rv = VSCORE_ERR_NO_DATASET;

 end:
    return rv;
}

int dataset_list(u64 userId)
{
    int rv = 0;
    google::protobuf::RepeatedPtrField<vplex::vsDirectory::DatasetDetail> datasetDetails;

    rv = get_datasets(userId, datasetDetails);
    if (rv != CCD_OK) {
        LOG_ERROR("Failed to get list of datasets: %d", rv);
        goto end;
    }

    std::cout << "Found " << datasetDetails.size() << " dataset(s)" << std::endl;

    {
        google::protobuf::RepeatedPtrField<vplex::vsDirectory::DatasetDetail>::const_iterator it;
        for (it = datasetDetails.begin(); it != datasetDetails.end(); it++) {
            std::cout << std::setw(15) << std::left << it->datasetname();
            std::cout << "  ";
            std::cout << std::dec << std::right << it->datasetid() << "(" << std::hex << it->datasetid() << ")";
            std::cout << "  ";
            std::cout << std::setw(12) << std::left << vplex::vsDirectory::DatasetType_Name(it->datasettype());
            std::cout << "  ";
            std::cout << std::setw(30) << std::left << it->storageclustername();
            if (it->archivestoragedeviceid_size() > 0) {
                std::cout << "  ";
                std::cout << std::setw(12) << std::left << it->archivestoragedeviceid(0);
            }
            std::cout << std::endl;
        }
    }

 end:
    return rv;
}

int dataset_add(u64 userId,
                const std::string &datasetName,
                ccd::NewDatasetType_t datasetType)
{
    int rv = 0;

    ccd::AddDatasetInput req;
    ccd::AddDatasetOutput res;
    req.set_user_id(userId);
    req.set_dataset_name(datasetName);
    req.set_dataset_type(datasetType);
    rv = CCDIAddDataset(req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("Failed to add dataset: %d", rv);
        goto end;
    }
    LOG_INFO("dataset %s added as datasetid "FMTx64, datasetName.c_str(), res.dataset_id());

 end:
    return rv;
}

int dataset_delete(u64 userId,
                   const std::string &datasetName)
{
    int rv = 0;
    u64 datasetId = 0;

    rv = find_dataset(userId, datasetName, datasetId);
    if (rv != 0) {
        LOG_ERROR("Failed to find dataset %s: %d", datasetName.c_str(), rv);
        goto end;
    }

    {
        ccd::DeleteDatasetInput req;
        req.set_user_id(userId);
        req.set_dataset_id(datasetId);
        rv = CCDIDeleteDataset(req);
        if (rv != CCD_OK) {
            LOG_ERROR("Failed to delete dataset: %d", rv);
            goto end;
        }
        LOG_INFO("dataset "FMTx64" deleted", datasetId);
    }

 end:
    return rv;
}

int dataset_file_list(u64 userId, 
                      const std::string &datasetName, 
                      const std::string &directory)
{
    int rv = 0;
    u64 datasetId = 0;

    rv = find_dataset(userId, datasetName, datasetId);
    if (rv != 0) {
        LOG_ERROR("Failed to find dataset %s: %d", datasetName.c_str(), rv);
        goto end;
    }

    LOG_INFO("Querying CCD for list of entries in dataset %s directory %s...",
             datasetName.c_str(), directory.c_str());

    {
        ccd::GetDatasetDirectoryEntriesInput req;
        ccd::GetDatasetDirectoryEntriesOutput res;
        req.set_user_id(userId);
        req.set_dataset_id(datasetId);
        req.set_directory_name(directory);
        int rv = CCDIGetDatasetDirectoryEntries(req, res);
        if (rv != CCD_OK) {
            LOG_ERROR("Failed to get list of entries in dataset %s directory %s: %d", 
                      datasetName.c_str(), directory.c_str(), rv);
            goto end;
        }

        std::cout << "Found " << res.entries_size() << " entries" << std::endl;

        for (int i = 0; i < res.entries_size(); i++) {
            const ccd::DatasetDirectoryEntry &dde = res.entries(i);
            std::cout << (dde.is_dir() ? "D " : "- ");
            std::cout << std::setw(10) << std::right << dde.size();
            std::cout << std::setw(14) << std::right << dde.mtime();
            std::cout << "  " << dde.name() << std::endl;
        }
    }

 end:
    return rv;
}

//--------------------------------------------------
// dispatcher

static int dataset_file_help(int argc, const char* argv[])
{
    // argv[0] == "File" if called from dispatch_dataset_file_cmd()
    // argv[0] == "Dataset" if called from dispatch_dataset_cmd()
    // Otherwise, called from dataset_file_*() handler function

    bool printAll = strcmp(argv[0], "File") == 0 || strcmp(argv[0], "Dataset") == 0 || strcmp(argv[0], "Help") == 0;

    if (printAll || strcmp(argv[0], "List") == 0)
        std::cout << "Dataset File List datasetName componentPath" << std::endl;
    std::cout << std::endl;

    return 0;
}

static int dataset_help(int argc, const char* argv[])
{
    // argv[0] == "Dataset" if called from dispatch_dataset_cmd()
    // Otherwise, called from dataset_*() handler function

    bool printAll = strcmp(argv[0], "Dataset") == 0 || strcmp(argv[0], "Help") == 0;

    if (printAll)
        std::cout << "Dataset List" << std::endl;
    if (printAll || strcmp(argv[0], "Add") == 0)
        std::cout << "Dataset Add datasetName [datasetType]" << std::endl;
    if (printAll || strcmp(argv[0], "Delete") == 0)
        std::cout << "Dataset Delete datasetName" << std::endl;
    if (printAll)
        dataset_file_help(argc, argv);
    else
        std::cout << std::endl;

    return 0;
}

static int dataset_list(int argc, const char* argv[])
{
    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return dataset_list(userId);
}

static int dataset_add(int argc, const char* argv[])
{
    if (argc < 2) {
        dataset_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    ccd::NewDatasetType_t datasetType = ccd::NEW_DATASET_TYPE_USER;
    if (argc >= 2) {
#ifdef WIN32
#define strcasecmp(s1, s2) _stricmp(s1, s2)
#endif
        if (strcasecmp(argv[2], "media") == 0)
            datasetType = ccd::NEW_DATASET_TYPE_MEDIA;
        else if (strcasecmp(argv[2], "cache") == 0)
            datasetType = ccd::NEW_DATASET_TYPE_CACHE;
        else if (strcasecmp(argv[2], "user") == 0)
            datasetType = ccd::NEW_DATASET_TYPE_USER;
        else if (strcasecmp(argv[2], "fs") == 0)
            datasetType = ccd::NEW_DATASET_TYPE_FS;
        else {
            LOG_ERROR("Unknown dataset type %s", argv[2]);
            return -1;
        }
#ifdef WIN32
#undef strcasecmp
#endif
    }

    return dataset_add(userId, argv[1], datasetType);
}

static int dataset_delete(int argc, const char* argv[])
{
    if (argc < 2) {
        dataset_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return dataset_delete(userId, argv[1]);
}

static int dataset_file_list(int argc, const char* argv[])
{
    if (argc < 3) {
        dataset_file_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return dataset_file_list(userId, argv[1], argv[2]);
}

class DatasetDispatchTable {
public:
    DatasetDispatchTable() {
        dataset_cmds["List"]   = dataset_list;
        dataset_cmds["Add"]    = dataset_add;
        dataset_cmds["Delete"] = dataset_delete;
        dataset_cmds["File"]   = dispatch_dataset_file_cmd;
        dataset_cmds["Help"]   = dataset_help;

        dataset_file_cmds["List"]     = dataset_file_list;
        dataset_file_cmds["Help"]     = dataset_file_help;
    }
    std::map<std::string, subcmd_fn> dataset_cmds;
    std::map<std::string, subcmd_fn> dataset_file_cmds;
};

static DatasetDispatchTable datasetDispatchTable;

int dispatch_dataset_cmd(int argc, const char* argv[])
{
    if (argc <= 1)
        return dataset_help(argc, argv);
    else
        return dispatch(datasetDispatchTable.dataset_cmds, argc - 1, &argv[1]);
}

int dispatch_dataset_file_cmd(int argc, const char* argv[])
{
    if (argc <= 1)
        return dataset_file_help(argc, argv);
    else
        return dispatch(datasetDispatchTable.dataset_file_cmds, argc - 1, &argv[1]);
}


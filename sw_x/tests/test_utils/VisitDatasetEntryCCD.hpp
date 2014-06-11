#include "FindDatasetCCD.hpp"

class VisitDatasetEntryCCD : public FindDatasetCCD {
public:
    int visitDatasetEntries(const std::string &datasetname,
                            const std::string &directory,
                            void (*visitor)(const ccd::DatasetDirectoryEntry &dde));
};

int VisitDatasetEntryCCD::visitDatasetEntries(const std::string &datasetname,
                                              const std::string &directory,
                                              void (*visitor)(const ccd::DatasetDirectoryEntry &dde))
{
    int rv = 0;
    u64 datasetid = 0;

    rv = findDataset(datasetname, datasetid);
    if (rv) goto end;

    LOG_INFO("Querying CCD for list of entries in dataset %s directory %s...",
             datasetname.c_str(), directory.c_str());

    {
        ccd::GetDatasetDirectoryEntriesInput req;
        ccd::GetDatasetDirectoryEntriesOutput res;
        req.set_user_id(userId);
        req.set_dataset_id(datasetid);
        req.set_directory_name(directory);
        int rv = CCDIGetDatasetDirectoryEntries(req, res);
        if (rv != CCD_OK) {
            LOG_ERROR("Failed to get list of entries in dataset %s directory %s: %d", 
                      datasetname.c_str(), directory.c_str(), rv);
            goto end;
        }
        for (int i = 0; i < res.entries_size(); i++) {
            const ccd::DatasetDirectoryEntry &dde = res.entries(i);
            visitor(dde);
        }
    }

 end:
    return rv;
}

#include "LoginCCD.hpp"

class FindDatasetCCD : public LoginCCD {
public:
    int findDataset(const std::string &datasetname,
                    u64 &datasetid);
    int findDataset(vplex::vsDirectory::DatasetType, 
                    u64 &datasetid);
};

int FindDatasetCCD::findDataset(const std::string &datasetname,
                                u64 &datasetid)
{
    int rv = 0;

    LOG_INFO("Querying CCD for list of owned datasets...");
    ccd::ListOwnedDatasetsInput req;
    ccd::ListOwnedDatasetsOutput res;
    req.set_user_id(userId);
    rv = CCDIListOwnedDatasets(req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("Failed to get list of owned datasets from CCD: %d", rv);
        goto end;
    }

    for (int i = 0; i < res.dataset_details_size(); i++) {
        const vplex::vsDirectory::DatasetDetail &dd = res.dataset_details(i);
        LOG_INFO("User owns dataset %s id "FMTx64, dd.datasetname().c_str(), dd.datasetid());
        if (dd.datasetname() == datasetname) {
            datasetid = dd.datasetid();
            rv = 0;
            goto end;
        }
    }
    LOG_ERROR("Failed to find dataset %s", datasetname.c_str());
    rv = -1;
    goto end;

 end:
    return rv;
}

int FindDatasetCCD::findDataset(vplex::vsDirectory::DatasetType datasettype, 
                                u64 &datasetid)
{
    int rv = 0;

    LOG_INFO("Querying CCD for list of owned datasets...");
    ccd::ListOwnedDatasetsInput req;
    ccd::ListOwnedDatasetsOutput res;
    req.set_user_id(userId);
    rv = CCDIListOwnedDatasets(req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("Failed to get list of owned datasets from CCD: %d", rv);
        goto end;
    }

    for (int i = 0; i < res.dataset_details_size(); i++) {
        const vplex::vsDirectory::DatasetDetail &dd = res.dataset_details(i);
        LOG_INFO("User owns dataset %s id "FMTx64, dd.datasetname().c_str(), dd.datasetid());
        if (dd.datasettype() == datasettype) {
            datasetid = dd.datasetid();
            rv = 0;
            goto end;
        }
    }
    LOG_ERROR("Failed to find dataset of type %d", datasettype);
    rv = -1;
    goto end;

 end:
    return rv;
}

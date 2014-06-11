#include "LoginCCD.hpp"

class VisitDatasetCCD : public LoginCCD {
public:
    int visitDatasets(void (*visitor)(const vplex::vsDirectory::DatasetDetail &dd));
};

int VisitDatasetCCD::visitDatasets(void (*visitor)(const vplex::vsDirectory::DatasetDetail &dd))
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
        visitor(dd);
    }

 end:
    return rv;
}

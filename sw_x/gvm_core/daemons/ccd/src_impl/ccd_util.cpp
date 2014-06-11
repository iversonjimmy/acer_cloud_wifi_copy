//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "ccd_util.hpp"
#include "ccd_storage.hpp"

#include "basic_vssi_wrapper.h"
#include "cslsha.h"
#include "virtual_device.hpp"
#include "vpl_fs.h"
#include "vplex_strings.h"
#include <sstream>

using namespace std;

s32
Util_GetServiceTicket(UCFBuffer* ticket, const char* serviceId, const void* secret, u32 secretSize)
{
    LOG_DEBUG("Util_GetServiceTicket");

    s32 rv = CCD_OK;
    CSL_ShaContext context;

    if (ticket == NULL || serviceId == NULL || secret == NULL) {
        LOG_ERROR("ticket = NULL | serviceId = NULL | secret = NULL)");
        rv = CCD_ERROR_PARAMETER;
        goto out;
    }
    
    CSL_ResetSha(&context);
    CSL_InputSha(&context, secret, secretSize);
    CSL_InputSha(&context, serviceId, strlen(serviceId));
    free(ticket->data);
    ticket->size = UCF_SESSION_SECRET_LENGTH;
#if (UCF_SESSION_SECRET_LENGTH != CSL_SHA1_DIGESTSIZE)
#  error "Compile-time check failed!"
#endif
    ticket->data = (char*)malloc(ticket->size);
    CSL_ResultSha(&context, (u8*)ticket->data);

out:
    return rv;
}

int
Util_RemoveAllFilesInDir(const char* dirName)
{
    int rv = CCD_OK;
    VPLFS_dirent_t entry;
    VPLFS_dir_t directory;
    char filename[CCD_PATH_MAX_LENGTH];

    {
        rv = VPLFS_Opendir(dirName, &directory);
        if (rv != VPL_OK) {
            LOG_ERROR("VPLFS_Opendir %s failed: %d", dirName, rv);
            rv = CCD_ERROR_OPENDIR;
            goto out;
        }
        ON_BLOCK_EXIT(VPLFS_Closedir, &directory);

        while ((rv = VPLFS_Readdir(&directory, &entry)) == VPL_OK) {
            if (strcmp(entry.filename, "..") == 0
                    || strcmp(entry.filename, ".") == 0) {
                continue;
            }
            snprintf(filename, CCD_PATH_MAX_LENGTH, "%s/%s",
                    dirName, entry.filename);
            {
                int temp_rv = VPLFS_Rmdir(filename);
                if (temp_rv != VPL_OK) {
                    LOG_WARN("VPLFS_Rmdir(%s) returned %d", filename, temp_rv);
                }
            }
        }
        if (rv == VPL_ERR_MAX) {
            // No more directories
            rv = VPL_OK;
        }
    }
out:
    return rv;
}

string Util_MakeMessageId()
{
    // TODO: use something fixed and random, so that we can better differentiate different clients
    //   at the server.
    stringstream stream;
    VPLTime_t currTimeMillis = VPLTIME_TO_MILLISEC(VPLTime_GetTime());
    stream << "ccd-" << currTimeMillis;
    return string(stream.str());
}

bool Util_IsUserStorageAddressEqual(const vplex::vsDirectory::UserStorageAddress& lhs,
                                    const vplex::vsDirectory::UserStorageAddress& rhs)
{
    if(lhs.has_direct_address() != rhs.has_direct_address() ||
       lhs.direct_address()     != rhs.direct_address()) {
        return false;
    }
    if(lhs.has_direct_port() != rhs.has_direct_port() ||
       lhs.direct_port()     != rhs.direct_port()) {
        return false;
    }
    if(lhs.has_proxy_address() != rhs.has_proxy_address() ||
       lhs.proxy_address()     != rhs.proxy_address()) {
        return false;
    }
    if(lhs.has_proxy_port() != rhs.has_proxy_port() ||
       lhs.proxy_port()     != rhs.proxy_port()) {
        return false;
    }
    if(lhs.has_internal_direct_address() != rhs.has_internal_direct_address() ||
       lhs.internal_direct_address()     != rhs.internal_direct_address()) {
        return false;
    }
    if(lhs.has_direct_secure_port() != rhs.has_direct_secure_port() ||
       lhs.direct_secure_port()     != rhs.direct_secure_port()) {
        return false;
    }
    if(lhs.has_access_handle() != rhs.has_access_handle() ||
       lhs.access_handle()     != rhs.access_handle()) {
        return false;
    }
    if(lhs.has_access_ticket() != rhs.has_access_ticket() ||
       lhs.access_ticket()     != rhs.access_ticket()) {
        return false;
    }

    return true;
}

bool Util_IsUserStorageAccessEqual(const google::protobuf::RepeatedPtrField<vplex::vsDirectory::StorageAccess>& lhs,
                                   const google::protobuf::RepeatedPtrField<vplex::vsDirectory::StorageAccess>& rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (int storageAccessIdx = 0; storageAccessIdx < lhs.size(); storageAccessIdx++) {
        const vplex::vsDirectory::StorageAccess &lhsStorageAccessInstance = lhs.Get(storageAccessIdx);
        const vplex::vsDirectory::StorageAccess &rhsStorageAccessInstance = rhs.Get(storageAccessIdx);
        if (lhsStorageAccessInstance.has_routetype() != rhsStorageAccessInstance.has_routetype() ||
            lhsStorageAccessInstance.routetype()     != rhsStorageAccessInstance.routetype()) {
            return false;
        }
        if (lhsStorageAccessInstance.has_protocol() != rhsStorageAccessInstance.has_protocol() ||
            lhsStorageAccessInstance.protocol()     != rhsStorageAccessInstance.protocol()) {
            return false;
        }
        if (lhsStorageAccessInstance.has_server() != rhsStorageAccessInstance.has_server() ||
            lhsStorageAccessInstance.server()     != rhsStorageAccessInstance.server()) {
            return false;
        }
        
        if (lhsStorageAccessInstance.ports().size() != rhsStorageAccessInstance.ports().size()) {
            return false;
        }
        for (int portIdx = 0; portIdx < lhsStorageAccessInstance.ports().size(); portIdx++) {
            if (lhsStorageAccessInstance.ports(portIdx).has_port() != rhsStorageAccessInstance.ports(portIdx).has_port() ||
                lhsStorageAccessInstance.ports(portIdx).port()     != rhsStorageAccessInstance.ports(portIdx).port()) {
                return false;
            }
            if (lhsStorageAccessInstance.ports(portIdx).has_porttype() != rhsStorageAccessInstance.ports(portIdx).has_porttype() ||
                lhsStorageAccessInstance.ports(portIdx).porttype()     != rhsStorageAccessInstance.ports(portIdx).porttype() ) {
                return false;
            }
        }
    }
    
    return true;
}

bool Util_IsUserStorageEqual(const vplex::vsDirectory::UserStorage* lhs,
                             const vplex::vsDirectory::UserStorage* rhs)
{
    if (lhs->storageclusterid() != rhs->storageclusterid() ||
        lhs->storagename() != rhs->storagename() ||
        lhs->storagetype() != rhs->storagetype() ||
        lhs->usagelimit() != rhs->usagelimit() ||
        lhs->accesshandle() != rhs->accesshandle() ||
        lhs->accessticket() != rhs->accessticket() ||
        !Util_IsUserStorageAccessEqual(lhs->storageaccess(), rhs->storageaccess()) ||
        lhs->featuremediaserverenabled() != rhs->featuremediaserverenabled() ||
        lhs->featurevirtdriveenabled() != rhs->featurevirtdriveenabled() ||    
        lhs->featureremotefileaccessenabled() != rhs->featureremotefileaccessenabled() ||
        lhs->featurefsdatasettypeenabled() != rhs->featurefsdatasettypeenabled() ||
        lhs->featurevirtsyncenabled() != rhs->featurevirtsyncenabled() ||
        lhs->featuremystorageserverenabled() != rhs->featuremystorageserverenabled()) {
        return false;
    }

    return true;
}

const vplex::vsDirectory::DeviceInfo* Util_FindInDeviceInfoList(u64 deviceId,
        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::DeviceInfo>& devices)
{
    for (int i = 0; i < devices.size(); i++) {
        const vplex::vsDirectory::DeviceInfo* curr = &devices.Get(i);
        if ((curr->deviceid() == deviceId)) {
            return curr;
        }
    }
    return NULL;
}

const vplex::vsDirectory::UserStorage* Util_FindInUserStorageList(u64 deviceId,
        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::UserStorage>& storage)
{
    for (int i = 0; i < storage.size(); i++) {
        const vplex::vsDirectory::UserStorage* curr = &storage.Get(i);
        if ((curr->storageclusterid() == deviceId)) {
            return curr;
        }
    }
    return NULL;
}

int Util_InitVsCore(u64 deviceId)
{
    LOG_INFO("vssi_wrap_start");
    int rc = vssi_wrap_start(deviceId);
    if (rc != 0) {
        LOG_ERROR("vssi_wrap_start:%d", rc);
    }
    return 0;
}
int Util_CloseVsCore()
{
    LOG_INFO("vssi_wrap_stop begin");
    vssi_wrap_stop();
    LOG_INFO("vssi_wrap_stop end");
    return 0;
}

int Util_CalcHash(const void* buf, size_t bufLen, std::string& hash_out)
{
    CSL_ShaContext context;
    u8 sha1digest[CSL_SHA1_DIGESTSIZE];
    int rv = CSL_ResetSha(&context);
    if(rv != 0) {
        LOG_ERROR("CSL_ResetSha should never fail:%d", rv);
        return rv;
    }
    rv = CSL_InputSha(&context, buf, static_cast<u32>(bufLen));
    if(rv != 0) {
        LOG_ERROR("CSL_InputSha should never fail:%d", rv);
        return rv;
    }
    rv = CSL_ResultSha(&context, sha1digest);
    if(rv != 0) {
        LOG_ERROR("CSL_ResultSha should never fail:%d", rv);
        return rv;
    }
    // Write recorded hash
    hash_out.clear();
    for(int hashIndex = 0; hashIndex<CSL_SHA1_DIGESTSIZE; hashIndex++) {
        char byteStr[4];
        snprintf(byteStr, sizeof(byteStr), "%02"PRIx8, sha1digest[hashIndex]);
        hash_out.append(byteStr);
    }
    return 0;
}

int Util_CalcHash(const char* cString, std::string& hash_out)
{
    return Util_CalcHash(cString, strlen(cString), hash_out);
}

// TODO: copy & pasted from vplex_http_util.cpp; clean up.
// Mask any text between begin-pattern and end-pattern.
// Masking will be done by replacing the substring with a single asterisk.
// E.g., maskText("abc<tag>def</tag>ghi", _, "<tag>", "</tag>") -> "abc<tag>*</tag>ghi"
// If begin-pattern is found but no end-pattern, mask until end of text.
// E.g., maskText("abc<tag>def", _, "<tag>", "</tag>") -> "abc<tag>*"
static size_t maskText(/*INOUT*/char *text, size_t len, const char *begin, const char *end)
{
    char *p = text;
    while ((p = VPLString_strnstr(p, begin, text + len - p)) != NULL) {
        // assert: "p" points to occurrence of begin-pattern.
        p += strlen(begin);
        // assert: "p" points to one char after begin-pattern.
        if (p >= text + len) break;

        char *q = VPLString_strnstr(p, end, text + len - p);
        if (q == NULL) {  // no end-pattern
            q = text + len;
        }
        // assert: "q" points to beginning of end-pattern, or one-char past end of text.

        // assert: text between begin-pattern and end-pattern is [p,q)

        // replace non-empty text inbetween with a single asterisk
        // we do this to hide the actual length of the text inbetween
        if (p < q) {
            *p++ = '*';
            if (p < q) {
                if (q < text + len) {
                    memmove(p, q, text + len - q);
                }
                len -= q - p;
            }
        }
        // assert: "p" points to beginning of end-pattern

        // prepare for next search
        p += strlen(end);
        if (p >= text + len) break;
    }
    return len;
}

void Util_LogSensitiveString(const char* prefix, const char* msg, const char* suffix, const char* file, int line, const char* function)
{
    size_t len = strlen(msg);
    char* worktext = (char*)malloc(len);
    if (worktext == NULL) {
        LOG_ERROR("Failed to malloc memory");
        return;
    }
    ON_BLOCK_EXIT(free, worktext);
    memcpy(worktext, msg, len);

    len = maskText(worktext, len, "accessTicket: ", "\n");
    len = maskText(worktext, len, "devSpecAccessTicket: ", "\n");
    len = maskText(worktext, len, "password: ", "\n");
    len = maskText(worktext, len, "service_ticket: ", "\n");
    // Intended to match both "userPwd" and "reenterUserPwd" (from OPS register InfraHttpRequest).
    len = maskText(worktext, len, "serPwd=", "&");
    len = maskText(worktext, len, "x-ac-serviceTicket: ", "\n");
    len = maskText(worktext, len, "X-ac-serviceTicket: ", "\n");
    // **IMPORTANT** Be sure to update #VPL_LogHttpBuffer() if a sensitive string can also
    //   be present at the HTTP layer.

    LOGPrint(LOG_LEVEL_INFO, file, line, function, "%s%.*s%s", prefix, (int)len, worktext, suffix);
}


int Util_ParseRange(const std::string* reqRange, u64 filesize, u64 &start, u64 &end)
{
    // Check range request.
    u64 total=0;
    if(reqRange) {
        std::string startRange, endRange;
        size_t startToken = reqRange->find("="); // fast-forward past "bytes"
        if(startToken == string::npos) {
            startToken = 0;
        }
        else {
            if(reqRange->substr(0, startToken).compare("bytes") != 0) {
                goto invalidRange;
            }
            startToken++; // Skip past "="
        }

        // Eliminate invalid range definitions.
        if(reqRange->find_first_not_of("0123456789-,", startToken) != string::npos) {
            goto invalidRange;
        }
        if(reqRange->find('-', startToken) == string::npos) {
            goto invalidRange;
        }

        // TODO: More comprehensive parsing of Range header.
        // Multiple ranges may be provided.

        startRange = reqRange->substr(startToken, reqRange->find('-') - startToken);
        endRange = reqRange->substr(reqRange->find('-', 0) + 1);

        // Must accept ranges only where end > start,
        // except for suffix-range where start is not defined.
        if(startRange.empty()) {
            if(endRange.empty()) {
                goto invalidRange;
            }
            else {
                // Suffix range. Get last N bytes of file.
                end = strtoull(endRange.c_str(), 0, 10);
                if(filesize > end) {
                    start = (filesize - end);
                    end = filesize - 1;
                }
                else {
                    // Get whole file if file too small.
                    start = 0;
                    end = filesize - 1;
                }
            }
        }
        else {
            start = strtoull(startRange.c_str(), 0, 10);
            if(start > filesize - 1) {
                // Invalid range - start past EOF
                LOG_ERROR("Range \"%s\" starts past EOF("FMTu64")",
                          reqRange->c_str(), filesize);
                goto invalidRange;
            }
            if(endRange.empty()) {
                // Get from start through EOF
                end = filesize - 1;
            }
            else {
                // Get start-end inclusive
                end = strtoull(endRange.c_str(), 0, 10);
                if(end > filesize - 1) {
                    // End past EOF: stop at EOF.
                    end = filesize - 1;
                }
                if(end < start) {
                    // Invalid. Must ignore.
                    start = 0;
                    end = filesize - 1;
                }
            }
        }
    }
    else {
        // Get whole entity.
        start = 0;
        end = filesize - 1;
    }
    total = end - start + 1;

    //return total;
    return 0;

invalidRange:
    return -1;
}

const vplex::vsDirectory::DatasetDetail*
Util_FindDataset(const ccd::CachedUserDetails& userDetails, const char *name, vplex::vsDirectory::DatasetType datasetType)
{
    for (int i = 0; i < userDetails.datasets_size(); i++) {
        if ((userDetails.datasets(i).details().datasetname().compare(name) == 0) &&
                (userDetails.datasets(i).details().datasettype() == datasetType))
        {
            return &userDetails.datasets(i).details();
        }
    }
    return NULL;
}

const vplex::vsDirectory::DatasetDetail*
Util_FindSyncboxArchiveStorageDataset(const ccd::CachedUserDetails& userDetails)
{
    for (int i = 0; i < userDetails.datasets_size(); i++) {
        if ((userDetails.datasets(i).details().datasettype() == vplex::vsDirectory::SYNCBOX) &&
            (userDetails.datasets(i).details().archivestoragedeviceid_size() > 0)) {
            return &userDetails.datasets(i).details();
        }
    }
    return NULL;
}

int Util_GetDeviceStorageDatasetId(const ccd::CachedUserDetails& userDetails, u64 deviceId, u64& datasetId_out)
{
    for (int i = 0; i < userDetails.datasets_size(); i++){
        if ((userDetails.datasets(i).details().datasetname() == "Device Storage") &&
            (userDetails.datasets(i).details().clusterid() == deviceId)) {
            datasetId_out = userDetails.datasets(i).details().datasetid();
            return 0;
        }
    }

    LOG_INFO("Device storage dataset not found for device "FMTu64, deviceId);
    datasetId_out = 0;
    return CCD_ERROR_NOT_FOUND;
}

bool Util_IsLocalDeviceArchiveStorage(const vplex::vsDirectory::DatasetDetail& datasetDetail)
{
    u64 localDeviceId = VirtualDevice_GetDeviceId();
    for (int i = 0; i < datasetDetail.archivestoragedeviceid_size(); i++) {
        if (datasetDetail.archivestoragedeviceid(i) == localDeviceId) {
            return true;
        }
    }
    return false;
}

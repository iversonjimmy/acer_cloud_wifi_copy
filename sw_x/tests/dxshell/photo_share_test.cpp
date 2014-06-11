//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//
#include "photo_share_test.hpp"

#include "ccdi.hpp"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "vpl_conv.h"

#include "log.h"

const char* PHOTO_SHARE_STR = "PhotoShare";

int photo_share_enable(u64 userId)
{
    int rv;
    ccd::UpdateSyncSettingsInput syncSettingsIn;
    ccd::UpdateSyncSettingsOutput syncSettingsOut;
    syncSettingsIn.set_user_id(userId);
    syncSettingsIn.set_enable_shared_by_me(true);
    syncSettingsIn.set_enable_shared_with_me(true);
    rv = CCDIUpdateSyncSettings(syncSettingsIn, syncSettingsOut);
    if(rv != 0) {
        LOG_ERROR("CCDIUpdateSyncSettings(userId:"FMTu64"):%d, sbm_enable(%d) and swm_enable(%d)",
                  userId, rv,
                  syncSettingsOut.enable_shared_by_me_err(),
                  syncSettingsOut.enable_shared_with_me_err());
    }
    return rv;
}

int cmd_photo_share_enable(int argc, const char* argv[])
{
    u64 userId;
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 1)) {
        printf("%s %s - Enables Photoshare by creating the dataset.\n",
               PHOTO_SHARE_STR, argv[0]);
        return 0;
    }
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }
    rv = photo_share_enable(userId);
    if (rv != 0) {
        LOG_ERROR("photo_share_enable("FMTu64"):%d", userId, rv);
        return rv;
    }
    return rv;
}

int sharePhoto_store(u64 userId,
                     const std::string& absFilePath,
                     const std::string& opaqueMetadata,
                     const std::string& absPreviewPath,
                     u64& compId_out,
                     std::string& name_out)
{
    int rv = 0;

    ccd::SharedFilesStoreFileInput sfsfIn;
    ccd::SharedFilesStoreFileOutput sfsfOut;
    sfsfIn.set_user_id(userId);
    sfsfIn.set_abs_file_path(absFilePath);
    sfsfIn.set_opaque_metadata(opaqueMetadata);
    sfsfIn.set_abs_preview_path(absPreviewPath);
    rv = CCDISharedFilesStoreFile(sfsfIn, sfsfOut);
    if (rv != 0) {
        LOG_ERROR("CCDISharedFilesStoreFile(%s,%s,%s):%d",
                  absFilePath.c_str(), opaqueMetadata.c_str(), absPreviewPath.c_str(), rv);
        return rv;
    }
    LOG_ALWAYS("CCDISharedFilesStoreFile success.\n"
               "    compId("FMTu64")\n"
               "    name(%s)",
               sfsfOut.comp_id(), sfsfOut.stored_name().c_str());
    compId_out = sfsfOut.comp_id();
    name_out = sfsfOut.stored_name();
    return rv;
}

int sharePhoto_share(u64 userId,
                     u64 compId,
                     const std::string& storedName,
                     const std::vector<std::string>& recipientEmails)
{
    int rv = 0;

    ccd::SharedFilesShareFileInput sfsfIn;
    sfsfIn.set_user_id(userId);
    sfsfIn.set_comp_id(compId);
    sfsfIn.set_stored_name(storedName);
    for (std::vector<std::string>::const_iterator iter = recipientEmails.begin();
         iter != recipientEmails.end(); ++iter)
    {
        sfsfIn.add_recipient_emails(*iter);
    }

    rv = CCDISharedFilesShareFile(sfsfIn);
    if (rv != 0) {
        LOG_ERROR("CCDISharedFilesShareFile("FMTu64",%s,%s):%d",
                  compId, storedName.c_str(), recipientEmails[0].c_str(), rv);
        return rv;
    }
    LOG_ALWAYS("CCDISharedFilesShareFile success");
    return rv;
}

int sharePhoto_unshare(u64 userId,
                       u64 compId,
                       const std::string& storedName,
                       const std::vector<std::string>& recipientEmails)
{
    int rv = 0;
    ccd::SharedFilesUnshareFileInput sfufIn;
    sfufIn.set_user_id(userId);
    sfufIn.set_comp_id(compId);
    sfufIn.set_stored_name(storedName);
    for (std::vector<std::string>::const_iterator iter = recipientEmails.begin();
         iter != recipientEmails.end(); ++iter)
    {
        sfufIn.add_recipient_emails(*iter);
    }

    rv = CCDISharedFilesUnshareFile(sfufIn);
    if (rv != 0) {
        LOG_ERROR("SharedFilesUnshareFileInput("FMTu64",%s,%s):%d",
                  compId, storedName.c_str(), recipientEmails[0].c_str(), rv);
        return rv;
    }
    LOG_ALWAYS("SharedFilesUnshareFileInput success");
    return rv;
}

int sharePhoto_deleteSharedWithMe(u64 userId,
                                  u64 compId,
                                  const std::string& storedName)
{
    int rv = 0;

    ccd::SharedFilesDeleteSharedFileInput sfdsfIn;
    sfdsfIn.set_user_id(userId);
    sfdsfIn.set_comp_id(compId);
    sfdsfIn.set_stored_name(storedName);
    rv = CCDISharedFilesDeleteSharedFile(sfdsfIn);
    if (rv != 0) {
        LOG_ERROR("CCDISharedFilesDeleteSharedFile("FMTu64",%s):%d",
                  compId, storedName.c_str(), rv);
        return rv;
    }
    LOG_ALWAYS("CCDISharedFilesDeleteSharedFile success");
    return rv;
}

static void printSharedFilesQueryResponse(const ccd::SharedFilesQueryOutput& response)
{
    LOG_ALWAYS("SharedFilesQueryOutput: %s", response.DebugString().c_str());
}

int sharePhoto_dumpFilesQuery(u64 userId,
                              ccd::SyncFeature_t syncFeature,
                              bool printResults,
                              ccd::SharedFilesQueryOutput& sfq_out)
{
    int rv = 0;
    sfq_out.Clear();

    ccd::SharedFilesQueryInput sfqIn;
    sfqIn.set_user_id(userId);
    sfqIn.set_sync_feature(syncFeature);
    rv = CCDISharedFilesQuery(sfqIn, /*OUT*/ sfq_out);
    if (rv != 0) {
        LOG_ERROR("CCDISharedFilesQuery(user:"FMTu64",feat:%d):%d",
                  userId, static_cast<int>(sfqIn.sync_feature()), rv);
        return rv;
    }

    if (printResults) {
        printSharedFilesQueryResponse(sfq_out);
    }
    LOG_ALWAYS("CCDISharedFilesQuery success, printed %d results:",
               sfq_out.query_objects_size());
    return 0;
}

int cmd_sharePhoto_store(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 3)) {
        printf("%s %s <absFilePath> <absPreviewPath>\n",
               PHOTO_SHARE_STR, argv[0]);
        return 0;
    }
    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }
    u64 compId;
    std::string name;
    std::string absFilePath = argv[1];
    std::string absPreviewPath = argv[2];
    // zhongwen, copied chinese characters into my favorite editor and 
    // copied the characters in hexidecimal format below.
    // Unsigned because these values are between 128 and 255 inclusive
    unsigned char zhongwen[] = {0xe4, 0xb8, 0xad, 0xe6, 0x96, 0x87, 0x0};
    std::string chinese = "chinese(";
    chinese.append(reinterpret_cast<char*>(zhongwen));
    chinese.append(")");
    std::string opaqueMetadata("Blah Blah Test Sheep:");
    opaqueMetadata += chinese;
    rv = sharePhoto_store(userId, absFilePath, opaqueMetadata, absPreviewPath,
                          /*OUT*/ compId, /*OUT*/ name);
    if (rv != 0) {
        LOG_ERROR("sharePhoto_store:%d", rv);
        return rv;
    }
    return 0;
}

int cmd_sharePhoto_share(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc < 4)) {
        printf("%s %s <name> <compId> <recipientEmail1> [recipientEmail2] [recipientEmail3] ...\n",
               PHOTO_SHARE_STR, argv[0]);
        return 0;
    }
    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }
    int argIndex = 1;
    std::string name = argv[argIndex++]; // ZeroIndex 1
    u64 compId = VPLConv_strToU64(argv[argIndex++], NULL, 10); // ZeroIndex 2
    std::vector<std::string> recipientEmails;
    while (argIndex < argc) {            // ZeroIndex 3 and beyond
        recipientEmails.push_back(argv[argIndex++]);
    }
    rv = sharePhoto_share(userId, compId, name, recipientEmails);
    if (rv != 0) {
        LOG_ERROR("sharePhoto_share:%d", rv);
        return rv;
    }
    return 0;
}

int cmd_sharePhoto_unshare(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc < 4)) {
        printf("%s %s <name> <compId> <recipientEmail1> [recipientEmail2] [recipientEmail3] ...\n",
               PHOTO_SHARE_STR, argv[0]);
        return 0;
    }

    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    int argIndex = 1;
    std::string name = argv[argIndex++]; // ZeroIndex 1
    u64 compId = VPLConv_strToU64(argv[argIndex++], NULL, 10); // ZeroIndex 2
    std::vector<std::string> recipientEmails;
    while (argIndex < argc) {            // ZeroIndex 3 and beyond
        recipientEmails.push_back(argv[argIndex++]);
    }
    rv = sharePhoto_unshare(userId, compId, name, recipientEmails);
    if (rv != 0) {
        LOG_ERROR("sharePhoto_unshare:%d", rv);
        return rv;
    }
    return 0;
}

int cmd_sharePhoto_deleteSharedWithMe(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 3)) {
        printf("%s %s <name> <compId> \n",
               PHOTO_SHARE_STR, argv[0]);
        return 0;
    }

    u64 userId;
    rv = getUserId(userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    int argIndex = 1;
    std::string name = argv[argIndex++]; // ZeroIndex 1
    u64 compId = VPLConv_strToU64(argv[argIndex++], NULL, 10); // ZeroIndex 2
    rv = sharePhoto_deleteSharedWithMe(userId, compId, name);
    if (rv != 0) {
        LOG_ERROR("sharePhoto_deleteSharedWithMe:%d", rv);
        return rv;
    }
    return 0;
}

int cmd_sharePhoto_dumpSharedWithMe(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 1)) {
        printf("%s %s\n",
               PHOTO_SHARE_STR, argv[0]);
        return 0;
    }

    u64 userId;
    rv = getUserId(/*OUT*/ userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::SharedFilesQueryOutput unused;
    rv = sharePhoto_dumpFilesQuery(userId,
                                   ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_WITH_ME,
                                   true,
                                   /*OUT*/ unused);
    if (rv != 0) {
        LOG_ERROR("sharePhoto_dumpFilesQuery:%d", rv);
        return rv;
    }
    return 0;
}

int cmd_sharePhoto_dumpSharedByMe(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 1)) {
        printf("%s %s\n",
               PHOTO_SHARE_STR, argv[0]);
        return 0;
    }

    u64 userId;
    rv = getUserId(/*OUT*/ userId);
    if(rv != 0) {
        LOG_ERROR("getUserId:%d", rv);
        return rv;
    }

    ccd::SharedFilesQueryOutput unused;
    rv = sharePhoto_dumpFilesQuery(userId,
                                   ccd::SYNC_FEATURE_SHARED_FILES_PHOTOS_SHARED_BY_ME,
                                   true,
                                   /*OUT*/ unused);
    if (rv != 0) {
        LOG_ERROR("sharePhoto_dumpFilesQuery:%d", rv);
        return rv;
    }
    return 0;
}

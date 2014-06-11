//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef VCS_ERRORS_HPP_3_15_2013
#define VCS_ERRORS_HPP_3_15_2013

// See http://wiki.ctbg.acer.com/wiki/index.php/RESTful_API_External_Error_Codes
//    5300-5799 - VCS Errors
//       5300-5749 - Non-retry-able Errors (There is an error which cannot be
//                   recovered without client correcting the request, e.g,
//                   compId and path don't match)
//       5750-5799 - Retry-able Errors (There is an error which can be recovered
//                   without client making any change of the request, e.g,
//                   internal server error, DB error or network error)
//    5800-5999 - ACS Errors
//       5800-5949 - Non-retry-able Errors (There is an error which cannot be
//                   recovered without client correcting the request, e.g,
//                   request is not submitted before it expires)
//       5950-5999 - Retry-able Errors (There is an error which can be recovered
//                   without client making any change of the request, e.g,
//                   internal server error, third party storage error or network
//                   error)
// Verbatim from
//    http://source/cgi-bin/viewcvs.cgi/sw_i/infra/modules/src/java/com/broadon/vcs/util/VcsConstants.java?revision=1.77&view=markup
//    (cvs version 1.77)  line:493

#define VCS_ERR_INVALID_PARAMETER_UPLOADREVISION                   -35300
//
#define VCS_ERR_UPLOADREVISION_NOT_HWM_PLUS_1                      -35302
#define VCS_ERR_CANT_CREATE_UPLOADREVISION                         -35303
//
#define VCS_ERR_UNAUTHORIZED_USER_HAS_NO_RIGHTS_TO_DATASET           -35305
#define VCS_FORBIDDEN_DATASET_SUSPENDED                              -35306
#define VCS_INVALID_REQUEST_URI                                      -35307
#define VCS_PROVIDED_FOLDER_PATH_DOESNT_MATCH_PROVIDED_PARENTCOMPID  -35308
#define VCS_PROVIDED_FOLDER_PATH_DOESNT_MATCH_PROVIDED_FOLDERCOMPID  -35309
//
//
#define VCS_ERR_INVALID_PARAMETER_FOLDERCOMPID                     -35310
#define VCS_ERR_INVALID_HEADER_X_AC_VERSION                        -35311
#define VCS_ERR_INVALID_PARAMETER_COMPID                           -35313
#define VCS_ERR_INVALID_COMPONENT_TYPE                             -35314
#define VCS_ERR_DELETE_DIR_NOT_ALLOWED_FOR_CLOUD_DOC               -35315
#define VCS_ERR_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID         -35316
#define VCS_ERR_TARGET_FOLDER_NOT_EMPTY                            -35317
#define VCS_ERR_DELETE_DIR_NOT_ALLOWED_FOR_ROOT_FOLDER             -35318
#define VCS_ERR_INVALID_PARAMETER_REVISION                         -35319
#define VCS_ERR_INVALID_PATH                                       -35320
#define VCS_ERR_INVALID_PARAMETER_INDEX                            -35321
#define VCS_ERR_INVALID_PARAMETER_MAX                              -35322
#define VCS_ERR_INVALID_PARAMETER_SORTBY                           -35323
#define VCS_ERR_LATEST_REVISION_NOT_FOUND                          -35324
#define VCS_ERR_REVISION_NOT_FOUND                                 -35325
#define VCS_ERR_INVALID_PARAMETER_PARENTCOMPID                     -35326
#define VCS_ERR_UNKNOWN_JSON_FIELD                                 -35327
#define VCS_ERR_JSON_PARSE_EXCEPTION                               -35328
#define VCS_ERR_COMPONENT_ALREADY_EXISTS_AT_PATH                   -35329
#define VCS_ERR_INVALID_JSON_FIELD_SIZE                            -35330
#define VCS_ERR_INVALID_JSON_FIELD_UPDATEDEVICE                    -35331
#define VCS_ERR_COMPONENTURI_OR_ACCESSURL_NEEDED                   -35332
#define VCS_ERR_DOCNAME_NOT_ORIGDEVICEID_PLUS_PATH                 -35333
#define VCS_ERR_INVALID_PATH_COMPONENT_ORIGDEVICEID                -35334
#define VCS_ERR_UPLOADREVISION_PARAMETER_NOT_SUPPORTED             -35335
#define VCS_ERR_INVALID_JSON_FIELD_CREATEDATE                      -35336
#define VCS_ERR_PATH_MISSING_EXTENSION                             -35337
#define VCS_ERR_INVALID_PATH_COMPONENT_DATASETNAME                 -35338
#define VCS_ERR_MOVEFROM_COMPONENT_NOT_FOUND                       -35339
#define VCS_ERR_PRECONSISTENCY_CHECK_SIZE_MISMATCH                 -35340
#define VCS_ERR_BASEREVISION_NOT_FOUND                             -35341
#define VCS_ERR_INVALID_JSON_FIELD_ACCESSURL                       -35342
#define VCS_ERR_EMPTY_PREVIEW_HASH                                 -35343
#define VCS_ERR_EMPTY_CONTENT                                      -35344
#define VCS_ERR_INVALID_PATH_COMPONENT_DATASETID                   -35345
#define VCS_ERR_INVALID_HEADER_X_AC_USERID                         -35346
#define VCS_ERR_INVALID_HEADER_X_AC_SESSIONHANDLE                  -35347
#define VCS_ERR_INVALID_HEADER_X_AC_SERVICETICKET                  -35348
#define VCS_ERR_INVALID_PARAMETER_BASEREVISION                     -35349
#define VCS_ERR_COMPONENT_NOT_FOUND                                -35350
#define VCS_ERR_MOVEFROM_PARAMETER_NOT_SUPPORTED                   -35351
#define VCS_ERR_PATH_DOESNT_POINT_TO_KNOWN_COMPONENT               -35352
#define VCS_ERR_INVALID_INPUT_METHOD                               -35353
#define VCS_ERR_ACS_FILE_NOT_FOUND                                 -35354
#define VCS_ERR_INVALID_INPUT_LASTCHANGED                          -35355
#define VCS_ERR_API_NOT_SUPPORTED_FOR_TARGET_VCS_VERSION           -35356
#define VCS_ERR_INVALID_INPUT_DATASET_VERSION                      -35357
#define VCS_ERR_INCAPABLE_VSYNC_ARCHIVE_DEVICE                     -35358
#define VCS_ERR_COMPONENT_URI_NOT_EMPTY                            -35359
#define VCS_ERR_REVISION_IS_DELETED                                -35367
#define VCS_ERR_DS_DATASET_NOT_FOUND                               -35372

#define VCS_ERR_SQLEXCEPTION                                       -35798
#define VCS_ERR_INTERNAL_SERVER_ERROR                              -35799

#endif /* VCS_ERRORS_HPP_3_15_2013 */

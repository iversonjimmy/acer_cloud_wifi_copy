#ifndef __CCD_MONITOR_SERVICE_CLIENT_H__
#define __CCD_MONITOR_SERVICE_CLIENT_H__

#include <Windows.h>
#include "CCDMonSrv_Defs.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CCDM_ERROR_SUCCESS 0
#define CCDM_ERROR_BUSY -1
#define CCDM_ERROR_NOT_FOUND -2
#define CCDM_ERROR_OTHERS -3

#define CCDM_ERROR_INIT -4
#define CCDM_ERROR_HANDLE -5
#define CCDM_ERROR_WRITE_VAR -6
#define CCDM_ERROR_WRITE -7
#define CCDM_ERROR_READ_VAR -8
#define CCDM_ERROR_READ -9
#define CCDM_ERROR_GET_TRUSTEES -10


extern BOOL CCDMonSrv_initClient(/*const char *appPath*/);
extern DWORD CCDMonSrv_sendRequest(CCDMonSrv::REQINPUT& input, CCDMonSrv::REQOUTPUT& output);

#ifdef __cplusplus
}
#endif

#endif

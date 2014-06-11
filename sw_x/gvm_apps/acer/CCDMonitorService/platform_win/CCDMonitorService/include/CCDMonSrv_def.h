#ifndef __CCD_MONITOR_SERVICE_DEF_H__
#define __CCD_MONITOR_SERVICE_DEF_H__

#include <Windows.h>

#define GET_REQ_SIZE(x) (DWORD)sizeof(x->type) + (_tcslen((LPTSTR)x->payload)+1)*sizeof(TCHAR)
#define GET_RESP_SIZE(x) (DWORD)sizeof(x->dwResult) + (_tcslen((LPTSTR)x->payload)+1)*sizeof(TCHAR)

enum REQTYPE {
    NEW_CCD = 1,
        CLOSE_CCD = 2,
        CLOSE_CCD_LOGOUT = 3,
};

typedef struct REQINPUT {
    REQTYPE type;
    LPVOID payload;
} ReqInput, *LReqInput;

typedef struct REQOUTPUT {
    DWORD dwResult;
    LPVOID payload;
} ReqOutput, *LReqOutput;

#endif

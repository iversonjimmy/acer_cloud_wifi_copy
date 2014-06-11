#pragma once

#include "CCDMonSrvClient.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "utils_ProtobufFileStream.h"

#include <Windows.h>
#include <tchar.h>
#include <io.h>
#include <Sddl.h>

#define _trustees_enable

#define MAX_RETRIES 10

#define DEFAULT_PIPE_LEN 1024
#define DEFAULT_PIPE_TO_MS 100000

const char *pipeName= "\\\\.\\pipe\\com.acer.ccdm.sock.server";
static char AppPath[MAX_PATH+1]={0};
static char UserSid[MAX_PATH+1]={0};
static CRITICAL_SECTION g_CS;
static BOOL bInit=FALSE;
HANDLE hPipe=NULL;

static FileStream *_fs;
DWORD connectPipe();
DWORD writeMessage(CCDMonSrv::REQINPUT& intput);
DWORD readMessage(CCDMonSrv::REQOUTPUT& output);
void releasePipe();

extern BOOL CCDMonSrv_initClient(/*const char *appPath*/)
{
	InitializeCriticalSection(&g_CS);
	bInit = TRUE;
	return TRUE;
}
extern DWORD 
connectPipe()
{
	DWORD rv = CCDM_ERROR_SUCCESS;
	int countBusy=0;
	int countNotFound=0;
	
	while(true) {
		hPipe = CreateFileA(pipeName,
			                GENERIC_READ | GENERIC_WRITE,
							0,
							NULL,
							OPEN_EXISTING,
							0,
							NULL);
		if (hPipe != INVALID_HANDLE_VALUE) {
			
			int fd = _open_osfhandle((intptr_t)hPipe, 0);
			if (fd == -1) {
				rv = CCDM_ERROR_HANDLE;
			}
			else {
				_fs = new FileStream(fd);
				rv = CCDM_ERROR_SUCCESS;
			}
			break;
		}
		rv = GetLastError();
		if (rv  == ERROR_PIPE_BUSY) {
			/// TODO: handle busy cases
			countBusy++;
			if (countBusy > MAX_RETRIES) {
				rv = CCDM_ERROR_BUSY;
				break;
			}
		}
		else if (rv == ERROR_FILE_NOT_FOUND) {
			/// TODO: handle file not found
			countNotFound++;
			if (countNotFound > MAX_RETRIES) {
				rv = CCDM_ERROR_NOT_FOUND;
				break;
			}
		}
		else {
			rv = CCDM_ERROR_OTHERS;
			break;
		}
		if (countNotFound + countBusy >= MAX_RETRIES) {
			rv = CCDM_ERROR_OTHERS;
			break;
		}
		Sleep(50);
	}

	return rv;
}

extern DWORD CCDMonSrv_sendRequest(CCDMonSrv::REQINPUT &input, CCDMonSrv::REQOUTPUT &output)
{
	::EnterCriticalSection(&g_CS);

	DWORD rv = CCDM_ERROR_SUCCESS, rt = CCDM_ERROR_SUCCESS;
	if (!bInit) {
		return CCDM_ERROR_INIT;
	}
#ifdef _trustees_enable
    if (input.type() == CCDMonSrv::NEW_CCD) {
        HANDLE hToken = NULL;
        BOOL bResult = TRUE;

        bResult = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
        if (!bResult) {
            return CCDM_ERROR_HANDLE;
        }
        DWORD dwSize = 0;
        // Get token of the user
        bResult = GetTokenInformation(hToken, TokenUser, NULL, dwSize, &dwSize);
        if (!bResult) {
            DWORD rt = GetLastError();
            if (rt != ERROR_INSUFFICIENT_BUFFER) {
                fprintf(stderr, "GetTokenInformation user get size, error: %u\n", rt);
                return CCDM_ERROR_GET_TRUSTEES;
            }
        }
        PTOKEN_USER pTokenUserInfo = (PTOKEN_USER) GlobalAlloc(GPTR, dwSize);
        bResult = GetTokenInformation(hToken, TokenUser, pTokenUserInfo, dwSize, &dwSize);
        if (!bResult) {
            fprintf(stderr, "GetTokenInformation user, error: %d\n", GetLastError());
            return CCDM_ERROR_GET_TRUSTEES;
        }
        else {
            CCDMonSrv::TRUSTEE* trustee = input.add_trustees();
            LPSTR userSid=NULL;
            ::ConvertSidToStringSidA(pTokenUserInfo->User.Sid, &userSid);
            trustee->set_sid(userSid);
            trustee->set_attr(pTokenUserInfo->User.Attributes);
            LocalFree(userSid);
        }
        // Get token of the groups
        dwSize = 0;
        bResult = GetTokenInformation(hToken, TokenGroups, NULL, dwSize, &dwSize);
        if (!bResult) {
            DWORD rt = GetLastError();
            if (rt != ERROR_INSUFFICIENT_BUFFER) {
                fprintf(stderr, "GetTokenInformation groups get size, error: %u\n", rt);
                return CCDM_ERROR_GET_TRUSTEES;
            }
        }
        PTOKEN_GROUPS pTokenGroupInfo = (PTOKEN_GROUPS) GlobalAlloc(GPTR, dwSize);
        bResult = GetTokenInformation(hToken, TokenGroups, pTokenGroupInfo, dwSize, &dwSize);
        if (!bResult) {
            fprintf(stderr, "GetTokenInformation groups, error: %d\n", GetLastError());
            return CCDM_ERROR_GET_TRUSTEES;
        }
        else {
            for (int i=0; i<(int)pTokenGroupInfo->GroupCount; i++) {
                LPSTR strSid = NULL;
                ::ConvertSidToStringSidA(pTokenGroupInfo->Groups[i].Sid, &strSid);
                CCDMonSrv::TRUSTEE* trustee = input.add_trustees();
                trustee->set_sid(strSid);
                trustee->set_attr(pTokenGroupInfo->Groups[i].Attributes);
                LocalFree(strSid);
            }
        }
    }
#endif
	rt = rv = connectPipe();
	if (rv == CCDM_ERROR_SUCCESS) {
		rv = writeMessage(input);
		if (rv == 0) {
			rv = readMessage(output);
		}
	}
	if (rt == CCDM_ERROR_SUCCESS)
		releasePipe();

	::LeaveCriticalSection(&g_CS);
	return rv;
}

DWORD
writeMessage (CCDMonSrv::REQINPUT& input)
{
	DWORD rv=CCDM_ERROR_SUCCESS;
	bool success=true;
	google::protobuf::uint32 bytes=0;

	if (hPipe == NULL) {
		rv = CCDM_ERROR_HANDLE;
	}
	else {
		if (_fs->getInputSteamErr() || _fs->getOutputStreamErr()) {
			rv = CCDM_ERROR_HANDLE;
			goto out;
		}

		/// stream for writing request
		google::protobuf::io::ZeroCopyOutputStream *outStream = _fs->getOutputStream();
		{
			/// Send request
			google::protobuf::io::CodedOutputStream out(outStream);
			bytes = (google::protobuf::uint32)input.ByteSize();
			out.WriteVarint32(bytes);
			success &= !out.HadError();
		}
		if (!success) {
			//fprintf(stderr, "Failed to write\n");
			rv = CCDM_ERROR_WRITE_VAR;
		}
		else {
			success &= input.SerializeToZeroCopyStream(outStream);
			if (!success) {
				//fprintf(stderr, "Failed to write %s\n", input.DebugString().c_str());
				rv = CCDM_ERROR_WRITE;
			}
		}
		_fs->flushOutputStream();
 	}
out:
	return rv;
}

DWORD
readMessage(CCDMonSrv::REQOUTPUT& output)
{
	DWORD rv=0;
	bool success=true;
	google::protobuf::uint32 bytes=0;

	if (hPipe == NULL) {
		rv = CCDM_ERROR_HANDLE;
	}
	else {
		if (_fs->getInputSteamErr() || _fs->getOutputStreamErr()) {
			rv = CCDM_ERROR_HANDLE;
			goto out;
		}

		/// stream for reading response
		google::protobuf::io::ZeroCopyInputStream *inStream = _fs->getInputSteam();
		{
			google::protobuf::io::CodedInputStream in(inStream);
			success &= in.ReadVarint32(&bytes);
		}
		if (!success) {	
			//fprintf(stderr, "failed to read size\n");
			rv = CCDM_ERROR_READ_VAR;
		}
		else {
			google::protobuf::int64 bytesReadPre = inStream->ByteCount();
			if (bytes > 0) {
				success &= output.ParseFromBoundedZeroCopyStream(inStream, (int)bytes);
				if (success) {
					//fprintf(stderr, "%s", output.DebugString().c_str());
					rv = CCDM_ERROR_SUCCESS;
				}
				else {
					int rt = _fs->getInputSteamErr();
					if (rt == 0) {
						google::protobuf::int64 bytesReadPost = inStream->ByteCount();
						google::protobuf::uint32 bytesRead = (google::protobuf::uint32)(bytesReadPost - bytesReadPre);
						if (bytesRead < bytes) {
							inStream->Skip((int)(bytes - bytesRead));
						}
						//fprintf(stderr, "%s", output.DebugString().c_str());
						rv = CCDM_ERROR_SUCCESS;
					}
					else {
						//fprintf(stderr, "read failed, error = %s\n", rt);
						rv = CCDM_ERROR_READ;
					}
				}
			}
		}
	}
out:
	return rv;
}

void releasePipe()
{
	try {
		delete _fs;
	}
	catch (std::bad_alloc& e) {
        fprintf(stderr, "allocation failed: %s", e.what());
    }
	catch (std::exception& e) {
        fprintf(stderr, "caught unexpected exception: %s", e.what());
    }

	CloseHandle(hPipe);
	hPipe = NULL;
}

#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <ccdi.hpp>
#include "log.h"

#define OTHERS 0
#define SYNCED 1
#define SYNCING 2
#define TO_BE_SYNCED 3
#define COMPUTING 4

class ShlExtUtility
{
protected: 
	//static std::ofstream myfile;

public:
	ShlExtUtility(void);
	~ShlExtUtility(void);

	static int ConvertToTString(const char *astring, TCHAR *string);
	static int ConvertFromTString(const TCHAR* tstring, char* astring, int abuflen);
	
	//windows api
	static void CopyItem(LPCTSTR source, LPCTSTR destination);
	static void MoveItem(LPCTSTR source, LPCTSTR destination);

	//ccdi api calls
	static int ListSyncFolderName(std::vector<std::string>& syncFolderName, std::vector<std::string>& deviceRootPath);
	static int GetMyCloudRootPath(std::string& myCldPath);
	static int GetSyncStatus(std::string pathStr, int& status);
	static BOOL IsUdrMyCloudRoot(TCHAR* path, TCHAR* rootPath);
	static int GetType(TCHAR* path);
	static int GetURLPrefix(std::string& url);
	static int SyncFolderCheck(std::string pathStr, BOOL& isSyncFolder, BOOL& inSyncFolder);
	static void SyncFolderCheck(std::vector<std::string> deviceRootPath, std::string filePath, BOOL& isSyncFolder, BOOL& inSyncFolder);
	static int GetLogDirectory(LPTSTR path);
	static int GetDirectoryStr(TCHAR* path, std::string rootPath, std::string& dirStr); 
	static int SetLogLevel();

	static int getDeviceId(u64& deviceId);
	static int getUserId(u64& userId);
	static int getArchiveStorageDeviceId(u64 userId, u64& archiveStorageDeviceId);
	static int dumpDatasetList(u64 userId);
	static BOOL isOnline();

	static void sendShellChangeNotify(LPCWSTR pwszPath);
};

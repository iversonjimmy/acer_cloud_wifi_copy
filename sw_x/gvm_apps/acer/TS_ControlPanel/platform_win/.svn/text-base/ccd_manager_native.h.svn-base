//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#pragma once
#pragma unmanaged

#include <ccdi.hpp>
#include <cstring>
#include <vector>

struct DirEntry
{
    bool isDir;
    std::wstring name;
};

struct DataSet
{
    bool isSubscribed;
    std::wstring filter;
    std::wstring name;
    std::wstring currentDirName;
    vplex::vsDirectory::DatasetType type;
    ::google::protobuf::uint64 id;
    std::vector<DirEntry> currentDirEntries;
};

//Add by Vincent, 20111006
struct CloudDeviceInfo
{
	::google::protobuf::uint64 deviceID;
	std::wstring deviceclass;
	std::wstring devicename;
	bool isAcerMachine;
	bool isAndroidDevice;
};

class CcdManagerNative
{
public:
    CcdManagerNative();
    ~CcdManagerNative();

    // long function names are used to make managed/unmanaged interop easier
    // class and struct interop is much more complicated than strings and ints
    // ideally class and struct interop would be used

    int          AddSubscription(int dataSetIndex);
    int          DeleteSubscription(int dataSetIndex);
    std::wstring GetDataSetCurrentDirName(int index);
    std::wstring GetDataSetCurrentDirEntryName(int dataSetIndex, int entryIndex);
    std::wstring GetDataSetName(int index);
    int          GetDataSetType(int index);
    std::wstring GetDeviceName();
    std::wstring GetFilter(int dataSetIndex);
    std::wstring GetLocation();
    std::wstring GetUrl();
    bool         IsDataSetCurrentDirEntryDir(int dataSetIndex, int entryIndex);
    bool         IsDeviceLinked();
    bool         IsLoggedIn();
    bool         IsSubscribed(int dataSetIndex);
    bool         IsSync();
    int          Link(const wchar_t* deviceName);
    int          Login(const wchar_t* user);
    int          Login(const wchar_t* user, const wchar_t* pass);
    int          Logout();
    int          NumDataSetCurrentDirEntries(int dataSetIndex);
    int          NumDataSets();
    int          NumFilesDownload();
    int          NumFilesUpload();
    int          Register(const wchar_t* user, const wchar_t* pass, const wchar_t* email);
	int          RegisterStorageNode();
	int          UnregisterStorageNode();
    int          SetDataSetCurrentDir(int index, const wchar_t* path);
    int          SetDeviceName(const wchar_t* name);
    int          SetLocation(const wchar_t* path);
    int          SetFilter(int dataSetIndex, const wchar_t* filter);
    int          SetSync(bool enable);
    int          Unlink();
	// Add by Vincent, 20111006
	int			 Unlink(::google::protobuf::uint64 deviceID);
    int          UpdateDataSets();
	// Add by Vincent, 20111006
	int          ListLinkedDevices();
	int			 NumDevices();
	std::wstring GetDeviceInfo_Name(int index);
	std::wstring GetDeviceInfo_Class(int index);
	::google::protobuf::int64 GetDeviceInfo_ID(int index);
	bool		 GetDeviceInfo_IsAcerMachine(int index);
	bool		 GetDeviceInfo_IsAndroidDevice(int index);

private:
    std::vector<::DataSet> mDataSets;
    ::google::protobuf::uint64 mUserId;
	// Add by Vincent, 20111006
	std::vector<::CloudDeviceInfo> mDeviceInfo;

    int          AddSubscription(int dataSetIndex, ccd::SyncSubscriptionType_t type);
    std::string  ConvertToUtf8(const wchar_t* buffer, int length);
    std::string  ConvertToUtf8(const std::wstring& input);
    std::wstring ConvertToUtf16(const char* buffer, int length);
    std::wstring ConvertToUtf16(const std::string& input);
};

#pragma managed

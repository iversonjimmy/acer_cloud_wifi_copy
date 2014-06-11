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

#include "ccd_manager_native.h"

using namespace System;

public ref class CcdManager 
{
public:
    static property CcdManager^ Instance
    {
        CcdManager^ get()
        {
            return mInstance;
        }
    }

    property String^ DeviceName { String^ get(); void set(String^ value); }
    property bool    IsDeviceLinked { bool get(); }
    property bool    IsLoggedIn { bool get(); }
	property bool    IsLogInPSN { bool get(); }
    property bool    IsSync { bool get(); void set(bool value); }
    property String^ Language { String^ get(); void set(String^ value); }
    property String^ Location { String^ get(); void set(String^ value); }
    property int     NumDataSets { int get(); }
    property int     NumFilesDownload { int get(); }
    property int     NumFilesUpload { int get(); }
    property bool    ShowNotify { bool get(); void set(bool value); }
    property String^ UserName { String^ get(); }
    property String^ Url { String^ get(); }
    property String^ UrlForgetPassword { String^ get(); }
    property String^ UrlPrivacy { String^ get(); }
    property String^ UrlTermsService { String^ get(); }

    void    AddSubscription(int dataSetIndex);
    void    DeleteSubscription(int dataSetIndex);
    String^ GetCurrentDirName(int dataSetIndex);
    String^ GetCurrentDirEntryName(int dataSetIndex, int entryIndex);
    String^ GetDataSetName(int index);
    String^ GetFilter(int dataSetIndex);
    bool    IsCurrentDirEntryDir(int dataSetIndex, int entryIndex);
    bool    IsSubscribed(int dataSetIndex);
    void    Link(String^ deviceName);
    void    LoadSettings();
    void    Login(String^ user);
    void    Login(String^ user, String^ pass);
    void    Logout(bool save);
    int	  NumCurrentDirEntries(int dataSetIndex);
    void    Register(String^ user, String^ pass, String^ email);
	void		RegisterStorageNode();
	void	UnregisterStorageNode();
    void    SaveSettings();
    void    SetCurrentDir(int dataSetIndex, String^ path);
    void    SetFilter(int dataSetIndex, String^ filter);
    void    Unlink();
	// Add by Vincent, 20111006, Unlink with device ID.
	void    Unlink(Int64 deviceID);
    void    UpdateDataSets();
	// Add by Vincent, 20111006, Unlink with device ID.
	void    UpdateLinkedDevices();
	property int     NumDevices { int get(); }
	String^ GetDeviceName(int index);
	String^ GetDeviceClass(int index);
	bool	IsAcerMachine(int index);
	bool	IsAndroidDevice(int index);
	Int64	GetDeviceID(int index);

private:
    static CcdManager^ mInstance = gcnew CcdManager();
    static String^ settingsFile = L"\\iGware\\SyncAgent\\settings.txt";

    CcdManagerNative* mNative;
    String^           mLanguage;    // TODO: replace with CCDI
    String^           mUserName;
    bool              mShowNotify;
	bool				mIsLoginPSN;

    CcdManager();
    ~CcdManager();

    void SetDefaults();
};

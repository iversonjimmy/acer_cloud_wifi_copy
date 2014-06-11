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

using namespace System;

[Serializable]
public ref class Settings 
{
public:
    Settings();
    Settings(Settings^ value);
    ~Settings();

    property String^ DeviceName { String^ get(); void set(String^ value); }
    property int     DownloadRate { int get(); void set(int value); }
    property String^ ForgetPassUrl { String^ get(); void set(String^ value); }
    property bool    IsStartup { bool get(); void set(bool value); }
    property bool    IsStorageNode { bool get(); void set(bool value); }
    property bool    IsSync { bool get(); void set(bool value); }
    property String^ Language { String^ get(); void set(String^ value); }
    property bool    LimitDownload { bool get(); void set(bool value); }
    property bool    LimitUpload { bool get(); void set(bool value); }
    property bool    LimitUploadAuto { bool get(); void set(bool value); }
    property String^ Location { String^ get(); void set(String^ value); }
    property String^ PrivacyUrl { String^ get(); void set(String^ value); }
    property String^ ProxyPassword { String^ get(); void set(String^ value); }
    property int     ProxyPort { int get(); void set(int value); }
    property String^ ProxyServer { String^ get(); void set(String^ value); }
    property String^ ProxyType { String^ get(); void set(String^ value); }
    property String^ ProxyUsername { String^ get(); void set(String^ value); }
    property bool    ShowNotify { bool get(); void set(bool value); }
    property String^ StorageNodeLocation { String^ get(); void set(String^ value); }
    property String^ TermsServiceUrl { String^ get(); void set(String^ value); }
    property int     UploadRate { int get(); void set(int value); }
    property String^ Url { String^ get(); void set(String^ value); }
    property bool    UseProxy { bool get(); void set(bool value); }
    property bool    UseProxyAuto { bool get(); void set(bool value); }
    property bool    UseProxyPassword { bool get(); void set(bool value); }
    property String^ Username { String^ get(); void set(String^ value); }

private:
    bool    mIsStartup;
    bool    mIsStorageNode;
    bool    mIsSync;
    bool    mLimitDownload;
    bool    mLimitUpload;
    bool    mLimitUploadAuto;
    bool    mShowNotify;
    bool    mUseProxy;
    bool    mUseProxyAuto;
    bool    mUseProxyPassword;
    int     mDownloadRate;      // kB/s
    int     mProxyPort;
    int     mUploadRate;        // kB/s
    String^ mDeviceName;
    String^ mForgetPassUrl;
    String^ mLanguage;
    String^ mLocation;
    String^ mPrivacyUrl;
    String^ mProxyPassword;
    String^ mProxyServer;
    String^ mProxyType;
    String^ mProxyUsername;
    String^ mStorageNodeLocation;
    String^ mTermsServiceUrl;
    String^ mUrl;
    String^ mUsername;
};

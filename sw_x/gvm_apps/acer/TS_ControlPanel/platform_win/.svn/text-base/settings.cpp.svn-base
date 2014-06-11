//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "stdafx.h"
#include "settings.h"

Settings::Settings()
{
    mIsStartup = true;
    mIsStorageNode = false;
    mIsSync = true;
    mLimitDownload = false;
    mLimitUpload = true;
    mLimitUploadAuto = true;
    mUseProxy = true;
    mUseProxyAuto = true;
    mUseProxyPassword = false;
    mShowNotify = true;
    mDownloadRate = 100;
    mProxyPort = 8080;
    mUploadRate = 10;
    mDeviceName = Environment::MachineName->ToLower();
    mForgetPassUrl = L"https://ops.lab1.routefree.com/ops/forgotpassword";
    mLanguage = L"English";
    mLocation = L"c:\\windows\\";
    mPrivacyUrl = L"https://ops.lab1.routefree.com/ops/welcome";
    mProxyPassword = L"";
    mProxyServer = L"";
    mProxyType = L"HTTP";
    mProxyUsername = L"";
    mStorageNodeLocation = L"c:\\windows\\";
    mTermsServiceUrl = L"https://ops.lab1.routefree.com/ops/welcome";
    mUrl = L"https://ops.lab1.routefree.com/ops/welcome";
    mUsername = L"bob";
}

Settings::Settings(Settings^ value)
{
    mIsStartup = value->IsStartup;
    mIsStorageNode = value->IsStorageNode;
    mIsSync = value->IsSync;
    mLimitDownload = value->LimitDownload;
    mLimitUpload = value->LimitUpload;
    mLimitUploadAuto = value->LimitUploadAuto;
    mUseProxy = value->UseProxy;
    mUseProxyAuto = value->UseProxyAuto;
    mUseProxyPassword = value->UseProxyPassword;
    mShowNotify = value->ShowNotify;
    mDownloadRate = value->DownloadRate;
    mProxyPort = value->ProxyPort;
    mUploadRate = value->UploadRate;
    mDeviceName = value->DeviceName;
    mForgetPassUrl = value->ForgetPassUrl;
    mLanguage = value->Language;
    mLocation = value->Location;
    mPrivacyUrl = value->PrivacyUrl;
    mProxyPassword = value->ProxyPassword;
    mProxyServer = value->ProxyServer;
    mProxyType = value->ProxyType;
    mProxyUsername = value->ProxyUsername;
    mStorageNodeLocation = value->StorageNodeLocation;
    mTermsServiceUrl = value->TermsServiceUrl;
    mUrl = value->Url;
    mUsername = value->Username;
}

Settings::~Settings()
{
    // do nothing, memory is managed
}

String^ Settings::DeviceName::get()
{
    return mDeviceName;
}

void Settings::DeviceName::set(String^ value)
{
    mDeviceName = value;
}

int Settings::DownloadRate::get()
{
    return mDownloadRate;
}

void Settings::DownloadRate::set(int value)
{
    mDownloadRate = value;
}

String^ Settings::ForgetPassUrl::get()
{
    return mForgetPassUrl;
}

void Settings::ForgetPassUrl::set(String^ value)
{
    mForgetPassUrl = value;
}

bool Settings::IsStartup::get()
{
    return mIsStartup;
}

void Settings::IsStartup::set(bool value)
{
    mIsStartup = value;
}

bool Settings::IsStorageNode::get()
{
    return mIsStorageNode;
}

void Settings::IsStorageNode::set(bool value)
{
    mIsStorageNode = value;
}

bool Settings::IsSync::get()
{
    return mIsSync;
}

void Settings::IsSync::set(bool value)
{
    mIsSync = value;
}

String^ Settings::Language::get()
{
    return mLanguage;
}

void Settings::Language::set(String^ value)
{
    mLanguage = value;
}

bool Settings::LimitDownload::get()
{
    return mLimitDownload;
}

void Settings::LimitDownload::set(bool value)
{
    mLimitDownload = value;
}

bool Settings::LimitUpload::get()
{
    return mLimitUpload;
}

void Settings::LimitUpload::set(bool value)
{
    mLimitUpload = value;
}

bool Settings::LimitUploadAuto::get()
{
    return mLimitUploadAuto;
}

void Settings::LimitUploadAuto::set(bool value)
{
    mLimitUploadAuto = value;
}

String^ Settings::Location::get()
{
    return mLocation;
}

void Settings::Location::set(String^ value)
{
    mLocation = value;
}

String^ Settings::PrivacyUrl::get()
{
    return mPrivacyUrl;
}

void Settings::PrivacyUrl::set(String^ value)
{
    mPrivacyUrl = value;
}

String^ Settings::ProxyPassword::get()
{
    return mProxyPassword;
}

void Settings::ProxyPassword::set(String^ value)
{
    mProxyPassword = value;
}

int Settings::ProxyPort::get()
{
    return mProxyPort;
}

void Settings::ProxyPort::set(int value)
{
    mProxyPort = value;
}

String^ Settings::ProxyServer::get()
{
    return mProxyServer;
}

void Settings::ProxyServer::set(String^ value)
{
    mProxyServer = value;
}

String^ Settings::ProxyType::get()
{
    return mProxyType;
}

void Settings::ProxyType::set(String^ value)
{
    mProxyType = value;
}

String^ Settings::ProxyUsername::get()
{
    return mProxyType;
}

void Settings::ProxyUsername::set(String^ value)
{
    mProxyType = value;
}

bool Settings::ShowNotify::get()
{
    return mShowNotify;
}

void Settings::ShowNotify::set(bool value)
{
    mShowNotify = value;
}

String^ Settings::StorageNodeLocation::get()
{
    return mStorageNodeLocation;
}

void Settings::StorageNodeLocation::set(String^ value)
{
    mStorageNodeLocation = value;
}

String^ Settings::TermsServiceUrl::get()
{
    return mTermsServiceUrl;
}

void Settings::TermsServiceUrl::set(String^ value)
{
    mTermsServiceUrl = value;
}

int Settings::UploadRate::get()
{
    return mUploadRate;
}

void Settings::UploadRate::set(int value)
{
    mUploadRate = value;
}

String^ Settings::Url::get()
{
    return mUrl;
}

void Settings::Url::set(String^ value)
{
    mUrl = value;
}

bool Settings::UseProxy::get()
{
    return mUseProxy;
}

void Settings::UseProxy::set(bool value)
{
    mUseProxy = value;
}

bool Settings::UseProxyAuto::get()
{
    return mUseProxyAuto;
}

void Settings::UseProxyAuto::set(bool value)
{
    mUseProxyAuto = value;
}

bool Settings::UseProxyPassword::get()
{
    return mUseProxyPassword;
}

void Settings::UseProxyPassword::set(bool value)
{
    mUseProxyPassword = value;
}

String^ Settings::Username::get()
{
    return mUsername;
}

void Settings::Username::set(String^ value)
{
    mUsername = value;
}

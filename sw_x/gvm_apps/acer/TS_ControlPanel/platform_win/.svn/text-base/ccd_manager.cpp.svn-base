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
#include "ccd_manager.h"
#include "util_funcs.h"
#include "logger.h"

#using <mscorlib.dll>
#using <Interop\Interop.IWshRuntimeLibrary.1.0.dll>

using namespace IWshRuntimeLibrary;
using namespace System::IO;
using namespace System::Runtime::InteropServices;
using namespace System::Runtime::Serialization;
using namespace System::Runtime::Serialization::Formatters::Binary;
using namespace System::Web;
using namespace System::Windows::Forms;

CcdManager::CcdManager()
{
    mNative = new CcdManagerNative();
    this->SetDefaults();
	mIsLoginPSN = false;
}

CcdManager::~CcdManager()
{
    if (mNative) {
        delete mNative;
    }
}

// PROPERTIES

String^ CcdManager::DeviceName::get()
{
    return gcnew String(mNative->GetDeviceName().c_str());
}

void CcdManager::DeviceName::set(String^ value)
{
    wchar_t* nameNative = (wchar_t*)Marshal::StringToHGlobalUni(value).ToPointer();

    int rv = mNative->SetDeviceName(nameNative);

    Marshal::FreeHGlobal(IntPtr(nameNative));

    Logger::Instance->WriteLine(L"SetDeviceName device(" + value + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Set device name failed: " + rv.ToString());
    }
}

bool CcdManager::IsDeviceLinked::get()
{
    return mNative->IsDeviceLinked();
}

bool CcdManager::IsLoggedIn::get()
{
    return mNative->IsLoggedIn();
}

bool CcdManager::IsLogInPSN::get()
{
    return mIsLoginPSN;
}

bool CcdManager::IsSync::get()
{
    return mNative->IsSync();
}

void CcdManager::IsSync::set(bool value)
{
    int rv = mNative->SetSync(value);

    Logger::Instance->WriteLine(L"SetSync enable(" + value + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Set sync failed: " + rv.ToString());
    }
}

String^ CcdManager::Language::get()
{
    return mLanguage;
}

void CcdManager::Language::set(String^ value)
{
    mLanguage = value;
    this->SaveSettings();
}

String^ CcdManager::Location::get()
{
    return gcnew String(mNative->GetLocation().c_str());
}

void CcdManager::Location::set(String^ value)
{
    String^ old = this->Location + L"\\My Cloud";

    wchar_t* pathNative = (wchar_t*)Marshal::StringToHGlobalUni(value).ToPointer();

    int rv = mNative->SetLocation(pathNative);

    Marshal::FreeHGlobal(IntPtr(pathNative));

    Logger::Instance->WriteLine(L"SetLocation path(" + value + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Set location failed: " + rv.ToString());
    }
}

int CcdManager::NumDataSets::get()
{
    return mNative->NumDataSets();
}

int CcdManager::NumFilesDownload::get()
{
    return mNative->NumFilesDownload();
}

int CcdManager::NumFilesUpload::get()
{
    return mNative->NumFilesUpload();
}

//String^ CcdManager::Password::get()
//{
//    return mPassword;
//}

bool CcdManager::ShowNotify::get()
{
    return mShowNotify;
}

void CcdManager::ShowNotify::set(bool value)
{
    mShowNotify = value;
    this->SaveSettings();
}

String^ CcdManager::UserName::get()
{
    return mUserName;
}

String^ CcdManager::Url::get()
{
    return gcnew String(mNative->GetUrl().c_str());
}

String^ CcdManager::UrlForgetPassword::get()
{
    return this->Url + L"/ops/forgotpassword";
}

String^ CcdManager::UrlPrivacy::get()
{
    return this->Url + L"";
}

String^ CcdManager::UrlTermsService::get()
{
    return this->Url + L"";
}

// METHODS

void CcdManager::AddSubscription(int dataSetIndex)
{
    int rv = mNative->AddSubscription(dataSetIndex);

    Logger::Instance->WriteLine(L"Add subscription name("
        + this->GetDataSetName(dataSetIndex) + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Add subscription name("
            + this->GetDataSetName(dataSetIndex) + L"): " + rv.ToString());
    }
}

void CcdManager::DeleteSubscription(int dataSetIndex)
{
    int rv = mNative->DeleteSubscription(dataSetIndex);

    Logger::Instance->WriteLine(L"Delete subscription name("
        + this->GetDataSetName(dataSetIndex) + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Delete subscription name("
            + this->GetDataSetName(dataSetIndex) + L"): " + rv.ToString());
    }
}

String^ CcdManager::GetCurrentDirName(int dataSetIndex)
{
    return gcnew String(mNative->GetDataSetCurrentDirName(dataSetIndex).c_str());
}

String^ CcdManager::GetCurrentDirEntryName(int dataSetIndex, int entryIndex)
{
    return gcnew String(mNative->GetDataSetCurrentDirEntryName(dataSetIndex, entryIndex).c_str());
}

String^ CcdManager::GetDataSetName(int index)
{
    return gcnew String(mNative->GetDataSetName(index).c_str());
}

String^ CcdManager::GetFilter(int dataSetIndex)
{
    return gcnew String(mNative->GetFilter(dataSetIndex).c_str());
}

bool CcdManager::IsCurrentDirEntryDir(int dataSetIndex, int entryIndex)
{
    return mNative->IsDataSetCurrentDirEntryDir(dataSetIndex, entryIndex);
}

bool CcdManager::IsSubscribed(int dataSetIndex)
{
    return mNative->IsSubscribed(dataSetIndex);
}

void CcdManager::Link(String^ deviceName)
{
    wchar_t* nameNative = (wchar_t*)Marshal::StringToHGlobalUni(deviceName).ToPointer();

    int rv = mNative->Link(nameNative);

    Marshal::FreeHGlobal(IntPtr(nameNative));

    Logger::Instance->WriteLine(L"Link device(" + deviceName + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Link device failed: " + rv.ToString());
    }
}

void CcdManager::LoadSettings()
{
    cli::array<wchar_t>^ separator = gcnew cli::array<wchar_t>(1);
    separator[0] = '=';

    this->SetDefaults();

    try {
        String^ fileName = Environment::GetFolderPath(Environment::SpecialFolder::LocalApplicationData);
        fileName += settingsFile;
        StreamReader^ reader = System::IO::File::OpenText(fileName);
        while (reader->EndOfStream == false) {
            String^ line = reader->ReadLine();
            Logger::Instance->WriteLine(L"load setting: " + line);
            if (line->Contains("=") == true) {
                cli::array<String^>^ tokens = line->Split(separator);
                if (tokens->Length == 2) {
                    if (tokens[0] == L"Language") {
                        mLanguage = tokens[1];
                    } else if (tokens[0] == L"ShowNotify") {
                        mShowNotify = Convert::ToBoolean(tokens[1]);
                    } else if (tokens[0] == L"UserName") {
                        mUserName = tokens[1];
                    }
                }
            }
        }
        reader->Close();
    } catch (Exception^) {
        this->SetDefaults();
    }
}

void CcdManager::Login(String^ user)
{
    wchar_t* userNative = (wchar_t*)Marshal::StringToHGlobalUni(user).ToPointer();

    int rv = mNative->Login(userNative);

    Marshal::FreeHGlobal(IntPtr(userNative));

    Logger::Instance->WriteLine(L"Login user(" + user + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Login failed: " + rv.ToString());
    } else {
        mUserName = user;
        this->SaveSettings();
    }
}

void CcdManager::Login(String^ user, String^ pass)
{
    wchar_t* userNative = (wchar_t*)Marshal::StringToHGlobalUni(user).ToPointer();
    wchar_t* passNative = (wchar_t*)Marshal::StringToHGlobalUni(pass).ToPointer();

    int rv = mNative->Login(userNative, passNative);

    Marshal::FreeHGlobal(IntPtr(userNative));
    Marshal::FreeHGlobal(IntPtr(passNative));

    Logger::Instance->WriteLine(L"Login user("
        + user + L") pass(" + pass + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Login failed: " + rv.ToString());
    } else {
        mUserName = user;
        this->SaveSettings();
    }
}

void CcdManager::Logout(bool save)
{
    int rv = mNative->Logout();
	
    Logger::Instance->WriteLine(L"Logout user(" + mUserName + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Logout failed: " + rv.ToString());
    } else {
        mUserName = L"";
        if (save == true) {
            this->SaveSettings();
        }
    }
}

int CcdManager::NumCurrentDirEntries(int dataSetIndex)
{
    return mNative->NumDataSetCurrentDirEntries(dataSetIndex);
}

void CcdManager::RegisterStorageNode() {
	int rv = mNative->RegisterStorageNode();
	Logger::Instance->WriteLine(L"Register Storage Node: " + rv.ToString());
    if (rv != 0 && rv != -9097 /*VPL_ERR_ALREADY - already registered (9/16/11)*/) 
	{
		mIsLoginPSN = false;
        throw gcnew Exception(L"Register Storage Node failed: " + rv.ToString());
    }
	else 
		mIsLoginPSN = true;

	UtilFuncs::NotifyLoginAndPSNChange();
	
}

void CcdManager::UnregisterStorageNode() {
	int rv = mNative->UnregisterStorageNode();
	mIsLoginPSN = false;
	Logger::Instance->WriteLine(L"Unregister Storage Node: " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Unregister Storage Node failed: " + rv.ToString());
    }

	UtilFuncs::NotifyLoginAndPSNChange();
} 

void CcdManager::Register(String^ user, String^ pass, String^ email)
{
    String^ emailUrl = HttpUtility::UrlEncode(email);

    wchar_t* userNative = (wchar_t*)Marshal::StringToHGlobalUni(user).ToPointer();
    wchar_t* passNative = (wchar_t*)Marshal::StringToHGlobalUni(pass).ToPointer();
    wchar_t* emailNative = (wchar_t*)Marshal::StringToHGlobalUni(emailUrl).ToPointer();

    int rv = mNative->Register(userNative, passNative, emailNative);

    Marshal::FreeHGlobal(IntPtr(userNative));
    Marshal::FreeHGlobal(IntPtr(passNative));
    Marshal::FreeHGlobal(IntPtr(emailNative));

    Logger::Instance->WriteLine(L"Register user("
        + user + L") pass(" + pass + L") email(" + email + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Register failed: " + rv.ToString());
    }
}

void CcdManager::SaveSettings()
{
    try {
        String^ fileName = Environment::GetFolderPath(Environment::SpecialFolder::LocalApplicationData);
        fileName += settingsFile;
        StreamWriter^ writer = System::IO::File::CreateText(fileName);
        writer->WriteLine(L"Language=" + mLanguage);
        writer->WriteLine(L"ShowNotify=" + mShowNotify.ToString());
        writer->WriteLine(L"UserName=" + mUserName);
		writer->Close();

		Microsoft::Win32::RegistryKey ^ rk = Microsoft::Win32::Registry::CurrentUser;
		Microsoft::Win32::RegistryKey^ autorunKey = rk->CreateSubKey("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
		rk->Close();
		
		if(autorunKey != nullptr)	
		{
			if(mUserName->Equals(System::String::Empty))
			{
				autorunKey->DeleteValue(L"acerCloud Main App");
				//Delete autorun registry value
			}
			else
			{
				autorunKey->SetValue(L"acerCloud Main App", System::Reflection::Assembly::GetExecutingAssembly()->Location);
				//Set autorun registry value
			}
			autorunKey->Close();
		}

        
    } catch (Exception^) {
        // do nothing
    }
}

void CcdManager::SetCurrentDir(int dataSetIndex, String^ path)
{
    wchar_t* pathNative = (wchar_t*)Marshal::StringToHGlobalUni(path).ToPointer();

    int rv = mNative->SetDataSetCurrentDir(dataSetIndex, pathNative);

    Marshal::FreeHGlobal(IntPtr(pathNative));

    Logger::Instance->WriteLine(L"SetCurrentDir dataset("
        + this->GetDataSetName(dataSetIndex) + L") path(" + path + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"SetCurrentDir failed: " + rv.ToString());
    }
}

void CcdManager::SetDefaults()
{
    mLanguage = L"English";
    mUserName = L"";
    mShowNotify = true;
}

void CcdManager::SetFilter(int dataSetIndex, String^ filter)
{
    wchar_t* filterNative = (wchar_t*)Marshal::StringToHGlobalUni(filter).ToPointer();

    int rv = mNative->SetFilter(dataSetIndex, filterNative);

    Marshal::FreeHGlobal(IntPtr(filterNative));

    Logger::Instance->WriteLine(L"SetFilter dataset("
        + this->GetDataSetName(dataSetIndex) + L") filter( " + filter + L" ): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"SetFilter failed: " + rv.ToString());
    }
}

void CcdManager::Unlink()
{
    int rv = mNative->Unlink();

    Logger::Instance->WriteLine(L"Unlink device(" + this->DeviceName + L"): " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Unlink device failed: " + rv.ToString());
    }
}


//Add by Vincent, 20111006, Unlink with device ID
void CcdManager::Unlink(Int64 deviceID)
{
	int rv = mNative->Unlink(deviceID);

	Logger::Instance->WriteLine(L"Unlink device(" + this->DeviceName + L"): " + rv.ToString());
	if (rv != 0) {
		throw gcnew Exception(L"Unlink device failed: " + rv.ToString());
	}
}

void CcdManager::UpdateDataSets()
{
    int rv = mNative->UpdateDataSets();

    Logger::Instance->WriteLine(L"Update datasets: " + rv.ToString());
    if (rv != 0) {
        throw gcnew Exception(L"Update datasets failed: " + rv.ToString());
    }
}

//Add by Vincent, 20111006, Unlink with device ID
void CcdManager::UpdateLinkedDevices()
{
	int rv = mNative->ListLinkedDevices();

	Logger::Instance->WriteLine(L"Update linked devices: " + rv.ToString());
	if (rv != 0) {
		throw gcnew Exception(L"Update linked datasets failed: " + rv.ToString());
	}
}

int CcdManager::NumDevices::get()
{
	return mNative->NumDevices();
}

String^ CcdManager::GetDeviceName(int index)
{
	return gcnew String(mNative->GetDeviceInfo_Name(index).c_str());
}

String^ CcdManager::GetDeviceClass(int index)
{
	return gcnew String(mNative->GetDeviceInfo_Class(index).c_str());
}

bool CcdManager::IsAcerMachine(int index)
{
	return mNative->GetDeviceInfo_IsAcerMachine(index);
}

bool CcdManager::IsAndroidDevice(int index)
{
	return mNative->GetDeviceInfo_IsAndroidDevice(index);
}

Int64 CcdManager::GetDeviceID(int index)
{
	return mNative->GetDeviceInfo_ID(index);
}
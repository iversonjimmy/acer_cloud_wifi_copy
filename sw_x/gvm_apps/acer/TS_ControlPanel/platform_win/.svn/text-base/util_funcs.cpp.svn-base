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
#include "util_funcs.h"
#include "logger.h"
#include "ccd_manager.h"
#include "PlayerOptions.h"
#include "WM_USER.h"

#using <System.dll>

using namespace System::ComponentModel;
using namespace System::Diagnostics;
using namespace System::IO;
using namespace System::Threading;
using namespace System::Windows::Forms;
using namespace System::Management;


String^ UtilFuncs::GetUserHomeDir()
{
    return Environment::ExpandEnvironmentVariables("%HOMEDRIVE%%HOMEPATH%");
}

String^ UtilFuncs::GetVersion()
{
    try {
        FileInfo^ info = gcnew FileInfo(Application::ExecutablePath);
        return "v" + File::ReadAllText(info->DirectoryName + "\\Product.Version");
    } catch (Exception^) {
        return L"v0.0.0";
    }
}

void UtilFuncs::ShowPath(String^ path)
{
    try {
        Process::Start(path);
    } catch (Exception^ e) {
        MessageBox::Show(e->Message);
    }
}

void UtilFuncs::ShowUrl(String^ url)
{
    try {
        Process::Start(url);
    } catch (Win32Exception^ noBrowser) {
        if (noBrowser->ErrorCode == -2147467259) {
            MessageBox::Show(noBrowser->Message);
        }
    } catch (Exception^ e) {
        MessageBox::Show(e->Message);
    }
}

//Filter the charaters in chars from input, and return the result.
String^ UtilFuncs::StripChars(String^ input, String^ chars)
{
    try {
        String^ output = L"";
        for (int i = 0; i < input->Length; i++) {
            bool addChar = true;
            for (int j = 0; j < chars->Length; j++) {
                if (input[i] == chars[j]) {
                    addChar = false;
                }
            }
            if (addChar == true) {
                output += input[i];
            }
        }
        return output;
    } catch (Exception^) {
        return input;
    }
}

//replace '\" in input with '/'
// only used for filtering
String^ UtilFuncs::StripDataSet(String^ input)
{
    cli::array<wchar_t>^ separator = gcnew cli::array<wchar_t>(1) { L'\\' };
    cli::array<String^>^ tokens = input->Split(separator);

    // strip top-level directory from path
    String^ path = L"";
    for (int i = 1; i < tokens->Length; i++) {
        path += L"/" + tokens[i];
    }

    return path;
}

//delete storageNode process if exists, and restart it.
// HACK
//void UtilFuncs::StartStorageNode(String^ user, String^ pass, int waitMS, bool startNew)
//{
//
//	return; // 2011 Sep. 19 . PSN and CCD are combined. do not process this funciton. (Guess)
//
//
//    // kill all storageNode.exe processes
//    cli::array<Process^>^ list = Process::GetProcesses();
//    for (int i = 0; i < list->Length; i++) {
//        if (list[i]->ProcessName == L"storageNode") {
//            if (startNew == false) {
//                return;
//            }
//            list[i]->Kill();
//            list[i]->WaitForExit();
//        }
//    }
//
//    Process^ psn = gcnew Process();
//    psn->StartInfo->FileName = Environment::GetFolderPath(Environment::SpecialFolder::ProgramFiles);
//    psn->StartInfo->FileName += L"\\iGware\\StorageNode\\storageNode.exe";
//    psn->StartInfo->Arguments = L"-u \"" + user + L"\" -p \"" + pass + L"\"";
//    psn->StartInfo->UseShellExecute = false;
//    psn->StartInfo->CreateNoWindow = true;
//
//    Logger::Instance->WriteLine("starting " + psn->StartInfo->FileName + " " + psn->StartInfo->Arguments + "...");
//    psn->Start();
//
//    if (waitMS > 0) {
//        Thread::Sleep(waitMS);
//    }
//}
//


bool UtilFuncs::CheckAcerWMI()
{
	try
	{
		Microsoft::Win32::RegistryKey^ rk;
		rk = Microsoft::Win32::Registry ::LocalMachine->OpenSubKey(L"Software\\OEM\\Metadata", false);
		if (rk!=nullptr)
		{
			//key doesn't exist -- create it
			String^ brand = rk->GetValue(L"Brand")->ToString();
			if(brand->ToLower()->Equals(L"acer"))
			{
				return true;
			}
		}
	}
	catch(Exception^ e)
	{
	
	}

	ManagementObjectSearcher^ searcher = gcnew ManagementObjectSearcher(L"root\\CIMV2", "SELECT * FROM Win32_ComputerSystem");
	ManagementObject^ queryObj = gcnew ManagementObject();

	for each (queryObj in searcher->Get())
    {
		String^ Manufacturer = (queryObj["Manufacturer"])->ToString();
		System::Diagnostics::Debug::WriteLine(L"Manufacture: " + Manufacturer);
		
		if(Manufacturer->ToLower()->Contains(L"acer"))
		{
			return true;
		}
    }
	return false;
}


void UtilFuncs::EnablePSN()
{
	Logger::Instance->WriteLine(L"EnablePSN: Start PSN node");
	try
	{
		CcdManager::Instance->RegisterStorageNode();	

		PlayerOptionForm^ optionForm = gcnew PlayerOptionForm();
		optionForm->SetPlayers(0,1,0);
		//optionForm->ShowDialog();

		if(mFileSystemMonitor == nullptr)
			mFileSystemMonitor = gcnew CFileSystemMonitor();

		mFileSystemMonitor->Start();

		if(optionForm->IsSettingChanged)		
		{
			UtilFuncs::NotifyCloudAgent(WM_PLAYER_SETTING_CHANGED);

			//IntPtr hwnd = FindWindow(L"ACLOUDAGENT", L"cloudMediaAgent_ ");	
			//PostMessage(hwnd, WM_PLAYER_SETTING_CHANGED, 0 , 0);
			//Logger::Instance->WriteLine(L"EnablePSN: post message to cloud medai agent ");
		}
	}
	catch(Exception^ e)
	{
		Logger::Instance->WriteLine(L"EnablePSN: exception, " + e->Message);
	}
}

void UtilFuncs::DisablePSN()
{
	Logger::Instance->WriteLine(L"DisablePSN: UnregisterStorageNode");
	try
	{
		CcdManager::Instance->UnregisterStorageNode();	

		if(mFileSystemMonitor == nullptr)
			mFileSystemMonitor = gcnew CFileSystemMonitor();

		mFileSystemMonitor->Stop();
	}
	catch(Exception^ e)
	{
		Logger::Instance->WriteLine(L"EnablePSN: exception, " + e->Message);
	}
}

void UtilFuncs::NotifyCloudAgent(int msg)
{
	IntPtr hwnd = FindWindow(L"ACLOUDAGENT", L"cloudMediaAgent_ ");	
	if(hwnd.ToInt32() == 0)
	{
		try
		{
			String^ filePath = System::IO::Path::GetDirectoryName(System::Reflection::Assembly::GetExecutingAssembly()->Location);// 

			System::Diagnostics::ProcessStartInfo^ startInfo = gcnew System::Diagnostics::ProcessStartInfo();
			startInfo->WorkingDirectory = filePath;
			startInfo->FileName = filePath + "\\cloudMediaAgent.exe";
			startInfo->WindowStyle = ProcessWindowStyle::Minimized;
			
			Logger::Instance->WriteLine("NotifyCloudAgent: path" + startInfo->FileName);
			Process::Start( startInfo );
		}
		catch(Exception ^ e)
		{
			Logger::Instance->WriteLine("NotifyCloudAgent: launch cloudMediaAgent.exe" + e->Message);
		}
	}
	else
	{
		PostMessage(hwnd, msg, 0 , 0);
		Logger::Instance->WriteLine(L"EnablePSN: post message to cloud medai agent ");
	}
}

String^ UtilFuncs::ACPanelSettingFolder()
{	
	String^ folderPath = Environment::GetFolderPath(Environment::SpecialFolder::LocalApplicationData)+L"\\iGware\\ACPanel\\";
	if(!System::IO::Directory::Exists(folderPath))
		System::IO::Directory::CreateDirectory(folderPath);
	return folderPath;
}

String^ UtilFuncs::GetSettingFileFullPath()
{
	return ACPanelSettingFolder() + FILE_NAME_Setting;
}

String^ UtilFuncs::GetChangeFileFullPath()
{
	return ACPanelSettingFolder() + FILE_NAME_Changed;	
}

void UtilFuncs::NotifyLoginAndPSNChange()
{
	System::IntPtr handle = FindWindow(nullptr, CAPTION_WIN_MESSAGE_FORM);
	if(handle.ToInt32() != 0)
	{
		int nRet = 0;
		if(CcdManager::Instance->IsLoggedIn)
			nRet = 1;

		PostMessage(handle, WM_LOGIN, nRet, 0);

		if(CcdManager::Instance->IsLogInPSN)
			nRet = 1;
		else 
			nRet = 0;

		PostMessage(handle, WM_PSN, nRet, 0);
	}
}
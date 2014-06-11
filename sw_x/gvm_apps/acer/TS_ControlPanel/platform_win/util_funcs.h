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
#include "FileSystemMonitor.h"
using namespace System;
//using namespace System::Runtime::InteropServices; 

#define CAPTION_WIN_MESSAGE_FORM L"Acer TS MsgProc "
#define FILE_NAME_Changed					L"LocalMediaChange.ini"	//
#define FILE_NAME_Setting					L"LocalMediaSetting.ini"	

#define INI_SEC_MediaChange				L"MediaDBChange"
#define INI_SEC_PlayerSetting				L"Player"

#define INI_KEY_Win								L"WinLib"
#define INI_KEY_iTune							L"iTune"
#define INI_KEY_WMP								L"WMP"


[System::Runtime::InteropServices::DllImport("KERNEL32.DLL", CharSet=System::Runtime::InteropServices::CharSet::Auto, EntryPoint="WritePrivateProfileString")]
bool WritePrivateProfileString(String^ lpAppName, String^ lpKeyName, String^ lpString, String^ lpFileName);

[System::Runtime::InteropServices::DllImport("KERNEL32.DLL", CharSet=System::Runtime::InteropServices::CharSet::Auto, EntryPoint="GetPrivateProfileString")]
UInt32 GetPrivateProfileString(String^ lpAppName, String^ lpKeyName, String^ lpDefault, System::Text::StringBuilder^ lpReturnedString, UInt32 nSize, String^ lpFileName);

[System::Runtime::InteropServices::DllImport("user32.dll",   CharSet=System::Runtime::InteropServices::CharSet::Auto, EntryPoint="PostMessage")] 
bool PostMessage(IntPtr hWnd, UInt32 Msg, int wParam, int lParam);

[System::Runtime::InteropServices::DllImport("user32.dll", CharSet=System::Runtime::InteropServices::CharSet::Auto, EntryPoint="FindWindow")] 
IntPtr FindWindow(String^ lpClassName, String^ lpWindowName);



public ref class UtilFuncs
{
public:
    static String^ GetVersion();
    static String^ GetUserHomeDir();
    static void    ShowPath(String^ path);
    static void    ShowUrl(String^ url);
    static String^ StripChars(String^ input, String^ chars);
    static String^ StripDataSet(String^ input);
	static bool		CheckAcerWMI();
	static void		EnablePSN();
	static void		DisablePSN();

	static String^ ACPanelSettingFolder();
	static String^ GetSettingFileFullPath();
	static String^ GetChangeFileFullPath();
	static void	  NotifyCloudAgent(int msg);
	static void	  NotifyLoginAndPSNChange();

protected:
	static CFileSystemMonitor^ mFileSystemMonitor ;

    // HACK
    //static void StartStorageNode(String^ user, String^ pass, int waitMS, bool startNew);
};

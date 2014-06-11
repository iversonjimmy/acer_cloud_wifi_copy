//
//  Copyright 2011 Acer Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
//	Created by Cigar  2011 Sep. 27



//Reference Data
// http://blogs.microsoft.co.il/blogs/arik/archive/2010/03/15/windows-7-libraries-c-quick-reference.aspx
//	http://www.codeproject.com/KB/files/FileSpyArticle.aspx
//

#pragma once

using namespace System;
using namespace System::IO;

enum class LibChange
{
    Windows,
	WMP,
	iTune,
	File,
};

//
//
//[DllImport("KERNEL32.DLL", CharSet=CharSet::Auto, EntryPoint="WritePrivateProfileString")]
//bool WritePrivateProfileString(String^ lpAppName, String^ lpKeyName, String^ lpString, String^ lpFileName);
//
//[DllImport("KERNEL32.DLL", CharSet=CharSet::Auto, EntryPoint="GetPrivateProfileString")]
//UInt32 GetPrivateProfileString(String^ lpAppName, String^ lpKeyName, String^ lpDefault, System::Text::StringBuilder^ lpReturnedString, UInt32 nSize, String^ lpFileName);
//
//[DllImport("user32.dll",   CharSet=CharSet::Auto, EntryPoint="PostMessage")] 
//bool PostMessage(IntPtr hWnd, UInt32 Msg, int wParam, int lParam);
//
//[DllImport("user32.dll", CharSet=CharSet::Auto, EntryPoint="FindWindow")] 
//IntPtr FindWindow(String^ lpClassName, String^ lpWindowName);



public ref class CFileSystemMonitor
{
public:

	CFileSystemMonitor();
	~CFileSystemMonitor();
	//delegate void FolderChangeHandler(System::IO::WatcherChangeTypes whatChange, String^ szOldFolderName, String^ szNewFolderName);	
 //   FolderChangeHandler^ folderChangeEvent;

	void Start();
	void Stop();
	void MonitorFolder(String^ folderName);
	void MonitorWMPLibrary();
	void MonitoriTuneLibrary();
	void MonitorWindowsLibrary();

	void DeMonitorWMPLibrary();
	void DeMonitoriTuneLibrary();
	void DeMonitorWindowsLibrary();
	property bool IsMediaChange{bool get(); void set(bool value);};
	


	//static String^ ACPanelSettingFolder();
	static String^ ChangeFileName = L"LocalMediaChange.ini";
	static String^ SettingFileName = L"LocalMediaSetting.ini";

protected:
	void  WinLibWatcher_Changed(System::Object^ sender, System::IO::FileSystemEventArgs^e);
	void  WMPLibWatcher_Changed(System::Object^ sender, System::IO::FileSystemEventArgs^e);
	void  iTuneLibWatcher_Changed(System::Object^ sender, System::IO::FileSystemEventArgs^e);

	void LibChangeProc(LibChange);
	void SaveMediaFileProperty(String^ fullFilePath);
	bool IsMediaDataBaseFileChange(String^ fullFilePath);
	void CheckAllMediaDataBaseFile(bool bOnlyUpdate);
	void NotifyChange();

	static  String^ WMP_DB_NAME = L"CurrentDatabase_372.wmdb";
	static  String^ ITUNE_DB_NAME = L"iTunes Library.itl";

private:
	FileSystemWatcher^ miTuneLibWatcher;
	FileSystemWatcher^ mWMPLibWatcher;
	FileSystemWatcher^ mWinLibWatcher;
	System::Collections::Generic::List<FileSystemWatcher^>^ mFolderWatchers;

	bool mIsMediaChange;
	System::Timers::Timer^ mTimer;

	void OnChanged(System::Object^ source, System::IO::FileSystemEventArgs^ e);
	void OnRenamed(System::Object^ source, System::IO::RenamedEventArgs^ e);
	static void OnTimedEvent( System::Object^ /*source*/, System::Timers::ElapsedEventArgs^ /*e*/ );
};

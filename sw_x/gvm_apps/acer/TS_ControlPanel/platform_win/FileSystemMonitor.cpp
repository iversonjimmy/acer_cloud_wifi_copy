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

#include "stdafx.h"


#include "FileSystemMonitor.h"
#include "logger.h"
#include "WM_USER.h"
#include "util_funcs.h"
#include "ShellLibUtilites.h"
//#include <windows.h>

//#pragma comment(lib, "User32.lib")

#using <System.dll>



using namespace System::IO;


CFileSystemMonitor::CFileSystemMonitor()
{
//	folderChangeEvent = nullptr;

	miTuneLibWatcher = nullptr;
	mWMPLibWatcher = nullptr;
	mWinLibWatcher = nullptr;

	Logger::Instance->Init();

	CheckAllMediaDataBaseFile(false);
	mFolderWatchers = gcnew System::Collections::Generic::List<FileSystemWatcher^>();

	mIsMediaChange = false;
}


CFileSystemMonitor::~CFileSystemMonitor()
{
	DeMonitoriTuneLibrary();
	DeMonitorWindowsLibrary();
	DeMonitorWMPLibrary();
}


void CFileSystemMonitor::Start()
{
	 MonitoriTuneLibrary();
	 MonitorWindowsLibrary();
	 //MonitorWMPLibrary();
}

void CFileSystemMonitor::Stop()
{
	 DeMonitoriTuneLibrary();
	 DeMonitorWindowsLibrary();
	 DeMonitorWMPLibrary();
}


void CFileSystemMonitor::MonitorFolder(String^ folderName)
{
	if (!System::IO::Directory::Exists(folderName))    //If the folder does not exist, return directly.  //by Cigar 16 Jun. 2011
		return;
	try
    {
		System::IO::FileSystemWatcher^ watcher = gcnew FileSystemWatcher();
        watcher->Path = folderName;
        
         //Watch for changes in LastAccess and LastWrite times, and 
         //  the renaming of files or directories. 
		watcher->NotifyFilter = NotifyFilters::LastAccess | NotifyFilters::LastWrite
			| NotifyFilters::FileName | NotifyFilters::DirectoryName;

        // Add event handlers.
		watcher->Created += gcnew FileSystemEventHandler(this, &CFileSystemMonitor::OnChanged);
        watcher->Deleted += gcnew FileSystemEventHandler(this, &CFileSystemMonitor::OnChanged);
        watcher->Renamed += gcnew RenamedEventHandler(this, &CFileSystemMonitor::OnRenamed);

        // Begin watching.
		watcher->EnableRaisingEvents = true;

		mFolderWatchers->Add(watcher);
    }
    catch (Exception^ e)
    {
		Logger::Instance->WriteLine("MonitorFolder: "+ e->Message);
    }
}




void CFileSystemMonitor::MonitorWMPLibrary()
{

	//The folder of WMP database 
	String^ folderPath=  Environment::GetFolderPath(Environment::SpecialFolder::LocalApplicationData) + L"\\Microsoft\\Media Player\\";

	if (!System::IO::Directory::Exists(folderPath ))    //If the folder does not exist, return directly.  //by Cigar 16 Jun. 2011
	{
		Logger::Instance->WriteLine("MonitorWMPLibrary: " + folderPath + " does not exist");
		return;
	}

        //Initialize the watcher.
	if(mWMPLibWatcher == nullptr)
	{
		mWMPLibWatcher = gcnew FileSystemWatcher(folderPath);
	}
	try
	{
		mWMPLibWatcher->NotifyFilter = NotifyFilters::LastWrite;			//NotifyFilters.Size; 
																											//The problem to use the Size monitor: 
																											//if the data written into file are smaller than size unit, the size of file does not change, 
																											//and the event will not be issued.

		mWMPLibWatcher->Filter = WMP_DB_NAME; //Only monitor the database file.
		
		mWMPLibWatcher->IncludeSubdirectories = false;
	

	mWMPLibWatcher->Changed += gcnew FileSystemEventHandler(this, &CFileSystemMonitor::WMPLibWatcher_Changed);

    mWMPLibWatcher->EnableRaisingEvents = true;
	}
	catch (Exception^ e)
	{
		Logger::Instance->WriteLine(L"MonitorWMPLibrary: "+ e->Message);
	}

}

void CFileSystemMonitor::DeMonitorWMPLibrary()
{
	if(mWMPLibWatcher != nullptr)
	{
		mWMPLibWatcher->Changed -= gcnew FileSystemEventHandler(this, &CFileSystemMonitor::WMPLibWatcher_Changed);		
	}
}


void CFileSystemMonitor::MonitorWindowsLibrary()
{
	//Get all attached folders in windows library, and hook monitor event.
	System::Collections::Generic::List<System::String^>^  libNames = ShellLibUtilites::GetAllLibraryNames();
	for(int i=0; i< libNames->Count ; i++)
	{
		
		System::String^ libName = libNames[i];
		if(!libName->ToUpper()->Contains(L"MUSIC") && !libName->ToUpper()->Contains(L"PICTURE") && !libName->ToUpper()->Contains(L"VIDEO"))
			continue;

		Logger::Instance->WriteLine(L"Library Name: " + libNames[i]+"==================================");
		System::Collections::Generic::List<System::String^>^  folderNames = ShellLibUtilites::GetAllAttachedFolders(libName);
		for(int j=0 ; j<folderNames->Count ; j++)
		{
			MonitorFolder(folderNames[j]);
		}
	}

	//Monitor the folder containing the property file of windows library.
	String^ folderPath = Environment::GetFolderPath(Environment::SpecialFolder::ApplicationData) + L"\\Microsoft\\Windows\\Libraries\\";

	if (!System::IO::Directory::Exists(folderPath ))    //If the folder does not exist, return directly.  //by Cigar 16 Jun. 2011
	{
		Logger::Instance->WriteLine("MonitorWindowsLibrary: " + folderPath + " does not exist");
		return;
	}

     //Initialize the watcher.
	if(mWinLibWatcher == nullptr)
	{
		mWinLibWatcher = gcnew FileSystemWatcher(folderPath);
	}
	try
	{
		mWinLibWatcher->NotifyFilter = NotifyFilters::LastWrite;  //NotifyFilters.Size; 
																								//The problem to use the Size monitor: 
																								//if the data written into file are smaller than size unit, the size of file does not change, 
																								//and the event will not be issued.
		mWinLibWatcher->Filter = "*.library-ms*"; //Only monitor the database file.
		//mWinLibWatcher->Filter = L"Music.library-ms"; //Only monitor the database file.
		mWinLibWatcher->IncludeSubdirectories = false;
		mWinLibWatcher->Changed += gcnew FileSystemEventHandler(this, &CFileSystemMonitor::WinLibWatcher_Changed);
		mWinLibWatcher->EnableRaisingEvents = true;
	}
	catch(Exception^ e)
	{
		Logger::Instance->WriteLine(L"MonitorWindowsLibrary: " + e->Message);
	}



	

}

void CFileSystemMonitor::DeMonitorWindowsLibrary()
{
	if(mWinLibWatcher != nullptr)
		mWinLibWatcher->Changed -= gcnew FileSystemEventHandler(this, &CFileSystemMonitor::WinLibWatcher_Changed);

	if(mFolderWatchers != nullptr)
	{
		for(int i=0 ; i <mFolderWatchers->Count ; i++)
		{
			mFolderWatchers[i]->Created -= gcnew FileSystemEventHandler(this, &CFileSystemMonitor::OnChanged);
			mFolderWatchers[i]->Deleted -= gcnew FileSystemEventHandler(this, &CFileSystemMonitor::OnChanged);
			mFolderWatchers[i]->Renamed -= gcnew RenamedEventHandler(this, &CFileSystemMonitor::OnRenamed);
		}
		mFolderWatchers->Clear();
	}
}

void CFileSystemMonitor::MonitoriTuneLibrary()
{
	//The folder of iTune database 
	String^ folderPath = Environment::GetFolderPath(Environment::SpecialFolder::MyMusic) + L"\\iTunes\\";

	if (!System::IO::Directory::Exists(folderPath ))    //If the folder does not exist, return directly.  //by Cigar 16 Jun. 2011
	{
		Logger::Instance->WriteLine("MonitoriTuneLibrary: " + folderPath + " does not exist");
		return;
	}

     //Initialize the watcher.
	if(miTuneLibWatcher == nullptr)
	{
		miTuneLibWatcher= gcnew FileSystemWatcher(folderPath);
	}

	try
	{
		miTuneLibWatcher->NotifyFilter = NotifyFilters::LastWrite;  //NotifyFilters.Size; 
																								//The problem to use the Size monitor: 
																								//if the data written into file are smaller than size unit, the size of file does not change, 
																								//and the event will not be issued.
		miTuneLibWatcher->Filter = ITUNE_DB_NAME; //Only monitor the database file.
		miTuneLibWatcher->IncludeSubdirectories = false;
		
	miTuneLibWatcher->Changed += gcnew FileSystemEventHandler(this, &CFileSystemMonitor::iTuneLibWatcher_Changed);

    miTuneLibWatcher->EnableRaisingEvents = true;
	}
	catch(Exception^ e)
	{
		Logger::Instance->WriteLine(L"MonitoriTuneLibrary: " + e->Message);
	}

}

void CFileSystemMonitor::DeMonitoriTuneLibrary()
{
	if(miTuneLibWatcher != nullptr)
	miTuneLibWatcher->Changed -= gcnew FileSystemEventHandler(this, &CFileSystemMonitor::iTuneLibWatcher_Changed);
}

void  CFileSystemMonitor::WinLibWatcher_Changed(System::Object^ sender, System::IO::FileSystemEventArgs^ e)
{
	Logger::Instance->WriteLine("WinLibWatcher_Changed: "+ e->FullPath +", type: "+ e->ChangeType.ToString());
	

	LibChangeProc(LibChange::Windows);
}

void  CFileSystemMonitor::WMPLibWatcher_Changed(System::Object^ sender, System::IO::FileSystemEventArgs^ e)
{
	Logger::Instance->WriteLine("WMPLibWatcher_Changed: " + e->FullPath +", type: "+ e->ChangeType.ToString());
	//String^ filePath = UtilFuncs::GetChangeFileFullPath();
	//System::Text::StringBuilder^ buf = gcnew System::Text::StringBuilder(100);
	//GetPrivateProfileString( INI_SEC_MediaChange, L"WMPFileCount",  L"NONE", buf, 100, filePath);

	//WritePrivateProfileString(INI_SEC_MediaChange, L"WMPFileCount", gcnew String(L"1"), filePath);

	LibChangeProc(LibChange::WMP);
}

void  CFileSystemMonitor::iTuneLibWatcher_Changed(System::Object^ sender, System::IO::FileSystemEventArgs^ e)
{
	Logger::Instance->WriteLine("iTuneLibWatcher_Changed: "+ e->FullPath +", type: "+ e->ChangeType.ToString());

	LibChangeProc(LibChange::iTune);
}

///Save the changing notification in ChangeFileName and it will be read by process of uploading metadata.
void CFileSystemMonitor::LibChangeProc(LibChange whichLibChange)
{
	//Logger::Instance->WriteLine("LibChangeProc" + whichLibChange.ToString() + " changed.");

	String^ filePath = UtilFuncs::GetChangeFileFullPath();// ACPanelSettingFolder();

	mIsMediaChange = true;

	WritePrivateProfileString(INI_SEC_MediaChange, whichLibChange.ToString(), gcnew String(L"1"), filePath);

	NotifyChange();
}


void CFileSystemMonitor::NotifyChange()
{
	if(mTimer != nullptr)
	{
		mTimer->Stop();
		mTimer->Close();
		mTimer= nullptr;
	}


	if(mTimer == nullptr)
	{
		mTimer = gcnew System::Timers::Timer;

      // Hook up the Elapsed event for the timer.
		mTimer->Elapsed += gcnew System::Timers::ElapsedEventHandler( CFileSystemMonitor::OnTimedEvent );
      
      // Set the Interval to 2 seconds (2000 milliseconds).
		mTimer->Interval = 3000;
		mTimer->AutoReset = false;
	}
	
	if(!mTimer->Enabled)
	{	
	mTimer->Enabled = true;
	mTimer->Start();
}
}

void CFileSystemMonitor::OnTimedEvent( System::Object^ source, System::Timers::ElapsedEventArgs^ e )
{

	UtilFuncs::NotifyCloudAgent(WM_MEDIA_CHANGED);
}

///Save the ticks of last write of file. The saved data is compared with next launch, and it is read in IsMediaDataBaseFileChange
void CFileSystemMonitor::SaveMediaFileProperty(String^ fullFilePath)
{
	if(!System::IO::File::Exists(fullFilePath))
		return;

	FileInfo^ fileInfo = gcnew FileInfo(fullFilePath);

	String^ SettingFilePath = UtilFuncs::GetSettingFileFullPath();// ACPanelSettingFolder() + FILE_NAME_Setting;
	WritePrivateProfileString(INI_SEC_MediaChange, System::IO::Path::GetFileNameWithoutExtension(fullFilePath), fileInfo->LastWriteTime.Ticks.ToString(), SettingFilePath);
}

/**
Check the last written date of all media database file with the date last record.
@param bOnlyUpdate: true, do not issue change event., only update the data in record file.
									 false,  update data and issue change event.
**/
void CFileSystemMonitor::CheckAllMediaDataBaseFile(bool bOnlyUpdate)
{

	//Windows Library
	System::String^ folderPath = Environment::GetFolderPath(Environment::SpecialFolder::ApplicationData) + L"\\Microsoft\\Windows\\Libraries\\";

	array<String^>^fileEntries = System::IO::Directory::GetFiles(folderPath, L"*.library-ms");
	System::Collections::IEnumerator^ files = fileEntries->GetEnumerator();
	while ( files->MoveNext() )
	{
		String^ fileName = safe_cast<String^>(files->Current);
		if(!fileName->ToUpper()->Contains(L"MUSIC") && !fileName->ToUpper()->Contains(L"PICTURE") && !fileName->ToUpper()->Contains(L"VIDEO"))
			continue;
		if(IsMediaDataBaseFileChange(fileName))
		{	
			if(!bOnlyUpdate)
				LibChangeProc(LibChange::Windows);			
			SaveMediaFileProperty(fileName);
		}
	}

	return;


	//iTune
	folderPath = Environment::GetFolderPath(Environment::SpecialFolder::MyMusic) + L"\\iTunes\\";	

	if(IsMediaDataBaseFileChange(folderPath + ITUNE_DB_NAME))
	{	
		if(!bOnlyUpdate)
			LibChangeProc(LibChange::iTune);
		SaveMediaFileProperty(folderPath + ITUNE_DB_NAME);
	}



	//WMP
	folderPath=  Environment::GetFolderPath(Environment::SpecialFolder::LocalApplicationData) + L"\\Microsoft\\Media Player\\";
	if(IsMediaDataBaseFileChange(folderPath+WMP_DB_NAME))
	{	
		if(!bOnlyUpdate)
			LibChangeProc(LibChange::WMP);
		SaveMediaFileProperty(folderPath+WMP_DB_NAME);
	}
}


///Compare the ticks data of file with the ticks saved in ini file.
bool CFileSystemMonitor::IsMediaDataBaseFileChange(String^ fullFilePath)
{
	if(!System::IO::File::Exists(fullFilePath))
		return false;

	FileInfo^ fileInfo = gcnew FileInfo(fullFilePath);

	String^ SettingFilePath = UtilFuncs::GetSettingFileFullPath();// ACPanelSettingFolder() + FILE_NAME_Setting;
	System::Text::StringBuilder^ buf = gcnew System::Text::StringBuilder(100);
	GetPrivateProfileString( INI_SEC_MediaChange, System::IO::Path::GetFileNameWithoutExtension(fullFilePath),  L"NONE", buf, 100, SettingFilePath);
	if(!buf->ToString()->Equals(L"NONE"))
	{
		try
		{
			Int64 ticks = Convert::ToInt64(buf->ToString());
			if(fileInfo->LastWriteTime.Ticks != ticks)
				return true;
			else 
				return false;
		}
		catch(Exception ^ e)
		{
			Logger::Instance->WriteLine(L"IsMediaDataBaseFileChange: "+ e->Message);
			return true;
		}
	}
	else 
		return true;
}

bool CFileSystemMonitor::IsMediaChange::get()
{
	return mIsMediaChange;
}

void CFileSystemMonitor::IsMediaChange::set(bool value)
{
	mIsMediaChange = value;
}


void CFileSystemMonitor::OnChanged(Object^ source, FileSystemEventArgs^ e)
{
	switch (e->ChangeType)
	{
        ////File added into folders of Windows' Library is also added into WMP library, so the event "Created" is removed,        
		case WatcherChangeTypes::Created:    //marked by Cigar 5 Jul. 2011                                 
			//if (folderChangeEvent != nullptr)
			//	folderChangeEvent(e->ChangeType, e->FullPath, String::Empty);
			//break;
		case WatcherChangeTypes::Deleted:
			//if (folderChangeEvent != nullptr)
			//	folderChangeEvent(e->ChangeType, e->FullPath, String::Empty);
			LibChangeProc(LibChange::File);
		break;
	}
	
	// Specify what is done when a file is changed, created, or deleted.
	Logger::Instance->WriteLine("OnChanged: "+ e->FullPath + " is " + e->ChangeType.ToString());
}

void CFileSystemMonitor::OnRenamed(Object^ source, RenamedEventArgs^ e)
{
	//if (folderChangeEvent != nullptr)
	//	folderChangeEvent(WatcherChangeTypes::Renamed, e->OldFullPath, e->FullPath);
	LibChangeProc(LibChange::File);
	// Specify what is done when a file is renamed.
	Logger::Instance->WriteLine("OnRenamed: Old Path:"+e->OldFullPath +", New Path:"+e->FullPath);
}


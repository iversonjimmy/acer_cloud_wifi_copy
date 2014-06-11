#include "StdAfx.h"
#include "CWinLib.h"


CWinLib::CWinLib(void)
{
	m_litem = NULL;
}


CWinLib::~CWinLib(void)
{	
	if(m_litem!= NULL)
	{
		m_litem->clear();
	
		SAFE_DELETE(m_litem);	
	}
}

void CWinLib::GetAllList(TContentMap* inlitem)
{
	if(m_litem!= NULL)
	{
		m_litem->clear();
	
		SAFE_DELETE(m_litem);	
	}
	m_litem = new TContentMap;
	m_litem = inlitem;

	GetWinLibPathByType(m_litem,MEDIA_TYPE_photo);
	GetWinLibPathByType(m_litem,MEDIA_TYPE_audio);
	GetWinLibPathByType(m_litem,MEDIA_TYPE_video); 
}

BOOL CWinLib::GetWinLibPathByType(TContentMap* inlitem, MEDIA_TYPE inMediatype)
{
	CoInitialize(NULL);
	//Load Picture Library
	IShellLibrary *pslLibrary; 
	HRESULT hr;
		
	if (MEDIA_TYPE_photo == inMediatype)
		hr = SHLoadLibraryFromKnownFolder(FOLDERID_PicturesLibrary, STGM_READ, IID_PPV_ARGS(&pslLibrary)); 
	else if (MEDIA_TYPE_video == inMediatype)
		hr = SHLoadLibraryFromKnownFolder(FOLDERID_VideosLibrary, STGM_READ, IID_PPV_ARGS(&pslLibrary)); 
	else if (MEDIA_TYPE_audio == inMediatype)
		hr = SHLoadLibraryFromKnownFolder(FOLDERID_MusicLibrary, STGM_READ, IID_PPV_ARGS(&pslLibrary)); 
	
	if(SUCCEEDED(hr)) 
	{
		IShellItemArray *psiaFolders; 
		DWORD dwCount = 0;

		hr = pslLibrary->GetFolders(LFF_ALLITEMS, IID_PPV_ARGS(&psiaFolders)); 

		if(SUCCEEDED(hr)) 
		{
			hr = psiaFolders->GetCount(&dwCount);

			for(DWORD i=0;i<dwCount;i++)
			{
				TCHAR* szPath = new TCHAR[MAX_PATH];
				TCHAR szDisplay[MAX_PATH] = _T("");
				IShellItem *psiFolder;
				
				hr = psiaFolders->GetItemAt(i, &psiFolder);

				if(SUCCEEDED(hr))
				{
					//Get the Display Name
					//if (GetFolderDisplay(psiFolder, szDisplay)){}
					//Get the path
					if (GetFolderPath(pslLibrary, psiFolder, szPath))
					{
						ScanPath(szPath);
					}
					
					//szPathBuffer.push_back(szPath);

					psiFolder->Release();
				}
			}

			psiaFolders->Release();
		}
		
		pslLibrary->Release();
	}

	CoUninitialize();

	return TRUE;
}

void CWinLib::GetItemInfo(CString szPath)
{
 
		CLOUD_MEIDA_METADATA_STRUCT* cmd = new CLOUD_MEIDA_METADATA_STRUCT();
	    InitMetadataStruct(cmd);

		 bool bIsAudio = false;
		 bool bIsPhoto = false;
		 bool bIsVideo = false;

		GetFileInfo(cmd,szPath, &bIsAudio,&bIsPhoto,&bIsVideo);

		if(!bIsAudio && !bIsPhoto && !bIsVideo)
			return;

		cmd->Source = cmd->DeviceID = PLAYER_TYPE_iTunes;
		cmd->Location = szPath;
		BOOL bHadThumb = GetThumbNail(GetDC(NULL),cmd->Location,GetThumbnailURL(cmd->Title));
  
		cmd->UID = cmd->ContentID = GetGUID(cmd->Location);
		cmd->MetadataType = _Metadata_Item;
		cmd->MediaType = bIsAudio?MEDIA_TYPE_audio:(bIsPhoto?MEDIA_TYPE_photo:(bIsVideo?MEDIA_TYPE_video:MEDIA_TYPE_none));
        if(bIsAudio /*&& m_lAlbumName.Find(cmd->Album) == NULL*/)
		{
			//m_lAlbumName.AddHead(cmd->Album);
			if(!cmd->Album.IsEmpty())
			{
				bool bAdded = false;
				CString tempAlbumGUID = GetAlbumGUID(cmd->Album,PLAYER_TYPE_iTunes,m_litem,&bAdded);
				if(!bAdded)
				{			
					CLOUD_MEIDA_METADATA_STRUCT* cmdAlbum = new CLOUD_MEIDA_METADATA_STRUCT();
					InitMetadataStruct(cmdAlbum);

					cmdAlbum->UID = cmd->AlbumID = tempAlbumGUID;
					cmdAlbum->Source =  PLAYER_TYPE_iTunes;
					cmdAlbum->AlbumName = cmd->Album;
					cmdAlbum->Artist = cmd->Artist;
					cmdAlbum->MediaType = MEDIA_TYPE_audio;
					cmdAlbum->MetadataType = _Metadata_Album;
					cmdAlbum->Thumbnail = bHadThumb ? GetThumbnailURL(cmd->Title) : _T("");
					m_litem->insert(std::make_pair(tempAlbumGUID,cmdAlbum));
					//afxDump <<"album list insert location :"<<cmdAlbum->Location<<"\n";
				}
				else
					cmd->AlbumID = tempAlbumGUID;				
			}
		}
		m_litem->insert(std::make_pair(cmd->UID,cmd));
		//afxDump <<" list insert location :"<<cmd->Location<<"\n";
}

BOOL CWinLib::ScanPath(/*MediaType mediatype,*/ TCHAR szPath[])
{
	TCHAR szTarget[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA fd;

	wsprintf(szTarget, _T("%s\\*"), szPath);
	hFind = FindFirstFile(szTarget, &fd);

	if (INVALID_HANDLE_VALUE == hFind)
		return FALSE;

	do
	{
		if (!wcscmp(fd.cFileName, _T("..")) || !wcscmp(fd.cFileName, _T(".")) || fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;

		//Get the path
		TCHAR szSubPath[MAX_PATH], szTmp[MAX_PATH];
		int nDuration = 0;

		wsprintf(szSubPath, _T("%s\\%s"), szPath, fd.cFileName);

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			ScanPath(/*mediatype,*/ szSubPath);
			continue;
		}
		GetItemInfo(szSubPath);
		//if ( !(mediatype == MEDIA_MUSIC && IsMusic(fd.cFileName)) &&
		//	!(mediatype == MEDIA_VIDEO && IsVideo(fd.cFileName)) &&
		//	!(mediatype == MEDIA_PHOTO && IsPhoto(fd.cFileName)))
		//	continue;
		//
		////Create db item
		//DbItem item;

		//memset(&item, 0, sizeof(item));
		////Title
		//wcscpy(item.szTitle, fd.cFileName);
		////Resource Uri
		//wcscpy(item.szResourceUri, szSubPath);
		////Media Type
		//item.nMediaType = mediatype;
		////Size
		//item.nSize = (fd.nFileSizeHigh * (MAXDWORD+1)) + fd.nFileSizeLow < 1024 ? 1 : ((fd.nFileSizeHigh * (MAXDWORD+1)) + fd.nFileSizeLow)/1024;
		////AddedDate
		//FileTimeToInt(fd.ftLastWriteTime, item.nAddedDate);
		////CreateDate
		//FileTimeToInt(fd.ftCreationTime, item.nCreateDate);
		//
		////check media player library to update item if item exist.
		//if (!FindTargetByWMPLib(mediatype, szSubPath, item))
		//{
		//	char szUuid[MAX_PATH];
		//	GenerateGuid(szUuid);
		//	BytesToUTF8(szUuid, item.szContentId);
		//	item.bShare = FALSE;
		//}
		//		
		//if ( mediatype == MEDIA_MUSIC || mediatype == MEDIA_VIDEO )
		//{
		//	//Get Thumbnail
		//	TCHAR szThumbnailUrl[MAX_PATH];
		//	HBITMAP hbitmap = NULL;

		//	GetThumbnailFromImage(szSubPath, hbitmap);

		//	if (hbitmap)
		//	{
		//		HDC hDC = ::GetDC(m_hwnd);

		//		wsprintf(item.szThumbnailUri, _T("%s\\%s.bmp"), m_szAppPath, item.szContentId);
		//		SaveImage(hDC, hbitmap, item.szThumbnailUri);
		//		DeleteObject(hbitmap);
		//		DeleteDC(hDC);
		//	}	
		//}
		//else if (mediatype == MEDIA_PHOTO)
		//{
		//	//Get Thumbnail
		//	wcscpy(item.szThumbnailUri, item.szResourceUri);
		//}

		////Album
		//if ( mediatype == MEDIA_PHOTO || mediatype == MEDIA_VIDEO )
		//	GetFolderName(szPath, item.szAlbum);
		////Save to DB
		//InsertDB(item);
	}
	while(FindNextFile(hFind, &fd));

	FindClose(hFind);

	return TRUE;
}
 
BOOL CWinLib::GetFolderPath(IShellLibrary *pslLibrary, IShellItem *psiFolder, TCHAR szPath[])
{
	IShellItem2 *psiFolder2;
	HRESULT hr = psiFolder->QueryInterface(IID_PPV_ARGS(&psiFolder2));
	BOOL bRet = FALSE;

	if(SUCCEEDED(hr))
	{
		//fix the changed path
		IShellItem2 *shellItemResolvedFolder = NULL;

		hr = pslLibrary->ResolveFolder(psiFolder2, 5000, IID_PPV_ARGS(&shellItemResolvedFolder));

		if(SUCCEEDED(hr))
		{
			//Get the final path
			PWSTR pszFolderPath;

			hr = shellItemResolvedFolder->GetString(PKEY_ParsingPath, &pszFolderPath);

			if(SUCCEEDED(hr))
			{
				bRet = TRUE;
				wcscpy(szPath, pszFolderPath);
				CoTaskMemFree(pszFolderPath);
			}

			shellItemResolvedFolder->Release();
		}

		psiFolder2->Release();
	}

	return bRet;
}

BOOL CWinLib::GetFolderDisplay(IShellItem *psiFolder, TCHAR szDisplay[])
{
	PWSTR pszName;
	HRESULT hr = psiFolder->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName);

	if(SUCCEEDED(hr))
	{
		wcscpy(szDisplay, pszName);
		CoTaskMemFree(pszName);
		return TRUE;
	}

	return FALSE;
}
 
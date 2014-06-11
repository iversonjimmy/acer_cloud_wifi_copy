#include "stdafx.h"
#include "CommonTool.h"
#include <shlobj.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib") 
//



std::string TcharToStdString(LPWSTR inChar )
{
	CT2CA pszConvertedAnsiString(inChar);
	std::string szResult(pszConvertedAnsiString);
	return szResult;

}

std::string CStringToStdString1(CString theCStr)
{
	//CT2CA pszConvertedAnsiString(inCString);
	//std::string szResult(pszConvertedAnsiString);
	//return szResult;
  
	// Convert the CString to a regular char array
	const int theCStrLen = theCStr.GetLength();
	char *buffer = (char*)malloc(sizeof(char)*(theCStrLen+1));
	memset((void*)buffer, 0, sizeof(buffer));
	WideCharToMultiByte(CP_UTF8, 0, static_cast<CString>(theCStr).GetBuffer(), theCStrLen, buffer, sizeof(char)*(theCStrLen+1), NULL, NULL);
	// Construct a std::string with the char array, free the memory used by the char array, and
	// return the std::string object.
	std::string STDStr(buffer);
	free((void*)buffer);
	return STDStr;


	//std::string STDStr( CW2A( inCString.GetString(), CP_UTF8 ) );
	//CT2CA pszName(inCString); 
	//return (pszName);
}

//----------------------------------------------------------------------------
// NAME: ConvertUTF16ToUTF8
// DESC: Converts Unicode UTF-16 (Windows default) text to Unicode UTF-8
//----------------------------------------------------------------------------
std::string CStringToStdString( IN const wchar_t * utf16 )
{
	//
	// Check input pointer
	//
	ATLASSERT( utf16 != NULL );
	if ( utf16 == NULL )
		AtlThrow( E_POINTER );


	//
	// Handle special case of empty string
	//
	if ( *utf16 == L'\0' )
	{
		return "";
	}


	//
	// Consider wchar_t's count corresponding to total string length,
	// including end-of-string (L'\0') character.
	//
	const int utf16Length = wcslen( utf16 ) + 1;


	//
	// Get size of destination UTF-8 buffer, in chars (= bytes)
	//
	int utf8Size = ::WideCharToMultiByte(
										CP_UTF8,	 // convert to UTF-8
										0,	 // default flags
										utf16,	 // source UTF-16 string
										utf16Length,	// total source string length, in wchar_t's,
										// including end-of-string \0
										NULL,	 // unused - no conversion required in this step
										0,	 // request buffer size
										NULL, NULL	 // unused
	);
	ATLASSERT( utf8Size != 0 );
	if ( utf8Size == 0 )
	{
		AtlThrowLastWin32();
	}


	//
	// Allocate destination buffer for UTF-8 string
	//
	std::vector< char > utf8Buffer( utf8Size );


	//
	// Do the conversion from UTF-16 to UTF-8
	//
	int result = ::WideCharToMultiByte(
										CP_UTF8,	 // convert to UTF-8
										0,	 // default flags
										utf16,	 // source UTF-16 string
										utf16Length,	// total source string length, in wchar_t's,
										// including end-of-string \0
										&utf8Buffer[0],	// destination buffer
										utf8Size,	 // destination buffer size, in bytes
										NULL, NULL	 // unused
										);	
	ATLASSERT( result != 0 );
	if ( result == 0 )
	{
		AtlThrowLastWin32();
	}


	//
	// Build UTF-8 string from conversion buffer
	//
	return std::string( &utf8Buffer[0] );
}
 
CString StdStringToCString(std::string szStd)
{
	 CString szResult;
	const char* szIdA = szStd.c_str();
	char* szTmpA = new char[strlen(szIdA) + 1];
	
	strcpy(szTmpA, szIdA);
	
	int nSize = MultiByteToWideChar(CP_UTF8, 0, szTmpA, -1, NULL, 0);

	if (nSize <= 0)
		return szResult;


	TCHAR* szTmpW = new TCHAR[nSize  +1];

	MultiByteToWideChar(CP_UTF8, 0, szTmpA, -1, szTmpW, nSize);

	szResult = szTmpW;

	delete szTmpA;
	delete szTmpW;

	return szResult;
}

int SplitString(const CString& input, const CString& delimiter, vector<CString>& results,  bool includeEmpties)
{
  int iPos = 0;
  int newPos = -1;
  int sizeS2 = delimiter.GetLength();
  int isize = input.GetLength();

  CArray<INT, int> positions;

  newPos = input.Find (delimiter, 0);

  if( newPos < 0 ) { return 0; }

  int numFound = 0;

  while( newPos > iPos )
  {
    numFound++;
    positions.Add(newPos);
    iPos = newPos;
    newPos = input.Find (delimiter, iPos+sizeS2+1);
  }

  for( int i=0; i <= positions.GetSize(); i++ )
  {
    CString s;
    if( i == 0 )
      s = input.Mid( i, positions[i] );
    else
    {
      int offset = positions[i-1] + sizeS2;
      if( offset < isize )
      {
        if( i == positions.GetSize() )
          s = input.Mid(offset);
        else if( i > 0 )
          s = input.Mid( positions[i-1] + sizeS2, 
                 positions[i] - positions[i-1] - sizeS2 );
      }
    }
    if( s.GetLength() > 0 )
       results.push_back(s);
  }
  return numFound;
}

int LogFile(CString LogItem, CString LogEnrty, CString LogMessage)
{	
	SYSTEMTIME st;   
	CString LogTime;
	CString LogFileFullName;

	GetSystemTime(&st);  
	LogTime.Format(_T("%4d\\%2d\\%2d %2d:%2d:%2d::%3d"), st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute, st.wSecond, st.wMilliseconds);
	LogEnrty = _T("(") + LogTime + _T(")   ") + LogEnrty; //     (YYY/MM/DD HH:MM:SS::MS)  LogEntry = LogMessage 
	
	TCHAR	dirPath[255];
	//SHGetFolderPath(NULL, CSIDL_COMMON_DOCUMENTS, NULL, 0, dirPath);
	_stprintf(dirPath, _T("%s"), _T("C:\\OEM\\Cloud"));
	_tmkdir(dirPath);
	LogFileFullName = CString("") + dirPath + _T("\\CloudMediaAgent_Log.txt");

	return WritePrivateProfileString((LPCTSTR)LogItem, (LPCTSTR)LogEnrty, (LPCTSTR)LogMessage, (LPCTSTR)LogFileFullName);

}
 
BOOL GetFileInfo(CLOUD_MEIDA_METADATA_STRUCT* instruct,const CString filepath,bool* bIsAudio,bool* bIsPhoto,bool* bIsVideo)
{
	CString fileDir ,fileName;
	fileDir.Append(filepath);
 	
	PathRemoveFileSpec((LPWSTR)(LPCTSTR)fileDir);//Get dir Name
	fileName.Append(PathFindFileName(filepath));//Get file name
 
	IShellDispatch* pShellDisp = NULL;  
    Folder *pFolder;  
    FolderItem *pFolderItem;  
    CComBSTR    stitle,str;  
    HRESULT hr = S_OK;  
	CoInitialize(NULL);  
  
    hr =  ::CoCreateInstance( CLSID_Shell, NULL,CLSCTX_SERVER, IID_IShellDispatch, (LPVOID*)&pShellDisp );  
  
    if( hr == S_OK )  
    {  
        hr = pShellDisp->NameSpace(CComVariant(fileDir),&pFolder);    
        hr = pFolder->ParseName(CComBSTR(fileName),&pFolderItem);  
        CComVariant vItem(pFolderItem);  

        //CComVariant vEmpty;  
        //int i = 0;  
   
   //     for (int i = 0 ;i < 100;i++)  
   //     {  
   //         hr = pFolder->GetDetailsOf(vEmpty,i,&stitle);  
   //         hr = pFolder->GetDetailsOf(vItem,i,&str);  
			//COLE2T lpszTitle(stitle);  
			//COLE2T lpszInfo(str);  
			//TCHAR buf[300];  
			//afxDump <<i <<":		"<< lpszTitle.m_psz<<"		"<<lpszInfo.m_psz<<"\n";
   //          int a = 1;  
   //     }  
		CString csPerceivedType;
 
		csPerceivedType = GetDetailsOf(pFolder,vItem,COLUMN_SORTING_TYPE_PerceivedType);
		*bIsAudio = PathMatchSpec(csPerceivedType,_T("audio"));
		*bIsPhoto = PathMatchSpec(csPerceivedType,_T("image"));
		*bIsVideo =  PathMatchSpec(csPerceivedType,_T("video"));
		instruct->Duration = GetDurection(GetDetailsOf(pFolder,vItem,COLUMN_SORTING_TYPE_Duration));

		if(!*bIsAudio && !*bIsPhoto && !*bIsVideo)
			return FALSE;

		if(*bIsAudio)
		{
			instruct->Album = GetDetailsOf(pFolder,vItem,COLUMN_SORTING_TYPE_Album);
		}
		else
		{
 
				instruct->Album = fileDir;
				instruct->Date = GetTime(GetDetailsOf(pFolder,vItem,COLUMN_SORTING_TYPE_CreatedDate));	
 
 		}
		instruct->Size = GetDetailsOf(pFolder,vItem,COLUMN_SORTING_TYPE_Size);
		instruct->Title = GetDetailsOf(pFolder,vItem,COLUMN_SORTING_TYPE_Title);
		if(instruct->Title.IsEmpty())
		{
			CString csFileNameWithOutExtension;
			csFileNameWithOutExtension.Append(fileName);
			 PathRemoveExtension((LPWSTR)(LPCTSTR)csFileNameWithOutExtension);
			 instruct->Title = csFileNameWithOutExtension;
		}
		instruct->Genre = GetDetailsOf(pFolder,vItem,COLUMN_SORTING_TYPE_Genre);
        hr = pShellDisp->Release();  
        pShellDisp = NULL;  

    }  
    CoUninitialize();  
	return TRUE;
}

CString CreateGUID()
{
	GUID   guid; 
	CString   strGUID ; 
	if   (S_OK   ==   ::CoCreateGuid(&guid)) 
	{ 
		strGUID.Format( _T("%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X") 
									,   guid.Data1 
									,   guid.Data2 
									,   guid.Data3 
									,   guid.Data4[0],   guid.Data4[1] 
									,   guid.Data4[2],   guid.Data4[3],   guid.Data4[4],   guid.Data4[5] 
									,   guid.Data4[6],   guid.Data4[7] 
									); 
		return strGUID;
	} 
}

int GetDurection(CString inDate)//"00:05:02"
{
 
 			int nHour,   nMin,   nSec,nYear,   nMonth,   nDate;   
			nHour = nMin = nSec= 0;
			swscanf(inDate,   _T("%d%*c%d%*c%d"),   &nHour,   &nMin,   &nSec );   	
			//return CTime(0,0,0,nHour,nMin,nSec).GetSecond();
            return  nHour*3600+nMin*60+nSec;
 
}

int GetTime(CString inDate)//"2011/5/9 上午 09:47"
{
 			int   nYear,   nMonth,   nDate;//,   nHour,   nMin,   nSec;   
			nYear = nMonth = nDate = 0;
			swscanf(inDate,   _T("%d%*c%d%*c%d"),   &nYear,   &nMonth,   &nDate );   	
 
			return  nYear*10000+nMonth*100+nDate;
}

#pragma region GUID

CString GetAlbumGUID(CString csAlbum,int PLAYER_TYPE, TContentMap* inMap,bool* bAdded)
{
		CString csContentID = _T("");
		TContentMap::iterator p;
 
		// Show key
		for(p = inMap->begin(); p!=inMap->end(); ++p)
		{
			if(p->second->Source == PLAYER_TYPE && PathMatchSpec(p->second->Album,csAlbum))
			{
				csContentID = p->second->UID;
				*bAdded = true;
				break;
			}
		}
		if(!*bAdded)
		{
			csContentID = CreateGUID();
		}
		return csContentID;
}

CString GetGUID( long lSourceID,long lPlaylistID, long lTrackID,long lTrackDatabaseID )//iTunes
{
	CString str,csResult;
	str.Format(_T("%.8d"), (int)PLAYER_TYPE_iTunes);
	csResult.Append(str );csResult.Append(_T("."));
	str.Format(_T("%.4d"), lPlaylistID);
	csResult.Append(str );csResult.Append(_T("."));
	str.Format(_T("%.4d"), lSourceID );
	csResult.Append( str);csResult.Append(_T("."));
	str.Format(_T("%.4d"), lTrackDatabaseID);
	csResult.Append( str);csResult.Append(_T("."));
	str.Format(_T("%.12d"), lTrackID);
	csResult.Append(str);
 
	 return csResult;
			 //string sContentID = string.Format("{0:X8}", (int)PLAYER_TYPE.iTunes) + "." +
             //                   string.Format("{0:X4}", playlistID) + "." +
             //                   string.Format("{0:X4}", sourceID ) + "." +
             //                   string.Format("{0:X4}", TrackDatabaseID) +
             //                   string.Format("{0:X12}", trackID);
}

CString GetGUID( CComBSTR TrackingID)//WMP
{	 
	//string sContentID = file.getItemInfo("TrackingID");
 
	CString csResult;
	csResult.Append(TrackingID);
	return csResult;		
}

CString GetGUID( CString sLocation )//WinLib
{
	CString str,csResult;
    csResult = CreateGUID();
	 return csResult;

		//sContentID =  Guid.NewGuid()
}
 
#pragma endregion GUID
 
CString GetDetailsOf(Folder * inFolder,VARIANT vItem, int iColumn )
{ 
	CString  outString;
	CComBSTR str;  
	inFolder->GetDetailsOf(vItem,iColumn,&str);
	COLE2T lpszInfo(str);  
	outString = lpszInfo;
	return  outString; 
}

CString sAPPDATA()
{
	 CString  str;   	
	TCHAR szPath[MAX_PATH];
 	SHGetSpecialFolderPath(NULL,  szPath, CSIDL_APPDATA  , NULL);
	str.Format(_T("%s"),szPath);
	return str;
}

CString sPicSavePath()
{
        CString sPath;
		sPath.Append(sAPPDATA());
		sPath.Append(_T("\\Acer\\clearfi\\MediaInfo")); 
		_tmkdir(sPath);		
		sPath.Append(_T("\\MediaThumbnail\\")); 
	    _tmkdir(sPath);
      
        return sPath;
   
}

CString GetThumbnailURL(CString inTitle)
{
	CString sPath;
	sPath.Append(sPicSavePath());
	sPath.Append(inTitle);
	sPath.Append(_T(".png"));
	return sPath;
}

#pragma region THUMBNAIL

//void GetThumbnail(CString szPath, HDC hDC , LPCTSTR lpszSavepath)
//{
//	#include <Rpc.h>
//	#pragma comment(lib, "Rpcrt4.lib")
//	//Init Com library
//	CoInitialize(NULL);
//
//	HBITMAP hbitmap;
//
//	IShellFolder* psiDesktopFolder = NULL;
//	IShellFolder* psiSomeFolder = NULL;
//	TCHAR szFolder[MAX_PATH], szFile[MAX_PATH];
// 
//	CString csFolderName,fileDir;
//	fileDir.Append(szPath);
//
//	PathRemoveFileSpec((LPWSTR)(LPCTSTR)fileDir);//Get dir Name
//	csFolderName.Append(PathFindFileName((LPWSTR)(LPCTSTR)szPath));
//	_tcscpy(szFile, csFolderName );
//	_tcscpy(szFolder, fileDir );
//	//GetFolderName(szPath, szFile);
//	//GetPath(szPath, szFolder);
//
//	SHGetDesktopFolder(&psiDesktopFolder);
//
//	ULONG uParsed = 0;
//	ULONG uAttrib = 0;
//	LPITEMIDLIST pidl = NULL;
//	LPITEMIDLIST filePidl = NULL;
//
//	UUID IID_IShellFolder;
//	UUID IID_IExtractImage;
//	char* szIidSHellFolder = new char[MAX_PATH];
//	char* szIidExtracgImage = new char[MAX_PATH];
//	
//	strcpy(szIidSHellFolder, "000214E6-0000-0000-C000-000000000046");
//	strcpy(szIidExtracgImage, "BB2E617C-0920-11d1-9A0B-00C04FC2D6C1");
//		
//	UuidFromStringA((unsigned char*)szIidSHellFolder, &IID_IShellFolder);
//	UuidFromStringA((unsigned char*)szIidExtracgImage, &IID_IExtractImage);
//
//	delete szIidSHellFolder;
//	delete szIidExtracgImage;
//
//	HRESULT hr = psiDesktopFolder->ParseDisplayName(NULL, NULL, szFolder, &uParsed, &pidl, &uAttrib);
//
//	if (SUCCEEDED(hr))
//	{
//		hr = psiDesktopFolder->BindToObject(pidl, NULL, IID_IShellFolder, (void**)&psiSomeFolder);
//
//		if (SUCCEEDED(hr))
//		{
//			hr = psiSomeFolder->ParseDisplayName(NULL, NULL, szFile, &uParsed, &filePidl, &uAttrib);
//
//			if (SUCCEEDED(hr))
//			{
//				IExtractImage* extractimg;
//				LPCITEMIDLIST imgPidl = filePidl;
//
//				hr = psiSomeFolder->GetUIObjectOf(NULL, 1, &imgPidl, IID_IExtractImage, NULL, (void**)&extractimg);
//
//				if (SUCCEEDED(hr))
//				{
//					SIZE size = {500, 500};
//					DWORD dwFlags = IEIFLAG_ORIGSIZE | IEIFLAG_QUALITY; 
//					int colorDepth = 32;
//					OLECHAR pathBuffer[MAX_PATH];
//					
//					hr = extractimg->GetLocation(pathBuffer, MAX_PATH, NULL, &size, colorDepth, &dwFlags);
//
//					if (SUCCEEDED(hr))
//					{
//						hbitmap = NULL;
//
//						hr = extractimg->Extract(&hbitmap);
//
//						if (SUCCEEDED(hr))
//						{
//							SaveImage(hDC,hbitmap,lpszSavepath);
//						}
//					}
//
//					extractimg->Release();
//				}
//			}
//
//			psiSomeFolder->Release();
//		}
//	}
//
//	psiDesktopFolder->Release();
//	//Release Com library
//	CoUninitialize();
//}

BOOL GetThumbNail(HDC hDC, LPCTSTR lpszFilePath,LPCTSTR lpszSavepath)
{
	 bool bFileExit = PathFileExists(lpszSavepath);
	 if(bFileExit)
		 return TRUE;
	IShellFolder * pShellFolder = NULL;
	if( SHGetDesktopFolder( &pShellFolder) == S_OK )
	{
		LPITEMIDLIST pidl = NULL;
		HRESULT hRes = pShellFolder->ParseDisplayName( NULL, NULL, (LPTSTR)(LPCTSTR)lpszFilePath, NULL, &pidl, NULL);
		if( hRes == S_OK )
		{
			LPCITEMIDLIST pidlLast = NULL;
			IShellFolder * pParentFolder = NULL;
			HRESULT hRes = SHBindToParent( pidl, IID_IShellFolder, (void**)&pParentFolder, &pidlLast );
			if( hRes == S_OK )
			{
	 
				HBITMAP hBmpImage = ExtractThumb(pParentFolder, pidlLast );
 
				if( hBmpImage )
				{
					SaveImage(hDC,hBmpImage,lpszSavepath);
					//HBITMAP hbmpOld = hBmpImage;
					//if( hbmpOld )
					//	DeleteObject(hbmpOld);
				} 
				else
					return false;
				pParentFolder->Release();
			}
		}
		pShellFolder->Release();
		return true;
	}
return false;
}

 

HBITMAP ExtractThumb(LPSHELLFOLDER psfFolder, LPCITEMIDLIST localPidl )
{
   LPEXTRACTIMAGE pIExtract = NULL;
   HRESULT hr;
   hr = psfFolder->GetUIObjectOf(NULL, 1, &localPidl, IID_IExtractImage,
                                 NULL, (void**)&pIExtract);
   if(NULL == pIExtract) // early shell version, thumbs not supported
      return NULL;

   OLECHAR wszPathBuffer[MAX_PATH];
   DWORD dwPriority = 0; // IEI_PRIORITY_NORMAL is defined nowhere!
   DWORD dwFlags = IEIFLAG_SCREEN;
   HBITMAP hBmpImage = NULL;
   SIZE  prgSize = {500, 500};
   hr = pIExtract->GetLocation(wszPathBuffer, MAX_PATH, &dwPriority,
                               &prgSize, 32, &dwFlags);
 
   if(NOERROR == hr) hr = pIExtract->Extract(&hBmpImage);
		pIExtract->Release();

   return hBmpImage; // callers should DeleteObject this handle after use
}

void SaveImage(HDC hDC, HBITMAP bitmap, LPCTSTR lpszSavepath)
{
	BITMAP bmp;
	PBITMAPINFO pbmi;
	WORD cClrBits;
	HANDLE hf; // file handle
	BITMAPFILEHEADER hdr; // bitmap file-header
	PBITMAPINFOHEADER pbih; // bitmap info-header
	LPBYTE lpBits; // memory pointer
	DWORD dwTotal; // total count of bytes
	DWORD cb; // incremental count of bytes
	BYTE *hp; // byte pointer
	DWORD dwTmp;

	// create the bitmapinfo header information
	if (!GetObject(bitmap, sizeof(BITMAP), (LPSTR)&bmp))
		return;

	// Convert the color format to a count of bits.
	cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
	if (cClrBits == 1)
	cClrBits = 1;
	else if (cClrBits <= 4)
	cClrBits = 4;
	else if (cClrBits <= 8)
	cClrBits = 8;
	else if (cClrBits <= 16)
	cClrBits = 16;
	else if (cClrBits <= 24)
	cClrBits = 24;
	else cClrBits = 32;
 
	// Allocate memory for the BITMAPINFO structure.
	if (cClrBits != 24)
	pbmi = (PBITMAPINFO) LocalAlloc(LPTR,
	sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1<< cClrBits));
	else
	pbmi = (PBITMAPINFO) LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));

	// Initialize the fields in the BITMAPINFO structure.
	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biWidth = bmp.bmWidth;
	pbmi->bmiHeader.biHeight = bmp.bmHeight;
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;

	if (cClrBits < 24)
		pbmi->bmiHeader.biClrUsed = (1<<cClrBits);

	// If the bitmap is not compressed, set the BI_RGB flag.
	pbmi->bmiHeader.biCompression = BI_RGB;

	// Compute the number of bytes in the array of color
	// indices and store the result in biSizeImage.
	pbmi->bmiHeader.biSizeImage = (pbmi->bmiHeader.biWidth + 7) /8 * pbmi->bmiHeader.biHeight * cClrBits;
	// Set biClrImportant to 0, indicating that all of the
	// device colors are important.
	pbmi->bmiHeader.biClrImportant = 0;

	// now open file and save the data
	pbih = (PBITMAPINFOHEADER) pbmi;
	lpBits = (LPBYTE) GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);

	if (!lpBits)
		return;

	// Retrieve the color table (RGBQUAD array) and the bits
	if (!GetDIBits(hDC, HBITMAP(bitmap), 0, (WORD) pbih->biHeight, lpBits, pbmi, DIB_RGB_COLORS)) 
		return;

	// Create the .BMP file.
	hf = CreateFile(lpszSavepath, GENERIC_READ | GENERIC_WRITE, (DWORD) 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 	(HANDLE) NULL);

	if (hf == INVALID_HANDLE_VALUE)
		return;

	hdr.bfType = 0x4d42; // 0x42 = "B" 0x4d = "M"
	// Compute the size of the entire file.
	hdr.bfSize = (DWORD) (sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed* sizeof(RGBQUAD) + pbih->biSizeImage);
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;

	// Compute the offset to the array of color indices.
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) +pbih->biSize + pbih->biClrUsed* sizeof (RGBQUAD);

	// Copy the BITMAPFILEHEADER into the .BMP file.
	if (!WriteFile(hf, (LPVOID) &hdr, sizeof(BITMAPFILEHEADER),(LPDWORD) &dwTmp, NULL)) 
		return;

	// Copy the BITMAPINFOHEADER and RGBQUAD array into the file.
	if (!WriteFile(hf, (LPVOID) pbih, sizeof(BITMAPINFOHEADER)+ pbih->biClrUsed * sizeof (RGBQUAD),(LPDWORD) &dwTmp, ( NULL)))
		return;
	// Copy the array of color indices into the .BMP file.
	dwTotal = cb = pbih->biSizeImage;
	hp = lpBits;

	if (!WriteFile(hf, (LPSTR) hp, (int) cb, (LPDWORD) &dwTmp,NULL))
		return;

	CloseHandle(hf);
	// Free memory.
	GlobalFree((HGLOBAL)lpBits);
}

#pragma endregion THUMBNAIL

void InitMetadataStruct(CLOUD_MEIDA_METADATA_STRUCT* incmd)
{
	incmd->MetadataType = incmd->DeviceID = incmd->Duration = incmd->Date = incmd->Source = incmd->Count = 0;
}


#pragma region SQLite Manager

	
BOOL ToMultiBytes(TCHAR szSource[], char szResult[])
{
	int nSize = WideCharToMultiByte(CP_UTF8, 0, szSource, -1, NULL, 0, NULL, NULL);

	if (!nSize)
		return FALSE;

	nSize = WideCharToMultiByte(CP_UTF8, 0, szSource, -1, szResult, nSize, NULL, NULL);

	if (!nSize)
		return FALSE;

	return TRUE;
}

void OpenDb(CppSQLite3DB* m_db,char m_szDBPathA[])
{
	//open db
	TCHAR szQuery[1024];
	TCHAR szTableList[][MAX_PATH] = {AUDIO_TABLE_NAME, VIDEO_TABLE_NAME, PHOTO_TABLE_NAME};
	char szQueryA[1024];

	m_db->open( m_szDBPathA);
	
	//check table 
	for(int i=0;i<sizeof(szTableList)/sizeof(szTableList[0]);i++)
	{
		try
		{
			wsprintf(szQuery, _T("SELECT name FROM sqlite_master WHERE type='table' AND name=%s"), szTableList[i]);
			ToMultiBytes(szQuery, szQueryA);
			m_db->execDML(szQueryA);
		}
		catch(CppSQLite3Exception ex)
		{
			wsprintf(szQuery, _T("CREATE TABLE IF NOT EXISTS %s (%s varchar(200), %s varchar(200) NOT NULL, %s varchar(200), %s integer, %s varchar(200), %s varchar(200), %s varchar, %s integer, %s integer, %s integer, %s varchar(200), %s varchar(200), %s varchar(200), %s integer, %s integer, %s integer, %s integer, %s integer, primary key(%s));"), 
					szTableList[i],
					COL_TITLE,
					COL_ID,
					COL_DEVICE_NAME,
					COL_TYPE,
					COL_RESOURCE_URI,
					COL_THUMBNAIL_URI,
					COL_ADDED_DATE,
					COL_HIT_COUNT,
					COL_LAST_SEEK_TIME,
					COL_CREATE_DATE,
					COL_ARTIST,
					COL_GENRE,
					COL_ALBUM,
					COL_SHARED,
					COL_SIZE, 
					COL_RESOLUTION_WIDTH,
					COL_RESOLUTION_HEIGHT,
					COL_DURATION,
					COL_RESOURCE_URI);
			ToMultiBytes(szQuery, szQueryA);
			m_db->execDML(szQueryA);
		}
	}

}


void InsertDB(CppSQLite3DB* m_db,DBITEM dbitem)
{
	TCHAR szQuery[1024];
	TCHAR szTable[MAX_PATH];
	char szQueryA[1024];

	if (MEDIA_TYPE_photo == dbitem.nMediaType)
		wcscpy(szTable, PHOTO_TABLE_NAME);
	else if (MEDIA_TYPE_audio == dbitem.nMediaType)
		wcscpy(szTable, AUDIO_TABLE_NAME);
	else if (MEDIA_TYPE_video == dbitem.nMediaType)
		wcscpy(szTable, VIDEO_TABLE_NAME);

	try
	{
		wsprintf(szQuery, _T("INSERT INTO %s (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s) VALUES ('%d', '%s', '%s', '%d', '%d', '%s', '%d', '%s', '%d', '%s', '%s', '%d', '%d', '%s', '%s', '%s', '%s', '%d')"),
				szTable, 
				COL_ADDED_DATE,
				COL_ALBUM, 
				COL_ARTIST, 
				COL_CREATE_DATE, 
				COL_DURATION, 
				COL_GENRE, 
				COL_HIT_COUNT, 
				COL_ID, 
				COL_LAST_SEEK_TIME, 
				COL_DEVICE_NAME, 
				COL_SIZE, 
				COL_RESOLUTION_HEIGHT, 
				COL_RESOLUTION_WIDTH, 
				COL_RESOURCE_URI, 
				COL_SHARED, 
				COL_THUMBNAIL_URI, 
				COL_TITLE, 
				COL_TYPE, 
				dbitem.nAddedDate, 
				dbitem.szAlbum,
				dbitem.szArtist,
				dbitem.nCreateDate,
				dbitem.nDuration,
				dbitem.szGenre,
				dbitem.nHitCount, 
				dbitem.szContentId,
				dbitem.nLastSeekTime,
				dbitem.szDevice,
				dbitem.szSize,
				dbitem.nResolutionHeight,
				dbitem.nResolutionWidth,
				dbitem.szResourceUri,
				dbitem.bShare ? _T("true") : _T("false"), 
				dbitem.szThumbnailUri,
				dbitem.szTitle,
				dbitem.nMediaType);
		ToMultiBytes(szQuery, szQueryA);
		m_db->execDML(szQueryA);
	}
	catch(CppSQLite3Exception ex)
	{
	}
 }

void AddToDB(CppSQLite3DB* m_db,CLOUD_MEIDA_METADATA_STRUCT* incmd)
{
	   //Create db item
		DbItem item;

		memset(&item, 0, sizeof(item));
		//Title
		wcscpy(item.szTitle, incmd->Title);
		//Resource Uri
		wcscpy(item.szResourceUri, incmd->Location);
		//Media Type
		item.nMediaType = incmd->MediaType;
		//Size
		wcscpy(item.szSize, incmd->Size);
		//item.szSize = incmd->Size;
		//CreateDate
		item.nCreateDate = incmd->Date;
		////AddedDate
		//FileTimeToInt(fd.ftCreationTime, item.nAddedDate);
 
		//Save to DB
		InsertDB(m_db,item);
}

#pragma endregion SQLite Manager

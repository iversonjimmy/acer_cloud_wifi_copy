#pragma once
 #include <string>
#include <vector>
#include <map> 
#include "CppSQLite3.h"


using namespace std;

#define SAFE_RELEASE(x) { if( x ) x->Release(); x = NULL; } 
#define SAFE_DELETE(x) { delete x; x = NULL ;}


#pragma region Table Define
//table define
#define AUDIO_TABLE_NAME _T("audioTable")
#define VIDEO_TABLE_NAME _T("videoTable")
#define PHOTO_TABLE_NAME _T("photoTable")
//coloumn define
#define COL_TITLE _T("title")
#define COL_ID _T("id")
#define COL_DEVICE_NAME _T("DeviceName")
#define COL_TYPE _T("type")
#define COL_RESOURCE_URI _T("resourceUri")
#define COL_THUMBNAIL_URI _T("thumbnailUri")
#define COL_ADDED_DATE _T("recentlyAddedDate")
#define COL_HIT_COUNT _T("hitCount")
#define COL_LAST_SEEK_TIME _T("lastSeekTime")
#define COL_SHARED _T("Shared")
#define COL_SIZE _T("Size")
#define COL_CREATE_DATE _T("creationDate")
#define COL_ARTIST _T("artist")
#define COL_GENRE _T("genre")
#define COL_ALBUM _T("album")
#define COL_RESOLUTION_WIDTH _T("resolutionWidth")
#define COL_RESOLUTION_HEIGHT _T("resolutionHeight")
#define COL_DURATION _T("duration")

#define COL_TITLE_A "title"
#define COL_ID_A "id"
#define COL_DEVICE_NAME_A "DeviceName"
#define COL_TYPE_A "type"
#define COL_RESOURCE_URI_A "resourceUri"
#define COL_THUMBNAIL_URI_A "thumbnailUri"
#define COL_ADDED_DATE_A "recentlyAddedDate"
#define COL_HIT_COUNT_A "hitCount"
#define COL_LAST_SEEK_TIME_A "lastSeekTime"
#define COL_SHARED_A "Shared"
#define COL_SIZE_A "Size"
#define COL_CREATE_DATE_A "creationDate"
#define COL_ARTIST_A "artist"
#define COL_GENRE_A "genre"
#define COL_ALBUM_A "album"
#define COL_RESOLUTION_WIDTH_A "resolutionWidth"
#define COL_RESOLUTION_HEIGHT_A "resolutionHeight"
#define COL_DURATION_A "duration"

#pragma endregion


enum PLAYER_TYPE
{
    PLAYER_TYPE_WMP = 1,
    PLAYER_TYPE_iTunes = 2,
    PLAYER_TYPE_WinLib = 0,

};

enum MEDIA_TYPE
{
    MEDIA_TYPE_none = 0,
    MEDIA_TYPE_video = 1,
    MEDIA_TYPE_audio = 2,
    MEDIA_TYPE_photo = 3,
	MEDIA_TYPE_playlist = 4,
    MEDIA_TYPE_all = 20,

};

enum MetadataType
{
    _Metadata_Item  ,
    _Metadata_Album,
    _Metadata_Playlist,
    _Metadata_Player
};

enum eventType
{
        evNone = -1,
        evStart,
        evAddContent,
        evModifyContent,
        evRemoveContent,
        evFinish,
		evCleanAll
};

typedef struct DBITEM
{
	TCHAR szTitle[MAX_PATH]; 
	TCHAR szAlbum[MAX_PATH]; 
	TCHAR szArtist[MAX_PATH]; 
	TCHAR szGenre[MAX_PATH]; 
	int nMediaType; 
	TCHAR szResourceUri[MAX_PATH]; 
	BOOL bShare; 
	TCHAR szContentId[MAX_PATH]; 
	TCHAR szSize[MAX_PATH]; 
	long nAddedDate; 
	long nCreateDate; 
	long nLastSeekTime;
	int nResolutionWidth;
	int nResolutionHeight;
	TCHAR szThumbnailUri[MAX_PATH]; 
	int nDuration; 
	int nHitCount;
	TCHAR szDevice[MAX_PATH];
}DbItem, *PDbItem;

 typedef struct 
{
CString FileURL ; 
char Time[50]; 
HICON hIcon; 

}FILEINFO;

 enum COLUMN_SORTING_TYPE
{
        COLUMN_SORTING_TYPE_Size = 1,
		COLUMN_SORTING_TYPE_PerceivedType = 9,
        COLUMN_SORTING_TYPE_PDTime = 12, //PHoto digitized time. Date taken
        COLUMN_SORTING_TYPE_Album = 14,
        COLUMN_SORTING_TYPE_Artist = 13,
        COLUMN_SORTING_TYPE_Genre = 16,
        COLUMN_SORTING_TYPE_Title = 21,
        COLUMN_SORTING_TYPE_CreatedDate = 4,
        COLUMN_SORTING_TYPE_Duration = 27

};

 //typedef struct _FILE_INFO {
 //         TCHAR szFileTitle[MAX_PATH]; //FILE TITLE
 //         DWORD dwFileAttributes; //FILE PROPERTY
 //         FILETIME ftCreationTime; //FILE CREATETIME
 //         FILETIME ftLastAccessTime; //LastAccessTime
 //         FILETIME ftLastWriteTime;  
 //         DWORD nFileSizeHigh;  
 //         DWORD nFileSizeLow;  
 //         DWORD dwReserved0;  
 //         DWORD dwReserved1; 
 //     } FILE_INFO, * PFILE_INFO; 



typedef struct 
{	 
    int MetadataType ;
    CString UID ;
    int DeviceID ;
    CString ContentID ;
    CString AlbumID ;
    CString Title ;
    CString Thumbnail ;
    CString Album ;
    CString Genre ;
    CString Artist ;
    int Duration ;
    CString Location ;
    int Date ;
    int Source ;
    int MediaType ;
    CString AlbumName ;
    CString PlaylistID ;
    CString PlaylistName ;
    int Count ;
    CList<CString,CString&> ContentIDs;
	CString Size;
    
}CLOUD_MEIDA_METADATA_STRUCT, *PCLOUD_MEIDA_METADATA_STRUCT;

std::string TcharToStdString(LPWSTR inChar );
std::string CStringToStdString1(CString inCString);//m_list[i]->UID.GetBuffer(m_list[i]->UID.GetLength())
std::string CStringToStdString(IN  const wchar_t * utf16 );
CString StdStringToCString(std::string szStd );
int SplitString(const CString& input, const CString& delimiter,vector<CString>& results, bool includeEmpties = true);
int LogFile(CString LogItem, CString LogEnrty, CString LogMessage);

 
BOOL GetFileInfo(CLOUD_MEIDA_METADATA_STRUCT* instruct, const CString filepath,bool* IsAudio,bool* bIsPhoto,bool* bIsVideo);
CString GetDetailsOf(Folder * inFolder,VARIANT vItem, int iColumn );
int GetTime(CString inDate);
int GetDurection(CString inDate);

HBITMAP ExtractThumb(LPSHELLFOLDER psfFolder, LPCITEMIDLIST localPidl );
BOOL GetThumbNail(HDC hDC, LPCTSTR lpszFilePath,LPCTSTR lpszSavepath);
//void GetThumbnail(CString szPath, HDC hDC , LPCTSTR lpszSavepath);
void SaveImage(HDC hDC, HBITMAP bitmap, LPCTSTR lpszSavepath);

CString GetGUID( CString sLocation );
CString GetGUID( CComBSTR TrackingID);
CString GetGUID( long lSourceID,long lPlaylistID, long lTrackId,long lTrackDatabaseID );
CString CreateGUID();
CString sAPPDATA();
CString sPicSavePath();
CString GetThumbnailURL(CString inFilepath);

BOOL ToMultiBytes(TCHAR szSource[], char szResult[]);
void OpenDb(CppSQLite3DB* m_db,char m_szDBPathA[]);
void InsertDB(CppSQLite3DB* m_db,DBITEM dbitem);
void AddToDB(CppSQLite3DB* m_db,CLOUD_MEIDA_METADATA_STRUCT* incmd);


typedef std::map<CString, CLOUD_MEIDA_METADATA_STRUCT*> TContentMap;
CString GetAlbumGUID(CString csAlbum,int PLAYER_TYPE, TContentMap* inMap,bool* bAdded );
void InitMetadataStruct(CLOUD_MEIDA_METADATA_STRUCT* incmd);


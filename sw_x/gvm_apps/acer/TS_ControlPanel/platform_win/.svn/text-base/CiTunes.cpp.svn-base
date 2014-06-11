#include "StdAfx.h"
#include "CiTunes.h"

CiTunes::CiTunes(void)
{
	m_iITunes = NULL;
	m_litem = NULL;
	//GetAllList();
}

CiTunes::~CiTunes(void)
{
	ReleaseiTunes();
	//m_lAlbumName.RemoveAll();
	if(m_litem!= NULL)
	{
		m_litem->clear();
	
		SAFE_DELETE(m_litem);	
	}
}

void CiTunes::ReleaseiTunes()
{
	m_iITunes.Release();
	m_iITunes = 0;
	CoUninitialize();
}

void CiTunes::GetAllList(TContentMap* inlitem)
{
		if(m_litem!= NULL)
		{
			m_litem->clear();
	
			SAFE_DELETE(m_litem);	
		}
		m_litem = new TContentMap;
		m_litem = inlitem;

		GetMediaItems();
		GetPlaylist();
 
}

void CiTunes::Init()
{
	if(!m_iITunes)
	{
		CoInitialize(NULL);

		try
		{
			HRESULT hRes;

			BSTR bstrURL = 0;
			
			// Create itunes interface
			//hRes = ::CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *)&m_iITunes);
			hRes =::CoCreateInstance(CLSID_iTunesApp, 
                                    NULL, CLSCTX_LOCAL_SERVER,//local server 
                                    __uuidof(m_iITunes), 
                                    (void **)&m_iITunes);

			    if(FAILED(hRes))
				{
					MessageBox(NULL,L"",L"Failed to load iTunes Component, is iTunes installed????",0);
					return;
				}

			//IITLibraryPlaylist *_IITLibraryPlaylist = 0;
			//IITTrackCollection *_IITTrackCollection= 0;

			//m_iITunes->get_LibraryPlaylist(&_IITLibraryPlaylist);
			//if(_IITLibraryPlaylist)
			//{
			//	_IITLibraryPlaylist->get_Tracks(&_IITTrackCollection);
 
			//	long count;
			//	_IITTrackCollection->get_Count(&count);
			//	_IITTrackCollection[0];

			//}
			//SAFE_RELEASE(_IITLibraryPlaylist);
			//SAFE_RELEASE(_IITTrackCollection);

		}
		catch(...)
		{
			ReleaseiTunes();
 
 		}
	}
}

void CiTunes::GetTrackInfo(CComPtr<IITFileOrCDTrack> _IITFileOrCDTrack )
{
		CComBSTR bstrLocation;
		long lSourceID ,lPlaylistID,lTrackId,lDatabaseID;
		CLOUD_MEIDA_METADATA_STRUCT* cmd = new CLOUD_MEIDA_METADATA_STRUCT();
	    InitMetadataStruct(cmd);

		 bool bIsAudio = false;
		 bool bIsPhoto = false;
		 bool bIsVideo = false;

		_IITFileOrCDTrack->get_Location(&bstrLocation);
		_IITFileOrCDTrack->get_SourceID (&lSourceID);
		_IITFileOrCDTrack->get_PlaylistID(&lPlaylistID);
		_IITFileOrCDTrack->get_TrackID(&lTrackId);
		_IITFileOrCDTrack->get_TrackDatabaseID(&lDatabaseID);
 
		GetFileInfo(cmd,OLE2T(bstrLocation), &bIsAudio,&bIsPhoto,&bIsVideo);

		if(!bIsAudio && !bIsPhoto && !bIsVideo)
			return;

		cmd->Source = cmd->DeviceID = PLAYER_TYPE_iTunes;
		cmd->Location = OLE2T(bstrLocation);
		BOOL bHadThumb = GetThumbNail(GetDC(NULL),cmd->Location,GetThumbnailURL(cmd->Title));
		if(!bHadThumb)
		{
			HRESULT hr = SaveiTunesArt(_IITFileOrCDTrack,GetThumbnailURL(cmd->Title));
			bHadThumb = hr != S_FALSE;
		}
		//GetThumbnail(cmd->Location,GetDC(NULL),GetThumbnailURL(cmd->Title));
		cmd->UID = cmd->ContentID = GetGUID(lSourceID,lPlaylistID,lTrackId,lDatabaseID);
		cmd->MetadataType = _Metadata_Item;
		cmd->MediaType = bIsAudio?MEDIA_TYPE_audio:(bIsPhoto?MEDIA_TYPE_photo:MEDIA_TYPE_video);
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

				}
				else
					cmd->AlbumID = tempAlbumGUID;				
			}
		}
		m_litem->insert(std::make_pair(cmd->UID,cmd));
}

void CiTunes::GetPlaylist()
{
	Init();
	long lSourceID;
    long lPlaylistID;
    long lTrackId;
    long countSource;
 
    
    IITObject * pObject;
//    IITUserPlaylist * pUserPlaylist;
    IITSourceCollection * pCollection;      
       
    USES_CONVERSION;       

    short bIsSmart=0;

    HRESULT hr=m_iITunes->get_Sources(&pCollection); 
    hr=pCollection->get_Count (&countSource);

    IITSource * pSource = NULL;
    IITPlaylistCollection * pPlaylistCollection = NULL;
    IITPlaylist * pPlaylist = NULL;
    IITTrackCollection * pTrackCollection = NULL;
    IITTrack * pTrack = NULL;

    CComPtr<IITFileOrCDTrack> fileTrack;

    long countPlaylist;
    long countTracklist;
    CComBSTR bstrName;

    for(long i=1;i<=countSource;i++)
    {
        hr=pCollection->get_Item (i,&pSource);
        //
        pSource->get_SourceID (&lSourceID);
        pSource->get_Playlists (&pPlaylistCollection);
        pPlaylistCollection->get_Count (&countPlaylist);

        for(long j=7;j<=countPlaylist;j++)//get playlists
        {
            hr=pPlaylistCollection->get_Item (j,&pPlaylist);

            pPlaylist->get_PlaylistID (&lPlaylistID);          
		    CLOUD_MEIDA_METADATA_STRUCT* cmd = new CLOUD_MEIDA_METADATA_STRUCT();
			InitMetadataStruct(cmd);
			              
            hr=m_iITunes->GetITObjectByID (lSourceID,lPlaylistID,0,0,&pObject);
            pObject->get_Name (&bstrName);

            CComPtr<IITUserPlaylist> spUserPlaylist; 
            hr=pPlaylist->QueryInterface(&spUserPlaylist);

            if(SUCCEEDED(hr))
            {
			/*	spUserPlaylist->get_Smart(&bIsSmart);
                if(bIsSmart==0)*/
                //{
            /*        ITUserPlaylistSpecialKind  plKind;
                    spUserPlaylist->get_SpecialKind (&plKind);*/
                    //if(plKind==ITUserPlaylistSpecialKindPodcasts)
                    //{
                        //AfxMessageBox(OLE2T(bstrName));

                        spUserPlaylist->get_Tracks (&pTrackCollection);
                        pTrackCollection->get_Count (&countTracklist);

 						long _lSourceID,_lPlaylistID,_lTrackId, _lDatabaseID;

						pPlaylist->get_SourceID (&_lSourceID);
						pPlaylist->get_PlaylistID(&_lPlaylistID);
						pPlaylist->get_TrackID(&_lTrackId);
						pPlaylist->get_TrackDatabaseID(&_lDatabaseID);

						cmd->UID = cmd->PlaylistID = GetGUID(_lSourceID,_lPlaylistID,_lTrackId,_lDatabaseID);	
						cmd->PlaylistName = OLE2T(bstrName);
						cmd->Count = (int)countTracklist;
						cmd->Source = PLAYER_TYPE_iTunes;
						cmd->MediaType = MEDIA_TYPE_audio;
						cmd->MetadataType = _Metadata_Playlist;

                        for(long k=1;k<=countTracklist;k++)//get tracks form playlist
                        {

								hr=pTrackCollection->get_Item (k,&pTrack);

								CComPtr<IITFileOrCDTrack> spFileTrack;

								 hr=pTrack->QueryInterface (__uuidof(spFileTrack),(void **)&spFileTrack);

								 if(SUCCEEDED(hr))
								 {					
								 		spFileTrack->get_SourceID (&_lSourceID);
										spFileTrack->get_PlaylistID(&_lPlaylistID);
										spFileTrack->get_TrackID(&_lTrackId);
										spFileTrack->get_TrackDatabaseID(&_lDatabaseID);
										cmd->ContentIDs.AddTail(GetGUID(_lSourceID,_lPlaylistID,_lTrackId,_lDatabaseID));	
						/*			 GetTrackInfo(spFileTrack );*/
								 }

							 spFileTrack.Release();

							 pTrack->Release();
                            //pTrack->get_TrackID(&lTrackId);
                            //pTrack->get_Name (&bstrName);
                            //AfxMessageBox(OLE2T(bstrName));
                        //}
                    //}
						 }   
						if(countTracklist >0)
							m_litem->insert(std::make_pair(cmd->UID,cmd));

            }
			spUserPlaylist.Release();
        }
		if(pSource!= NULL)
			SAFE_RELEASE(pSource);
		if(pPlaylistCollection!= NULL)
			SAFE_RELEASE(pPlaylistCollection);
		if(pPlaylist!= NULL)
			SAFE_RELEASE(pPlaylist);
		if(pTrackCollection!= NULL)
			SAFE_RELEASE(pTrackCollection);
     }
		ReleaseiTunes();
}

void CiTunes::GetMediaItems()
{	
	Init();
	//CoInitialize(0);

	IITLibraryPlaylist *_IITLibraryPlaylist = 0;
	IITTrackCollection *_IITTrackCollection= 0;

	m_iITunes->get_LibraryPlaylist(&_IITLibraryPlaylist);

	if(_IITLibraryPlaylist)
	{
		_IITLibraryPlaylist->get_Tracks(&_IITTrackCollection);
 
		long count;

		_IITTrackCollection->get_Count(&count);

	    HRESULT hr;

		for(long i = count ; i >0;i--)
		{		
				CComPtr<IITTrack>  spTrack;          
           
				hr=_IITTrackCollection->get_Item (i,&spTrack);
 
				CComPtr<IITFileOrCDTrack> spFileTrack;

				 hr=spTrack->QueryInterface (__uuidof(spFileTrack),(void **)&spFileTrack);

				 if(SUCCEEDED(hr))
				 {					
					 GetTrackInfo(spFileTrack );
				 }
				 spFileTrack.Release();

				 spTrack.Release();
		}
	}

	SAFE_RELEASE(_IITTrackCollection);
	SAFE_RELEASE(_IITLibraryPlaylist);

	ReleaseiTunes();
}

HRESULT CiTunes::SaveiTunesArt(CComPtr<IITFileOrCDTrack> inTrack, CString csSavePath)
{
		HRESULT hr;
		CComPtr<IITArtworkCollection> spAlbumArtCollection;
		CComPtr<IITArtwork>  spAlbumArt; 
		CComBSTR bstrAlbumArtPath;
		CComBSTR bstrName;
		ITArtworkFormat artFormat;
 

		//Get the ArtWork
		hr=inTrack->get_Name (&bstrName);
		hr=inTrack->get_Artwork (&spAlbumArtCollection);
		hr=spAlbumArtCollection->get_Item (1,&spAlbumArt);

		USES_CONVERSION;
           
        if(SUCCEEDED(hr) && hr!=S_FALSE)
        {
            hr=spAlbumArt->get_Format (&artFormat);
               
            if(SUCCEEDED(hr))
            {
            /*     CString strTemp=OLE2T(bstrName);
				strTemp.TrimLeft("\"");
                strTemp.TrimRight ("\"");*/

                //sprintf(szAlbumArtPath,  "%s%s%s%d%s",m_szCurDir,"\\",strTemp.GetBuffer (),k,".");
                   
                //switch(artFormat)
                //{
                //case ITArtworkFormatJPEG:
                //    //strcat(szAlbumArtPath,"jpg");
                //    bstrAlbumArtPath=T2OLE((LPTSTR)(LPCTSTR)csSavePath);
                //    hr=spAlbumArt->SaveArtworkToFile (bstrAlbumArtPath);
                //   break;
                //case ITArtworkFormatPNG :
                //    //strcat(szAlbumArtPath,"png");
                //    bstrAlbumArtPath=T2OLE((LPTSTR)(LPCTSTR)csSavePath);
                //    spAlbumArt->SaveArtworkToFile (bstrAlbumArtPath);
                //    break;
                //case ITArtworkFormatBMP:
                //    //strcat(szAlbumArtPath,"bmp");
                //    bstrAlbumArtPath=T2OLE((LPTSTR)(LPCTSTR)csSavePath);
                //    spAlbumArt->SaveArtworkToFile (bstrAlbumArtPath);
                //    break;
                //default:
                //    break;
                //}
				    bstrAlbumArtPath=T2OLE((LPTSTR)(LPCTSTR)csSavePath);//always save to .png
                    hr=spAlbumArt->SaveArtworkToFile (bstrAlbumArtPath);
            }
        }
		return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
std::string CiTunes::GetCurrentSongTitle_iTunes()
{
	Init();

	using std::string;
	using std::wstring;
 
	IITTrack *iITrack = 0;

	// String operations done in a wstring, then converted for return
	wstring wstrRet;
	string strRet;
 
	try
	{
 		BSTR bstrURL = 0;
 
		if(m_iITunes)
		{
			ITPlayerState iIPlayerState;

			m_iITunes->get_CurrentTrack(&iITrack);
			m_iITunes->get_CurrentStreamURL((BSTR *)&bstrURL);	

			if(iITrack)
			{
				BSTR bstrTrack = 0;

				iITrack->get_Name((BSTR *)&bstrTrack);
				
				// Add song title
				if(bstrTrack)
					wstrRet += bstrTrack;

				iITrack->Release();
			}
			else
			{
				// Couldn't get track name
			}

			// Add url, if present
			if(bstrURL)
			{
				wstrRet += L" ";
				wstrRet += bstrURL;
			}

			m_iITunes->get_PlayerState(&iIPlayerState);

			// Add player state, if special
			switch(iIPlayerState)
			{
			case ITPlayerStatePlaying:
			default:
				break;
			case ITPlayerStateStopped:
				wstrRet += L" [stopped]";
				break;
			case ITPlayerStateFastForward:
				wstrRet += L" [fast]";
				break;
			case ITPlayerStateRewind:
				wstrRet += L" [rewind]";
				break;
			}

			/*m_iITunes->Release();*/			
		}
		else
		{
			// iTunes interface not found/failed
			wstrRet = L"";
		}

	}
	catch(...)
	{
 
			SAFE_RELEASE(iITrack);
			ReleaseiTunes();
	}
 

	// Convert the result from wstring to string
	size_t len = wstrRet.size();
	strRet.resize(len);
	for(size_t i = 0; i < len; i++)
		strRet[i] = static_cast<char>(wstrRet[i]);

	return strRet;
}
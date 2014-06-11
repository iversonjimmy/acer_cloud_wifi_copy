#include "StdAfx.h"
#include "CWMP.h"


CWMP::CWMP(void)
{
	m_spPlayer = NULL;
	m_litem = NULL;
}
 
CWMP::~CWMP(void)
{
	ReleaseWMPLibrary();
	if(m_litem!= NULL)
	{
		m_litem->clear();
	
		SAFE_DELETE(m_litem);	
	}
}

void CWMP::InitWMP()
{

	if(!m_spPlayer)
	{
		HRESULT hr;
    
		hr = m_spPlayer.CoCreateInstance( __uuidof(WindowsMediaPlayer), 0, CLSCTX_INPROC_SERVER );

	//if(SUCCEEDED(hr))
	//{
	//	//Get Service
	//	hr = m_spPlayer->QueryInterface(&m_spLibSvc);
	//}
	}
}

void CWMP::ReleaseWMPLibrary()
{
	if (m_spPlayer)
	{
		m_spPlayer.Release();
		m_spPlayer = NULL;
	}
}

void CWMP::GetAllList(TContentMap* inlitem)
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
	ReleaseWMPLibrary();
 
}

void CWMP::GetMediaItems()
{
	InitWMP();
	GetWMPitemFromList(MEDIA_TYPE_photo);
	GetWMPitemFromList(MEDIA_TYPE_audio);
	GetWMPitemFromList(MEDIA_TYPE_video);
}

void CWMP::GetPlaylist()
{
	InitWMP();
	GetWMPitemFromList(MEDIA_TYPE_playlist);
}

void CWMP::GetWMPitemFromList(MEDIA_TYPE inMediatype )
{
	HRESULT hr;
	//Get Collection
	CComPtr<IWMPMediaCollection> spCollection;

	hr = m_spPlayer->get_mediaCollection(&spCollection);

	if(SUCCEEDED(hr))
	{
		//Get PlayList
		CComPtr<IWMPPlaylist> spPlayList;
		CComBSTR bstrTypeValue;
			
		if (MEDIA_TYPE_photo == inMediatype)
			bstrTypeValue = _T("photo");
		else if (MEDIA_TYPE_audio == inMediatype)
			bstrTypeValue = _T("audio");
		else if (MEDIA_TYPE_video == inMediatype)
			bstrTypeValue = _T("video");
		else if (MEDIA_TYPE_playlist == inMediatype)
			bstrTypeValue = _T("playlist");

		hr = spCollection->getByAttribute(_T("MediaType"), bstrTypeValue, &spPlayList);

		if(SUCCEEDED(hr))
		{
			GetWMPitemFromList(spPlayList,MEDIA_TYPE_playlist == inMediatype);
		}
	}
}

void CWMP::GetWMPitemFromList(CComPtr<IWMPPlaylist> inlist, bool bIsPlaylist)
{		
		HRESULT hr;
		long nCount = 0;
		inlist->get_count(&nCount);
 
		for(int i=0;i<nCount;i++)
		{
			//Get Item
			CComPtr<IWMPMedia> spItem;

			hr = inlist->get_item(i, &spItem);

			if(SUCCEEDED(hr))
			{
				CLOUD_MEIDA_METADATA_STRUCT* cmd = new CLOUD_MEIDA_METADATA_STRUCT();
				InitMetadataStruct(cmd);

				CComBSTR bstrLocation = _T(""), bstrID = _T(""),bstrName = _T("") ;

				spItem->getItemInfo(_T("SourceURL"), &bstrLocation);

				spItem->getItemInfo(_T("TrackingID"), &bstrID);
				
				spItem->get_name(&bstrName);

				if(!PathFileExists(bstrLocation))
				{
					spItem.Release();
					continue;
				}
				if(!bIsPlaylist)
				{
	#pragma region !bIsPlaylist

					bool bIsAudio = false;
					bool bIsPhoto = false;
					bool bIsVideo = false;
 
				GetFileInfo(cmd,OLE2T(bstrLocation), &bIsAudio,&bIsPhoto,&bIsVideo);

				if(!bIsAudio && !bIsPhoto && !bIsVideo)
					return;

				cmd->Source = cmd->DeviceID = PLAYER_TYPE_WMP;
				cmd->Location = OLE2T(bstrLocation); 
				cmd->UID = cmd->ContentID = GetGUID(bstrID);
				cmd->MetadataType = _Metadata_Item;
				cmd->MediaType = bIsAudio?MEDIA_TYPE_audio:(bIsPhoto?MEDIA_TYPE_photo:MEDIA_TYPE_video);

				BOOL bHadThumb = GetThumbNail(GetDC(NULL),cmd->Location,GetThumbnailURL(cmd->Title));


				if(bIsAudio /*&& m_lAlbumName.Find(cmd->Album) == NULL*/)
				{
					//m_lAlbumName.AddHead(cmd->Album);
					if(!cmd->Album.IsEmpty())
					{
						bool bAdded = false;
						CString tempAlbumGUID = GetAlbumGUID(cmd->Album,PLAYER_TYPE_WMP,m_litem,&bAdded);
						if(!bAdded)
						{			
							CLOUD_MEIDA_METADATA_STRUCT* cmdAlbum = new CLOUD_MEIDA_METADATA_STRUCT();
							InitMetadataStruct(cmdAlbum);

							cmdAlbum->UID = cmd->AlbumID = tempAlbumGUID;
							cmdAlbum->Source =  PLAYER_TYPE_WMP;
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

	#pragma endregion !bIsPlaylist
				}
				else
				{
	#pragma region bIsPlaylist
					if(!PathMatchSpec(bstrLocation,_T("Sync Playlists")))
					{		 							

						CComPtr<IWMPPlaylist> inlist1;
						//Get Collection
						CComPtr<IWMPMediaCollection> spCollection = NULL;

						hr = m_spPlayer->get_mediaCollection(&spCollection);
						if(SUCCEEDED(hr))
						{
							spCollection->getByName(bstrName,&inlist1);
							long nCount1 = 0;
							inlist1->get_count(&nCount1);

							for(int j = 0 ; j < (int)nCount1;j++)
							{
								CComBSTR bstrID1 = _T("");
								CComPtr<IWMPMedia> spItem1 = NULL;
								hr = inlist1->get_item(j, &spItem1);
								spItem1->getItemInfo(_T("TrackingID"), &bstrID1);
								if(SUCCEEDED(hr))
								{
									CString csID = bstrID1;
									cmd->ContentIDs.AddTail(csID);
 
								}
								if(spItem1!= NULL)
									spItem1.Release();
							}
							if(nCount1 != 0)
							{
								cmd->Count = (int)nCount1;
								cmd->PlaylistID = cmd->UID = bstrID;
								cmd->PlaylistName = bstrName;
								cmd->Source = PLAYER_TYPE_WMP;
								cmd->MetadataType = _Metadata_Playlist;
								cmd->MediaType = MEDIA_TYPE_audio;

								m_litem->insert(std::make_pair(cmd->UID,cmd));
							}
						}
						if(spCollection != NULL)
							spCollection.Release();
					}
	#pragma endregion bIsPlaylist
				}
			spItem.Release();
			}
		}
				
 
}
